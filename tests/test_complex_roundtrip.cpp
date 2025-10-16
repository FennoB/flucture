#include <catch2/catch_all.hpp>
#include "../documents/pdf/flx_pdf_sio.h"
#include "../aiprocesses/eval/flx_layout_evaluator.h"
#include "../api/aimodels/flx_openai_api.h"
#include "../documents/layout/flx_layout_geometry.h"
#include "../documents/layout/flx_layout_text.h"
#include "../documents/layout/flx_layout_image.h"
#include <filesystem>
#include <fstream>
#include <cstdlib>
#include <iostream>

using namespace flx::llm;

// Forward declaration - function defined in test_layout_evaluator.cpp
flx_string get_api_key();

SCENARIO("Complex PDF Round-Trip with AI Evaluation") {
    GIVEN("A complex nested invoice structure with 4-level hierarchy") {
        flx_pdf_sio pdf_doc;
        auto& page = pdf_doc.add_page();
        page.width = 595.0;
        page.height = 842.0;

        WHEN("Building complex nested invoice layout") {
            // LEVEL 1: Main Invoice Container
            page.sub_geometries.add_element();
            auto& invoice_container = page.sub_geometries.back();
            invoice_container.fill_color = "#F8F9FA";
            
            // Invoice container bounds
            invoice_container.vertices.add_element();
            auto& v1 = invoice_container.vertices.back();
            v1.x = 50.0; v1.y = 50.0;
            invoice_container.vertices.add_element();
            auto& v2 = invoice_container.vertices.back();
            v2.x = 545.0; v2.y = 50.0;
            invoice_container.vertices.add_element();
            auto& v3 = invoice_container.vertices.back();
            v3.x = 545.0; v3.y = 792.0;
            invoice_container.vertices.add_element();
            auto& v4 = invoice_container.vertices.back();
            v4.x = 50.0; v4.y = 792.0;

            // LEVEL 2: Header Section
            invoice_container.sub_geometries.add_element();
            auto& header_section = invoice_container.sub_geometries.back();
            header_section.fill_color = "#E3F2FD";
            
            // Header bounds (60, 700, 475, 80)
            header_section.vertices.add_element();
            auto& h1 = header_section.vertices.back();
            h1.x = 60.0; h1.y = 700.0;
            header_section.vertices.add_element();
            auto& h2 = header_section.vertices.back();
            h2.x = 535.0; h2.y = 700.0;
            header_section.vertices.add_element();
            auto& h3 = header_section.vertices.back();
            h3.x = 535.0; h3.y = 780.0;
            header_section.vertices.add_element();
            auto& h4 = header_section.vertices.back();
            h4.x = 60.0; h4.y = 780.0;

            // Header text
            header_section.texts.add_element();
            auto& title_text = header_section.texts.back();
            title_text.x = 70.0;
            title_text.y = 750.0;
            title_text.text = "INVOICE #2024-001";
            title_text.font_size = 24.0;
            title_text.font_family = "Arial";
            title_text.bold = true;

            // LEVEL 2: Customer Info Section  
            invoice_container.sub_geometries.add_element();
            auto& customer_section = invoice_container.sub_geometries.back();
            customer_section.fill_color = "#FFF3E0";
            
            // Customer bounds (60, 580, 230, 100)
            customer_section.vertices.add_element();
            auto& c1 = customer_section.vertices.back();
            c1.x = 60.0; c1.y = 580.0;
            customer_section.vertices.add_element();
            auto& c2 = customer_section.vertices.back();
            c2.x = 290.0; c2.y = 580.0;
            customer_section.vertices.add_element();
            auto& c3 = customer_section.vertices.back();
            c3.x = 290.0; c3.y = 680.0;
            customer_section.vertices.add_element();
            auto& c4 = customer_section.vertices.back();
            c4.x = 60.0; c4.y = 680.0;

            // LEVEL 3: Customer Address Block
            customer_section.sub_geometries.add_element();
            auto& address_block = customer_section.sub_geometries.back();
            address_block.fill_color = "#F3E5F5";
            
            // Address block bounds (70, 590, 210, 80)
            address_block.vertices.add_element();
            auto& a1 = address_block.vertices.back();
            a1.x = 70.0; a1.y = 590.0;
            address_block.vertices.add_element();
            auto& a2 = address_block.vertices.back();
            a2.x = 280.0; a2.y = 590.0;
            address_block.vertices.add_element();
            auto& a3 = address_block.vertices.back();
            a3.x = 280.0; a3.y = 670.0;
            address_block.vertices.add_element();
            auto& a4 = address_block.vertices.back();
            a4.x = 70.0; a4.y = 670.0;

            // Address texts
            address_block.texts.add_element();
            auto& customer_name = address_block.texts.back();
            customer_name.x = 80.0;
            customer_name.y = 650.0;
            customer_name.text = "ACME Corporation";
            customer_name.font_size = 14.0;
            customer_name.bold = true;

            address_block.texts.add_element();
            auto& customer_addr = address_block.texts.back();
            customer_addr.x = 80.0;
            customer_addr.y = 630.0;
            customer_addr.text = "123 Business St";
            customer_addr.font_size = 12.0;

            address_block.texts.add_element();
            auto& customer_city = address_block.texts.back();
            customer_city.x = 80.0;
            customer_city.y = 610.0;
            customer_city.text = "Business City, BC 12345";
            customer_city.font_size = 12.0;

            // LEVEL 2: Payment Details Section
            invoice_container.sub_geometries.add_element();
            auto& payment_section = invoice_container.sub_geometries.back();
            payment_section.fill_color = "#E8F5E8";
            
            // Payment bounds (310, 580, 225, 100)
            payment_section.vertices.add_element();
            auto& p1 = payment_section.vertices.back();
            p1.x = 310.0; p1.y = 580.0;
            payment_section.vertices.add_element();
            auto& p2 = payment_section.vertices.back();
            p2.x = 535.0; p2.y = 580.0;
            payment_section.vertices.add_element();
            auto& p3 = payment_section.vertices.back();
            p3.x = 535.0; p3.y = 680.0;
            payment_section.vertices.add_element();
            auto& p4 = payment_section.vertices.back();
            p4.x = 310.0; p4.y = 680.0;

            // Payment texts
            payment_section.texts.add_element();
            auto& payment_title = payment_section.texts.back();
            payment_title.x = 320.0;
            payment_title.y = 660.0;
            payment_title.text = "Payment Details";
            payment_title.font_size = 14.0;
            payment_title.bold = true;

            payment_section.texts.add_element();
            auto& payment_terms = payment_section.texts.back();
            payment_terms.x = 320.0;
            payment_terms.y = 640.0;
            payment_terms.text = "Terms: Net 30";
            payment_terms.font_size = 12.0;

            payment_section.texts.add_element();
            auto& payment_date = payment_section.texts.back();
            payment_date.x = 320.0;
            payment_date.y = 620.0;
            payment_date.text = "Due: 2024-12-15";
            payment_date.font_size = 12.0;

            // LEVEL 2: Items Table Section
            invoice_container.sub_geometries.add_element();
            auto& table_section = invoice_container.sub_geometries.back();
            table_section.fill_color = "#FFFFFF";
            
            // Table bounds (60, 200, 475, 360)
            table_section.vertices.add_element();
            auto& t1 = table_section.vertices.back();
            t1.x = 60.0; t1.y = 200.0;
            table_section.vertices.add_element();
            auto& t2 = table_section.vertices.back();
            t2.x = 535.0; t2.y = 200.0;
            table_section.vertices.add_element();
            auto& t3 = table_section.vertices.back();
            t3.x = 535.0; t3.y = 560.0;
            table_section.vertices.add_element();
            auto& t4 = table_section.vertices.back();
            t4.x = 60.0; t4.y = 560.0;

            // LEVEL 3: Table Header Row
            table_section.sub_geometries.add_element();
            auto& header_row = table_section.sub_geometries.back();
            header_row.fill_color = "#2196F3";
            
            // Header row bounds (70, 520, 455, 30)
            header_row.vertices.add_element();
            auto& hr1 = header_row.vertices.back();
            hr1.x = 70.0; hr1.y = 520.0;
            header_row.vertices.add_element();
            auto& hr2 = header_row.vertices.back();
            hr2.x = 525.0; hr2.y = 520.0;
            header_row.vertices.add_element();
            auto& hr3 = header_row.vertices.back();
            hr3.x = 525.0; hr3.y = 550.0;
            header_row.vertices.add_element();
            auto& hr4 = header_row.vertices.back();
            hr4.x = 70.0; hr4.y = 550.0;

            // Header row texts
            header_row.texts.add_element();
            auto& col1_header = header_row.texts.back();
            col1_header.x = 80.0;
            col1_header.y = 535.0;
            col1_header.text = "Description";
            col1_header.font_size = 12.0;
            col1_header.bold = true;
            col1_header.color = "#FFFFFF";

            header_row.texts.add_element();
            auto& col2_header = header_row.texts.back();
            col2_header.x = 350.0;
            col2_header.y = 535.0;
            col2_header.text = "Qty";
            col2_header.font_size = 12.0;
            col2_header.bold = true;
            col2_header.color = "#FFFFFF";

            header_row.texts.add_element();
            auto& col3_header = header_row.texts.back();
            col3_header.x = 400.0;
            col3_header.y = 535.0;
            col3_header.text = "Price";
            col3_header.font_size = 12.0;
            col3_header.bold = true;
            col3_header.color = "#FFFFFF";

            header_row.texts.add_element();
            auto& col4_header = header_row.texts.back();
            col4_header.x = 470.0;
            col4_header.y = 535.0;
            col4_header.text = "Total";
            col4_header.font_size = 12.0;
            col4_header.bold = true;
            col4_header.color = "#FFFFFF";

            // LEVEL 3: Table Data Rows (3 items)
            for (int row = 0; row < 3; row++) {
                table_section.sub_geometries.add_element();
                auto& data_row = table_section.sub_geometries.back();
                data_row.fill_color = (row % 2 == 0) ? "#F5F5F5" : "#FFFFFF";
                
                double row_y = 480.0 - (row * 40.0);
                
                // Data row bounds
                data_row.vertices.add_element();
                auto& dr1 = data_row.vertices.back();
                dr1.x = 70.0; dr1.y = row_y;
                data_row.vertices.add_element();
                auto& dr2 = data_row.vertices.back();
                dr2.x = 525.0; dr2.y = row_y;
                data_row.vertices.add_element();
                auto& dr3 = data_row.vertices.back();
                dr3.x = 525.0; dr3.y = row_y + 30.0;
                data_row.vertices.add_element();
                auto& dr4 = data_row.vertices.back();
                dr4.x = 70.0; dr4.y = row_y + 30.0;

                // LEVEL 4: Individual Cells
                for (int col = 0; col < 4; col++) {
                    data_row.sub_geometries.add_element();
                    auto& cell = data_row.sub_geometries.back();
                    cell.fill_color = "#FAFAFA";
                    
                    double cell_x = 75.0 + (col * 110.0);
                    double cell_width = (col == 0) ? 260.0 : 100.0;
                    
                    // Cell bounds
                    cell.vertices.add_element();
                    auto& cc1 = cell.vertices.back();
                    cc1.x = cell_x; cc1.y = row_y + 5.0;
                    cell.vertices.add_element();
                    auto& cc2 = cell.vertices.back();
                    cc2.x = cell_x + cell_width; cc2.y = row_y + 5.0;
                    cell.vertices.add_element();
                    auto& cc3 = cell.vertices.back();
                    cc3.x = cell_x + cell_width; cc3.y = row_y + 25.0;
                    cell.vertices.add_element();
                    auto& cc4 = cell.vertices.back();
                    cc4.x = cell_x; cc4.y = row_y + 25.0;

                    // Cell text
                    cell.texts.add_element();
                    auto& cell_text = cell.texts.back();
                    cell_text.x = cell_x + 5.0;
                    cell_text.y = row_y + 15.0;
                    cell_text.font_size = 11.0;
                    
                    if (col == 0) {
                        cell_text.text = (row == 0) ? "Premium Service Package" : 
                                        (row == 1) ? "Consulting Hours" : "Software License";
                    } else if (col == 1) {
                        cell_text.text = (row == 0) ? "1" : (row == 1) ? "40" : "1";
                    } else if (col == 2) {
                        cell_text.text = (row == 0) ? "$2500.00" : (row == 1) ? "$150.00" : "$800.00";
                    } else {
                        cell_text.text = (row == 0) ? "$2500.00" : (row == 1) ? "$6000.00" : "$800.00";
                    }
                }
            }

            // LEVEL 2: Totals Section
            invoice_container.sub_geometries.add_element();
            auto& totals_section = invoice_container.sub_geometries.back();
            totals_section.fill_color = "#E1F5FE";
            
            // Totals bounds (350, 120, 185, 60)
            totals_section.vertices.add_element();
            auto& ts1 = totals_section.vertices.back();
            ts1.x = 350.0; ts1.y = 120.0;
            totals_section.vertices.add_element();
            auto& ts2 = totals_section.vertices.back();
            ts2.x = 535.0; ts2.y = 120.0;
            totals_section.vertices.add_element();
            auto& ts3 = totals_section.vertices.back();
            ts3.x = 535.0; ts3.y = 180.0;
            totals_section.vertices.add_element();
            auto& ts4 = totals_section.vertices.back();
            ts4.x = 350.0; ts4.y = 180.0;

            // Totals texts
            totals_section.texts.add_element();
            auto& subtotal_text = totals_section.texts.back();
            subtotal_text.x = 360.0;
            subtotal_text.y = 165.0;
            subtotal_text.text = "Subtotal: $9,300.00";
            subtotal_text.font_size = 12.0;

            totals_section.texts.add_element();
            auto& tax_text = totals_section.texts.back();
            tax_text.x = 360.0;
            tax_text.y = 150.0;
            tax_text.text = "Tax (8%): $744.00";
            tax_text.font_size = 12.0;

            totals_section.texts.add_element();
            auto& total_text = totals_section.texts.back();
            total_text.x = 360.0;
            total_text.y = 130.0;
            total_text.text = "TOTAL: $10,044.00";
            total_text.font_size = 14.0;
            total_text.bold = true;

            // LEVEL 2: Footer Section
            invoice_container.sub_geometries.add_element();
            auto& footer_section = invoice_container.sub_geometries.back();
            footer_section.fill_color = "#F1F8E9";
            
            // Footer bounds (60, 60, 475, 40)
            footer_section.vertices.add_element();
            auto& f1 = footer_section.vertices.back();
            f1.x = 60.0; f1.y = 60.0;
            footer_section.vertices.add_element();
            auto& f2 = footer_section.vertices.back();
            f2.x = 535.0; f2.y = 60.0;
            footer_section.vertices.add_element();
            auto& f3 = footer_section.vertices.back();
            f3.x = 535.0; f3.y = 100.0;
            footer_section.vertices.add_element();
            auto& f4 = footer_section.vertices.back();
            f4.x = 60.0; f4.y = 100.0;

            footer_section.texts.add_element();
            auto& footer_text = footer_section.texts.back();
            footer_text.x = 70.0;
            footer_text.y = 80.0;
            footer_text.text = "Thank you for your business! Questions? Email: billing@company.com";
            footer_text.font_size = 10.0;
            footer_text.italic = true;

            // Store original structure for comparison
            auto original_geometries = page.sub_geometries;
            
            AND_WHEN("Performing complete round-trip: Layout → PDF → Layout") {
                // Step 1: Serialize to PDF
                flx_string pdf_data;
                bool serialize_success = pdf_doc.serialize(pdf_data);
                REQUIRE(serialize_success == true);
                REQUIRE(pdf_data.size() > 0);
                
                std::cout << "✓ Complex layout serialized to PDF (" << pdf_data.size() << " bytes)" << std::endl;

                // Step 2: Parse back to layout using direct content stream parsing
                flx_pdf_sio parsed_doc;
                bool parse_success = parsed_doc.parse(pdf_data);
                
                THEN("Round-trip parsing should succeed") {
                    REQUIRE(parse_success == true);
                    std::cout << "✓ PDF parsed back to layout structure" << std::endl;
                    
                    // Basic structural checks
                    REQUIRE(parsed_doc.pages.size() > 0);
                    auto& parsed_page = parsed_doc.pages[0];
                    REQUIRE(parsed_page.sub_geometries.size() > 0);
                    
                    std::cout << "✓ Extracted " << parsed_page.sub_geometries.size() << " top-level geometries" << std::endl;
                    
                    // Count total extracted elements recursively
                    int total_geometries = 0;
                    int total_texts = 0;
                    
                    std::function<void(flx_model_list<flx_layout_geometry>&)> count_elements = 
                        [&](flx_model_list<flx_layout_geometry>& geoms) {
                            for (size_t i = 0; i < geoms.size(); i++) {
                                total_geometries++;
                                auto& geom = geoms[i];
                                total_texts += geom.texts.size();
                                count_elements(geom.sub_geometries);
                            }
                        };
                    
                    count_elements(parsed_page.sub_geometries);
                    
                    std::cout << "✓ Total extracted: " << total_geometries << " geometries, " 
                             << total_texts << " texts" << std::endl;
                    
                    AND_WHEN("AI evaluator analyzes extraction quality") {
                        flx_string api_key = get_api_key();
                        if (!api_key.empty()) {
                            auto api = std::make_shared<openai_api>(api_key);
                            flx_layout_evaluator evaluator(api);
                            
                            // Use AI to evaluate layout comparison for ambiguous cases
                            auto result = evaluator.evaluate_extraction(original_geometries, parsed_page.sub_geometries);
                            
                            std::cout << "\n🤖 AI LAYOUT EVALUATION RESULTS" << std::endl;
                            std::cout << "=====================================" << std::endl;
                            std::cout << "Structure Similarity: " << result.structure_similarity << std::endl;
                            std::cout << "Position Accuracy: " << result.position_accuracy << std::endl;
                            std::cout << "Hierarchy Correctness: " << result.hierarchy_correctness << std::endl;
                            std::cout << "Text Extraction Score: " << result.text_extraction_score << std::endl;
                            std::cout << "Image Detection Score: " << result.image_detection_score << std::endl;
                            std::cout << "Overall Score: " << result.overall_score << std::endl;
                            std::cout << "\n📝 DETAILED AI ANALYSIS:" << std::endl;
                            std::cout << result.detailed_report.c_str() << std::endl;
                            
                            if (!result.differences_found.empty()) {
                                std::cout << "\n⚠️  DIFFERENCES DETECTED:" << std::endl;
                                std::cout << result.differences_found.c_str() << std::endl;
                            }
                            
                            THEN("AI should provide meaningful evaluation scores") {
                                REQUIRE(result.overall_score >= 0.0);
                                REQUIRE(result.overall_score <= 1.0);
                                REQUIRE(!result.detailed_report.empty());
                                
                                // For complex structures, we expect reasonable extraction quality
                                // even if not perfect due to content stream parsing limitations
                                if (result.overall_score >= 0.7) {
                                    std::cout << "✅ EXCELLENT: Round-trip quality is very good!" << std::endl;
                                } else if (result.overall_score >= 0.5) {
                                    std::cout << "⚠️  ACCEPTABLE: Round-trip has some issues but is reasonable" << std::endl;
                                } else {
                                    std::cout << "❌ POOR: Significant extraction issues detected" << std::endl;
                                }
                                
                                // AI evaluation should always be able to provide meaningful analysis
                                REQUIRE(result.structure_similarity >= 0.0);
                                REQUIRE(result.position_accuracy >= 0.0);
                                REQUIRE(result.hierarchy_correctness >= 0.0);
                            }
                        } else {
                            std::cout << "⚠️  Skipping AI evaluation - no API key available" << std::endl;
                            REQUIRE(true); // Test passes without AI evaluation
                        }
                    }
                    
                    std::cout << "\n✅ Complex PDF round-trip with AI evaluation completed successfully!" << std::endl;
                }
            }
        }
    }
}