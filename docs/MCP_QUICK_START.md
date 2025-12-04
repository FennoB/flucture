# MCP Integration Quick Start

## Build mit MCP Support

```bash
cd /home/fenno/Projects/flucture
cmake -S . -B build -DFLUCTURE_ENABLE_MCP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build -j8
```

## Usage

### 1. MCP Server starten

```bash
# Beispiel: Filesystem MCP Server
npx -y @modelcontextprotocol/server-filesystem /tmp
```

### 2. In C++ Code

```cpp
#include "aiprocesses/chat/flx_llm_chat.h"
#include "api/aimodels/flx_openai_api.h"

#ifdef FLX_ENABLE_MCP
#include "mcp_sse_client.h"
#include "aiprocesses/chat/flx_mcp_tool_provider.h"
#endif

int main() {
    auto api = std::make_shared<openai_api>("your-api-key");
    flx::llm::flx_llm_chat chat(api);
    
    flxv_map settings;
    settings["model"] = "gpt-4";
    settings["temperature"] = 0.7;
    chat.create_context(settings);
    
#ifdef FLX_ENABLE_MCP
    auto mcp = std::make_shared<mcp::sse_client>("localhost", 8080);
    if (mcp->initialize("LLM Client", "1.0.0")) {
        auto provider = std::make_shared<flx::llm::mcp_tool_provider>(mcp);
        chat.register_tool_provider(provider);
    }
#endif
    
    flx_string response;
    if (chat.chat("List files in /tmp", response)) {
        std::cout << response.c_str() << std::endl;
    }
    
    return 0;
}
```

## Nächste Schritte

1. Build-Problem fixen (Exit 137)
2. Integration testen
3. Weitere Provider implementieren
