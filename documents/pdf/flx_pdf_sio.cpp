#include "flx_pdf_sio.h"
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
    return true;
  } catch (const std::exception& e) {
    std::cerr << "Error parsing PDF: " << e.what() << std::endl;
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
  auto* font = m_pdf->GetFonts().SearchFont("Arial");
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