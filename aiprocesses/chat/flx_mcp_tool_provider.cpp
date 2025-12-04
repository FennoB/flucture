#include "flx_mcp_tool_provider.h"

#ifdef FLX_ENABLE_MCP

#include "mcp_client.h"
#include "mcp_tool.h"
#include "api/mcp/flx_mcp_adapter.h"
#include "api/json/flx_json.h"

namespace flx::llm {

mcp_function_adapter::mcp_function_adapter(
    const flx_string& name,
    const flx_string& description,
    const flxv_map& input_schema,
    std::shared_ptr<mcp::client> client
)
    : name_(name)
    , description_(description)
    , mcp_client_(client)
{
    parameters_ = convert_json_schema_to_openai_params(input_schema);
}

flx_string mcp_function_adapter::call(const flxv_map& arguments) {
    if (!mcp_client_ || !mcp_client_->is_running()) {
        return "Error: MCP client not connected";
    }

    mcp::json mcp_args;
    if (!mcp_adapter::map_to_mcp_json(arguments, &mcp_args)) {
        return "Error: Failed to convert arguments";
    }

    try {
        mcp::json result = mcp_client_->call_tool(name_.c_str(), mcp_args);

        if (result.contains("content") && result["content"].is_array()) {
            flx_string combined_result;
            for (const auto& content_block : result["content"]) {
                if (content_block.contains("text")) {
                    combined_result = combined_result + content_block["text"].get<std::string>().c_str();
                }
            }
            return combined_result;
        }

        flxv_map result_map;
        if (mcp_adapter::mcp_json_to_map(&result, result_map)) {
            flx_json json_converter(&result_map);
            return json_converter.create();
        }

        return "Error: Invalid result format";
    }
    catch (const std::exception& e) {
        return flx_string("Error calling MCP tool: ") + e.what();
    }
}

flxv_map mcp_function_adapter::convert_json_schema_to_openai_params(const flxv_map& schema) {
    flxv_map openai_params;
    openai_params["type"] = flx_variant("object");

    if (schema.count("properties")) {
        openai_params["properties"] = schema.at("properties");
    }

    if (schema.count("required")) {
        openai_params["required"] = schema.at("required");
    }

    return openai_params;
}

mcp_tool_provider::mcp_tool_provider(
    std::shared_ptr<mcp::client> client,
    const flx_string& name
)
    : mcp_client_(client)
    , provider_name_(name)
{
    if (mcp_client_) {
        refresh_tools();
    }
}

std::vector<i_llm_function*> mcp_tool_provider::get_available_tools() {
    std::vector<i_llm_function*> tools;
    tools.reserve(adapters_.size());
    for (auto& adapter : adapters_) {
        tools.push_back(adapter.get());
    }
    return tools;
}

bool mcp_tool_provider::refresh_tools() {
    if (!mcp_client_ || !mcp_client_->is_running()) {
        return false;
    }

    try {
        std::vector<mcp::tool> mcp_tools = mcp_client_->get_tools();

        adapters_.clear();
        adapters_.reserve(mcp_tools.size());

        for (const auto& mcp_tool : mcp_tools) {
            flx_string name(mcp_tool.name.c_str());
            flx_string description(mcp_tool.description.c_str());

            flxv_map input_schema;
            mcp::json schema_json = mcp_tool.parameters_schema;
            mcp_adapter::mcp_json_to_map(&schema_json, input_schema);

            auto adapter = std::make_shared<mcp_function_adapter>(
                name, description, input_schema, mcp_client_
            );

            adapters_.push_back(adapter);
        }

        return true;
    }
    catch (const std::exception&) {
        return false;
    }
}

bool mcp_tool_provider::is_available() const {
    return mcp_client_ && mcp_client_->is_running();
}

}

#endif
