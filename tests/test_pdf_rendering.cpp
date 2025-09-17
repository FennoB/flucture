#include <catch2/catch_all.hpp>
#include "../documents/pdf/flx_pdf_sio.h"
#include "../documents/layout/flx_layout_vertex.h"
#include <filesystem>
#include <fstream>

SCENARIO("PDF rendering with complete layout system") {
    GIVEN("A PDF document with pages property") {
        flx_pdf_sio pdf_doc;
        
        WHEN("Adding a page with layout elements") {
            auto& page = pdf_doc.add_page();
            page.x = 0.0;
            page.y = 0.0;
            page.width = 595.0;  // A4 width in points
            page.height = 842.0; // A4 height in points
            
            THEN("Page should be added to pages list") {
                REQUIRE(pdf_doc.page_count() == 1);
                REQUIRE(pdf_doc.pages.size() == 1);
                REQUIRE(pdf_doc.pages[0].width == 595.0);
            }
        }
    }
    
    GIVEN("A page with text elements") {
        flx_pdf_sio pdf_doc;
        auto& page = pdf_doc.add_page();
        
        WHEN("Adding text with different properties") {
            flx_layout_text title;
            title.x = 50.0;
            title.y = 750.0;
            title.width = 200.0;
            title.height = 30.0;
            title.text = "Document Title";
            title.font_family = "Arial";
            title.font_size = 16.0;
            title.bold = true;
            
            flx_layout_text subtitle;
            subtitle.x = 50.0;
            subtitle.y = 720.0;
            subtitle.width = 300.0;
            subtitle.height = 20.0;
            subtitle.text = "Subtitle Text";
            subtitle.font_size = 12.0;
            subtitle.italic = true;
            
            page.add_text(title);
            page.add_text(subtitle);
            
            THEN("Text elements should be stored correctly") {
                REQUIRE(page.texts.size() == 2);
                
                flx_string title_text = page.texts[0].text;
                REQUIRE(title_text == "Document Title");
                REQUIRE(page.texts[0].font_size == 16.0);
                REQUIRE(page.texts[0].bold == true);
                
                flx_string subtitle_text = page.texts[1].text;
                REQUIRE(subtitle_text == "Subtitle Text");
                REQUIRE(page.texts[1].italic == true);
            }
        }
    }
    
    GIVEN("A page with polygon geometry") {
        flx_pdf_sio pdf_doc;
        auto& page = pdf_doc.add_page();
        
        WHEN("Creating a triangle polygon") {
            flx_layout_geometry triangle;
            triangle.fill_color = "#FF0000";
            
            // Create triangle vertices
            flx_layout_vertex v1(100.0, 100.0);
            flx_layout_vertex v2(200.0, 100.0);
            flx_layout_vertex v3(150.0, 200.0);
            
            triangle.vertices.push_back(v1);
            triangle.vertices.push_back(v2);
            triangle.vertices.push_back(v3);
            
            page.add_sub_geometry(triangle);
            
            THEN("Triangle should be stored with correct vertices") {
                REQUIRE(page.sub_geometries.size() == 1);
                
                auto& stored_triangle = page.sub_geometries[0];
                flx_string color = stored_triangle.fill_color;
                REQUIRE(color == "#FF0000");
                REQUIRE(stored_triangle.vertices.size() == 3);
                
                REQUIRE(stored_triangle.vertices[0].x == 100.0);
                REQUIRE(stored_triangle.vertices[0].y == 100.0);
                REQUIRE(stored_triangle.vertices[1].x == 200.0);
                REQUIRE(stored_triangle.vertices[2].y == 200.0);
            }
        }
        
        WHEN("Creating a complex polygon") {
            flx_layout_geometry pentagon;
            pentagon.fill_color = "#00FF00";
            
            // Pentagon vertices
            flx_layout_vertex p1(300.0, 300.0);
            flx_layout_vertex p2(350.0, 280.0);
            flx_layout_vertex p3(380.0, 320.0);
            flx_layout_vertex p4(360.0, 360.0);
            flx_layout_vertex p5(320.0, 350.0);
            
            pentagon.vertices.push_back(p1);
            pentagon.vertices.push_back(p2);
            pentagon.vertices.push_back(p3);
            pentagon.vertices.push_back(p4);
            pentagon.vertices.push_back(p5);
            
            page.add_sub_geometry(pentagon);
            
            THEN("Pentagon should have 5 vertices") {
                REQUIRE(page.sub_geometries.size() == 1);
                REQUIRE(page.sub_geometries[0].vertices.size() == 5);
                
                flx_string color = page.sub_geometries[0].fill_color;
                REQUIRE(color == "#00FF00");
            }
        }
    }
    
    GIVEN("A page with image elements") {
        flx_pdf_sio pdf_doc;
        auto& page = pdf_doc.add_page();
        
        WHEN("Adding image with metadata") {
            flx_layout_image logo;
            logo.x = 400.0;
            logo.y = 700.0;
            logo.width = 100.0;
            logo.height = 80.0;
            logo.image_path = "/tmp/test_image.png";
            logo.description = "Company Logo";
            logo.mime_type = "image/png";
            
            page.add_image(logo);
            
            THEN("Image should be stored with correct properties") {
                REQUIRE(page.images.size() == 1);
                
                auto& stored_image = page.images[0];
                REQUIRE(stored_image.x == 400.0);
                REQUIRE(stored_image.y == 700.0);
                REQUIRE(stored_image.width == 100.0);
                REQUIRE(stored_image.height == 80.0);
                
                flx_string path = stored_image.image_path;
                flx_string desc = stored_image.description;
                flx_string mime = stored_image.mime_type;
                
                REQUIRE(path == "/tmp/test_image.png");
                REQUIRE(desc == "Company Logo");
                REQUIRE(mime == "image/png");
            }
        }
    }
    
    GIVEN("A complete document layout") {
        flx_pdf_sio pdf_doc;
        
        WHEN("Creating multi-page document with mixed content") {
            // Page 1: Title page
            auto& page1 = pdf_doc.add_page();
            page1.width = 595.0;
            page1.height = 842.0;
            
            // Title
            flx_layout_text main_title;
            main_title.x = 100.0;
            main_title.y = 750.0;
            main_title.text = "Test Document";
            main_title.font_size = 24.0;
            main_title.bold = true;
            page1.add_text(main_title);
            
            // Background shape
            flx_layout_geometry background;
            background.fill_color = "#F0F0F0";
            
            flx_layout_vertex bg1(50.0, 50.0);
            flx_layout_vertex bg2(545.0, 50.0);
            flx_layout_vertex bg3(545.0, 792.0);
            flx_layout_vertex bg4(50.0, 792.0);
            
            background.vertices.push_back(bg1);
            background.vertices.push_back(bg2);
            background.vertices.push_back(bg3);
            background.vertices.push_back(bg4);
            page1.add_sub_geometry(background);
            
            // Page 2: Content page
            auto& page2 = pdf_doc.add_page();
            page2.width = 595.0;
            page2.height = 842.0;
            
            // Content text
            flx_layout_text content;
            content.x = 80.0;
            content.y = 700.0;
            content.text = "This is page 2 content";
            content.font_size = 12.0;
            page2.add_text(content);
            
            // Decorative triangle
            flx_layout_geometry decoration;
            decoration.fill_color = "#0000FF";
            
            flx_layout_vertex d1(500.0, 100.0);
            flx_layout_vertex d2(550.0, 100.0);
            flx_layout_vertex d3(525.0, 150.0);
            
            decoration.vertices.push_back(d1);
            decoration.vertices.push_back(d2);
            decoration.vertices.push_back(d3);
            page2.add_sub_geometry(decoration);
            
            THEN("Document should have complete structure") {
                REQUIRE(pdf_doc.page_count() == 2);
                
                // Check page 1
                REQUIRE(pdf_doc.pages[0].texts.size() == 1);
                REQUIRE(pdf_doc.pages[0].sub_geometries.size() == 1);
                
                flx_string title_text = pdf_doc.pages[0].texts[0].text;
                REQUIRE(title_text == "Test Document");
                REQUIRE(pdf_doc.pages[0].sub_geometries[0].vertices.size() == 4);
                
                // Check page 2
                REQUIRE(pdf_doc.pages[1].texts.size() == 1);
                REQUIRE(pdf_doc.pages[1].sub_geometries.size() == 1);
                
                flx_string content_text = pdf_doc.pages[1].texts[0].text;
                REQUIRE(content_text == "This is page 2 content");
                REQUIRE(pdf_doc.pages[1].sub_geometries[0].vertices.size() == 3);
            }
        }
    }
    
    GIVEN("Document ready for PDF serialization") {
        flx_pdf_sio pdf_doc;
        auto& page = pdf_doc.add_page();
        
        // Add simple content
        flx_layout_text simple_text;
        simple_text.x = 100.0;
        simple_text.y = 400.0;
        simple_text.text = "Serialization Test";
        simple_text.font_size = 14.0;
        page.add_text(simple_text);
        
        flx_layout_geometry simple_rect;
        simple_rect.fill_color = "#CCCCCC";
        
        flx_layout_vertex r1(200.0, 200.0);
        flx_layout_vertex r2(300.0, 200.0);
        flx_layout_vertex r3(300.0, 250.0);
        flx_layout_vertex r4(200.0, 250.0);
        
        simple_rect.vertices.push_back(r1);
        simple_rect.vertices.push_back(r2);
        simple_rect.vertices.push_back(r3);
        simple_rect.vertices.push_back(r4);
        page.add_sub_geometry(simple_rect);
        
        WHEN("Serializing to PDF data") {
            flx_string pdf_data;
            bool result = pdf_doc.serialize(pdf_data);
            
            THEN("Serialization should succeed") {
                REQUIRE(result == true);
                REQUIRE(pdf_data.size() > 0);
                
                // Check for PDF header
                REQUIRE(pdf_data.substr(0, 4) == "%PDF");
            }
        }
        
        WHEN("Writing to file") {
            std::string test_filename = "/tmp/test_output.pdf";
            bool result = pdf_doc.write(test_filename);
            
            THEN("File should be created") {
                REQUIRE(result == true);
                REQUIRE(std::filesystem::exists(test_filename));
                
                // Check file size
                auto file_size = std::filesystem::file_size(test_filename);
                REQUIRE(file_size > 1000); // PDF should be substantial
                
                // Cleanup
                std::filesystem::remove(test_filename);
            }
        }
    }
}

SCENARIO("Vertex class functionality") {
    GIVEN("Vertex creation") {
        WHEN("Creating with coordinates") {
            flx_layout_vertex vertex(150.5, 200.7);
            
            THEN("Coordinates should be stored correctly") {
                REQUIRE(vertex.x == 150.5);
                REQUIRE(vertex.y == 200.7);
            }
        }
        
        WHEN("Creating default vertex") {
            flx_layout_vertex vertex;
            vertex.x = 50.0;
            vertex.y = 75.0;
            
            THEN("Properties should work like normal variables") {
                REQUIRE(vertex.x == 50.0);
                REQUIRE(vertex.y == 75.0);
                
                vertex.x = vertex.x + 10.0;
                REQUIRE(vertex.x == 60.0);
            }
        }
    }
}

SCENARIO("Format-agnostic document base") {
    GIVEN("Base document class") {
        flx_pdf_sio pdf_doc; // Inherits from flx_doc_sio
        
        WHEN("Using base class functionality") {
            // Test that base class methods work
            REQUIRE(pdf_doc.page_count() == 0);
            
            auto& page = pdf_doc.add_page();
            REQUIRE(pdf_doc.page_count() == 1);
            
            THEN("Base class should provide format-agnostic interface") {
                // This demonstrates that pages property is in base class
                // and can be used by any format (PDF, DOCX, etc.)
                REQUIRE(pdf_doc.pages.size() == 1);
                
                // Pages contain layout structure independent of format
                page.fill_color = "#FFFFFF";
                flx_string color = page.fill_color;
                REQUIRE(color == "#FFFFFF");
            }
        }
    }
}