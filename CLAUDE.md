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