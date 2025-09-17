# Flucture Project Documentation

## Core Architecture

### flx_model Framework
The project uses a custom model system with variant-based properties:

- **Properties are declared with macros**: `flxp_double(x)`, `flxp_int(count)`, `flxp_string(name)`
- **Properties work like normal variables**: Use `x = 5.0` not `x(5.0)`, and `double val = x` not `double val = x()`
- **Properties have automatic null handling**: Accessing null properties throws `flx_null_field_exception`
- **Models inherit from flx_model**: Which is a lazy pointer to flxv_map for storage
- **Nested models**: Use `flxp_model(child, ChildType)` and `flxp_model_list(items, ItemType)`
- **Const-correctness**: Non-const access creates defaults, const access throws on null
- **Reference assignment**: `auto& ref = model.prop; ref = value;` works naturally

### Layout System Classes
- **flx_layout_bounds**: Base class with position/size (x, y, width, height) and AABB containment methods
- **flx_layout_text**: Text elements with content, font properties (font_family, font_size, color, bold, italic)
- **flx_layout_image**: Image elements with metadata (image_path, description, mime_type, original_width/height)
- **flx_layout_geometry**: Hierarchical containers with flx_model_list for texts, images, and sub_geometries

### Coordinate System
- **PDF coordinates**: Points (1/72 inch) converted via `flx_coords::pdf_to_png_coord(coord, dpi)`
- **Containment principle**: "What is inside what" using AABB intersection methods
- **No page property**: Layout classes don't track page numbers directly

## Development Approach
- Test-driven development with Catch2 framework
- "From ground up" design - no legacy compatibility constraints  
- Minimal comments in code - documentation generated before commits
- Consistent naming: `flx_layout_*` for all layout classes

## File Structure
- `utils/`: Core framework (flx_model, flx_variant, flx_lazy_ptr, flx_string)
- `documents/layout/`: Layout detection classes (flx_layout_bounds, flx_layout_text, flx_layout_image, flx_layout_geometry)
- `documents/pdf/`: PDF-specific processing and coordinate conversion (flx_pdf_coords.h)
- `tests/`: Catch2 tests with auto-discovery, uses SCENARIO/GIVEN/WHEN/THEN structure
- `build/`: Generated build files (created by cmake)

## Testing Framework
- **Catch2**: Uses `#include <catch2/catch_all.hpp>`
- **Structure**: SCENARIO("description") with GIVEN/WHEN/THEN blocks
- **Assertions**: REQUIRE() for conditions, REQUIRE_THROWS_AS() for exceptions
- **Auto-discovery**: CMake finds all `test_*.cpp` files automatically
- **Main runner**: `test_main.cpp` with `#define CATCH_CONFIG_MAIN`

## Rules

### EVENT: Learning new concepts or making architectural changes
- Update CLAUDE.md with new understanding
- Document API changes and implementation details
- Update file structure documentation if changed

### EVENT: Creating new classes or files
- Add .cpp and .h files to CMakeLists.txt SOURCES and HEADERS sections
- Create corresponding Catch2 tests in tests/ directory
- Run full test suite to verify integration

### EVENT: Deleting or renaming files
- Remove file references from CMakeLists.txt
- Update include statements in dependent files
- Clean up any obsolete documentation references

### EVENT: Making major code changes
- Always run tests after implementation: `cd build && cmake .. && make && ./flucture_tests`
- Verify all tests pass before proceeding
- Update tests if behavior has intentionally changed

### EVENT: Adding new dependencies or libraries
- Update CMakeLists.txt target_link_libraries section
- Add include directories if needed
- Document new dependency in file structure section

### EVENT: Completing a major topic or feature
- Update CLAUDE.md with new learnings and implementation details
- Document any new API patterns, architectural decisions, or gotchas discovered
- Create a git commit with descriptive message summarizing the completed work
- Ensure all tests pass before committing

### EVENT: Before each commit
- Generate documentation for new code
- Ensure minimal inline comments (documentation is external)
- Run full test suite to verify stability

## Important Implementation Details
- Use `long long` internally but support `int` literals via SFINAE template magic
- Exception-based null access handling for const-correctness
- Properties use operator overloading for natural syntax
- Lazy pointer pattern with reference counting for memory management
- flx_model_list::push_back() handles models without explicit copying
- Use `double` literals in tests (e.g., `10.0` not `10`) to avoid type ambiguity with property comparisons
- flx_model_list requires non-const references for push_back operations

## Recent Fixes
- **Nested model_list access**: Fixed by implementing parent property sync system - models and model_lists now automatically sync with their parent properties before data access
- **Virtual operator* overrides**: Added to flx_model and flx_model_list to sync with parent before accessing underlying data

## Known Issues
- **Removed legacy classes**: Old flx_element and flx_geometry classes replaced by new layout system

## PoDoFo PDF API Learnings

### Polygon/Shape Rendering
- **Path Creation**: Use `PdfPainterPath` class, not direct painter methods
- **Path Operations**: 
  - `path.MoveTo(x, y)` - Start path at point
  - `path.AddLineTo(x, y)` - Add line to point
  - `path.Close()` - Close path
- **Drawing**: `painter.DrawPath(path, PdfPathDrawMode::Fill)` for filled shapes
- **Draw Modes**: `PdfPathDrawMode::Fill`, `::Stroke`, `::StrokeFill`, etc.

### Color Management
- **Private Methods**: `painter.SetNonStrokingColor()` is private - cannot use directly
- **GraphicsState Access**: `painter.GraphicsState` is wrapper, not direct struct
- **Color Setting**: Complex API - needs further investigation for proper color support
- **Temporary Solution**: Skip color setting for initial implementation

### Document Serialization
- **Buffer Output**: Use `StandardStreamDevice` with `std::stringstream`
- **Save Method**: `m_pdf->Save(StandardStreamDevice(buffer))` works correctly
- **Memory Management**: Always clean up PdfMemDocument* properly

### Forward Declarations
- **Header Files**: Use forward declarations for PoDoFo classes in headers
- **Required**: `namespace PoDoFo { class PdfMemDocument; class PdfPainter; }`
- **Include Strategy**: Include full headers only in .cpp files

## Polygon-Based Geometry System

### New Classes
- **flx_layout_vertex**: Simple point class with x,y properties
- **Enhanced flx_layout_geometry**: Now supports both container AND visual element
- **Dual Purpose**: Can be empty container OR polygon with fill_color

### Design Decisions
- **No stroke properties**: stroke_color/stroke_width removed (OpenCV detection difficulty)
- **Focus on detectables**: Only properties that CV can reliably extract
- **Vertex-based**: Arbitrary polygons supported via vertex list
- **Format-agnostic**: Pages property moved to base flx_doc_sio class

### OpenCV Considerations
- **Easy to detect**: fill_color (dominant color in region), vertices (contour detection)
- **Hard to detect**: stroke_color (edge analysis), stroke_width (line thickness measurement)
- **Future bidirectional mapping**: Only include properties we can both render AND detect

## Image Rendering System

### PoDoFo Image Integration
- **Image Creation**: `m_pdf->CreateImage()` returns unique_ptr<PdfImage>
- **Loading**: `pdf_image->LoadFromBuffer(bufferview)` from file data
- **Drawing**: `painter.DrawImage(*pdf_image, x, y, width, height)` with positioning
- **File Support**: PNG, JPEG via PoDoFo's built-in codecs

### OpenCV Integration
- **Validation**: `cv::imread()` for image loading validation
- **Metadata**: Extract original dimensions (width, height) from cv::Mat
- **Future**: Direct cv::Mat to PoDoFo conversion for processed images
- **Strategy**: File-based approach (load file → PoDoFo) vs pixel-based (cv::Mat → buffer)

### Implementation Strategy
- **Clean separation**: flx_layout_image stores metadata, rendering loads file
- **Error handling**: Graceful fallbacks when images missing/corrupt
- **Memory efficient**: No cv::Mat storage in layout model
- **Format agnostic**: PoDoFo handles format detection automatically

## Complete PDF Rendering System

### Text Rendering Over Polygons
- **Critical Fix**: Text must have explicit color set with `painter.GraphicsState.SetNonStrokingColor(PdfColor(0,0,0))`
- **Z-Order Issue**: Without explicit color, text can appear behind filled polygons
- **PDF Graphics State**: Text and polygon fills share the same graphics state, requiring color resets

### Image Positioning and Scaling
- **Coordinate System**: PDF uses bottom-left origin, layout system uses top-left
- **Y-Coordinate Conversion**: `pdf_y = 842.0 - layout_y - image_height` for A4 pages
- **Scaling API**: PoDoFo uses `DrawImage(image, x, y, scaleX, scaleY)` not width/height
- **Scale Calculation**: `scale = desired_size / original_image_size`

### Polygon Color Rendering
- **Fill Color API**: Use `painter.GraphicsState.SetNonStrokingColor(PdfColor)` for polygon fills
- **Color Parsing**: Convert hex strings (#RRGGBB) to PdfColor with RGB values 0.0-1.0
- **Rendering Order**: Polygons → Sub-geometries → Text → Images for correct layering

### Deep Nested Structures
- **4-Level Hierarchies**: Successfully tested with sidebar → bands → dots → accents
- **Color Variations**: Each level can have different fill colors creating complex visual patterns
- **Performance**: Nested rendering scales well with recursive `render_geometry_to_page()`