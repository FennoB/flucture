# Flucture Test Suite

## Quick Start

```bash
# Run all fast tests (no DB, no external APIs)
cd build
./flucture_tests "~[db]~[slow]~[integration]~[ai]~[llm]"

# Run only unit tests
./flucture_tests "~[db]~[integration]"

# Run specific test by name
./flucture_tests "flx_datetime"

# List all available tests
./flucture_tests --list-tests

# List all tags
./flucture_tests --list-tags
```

## Test Tags

### Speed & Isolation
- `[unit]` - Fast unit tests with no external dependencies
- `[integration]` - Tests requiring external services (DB, APIs)
- `[slow]` - Tests taking >5 seconds
- `[disabled]` - Currently disabled tests (broken or WIP)

### Requirements
- `[db]` - Requires PostgreSQL connection
- `[ai]` - Requires OpenAI API (gpt-4-turbo)
- `[llm]` - Requires any LLM API
- `[semantic_search]` - Requires both DB and OpenAI API

### Features
- `[pdf]` - PDF rendering/parsing tests
- `[text]` - Text extraction tests
- `[xobject]` - XObject handling tests
- `[coordinates]` - Coordinate transformation tests
- `[evaluator]` - AI layout evaluator tests
- `[e2e]` - End-to-end pipeline tests

## Environment Variables

### Required for specific tests:

**Database tests** (`[db]`):
```bash
export DB_HOST=localhost
export DB_PORT=5432
export DB_NAME=test_db
export DB_USER=test_user
export DB_PASS=test_password
```

**AI/LLM tests** (`[ai]`, `[llm]`, `[evaluator]`):
```bash
export OPENAI_API_KEY="sk-..."
export OPENAI_MODEL="gpt-4-turbo-preview"
```

**Semantic search tests** (`[semantic_search]`):
```bash
# Needs both DB and OpenAI API
source /path/to/.env
export DB_HOST=... DB_PORT=... DB_NAME=... DB_USER=... DB_PASS=...
```

## Common Test Scenarios

### 1. Quick Development Cycle (no external deps)
```bash
./flucture_tests "~[db]~[slow]~[integration]~[ai]~[llm]~[disabled]"
```
**Runs**: Core model tests, datetime, string utilities, basic PDF operations
**Time**: ~5 seconds

### 2. Database Tests Only
```bash
source ../.env
export DB_HOST=localhost DB_PORT=5432 DB_NAME=test_db DB_USER=test DB_PASS=test
./flucture_tests "[db]~[slow]"
```
**Runs**: Repository CRUD, hierarchical operations, query builder
**Time**: ~30 seconds

### 3. All Fast Tests (including DB)
```bash
source ../.env && export DB_HOST=... DB_PORT=... DB_NAME=... DB_USER=... DB_PASS=...
./flucture_tests "~[slow]~[disabled]"
```
**Runs**: Everything except slow integration tests
**Time**: ~1 minute

### 4. AI/LLM Tests
```bash
export OPENAI_API_KEY="sk-..."
./flucture_tests "[ai]"
```
**Runs**: Layout evaluator, embedding generation
**Time**: ~20 seconds (depends on API)

### 5. Complete Test Suite
```bash
source ../.env
export DB_HOST=... DB_PORT=... DB_NAME=... DB_USER=... DB_PASS=...
export OPENAI_API_KEY="sk-..."
./flucture_tests "~[disabled]"
```
**Runs**: All enabled tests
**Time**: ~5 minutes

## Test Scripts

Convenience scripts are available in `../test-scripts/`:

```bash
# Fast unit tests only
../test-scripts/run-unit.sh

# Database tests with test DB
../test-scripts/run-db-tests.sh

# All fast tests
../test-scripts/run-fast.sh

# Full test suite
../test-scripts/run-all.sh
```

## Makefile Targets

From the `build/` directory:

```bash
make test-unit          # Fast unit tests
make test-db            # Database tests
make test-fast          # All fast tests
make test-all           # Complete suite
```

## Troubleshooting

### Tests hang or timeout
- Likely hitting external APIs without proper credentials
- Use `~[slow]~[integration]` to exclude slow tests
- Check that required environment variables are set

### Database connection errors
```
REQUIRE( conn->connect() ) failed
```
- Verify PostgreSQL is running: `pg_isready`
- Check DB credentials in environment variables
- Ensure test database exists

### OpenAI API errors
```
ERROR: OPENAI_API_KEY not set
```
- Export API key: `export OPENAI_API_KEY="sk-..."`
- Source .env file: `source /path/to/.env`

### Segmentation faults
- Usually pointer issues in PDF processing or model operations
- Run with debugger: `gdb --args ./flucture_tests "test name"`
- Check recent code changes to move semantics or property access

## Writing New Tests

### Basic Structure
```cpp
#include <catch2/catch_all.hpp>

SCENARIO("Feature description") {
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

### Tag Your Tests
```cpp
SCENARIO("My test", "[unit]") { ... }              // Fast, no deps
SCENARIO("My test", "[db][slow]") { ... }          // Needs DB, >5sec
SCENARIO("My test", "[integration][ai]") { ... }   // Needs OpenAI API
```

### Best Practices
- Use `[unit]` for tests that run in <1 second with no external deps
- Use `[slow]` for anything taking >5 seconds
- Use `[disabled]` to temporarily skip broken tests (with TODO comment)
- Add `[integration]` for anything touching external services
- Document required ENV variables in test comments

## Test File Organization

```
tests/
├── test_main.cpp                      # Catch2 main runner
├── test_flx_model.cpp                 # [unit] Core model system
├── test_flx_datetime.cpp              # [unit] Date/time utilities
├── test_db_repository.cpp             # [db] Database operations
├── test_db_hierarchical.cpp           # [db][slow] Complex CRUD
├── test_layout_evaluator.cpp          # [ai][evaluator] AI evaluation
├── test_pdf_rendering.cpp             # [pdf] PDF generation
└── test_embedding.cpp                 # [ai][semantic_search] Embeddings
```

## Continuous Integration

Recommended CI pipeline stages:

1. **Fast checks** (every commit): `./flucture_tests "~[db]~[slow]~[integration]"`
2. **DB tests** (every commit): `./flucture_tests "[db]~[slow]"`
3. **Full suite** (pre-merge): `./flucture_tests "~[disabled]"`

Total CI time: ~2 minutes for stages 1+2, ~5 minutes for stage 3
