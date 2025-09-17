#ifndef flx_ELEMENT_H
#define flx_ELEMENT_H

#include "flx_model.h"
#include "flx_geometry.h"

class flx_element : public flx_model, public flx_geometry
{
public:
  flx_element();
  flx_element(float x, float y, float width, float height,
    int horizontal_alignment = -1, int vertical_alignment = -1);
  flxp_double(x);
  flxp_double(y);
  flxp_double(width);
  flxp_double(height);
  flxp_int(horizontal_alignment); // -1 for left, 0 for center, 1 for right
  flxp_int(vertical_alignment); // -1 for top, 0 for center, 1 for bottom
  flxp_int(page);

  // flx_geometry interface
public:
  float get_center_x() ;
  float get_center_y() ;
  float get_bb_x() ;
  float get_bb_y() ;
  float get_bb_width() ;
  float get_bb_height() ;
};

#endif // flx_ELEMENT_H
