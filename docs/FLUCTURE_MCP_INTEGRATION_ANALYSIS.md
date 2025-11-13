# Flucture Framework Analysis for MCP Integration

## Project Overview

**Flucture** is an AI-powered PDF layout extraction framework written in C++17 that enables:
- PDF rendering with hierarchical layouts (Layout → PDF)
- PDF semantic structure extraction (PDF → Layout)
- AI-powered evaluation of extraction quality
- Database ORM integration with PostgreSQL
- REST API HTTP server with JSON/XML support

**Location**: `/home/fenno/Projects/flucture/`

---

## 1. Overall Project Structure

```
/home/fenno/Projects/flucture/
├── utils/                      # Core framework (5 files, ~1.2K LOC)
│   ├── flx_model.{h,cpp}       # Property system & variant-based models (579h + 341c)
│   ├── flx_variant.{h,cpp}     # Type-safe variant storage (176h + 514c)
│   ├── flx_lazy_ptr.h          # Reference-counted lazy pointers
│   ├── flx_string.{h,cpp}      # Enhanced string utilities
│   └── flx_datetime.{h,cpp}    # UTC datetime with millisecond precision
│
├── api/                        # API integrations & protocols
│   ├── server/                 # HTTP server (libmicrohttpd)
│   │   ├── flx_httpdaemon.{h,cpp}  (63h + 221c) - Main server daemon
│   │   ├── flx_rest_api.{h,cpp}    - REST endpoint handler
│   │   └── libmicrohttpd_x86_64/   - Embedded library
│   │
│   ├── client/                 # HTTP client (libcurl)
│   │   └── flx_http_request.{h,cpp} (155h + 345c) - Client wrapper
│   │
│   ├── json/                   # JSON processing
│   │   ├── flx_json.{h,cpp}    (38h + 169c) - JSON ↔ flx_variant conversion
│   │   └── json.hpp            - nlohmann/json header-only library
│   │
│   ├── db/                     # Database ORM (PostgreSQL)
│   │   ├── db_repository.h     - Generic repository pattern
│   │   ├── pg_connection.{h,cpp} - PostgreSQL implementation
│   │   ├── pg_query.{h,cpp}    - Query execution
│   │   ├── db_query_builder.{h,cpp} - SQL builder
│   │   ├── db_search_criteria.{h,cpp} - Search filters
│   │   └── flx_semantic_embedder.{h,cpp} - Vector embeddings
│   │
│   └── xml/                    # XML processing
│       └── flx_xml.{h,cpp}     - XML ↔ flx_variant conversion
│
├── documents/                  # Document processing
│   ├── layout/                 # Layout structure classes
│   ├── pdf/                    # PDF processing (PoDoFo 1.0.0)
│   └── qr/                     # QR code generation
│
├── aiprocesses/                # AI-powered processes
│   ├── eval/                   # Evaluation systems
│   ├── chat/                   # LLM chat integration
│   └── snippets/               # Code snippet management
│
├── tests/                      # Catch2 test suite (30+ files, 200K+ LOC)
├── build/                      # CMake build output
├── docs/                       # Documentation files
├── CMakeLists.txt              - Build configuration
├── CLAUDE.md                   - Detailed architecture documentation
└── main.cpp                    - Executable entry point
```

---

## 2. Target Modules for MCP Integration

### A. flx_http_daemon (HTTP Server)

**Location**: `/home/fenno/Projects/flucture/api/server/`

**Files**:
- Header: `flx_httpdaemon.h` (63 lines)
- Implementation: `flx_httpdaemon.cpp` (221 lines)

**Key Classes & Interfaces**:

```cpp
class flx_http_daemon
{
public:
    // Core methods
    bool exec(int port);                    // Start daemon on port
    void stop();                            // Stop daemon
    bool is_running();                      // Check status

    // SSL support
    bool check_ssl_supported();
    void activate_ssl(flx_string privatekey, flx_string certificate);
    void activate_thread_pool(size_t threads);

    // Request/Response structures
    struct request {
        flx_string path;
        flx_string method;
        flx_string body;
        std::map<flx_string, flx_string> headers;
        std::map<flx_string, flx_string> params;
    };

    struct response {
        flx_string body;
        std::map<flx_string, flx_string> headers;
        int statuscode = 0;
    };

    // Override to handle requests
    virtual response handle(request req);
};
```

**Technology Stack**:
- **Library**: libmicrohttpd (embedded, x86_64)
- **Threading**: Configurable thread pool
- **Security**: SSL/TLS support
- **Data Format**: JSON/XML compatible with flx_variant

**Usage Pattern**:
```cpp
class MyServer : public flx_http_daemon {
    virtual response handle(request req) override {
        response resp;
        if (req.path == "/api/data") {
            resp.body = "response data";
            resp.statuscode = 200;
        }
        return resp;
    }
};

MyServer server;
server.activate_thread_pool(4);
server.exec(8080);
```

---

### B. flx_http_request (HTTP Client)

**Location**: `/home/fenno/Projects/flucture/api/client/`

**Files**:
- Header: `flx_http_request.h` (155 lines)
- Implementation: `flx_http_request.cpp` (345 lines)

**Key Methods**:

```cpp
class flx_http_request {
public:
    // Constructors
    flx_http_request();
    explicit flx_http_request(const flx_string& url);

    // Configuration
    void set_url(const flx_string& url);
    void set_method(const flx_string& method);  // GET, POST, PUT, DELETE
    void set_header(const flx_string& key, const flx_string& value);
    void set_param(const flx_string& key, const flx_string& value);
    void set_body(const flx_string& body);

    // Execution
    bool send();                                // Send request synchronously
    bool download_to_file(const flx_string& output_path);

    // Response access
    int get_status_code() const;
    flx_string get_response_body() const;
    const flxv_map& get_response_headers() const;
    flx_string get_error_message() const;

    // Map access for headers/params
    flxv_map& get_headers();
    flxv_map& get_params();
};
```

**Technology Stack**:
- **Library**: libcurl
- **Data Format**: flx_variant-based headers/params/body
- **Features**: URL encoding, header handling, file downloads

**Usage Pattern**:
```cpp
flx_http_request request("https://api.example.com/data");
request.set_method("POST");
request.set_header("Content-Type", "application/json");
request.set_header("Authorization", "Bearer token");
request.set_body(json_data);

if (request.send()) {
    flx_string response = request.get_response_body();
    int status = request.get_status_code();
} else {
    flx_string error = request.get_error_message();
}
```

---

### C. flx_json (JSON Processing)

**Location**: `/home/fenno/Projects/flucture/api/json/`

**Files**:
- Header: `flx_json.h` (38 lines)
- Implementation: `flx_json.cpp` (169 lines)
- Dependency: `json.hpp` (956KB - nlohmann/json header-only library)

**Key Methods**:

```cpp
class flx_json {
public:
    // Constructor takes a pointer to flxv_map
    explicit flx_json(flxv_map* map_ptr);

    // Bidirectional conversion
    bool parse(const flx_string& json_string);    // JSON string → flxv_map
    flx_string create() const;                     // flxv_map → JSON string
};
```

**Design Pattern**:
- Non-owning pointer pattern (caller manages map lifetime)
- Direct integration with flx_variant system
- Automatic type conversion during parse/create

**Usage Pattern**:
```cpp
flxv_map data;
flx_json json(&data);

// Parse JSON
flx_string json_str = R"({"name": "Test", "age": 30})";
if (json.parse(json_str)) {
    flx_string name = data["name"].string_value();
    long long age = data["age"].int_value();
}

// Create JSON
data["status"] = flx_variant("active");
data["count"] = flx_variant(42LL);
flx_string output = json.create();
```

---

### D. flx_variant (Type-Safe Variant Storage)

**Location**: `/home/fenno/Projects/flucture/utils/`

**Files**:
- Header: `flx_variant.h` (176 lines)
- Implementation: `flx_variant.cpp` (514 lines)

**Supported Types**:

```cpp
enum state {
    none,           // Null/uninitialized
    string_state,   // flx_string
    int_state,      // long long
    bool_state,     // bool
    double_state,   // double
    vector_state,   // flxv_vector (vector<flx_variant>)
    map_state       // flxv_map (map<flx_string, flx_variant>)
};
```

**Key Methods**:

```cpp
class flx_variant {
public:
    // Constructors (auto-detect type)
    flx_variant();
    flx_variant(const char* str);
    flx_variant(const flx_string& str);
    flx_variant(int value);
    flx_variant(long long value);
    flx_variant(bool value);
    flx_variant(double value);
    flx_variant(const flxv_vector& vec);
    flx_variant(const flxv_map& map);

    // Type checking
    state in_state() const;
    bool is_null() const;
    bool is_string() const;
    bool is_int() const;
    bool is_bool() const;
    bool is_double() const;
    bool is_vector() const;
    bool is_map() const;

    // Type conversions (auto-convert)
    flx_string& to_string();
    long long& to_int();
    bool& to_bool();
    double& to_double();
    flxv_vector& to_vector();
    flxv_map& to_map();

    // Template-based conversion
    template<typename type> type& to();

    // Type aliases
    typedef std::vector<flx_variant> flxv_vector;
    typedef std::map<flx_string, flx_variant> flxv_map;
};
```

**Type Safety Features**:
- Automatic type detection during construction
- Safe conversions with automatic type coercion
- Nested container support (vector of variants, maps)
- Type macros for convenience

**Usage Pattern**:
```cpp
flxv_map data;
data["count"] = flx_variant(42LL);
data["name"] = flx_variant("John");
data["active"] = flx_variant(true);

// Type checking
if (data["count"].is_int()) {
    long long val = data["count"].int_value();
}

// Type conversion
flxv_vector vec;
vec.push_back(flx_variant(1LL));
vec.push_back(flx_variant("two"));
vec.push_back(flx_variant(3.0));
```

---

### E. flx_model (Property System & Models)

**Location**: `/home/fenno/Projects/flucture/utils/`

**Files**:
- Header: `flx_model.h` (579 lines)
- Implementation: `flx_model.cpp` (341 lines)

**Core Concepts**:

```cpp
class flx_model {
    // Properties declared with macros
    // Each property is:
    // - A C++ variable (not a method)
    // - Automatically managed via flx_variant
    // - Supports null values
    // - Database-mappable with metadata
};

// Property declaration macros
flxp_int(name)                          // long long
flxp_double(name)                       // double
flxp_bool(name)                         // bool
flxp_string(name)                       // flx_string
flxp_vector(name)                       // flxv_vector
flxp_map(name)                          // flxv_map
flxp_model(name, ChildType)             // Nested model
flxp_model_list(name, ChildType)        // Collection of models

// With database metadata (optional, basic properties only)
flxp_int(id, {{"column", "user_id"}, {"primary_key", "users"}})
flxp_string(email, {{"column", "email_address"}})
flxp_int(company_id, {{"column", "company_id"}, {"foreign_key", "companies"}})
```

**Key Classes**:

```cpp
// Base interface for all properties
class flx_property_i {
public:
    flx_string prop_name() const;
    const flxv_map& get_meta() const;
    virtual flx_variant::state get_variant_type() const = 0;
    virtual bool is_relation() const { return false; }
    virtual flx_variant& access();         // Non-const (creates defaults)
    virtual const flx_variant& const_access() const;  // Const (throws if null)
    bool is_null() const;
};

// Typed property template
template <typename type>
class flx_property : public flx_property_i {
public:
    type& value();                         // Non-const: creates defaults
    const type& value() const;             // Const: throws if null
    type& operator*();                     // Dereference
    type* operator->();                   // Arrow operator
    flx_property& operator=(const type& val);  // Assignment
    bool operator==(const type& other) const; // Comparison
};
```

**Design Principles**:

1. **Properties work like variables**: `model.x = 5.0` (not `model.x(5.0)`)
2. **Automatic null handling**: Null on creation, default on non-const access, exception on const access
3. **Database integration**: Optional metadata for ORM mapping
4. **Lazy initialization**: No allocation until first use
5. **Const-correctness**: Different behavior in const vs non-const context

**Usage Pattern**:

```cpp
class User : public flx_model {
public:
    flxp_int(id, {{"column", "id"}, {"primary_key", "users"}});
    flxp_string(name, {{"column", "username"}});
    flxp_string(email, {{"column", "email"}});
    flxp_int(company_id, {{"column", "company_id"}, {"foreign_key", "companies"}});
};

class Order : public flx_model {
public:
    flxp_int(id, {{"column", "id"}, {"primary_key", "orders"}});
    flxp_int(user_id, {{"column", "user_id"}, {"foreign_key", "users"}});
    flxp_double(total, {{"column", "total_amount"}});
    flxp_model(user, User);                // Child reference
    flxp_model_list(items, OrderItem);     // Collection
};

// Usage
User user;
user.id = 123;                             // Direct assignment
user.name = "John Doe";
long long id_val = user.id;                // Direct access

const User& const_user = user;
if (const_user.name.is_null()) { /*...*/ } // Safe null check
```

**Exception Handling**:

```cpp
class flx_null_field_exception : public std::exception {
    // Thrown when accessing null property in const context
};

class flx_null_access_exception : public std::exception {
    // Thrown when dereferencing null lazy pointer in const context
};
```

---

## 3. Module Relationships & Dependencies

```
flx_model
  ├─> flx_variant (property storage)
  │   ├─> flx_string (string values)
  │   ├─> flxv_vector (container)
  │   └─> flxv_map (container)
  └─> flx_lazy_ptr (memory management)

flx_http_daemon
  └─> flx_string (request/response data)

flx_http_request
  ├─> flx_variant (headers/params/body)
  ├─> flx_string (URLs, bodies)
  └─> flxv_map (headers/params storage)

flx_json
  ├─> flx_variant (data structure)
  ├─> flx_string (JSON strings)
  ├─> flxv_map (JSON objects)
  ├─> flxv_vector (JSON arrays)
  └─> json.hpp (nlohmann/json library)

flx_xml
  ├─> flx_variant (data structure)
  ├─> flx_string (XML strings)
  ├─> flxv_map (XML elements)
  └─> pugixml (XML library)
```

---

## 4. Summary of Target Module Locations

| Module | Location | Files | LOC | Purpose |
|--------|----------|-------|-----|---------|
| **flx_http_daemon** | `/api/server/` | .h + .cpp | 284 | HTTP server (libmicrohttpd) |
| **flx_http_request** | `/api/client/` | .h + .cpp | 500 | HTTP client (libcurl) |
| **flx_json** | `/api/json/` | .h + .cpp | 207 | JSON ↔ variant conversion |
| **flx_variant** | `/utils/` | .h + .cpp | 690 | Type-safe variant storage |
| **flx_model** | `/utils/` | .h + .cpp | 920 | Property system & models |

**Total for 5 modules**: ~2,601 lines of code

---

## 5. Integration Points for MCP

For Model Context Protocol integration, we need to consider:

1. **Data Model Interface**: Use `flx_model` as base for protocol messages
2. **HTTP Transport**: Leverage `flx_http_daemon` for server, `flx_http_request` for client
3. **Serialization**: Use `flx_json` for protocol encoding (JSON-RPC)
4. **Type System**: Leverage `flx_variant` for flexible message payloads
5. **Properties**: Use metadata system for schema definition

### MCP-Specific Requirements

**JSON-RPC 2.0 Support**:
- Request/Response message format
- Notification handling (no response expected)
- Error object structure
- Batch requests

**Server-Sent Events (SSE)**:
- Required for MCP server → client notifications
- Need to extend `flx_http_daemon` with SSE support
- Event stream format: `data: {...}\n\n`

**MCP Protocol Messages**:
- Tool registration and invocation
- Resource management (prompts, resources)
- Capability negotiation
- Session management

---

## 6. Wrapper Architecture Strategy

To keep flucture independent from cpp-mcp library:

### Layer 1: MCP Core Protocol (use cpp-mcp)
- JSON-RPC message parsing/creation
- MCP message type definitions
- Protocol validation logic

### Layer 2: Flucture Adapters (custom wrapper)
- Convert cpp-mcp types ↔ flx_variant
- Adapt transport interfaces
- Bridge serialization layer

### Layer 3: Flucture Integration (native implementation)
- `flx_mcp_server` using `flx_http_daemon`
- `flx_mcp_client` using `flx_http_request`
- SSE support for notifications
- JSON-RPC over HTTP using `flx_json`

This ensures:
- No cpp-mcp dependencies in flucture headers
- Clean separation of concerns
- Easy to swap out cpp-mcp if needed
- Type-safe integration with flx_variant
