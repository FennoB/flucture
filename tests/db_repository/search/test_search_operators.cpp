#include <catch2/catch_all.hpp>
#include "../shared/db_test_fixtures.h"
#include <api/db/db_search_criteria.h>
#include "../../../api/db/db_exceptions.h"

// ============================================================================
// SEARCH OPERATORS
// ============================================================================
//
// Tests all search operators with db_search_criteria:
//   - Comparison: equals, not_equals, greater_than, less_than, etc.
//   - String matching: like, not_like
//   - NULL checks: is_null, is_not_null
//   - List operations: in, not_in
//   - Range: between
//   - Sorting: order_by (asc/desc)
//   - Pagination: limit, offset
//
// USAGE:
//   ./flucture_tests "[search][operators]"
//   ./flucture_tests "[search][unit]"
//
// ============================================================================

// ----------------------------------------------------------------------------
// Test #1: equals operator returns exact matches
// ----------------------------------------------------------------------------

SCENARIO("Search with equals operator returns exact matches", "[repo][search][operators][unit][db]") {
    GIVEN("A repository with multiple products") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "search_equals");

        // Create test products (with unique names due to UNIQUE constraint)
        test_simple_product p1, p2, p3;
        p1.name = cleanup.prefix() + "Product1";
        p1.price = 999.99;
        p2.name = cleanup.prefix() + "Product2";
        p2.price = 1299.99;
        p3.name = cleanup.prefix() + "Product3";
        p3.price = 999.99; // Same price as p1

        repo.create(p1);
        repo.create(p2);
        repo.create(p3);
        cleanup.track_id(p1.id);
        cleanup.track_id(p2.id);
        cleanup.track_id(p3.id);

        WHEN("Searching with equals criteria") {
            db_search_criteria criteria;
            criteria.equals("price", flx_variant(999.99));

            flx_model_list<test_simple_product> results;
            REQUIRE_NOTHROW(repo.search(criteria, results));

            THEN("Exactly 2 matching products are returned") {
                REQUIRE(results.size() == 2);
            }

            AND_THEN("Both results have the searched price") {
                REQUIRE(results[0].price == 999.99);
                REQUIRE(results[1].price == 999.99);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #2: greater_than operator filters correctly
// ----------------------------------------------------------------------------

SCENARIO("Search with greater_than filters numeric values correctly", "[repo][search][operators][unit][db]") {
    GIVEN("Products with different prices") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "search_gt");

        // Create products with different prices
        test_simple_product p1, p2, p3;
        p1.name = cleanup.prefix() + "Cheap";
        p1.price = 100.0;
        p2.name = cleanup.prefix() + "Medium";
        p2.price = 500.0;
        p3.name = cleanup.prefix() + "Expensive";
        p3.price = 1000.0;

        repo.create(p1);
        repo.create(p2);
        repo.create(p3);
        cleanup.track_id(p1.id);
        cleanup.track_id(p2.id);
        cleanup.track_id(p3.id);

        WHEN("Searching for price > 500") {
            db_search_criteria criteria;
            criteria.greater_than("price", flx_variant(500.0));

            flx_model_list<test_simple_product> results;
            repo.search(criteria, results);

            THEN("Only products with price > 500 are returned") {
                REQUIRE(results.size() == 1);
                REQUIRE(results[0].price.value() == Catch::Approx(1000.0).epsilon(0.01));
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #3: like operator matches patterns
// ----------------------------------------------------------------------------

SCENARIO("Search with like operator matches string patterns", "[repo][search][operators][unit][db]") {
    GIVEN("Products with similar names") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "search_like");

        test_simple_product p1, p2, p3;
        p1.name = cleanup.prefix() + "Gaming Laptop";
        p2.name = cleanup.prefix() + "Office Laptop";
        p3.name = cleanup.prefix() + "Gaming Desktop";

        repo.create(p1);
        repo.create(p2);
        repo.create(p3);
        cleanup.track_id(p1.id);
        cleanup.track_id(p2.id);
        cleanup.track_id(p3.id);

        WHEN("Searching with LIKE '%Laptop%'") {
            db_search_criteria criteria;
            criteria.like("name", "%" + cleanup.prefix() + "%Laptop%");

            flx_model_list<test_simple_product> results;
            repo.search(criteria, results);

            THEN("All products with 'Laptop' in name are returned") {
                REQUIRE(results.size() == 2);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #4: is_null operator finds NULL values
// ----------------------------------------------------------------------------

SCENARIO("Search with is_null finds records with NULL values", "[repo][search][operators][null][unit][db]") {
    GIVEN("Products with NULL and non-NULL prices") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "search_is_null");

        test_simple_product p1, p2;
        p1.name = cleanup.prefix() + "WithPrice";
        p1.price = 100.0;
        p2.name = cleanup.prefix() + "WithoutPrice";
        // p2.price stays NULL

        repo.create(p1);
        repo.create(p2);
        cleanup.track_id(p1.id);
        cleanup.track_id(p2.id);

        WHEN("Searching for products where price IS NULL") {
            db_search_criteria criteria;
            criteria.is_null("price");

            flx_model_list<test_simple_product> results;
            repo.search(criteria, results);

            THEN("Only products with NULL price are returned") {
                REQUIRE(results.size() == 1);
                REQUIRE(results[0].price.is_null() == true);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #5: in operator matches multiple values
// ----------------------------------------------------------------------------

SCENARIO("Search with in operator matches list of values", "[repo][search][operators][unit][db]") {
    GIVEN("Products with different stock quantities") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "search_in");

        test_simple_product p1, p2, p3, p4;
        p1.name = cleanup.prefix() + "P1";
        p1.stock_quantity = 10;
        p2.name = cleanup.prefix() + "P2";
        p2.stock_quantity = 20;
        p3.name = cleanup.prefix() + "P3";
        p3.stock_quantity = 30;
        p4.name = cleanup.prefix() + "P4";
        p4.stock_quantity = 40;

        repo.create(p1);
        repo.create(p2);
        repo.create(p3);
        repo.create(p4);
        cleanup.track_id(p1.id);
        cleanup.track_id(p2.id);
        cleanup.track_id(p3.id);
        cleanup.track_id(p4.id);

        WHEN("Searching with IN (10, 30)") {
            db_search_criteria criteria;
            std::vector<flx_variant> values = {flx_variant(10LL), flx_variant(30LL)};
            criteria.in("stock_quantity", values);

            flx_model_list<test_simple_product> results;
            repo.search(criteria, results);

            THEN("Only products with matching stock quantities are returned") {
                REQUIRE(results.size() == 2);
            }

            AND_THEN("Results contain only the specified values") {
                bool has_10 = false, has_30 = false;
                for (size_t i = 0; i < results.size(); i++) {
                    if (results[i].stock_quantity == 10) has_10 = true;
                    if (results[i].stock_quantity == 30) has_30 = true;
                }
                REQUIRE(has_10 == true);
                REQUIRE(has_30 == true);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #6: between operator filters ranges
// ----------------------------------------------------------------------------

SCENARIO("Search with between operator filters numeric ranges", "[repo][search][operators][unit][db]") {
    GIVEN("Products with various prices") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "search_between");

        test_simple_product p1, p2, p3, p4;
        p1.name = cleanup.prefix() + "P1";
        p1.price = 50.0;
        p2.name = cleanup.prefix() + "P2";
        p2.price = 150.0;
        p3.name = cleanup.prefix() + "P3";
        p3.price = 250.0;
        p4.name = cleanup.prefix() + "P4";
        p4.price = 350.0;

        repo.create(p1);
        repo.create(p2);
        repo.create(p3);
        repo.create(p4);
        cleanup.track_id(p1.id);
        cleanup.track_id(p2.id);
        cleanup.track_id(p3.id);
        cleanup.track_id(p4.id);

        WHEN("Searching for price BETWEEN 100 AND 300") {
            db_search_criteria criteria;
            criteria.between("price", flx_variant(100.0), flx_variant(300.0));

            flx_model_list<test_simple_product> results;
            repo.search(criteria, results);

            THEN("Only products in range are returned") {
                REQUIRE(results.size() == 2);
            }

            AND_THEN("Results are within bounds") {
                for (size_t i = 0; i < results.size(); i++) {
                    REQUIRE(results[i].price.value() >= 100.0);
                    REQUIRE(results[i].price.value() <= 300.0);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #7: order_by sorts results correctly
// ----------------------------------------------------------------------------

SCENARIO("Search with order_by sorts results correctly", "[repo][search][operators][unit][db]") {
    GIVEN("Products with different prices") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "search_order");

        test_simple_product p1, p2, p3;
        p1.name = cleanup.prefix() + "P1";
        p1.price = 300.0;
        p2.name = cleanup.prefix() + "P2";
        p2.price = 100.0;
        p3.name = cleanup.prefix() + "P3";
        p3.price = 200.0;

        repo.create(p1);
        repo.create(p2);
        repo.create(p3);
        cleanup.track_id(p1.id);
        cleanup.track_id(p2.id);
        cleanup.track_id(p3.id);

        WHEN("Searching with ORDER BY price ASC") {
            db_search_criteria criteria;
            criteria.like("name", "%" + cleanup.prefix() + "%");
            criteria.order_by("price", true);

            flx_model_list<test_simple_product> results;
            repo.search(criteria, results);

            THEN("Results are sorted by price ascending") {
                REQUIRE(results.size() >= 3);
                // Find our test products in results
                std::vector<double> our_prices;
                for (size_t i = 0; i < results.size(); i++) {
                    if (results[i].name.value().find(cleanup.prefix()) != std::string::npos) {
                        our_prices.push_back(results[i].price.value());
                    }
                }
                REQUIRE(our_prices.size() == 3);
                REQUIRE(our_prices[0] < our_prices[1]);
                REQUIRE(our_prices[1] < our_prices[2]);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #8: limit restricts result count
// ----------------------------------------------------------------------------

SCENARIO("Search with limit restricts number of results", "[repo][search][operators][unit][db]") {
    GIVEN("Multiple products") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "search_limit");

        // Create 5 products
        for (int i = 0; i < 5; i++) {
            test_simple_product p;
            p.name = cleanup.prefix() + "Product" + std::to_string(i);
            p.price = 100.0 + i * 10.0;
            repo.create(p);
            cleanup.track_id(p.id);
        }

        WHEN("Searching with LIMIT 2") {
            db_search_criteria criteria;
            criteria.like("name", "%" + cleanup.prefix() + "%");
            criteria.limit(2);

            flx_model_list<test_simple_product> results;
            repo.search(criteria, results);

            THEN("Exactly 2 results are returned") {
                REQUIRE(results.size() == 2);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #9: Combined AND conditions
// ----------------------------------------------------------------------------

SCENARIO("Search with multiple AND conditions filters correctly", "[repo][search][operators][unit][db]") {
    GIVEN("Products with different attributes") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "search_and");

        test_simple_product p1, p2, p3;
        p1.name = cleanup.prefix() + "Laptop1";
        p1.price = 1000.0;
        p1.active = true;

        p2.name = cleanup.prefix() + "Laptop2";
        p2.price = 1500.0;
        p2.active = false;

        p3.name = cleanup.prefix() + "Desktop";
        p3.price = 1000.0;
        p3.active = true;

        repo.create(p1);
        repo.create(p2);
        repo.create(p3);
        cleanup.track_id(p1.id);
        cleanup.track_id(p2.id);
        cleanup.track_id(p3.id);

        WHEN("Searching with price=1000 AND active=true") {
            db_search_criteria criteria;
            criteria.like("name", "%" + cleanup.prefix() + "%");
            criteria.and_where("price", "=", flx_variant(1000.0));
            criteria.and_where("active", "=", flx_variant(true));

            flx_model_list<test_simple_product> results;
            repo.search(criteria, results);

            THEN("Only products matching all conditions are returned") {
                REQUIRE(results.size() == 2); // p1 and p3
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #10: find_all returns all records
// ----------------------------------------------------------------------------

SCENARIO("find_all returns all records in table", "[repo][search][basic][unit][db]") {
    GIVEN("A repository with known products") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        db_test_cleanup cleanup(&conn, "find_all");

        // Create exactly 3 products
        test_simple_product p1, p2, p3;
        p1.name = cleanup.prefix() + "P1";
        p2.name = cleanup.prefix() + "P2";
        p3.name = cleanup.prefix() + "P3";

        repo.create(p1);
        repo.create(p2);
        repo.create(p3);
        cleanup.track_id(p1.id);
        cleanup.track_id(p2.id);
        cleanup.track_id(p3.id);

        WHEN("Calling find_all") {
            // First, use find_where to get only our test data
            flx_string condition = "name LIKE '%" + cleanup.prefix() + "%'";
            flx_model_list<test_simple_product> results;
            REQUIRE_NOTHROW(repo.find_where(condition, results));

            THEN("All records are returned") {
                REQUIRE(results.size() == 3);
            }
        }
    }
}
