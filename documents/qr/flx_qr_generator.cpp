#include "flx_qr_generator.h"
#include "qrcodegen.hpp"
#include <opencv2/opencv.hpp>
#include <cmath>
#include <fstream>
#include <iostream>

using namespace qrcodegen;
using namespace cv;

// ============================================================================
// Core Generation
// ============================================================================

bool flx_qr_generator::generate(const flx_string& data,
                                  flx_qr_style qr_style,
                                  flx_qr_params qr_params) {
    try {
        // Store parameters
        style = qr_style;
        params = qr_params;
        params.data = data;

        // Parse error correction level
        QrCode::Ecc ecc = QrCode::Ecc::MEDIUM;
        if (params.error_correction.value() == "LOW") {
            ecc = QrCode::Ecc::LOW;
        } else if (params.error_correction.value() == "QUARTILE") {
            ecc = QrCode::Ecc::QUARTILE;
        } else if (params.error_correction.value() == "HIGH") {
            ecc = QrCode::Ecc::HIGH;
        }

        // Generate QR code
        auto qr = QrCode::encodeText(data.c_str(), ecc);
        qr_code = std::make_shared<QrCode>(qr);

        return true;
    } catch (const std::exception& e) {
        std::cerr << "QR generation failed: " << e.what() << std::endl;
        return false;
    }
}

int flx_qr_generator::get_size() const {
    if (!qr_code) return 0;
    return qr_code->getSize();
}

bool flx_qr_generator::get_module(int x, int y) const {
    if (!qr_code) return false;
    return qr_code->getModule(x, y);
}

// ============================================================================
// SDF (Signed Distance Field) Functions
// ============================================================================

float flx_qr_generator::sdf_rect(float x, float y, float ex, float ey, float r) const {
    float dx = std::abs(x) + r - ex;
    float dy = std::abs(y) + r - ey;
    dx = dx < 0 ? 0 : dx;
    dy = dy < 0 ? 0 : dy;
    return std::sqrt(dx * dx + dy * dy) - r;
}

float flx_qr_generator::sdf_circle(float x, float y, float radius) const {
    return std::sqrt(x * x + y * y) - radius;
}

bool flx_qr_generator::should_render_pixel(int px, int py, int mx, int my,
                                             int module_size, bool is_dark) {
    if (!is_dark) return false;

    // Center of module
    float center_x = mx * module_size + module_size / 2.0f;
    float center_y = my * module_size + module_size / 2.0f;

    // Relative position
    float rx = px - center_x;
    float ry = py - center_y;

    // Get style
    float size_factor = style.module_style.size_factor.is_null() ? 1.0 : static_cast<double>(style.module_style.size_factor);
    float half_size = (module_size / 2.0f) * size_factor;

    // Shape-based rendering
    flx_string shape = style.module_style.shape.is_null() ? "square" : static_cast<flx_string>(style.module_style.shape);

    if (shape == "circle") {
        float dist = sdf_circle(rx, ry, half_size);
        return dist <= 0;
    } else if (shape == "rounded") {
        float corner_r = style.module_style.corner_radius.is_null() ? 0.3 : static_cast<double>(style.module_style.corner_radius);
        float radius = half_size * corner_r;
        float dist = sdf_rect(rx, ry, half_size, half_size, radius);
        return dist <= 0;
    } else {
        // Square (default)
        return std::abs(rx) <= half_size && std::abs(ry) <= half_size;
    }
}

// ============================================================================
// Image Rendering with OpenCV (PNG/JPEG)
// ============================================================================

bool flx_qr_generator::render_to_image(const flx_string& output_path, int width) {
    if (!qr_code) {
        std::cerr << "No QR code generated" << std::endl;
        return false;
    }

    try {
        int qr_size = qr_code->getSize();
        int margin = static_cast<int>(style.margin.is_null() ? 4.0 : style.margin.value());
        int total_modules = qr_size + 2 * margin;
        int module_size = width / total_modules;
        int img_size = module_size * total_modules;

        // Create image
        Mat image(img_size, img_size, CV_8UC3);

        // Background color
        Scalar bg_cv(static_cast<double>(style.background_color.b) * 255,
                     static_cast<double>(style.background_color.g) * 255,
                     static_cast<double>(style.background_color.r) * 255);
        image.setTo(bg_cv);

        // Foreground color
        Scalar fg_cv(static_cast<double>(style.foreground_color.b) * 255,
                     static_cast<double>(style.foreground_color.g) * 255,
                     static_cast<double>(style.foreground_color.r) * 255);

        // Render modules
        for (int y = 0; y < qr_size; y++) {
            for (int x = 0; x < qr_size; x++) {
                if (qr_code->getModule(x, y)) {
                    // Calculate pixel position
                    int px = (x + margin) * module_size;
                    int py = (y + margin) * module_size;

                    // Render module based on style
                    render_module_to_mat(image, x, y, module_size, true);
                }
            }
        }

        // Apply logos
        for (size_t i = 0; i < style.logos.size(); i++) {
            apply_logo(image, style.logos[i]);
        }

        // Apply effects
        apply_effects(image);

        // Save image
        return imwrite(output_path.c_str(), image);
    } catch (const std::exception& e) {
        std::cerr << "PNG rendering failed: " << e.what() << std::endl;
        return false;
    }
}

void flx_qr_generator::render_module_to_mat(Mat& image, int mx, int my,
                                              int module_size, bool is_dark) {
    if (!is_dark) return;

    int qr_size = qr_code->getSize();
    int margin = static_cast<int>(style.margin.is_null() ? 4.0 : style.margin.value());

    // Get foreground color
    Scalar fg_cv(static_cast<double>(style.foreground_color.b) * 255,
                 static_cast<double>(style.foreground_color.g) * 255,
                 static_cast<double>(style.foreground_color.r) * 255);

    // Calculate pixel position
    int base_x = (mx + margin) * module_size;
    int base_y = (my + margin) * module_size;

    // Render based on shape
    bool use_sdf = !style.module_style.use_sdf.is_null() && static_cast<bool>(style.module_style.use_sdf);

    if (use_sdf) {
        // SDF-based rendering with anti-aliasing
        for (int dy = 0; dy < module_size; dy++) {
            for (int dx = 0; dx < module_size; dx++) {
                int px = base_x + dx;
                int py = base_y + dy;

                if (should_render_pixel(px, py, mx, my, module_size, true)) {
                    image.at<Vec3b>(py, px) = Vec3b(fg_cv[0], fg_cv[1], fg_cv[2]);
                }
            }
        }
    } else {
        // Fast rectangle rendering
        rectangle(image, Point(base_x, base_y),
                  Point(base_x + module_size, base_y + module_size),
                  fg_cv, FILLED);
    }
}

void flx_qr_generator::apply_logo(Mat& image, const flx_qr_logo& logo) {
    try {
        // Load logo image
        Mat logo_img;
        if (!logo.image_path.is_null() && !logo.image_path.value().empty()) {
            logo_img = imread(logo.image_path.value().c_str(), IMREAD_UNCHANGED);
        }

        if (logo_img.empty()) {
            return;
        }

        // Calculate logo size and position
        double scale = logo.scale.is_null() ? 0.2 : logo.scale.value();
        double pos_x = logo.pos_x.is_null() ? 0.5 : logo.pos_x.value();
        double pos_y = logo.pos_y.is_null() ? 0.5 : logo.pos_y.value();

        int logo_width = static_cast<int>(image.cols * scale);
        int logo_height = static_cast<int>(logo_width * logo_img.rows / (double)logo_img.cols);

        // Resize logo
        Mat logo_resized;
        resize(logo_img, logo_resized, Size(logo_width, logo_height), 0, 0, INTER_AREA);

        // Calculate position
        int x = static_cast<int>(image.cols * pos_x - logo_width / 2);
        int y = static_cast<int>(image.rows * pos_y - logo_height / 2);

        // Ensure logo is within bounds
        x = std::max(0, std::min(x, image.cols - logo_width));
        y = std::max(0, std::min(y, image.rows - logo_height));

        // Blend logo onto image (handle alpha channel)
        if (logo_resized.channels() == 4) {
            for (int ly = 0; ly < logo_height; ly++) {
                for (int lx = 0; lx < logo_width; lx++) {
                    Vec4b logo_pixel = logo_resized.at<Vec4b>(ly, lx);
                    float alpha = logo_pixel[3] / 255.0f;

                    if (alpha > 0) {
                        Vec3b& img_pixel = image.at<Vec3b>(y + ly, x + lx);
                        img_pixel[0] = static_cast<uchar>(logo_pixel[0] * alpha + img_pixel[0] * (1 - alpha));
                        img_pixel[1] = static_cast<uchar>(logo_pixel[1] * alpha + img_pixel[1] * (1 - alpha));
                        img_pixel[2] = static_cast<uchar>(logo_pixel[2] * alpha + img_pixel[2] * (1 - alpha));
                    }
                }
            }
        } else {
            // No alpha channel, direct copy
            logo_resized.copyTo(image(Rect(x, y, logo_width, logo_height)));
        }
    } catch (const std::exception& e) {
        std::cerr << "Logo application failed: " << e.what() << std::endl;
    }
}

void flx_qr_generator::apply_effects(Mat& image) {
    // Apply blur if requested
    if (!style.blur_radius.is_null() && style.blur_radius.value() > 0) {
        int blur_size = static_cast<int>(style.blur_radius.value());
        blur_size = (blur_size % 2 == 0) ? blur_size + 1 : blur_size; // Must be odd
        GaussianBlur(image, image, Size(blur_size, blur_size), 0);
    }
}

// ============================================================================
// SVG Rendering
// ============================================================================

bool flx_qr_generator::render_to_svg(const flx_string& output_path, double size) {
    if (!qr_code) {
        std::cerr << "No QR code generated" << std::endl;
        return false;
    }

    try {
        std::ofstream svg_file(output_path.c_str());
        if (!svg_file.is_open()) {
            return false;
        }

        int qr_size = qr_code->getSize();
        int margin = static_cast<int>(style.margin.is_null() ? 4.0 : style.margin.value());
        int total_modules = qr_size + 2 * margin;
        double module_size = size / total_modules;

        // SVG header
        svg_file << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n";
        svg_file << "<svg xmlns=\"http://www.w3.org/2000/svg\" ";
        svg_file << "version=\"1.1\" viewBox=\"0 0 " << size << " " << size << "\" ";
        svg_file << "stroke=\"none\">\n";

        // Background
        svg_file << "<rect width=\"100%\" height=\"100%\" fill=\""
                 << style.background_color.to_hex().c_str() << "\"/>\n";

        // Foreground modules
        svg_file << "<path d=\"";

        for (int y = 0; y < qr_size; y++) {
            for (int x = 0; x < qr_size; x++) {
                if (qr_code->getModule(x, y)) {
                    double px = (x + margin) * module_size;
                    double py = (y + margin) * module_size;
                    svg_file << "M" << px << "," << py
                             << "h" << module_size
                             << "v" << module_size
                             << "h-" << module_size << "z ";
                }
            }
        }

        svg_file << "\" fill=\"" << style.foreground_color.to_hex().c_str() << "\"/>\n";
        svg_file << "</svg>\n";

        svg_file.close();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "SVG rendering failed: " << e.what() << std::endl;
        return false;
    }
}

// ============================================================================
// ASCII Art
// ============================================================================

flx_string flx_qr_generator::to_ascii_art(const flx_string& dark, const flx_string& light) const {
    if (!qr_code) return "";

    std::string result;
    int qr_size = qr_code->getSize();
    int margin = static_cast<int>(style.margin.is_null() ? 4.0 : style.margin.value());

    // Top margin
    for (int i = 0; i < margin; i++) {
        for (int j = 0; j < qr_size + 2 * margin; j++) {
            result += light.c_str();
        }
        result += "\n";
    }

    // QR code with side margins
    for (int y = 0; y < qr_size; y++) {
        // Left margin
        for (int i = 0; i < margin; i++) {
            result += light.c_str();
        }

        // QR modules
        for (int x = 0; x < qr_size; x++) {
            result += qr_code->getModule(x, y) ? dark.c_str() : light.c_str();
        }

        // Right margin
        for (int i = 0; i < margin; i++) {
            result += light.c_str();
        }
        result += "\n";
    }

    // Bottom margin
    for (int i = 0; i < margin; i++) {
        for (int j = 0; j < qr_size + 2 * margin; j++) {
            result += light.c_str();
        }
        result += "\n";
    }

    return flx_string(result);
}

// ============================================================================
// Animation
// ============================================================================

int flx_qr_generator::render_animation(const flx_string& output_pattern,
                                         int frame_count, int width) {
    if (!qr_code) return 0;

    int frames_rendered = 0;
    for (int frame = 0; frame < frame_count; frame++) {
        // Generate filename
        char filename[256];
        snprintf(filename, sizeof(filename), output_pattern.c_str(), frame);

        // TODO: Apply rotation/animation transformations based on style.rotation_per_frame
        // For now, just render the same image
        if (render_to_image(filename, width)) {
            frames_rendered++;
        }
    }

    return frames_rendered;
}

void flx_qr_generator::render_finder_pattern(Mat& image, int center_x, int center_y,
                                               int module_size) {
    // TODO: Implement custom finder pattern rendering
}

void flx_qr_generator::apply_gradient(Mat& image, const flx_qr_gradient& gradient) {
    // TODO: Implement gradient application
}
