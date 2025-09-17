#include "flx_element.h"

flx_element::flx_element()
{
}

flx_element::flx_element(float x, float y, float width, float height, int horizontal_alignment, int vertical_alignment)
{
  this->x = x;
  this->y = y;
  this->width = width;
  this->height = height;
  this->horizontal_alignment = horizontal_alignment; // -1 for left, 0 for center, 1 for right
  this->vertical_alignment = vertical_alignment; // -1 for top, 0 for center, 1 for bottom
}

float flx_element::get_center_x()
{
  if (horizontal_alignment == -1l) {
    return x + width / 2.0;
  } else if (horizontal_alignment == 0l) {
    return x + width / 2.0;
  } else { // horizontal_alignment == 1
    return x + width / 2.0;
  }
}

float flx_element::get_center_y()
{
  if (vertical_alignment == -1l) {
    return y + height / 2.0;
  } else if (vertical_alignment == 0l) {
    return y + height / 2.0;
  } else { // vertical_alignment == 1
    return y + height / 2.0;
  }
}

float flx_element::get_bb_x()
{
  if (horizontal_alignment == -1l) {
    return x;
  } else if (horizontal_alignment == 0l) {
    return x - width / 2.0;
  } else { // horizontal_alignment == 1
    return x - width;
  }
}

float flx_element::get_bb_y()
{
  if (vertical_alignment == -1l) {
    return y;
  } else if (vertical_alignment == 0l) {
    return y - height / 2.0;
  } else { // vertical_alignment == 1
    return y - height;
  }
}

float flx_element::get_bb_width()
{
  return width;
}

float flx_element::get_bb_height()
{
  return height;
}
