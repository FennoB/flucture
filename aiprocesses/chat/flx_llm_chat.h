// flx_llm_chat.h
#ifndef FLX_LLM_CHAT_H
#define FLX_LLM_CHAT_H

#include <memory>
#include "../../utils/flx_variant.h"
#include "flx_llm_api.h"

namespace flx::llm {
  class flx_llm_chat {
    std::shared_ptr<i_llm_api> api;
    std::unique_ptr<i_llm_chat_context> context;
    std::map<flx_string, std::shared_ptr<i_llm_function>> available_functions;

  public:
    explicit flx_llm_chat(std::shared_ptr<i_llm_api> api_impl);
    ~flx_llm_chat();
    void create_context(const flxv_map& settings);
    void set_context(std::unique_ptr<i_llm_chat_context> new_context);
    void register_function(std::shared_ptr<i_llm_function> func);
    bool chat(const flx_string& user_message, flx_string& final_response, int max_tool_calls = 5);

  private:
    void handle_tool_call(const flx_variant& tool_call_data);
    std::vector<i_llm_function*> get_function_list_for_api();
  };

} // namespace flx::llm

#endif // FLX_LLM_CHAT_H
