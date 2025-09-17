#ifndef FLX_LAYOUT_BOUNDS_H
#define FLX_LAYOUT_BOUNDS_H

#include "../../utils/flx_model.h"

class flx_layout_bounds : public flx_model
{
public:
  flxp_double(x);
  flxp_double(y);
  flxp_double(width);
  flxp_double(height);

  flx_layout_bounds();
  flx_layout_bounds(double x_val, double y_val, double width_val, double height_val);

  double get_left() const;
  double get_right() const;
  double get_top() const;
  double get_bottom() const;
  double get_center_x() const;
  double get_center_y() const;

  bool contains_point(double px, double py) const;
  bool contains_bounds(const flx_layout_bounds& other) const;
  bool intersects_bounds(const flx_layout_bounds& other) const;
};

#endif // FLX_LAYOUT_BOUNDS_H