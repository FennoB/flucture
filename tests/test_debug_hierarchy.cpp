#include <catch2/catch_all.hpp>
#include "db_repository/shared/db_test_fixtures.h"
#include "../api/db/db_repository.h"
#include "../api/db/db_exceptions.h"
#include <iostream>

// Simplified models for quick testing
class test_review : public flx_model {
public:
    flxp_int(id, {{"column", "id"}, {"primary_key", "test_semantic_reviews"}});
    flxp_int(product_id, {{"foreign_key", "test_semantic_products"}, {"column", "product_id"}});
    flxp_string(review_text, {{"column", "review_text"}});
};

class test_product_with_reviews : public flx_model {
public:
    flxp_int(id, {{"column", "id"}, {"primary_key", "test_semantic_products"}});
    flxp_string(name, {{"column", "name"}});
    flxp_model_list(reviews, test_review);
};

SCENARIO("DEBUG: Quick hierarchy test", "[debug][db]") {
    GIVEN("A test product with 2 reviews in DB") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);

        // Setup
        test_product_with_reviews sample_product;
        test_review sample_review;
        repo.ensure_structures(sample_product);
        repo.ensure_structures(sample_review);

        // Clean up old data - use search + remove
        flx_model_list<test_review> old_reviews;
        repo.search({}, old_reviews);
        for (size_t i = 0; i < old_reviews.size(); i++) {
            repo.remove(old_reviews[i]);
        }

        flx_model_list<test_product_with_reviews> old_products;
        repo.search({}, old_products);
        for (size_t i = 0; i < old_products.size(); i++) {
            repo.remove(old_products[i]);
        }

        // Insert test product
        test_product_with_reviews product;
        product.name = "Test Headphones";
        repo.create(product);

        std::cout << "Created product with ID: " << product.id.value() << std::endl;

        // Insert 2 reviews
        test_review review1;
        review1.product_id = product.id.value();
        review1.review_text = "Great sound quality";
        std::cout << "Before create: review1.product_id = " << review1.product_id.value() << std::endl;
        repo.create(review1);
        std::cout << "Created review1 with ID: " << review1.id.value() << std::endl;

        test_review review2;
        review2.product_id = product.id.value();
        review2.review_text = "Amazing bass";
        std::cout << "Before create: review2.product_id = " << review2.product_id.value() << std::endl;
        repo.create(review2);
        std::cout << "Created review2 with ID: " << review2.id.value() << std::endl;

        // Verify data in DB with raw SQL
        flx_model_list<test_review> all_reviews;
        db_search_criteria review_criteria;
        review_criteria.equals("product_id", product.id.value());
        repo.search(review_criteria, all_reviews);

        std::cout << "DB shows " << all_reviews.size() << " reviews for product" << std::endl;
        REQUIRE(all_reviews.size() == 2);

        WHEN("Loading via search_hierarchical") {
            db_search_criteria criteria;
            criteria.equals("id", product.id.value());

            flx_model_list<test_product_with_reviews> results;

            std::cout << "\n=== BEFORE search_hierarchical ===" << std::endl;
            std::cout << "Sample product model_lists size: " << sample_product.get_model_lists().size() << std::endl;
            for (const auto& [name, list] : sample_product.get_model_lists()) {
                std::cout << "  - " << name.c_str() << std::endl;
            }

            try {
                repo.search_hierarchical(criteria, results);
            } catch (const db_exception& e) {
                std::cout << "\n!!! EXCEPTION !!!" << std::endl;
                std::cout << "Message: " << e.what() << std::endl;
                std::cout << "SQL: " << e.get_sql().c_str() << std::endl;
                std::cout << "DB Error: " << e.get_database_error().c_str() << std::endl;
                throw;
            }

            std::cout << "\n=== AFTER search_hierarchical ===" << std::endl;
            std::cout << "Results size: " << results.size() << std::endl;

            THEN("Result should have 2 reviews") {
                REQUIRE(results.size() == 1);
                std::cout << "Result[0].reviews.size() = " << results[0].reviews.size() << std::endl;
                REQUIRE(results[0].reviews.size() == 2);
            }
        }
    }
}
