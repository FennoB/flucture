#ifndef flx_DOC_SIO_H
#define flx_DOC_SIO_H

#include "../utils/flx_string.h"
#include "../utils/flx_element.h"

// Document Semantic IO
// This class is responsible for reading and writing documents in a semantic way.
// A document is a collection of elements. An element is a semantic unit of the document.
// Elements can be composed of other elements. Base elements are text, rect and image.
// Inherit from this class to implement a specific document format.
// Lets now define the interface of a document semantic IO class.
// A document is a file that contains a collection of elements in a specific format.
// We have to be able to read files, parse its elements, manipulate them, serialize and write them again.
// Documents hav
class flx_doc_sio : public flx_element
{
public:
  virtual bool read(flx_string filename);
  virtual bool write(flx_string filename);
  virtual bool parse(flx_string &data)= 0;
  virtual bool serialize(flx_string &data) = 0;
};

#endif // flx_DOC_SIO_H
