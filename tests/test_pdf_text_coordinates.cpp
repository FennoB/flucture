#include <catch2/catch_all.hpp>
#include "../documents/layout/flx_layout_text.h"
#include "../documents/layout/flx_layout_geometry.h"
#include "../documents/pdf/flx_pdf_sio.h"
#include <iostream>

SCENARIO("PDF text extraction preserves coordinates", "[pdf][text][coordinates]") {
    GIVEN("A layout with texts at different positions") {
        // Create a simple layout with 3 texts at different positions
        flx_layout_geometry root;
        root.x = 0;
        root.y = 0;
        root.width = 595;  // A4 width in points
        root.height = 842; // A4 height in points

        // Text 1: Top-left
        flx_layout_text text1;
        text1.x = 100;
        text1.y = 100;
        text1.text = "Text at 100,100";
        text1.font_family = "Arial";
        text1.font_size = 12;
        text1.color = "#000000";
        root.texts.push_back(text1);

        // Text 2: Middle
        flx_layout_text text2;
        text2.x = 200;
        text2.y = 400;
        text2.text = "Text at 200,400";
        text2.font_family = "Arial";
        text2.font_size = 12;
        text2.color = "#000000";
        root.texts.push_back(text2);

        // Text 3: Bottom-right
        flx_layout_text text3;
        text3.x = 400;
        text3.y = 700;
        text3.text = "Text at 400,700";
        text3.font_family = "Arial";
        text3.font_size = 12;
        text3.color = "#000000";
        root.texts.push_back(text3);

        WHEN("Converting to PDF and back to layout") {
            // Create PDF
            flx_pdf_sio pdf_writer;
            pdf_writer.pages.push_back(root);

            flx_string pdf_data;
            REQUIRE(pdf_writer.serialize(pdf_data));

            std::cout << "\n=== Created PDF with " << root.texts.size() << " texts ===" << std::endl;
            std::cout << "Original positions:" << std::endl;
            std::cout << "  Text1: (" << text1.x << ", " << text1.y << ")" << std::endl;
            std::cout << "  Text2: (" << text2.x << ", " << text2.y << ")" << std::endl;
            std::cout << "  Text3: (" << text3.x << ", " << text3.y << ")" << std::endl;

            // Parse PDF back
            flx_pdf_sio pdf_reader;
            REQUIRE(pdf_reader.parse(pdf_data));

            THEN("Extracted texts should have correct coordinates") {
                REQUIRE(pdf_reader.pages.size() > 0);

                auto& extracted_page = pdf_reader.pages[0];

                std::cout << "\n=== Extracted " << extracted_page.texts.size() << " texts ===" << std::endl;

                REQUIRE(extracted_page.texts.size() == 3);

                // Check each extracted text
                for (int i = 0; i < 3; i++) {
                    auto& extracted_text = extracted_page.texts[i];
                    std::cout << "Extracted Text[" << i << "]: "
                              << "X=" << extracted_text.x << " "
                              << "Y=" << extracted_text.y << " "
                              << "\"" << extracted_text.text->c_str() << "\"" << std::endl;
                }

                // Verify coordinates are different (not all the same)
                bool all_same_x = (extracted_page.texts[0].x == extracted_page.texts[1].x) &&
                                  (extracted_page.texts[1].x == extracted_page.texts[2].x);
                bool all_same_y = (extracted_page.texts[0].y == extracted_page.texts[1].y) &&
                                  (extracted_page.texts[1].y == extracted_page.texts[2].y);

                REQUIRE_FALSE(all_same_x);
                REQUIRE_FALSE(all_same_y);

                std::cout << "\n✅ SUCCESS: Texts have different coordinates!" << std::endl;
            }
        }
    }
}
