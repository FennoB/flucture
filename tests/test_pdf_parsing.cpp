#include <catch2/catch_all.hpp>
#include "../documents/pdf/flx_pdf_sio.h"
#include "../documents/layout/flx_layout_vertex.h"
#include <filesystem>
#include <fstream>

SCENARIO("PDF → Layout extraction pipeline") {
    GIVEN("An existing PDF generated from known layout") {
        flx_pdf_sio pdf_doc;
        
        WHEN("Creating a simple test document and parsing it back") {
            // Create a simple layout structure
            auto& page = pdf_doc.add_page();
            page.width = 595.0;
            page.height = 842.0;
            
            // Add simple geometry
            flx_layout_geometry test_rect;
            test_rect.fill_color = "#FF0000";
            
            flx_layout_vertex v1(100.0, 100.0);
            flx_layout_vertex v2(200.0, 100.0);
            flx_layout_vertex v3(200.0, 200.0);
            flx_layout_vertex v4(100.0, 200.0);
            test_rect.vertices.push_back(v1);
            test_rect.vertices.push_back(v2);
            test_rect.vertices.push_back(v3);
            test_rect.vertices.push_back(v4);
            
            // Add test text
            flx_layout_text test_text;
            test_text.x = 110.0;
            test_text.y = 150.0;
            test_text.text = "Test Text";
            test_text.font_size = 12.0;
            test_rect.add_text(test_text);
            
            page.add_sub_geometry(test_rect);
            
            // Generate PDF
            flx_string pdf_data;
            bool serialize_success = pdf_doc.serialize(pdf_data);
            REQUIRE(serialize_success == true);
            REQUIRE(pdf_data.size() > 0);
            
            // Parse the generated PDF back to layout (parse() now does everything)
            bool parse_success = pdf_doc.parse(pdf_data);
            
            THEN("Basic parsing pipeline should execute without errors") {
                REQUIRE(parse_success == true);
                std::cout << "✓ PDF → Layout parsing pipeline initiated successfully" << std::endl;
                
                // Verify that parsed document has the expected structure
                REQUIRE(pdf_doc.pages.size() > 0);
                auto& parsed_page = pdf_doc.pages[0];
                
                // This is the main goal: texts should be in pages[0].texts with positions!
                REQUIRE(parsed_page.texts.size() > 0);
                auto& first_text = parsed_page.texts[0];
                
                // Verify text has position data
                REQUIRE(first_text.x >= 0.0);
                REQUIRE(first_text.y >= 0.0);
                REQUIRE(first_text.text->size() > 0);
                
                std::cout << "✅ SUCCESS: Found " << parsed_page.texts.size() 
                         << " texts in pages[0].texts with positions!" << std::endl;
                std::cout << "   Text: \"" << first_text.text->c_str() << "\"" << std::endl;
                std::cout << "   Position: (" << first_text.x << ", " << first_text.y << ")" << std::endl;
                std::cout << "   Size: (" << first_text.width << " x " << first_text.height << ")" << std::endl;
                std::cout << "   Font: " << first_text.font_family->c_str() 
                         << ", Size: " << first_text.font_size << std::endl;
                std::cout << "   Color: " << first_text.color->c_str() 
                         << ", Bold: " << (first_text.bold ? "yes" : "no")
                         << ", Italic: " << (first_text.italic ? "yes" : "no") << std::endl;
            }
        }
    }
}

SCENARIO("Round-trip testing framework") {
    GIVEN("A test document with known ground truth") {
        WHEN("Performing full round-trip: Layout → PDF → Layout") {
            // This will be the foundation for AI evaluation
            std::cout << "Round-trip testing framework ready for implementation" << std::endl;
            
            THEN("Framework should be prepared for AI evaluator development") {
                REQUIRE(true); // Placeholder for future implementation
            }
        }
    }
}