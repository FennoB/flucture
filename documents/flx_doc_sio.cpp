#include "flx_doc_sio.h"
#include "flx_layout_to_html.h"
#include "fstream"
#include <sstream>
#include <algorithm>
#include <cmath>

bool flx_doc_sio::read(flx_string filename)
{
  std::fstream f;
  f.open(filename.c_str(), std::ios::in | std::ios::binary);
  if (!f.is_open())
  {
    return false;
  }
  // read all bytes from the file
  std::string data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  f.close();

  flx_string str = data;
  // parse the data
  if (!parse(str))
  {
    return false;
  }
  return true;
}

bool flx_doc_sio::write(flx_string filename)
{
  std::fstream f;
  f.open(filename.c_str(), std::ios::out | std::ios::binary);
  if (!f.is_open())
  {
    return false;
  }
  // serialize the data
  flx_string data;
  if (!serialize(data))
  {
    return false;
  }
  f.write(data.c_str(), data.size());
  f.close();
  return true;
}

flx_layout_geometry& flx_doc_sio::add_page() {
  flx_layout_geometry new_page;
  pages.push_back(new_page);
  return pages[pages.size() - 1];
}

size_t flx_doc_sio::page_count() const {
  return pages.size();
}

flx_string flx_doc_sio::page_to_html(size_t page_index) {
  if (page_index >= pages.size()) {
    return flx_string("");
  }

  flx_layout_to_html converter;
  return converter.convert_page_to_html(pages[page_index]);
}

flx_string flx_doc_sio::to_text_layout() const {
  std::stringstream ascii_layout;

  // Use const_cast to access pages (flx_model_list doesn't have const operator[])
  flx_model_list<flx_layout_geometry>& mutable_pages = const_cast<flx_model_list<flx_layout_geometry>&>(pages);

  for (size_t page_idx = 0; page_idx < pages.size(); page_idx++) {
    auto& page = mutable_pages[page_idx];

    // Page separator (except before first page)
    if (page_idx > 0) {
      ascii_layout << "\n\n=== Seite " << (page_idx + 1) << " ===\n\n";
    }

    // Sort texts on this page by Y (top to bottom), then X (left to right)
    std::vector<flx_layout_text*> sorted_texts;
    for (int text_idx = 0; text_idx < page.texts.size(); text_idx++) {
      sorted_texts.push_back(&page.texts[text_idx]);
    }

    std::sort(sorted_texts.begin(), sorted_texts.end(),
      [](flx_layout_text* a, flx_layout_text* b) {
        // Sort by Y (top to bottom)
        // Y coordinates increase downward after PDF transformation
        // Lower Y = higher on page
        if (std::abs(a->y.value() - b->y.value()) > 5) {
          return a->y.value() < b->y.value();
        }
        // Same line: sort by X (left to right)
        return a->x.value() < b->x.value();
      });

    // Build ASCII layout
    double last_y = -1000;
    int current_x = 0;

    for (auto* text : sorted_texts) {
      // Skip empty texts
      if (text->text.is_null() || text->text.value().empty()) {
        continue;
      }

      std::string text_str(text->text.value().c_str());

      // Skip whitespace-only texts
      if (text_str.find_first_not_of(" \t\n\r") == std::string::npos) {
        continue;
      }

      // New line when Y position changes
      if (std::abs(text->y.value() - last_y) > 5) {
        if (last_y != -1000) ascii_layout << "\n";
        last_y = text->y.value();
        current_x = 0; // Reset X position for new line
      }

      // Insert spaces based on X position
      int target_x = (int)(text->x.value() / 8.0); // ~8 pixels per character (rough estimate)
      if (target_x > current_x) {
        ascii_layout << std::string(target_x - current_x, ' ');
        current_x = target_x;
      } else if (current_x > 0) {
        // At least ONE space between texts on the same line
        ascii_layout << " ";
        current_x++;
      }

      // Output text
      ascii_layout << text_str;
      current_x += text_str.length();
    }
  }

  return flx_string(ascii_layout.str().c_str());
}
