#include <catch2/catch_all.hpp>
#include <api/aimodels/flx_openai_api.h>
#include <fstream>
#include <cstdlib>
#include <iostream>
#include <cmath>

static flx_string get_api_key() {
  // Try to read from .env file
  std::ifstream env_file(".env");
  if (env_file.is_open()) {
    std::string line;
    while (std::getline(env_file, line)) {
      if (line.find("OPENAI_API_KEY=") == 0) {
        return flx_string(line.substr(15));
      }
    }
  }

  // Try environment variable
  const char* env_key = std::getenv("OPENAI_API_KEY");
  if (env_key) {
    return flx_string(env_key);
  }

  return flx_string("");
}

SCENARIO("OpenAI embedding generation") {
  GIVEN("An OpenAI API client with valid API key") {
    flx_string api_key = get_api_key();

    if (api_key.empty()) {
      WARN("Skipping embedding test - No API key found in .env or OPENAI_API_KEY");
      return;
    }

    flx::llm::openai_api api(api_key);

    WHEN("Requesting embedding for simple text") {
      flx_string test_text = "Hello, this is a test sentence for embedding.";
      flxv_vector embedding;

      bool success = api.embedding(test_text, embedding);

      THEN("Embedding should be generated successfully") {
        REQUIRE(success == true);
        REQUIRE(embedding.empty() == false);

        // text-embedding-3-large has 3072 dimensions by default
        REQUIRE(embedding.size() == 3072);

        // Check that embeddings are numeric (doubles)
        REQUIRE(embedding[0].is_double() == true);
        REQUIRE(embedding[100].is_double() == true);
        REQUIRE(embedding[3071].is_double() == true);
      }
    }

    WHEN("Requesting embeddings for different texts") {
      flx_string text1 = "The cat sits on the mat.";
      flx_string text2 = "A feline rests on a rug.";
      flx_string text3 = "Quantum physics is fascinating.";

      flxv_vector emb1, emb2, emb3;

      bool success1 = api.embedding(text1, emb1);
      bool success2 = api.embedding(text2, emb2);
      bool success3 = api.embedding(text3, emb3);

      THEN("All embeddings should be generated") {
        REQUIRE(success1 == true);
        REQUIRE(success2 == true);
        REQUIRE(success3 == true);

        REQUIRE(emb1.size() == 3072);
        REQUIRE(emb2.size() == 3072);
        REQUIRE(emb3.size() == 3072);
      }

      THEN("Similar texts should have more similar embeddings") {
        // Calculate cosine similarity between embeddings
        auto cosine_similarity = [](const flxv_vector& a, const flxv_vector& b) {
          double dot_product = 0.0;
          double norm_a = 0.0;
          double norm_b = 0.0;

          for (size_t i = 0; i < a.size(); ++i) {
            double val_a = a[i].double_value();
            double val_b = b[i].double_value();
            dot_product += val_a * val_b;
            norm_a += val_a * val_a;
            norm_b += val_b * val_b;
          }

          return dot_product / (std::sqrt(norm_a) * std::sqrt(norm_b));
        };

        double sim_12 = cosine_similarity(emb1, emb2);  // Similar sentences
        double sim_13 = cosine_similarity(emb1, emb3);  // Dissimilar sentences

        std::cout << "Similarity (cat/feline): " << sim_12 << std::endl;
        std::cout << "Similarity (cat/quantum): " << sim_13 << std::endl;

        // Similar sentences should have higher similarity
        REQUIRE(sim_12 > sim_13);
        REQUIRE(sim_12 > 0.5);  // Semantically similar
      }
    }

    WHEN("Requesting embedding for longer text") {
      flx_string long_text = "This is a longer text that contains multiple sentences. "
                             "It talks about various topics including technology, science, "
                             "and everyday life. The embedding model should handle this "
                             "without any issues and return a proper vector representation.";
      flxv_vector embedding;

      bool success = api.embedding(long_text, embedding);

      THEN("Embedding should be generated for longer text") {
        REQUIRE(success == true);
        REQUIRE(embedding.size() == 3072);
      }
    }
  }
}
