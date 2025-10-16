#ifndef FLX_LAYOUT_ELEMENTS_H
#define FLX_LAYOUT_ELEMENTS_H

#include "flx_model.h"

// Forward declarations
class flx_text_element;
class flx_image_element;
class flx_geometry_element;

/**
 * Text element with comprehensive styling and positioning
 */
class flx_text_element : public flx_model {
public:
    // Content
    flxp_string(content);
    
    // Font styling
    flxp_string(font_family);
    flxp_double(font_size);
    flxp_bool(bold);
    flxp_bool(italic);
    flxp_bool(underline);
    
    // Text alignment and layout
    flxp_int(horizontal_alignment); // -1=left, 0=center, 1=right
    flxp_int(vertical_alignment);   // -1=top, 0=center, 1=bottom
    flxp_bool(wrap);                // Text wrapping enabled
    flxp_bool(manual_breaks);       // Manual line breaks preserved
    
    // Colors
    flxp_string(color_foreground);  // Text color (hex: "#FF0000" or rgb: "rgb(255,0,0)")
    flxp_string(color_background);  // Background color (optional)
    
    // Bounding rectangle
    flxp_double(x);
    flxp_double(y);
    flxp_double(width);
    flxp_double(height);
    
    // Advanced text properties
    flxp_double(line_height);       // Line spacing multiplier
    flxp_double(letter_spacing);    // Character spacing
    flxp_string(text_transform);    // "uppercase", "lowercase", "capitalize"
};

/**
 * Image element with format support and positioning
 */
class flx_image_element : public flx_model {
public:
    // Image data and format
    flxp_string(format);            // "png", "jpg", "jpeg", "svg", "gif"
    flxp_vector(image_data);        // Binary image data as byte array
    flxp_string(source_path);       // Optional: original file path for reference
    
    // Positioning and sizing
    flxp_double(x);
    flxp_double(y);
    flxp_double(width);
    flxp_double(height);
    
    // Image properties
    flxp_double(opacity);           // 0.0 to 1.0
    flxp_double(rotation);          // Rotation angle in degrees
    flxp_bool(maintain_aspect);     // Maintain aspect ratio when scaling
    
    // Image processing
    flxp_string(scaling_mode);      // "fit", "fill", "stretch", "crop"
};

/**
 * Geometry element - closed polygonal shapes with children
 * Supports containment hierarchy: geometries can contain texts, images, and sub-geometries
 */
class flx_geometry_element : public flx_model {
public:
    // Shape definition - closed polygon path
    flxp_vector(vertices);          // Array of {x,y} coordinate pairs: [{x1,y1}, {x2,y2}, ...]
    
    // Visual styling
    flxp_string(fill_color);        // Interior color (hex or rgb)
    flxp_string(stroke_color);      // Border color (optional)
    flxp_double(stroke_width);      // Border width in points
    flxp_string(stroke_style);      // "solid", "dashed", "dotted"
    flxp_double(opacity);           // 0.0 to 1.0
    
    // Advanced styling
    flxp_string(fill_pattern);      // "solid", "gradient", "texture" (future)
    flxp_double(corner_radius);     // Rounded corners for rectangles
    
    // Hierarchical containment - the core of our layout system!
    flxp_model_list(texts, flx_text_element);              // Text elements inside this geometry
    flxp_model_list(images, flx_image_element);            // Image elements inside this geometry  
    flxp_model_list(subgeometries, flx_geometry_element);  // Nested geometries inside this geometry
    
    // Metadata
    flxp_string(semantic_type);     // "paragraph", "table", "header", "footer", etc.
    flxp_string(description);       // Human-readable description
};

/**
 * Document root - represents a complete PDF page or document
 */
class flx_document : public flx_model {
public:
    // Page dimensions
    flxp_double(page_width);        // Page width in points (1 point = 1/72 inch)
    flxp_double(page_height);       // Page height in points
    
    // Document metadata
    flxp_string(title);
    flxp_string(author);
    flxp_string(subject);
    flxp_string(creator);
    
    // Root geometry containers
    flxp_model_list(root_geometries, flx_geometry_element);
    
    // Multi-page support (future)
    flxp_int(page_number);          // Current page number (1-based)
    flxp_int(total_pages);          // Total number of pages
};

#endif // FLX_LAYOUT_ELEMENTS_H