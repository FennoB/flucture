#ifndef FLX_LAYOUT_ELEMENT_H
#define FLX_LAYOUT_ELEMENT_H

#include "flx_text_element.h"

class flx_layout_element;
class flx_layout_element : public flx_element
{
public:
  std::vector<flx_text_element> text_elements; // Vector of text elements in the layout
  std::vector<flx_layout_element>* sub_elements; // Vector of sublayouts
  // Constructor to initialize the layout element
  flx_layout_element() {
    sub_elements = new std::vector<flx_layout_element>();
  }
  // Copy constructor
  flx_layout_element(const flx_layout_element& other)
    : flx_element(other) // Call base class copy constructor
  {
    text_elements = other.text_elements; // Copy text elements
    sub_elements = new std::vector<flx_layout_element>(*other.sub_elements); // Deep copy of sub_elements
  }

  ~flx_layout_element() {
    if (sub_elements) {
      delete sub_elements; // Clean up dynamically allocated memory
    }
  }

  // Assignment operator
  flx_layout_element& operator=(const flx_layout_element& other) {
    if (this != &other) {
      text_elements = other.text_elements; // Copy text elements
      if (sub_elements) {
        delete sub_elements; // Clean up existing memory
      }
      sub_elements = new std::vector<flx_layout_element>(*other.sub_elements); // Deep copy of sub_elements
    }
    flx_element::operator=(other); // Call base class assignment operator
    return *this;
  }
};

#endif // FLX_LAYOUT_ELEMENT_H
