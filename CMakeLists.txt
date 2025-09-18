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
  documents/layout/flx_layout_bounds.cpp
  documents/layout/flx_layout_text.cpp
  documents/layout/flx_layout_image.cpp
  documents/layout/flx_layout_vertex.cpp
  documents/layout/flx_layout_geometry.cpp
  documents/flx_doc_sio.cpp
  api/aimodels/flx_openai_api.cpp
  documents/pdf/flx_pdf_sio.cpp
  documents/pdf/flx_pdf_text_extractor.cpp
  api/server/flx_rest_api.cpp
  api/server/flx_httpdaemon.cpp
  api/json/flx_json.cpp
  api/client/flx_http_request.cpp
  aiprocesses/chat/flx_llm_api.cpp
  aiprocesses/flx_ai_process.cpp

  aiprocesses/eval/flx_ai_process_evaluator.cpp
  aiprocesses/eval/flx_layout_evaluator.cpp
  aiprocesses/chat/flx_llm_chat.cpp
  aiprocesses/chat/flx_chat_snippet_source.cpp
  aiprocesses/snippets/flx_snippet.cpp
  aiprocesses/snippets/flx_snippet_source.cpp
)

set(FLUCTURE_CORE_HEADERS
  utils/flx_string.h
  utils/flx_model.h
  utils/flx_variant.h
  utils/flx_datetime.h
  utils/flx_lazy_ptr.h
  documents/layout/flx_layout_bounds.h
  documents/layout/flx_layout_text.h
  documents/layout/flx_layout_image.h
  documents/layout/flx_layout_vertex.h
  documents/layout/flx_layout_geometry.h
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
  aiprocesses/chat/flx_llm_api.h
  aiprocesses/chat/flx_llm_chat_interfaces.h
  aiprocesses/flx_ai_process.h
  aiprocesses/eval/flx_ai_process_evaluator.h
  aiprocesses/eval/flx_layout_evaluator.h
  aiprocesses/chat/flx_llm_chat.h
  aiprocesses/chat/flx_chat_snippet_source.h
  aiprocesses/snippets/flx_snippet.h
  aiprocesses/snippets/flx_snippet_source.h
)

# --- PoDoFo Setup ---
# These settings are specific for the PoDoFo build.
set(PODOFO_BUILD_LIB_ONLY TRUE CACHE BOOL "" FORCE) #
set(PODOFO_BUILD_STATIC TRUE CACHE BOOL "" FORCE) #
add_subdirectory(${CMAKE_SOURCE_DIR}/documents/pdf/podofo-master) #

# --- OpenCV ---
find_package(OpenCV REQUIRED) #

# libcurl
find_package(CURL REQUIRED) #

# openssl
find_package(OpenSSL REQUIRED) #

# --- flucture_core Library ---
add_library(flucture_core ${FLUCTURE_CORE_SOURCES} ${FLUCTURE_CORE_HEADERS})

# Include paths for the flucture_core library
target_include_directories(flucture_core PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR} # Allows includes relative to project root (e.g. "utils/flx_string.h")
  ${CMAKE_SOURCE_DIR}/documents/pdf/podofo-master/src #
  ${CMAKE_SOURCE_DIR}/documents/pdf/podofo-master/src/podofo #
  ${CMAKE_SOURCE_DIR}/documents/pdf/podofo-master/3rdparty #
  /usr/include/freetype2/ #
  ${CMAKE_SOURCE_DIR}/api/client/libmicrohttpd_x86_64/include #
  ${OpenCV_INCLUDE_DIRS} #
  ${CURL_INCLUDE_DIRS} #
  ${OPENSSL_INCLUDE_DIR} #
)

# Library paths for libmicrohttpd (an Imported Target would be more modern)
target_link_directories(flucture_core PUBLIC
  ${CMAKE_SOURCE_DIR}/api/client/libmicrohttpd_x86_64/lib #
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
)

# --- flucture executable ---
add_executable(flucture ${APP_MAIN_SOURCE})
target_link_libraries(flucture PRIVATE flucture_core)

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

    # Auto-discover all test files with pattern test_*.cpp
    file(GLOB TEST_SOURCES "tests/test_*.cpp")
    
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
