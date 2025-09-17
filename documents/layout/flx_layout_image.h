#ifndef FLX_LAYOUT_IMAGE_H
#define FLX_LAYOUT_IMAGE_H

#include "flx_layout_bounds.h"

class flx_layout_image : public flx_layout_bounds
{
public:
  flxp_string(image_path);
  flxp_string(description);
  flxp_string(mime_type);
  flxp_int(original_width);
  flxp_int(original_height);

  flx_layout_image();
  flx_layout_image(double x_val, double y_val, double width_val, double height_val);
};

#endif // FLX_LAYOUT_IMAGE_H