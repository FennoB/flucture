#ifndef flx_DOC_SIO_H
#define flx_DOC_SIO_H

#include "../utils/flx_string.h"
#include "../utils/flx_model.h"
#include "layout/flx_layout_geometry.h"

class flx_doc_sio : public flx_model
{
public:
  // Format-agnostic document structure
  flxp_model_list(pages, flx_layout_geometry);
  
  // Core document operations
  virtual bool read(flx_string filename);
  virtual bool write(flx_string filename);
  virtual bool parse(flx_string &data) = 0;
  virtual bool serialize(flx_string &data) = 0;
  
  // Layout manipulation helpers
  flx_layout_geometry& add_page();
  size_t page_count() const;
};

#endif // flx_DOC_SIO_H
