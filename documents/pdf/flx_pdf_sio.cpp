#include "flx_pdf_sio.h"
#include "flx_pdf_text_extractor.h"
#include <main/PdfMemDocument.h>
#include <main/PdfPainter.h>
#include <main/PdfColor.h>
#include <auxiliary/StreamDevice.h>
#include <main/PdfContentStreamReader.h>
#include <unordered_map>
#include <opencv2/opencv.hpp>
#include <iostream>
#include <iomanip>
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

    // CRITICAL FIX for XObject support: Create stable buffer copy!
    // PoDoFo's XObject processing accesses the buffer LATER (lazy loading).
    // The buffer MUST remain valid throughout the entire lifetime of PdfMemDocument!
    // std::vector<char> guarantees stable pointer (no reallocation after construction).
    pdf_data_buffer.clear();
    pdf_data_buffer.reserve(data.size());
    pdf_data_buffer.assign(data.c_str(), data.c_str() + data.size());

    bufferview buffer(pdf_data_buffer.data(), pdf_data_buffer.size());

    // Try to load PDF - some PDFs may have EOF marker issues
    try {
      m_pdf->LoadFromBuffer(buffer);
      std::cout << "✅ PDF loaded successfully" << std::endl;
    } catch (const std::exception& e) {
      std::cout << "❌ PDF loading failed: " << e.what() << std::endl;
      std::cout << "This may be due to PDF format issues or missing EOF markers." << std::endl;
      std::cout << "Try using a different PDF or repair the PDF first." << std::endl;
      return false;
    }
    
    std::cout << "Starting complete PDF → Layout extraction..." << std::endl;

    // DISABLED: Phase 1 extraction was interfering with XObject processing in Phase 2!
    // The first pass through all pages was somehow consuming/modifying XObjects,
    // causing the per-page extraction to miss XObject texts (69 vs 75 texts on BTS 5070 page 5).
    // Since these results aren't used anyway (were for disabled geometry pipeline), skip it.

    // Step 1: Extract texts from PDF using simple PoDoFo extraction
    // std::cout << "Extracting texts from PDF..." << std::endl;
    // flx_model_list<flx_layout_text> extracted_texts;
    // flx_model_list<flx_layout_image> extracted_images;
    //
    // if (!extract_texts_and_images(extracted_texts, extracted_images)) {
    //   std::cout << "Text extraction failed, falling back to dummy content..." << std::endl;
    // }
    //
    // std::cout << "Final result: " << extracted_texts.size() << " texts and "
    //           << extracted_images.size() << " images" << std::endl;
    
    // Step 2: SKIP geometry extraction for now to avoid segfault
    // std::cout << "Extracting geometries from original PDF..." << std::endl;
    // flx_model_list<flx_layout_geometry> geometry_pages;
    // if (!extract_geometries_from_content_streams(geometry_pages)) {
    //   std::cout << "Failed to extract geometries from content streams" << std::endl;
    //   return false;
    // }
    
    // TEMPORARILY DISABLED: Complex geometry extraction - using simple text-only approach
    /*
    // Step 3: Create NEW clean PDF with ONLY the extracted geometries
    std::cout << "Creating new PDF with only geometries..." << std::endl;
    auto clean_pdf = create_geometry_only_pdf(geometry_pages);
    if (!clean_pdf) {
      std::cout << "Failed to create geometry-only PDF" << std::endl;
      return false;
    }
    
    // Step 4: Render the clean geometry-only PDF to images for OpenCV processing
    std::cout << "Rendering geometry-only PDF pages to images..." << std::endl;
    std::vector<cv::Mat> clean_images;
    if (!render_clean_pdf_to_images(clean_pdf.get(), clean_images)) {
      std::cout << "Failed to render geometry-only PDF to images" << std::endl;
      return false;
    }
    
    // Step 5: Process images with OpenCV to detect regions (WITH DEBUG OUTPUT)
    std::cout << "Processing images with OpenCV to detect color-coherent regions..." << std::endl;
    std::vector<std::vector<std::vector<cv::Point>>> all_page_contours;
    
    for (size_t page_idx = 0; page_idx < clean_images.size(); page_idx++) {
      const cv::Mat& page_image = clean_images[page_idx];
      std::cout << "📄 Processing page " << (page_idx + 1) << " (" << page_image.cols << "x" << page_image.rows << ")" << std::endl;
      
      // Create debug directory for this page
      std::string debug_dir = "debug_page_" + std::to_string(page_idx + 1);
      create_debug_directory(debug_dir);
      
      // Save original rendered PDF image
      std::string original_path = debug_dir + "/01_original_pdf_render.png";
      cv::imwrite(original_path, page_image);
      std::cout << "💾 Saved original PDF render: " << original_path << std::endl;
      
      // Detect color regions using flood-fill
      auto color_masks = detect_color_regions(page_image, debug_dir);
      
      // Extract contours from masks
      auto page_contours = extract_contours_from_masks(color_masks, page_image, debug_dir);
      all_page_contours.push_back(page_contours);
      
      std::cout << "✅ Page " << (page_idx + 1) << " complete: " << page_contours.size() << " regions detected" << std::endl;
    }
    
    // Step 6: Build hierarchical geometry structure from contours
    std::cout << "Building hierarchical geometry structure..." << std::endl;
    pages = flx_model_list<flx_layout_geometry>();  // Initialize pages
    build_geometry_hierarchy(all_page_contours, clean_images, pages);
    
    // Step 7: Assign texts and images to geometries ("what is inside what")
    std::cout << "Assigning content to geometries..." << std::endl;
    try {
      assign_content_to_geometries_flx(extracted_texts, extracted_images, pages);
      std::cout << "✅ Content assignment completed successfully" << std::endl;
    } catch (const std::exception& e) {
      std::cout << "⚠️ Error assigning content to geometries: " << e.what() << std::endl;
      std::cout << "⚠️ Continuing without content assignment..." << std::endl;
    }
    */
    
    // Create page structures and extract texts directly into them
    std::cout << "Creating page structures from PDF..." << std::endl;

    // CRITICAL FIX: Clear static font cache before second extraction pass!
    // The font cache contains PdfFont* pointers from the first pass, which may be invalid
    // for the second pass extraction. This was causing XObject texts to be skipped.
    std::cout << "🔧 Clearing font cache before per-page extraction..." << std::endl;
    flx_pdf_text_extractor::clear_font_cache();

    pages = flx_model_list<flx_layout_geometry>();
    int page_count = m_pdf->GetPages().GetCount();

    // Extract texts page by page directly into page structures
    for (int page_idx = 0; page_idx < page_count; page_idx++) {
        pages.add_element();
        auto& page_geom = pages.back();

        // Set up basic page geometry (full page bounds)
        page_geom.x = 0.0;
        page_geom.y = 0.0;
        page_geom.width = 595.0;  // A4 width in points (TODO: read actual page size)
        page_geom.height = 842.0; // A4 height in points

        // CRITICAL FIX: Clear font cache between pages to prevent XObject processing issues
        // The static font cache can contain stale PdfFont* pointers from previous pages,
        // causing XObject text extraction to fail on subsequent pages.
        flx_pdf_text_extractor::clear_font_cache();

        // Extract texts for this specific page
        auto& pdf_page = m_pdf->GetPages().GetPageAt(page_idx);
        flx_pdf_text_extractor extractor;
        extractor.extract_text_with_fonts(pdf_page, page_geom.texts);
    }

    // Count total texts across all pages
    int total_texts = 0;
    for (int i = 0; i < pages.size(); i++) {
        total_texts += pages[i].texts.size();
    }

    std::cout << "✅ Created " << pages.size() << " page structures with "
              << total_texts << " texts total" << std::endl;
    
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
  if (geometry.vertices.size() < 2) return; // Need at least 2 points for line or polygon
  
  // Create path from vertices
  PdfPainterPath path;
  auto& first_vertex = geometry.vertices[0];
  path.MoveTo(first_vertex.x, first_vertex.y);
  
  // Add remaining vertices
  for (size_t i = 1; i < geometry.vertices.size(); ++i) {
    auto& vertex = geometry.vertices[i];
    path.AddLineTo(vertex.x, vertex.y);
  }
  
  // Determine rendering mode based on available colors
  if (geometry.fill_color->empty() == false && geometry.stroke_color->empty() == false) {
    // Both fill and stroke
    path.Close();
    PdfColor pdf_fill_color = parse_hex_color(geometry.fill_color);
    PdfColor pdf_stroke_color = parse_hex_color(geometry.stroke_color);
    painter.GraphicsState.SetNonStrokingColor(pdf_fill_color);
    painter.GraphicsState.SetStrokingColor(pdf_stroke_color);
    painter.DrawPath(path, PdfPathDrawMode::StrokeFill);
  } else if (geometry.fill_color->empty() == false) {
    // Fill only (polygon)
    path.Close();
    PdfColor pdf_color = parse_hex_color(geometry.fill_color);
    painter.GraphicsState.SetNonStrokingColor(pdf_color);
    painter.DrawPath(path, PdfPathDrawMode::Fill);
  } else if (geometry.stroke_color->empty() == false) {
    // Stroke only (line/outline)
    PdfColor pdf_color = parse_hex_color(geometry.stroke_color);
    painter.GraphicsState.SetStrokingColor(pdf_color);
    painter.DrawPath(path, PdfPathDrawMode::Stroke);
  }
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

bool flx_pdf_sio::extract_texts_and_images(flx_model_list<flx_layout_text>& texts, flx_model_list<flx_layout_image>& images) {
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
      
      // Extract text directly into the flx_model_list
      bool extraction_success = text_extractor.extract_text_with_fonts(page, texts);
      
      if (extraction_success) {
        std::cout << "Page " << (page_num + 1) << ": Successfully extracted text entries directly to model_list" << std::endl;
      } else {
        std::cout << "⚠️ Page " << (page_num + 1) << ": Text extraction with fonts failed! Using basic extraction..." << std::endl;
        
        // SIMPLIFIED FALLBACK: Use emplace_back to avoid copy constructor
        try {
          texts.add_element();
          auto& dummy_text = texts.back();
          dummy_text.text = flx_string("Page " + std::to_string(page_num + 1) + " content");
          dummy_text.x = 50.0;
          dummy_text.y = 50.0;
          dummy_text.font_size = 12.0;
          dummy_text.font_family = flx_string("Arial");
          std::cout << "    Fallback: Added dummy text for page " << (page_num + 1) << std::endl;
        } catch (const std::exception& e) {
          std::cout << "    Even dummy text creation failed: " << e.what() << std::endl;
        }
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
      try {
        std::cout << "    Processing page " << (page_num + 1) << "..." << std::endl;
        
        auto& page = pages.GetPageAt(page_num);
        
        // Get the page's content stream
        auto contents = page.GetContents();
        if (contents == nullptr) {
          std::cout << "    Page " << (page_num + 1) << ": No contents found" << std::endl;
          continue;
        }
      
      // Read the current content stream  
      auto content_buffer = contents->GetCopy();
      std::string content_str(content_buffer.data(), content_buffer.size());
      
      if (content_str.empty()) {
        std::cout << "    Page " << (page_num + 1) << ": Empty content stream" << std::endl;
        continue;
      }
      
      // Debug: Show original content
      std::cout << "    Original content (" << content_str.size() << " bytes):" << std::endl;
      std::cout << "    " << content_str.substr(0, 200) << "..." << std::endl;
      
      // Filter out text and image operators while keeping path operations
      std::string filtered_content = filter_pdf_content_stream(content_str);
      
      // Debug: Show filtered content  
      std::cout << "    Filtered content (" << filtered_content.size() << " bytes):" << std::endl;
      std::cout << "    " << filtered_content.substr(0, 200) << "..." << std::endl;
      
      // Replace the content stream with filtered version
      if (filtered_content.empty()) {
        std::cout << "    Page " << (page_num + 1) << ": Warning - filtered content is empty, skipping" << std::endl;
      } else {
        try {
          contents->Reset();  // Clear existing content
          auto& stream = contents->CreateStreamForAppending();
          stream.GetOutputStream().Write(filtered_content);
          std::cout << "    Page " << (page_num + 1) << ": Stream write successful" << std::endl;
        } catch (const std::exception& stream_error) {
          std::cout << "    Page " << (page_num + 1) << ": Stream write error: " << stream_error.what() << std::endl;
          return false;
        }
      }
      
      std::cout << "    Page " << (page_num + 1) << ": Successfully filtered " 
                << content_str.size() << " -> " << filtered_content.size() << " bytes" << std::endl;
        
      } catch (const std::exception& page_error) {
        std::cout << "    Page " << (page_num + 1) << ": Error processing page: " << page_error.what() << std::endl;
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

std::vector<cv::Mat> flx_pdf_sio::detect_color_regions(const cv::Mat& page_image, const std::string& debug_dir) {
  std::cout << "🎨 Detecting color regions using flood-fill..." << std::endl;
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
  
  // Single pass: Process every pixel exactly once
  // Each pixel becomes a seed only if not already processed by flood-fill
  for (int y = 0; y < page_image.rows; y++) {
    for (int x = 0; x < page_image.cols; x++) {
      
      // Skip if already processed
      if (processed.at<uchar>(y, x) > 0) {
        continue;
      }
      
      // This pixel hasn't been processed yet - use as seed for new region
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
      // Note: Even small regions are marked as processed to avoid reprocessing
    }
  }
  
  std::cout << "🎯 Found " << masks.size() << " color-coherent regions (processed every pixel exactly once)" << std::endl;
  
  // Debug: Save region visualization if debug directory provided
  if (!debug_dir.empty() && !masks.empty()) {
    std::cout << "🖼️  Creating debug visualizations..." << std::endl;
    
    // Create a colored visualization of all regions
    cv::Mat region_colors = page_image.clone();
    cv::Mat all_regions_mask = cv::Mat::zeros(page_image.size(), CV_8UC3);
    
    // Use original dominant colors for each region  
    std::vector<cv::Vec3b> region_color_palette;
    for (size_t i = 0; i < masks.size(); i++) {
      // Calculate actual dominant color from original image
      cv::Scalar mean_color_scalar = cv::mean(page_image, masks[i]);
      cv::Vec3b original_color(
        (uchar)mean_color_scalar[0], // B
        (uchar)mean_color_scalar[1], // G  
        (uchar)mean_color_scalar[2]  // R
      );
      region_color_palette.push_back(original_color);
    }
    
    // Apply original colored masks
    for (size_t i = 0; i < masks.size(); i++) {
      cv::Vec3b color = region_color_palette[i];
      
      // Save individual region mask
      std::string mask_path = debug_dir + "/02_region_" + std::to_string(i+1) + "_mask.png";
      cv::imwrite(mask_path, masks[i]);
      
      // Add colored overlay to combined visualization
      all_regions_mask.setTo(color, masks[i]);
      
      // Calculate region stats
      int pixel_count = cv::countNonZero(masks[i]);
      cv::Rect bbox = cv::boundingRect(masks[i]);
      cv::Scalar mean_color = cv::mean(page_image, masks[i]);
      
      std::cout << "    🟦 Region " << (i+1) << ": " << pixel_count << " pixels, "
                << "bbox(" << bbox.x << "," << bbox.y << "," << bbox.width << "," << bbox.height << "), "
                << "color(" << (int)mean_color[2] << "," << (int)mean_color[1] << "," << (int)mean_color[0] << ")" << std::endl;
    }
    
    // Save combined visualization
    std::string regions_path = debug_dir + "/02_all_regions_colored.png";
    cv::imwrite(regions_path, all_regions_mask);
    std::cout << "💾 Saved regions visualization: " << regions_path << std::endl;
    
    // Create overlay version
    cv::Mat overlay;
    cv::addWeighted(page_image, 0.6, all_regions_mask, 0.4, 0, overlay);
    std::string overlay_path = debug_dir + "/02_regions_overlay.png";
    cv::imwrite(overlay_path, overlay);
    std::cout << "💾 Saved regions overlay: " << overlay_path << std::endl;
  }
  
  return masks;
}

std::vector<std::vector<cv::Point>> flx_pdf_sio::extract_contours_from_masks(const std::vector<cv::Mat>& masks, const cv::Mat& original_image, const std::string& debug_dir) {
  std::cout << "📐 Extracting contours from masks..." << std::endl;
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
  
  std::cout << "🔺 Extracted " << all_contours.size() << " polygon contours" << std::endl;
  
  // Debug: Save contour visualizations if debug directory provided
  if (!debug_dir.empty() && !original_image.empty() && !all_contours.empty()) {
    std::cout << "🖊️  Creating contour visualizations..." << std::endl;
    
    // Create contour visualization
    cv::Mat contour_image = original_image.clone();
    cv::Mat pure_contours = cv::Mat::zeros(original_image.size(), CV_8UC3);
    
    // Draw all contours with different colors
    for (size_t i = 0; i < all_contours.size(); i++) {
      cv::Scalar color(rand() % 256, rand() % 256, rand() % 256);
      
      // Draw filled contour on pure background
      cv::fillPoly(pure_contours, std::vector<std::vector<cv::Point>>{all_contours[i]}, color);
      
      // Draw contour outline on original image
      cv::polylines(contour_image, std::vector<std::vector<cv::Point>>{all_contours[i]}, true, color, 2);
      
      // Draw vertices as points
      for (const cv::Point& pt : all_contours[i]) {
        cv::circle(contour_image, pt, 3, cv::Scalar(0, 255, 0), -1);
      }
      
      std::cout << "    📍 Contour " << (i+1) << ": " << all_contours[i].size() << " vertices" << std::endl;
    }
    
    // Save contour visualizations
    std::string contours_path = debug_dir + "/03_contours_on_original.png";
    cv::imwrite(contours_path, contour_image);
    std::cout << "💾 Saved contours on original: " << contours_path << std::endl;
    
    std::string pure_contours_path = debug_dir + "/03_contours_filled.png";
    cv::imwrite(pure_contours_path, pure_contours);
    std::cout << "💾 Saved filled contours: " << pure_contours_path << std::endl;
  }
  
  return all_contours;
}

void flx_pdf_sio::build_geometry_hierarchy(const std::vector<std::vector<std::vector<cv::Point>>>& page_contours,
                                          const std::vector<cv::Mat>& clean_images,
                                          flx_model_list<flx_layout_geometry>& geometries) {
  std::cout << "📊 Building geometry hierarchy..." << std::endl;
  geometries = flx_model_list<flx_layout_geometry>();
  
  std::cout << "   Processing " << page_contours.size() << " pages with contours..." << std::endl;
  
  // Process each page separately
  for (size_t page_idx = 0; page_idx < page_contours.size(); page_idx++) {
    const auto& page_contour_list = page_contours[page_idx];
    const cv::Mat& page_image = (page_idx < clean_images.size()) ? clean_images[page_idx] : cv::Mat();
    
    std::cout << "   📄 Page " << (page_idx + 1) << ": " << page_contour_list.size() << " contours" << std::endl;
    
    // Create page geometry
    flx_layout_geometry page_geom;
    
    // Set page bounds (full image size)
    if (!page_image.empty()) {
      page_geom.x = 0;
      page_geom.y = 0;
      page_geom.width = page_image.cols;
      page_geom.height = page_image.rows;
      page_geom.fill_color = "#F7F4F6"; // Default page background
    }
  
    // Clean up old debug directory and create fresh one
    std::string debug_dir = "debug_page_" + std::to_string(page_idx + 1);
    std::string cleanup_cmd = "rm -rf " + debug_dir;
    system(cleanup_cmd.c_str());
    create_debug_directory(debug_dir);
    
    // TEMPORARILY DISABLE Hough line detection - causing crash
    std::vector<flx_layout_geometry> detected_lines; // Empty
    std::cout << "    🔍 Hough line detection temporarily disabled - crash debugging" << std::endl;
    
    // Convert contours to sub-geometries for this page
    flx_model_list<flx_layout_geometry> page_sub_geometries;
    for (size_t i = 0; i < page_contour_list.size(); ++i) {
      const auto& contour = page_contour_list[i];
      
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
      
      // Calculate dominant color from the page image
      cv::Scalar mean_color = cv::Scalar(128, 128, 128); // Default gray
      if (!page_image.empty()) {
        mean_color = calculate_dominant_color_for_contour_from_image(contour, page_image);
      }
      
      // Check if this contour should be treated as a line (≤5px width)
      if (is_contour_line_shaped(contour, 5.0)) {
        // Store as stroke_color for lines
        geom.stroke_color = rgb_to_hex_string(mean_color);
      } else {
        // Store as fill_color for filled shapes  
        geom.fill_color = rgb_to_hex_string(mean_color);
      }
      
      page_sub_geometries.push_back(geom);
      
      // Debug output differentiate between lines and shapes
      if (geom.stroke_color->empty() == false) {
        std::cout << "     🔸 LINE " << (i+1) << ": " << geom.vertices.size() << " vertices, "
                  << "stroke_color=" << geom.stroke_color->c_str() << ", bbox(" 
                  << geom.x << "," << geom.y << "," << geom.width << "," << geom.height << ")" << std::endl;
      } else {
        std::cout << "     🔸 SHAPE " << (i+1) << ": " << geom.vertices.size() << " vertices, "
                  << "fill_color=" << geom.fill_color->c_str() << ", bbox(" 
                  << geom.x << "," << geom.y << "," << geom.width << "," << geom.height << ")" << std::endl;
      }
    }
    
    // Add detected lines to the page sub-geometries
    for (const auto& line : detected_lines) {
      page_sub_geometries.push_back(line);
      std::cout << "     📏 HOUGH LINE: " << line.vertices.size() << " vertices, "
                << "stroke_color=" << line.stroke_color->c_str() << ", bbox(" 
                << line.x << "," << line.y << "," << line.width << "," << line.height << ")" << std::endl;
    }
    
    // TEMPORARILY DISABLE hierarchical structure - causing crash
    //build_hierarchical_structure(page_sub_geometries);
    std::cout << "    🏗️ Hierarchical structure temporarily disabled - debugging content assignment" << std::endl;
    
    // Assign sub-geometries to page
    page_geom.sub_geometries = page_sub_geometries;
    
    // Add page to main geometries list
    geometries.push_back(page_geom);
    
    std::cout << "     ✅ Page " << (page_idx + 1) << " complete: " << page_sub_geometries.size() << " sub-geometries" << std::endl;
  }
  
  std::cout << "  📊 Built hierarchy: " << geometries.size() << " pages with nested geometries" << std::endl;
}

void flx_pdf_sio::assign_content_to_geometries(flx_model_list<flx_layout_text>& texts, 
                                              flx_model_list<flx_layout_image>& images,
                                              flx_model_list<flx_layout_geometry>& geometries) {
  std::cout << "  Assigning content to geometries..." << std::endl;
  
  // Assign texts to geometries
  for (size_t i = 0; i < texts.size(); i++) {
    assign_content_to_geometry_recursive(texts.at(i), geometries);
  }
  
  // Assign images to geometries
  for (size_t i = 0; i < images.size(); i++) {
    assign_content_to_geometry_recursive(images.at(i), geometries);
  }
  
  std::cout << "  Content assignment completed" << std::endl;
}

void flx_pdf_sio::assign_content_to_geometries_flx(flx_model_list<flx_layout_text>& texts, 
                                                  flx_model_list<flx_layout_image>& images,
                                                  flx_model_list<flx_layout_geometry>& geometries) {
  std::cout << "  Assigning content to geometries..." << std::endl;
  
  // Assign texts to geometries
  for (size_t i = 0; i < texts.size(); i++) {
    assign_content_to_geometry_recursive_flx(texts.at(i), geometries);
  }
  
  // Assign images to geometries
  for (size_t i = 0; i < images.size(); i++) {
    assign_content_to_geometry_recursive_flx(images.at(i), geometries);
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
    
    // Safely trim leading whitespace
    size_t start = trimmed.find_first_not_of(" \t");
    if (start == std::string::npos) {
      trimmed = "";  // Line is all whitespace
    } else {
      // Safely trim trailing whitespace
      size_t end = trimmed.find_last_not_of(" \t");
      if (end != std::string::npos) {
        trimmed = trimmed.substr(start, end - start + 1);
      } else {
        trimmed = "";  // Shouldn't happen, but be safe
      }
    }
    
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

bool flx_pdf_sio::extract_geometries_from_content_streams(flx_model_list<flx_layout_geometry>& all_geometries) {
  try {
    auto& pdf_pages = m_pdf->GetPages();
    
    for (unsigned int page_num = 0; page_num < pdf_pages.GetCount(); ++page_num) {
      auto& pdf_page = pdf_pages.GetPageAt(page_num);
      
      // Create a new page geometry for this PDF page
      all_geometries.add_element();
      auto& page_geometry = all_geometries.back();
            
      // Set page dimensions (A4 default for now)
      page_geometry.x = 0.0;
      page_geometry.y = 0.0;
      page_geometry.width = 595.0;
      page_geometry.height = 842.0;
      
      // Get content stream
      auto contents = pdf_page.GetContents();
      if (contents == nullptr) {
        std::cout << "Page " << (page_num + 1) << ": No content stream" << std::endl;
        continue;
      }
      
      // Extract content as string
      auto content_buffer = contents->GetCopy();
      std::string content_str(content_buffer.data(), content_buffer.size());
      
      // Parse geometries into this page's sub_geometries
      if (parse_content_stream_for_geometries(content_str, page_geometry.sub_geometries)) {
        std::cout << "Page " << (page_num + 1) << ": Successfully parsed content stream" << std::endl;
        std::cout << "Page " << (page_num + 1) << ": Extracted " << page_geometry.sub_geometries.size() << " geometries" << std::endl;
      } else {
        std::cout << "Page " << (page_num + 1) << ": Failed to parse geometries" << std::endl;
        return false;
      }
    }
    
    std::cout << "Total pages created: " << all_geometries.size() << std::endl;
    return true;
    
  } catch (const std::exception& e) {
    std::cout << "Error extracting geometries: " << e.what() << std::endl;
    return false;
  }
}

bool flx_pdf_sio::parse_content_stream_for_geometries(const std::string& content, flx_model_list<flx_layout_geometry>& geometries) {
  std::istringstream input(content);
  std::string line;
  pdf_graphics_state state;
  std::stack<pdf_graphics_state> state_stack;  // For q/Q save/restore
  
  while (std::getline(input, line)) {
    // Remove whitespace
    std::string trimmed = line;
    trimmed.erase(0, trimmed.find_first_not_of(" \t"));
    trimmed.erase(trimmed.find_last_not_of(" \t") + 1);
    
    if (trimmed.empty()) continue;
    
    std::cout << "  Processing line: '" << trimmed << "'" << std::endl;
    
    // Graphics state save/restore - DISABLED FOR DEBUGGING
    if (trimmed == "q") {
      // state_stack.push(state);  // DISABLED - causes segfault with flx_model_list
      std::cout << "  Ignoring q (save graphics state)" << std::endl;
      continue;
    } else if (trimmed == "Q") {
      // if (!state_stack.empty()) {
      //   state = state_stack.top();
      //   state_stack.pop();
      // }
      std::cout << "  Ignoring Q (restore graphics state)" << std::endl;
      continue;
    }
    
    // Path construction operators
    if (trimmed.find(" m") != std::string::npos && trimmed.back() == 'm') {
      parse_move_to(trimmed, state);
    } else if (trimmed.find(" l") != std::string::npos && trimmed.back() == 'l') {
      parse_line_to(trimmed, state);
    } else if (trimmed.find(" c") != std::string::npos && trimmed.back() == 'c') {
      parse_curve_to(trimmed, state);
    } else if (trimmed == "h") {
      parse_close_path(state);
    } else if (trimmed.find(" re") != std::string::npos && trimmed.substr(trimmed.length()-2) == "re") {
      parse_rectangle(trimmed, state);
    }
    
    // Color operators  
    else if (trimmed.find(" rg") != std::string::npos && trimmed.substr(trimmed.length()-2) == "rg") {
      parse_fill_color(trimmed, state);
    } else if (trimmed.find(" RG") != std::string::npos && trimmed.substr(trimmed.length()-2) == "RG") {
      parse_stroke_color(trimmed, state);
    }
    
    // Paint operators
    else if (trimmed == "f" || trimmed == "F") {
      parse_fill_path(state, geometries);
    } else if (trimmed == "S") {
      parse_stroke_path(state, geometries);
    } else if (trimmed == "B" || trimmed == "b") {
      // Fill and stroke
      parse_fill_path(state, geometries);
      parse_stroke_path(state, geometries);
    }
  }
  
  return true;
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

cv::Scalar flx_pdf_sio::calculate_dominant_color_for_contour_from_image(const std::vector<cv::Point>& contour, const cv::Mat& image) {
  if (image.empty() || contour.empty()) {
    return cv::Scalar(255, 255, 255); // Default white
  }
  
  // Create a mask for the contour
  cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
  cv::fillPoly(mask, std::vector<std::vector<cv::Point>>{contour}, cv::Scalar(255));
  
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

bool flx_pdf_sio::is_contour_line_shaped(const std::vector<cv::Point>& contour, double max_width) {
  if (contour.size() < 4) return false; // Need at least 4 points for a meaningful shape
  
  // Calculate bounding rectangle
  cv::Rect bounding_rect = cv::boundingRect(contour);
  
  // Check if width or height is very small (line-like)
  double width = bounding_rect.width;
  double height = bounding_rect.height;
  
  // Consider it a line if one dimension is ≤ max_width pixels
  bool is_thin_horizontal = (height <= max_width && width > max_width);
  bool is_thin_vertical = (width <= max_width && height > max_width);
  
  if (is_thin_horizontal || is_thin_vertical) {
    std::cout << "    🔍 Detected line-shaped contour: " << width << "x" << height 
              << " pixels (threshold: " << max_width << "px)" << std::endl;
    return true;
  }
  
  return false;
}

flx_model_list<flx_layout_geometry> flx_pdf_sio::detect_lines_hough(const cv::Mat& image, const std::string& debug_dir) {
  flx_model_list<flx_layout_geometry> detected_lines;
  
  if (image.empty()) {
    std::cout << "    ⚠️ Empty image for Hough line detection" << std::endl;
    return detected_lines;
  }
  
  std::cout << "    🔍 Starting Hough line detection..." << std::endl;
  
  // Convert to grayscale if needed
  cv::Mat gray;
  if (image.channels() == 3) {
    cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
  } else {
    gray = image.clone();
  }
  
  // Apply edge detection (Canny)
  cv::Mat edges;
  cv::Canny(gray, edges, 50, 150, 3);
  
  // Save debug edge detection image
  if (!debug_dir.empty()) {
    std::string edge_path = debug_dir + "/04_edges_canny.png";
    cv::imwrite(edge_path, edges);
    std::cout << "      💾 Saved edge detection: " << edge_path << std::endl;
  }
  
  // Apply Hough Line Transform  
  std::vector<cv::Vec4i> lines;
  cv::HoughLinesP(edges, lines, 1, CV_PI/180, 50, 30, 10);
  
  std::cout << "    📏 Detected " << lines.size() << " lines with Hough transform" << std::endl;
  
  // Convert lines to flx_layout_geometry with stroke_color
  for (size_t i = 0; i < lines.size(); i++) {
    cv::Vec4i line = lines[i];
    
    flx_layout_geometry line_geom;
    
    // Add start and end points as vertices
    line_geom.vertices.add_element();
    line_geom.vertices.back().x = line[0];  // x1
    line_geom.vertices.back().y = line[1];  // y1
    
    line_geom.vertices.add_element();
    line_geom.vertices.back().x = line[2];  // x2  
    line_geom.vertices.back().y = line[3];  // y2
    
    // Calculate bounding box
    int min_x = std::min(line[0], line[2]);
    int max_x = std::max(line[0], line[2]);
    int min_y = std::min(line[1], line[3]);
    int max_y = std::max(line[1], line[3]);
    
    line_geom.x = min_x;
    line_geom.y = min_y;
    line_geom.width = max_x - min_x;
    line_geom.height = max_y - min_y;
    
    // Sample color along the line
    cv::Point center((line[0] + line[2]) / 2, (line[1] + line[3]) / 2);
    if (center.x >= 0 && center.x < image.cols && center.y >= 0 && center.y < image.rows) {
      cv::Scalar line_color;
      if (image.channels() == 3) {
        cv::Vec3b pixel = image.at<cv::Vec3b>(center.y, center.x);
        line_color = cv::Scalar(pixel[0], pixel[1], pixel[2]); // BGR
      } else if (image.channels() == 1) {
        uchar pixel = image.at<uchar>(center.y, center.x);
        line_color = cv::Scalar(pixel, pixel, pixel); // Convert gray to BGR
      } else {
        line_color = cv::Scalar(0, 0, 0); // Default black for other formats
      }
      line_geom.stroke_color = rgb_to_hex_string(line_color);
    } else {
      line_geom.stroke_color = "#000000"; // Default black
    }
    
    detected_lines.push_back(line_geom);
    
    std::cout << "      📏 Line " << (i+1) << ": (" << line[0] << "," << line[1] 
              << ") → (" << line[2] << "," << line[3] << "), color="; 
    if (line_geom.stroke_color->empty() == false) {
      std::cout << line_geom.stroke_color->c_str();
    } else {
      std::cout << "(empty)";
    }
    std::cout << std::endl;
  }
  
  // Create debug visualization
  if (!debug_dir.empty() && lines.size() > 0) {
    cv::Mat line_vis = image.clone();
    for (const auto& line : lines) {
      cv::line(line_vis, cv::Point(line[0], line[1]), cv::Point(line[2], line[3]), 
               cv::Scalar(0, 255, 0), 2); // Green lines
    }
    std::string lines_path = debug_dir + "/04_detected_lines.png";
    cv::imwrite(lines_path, line_vis);
    std::cout << "      💾 Saved line visualization: " << lines_path << std::endl;
  }
  
  return detected_lines;
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

void flx_pdf_sio::assign_content_to_geometry_recursive(flx_layout_text& text, flx_model_list<flx_layout_geometry>& geometries) {
  for (size_t i = 0; i < geometries.size(); i++) {
    auto& geometry = geometries.at(i);
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

void flx_pdf_sio::assign_content_to_geometry_recursive(flx_layout_image& image, flx_model_list<flx_layout_geometry>& geometries) {
  for (size_t i = 0; i < geometries.size(); i++) {
    auto& geometry = geometries.at(i);
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

void flx_pdf_sio::build_hierarchical_structure(flx_model_list<flx_layout_geometry>& geometries) {
  std::cout << "    🏗️ Building hierarchical structure for " << geometries.size() << " geometries..." << std::endl;
  
  if (geometries.size() <= 1) return;
  
  try {
    // Convert to std::vector for safer manipulation
    std::vector<flx_layout_geometry> geometry_vector;
    for (size_t i = 0; i < geometries.size(); i++) {
      geometry_vector.push_back(geometries[i]);
    }
    
    // Sort geometries by area (largest first) to process containers before contained items
    std::vector<std::pair<size_t, double>> geometry_areas;
    for (size_t i = 0; i < geometry_vector.size(); i++) {
      double area = geometry_vector[i].width * geometry_vector[i].height;
      geometry_areas.push_back({i, area});
    }
    
    std::sort(geometry_areas.begin(), geometry_areas.end(), 
             [](const auto& a, const auto& b) { return a.second > b.second; });
    
    // Create a working list to track which geometries are still available for assignment
    std::vector<bool> available(geometry_vector.size(), true);
    
    // Process geometries from largest to smallest
    for (const auto& outer_pair : geometry_areas) {
      size_t outer_idx = outer_pair.first;
      
      if (!available[outer_idx]) continue;
      
      auto& outer_geom = geometry_vector[outer_idx];
      
      // Find all smaller geometries that are contained in this one
      for (const auto& inner_pair : geometry_areas) {
        size_t inner_idx = inner_pair.first;
        
        if (inner_idx == outer_idx || !available[inner_idx]) continue;
        if (inner_pair.second >= outer_pair.second) continue; // Only check smaller geometries
        
        auto& inner_geom = geometry_vector[inner_idx];
        
        // Check if inner is contained in outer
        if (is_geometry_contained_in(inner_geom, outer_geom)) {
          std::cout << "      📦 Moving geometry " << inner_idx << " into geometry " << outer_idx 
                    << " (container: " << outer_geom.width << "x" << outer_geom.height 
                    << ", contained: " << inner_geom.width << "x" << inner_geom.height << ")" << std::endl;
          
          // Move the contained geometry to the container's sub_geometries
          outer_geom.sub_geometries.push_back(inner_geom);
          available[inner_idx] = false; // Mark as used
        }
      }
    }
    
    // Create new flx_model_list with only top-level geometries
    flx_model_list<flx_layout_geometry> top_level_geometries;
    for (size_t i = 0; i < geometry_vector.size(); i++) {
      if (available[i]) {
        top_level_geometries.push_back(geometry_vector[i]);
      }
    }
    
    std::cout << "      🏆 Result: " << top_level_geometries.size() << " top-level, " 
              << (geometry_vector.size() - top_level_geometries.size()) << " nested" << std::endl;
    
    // Replace original list with hierarchical structure
    geometries = top_level_geometries;
    
  } catch (const std::exception& e) {
    std::cout << "      ⚠️ Error in hierarchical structure building: " << e.what() << std::endl;
    std::cout << "      ⚠️ Continuing with flat structure..." << std::endl;
  }
}

// PDF Operator Parsers

bool flx_pdf_sio::parse_move_to(const std::string& line, pdf_graphics_state& state) {
  // Format: "x y m"
  std::istringstream iss(line);
  double x, y;
  char op;
  
  if (iss >> x >> y >> op && op == 'm') {
    state.current_x = x;
    state.current_y = y;
    state.current_path.clear();  // Clear path
    state.current_path.push_back({x, y});
    state.path_closed = false;
    return true;
  }
  return false;
}

bool flx_pdf_sio::parse_line_to(const std::string& line, pdf_graphics_state& state) {
  // Format: "x y l"  
  std::istringstream iss(line);
  double x, y;
  char op;
  
  if (iss >> x >> y >> op && op == 'l') {
    state.current_x = x;
    state.current_y = y;
    state.current_path.push_back({x, y});
    return true;
  }
  return false;
}

bool flx_pdf_sio::parse_curve_to(const std::string& line, pdf_graphics_state& state) {
  // Format: "x1 y1 x2 y2 x3 y3 c" - Cubic Bezier curve
  std::istringstream iss(line);
  double x1, y1, x2, y2, x3, y3;
  char op;
  
  if (iss >> x1 >> y1 >> x2 >> y2 >> x3 >> y3 >> op && op == 'c') {
    // For now, approximate Bezier curve with line to end point
    // TODO: Add proper Bezier support to flx_layout_geometry
    state.current_x = x3;
    state.current_y = y3;
    state.current_path.push_back({x3, y3});
    std::cout << "    Bezier curve approximated as line: (" << x1 << "," << y1 << ") -> (" << x3 << "," << y3 << ")" << std::endl;
    return true;
  }
  return false;
}

bool flx_pdf_sio::parse_close_path(pdf_graphics_state& state) {
  if (state.current_path.size() > 0) {
    // Close path by adding line back to first point if not already there
    const auto& first = state.current_path[0];
    double first_x = first.first;
    double first_y = first.second;
    if (state.current_x != first_x || state.current_y != first_y) {
      state.current_path.push_back({first_x, first_y});
    }
    state.path_closed = true;
    state.current_x = first_x;
    state.current_y = first_y;
  }
  return true;
}

bool flx_pdf_sio::parse_rectangle(const std::string& line, pdf_graphics_state& state) {
  // Format: "x y w h re"
  std::istringstream iss(line);
  double x, y, w, h;
  std::string op;
  
  if (iss >> x >> y >> w >> h >> op && op == "re") {
    // Create rectangle path
    state.current_path.clear();  // Clear path
    
    state.current_path.push_back({x, y});
    state.current_path.push_back({x + w, y});
    state.current_path.push_back({x + w, y + h});
    state.current_path.push_back({x, y + h});
    state.current_path.push_back({x, y});  // Close rectangle
    
    state.path_closed = true;
    state.current_x = x;
    state.current_y = y;
    return true;
  }
  return false;
}

bool flx_pdf_sio::parse_fill_color(const std::string& line, pdf_graphics_state& state) {
  // Format: "r g b rg"
  std::istringstream iss(line);
  double r, g, b;
  std::string op;
  
  if (iss >> r >> g >> b >> op && op == "rg") {
    state.fill_color_r = r;
    state.fill_color_g = g; 
    state.fill_color_b = b;
    return true;
  }
  return false;
}

bool flx_pdf_sio::parse_stroke_color(const std::string& line, pdf_graphics_state& state) {
  // Format: "r g b RG"
  std::istringstream iss(line);
  double r, g, b;
  std::string op;
  
  if (iss >> r >> g >> b >> op && op == "RG") {
    state.stroke_color_r = r;
    state.stroke_color_g = g; 
    state.stroke_color_b = b;
    return true;
  }
  return false;
}

bool flx_pdf_sio::parse_fill_path(pdf_graphics_state& state, flx_model_list<flx_layout_geometry>& geometries) {
  if (state.current_path.size() == 0) return false;
  
  std::cout << "    Creating geometry with " << state.current_path.size() << " vertices" << std::endl;
  
  // Work directly with flx_model_list - proper way!
  geometries.add_element();  // Add new geometry to the list
  auto& geom = geometries.back();  // Get reference to the new geometry
  
  std::cout << "    Added geometry to flx_model_list successfully" << std::endl;
  
  // Now work directly on the geometry in the vector - ADD ALL VERTICES
  try {
    for (const auto& point : state.current_path) {
      geom.vertices.add_element();
      auto& vertex = geom.vertices.back();
      vertex.x = point.first;
      vertex.y = point.second;
    }
    std::cout << "    Added all " << state.current_path.size() << " vertices successfully" << std::endl;
  } catch (const std::exception& e) {
    std::cout << "    ERROR in fill_path: " << e.what() << std::endl;
    return false;
  }
  
  // Convert RGB to hex color
  int r = (int)(state.fill_color_r * 255);
  int g = (int)(state.fill_color_g * 255);
  int b = (int)(state.fill_color_b * 255);
  
  std::ostringstream color_stream;
  color_stream << "#" << std::setfill('0') << std::setw(2) << std::hex << r
               << std::setw(2) << g << std::setw(2) << b;
  geom.fill_color = color_stream.str();
  
  std::cout << "    SUCCESS! Created filled geometry with " << state.current_path.size() 
            << " vertices, color: #FF0000" << std::endl;
  
  return true;
}

bool flx_pdf_sio::parse_stroke_path(pdf_graphics_state& state, flx_model_list<flx_layout_geometry>& geometries) {
  if (state.current_path.size() == 0) return false;
  
  std::cout << "    Creating stroked geometry with " << state.current_path.size() << " vertices" << std::endl;
  
  // Work directly with flx_model_list
  geometries.add_element();
  auto& geom = geometries.back();
  
  // Add all vertices
  for (const auto& point : state.current_path) {
    geom.vertices.add_element();
    auto& vertex = geom.vertices.back();
    vertex.x = point.first;
    vertex.y = point.second;
  }
  
  // Convert RGB to hex color for stroke
  int r = (int)(state.stroke_color_r * 255);
  int g = (int)(state.stroke_color_g * 255);
  int b = (int)(state.stroke_color_b * 255);
  
  std::ostringstream color_stream;
  color_stream << "#" << std::setfill('0') << std::setw(2) << std::hex << r
               << std::setw(2) << g << std::setw(2) << b;
  geom.stroke_color = color_stream.str();
  
  std::cout << "    Created stroked geometry successfully!" << std::endl;
  
  return true;
}

std::unique_ptr<PoDoFo::PdfMemDocument> flx_pdf_sio::create_geometry_only_pdf(flx_model_list<flx_layout_geometry>& geometry_pages) {
  try {
    auto clean_pdf = std::make_unique<PdfMemDocument>();
    
    std::cout << "  Creating clean PDF with " << geometry_pages.size() << " pages of geometry data..." << std::endl;
    
    // Create pages with only geometries (no texts or images)
    for (size_t i = 0; i < geometry_pages.size(); ++i) {
      auto& page_geometry = geometry_pages[i];
      
      // Create PDF page
      auto& pdf_page = clean_pdf->GetPages().CreatePage(PdfPageSize::A4);
      PdfPainter painter;
      painter.SetCanvas(pdf_page);
      
      // Render ONLY the geometry (polygons, shapes) - skip texts and images
      render_geometry_only_to_page(painter, page_geometry);
      
      painter.FinishDrawing();
      std::cout << "    Page " << (i + 1) << ": Rendered geometry shapes" << std::endl;
    }
    
    std::cout << "  Successfully created geometry-only PDF" << std::endl;
    return clean_pdf;
    
  } catch (const std::exception& e) {
    std::cout << "  Error creating geometry-only PDF: " << e.what() << std::endl;
    return nullptr;
  }
}

void flx_pdf_sio::render_geometry_only_to_page(PoDoFo::PdfPainter& painter, flx_layout_geometry& geometry) {
  // Render the geometry's polygon shape if it has vertices
  if (geometry.vertices.size() > 0) {
    render_polygon_shape(painter, geometry);
  }
  
  // Recursively render sub-geometries (ONLY shapes, no texts/images)
  for (size_t i = 0; i < geometry.sub_geometries.size(); ++i) {
    render_geometry_only_to_page(painter, geometry.sub_geometries[i]);
  }
  
  // Skip texts and images completely!
  // This creates a "geometry-only" version of the PDF
}

void flx_pdf_sio::create_debug_directory(const std::string& dir_name) {
  // Create directory using mkdir -p equivalent
  std::string command = "mkdir -p " + dir_name;
  system(command.c_str());
}

void flx_pdf_sio::clear() {
  // Clear the PDF document
  if (m_pdf != nullptr) {
    delete m_pdf;
    m_pdf = nullptr;
  }
  
  // Clear all pages and their contents
  pages.clear();
  
  // CRITICAL: Clear static font cache in text extractor to avoid stale font pointers
  clear_static_font_cache();
  
  std::cout << "PDF processor cleared and memory released" << std::endl;
}

void flx_pdf_sio::clear_static_font_cache() {
  flx_pdf_text_extractor::clear_font_cache();
}

