#include <catch2/catch_all.hpp>
#include "../documents/layout/flx_layout_geometry.h"

SCENARIO("flx_layout_bounds provides basic geometry operations") {
    GIVEN("A layout bounds with position and size") {
        flx_layout_bounds bounds(10, 20, 100, 50);
        
        WHEN("Checking basic properties") {
            THEN("Properties should be set correctly") {
                REQUIRE(bounds.x == 10.0);
                REQUIRE(bounds.y == 20.0);
                REQUIRE(bounds.width == 100.0);
                REQUIRE(bounds.height == 50.0);
            }
        }
        
        WHEN("Calculating edge positions") {
            THEN("Edge calculations should be correct") {
                REQUIRE(bounds.get_left() == 10);
                REQUIRE(bounds.get_right() == 110);
                REQUIRE(bounds.get_top() == 20);
                REQUIRE(bounds.get_bottom() == 70);
                REQUIRE(bounds.get_center_x() == 60);
                REQUIRE(bounds.get_center_y() == 45);
            }
        }
        
        WHEN("Testing point containment") {
            THEN("Should correctly identify contained points") {
                REQUIRE(bounds.contains_point(50, 40) == true);
                REQUIRE(bounds.contains_point(10, 20) == true);  // Edge case
                REQUIRE(bounds.contains_point(5, 40) == false);
                REQUIRE(bounds.contains_point(115, 40) == false);
            }
        }
        
        WHEN("Testing bounds containment") {
            flx_layout_bounds inner(20, 30, 50, 20);
            flx_layout_bounds outside(200, 200, 10, 10);
            
            THEN("Should correctly identify contained bounds") {
                REQUIRE(bounds.contains_bounds(inner) == true);
                REQUIRE(bounds.contains_bounds(outside) == false);
            }
        }
        
        WHEN("Testing bounds intersection") {
            flx_layout_bounds overlapping(80, 40, 50, 20);
            flx_layout_bounds separate(200, 200, 10, 10);
            
            THEN("Should correctly identify intersections") {
                REQUIRE(bounds.intersects_bounds(overlapping) == true);
                REQUIRE(bounds.intersects_bounds(separate) == false);
            }
        }
    }
}

SCENARIO("flx_layout_text handles text properties") {
    GIVEN("A text element with content") {
        flx_layout_text text(10, 20, 100, 30, "Hello World");
        
        WHEN("Checking initial values") {
            THEN("Position and text should be set") {
                REQUIRE(text.x == 10.0);
                REQUIRE(text.y == 20.0);
                REQUIRE(text.width == 100.0);
                REQUIRE(text.height == 30.0);
                REQUIRE(text.text == "Hello World");
            }
        }
        
        WHEN("Setting font properties") {
            text.font_size = 12.0;
            text.bold = true;
            text.italic = false;
            text.font_family = "Arial";
            text.color = "#000000";
            
            THEN("Font properties should be stored") {
                REQUIRE(text.font_size == 12.0);
                REQUIRE(text.bold == true);
                REQUIRE(text.italic == false);
                REQUIRE(text.font_family == "Arial");
                REQUIRE(text.color == "#000000");
            }
        }
    }
}

SCENARIO("flx_layout_image handles image metadata") {
    GIVEN("An image element") {
        flx_layout_image image(50, 60, 200, 150);
        
        WHEN("Setting image properties") {
            image.image_path = "/path/to/image.jpg";
            image.description = "Test image";
            image.original_width = 800;
            image.original_height = 600;
            image.mime_type = "image/jpeg";
            
            THEN("Image metadata should be stored") {
                REQUIRE(image.x == 50.0);
                REQUIRE(image.y == 60.0);
                REQUIRE(image.image_path == "/path/to/image.jpg");
                REQUIRE(image.description == "Test image");
                REQUIRE(image.original_width == 800);
                REQUIRE(image.original_height == 600);
                REQUIRE(image.mime_type == "image/jpeg");
            }
        }
    }
}

SCENARIO("flx_layout_geometry manages hierarchical content") {
    GIVEN("A geometry container") {
        flx_layout_geometry container(0, 0, 300, 200);
        
        WHEN("Adding text elements") {
            flx_layout_text text1(10, 10, 100, 20, "Title");
            flx_layout_text text2(10, 40, 150, 20, "Subtitle");
            
            container.add_text(text1);
            container.add_text(text2);
            
            THEN("Text elements should be stored") {
                REQUIRE(container.texts.size() == 2);
                REQUIRE(container.texts[0].text == "Title");
                REQUIRE(container.texts[1].text == "Subtitle");
            }
        }
        
        WHEN("Adding image elements") {
            flx_layout_image image(10, 70, 150, 100);
            image.description = "Test image";
            
            container.add_image(image);
            
            THEN("Image should be stored") {
                REQUIRE(container.images.size() == 1);
                REQUIRE(container.images[0].description == "Test image");
            }
        }
        
        WHEN("Adding sub-geometries") {
            flx_layout_geometry sub_container(200, 50, 80, 120);
            sub_container.x = 200;  // Verify property access
            
            container.add_sub_geometry(sub_container);
            
            THEN("Sub-geometry should be stored") {
                REQUIRE(container.sub_geometries.size() == 1);
                REQUIRE(container.sub_geometries[0].x == 200.0);
            }
        }
        
        WHEN("Building complex hierarchy") {
            flx_layout_text title(10, 10, 100, 20, "Document Title");
            flx_layout_image logo(200, 10, 50, 20);
            
            flx_layout_geometry section(10, 50, 280, 140);
            flx_layout_text section_text(10, 10, 200, 15, "Section content");
            section.add_text(section_text);
            
            container.add_text(title);
            container.add_image(logo);
            container.add_sub_geometry(section);
            
            THEN("Full hierarchy should be accessible") {
                REQUIRE(container.texts.size() == 1);
                REQUIRE(container.images.size() == 1);
                REQUIRE(container.sub_geometries.size() == 1);
                REQUIRE(container.sub_geometries[0].texts.size() == 1);
                REQUIRE(container.sub_geometries[0].texts[0].text == "Section content");
            }
        }
    }
}