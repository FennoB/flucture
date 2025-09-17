#ifndef FLX_LAYOUT_GEOMETRY_H
#define FLX_LAYOUT_GEOMETRY_H

#include "flx_layout_bounds.h"
#include "flx_layout_text.h"
#include "flx_layout_image.h"
#include "flx_layout_vertex.h"

class flx_layout_geometry : public flx_layout_bounds
{
public:
  // Container properties
  flxp_model_list(texts, flx_layout_text);
  flxp_model_list(images, flx_layout_image);
  flxp_model_list(sub_geometries, flx_layout_geometry);
  
  // Polygon properties
  flxp_model_list(vertices, flx_layout_vertex);
  flxp_string(fill_color);

  flx_layout_geometry();

  void add_text(flx_layout_text& text);
  void add_image(flx_layout_image& image);
  void add_sub_geometry(flx_layout_geometry& geometry);
};

#endif // FLX_LAYOUT_GEOMETRY_H