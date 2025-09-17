#ifndef FLX_LAYOUT_GEOMETRY_H
#define FLX_LAYOUT_GEOMETRY_H

#include "flx_layout_bounds.h"
#include "flx_layout_text.h"
#include "flx_layout_image.h"

class flx_layout_geometry : public flx_layout_bounds
{
public:
  flxp_model_list(texts, flx_layout_text);
  flxp_model_list(images, flx_layout_image);
  flxp_model_list(sub_geometries, flx_layout_geometry);

  flx_layout_geometry();
  flx_layout_geometry(double x_val, double y_val, double width_val, double height_val);

  void add_text(flx_layout_text& text);
  void add_image(flx_layout_image& image);
  void add_sub_geometry(flx_layout_geometry& geometry);
};

#endif // FLX_LAYOUT_GEOMETRY_H