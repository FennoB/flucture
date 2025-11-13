/**
 * Example: QR Code Generation with flucture
 *
 * This example demonstrates the various capabilities of the flx_qr_generator:
 * - Simple QR code generation
 * - Custom styling (colors, shapes, gradients)
 * - Multiple output formats (PNG, PDF, SVG)
 * - Logo embedding
 * - ASCII art output
 */

#include "flx_qr_generator.h"
#include <iostream>

void example_1_simple_qr() {
    std::cout << "\n=== Example 1: Simple QR Code ===" << std::endl;

    flx_qr_generator qr;

    // Generate with default settings
    if (qr.generate("https://example.com")) {
        std::cout << "QR Code generated! Size: " << qr.get_size() << "x" << qr.get_size() << std::endl;

        // Output to different formats
        qr.render_to_png("example_simple.png", 500);
        qr.render_to_svg("example_simple.svg", 500.0);
        qr.render_to_pdf("example_simple.pdf");

        std::cout << "Files created: example_simple.{png,svg,pdf}" << std::endl;
    }
}

void example_2_custom_colors() {
    std::cout << "\n=== Example 2: Custom Colors ===" << std::endl;

    flx_qr_generator qr;
    auto style = flx_qr_style::default_style();

    // Deep blue foreground
    flx_qr_color blue;
    blue.init_rgb(0.1, 0.2, 0.6);
    style.foreground_color = blue;

    // Light cream background
    flx_qr_color cream;
    cream.init_rgb(0.98, 0.96, 0.92);
    style.background_color = cream;

    if (qr.generate("Custom Color QR Code", style)) {
        qr.render_to_png("example_colors.png", 500);
        std::cout << "Created: example_colors.png" << std::endl;
    }
}

void example_3_rounded_modules() {
    std::cout << "\n=== Example 3: Rounded Modules with SDF ===" << std::endl;

    flx_qr_generator qr;
    auto style = flx_qr_style::default_style();

    // Enable rounded corners with SDF rendering
    style.module_style.value().shape = "rounded";
    style.module_style.value().corner_radius = 0.4;
    style.module_style.value().use_sdf = true;
    style.module_style.value().size_factor = 0.95; // Slightly smaller modules

    // Purple theme
    flx_qr_color purple;
    purple.init_rgb(0.5, 0.2, 0.7);
    style.foreground_color = purple;

    if (qr.generate("Rounded QR Code", style)) {
        qr.render_to_png("example_rounded.png", 800);
        std::cout << "Created: example_rounded.png" << std::endl;
    }
}

void example_4_gradient() {
    std::cout << "\n=== Example 4: Gradient QR Code ===" << std::endl;

    flx_qr_generator qr;
    auto style = flx_qr_style::gradient_style(); // Uses preset gradient

    if (qr.generate("Gradient QR Code", style)) {
        qr.render_to_png("example_gradient.png", 600);
        std::cout << "Created: example_gradient.png" << std::endl;
    }
}

void example_5_with_logo() {
    std::cout << "\n=== Example 5: QR Code with Logo ===" << std::endl;

    // Note: Requires logo.png to exist
    const char* logo_path = "logo.png";

    flx_qr_generator qr;
    auto style = flx_qr_style::logo_style(logo_path);

    if (qr.generate("QR Code with Logo", style)) {
        qr.render_to_png("example_logo.png", 700);
        std::cout << "Created: example_logo.png (requires logo.png)" << std::endl;
    } else {
        std::cout << "Note: logo.png not found, skipping this example" << std::endl;
    }
}

void example_6_minimal() {
    std::cout << "\n=== Example 6: Minimal QR Code ===" << std::endl;

    flx_qr_generator qr;
    auto style = flx_qr_style::minimal_style(); // Reduced margins

    if (qr.generate("Minimal QR", style)) {
        qr.render_to_png("example_minimal.png", 400);
        std::cout << "Created: example_minimal.png" << std::endl;
    }
}

void example_7_ascii_art() {
    std::cout << "\n=== Example 7: ASCII Art ===" << std::endl;

    flx_qr_generator qr;
    auto style = flx_qr_style::minimal_style();

    if (qr.generate("ASCII", style)) {
        std::cout << "\nASCII QR Code:" << std::endl;
        std::cout << qr.to_ascii_art().c_str() << std::endl;
    }
}

void example_8_high_capacity() {
    std::cout << "\n=== Example 8: High Capacity QR Code ===" << std::endl;

    flx_qr_generator qr;
    auto params = flx_qr_params::defaults();
    params.error_correction = "LOW"; // Lower ECC allows more data

    // Encode a longer message
    flx_string long_message = "This is a longer message that demonstrates the capacity of QR codes. "
                              "They can hold significant amounts of data, including URLs, text, "
                              "contact information, and more. Version and error correction level "
                              "are automatically chosen based on data length.";

    if (qr.generate(long_message, flx_qr_style::default_style(), params)) {
        std::cout << "QR Code size: " << qr.get_size() << "x" << qr.get_size() << std::endl;
        qr.render_to_png("example_high_capacity.png", 800);
        std::cout << "Created: example_high_capacity.png" << std::endl;
    }
}

void example_9_custom_finder_patterns() {
    std::cout << "\n=== Example 9: Custom Finder Patterns ===" << std::endl;

    flx_qr_generator qr;
    auto style = flx_qr_style::default_style();

    // Customize the finder patterns (corner squares)
    flx_qr_color red;
    red.init_rgb(0.9, 0.2, 0.2);
    style.finder_style.value().outer_color = red;

    flx_qr_color dark_red;
    dark_red.init_rgb(0.6, 0.1, 0.1);
    style.finder_style.value().inner_color = dark_red;

    style.finder_style.value().shape = "rounded";
    style.finder_style.value().corner_radius = 0.3;

    if (qr.generate("Custom Finders", style)) {
        qr.render_to_png("example_custom_finders.png", 600);
        std::cout << "Created: example_custom_finders.png" << std::endl;
    }
}

void example_10_comprehensive() {
    std::cout << "\n=== Example 10: Comprehensive Styling ===" << std::endl;

    flx_qr_generator qr;
    auto style = flx_qr_style::default_style();

    // Colors
    flx_qr_color teal;
    teal.init_rgb(0.0, 0.6, 0.6);
    style.foreground_color = teal;

    // Module styling
    style.module_style.value().shape = "circle";
    style.module_style.value().size_factor = 0.9;
    style.module_style.value().use_sdf = true;

    // Margins and scale
    style.margin = 3.0;
    style.scale = 12.0;
    style.anti_alias = true;

    // Effects
    style.blur_radius = 0.5; // Subtle blur

    if (qr.generate("Comprehensive Styling Demo", style)) {
        qr.render_to_png("example_comprehensive.png", 800);
        qr.render_to_pdf("example_comprehensive.pdf");
        qr.render_to_svg("example_comprehensive.svg", 800.0);
        std::cout << "Created: example_comprehensive.{png,pdf,svg}" << std::endl;
    }
}

int main() {
    std::cout << "==================================================" << std::endl;
    std::cout << "   flucture QR Code Generator - Examples" << std::endl;
    std::cout << "==================================================" << std::endl;

    // Run all examples
    example_1_simple_qr();
    example_2_custom_colors();
    example_3_rounded_modules();
    example_4_gradient();
    example_5_with_logo();
    example_6_minimal();
    example_7_ascii_art();
    example_8_high_capacity();
    example_9_custom_finder_patterns();
    example_10_comprehensive();

    std::cout << "\n==================================================" << std::endl;
    std::cout << "All examples completed!" << std::endl;
    std::cout << "Check the generated files in the current directory." << std::endl;
    std::cout << "==================================================" << std::endl;

    return 0;
}
