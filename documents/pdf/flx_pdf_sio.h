#ifndef flx_PDF_SIO_H
#define flx_PDF_SIO_H

#include "../flx_doc_sio.h"
#include "../layout/flx_layout_geometry.h"
#include "flx_pdf_coords.h"
#include <vector>
#include <memory>
#include <stack>
#include <opencv2/opencv.hpp>

namespace PoDoFo { 
  class PdfMemDocument; 
  class PdfPainter;
  class PdfImage;
  class PdfPage;
}
namespace cv { class Mat; }

class flx_pdf_sio : public flx_doc_sio
{
private:
  PoDoFo::PdfMemDocument *m_pdf;
  flx_string pdf_data;
  std::vector<char> pdf_data_buffer;  // Stable buffer for LoadFromBuffer (XObject compatibility!)

public:
  flx_pdf_sio();
  ~flx_pdf_sio();

  // Core document operations
  bool parse(flx_string &data) override;
  bool serialize(flx_string &data) override;

  // PDF rendering
  bool render(std::vector<cv::Mat>& output_images, int dpi = 300);

  // Text operations
  bool add_text(flx_string text, double x, double y);

  // Memory cleanup
  void clear();
  void clear_static_font_cache();

private:
  // Internal rendering methods
  void render_geometry_to_page(PoDoFo::PdfPainter& painter, flx_layout_geometry& geometry);
  void render_geometry_only_to_page(PoDoFo::PdfPainter& painter, flx_layout_geometry& geometry);
  void render_polygon_shape(PoDoFo::PdfPainter& painter, flx_layout_geometry& geometry);
  void render_text_element(PoDoFo::PdfPainter& painter, flx_layout_text& text_elem);
  void render_image_element(PoDoFo::PdfPainter& painter, flx_layout_image& image_elem);
  
  // Image processing helpers
  bool load_image_from_path(flx_layout_image& image_elem);
  std::unique_ptr<PoDoFo::PdfImage> create_pdf_image_from_mat(flx_layout_image& image_elem);
  
  
  // Geometry-only PDF creation
  std::unique_ptr<PoDoFo::PdfMemDocument> create_geometry_only_pdf(flx_model_list<flx_layout_geometry>& geometry_pages);
  
  // Debug helpers
  void create_debug_directory(const std::string& dir_name);
  
  // PDF parsing methods
  std::unique_ptr<PoDoFo::PdfMemDocument> create_pdf_copy();
  bool extract_texts_and_images(flx_model_list<flx_layout_text>& texts, flx_model_list<flx_layout_image>& images);
  bool remove_texts_and_images_from_copy(PoDoFo::PdfMemDocument* pdf_copy);
  bool render_clean_pdf_to_images(PoDoFo::PdfMemDocument* pdf_copy, std::vector<cv::Mat>& clean_images);
  std::vector<cv::Mat> detect_color_regions(const cv::Mat& page_image, const std::string& debug_dir = "");
  std::vector<std::vector<cv::Point>> extract_contours_from_masks(const std::vector<cv::Mat>& masks, const cv::Mat& original_image = cv::Mat(), const std::string& debug_dir = "");
  void build_geometry_hierarchy(const std::vector<std::vector<std::vector<cv::Point>>>& page_contours,
                              const std::vector<cv::Mat>& clean_images,
                              flx_model_list<flx_layout_geometry>& geometries);
  void assign_content_to_geometries(flx_model_list<flx_layout_text>& texts, 
                                  flx_model_list<flx_layout_image>& images,
                                  flx_model_list<flx_layout_geometry>& geometries);
  void assign_content_to_geometries_flx(flx_model_list<flx_layout_text>& texts, 
                                       flx_model_list<flx_layout_image>& images,
                                       flx_model_list<flx_layout_geometry>& geometries);
  
  // PDF content stream filtering  
  std::string filter_pdf_content_stream(const std::string& content);
  bool replace_page_content_stream(PoDoFo::PdfPage& page, const std::string& new_content);
  
  // NEW: Direct PDF content stream geometry extraction
  bool extract_geometries_from_content_streams(flx_model_list<flx_layout_geometry>& all_geometries);
  bool parse_content_stream_for_geometries(const std::string& content, flx_model_list<flx_layout_geometry>& geometries);
  
  // PDF Graphics State tracking
  struct pdf_graphics_state {
    double fill_color_r, fill_color_g, fill_color_b;
    double stroke_color_r, stroke_color_g, stroke_color_b;
    double current_x, current_y;  // Current point for path operations
    std::vector<std::pair<double, double>> current_path;  // Simple x,y pairs for now
    bool path_closed;
    
    pdf_graphics_state() : fill_color_r(0), fill_color_g(0), fill_color_b(0),
                          stroke_color_r(0), stroke_color_g(0), stroke_color_b(0), 
                          current_x(0), current_y(0), path_closed(false) {}
  };
  
  // PDF operator parsing helpers
  bool parse_move_to(const std::string& line, pdf_graphics_state& state);
  bool parse_line_to(const std::string& line, pdf_graphics_state& state);
  bool parse_curve_to(const std::string& line, pdf_graphics_state& state);
  bool parse_close_path(pdf_graphics_state& state);
  bool parse_rectangle(const std::string& line, pdf_graphics_state& state);
  bool parse_fill_color(const std::string& line, pdf_graphics_state& state);
  bool parse_stroke_color(const std::string& line, pdf_graphics_state& state);
  bool parse_fill_path(pdf_graphics_state& state, flx_model_list<flx_layout_geometry>& geometries);
  bool parse_stroke_path(pdf_graphics_state& state, flx_model_list<flx_layout_geometry>& geometries);
  
  // Flood-fill algorithm helpers
  int flood_fill_neighbor_based(const cv::Mat& image, cv::Point seed, cv::Mat& region_mask, cv::Mat& processed, int tolerance);
  bool is_color_similar(const cv::Vec3b& color1, const cv::Vec3b& color2, int tolerance);
  
  // Geometry processing helpers
  cv::Scalar calculate_dominant_color_for_contour(const std::vector<cv::Point>& contour, 
                                                const std::vector<cv::Mat>& clean_images, 
                                                size_t contour_index);
  bool is_contour_line_shaped(const std::vector<cv::Point>& contour, double max_width = 5.0);
  
  // Line detection using Hough Transform
  flx_model_list<flx_layout_geometry> detect_lines_hough(const cv::Mat& image, const std::string& debug_dir = "");
  cv::Scalar calculate_dominant_color_for_contour_from_image(const std::vector<cv::Point>& contour, const cv::Mat& image);
  std::string rgb_to_hex_string(const cv::Scalar& color);
  bool is_geometry_contained_in(const flx_layout_geometry& inner, const flx_layout_geometry& outer);
  void assign_content_to_geometry_recursive(flx_layout_text& text, flx_model_list<flx_layout_geometry>& geometries);
  void assign_content_to_geometry_recursive(flx_layout_image& image, flx_model_list<flx_layout_geometry>& geometries);
  void assign_content_to_geometry_recursive_flx(flx_layout_text& text, flx_model_list<flx_layout_geometry>& geometries);
  void assign_content_to_geometry_recursive_flx(flx_layout_image& image, flx_model_list<flx_layout_geometry>& geometries);
  bool is_point_in_geometry(double x, double y, const flx_layout_geometry& geometry);
  void build_hierarchical_structure(flx_model_list<flx_layout_geometry>& geometries);
};

#endif // flx_PDF_SIO_H
