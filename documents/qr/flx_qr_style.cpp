#include "flx_qr_style.h"
#include <sstream>
#include <iomanip>

// flx_qr_color implementations
flx_string flx_qr_color::to_hex() const {
    std::stringstream ss;
    ss << "#"
       << std::hex << std::setfill('0')
       << std::setw(2) << static_cast<int>(r.value() * 255)
       << std::setw(2) << static_cast<int>(g.value() * 255)
       << std::setw(2) << static_cast<int>(b.value() * 255);

    if (!a.is_null() && a.value() < 1.0) {
        ss << std::setw(2) << static_cast<int>(a.value() * 255);
    }

    return flx_string(ss.str());
}

flx_qr_color flx_qr_color::from_hex(const flx_string& hex) {
    flx_qr_color color;
    std::string hex_str = hex.to_std_const();

    // Remove # if present
    if (hex_str[0] == '#') {
        hex_str = hex_str.substr(1);
    }

    // Parse RGB or RGBA
    if (hex_str.length() >= 6) {
        color.r = std::stoi(hex_str.substr(0, 2), nullptr, 16) / 255.0;
        color.g = std::stoi(hex_str.substr(2, 2), nullptr, 16) / 255.0;
        color.b = std::stoi(hex_str.substr(4, 2), nullptr, 16) / 255.0;

        if (hex_str.length() == 8) {
            color.a = std::stoi(hex_str.substr(6, 2), nullptr, 16) / 255.0;
        } else {
            color.a = 1.0;
        }
    }

    return color;
}

// flx_qr_style preset methods
flx_qr_style flx_qr_style::default_style() {
    flx_qr_style style;

    // Black on white
    style.foreground_color = flx_qr_color::black();
    style.background_color = flx_qr_color::white();

    // Square modules
    style.module_style.shape = "square";
    style.module_style.size_factor = 1.0;
    style.module_style.use_sdf = false;

    // Standard finders
    style.finder_style.shape = "square";
    style.finder_style.outer_color = flx_qr_color::black();
    style.finder_style.inner_color = flx_qr_color::black();

    // Standard margins
    style.margin = 4.0;
    style.scale = 10.0;
    style.anti_alias = true;

    return style;
}

flx_qr_style flx_qr_style::minimal_style() {
    flx_qr_style style = default_style();
    style.margin = 1.0;
    return style;
}

flx_qr_style flx_qr_style::gradient_style() {
    flx_qr_style style = default_style();

    // Blue to purple gradient for foreground
    flx_qr_gradient fg_gradient;
    fg_gradient.type = "linear";
    fg_gradient.angle = 45.0;

    flx_qr_color blue;
    blue.init_rgb(0.2, 0.4, 0.9);
    fg_gradient.colors.push_back(blue);

    flx_qr_color purple;
    purple.init_rgb(0.7, 0.2, 0.9);
    fg_gradient.colors.push_back(purple);

    style.foreground_gradient = fg_gradient;

    // Rounded modules
    style.module_style.shape = "rounded";
    style.module_style.corner_radius = 0.3;

    return style;
}

flx_qr_style flx_qr_style::logo_style(const flx_string& logo_path) {
    flx_qr_style style = default_style();

    // Add centered logo
    flx_qr_logo logo;
    logo.image_path = logo_path;
    logo.scale = 0.2;
    logo.pos_x = 0.5;
    logo.pos_y = 0.5;
    logo.corner_radius = 0.1;
    logo.padding = 1.0;
    logo.clear_under_logo = true;

    style.logos.push_back(logo);

    // Rounded modules for modern look
    style.module_style.shape = "rounded";
    style.module_style.corner_radius = 0.2;
    style.module_style.use_sdf = true;

    return style;
}
