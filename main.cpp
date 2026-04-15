#include <exception>
#include <iostream>
#include <string>

#include "src/config/config.h"
#include "src/json_rpc/http_jsonrpc.h"
#include "src/logger/logger.h"

namespace mcp {

    void InitLoggerFromConfig(const mcp::Config& config) {
        MCP_LOG_INIT(
            "mcp",
            config.GetLogFilePath(),
            config.GetLogFileSize(),
            static_cast<size_t>(config.GetLogFileCount()),
            config.GetLogConsoleOutput()
        );

        const auto level = config.GetLogLevel();
        if (level == "trace") {
            MCP_LOG_SET_LEVEL(spdlog::level::trace);
        } else if (level == "debug") {
            MCP_LOG_SET_LEVEL(spdlog::level::debug);
        } else if (level == "info") {
            MCP_LOG_SET_LEVEL(spdlog::level::info);
        } else if (level == "warn") {
            MCP_LOG_SET_LEVEL(spdlog::level::warn);
        } else if (level == "error") {
            MCP_LOG_SET_LEVEL(spdlog::level::err);
        } else if (level == "critical") {
            MCP_LOG_SET_LEVEL(spdlog::level::critical);
        }
    }

    mcp::json_rpc::JsonRpcDispatcher BuildDispatcher() {
        mcp::json_rpc::JsonRpcDispatcher dispatcher;

        dispatcher.RegisterHandler("ping", [](const mcp::json_rpc::json& /*params*/) {
            return mcp::json_rpc::json{
                {"message", "pong"}
            };
        });

        dispatcher.RegisterHandler("server.info", [](const mcp::json_rpc::json& /*params*/) {
            return mcp::json_rpc::json{
                {"name", "MCP_Server"},
                {"protocol", "json-rpc-2.0"},
                {"transport", "http"}
            };
        });

        dispatcher.RegisterHandler("echo", [](const mcp::json_rpc::json& params) {
            return params;
        });
        return dispatcher;
}

}  // namespace

int main() {
    const std::string config_path = "../config/server.json";

    try {
        auto& config = mcp::Config::GetInstance();
        if (!config.LoadFromFile(config_path)) {
            std::cerr << "Failed to load config file: " << config_path << std::endl;
            return 1;
        }

        InitLoggerFromConfig(config);
        MCP_LOG_INFO("Config loaded successfully from {}", config_path);

        auto dispatcher = mcp::BuildDispatcher();
        mcp::json_rpc::HttpJsonRpcServer server(std::move(dispatcher));

        MCP_LOG_INFO("HTTP JSON-RPC server is ready on port {}", config.GetServerPort());
        server.run();
        return 0;
    } catch (const std::exception& e) {
        auto logger = mcp::logger::Logger::getInstance().getLogger();
        if (logger) {
            MCP_LOG_ERROR("Fatal error in main: {}", e.what());
            MCP_LOG_SHUTDOWN();
        } else {
            std::cerr << "Fatal error in main: " << e.what() << std::endl;
        }
        return 1;
    }
}
