# Exception-Based Error Handling Concept for flucture

**Version:** 1.0
**Status:** Design Phase (Not Implemented)
**Date:** 2025-01-11

---

## Executive Summary

This document proposes a comprehensive exception-based error handling system for `db_repository` and `flx_model`, replacing the current `bool + last_error_` pattern with type-safe, structured exceptions.

**Current State:**
- ✅ `flx_null_field_exception` exists (const property access)
- ❌ All db_repository operations return `bool`
- ❌ Error details stored in `last_error_` string
- ❌ No distinction between error categories

**Proposed State:**
- ✅ Type-safe exception hierarchy
- ✅ Automatic error propagation
- ✅ Structured error information (SQL, context)
- ✅ Clear separation of concerns

---

## Error Taxonomy

### Category 1: Connection Errors
**When:** Database connection is unavailable, lost, or misconfigured

| Error | Cause | Recovery |
|-------|-------|----------|
| `db_connection_error` | Connection is NULL or not connected | Reconnect, check credentials |
| `db_connection_lost` | Connection dropped during operation | Retry with exponential backoff |
| `db_timeout_error` | Query exceeded timeout | Increase timeout, optimize query |

### Category 2: Query Errors
**When:** SQL operations fail due to syntax, constraints, or database state

| Error | Cause | Recovery |
|-------|-------|----------|
| `db_query_error` | Generic SQL execution failure | Check SQL syntax, inspect query->get_last_error() |
| `db_prepare_error` | Query preparation failed | Validate SQL template |
| `db_constraint_violation` | FK, PK, UNIQUE, CHECK constraint violated | Fix data or relax constraints |
| `db_foreign_key_violation` | FK constraint violated (subclass of constraint) | Ensure referenced record exists |
| `db_unique_violation` | UNIQUE constraint violated | Check for duplicates |

### Category 3: Model State Errors
**When:** Model is in invalid state for operation

| Error | Cause | Recovery |
|-------|-------|----------|
| `db_null_id_error` | ID field is NULL when required (update/delete) | Create record first |
| `db_record_not_found` | find_by_id returns no results | Check if ID exists, handle gracefully |
| `db_invalid_model_error` | Model has no metadata or invalid structure | Add column metadata |
| `db_no_table_name_error` | Cannot extract table name from model | Add primary_key metadata |

### Category 4: Metadata Errors
**When:** Model metadata is missing or malformed

| Error | Cause | Recovery |
|-------|-------|----------|
| `db_no_fields_error` | No properties with column metadata | Add column metadata to properties |
| `db_metadata_error` | Metadata is malformed or contradictory | Fix metadata definition |

### Category 5: Hierarchical Errors
**When:** Nested model operations fail

| Error | Cause | Recovery |
|-------|-------|----------|
| `db_nested_save_error` | Failed to save child objects | Check child model constraints |
| `db_nested_load_error` | Failed to load child objects | Check FK relationships |
| `db_cascade_error` | CASCADE DELETE failed unexpectedly | Check database CASCADE rules |

### Category 6: Schema Errors
**When:** Database schema operations fail

| Error | Cause | Recovery |
|-------|-------|----------|
| `db_table_not_found` | Table doesn't exist | Call ensure_structures() |
| `db_schema_error` | CREATE/ALTER TABLE failed | Check permissions, SQL syntax |
| `db_migration_error` | Auto-migration failed | Manual schema update |

---

## Exception Hierarchy

```cpp
// Base exception for all database errors
class db_exception : public std::exception {
    flx_string message_;
    flx_string sql_;           // The SQL that caused the error (if applicable)
    flx_string table_name_;    // Table involved (if applicable)
    flx_string last_db_error_; // Raw database error from query->get_last_error()

public:
    db_exception(const flx_string& msg);
    db_exception(const flx_string& msg, const flx_string& sql);
    db_exception(const flx_string& msg, const flx_string& sql, const flx_string& db_error);

    const char* what() const noexcept override;
    const flx_string& get_message() const;
    const flx_string& get_sql() const;
    const flx_string& get_table_name() const;
    const flx_string& get_database_error() const;

    void set_table_name(const flx_string& table);
};

// ============================================================================
// CONNECTION ERRORS
// ============================================================================

class db_connection_error : public db_exception {
public:
    db_connection_error(const flx_string& msg);
};

class db_connection_lost : public db_connection_error {
public:
    db_connection_lost();
};

class db_timeout_error : public db_connection_error {
public:
    db_timeout_error(const flx_string& sql);
};

// ============================================================================
// QUERY ERRORS
// ============================================================================

class db_query_error : public db_exception {
public:
    db_query_error(const flx_string& msg, const flx_string& sql, const flx_string& db_error);
};

class db_prepare_error : public db_query_error {
public:
    db_prepare_error(const flx_string& sql, const flx_string& db_error);
};

class db_constraint_violation : public db_query_error {
protected:
    flx_string constraint_name_;
public:
    db_constraint_violation(const flx_string& msg, const flx_string& sql,
                           const flx_string& db_error, const flx_string& constraint);
    const flx_string& get_constraint_name() const;
};

class db_foreign_key_violation : public db_constraint_violation {
    flx_string referenced_table_;
    flx_string foreign_key_column_;
public:
    db_foreign_key_violation(const flx_string& table, const flx_string& fk_column,
                            const flx_string& referenced_table,
                            const flx_string& sql, const flx_string& db_error);
    const flx_string& get_referenced_table() const;
    const flx_string& get_foreign_key_column() const;
};

class db_unique_violation : public db_constraint_violation {
    flx_string column_name_;
    flx_variant duplicate_value_;
public:
    db_unique_violation(const flx_string& table, const flx_string& column,
                       const flx_variant& value, const flx_string& sql,
                       const flx_string& db_error);
    const flx_string& get_column_name() const;
    const flx_variant& get_duplicate_value() const;
};

// ============================================================================
// MODEL STATE ERRORS
// ============================================================================

class db_model_error : public db_exception {
public:
    db_model_error(const flx_string& msg);
    db_model_error(const flx_string& msg, const flx_string& table);
};

class db_null_id_error : public db_model_error {
    flx_string operation_; // "update" or "delete"
public:
    db_null_id_error(const flx_string& operation, const flx_string& table);
    const flx_string& get_operation() const;
};

class db_record_not_found : public db_model_error {
    long long id_;
public:
    db_record_not_found(const flx_string& table, long long id);
    long long get_id() const;
};

class db_invalid_model_error : public db_model_error {
public:
    db_invalid_model_error(const flx_string& reason);
};

class db_no_table_name_error : public db_invalid_model_error {
public:
    db_no_table_name_error();
};

// ============================================================================
// METADATA ERRORS
// ============================================================================

class db_metadata_error : public db_exception {
public:
    db_metadata_error(const flx_string& msg);
};

class db_no_fields_error : public db_metadata_error {
public:
    db_no_fields_error(const flx_string& table);
};

// ============================================================================
// HIERARCHICAL ERRORS
// ============================================================================

class db_hierarchical_error : public db_exception {
    flx_string parent_table_;
    flx_string child_table_;
public:
    db_hierarchical_error(const flx_string& msg, const flx_string& parent,
                         const flx_string& child);
    const flx_string& get_parent_table() const;
    const flx_string& get_child_table() const;
};

class db_nested_save_error : public db_hierarchical_error {
public:
    db_nested_save_error(const flx_string& parent_table, const flx_string& child_table,
                        const flx_string& reason);
};

class db_nested_load_error : public db_hierarchical_error {
public:
    db_nested_load_error(const flx_string& parent_table, const flx_string& child_table,
                        const flx_string& reason);
};

class db_cascade_error : public db_hierarchical_error {
public:
    db_cascade_error(const flx_string& parent_table, const flx_string& child_table);
};

// ============================================================================
// SCHEMA ERRORS
// ============================================================================

class db_schema_error : public db_exception {
public:
    db_schema_error(const flx_string& msg, const flx_string& table);
};

class db_table_not_found : public db_schema_error {
public:
    db_table_not_found(const flx_string& table);
};

class db_migration_error : public db_schema_error {
    std::vector<flx_string> failed_columns_;
public:
    db_migration_error(const flx_string& table, const std::vector<flx_string>& failed_cols);
    const std::vector<flx_string>& get_failed_columns() const;
};
```

---

## API Changes

### Before (Current)
```cpp
// Boolean return + last_error_ string
test_product product;
bool success = repo.create(product);
if (!success) {
    std::cerr << "Error: " << repo.get_last_error() << std::endl;
    return;
}
```

### After (Proposed)
```cpp
// Exception-based
test_product product;
try {
    repo.create(product);  // void return
} catch (const db_connection_error& e) {
    std::cerr << "Connection failed: " << e.what() << std::endl;
    reconnect();
} catch (const db_constraint_violation& e) {
    std::cerr << "Constraint violated: " << e.get_constraint_name() << std::endl;
    std::cerr << "SQL: " << e.get_sql() << std::endl;
} catch (const db_exception& e) {
    std::cerr << "Database error: " << e.what() << std::endl;
}
```

---

## Method Signature Changes

### db_repository Methods

**Current:**
```cpp
bool create(flx_model& model);
bool update(flx_model& model);
bool remove(flx_model& model);
bool find_by_id(long long id, flx_model& model);
bool find_all(flx_list& results);
bool find_where(const flx_string& condition, flx_list& results);
bool search(const db_search_criteria& criteria, flx_list& results);
bool table_exists(flx_model& model);
bool create_table(flx_model& model);
bool migrate_table(flx_model& model);
bool ensure_structures(flx_model& model);
```

**Proposed:**
```cpp
void create(flx_model& model);  // throws db_exception
void update(flx_model& model);  // throws db_null_id_error, db_exception
void remove(flx_model& model);  // throws db_null_id_error, db_exception
void find_by_id(long long id, flx_model& model);  // throws db_record_not_found
void find_all(flx_list& results);  // throws db_exception
void find_where(const flx_string& condition, flx_list& results);  // throws db_exception
void search(const db_search_criteria& criteria, flx_list& results);  // throws db_exception

// Schema operations - these MAY still return bool for idempotent checks
bool table_exists(flx_model& model);  // No throw - just checks
void create_table(flx_model& model);  // throws db_schema_error
void migrate_table(flx_model& model);  // throws db_migration_error
void ensure_structures(flx_model& model);  // throws db_schema_error
```

---

## Error Detection Strategy

### Constraint Violation Detection
Parse PostgreSQL error codes and messages:

```cpp
// PostgreSQL SQLSTATE codes
23503 - foreign_key_violation
23505 - unique_violation
23514 - check_violation
23000 - integrity_constraint_violation (generic)

// Detection pattern
if (db_error.contains("violates foreign key constraint")) {
    throw db_foreign_key_violation(...);
}
if (db_error.contains("duplicate key value")) {
    throw db_unique_violation(...);
}
```

### Connection Loss Detection
```cpp
// PostgreSQL connection loss indicators
if (!connection_->is_connected()) {
    throw db_connection_lost();
}
```

### Null ID Detection
```cpp
// Check before update/delete
if (model[id_column_].is_null()) {
    throw db_null_id_error("update", extract_table_name(model));
}
```

---

## Migration Strategy

### Phase 1: Add Exception Classes (No Breaking Changes)
- Create all exception classes in `api/db/db_exceptions.h`
- Add to CMakeLists.txt
- No changes to db_repository yet

### Phase 2: Hybrid Mode (Both Patterns Supported)
- Add `void` variants of methods: `create_or_throw()`, `update_or_throw()`
- Old `bool` methods call new methods with try/catch
- Tests can use either pattern

```cpp
// Hybrid implementation
void db_repository::create_or_throw(flx_model& model) {
    if (!connection_ || !connection_->is_connected()) {
        throw db_connection_error("Not connected to database");
    }
    // ... rest of implementation with exceptions
}

bool db_repository::create(flx_model& model) {
    try {
        create_or_throw(model);
        return true;
    } catch (const db_exception& e) {
        last_error_ = e.what();
        return false;
    }
}
```

### Phase 3: Full Migration
- Remove `bool` methods
- Remove `last_error_` member
- Remove `get_last_error()` method
- Update all tests to use exceptions
- Update CLAUDE.md with new patterns

---

## Test-Driven Design

### Test File Structure
```
tests/db_repository/errors/
├── test_connection_errors.cpp      # Connection failures
├── test_constraint_violations.cpp  # FK, UNIQUE violations
├── test_model_state_errors.cpp     # NULL ID, not found
├── test_metadata_errors.cpp        # Missing metadata
├── test_hierarchical_errors.cpp    # Nested save/load failures
└── test_schema_errors.cpp          # Table creation, migration
```

### Example Test Pattern
```cpp
SCENARIO("Update with NULL ID throws db_null_id_error", "[repo][error][unit][db]") {
    GIVEN("A product without ID") {
        test_product product;
        product.name = "Test";
        // ID is NULL

        WHEN("Attempting to update") {
            THEN("Throws db_null_id_error") {
                REQUIRE_THROWS_AS(repo.update(product), db_null_id_error);
            }

            AND_THEN("Exception contains correct operation") {
                try {
                    repo.update(product);
                    FAIL("Should have thrown");
                } catch (const db_null_id_error& e) {
                    REQUIRE(e.get_operation() == "update");
                    REQUIRE(e.get_table_name() == "test_simple_products");
                }
            }
        }
    }
}
```

---

## Benefits

### Type Safety
❌ Before: `if (!success) { /* what failed? */ }`
✅ After: `catch (const db_foreign_key_violation& e) { /* clear failure reason */ }`

### Error Context
❌ Before: `"Failed to execute insert: <cryptic postgres message>"`
✅ After: Full structured exception with SQL, table, constraint, value

### Automatic Propagation
❌ Before: Check every `bool`, propagate manually
✅ After: Exceptions propagate automatically through call stack

### Cleaner Code
❌ Before:
```cpp
if (!repo.create(parent)) return false;
if (!repo.create(child1)) return false;
if (!repo.create(child2)) return false;
return true;
```

✅ After:
```cpp
repo.create(parent);
repo.create(child1);
repo.create(child2);
// If any fails, exception propagates
```

### Better Testing
❌ Before: `REQUIRE(success == false); REQUIRE(error.contains("..."));`
✅ After: `REQUIRE_THROWS_AS(operation, db_foreign_key_violation);`

---

## Open Questions

1. **Should find_by_id throw or return bool?**
   - Option A: Throw `db_record_not_found` (exceptional condition)
   - Option B: Return bool (expected case for lookups)
   - **Recommendation:** Throw - consistent with exceptions, not found IS exceptional

2. **Should table_exists() throw?**
   - **Recommendation:** No - it's a query, not an assertion

3. **Backwards compatibility period?**
   - **Recommendation:** 1 release cycle with hybrid mode, then remove bool methods

4. **Performance impact of exceptions?**
   - **Analysis:** Exceptions are zero-cost when not thrown (modern C++)
   - Only impact is when error actually occurs (already failing)

---

## Implementation Checklist

### Phase 1: Exception Classes
- [ ] Create `api/db/db_exceptions.h`
- [ ] Create `api/db/db_exceptions.cpp`
- [ ] Implement all exception classes
- [ ] Add to CMakeLists.txt
- [ ] Write unit tests for exception constructors/getters

### Phase 2: Hybrid Mode
- [ ] Add `*_or_throw()` methods to db_repository
- [ ] Implement exception detection logic
- [ ] Keep existing `bool` methods as wrappers
- [ ] Write tests for new methods
- [ ] Update TESTING_GUIDELINES.md

### Phase 3: Full Migration
- [ ] Deprecate `bool` methods (add `[[deprecated]]`)
- [ ] Update all internal code to use exceptions
- [ ] Update all tests to use exceptions
- [ ] Remove `last_error_` member
- [ ] Remove `get_last_error()` method
- [ ] Update CLAUDE.md

---

## Conclusion

Exception-based error handling provides:
- ✅ Type safety
- ✅ Structured error information
- ✅ Automatic propagation
- ✅ Cleaner code
- ✅ Better testability

**Recommendation:** Proceed with Phase 1 (exception classes) immediately, then gradual migration.
