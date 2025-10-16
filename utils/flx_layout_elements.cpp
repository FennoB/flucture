#include "flx_layout_elements.h"
#include <cmath>

// Helper functions for layout elements

namespace flx_layout {

/**
 * Check if a point is inside a geometry element
 */
bool point_in_geometry(const flx_geometry_element& geometry, double x, double y) {
    // TODO: Implement point-in-polygon test
    // For now, simple bounding box check would work for rectangles
    return false;
}

/**
 * Calculate bounding box of vertices
 */
struct BoundingBox {
    double min_x, min_y, max_x, max_y;
    double width() const { return max_x - min_x; }
    double height() const { return max_y - min_y; }
};

BoundingBox calculate_bounding_box(const flxv_vector& vertices) {
    BoundingBox bbox = {INFINITY, INFINITY, -INFINITY, -INFINITY};
    
    for (size_t i = 0; i < vertices.size(); i += 2) {
        if (i + 1 < vertices.size()) {
            double x = vertices[i].convert(flx_variant::double_state).to_double();
            double y = vertices[i + 1].convert(flx_variant::double_state).to_double();
            
            bbox.min_x = std::min(bbox.min_x, x);
            bbox.max_x = std::max(bbox.max_x, x);
            bbox.min_y = std::min(bbox.min_y, y);
            bbox.max_y = std::max(bbox.max_y, y);
        }
    }
    
    return bbox;
}

/**
 * Create a rectangular geometry
 */
flx_geometry_element create_rectangle(double x, double y, double width, double height) {
    flx_geometry_element rect;
    
    // Create rectangular vertices: top-left, top-right, bottom-right, bottom-left
    flxv_vector vertices;
    vertices.push_back(flx_variant(x));              // top-left x
    vertices.push_back(flx_variant(y));              // top-left y
    vertices.push_back(flx_variant(x + width));      // top-right x
    vertices.push_back(flx_variant(y));              // top-right y
    vertices.push_back(flx_variant(x + width));      // bottom-right x
    vertices.push_back(flx_variant(y + height));     // bottom-right y
    vertices.push_back(flx_variant(x));              // bottom-left x
    vertices.push_back(flx_variant(y + height));     // bottom-left y
    
    rect.vertices = vertices;
    rect.fill_color = "#FFFFFF";      // Default white fill
    rect.stroke_color = "#000000";    // Default black border
    rect.stroke_width = 1.0;          // Default 1pt border
    
    return rect;
}

/**
 * Create a simple text element
 */
flx_text_element create_text(const std::string& content, double x, double y, double font_size) {
    flx_text_element text;
    
    text.content = content;
    text.x = x;
    text.y = y;
    text.font_size = font_size;
    text.font_family = "Arial";       // Default font
    text.color_foreground = "#000000"; // Default black text
    text.horizontal_alignment = -1;    // Left aligned
    text.vertical_alignment = -1;      // Top aligned
    text.wrap = false;                 // No wrapping by default
    
    // Estimate text dimensions (rough approximation)
    text.width = content.length() * font_size * 0.6;  // Rough character width
    text.height = font_size * 1.2;                    // Rough line height
    
    return text;
}

} // namespace flx_layout