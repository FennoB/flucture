#include "flx_layout_text.h"

flx_layout_text::flx_layout_text() {}

flx_layout_text::flx_layout_text(double x_val, double y_val, double width_val, double height_val, 
                                 const flx_string& text_val)
  : flx_layout_bounds(x_val, y_val, width_val, height_val)
{
  text = text_val;
}