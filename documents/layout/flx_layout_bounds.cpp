#include "flx_layout_bounds.h"

flx_layout_bounds::flx_layout_bounds() {}

flx_layout_bounds::flx_layout_bounds(double x_val, double y_val, double width_val, double height_val)
{
  x = x_val;
  y = y_val;
  width = width_val;
  height = height_val;
}

double flx_layout_bounds::get_left() const {
  return x;
}

double flx_layout_bounds::get_right() const {
  return x + width;
}

double flx_layout_bounds::get_top() const {
  return y;
}

double flx_layout_bounds::get_bottom() const {
  return y + height;
}

double flx_layout_bounds::get_center_x() const {
  return x + width / 2.0;
}

double flx_layout_bounds::get_center_y() const {
  return y + height / 2.0;
}

bool flx_layout_bounds::contains_point(double px, double py) const {
  return px >= get_left() && px <= get_right() && 
         py >= get_top() && py <= get_bottom();
}

bool flx_layout_bounds::contains_bounds(const flx_layout_bounds& other) const {
  return other.get_left() >= get_left() && 
         other.get_right() <= get_right() &&
         other.get_top() >= get_top() && 
         other.get_bottom() <= get_bottom();
}

bool flx_layout_bounds::intersects_bounds(const flx_layout_bounds& other) const {
  return !(other.get_right() < get_left() || 
           other.get_left() > get_right() ||
           other.get_bottom() < get_top() || 
           other.get_top() > get_bottom());
}