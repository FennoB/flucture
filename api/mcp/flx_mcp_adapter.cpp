/**
 * @file flx_mcp_adapter.cpp
 * @brief Implementation of flucture MCP adapter layer
 */

#include "api/mcp/flx_mcp_adapter.h"

#ifdef FLX_ENABLE_MCP

// Include cpp-mcp headers (only in implementation)
#include "mcp_message.h"

namespace flx {

// Helper function: Convert flx_variant to mcp::json
static mcp::json variant_to_json(const flx_variant& variant) {
    switch (variant.in_state()) {
        case flx_variant::state::none:
            return nullptr;

        case flx_variant::state::string_state:
            return variant.string_value().c_str();

        case flx_variant::state::int_state:
            return variant.int_value();

        case flx_variant::state::bool_state:
            return variant.bool_value();

        case flx_variant::state::double_state:
            return variant.double_value();

        case flx_variant::state::vector_state: {
            mcp::json json_array = mcp::json::array();
            const flxv_vector& vec = variant.vector_value();
            for (const auto& item : vec) {
                json_array.push_back(variant_to_json(item));
            }
            return json_array;
        }

        case flx_variant::state::map_state: {
            mcp::json json_object = mcp::json::object();
            const flxv_map& map = variant.map_value();
            for (const auto& [key, value] : map) {
                json_object[key.c_str()] = variant_to_json(value);
            }
            return json_object;
        }

        default:
            return nullptr;
    }
}

// Helper function: Convert mcp::json to flx_variant
static flx_variant json_to_variant(const mcp::json& j) {
    if (j.is_null()) {
        return flx_variant();
    }
    else if (j.is_boolean()) {
        return flx_variant(j.get<bool>());
    }
    else if (j.is_number_integer()) {
        return flx_variant(static_cast<long long>(j.get<int64_t>()));
    }
    else if (j.is_number_float()) {
        return flx_variant(j.get<double>());
    }
    else if (j.is_string()) {
        return flx_variant(flx_string(j.get<std::string>().c_str()));
    }
    else if (j.is_array()) {
        flxv_vector vec;
        for (const auto& item : j) {
            vec.push_back(json_to_variant(item));
        }
        return flx_variant(vec);
    }
    else if (j.is_object()) {
        flxv_map map;
        for (auto it = j.begin(); it != j.end(); ++it) {
            map[flx_string(it.key().c_str())] = json_to_variant(it.value());
        }
        return flx_variant(map);
    }

    // Unknown type - return null variant
    return flx_variant();
}

// Public API implementations

bool mcp_adapter::to_mcp_json(const flx_variant& variant, void* mcp_json_out) {
    if (!mcp_json_out) {
        return false;
    }

    try {
        mcp::json* json_ptr = static_cast<mcp::json*>(mcp_json_out);
        *json_ptr = variant_to_json(variant);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool mcp_adapter::from_mcp_json(const void* mcp_json_in, flx_variant& variant_out) {
    if (!mcp_json_in) {
        return false;
    }

    try {
        const mcp::json* json_ptr = static_cast<const mcp::json*>(mcp_json_in);
        variant_out = json_to_variant(*json_ptr);
        return true;
    }
    catch (...) {
        return false;
    }
}

bool mcp_adapter::map_to_mcp_json(const flxv_map& map, void* mcp_json_out) {
    if (!mcp_json_out) {
        return false;
    }

    try {
        mcp::json* json_ptr = static_cast<mcp::json*>(mcp_json_out);
        *json_ptr = mcp::json::object();

        for (const auto& [key, value] : map) {
            (*json_ptr)[key.c_str()] = variant_to_json(value);
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

bool mcp_adapter::mcp_json_to_map(const void* mcp_json_in, flxv_map& map_out) {
    if (!mcp_json_in) {
        return false;
    }

    try {
        const mcp::json* json_ptr = static_cast<const mcp::json*>(mcp_json_in);

        if (!json_ptr->is_object()) {
            return false;
        }

        map_out.clear();
        for (auto it = json_ptr->begin(); it != json_ptr->end(); ++it) {
            map_out[flx_string(it.key().c_str())] = json_to_variant(it.value());
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

bool mcp_adapter::vector_to_mcp_json(const flxv_vector& vector, void* mcp_json_out) {
    if (!mcp_json_out) {
        return false;
    }

    try {
        mcp::json* json_ptr = static_cast<mcp::json*>(mcp_json_out);
        *json_ptr = mcp::json::array();

        for (const auto& item : vector) {
            json_ptr->push_back(variant_to_json(item));
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

bool mcp_adapter::mcp_json_to_vector(const void* mcp_json_in, flxv_vector& vector_out) {
    if (!mcp_json_in) {
        return false;
    }

    try {
        const mcp::json* json_ptr = static_cast<const mcp::json*>(mcp_json_in);

        if (!json_ptr->is_array()) {
            return false;
        }

        vector_out.clear();
        for (const auto& item : *json_ptr) {
            vector_out.push_back(json_to_variant(item));
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

flx_string mcp_adapter::get_mcp_version() {
    return flx_string(mcp::MCP_VERSION);
}

} // namespace flx

#endif // FLX_ENABLE_MCP
