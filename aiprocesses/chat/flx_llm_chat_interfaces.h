#ifndef FLX_LLM_CHAT_INTERFACES_H
#define FLX_LLM_CHAT_INTERFACES_H

#include "../../utils/flx_variant.h"
#include <memory>

// NEU: Alles im Namespace flx::llm gekapselt
namespace flx::llm {
  enum class message_role {
    SYSTEM,
    USER,
    ASSISTANT,
    TOOL
  };

  class i_llm_function {
  public:
    virtual ~i_llm_function() = default;
    virtual flx_string get_name() const = 0;
    virtual flx_string get_description() const = 0;
    virtual flxv_map get_parameters() const = 0;
    virtual flx_string call(const flxv_map& in) = 0;
  };

  class i_llm_message {
  public:
    virtual ~i_llm_message() = default;
    virtual message_role get_role() const noexcept = 0;
    virtual const flx_string& get_content() const = 0;
    virtual void set_role(message_role r) = 0;
    virtual void set_content(const flx_string& content) = 0;
    virtual const flxv_map& get_data() const noexcept = 0;
    virtual std::unique_ptr<i_llm_message> clone() const = 0;
  };

  class i_llm_chat_context {
  public:
    virtual ~i_llm_chat_context() = default;
    virtual void replace_system_message(const flx_string& new_system_message) = 0;
    virtual void set_settings(const flxv_map& settings) = 0;
    virtual void add_message(std::unique_ptr<i_llm_message> message) = 0;
    virtual const std::vector<std::unique_ptr<i_llm_message>>& get_messages() const noexcept = 0;
    virtual std::unique_ptr<i_llm_chat_context> clone() const = 0;
  };
}

#endif // FLX_LLM_CHAT_INTERFACES_H
