#include "flx_semantic_embedder.h"
#include <sstream>
#include <iostream>

flx_semantic_embedder::flx_semantic_embedder(const flx_string& openai_api_key)
    : api_(openai_api_key)
{
}

flx_string flx_semantic_embedder::extract_text_from_property(flx_property_i* prop, flx_model& model) {
    if (prop->is_null()) {
        return flx_string();
    }

    // Use property's access() method - it knows how to navigate fieldnames!
    flx_variant val = prop->access();

    // Convert to string based on type
    if (val.in_state() == flx_variant::string_state) {
        flx_string str = val.to_string();
        if (!str.empty()) {
            return str;
        }
    } else if (val.in_state() == flx_variant::int_state) {
        return flx_string(std::to_string(val.to_int()).c_str());
    } else if (val.in_state() == flx_variant::double_state) {
        return flx_string(std::to_string(val.to_double()).c_str());
    } else if (val.in_state() == flx_variant::map_state) {
        // Nested model - look for child in model's children map
        const auto& children = model.get_children();
        auto it = children.find(prop->prop_name());
        if (it != children.end() && it->second != nullptr) {
            // Found typed child model - recursively extract semantic text
            return create_semantic_dna(*it->second);
        }
        return flx_string();  // No child found
    } else if (val.in_state() == flx_variant::vector_state) {
        // Model list - access via get_model_lists() like db_repository does
        const auto& lists = model.get_model_lists();
        auto list_it = lists.find(prop->prop_name());
        if (list_it != lists.end() && list_it->second != nullptr) {
            flx_list* list_ptr = list_it->second;
            std::stringstream list_texts;

            // Iterate through all items in the list
            for (size_t i = 0; i < list_ptr->list_size(); ++i) {
                flx_model* item = list_ptr->get_model_at(i);
                if (item != nullptr) {
                    // Recursively extract semantic text from each item
                    flx_string item_text = create_semantic_dna(*item);
                    if (!item_text.empty()) {
                        list_texts << item_text.c_str() << " ";
                    }
                }
            }

            return flx_string(list_texts.str().c_str());
        }
        return flx_string();
    }

    return flx_string();
}

flx_string flx_semantic_embedder::create_semantic_dna(flx_model& model) {
    std::stringstream dna;

    // Iterate through all properties
    const auto& properties = model.get_properties();
    for (const auto& pair : properties) {
        flx_property_i* prop = pair.second;

        // Check if property has semantic metadata
        const flxv_map& meta = prop->get_meta();
        auto it = meta.find("semantic");
        if (it != meta.end()) {
            flx_variant semantic_flag = it->second;  // Non-const copy
            if (semantic_flag.in_state() == flx_variant::bool_state && semantic_flag.to_bool()) {
                // Extract text from this property
                flx_string text = extract_text_from_property(prop, model);
                if (!text.empty()) {
                    dna << text.c_str() << ". ";
                }
            }
        }
    }

    return flx_string(dna.str().c_str());
}

bool flx_semantic_embedder::generate_embedding(const flx_string& text, flxv_vector& embedding) {
    if (text.empty()) {
        return false;
    }

    return api_.embedding(text, embedding);
}

bool flx_semantic_embedder::embed_model(flx_model& model) {
    // Step 1: Create semantic DNA
    flx_string dna = create_semantic_dna(model);

    if (dna.empty()) {
        return false;  // No semantic properties found
    }

    // Step 2: Generate embedding
    flxv_vector embedding;
    if (!generate_embedding(dna, embedding)) {
        return false;
    }

    // Step 3: Store in model
    model["semantic_text"] = flx_variant(dna);
    model["semantic_embedding"] = flx_variant(embedding);

    return true;
}
