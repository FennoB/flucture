#ifndef FLX_QR_STYLE_H
#define FLX_QR_STYLE_H

#include "../../utils/flx_model.h"
#include "../../utils/flx_string.h"
#include <vector>

/**
 * Color representation (RGB with optional alpha)
 */
class flx_qr_color : public flx_model {
public:
    flxp_double(r);  // Red 0.0-1.0
    flxp_double(g);  // Green 0.0-1.0
    flxp_double(b);  // Blue 0.0-1.0
    flxp_double(a);  // Alpha 0.0-1.0 (1.0 = opaque)

    // Convenience constructors via init method
    void init_rgb(double red, double green, double blue) {
        r = red;
        g = green;
        b = blue;
        a = 1.0;
    }

    void init_rgba(double red, double green, double blue, double alpha) {
        r = red;
        g = green;
        b = blue;
        a = alpha;
    }

    // Predefined colors
    static flx_qr_color black() {
        flx_qr_color c;
        c.init_rgb(0.0, 0.0, 0.0);
        return c;
    }

    static flx_qr_color white() {
        flx_qr_color c;
        c.init_rgb(1.0, 1.0, 1.0);
        return c;
    }

    // Convert to/from hex string (#RRGGBB or #RRGGBBAA)
    flx_string to_hex() const;
    static flx_qr_color from_hex(const flx_string& hex);
};

/**
 * Gradient definition (linear or radial)
 */
class flx_qr_gradient : public flx_model {
public:
    flxp_string(type);  // "linear" or "radial"
    flxp_model_list(colors, flx_qr_color);  // Color stops
    flxp_double(angle);  // For linear gradient (degrees)
    flxp_double(center_x);  // For radial gradient (0.0-1.0)
    flxp_double(center_y);  // For radial gradient (0.0-1.0)
};

/**
 * Logo configuration
 */
class flx_qr_logo : public flx_model {
public:
    flxp_string(image_path);       // Path to logo image
    flxp_string(image_base64);     // Or base64 encoded image data
    flxp_double(scale);            // Size relative to QR code (0.0-1.0)
    flxp_double(pos_x);            // Position X (0.0-1.0, 0.5=center)
    flxp_double(pos_y);            // Position Y (0.0-1.0, 0.5=center)
    flxp_double(corner_radius);    // Rounded corners (0.0-1.0)
    flxp_double(padding);          // Padding around logo (modules)
    flxp_bool(clear_under_logo);   // Remove QR modules under logo
};

/**
 * Module shape style
 */
class flx_qr_module_style : public flx_model {
public:
    flxp_string(shape);         // "square", "circle", "rounded", "diamond", "dots"
    flxp_double(corner_radius); // For rounded squares (0.0-1.0)
    flxp_double(size_factor);   // Module size relative to cell (0.0-1.0)
    flxp_bool(use_sdf);         // Use signed distance field for smooth edges
    flxp_double(sdf_threshold); // SDF smoothness threshold
};

/**
 * Finder pattern (corner markers) style
 */
class flx_qr_finder_style : public flx_model {
public:
    flxp_model(outer_color, flx_qr_color);
    flxp_model(inner_color, flx_qr_color);
    flxp_model(gradient, flx_qr_gradient);  // Optional gradient override
    flxp_string(shape);         // "square", "circle", "rounded", "custom"
    flxp_double(corner_radius); // For rounded style
    flxp_bool(use_image);       // Use custom image for finders
    flxp_string(image_path);    // Custom finder pattern image
};

/**
 * Complete QR code styling configuration
 */
class flx_qr_style : public flx_model {
public:
    // Module colors/gradients
    flxp_model(foreground_color, flx_qr_color);
    flxp_model(background_color, flx_qr_color);
    flxp_model(foreground_gradient, flx_qr_gradient);
    flxp_model(background_gradient, flx_qr_gradient);

    // Module appearance
    flxp_model(module_style, flx_qr_module_style);

    // Finder patterns (corner markers)
    flxp_model(finder_style, flx_qr_finder_style);

    // Logos (supports multiple logos)
    flxp_model_list(logos, flx_qr_logo);

    // Margins and sizing
    flxp_double(margin);        // Quiet zone size (modules)
    flxp_double(scale);         // Overall scale factor

    // Advanced rendering
    flxp_bool(anti_alias);      // Enable anti-aliasing
    flxp_double(blur_radius);   // Optional blur effect
    flxp_bool(shadow);          // Add drop shadow
    flxp_double(shadow_offset_x);
    flxp_double(shadow_offset_y);
    flxp_double(shadow_blur);
    flxp_model(shadow_color, flx_qr_color);

    // Frame animation support
    flxp_int(frame_count);      // Number of animation frames
    flxp_double(rotation_per_frame);  // Rotation angle per frame

    // Utility methods
    static flx_qr_style default_style();
    static flx_qr_style minimal_style();
    static flx_qr_style gradient_style();
    static flx_qr_style logo_style(const flx_string& logo_path);
};

#endif // FLX_QR_STYLE_H
