#include <catch2/catch_all.hpp>
#include "../shared/db_test_fixtures.h"
#include "../../../api/db/db_exceptions.h"

// ============================================================================
// ERROR HANDLING SPECIFICATION
// ============================================================================
//
// This file specifies the EXPECTED exception-based error handling behavior.
// It will NOT compile until exception classes are implemented.
//
// PURPOSE: Test-Driven Design - specify behavior before implementation
//
// IMPLEMENTATION STATUS:
//   ❌ Exception classes not yet defined
//   ❌ db_repository does not throw exceptions yet
//   ❌ Methods still return bool
//
// TO IMPLEMENT:
//   1. Create api/db/db_exceptions.h with all exception classes
//   2. Modify db_repository to throw exceptions
//   3. This file will then compile and can be used for verification
//
// ============================================================================

// ============================================================================
// CONNECTION ERRORS
// ============================================================================

SCENARIO("Creating record with no connection throws db_connection_error", "[repo][error][connection][unit][db]") {
    GIVEN("A repository with disconnected connection") {
        pg_connection conn;
        // Not calling conn.connect() - connection is NULL/disconnected
        db_repository repo(&conn);

        test_simple_product product;
        product.name = "Test Product";

        WHEN("Attempting to create") {
            THEN("Throws db_connection_error") {
                REQUIRE_THROWS_AS(repo.create(product), db_connection_error);
            }

            AND_THEN("Exception message describes the problem") {
                try {
                    repo.create(product);
                    FAIL("Should have thrown db_connection_error");
                } catch (const db_connection_error& e) {
                    flx_string msg = e.what();
                    REQUIRE(msg.contains("not connected"));
                }
            }
        }
    }
}

// ============================================================================
// MODEL STATE ERRORS
// ============================================================================

SCENARIO("Updating product with NULL ID throws db_null_id_error", "[repo][error][model][unit][db]") {
    GIVEN("A repository and product without ID") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);

        test_simple_product product;
        product.name = "Test Product";
        // ID is NULL

        WHEN("Attempting to update") {
            THEN("Throws db_null_id_error") {
                REQUIRE_THROWS_AS(repo.update(product), db_null_id_error);
            }

            AND_THEN("Exception identifies the operation") {
                try {
                    repo.update(product);
                    FAIL("Should have thrown");
                } catch (const db_null_id_error& e) {
                    REQUIRE(e.get_operation() == "update");
                }
            }

            AND_THEN("Exception identifies the table") {
                try {
                    repo.update(product);
                    FAIL("Should have thrown");
                } catch (const db_null_id_error& e) {
                    REQUIRE(e.get_table_name() == "test_simple_products");
                }
            }
        }
    }
}

SCENARIO("Deleting product with NULL ID throws db_null_id_error", "[repo][error][model][unit][db]") {
    GIVEN("A repository and product without ID") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);

        test_simple_product product;
        product.name = "Test Product";
        // ID is NULL

        WHEN("Attempting to delete") {
            THEN("Throws db_null_id_error") {
                REQUIRE_THROWS_AS(repo.remove(product), db_null_id_error);
            }

            AND_THEN("Exception identifies operation as 'delete'") {
                try {
                    repo.remove(product);
                    FAIL("Should have thrown");
                } catch (const db_null_id_error& e) {
                    REQUIRE(e.get_operation() == "delete");
                }
            }
        }
    }
}

SCENARIO("Finding non-existent record throws db_record_not_found", "[repo][error][model][unit][db]") {
    GIVEN("A repository") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "not_found");

        long long non_existent_id = 999999999;

        WHEN("Searching for non-existent ID") {
            test_simple_product product;

            THEN("Throws db_record_not_found") {
                REQUIRE_THROWS_AS(repo.find_by_id(non_existent_id, product), db_record_not_found);
            }

            AND_THEN("Exception contains the table name") {
                try {
                    repo.find_by_id(non_existent_id, product);
                    FAIL("Should have thrown");
                } catch (const db_record_not_found& e) {
                    REQUIRE(e.get_table_name() == "test_simple_products");
                }
            }

            AND_THEN("Exception contains the searched ID") {
                try {
                    repo.find_by_id(non_existent_id, product);
                    FAIL("Should have thrown");
                } catch (const db_record_not_found& e) {
                    REQUIRE(e.get_id() == non_existent_id);
                }
            }

            AND_THEN("Model remains unchanged") {
                test_simple_product original;
                original.name = "Original";
                original.price = 999.99;

                try {
                    repo.find_by_id(non_existent_id, original);
                    FAIL("Should have thrown");
                } catch (const db_record_not_found&) {
                    // Model should not be modified
                    REQUIRE(original.name == "Original");
                    REQUIRE(original.price == 999.99);
                }
            }
        }
    }
}

// ============================================================================
// CONSTRAINT VIOLATIONS
// ============================================================================

SCENARIO("Foreign key violation throws db_foreign_key_violation", "[repo][error][constraint][integration][db]") {
    GIVEN("A department referencing non-existent company") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "fk_violation");

        test_department dept;
        dept.name = "Orphan Department";
        dept.company_id = 999999999;  // Non-existent company

        WHEN("Attempting to create department") {
            THEN("Throws db_foreign_key_violation") {
                REQUIRE_THROWS_AS(repo.create(dept), db_foreign_key_violation);
            }

            AND_THEN("Exception identifies the foreign key column") {
                try {
                    repo.create(dept);
                    FAIL("Should have thrown");
                } catch (const db_foreign_key_violation& e) {
                    REQUIRE(e.get_foreign_key_column() == "company_id");
                }
            }

            AND_THEN("Exception identifies the referenced table") {
                try {
                    repo.create(dept);
                    FAIL("Should have thrown");
                } catch (const db_foreign_key_violation& e) {
                    REQUIRE(e.get_referenced_table() == "test_companies");
                }
            }

            AND_THEN("Exception contains the SQL that failed") {
                try {
                    repo.create(dept);
                    FAIL("Should have thrown");
                } catch (const db_foreign_key_violation& e) {
                    flx_string sql = e.get_sql();
                    REQUIRE(sql.contains("INSERT"));
                    REQUIRE(sql.contains("test_departments"));
                }
            }

            AND_THEN("Exception contains raw database error") {
                try {
                    repo.create(dept);
                    FAIL("Should have thrown");
                } catch (const db_foreign_key_violation& e) {
                    flx_string db_error = e.get_database_error();
                    REQUIRE(!db_error.empty());
                }
            }
        }
    }
}

SCENARIO("Unique constraint violation throws db_unique_violation", "[repo][error][constraint][integration][db]") {
    GIVEN("A product with specific name already exists") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "unique_violation");

        // Assume we add UNIQUE constraint on name in future
        // This test will be skipped until schema supports it

        test_simple_product p1;
        p1.name = cleanup.prefix() + "UniqueProduct";
        repo.create(p1);
        cleanup.track_id(p1.id);

        test_simple_product p2;
        p2.name = cleanup.prefix() + "UniqueProduct";  // Duplicate

        WHEN("Attempting to create duplicate") {
            THEN("Throws db_unique_violation") {
                REQUIRE_THROWS_AS(repo.create(p2), db_unique_violation);
            }

            AND_THEN("Exception identifies the column") {
                try {
                    repo.create(p2);
                    FAIL("Should have thrown");
                } catch (const db_unique_violation& e) {
                    REQUIRE(e.get_column_name() == "name");
                }
            }

            AND_THEN("Exception contains the duplicate value") {
                try {
                    repo.create(p2);
                    FAIL("Should have thrown");
                } catch (const db_unique_violation& e) {
                    REQUIRE(e.get_duplicate_value().string_value() == cleanup.prefix() + "UniqueProduct");
                }
            }
        }
    }
}

// ============================================================================
// HIERARCHICAL ERRORS
// ============================================================================

SCENARIO("Failed nested save throws db_nested_save_error", "[repo][error][hierarchy][integration][db]") {
    GIVEN("A company with invalid department") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "nested_save_error");

        test_company company;
        company.name = cleanup.prefix() + "TestCorp";

        // Create department with constraint violation (NULL name violates NOT NULL)
        test_department dept;
        // Don't set dept.name - leave it NULL to violate NOT NULL constraint
        company.departments.push_back(dept);

        WHEN("Attempting to create company") {
            THEN("Throws db_nested_save_error") {
                REQUIRE_THROWS_AS(repo.create(company), db_nested_save_error);
            }

            AND_THEN("Exception identifies parent table") {
                try {
                    repo.create(company);
                    FAIL("Should have thrown");
                } catch (const db_nested_save_error& e) {
                    REQUIRE(e.get_parent_table() == "test_companies");
                }
            }

            AND_THEN("Exception identifies child table") {
                try {
                    repo.create(company);
                    FAIL("Should have thrown");
                } catch (const db_nested_save_error& e) {
                    REQUIRE(e.get_child_table() == "test_departments");
                }
            }
        }
    }
}

// ============================================================================
// SCHEMA ERRORS
// ============================================================================

SCENARIO("Operating on non-existent table throws db_table_not_found", "[repo][error][schema][integration][db]") {
    GIVEN("A repository and non-existent table") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);

        // Drop table to simulate non-existence
        auto query = conn.create_query();
        query->prepare("DROP TABLE IF EXISTS test_simple_products");
        query->execute();

        test_simple_product product;
        product.name = "Test";

        WHEN("Attempting to create record") {
            THEN("Throws db_table_not_found") {
                REQUIRE_THROWS_AS(repo.create(product), db_table_not_found);
            }

            AND_THEN("Exception identifies the table") {
                try {
                    repo.create(product);
                    FAIL("Should have thrown");
                } catch (const db_table_not_found& e) {
                    REQUIRE(e.get_table_name() == "test_simple_products");
                }
            }
        }

        // Cleanup: recreate table
        repo.ensure_structures(product);
    }
}

// ============================================================================
// EXCEPTION HIERARCHY TESTS
// ============================================================================

SCENARIO("All database exceptions inherit from db_exception", "[repo][error][hierarchy][unit]") {
    THEN("db_connection_error is a db_exception") {
        REQUIRE(std::is_base_of<db_exception, db_connection_error>::value);
    }

    AND_THEN("db_query_error is a db_exception") {
        REQUIRE(std::is_base_of<db_exception, db_query_error>::value);
    }

    AND_THEN("db_model_error is a db_exception") {
        REQUIRE(std::is_base_of<db_exception, db_model_error>::value);
    }

    AND_THEN("db_constraint_violation is a db_query_error") {
        REQUIRE(std::is_base_of<db_query_error, db_constraint_violation>::value);
    }

    AND_THEN("db_foreign_key_violation is a db_constraint_violation") {
        REQUIRE(std::is_base_of<db_constraint_violation, db_foreign_key_violation>::value);
    }

    AND_THEN("db_null_id_error is a db_model_error") {
        REQUIRE(std::is_base_of<db_model_error, db_null_id_error>::value);
    }
}

SCENARIO("Catching base exception catches all derived exceptions", "[repo][error][hierarchy][unit]") {
    GIVEN("Various database exceptions") {
        THEN("Can catch all as db_exception") {
            bool caught = false;
            try {
                throw db_connection_error("test");
            } catch (const db_exception& e) {
                caught = true;
            }
            REQUIRE(caught == true);
        }

        AND_THEN("Can catch constraint violations as db_query_error") {
            bool caught = false;
            try {
                throw db_foreign_key_violation("table", "fk_col", "ref_table", "sql", "error");
            } catch (const db_query_error& e) {
                caught = true;
            }
            REQUIRE(caught == true);
        }

        AND_THEN("Can catch specific exception type") {
            bool caught_specific = false;
            bool caught_base = false;
            try {
                throw db_null_id_error("update", "products");
            } catch (const db_null_id_error& e) {
                caught_specific = true;
            } catch (const db_exception& e) {
                caught_base = true;
            }
            REQUIRE(caught_specific == true);
            REQUIRE(caught_base == false);  // Specific catch prevents base
        }
    }
}

// ============================================================================
// EXCEPTION INFORMATION TESTS
// ============================================================================

SCENARIO("Exceptions provide structured error information", "[repo][error][info][unit]") {
    THEN("db_exception provides message") {
        db_exception e("Test message");
        REQUIRE(flx_string(e.what()) == "Test message");
    }

    AND_THEN("db_exception provides SQL") {
        db_exception e("Error", "SELECT * FROM table");
        REQUIRE(e.get_sql() == "SELECT * FROM table");
    }

    AND_THEN("db_exception provides database error") {
        db_exception e("Error", "SELECT", "PG error");
        REQUIRE(e.get_database_error() == "PG error");
    }

    AND_THEN("db_null_id_error provides operation") {
        db_null_id_error e("update", "products");
        REQUIRE(e.get_operation() == "update");
    }

    AND_THEN("db_record_not_found provides ID") {
        db_record_not_found e("products", 12345);
        REQUIRE(e.get_id() == 12345);
    }

    AND_THEN("db_foreign_key_violation provides FK details") {
        db_foreign_key_violation e("orders", "customer_id", "customers", "sql", "error");
        REQUIRE(e.get_foreign_key_column() == "customer_id");
        REQUIRE(e.get_referenced_table() == "customers");
    }
}

// ============================================================================
// SUCCESSFUL OPERATIONS (NO EXCEPTIONS)
// ============================================================================

SCENARIO("Successful operations do not throw", "[repo][error][success][integration][db]") {
    GIVEN("A repository with valid data") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "success");

        test_simple_product product;
        product.name = cleanup.prefix() + "Success";
        product.price = 99.99;

        WHEN("Performing valid operations") {
            THEN("Create does not throw") {
                REQUIRE_NOTHROW(repo.create(product));
                cleanup.track_id(product.id);
            }

            AND_THEN("Update does not throw") {
                repo.create(product);
                cleanup.track_id(product.id);
                product.price = 199.99;
                REQUIRE_NOTHROW(repo.update(product));
            }

            AND_THEN("Find does not throw") {
                repo.create(product);
                cleanup.track_id(product.id);
                test_simple_product loaded;
                REQUIRE_NOTHROW(repo.find_by_id(product.id.value(), loaded));
            }

            AND_THEN("Delete does not throw") {
                repo.create(product);
                long long id = product.id.value();
                REQUIRE_NOTHROW(repo.remove(product));
            }
        }
    }
}
