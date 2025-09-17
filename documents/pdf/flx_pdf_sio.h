#ifndef flx_PDF_SIO_H
#define flx_PDF_SIO_H

#include "../flx_doc_sio.h"
#include "../../utils/flx_geometry.h"
#include "../../utils/flx_text_element.h"
#include "../../utils/flx_layout_element.h"

namespace PoDoFo
{
  class PdfMemDocument;
}
namespace cv
{
  class Mat;
}
class flx_pdf_sio : public flx_doc_sio
{
  PoDoFo::PdfMemDocument *m_pdf;
  flx_string data;
  bool pdf2images(std::vector<cv::Mat>& output_mats, int dpi = 300);

public:
  flx_pdf_sio();

  bool parse(flx_string &data);
  bool serialize(flx_string &data);
  bool add_text(flx_string text, flx_point position);
  bool extract_texts(flx_model_list<flx_text_element> &texts);
  bool extract_contents(flx_string &data);
  void draw_text_boxes_on_images(std::vector<cv::Mat> &images, const std::vector<flx_text_element> &text_elements, double dpi, int box_size_px = 10);
  bool extract_layout_from_page(
    const cv::Mat& page_image,                      // Das Bild der aktuellen Seite
    int current_page_index,                         // 0-basierter Index dieser Seite
    const std::vector<flx_text_element>& all_texts, // Alle Textelemente des Dokuments
    double dpi,                                      // Die DPI-Einstellung,
    std::vector<flx_layout_element> &layout_elements // Ausgabearray für die Layout-Elemente
    );
  bool extract_full_layout(const std::vector<cv::Mat> &images, const std::vector<flx_text_element> &all_text_elements, double dpi, std::vector<flx_layout_element> &document_pages);
};

#endif // flx_PDF_SIO_H
