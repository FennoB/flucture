#include "flx_text_element.h"

flx_text_element::flx_text_element(flx_string text, float x, float y, float width, float height, int horizontal_alignment, int vertical_alignment)
{
  this->text = text;
  this->x = x;
  this->y = y;
  this->width = width;
  this->height = height;
  this->horizontal_alignment = horizontal_alignment; // -1 for left, 0 for center, 1 for right
  this->vertical_alignment = vertical_alignment; // -1 for top, 0 for center, 1 for bottom
}
