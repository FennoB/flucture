# MCP Integration Status

Stand: 2025-11-14

## Übersicht

Integration des Model Context Protocol (MCP) in flucture für dynamisches Tool-Discovery im LLM Chat System.

## Ziel

LLM Chat System soll Tools von MCP-Servern nutzen können, sodass das Model immer die aktuellsten verfügbaren Tools hat ohne manuelle Registrierung.

## Architektur

```
flx_llm_chat
  ├─> manual_tool_provider (statische Tools)
  └─> mcp_tool_provider (dynamische MCP Tools)
       ├─> mcp::client (cpp-mcp library)
       │    └─> HTTP/SSE Transport
       └─> mcp_function_adapter (wrapper: mcp::tool → i_llm_function)
```

## Implementierte Komponenten

### 1. MCP Adapter Layer (✅ Komplett)

**Files:**
- `api/mcp/flx_mcp_adapter.h`
- `api/mcp/flx_mcp_adapter.cpp`

**Funktionalität:**
- Bidirektionale Konvertierung: `flx_variant` ↔ `mcp::json`
- Keine cpp-mcp Dependencies im Header
- Unterstützt alle Typen: string, int, bool, double, vector, map, null
- Nested structures

**Tests:** 6 Szenarien, 92 Assertions - alle bestanden ✅

### 2. Tool Provider Interfaces (✅ Komplett)

**File:** `aiprocesses/chat/flx_llm_tool_provider.h`

**Klassen:**
```cpp
class i_llm_tool_provider {
    virtual std::vector<i_llm_function*> get_available_tools() = 0;
    virtual bool refresh_tools() = 0;
    virtual flx_string get_provider_name() const = 0;
    virtual bool is_available() const = 0;
};

class manual_tool_provider : public i_llm_tool_provider {
    // Für statische, manuell registrierte Tools
    void register_function(std::shared_ptr<i_llm_function> func);
    void unregister_function(const flx_string& name);
};
```

**Design:**
- SOLID principles (Interface Segregation, Dependency Inversion)
- Loose coupling zwischen LLM und Tool-Quellen
- Erweiterbar für weitere Provider (Plugins, REST APIs, etc.)

### 3. MCP Tool Provider (✅ Implementiert, Build pending)

**Files:**
- `aiprocesses/chat/flx_mcp_tool_provider.h`
- `aiprocesses/chat/flx_mcp_tool_provider.cpp`

**Klassen:**

```cpp
class mcp_function_adapter : public i_llm_function {
    // Wraps mcp::tool als i_llm_function
    // - Konvertiert MCP JSON Schema → OpenAI Function Parameters
    // - Führt Tool-Calls via mcp::client aus
    // - Formatiert MCP result → string für LLM
};

class mcp_tool_provider : public i_llm_tool_provider {
    // Provider backed by MCP client
    // - Lädt Tools via client->get_tools()
    // - Erstellt mcp_function_adapter für jedes Tool
    // - refresh_tools() updated Tool-Liste dynamisch
};
```

**Features:**
- Automatisches Tool Discovery
- Schema-Konvertierung (JSON Schema → OpenAI)
- Error Handling
- Connection Status Check

### 4. LLM Chat Integration (✅ Komplett)

**Modified Files:**
- `aiprocesses/chat/flx_llm_chat.h`
- `aiprocesses/chat/flx_llm_chat.cpp`

**Änderungen:**

```cpp
class flx_llm_chat {
    std::vector<std::shared_ptr<i_llm_tool_provider>> tool_providers;  // NEU

    void register_tool_provider(std::shared_ptr<i_llm_tool_provider> provider);  // NEU

private:
    i_llm_function* find_function(const flx_string& name);  // NEU: sucht in allen Providern
    std::vector<i_llm_function*> get_function_list_for_api();  // ERWEITERT: sammelt aus allen Providern
};
```

**Verhalten:**
- Sammelt Tools aus allen registrierten Providern
- Refresh bei jedem Chat-Request (immer aktuelle Tools)
- Backwards compatible (manuelle Funktionen funktionieren weiter)

### 5. Build System Integration (✅ Komplett)

**CMakeLists.txt:**

```cmake
option(FLUCTURE_ENABLE_MCP "Enable MCP support" OFF)

if(FLUCTURE_ENABLE_MCP)
  # Findet cpp-mcp in /home/fenno/cpp-mcp oder ${MCP_ROOT}
  # Fügt hinzu:
  #   - api/mcp/flx_mcp_adapter.cpp
  #   - aiprocesses/chat/flx_mcp_tool_provider.cpp
  # Linked gegen: libmcp.a
  # Define: FLX_ENABLE_MCP
endif()
```

**Build Commands:**
```bash
# Mit MCP:
cmake -S . -B build -DFLUCTURE_ENABLE_MCP=ON
cmake --build build -j8

# Ohne MCP (default):
cmake -S . -B build
cmake --build build -j8
```

## Usage Example

```cpp
#ifdef FLX_ENABLE_MCP
#include "mcp_sse_client.h"
#include "aiprocesses/chat/flx_mcp_tool_provider.h"
#endif

// Setup LLM Chat
auto openai_api = std::make_shared<openai_api>(api_key);
flx::llm::flx_llm_chat chat(openai_api);

// Manuelle Tools (wie bisher)
chat.register_function(my_custom_tool);

#ifdef FLX_ENABLE_MCP
// MCP Server connecten
auto mcp_client = std::make_shared<mcp::sse_client>("localhost", 8080);
mcp_client->initialize("My LLM Client", "1.0.0");

// MCP Tool Provider registrieren
auto mcp_provider = std::make_shared<flx::llm::mcp_tool_provider>(
    mcp_client,
    "Filesystem Tools"
);
chat.register_tool_provider(mcp_provider);
#endif

// Chat - nutzt automatisch alle verfügbaren Tools
flx_string response;
chat.chat("List files in /tmp", response);
```

## Offene Punkte

### Sofort notwendig:
1. **Build fixen** - letzter Build-Versuch mit Exit 137 (OOM?)
2. **Tests schreiben** für MCP Tool Provider
3. **Schema-Konvertierung testen** (JSON Schema → OpenAI Format)

### Nice-to-have:
1. **Tool refresh strategy** - aktuell bei jedem Request, evtl. cached mit TTL?
2. **Error Handling** - bessere Fehler-Messages für Tool Call failures
3. **Logging** - Debug-Ausgaben für Tool Discovery
4. **stdio_client Support** - zusätzlich zu sse_client
5. **Tool filtering** - Optional nur bestimmte Tools vom Provider nutzen

## Dependencies

### Compile-Time:
- cpp-mcp library (in `/home/fenno/cpp-mcp`)
- C++17
- nlohmann/json (included in cpp-mcp)

### Runtime (bei -DFLUCTURE_ENABLE_MCP=ON):
- Laufender MCP Server (HTTP/SSE oder stdio)
- OpenAI API Key (für LLM Chat)

## File Structure

```
flucture/
├── api/
│   └── mcp/
│       ├── flx_mcp_adapter.h          # Variant ↔ JSON conversion
│       └── flx_mcp_adapter.cpp
├── aiprocesses/
│   └── chat/
│       ├── flx_llm_chat.h             # MODIFIED: Tool Provider support
│       ├── flx_llm_chat.cpp           # MODIFIED
│       ├── flx_llm_tool_provider.h    # NEW: Provider interfaces
│       ├── flx_mcp_tool_provider.h    # NEW: MCP implementation
│       └── flx_mcp_tool_provider.cpp  # NEW
├── tests/
│   └── test_mcp_adapter.cpp           # MCP adapter tests (passing)
├── CMakeLists.txt                      # MODIFIED: MCP integration
└── docs/
    └── MCP_INTEGRATION_STATUS.md       # This file
```

## Next Steps

1. **Build-Problem debuggen:**
   - Exit 137 = OOM oder Signal
   - Evtl. einzeln kompilieren: `g++ -c flx_mcp_tool_provider.cpp ...`
   - Memory usage prüfen während Build

2. **Integration testen:**
   - MCP Test-Server starten
   - Tool Provider registrieren
   - Chat durchführen mit Tool Call
   - Verify Tool wurde aufgerufen

3. **Dokumentation erweitern:**
   - Code-Beispiele in CLAUDE.md
   - API-Referenz für Tool Provider
   - MCP Server Setup Guide

## Design Decisions

### Warum kein Transport-Layer?
- Ursprünglich geplant: generische `transport_channel` Abstraktion
- **Entschieden:** Direkt cpp-mcp nutzen
- **Grund:** cpp-mcp macht Transport intern (HTTP/SSE/stdio)
- **Vorteil:** Weniger Code, nutzt bewährte Implementation

### Warum Tool Provider Pattern?
- **Separation of Concerns:** LLM Chat kennt nur Interface
- **Open/Closed Principle:** Neue Provider ohne Chat-Änderung
- **Dependency Inversion:** Chat depends auf Abstraction, nicht Implementierung
- **Erweiterbar:** Plugins, REST APIs, DB-backed Tools, etc.

### Warum Wrapper statt direkt cpp-mcp?
- **Lose Kopplung:** LLM System unabhängig von MCP
- **Type Safety:** flx_variant statt void* oder nlohmann::json
- **Consistent API:** Gleiche Interfaces für alle Tool-Quellen
- **Optional:** MCP kann aus-kompiliert werden

## Known Issues

- Build mit MCP aktiviert noch nicht erfolgreich (OOM?)
- Schema-Konvertierung JSON Schema → OpenAI noch nicht getestet
- SSE Event-Stream noch nicht implementiert (wird für Notifications gebraucht)

## References

- MCP Spec: https://spec.modelcontextprotocol.io/
- cpp-mcp: /home/fenno/cpp-mcp
- flucture MCP Analysis: /home/fenno/Projects/flucture/docs/FLUCTURE_MCP_INTEGRATION_ANALYSIS.md
