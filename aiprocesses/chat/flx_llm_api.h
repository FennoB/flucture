// flx_llm_api.h
#ifndef FLX_LLM_API_H
#define FLX_LLM_API_H

#include "../../utils/flx_variant.h"
#include "flx_llm_chat_interfaces.h"

// NEU: Alles im Namespace flx::llm gekapselt
namespace flx::llm {
  class i_llm_api {
  public:
    virtual ~i_llm_api() = default;

    virtual std::unique_ptr<i_llm_chat_context> create_chat_context() = 0;
    virtual std::unique_ptr<i_llm_message> create_message(message_role role, flx_variant content) = 0;
    virtual std::unique_ptr<i_llm_message> create_message(flxv_map& data) = 0;


    // NEU: Nimmt eine Liste von non-owning raw pointers, da die Funktion nur beobachtet, nicht besitzt.
    virtual std::unique_ptr<i_llm_message> generate_response(
      i_llm_chat_context& context,
      const std::vector<i_llm_function*>* functions=0
      ) = 0;

    virtual bool embedding(const flx_string& text, flxv_vector& embedding) = 0;
  };

} // namespace flx::llm

#endif // FLX_LLM_API_H
