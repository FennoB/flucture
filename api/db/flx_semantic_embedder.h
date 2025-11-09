#ifndef FLX_SEMANTIC_EMBEDDER_H
#define FLX_SEMANTIC_EMBEDDER_H

#include "../../utils/flx_model.h"
#include "../../utils/flx_string.h"
#include "../../utils/flx_variant.h"
#include "../aimodels/flx_openai_api.h"

/**
 * Generic semantic embedder for any flx_model
 *
 * Usage:
 * 1. Mark properties with metadata: flxp_string(title, flxv_map{{"semantic", true}})
 * 2. Model must have a semantic_embedding property to store the vector
 * 3. Call embed_model() before saving to DB
 *
 * The embedder automatically:
 * - Scans all properties for {"semantic": true} metadata
 * - Combines them into a "semantic DNA" text
 * - Generates OpenAI embedding vector
 * - Stores in model["semantic_embedding"]
 */
class flx_semantic_embedder {
public:
    explicit flx_semantic_embedder(const flx_string& openai_api_key);

    /**
     * Creates semantic DNA from all properties marked with {"semantic": true}
     * Returns empty string if no semantic properties found
     */
    flx_string create_semantic_dna(flx_model& model);

    /**
     * Generates embedding vector for given text
     */
    bool generate_embedding(const flx_string& text, flxv_vector& embedding);

    /**
     * Complete workflow: DNA → embedding → store in model
     * Returns true if successful, false if no semantic properties or API error
     */
    bool embed_model(flx_model& model);

private:
    flx::llm::openai_api api_;

    // Helper to extract text from a property
    flx_string extract_text_from_property(flx_property_i* prop, flx_model& model);
};

#endif // FLX_SEMANTIC_EMBEDDER_H
