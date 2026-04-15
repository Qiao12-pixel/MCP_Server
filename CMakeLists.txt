cmake_minimum_required(VERSION 3.24)
project(MCP_Server)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES arm64)

# 加载 Conan
include(${CMAKE_BINARY_DIR}/build/Debug/generators/conan_toolchain.cmake)

find_package(Threads REQUIRED)
find_package(nlohmann_json REQUIRED)
find_package(spdlog REQUIRED)
find_package(CURL REQUIRED)
find_package(GTest REQUIRED)
find_package(httplib REQUIRED)

enable_testing()

add_executable(MCP_Server
        main.cpp
        knowledge/json.cpp
        knowledge/spdlog.cpp
        knowledge/http_server.cpp
        knowledge/http_client.cpp
        src/logger/logger.cpp
        src/logger/logger.h
        src/config/config.cpp
        src/config/config.h
        src/json_rpc/jsonrpc.h
        src/json_rpc/jsonrpc_serialization.cpp
        src/json_rpc/jsonrpc_serialization.h
        src/json_rpc/http_jsonrpc.cpp
        src/json_rpc/http_jsonrpc.h
        src/json_rpc/stdio_jsonrpc.cpp
)

target_compile_definitions(MCP_Server PRIVATE
        MCP_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(MCP_Server PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        CURL::libcurl
        httplib::httplib
)
# 用于测试模块是不是能够正常工作
add_executable(mcp_tests
        tests/config_load_from_file_test.cpp
        tests/logger_initialization_test.cpp
        src/logger/logger.cpp
        src/config/config.cpp
)

target_include_directories(mcp_tests PRIVATE
        src/logger
        src/config
)

target_compile_definitions(mcp_tests PRIVATE
        MCP_PROJECT_ROOT="${CMAKE_SOURCE_DIR}"
)

target_link_libraries(mcp_tests PRIVATE
        nlohmann_json::nlohmann_json
        spdlog::spdlog
        GTest::gtest
        GTest::gtest_main
)

add_test(NAME mcp_tests COMMAND mcp_tests)
