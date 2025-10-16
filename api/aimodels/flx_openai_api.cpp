// openai_api.cpp
#include "flx_openai_api.h"
#include "../client/flx_http_request.h"
#include <iostream>
#include <api/json/flx_json.h>
#include <chrono>

// Debug timing helper for API calls
static auto api_debug_start = std::chrono::high_resolution_clock::now();

static void api_timestamp(const std::string& label) {
    auto now = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - api_debug_start);
    std::cout << "[API " << duration.count() << "ms] " << label << std::endl;
}

namespace flx::llm {
  static flx_string role_to_string(message_role role) {
    switch (role) {
      case message_role::SYSTEM: return "system";
      case message_role::USER: return "user";
      case message_role::ASSISTANT: return "assistant";
      case message_role::TOOL: return "tool";
    }
    return "";
  }

  openai_message::openai_message(flxv_map data) : data(data)
  {
    auto it = data.find("role");
    if (it != data.end()) {
      if (it->second.is_string()) {
        flx_string role_str = it->second.string_value();
        if (role_str == "system") role = message_role::SYSTEM;
        else if (role_str == "user") role = message_role::USER;
        else if (role_str == "assistant") role = message_role::ASSISTANT;
        else if (role_str == "tool") role = message_role::TOOL;
        else throw std::runtime_error("Unknown message role: " + role_str.to_std());
      } else {
        throw std::runtime_error("Role must be a string");
      }
    } else {
      throw std::runtime_error("Role not found in message data");
    }
  }

  openai_message::openai_message(message_role r, flx_string text) : role(r)
  {
    data["role"] = role_to_string(r);
    data["content"] = text;
  }

  void openai_message::set_role(message_role r)
  {
    data["role"] = role_to_string(r);
  }

  void openai_message::set_content(const flx_string &content)
  {
    data["content"] = content;
  }

  const flx_string &openai_message::get_content() const {
    auto it = data.find("content");
    if (it != data.end() && it->second.is_string()) {
      return it->second.string_value();
    }
    throw std::runtime_error("Content not found or not a string in message data");
  }

  openai_api::openai_api(flx_string key) : api_key(std::move(key)) {}

  std::unique_ptr<i_llm_chat_context> openai_api::create_chat_context() {
    return std::make_unique<openai_chat_context>();
  }

  std::unique_ptr<i_llm_message> openai_api::create_message(message_role role, flx_variant content) {
    return std::make_unique<openai_message>(role, std::move(content));
  }

  std::unique_ptr<i_llm_message> openai_api::create_message(flxv_map &data) {
    return std::make_unique<openai_message>(data);
  }

  std::unique_ptr<i_llm_message> openai_api::generate_response(
    i_llm_chat_context& context,
    const std::vector<i_llm_function*>* functions)
  {
    auto* openai_ctx = dynamic_cast<openai_chat_context*>(&context);
    if (!openai_ctx) return nullptr;

    flxv_map request_body_map;
    const auto& settings = openai_ctx->get_settings();
    // if model not set throw missing settings exception
    if (!settings.count("model")) {
      std::cerr << "Error: Model setting is missing." << std::endl;
      return nullptr;
    }
    request_body_map["model"] = settings.at("model");

           // ? Add support for structured outputs via response_format.
           // The user should provide the entire response_format map in the settings.
    if (settings.count("response_format")) {
      request_body_map["response_format"] = settings.at("response_format");
    }

    flxv_vector messages_vec;
    for (const auto& msg : openai_ctx->get_messages()) {
      messages_vec.push_back(msg->get_data());
    }
    request_body_map["messages"] = messages_vec;

    if (functions && !functions->empty()) {
      flxv_vector tools_vec;
      for (const auto& func : *functions) {
        tools_vec.push_back(function_to_variant(*func));
      }
      request_body_map["tools"] = tools_vec;
      request_body_map["tool_choice"] = flx_string("auto");
    }

    flx_json json_handler(&request_body_map);
    flx_string json_body_string = json_handler.create();

    if (json_body_string.empty()) {
      std::cerr << "Error: Failed to create JSON request body." << std::endl;
      return nullptr;
    }

    // LOGGING: Request details
    api_timestamp("START OpenAI API Request");
    std::cout << "\n=== OPENAI API REQUEST ===" << std::endl;
    std::cout << "Model: " << settings.at("model").string_value().c_str() << std::endl;
    std::cout << "Request size: " << json_body_string.length() << " chars" << std::endl;
    std::cout << "Request body (first 500 chars):\n" << std::string(json_body_string.c_str()).substr(0, 500) << "..." << std::endl;
    if (settings.count("temperature")) {
      try {
        if (settings.at("temperature").is_double()) {
          std::cout << "Temperature setting: " << settings.at("temperature").double_value() << " (NOT sent to API!)" << std::endl;
        } else if (settings.at("temperature").is_int()) {
          std::cout << "Temperature setting: " << settings.at("temperature").int_value() << " (NOT sent to API!)" << std::endl;
        }
      } catch (...) {
        std::cout << "Temperature setting exists but cannot be read" << std::endl;
      }
    }

    flx_http_request request("https://api.openai.com/v1/chat/completions");
    request.set_header("Content-Type", "application/json");
    request.set_header("Authorization", "Bearer " + api_key.to_std_const());
    request.set_method("POST");
    request.set_body(json_body_string.to_std());

    api_timestamp("Before HTTP request.send()");
    if (!request.send() || request.get_status_code() != 200) {
      std::cerr << "HTTP Request failed: " << request.get_error_message().to_std() << std::endl;
      std::cerr << "Response Body: " << request.get_response_body().to_std() << std::endl;
      return nullptr;
    }
    api_timestamp("After HTTP request.send()");

    flx_string response_body_str = request.get_response_body();

    // LOGGING: Response details
    std::cout << "\n=== OPENAI API RESPONSE ===" << std::endl;
    std::cout << "Response size: " << response_body_str.length() << " chars" << std::endl;
    std::cout << "Response body (first 1000 chars):\n" << std::string(response_body_str.c_str()).substr(0, 1000) << "..." << std::endl;
    flxv_map response_map;
    flx_json response_handler(&response_map);

    if (!response_handler.parse(response_body_str)) {
      std::cerr << "Error: Failed to parse JSON response." << std::endl;
      return nullptr;
    }

    // LOGGING: Usage information
    if (response_map.count("usage") && response_map["usage"].is_map()) {
      auto& usage = response_map["usage"].to_map();
      std::cout << "\n=== API USAGE STATS ===" << std::endl;
      if (usage.count("prompt_tokens") && usage["prompt_tokens"].is_int()) {
        std::cout << "Prompt tokens: " << usage["prompt_tokens"].int_value() << std::endl;
      }
      if (usage.count("completion_tokens") && usage["completion_tokens"].is_int()) {
        std::cout << "Completion tokens: " << usage["completion_tokens"].int_value() << std::endl;
      }
      if (usage.count("total_tokens") && usage["total_tokens"].is_int()) {
        std::cout << "Total tokens: " << usage["total_tokens"].int_value() << std::endl;
      }
    }

    // LOGGING: Model used
    if (response_map.count("model") && response_map["model"].is_string()) {
      std::cout << "Model actually used: " << response_map["model"].string_value().c_str() << std::endl;
    }
    std::cout << "========================\n" << std::endl;

    api_timestamp("END OpenAI API Request");

    if (!response_map["choices"].is_vector() || response_map["choices"].vector_value().empty()) {
      return nullptr;
    }

    flx_variant& choice_var = response_map["choices"].to_vector()[0];
    if (!choice_var.is_map()) return nullptr;

    flxv_map& choice_map = choice_var.to_map();
    flx_variant& message_var = choice_map["message"];
    if (!message_var.is_map()) return nullptr;

    flx_variant data = message_var;
    return std::make_unique<openai_message>(data);
  }

  bool openai_api::embedding(const flx_string& text, flxv_vector& embedding) {
    return false;
  }

  flx_variant openai_api::function_to_variant(const i_llm_function& func) {
    flxv_map tool_map;
    tool_map["type"] = flx_string("function");

    flxv_map func_details;
    func_details["name"] = func.get_name();
    func_details["description"] = func.get_description();
    func_details["parameters"] = func.get_parameters();

    tool_map["function"] = func_details;
    return tool_map;
  }
} // namespace flx::llm
