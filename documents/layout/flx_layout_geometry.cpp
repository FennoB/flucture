#include "flx_layout_geometry.h"

flx_layout_geometry::flx_layout_geometry() {
  fill_color = "#FFFFFF";
}

void flx_layout_geometry::add_text(flx_layout_text& text) {
  texts.push_back(text);
}

void flx_layout_geometry::add_image(flx_layout_image& image) {
  images.push_back(image);
}

void flx_layout_geometry::add_sub_geometry(flx_layout_geometry& geometry) {
  sub_geometries.push_back(geometry);
}