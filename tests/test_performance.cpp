#include <catch2/catch_all.hpp>
#include "../documents/pdf/flx_pdf_sio.h"
#include "../documents/layout/flx_layout_vertex.h"
#include <chrono>
#include <iostream>

SCENARIO("PDF text extraction performance benchmarks") {
    GIVEN("Complex PDFs with multiple pages and text elements") {
        
        WHEN("Processing a medium-sized PDF with 1000+ text elements") {
            flx_pdf_sio pdf_doc;
            
            // Create a PDF with multiple pages and lots of text
            for (int page_num = 0; page_num < 5; page_num++) {
                auto& page = pdf_doc.add_page();
                page.width = 595.0;
                page.height = 842.0;
                
                // Add 200 text elements per page (1000 total)
                for (int i = 0; i < 200; i++) {
                    flx_layout_geometry text_container;
                    text_container.fill_color = "#FFFFFF";
                    
                    flx_layout_text test_text;
                    test_text.x = 50.0 + (i % 10) * 50.0;
                    test_text.y = 50.0 + (i / 10) * 30.0;
                    test_text.text = flx_string("Performance test text element number ") + flx_string(std::to_string(i));
                    test_text.font_size = 12.0;
                    test_text.font_family = flx_string("Arial");
                    
                    text_container.add_text(test_text);
                    page.add_sub_geometry(text_container);
                }
                
                std::cout << "Created page " << (page_num + 1) << " with 200 text elements" << std::endl;
            }
            
            // Generate PDF
            flx_string pdf_data;
            auto serialize_start = std::chrono::high_resolution_clock::now();
            bool serialize_success = pdf_doc.serialize(pdf_data);
            auto serialize_end = std::chrono::high_resolution_clock::now();
            
            auto serialize_duration = std::chrono::duration_cast<std::chrono::milliseconds>(serialize_end - serialize_start);
            std::cout << "📝 PDF serialization took: " << serialize_duration.count() << " ms" << std::endl;
            
            REQUIRE(serialize_success == true);
            REQUIRE(pdf_data.size() > 0);
            std::cout << "Generated PDF size: " << pdf_data.size() << " bytes" << std::endl;
            
            THEN("Text extraction should be fast") {
                auto extraction_start = std::chrono::high_resolution_clock::now();
                bool parse_success = pdf_doc.parse(pdf_data);
                auto extraction_end = std::chrono::high_resolution_clock::now();
                
                auto extraction_duration = std::chrono::duration_cast<std::chrono::milliseconds>(extraction_end - extraction_start);
                std::cout << "🚀 PDF text extraction took: " << extraction_duration.count() << " ms" << std::endl;
                
                REQUIRE(parse_success == true);
                REQUIRE(pdf_doc.pages.size() > 0);
                
                // Count extracted texts
                int total_extracted_texts = 0;
                for (int i = 0; i < pdf_doc.pages.size(); i++) {
                    total_extracted_texts += pdf_doc.pages[i].texts.size();
                }
                
                std::cout << "✅ Extracted " << total_extracted_texts << " text elements total" << std::endl;
                std::cout << "⚡ Performance: " << (double)total_extracted_texts / extraction_duration.count() << " texts per millisecond" << std::endl;
                
                // Performance requirements
                REQUIRE(extraction_duration.count() < 5000); // Should be under 5 seconds
                REQUIRE(total_extracted_texts > 900); // Should extract most texts
                
                // Target: 100x speedup means we want < 50ms for 1000 texts
                if (extraction_duration.count() > 50) {
                    std::cout << "⚠️  PERFORMANCE WARNING: Extraction too slow!" << std::endl;
                    std::cout << "    Current: " << extraction_duration.count() << " ms" << std::endl;
                    std::cout << "    Target:  < 50 ms (100x speedup)" << std::endl;
                }
            }
        }
    }
}