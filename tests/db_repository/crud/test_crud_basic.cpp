#include <catch2/catch_all.hpp>
#include "../shared/db_test_fixtures.h"
#include "../../../api/db/db_exceptions.h"

// ============================================================================
// BASIC CRUD OPERATIONS
// ============================================================================
//
// Tests fundamental Create, Read, Update, Delete operations with:
//   - All scalar property types (int, string, double, bool)
//   - NULL value handling
//   - Auto-assigned IDs
//   - Round-trip integrity (save → load → compare)
//
// USAGE:
//   ./flucture_tests "[crud][basic]"     # All basic CRUD tests
//   ./flucture_tests "[smoke]"            # Critical tests only
//   ./flucture_tests "[null]"             # NULL handling tests
//
// ============================================================================

// ----------------------------------------------------------------------------
// Test #1: Repository assigns auto-incrementing IDs on create
// ----------------------------------------------------------------------------

SCENARIO("Repository assigns auto-incrementing IDs on create", "[repo][crud][basic][smoke][unit][db]") {
    GIVEN("A database connection and product repository") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "assign_ids");

        AND_GIVEN("A product without ID") {
            test_simple_product product;
            product.name = cleanup.prefix() + "Laptop";
            product.price = 1299.99;

            WHEN("Creating the product") {
                REQUIRE_NOTHROW(repo.create(product));

                THEN("ID is assigned") {
                    REQUIRE(product.id.is_null() == false);
                }

                AND_THEN("ID is positive") {
                    REQUIRE(product.id.value() > 0);
                }

                cleanup.track_id(product.id.value());
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #2: Repository round-trips string properties exactly
// ----------------------------------------------------------------------------

SCENARIO("Repository round-trips string properties exactly", "[repo][crud][basic][unit][db]") {
    GIVEN("A database connection and product repository") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "roundtrip_string");

        AND_GIVEN("A product with a specific string value") {
            test_simple_product original;
            original.name = cleanup.prefix() + "Gaming Laptop";
            repo.create(original);
            cleanup.track_id(original.id.value());

            WHEN("Loading the product") {
                test_simple_product loaded;
                REQUIRE_NOTHROW(repo.find_by_id(original.id.value(), loaded));

                THEN("String property matches exactly") {
                    REQUIRE(loaded.name.value() == original.name.value());
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #3: Repository round-trips double properties within epsilon
// ----------------------------------------------------------------------------

SCENARIO("Repository round-trips double properties within epsilon", "[repo][crud][basic][unit][db]") {
    GIVEN("A database connection and product repository") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "roundtrip_double");

        AND_GIVEN("A product with a specific double value") {
            test_simple_product original;
            original.name = cleanup.prefix() + "Product";
            original.price = 1299.99;
            repo.create(original);
            cleanup.track_id(original.id.value());

            WHEN("Loading the product") {
                test_simple_product loaded;
                repo.find_by_id(original.id.value(), loaded);

                THEN("Double property matches within epsilon") {
                    REQUIRE(loaded.price.value() == Catch::Approx(1299.99).epsilon(0.01));
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #4: Repository round-trips integer properties exactly
// ----------------------------------------------------------------------------

SCENARIO("Repository round-trips integer properties exactly", "[repo][crud][basic][unit][db]") {
    GIVEN("A database connection and product repository") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "roundtrip_int");

        AND_GIVEN("A product with a specific integer value") {
            test_simple_product original;
            original.name = cleanup.prefix() + "Product";
            original.stock_quantity = 50;
            repo.create(original);
            cleanup.track_id(original.id.value());

            WHEN("Loading the product") {
                test_simple_product loaded;
                repo.find_by_id(original.id.value(), loaded);

                THEN("Integer property matches exactly") {
                    REQUIRE(loaded.stock_quantity.value() == 50);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #5: Repository round-trips boolean properties exactly
// ----------------------------------------------------------------------------

SCENARIO("Repository round-trips boolean properties exactly", "[repo][crud][basic][unit][db]") {
    GIVEN("A database connection and product repository") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "roundtrip_bool");

        AND_GIVEN("A product with active=true") {
            test_simple_product original;
            original.name = cleanup.prefix() + "Product";
            original.active = true;
            repo.create(original);
            cleanup.track_id(original.id.value());

            WHEN("Loading the product") {
                test_simple_product loaded;
                repo.find_by_id(original.id.value(), loaded);

                THEN("Boolean property is true") {
                    REQUIRE(loaded.active.value() == true);
                }
            }
        }

        AND_GIVEN("A product with active=false") {
            test_simple_product original;
            original.name = cleanup.prefix() + "Product2";
            original.active = false;
            repo.create(original);
            cleanup.track_id(original.id.value());

            WHEN("Loading the product") {
                test_simple_product loaded;
                repo.find_by_id(original.id.value(), loaded);

                THEN("Boolean property is false") {
                    REQUIRE(loaded.active.value() == false);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #6: Repository preserves NULL values distinctly from defaults
// ----------------------------------------------------------------------------

SCENARIO("Repository preserves NULL values distinctly from defaults", "[repo][crud][null][unit][db]") {
    GIVEN("A database connection and product repository") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "null_values");

        AND_GIVEN("A product with all properties NULL") {
            test_simple_product original;
            // Don't set any properties - all remain NULL
            repo.create(original);
            cleanup.track_id(original.id.value());

            WHEN("Loading the product") {
                test_simple_product loaded;
                repo.find_by_id(original.id.value(), loaded);

                THEN("ID is assigned") {
                    REQUIRE(!loaded.id.is_null());
                }

                AND_THEN("Name is NULL") {
                    REQUIRE(loaded.name.is_null() == true);
                }

                AND_THEN("Price is NULL") {
                    REQUIRE(loaded.price.is_null() == true);
                }

                AND_THEN("Stock quantity is NULL") {
                    REQUIRE(loaded.stock_quantity.is_null() == true);
                }

                AND_THEN("Active is NULL") {
                    REQUIRE(loaded.active.is_null() == true);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #7: Repository distinguishes empty string from NULL
// ----------------------------------------------------------------------------

SCENARIO("Repository distinguishes empty string from NULL", "[repo][crud][boundary][null][unit][db]") {
    GIVEN("A database connection and product repository") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "empty_vs_null");

        AND_GIVEN("Two products: one with empty string, one with NULL") {
            test_simple_product p_empty, p_null;
            p_empty.name = "";  // Empty string
            // p_null.name stays NULL

            repo.create(p_empty);
            repo.create(p_null);
            cleanup.track_id(p_empty.id.value());
            cleanup.track_id(p_null.id.value());

            WHEN("Loading both products") {
                test_simple_product loaded_empty, loaded_null;
                repo.find_by_id(p_empty.id.value(), loaded_empty);
                repo.find_by_id(p_null.id.value(), loaded_null);

                THEN("Empty string is preserved as empty") {
                    REQUIRE(!loaded_empty.name.is_null());
                    REQUIRE(loaded_empty.name.value() == "");
                }

                AND_THEN("NULL is preserved as NULL") {
                    REQUIRE(loaded_null.name.is_null());
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #8: Repository updates existing records without changing ID
// ----------------------------------------------------------------------------

SCENARIO("Repository updates existing records without changing ID", "[repo][crud][update][unit][db]") {
    GIVEN("A database connection and product repository") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "update_record");

        AND_GIVEN("An existing product") {
            test_simple_product original;
            original.name = cleanup.prefix() + "Laptop";
            original.price = 1299.99;
            original.stock_quantity = 50;
            original.active = true;
            repo.create(original);
            cleanup.track_id(original.id.value());

            long long original_id = original.id.value();

            WHEN("Updating all fields") {
                original.name = cleanup.prefix() + "Gaming Laptop";
                original.price = 1599.99;
                original.stock_quantity = 30;
                original.active = false;

                REQUIRE_NOTHROW(repo.update(original));

                THEN("ID is unchanged") {
                    REQUIRE(original.id.value() == original_id);
                }

                AND_WHEN("Loading the updated product") {
                    test_simple_product loaded;
                    repo.find_by_id(original_id, loaded);

                    THEN("All fields are updated") {
                        REQUIRE(loaded.name.value() == cleanup.prefix() + "Gaming Laptop");
                        REQUIRE(loaded.price.value() == Catch::Approx(1599.99).epsilon(0.01));
                        REQUIRE(loaded.stock_quantity.value() == 30);
                        REQUIRE(loaded.active.value() == false);
                    }
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #9: Repository removes records completely on delete
// ----------------------------------------------------------------------------

SCENARIO("Repository removes records completely on delete", "[repo][crud][delete][unit][db]") {
    GIVEN("A database connection and product repository") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "delete_record");

        AND_GIVEN("An existing product") {
            test_simple_product product;
            product.name = cleanup.prefix() + "Laptop";
            repo.create(product);
            long long product_id = product.id.value();

            WHEN("Deleting the product") {
                REQUIRE_NOTHROW(repo.remove(product));

                AND_WHEN("Trying to load the deleted product") {
                    test_simple_product loaded;

                    THEN("Record is not found") {
                        REQUIRE_THROWS_AS(repo.find_by_id(product_id, loaded), db_record_not_found);
                    }

                    AND_THEN("Loaded model has NULL ID") {
                        REQUIRE(loaded.id.is_null() == true);
                    }
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #10: Repository round-trips complete models (all properties)
// ----------------------------------------------------------------------------

SCENARIO("Repository round-trips complete models with all properties", "[repo][crud][basic][smoke][unit][db]") {
    GIVEN("A database connection and product repository") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "roundtrip_complete");

        AND_GIVEN("A product with all properties set") {
            test_simple_product original;
            original.name = cleanup.prefix() + "Complete Product";
            original.price = 1299.99;
            original.stock_quantity = 50;
            original.active = true;

            repo.create(original);
            cleanup.track_id(original.id.value());

            WHEN("Loading the product") {
                test_simple_product loaded;
                repo.find_by_id(original.id.value(), loaded);

                THEN("Models are deeply equal") {
                    REQUIRE(products_are_equal(original, loaded));
                }
            }
        }
    }
}
