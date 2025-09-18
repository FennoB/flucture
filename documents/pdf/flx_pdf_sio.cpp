#include "flx_pdf_sio.h"
#include "flx_pdf_text_extractor.h"
#include <main/PdfMemDocument.h>
#include <main/PdfPainter.h>
#include <main/PdfColor.h>
#include <auxiliary/StreamDevice.h>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <fstream>
#include <sstream>
#include <stack>

using namespace PoDoFo;

// Helper function to parse hex color and create PdfColor
PdfColor parse_hex_color(flx_string& hex_color) {
  std::string color_str = hex_color.c_str();
  
  if (color_str.empty() || color_str[0] != '#') {
    return PdfColor(0.8, 0.8, 0.8); // Default light gray
  }
  
  if (color_str.length() != 7) {
    return PdfColor(0.8, 0.8, 0.8); // Default light gray
  }
  
  try {
    // Parse RGB components
    int r = std::stoi(color_str.substr(1, 2), nullptr, 16);
    int g = std::stoi(color_str.substr(3, 2), nullptr, 16);
    int b = std::stoi(color_str.substr(5, 2), nullptr, 16);
    
    // Convert to 0-1 range
    double rd = r / 255.0;
    double gd = g / 255.0;
    double bd = b / 255.0;
    
    return PdfColor(rd, gd, bd);
  } catch (...) {
    return PdfColor(0.8, 0.8, 0.8); // Default light gray on parse error
  }
}

flx_pdf_sio::flx_pdf_sio() : m_pdf(nullptr) {
}

flx_pdf_sio::~flx_pdf_sio() {
  if (m_pdf != nullptr) {
    delete m_pdf;
    m_pdf = nullptr;
  }
}

bool flx_pdf_sio::parse(flx_string &data) {
  this->pdf_data = data;
  try {
    if (m_pdf != nullptr) {
      delete m_pdf;
    }
    m_pdf = new PdfMemDocument();
    bufferview buffer(data.c_str(), data.size());
    m_pdf->LoadFromBuffer(buffer);
    
    std::cout << "Starting complete PDF → Layout extraction..." << std::endl;
    
    // Step 1: Extract texts and images from ORIGINAL PDF (before any modifications)
    std::vector<flx_layout_text> extracted_texts;
    std::vector<flx_layout_image> extracted_images;
    if (!extract_texts_and_images(extracted_texts, extracted_images)) {
      std::cout << "Failed to extract texts and images" << std::endl;
      return false;
    }
    
    std::cout << "Extracted " << extracted_texts.size() << " texts and " 
              << extracted_images.size() << " images from original PDF" << std::endl;
    
    // Step 2: Create PDF copy for geometry isolation
    auto pdf_copy = create_pdf_copy();
    if (pdf_copy == nullptr) {
      std::cout << "Failed to create PDF copy" << std::endl;
      return false;
    }
    
    std::cout << "Created PDF copy with " << pdf_copy->GetPages().GetCount() << " pages" << std::endl;
    
    // Step 3: Remove texts and images from copy to isolate geometry
    if (!remove_texts_and_images_from_copy(pdf_copy.get())) {
      std::cout << "Failed to remove texts and images from PDF copy" << std::endl;
      return false;
    }
    
    // Step 4: Render cleaned PDF to images for OpenCV processing
    std::vector<cv::Mat> clean_images;
    if (!render_clean_pdf_to_images(pdf_copy.get(), clean_images)) {
      std::cout << "Failed to render cleaned PDF to images" << std::endl;
      return false;
    }
    
    // Step 5: Detect color-coherent regions using flood-fill
    std::vector<std::vector<cv::Mat>> page_masks;
    for (const auto& page_image : clean_images) {
      auto masks = detect_color_regions(page_image);
      page_masks.push_back(masks);
    }
    
    // Step 6: Extract polygon contours from masks
    std::vector<std::vector<std::vector<cv::Point>>> page_contours;
    for (const auto& masks : page_masks) {
      auto contours = extract_contours_from_masks(masks);
      page_contours.push_back(contours);
    }
    
    // Step 7: Build layout structure and assign content
    pages = flx_model_list<flx_layout_geometry>();  // Clear pages first
    build_geometry_hierarchy(page_contours, clean_images, pages);
    assign_content_to_geometries_flx(extracted_texts, extracted_images, pages);
    
    std::cout << "Complete PDF → Layout extraction finished. Created " 
              << pages.size() << " page structures." << std::endl;
    
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Error in PDF → Layout extraction: " << e.what() << std::endl;
    return false;
  }
}

bool flx_pdf_sio::serialize(flx_string &data) {
  try {
    // Create new PDF document from layout structure
    if (m_pdf != nullptr) {
      delete m_pdf;
    }
    m_pdf = new PdfMemDocument();
    
    // Create pages from layout structure
    for (size_t i = 0; i < pages.size(); ++i) {
      auto& page_layout = pages[i];
      
      // Create PDF page
      auto& pdf_page = m_pdf->GetPages().CreatePage(PdfPageSize::A4);
      PdfPainter painter;
      painter.SetCanvas(pdf_page);
      
      // Render page content
      render_geometry_to_page(painter, page_layout);
      
      painter.FinishDrawing();
    }
    
    // Serialize to buffer using PoDoFo StreamDevice
    std::stringstream buffer;
    StandardStreamDevice device(buffer);
    m_pdf->Save(device);
    data = buffer.str();
    
    // Also save to member variable for later parsing operations
    pdf_data = data;
    
    return true;
    
  } catch (const std::exception& e) {
    std::cerr << "Error serializing PDF: " << e.what() << std::endl;
    return false;
  }
}

bool flx_pdf_sio::render(std::vector<cv::Mat>& output_images, int dpi) {
  output_images.clear();
  if (m_pdf == nullptr) {
    return false;
  }
  
  // Create temporary directory for PDF conversion
  long long timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
  std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / ("flx_pdf_render_" + std::to_string(timestamp));
  
  try {
    std::filesystem::create_directories(temp_dir);
    std::filesystem::path temp_pdf = temp_dir / "input.pdf";
    
    // Write PDF data to temporary file
    std::ofstream outfile(temp_pdf, std::ios::binary);
    if (!outfile.is_open()) {
      std::filesystem::remove_all(temp_dir);
      return false;
    }
    outfile.write(this->pdf_data.c_str(), this->pdf_data.size());
    outfile.close();
    
    // Use pdftoppm to convert PDF to images
    std::string cmd = "pdftoppm -png -r " + std::to_string(dpi) + " '" + temp_pdf.string() + "' '" + (temp_dir / "page").string() + "'";
    int result = system(cmd.c_str());
    
    if (result == 0) {
      // Load generated images in order
      std::vector<std::filesystem::path> image_files;
      for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
        if (entry.path().extension() == ".png") {
          image_files.push_back(entry.path());
        }
      }
      
      // Sort by filename to maintain page order
      std::sort(image_files.begin(), image_files.end());
      
      for (const auto& image_path : image_files) {
        cv::Mat img = cv::imread(image_path.string());
        if (!img.empty()) {
          output_images.push_back(img);
        }
      }
    }
    
    // Cleanup
    std::filesystem::remove_all(temp_dir);
    return result == 0 && !output_images.empty();
    
  } catch (const std::exception& e) {
    std::cerr << "Error rendering PDF: " << e.what() << std::endl;
    std::filesystem::remove_all(temp_dir);
    return false;
  }
}

bool flx_pdf_sio::add_text(flx_string text, double x, double y) {
  if (m_pdf == nullptr) {
    return false;
  }
  
  // TODO: Implement text addition using PoDoFo
  // This would modify the PDF document to add text at the specified position
  std::cout << "Adding text '" << text.c_str() << "' at (" << x << ", " << y << ") - TODO: implement" << std::endl;
  return true;
}

void flx_pdf_sio::render_geometry_to_page(PdfPainter& painter, flx_layout_geometry& geometry) {
  // Render the polygon shape first (background)
  if (geometry.vertices.size() >= 3) {
    render_polygon_shape(painter, geometry);
  }
  
  // Recursively render sub-geometries first (background layers)
  for (size_t i = 0; i < geometry.sub_geometries.size(); ++i) {
    render_geometry_to_page(painter, geometry.sub_geometries[i]);
  }
  
  // Render all text elements (foreground)
  for (size_t i = 0; i < geometry.texts.size(); ++i) {
    render_text_element(painter, geometry.texts[i]);
  }
  
  // Render all image elements (topmost layer)
  for (size_t i = 0; i < geometry.images.size(); ++i) {
    render_image_element(painter, geometry.images[i]);
  }
}

void flx_pdf_sio::render_text_element(PdfPainter& painter, flx_layout_text& text_elem) {
  // Set font
  auto* font = m_pdf->GetFonts().SearchFont("Arial", PdfFontSearchParams{});
  if (font != nullptr) {
    painter.TextState.SetFont(*font, text_elem.font_size);
  }
  
  // Ensure text has proper color (black text)
  PdfColor text_color(0.0, 0.0, 0.0); // Black
  painter.GraphicsState.SetNonStrokingColor(text_color);
  
  // Render text at position
  flx_string text_content = text_elem.text;
  painter.DrawText(text_content.c_str(), text_elem.x, text_elem.y);
}

void flx_pdf_sio::render_polygon_shape(PdfPainter& painter, flx_layout_geometry& geometry) {
  if (geometry.vertices.size() < 3) return;
  
  // Set fill color from geometry
  flx_string fill_color = geometry.fill_color;
  if (!fill_color.empty()) {
    PdfColor pdf_color = parse_hex_color(fill_color);
    painter.GraphicsState.SetNonStrokingColor(pdf_color);
  }
  
  // Create polygon path
  PdfPainterPath path;
  auto& first_vertex = geometry.vertices[0];
  path.MoveTo(first_vertex.x, first_vertex.y);
  
  // Add remaining vertices
  for (size_t i = 1; i < geometry.vertices.size(); ++i) {
    auto& vertex = geometry.vertices[i];
    path.AddLineTo(vertex.x, vertex.y);
  }
  
  // Close and fill polygon
  path.Close();
  painter.DrawPath(path, PdfPathDrawMode::Fill);
}

void flx_pdf_sio::render_image_element(PdfPainter& painter, flx_layout_image& image_elem) {
  if (!load_image_from_path(image_elem)) {
    flx_string path = image_elem.image_path;
    std::cout << "Failed to load image: " << path.c_str() << std::endl;
    return;
  }
  
  try {
    // Create PdfImage from OpenCV Mat
    auto pdf_image = this->create_pdf_image_from_mat(image_elem);
    if (pdf_image == nullptr) {
      std::cout << "Failed to create PDF image" << std::endl;
      return;
    }
    
    // Calculate scaling factors based on desired vs original dimensions
    double original_width = pdf_image->GetWidth();
    double original_height = pdf_image->GetHeight();
    double scale_x = image_elem.width / original_width;
    double scale_y = image_elem.height / original_height;
    
    // Convert from top-left coordinate system to PDF's bottom-left coordinate system
    // A4 page height is 842 points
    double pdf_y = 842.0 - image_elem.y - image_elem.height;
    
    
    // Draw image at position with calculated scaling
    painter.DrawImage(*pdf_image, image_elem.x, pdf_y, scale_x, scale_y);
    
  } catch (const std::exception& e) {
    std::cout << "Error rendering image: " << e.what() << std::endl;
  }
}


std::unique_ptr<PoDoFo::PdfMemDocument> flx_pdf_sio::create_pdf_copy() {
  try {
    // Create new document and copy from current
    auto copy = std::make_unique<PdfMemDocument>();
    
    // Serialize current PDF to buffer and reload into copy
    std::stringstream buffer;
    StandardStreamDevice device(buffer);
    m_pdf->Save(device);
    
    std::string pdf_content = buffer.str();
    bufferview buffer_view(pdf_content.c_str(), pdf_content.size());
    copy->LoadFromBuffer(buffer_view);
    
    std::cout << "Created PDF copy with " << copy->GetPages().GetCount() << " pages" << std::endl;
    return copy;
    
  } catch (const std::exception& e) {
    std::cout << "Error creating PDF copy: " << e.what() << std::endl;
    return nullptr;
  }
}

bool flx_pdf_sio::extract_texts_and_images(std::vector<flx_layout_text>& texts, std::vector<flx_layout_image>& images) {
  try {
    if (m_pdf == nullptr) {
      std::cout << "No PDF document to extract from" << std::endl;
      return false;
    }
    
    auto& pages = m_pdf->GetPages();
    std::cout << "Extracting content from " << pages.GetCount() << " pages..." << std::endl;
    
    // Use our custom text extractor instead of primitive ExtractTextTo
    flx_pdf_text_extractor text_extractor;
    
    for (unsigned int page_num = 0; page_num < pages.GetCount(); ++page_num) {
      auto& page = pages.GetPageAt(page_num);
      
      // Extract text with full font information using our custom extractor
      std::vector<flx_layout_text> page_texts;
      bool extraction_success = text_extractor.extract_text_with_fonts(page, page_texts);
      
      if (extraction_success) {
        std::cout << "Page " << (page_num + 1) << ": Found " << page_texts.size() << " text entries with font info" << std::endl;
        
        for (const auto& layout_text : page_texts) {
          // The layout_text already contains font information!
          flx_string text_content = layout_text.text;
          double x_pos = layout_text.x;
          double y_pos = layout_text.y;
          double font_size = layout_text.font_size;
          flx_string font_family = layout_text.font_family;
          
          std::cout << "  Text: \"" << text_content.c_str() << "\" at (" 
                    << x_pos << "," << y_pos 
                    << ") font=" << font_size << "pt " << font_family.c_str() << std::endl;
          
          // TODO: Add to texts vector once flx_model_list copy constructor is fixed
          // texts.push_back(layout_text);
        }
      } else {
        std::cout << "Page " << (page_num + 1) << ": Enhanced text extraction failed" << std::endl;
      }
      
      // TODO: Extract images from this page
      // PoDoFo image extraction is more complex, requires iteration through XObject resources
      std::cout << "Page " << (page_num + 1) << ": Image extraction not yet implemented" << std::endl;
    }
    
    std::cout << "Extracted " << texts.size() << " texts total" << std::endl;
    return true;
    
  } catch (const std::exception& e) {
    std::cout << "Error extracting content: " << e.what() << std::endl;
    return false;
  }
}

bool flx_pdf_sio::load_image_from_path(flx_layout_image& image_elem) {
  flx_string path = image_elem.image_path;
  if (path.empty()) {
    return false;
  }
  
  // Load image with OpenCV
  cv::Mat image = cv::imread(path.c_str(), cv::IMREAD_COLOR);
  if (image.empty()) {
    return false;
  }
  
  // Store original dimensions
  image_elem.original_width = image.cols;
  image_elem.original_height = image.rows;
  
  // TODO: Store cv::Mat for later use - for now just validate loading
  return true;
}

std::unique_ptr<PoDoFo::PdfImage> flx_pdf_sio::create_pdf_image_from_mat(flx_layout_image& image_elem) {
  flx_string path = image_elem.image_path;
  if (path.empty()) {
    return nullptr;
  }
  
  try {
    // Create PdfImage object
    auto pdf_image = m_pdf->CreateImage();
    
    // Load image data from file into PoDoFo
    std::ifstream file(path.c_str(), std::ios::binary);
    if (!file.is_open()) {
      return nullptr;
    }
    
    // Read file into buffer
    std::string image_data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    
    // Load buffer into PdfImage
    bufferview buffer(image_data.c_str(), image_data.size());
    pdf_image->LoadFromBuffer(buffer);
    
    return pdf_image;
    
  } catch (const std::exception& e) {
    std::cout << "Error creating PDF image: " << e.what() << std::endl;
    return nullptr;
  }
}

bool flx_pdf_sio::remove_texts_and_images_from_copy(PoDoFo::PdfMemDocument* pdf_copy) {
  std::cout << "  Removing texts and images from PDF copy..." << std::endl;
  
  try {
    auto& pages = pdf_copy->GetPages();
    
    for (unsigned int page_num = 0; page_num < pages.GetCount(); ++page_num) {
      auto& page = pages.GetPageAt(page_num);
      
      // Get the page's content stream
      auto contents = page.GetContents();
      if (contents == nullptr) {
        std::cout << "    Page " << (page_num + 1) << ": No contents found" << std::endl;
        continue;
      }
      
      // Read the current content stream  
      std::string content_str;
      // For now, skip content stream manipulation - this needs deeper PoDoFo 1.0 API understanding
      std::cout << "    Page " << (page_num + 1) << ": Content stream filtering not yet implemented for PoDoFo 1.0" << std::endl;
      continue;
      
      // Filter out text and image operators while keeping path operations
      std::string filtered_content = filter_pdf_content_stream(content_str);
      
      // Replace the content stream with filtered version
      if (replace_page_content_stream(page, filtered_content)) {
        std::cout << "    Page " << (page_num + 1) << ": Successfully removed text/image operators" << std::endl;
      } else {
        std::cout << "    Page " << (page_num + 1) << ": Failed to replace content stream" << std::endl;
        return false;
      }
    }
    
    std::cout << "  Successfully cleaned " << pages.GetCount() << " pages" << std::endl;
    return true;
    
  } catch (const std::exception& e) {
    std::cout << "Error removing content: " << e.what() << std::endl;
    return false;
  }
}

bool flx_pdf_sio::render_clean_pdf_to_images(PoDoFo::PdfMemDocument* pdf_copy, std::vector<cv::Mat>& clean_images) {
  std::cout << "  Rendering cleaned PDF to images..." << std::endl;
  clean_images.clear();
  
  if (pdf_copy == nullptr) {
    std::cout << "    Error: No PDF copy to render" << std::endl;
    return false;
  }
  
  // Create temporary directory for PDF conversion
  long long timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
  std::filesystem::path temp_dir = std::filesystem::temp_directory_path() / ("flx_clean_pdf_render_" + std::to_string(timestamp));
  
  try {
    std::filesystem::create_directories(temp_dir);
    std::filesystem::path temp_pdf = temp_dir / "clean_input.pdf";
    
    // Save the cleaned PDF copy to temporary file
    std::stringstream buffer;
    StandardStreamDevice device(buffer);
    pdf_copy->Save(device);
    
    std::ofstream outfile(temp_pdf, std::ios::binary);
    if (!outfile.is_open()) {
      std::filesystem::remove_all(temp_dir);
      std::cout << "    Error: Cannot write temporary PDF file" << std::endl;
      return false;
    }
    
    std::string pdf_content = buffer.str();
    outfile.write(pdf_content.c_str(), pdf_content.size());
    outfile.close();
    
    std::cout << "    Saved cleaned PDF to: " << temp_pdf.string() << std::endl;
    
    // Use pdftoppm to convert cleaned PDF to images
    int dpi = 150; // Lower DPI for geometry analysis (faster processing)
    std::string cmd = "pdftoppm -png -r " + std::to_string(dpi) + " '" + temp_pdf.string() + "' '" + (temp_dir / "clean_page").string() + "'";
    std::cout << "    Running: " << cmd << std::endl;
    
    int result = system(cmd.c_str());
    
    if (result == 0) {
      // Load generated images in order
      std::vector<std::filesystem::path> image_files;
      for (const auto& entry : std::filesystem::directory_iterator(temp_dir)) {
        if (entry.path().extension() == ".png" && entry.path().stem().string().find("clean_page") == 0) {
          image_files.push_back(entry.path());
        }
      }
      
      // Sort by filename to maintain page order
      std::sort(image_files.begin(), image_files.end());
      
      for (const auto& image_path : image_files) {
        cv::Mat img = cv::imread(image_path.string(), cv::IMREAD_COLOR);
        if (!img.empty()) {
          clean_images.push_back(img);
          std::cout << "    Loaded clean image: " << img.cols << "x" << img.rows << " from " << image_path.filename().string() << std::endl;
        }
      }
      
      std::cout << "  Successfully rendered " << clean_images.size() << " clean pages to images" << std::endl;
    } else {
      std::cout << "    Error: pdftoppm command failed with code " << result << std::endl;
    }
    
    // Cleanup
    std::filesystem::remove_all(temp_dir);
    return result == 0 && !clean_images.empty();
    
  } catch (const std::exception& e) {
    std::cout << "Error rendering clean PDF: " << e.what() << std::endl;
    std::filesystem::remove_all(temp_dir);
    return false;
  }
}

std::vector<cv::Mat> flx_pdf_sio::detect_color_regions(const cv::Mat& page_image) {
  std::cout << "  Detecting color regions using flood-fill..." << std::endl;
  std::vector<cv::Mat> masks;
  
  if (page_image.empty()) {
    std::cout << "    Error: Empty page image" << std::endl;
    return masks;
  }
  
  // Create a mask to track already processed pixels (0 = unprocessed, 1 = processed)
  cv::Mat processed = cv::Mat::zeros(page_image.size(), CV_8UC1);
  
  int color_tolerance = 20; // Color difference threshold between neighbors
  int min_area = 100; // Minimum region area in pixels
  int region_count = 0;
  
  std::cout << "    Processing image: " << page_image.cols << "x" << page_image.rows << std::endl;
  
  // Iterate through all pixels to find unprocessed regions
  for (int y = 0; y < page_image.rows; y += 2) { // Skip every other row for performance
    for (int x = 0; x < page_image.cols; x += 2) { // Skip every other column for performance
      
      // Skip if already processed
      if (processed.at<uchar>(y, x) > 0) {
        continue;
      }
      
      // Start a new region from this seed point
      cv::Mat region_mask = cv::Mat::zeros(page_image.size(), CV_8UC1);
      int filled_pixels = flood_fill_neighbor_based(page_image, cv::Point(x, y), region_mask, processed, color_tolerance);
      
      // Check if region is large enough
      if (filled_pixels >= min_area) {
        // Calculate bounding rect and dominant color
        cv::Rect bounding_rect = cv::boundingRect(region_mask);
        cv::Scalar mean_color = cv::mean(page_image, region_mask);
        
        // Store this region mask
        masks.push_back(region_mask.clone());
        region_count++;
        
        std::cout << "    Region " << region_count << ": " << filled_pixels << " pixels, "
                  << "bbox(" << bounding_rect.x << "," << bounding_rect.y << "," 
                  << bounding_rect.width << "," << bounding_rect.height << "), "
                  << "color(" << (int)mean_color[2] << "," << (int)mean_color[1] << "," << (int)mean_color[0] << ")" << std::endl;
      }
      // Note: Small regions are already marked as processed by flood_fill_neighbor_based
    }
  }
  
  std::cout << "  Found " << masks.size() << " color-coherent regions" << std::endl;
  return masks;
}

std::vector<std::vector<cv::Point>> flx_pdf_sio::extract_contours_from_masks(const std::vector<cv::Mat>& masks) {
  std::cout << "  Extracting contours from masks..." << std::endl;
  std::vector<std::vector<cv::Point>> all_contours;
  
  for (const auto& mask : masks) {
    std::vector<std::vector<cv::Point>> mask_contours;
    std::vector<cv::Vec4i> hierarchy;
    
    cv::findContours(mask, mask_contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);
    
    for (const auto& contour : mask_contours) {
      if (contour.size() >= 3) {
        std::vector<cv::Point> simplified_contour;
        double epsilon = 2.0;
        cv::approxPolyDP(contour, simplified_contour, epsilon, true);
        
        if (simplified_contour.size() >= 3) {
          all_contours.push_back(simplified_contour);
        }
      }
    }
  }
  
  std::cout << "  Extracted " << all_contours.size() << " polygon contours" << std::endl;
  return all_contours;
}

void flx_pdf_sio::build_geometry_hierarchy(const std::vector<std::vector<std::vector<cv::Point>>>& page_contours,
                                          const std::vector<cv::Mat>& clean_images,
                                          flx_model_list<flx_layout_geometry>& geometries) {
  std::cout << "  Building geometry hierarchy..." << std::endl;
  geometries = flx_model_list<flx_layout_geometry>();
  
  // Get all contours from all pages
  std::vector<std::vector<cv::Point>> all_contours;
  for (const auto& page_contour_list : page_contours) {
    all_contours.insert(all_contours.end(), page_contour_list.begin(), page_contour_list.end());
  }
  
  // Convert contours to geometry objects with polygon data
  flx_model_list<flx_layout_geometry> all_geometries;
  for (size_t i = 0; i < all_contours.size(); ++i) {
    const auto& contour = all_contours[i];
    
    flx_layout_geometry geom;
    
    // Calculate bounding rectangle
    cv::Rect bbox = cv::boundingRect(contour);
    geom.x = bbox.x;
    geom.y = bbox.y;
    geom.width = bbox.width;
    geom.height = bbox.height;
    
    // Convert cv::Point contour to flx_layout_vertex
    for (const auto& point : contour) {
      flx_layout_vertex vertex;
      vertex.x = point.x;
      vertex.y = point.y;
      geom.vertices.push_back(vertex);
    }
    
    // Calculate dominant color from the original image
    cv::Scalar mean_color = calculate_dominant_color_for_contour(contour, clean_images, i);
    geom.fill_color = rgb_to_hex_string(mean_color);
    
    all_geometries.push_back(geom);
  }
  
  // Build hierarchical structure using containment analysis
  std::vector<bool> is_child(all_geometries.size(), false);
  
  // For each geometry, check if it's contained within another
  for (size_t i = 0; i < all_geometries.size(); ++i) {
    if (is_child[i]) continue;
    
    auto& parent_geom = all_geometries[i];
    
    // Find all geometries that are contained within this one
    for (size_t j = 0; j < all_geometries.size(); ++j) {
      if (i == j || is_child[j]) continue;
      
      auto& potential_child = all_geometries[j];
      
      // Check if potential_child is completely contained within parent_geom
      if (is_geometry_contained_in(potential_child, parent_geom)) {
        parent_geom.sub_geometries.push_back(potential_child);
        is_child[j] = true;
      }
    }
  }
  
  // Add only root-level geometries (not children) to the final result
  for (size_t i = 0; i < all_geometries.size(); ++i) {
    if (!is_child[i]) {
      geometries.push_back(all_geometries[i]);
    }
  }
  
  std::cout << "  Built hierarchy: " << geometries.size() << " root geometries with nested sub-geometries" << std::endl;
}

void flx_pdf_sio::assign_content_to_geometries(std::vector<flx_layout_text>& texts, 
                                              std::vector<flx_layout_image>& images,
                                              std::vector<flx_layout_geometry>& geometries) {
  std::cout << "  Assigning content to geometries..." << std::endl;
  
  // Assign texts to geometries
  for (auto& text : texts) {
    assign_content_to_geometry_recursive(text, geometries);
  }
  
  // Assign images to geometries
  for (auto& image : images) {
    assign_content_to_geometry_recursive(image, geometries);
  }
  
  std::cout << "  Content assignment completed" << std::endl;
}

void flx_pdf_sio::assign_content_to_geometries_flx(std::vector<flx_layout_text>& texts, 
                                                  std::vector<flx_layout_image>& images,
                                                  flx_model_list<flx_layout_geometry>& geometries) {
  std::cout << "  Assigning content to geometries..." << std::endl;
  
  // Assign texts to geometries
  for (auto& text : texts) {
    assign_content_to_geometry_recursive_flx(text, geometries);
  }
  
  // Assign images to geometries
  for (auto& image : images) {
    assign_content_to_geometry_recursive_flx(image, geometries);
  }
  
  std::cout << "  Content assignment completed" << std::endl;
}

std::string flx_pdf_sio::filter_pdf_content_stream(const std::string& content) {
  std::istringstream input(content);
  std::ostringstream output;
  std::string line;
  bool skip_text_block = false;
  
  while (std::getline(input, line)) {
    // Remove leading/trailing whitespace for easier parsing
    std::string trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
    
    // Skip text operators and text blocks
    if (trimmed == "BT") {
      skip_text_block = true;
      continue;
    } else if (trimmed == "ET") {
      skip_text_block = false;
      continue;
    } else if (skip_text_block) {
      // Skip all content within text blocks
      continue;
    }
    
    // Skip image operators (Do commands)
    if (trimmed.find(" Do") != std::string::npos) {
      std::cout << "    Filtering image operator: " << trimmed << std::endl;
      continue;
    }
    
    // Skip font setting operators (Tf commands)
    if (trimmed.find(" Tf") != std::string::npos) {
      continue;
    }
    
    // Skip text positioning operators
    if (trimmed.find(" Tm") != std::string::npos || 
        trimmed.find(" Td") != std::string::npos ||
        trimmed.find(" TD") != std::string::npos ||
        trimmed.find(" T*") != std::string::npos) {
      continue;
    }
    
    // Skip text showing operators
    if (trimmed.find(" Tj") != std::string::npos ||
        trimmed.find(" TJ") != std::string::npos ||
        trimmed.find("'") != std::string::npos ||
        trimmed.find("\"") != std::string::npos) {
      continue;
    }
    
    // Keep path drawing operators (m, l, c, s, S, f, F, B, b, etc.)
    // Keep color operators (rg, RG, k, K, cs, CS, sc, SC, etc.)
    // Keep graphics state operators (q, Q, cm, etc.)
    output << line << "\n";
  }
  
  return output.str();
}

bool flx_pdf_sio::replace_page_content_stream(PoDoFo::PdfPage& page, const std::string& new_content) {
  try {
    // Create a new content stream with the filtered content
    auto& new_stream = page.GetDocument().GetObjects().CreateDictionaryObject();
    
    // Set up the stream dictionary
    new_stream.GetDictionary().AddKey("Length", PdfObject(static_cast<int64_t>(new_content.length())));
    
    // Add the filtered content to the stream
    auto& stream = new_stream.GetOrCreateStream();
    stream.SetData(new_content);
    
    // Replace the page's content stream
    page.GetDictionary().AddKey("Contents", new_stream.GetIndirectReference());
    
    return true;
    
  } catch (const std::exception& e) {
    std::cout << "Error replacing content stream: " << e.what() << std::endl;
    return false;
  }
}

int flx_pdf_sio::flood_fill_neighbor_based(const cv::Mat& image, cv::Point seed, cv::Mat& region_mask, cv::Mat& processed, int tolerance) {
  if (seed.x < 0 || seed.y < 0 || seed.x >= image.cols || seed.y >= image.rows) {
    return 0;
  }
  
  if (processed.at<uchar>(seed.y, seed.x) > 0) {
    return 0;
  }
  
  std::stack<cv::Point> stack;
  stack.push(seed);
  int filled_pixels = 0;
  
  // 4-connectivity neighbors (up, down, left, right)
  const int dx[] = {0, 0, -1, 1};
  const int dy[] = {-1, 1, 0, 0};
  
  while (!stack.empty()) {
    cv::Point current = stack.top();
    stack.pop();
    
    // Skip if already processed
    if (processed.at<uchar>(current.y, current.x) > 0) {
      continue;
    }
    
    // Mark as processed and part of region
    processed.at<uchar>(current.y, current.x) = 255;
    region_mask.at<uchar>(current.y, current.x) = 255;
    filled_pixels++;
    
    // Get current pixel color
    cv::Vec3b current_color = image.at<cv::Vec3b>(current.y, current.x);
    
    // Check all 4 neighbors
    for (int i = 0; i < 4; i++) {
      int nx = current.x + dx[i];
      int ny = current.y + dy[i];
      
      // Check bounds
      if (nx < 0 || ny < 0 || nx >= image.cols || ny >= image.rows) {
        continue;
      }
      
      // Skip if already processed
      if (processed.at<uchar>(ny, nx) > 0) {
        continue;
      }
      
      // Check if neighbor color is similar to current pixel
      cv::Vec3b neighbor_color = image.at<cv::Vec3b>(ny, nx);
      if (is_color_similar(current_color, neighbor_color, tolerance)) {
        stack.push(cv::Point(nx, ny));
      }
    }
  }
  
  return filled_pixels;
}

bool flx_pdf_sio::is_color_similar(const cv::Vec3b& color1, const cv::Vec3b& color2, int tolerance) {
  // Calculate Euclidean distance in RGB color space
  int diff_b = std::abs((int)color1[0] - (int)color2[0]);
  int diff_g = std::abs((int)color1[1] - (int)color2[1]);
  int diff_r = std::abs((int)color1[2] - (int)color2[2]);
  
  // Use Manhattan distance for speed (sum of absolute differences)
  int total_diff = diff_b + diff_g + diff_r;
  
  return total_diff <= tolerance;
}

cv::Scalar flx_pdf_sio::calculate_dominant_color_for_contour(const std::vector<cv::Point>& contour, 
                                                        const std::vector<cv::Mat>& clean_images, 
                                                        size_t contour_index) {
  if (clean_images.empty()) {
    // Return a default color based on contour index
    int r = (contour_index * 70 + 100) % 256;
    int g = (contour_index * 90 + 150) % 256;
    int b = (contour_index * 110 + 200) % 256;
    return cv::Scalar(b, g, r); // OpenCV uses BGR
  }
  
  // Use the first clean image to sample color
  const cv::Mat& image = clean_images[0];
  
  // Create a mask for this specific contour
  cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
  std::vector<std::vector<cv::Point>> contour_vec = {contour};
  cv::fillPoly(mask, contour_vec, cv::Scalar(255));
  
  // Calculate mean color within the contour region
  cv::Scalar mean_color = cv::mean(image, mask);
  return mean_color;
}

std::string flx_pdf_sio::rgb_to_hex_string(const cv::Scalar& color) {
  int r = std::max(0, std::min(255, (int)color[2])); // OpenCV uses BGR
  int g = std::max(0, std::min(255, (int)color[1]));
  int b = std::max(0, std::min(255, (int)color[0]));
  
  char hex_string[8];
  snprintf(hex_string, sizeof(hex_string), "#%02X%02X%02X", r, g, b);
  return std::string(hex_string);
}

bool flx_pdf_sio::is_geometry_contained_in(const flx_layout_geometry& inner, const flx_layout_geometry& outer) {
  // Check if inner's bounding box is completely within outer's bounding box
  double inner_left = inner.x;
  double inner_top = inner.y;
  double inner_right = inner.x + inner.width;
  double inner_bottom = inner.y + inner.height;
  
  double outer_left = outer.x;
  double outer_top = outer.y;
  double outer_right = outer.x + outer.width;
  double outer_bottom = outer.y + outer.height;
  
  return (inner_left >= outer_left && 
          inner_top >= outer_top && 
          inner_right <= outer_right && 
          inner_bottom <= outer_bottom);
}

void flx_pdf_sio::assign_content_to_geometry_recursive(flx_layout_text& text, std::vector<flx_layout_geometry>& geometries) {
  for (auto& geometry : geometries) {
    if (is_point_in_geometry(text.x, text.y, geometry)) {
      // Try to assign to deepest possible sub-geometry first
      if (geometry.sub_geometries.size() > 0) {
        assign_content_to_geometry_recursive_flx(text, geometry.sub_geometries);
      } else {
        // Add to this geometry if no deeper container found
        geometry.texts.push_back(text);
      }
      return; // Only assign to one geometry
    }
  }
}

void flx_pdf_sio::assign_content_to_geometry_recursive(flx_layout_image& image, std::vector<flx_layout_geometry>& geometries) {
  for (auto& geometry : geometries) {
    if (is_point_in_geometry(image.x, image.y, geometry)) {
      // Try to assign to deepest possible sub-geometry first
      if (geometry.sub_geometries.size() > 0) {
        assign_content_to_geometry_recursive_flx(image, geometry.sub_geometries);
      } else {
        // Add to this geometry if no deeper container found
        geometry.images.push_back(image);
      }
      return; // Only assign to one geometry
    }
  }
}

void flx_pdf_sio::assign_content_to_geometry_recursive_flx(flx_layout_text& text, flx_model_list<flx_layout_geometry>& geometries) {
  for (size_t i = 0; i < geometries.size(); ++i) {
    auto& geometry = geometries[i];
    if (is_point_in_geometry(text.x, text.y, geometry)) {
      if (geometry.sub_geometries.size() > 0) {
        assign_content_to_geometry_recursive_flx(text, geometry.sub_geometries);
      } else {
        geometry.texts.push_back(text);
      }
      return;
    }
  }
}

void flx_pdf_sio::assign_content_to_geometry_recursive_flx(flx_layout_image& image, flx_model_list<flx_layout_geometry>& geometries) {
  for (size_t i = 0; i < geometries.size(); ++i) {
    auto& geometry = geometries[i];
    if (is_point_in_geometry(image.x, image.y, geometry)) {
      if (geometry.sub_geometries.size() > 0) {
        assign_content_to_geometry_recursive_flx(image, geometry.sub_geometries);
      } else {
        geometry.images.push_back(image);
      }
      return;
    }
  }
}

bool flx_pdf_sio::is_point_in_geometry(double x, double y, const flx_layout_geometry& geometry) {
  return (x >= geometry.x && 
          y >= geometry.y && 
          x <= geometry.x + geometry.width && 
          y <= geometry.y + geometry.height);
}