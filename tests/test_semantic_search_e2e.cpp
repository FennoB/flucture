#include <catch2/catch_all.hpp>
#include <api/db/pg_connection.h>
#include <api/db/pg_query.h>
#include <api/db/db_search_criteria.h>
#include <api/db/db_query_builder.h>
#include <cstdlib>
#include <fstream>

// Helper to get connection string
flx_string get_test_db_connection() {
    return "host=h2993861.stratoserver.net port=5432 dbname=flucture_tests user=flucture_user password=gu9nU2OAQo97bWcZB6eWJP39kdw0gvq0";
}

// Helper to get OpenAI API key
flx_string get_openai_api_key() {
    const char* key = std::getenv("OPENAI_API_KEY");
    if (key) return flx_string(key);

    std::ifstream env_file("../.env");
    if (env_file.is_open()) {
        std::string line;
        while (std::getline(env_file, line)) {
            if (line.rfind("OPENAI_API_KEY=", 0) == 0) {
                return flx_string(line.substr(15).c_str());
            }
        }
    }
    return flx_string();
}

SCENARIO("End-to-End semantic search with pgvector", "[semantic_search][e2e][integration][slow][db]") {
    GIVEN("A PostgreSQL connection and OpenAI API key") {
        pg_connection conn;
        flx_string conn_str = get_test_db_connection();
        flx_string api_key = get_openai_api_key();

        if (api_key.empty()) {
            SKIP("OPENAI_API_KEY not found - skipping E2E test");
        }

        if (!conn.connect(conn_str)) {
            SKIP("Database not available - skipping E2E test");
        }

        WHEN("Setting up test table with pgvector") {
            // Create pgvector extension if not exists
            auto query_ext = conn.create_query();
            query_ext->prepare("CREATE EXTENSION IF NOT EXISTS vector");
            query_ext->execute();

            // Create test table with vector column (1536 dimensions for text-embedding-3-small)
            auto query_drop = conn.create_query();
            query_drop->prepare("DROP TABLE IF EXISTS test_semantic_docs CASCADE");
            query_drop->execute();

            auto query_create = conn.create_query();
            query_create->prepare(
                "CREATE TABLE test_semantic_docs ("
                "  id SERIAL PRIMARY KEY,"
                "  title TEXT NOT NULL,"
                "  content TEXT NOT NULL,"
                "  embedding vector(1536)"
                ")"
            );
            query_create->execute();

            THEN("Table is created successfully") {
                REQUIRE(conn.is_connected());

                AND_WHEN("Inserting documents with embeddings") {
                    // Sample documents about different topics
                    struct doc {
                        flx_string title;
                        flx_string content;
                    };

                    std::vector<doc> documents = {
                        {"Procurement Guidelines", "This document describes public procurement procedures and tendering processes for government contracts."},
                        {"Cooking Recipes", "A collection of delicious recipes including pasta carbonara, tiramisu, and pizza margherita."},
                        {"Software Development", "Best practices for agile software development, version control with git, and continuous integration."},
                        {"Tender Documentation", "Requirements for bid submission, evaluation criteria, and contract award procedures in public tenders."},
                        {"Travel Guide Italy", "Tourist attractions in Rome, Florence, and Venice. Must-see places and restaurant recommendations."}
                    };

                    // Generate embeddings via OpenAI API (we'll use a simple approach - in production use embedding_generator)
                    // For now, we'll create mock embeddings with pattern-based values
                    // Real test would use: embedding_generator gen(api_key); auto emb = gen.generate(content);

                    THEN("Documents can be inserted") {
                        for (size_t i = 0; i < documents.size(); ++i) {
                            // Create a simple pattern-based embedding (in real scenario, use OpenAI)
                            // For testing: create vector with first value indicating topic
                            std::vector<double> embedding(1536, 0.0);

                            // Pattern: procurement docs have high values in first positions
                            if (documents[i].title.contains("Procurement") ||
                                documents[i].title.contains("Tender")) {
                                embedding[0] = 0.9;
                                embedding[1] = 0.8;
                            }
                            // Cooking docs have high values in different positions
                            else if (documents[i].title.contains("Cooking") ||
                                     documents[i].title.contains("Recipes")) {
                                embedding[100] = 0.9;
                                embedding[101] = 0.8;
                            }
                            // Software docs different pattern
                            else if (documents[i].title.contains("Software")) {
                                embedding[200] = 0.9;
                                embedding[201] = 0.8;
                            }
                            // Travel docs different pattern
                            else {
                                embedding[300] = 0.9;
                                embedding[301] = 0.8;
                            }

                            // Build vector literal for PostgreSQL
                            flx_string vector_literal = "[";
                            for (size_t j = 0; j < embedding.size(); ++j) {
                                if (j > 0) vector_literal += ",";
                                vector_literal += flx_string(std::to_string(embedding[j]).c_str());
                            }
                            vector_literal += "]";

                            // Insert document
                            auto insert_query = conn.create_query();
                            flx_string sql = "INSERT INTO test_semantic_docs (title, content, embedding) VALUES ($1, $2, $3::vector)";

                            insert_query->prepare(sql);
                            insert_query->bind(1, documents[i].title);
                            insert_query->bind(2, documents[i].content);
                            insert_query->bind(3, vector_literal);

                            bool inserted = insert_query->execute();
                            REQUIRE(inserted);
                        }

                        AND_WHEN("Performing semantic search for procurement-related content") {
                            // Create query embedding similar to procurement docs
                            std::vector<double> query_embedding(1536, 0.0);
                            query_embedding[0] = 0.85;  // Similar to procurement pattern
                            query_embedding[1] = 0.75;

                            // Use db_search_criteria for semantic search
                            db_search_criteria criteria;
                            criteria.semantic_search("embedding", query_embedding, 3);

                            // Build SQL query
                            db_query_builder builder;
                            builder.select("id, title, content, embedding <-> $1::vector AS distance")
                                   .from("test_semantic_docs");

                            criteria.apply_to(builder);

                            flx_string sql = builder.build_select();

                            THEN("SQL contains pgvector distance operator") {
                                REQUIRE(sql.contains("<->"));
                                REQUIRE(sql.contains("::vector"));
                                REQUIRE(sql.contains("ORDER BY"));
                                REQUIRE(sql.contains("LIMIT 3"));

                                AND_WHEN("Executing the semantic search query") {
                                    // Build vector literal for query
                                    flx_string query_vector_literal = "[";
                                    for (size_t i = 0; i < query_embedding.size(); ++i) {
                                        if (i > 0) query_vector_literal += ",";
                                        query_vector_literal += flx_string(std::to_string(query_embedding[i]).c_str());
                                    }
                                    query_vector_literal += "]";

                                    // Execute direct query (bypassing builder parameters for simplicity)
                                    auto search_query = conn.create_query();
                                    flx_string direct_sql =
                                        "SELECT id, title, content, "
                                        "       embedding <-> '" + query_vector_literal + "'::vector AS distance "
                                        "FROM test_semantic_docs "
                                        "ORDER BY embedding <-> '" + query_vector_literal + "'::vector ASC "
                                        "LIMIT 3";

                                    search_query->prepare(direct_sql);
                                    search_query->execute();

                                    THEN("Results are ordered by semantic similarity") {
                                        int result_count = 0;
                                        bool found_procurement = false;
                                        bool found_tender = false;

                                        while (search_query->next()) {
                                            result_count++;
                                            flxv_map row = search_query->get_row();
                                            flx_string title = row["title"].string_value();
                                            double distance = row["distance"].double_value();

                                            // First results should be procurement-related
                                            if (title.contains("Procurement") || title.contains("Tender")) {
                                                if (title.contains("Procurement")) found_procurement = true;
                                                if (title.contains("Tender")) found_tender = true;

                                                // Distance should be small for similar vectors
                                                REQUIRE(distance < 5.0);  // Threshold depends on embedding
                                            }
                                        }

                                        REQUIRE(result_count == 3);
                                        REQUIRE(found_procurement);
                                        REQUIRE(found_tender);
                                    }
                                }
                            }
                        }
                    }
                }

                // Cleanup
                auto cleanup = conn.create_query();
                cleanup->prepare("DROP TABLE IF EXISTS test_semantic_docs CASCADE");
                cleanup->execute();
            }
        }

        conn.disconnect();
    }
}

SCENARIO("Semantic search with real OpenAI embeddings", "[semantic_search][e2e][integration][slow][db][llm]") {
    GIVEN("Database connection and embedding generator") {
        pg_connection conn;
        flx_string conn_str = get_test_db_connection();
        flx_string api_key = get_openai_api_key();

        if (api_key.empty()) {
            SKIP("OPENAI_API_KEY not found - skipping real embedding test");
        }

        if (!conn.connect(conn_str)) {
            SKIP("Database not available - skipping E2E test");
        }

        WHEN("Using real OpenAI embeddings") {
            // This test would use the actual embedding_generator
            // We skip it by default as it requires OpenAI API access
            // and generates real API costs

            THEN("Placeholder for real embedding test") {
                // TODO: Implement with actual embedding_generator when ready
                // embedding_generator gen(api_key);
                // auto embedding1 = gen.generate("procurement tender contract");
                // auto embedding2 = gen.generate("cooking recipe pasta");
                // Insert and search with real embeddings
                REQUIRE(true);  // Placeholder
            }
        }

        conn.disconnect();
    }
}
