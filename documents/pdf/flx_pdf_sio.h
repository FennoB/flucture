#ifndef flx_PDF_SIO_H
#define flx_PDF_SIO_H

#include "../flx_doc_sio.h"
#include "../layout/flx_layout_geometry.h"
#include "flx_pdf_coords.h"
#include <vector>
#include <memory>

namespace PoDoFo { 
  class PdfMemDocument; 
  class PdfPainter;
  class PdfImage;
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
};

#endif // flx_PDF_SIO_H
