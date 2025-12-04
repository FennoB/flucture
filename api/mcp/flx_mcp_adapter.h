/**
 * @file flx_mcp_adapter.h
 * @brief Flucture adapter layer for Model Context Protocol (MCP)
 *
 * This file provides a clean abstraction layer between flucture's type system
 * and the cpp-mcp library. It converts between flx_variant and mcp::json types
 * without exposing cpp-mcp dependencies in the header.
 *
 * Design Philosophy:
 * - Layer 2: Flucture Adapters (this file)
 * - Converts cpp-mcp types ↔ flx_variant
 * - No cpp-mcp dependencies in this header
 * - Clean separation of concerns
 *
 * Usage requires FLX_ENABLE_MCP compile definition.
 */

#ifndef FLX_MCP_ADAPTER_H
#define FLX_MCP_ADAPTER_H

#include "utils/flx_variant.h"
#include "utils/flx_string.h"

#ifdef FLX_ENABLE_MCP

namespace flx {

/**
 * @class mcp_adapter
 * @brief Adapter for converting between flucture types and MCP protocol types
 *
 * This class provides bidirectional conversion between:
 * - flx_variant ↔ MCP JSON types
 * - flxv_map ↔ MCP JSON objects
 * - flxv_vector ↔ MCP JSON arrays
 *
 * Implementation Note:
 * The actual cpp-mcp types are handled in the .cpp file to avoid
 * exposing cpp-mcp dependencies to flucture headers.
 */
class mcp_adapter {
public:
    /**
     * @brief Forward declaration of opaque MCP JSON type
     *
     * This prevents cpp-mcp headers from being required in flucture code.
     * The actual type (mcp::json) is only used in the implementation.
     */
    struct mcp_json_impl;

    /**
     * @brief Convert flx_variant to MCP JSON
     * @param variant The flucture variant to convert
     * @param mcp_json_out Output pointer to MCP JSON (caller must provide storage)
     * @return True if conversion was successful
     *
     * Converts flx_variant types to their MCP JSON equivalents:
     * - flx_variant::string_state → JSON string
     * - flx_variant::int_state → JSON integer
     * - flx_variant::bool_state → JSON boolean
     * - flx_variant::double_state → JSON number
     * - flx_variant::vector_state → JSON array
     * - flx_variant::map_state → JSON object
     * - flx_variant::none → JSON null
     */
    static bool to_mcp_json(const flx_variant& variant, void* mcp_json_out);

    /**
     * @brief Convert MCP JSON to flx_variant
     * @param mcp_json_in Pointer to MCP JSON object
     * @param variant_out Output variant to populate
     * @return True if conversion was successful
     *
     * Converts MCP JSON types to flx_variant equivalents:
     * - JSON string → flx_variant(flx_string)
     * - JSON integer → flx_variant(long long)
     * - JSON boolean → flx_variant(bool)
     * - JSON number → flx_variant(double)
     * - JSON array → flx_variant(flxv_vector)
     * - JSON object → flx_variant(flxv_map)
     * - JSON null → flx_variant() [none state]
     */
    static bool from_mcp_json(const void* mcp_json_in, flx_variant& variant_out);

    /**
     * @brief Convert flxv_map to MCP JSON object
     * @param map The flucture map to convert
     * @param mcp_json_out Output pointer to MCP JSON object
     * @return True if conversion was successful
     */
    static bool map_to_mcp_json(const flxv_map& map, void* mcp_json_out);

    /**
     * @brief Convert MCP JSON object to flxv_map
     * @param mcp_json_in Pointer to MCP JSON object
     * @param map_out Output map to populate
     * @return True if conversion was successful
     */
    static bool mcp_json_to_map(const void* mcp_json_in, flxv_map& map_out);

    /**
     * @brief Convert flxv_vector to MCP JSON array
     * @param vector The flucture vector to convert
     * @param mcp_json_out Output pointer to MCP JSON array
     * @return True if conversion was successful
     */
    static bool vector_to_mcp_json(const flxv_vector& vector, void* mcp_json_out);

    /**
     * @brief Convert MCP JSON array to flxv_vector
     * @param mcp_json_in Pointer to MCP JSON array
     * @param vector_out Output vector to populate
     * @return True if conversion was successful
     */
    static bool mcp_json_to_vector(const void* mcp_json_in, flxv_vector& vector_out);

    /**
     * @brief Get MCP library version string
     * @return Version string (e.g. "2024-11-05")
     */
    static flx_string get_mcp_version();

    /**
     * @brief Check if MCP support is compiled in
     * @return True if MCP support is available
     */
    static bool is_mcp_available() { return true; }
};

} // namespace flx

#else // FLX_ENABLE_MCP not defined

namespace flx {

// Stub implementation when MCP is not available
class mcp_adapter {
public:
    static bool is_mcp_available() { return false; }
    static flx_string get_mcp_version() { return "MCP support not compiled"; }
};

} // namespace flx

#endif // FLX_ENABLE_MCP

#endif // FLX_MCP_ADAPTER_H
