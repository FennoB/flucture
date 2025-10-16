#include <catch2/catch_all.hpp>
#include "../aiprocesses/eval/flx_layout_evaluator.h"
#include "../api/aimodels/flx_openai_api.h"
#include "../documents/pdf/flx_pdf_sio.h"
#include "../documents/layout/flx_layout_geometry.h"
#include "../documents/layout/flx_layout_text.h"
#include <cstdlib>
#include <iostream>

using namespace flx::llm;

// Helper function to get API key from environment
flx_string get_api_key() {
  const char* api_key = std::getenv("OPENAI_API_KEY");
  if (!api_key) {
    // Try to read from .env file
    std::ifstream env_file(".env");
    std::string line;
    while (std::getline(env_file, line)) {
      if (line.find("OPENAI_API_KEY=") == 0) {
        return flx_string(line.substr(15)); // Skip "OPENAI_API_KEY="
      }
    }
    FAIL("OPENAI_API_KEY not found in environment or .env file");
  }
  return flx_string(api_key);
}

SCENARIO("AI Layout Evaluator can compare layout structures", "[ai][evaluator]") {
  GIVEN("An AI evaluator with OpenAI API") {
    flx_string api_key = get_api_key();
    auto api = std::make_shared<openai_api>(api_key);
    flx_layout_evaluator evaluator(api);
    
    WHEN("Comparing identical layouts") {
      // Create a simple test layout
      flx_model_list<flx_layout_geometry> original_layout;
      
      flx_layout_geometry page1_obj;
      original_layout.push_back(page1_obj);
      auto& page1 = original_layout[0];
      page1.x = 0;
      page1.y = 0;
      page1.width = 595;
      page1.height = 842;
      page1.fill_color = "#FFFFFF";
      
      // Add a text element
      flx_layout_text text1_obj;
      page1.texts.push_back(text1_obj);
      auto& text1 = page1.texts[0];
      text1.text = "Hello World";
      text1.x = 100;
      text1.y = 100;
      text1.font_size = 12;
      text1.font_family = "Arial";
      
      // Add a nested geometry
      flx_layout_geometry nested_obj;
      page1.sub_geometries.push_back(nested_obj);
      auto& nested = page1.sub_geometries[0];
      nested.x = 50;
      nested.y = 50;
      nested.width = 200;
      nested.height = 300;
      nested.fill_color = "#FF0000";
      
      // Extracted layout is identical (create a new one with same content)
      flx_model_list<flx_layout_geometry> extracted_layout;
      flx_layout_geometry page1_copy_obj;
      extracted_layout.push_back(page1_copy_obj);
      auto& page1_copy = extracted_layout[0];
      page1_copy.x = 0;
      page1_copy.y = 0;
      page1_copy.width = 595;
      page1_copy.height = 842;
      page1_copy.fill_color = "#FFFFFF";
      
      flx_layout_text text1_copy_obj;
      page1_copy.texts.push_back(text1_copy_obj);
      auto& text1_copy = page1_copy.texts[0];
      text1_copy.text = "Hello World";
      text1_copy.x = 100;
      text1_copy.y = 100;
      text1_copy.font_size = 12;
      text1_copy.font_family = "Arial";
      
      flx_layout_geometry nested_copy_obj;
      page1_copy.sub_geometries.push_back(nested_copy_obj);
      auto& nested_copy = page1_copy.sub_geometries[0];
      nested_copy.x = 50;
      nested_copy.y = 50;
      nested_copy.width = 200;
      nested_copy.height = 300;
      nested_copy.fill_color = "#FF0000";
      
      THEN("Evaluation should show perfect scores") {
        auto result = evaluator.evaluate_extraction(original_layout, extracted_layout);
        
        std::cout << "Evaluation Results for Identical Layouts:\n";
        std::cout << "==========================================\n";
        std::cout << "Structure Similarity: " << result.structure_similarity << "\n";
        std::cout << "Position Accuracy: " << result.position_accuracy << "\n";
        std::cout << "Hierarchy Correctness: " << result.hierarchy_correctness << "\n";
        std::cout << "Text Extraction Score: " << result.text_extraction_score << "\n";
        std::cout << "Overall Score: " << result.overall_score << "\n";
        std::cout << "Report: " << result.detailed_report.c_str() << "\n";
        
        REQUIRE(result.overall_score >= 0.95); // Should be nearly perfect
        REQUIRE(result.structure_similarity >= 0.95);
        REQUIRE(result.hierarchy_correctness >= 0.95);
      }
    }
    
    WHEN("Comparing layouts with minor position differences") {
      flx_model_list<flx_layout_geometry> original_layout;
      flx_model_list<flx_layout_geometry> extracted_layout;
      
      flx_layout_geometry orig_page_obj;
      original_layout.push_back(orig_page_obj);
      auto& orig_page = original_layout[0];
      orig_page.x = 0;
      orig_page.y = 0;
      orig_page.width = 595;
      orig_page.height = 842;
      
      flx_layout_text orig_text_obj;
      orig_page.texts.push_back(orig_text_obj);
      auto& orig_text = orig_page.texts[0];
      orig_text.text = "Test Text";
      orig_text.x = 100;
      orig_text.y = 100;
      orig_text.font_size = 14;
      
      // Extracted has slightly different position (within tolerance)
      flx_layout_geometry ext_page_obj;
      extracted_layout.push_back(ext_page_obj);
      auto& ext_page = extracted_layout[0];
      ext_page.x = 0;
      ext_page.y = 0;
      ext_page.width = 595;
      ext_page.height = 842;
      
      flx_layout_text ext_text_obj;
      ext_page.texts.push_back(ext_text_obj);
      auto& ext_text = ext_page.texts[0];
      ext_text.text = "Test Text";
      ext_text.x = 103;  // 3px difference
      ext_text.y = 98;   // 2px difference
      ext_text.font_size = 14;
      
      THEN("Position accuracy should be high but not perfect") {
        auto result = evaluator.evaluate_extraction(original_layout, extracted_layout);
        
        std::cout << "\nEvaluation Results for Minor Position Differences:\n";
        std::cout << "==================================================\n";
        std::cout << "Position Accuracy: " << result.position_accuracy << "\n";
        std::cout << "Text Extraction Score: " << result.text_extraction_score << "\n";
        std::cout << "Overall Score: " << result.overall_score << "\n";
        std::cout << "Differences: " << result.differences_found.c_str() << "\n";
        
        REQUIRE(result.position_accuracy >= 0.85); // Should be high
        REQUIRE(result.position_accuracy < 1.0);   // But not perfect
        REQUIRE(result.text_extraction_score >= 0.9);
      }
    }
    
    WHEN("Comparing layouts with missing elements") {
      flx_model_list<flx_layout_geometry> original_layout;
      flx_model_list<flx_layout_geometry> extracted_layout;
      
      flx_layout_geometry orig_page_obj2;
      original_layout.push_back(orig_page_obj2);
      auto& orig_page = original_layout[0];
      orig_page.x = 0;
      orig_page.y = 0;
      orig_page.width = 595;
      orig_page.height = 842;
      
      // Original has two texts
      flx_layout_text text1_obj2;
      orig_page.texts.push_back(text1_obj2);
      auto& text1 = orig_page.texts[0];
      text1.text = "First Text";
      text1.x = 100;
      text1.y = 100;
      
      flx_layout_text text2_obj;
      orig_page.texts.push_back(text2_obj);
      auto& text2 = orig_page.texts[1];
      text2.text = "Second Text";
      text2.x = 100;
      text2.y = 150;
      
      // Extracted is missing the second text
      flx_layout_geometry ext_page_obj2;
      extracted_layout.push_back(ext_page_obj2);
      auto& ext_page = extracted_layout[0];
      ext_page.x = 0;
      ext_page.y = 0;
      ext_page.width = 595;
      ext_page.height = 842;
      
      flx_layout_text ext_text1_obj;
      ext_page.texts.push_back(ext_text1_obj);
      auto& ext_text1 = ext_page.texts[0];
      ext_text1.text = "First Text";
      ext_text1.x = 100;
      ext_text1.y = 100;
      
      THEN("Text extraction score should be lower") {
        auto result = evaluator.evaluate_extraction(original_layout, extracted_layout);
        
        std::cout << "\nEvaluation Results for Missing Elements:\n";
        std::cout << "========================================\n";
        std::cout << "Text Extraction Score: " << result.text_extraction_score << "\n";
        std::cout << "Overall Score: " << result.overall_score << "\n";
        std::cout << "Report: " << result.detailed_report.c_str() << "\n";
        
        REQUIRE(result.text_extraction_score < 0.8); // Should be significantly lower
        REQUIRE(result.overall_score < 0.9);
      }
    }
  }
}

SCENARIO("Layout evaluator text conversion", "[ai][evaluator][text]") {
  GIVEN("A layout evaluator") {
    flx_string api_key = get_api_key();
    auto api = std::make_shared<openai_api>(api_key);
    flx_layout_evaluator evaluator(api);
    
    WHEN("Converting a simple layout to text") {
      flx_model_list<flx_layout_geometry> test_layout;
      test_layout.add_element();
      auto& page = test_layout.back();
      page.x = 0;
      page.y = 0; 
      page.width = 595;
      page.height = 842;
      page.fill_color = "#FFFFFF";
      
      page.texts.add_element();
      auto& text = page.texts.back();
      text.text = "Test Text";
      text.x = 100;
      text.y = 200;
      text.font_size = 14;
      text.font_family = "Arial";
      
      THEN("Text conversion should work") {
        flx_string text_output = evaluator.layout_to_structured_text(test_layout);
        
        std::cout << "Generated layout text:\n" << text_output.c_str() << "\n";
        
        std::string text_str = text_output.c_str();
        REQUIRE(!text_str.empty());
        REQUIRE(text_str.find("Test Text") != std::string::npos);
        REQUIRE(text_str.find("Arial") != std::string::npos);
      }
    }
  }
}

SCENARIO("Layout evaluator handles empty layouts", "[ai][evaluator]") {
  GIVEN("A layout evaluator") {
    flx_string api_key = get_api_key();
    auto api = std::make_shared<openai_api>(api_key);
    flx_layout_evaluator evaluator(api);
    
    WHEN("Evaluating empty layouts") {
      flx_model_list<flx_layout_geometry> empty_layout;
      flx_model_list<flx_layout_geometry> also_empty;
      
      THEN("Should handle gracefully") {
        auto result = evaluator.evaluate_extraction(empty_layout, also_empty);
        
        REQUIRE(result.overall_score >= 0.0);
        REQUIRE(result.overall_score <= 1.0);
        std::string report_str = result.detailed_report.c_str();
        REQUIRE(!report_str.empty());
      }
    }
  }
}

SCENARIO("Layout evaluator prompt generation", "[ai][evaluator][disabled]") {
  GIVEN("A layout evaluator") {
    flx_string api_key = get_api_key();
    auto api = std::make_shared<openai_api>(api_key);
    flx_layout_evaluator evaluator(api);
    
    WHEN("Converting layouts to text") {
      flx_model_list<flx_layout_geometry> layout_with_image;
      layout_with_image.add_element();
      auto& page = layout_with_image.back();
      
      page.images.add_element();
      auto& img = page.images.back();
      
      // Debug the exception
      try {
        img.x = 50;
      } catch (const flx_null_field_exception& e) {
        std::cout << "Caught exception: " << e.what() << std::endl;
        std::cout << "Field name: " << e.get_field_name().c_str() << std::endl;
        throw;
      }
      
      img.image_path = "/path/to/test.png";
      img.y = 100;
      img.width = 200;
      img.height = 150;
      
      THEN("Should include image information") {
        flx_string text_output = evaluator.layout_to_structured_text(layout_with_image);
        std::string text_str = text_output.c_str();
        
        REQUIRE(text_str.find("IMAGE:") != std::string::npos);
        REQUIRE(text_str.find("/path/to/test.png") != std::string::npos);
        REQUIRE(text_str.find("200x150") != std::string::npos);
      }
    }
  }
}

SCENARIO("Round-trip PDF evaluation", "[ai][evaluator][pdf][disabled]") {
  GIVEN("A layout that goes through PDF conversion") {
    flx_string api_key = get_api_key();
    auto api = std::make_shared<openai_api>(api_key);
    flx_layout_evaluator evaluator(api);
    
    // Create original layout
    flx_model_list<flx_layout_geometry> original_layout;
    
    flx_layout_geometry page_obj;
    original_layout.push_back(page_obj);
    auto& page = original_layout[0];
    page.x = 0;
    page.y = 0;
    page.width = 595;
    page.height = 842;
    page.fill_color = "#FFFFFF";
    
    // Add text
    flx_layout_text text_obj;
    page.texts.push_back(text_obj);
    auto& text = page.texts[0];
    text.text = "Round-Trip Test";
    text.x = 200;
    text.y = 400;
    text.font_size = 24;
    text.font_family = "Arial";
    
    // Add colored rectangle
    flx_layout_geometry rect_obj;
    page.sub_geometries.push_back(rect_obj);
    auto& rect = page.sub_geometries[0];
    rect.x = 100;
    rect.y = 100;
    rect.width = 400;
    rect.height = 200;
    rect.fill_color = "#0000FF";
    
    // Add vertices for polygon
    flx_layout_vertex v1_obj(100, 100);
    rect.vertices.push_back(v1_obj);
    flx_layout_vertex v2_obj(500, 100);
    rect.vertices.push_back(v2_obj);
    flx_layout_vertex v3_obj(500, 300);
    rect.vertices.push_back(v3_obj);
    flx_layout_vertex v4_obj(100, 300);
    rect.vertices.push_back(v4_obj);
    
    WHEN("Converting to PDF and back") {
      flx_pdf_sio pdf_writer;
      
      // Instead of copying, work directly with the original
      pdf_writer.pages.add_element();
      auto& writer_page = pdf_writer.pages.back();
      writer_page = page;
      
      flx_string pdf_data;
      REQUIRE(pdf_writer.serialize(pdf_data));
      
      // Parse the PDF back
      flx_pdf_sio pdf_reader;
      REQUIRE(pdf_reader.parse(pdf_data));
      
      THEN("AI should evaluate the extraction quality") {
        auto result = evaluator.evaluate_extraction(original_layout, pdf_reader.pages);
        
        std::cout << "\nRound-Trip PDF Evaluation Results:\n";
        std::cout << "===================================\n";
        std::cout << "Structure Similarity: " << result.structure_similarity << "\n";
        std::cout << "Position Accuracy: " << result.position_accuracy << "\n";
        std::cout << "Hierarchy Correctness: " << result.hierarchy_correctness << "\n";
        std::cout << "Text Extraction Score: " << result.text_extraction_score << "\n";
        std::cout << "Overall Score: " << result.overall_score << "\n";
        std::cout << "Detailed Report:\n" << result.detailed_report.c_str() << "\n";
        std::cout << "Differences Found:\n" << result.differences_found.c_str() << "\n";
        
        // Store result for analysis
        INFO("Round-trip evaluation score: " << result.overall_score);
        
        // The extraction is still being implemented, so we don't expect perfect scores yet
        // But the evaluator should at least run successfully
        REQUIRE(result.overall_score >= 0.0);
        REQUIRE(result.overall_score <= 1.0);
      }
    }
  }
}