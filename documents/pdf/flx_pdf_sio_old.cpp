#include "flx_pdf_sio.h"
#include "flx_pdf_coords.h"
#include <main/PdfPainter.h>
#include <iostream>
#include <main/PdfMemDocument.h>
#include <string>
#include <vector>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <filesystem>
#include <chrono>
#include <opencv2/opencv.hpp> // Für cv::Mat und cv::imread

using namespace PoDoFo;

// Forward declaration or include for PoDoFo::PdfMemDocument
// namespace PoDoFo { class PdfMemDocument; }

// Annahme: flx_pdf_sio Klasse mit Membern:
// flx_string data;
// PoDoFo::PdfMemDocument *m_pdf;

bool flx_pdf_sio::pdf2images(std::vector<cv::Mat>& output_mats, int dpi)
{
  output_mats.clear();

  long long timestamp = std::chrono::steady_clock::now().time_since_epoch().count();
  std::filesystem::path temp_dir_path = std::filesystem::temp_directory_path() / ("flx_pdf_conversion_" + std::to_string(timestamp));

  try {
    if (!std::filesystem::create_directories(temp_dir_path)) {
      std::cerr << "Fehler: Konnte temporäres Verzeichnis nicht erstellen: " << temp_dir_path << std::endl;
        return false;
    }
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Fehler beim Erstellen des temporären Verzeichnisses: " << temp_dir_path << " :: " << e.what() << std::endl;
      return false;
  }

  std::filesystem::path temp_pdf_filepath = temp_dir_path / "input.pdf";

  std::ofstream outfile(temp_pdf_filepath, std::ios::binary);
  if (!outfile.is_open()) {
    std::cerr << "Fehler: Konnte temporäre PDF-Datei nicht öffnen: " << temp_pdf_filepath << std::endl;
    std::filesystem::remove_all(temp_dir_path);
    return false;
  }
  outfile.write(this->data.c_str(), this->data.size());
  outfile.close();
  if (!outfile) {
    std::cerr << "Fehler: Konnte nicht in temporäre PDF-Datei schreiben: " << temp_pdf_filepath << std::endl;
    std::filesystem::remove_all(temp_dir_path);
    return false;
  }

  std::string output_pattern_png = (temp_dir_path / "page_%d.png").string();

  std::string command = "mutool draw -r " + std::to_string(dpi) +
                        " -o \"" + output_pattern_png + "\" \"" +
                        temp_pdf_filepath.string() + "\"";

  int system_result = system(command.c_str());

  if (system_result != 0) {
    std::cerr << "Fehler bei der Ausführung von mutool (Exit-Code: " << system_result << ")" << std::endl;
    std::filesystem::remove_all(temp_dir_path);
    return false;
  }

  int page_count = this->m_pdf->GetPages().GetCount();

  for (int i = 1; i <= page_count; ++i) {
    std::filesystem::path current_image_path = temp_dir_path / ("page_" + std::to_string(i) + ".png");
    if (std::filesystem::exists(current_image_path)) {
      cv::Mat image = cv::imread(current_image_path.string(), cv::IMREAD_COLOR);
      if (!image.empty()) {
        output_mats.push_back(image);
      } else {
        std::cerr << "Warnung: Konnte Bilddatei nicht laden oder sie ist leer: " << current_image_path << std::endl;
      }
    } else {
      std::cerr << "Warnung: Erwartete Bilddatei nicht gefunden: " << current_image_path << std::endl;
    }
  }

  try {
    std::filesystem::remove_all(temp_dir_path);
  } catch (const std::filesystem::filesystem_error& e) {
    std::cerr << "Warnung: Fehler beim Löschen des temporären Verzeichnisses: " << temp_dir_path << " :: " << e.what() << std::endl;
  }

  if (output_mats.empty() && page_count > 0) {
    std::cerr << "Keine Bilder erfolgreich in cv::Mat geladen, obwohl Seiten erwartet wurden." << std::endl;
    return false;
  }

  if (output_mats.size() != static_cast<size_t>(page_count) && page_count > 0) {
    std::cerr << "Warnung: Anzahl der geladenen Bilder (" << output_mats.size()
              << ") stimmt nicht mit der Seitenzahl im PDF (" << page_count << ") überein." << std::endl;
  }

  std::cout << output_mats.size() << " Bild(er) erfolgreich in cv::Mat geladen und temporäres Verzeichnis gelöscht." << std::endl;
  return true;
}

flx_pdf_sio::flx_pdf_sio()
{
  m_pdf = new PdfMemDocument();
}

bool flx_pdf_sio::parse(flx_string &data)
{
  this->data = data;
  bufferview buffer(this->data.c_str(), this->data.size());
  if (buffer.empty())
  {
    return false;
  }
  m_pdf->LoadFromBuffer(buffer);
  if (m_pdf->GetPages().GetCount() > 0) {
    std::cout << "PDF erfolgreich geparst, " << m_pdf->GetPages().GetCount() << " Seiten gefunden." << std::endl;
    return true;
  } else {
    std::cout << "PDF geparst, aber keine Seiten gefunden oder Ladefehler." << std::endl;
    return false;
  }
  return false;
}

bool flx_pdf_sio::serialize(flx_string &data)
{
  if (m_pdf->GetPages().GetCount())
  {
    // Save the PDF document to a string
    StringStreamDevice device(data.to_std());
    m_pdf->Save(device);
    return true;
  }
  return false;
}

bool flx_pdf_sio::add_text(flx_string text, flx_point position)
{
  if (m_pdf->GetPages().GetCount() == 0)
  {
    return false;
  }
  PdfPage& page = m_pdf->GetPages().GetPageAt(0);
  PdfPainter painter;
  painter.SetCanvas(page);
  PdfFontManager& fm = m_pdf->GetFonts();
  PdfFont *font = fm.SearchFont("Arial");
  painter.TextState.SetFont(*font, 16);
  painter.TextObject.Begin();
  painter.TextObject.MoveTo(position.get_bb_x(), position.get_bb_y());
  painter.TextObject.AddText(text.to_std());
  painter.TextObject.End();
  return true;
}

bool flx_pdf_sio::extract_texts(flx_model_list<flx_text_element> &texts)
{
  if (!m_pdf || m_pdf->GetPages().GetCount() == 0) {
    // Kein Dokument geladen oder keine Seiten vorhanden
    return false;
  }

  for (size_t i = 0; i < m_pdf->GetPages().GetCount(); ++i) {
    PoDoFo::PdfPage& page = m_pdf->GetPages().GetPageAt(i); // GetPage(i) statt GetPageAt(i) für non-const

    std::vector<PoDoFo::PdfTextEntry> page_text_entries;
    page.ExtractTextTo(page_text_entries);

    for (auto& entry : page_text_entries) {
      flx_text_element text_element;
      text_element.text = flx_string(entry.Text);
      text_element.x = entry.X;
      text_element.y = entry.Y;
      text_element.page = i; // Seite, auf der der Text gefunden wurde
      texts.push_back(text_element);
    }
  }
  return true;
}

void flx_pdf_sio::draw_text_boxes_on_images(
  std::vector<cv::Mat>& images, // Referenz, da wir die Bilder modifizieren
  const std::vector<flx_text_element>& text_elements,
  double dpi,
  int box_size_px)
{
  if (images.empty()) {
    std::cerr << "Keine Bilder zum Zeichnen vorhanden." << std::endl;
    return;
  }

  for (const auto& text_el : text_elements) {
    // Prüfe, ob der page_index gültig ist
    if (text_el.page < 0 || static_cast<size_t>(text_el.page) >= images.size()) {
      std::cerr << "Ungültiger page_index (" << text_el.page
                      << ") für Text: '" << text_el.text.c_str() << "'" << std::endl;
        continue;
    }

    cv::Mat& current_image = images[text_el.page];
    if (current_image.empty()) {
      std::cerr << "Bild für Seite " << text_el.page << " ist leer." << std::endl;
        continue;
    }

    double page_height_px = static_cast<double>(current_image.rows);

           // Konvertiere PDF-Koordinaten (Ursprung unten links des Textes) in PNG-Koordinaten
    double png_x_bl = flx_coords::pdf_to_png_coord(text_el.x, dpi);

    double pdf_y_bl_scaled_from_bottom_px = flx_coords::pdf_to_png_coord(text_el.y, dpi);
    double png_y_bl_from_top = page_height_px - pdf_y_bl_scaled_from_bottom_px;

           // Definiere den Kasten: (png_x_bl, png_y_bl_from_top) ist die untere linke Ecke des Texts.
           // Wir zeichnen einen Kasten, dessen untere linke Ecke dieser Punkt ist.
    cv::Point cv_pt_bottom_left(
      static_cast<int>(std::round(png_x_bl)),
      static_cast<int>(std::round(png_y_bl_from_top))
      );
    cv::Point cv_pt_top_right(
      static_cast<int>(std::round(png_x_bl + box_size_px)),
      static_cast<int>(std::round(png_y_bl_from_top - box_size_px)) // Y-Koordinate nach oben im Bild
      );

    cv::rectangle(current_image, cv_pt_bottom_left, cv_pt_top_right, cv::Scalar(0, 0, 255), 1); // Roter Kasten, 1px Dicke
  }
  std::cout << "Kästen um Texte gezeichnet." << std::endl;
}

void display_images(
  const cv::Mat& image,
  double scale_factor = 1.0, flx_string window_name = "Bildanzeige"
  ) {
  cv::Mat displayed_image;
  if (scale_factor == 1.0) {
    displayed_image = image; // Keine Skalierung nötig
  } else if (scale_factor > 0) {
    cv::resize(image, displayed_image, cv::Size(), scale_factor, scale_factor, cv::INTER_AREA);
    // cv::INTER_AREA ist gut zum Verkleinern.
    // cv::Size() bedeutet, dass die Größe aus fx und fy (scale_factor) berechnet wird.
  } else {
    std::cout << "Ungültiger Skalierungsfaktor: " << scale_factor << ". Zeige Originalgröße." << std::endl;
    displayed_image = image;
  }

  if (displayed_image.empty()){
    std::cout << "Fehler beim Skalieren " << std::endl;
    return;
  }

  cv::imshow(window_name.to_std(), displayed_image);

  std::cout << "Zeige Bild: " << window_name.to_std() << ". Drücke eine beliebige Taste im Bildfenster, um fortzufahren..." << std::endl;
    int key = cv::waitKey(0); // Wartet unendlich auf einen Tastendruck

  cv::destroyWindow(window_name.to_std());

  if (key == 27) { // 27 ist der ASCII-Code für die ESC-Taste
    std::cout << "ESC gedrückt, Anzeige wird abgebrochen." << std::endl;
      return; // Schleife abbrechen, wenn ESC gedrückt wird
  }
}

void display_images(
  const std::vector<cv::Mat>& images,
  double scale_factor = 1.0 // 1.0 = Originalgröße, 0.5 = halbe Größe, etc.
  ) {
  if (images.empty()) {
    std::cout << "Keine Bilder zum Anzeigen vorhanden." << std::endl;
    return;
  }

  for (size_t i = 0; i < images.size(); ++i) {
    if (images[i].empty()) {
      std::cout << "Bild " << i + 1 << " ist leer und kann nicht angezeigt werden." << std::endl;
      continue;
    }
    std::string window_name = "Bild " + std::to_string(i + 1) + " von " + std::to_string(images.size());
    display_images(images[i], scale_factor, window_name);
  }
  // cv::destroyAllWindows(); // Falls noch Fenster offen sein sollten
}

bool flx_pdf_sio::extract_layout_from_page(const cv::Mat &page_image, int current_page_index, const std::vector<flx_text_element> &all_texts, double dpi, std::vector<flx_layout_element> &layout_elements)
{
  if (page_image.empty()) {
    return false;
  }

  // 1. Bildvorverarbeitung (wie von dir angedacht: Graustufen, Sobel, Gradienten-Threshold)
  cv::Mat gray, grad_x, grad_y, abs_grad_x, abs_grad_y, grad_magnitude, binary_edges;
  cv::cvtColor(page_image, gray, cv::COLOR_BGR2GRAY);

  cv::Sobel(gray, grad_x, CV_16S, 1, 0);
  cv::Sobel(gray, grad_y, CV_16S, 0, 1);
  cv::convertScaleAbs(grad_x, abs_grad_x);
  cv::convertScaleAbs(grad_y, abs_grad_y);
  cv::addWeighted(abs_grad_x, 1, abs_grad_y, 1, 0, grad_magnitude);
  double gradient_threshold = 0; // Diesen Wert musst du experimentell ermitteln!
  cv::threshold(grad_magnitude, binary_edges, gradient_threshold, 255, cv::THRESH_BINARY);
  // Bild anzeigen

  display_images(binary_edges, 0.2, "Binary Edges"); // Zeige die binären Kanten

  // Optional: Morphologische Operationen, um Linien zu schließen / Rauschen zu entfernen
  cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(3,3));
  cv::morphologyEx(binary_edges, binary_edges, cv::MORPH_CLOSE, kernel, cv::Point(-1,-1), 2); // 2 Iterationen z.B.

  // 2. Konturen finden (potenzielle Boxen)
  std::vector<std::vector<cv::Point>> contours;
  std::vector<cv::Vec4i> hierarchy; // Für spätere hierarchische Analyse
  cv::findContours(binary_edges, contours, hierarchy, cv::RETR_TREE, cv::CHAIN_APPROX_SIMPLE);

  double page_height_png = static_cast<double>(page_image.rows);

  for (const auto& contour : contours) {
    std::vector<cv::Point> approx_poly;
    cv::approxPolyDP(contour, approx_poly, cv::arcLength(contour, true) * 0.00, true);

    if (/*approx_poly.size() == 4 && */cv::isContourConvex(approx_poly)) {
      cv::Rect png_bounding_rect = cv::boundingRect(approx_poly); // Oben-Links Ursprung (PNG)

      // Konvertiere PNG-Rechteckkoordinaten in PDF-Koordinaten (Unten-Links Ursprung)

      // X-Koordinate und Breite sind direkte Skalierungen
      double pdf_x = flx_coords::png_to_pdf_coord(static_cast<double>(png_bounding_rect.x), dpi);
      double pdf_width = flx_coords::png_to_pdf_coord(static_cast<double>(png_bounding_rect.width), dpi);

      // Höhe ist auch eine direkte Skalierung
      double pdf_height = flx_coords::png_to_pdf_coord(static_cast<double>(png_bounding_rect.height), dpi);

      // Y-Koordinate (untere Kante des Rechtecks, von PDF-Seitenunterkante gemessen):
      // 1. Y-Position der unteren Kante des Rechtecks in PNG-Pixeln (von oben gemessen):
      double png_y_bottom_edge_from_top_px = static_cast<double>(png_bounding_rect.y + png_bounding_rect.height);

      // 2. Y-Position der unteren Kante des Rechtecks in PNG-Pixeln (von unten gemessen):
      double png_y_bottom_edge_from_bottom_px = page_height_png - png_y_bottom_edge_from_top_px;

      // 3. Konvertiere diesen Abstand von unten (in PNG-Pixeln) in PDF-Punkte.
      //    Dies ist die pdf_y Koordinate (untere Kante des Layout-Elements von der PDF-Seitenunterkante).
      double pdf_y = flx_coords::png_to_pdf_coord(png_y_bottom_edge_from_bottom_px, dpi);

      if (pdf_width < 10 || pdf_height < 10)
        continue;
      // Erstelle das Layout-Element mit PDF-Koordinaten (x, y ist untere linke Ecke)
      flx_layout_element layout_el;
      layout_el.page = current_page_index;
      layout_el.x = pdf_x;
      layout_el.y = pdf_y;
      layout_el.width = pdf_width;
      layout_el.height = pdf_height;

      // Textelemente diesem Layout-Element zuordnen
      for (const auto& text_el : all_texts) {
        if (text_el.page != current_page_index) continue;

        // Prüfe, ob der Startpunkt des Textes (text_el.x, text_el.y in PDF-Koordinaten, Ursprung unten links)
        // innerhalb des PDF-Bounding-Box des Layout-Elements liegt.
        bool x_matches = (text_el.x >= layout_el.x) && (text_el.x < (layout_el.x + layout_el.width));
        bool y_matches = (text_el.y >= layout_el.y) && (text_el.y < (layout_el.y + layout_el.height));

        if (x_matches && y_matches) {
          layout_el.text_elements.push_back(text_el);
        }
      }
      if (!layout_el.text_elements.size() && layout_el.width > 0 && layout_el.height > 0) { // Nur hinzufügen, wenn Text drin oder Größe hat
        layout_elements.push_back(layout_el);
      }
    }
  }
  // TODO: Hierarchische Struktur aus `hierarchy` ableiten, falls nötig (für sub_elements)
  // TODO: Linien explizit erkennen und zu Layout-Elementen gruppieren (z.B. Tabellen)

  return !layout_elements.empty();
}

// Eine Wrapper-Funktion, die über alle Seiten iteriert:
bool flx_pdf_sio::extract_full_layout(
  const std::vector<cv::Mat>& images,
  const std::vector<flx_text_element>& all_text_elements,
  double dpi, std::vector<flx_layout_element>& document_pages
  ) {
  for (size_t i = 0; i < images.size(); ++i) {
    // Filtere all_text_elements für die aktuelle Seite i (optional, wenn extract_layout_from_page das macht)
    std::vector<flx_layout_element> page_elements;
    extract_layout_from_page(images[i], i, all_text_elements, dpi, page_elements);
    document_pages.push_back(flx_layout_element());
    auto el = document_pages.back().sub_elements;
    el->insert(el->end(), page_elements.begin(), page_elements.end());
  }
  return true;
}


void draw_layout_elements_on_images(
  std::vector<cv::Mat>& images, // Referenz, da wir die Bilder modifizieren
  const std::vector<flx_layout_element>& layout_elements,
  double dpi,
  const cv::Scalar& color = cv::Scalar(0, 0, 255) // Standardfarbe Grün (BGR)
  ) {
  if (images.empty()) {
    std::cerr << "Keine Bilder zum Zeichnen der Layout-Elemente vorhanden." << std::endl;
    return;
  }

  for (const auto& layout_el : layout_elements) {
    // Prüfe, ob der page_index gültig ist
    if (layout_el.page < 0 || static_cast<size_t>(layout_el.page) >= images.size()) {
      std::cerr << "Ungültiger page_index (" << layout_el.page
                      << ") für Layout-Element." << std::endl; // Ggf. mehr Infos zum Element loggen
      continue;
    }

    cv::Mat& current_image = images[layout_el.page];
    if (current_image.empty()) {
      std::cerr << "Bild für Seite " << layout_el.page << " ist leer." << std::endl;
        continue;
    }

    double page_height_png = static_cast<double>(current_image.rows);

           // layout_el.x, layout_el.y, layout_el.width, layout_el.height sind in PDF-Koordinaten
           // layout_el.y ist die untere Kante des Elements, von der PDF-Seitenunterkante gemessen.

           // Konvertiere die PDF-Koordinaten des Layout-Elements in PNG-Pixelkoordinaten
           // für cv::rectangle (benötigt obere linke Ecke und untere rechte Ecke, oder obere linke Ecke + Breite/Höhe)

           // 1. Obere linke X-Koordinate in PNG-Pixeln
    double png_x_top_left = flx_coords::pdf_to_png_coord(layout_el.x, dpi);

           // 2. Breite und Höhe in PNG-Pixeln
    double png_width = flx_coords::pdf_to_png_coord(layout_el.width, dpi);
    double png_height = flx_coords::pdf_to_png_coord(layout_el.height, dpi);

           // 3. Obere linke Y-Koordinate in PNG-Pixeln
           //    layout_el.y ist die PDF Y-Koordinate der *unteren* Kante des Rechtecks (von PDF-Unten)
           //    Die Y-Koordinate der *oberen* Kante des Rechtecks (von PDF-Unten) ist layout_el.y + layout_el.height
    double pdf_y_top_edge_from_bottom = layout_el.y + layout_el.height;

    //    Konvertiere diese in PNG-Pixel (Abstand von PNG-Unten)
    double png_y_top_edge_scaled_from_bottom = flx_coords::pdf_to_png_coord(pdf_y_top_edge_from_bottom, dpi);

    //    Konvertiere zu Abstand von PNG-Oben (das ist die Y-Koordinate für cv::rectangle)
    double png_y_top_left = page_height_png - png_y_top_edge_scaled_from_bottom;

           // Definiere die Punkte für cv::rectangle
    cv::Point pt1(
      static_cast<int>(std::round(png_x_top_left)),
      static_cast<int>(std::round(png_y_top_left))
      );
    cv::Point pt2(
      static_cast<int>(std::round(png_x_top_left + png_width)),
      static_cast<int>(std::round(png_y_top_left + png_height))
      );

    cv::rectangle(current_image, pt1, pt2, color, 2); // Zeichne das Rechteck, Dicke 2

           // Optional: Zeichne auch die enthaltenen Textelemente (mit ihren roten Kästen)
           // oder eine ID des Layout-Elements etc.
           // for (const auto& text_el : layout_el.contained_text_elements) { ... }
  }
  std::cout << "Layout-Elemente auf Bildern gezeichnet." << std::endl;
}

bool flx_pdf_sio::extract_contents(flx_string &data)
{
  std::vector<cv::Mat> images;
  if (pdf2images(images, 300))
  {
    flx_model_list<flx_text_element> texts;
    extract_texts(texts);
    std::vector<flx_layout_element> doc_pages;
    extract_full_layout(images, texts, 300, doc_pages);


    // lets now output the contents of doc_pages to the console
    for (const auto& page : doc_pages) {
      std::cout << "Seite " << page.page << ": " << std::endl;
      // Loop over sub_elements
      for (const auto& sub_element : *page.sub_elements) {
        std::cout << "  Layout-Element: " << sub_element.x << ", " << sub_element.y
                  << ", " << sub_element.width << ", " << sub_element.height
                  << ", Text-Elemente: " << sub_element.text_elements.size() << std::endl;
        for (const auto& text_el : sub_element.text_elements) {
          std::cout << "    Text: '" << text_el.text.c_str() << "' bei ("
                    << text_el.x << ", " << text_el.y << ")"
                    << " auf Seite " << text_el.page << std::endl;
        }
      }
    }


    draw_layout_elements_on_images(images, *doc_pages[0].sub_elements, 300);
    draw_layout_elements_on_images(images, *doc_pages[1].sub_elements, 300);

    // Zeige die Bilder mit den gezeichneten Layout-Elementen an
    display_images(images, 0.2);
    return true;
  }
  std::cout << "Failed to extract images from PDF." << std::endl;
  return false;
}
