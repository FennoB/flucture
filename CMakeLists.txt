cmake_minimum_required(VERSION 3.15) # Recommended for FetchContent (for Catch2)

project(flucture_suite LANGUAGES CXX) # Project name adjusted as we build multiple artifacts

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- Define source files ---
# Separate main.cpp from library sources
set(APP_MAIN_SOURCE main.cpp)

# List of source and header files for the core library
# It's often better to list files explicitly instead of using file(GLOB),
# as CMake doesn't always automatically detect changes through GLOB.
set(FLUCTURE_CORE_SOURCES
  utils/flx_model.cpp
  utils/flx_variant.cpp
  utils/flx_datetime.cpp
  utils/flx_string.cpp
  utils/flx_env.cpp
  documents/layout/flx_layout_bounds.cpp
  documents/layout/flx_layout_text.cpp
  documents/layout/flx_layout_image.cpp
  documents/layout/flx_layout_vertex.cpp
  documents/layout/flx_layout_geometry.cpp
  documents/flx_doc_sio.cpp
  documents/flx_layout_to_html.cpp
  api/aimodels/flx_openai_api.cpp
  documents/pdf/flx_pdf_sio.cpp
  documents/pdf/flx_pdf_text_extractor.cpp
  api/server/flx_rest_api.cpp
  api/server/flx_httpdaemon.cpp
  api/json/flx_json.cpp
  api/client/flx_http_request.cpp
  api/db/pg_connection.cpp
  api/db/pg_query.cpp
  api/db/db_query_builder.cpp
  api/db/db_search_criteria.cpp
  api/db/flx_semantic_embedder.cpp
  aiprocesses/chat/flx_llm_api.cpp
  aiprocesses/flx_ai_process.cpp

  aiprocesses/eval/flx_ai_process_evaluator.cpp
  aiprocesses/eval/flx_layout_evaluator.cpp
  aiprocesses/chat/flx_llm_chat.cpp
  aiprocesses/chat/flx_chat_snippet_source.cpp
  aiprocesses/snippets/flx_snippet.cpp
  aiprocesses/snippets/flx_snippet_source.cpp
  documents/qr/flx_qr_style.cpp
  documents/qr/flx_qr_generator.cpp
  documents/qr/qrcodegen.cpp
)

set(FLUCTURE_CORE_HEADERS
  utils/flx_string.h
  utils/flx_model.h
  utils/flx_variant.h
  utils/flx_datetime.h
  utils/flx_env.h
  utils/flx_lazy_ptr.h
  documents/layout/flx_layout_bounds.h
  documents/layout/flx_layout_text.h
  documents/layout/flx_layout_image.h
  documents/layout/flx_layout_vertex.h
  documents/layout/flx_layout_geometry.h
  documents/flx_layout_to_html.h
  api/json/json.hpp # Header-only library
  api/aimodels/flx_openai_api.h
  # Presumably header-only or part of flx_http_request.cpp
  documents/flx_doc_sio.h
  documents/pdf/flx_pdf_sio.h
  documents/pdf/flx_pdf_text_extractor.h
  documents/pdf/podofo_config.h # Configuration header for PoDoFo
  api/server/flx_httpdaemon.h
  api/server/flx_rest_api.h
  documents/pdf/flx_pdf_coords.h
  api/json/flx_json.h
  api/client/flx_http_request.h
  api/db/db_connection.h
  api/db/db_query.h
  api/db/db_query_builder.h
  api/db/db_search_criteria.h
  api/db/db_repository.h
  api/db/flx_semantic_embedder.h
  api/db/pg_connection.h
  api/db/pg_query.h
  aiprocesses/chat/flx_llm_api.h
  aiprocesses/chat/flx_llm_chat_interfaces.h
  aiprocesses/flx_ai_process.h
  aiprocesses/eval/flx_ai_process_evaluator.h
  aiprocesses/eval/flx_layout_evaluator.h
  aiprocesses/chat/flx_llm_chat.h
  aiprocesses/chat/flx_chat_snippet_source.h
  aiprocesses/snippets/flx_snippet.h
  aiprocesses/snippets/flx_snippet_source.h
  documents/qr/flx_qr_style.h
  documents/qr/flx_qr_generator.h
  documents/qr/qrcodegen.hpp
)

# --- PoDoFo Setup ---
# These settings are specific for the PoDoFo build.
set(PODOFO_BUILD_LIB_ONLY TRUE CACHE BOOL "" FORCE)
set(PODOFO_BUILD_STATIC TRUE CACHE BOOL "" FORCE)
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/documents/pdf/podofo-master)

# --- OpenCV ---
find_package(OpenCV REQUIRED) #

# libcurl
find_package(CURL REQUIRED) #

# openssl
find_package(OpenSSL REQUIRED) #

# libpqxx (PostgreSQL C++ client library)
find_package(PkgConfig REQUIRED)
pkg_check_modules(PQXX REQUIRED libpqxx)

# pugixml (XML parser library) - OPTIONAL
pkg_check_modules(PUGIXML pugixml)

# MCP (Model Context Protocol library) - OPTIONAL
option(FLUCTURE_ENABLE_MCP "Enable MCP (Model Context Protocol) support" OFF)

if(FLUCTURE_ENABLE_MCP)
  # Try to find cpp-mcp library
  # First check if MCP_ROOT is set (for custom installations)
  if(DEFINED ENV{MCP_ROOT})
    set(MCP_ROOT $ENV{MCP_ROOT})
  elseif(EXISTS "${CMAKE_SOURCE_DIR}/../cpp-mcp")
    set(MCP_ROOT "${CMAKE_SOURCE_DIR}/../cpp-mcp")
  elseif(EXISTS "/home/fenno/cpp-mcp")
    set(MCP_ROOT "/home/fenno/cpp-mcp")
  endif()

  if(MCP_ROOT AND EXISTS "${MCP_ROOT}/include/mcp_server.h")
    message(STATUS "Found cpp-mcp at: ${MCP_ROOT}")
    set(MCP_INCLUDE_DIRS
      "${MCP_ROOT}/include"
      "${MCP_ROOT}/common"
    )
    set(MCP_LIBRARY_DIR "${MCP_ROOT}/build/src")
    set(MCP_LIBRARIES "mcp")
    set(MCP_FOUND TRUE)

    # Add MCP source files to the core library
    message(STATUS "Adding MCP source files to flucture_core")
    list(APPEND FLUCTURE_CORE_SOURCES
      api/mcp/flx_mcp_adapter.cpp
      aiprocesses/chat/flx_mcp_tool_provider.cpp
    )
    list(APPEND FLUCTURE_CORE_HEADERS
      api/mcp/flx_mcp_adapter.h
      aiprocesses/chat/flx_llm_tool_provider.h
      aiprocesses/chat/flx_mcp_tool_provider.h
    )
  else()
    message(WARNING "cpp-mcp not found - MCP support disabled. Set MCP_ROOT to cpp-mcp installation directory.")
    set(MCP_FOUND FALSE)
  endif()
endif()

# Add XML support if pugixml is available (check AFTER pkg_check_modules)
if(PUGIXML_FOUND)
  message(STATUS "Adding XML source files to flucture_core")
  list(APPEND FLUCTURE_CORE_SOURCES api/xml/flx_xml.cpp)
  list(APPEND FLUCTURE_CORE_HEADERS api/xml/flx_xml.h)
endif()

# --- flucture_core Library ---
add_library(flucture_core ${FLUCTURE_CORE_SOURCES} ${FLUCTURE_CORE_HEADERS})

# Enable XML support if pugixml is available
if(PUGIXML_FOUND)
  target_compile_definitions(flucture_core PUBLIC FLX_ENABLE_XML)
endif()

# Enable MCP support if cpp-mcp is available
if(FLUCTURE_ENABLE_MCP AND MCP_FOUND)
  target_compile_definitions(flucture_core PUBLIC FLX_ENABLE_MCP)
endif()

# Include paths for the flucture_core library
# Use CMAKE_CURRENT_SOURCE_DIR to work correctly when added as subdirectory
target_include_directories(flucture_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR} # Allows includes relative to project root (e.g. "utils/flx_string.h")
  ${CMAKE_CURRENT_SOURCE_DIR}/documents/pdf/podofo-master/src #
  ${CMAKE_CURRENT_SOURCE_DIR}/documents/pdf/podofo-master/src/podofo #
  ${CMAKE_CURRENT_SOURCE_DIR}/documents/pdf/podofo-master/3rdparty #
  /usr/include/freetype2/ #
  ${CMAKE_CURRENT_SOURCE_DIR}/api/client/libmicrohttpd_x86_64/include #
  ${OpenCV_INCLUDE_DIRS} #
  ${CURL_INCLUDE_DIRS} #
  ${OPENSSL_INCLUDE_DIR} #
  ${PQXX_INCLUDE_DIRS} #
)

# Add pugixml include dirs if available
if(PUGIXML_FOUND)
  target_include_directories(flucture_core PUBLIC ${PUGIXML_INCLUDE_DIRS})
endif()

# Add MCP include dirs if available
if(FLUCTURE_ENABLE_MCP AND MCP_FOUND)
  target_include_directories(flucture_core PUBLIC ${MCP_INCLUDE_DIRS})
  target_link_directories(flucture_core PUBLIC ${MCP_LIBRARY_DIR})
endif()

# Library paths for libmicrohttpd (an Imported Target would be more modern)
# Use CMAKE_CURRENT_SOURCE_DIR to work correctly when added as subdirectory
target_link_directories(flucture_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/api/client/libmicrohttpd_x86_64/lib #
)

# Link dependencies to the flucture_core library
target_link_libraries(flucture_core PUBLIC
  podofo::podofo #
  m              #
  freetype       #
  microhttpd     #
  ${OpenCV_LIBS} #
  CURL::libcurl  #
  OpenSSL::SSL   #
  ${PQXX_LIBRARIES} #
)

# Link pugixml if available
if(PUGIXML_FOUND)
  target_link_libraries(flucture_core PUBLIC ${PUGIXML_LIBRARIES})
endif()

# Link MCP if available
if(FLUCTURE_ENABLE_MCP AND MCP_FOUND)
  target_link_libraries(flucture_core PUBLIC ${MCP_LIBRARIES})
endif()

# --- flucture executable ---
add_executable(flucture ${APP_MAIN_SOURCE})
target_link_libraries(flucture PRIVATE flucture_core)

# --- PDF to Layout API executable (for production use) ---
add_executable(pdf_to_layout pdf_to_layout_api.cpp)
target_link_libraries(pdf_to_layout PRIVATE flucture_core)

# --- QR Code Generator Tool ---
add_executable(qr_tool qr_tool.cpp)
target_link_libraries(qr_tool PRIVATE flucture_core)

# --- Simple Vergabefix QR Generator ---
add_executable(qr_vergabefix qr_vergabefix.cpp documents/qr/qrcodegen.cpp)
target_include_directories(qr_vergabefix PRIVATE ${CMAKE_CURRENT_SOURCE_DIR} ${OpenCV_INCLUDE_DIRS})
target_link_libraries(qr_vergabefix PRIVATE ${OpenCV_LIBS})


# --- Installation (remains for the executable) ---
include(GNUInstallDirs) #
install(TARGETS flucture
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
) #

# --- Test Setup (e.g. with Catch2) ---
option(BUILD_FLUCTURE_TESTS "Build tests for flucture" ON)

if(BUILD_FLUCTURE_TESTS)
    message(STATUS "Building tests for flucture...")
    include(FetchContent)
    FetchContent_Declare(
        Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2.git
        GIT_TAG v3.6.0 # Check for the latest stable version
    )
    FetchContent_MakeAvailable(Catch2)

    # Auto-discover all test files with pattern test_*.cpp (including subdirectories)
    file(GLOB_RECURSE TEST_SOURCES "tests/test_*.cpp")
    
    # Add main test runner (contains CATCH_CONFIG_MAIN)
    add_executable(flucture_tests
        tests/test_main.cpp
        ${TEST_SOURCES}
    )
    target_link_libraries(flucture_tests PRIVATE flucture_core Catch2::Catch2WithMain)

    include(CTest)
    add_test(NAME UnitTests COMMAND flucture_tests)
else()
    message(STATUS "Skipping build of flucture tests (BUILD_FLUCTURE_TESTS is OFF).")
endif()
