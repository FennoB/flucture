// flx_llm_chat.cpp
#include "flx_llm_chat.h"
#include <stdexcept>
#include <api/json/flx_json.h>

namespace flx::llm {
  flx_llm_chat::flx_llm_chat(std::shared_ptr<i_llm_api> api_impl)
      : api(std::move(api_impl)) {}

  flx_llm_chat::~flx_llm_chat() = default;

  void flx_llm_chat::create_context(const flxv_map& settings, flx_string system_prompt ) {
    context = api->create_chat_context();
    if (context) {
      context->set_settings(settings);
      context->add_message(api->create_message(message_role::SYSTEM, system_prompt));
    }
  }

  void flx_llm_chat::set_context(std::unique_ptr<i_llm_chat_context> new_context)
  {
    if (!new_context) {
      throw std::invalid_argument("New context cannot be null");
    }
    context = std::move(new_context);
    if (!context) {
      throw std::runtime_error("Failed to set new chat context");
    }
  }

  void flx_llm_chat::register_function(std::shared_ptr<i_llm_function> func) {
    if (func) {
      available_functions[func->get_name()] = std::move(func);
    }
  }

  void flx_llm_chat::register_tool_provider(std::shared_ptr<i_llm_tool_provider> provider) {
    if (provider) {
      tool_providers.push_back(provider);
    }
  }

  std::vector<i_llm_function*> flx_llm_chat::get_function_list_for_api() {
    std::vector<i_llm_function*> funcs;

    for(const auto& [name, func_ptr] : available_functions) {
      funcs.push_back(func_ptr.get());
    }

    for(const auto& provider : tool_providers) {
      if (provider && provider->is_available()) {
        auto provider_tools = provider->get_available_tools();
        funcs.insert(funcs.end(), provider_tools.begin(), provider_tools.end());
      }
    }

    return funcs;
  }

  i_llm_function* flx_llm_chat::find_function(const flx_string& name) {
    auto it = available_functions.find(name);
    if (it != available_functions.end()) {
      return it->second.get();
    }

    for(const auto& provider : tool_providers) {
      if (provider && provider->is_available()) {
        auto tools = provider->get_available_tools();
        for(auto* tool : tools) {
          if (tool && tool->get_name() == name) {
            return tool;
          }
        }
      }
    }

    return nullptr;
  }

  bool flx_llm_chat::chat(const flx_string& user_message, flx_string& final_response, int max_tool_calls) {
    if (!context) return false;

    context->add_message(api->create_message(message_role::USER, user_message));

    for (int i = 0; i < max_tool_calls; ++i) {
      auto functions = get_function_list_for_api();
      auto response_message = api->generate_response(*context, &functions);
      if (!response_message) return false;

      const flx_variant& data = response_message->get_data();

      context->add_message(std::move(response_message));

      if (data.is_map()) {
        const flxv_map& content_map = data.map_value();
        auto it = content_map.find("tool_calls");

        if (it != content_map.end() && it->second.is_vector()) {
          const flxv_vector& tool_calls_vector = it->second.vector_value();
          for (const auto& tool_call_var : tool_calls_vector) {
            handle_tool_call(tool_call_var);
          }
        }
        else if (content_map.count("content") && content_map.at("content").is_string()) {
          final_response = content_map.at("content").string_value();
          return true; // Konversation ist erfolgreich beendet.
        } else {
          continue;
        }
      } else {
        return false;
      }
    }

    return true;
  }

  void flx_llm_chat::handle_tool_call(const flx_variant& tool_call_data) {
    if (!tool_call_data.is_map()) return;

    const flxv_map& tool_call_map = tool_call_data.map_value();
    const flxv_map& function_map = tool_call_map.at("function").map_value();
    const flx_string& func_name = function_map.at("name").string_value();

    const flx_string& args_str = function_map.at("arguments").string_value();
    flxv_map args_map;
    flx_json args_parser(&args_map);
    if (!args_parser.parse(args_str)) {
      return;
    }

    i_llm_function* func = find_function(func_name);
    if (func) {
      flx_string res = func->call(args_map);
      flxv_map result_message_content;
      result_message_content["role"] = "tool";
      result_message_content["content"] = res;
      result_message_content["tool_call_id"] = tool_call_map.at("id");
      context->add_message(api->create_message(result_message_content));
    }
  }
} // namespace flx::llm
