#include "flx_chat_snippet_source.h"
#include "flx_llm_chat.h"

#include <api/json/flx_json.h>
#include <iostream>
#include <ostream>

using namespace flx;

void chat_snippet_source::process_changes()
{
  // Check if new messages have been added
  while (chat_context->get_messages().size()-1 > last_index) {
    flx_string snippet_slicer_prompt = "You are a text-slicing bot. You will get a message. Your job is to split it into a JSON array of paragraphs. Make a new paragraph every time the topic changes. The JSON object must have a key called 'slices' which contains the array of strings.";
    snippet_slicer_prompt += flx_string("\n\nThe topic before this message came in was: ") + last_topic + "\n\n";
    // --- Settings Map for Paragraph Slicing ---

    // 1. Define the JSON schema for an array of strings
    flxv_map slices_schema;
    slices_schema["type"] = "object";

    flxv_map item_property;
    item_property["type"] = "object";
    // each item has a key "slice" with the string text part and a key "topic" with the topic
    item_property["properties"] = flxv_map{
        {"slice", flxv_map{{"type", "string"}}},
        {"topic", flxv_map{{"type", "string"}}}
    };
    item_property["required"] = flxv_vector{"slice", "topic"}; // Each item must have both "slice" and "topic"
    item_property["additionalProperties"] = false;

    flxv_map array_property;
    array_property["type"] = "array";
    array_property["description"] = "The list of semantically coherent text slices.";
    array_property["items"] = item_property; // Each item in the array is an object with "slice" and "topic"

    flxv_map properties;
    properties["slices"] = array_property; // The object has one key: "slices"
    slices_schema["properties"] = properties;
    slices_schema["additionalProperties"] = false;

    flxv_vector required_fields;
    required_fields.push_back("slices");
    slices_schema["required"] = required_fields;


    // 2. Construct the full response_format object
    flxv_map response_format_map;
    response_format_map["type"] = "json_schema";

    flxv_map json_schema_object;
    json_schema_object["name"] = "paragraph_slicer";
    json_schema_object["strict"] = true; // Ensures the output strictly follows the schema
    json_schema_object["schema"] = slices_schema;

    response_format_map["json_schema"] = json_schema_object;

    // 3. Set the final settings for the API call
    flxv_map settings;
    // Remember to use a model compatible with structured outputs, like gpt-4o-mini
    settings["model"] = "gpt-4o-mini";
    settings["response_format"] = response_format_map;

    // 4. Create a new chat context with the system prompt, add the last topic and the next message and generate a response
    auto context = chat_api->create_chat_context();
    context->set_settings(settings);
    context->add_message(
      chat_api->create_message(llm::message_role::SYSTEM, snippet_slicer_prompt));
    auto message = chat_context->get_messages().at(last_index + 1)->clone();
    last_index++;
    message->set_role(llm::message_role::USER);
    context->add_message(std::move(message));
    auto response_msg = chat_api->generate_response(*context);

    // 5. Get the response and parse it
    flx_string response = response_msg->get_content();
    std::cout << "Response from chat API: " << response.to_std() << std::endl;
    flxv_map response_data;
    flx_json json_handler(&response_data);
    json_handler.parse(response);
    flxv_vector slices = response_data["slices"];
    for (const auto& item : slices) {
      const flx_variant& slice_var = item.map_value().at("slice");
      const flx_variant& topic_var = item.map_value().at("topic");
      flx_string slice_content = slice_var.string_value();
      flx_string topic = topic_var.string_value();
      // Add the new snippet with the topic
      add_snippet(snippet(flxv_map{{"topic", topic}}, std::move(slice_content)));
    }
    const flx_variant& last_topic_var = slices.back().map_value().at("topic");
    last_topic = last_topic_var.string_value(); // Update the last topic based on the last slice
  }
}
