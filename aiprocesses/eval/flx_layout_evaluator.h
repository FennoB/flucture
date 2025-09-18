#ifndef FLX_LAYOUT_EVALUATOR_H
#define FLX_LAYOUT_EVALUATOR_H

#include "../chat/flx_llm_chat.h"
#include "../../documents/layout/flx_layout_geometry.h"
#include "../../utils/flx_string.h"
#include <memory>

namespace flx::llm {

struct layout_evaluation_result {
  double structure_similarity;  // 0.0-1.0 structural similarity score
  double position_accuracy;     // 0.0-1.0 coordinate accuracy score  
  double hierarchy_correctness; // 0.0-1.0 nesting correctness score
  double text_extraction_score; // 0.0-1.0 text extraction quality
  double image_detection_score; // 0.0-1.0 image detection accuracy
  double overall_score;         // 0.0-1.0 weighted overall score
  flx_string detailed_report;   // Detailed AI analysis report
  flx_string differences_found;  // List of specific differences
};

class flx_layout_evaluator {
private:
  std::shared_ptr<i_llm_api> api;
  std::unique_ptr<flx_llm_chat> chat;
  
  // Convert layout to structured text representation for AI
  flx_string layout_to_structured_text(const flx_model_list<flx_layout_geometry>& layout);
  flx_string geometry_to_text(const flx_layout_geometry& geom, int indent_level = 0);
  
  // Create evaluation prompt
  flx_string create_evaluation_prompt(const flx_string& original_layout, 
                                     const flx_string& extracted_layout);
  
  // Parse AI response into scores
  layout_evaluation_result parse_evaluation_response(const flx_string& response);

public:
  explicit flx_layout_evaluator(std::shared_ptr<i_llm_api> api_impl);
  ~flx_layout_evaluator();
  
  // Main evaluation function
  layout_evaluation_result evaluate_extraction(
    const flx_model_list<flx_layout_geometry>& original_layout,
    const flx_model_list<flx_layout_geometry>& extracted_layout
  );
  
  // Configure evaluation parameters
  void set_tolerance(double coordinate_tolerance, double color_tolerance);
  void enable_detailed_analysis(bool enable);
};

} // namespace flx::llm

#endif // FLX_LAYOUT_EVALUATOR_H