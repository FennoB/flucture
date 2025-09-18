#ifndef flx_PDF_SIO_H
#define flx_PDF_SIO_H

#include "../flx_doc_sio.h"
#include "../layout/flx_layout_geometry.h"
#include "flx_pdf_coords.h"
#include <vector>
#include <memory>
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

private:
  // Internal rendering methods
  void render_geometry_to_page(PoDoFo::PdfPainter& painter, flx_layout_geometry& geometry);
  void render_polygon_shape(PoDoFo::PdfPainter& painter, flx_layout_geometry& geometry);
  void render_text_element(PoDoFo::PdfPainter& painter, flx_layout_text& text_elem);
  void render_image_element(PoDoFo::PdfPainter& painter, flx_layout_image& image_elem);
  
  // Image processing helpers
  bool load_image_from_path(flx_layout_image& image_elem);
  std::unique_ptr<PoDoFo::PdfImage> create_pdf_image_from_mat(flx_layout_image& image_elem);
  
  // PDF parsing methods
  std::unique_ptr<PoDoFo::PdfMemDocument> create_pdf_copy();
  bool extract_texts_and_images(std::vector<flx_layout_text>& texts, std::vector<flx_layout_image>& images);
  bool remove_texts_and_images_from_copy(PoDoFo::PdfMemDocument* pdf_copy);
  bool render_clean_pdf_to_images(PoDoFo::PdfMemDocument* pdf_copy, std::vector<cv::Mat>& clean_images);
  std::vector<cv::Mat> detect_color_regions(const cv::Mat& page_image);
  std::vector<std::vector<cv::Point>> extract_contours_from_masks(const std::vector<cv::Mat>& masks);
  void build_geometry_hierarchy(const std::vector<std::vector<std::vector<cv::Point>>>& page_contours,
                              const std::vector<cv::Mat>& clean_images,
                              flx_model_list<flx_layout_geometry>& geometries);
  void assign_content_to_geometries(std::vector<flx_layout_text>& texts, 
                                  std::vector<flx_layout_image>& images,
                                  std::vector<flx_layout_geometry>& geometries);
  void assign_content_to_geometries_flx(std::vector<flx_layout_text>& texts, 
                                       std::vector<flx_layout_image>& images,
                                       flx_model_list<flx_layout_geometry>& geometries);
  
  // PDF content stream filtering
  std::string filter_pdf_content_stream(const std::string& content);
  bool replace_page_content_stream(PoDoFo::PdfPage& page, const std::string& new_content);
  
  // Flood-fill algorithm helpers
  int flood_fill_neighbor_based(const cv::Mat& image, cv::Point seed, cv::Mat& region_mask, cv::Mat& processed, int tolerance);
  bool is_color_similar(const cv::Vec3b& color1, const cv::Vec3b& color2, int tolerance);
  
  // Geometry processing helpers
  cv::Scalar calculate_dominant_color_for_contour(const std::vector<cv::Point>& contour, 
                                                const std::vector<cv::Mat>& clean_images, 
                                                size_t contour_index);
  std::string rgb_to_hex_string(const cv::Scalar& color);
  bool is_geometry_contained_in(const flx_layout_geometry& inner, const flx_layout_geometry& outer);
  void assign_content_to_geometry_recursive(flx_layout_text& text, std::vector<flx_layout_geometry>& geometries);
  void assign_content_to_geometry_recursive(flx_layout_image& image, std::vector<flx_layout_geometry>& geometries);
  void assign_content_to_geometry_recursive_flx(flx_layout_text& text, flx_model_list<flx_layout_geometry>& geometries);
  void assign_content_to_geometry_recursive_flx(flx_layout_image& image, flx_model_list<flx_layout_geometry>& geometries);
  bool is_point_in_geometry(double x, double y, const flx_layout_geometry& geometry);
};

#endif // flx_PDF_SIO_H
