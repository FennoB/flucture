#ifndef FLX_TEXT_ELEMENT_H
#define FLX_TEXT_ELEMENT_H

#include "flx_element.h"

class flx_text_element : public flx_element
{
public:
  flx_text_element(){}
  flx_text_element(flx_string text, float x, float y, float width, float height,
                   int horizontal_alignment = -1, int vertical_alignment = -1);
  flxp_string(text);
};

#endif // FLX_TEXT_ELEMENT_H
