#include "flx_layout_evaluator.h"
#include <sstream>
#include <iomanip>
#include <regex>

namespace flx::llm {

flx_layout_evaluator::flx_layout_evaluator(std::shared_ptr<i_llm_api> api_impl)
    : api(std::move(api_impl)) {
  chat = std::make_unique<flx_llm_chat>(api);
}

flx_layout_evaluator::~flx_layout_evaluator() = default;

flx_string flx_layout_evaluator::layout_to_structured_text(const flx_model_list<flx_layout_geometry>& layout) {
  std::stringstream ss;
  ss << "Layout Structure:\n";
  ss << "================\n";
  
  // Cast away const to access elements (flx_model_list doesn't have const operator[])
  auto& mutable_layout = const_cast<flx_model_list<flx_layout_geometry>&>(layout);
  
  for (size_t i = 0; i < mutable_layout.size(); ++i) {
    ss << "Page " << (i + 1) << ":\n";
    flx_string geom_text = geometry_to_text(mutable_layout[i], 1);
    ss << geom_text.c_str();
    ss << "\n";
  }
  
  return flx_string(ss.str());
}

flx_string flx_layout_evaluator::geometry_to_text(const flx_layout_geometry& geom, int indent_level) {
  std::stringstream ss;
  std::string indent(indent_level * 2, ' ');
  
  ss << indent << "Geometry [" << std::fixed << std::setprecision(1) 
     << geom.x << "," << geom.y << "," << geom.width << "x" << geom.height << "]";
  
  if (!geom.fill_color->empty()) {
    ss << " fill=" << geom.fill_color->c_str();
  }
  
  if (geom.vertices.size() > 0) {
    ss << " vertices=" << geom.vertices.size();
  }
  
  ss << "\n";
  
  // Add texts
  auto& mutable_geom_texts = const_cast<flx_layout_geometry&>(geom);
  for (size_t i = 0; i < mutable_geom_texts.texts.size(); ++i) {
    auto& text = mutable_geom_texts.texts[i];
    ss << indent << "  TEXT: \"" << text.text->c_str() << "\" "
       << "[" << text.x << "," << text.y << "] "
       << "font=" << text.font_size << "pt";
    if (!text.font_family->empty()) {
      ss << " " << text.font_family->c_str();
    }
    ss << "\n";
  }
  
  // Add images
  auto& mutable_geom_images = const_cast<flx_layout_geometry&>(geom);
  for (size_t i = 0; i < mutable_geom_images.images.size(); ++i) {
    auto& img = mutable_geom_images.images[i];
    ss << indent << "  IMAGE: \"" << img.image_path->c_str() << "\" "
       << "[" << img.x << "," << img.y << "," 
       << img.width << "x" << img.height << "]\n";
  }
  
  // Recursively process sub-geometries
  auto& mutable_geom = const_cast<flx_layout_geometry&>(geom);
  for (size_t i = 0; i < mutable_geom.sub_geometries.size(); ++i) {
    flx_string sub_text = geometry_to_text(mutable_geom.sub_geometries[i], indent_level + 1);
    ss << sub_text.c_str();
  }
  
  return flx_string(ss.str());
}

flx_string flx_layout_evaluator::create_evaluation_prompt(const flx_string& original_layout, 
                                                         const flx_string& extracted_layout) {
  std::stringstream ss;
  
  ss << "You are an AI evaluator for document layout extraction quality. "
     << "Compare the original layout structure with the extracted layout and provide scores.\n\n";
  
  ss << "ORIGINAL LAYOUT:\n"
     << original_layout.c_str() << "\n\n";
  
  ss << "EXTRACTED LAYOUT:\n"
     << extracted_layout.c_str() << "\n\n";
  
  ss << "Please evaluate the extraction quality and respond in EXACTLY this JSON format:\n"
     << "{\n"
     << "  \"structure_similarity\": 0.0-1.0,\n"
     << "  \"position_accuracy\": 0.0-1.0,\n"
     << "  \"hierarchy_correctness\": 0.0-1.0,\n"
     << "  \"text_extraction_score\": 0.0-1.0,\n"
     << "  \"image_detection_score\": 0.0-1.0,\n"
     << "  \"overall_score\": 0.0-1.0,\n"
     << "  \"detailed_report\": \"Detailed analysis...\",\n"
     << "  \"differences_found\": \"List of specific differences...\"\n"
     << "}\n\n"
     << "Scoring guidelines:\n"
     << "- structure_similarity: How well the overall structure matches\n"
     << "- position_accuracy: How accurate are the coordinates (allow 5px tolerance)\n"
     << "- hierarchy_correctness: Is the nesting/containment preserved?\n"
     << "- text_extraction_score: Are all texts extracted with correct properties?\n"
     << "- image_detection_score: Are all images detected with correct bounds?\n"
     << "- overall_score: Weighted average of all scores\n";
  
  return flx_string(ss.str());
}

layout_evaluation_result flx_layout_evaluator::parse_evaluation_response(const flx_string& response) {
  layout_evaluation_result result;
  
  // Simple regex-based parsing for JSON response
  std::string resp = response.c_str();
  
  auto extract_double = [&resp](const std::string& key) -> double {
    std::regex pattern("\"" + key + "\"\\s*:\\s*([0-9.]+)");
    std::smatch match;
    if (std::regex_search(resp, match, pattern)) {
      return std::stod(match[1]);
    }
    return 0.0;
  };
  
  auto extract_string = [&resp](const std::string& key) -> flx_string {
    std::regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]+)\"");
    std::smatch match;
    if (std::regex_search(resp, match, pattern)) {
      return flx_string(match[1]);
    }
    return flx_string("");
  };
  
  result.structure_similarity = extract_double("structure_similarity");
  result.position_accuracy = extract_double("position_accuracy");
  result.hierarchy_correctness = extract_double("hierarchy_correctness");
  result.text_extraction_score = extract_double("text_extraction_score");
  result.image_detection_score = extract_double("image_detection_score");
  result.overall_score = extract_double("overall_score");
  result.detailed_report = extract_string("detailed_report");
  result.differences_found = extract_string("differences_found");
  
  return result;
}

layout_evaluation_result flx_layout_evaluator::evaluate_extraction(
    const flx_model_list<flx_layout_geometry>& original_layout,
    const flx_model_list<flx_layout_geometry>& extracted_layout) {
  
  // Convert layouts to structured text
  flx_string original_text = layout_to_structured_text(original_layout);
  flx_string extracted_text = layout_to_structured_text(extracted_layout);
  
  // Create evaluation prompt
  flx_string prompt = create_evaluation_prompt(original_text, extracted_text);
  
  // Setup chat context with GPT-4 for best analysis
  flxv_map settings;
  settings["model"] = flx_string("gpt-4-turbo-preview");
  settings["temperature"] = 0.1;  // Low temperature for consistent evaluation
  
  // Enable structured JSON output
  flxv_map response_format;
  response_format["type"] = flx_string("json_object");
  settings["response_format"] = response_format;
  
  chat->create_context(settings);
  
  // Add system message to context
  auto system_msg = api->create_message(message_role::SYSTEM, 
    flx_string("You are a precise document layout evaluation AI. Always respond with valid JSON."));
  
  // Create a new context with system message
  auto new_context = api->create_chat_context();
  new_context->set_settings(settings);
  new_context->add_message(std::move(system_msg));
  chat->set_context(std::move(new_context));
  
  // Get evaluation from AI
  flx_string ai_response;
  if (!chat->chat(prompt, ai_response)) {
    // Return default result on failure
    layout_evaluation_result result;
    result.overall_score = 0.0;
    result.detailed_report = "Evaluation failed - API error";
    return result;
  }
  
  // Parse and return result
  return parse_evaluation_response(ai_response);
}

void flx_layout_evaluator::set_tolerance(double coordinate_tolerance, double color_tolerance) {
  // Store tolerances for future use in evaluation
  // TODO: Implement tolerance configuration
}

void flx_layout_evaluator::enable_detailed_analysis(bool enable) {
  // Configure detailed analysis mode
  // TODO: Implement detailed analysis toggle
}

} // namespace flx::llm