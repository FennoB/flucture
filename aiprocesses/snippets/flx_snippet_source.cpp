#include "flx_snippet_source.h"

using namespace flx;
snippet_source::snippet_source() {}

snippet snippet_source::get_snippet() {
  if (snippets.empty()) {
    throw std::runtime_error("No more snippets available.");
  }
  snippet snip = snippets.front();
  snippets.pop();
  return snip;
}

bool snippet_source::end_reached() const{
  return snippets.empty();
}

void snippet_source::add_snippet(const snippet &snip) {
  snippets.push(snip);
}
