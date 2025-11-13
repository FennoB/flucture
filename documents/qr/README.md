# flucture QR Code Generator

Advanced QR code generation system with extensive styling capabilities, inspired by Aidoo's customQRCode system but with significantly enhanced features.

## Features

### Core Capabilities
- ✅ **Multiple Output Formats**: PNG, PDF, SVG
- ✅ **Error Correction Levels**: LOW, MEDIUM, QUARTILE, HIGH
- ✅ **Automatic Version Selection**: QR versions 1-40
- ✅ **ASCII Art Export**: Terminal-friendly output

### Styling Options

#### Module Appearance
- **Shapes**: Square, Circle, Rounded, Diamond, Dots
- **SDF Rendering**: Signed Distance Field for smooth edges
- **Size Control**: Adjustable module size factor
- **Corner Radius**: Customizable rounding for rounded style

#### Colors & Gradients
- **RGB/RGBA Support**: Full color control with alpha channel
- **Hex Color Parsing**: `#RRGGBB` and `#RRGGBBAA` formats
- **Linear Gradients**: Angle-based color transitions
- **Radial Gradients**: Center-based color spreading
- **Predefined Colors**: Black, White, custom presets

#### Logo Embedding
- **Multiple Logos**: Support for multiple logo overlays
- **Position Control**: Precise X/Y placement (0.0-1.0 range)
- **Scale Adjustment**: Logo size relative to QR code
- **Alpha Blending**: Transparent logo support
- **Clear Under Logo**: Option to remove QR modules beneath logo
- **Base64 Support**: Embed logos directly without external files

#### Advanced Effects
- **Anti-Aliasing**: Smooth edges
- **Gaussian Blur**: Customizable blur radius
- **Drop Shadow**: Shadow offset and blur control
- **Custom Finder Patterns**: Styled corner markers
- **Animation Frames**: Multi-frame rendering support

### Preset Styles

#### `default_style()`
Standard black-on-white QR code with square modules.

```cpp
auto style = flx_qr_style::default_style();
// Black foreground, white background
// Square modules, 4-module margin
// Anti-aliasing enabled
```

#### `minimal_style()`
Compact QR code with reduced margins.

```cpp
auto style = flx_qr_style::minimal_style();
// 1-module margin instead of 4
// Otherwise same as default
```

#### `gradient_style()`
Modern gradient with rounded modules.

```cpp
auto style = flx_qr_style::gradient_style();
// Blue-to-purple linear gradient
// Rounded corners (0.3 radius)
// SDF rendering for smooth edges
```

#### `logo_style(path)`
Professional QR code with centered logo.

```cpp
auto style = flx_qr_style::logo_style("logo.png");
// 20% logo size, centered
// Rounded modules (0.2 radius)
// SDF enabled, 1-module padding
// Clears modules under logo
```

## Usage Examples

### Simple Generation

```cpp
#include "documents/qr/flx_qr_generator.h"

flx_qr_generator qr;
qr.generate("https://example.com");
qr.render_to_png("output.png", 500);    // 500px width
qr.render_to_pdf("output.pdf");          // A4 page
qr.render_to_svg("output.svg", 500.0);   // 500 units
```

### Custom Colors

```cpp
auto style = flx_qr_style::default_style();

flx_qr_color blue;
blue.init_rgb(0.0, 0.4, 0.9);
style.foreground_color = blue;

flx_qr_color light_gray;
light_gray.init_rgb(0.95, 0.95, 0.95);
style.background_color = light_gray;

qr.generate("Custom Colors", style);
```

### Hex Colors

```cpp
style.foreground_color = flx_qr_color::from_hex("#FF5733");
style.background_color = flx_qr_color::from_hex("#F0F0F0");
```

### Rounded Modules with SDF

```cpp
auto style = flx_qr_style::default_style();
style.module_style.value().shape = "rounded";
style.module_style.value().corner_radius = 0.4;  // 0.0-1.0
style.module_style.value().use_sdf = true;
style.module_style.value().size_factor = 0.95;   // Slightly smaller

qr.generate("Smooth Edges", style);
```

### Gradient Fill

```cpp
auto style = flx_qr_style::default_style();

flx_qr_gradient gradient;
gradient.type = "linear";
gradient.angle = 45.0;  // Degrees

flx_qr_color start;
start.init_rgb(1.0, 0.0, 0.0);  // Red
gradient.colors.push_back(start);

flx_qr_color end;
end.init_rgb(0.0, 0.0, 1.0);  // Blue
gradient.colors.push_back(end);

style.foreground_gradient = gradient;
```

### Logo Embedding

```cpp
auto style = flx_qr_style::default_style();

flx_qr_logo logo;
logo.image_path = "company_logo.png";
logo.scale = 0.25;           // 25% of QR code size
logo.pos_x = 0.5;            // Centered X
logo.pos_y = 0.5;            // Centered Y
logo.corner_radius = 0.15;   // Rounded corners
logo.padding = 1.5;          // Padding in modules
logo.clear_under_logo = true; // Remove QR modules

style.logos.push_back(logo);

qr.generate("With Logo", style);
```

### Multiple Logos

```cpp
// Center logo
flx_qr_logo center_logo;
center_logo.image_path = "main_logo.png";
center_logo.scale = 0.2;
center_logo.pos_x = 0.5;
center_logo.pos_y = 0.5;
style.logos.push_back(center_logo);

// Corner logo
flx_qr_logo corner_logo;
corner_logo.image_path = "badge.png";
corner_logo.scale = 0.1;
corner_logo.pos_x = 0.9;
corner_logo.pos_y = 0.1;
style.logos.push_back(corner_logo);
```

### Error Correction Control

```cpp
auto params = flx_qr_params::defaults();
params.error_correction = "HIGH";  // ~30% error tolerance
params.min_version = 5;            // Force larger version
params.max_version = 10;

qr.generate("High ECC Data", style, params);
```

### ASCII Art Output

```cpp
qr.generate("Terminal QR");
std::cout << qr.to_ascii_art().c_str() << std::endl;

// Custom characters
std::cout << qr.to_ascii_art("##", "  ").c_str() << std::endl;
```

### Animation Frames

```cpp
style.rotation_per_frame = 5.0;  // 5 degrees per frame
int frames_rendered = qr.render_animation("frame_%03d.png", 72, 600);
// Creates frame_000.png, frame_001.png, ..., frame_071.png
```

### Custom Finder Patterns

```cpp
style.finder_style.value().shape = "rounded";
style.finder_style.value().corner_radius = 0.4;

flx_qr_color finder_outer;
finder_outer.init_rgb(0.9, 0.1, 0.1);  // Red
style.finder_style.value().outer_color = finder_outer;

flx_qr_color finder_inner;
finder_inner.init_rgb(0.2, 0.1, 0.1);  // Dark red
style.finder_style.value().inner_color = finder_inner;
```

## Architecture

### Class Hierarchy

```
flx_model (base)
├── flx_qr_color
├── flx_qr_gradient
├── flx_qr_logo
├── flx_qr_module_style
├── flx_qr_finder_style
├── flx_qr_style
└── flx_qr_params

flx_qr_generator (main class)
└── Uses qrcodegen::QrCode (Nayuki's library)
```

### File Structure

```
documents/qr/
├── flx_qr_style.h              # Style model definitions
├── flx_qr_style.cpp            # Style implementations
├── flx_qr_generator.h          # Main generator interface
├── flx_qr_generator.cpp        # Rendering implementations
├── qrcodegen.hpp               # QR code library (Nayuki)
├── qrcodegen.cpp               # QR code library impl
├── example_qr_generation.cpp   # Usage examples
└── README.md                   # This file
```

### Dependencies

- **qrcodegen**: Nayuki's QR Code generator (MIT License)
- **OpenCV**: PNG rendering, image processing
- **PoDoFo**: PDF rendering
- **flx_model**: flucture's variant-based model system

## Integration with flucture

### CMakeLists.txt

Add to `FLUCTURE_CORE_SOURCES`:
```cmake
documents/qr/flx_qr_style.cpp
documents/qr/flx_qr_generator.cpp
documents/qr/qrcodegen.cpp
```

Add to `FLUCTURE_CORE_HEADERS`:
```cmake
documents/qr/flx_qr_style.h
documents/qr/flx_qr_generator.h
documents/qr/qrcodegen.hpp
```

### Build & Test

```bash
cd build
cmake ..
make

# Run tests
./flucture_tests "[qr]"

# Run examples
./qr_examples  # (if executable is built)
```

## Comparison with Aidoo System

| Feature | Aidoo customQRCode | flucture flx_qr_generator |
|---------|-------------------|---------------------------|
| **Output Formats** | PNG | PNG, PDF, SVG |
| **Logo Support** | Single centered | Multiple, positioned |
| **Gradients** | No | Yes (linear + radial) |
| **Module Shapes** | Square, rounded | Square, circle, rounded, diamond, dots |
| **SDF Rendering** | Yes | Yes (enhanced) |
| **Finder Styling** | Limited | Full customization |
| **Animation** | No | Yes (frame sequences) |
| **API Style** | C-style structs | flx_model properties |
| **Color Format** | RGB floats | RGB/RGBA + hex parsing |
| **Error Correction** | Fixed | Configurable (4 levels) |
| **ASCII Output** | No | Yes |

## Advanced Techniques

### SDF-Based Smooth Rendering

The system uses Signed Distance Fields for sub-pixel accurate rendering:

```cpp
// Enable for production-quality output
style.module_style.value().use_sdf = true;
style.module_style.value().sdf_threshold = 0.5;
```

SDF rendering calculates distance from pixel centers to module boundaries, enabling smooth anti-aliased edges even at high resolutions.

### Base64 Logo Embedding

```cpp
logo.image_base64 = "iVBORw0KGgoAAAANSUhEUgAA..."; // PNG base64
// No external file required
```

### Gradient Color Stops

```cpp
gradient.colors.push_back(color1);  // 0%
gradient.colors.push_back(color2);  // 50% (automatic)
gradient.colors.push_back(color3);  // 100%
// Evenly distributed stops
```

## Performance Notes

- **SDF rendering**: 2-3x slower than standard, use for final output
- **Logo blending**: Negligible impact with alpha channel
- **PDF generation**: Faster than PNG for vector output
- **SVG export**: Fastest, ideal for scalable designs

## Future Enhancements

Planned features:
- [ ] Explicit gradient stop positions
- [ ] Custom SVG path modules
- [ ] WebP output format
- [ ] GIF animation export
- [ ] Data URI scheme support
- [ ] Batch processing API
- [ ] QR code validation/verification
- [ ] Error correction visualization

## License

This QR code generator uses:
- **qrcodegen** by Project Nayuki (MIT License)
- **flucture framework** (Project license)

## See Also

- **qrcodegen library**: https://www.nayuki.io/page/qr-code-generator-library
- **flucture CLAUDE.md**: `/home/fenno/Projects/flucture/CLAUDE.md`
- **Test suite**: `tests/test_qr_generator.cpp`
- **Examples**: `example_qr_generation.cpp`
