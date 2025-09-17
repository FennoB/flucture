#ifndef FLX_LAYOUT_VERTEX_H
#define FLX_LAYOUT_VERTEX_H

#include "../../utils/flx_model.h"

class flx_layout_vertex : public flx_model
{
public:
  flxp_double(x);
  flxp_double(y);

  flx_layout_vertex();
  flx_layout_vertex(double x_val, double y_val);
};

#endif // FLX_LAYOUT_VERTEX_H