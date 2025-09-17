#ifndef FLX_SNIPPET_SOURCE_H
#define FLX_SNIPPET_SOURCE_H

#include "flx_snippet.h"
// include a suitable stl header for a fifo queue
#include <queue>

namespace flx {
  class snippet_source {
  protected:
    std::queue<snippet> snippets;
  public:
    snippet_source();
    virtual ~snippet_source() = default;
    virtual snippet get_snippet();
    bool end_reached() const;
    void add_snippet(const snippet& snip);
    virtual void process_changes() = 0;
  };
}

#endif // FLX_SNIPPET_SOURCE_H
