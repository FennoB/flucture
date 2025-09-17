#ifndef FLX_CHAT_SNIPPET_SOURCE_H
#define FLX_CHAT_SNIPPET_SOURCE_H

#include "../snippets/flx_snippet_source.h"
#include "../chat/flx_llm_chat_interfaces.h"
#include "flx_llm_api.h"

namespace flx {
  class chat_snippet_source : public snippet_source {
    std::shared_ptr<llm::i_llm_api> chat_api;
    std::shared_ptr<llm::i_llm_chat_context> chat_context;
    size_t last_index = 0;
    flx_string last_topic;
  public:
    chat_snippet_source(std::shared_ptr<llm::i_llm_api> api, std::shared_ptr<llm::i_llm_chat_context> context)
        : chat_api(std::move(api)), chat_context(std::move(context)) {}
    void process_changes();
  };
}

#endif // FLX_CHAT_SNIPPET_SOURCE_H
