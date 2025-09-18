# Flucture Project Documentation

## Project Vision & Strategy

### AI Evaluation System for Layout Extraction
The primary goal is building a **Round-Trip Test System** for AI-based document layout extraction:

1. **Ground Truth Creation**: Start with known Layout structures (flx_layout_*)
2. **PDF Generation**: Layout → PDF (✅ Complete)
3. **AI Extraction**: PDF → Layout (Next major milestone)  
4. **AI Evaluator**: Compare original vs. extracted structures with quantitative metrics

This creates **synthetic training data** and **evaluation benchmarks** for layout extraction AI:
- **Controlled test scenarios** with perfect ground truth
- **Quantitative metrics** for structure accuracy, position precision, hierarchy correctness
- **Training datasets** for AI models without relying on unknown/unverified PDFs
- **Systematic evaluation** of extraction quality across different document types

The AI Evaluator will assess:
- Layout structure semantic similarity
- Coordinate accuracy with configurable tolerances  
- Hierarchical nesting correctness ("what is inside what")
- Component-specific scores (text extraction, geometry detection, image placement)

### PDF → Layout Extraction Strategy
Comprehensive 7-step process for reverse-engineering PDF structure:

**Phase 1: Content Extraction**
0. **PDF Memory Copy**: Create working copy of PDF in memory with PoDoFo
1. **Text/Image Extraction**: Use PoDoFo API to extract all texts and images into separate lists

**Phase 2: Geometry Isolation** 
2. **Content Removal**: Strip texts and images from PDF copy to isolate pure geometric shapes
3. **Clean Rendering**: Render cleaned PDF pages to images for computer vision processing

**Phase 3: Advanced Region Detection**
4. **OpenCV Processing**: Open rendered pages with OpenCV for contour analysis
5. **Flood-Fill Algorithm**: Custom color-coherent region detection using neighbor-by-neighbor color comparison (NOT traditional findContours). Creates binary masks for each color-coherent region, allowing gradients by comparing only adjacent pixels. Implements flood-fill that maintains "already processed" binary mask.
6. **Polygon Extraction**: Apply contour detection to binary masks for precise polygon boundaries

**Phase 4: Hierarchical Reconstruction**
6. **Containment Analysis**: Sort geometry objects into hierarchy using recursive sibling containment checking
7. **Content Assignment**: Recursively sort extracted texts and images into appropriate geometry containers based on spatial relationships

This approach ensures clean separation of geometric shapes from text artifacts and handles color gradients gracefully.

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
- `documents/pdf/podofo-master/`: Embedded PoDoFo 1.0.0 library source
- `tests/`: Catch2 tests with auto-discovery, uses SCENARIO/GIVEN/WHEN/THEN structure
- `build/`: Generated build files (created by cmake)

## Library Versions & Build Configuration
- **PoDoFo**: Version 1.0.0 (Major: 1, Minor: 0, Patch: 0)
  - **Build**: Static library (`PODOFO_BUILD_STATIC TRUE`)
  - **Location**: `documents/pdf/podofo-master/` (embedded source)
  - **Include Paths**: 
    - `documents/pdf/podofo-master/src`
    - `documents/pdf/podofo-master/src/podofo` 
  - **Link**: `podofo::podofo` target
- **OpenCV**: For computer vision processing (`${OpenCV_LIBS}`)
- **Catch2**: Version v3.6.0 for testing framework
- **C++ Standard**: C++17 required
- **Other Dependencies**: libcurl, OpenSSL, freetype, libmicrohttpd

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
- **Immediately start the next major topic** - maintain momentum and continue development

### EVENT: Before each commit
- Generate documentation for new code
- Ensure minimal inline comments (documentation is external)
- Run full test suite to verify stability

### EVENT: When using file system commands
- **Always check current working directory first** with `pwd`
- Adjust file paths accordingly (project root vs build directory)
- Working directory is typically `/home/fenno/Projects/flucture/build`

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

## PDF Text Extraction with Font Information

### Custom Text Extractor Implementation
Created `flx_pdf_text_extractor` to extract text with font size and font family information:

**Key Components:**
- **Enhanced Text Entry Structure**: `flx_pdf_text_entry` extends basic text with font metadata
- **Content Stream Processing**: Uses `PdfContentStreamReader` to parse PDF operators
- **Font State Tracking**: Monitors `Tf` (font) operators to capture font changes
- **Text Operator Handling**: Processes `Tj`, `'`, `"` operators for text content

**PDF Operator Support:**
- `BT/ET`: Begin/End text blocks
- `Tf`: Set font name and size (fontname size Tf)
- `Tj`: Show text string 
- `'`, `"`: Text with positioning/spacing
- `TJ`: Text array with individual glyph positioning (TODO)

**Font Family Detection:**
- Basic mapping for common fonts (Arial, Times, Courier, Helvetica)
- Extracts font name from `PdfFont` object via `font->GetName()`
- Fallback to "Arial" for unmapped or missing fonts

**Known Limitations:**
- Text positioning not fully implemented (requires text matrix tracking)
- TJ operator (text arrays) not yet supported
- Font data corruption issues when using PDF memory copies
- **Critical Issue**: Font data becomes corrupted when PDFs are serialized/reloaded
- **Workaround**: Pipeline structure implemented without actual text extraction

**Current Status:**
- ✅ Complete PDF → Layout parsing pipeline structure implemented
- ✅ PDF memory copy creation working
- ✅ Basic geometry isolation framework in place
- ❌ Text extraction disabled due to font corruption
- ⏳ Next: Implement PDF content removal for geometry isolation

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

## Complete PDF Text Extraction Implementation

### Major Achievement: Full PoDoFo Integration
Successfully copied the entire 1400+ line `PdfPage_TextExtraction.cpp` implementation from PoDoFo into our custom `flx_pdf_text_extractor` class:

- ✅ **Complete Font-Encoding Support**: CMap tables, composite fonts, glyph-to-Unicode mapping
- ✅ **All "Wild" PDF Text Formats**: Hex strings, character codes, embedded encodings, subsetting
- ✅ **Precise Text Matrix Transformations**: Correct positioning calculations with CTM and text matrices
- ✅ **Full PDF Operator Support**: BT/ET, Tf, Tj/TJ/Quote/DoubleQuote, Tm/Td/TD/T*
- ✅ **StatefulString Processing**: Character widths, glyph positions, proper UTF-8 handling
- ✅ **Hierarchical XObject Support**: Form XObjects and complex PDF structures

### Implementation Architecture
- **Single Class Design**: All PoDoFo structures refactored as nested structs/classes
- **Public flx-Compatible API**: `extract_text_with_fonts(PdfPage, vector<flx_layout_text>)`
- **Private PoDoFo Implementation**: Direct copy of original extraction logic
- **Conversion Layer**: `convertPdfEntriesToFlxTexts()` maps PoDoFo::PdfTextEntry to flx_layout_text

### Key Classes Copied
```cpp
struct TextState {
    Matrix T_rm, CTM, T_m, T_lm;  // Text transformation matrices
    PdfTextState PdfState;        // Font, size, spacing
    void ScanString(...);         // Character-level string processing
};

class StatefulString {
    const string String;
    const vector<double> Lengths, RawLengths;
    const vector<unsigned> StringPositions;  // Glyph positions
};

struct ExtractionContext {
    TextStateStack States;        // Graphics state stack
    StringChunkList Chunks;       // Text collection system
};
```

### Critical Font Data Issue Solved
- **Root Cause**: Font corruption occurred during PDF serialization/copying operations
- **Solution**: Extract text IMMEDIATELY after PDF loading, before any modifications
- **Architecture**: Text extraction now happens in `parse()` method before any PDF copying

## Complete PDF → Layout Extraction Pipeline

### Refactored Architecture: parse() Does Everything
Completely restructured the PDF processing to be semantically correct:

**Before**: `parse()` only loaded PDF, `parse_to_layout()` did extraction
**After**: `parse()` does complete PDF → Layout semantic extraction

### 7-Step Extraction Process (All in parse())
```cpp
bool flx_pdf_sio::parse(flx_string &data) {
    // Step 1: Load PDF
    m_pdf->LoadFromBuffer(buffer);
    
    // Step 2: Extract texts/images from ORIGINAL PDF (before modifications)
    extract_texts_and_images(extracted_texts, extracted_images);
    
    // Step 3: Create PDF copy for geometry isolation
    auto pdf_copy = create_pdf_copy();
    
    // Step 4: Remove texts/images from copy to isolate geometry
    remove_texts_and_images_from_copy(pdf_copy.get());
    
    // Step 5: Render cleaned PDF to images for OpenCV processing
    render_clean_pdf_to_images(pdf_copy.get(), clean_images);
    
    // Step 6: Detect color-coherent regions using flood-fill
    detect_color_regions(page_image);
    
    // Step 7: Build layout structure and assign content
    build_geometry_hierarchy(root_geometries);
    assign_content_to_geometries(texts, images, geometries);
    
    // Store final layout in pages
    pages = root_geometries;
}
```

### Design Philosophy
- **Semantic Correctness**: Reading a file should extract its semantic structure
- **Single Responsibility**: `parse()` method does complete document understanding
- **No Intermediate States**: Either fully parsed or failed, no partial states
- **Immediate Processing**: Text extraction happens before any PDF modifications

### Font Corruption Prevention
- **Timing Critical**: Text extraction MUST happen immediately after PDF loading
- **No Serialization**: Extract from original PDF document, never from copies
- **Font State Preservation**: All font objects and encoding tables remain intact

### Current Implementation Status
- ✅ **Complete PoDoFo Text Extractor**: Full 1400+ line implementation integrated
- ✅ **Refactored parse() Method**: Complete PDF → Layout pipeline architecture
- ✅ **Font Issue Resolution**: Text extraction timing fixed
- ✅ **All Steps 1-7 Fully Implemented**: Complete pipeline working and compiling
- ✅ **Neighbor-Based Flood-Fill**: Custom algorithm for gradient-aware region detection
- ✅ **Hierarchical Geometry Reconstruction**: Containment-based structure building
- ✅ **Direct flx_model_list Integration**: No std::vector conversion needed

## Complete Round-Trip Test System
1. **Layout → PDF**: ✅ Complete and working
2. **PDF → Layout**: ✅ Complete pipeline implemented
3. **AI Evaluator**: Ready for quantitative comparison metrics

### Key Implementation Details
- **PDF Content Stream Filtering**: Removes text/image operators while preserving geometry
- **Neighbor-Comparison Flood-Fill**: Each pixel compared only to neighbors for gradient support
- **No White Filtering**: White regions preserved, filtered only at border detection stage
- **Recursive Content Assignment**: Texts/images assigned to deepest matching geometry container
- **Direct Model List Usage**: Works directly with flx_model_list throughout pipeline