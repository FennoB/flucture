#ifndef FLX_LLM_CHAT_H
#define FLX_LLM_CHAT_H

#include <memory>
#include "../../utils/flx_variant.h"
#include "flx_llm_api.h"
#include "flx_llm_tool_provider.h"

namespace flx::llm {
  class flx_llm_chat {
    std::shared_ptr<i_llm_api> api;
    std::unique_ptr<i_llm_chat_context> context;
    std::map<flx_string, std::shared_ptr<i_llm_function>> available_functions;
    std::vector<std::shared_ptr<i_llm_tool_provider>> tool_providers;

  public:
    explicit flx_llm_chat(std::shared_ptr<i_llm_api> api_impl);
    ~flx_llm_chat();
    void create_context(const flxv_map& settings, flx_string system_prompt = "You are a helpful assistant.");
    void set_context(std::unique_ptr<i_llm_chat_context> new_context);
    void register_function(std::shared_ptr<i_llm_function> func);
    void register_tool_provider(std::shared_ptr<i_llm_tool_provider> provider);
    bool chat(const flx_string& user_message, flx_string& final_response, int max_tool_calls = 5);

  private:
    void handle_tool_call(const flx_variant& tool_call_data);
    std::vector<i_llm_function*> get_function_list_for_api();
    i_llm_function* find_function(const flx_string& name);
  };

}

#endif
