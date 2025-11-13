/**
 * QR Tool - Simple command-line QR code generator
 *
 * Usage:
 *   ./qr_tool <data> [output_file] [--style=<preset>]
 *
 * Examples:
 *   ./qr_tool "https://example.com"
 *   ./qr_tool "Hello World" output.png
 *   ./qr_tool "Data" qr.png --style=gradient
 *   ./qr_tool "Text" qr.pdf --style=minimal
 */

#include "documents/qr/flx_qr_generator.h"
#include <iostream>
#include <string>
#include <filesystem>

namespace fs = std::filesystem;

void print_usage(const char* program_name) {
    std::cout << "QR Code Generator - flucture\n" << std::endl;
    std::cout << "Usage:" << std::endl;
    std::cout << "  " << program_name << " <data> [output_file] [options]\n" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --style=<preset>     Style preset: default, minimal, gradient, logo" << std::endl;
    std::cout << "  --logo=<path>        Path to logo image" << std::endl;
    std::cout << "  --ecc=<level>        Error correction: LOW, MEDIUM, QUARTILE, HIGH" << std::endl;
    std::cout << "  --size=<pixels>      Output size in pixels (default: 800)" << std::endl;
    std::cout << "  --fg=<color>         Foreground color (hex: #RRGGBB)" << std::endl;
    std::cout << "  --bg=<color>         Background color (hex: #RRGGBB)" << std::endl;
    std::cout << "  --rounded            Use rounded modules" << std::endl;
    std::cout << "  --ascii              Output ASCII art to console\n" << std::endl;
    std::cout << "Examples:" << std::endl;
    std::cout << "  " << program_name << " \"https://example.com\"" << std::endl;
    std::cout << "  " << program_name << " \"Hello\" qr.png --style=gradient --size=1000" << std::endl;
    std::cout << "  " << program_name << " \"Data\" qr.pdf --logo=logo.png --rounded" << std::endl;
    std::cout << "  " << program_name << " \"Text\" --ascii" << std::endl;
}

std::string get_option_value(const std::string& arg, const std::string& prefix) {
    if (arg.find(prefix) == 0) {
        return arg.substr(prefix.length());
    }
    return "";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    // Parse arguments
    std::string data = argv[1];
    std::string output_file = "qr_output.png";
    std::string style_preset = "default";
    std::string logo_path = "";
    std::string ecc_level = "HIGH";
    int size = 800;
    std::string fg_color = "";
    std::string bg_color = "";
    bool rounded = false;
    bool ascii_output = false;

    // Parse optional arguments
    for (int i = 2; i < argc; i++) {
        std::string arg = argv[i];

        if (arg.find("--style=") == 0) {
            style_preset = get_option_value(arg, "--style=");
        } else if (arg.find("--logo=") == 0) {
            logo_path = get_option_value(arg, "--logo=");
        } else if (arg.find("--ecc=") == 0) {
            ecc_level = get_option_value(arg, "--ecc=");
        } else if (arg.find("--size=") == 0) {
            size = std::stoi(get_option_value(arg, "--size="));
        } else if (arg.find("--fg=") == 0) {
            fg_color = get_option_value(arg, "--fg=");
        } else if (arg.find("--bg=") == 0) {
            bg_color = get_option_value(arg, "--bg=");
        } else if (arg == "--rounded") {
            rounded = true;
        } else if (arg == "--ascii") {
            ascii_output = true;
        } else if (arg[0] != '-') {
            output_file = arg;
        }
    }

    // Create style based on preset
    flx_qr_style style;
    if (style_preset == "minimal") {
        style = flx_qr_style::minimal_style();
    } else if (style_preset == "gradient") {
        style = flx_qr_style::gradient_style();
    } else if (style_preset == "logo" && !logo_path.empty()) {
        style = flx_qr_style::logo_style(logo_path.c_str());
    } else {
        style = flx_qr_style::default_style();
    }

    // Apply logo if specified separately
    if (!logo_path.empty() && style_preset != "logo") {
        flx_qr_logo logo;
        logo.image_path = logo_path.c_str();
        logo.scale = 0.2;
        logo.pos_x = 0.5;
        logo.pos_y = 0.5;
        logo.corner_radius = 0.15;
        logo.padding = 1.0;
        logo.clear_under_logo = true;
        style.logos.push_back(logo);
    }

    // Apply custom colors
    if (!fg_color.empty()) {
        style.foreground_color = flx_qr_color::from_hex(fg_color.c_str());
    }
    if (!bg_color.empty()) {
        style.background_color = flx_qr_color::from_hex(bg_color.c_str());
    }

    // Apply rounded style
    if (rounded) {
        style.module_style.shape = "rounded";
        style.module_style.corner_radius = 0.3;
        style.module_style.use_sdf = true;
    }

    // Setup parameters
    auto params = flx_qr_params::defaults();
    params.error_correction = ecc_level.c_str();

    // Generate QR code
    std::cout << "Generating QR code..." << std::endl;
    std::cout << "  Data: " << data << std::endl;
    std::cout << "  Style: " << style_preset << std::endl;
    std::cout << "  Error Correction: " << ecc_level << std::endl;

    flx_qr_generator qr;
    if (!qr.generate(data.c_str(), style, params)) {
        std::cerr << "ERROR: Failed to generate QR code" << std::endl;
        return 1;
    }

    std::cout << "  QR Size: " << qr.get_size() << "x" << qr.get_size() << " modules" << std::endl;

    // Output ASCII if requested
    if (ascii_output) {
        std::cout << "\nASCII QR Code:\n" << std::endl;
        std::cout << qr.to_ascii_art().c_str() << std::endl;
    }

    // Render to file
    std::string ext = fs::path(output_file).extension().string();
    bool success = false;

    if (ext == ".png" || ext == ".PNG" || ext == ".jpg" || ext == ".JPG" || ext == ".jpeg" || ext == ".JPEG") {
        std::cout << "  Output: " << output_file << " (" << size << "x" << size << " px)" << std::endl;
        success = qr.render_to_image(output_file.c_str(), size);
    } else if (ext == ".svg" || ext == ".SVG") {
        std::cout << "  Output: " << output_file << " (SVG)" << std::endl;
        success = qr.render_to_svg(output_file.c_str(), static_cast<double>(size));
    } else {
        std::cerr << "ERROR: Unsupported file format: " << ext << std::endl;
        std::cerr << "Supported: .png, .jpg, .jpeg, .svg" << std::endl;
        return 1;
    }

    if (success) {
        std::cout << "\n✓ QR code generated successfully!" << std::endl;
        std::cout << "  File: " << fs::absolute(output_file) << std::endl;
        return 0;
    } else {
        std::cerr << "\nERROR: Failed to render QR code to file" << std::endl;
        return 1;
    }
}
