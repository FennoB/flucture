// openai_api.h
#ifndef openai_api_H
#define openai_api_H


#include "../../aiprocesses/chat/flx_llm_api.h"

namespace flx::llm {
  class openai_message final : public i_llm_message {
    message_role role;
    flxv_map data;
  public:
    openai_message(flxv_map data);
    openai_message(message_role r, flx_string text);
    message_role get_role() const noexcept override { return role; }
    const flx_string& get_content() const override;
    void set_role(message_role r);
    void set_content(const flx_string &content);
    const flxv_map& get_data() const noexcept override { return data; }
    std::unique_ptr<i_llm_message> clone() const override {
      return std::make_unique<openai_message>(data);
    }
  };

  class openai_chat_context final : public i_llm_chat_context {
    flxv_map settings;
    std::vector<std::unique_ptr<i_llm_message>> messages;
  public:
    void set_settings(const flxv_map& s) override { settings = s; }
    flxv_map& get_settings() { return settings; }
    void add_message(std::unique_ptr<i_llm_message> message) override { messages.push_back(std::move(message)); }
    const std::vector<std::unique_ptr<i_llm_message>>& get_messages() const noexcept override { return messages; }
    void replace_system_message(const flx_string &new_system_message) override {
      messages[0] = std::make_unique<openai_message>(message_role::SYSTEM, new_system_message);
    }
    std::unique_ptr<i_llm_chat_context> clone() const override {
      auto cloned_context = std::make_unique<openai_chat_context>();
      cloned_context->set_settings(settings);
      for (const auto& msg : messages) {
        cloned_context->add_message(msg->clone());
      }
      return cloned_context;
    }
  };

  class openai_api final : public i_llm_api {
    flx_string api_key;
  public:
    explicit openai_api(flx_string key);

    std::unique_ptr<i_llm_chat_context> create_chat_context() override;
    std::unique_ptr<i_llm_message> create_message(message_role role, flx_variant content) override;
    std::unique_ptr<i_llm_message> create_message(flxv_map& data) override;

    std::unique_ptr<i_llm_message> generate_response(
      i_llm_chat_context& context,
      const std::vector<i_llm_function*>* functions=0
      ) override;

    bool embedding(const flx_string& text, flxv_vector& embedding) override;

  private:
    flx_variant function_to_variant(const i_llm_function& func);  };

} // namespace flx::llm

#endif // openai_api_H
