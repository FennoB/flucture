// In einer passenden Header-Datei (z.B. flx_geometry.h oder eine neue flx_coords.h)
#ifndef FLX_PDF_COORDS_H 
#define FLX_PDF_COORDS_H
#include <stdexcept>

namespace flx_coords { // Oder dein Namespace
  /**
   * @brief Konvertiert eine PDF-Koordinate (in Punkten) in eine PNG-Pixelkoordinate.
   * @param pdf_coord Die Koordinate in PDF-Punkten (1/72 Zoll).
   * @param dpi Die DPI (Dots Per Inch), mit der das PDF gerendert wurde/wird.
   * @return Die entsprechende Koordinate in Pixeln.
   */
  inline double pdf_to_png_coord(double pdf_coord, double dpi) {
    if (dpi <= 0) {
      // Fehlerfall oder Standardwert zurückgeben, hier werfen wir einen Fehler
      // In einer echten Anwendung ggf. bessere Fehlerbehandlung
      throw std::invalid_argument("DPI muss positiv sein.");
    }
    return (pdf_coord * dpi) / 72.0;
  }

  /**
   * @brief Konvertiert eine PNG-Pixelkoordinate in eine PDF-Koordinate (in Punkten).
   * @param png_coord Die Koordinate in Pixeln.
   * @param dpi Die DPI (Dots Per Inch), mit der das PDF gerendert wurde.
   * @return Die entsprechende Koordinate in PDF-Punkten (1/72 Zoll).
   */
  inline double png_to_pdf_coord(double png_coord, double dpi) {
    if (dpi <= 0) {
      throw std::invalid_argument("DPI muss positiv sein.");
    }
    return (png_coord * 72.0) / dpi;
  }
} // namespace flx

#endif // FLX_PDF_COORDS_H

