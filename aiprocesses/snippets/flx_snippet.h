#ifndef FLX_SNIPPET_H
#define FLX_SNIPPET_H

#include "../../utils/flx_variant.h"

namespace flx {
  class snippet {
    flxv_map metadata;
    flx_string content;

  public:
    snippet(flxv_map meta, flx_string text)
        : metadata(std::move(meta)), content(std::move(text)){}
    const flxv_map& get_metadata() const noexcept { return metadata; }
    const flx_string& get_content() const noexcept { return content; }
  };
}

#endif // FLX_SNIPPET_H
