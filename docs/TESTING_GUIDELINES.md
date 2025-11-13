# Flucture Testing Guidelines

**Effective, BDD-based testing strategy for production-quality software**

Version: 1.0
Last Updated: 2025-01-10

---

## Table of Contents

1. [Core Principles](#core-principles)
2. [Tag System](#tag-system)
3. [Test Isolation](#test-isolation)
4. [BDD Structure](#bdd-structure)
5. [Contract-Based Testing](#contract-based-testing)
6. [Boundary Testing](#boundary-testing)
7. [Assertions](#assertions)
8. [Test Organization](#test-organization)
9. [Common Patterns](#common-patterns)
10. [Anti-Patterns](#anti-patterns)

---

## Core Principles

### 1. Tests are Specifications

**Every test name must be a complete specification:**

```cpp
// ❌ BAD: Vague, doesn't specify what's being tested
SCENARIO("Test CRUD operations")

// ✅ GOOD: Precise specification of behavior
SCENARIO("Repository assigns auto-incrementing IDs on create")
```

**Why?** When a test fails, the name tells you exactly what contract was violated.

### 2. One Assertion per THEN

**Each THEN should test exactly one thing:**

```cpp
// ❌ BAD: Multiple concerns in one THEN
THEN("Product is saved correctly") {
    REQUIRE(found == true);
    REQUIRE(loaded.id == original.id);
    REQUIRE(loaded.name == "Laptop");
    REQUIRE(loaded.price == 1299.99);
}

// ✅ GOOD: Separate concerns
THEN("Record exists") {
    REQUIRE(found == true);
}

AND_THEN("ID is preserved") {
    REQUIRE(loaded.id == original.id);
}

AND_THEN("Name is preserved") {
    REQUIRE(loaded.name == "Laptop");
}

AND_THEN("Price is preserved within epsilon") {
    REQUIRE(loaded.price == Catch::Approx(1299.99).epsilon(0.01));
}
```

**Why?** Pinpoints exact failure location. No need to debug which assertion failed.

### 3. Tests Must Be Isolated

**Each test must be independent and repeatable:**

```cpp
// ❌ BAD: Relies on data from other tests
REQUIRE(results.size() >= 1); // "May contain data from other tests"

// ✅ GOOD: Exact assertions on isolated data
db_test_cleanup cleanup(&conn, "my_test");
test_product p;
p.name = cleanup.prefix() + "Laptop"; // Unique per test
repo.create(p);
cleanup.track_id(p.id); // Auto-cleanup

REQUIRE(results.size() == 1); // Exact count
```

**Why?** Tests can run in any order, in parallel, and produce consistent results.

### 4. Test Behavior, Not Implementation

**Focus on contracts (what), not internals (how):**

```cpp
// ❌ BAD: Tests implementation detail
SCENARIO("scan_fields() includes properties with column metadata")

// ✅ GOOD: Tests observable behavior
SCENARIO("Repository persists properties that have column metadata")
SCENARIO("Repository ignores properties without column metadata")
```

**Why?** Internal refactoring shouldn't break tests. Only breaking the public API should.

---

## Tag System

### Tag Dimensions

Use multiple orthogonal tag dimensions for flexible filtering:

**1. COMPONENT/PROJECT (what module):**
- `[repo]` - Database repository tests (db_repository module)
- `[model]` - Model system tests (flx_model)
- `[pdf]` - PDF processing tests
- `[layout]` - Layout system tests
- `[api]` - REST API tests

**2. FEATURE (what):**
- `[crud]` - Create, Read, Update, Delete operations
- `[basic]` - Core functionality that must work
- `[search]` - Search and query operations
- `[hierarchy]` - Nested models (1:1, 1:N)
- `[metadata]` - Metadata scanning and automation
- `[null]` - NULL value handling
- `[boundary]` - Edge cases (empty, min, max)
- `[jsonb]` - JSONB types (maps, vectors)
- `[unicode]` - Special characters and encodings
- `[error]` - Error handling and recovery
- `[cascade]` - CASCADE DELETE behavior

**3. SPEED (how fast):**
- `[unit]` - Fast (<1 second)
- `[integration]` - Medium (1-5 seconds)
- `[slow]` - Slow (>5 seconds)

**4. DEPENDENCIES (what's needed):**
- `[db]` - Requires database connection
- `[api]` - Requires external API
- `[no-external]` - No external dependencies

**5. CRITICALITY (how important):**
- `[smoke]` - Must pass for every commit (CI)
- `[regression]` - Prevents known bugs from returning

### Tag Usage Examples

```cpp
// Basic CRUD - must always work
SCENARIO("Repository creates records with auto-assigned IDs",
         "[repo][crud][basic][smoke][unit][db]")

// NULL handling edge case
SCENARIO("Repository preserves NULL distinctly from empty string",
         "[repo][crud][null][boundary][unit][db]")

// Performance-critical test
SCENARIO("Hierarchical load completes within 1 second for 100 children",
         "[repo][hierarchy][slow][db]")

// Regression test for specific bug
SCENARIO("Update does not cascade delete unmodified children",
         "[repo][crud][hierarchy][regression][integration][db]")
```

### Running Tests by Tag

```bash
# All db_repository tests
./flucture_tests "[repo]"

# All CRUD tests (across all modules)
./flucture_tests "[crud]"

# Only db_repository CRUD tests
./flucture_tests "[repo][crud]"

# Only smoke tests (CI)
./flucture_tests "[smoke]"

# All fast tests (pre-commit)
./flucture_tests "[unit]"

# DB tests excluding slow ones
./flucture_tests "[db]~[slow]"

# All boundary and error tests
./flucture_tests "[boundary],[error]"

# Specific feature in specific module
./flucture_tests "[repo][hierarchy]"

# All search operator tests
./flucture_tests "[repo][search][operators]"
```

---

## Test Isolation

### The Problem

**Without isolation, tests become unreliable:**

```cpp
// ❌ PROBLEM: Tests pollute each other
TEST_CASE("Create product A") {
    repo.create(productA); // ID=1
}

TEST_CASE("Count all products") {
    auto count = repo.count();
    REQUIRE(count == 1); // ❌ FAILS if test order changes!
}
```

### Solution: Test Fixtures with Cleanup

**Use RAII-based cleanup:**

```cpp
class db_test_cleanup {
private:
    pg_connection* conn_;
    flx_string test_prefix_;
    std::vector<long long> created_ids_;

public:
    db_test_cleanup(pg_connection* conn, const flx_string& test_name)
        : conn_(conn) {
        // Generate unique prefix: testname_timestamp_
        auto timestamp = std::chrono::high_resolution_clock::now()
                            .time_since_epoch().count();
        test_prefix_ = test_name + "_" + std::to_string(timestamp) + "_";
    }

    ~db_test_cleanup() {
        // Delete all tracked records
        for (long long id : created_ids_) {
            auto query = conn_->create_query();
            query->prepare("DELETE FROM table WHERE id = :id");
            query->bind("id", flx_variant(id));
            query->execute();
        }
    }

    flx_string prefix() const { return test_prefix_; }
    void track_id(long long id) { created_ids_.push_back(id); }
};
```

**Usage:**

```cpp
SCENARIO("Repository creates unique records") {
    GIVEN("A database connection") {
        pg_connection& conn = get_test_connection();
        db_test_cleanup cleanup(&conn, "create_test");

        WHEN("Creating a product") {
            test_product p;
            p.name = cleanup.prefix() + "Laptop"; // Unique name
            repo.create(p);
            cleanup.track_id(p.id); // Track for cleanup

            THEN("Product has unique ID") {
                REQUIRE(p.id > 0);
            }
        }
        // cleanup destructor runs here - deletes test data
    }
}
```

### Alternatives: Transactions

**For databases supporting transactions:**

```cpp
class db_transaction_fixture {
public:
    void setup() {
        conn.execute("BEGIN");
    }

    void teardown() {
        conn.execute("ROLLBACK"); // Undo everything
    }
};
```

---

## BDD Structure

### GIVEN-WHEN-THEN Pattern

**Structure every test with clear phases:**

```cpp
SCENARIO("Repository updates existing records without changing ID") {
    GIVEN("A database with an existing product") {
        // SETUP PHASE
        db_test_cleanup cleanup(&conn, "update_test");
        test_product original;
        original.name = cleanup.prefix() + "Laptop";
        original.price = 1299.99;
        repo.create(original);
        cleanup.track_id(original.id);

        long long original_id = original.id;

        WHEN("Updating the product's properties") {
            // ACTION PHASE
            original.name = cleanup.prefix() + "Gaming Laptop";
            original.price = 1599.99;
            bool updated = repo.update(original);

            THEN("Update succeeds") {
                // VERIFICATION PHASE
                REQUIRE(updated == true);
            }

            AND_THEN("ID remains unchanged") {
                REQUIRE(original.id == original_id);
            }

            AND_WHEN("Loading the updated product") {
                test_product loaded;
                repo.find_by_id(original_id, loaded);

                THEN("All fields reflect the changes") {
                    REQUIRE(loaded.name == cleanup.prefix() + "Gaming Laptop");
                    REQUIRE(loaded.price == Catch::Approx(1599.99).epsilon(0.01));
                }
            }
        }
    }
}
```

### Nesting Guidelines

**Maximum 3 levels deep:**

```
SCENARIO
  └─ GIVEN (setup)
       └─ WHEN (action)
            └─ THEN (verify)
                 └─ AND_THEN (additional checks)
                 └─ AND_WHEN (follow-up action)
                      └─ THEN (verify follow-up)
```

**Deeper nesting → Split into separate scenarios.**

---

## Contract-Based Testing

### Define System Contracts

**What MUST the system guarantee?**

**Example: Data Integrity Contract**

```
CONTRACT: Round-Trip Integrity
  ∀ model M: save(M) ⇒ load(M.id) ≡ M

GUARANTEES:
  1. All properties preserved exactly
  2. NULL values remain NULL
  3. Types are preserved (int stays int)
  4. IDs are auto-assigned and positive
```

**Example: Hierarchical Consistency Contract**

```
CONTRACT: Parent-Child Consistency
  ∀ parent P with children C:
    save(P) ⇒ load(P).children.size() == C.size()
    ∧ ∀c ∈ C: c.foreign_key == P.id
    ∧ remove(P) ⇒ ¬∃c ∈ C  (CASCADE DELETE)
```

### Test Each Contract Systematically

**One scenario per guarantee:**

```cpp
// Contract: Round-Trip Integrity
// Guarantee 1: String properties preserved
SCENARIO("Repository round-trips string properties exactly",
         "[repo][crud][basic][unit][db]")

// Guarantee 2: NULL values preserved
SCENARIO("Repository preserves NULL values distinctly from defaults",
         "[repo][crud][null][unit][db]")

// Guarantee 3: Types preserved
SCENARIO("Repository preserves integer types without conversion",
         "[repo][crud][basic][unit][db]")

// Guarantee 4: IDs auto-assigned
SCENARIO("Repository assigns positive auto-incrementing IDs",
         "[repo][crud][basic][smoke][unit][db]")
```

---

## Boundary Testing

### Systematic Edge Case Coverage

**Test the boundaries of every data dimension:**

| Dimension | Min | Typical | Max | Invalid |
|-----------|-----|---------|-----|---------|
| String length | "" | "Laptop" | 255 chars | NULL |
| Integer | LLONG_MIN | 50 | LLONG_MAX | NULL |
| Double | -DBL_MAX | 1299.99 | DBL_MAX | NaN, Inf |
| List size | 0 | 5 | 1000 | NULL vector |
| Nesting depth | 1 | 3 | 10 | Circular ref |
| Unicode | ASCII | "Über" | "東京🚀" | Invalid UTF-8 |

### Example: String Boundaries

```cpp
SCENARIO("Repository handles empty strings distinctly from NULL",
         "[repo][crud][boundary][unit][db]") {
    GIVEN("Two products: one empty string, one NULL") {
        test_product p_empty, p_null;
        p_empty.name = "";  // Empty
        // p_null.name stays NULL

        repo.create(p_empty);
        repo.create(p_null);

        WHEN("Loading both") {
            test_product loaded_empty, loaded_null;
            repo.find_by_id(p_empty.id, loaded_empty);
            repo.find_by_id(p_null.id, loaded_null);

            THEN("Empty string is preserved") {
                REQUIRE(!loaded_empty.name.is_null());
                REQUIRE(loaded_empty.name == "");
            }

            THEN("NULL is preserved") {
                REQUIRE(loaded_null.name.is_null());
            }
        }
    }
}

SCENARIO("Repository handles maximum length strings",
         "[repo][crud][boundary][unit][db]") {
    GIVEN("A product with 255-character name") {
        test_product p;
        p.name = std::string(255, 'x'); // Max length
        repo.create(p);

        WHEN("Loading the product") {
            test_product loaded;
            repo.find_by_id(p.id, loaded);

            THEN("Full string is preserved") {
                REQUIRE(loaded.name.value().length() == 255);
            }
        }
    }
}
```

### Example: Numeric Boundaries

```cpp
SCENARIO("Repository handles integer min/max values",
         "[repo][crud][boundary][unit][db]") {
    GIVEN("Products with extreme integer values") {
        test_product p_min, p_max;
        p_min.stock_quantity = LLONG_MIN;
        p_max.stock_quantity = LLONG_MAX;

        repo.create(p_min);
        repo.create(p_max);

        WHEN("Loading both products") {
            test_product loaded_min, loaded_max;
            repo.find_by_id(p_min.id, loaded_min);
            repo.find_by_id(p_max.id, loaded_max);

            THEN("MIN value is preserved") {
                REQUIRE(loaded_min.stock_quantity == LLONG_MIN);
            }

            THEN("MAX value is preserved") {
                REQUIRE(loaded_max.stock_quantity == LLONG_MAX);
            }
        }
    }
}
```

---

## Assertions

### Exact vs. Vague Assertions

**Always prefer exact assertions:**

```cpp
// ❌ BAD: Vague, hides bugs
REQUIRE(results.size() >= 1); // "May contain data from other tests"

// ✅ GOOD: Exact, catches bugs
REQUIRE(results.size() == 1);
```

### Floating Point Comparisons

**Use epsilon for doubles:**

```cpp
// ❌ BAD: Direct comparison
REQUIRE(loaded.price == 1299.99); // May fail due to rounding

// ✅ GOOD: Epsilon comparison
REQUIRE(loaded.price == Catch::Approx(1299.99).epsilon(0.01));
```

### Boolean Clarity

**Be explicit about true/false:**

```cpp
// ❌ BAD: Implicit truthiness
REQUIRE(created);

// ✅ GOOD: Explicit comparison
REQUIRE(created == true);
```

### NULL Checks

**Check NULL state explicitly:**

```cpp
// ❌ BAD: May hide NULL bugs
REQUIRE(loaded.name == "Laptop");

// ✅ GOOD: Check NULL first
REQUIRE(!loaded.name.is_null());
REQUIRE(loaded.name.value() == "Laptop");
```

---

## Test Organization

### File Structure

**Organize by contract and feature:**

```
tests/
├── db_repository/
│   ├── shared/                     # Common code
│   │   ├── db_test_models.h       # Test model definitions
│   │   ├── db_test_fixtures.h     # Setup/teardown helpers
│   │   └── db_test_helpers.h      # Utility functions
│   │
│   ├── crud/                       # CRUD operations
│   │   ├── test_crud_basic.cpp    # Basic create, read, update, delete
│   │   ├── test_crud_null.cpp     # NULL value handling
│   │   └── test_crud_roundtrip.cpp # Round-trip integrity
│   │
│   ├── search/                     # Search operations
│   │   ├── test_search_basic.cpp
│   │   └── test_search_hierarchical.cpp
│   │
│   ├── hierarchy/                  # Nested models
│   │   ├── test_hierarchy_basic.cpp
│   │   └── test_hierarchy_cascade.cpp
│   │
│   ├── boundaries/                 # Edge cases
│   │   ├── test_empty_values.cpp
│   │   ├── test_numeric_limits.cpp
│   │   └── test_unicode.cpp
│   │
│   ├── errors/                     # Failure modes
│   │   ├── test_not_found.cpp
│   │   ├── test_disconnected.cpp
│   │   └── test_constraint_violations.cpp
│   │
│   └── integration/                # End-to-end workflows
│       ├── test_full_lifecycle.cpp
│       └── test_migration_workflow.cpp
```

### File Naming

**Convention: `test_<feature>_<aspect>.cpp`**

- `test_crud_basic.cpp` - Basic CRUD operations
- `test_crud_null_handling.cpp` - NULL-specific CRUD tests
- `test_search_operators.cpp` - Search operator tests
- `test_hierarchy_cascade_delete.cpp` - CASCADE behavior

### Grouping Related Tests

**Use Catch2 sections for related sub-tests:**

```cpp
SCENARIO("Repository handles boolean properties", "[repo][crud][basic][unit][db]") {
    GIVEN("A repository") {
        db_test_cleanup cleanup(&conn, "bool_test");

        SECTION("True value") {
            test_product p;
            p.active = true;
            repo.create(p);

            test_product loaded;
            repo.find_by_id(p.id, loaded);
            REQUIRE(loaded.active == true);
        }

        SECTION("False value") {
            test_product p;
            p.active = false;
            repo.create(p);

            test_product loaded;
            repo.find_by_id(p.id, loaded);
            REQUIRE(loaded.active == false);
        }
    }
}
```

---

## Common Patterns

### Deep Equality Helper

**For comparing complex models:**

```cpp
bool models_are_equal(const test_product& a, const test_product& b) {
    // ID
    if (a.id.is_null() != b.id.is_null()) return false;
    if (!a.id.is_null() && a.id != b.id) return false;

    // Name
    if (a.name.is_null() != b.name.is_null()) return false;
    if (!a.name.is_null() && a.name != b.name) return false;

    // Price (epsilon for doubles)
    if (a.price.is_null() != b.price.is_null()) return false;
    if (!a.price.is_null()) {
        if (std::abs(a.price.value() - b.price.value()) > 0.01)
            return false;
    }

    // ... other properties

    return true;
}

// Usage:
THEN("Models are deeply equal") {
    REQUIRE(models_are_equal(original, loaded));
}
```

### Random Data Generation

**For property-based testing:**

```cpp
test_product generate_random_product(const std::string& prefix) {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    test_product p;
    p.name = prefix + "_" + random_string(10);
    p.price = random_double(1.0, 10000.0);
    p.stock_quantity = random_int(0, 1000);
    p.active = random_bool();

    return p;
}

// Usage:
SCENARIO("Repository preserves random data") {
    GIVEN("A randomly generated product") {
        auto original = generate_random_product("random_test");
        repo.create(original);

        WHEN("Loading the product") {
            test_product loaded;
            repo.find_by_id(original.id, loaded);

            THEN("All properties match") {
                REQUIRE(models_are_equal(original, loaded));
            }
        }
    }
}
```

### Performance Assertions

**For timing-critical operations:**

```cpp
SCENARIO("Search completes within performance budget",
         "[repo][search][slow][db]") {
    GIVEN("A database with 1000 products") {
        populate_test_data(1000);

        WHEN("Searching with complex criteria") {
            auto start = std::chrono::steady_clock::now();

            auto results = repo.search(criteria);

            auto end = std::chrono::steady_clock::now();
            auto duration_ms = std::chrono::duration_cast<
                std::chrono::milliseconds>(end - start).count();

            THEN("Search completes within 1000ms") {
                REQUIRE(duration_ms < 1000);
            }

            AND_THEN("Results are correct") {
                REQUIRE(results.size() > 0);
            }
        }
    }
}
```

---

## Anti-Patterns

### ❌ Tests That Compare Methods

**Don't write "A vs B" tests:**

```cpp
// ❌ BAD: Compares two methods
SCENARIO("search vs search_hierarchical behavior")
```

**Why bad?** Tests implementation choices, not behavior contracts.

**Fix:** Test each method's contract separately:

```cpp
// ✅ GOOD: Tests method contract
SCENARIO("search returns flat results matching criteria",
         "[repo][search][unit][db]")

SCENARIO("search_hierarchical returns nested results matching criteria",
         "[repo][search][hierarchy][integration][db]")
```

### ❌ Tests With Shared State

**Don't rely on data from other tests:**

```cpp
// ❌ BAD: Depends on test execution order
TEST_CASE("Create product A") {
    repo.create(productA); // ID gets auto-assigned
}

TEST_CASE("Update product A") {
    repo.update(productA); // ❌ productA not in scope!
}
```

**Fix:** Make each test self-contained:

```cpp
// ✅ GOOD: Self-contained
SCENARIO("Repository updates existing products") {
    GIVEN("An existing product") {
        test_product p;
        repo.create(p); // Create in this test

        WHEN("Updating the product") {
            p.name = "Updated";
            repo.update(p);

            THEN("Changes are persisted") {
                // verify
            }
        }
    }
}
```

### ❌ Testing Implementation Details

**Don't test internal methods directly:**

```cpp
// ❌ BAD: Tests internal method
SCENARIO("scan_fields returns correct metadata")

// ❌ BAD: Tests private helper
SCENARIO("build_insert_sql generates correct SQL")
```

**Why bad?** Internal refactoring breaks tests.

**Fix:** Test observable behavior:

```cpp
// ✅ GOOD: Tests public contract
SCENARIO("Repository persists models with metadata-defined columns")

// ✅ GOOD: Tests result, not implementation
SCENARIO("Repository creates records in database")
```

### ❌ Vague Test Names

**Don't use generic names:**

```cpp
// ❌ BAD: Doesn't specify what's tested
SCENARIO("Test repository")
SCENARIO("Check CRUD operations")
SCENARIO("Database test")
```

**Fix:** Be specific:

```cpp
// ✅ GOOD: Precise specification
SCENARIO("Repository assigns auto-incrementing IDs on create")
SCENARIO("Repository preserves NULL values distinctly from defaults")
SCENARIO("Repository cascades delete to child records")
```

### ❌ Multiple Assertions Without Context

**Don't pile assertions without structure:**

```cpp
// ❌ BAD: Which assertion failed?
REQUIRE(a == 1);
REQUIRE(b == 2);
REQUIRE(c == 3);
REQUIRE(d == 4);
```

**Fix:** Use AND_THEN for context:

```cpp
// ✅ GOOD: Clear failure location
THEN("First property is correct") {
    REQUIRE(a == 1);
}

AND_THEN("Second property is correct") {
    REQUIRE(b == 2);
}

AND_THEN("Third property is correct") {
    REQUIRE(c == 3);
}
```

### ❌ Ignoring Cleanup

**Don't leave test data in database:**

```cpp
// ❌ BAD: No cleanup
TEST_CASE("Create product") {
    test_product p;
    repo.create(p); // Left in DB forever
}
```

**Fix:** Use RAII cleanup:

```cpp
// ✅ GOOD: Automatic cleanup
SCENARIO("Repository creates products") {
    GIVEN("A repository") {
        db_test_cleanup cleanup(&conn, "create_test");

        test_product p;
        repo.create(p);
        cleanup.track_id(p.id); // Auto-deleted at end
    }
}
```

---

## Running Tests

### Common Commands

```bash
# All tests
./flucture_tests

# By feature
./flucture_tests "[crud]"
./flucture_tests "[search]"
./flucture_tests "[hierarchy]"

# By speed
./flucture_tests "[unit]"           # Fast only
./flucture_tests "[unit][smoke]"    # Fast + critical
./flucture_tests "~[slow]"          # Exclude slow

# By dependency
./flucture_tests "[db]"
./flucture_tests "[no-external]"

# Specific test
./flucture_tests "Repository assigns auto-incrementing IDs"

# List available tests
./flucture_tests --list-tests

# List tags
./flucture_tests --list-tags
```

### CI Pipeline

**Recommended test stages:**

```yaml
# Stage 1: Smoke tests (must pass)
run: ./flucture_tests "[smoke][unit]"

# Stage 2: All unit tests
run: ./flucture_tests "[unit]"

# Stage 3: Integration tests
run: ./flucture_tests "[integration]"

# Stage 4: Slow tests (nightly)
run: ./flucture_tests "[slow]"
```

---

## Summary Checklist

**Before writing a test, ask:**

- [ ] Is the test name a complete specification?
- [ ] Does each THEN test exactly one thing?
- [ ] Is the test isolated (no shared state)?
- [ ] Are assertions exact (not vague `>=` checks)?
- [ ] Does the test use proper cleanup?
- [ ] Are tags correct and complete?
- [ ] Does the test follow GIVEN-WHEN-THEN structure?
- [ ] Am I testing behavior, not implementation?
- [ ] Would this test catch regressions?
- [ ] Can this test run in parallel with others?

**When a test fails:**

- [ ] Does the test name tell me what contract was broken?
- [ ] Does the failure message pinpoint the exact assertion?
- [ ] Can I reproduce the failure locally?
- [ ] Is the failure deterministic (not flaky)?

---

## References

- **Catch2 Documentation:** https://github.com/catchorg/Catch2
- **BDD Principles:** https://dannorth.net/introducing-bdd/
- **Test Isolation:** https://martinfowler.com/bliki/TestIsolation.html
- **Contract Testing:** https://martinfowler.com/bliki/ContractTest.html

---

**Last Updated:** 2025-01-10
**Maintained by:** Flucture Development Team
**Questions?** See `tests/README.md` or ask in project chat.
