#include <catch2/catch_all.hpp>
#include <aiprocesses/chat/flx_llm_chat.h>
#include <utils/flx_string.h>
#include <api/aimodels/flx_openai_api.h>
#include <api/json/flx_json.h>
#include <aiprocesses/chat/flx_chat_snippet_source.h>
#include <iostream>

using namespace flx;
using namespace llm;

// NOTE: These tests require a valid OpenAI API key
// In production, this should come from environment variables
// For now, these tests are disabled to avoid hardcoded API keys

#ifdef ENABLE_LLM_TESTS

SCENARIO("OpenAI API basic functionality", "[llm][integration]") {
    GIVEN("A valid OpenAI API key and context") {
        // TODO: Get API key from environment variable
        flx_string api_key = std::getenv("OPENAI_API_KEY") ? std::getenv("OPENAI_API_KEY") : "";
        REQUIRE_FALSE(api_key.empty());
        
        openai_chat_context context;
        flxv_map settings;
        settings["model"] = "gpt-4o-mini";
        settings["max_tokens"] = 100;
        settings["temperature"] = 0.7;
        context.set_settings(settings);
        openai_api api(api_key);
        
        WHEN("A user sends a message") {
            std::unique_ptr message = api.create_message(message_role::USER, "Say hello in one word.");
            context.add_message(std::move(message));
            
            THEN("The API should return a valid response") {
                auto response_message = api.generate_response(context);
                REQUIRE(response_message != nullptr);
                REQUIRE(response_message->get_content().size() > 0);
            }
        }
    }
}

SCENARIO("LLM Chat maintains context", "[llm][integration]") {
    GIVEN("A chat object with valid API key") {
        flx_string api_key = std::getenv("OPENAI_API_KEY") ? std::getenv("OPENAI_API_KEY") : "";
        REQUIRE_FALSE(api_key.empty());
        
        flx_llm_chat chat(std::make_unique<openai_api>(api_key));
        
        flxv_map settings;
        settings["model"] = "gpt-4o-mini";
        chat.create_context(settings);
        
        // First exchange to establish context
        flx_string initial_response;
        chat.chat("My name is Thomas.", initial_response);
        
        WHEN("The user asks a follow-up question") {
            flx_string follow_up_response;
            bool success = chat.chat("What is my name?", follow_up_response);
            
            THEN("The chat should maintain context") {
                REQUIRE(success);
                REQUIRE_FALSE(follow_up_response.empty());
                REQUIRE(follow_up_response.to_lower().contains("thomas"));
            }
        }
    }
}

#else

// Placeholder test when LLM tests are disabled
SCENARIO("LLM tests are disabled", "[llm][disabled]") {
    GIVEN("LLM integration tests") {
        WHEN("Tests are run") {
            THEN("They should be skipped") {
                WARN("LLM tests disabled - set ENABLE_LLM_TESTS to enable");
                REQUIRE(true);  // Always pass
            }
        }
    }
}

#endif // ENABLE_LLM_TESTS