#ifndef FLX_PDF_COORDS_H 
#define FLX_PDF_COORDS_H
#include <stdexcept>

namespace flx_coords {
  inline double pdf_to_png_coord(double pdf_coord, double dpi) {
    if (dpi <= 0) {
      throw std::invalid_argument("DPI must be positive");
    }
    return (pdf_coord * dpi) / 72.0;
  }

  inline double png_to_pdf_coord(double png_coord, double dpi) {
    if (dpi <= 0) {
      throw std::invalid_argument("DPI must be positive");
    }
    return (png_coord * 72.0) / dpi;
  }
}

#endif // FLX_PDF_COORDS_H