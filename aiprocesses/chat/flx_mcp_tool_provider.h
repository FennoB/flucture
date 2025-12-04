#ifndef FLX_MCP_TOOL_PROVIDER_H
#define FLX_MCP_TOOL_PROVIDER_H

#include "flx_llm_tool_provider.h"

#ifdef FLX_ENABLE_MCP

#include <memory>

namespace mcp {
    class client;
}

namespace flx::llm {

class mcp_function_adapter : public i_llm_function {
    flx_string name_;
    flx_string description_;
    flxv_map parameters_;
    std::shared_ptr<mcp::client> mcp_client_;

public:
    mcp_function_adapter(
        const flx_string& name,
        const flx_string& description,
        const flxv_map& input_schema,
        std::shared_ptr<mcp::client> client
    );

    flx_string get_name() const override { return name_; }
    flx_string get_description() const override { return description_; }
    flxv_map get_parameters() const override { return parameters_; }
    flx_string call(const flxv_map& arguments) override;

private:
    static flxv_map convert_json_schema_to_openai_params(const flxv_map& schema);
};

class mcp_tool_provider : public i_llm_tool_provider {
    std::shared_ptr<mcp::client> mcp_client_;
    std::vector<std::shared_ptr<mcp_function_adapter>> adapters_;
    flx_string provider_name_;

public:
    explicit mcp_tool_provider(
        std::shared_ptr<mcp::client> client,
        const flx_string& name = "MCP Tools"
    );

    std::vector<i_llm_function*> get_available_tools() override;
    bool refresh_tools() override;
    flx_string get_provider_name() const override { return provider_name_; }
    bool is_available() const override;

    size_t tool_count() const { return adapters_.size(); }
};

}

#endif

#endif
