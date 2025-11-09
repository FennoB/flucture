#include "flx_layout_to_html.h"
#include <sstream>
#include <algorithm>
#include <map>
#include <cmath>

flx_layout_to_html::flx_layout_to_html() {
}

flx_string flx_layout_to_html::convert_page_to_html(flx_layout_geometry& page) {
  // Step 1: Build spatial inclusion tree
  auto tree = build_spatial_tree(page);

  // Step 2: Generate HTML with boilerplate
  return generate_html_with_boilerplate(page, tree);
}

// ============================================================================
// Layout Tree Building (Spatial Inclusion Analysis)
// ============================================================================

std::shared_ptr<flx_layout_to_html::layout_node> flx_layout_to_html::build_spatial_tree(flx_layout_geometry& page) {
  auto root = std::make_shared<layout_node>();
  root->geometry = &page;
  root->is_root = true;

  // Get all sub-geometries as candidates for hierarchy
  auto& sub_geos = page.sub_geometries;
  if (sub_geos.size() == 0) {
    return root;
  }

  // Track which geometries have been assigned to parents
  std::vector<bool> used(sub_geos.size(), false);

  // Recursively build hierarchy
  find_children_recursive(page, sub_geos, used, root);

  return root;
}

void flx_layout_to_html::find_children_recursive(flx_layout_geometry& parent,
                                                  flx_model_list<flx_layout_geometry>& candidates,
                                                  std::vector<bool>& used,
                                                  std::shared_ptr<layout_node> parent_node) {
  // Find all candidates that are directly contained in parent
  for (size_t i = 0; i < candidates.size(); ++i) {
    if (used[i]) continue;

    auto& candidate = candidates[i];

    // Check if candidate is contained in parent
    if (is_contained_in(candidate, parent)) {
      // Check if it's contained in any sibling (deeper nesting)
      bool contained_in_sibling = false;
      for (size_t j = 0; j < candidates.size(); ++j) {
        if (i == j || used[j]) continue;

        // If candidate is inside another candidate that's also inside parent,
        // then it should be child of that candidate, not of parent
        if (is_contained_in(candidate, candidates[j]) &&
            is_contained_in(candidates[j], parent)) {
          contained_in_sibling = true;
          break;
        }
      }

      // Only add as direct child if not contained in sibling
      if (!contained_in_sibling) {
        auto child_node = std::make_shared<layout_node>();
        child_node->geometry = &candidate;
        child_node->is_root = false;

        parent_node->children.push_back(child_node);
        used[i] = true;

        // Recursively find children of this child
        find_children_recursive(candidate, candidates, used, child_node);
      }
    }
  }
}

bool flx_layout_to_html::is_contained_in(flx_layout_geometry& inner, flx_layout_geometry& outer) {
  double inner_left = inner.x;
  double inner_top = inner.y;
  double inner_right = inner.x + inner.width;
  double inner_bottom = inner.y + inner.height;

  double outer_left = outer.x;
  double outer_top = outer.y;
  double outer_right = outer.x + outer.width;
  double outer_bottom = outer.y + outer.height;

  // Inner must be completely within outer (with small tolerance for rounding)
  const double tolerance = 0.1;
  return (inner_left >= outer_left - tolerance &&
          inner_top >= outer_top - tolerance &&
          inner_right <= outer_right + tolerance &&
          inner_bottom <= outer_bottom + tolerance);
}

// ============================================================================
// HTML Generation
// ============================================================================

flx_string flx_layout_to_html::generate_html_with_boilerplate(flx_layout_geometry& page,
                                                               std::shared_ptr<layout_node> tree) {
  std::ostringstream html;

  html << "<!DOCTYPE html>\n";
  html << "<html lang=\"de\">\n";
  html << "<head>\n";
  html << "    <meta charset=\"UTF-8\">\n";
  html << "    <title>PDF Konvertierung</title>\n";
  html << generate_css_style().c_str();
  html << "</head>\n";
  html << "<body>\n";
  html << generate_page_div(page, tree).c_str();
  html << "</body>\n";
  html << "</html>\n";

  return flx_string(html.str());
}

flx_string flx_layout_to_html::generate_css_style() {
  std::ostringstream css;

  css << "    <style>\n";
  css << "        body { margin: 0; font-family: Arial, sans-serif; }\n";
  css << "        .page { position: relative; background-color: white; margin: 20px auto; box-shadow: 0 0 10px rgba(0,0,0,0.1); }\n";
  css << "        .element { position: absolute; overflow: hidden; }\n";
  css << "        table { border-collapse: collapse; width: 100%; }\n";
  css << "        td { padding: 4px 8px; vertical-align: top; }\n";
  css << "        td:first-child { font-weight: normal; }\n";
  css << "    </style>\n";

  return flx_string(css.str());
}

flx_string flx_layout_to_html::generate_page_div(flx_layout_geometry& page,
                                                  std::shared_ptr<layout_node> tree) {
  std::ostringstream html;

  double page_width = page.width;
  double page_height = page.height;

  html << "    <div class=\"page\" style=\"width: " << page_width << "px; height: " << page_height << "px;\">\n";

  // If page has texts directly (no sub-geometries), format them
  if (page.texts.size() > 0 && tree->children.size() == 0) {
    auto& texts = const_cast<flx_model_list<flx_layout_text>&>(page.texts);
    html << format_text_content(texts).c_str();
  }

  // Generate content recursively from tree
  for (auto& child : tree->children) {
    html << generate_element_recursive(child).c_str();
  }

  html << "    </div>\n";

  return flx_string(html.str());
}

flx_string flx_layout_to_html::generate_element_recursive(std::shared_ptr<layout_node> node) {
  if (!node || !node->geometry) {
    return flx_string("");
  }

  auto& geom = *node->geometry;
  std::ostringstream html;

  // Create div for this element
  html << "        <div class=\"element\" style=\""
       << "left: " << geom.x << "px; "
       << "top: " << geom.y << "px; "
       << "width: " << geom.width << "px; "
       << "height: " << geom.height << "px;\">\n";

  // If this node has text children, format them
  if (geom.texts.size() > 0) {
    // const_cast needed because flx_property returns const reference
    auto& texts = const_cast<flx_model_list<flx_layout_text>&>(geom.texts);
    html << format_text_content(texts).c_str();
  }

  // Recursively render children
  for (auto& child : node->children) {
    html << generate_element_recursive(child).c_str();
  }

  html << "        </div>\n";

  return flx_string(html.str());
}

// ============================================================================
// Text Formatting (Column/Table Detection)
// ============================================================================

flx_string flx_layout_to_html::format_text_content(flx_model_list<flx_layout_text>& texts) {
  if (texts.size() == 0) {
    return flx_string("");
  }

  // Check if texts are arranged in tabular layout
  if (is_tabular_layout(texts)) {
    return format_as_markdown_table(texts);
  } else {
    return format_as_markdown_paragraphs(texts);
  }
}

bool flx_layout_to_html::is_tabular_layout(flx_model_list<flx_layout_text>& texts) {
  if (texts.size() < 2) {
    return false;
  }

  // Group texts by Y-coordinate (rows)
  std::map<int, int> y_groups; // y_coord -> count
  const double y_tolerance = 5.0; // pixels

  for (size_t i = 0; i < texts.size(); ++i) {
    double y = get_y_position(texts[i]);
    int y_key = static_cast<int>(y / y_tolerance);
    y_groups[y_key]++;
  }

  // If multiple texts share similar Y coordinates, it's likely tabular
  int rows_with_multiple_items = 0;
  for (const auto& pair : y_groups) {
    if (pair.second >= 2) {
      rows_with_multiple_items++;
    }
  }

  // Consider it tabular if at least 50% of groups have multiple items
  return rows_with_multiple_items >= static_cast<int>(y_groups.size()) / 2;
}

flx_string flx_layout_to_html::format_as_markdown_table(flx_model_list<flx_layout_text>& texts) {
  std::ostringstream md;

  // Group texts by Y-coordinate (rows)
  const double y_tolerance = 5.0;
  std::map<int, std::vector<size_t>> rows; // y_key -> indices

  for (size_t i = 0; i < texts.size(); ++i) {
    double y = get_y_position(texts[i]);
    int y_key = static_cast<int>(y / y_tolerance);
    rows[y_key].push_back(i);
  }

  // Sort each row by X coordinate
  for (auto& pair : rows) {
    std::sort(pair.second.begin(), pair.second.end(), [&](size_t a, size_t b) {
      return texts[a].x < texts[b].x;
    });
  }

  // Generate table with empty header trick
  md << "| &nbsp; | &nbsp; |\n";
  md << "|---|---|\n";

  for (const auto& pair : rows) {
    md << "|";
    for (size_t idx : pair.second) {
      flx_string text = texts[idx].text;
      md << " " << escape_html(text).c_str() << " |";
    }
    md << "\n";
  }

  return flx_string(md.str());
}

flx_string flx_layout_to_html::format_as_markdown_paragraphs(flx_model_list<flx_layout_text>& texts) {
  std::ostringstream md;

  // Sort texts by Y position (top to bottom)
  std::vector<size_t> indices;
  for (size_t i = 0; i < texts.size(); ++i) {
    indices.push_back(i);
  }

  std::sort(indices.begin(), indices.end(), [&](size_t a, size_t b) {
    return get_y_position(texts[a]) < get_y_position(texts[b]);
  });

  // Format as markdown
  for (size_t idx : indices) {
    const auto& text = texts[idx];

    // Check if text looks like a heading (larger font size)
    double font_size = 12.0;
    try {
      font_size = text.font_size;
    } catch (...) {}

    if (font_size > 14.0) {
      md << "# ";
    }

    md << escape_html(text.text).c_str() << "\n";
  }

  return flx_string(md.str());
}

// ============================================================================
// Helper Methods
// ============================================================================

flx_string flx_layout_to_html::escape_html(flx_string text) {
  std::string result = text.c_str();

  // Basic HTML escaping
  size_t pos = 0;
  while ((pos = result.find("&", pos)) != std::string::npos) {
    result.replace(pos, 1, "&amp;");
    pos += 5;
  }
  pos = 0;
  while ((pos = result.find("<", pos)) != std::string::npos) {
    result.replace(pos, 1, "&lt;");
    pos += 4;
  }
  pos = 0;
  while ((pos = result.find(">", pos)) != std::string::npos) {
    result.replace(pos, 1, "&gt;");
    pos += 4;
  }

  return flx_string(result);
}

double flx_layout_to_html::get_y_position(flx_layout_text& text) {
  try {
    return text.y;
  } catch (...) {
    return 0.0;
  }
}
