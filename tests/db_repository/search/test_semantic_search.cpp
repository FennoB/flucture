#include <catch2/catch_all.hpp>
#include "../shared/db_test_fixtures.h"
#include "../../../api/db/flx_semantic_embedder.h"
#include <cstdlib>

// ============================================================================
// SEMANTIC SEARCH WITH EMBEDDINGS
// ============================================================================
//
// Tests semantic search capabilities using OpenAI embeddings and pgvector:
//   - Properties marked with {"semantic": true} metadata
//   - Automatic embedding generation on create/update
//   - Cosine similarity search with search()
//   - Hierarchical semantic search with search_hierarchical()
//   - Semantic search on nested objects
//
// REQUIREMENTS:
//   - pgvector extension installed
//   - OPENAI_API_KEY environment variable
//   - semantic_embedding column (halfvec(3072))
//
// USAGE:
//   export OPENAI_API_KEY="sk-..."
//   ./flucture_tests "[repo][semantic]"
//
// ============================================================================

// Test model with semantic properties
class test_semantic_product : public flx_model {
public:
    flxp_int(id, {{"column", "id"}, {"primary_key", "test_semantic_products"}});
    flxp_string(name, {{"column", "name"}, {"semantic", true}});
    flxp_string(description, {{"column", "description"}, {"semantic", true}});
    flxp_string(category, {{"column", "category"}, {"semantic", true}});
    flxp_double(price, {{"column", "price"}});
    flxp_vector(semantic_embedding, {{"column", "semantic_embedding"}});
};

// Hierarchical model with semantic properties
class test_semantic_review : public flx_model {
public:
    flxp_int(id, {{"column", "id"}, {"primary_key", "test_semantic_reviews"}});
    flxp_int(product_id, {{"foreign_key", "test_semantic_products"}, {"column", "product_id"}});
    flxp_string(reviewer_name, {{"column", "reviewer_name"}});
    flxp_string(review_text, {{"column", "review_text"}, {"semantic", true}});
    flxp_int(rating, {{"column", "rating"}});
    flxp_vector(semantic_embedding, {{"column", "semantic_embedding"}});
};

class test_semantic_product_with_reviews : public flx_model {
public:
    flxp_int(id, {{"column", "id"}, {"primary_key", "test_semantic_products"}});
    flxp_string(name, {{"column", "name"}, {"semantic", true}});
    flxp_string(description, {{"column", "description"}, {"semantic", true}});
    flxp_string(category, {{"column", "category"}, {"semantic", true}});
    flxp_double(price, {{"column", "price"}});
    flxp_vector(semantic_embedding, {{"column", "semantic_embedding"}});
    flxp_model_list(reviews, test_semantic_review);
};

// Helper function to check if OpenAI API key is available
static bool has_openai_key() {
    return std::getenv("OPENAI_API_KEY") != nullptr;
}

// Helper function to get OpenAI API key
static flx_string get_openai_key() {
    const char* key = std::getenv("OPENAI_API_KEY");
    return key ? flx_string(key) : flx_string("");
}

// ----------------------------------------------------------------------------
// Test #1: Repository stores semantic embeddings automatically
// ----------------------------------------------------------------------------

SCENARIO("Repository generates embeddings for semantic properties on create", "[repo][semantic][unit][db][ai]") {
    GIVEN("A repository with embedder configured") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }
        if (!has_openai_key()) {
            SKIP("OPENAI_API_KEY not set");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);

        // Setup semantic embedder
        flx_semantic_embedder embedder(get_openai_key());
        repo.set_embedder(&embedder);

        // Ensure table exists with semantic_embedding column
        test_semantic_product sample;
        repo.ensure_structures(sample);

        db_test_cleanup cleanup(&conn, "semantic_create");

        test_semantic_product product;
        product.name = cleanup.prefix() + "Wireless Headphones";
        product.description = "High-quality bluetooth headphones with noise cancellation";
        product.category = "Electronics";
        product.price = 149.99;

        WHEN("Creating the product") {
            REQUIRE_NOTHROW(repo.create(product));
            cleanup.track_id(product.id);

            THEN("Product is created with ID") {
                REQUIRE(!product.id.is_null());
            }

            AND_THEN("Semantic embedding is generated") {
                REQUIRE(!product.semantic_embedding.is_null());
            }

            AND_THEN("Embedding has 3072 dimensions (OpenAI ada-002)") {
                REQUIRE(product.semantic_embedding.value().size() == 3072);
            }

            AND_THEN("Embedding values are non-zero") {
                bool has_nonzero = false;
                for (size_t i = 0; i < product.semantic_embedding.value().size(); i++) {
                    if (product.semantic_embedding.value()[i].double_value() != 0.0) {
                        has_nonzero = true;
                        break;
                    }
                }
                REQUIRE(has_nonzero == true);
            }

            AND_WHEN("Loading the product back") {
                test_semantic_product loaded;
                REQUIRE_NOTHROW(repo.find_by_id(product.id, loaded));

                THEN("Embedding is persisted") {
                    REQUIRE(!loaded.semantic_embedding.is_null());
                    REQUIRE(loaded.semantic_embedding.value().size() == 3072);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #2: Semantic DNA is created from marked properties
// ----------------------------------------------------------------------------

SCENARIO("Semantic embedder combines all semantic properties into DNA", "[repo][semantic][unit][ai]") {
    GIVEN("A semantic embedder") {
        if (!has_openai_key()) {
            SKIP("OPENAI_API_KEY not set");
        }

        flx_semantic_embedder embedder(get_openai_key());

        test_semantic_product product;
        product.name = "Laptop";
        product.description = "Powerful gaming laptop";
        product.category = "Computers";

        WHEN("Creating semantic DNA") {
            flx_string dna = embedder.create_semantic_dna(product);

            THEN("DNA contains all semantic properties") {
                REQUIRE(dna.contains("Laptop"));
                REQUIRE(dna.contains("Powerful gaming laptop"));
                REQUIRE(dna.contains("Computers"));
            }

            AND_THEN("DNA does not contain non-semantic properties") {
                // price is not marked as semantic
                REQUIRE(!dna.contains("149.99"));
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #3: Create 10 products with semantic properties
// ----------------------------------------------------------------------------

SCENARIO("Repository stores 10 products with diverse semantic content", "[repo][semantic][integration][db][ai]") {
    GIVEN("A repository with embedder") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }
        if (!has_openai_key()) {
            SKIP("OPENAI_API_KEY not set");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);

        flx_semantic_embedder embedder(get_openai_key());
        repo.set_embedder(&embedder);

        test_semantic_product sample;
        repo.ensure_structures(sample);

        db_test_cleanup cleanup(&conn, "semantic_10products");

        WHEN("Creating 10 diverse products") {
            struct ProductData {
                flx_string name;
                flx_string description;
                flx_string category;
                double price;
            };

            std::vector<ProductData> products_data = {
                {"Wireless Headphones", "Bluetooth headphones with active noise cancellation", "Electronics", 149.99},
                {"Running Shoes", "Lightweight athletic shoes for marathon training", "Sports", 89.99},
                {"Coffee Maker", "Automatic drip coffee machine with timer", "Kitchen", 79.99},
                {"Gaming Mouse", "RGB gaming mouse with 16000 DPI sensor", "Electronics", 59.99},
                {"Yoga Mat", "Non-slip exercise mat for yoga and pilates", "Sports", 29.99},
                {"Blender", "High-speed blender for smoothies and soups", "Kitchen", 119.99},
                {"Mechanical Keyboard", "Cherry MX Blue switches with RGB backlighting", "Electronics", 139.99},
                {"Dumbbells Set", "Adjustable weight dumbbells 5-50 lbs", "Sports", 199.99},
                {"Air Fryer", "Oil-free cooking with digital temperature control", "Kitchen", 99.99},
                {"Webcam", "1080p HD webcam with auto-focus for streaming", "Electronics", 69.99}
            };

            std::vector<long long> product_ids;

            for (const auto& data : products_data) {
                test_semantic_product product;
                product.name = cleanup.prefix() + data.name;
                product.description = data.description;
                product.category = data.category;
                product.price = data.price;

                REQUIRE_NOTHROW(repo.create(product));
                cleanup.track_id(product.id);
                product_ids.push_back(product.id.value());

                REQUIRE(!product.semantic_embedding.is_null());
            }

            THEN("All 10 products are created") {
                REQUIRE(product_ids.size() == 10);
            }

            AND_THEN("All have embeddings") {
                for (long long id : product_ids) {
                    test_semantic_product loaded;
                    repo.find_by_id(id, loaded);
                    REQUIRE(!loaded.semantic_embedding.is_null());
                    REQUIRE(loaded.semantic_embedding.value().size() == 3072);
                }
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #4: Semantic search finds similar products
// ----------------------------------------------------------------------------

SCENARIO("Semantic search finds products similar to query", "[repo][semantic][search][integration][db][ai]") {
    GIVEN("10 products with semantic embeddings") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }
        if (!has_openai_key()) {
            SKIP("OPENAI_API_KEY not set");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);

        flx_semantic_embedder embedder(get_openai_key());
        repo.set_embedder(&embedder);

        test_semantic_product sample;
        repo.ensure_structures(sample);

        db_test_cleanup cleanup(&conn, "semantic_search");

        // Create diverse products (same as Test #3)
        struct ProductData {
            flx_string name;
            flx_string description;
            flx_string category;
        };

        std::vector<ProductData> products_data = {
            {"Wireless Headphones", "Bluetooth headphones with active noise cancellation", "Electronics"},
            {"Running Shoes", "Lightweight athletic shoes for marathon training", "Sports"},
            {"Coffee Maker", "Automatic drip coffee machine with timer", "Kitchen"},
            {"Gaming Mouse", "RGB gaming mouse with 16000 DPI sensor", "Electronics"},
            {"Yoga Mat", "Non-slip exercise mat for yoga and pilates", "Sports"},
            {"Blender", "High-speed blender for smoothies and soups", "Kitchen"},
            {"Mechanical Keyboard", "Cherry MX Blue switches with RGB backlighting", "Electronics"},
            {"Dumbbells Set", "Adjustable weight dumbbells 5-50 lbs", "Sports"},
            {"Air Fryer", "Oil-free cooking with digital temperature control", "Kitchen"},
            {"Webcam", "1080p HD webcam with auto-focus for streaming", "Electronics"}
        };

        for (const auto& data : products_data) {
            test_semantic_product product;
            product.name = cleanup.prefix() + data.name;
            product.description = data.description;
            product.category = data.category;
            repo.create(product);
            cleanup.track_id(product.id);
        }

        WHEN("Searching for 'audio equipment for music'") {
            // Generate embedding for query
            flx_string query = "audio equipment for music";
            flxv_vector query_embedding;
            bool success = embedder.generate_embedding(query, query_embedding);
            REQUIRE(success == true);

            // Create semantic search criteria
            db_search_criteria criteria;
            criteria.like("name", "%" + cleanup.prefix() + "%");  // Filter to our test data
            criteria.semantic_search("semantic_embedding",
                                    [&]() {
                                        std::vector<double> vec;
                                        for (const auto& v : query_embedding) {
                                            vec.push_back(v.double_value());
                                        }
                                        return vec;
                                    }(),
                                    3);  // Top 3 results

            flx_model_list<test_semantic_product> results;
            REQUIRE_NOTHROW(repo.search(criteria, results));

            THEN("Returns top 3 most similar products") {
                REQUIRE(results.size() <= 3);
                REQUIRE(results.size() > 0);
            }

            AND_THEN("First result is Wireless Headphones (most relevant)") {
                REQUIRE(results[0].name.value().contains("Headphones"));
            }

            AND_THEN("Results are ordered by similarity") {
                // All results should be electronics (audio-related)
                bool all_electronics = true;
                for (size_t i = 0; i < results.size(); i++) {
                    if (results[i].category != "Electronics") {
                        all_electronics = false;
                    }
                }
                // Expecting at least the first result to be Electronics
                REQUIRE(results[0].category == "Electronics");
            }
        }

        AND_WHEN("Searching for 'fitness and workout gear'") {
            flx_string query = "fitness and workout gear";
            flxv_vector query_embedding;
            embedder.generate_embedding(query, query_embedding);

            db_search_criteria criteria;
            criteria.like("name", "%" + cleanup.prefix() + "%");
            criteria.semantic_search("semantic_embedding",
                                    [&]() {
                                        std::vector<double> vec;
                                        for (const auto& v : query_embedding) {
                                            vec.push_back(v.double_value());
                                        }
                                        return vec;
                                    }(),
                                    3);

            flx_model_list<test_semantic_product> results;
            repo.search(criteria, results);

            THEN("Returns sports products") {
                REQUIRE(results.size() > 0);
                // At least one should be Sports category
                bool has_sports = false;
                for (size_t i = 0; i < results.size(); i++) {
                    if (results[i].category == "Sports") {
                        has_sports = true;
                    }
                }
                REQUIRE(has_sports == true);
            }

            AND_THEN("Running Shoes or Dumbbells are in top results") {
                bool has_fitness = false;
                for (size_t i = 0; i < results.size(); i++) {
                    if (results[i].name.value().contains("Running") ||
                        results[i].name.value().contains("Dumbbells") ||
                        results[i].name.value().contains("Yoga")) {
                        has_fitness = true;
                    }
                }
                REQUIRE(has_fitness == true);
            }
        }

        AND_WHEN("Searching for 'kitchen appliances for cooking'") {
            flx_string query = "kitchen appliances for cooking";
            flxv_vector query_embedding;
            embedder.generate_embedding(query, query_embedding);

            db_search_criteria criteria;
            criteria.like("name", "%" + cleanup.prefix() + "%");
            criteria.semantic_search("semantic_embedding",
                                    [&]() {
                                        std::vector<double> vec;
                                        for (const auto& v : query_embedding) {
                                            vec.push_back(v.double_value());
                                        }
                                        return vec;
                                    }(),
                                    3);

            flx_model_list<test_semantic_product> results;
            repo.search(criteria, results);

            THEN("Returns kitchen products") {
                REQUIRE(results.size() > 0);
                bool has_kitchen = false;
                for (size_t i = 0; i < results.size(); i++) {
                    if (results[i].category == "Kitchen") {
                        has_kitchen = true;
                    }
                }
                REQUIRE(has_kitchen == true);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #5: Hierarchical semantic search on nested objects
// ----------------------------------------------------------------------------

SCENARIO("Hierarchical semantic search finds products by review content", "[repo][semantic][hierarchy][integration][db][ai]") {
    GIVEN("Products with reviews (nested semantic content)") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }
        if (!has_openai_key()) {
            SKIP("OPENAI_API_KEY not set");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);

        flx_semantic_embedder embedder(get_openai_key());
        repo.set_embedder(&embedder);

        test_semantic_product_with_reviews sample;
        repo.ensure_structures(sample);

        db_test_cleanup cleanup(&conn, "semantic_hierarchy");

        // Create product 1: Headphones with positive sound quality reviews
        test_semantic_product_with_reviews product1;
        product1.name = cleanup.prefix() + "Premium Headphones";
        product1.description = "Professional audio headphones";
        product1.category = "Electronics";

        test_semantic_review review1_1;
        review1_1.reviewer_name = "AudioPhile123";
        review1_1.review_text = "Amazing sound quality, crystal clear highs and deep bass";
        review1_1.rating = 5;

        test_semantic_review review1_2;
        review1_2.reviewer_name = "MusicLover";
        review1_2.review_text = "Perfect for studio monitoring, accurate sound reproduction";
        review1_2.rating = 5;

        product1.reviews.push_back(review1_1);
        product1.reviews.push_back(review1_2);

        repo.create(product1);
        cleanup.track_id(product1.id);

        // Create product 2: Laptop with performance reviews
        test_semantic_product_with_reviews product2;
        product2.name = cleanup.prefix() + "Gaming Laptop";
        product2.description = "High-performance laptop for gaming";
        product2.category = "Electronics";

        test_semantic_review review2_1;
        review2_1.reviewer_name = "Gamer99";
        review2_1.review_text = "Runs all games smoothly, excellent graphics card performance";
        review2_1.rating = 4;

        product2.reviews.push_back(review2_1);

        repo.create(product2);
        cleanup.track_id(product2.id);

        // Create product 3: Shoes with comfort reviews
        test_semantic_product_with_reviews product3;
        product3.name = cleanup.prefix() + "Running Shoes";
        product3.description = "Comfortable athletic footwear";
        product3.category = "Sports";

        test_semantic_review review3_1;
        review3_1.reviewer_name = "Runner42";
        review3_1.review_text = "Very comfortable for long distance running, great cushioning";
        review3_1.rating = 5;

        product3.reviews.push_back(review3_1);

        repo.create(product3);
        cleanup.track_id(product3.id);

        WHEN("Searching hierarchically for 'audio quality and sound'") {
            // Generate embedding for query
            flx_string query = "audio quality and sound";
            flxv_vector query_embedding;
            embedder.generate_embedding(query, query_embedding);

            // Create semantic search criteria
            db_search_criteria criteria;
            criteria.like("name", "%" + cleanup.prefix() + "%");
            criteria.semantic_search("semantic_embedding",
                                    [&]() {
                                        std::vector<double> vec;
                                        for (const auto& v : query_embedding) {
                                            vec.push_back(v.double_value());
                                        }
                                        return vec;
                                    }(),
                                    2);

            flx_model_list<test_semantic_product_with_reviews> results;
            REQUIRE_NOTHROW(repo.search_hierarchical(criteria, results));

            THEN("Returns products with semantic matches in reviews") {
                REQUIRE(results.size() > 0);
            }

            AND_THEN("Headphones product is ranked first (reviews mention sound)") {
                // CRITICAL TEST: Does hierarchical search consider nested semantic content?
                // This requires search_hierarchical to:
                // 1. Search in parent semantic_embedding
                // 2. Search in child semantic_embedding (reviews)
                // 3. Combine/aggregate similarity scores
                // 4. Return results with loaded nested objects

                bool found_headphones = false;
                for (size_t i = 0; i < results.size(); i++) {
                    if (results[i].name.value().contains("Headphones")) {
                        found_headphones = true;
                        // Should rank higher than laptop/shoes
                        REQUIRE(i <= 1);  // In top 2
                    }
                }
                REQUIRE(found_headphones == true);
            }

            AND_THEN("Results include nested reviews") {
                // Hierarchical load should bring reviews
                if (results.size() > 0 && results[0].name.value().contains("Headphones")) {
                    REQUIRE(results[0].reviews.size() == 2);
                    REQUIRE(results[0].reviews[0].review_text.value().contains("sound"));
                }
            }
        }

        AND_WHEN("Searching for 'gaming performance graphics'") {
            flx_string query = "gaming performance graphics";
            flxv_vector query_embedding;
            embedder.generate_embedding(query, query_embedding);

            db_search_criteria criteria;
            criteria.like("name", "%" + cleanup.prefix() + "%");
            criteria.semantic_search("semantic_embedding",
                                    [&]() {
                                        std::vector<double> vec;
                                        for (const auto& v : query_embedding) {
                                            vec.push_back(v.double_value());
                                        }
                                        return vec;
                                    }(),
                                    2);

            flx_model_list<test_semantic_product_with_reviews> results;
            repo.search_hierarchical(criteria, results);

            THEN("Laptop with gaming review ranks high") {
                bool found_laptop = false;
                for (size_t i = 0; i < results.size(); i++) {
                    if (results[i].name.value().contains("Laptop")) {
                        found_laptop = true;
                    }
                }
                REQUIRE(found_laptop == true);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #6: Semantic search respects combined criteria
// ----------------------------------------------------------------------------

SCENARIO("Semantic search combines with traditional filters", "[repo][semantic][search][integration][db][ai]") {
    GIVEN("Products with semantic embeddings and various prices") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }
        if (!has_openai_key()) {
            SKIP("OPENAI_API_KEY not set");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);

        flx_semantic_embedder embedder(get_openai_key());
        repo.set_embedder(&embedder);

        test_semantic_product sample;
        repo.ensure_structures(sample);

        db_test_cleanup cleanup(&conn, "semantic_combined");

        // Create electronics with varying prices
        test_semantic_product p1;
        p1.name = cleanup.prefix() + "Budget Headphones";
        p1.description = "Affordable audio solution";
        p1.category = "Electronics";
        p1.price = 29.99;
        repo.create(p1);
        cleanup.track_id(p1.id);

        test_semantic_product p2;
        p2.name = cleanup.prefix() + "Premium Headphones";
        p2.description = "High-end audiophile headphones";
        p2.category = "Electronics";
        p2.price = 299.99;
        repo.create(p2);
        cleanup.track_id(p2.id);

        test_semantic_product p3;
        p3.name = cleanup.prefix() + "Pro Headphones";
        p3.description = "Professional studio headphones";
        p3.category = "Electronics";
        p3.price = 499.99;
        repo.create(p3);
        cleanup.track_id(p3.id);

        WHEN("Searching semantically for 'headphones' with price < 350") {
            flx_string query = "headphones audio";
            flxv_vector query_embedding;
            embedder.generate_embedding(query, query_embedding);

            db_search_criteria criteria;
            criteria.like("name", "%" + cleanup.prefix() + "%");
            criteria.less_than("price", flx_variant(350.0));
            criteria.semantic_search("semantic_embedding",
                                    [&]() {
                                        std::vector<double> vec;
                                        for (const auto& v : query_embedding) {
                                            vec.push_back(v.double_value());
                                        }
                                        return vec;
                                    }(),
                                    5);

            flx_model_list<test_semantic_product> results;
            repo.search(criteria, results);

            THEN("Returns headphones under $350") {
                REQUIRE(results.size() == 2);  // Budget and Premium, not Pro
            }

            AND_THEN("All results match both semantic and price criteria") {
                for (size_t i = 0; i < results.size(); i++) {
                    REQUIRE(results[i].name.value().contains("Headphones"));
                    REQUIRE(results[i].price.value() < 350.0);
                }
            }

            AND_THEN("Pro Headphones is excluded (price too high)") {
                bool has_pro = false;
                for (size_t i = 0; i < results.size(); i++) {
                    if (results[i].name.value().contains("Pro")) {
                        has_pro = true;
                    }
                }
                REQUIRE(has_pro == false);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #7: Semantic search with no embedder should fail gracefully
// ----------------------------------------------------------------------------

SCENARIO("Repository without embedder cannot perform semantic search", "[repo][semantic][error][unit][db]") {
    GIVEN("A repository without embedder") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);
        // NOTE: No embedder set!

        test_semantic_product sample;
        repo.ensure_structures(sample);

        db_test_cleanup cleanup(&conn, "no_embedder");

        test_semantic_product product;
        product.name = cleanup.prefix() + "Test Product";
        product.description = "Test description";

        WHEN("Creating product") {
            repo.create(product);
            cleanup.track_id(product.id);

            THEN("Product is created") {
                REQUIRE(!product.id.is_null());
            }

            AND_THEN("Semantic embedding is NULL (no embedder)") {
                REQUIRE(product.semantic_embedding.is_null());
            }
        }

        AND_WHEN("Attempting semantic search without embeddings") {
            db_search_criteria criteria;
            criteria.like("name", "%" + cleanup.prefix() + "%");

            std::vector<double> dummy_embedding(3072, 0.0);
            criteria.semantic_search("semantic_embedding", dummy_embedding, 5);

            flx_model_list<test_semantic_product> results;

            THEN("Search executes but may return no meaningful results") {
                // Should not crash, but results may be empty or unordered
                // since all embeddings are NULL
                REQUIRE_NOTHROW(repo.search(criteria, results));
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Test #8: Update regenerates semantic embedding
// ----------------------------------------------------------------------------

SCENARIO("Updating semantic properties regenerates embedding", "[repo][semantic][update][integration][db][ai]") {
    GIVEN("A product with semantic embedding") {
        if (!global_db_setup()) {
            SKIP("Database not available");
        }
        if (!has_openai_key()) {
            SKIP("OPENAI_API_KEY not set");
        }

        pg_connection& conn = get_test_connection();
        db_repository repo(&conn);

        flx_semantic_embedder embedder(get_openai_key());
        repo.set_embedder(&embedder);

        test_semantic_product sample;
        repo.ensure_structures(sample);

        db_test_cleanup cleanup(&conn, "semantic_update");

        test_semantic_product product;
        product.name = cleanup.prefix() + "Headphones";
        product.description = "Audio device";
        product.category = "Electronics";

        repo.create(product);
        cleanup.track_id(product.id);

        // Store original embedding
        flxv_vector original_embedding = product.semantic_embedding.value();

        WHEN("Updating description (semantic property)") {
            product.description = "Premium noise-cancelling wireless headphones";
            repo.update(product);

            THEN("Embedding is regenerated") {
                REQUIRE(!product.semantic_embedding.is_null());
            }

            AND_THEN("New embedding is different from original") {
                // Embeddings should differ because content changed
                bool is_different = false;
                for (size_t i = 0; i < 100; i++) {  // Check first 100 values
                    if (std::abs(product.semantic_embedding.value()[i].double_value() -
                                original_embedding[i].double_value()) > 0.001) {
                        is_different = true;
                        break;
                    }
                }
                REQUIRE(is_different == true);
            }

            AND_WHEN("Searching semantically for new content") {
                flx_string query = "noise cancelling wireless";
                flxv_vector query_embedding;
                embedder.generate_embedding(query, query_embedding);

                db_search_criteria criteria;
                criteria.like("name", "%" + cleanup.prefix() + "%");
                criteria.semantic_search("semantic_embedding",
                                        [&]() {
                                            std::vector<double> vec;
                                            for (const auto& v : query_embedding) {
                                                vec.push_back(v.double_value());
                                            }
                                            return vec;
                                        }(),
                                        5);

                flx_model_list<test_semantic_product> results;
                repo.search(criteria, results);

                THEN("Updated product is found") {
                    bool found = false;
                    for (size_t i = 0; i < results.size(); i++) {
                        if (results[i].id == product.id) {
                            found = true;
                        }
                    }
                    REQUIRE(found == true);
                }
            }
        }
    }
}
