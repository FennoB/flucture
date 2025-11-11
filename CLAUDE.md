# Flucture Project Documentation

## Quick Reference

### Common Tasks
```bash
# Build project
cd build && cmake .. && make

# Run all tests
./flucture_tests

# Run fast unit tests only (5 seconds)
./flucture_tests "~[db]~[slow]~[integration]~[ai]"

# Run specific test
./flucture_tests "flx_datetime"

# List all tests
./flucture_tests --list-tests
```

### Where to Find What

**Need to understand/modify...**
- **Model system & properties** → `utils/flx_model.h`, `utils/flx_variant.h`
- **Layout structures** → `documents/layout/flx_layout_*.h`
- **PDF rendering** → `documents/pdf/flx_pdf_sio.h` (method: `render()`)
- **PDF parsing** → `documents/pdf/flx_pdf_sio.h` (method: `parse()`)
- **Text extraction from PDF** → `documents/pdf/flx_pdf_text_extractor.h`
- **AI layout evaluation** → `aiprocesses/eval/flx_layout_evaluator.h`
- **Database ORM** → `api/db/db_repository.h`, `api/db/pg_connection.h`
- **Database exceptions** → `api/db/db_exceptions.h` (11 exception classes)
- **OpenAI integration** → `api/aimodels/flx_openai_api.h`
- **HTTP server** → `api/server/flx_rest_api.h`
- **String utilities** → `utils/flx_string.h`
- **DateTime handling** → `utils/flx_datetime.h`
- **Test examples** → `tests/test_*.cpp`, `tests/db_repository/`
- **Build configuration** → `CMakeLists.txt`

### Key Documentation Files
- **This file (CLAUDE.md)**: Architecture, guidelines, API reference
- **tests/README.md**: Complete test system documentation
- **DEBUG_PH_WERT_PROBLEM.md**: XObject font-cache debugging case study (historical)
- **CHANGELOG.md**: Version history and breaking changes

---

## Project Overview

### Vision & Purpose
**AI-powered PDF layout extraction framework** with bidirectional round-trip capabilities:

1. **Layout → PDF**: Render complex layouts to PDF (nested geometries, polygons, text, images)
2. **PDF → Layout**: Extract semantic structure from PDFs (text with fonts, images, geometry hierarchy)
3. **AI Evaluator**: Quantitatively evaluate extraction quality (0.0-1.0 scores)

**Use Cases:**
- **Synthetic training data**: Generate PDFs with perfect ground truth for AI training
- **Extraction benchmarking**: Quantitative evaluation of layout extraction accuracy
- **Document processing**: Semantic understanding of PDF structure
- **Round-trip testing**: Verify Layout → PDF → Layout preserves structure

### Current Capabilities (Production-Ready ✅)
- ✅ **PDF Rendering**: Complete with 4+ level hierarchies, polygons, fonts, images
- ✅ **PDF Text Extraction**: Full PoDoFo 1400+ line implementation with font information
- ✅ **XObject Support**: Per-page font-cache clearing prevents corruption
- ✅ **AI Evaluation**: GPT-4 based quantitative scoring system
- ✅ **Database ORM**: PostgreSQL integration with hierarchical CRUD
- ✅ **REST API**: HTTP server with JSON/XML support
- ✅ **Semantic Search**: Vector embeddings with OpenAI

### Technology Stack
| Component | Technology | Version |
|-----------|-----------|---------|
| **PDF Processing** | PoDoFo (embedded static lib) | 1.0.0 |
| **Computer Vision** | OpenCV | System version |
| **AI Evaluation** | OpenAI GPT-4 | gpt-4-turbo-preview |
| **Database** | PostgreSQL (libpqxx) | System version |
| **HTTP Server** | libmicrohttpd | Embedded |
| **HTTP Client** | libcurl | System version |
| **Testing** | Catch2 | v3.6.0 |
| **XML Parsing** | pugixml | System version |
| **C++ Standard** | C++17 | Required |

---

## Architecture Guide

### Core Framework: flx_model

**Variant-based model system** with automatic property management:

```cpp
class MyModel : public flx_model {
public:
    flxp_int(id);                          // Properties declared with macros
    flxp_string(name);                     // Work like normal variables
    flxp_double(score);                    // Automatic null handling
    flxp_bool(active);
    flxp_model(child, ChildModel);         // Nested models
    flxp_model_list(items, ItemModel);     // Collections of models
};

// Usage - natural syntax
MyModel model;
model.id = 42;                             // Assignment works naturally
model.name = "Test";                       // No function calls needed
long long value = model.id;               // Direct access

// Null checking
if (model.name.is_null()) { /*...*/ }

// Const-correctness
const MyModel& const_ref = model;
// const_ref.id; // Throws flx_null_field_exception if null
```

**Key Characteristics:**
- **Properties are variables**: Use `x = 5.0` not `x(5.0)`
- **Lazy initialization**: Non-const access creates defaults, const throws on null
- **Variant storage**: flx_variant handles type conversions internally
- **Reference counting**: flx_lazy_ptr for efficient memory management
- **No const_cast**: ABSOLUTELY FORBIDDEN - framework incompatible

**Metadata System for Database Integration:**

Properties can be annotated with metadata for automatic ORM mapping:

```cpp
class User : public flx_model {
    // Primary key: table name in metadata value
    flxp_int(id, {
        {"column", "id"},
        {"primary_key", "users"}  // Value = table name
    });

    // Regular fields: column metadata is REQUIRED
    flxp_string(name, {{"column", "username"}});
    flxp_string(email, {{"column", "email_address"}});

    // Foreign key to another table (reference)
    flxp_int(company_id, {
        {"column", "company_id"},
        {"foreign_key", "companies"}  // Value = target table
    });

    // Child relation: NO metadata on parent side!
    flxp_model_list(orders, Order);  // No metadata here
};

class Order : public flx_model {
    flxp_int(id, {{"column", "id"}, {"primary_key", "orders"}});

    // Foreign key back to parent table
    flxp_int(user_id, {
        {"column", "user_id"},
        {"foreign_key", "users"}  // FK in child points to parent table
    });

    flxp_double(total, {{"column", "total_amount"}});
};
```

**Metadata Rules:**
- ✅ `{"column", "db_column_name"}` - **REQUIRED** for DB fields (no fallback!)
- ✅ `{"primary_key", "table_name"}` - Marks PK, value = table name
- ✅ `{"foreign_key", "target_table"}` - Marks FK, value = target table
- ❌ `flxp_model` and `flxp_model_list` have **NO metadata**
- ❌ No fallback to property name if `column` missing - field will be ignored

**Relationship Detection:**
Repository scans child models to find FK fields pointing back to parent table.

### Layout System

**Hierarchical document structure** with AABB containment:

```
flx_layout_bounds (x, y, width, height)
├── flx_layout_text (text content + font properties)
├── flx_layout_image (image path + metadata)
├── flx_layout_vertex (polygon vertex point)
└── flx_layout_geometry (container + visual element)
    ├── texts: flx_model_list<flx_layout_text>
    ├── images: flx_model_list<flx_layout_image>
    ├── sub_geometries: flx_model_list<flx_layout_geometry>
    └── vertices: flx_model_list<flx_layout_vertex>
```

**Design Principles:**
- **Dual-purpose geometry**: Can be empty container OR polygon with fill_color
- **No stroke properties**: Removed (difficult for OpenCV detection)
- **Containment-based**: "What is inside what" using AABB intersection
- **Detectable properties only**: Only include what both rendering AND detection can handle

### Document Processing Pipeline

**PDF → Layout Extraction** (7-step process in `flx_pdf_sio::parse()`):

```cpp
// Step 1: Load PDF into memory
m_pdf->LoadFromBuffer(buffer);

// Step 2: Extract text/images from ORIGINAL (before modifications)
extract_texts_and_images(extracted_texts, extracted_images);

// Step 3: Create working copy for geometry isolation
auto pdf_copy = create_pdf_copy();

// Step 4: Remove text/images from copy (isolate pure geometry)
remove_texts_and_images_from_copy(pdf_copy.get());

// Step 5: Render cleaned PDF to images for OpenCV
render_clean_pdf_to_images(pdf_copy.get(), clean_images);

// Step 6: Detect color-coherent regions (flood-fill algorithm)
detect_color_regions(page_image);

// Step 7: Build hierarchy and assign content
build_geometry_hierarchy(root_geometries);
assign_content_to_geometries(texts, images, geometries);
```

**Critical Timing:** Text extraction MUST happen immediately after loading (before any serialization/copying) to prevent font corruption.

---

## File Structure & Navigation

### Project Directory Layout

```
/home/fenno/Projects/flucture/
├── utils/                    # Core framework (15 files)
│   ├── flx_model.{h,cpp}    # Property system & variant-based models
│   ├── flx_variant.{h,cpp}  # Type-safe variant storage
│   ├── flx_lazy_ptr.h       # Reference-counted lazy pointers
│   ├── flx_string.{h,cpp}   # Enhanced string with 20+ utilities
│   └── flx_datetime.{h,cpp} # UTC datetime with millisecond precision
│
├── documents/                # Document processing
│   ├── layout/              # Layout structure classes
│   │   ├── flx_layout_bounds.{h,cpp}      # Base: position/size + AABB
│   │   ├── flx_layout_text.{h,cpp}        # Text + font properties
│   │   ├── flx_layout_image.{h,cpp}       # Image + metadata
│   │   ├── flx_layout_vertex.{h,cpp}      # Polygon vertex
│   │   └── flx_layout_geometry.{h,cpp}    # Hierarchical container
│   │
│   ├── pdf/                 # PDF processing
│   │   ├── flx_pdf_sio.{h,cpp}            # Main: parse() & render()
│   │   ├── flx_pdf_text_extractor.{h,cpp} # 1400+ line PoDoFo integration
│   │   ├── flx_pdf_coords.h               # Coordinate conversions
│   │   └── podofo-master/                 # Embedded PoDoFo 1.0.0 source
│   │
│   ├── flx_doc_sio.{h,cpp}  # Base document serialization class
│   └── flx_layout_to_html.{h,cpp}         # HTML export (alternative)
│
├── api/                      # API integrations
│   ├── aimodels/            # AI model APIs
│   │   └── flx_openai_api.{h,cpp}         # OpenAI GPT-4 integration
│   │
│   ├── db/                  # Database ORM
│   │   ├── db_repository.h              # Generic repository pattern
│   │   ├── db_connection.h, db_query.h  # Abstract interfaces
│   │   ├── pg_connection.{h,cpp}        # PostgreSQL implementation
│   │   ├── pg_query.{h,cpp}             # Query execution
│   │   ├── db_query_builder.{h,cpp}     # SQL builder
│   │   ├── db_search_criteria.{h,cpp}   # Search filters
│   │   └── flx_semantic_embedder.{h,cpp} # Vector embeddings
│   │
│   ├── server/              # HTTP server
│   │   ├── flx_httpdaemon.{h,cpp}       # libmicrohttpd wrapper
│   │   ├── flx_rest_api.{h,cpp}         # REST endpoint handling
│   │   └── libmicrohttpd_x86_64/        # Embedded server library
│   │
│   ├── client/              # HTTP client
│   │   └── flx_http_request.{h,cpp}     # libcurl wrapper
│   │
│   ├── json/                # JSON processing
│   │   ├── flx_json.{h,cpp}             # JSON ↔ flx_model conversion
│   │   └── json.hpp                     # nlohmann/json header-only
│   │
│   └── xml/                 # XML processing
│       └── flx_xml.{h,cpp}              # XML ↔ flx_model conversion
│
├── aiprocesses/              # AI-powered processes
│   ├── eval/                # Evaluation systems
│   │   ├── flx_layout_evaluator.{h,cpp} # AI layout comparison
│   │   └── flx_ai_process_evaluator.{h,cpp} # Generic evaluator
│   │
│   ├── chat/                # LLM chat integration
│   │   ├── flx_llm_api.h                # LLM interface
│   │   ├── flx_llm_chat.{h,cpp}         # Chat session management
│   │   ├── flx_llm_chat_interfaces.h    # Message interfaces
│   │   └── flx_chat_snippet_source.{h,cpp} # Context retrieval
│   │
│   ├── snippets/            # Code snippet management
│   │   ├── flx_snippet.{h,cpp}          # Snippet model
│   │   └── flx_snippet_source.{h,cpp}   # Snippet storage
│   │
│   └── flx_ai_process.{h,cpp}           # Base AI process class
│
├── tests/                    # Catch2 test suite (30+ files)
│   ├── test_main.cpp        # Catch2 main runner
│   ├── test_flx_model.cpp   # [unit] Model system tests
│   ├── test_flx_datetime.cpp # [unit] DateTime tests
│   ├── test_layout_classes.cpp # [unit] Layout structure tests
│   ├── test_pdf_rendering.cpp # [pdf] PDF generation tests
│   ├── test_pdf_parsing.cpp # [pdf] PDF extraction tests
│   ├── test_layout_evaluator.cpp # [ai] AI evaluation tests
│   ├── test_db_repository.cpp # [db] Database ORM tests
│   └── README.md            # Complete test documentation
│
├── build/                    # CMake build output
├── main.cpp                  # Executable entry point
├── CMakeLists.txt           # Build configuration
└── CLAUDE.md                # This file
```

### Build Configuration

**CMakeLists.txt** defines:
- `flucture_core` library (all sources except main.cpp)
- `flucture` executable (links against flucture_core)
- `pdf_to_layout` API executable
- `flucture_tests` test runner (auto-discovers test_*.cpp)

**Key Build Variables:**
```cmake
CMAKE_CXX_STANDARD 17
PODOFO_BUILD_STATIC TRUE
BUILD_FLUCTURE_TESTS ON
```

---

## Development Guidelines

### Critical Rules ⚠️

#### ABSOLUTE PROHIBITIONS

1. **❌ NEVER use `const_cast`**
   - Framework is const-incompatible
   - Leads to undefined behavior and segfaults
   - **Solution**: Accept non-const references, use mutable members, refactor API

2. **❌ NEVER use PoDoFo `ExtractTextTo()` methods**
   - Causes memory management issues and segfaults
   - **Solution**: Use custom `flx_pdf_text_extractor` with direct content stream parsing

3. **❌ NEVER create dummy/fallback implementations**
   - Hides real problems
   - All issues must be properly solved
   - No "Added dummy text" or placeholder code

4. **❌ NEVER skip pre-commit hooks**
   - No `--no-verify` or `--no-gpg-sign` unless explicitly requested

#### REQUIRED PRACTICES

1. **✅ Properties work like normal variables**
   ```cpp
   model.x = 5.0;        // ✅ Correct
   model.x(5.0);         // ❌ Wrong

   double val = model.x; // ✅ Correct
   double val = model.x(); // ❌ Wrong
   ```

2. **✅ Use double literals in tests**
   ```cpp
   REQUIRE(bounds.x == 10.0);  // ✅ Correct
   REQUIRE(bounds.x == 10);    // ❌ Type ambiguity
   ```

3. **✅ Check working directory before file operations**
   ```bash
   pwd  # Check first
   # Adjust paths accordingly (project root vs build/)
   ```

4. **✅ Extract PDF text IMMEDIATELY after loading**
   - Before any PDF copying/serialization
   - Prevents font data corruption

5. **✅ Always read API headers before using libraries**
   - Check method signatures, parameter types, return types
   - Prevents compilation errors

### Testing Framework

**See `tests/README.md` for complete documentation.**

**Quick Test Commands:**
```bash
cd build

# Fast unit tests (5 sec)
./flucture_tests "~[db]~[slow]~[integration]~[ai]"

# Database tests
export DB_HOST=localhost DB_PORT=5432 DB_NAME=test_db DB_USER=test DB_PASS=test
./flucture_tests "[db]~[slow]"

# AI evaluation tests
export OPENAI_API_KEY="sk-..."
./flucture_tests "[ai]"

# Full suite
./flucture_tests "~[disabled]"
```

**Test Tags:**
- `[unit]` - Fast (<1s), no external dependencies
- `[db]` - Requires PostgreSQL
- `[ai]`, `[llm]` - Requires OpenAI API
- `[slow]` - Takes >5 seconds
- `[integration]` - External services
- `[pdf]`, `[evaluator]`, `[e2e]` - Feature-specific

**Test Structure:**
```cpp
#include <catch2/catch_all.hpp>

SCENARIO("Feature description", "[unit]") {
    GIVEN("Initial state") {
        // Setup

        WHEN("Action performed") {
            // Execute

            THEN("Expected result") {
                REQUIRE(condition);
            }
        }
    }
}
```

### Build System

**Standard build process:**
```bash
cd /home/fenno/Projects/flucture
mkdir -p build
cd build
cmake ..
make -j$(nproc)
```

**CMake auto-discovers:**
- All `test_*.cpp` files in `tests/`
- All source files listed in `FLUCTURE_CORE_SOURCES`

**Adding new files:**
1. Add `.cpp` to `FLUCTURE_CORE_SOURCES` in CMakeLists.txt
2. Add `.h` to `FLUCTURE_CORE_HEADERS` in CMakeLists.txt
3. Create corresponding test file `tests/test_*.cpp` (auto-discovered)
4. Re-run `cmake ..` to update build configuration

### Event-Driven Documentation Rules

**When completing major features:**
1. Update CLAUDE.md with new architecture details
2. Document new API patterns and gotchas
3. Create git commit with descriptive message
4. Run full test suite to verify
5. **Immediately start next major topic** (maintain momentum)

**When learning new APIs:**
1. Read header files first
2. Document learnings in CLAUDE.md
3. Update relevant sections (don't create new ones)

**When changing file structure:**
1. Update CMakeLists.txt
2. Update "File Structure & Navigation" in CLAUDE.md
3. Update include statements in dependent files

**Before each commit:**
1. Run test suite: `cd build && make && ./flucture_tests`
2. Update CLAUDE.md if architecture changed
3. Ensure all tests pass

---

## API Reference

### flx_model Framework

**Property Declaration Macros:**
```cpp
// Basic properties (no metadata)
flxp_int(name)                    // long long storage, supports int literals
flxp_double(name)                 // double precision floating point
flxp_bool(name)                   // boolean
flxp_string(name)                 // flx_string with 20+ utility methods
flxp_vector(name)                 // flxv_vector (variant vector)
flxp_map(name)                    // flxv_map (variant map)
flxp_model(name, Type)            // Nested model
flxp_model_list(name, Type)       // Collection of models

// With database metadata (optional third parameter for basic properties only)
flxp_int(id, {{"column", "user_id"}, {"primary_key", "users"}})
flxp_string(email, {{"column", "email_address"}})
flxp_int(company_id, {{"column", "company_id"}, {"foreign_key", "companies"}})

// Nested models and lists: NO metadata!
flxp_model(address, Address)           // No third parameter
flxp_model_list(orders, Order)         // No third parameter

// Metadata Keys (only for basic properties):
// {"column", "db_name"}       - REQUIRED for DB fields (no fallback!)
// {"primary_key", "table"}    - Marks PK, value = table name
// {"foreign_key", "table"}    - Marks FK, value = target table
```

**Property Access:**
```cpp
// Non-const access (creates default if null)
model.property = value;
auto& ref = model.property;
type val = model.property.value();

// Const access (throws if null)
const Model& const_ref = model;
// const_ref.property.value();  // Throws flx_null_field_exception if null

// Null checking
if (model.property.is_null()) { /*...*/ }

// Arrow operator for property methods
if (model.name->empty()) { /*...*/ }
const char* str = model.name->c_str();
```

**Model Lists:**
```cpp
flxp_model_list(items, ItemType);

// Adding elements
ItemType item;
item.name = "Test";
items.push_back(item);           // Accepts const references

// Or create empty and access via back()
items.add_element();
items.back().name = "Test";

// Iteration
for (auto& item : items) {
    std::cout << item.name.value() << "\n";
}
```

**Exceptions:**
- `flx_null_field_exception` - Thrown when accessing null property in const context
- `flx_null_access_exception` - Thrown when dereferencing null lazy pointer

### Database Exception Handling

**Exception-based error handling** (replaces bool returns as of November 2025):

**File:** `api/db/db_exceptions.h`

**Exception Hierarchy:**
```
db_exception (base)
├── db_connection_error
├── db_query_error
│   ├── db_prepare_error
│   └── db_constraint_violation
│       ├── db_foreign_key_violation
│       └── db_unique_violation
├── db_model_error
│   ├── db_null_id_error
│   ├── db_record_not_found
│   ├── db_no_fields_error
│   └── db_no_table_name_error
├── db_nested_save_error
└── db_table_not_found
```

**Usage Pattern:**
```cpp
#include "api/db/db_repository.h"
#include "api/db/db_exceptions.h"

db_repository repo(&conn);
MyModel model;

try {
    // CRUD operations throw on error (no bool returns)
    repo.create(model);           // throws db_connection_error, db_query_error, etc.
    repo.update(model);           // throws db_null_id_error if ID is null
    repo.remove(model);           // throws db_null_id_error if ID is null
    repo.find_by_id(123, model);  // throws db_record_not_found if not found

} catch (const db_foreign_key_violation& e) {
    // Specific exception with parsed details
    std::cout << "FK violation: " << e.get_foreign_key_column()
              << " references " << e.get_referenced_table() << "\n";
    std::cout << "SQL: " << e.get_sql() << "\n";
    std::cout << "DB error: " << e.get_database_error() << "\n";

} catch (const db_unique_violation& e) {
    std::cout << "Duplicate: " << e.get_column_name()
              << " = " << e.get_duplicate_value().string_value() << "\n";

} catch (const db_record_not_found& e) {
    std::cout << "Not found in " << e.get_table_name()
              << ": ID = " << e.get_id() << "\n";

} catch (const db_nested_save_error& e) {
    std::cout << "Failed saving " << e.get_child_table()
              << " (parent: " << e.get_parent_table() << ")\n";

} catch (const db_exception& e) {
    // Base catch-all
    std::cout << "Database error: " << e.what() << "\n";
}
```

**Method Signatures (Exception-based):**
```cpp
// All throw db_exception or derived types on error
void create(flx_model& model);
void update(flx_model& model);
void remove(flx_model& model);
void find_by_id(long long id, flx_model& model);
void find_all(flx_list& results);
void find_where(const flx_string& condition, flx_list& results);

// Only table_exists() returns bool (semantic meaning)
bool table_exists(flx_model& model);
```

**PostgreSQL Error Parsing:**
```cpp
// db_repository automatically parses PostgreSQL error messages:
// - Foreign key violations → extracts column name + referenced table
// - Unique constraint violations → extracts column name + duplicate value
// - Table not found → detects during prepare() and execute()
```

**Testing Exception Handling:**
```cpp
// In tests (Catch2):
REQUIRE_THROWS_AS(repo.create(invalid_model), db_foreign_key_violation);
REQUIRE_THROWS_AS(repo.find_by_id(999, model), db_record_not_found);
REQUIRE_NOTHROW(repo.create(valid_model));

// Test exception details:
try {
    repo.create(department_without_company);
    FAIL("Should have thrown");
} catch (const db_foreign_key_violation& e) {
    REQUIRE(e.get_foreign_key_column() == "company_id");
    REQUIRE(e.get_referenced_table() == "test_companies");
}
```

**Migration Notes:**
- `get_last_error()` still exists but is legacy (unused in exception-based code)
- All CRUD methods throw exceptions instead of returning false
- Breaking change: Code using bool returns must migrate to try-catch

### PoDoFo PDF API

**Includes:**
```cpp
#include <podofo/podofo.h>
using namespace PoDoFo;
```

**Document Creation:**
```cpp
PdfMemDocument pdf;
PdfPage& page = pdf.GetPages().CreatePage(PdfPage::CreateStandardPageSize(PdfPageSize::A4));
PdfPainter painter;
painter.SetCanvas(page);
```

**Text Rendering:**
```cpp
// CRITICAL: Must set color explicitly
painter.GraphicsState.SetNonStrokingColor(PdfColor(0, 0, 0));

PdfFont* font = pdf.GetFonts().SearchFont("Arial");
painter.TextState.SetFont(*font, 12.0);
painter.DrawText("Text content", x, y);
```

**Polygon Rendering:**
```cpp
PdfPainterPath path;
path.MoveTo(x1, y1);
path.AddLineTo(x2, y2);
path.AddLineTo(x3, y3);
path.Close();

// Set fill color (RGB 0.0-1.0)
painter.GraphicsState.SetNonStrokingColor(PdfColor(r, g, b));
painter.DrawPath(path, PdfPathDrawMode::Fill);
```

**Image Rendering:**
```cpp
auto pdf_image = pdf.CreateImage();
pdf_image->LoadFromBuffer(bufferview);

// Y-coordinate conversion (PDF uses bottom-left origin)
double pdf_y = page_height - layout_y - image_height;
double scale_x = desired_width / original_width;
double scale_y = desired_height / original_height;

painter.DrawImage(*pdf_image, x, pdf_y, scale_x, scale_y);
```

**Content Stream Parsing:**
```cpp
PdfContentStreamReader reader(&page);

// Custom callback
struct TextExtractor : public PdfContentStreamReader::IStreamHandler {
    void OnOperator(const std::string& op, const PdfArray& args) override {
        if (op == "BT") { /* Begin text */ }
        if (op == "Tf") { /* Set font */ }
        if (op == "Tj") { /* Show text */ }
    }
};

TextExtractor extractor;
reader.Read(extractor);
```

**Document Serialization:**
```cpp
std::stringstream buffer;
StandardStreamDevice device(buffer);
pdf.Save(device);
std::string pdf_data = buffer.str();
```

### OpenCV Integration

**Image Loading:**
```cpp
#include <opencv2/opencv.hpp>

cv::Mat image = cv::imread(path, cv::IMREAD_COLOR);
if (image.empty()) { /* error */ }

int width = image.cols;
int height = image.rows;
```

**Contour Detection:**
```cpp
cv::Mat gray, binary;
cv::cvtColor(image, gray, cv::COLOR_BGR2GRAY);
cv::threshold(gray, binary, 127, 255, cv::THRESH_BINARY);

std::vector<std::vector<cv::Point>> contours;
cv::findContours(binary, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

for (const auto& contour : contours) {
    // Process contour points
}
```

**Flood-Fill Algorithm:**
```cpp
// Custom neighbor-by-neighbor color comparison
// Each pixel compared only to adjacent pixels (supports gradients)
// Implements binary "already processed" mask
// NOT using traditional findContours for initial region detection
```

### OpenAI API Integration

**Setup:**
```cpp
#include "api/aimodels/flx_openai_api.h"

flx_string api_key = std::getenv("OPENAI_API_KEY");
auto api = std::make_shared<openai_api>(api_key);
```

**Layout Evaluation:**
```cpp
#include "aiprocesses/eval/flx_layout_evaluator.h"

flx_layout_evaluator evaluator(api);
auto result = evaluator.evaluate_extraction(original_layout, extracted_layout);

// Scores (0.0 - 1.0)
double overall = result.overall_score;
double structure = result.structure_similarity;
double position = result.position_accuracy;
double hierarchy = result.hierarchy_correctness;
double text = result.text_extraction_score;
double images = result.image_detection_score;

// AI-generated report
flx_string report = result.detailed_report;
flx_string differences = result.differences_found;
```

**Environment Variables:**
```bash
OPENAI_API_KEY=sk-...
OPENAI_MODEL=gpt-4-turbo-preview
OPENAI_TEMPERATURE=0.1
LAYOUT_EVAL_COORDINATE_TOLERANCE=5.0
LAYOUT_EVAL_COLOR_TOLERANCE=10.0
```

---

## Implementation Notes

### Property System Internals

**Type Storage:**
- All `flxp_int` properties store as `long long` internally
- SFINAE template magic enables `property == 42` (int literal comparison)
- Non-const access creates defaults automatically
- Const access throws `flx_null_field_exception` when null

**Property Sync System:**
- Properties automatically sync with parent before access
- Nested model_lists sync recursively
- Virtual `operator*` overrides trigger sync in flx_model and flx_model_list

**Reference Counting:**
- `flx_lazy_ptr<T>` provides shared_ptr-like semantics
- Models are lazy-initialized (no allocation until first access)
- Copy constructor attempts to copy but ignores exceptions for const-safety

### PDF Processing Pipeline

**7-Step Extraction Process:**
1. **Load PDF** - PoDoFo `LoadFromBuffer()`
2. **Extract Text/Images** - IMMEDIATELY from original (before any copying)
3. **Create Copy** - Working copy for geometry isolation
4. **Remove Content** - Strip text/image operators from copy
5. **Render to Images** - Clean PDF → PNG for OpenCV
6. **Detect Regions** - Flood-fill with neighbor color comparison
7. **Build Hierarchy** - Recursive containment analysis + content assignment

**Critical Timing:**
- Text extraction MUST be Step 2 (immediately after loading)
- Any serialization/copying before extraction corrupts font data
- Per-page font-cache clearing prevents XObject corruption

### Coordinate Systems

**PDF Coordinates:**
- Origin: Bottom-left corner
- Unit: Points (1/72 inch)
- Y increases upward

**Layout Coordinates:**
- Origin: Top-left corner
- Unit: Pixels (at specified DPI)
- Y increases downward

**Conversion:**
```cpp
#include "documents/pdf/flx_pdf_coords.h"

// PDF points → PNG pixels
double png_coord = flx_coords::pdf_to_png_coord(pdf_coord, dpi);

// Y-axis flip for images
double pdf_y = page_height - layout_y - image_height;
```

### Memory Management

**Lazy Pointers:**
```cpp
flx_lazy_ptr<Type> ptr;  // Not allocated yet

*ptr = value;            // Allocates on first write
Type& ref = *ptr;        // Allocates on first non-const access

const auto& const_ptr = ptr;
// *const_ptr;           // Throws if not allocated (const-correct)

ptr.is_null();           // Check without allocating
```

**Model Lifecycle:**
- Models are value types (copyable)
- Internal storage is reference-counted via flx_lazy_ptr
- Copying models shares underlying data (copy-on-write semantics)
- No manual memory management required

### String Utilities

**flx_string** provides 20+ utility methods:

```cpp
flx_string str = "  Hello World  ";

str.trim();                        // Remove whitespace
str.to_upper();                    // "HELLO WORLD"
str.to_lower();                    // "hello world"
str.starts_with("Hello");          // true
str.ends_with("World");            // true
str.contains("World");             // true
str.replace("World", "There");     // "Hello There"
str.split(' ');                    // vector of tokens
flx_string::join(vec, ", ");       // Join with delimiter

// Search with position
size_t pos = str.find("World");               // Find from beginning
size_t pos2 = str.find("o", 5);               // Find starting at position 5
// Returns std::string::npos if not found

// Validation
str.is_numeric();                  // Check if numeric
str.is_empty();                    // Empty string

// Conversion
str.to_int();                      // Parse integer
str.to_double();                   // Parse double
```

### DateTime System

**flx_datetime** with millisecond precision:

```cpp
#include "utils/flx_datetime.h"

// Current time
auto now = flx_datetime::now();

// From components
auto dt = flx_datetime(2025, 1, 15, 14, 30, 0, 123);  // ms optional

// Parsing
auto parsed = flx_datetime::parse("2025-01-15T14:30:00.123Z");

// Formatting (ISO 8601)
flx_string iso = dt.to_iso_string();  // "2025-01-15T14:30:00.123Z"

// Arithmetic
auto later = now + flx_duration::hours(2);
auto diff = later - now;

// Comparison
if (dt1 < dt2) { /*...*/ }

// DST handling - automatic via tm_isdst = -1
// UTC internal storage, local time input/output conversion
```

---

## Documentation Guidelines (Meta)

### Purpose of CLAUDE.md

This file serves as the **primary onboarding document** for Claude Code sessions. It should enable a new session to:

1. **Quickly orient** themselves in the project (< 2 minutes)
2. **Find specific information** without reading everything
3. **Understand critical rules** to avoid common mistakes
4. **Reference API patterns** during implementation

### Structure Principles

**1. Quick Reference First**
- Most frequent tasks at the top
- Build/test commands immediately accessible
- "Where to Find What" navigation map

**2. Thematic Organization Over Chronology**
- Group by topic, not timeline
- "Current state" not "how we got here"
- Historical context goes in Appendix

**3. Action-Oriented Headings**
- "How to X" not "About X"
- Use imperatives: "Build System" not "Building"
- Make section purposes obvious

**4. Layered Information Depth**
```
Quick Reference (30 seconds)
  ↓
Overview (2 minutes)
  ↓
Architecture Guide (10 minutes)
  ↓
API Reference (when needed)
  ↓
Implementation Notes (deep dive)
  ↓
Appendix (historical/rarely needed)
```

### Content Guidelines

**DO:**
- ✅ **File paths with line numbers**: `flx_pdf_sio.cpp:202`
- ✅ **Working code examples**: Show actual usage patterns
- ✅ **Direct links**: "See `tests/README.md`" not "see test documentation"
- ✅ **Critical rules prominently**: ⚠️ warnings for dangerous patterns
- ✅ **Current capabilities**: What works NOW
- ✅ **Concrete examples**: Show don't tell
- ✅ **Navigation aids**: Tables, lists, tree structures
- ✅ **Search-friendly**: Use consistent terminology

**DON'T:**
- ❌ **Long historical narratives**: "We tried X but it failed because Y"
- ❌ **Obvious information**: "This file contains the code for X"
- ❌ **Duplicate information**: Say it once, reference it elsewhere
- ❌ **Vague rules**: "Be careful with pointers" → specify WHAT to avoid
- ❌ **Outdated timestamps**: Remove "As of September 2025" if it's still current
- ❌ **Implementation TODOs**: Move to issues, not documentation
- ❌ **Verbose explanations**: Get to the point quickly

### Section Guidelines

**Quick Reference**
- Commands that are run multiple times per day
- File navigation map (X → file Y)
- No explanations, just facts

**Project Overview**
- 2-3 paragraph summary
- Current capabilities (production-ready features)
- Technology stack table

**Architecture Guide**
- Conceptual understanding
- Diagrams (ASCII) showing relationships
- Key design principles

**File Structure & Navigation**
- Tree structure showing directory layout
- Brief (1 line) description per file
- Group related files together

**Development Guidelines**
- Critical rules (NEVER/ALWAYS) prominently
- Testing commands
- Build process
- Event-driven rules for maintenance

**API Reference**
- Organized by framework/library
- Working code examples
- Common patterns
- Error handling

**Implementation Notes**
- Deep technical details
- Memory management specifics
- Coordinate systems
- Framework internals

**Appendix**
- Resolved issues (historical)
- Deprecated approaches
- Debugging case studies
- Reference only when investigating similar issues

### Maintenance Guidelines

**When completing major features:**
```
1. Update relevant section (don't create new ones)
2. Add to "Current Capabilities" if production-ready
3. Update API Reference with new patterns
4. Move old approach to Appendix if replaced
```

**When learning new APIs:**
```
1. Document in API Reference section
2. Include working code examples
3. Note gotchas and common mistakes
4. Reference official docs for details
```

**When file structure changes:**
```
1. Update "File Structure & Navigation"
2. Update "Where to Find What" in Quick Reference
3. Update CMakeLists.txt documentation
```

**Quarterly review:**
```
1. Remove outdated information
2. Consolidate redundant sections
3. Update technology versions
4. Verify all file paths still valid
5. Check that "Quick Reference" matches current workflow
```

### Anti-Patterns to Avoid

**❌ The "History Book"**
```markdown
## PDF Text Extraction

We first tried using PoDoFo's ExtractTextTo() but it had memory issues.
Then we tried implementing our own parser but fonts were corrupted.
Finally we copied PoDoFo's internal implementation which works.
```

**✅ Better:**
```markdown
## PDF Text Extraction

**File:** `documents/pdf/flx_pdf_text_extractor.h`

Custom implementation based on PoDoFo's internal text extraction:
- Complete font-encoding support
- Works around font corruption issues
- See Appendix A for historical context
```

---

**❌ The "Obvious Documentation"**
```markdown
## flx_model.h

This file contains the flx_model class which is used for models.
```

**✅ Better:**
```markdown
## flx_model Framework

**Property system & variant-based models**

```cpp
class MyModel : public flx_model {
    flxp_int(id);        // Declare properties with macros
    flxp_string(name);   // Work like normal variables
};
```
```

---

**❌ The "Redundant Explanation"**
```markdown
PoDoFo uses PdfPainter to paint PDFs. The painter paints on a canvas...
```

**✅ Better:**
```markdown
```cpp
PdfPainter painter;
painter.SetCanvas(page);
painter.DrawText("Hello", x, y);
```
```

---

**❌ The "Vague Warning"**
```markdown
Be careful with const in the model system.
```

**✅ Better:**
```markdown
**❌ NEVER use `const_cast`**
- Framework is const-incompatible
- Leads to undefined behavior and segfaults
- **Solution**: Accept non-const references, refactor API
```

### Documentation Checklist

Before committing major documentation updates:

- [ ] Quick Reference still matches daily workflow?
- [ ] All file paths verified to exist?
- [ ] Code examples tested and working?
- [ ] No duplicate information across sections?
- [ ] Historical content moved to Appendix?
- [ ] Critical rules prominently marked?
- [ ] Search-friendly terminology consistent?
- [ ] Navigation aids (tables/trees) up to date?
- [ ] Removed outdated timestamps?
- [ ] Each section serves clear purpose?

---

## Appendix A: Resolved Issues (Historical Reference)

### XObject Text Extraction Fix (October 2025)

**Problem:** Only 69 of 75 texts extracted on page 5 (BTS 5070 PDF)

**Root Cause:** Static font cache contained stale `PdfFont*` pointers from previous pages

**Solution:** Per-page font-cache clearing in `flx_pdf_text_extractor::clear_font_cache()`

**Location:** `flx_pdf_sio.cpp:202`

**Result:** ✅ All 75 texts now extracted correctly

**Details:** See `DEBUG_PH_WERT_PROBLEM.md` for complete debugging protocol

### Font Corruption Prevention

**Problem:** Font data corrupted after PDF serialization/reloading

**Root Cause:** Font objects and encoding tables become invalid after PDF copy operations

**Solution:** Extract text IMMEDIATELY after PDF loading, before any modifications

**Architecture Change:** Text extraction moved to Step 2 of `parse()` method (before creating PDF copies)

### PoDoFo Integration Completion

**Achievement:** Copied entire 1400+ line `PdfPage_TextExtraction.cpp` from PoDoFo into custom `flx_pdf_text_extractor`

**Features:**
- Complete font-encoding support (CMap tables, composite fonts)
- Text matrix transformations with CTM
- All PDF text operators (BT/ET, Tf, Tj/TJ/'/'')
- StatefulString processing with glyph positions
- Hierarchical XObject support

**Key Classes Integrated:**
```cpp
struct TextState { Matrix T_rm, CTM, T_m, T_lm; PdfTextState PdfState; };
class StatefulString { string String; vector<double> Lengths; };
struct ExtractionContext { TextStateStack States; StringChunkList Chunks; };
```

### Segfault Resolution

**Problem:** Segfaults in `TextState::ScanString()` during text extraction

**Root Cause:** Null `PdfFont*` pointers not checked before dereferencing

**Solution:** Defensive nullptr checks in all font-dependent methods

**Code Pattern:**
```cpp
if (PdfState.Font == nullptr) {
    // Skip or use default behavior
    return;
}
// Safe to use PdfState.Font->...
```

---

## Additional Resources

- **PoDoFo Documentation**: `documents/pdf/podofo-master/README.md`
- **Catch2 Documentation**: https://github.com/catchorg/Catch2
- **OpenCV Documentation**: https://docs.opencv.org/
- **nlohmann/json**: `api/json/json.hpp` (header-only, self-documented)

---

**Last Updated:** November 2025
**Status:** Production-ready, actively maintained
**Working Directory:** `/home/fenno/Projects/flucture`
