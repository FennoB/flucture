#ifndef FLX_LAYOUT_TO_HTML_H
#define FLX_LAYOUT_TO_HTML_H

#include "../utils/flx_string.h"
#include "layout/flx_layout_geometry.h"
#include <vector>
#include <memory>

class flx_layout_to_html
{
public:
  flx_layout_to_html();

  // Main conversion method
  flx_string convert_page_to_html(flx_layout_geometry& page);

private:
  // Layout tree building
  struct layout_node {
    const flx_layout_geometry* geometry;
    std::vector<std::shared_ptr<layout_node>> children;
    bool is_root;

    layout_node() : geometry(nullptr), is_root(false) {}
  };

  std::shared_ptr<layout_node> build_spatial_tree(flx_layout_geometry& page);
  void find_children_recursive(flx_layout_geometry& parent,
                               flx_model_list<flx_layout_geometry>& candidates,
                               std::vector<bool>& used,
                               std::shared_ptr<layout_node> parent_node);
  bool is_contained_in(flx_layout_geometry& inner, flx_layout_geometry& outer);

  // HTML generation
  flx_string generate_html_with_boilerplate(flx_layout_geometry& page,
                                            std::shared_ptr<layout_node> tree);
  flx_string generate_css_style();
  flx_string generate_page_div(flx_layout_geometry& page,
                               std::shared_ptr<layout_node> tree);
  flx_string generate_element_recursive(std::shared_ptr<layout_node> node);

  // Text formatting
  flx_string format_text_content(flx_model_list<flx_layout_text>& texts);
  bool is_tabular_layout(flx_model_list<flx_layout_text>& texts);
  flx_string format_as_markdown_table(flx_model_list<flx_layout_text>& texts);
  flx_string format_as_markdown_paragraphs(flx_model_list<flx_layout_text>& texts);

  // Helper methods
  flx_string escape_html(flx_string text);
  double get_y_position(flx_layout_text& text);
};

#endif // FLX_LAYOUT_TO_HTML_H
