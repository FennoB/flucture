#ifndef FLX_QR_GENERATOR_H
#define FLX_QR_GENERATOR_H

#include "flx_qr_style.h"
#include "../../utils/flx_string.h"
#include <vector>
#include <memory>

// Forward declarations
namespace qrcodegen {
    class QrCode;
}

namespace cv {
    class Mat;
}

/**
 * QR Code generation parameters
 */
class flx_qr_params : public flx_model {
public:
    flxp_string(data);           // Data to encode
    flxp_string(error_correction); // "LOW", "MEDIUM", "QUARTILE", "HIGH"
    flxp_int(min_version);       // Minimum QR version (1-40)
    flxp_int(max_version);       // Maximum QR version (1-40)
    flxp_int(mask);              // Mask pattern (-1 for auto)
    flxp_bool(boost_ecc);        // Boost error correction if possible

    static flx_qr_params defaults() {
        flx_qr_params params;
        params.error_correction = "MEDIUM";
        params.min_version = 1;
        params.max_version = 40;
        params.mask = -1;
        params.boost_ecc = true;
        return params;
    }
};

/**
 * Advanced QR Code Generator
 *
 * Features:
 * - Multiple output formats (PNG, PDF, SVG)
 * - Extensive styling options (gradients, logos, shapes)
 * - SDF-based smooth rendering
 * - Animation frame generation
 * - OpenCV and PoDoFo integration
 */
class flx_qr_generator {
private:
    std::shared_ptr<qrcodegen::QrCode> qr_code;
    flx_qr_style style;
    flx_qr_params params;

    // Internal rendering helpers
    void render_module_to_mat(cv::Mat& image, int mx, int my, int module_size, bool is_dark);
    void render_finder_pattern(cv::Mat& image, int center_x, int center_y, int module_size);
    void apply_gradient(cv::Mat& image, const flx_qr_gradient& gradient);
    void apply_logo(cv::Mat& image, const flx_qr_logo& logo);
    void apply_effects(cv::Mat& image);

    // SDF (Signed Distance Field) rendering
    float sdf_rect(float x, float y, float ex, float ey, float r) const;
    float sdf_circle(float x, float y, float radius) const;
    bool should_render_pixel(int px, int py, int mx, int my, int module_size, bool is_dark);

public:
    flx_qr_generator() = default;

    /**
     * Generate QR code from data
     * @param data Data to encode
     * @param qr_style Styling configuration
     * @param qr_params Generation parameters
     * @return true if generation succeeded
     */
    bool generate(const flx_string& data,
                  flx_qr_style qr_style = flx_qr_style::default_style(),
                  flx_qr_params qr_params = flx_qr_params::defaults());

    /**
     * Render QR code to PNG/JPEG image
     * @param output_path Path to save image file (.png or .jpg)
     * @param width Output width in pixels (maintains aspect ratio)
     * @return true if rendering succeeded
     */
    bool render_to_image(const flx_string& output_path, int width = 800);

    /**
     * Render QR code to SVG
     * @param output_path Path to save SVG file
     * @param size Size in SVG units
     * @return true if rendering succeeded
     */
    bool render_to_svg(const flx_string& output_path, double size = 500.0);

    /**
     * Render animation frames
     * @param output_pattern Pattern for frame filenames (e.g., "frame_%03d.png")
     * @param frame_count Number of frames
     * @param width Frame width in pixels
     * @return Number of frames rendered
     */
    int render_animation(const flx_string& output_pattern, int frame_count, int width = 500);

    /**
     * Get QR code size (modules)
     */
    int get_size() const;

    /**
     * Get module state at position
     */
    bool get_module(int x, int y) const;

    /**
     * Get raw QR code data as string (ASCII art)
     */
    flx_string to_ascii_art(const flx_string& dark = "██", const flx_string& light = "  ") const;

    /**
     * Get current style
     */
    flx_qr_style& get_style() { return style; }

    /**
     * Update style
     */
    void set_style(flx_qr_style& new_style) { style = new_style; }
};

#endif // FLX_QR_GENERATOR_H
