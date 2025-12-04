/**
 * @file flx_llm_tool_provider.h
 * @brief Tool provider abstraction for LLM function calling
 *
 * This interface allows dynamic tool discovery from various sources
 * (manual registration, MCP servers, plugins, etc.)
 */

#ifndef FLX_LLM_TOOL_PROVIDER_H
#define FLX_LLM_TOOL_PROVIDER_H

#include "flx_llm_chat_interfaces.h"
#include <vector>
#include <memory>

namespace flx::llm {

/**
 * @class i_llm_tool_provider
 * @brief Abstract interface for tool providers
 *
 * Tool providers supply functions/tools to the LLM chat system.
 * They can be backed by different sources:
 * - Manual registration (static tools)
 * - MCP servers (dynamic tools)
 * - Plugin systems
 * - REST APIs
 *
 * The provider pattern allows:
 * - Dynamic tool discovery
 * - Automatic tool updates
 * - Loose coupling between LLM and tool sources
 */
class i_llm_tool_provider {
public:
    virtual ~i_llm_tool_provider() = default;

    /**
     * @brief Get all currently available tools
     * @return Vector of tool pointers (non-owning)
     *
     * Returns all tools that should be available to the LLM.
     * The provider retains ownership of the tools.
     */
    virtual std::vector<i_llm_function*> get_available_tools() = 0;

    /**
     * @brief Refresh tool list from source
     * @return True if refresh was successful
     *
     * Updates the internal tool list by querying the underlying source.
     * For MCP: Calls tools/list on the MCP server
     * For static: Returns immediately (no changes)
     */
    virtual bool refresh_tools() = 0;

    /**
     * @brief Get provider name for debugging/logging
     * @return Human-readable provider name
     */
    virtual flx_string get_provider_name() const = 0;

    /**
     * @brief Check if provider is currently available
     * @return True if provider can supply tools
     *
     * For MCP: Checks if connection to server is alive
     * For static: Always returns true
     */
    virtual bool is_available() const = 0;
};

/**
 * @class manual_tool_provider
 * @brief Simple tool provider for manually registered functions
 *
 * This provider allows direct registration of i_llm_function instances.
 * Tools are statically managed and don't change at runtime.
 */
class manual_tool_provider : public i_llm_tool_provider {
private:
    std::map<flx_string, std::shared_ptr<i_llm_function>> functions_;
    flx_string name_;

public:
    explicit manual_tool_provider(const flx_string& name = "Manual Tools")
        : name_(name) {}

    /**
     * @brief Register a function with this provider
     * @param func Shared pointer to function (provider shares ownership)
     */
    void register_function(std::shared_ptr<i_llm_function> func) {
        if (func) {
            functions_[func->get_name()] = std::move(func);
        }
    }

    /**
     * @brief Unregister a function by name
     * @param name Function name to remove
     */
    void unregister_function(const flx_string& name) {
        functions_.erase(name);
    }

    /**
     * @brief Clear all registered functions
     */
    void clear() {
        functions_.clear();
    }

    // i_llm_tool_provider implementation
    std::vector<i_llm_function*> get_available_tools() override {
        std::vector<i_llm_function*> tools;
        tools.reserve(functions_.size());
        for (const auto& [name, func] : functions_) {
            tools.push_back(func.get());
        }
        return tools;
    }

    bool refresh_tools() override {
        // Manual tools don't need refresh
        return true;
    }

    flx_string get_provider_name() const override {
        return name_;
    }

    bool is_available() const override {
        return true;
    }

    /**
     * @brief Get number of registered functions
     */
    size_t function_count() const {
        return functions_.size();
    }
};

} // namespace flx::llm

#endif // FLX_LLM_TOOL_PROVIDER_H
