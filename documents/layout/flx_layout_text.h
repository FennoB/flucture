#ifndef FLX_LAYOUT_TEXT_H
#define FLX_LAYOUT_TEXT_H

#include "flx_layout_bounds.h"

class flx_layout_text : public flx_layout_bounds
{
public:
  flxp_string(text);
  flxp_string(font_family);
  flxp_double(font_size);
  flxp_string(color);
  flxp_bool(bold);
  flxp_bool(italic);

  flx_layout_text();
  flx_layout_text(double x_val, double y_val, double width_val, double height_val,
                  const flx_string& text_val);
};

#endif // FLX_LAYOUT_TEXT_H