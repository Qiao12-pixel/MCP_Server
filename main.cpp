/**
* @file main.cpp
 * @brief MCP_Server的注册
 *
 * @author Joe
 * @date 26-4-18
 */

#include "src/config/config.h"
#include "src/json_rpc/jsonrpc.h"
#include "src/json_rpc/http_jsonrpc.h"
// #include "src/json_rpc/jsonrpc_serialization.h"
#include "src/logger/logger.h"
#include "src/mcp/mcp_server.h"

#include <iostream>
#include <csignal>//信号处理：处理 Ctrl+C、程序退出信号
#include <atomic>
#include <thread>
#include <memory>
#include <ctime>
#include <fstream>// 文件读写：打开、读、写本地文件
#include <sstream>// 文件读写：打开、读、写本地文件



using namespace mcp::mcp_inter;
using namespace mcp::logger;
static std::atomic<bool> g_running{true};
static std::unique_ptr<mcp::json_rpc::HttpJsonRpcServer> g_http_server{nullptr};


//信号处理函数
void signal_handler(int signal) {
    std::cerr << "\nReceived Signal " << signal << ", shutting down..." << std::endl;
    g_running = false;
    if (g_http_server) {
        g_http_server->stop();
    }
}

// 字符串转日志级别
spdlog::level::level_enum StringToLogLevel(const std::string& level_str) {
    if (level_str == "trace") {
        return spdlog::level::trace;
    } else if (level_str == "debug") {
        return spdlog::level::debug;
    } else if (level_str == "info") {
        return spdlog::level::info;
    } else if (level_str == "warn") {
        return spdlog::level::warn;
    } else if (level_str == "error") {
        return spdlog::level::err;
    } else if (level_str == "critical") {
        return spdlog::level::critical;
    }
    //Default
    return spdlog::level::info;
}




// 注册 MCP 工具、资源、提示词
void setup_mcp_server(McpServer& mcp) {

    //***************************************注册工具***************************************
    //Echo工具
    {
        Tool tool;
        tool.name = "echo";
        tool.description = "Echo back the input message";
        tool.input_schema.properties = {
            {"message",{{"type", "string"}, {"description", "Message to echo"}}}
        };
        tool.input_schema.required = {"message"};

        mcp.RegisterTool(tool, [](const json& args) -> ToolResult {
            ToolResult result;
            result.content.push_back(ContentItem{
                .type = "text",
                .text = "Echo: " + args.at("message").get<std::string>()
            });
            return result;
        });
    }
    //Calculate工具
    {
        mcp::mcp_inter::Tool tool;
        tool.name = "calculate";
        tool.description = "Perform basic arithmetic operations";
        tool.input_schema.properties = {
            {"operation", {{"type", "string"}, {"enum", json::array({"add", "subtract", "multiply", "divide"})}}},
            {"a", {"type", "string"}},
            {"b", {"type", "string"}}
        };
        tool.input_schema.required = {"operation", "a", "b"};

        mcp.RegisterTool(tool, [](const json& args) -> ToolResult {
            std::string opera = args.at("operation").get<std::string>();
            double a = args.at("a").get<double>();
            double b = args.at("b").get<double>();
            double result_value = 0;

            if (opera == "add") {
                result_value = a + b;
            } else if (opera == "subtract") {
                result_value = a - b;
            } else if (opera == "multiply") {
                result_value = a * b;
            } else if (opera == "divide") {
                if (b == 0) {
                    ToolResult error;
                    error.is_error = true;
                    error.content.push_back(ContentItem{
                        .type = "text",
                        .text = "Error: Division by zero"
                    });
                    return error;
                }
                result_value = a / b;
            }

            ToolResult result;
            result.content.push_back(ContentItem{
                .type = "text",
                .text = std::to_string(result_value)
            });
            return result;
        });
    }

    //GetTime工具
    {
        Tool tool;
        tool.name = "get_time";
        tool.description = "Get Current system time";
        tool.input_schema.properties = json::object();

        mcp.RegisterTool(tool, [](const json& /*args*/) ->ToolResult {
            std::time_t now = std::time(nullptr);
            char buff[100];
            std::strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

            ToolResult result;
            result.content.push_back(ContentItem{
                .type = "text",
                .text = buff
            });
            return result;
        });
    }
    //GetWeather工具
    {
        Tool tool;
        tool.name = "get_weather";
        tool.description = "Get weather information for a city";
        tool.input_schema.properties = {
            {"city", {{"type", "string"}, {"description", "City name (e.g. Beijing, Shanghai)"}}}
        };
        tool.input_schema.required = {"city"};

        mcp.RegisterTool(tool, [](const json &args) -> ToolResult {
            std::string city = args.at("city").get<std::string>();

            //先模拟天气数据
            std::time_t now = std::time(nullptr);
            char time_buff[100];
            std::strftime(time_buff, sizeof(time_buff), "%Y-%m-%d %H:%M:%S", std::localtime(&now));

            std::ostringstream weather_info;
            weather_info << "Weather Report for " << city << "\n";
            weather_info << "========================\n";
            weather_info << "Time: " << time_buff << "\n";
            weather_info << "Temperature: 22°C\n";
            weather_info << "Condition: Sunny\n";
            weather_info << "Humidity: 45%\n";
            weather_info << "Wind: 5 km/h NE\n";
            weather_info << "\n(Note: This is simulated data. Integrate with real weather API for production)";

            ToolResult result;
            result.content.push_back(ContentItem{
                .type = "text",
                .text = weather_info.str()
            });
            return result;
        });
    }
    //WriteFile工具
    {
        Tool tool;
        tool.name = "write_file";
        tool.description = "Write content to a file";
        tool.input_schema.properties = {
            {"path", {{"type", "string"}, {"description", "File path to a write to"}}},
            {"content",{{"type", "string"}, {"description", "Content to write to the file"}}}
        };
        tool.input_schema.required = {"path", "content"};
        mcp.RegisterTool(tool, [](const json& args) -> ToolResult {
            std::string path = args.at("path").get<std::string>();
            std::string content = args.at("content").get<std::string>();

            ToolResult result;

            try {
                std::ofstream file(path);//创建文件【若不存在】
                if (!file.is_open()) {
                    result.is_error = true;
                    result.content.push_back(ContentItem{
                        .type = "text",
                        .text = "Error: Failed to open file: " + path
                    });
                    return result;
                }
                file << content;
                file.close();

                result.content.push_back(ContentItem{
                    .type = "text",
                    .text = "Successfully wrote to file: " + path
                });
            } catch (const std::exception& e) {
                result.is_error = true;
                result.content.push_back(ContentItem{
                    .type = "text",
                    .text = std::string("Error writing file: ") + e.what()
                });
            }
            return result;
        });
    }
    //***************************************注册资源***************************************

    //系统资源
    {
        Resource res;
        res.url = "system://info";
        res.name = "System Information";
        res.description = "Basic system information";
        /*
         * "application/json"：JSON 数据
         * "text/plain"：纯文本
         * "image/png"：图片
         */
        res.mime_type = "text/plain";

        mcp.RegisterResource(res, [](const std::string& url) -> ResourceContent {
            ResourceContent content;
            content.url = url;
            content.mime_type = "text/plain";
            std::ostringstream oss;
            oss << "MCP Server - System Info\n";
            oss << "========================\n";
            std::time_t now = std::time(nullptr);
            char buff[100];
            std::strftime(buff, sizeof(buff), "%Y-%m-%d %H:%M:%S", std::localtime(&now));
            oss << "Time: " << buff << "\n";

            content.text = oss.str();
            return content;
        });
    }
    //服务器配置
    {
        Resource res;
        res.url = "config://server";
        res.name = "Server Configuration";
        res.mime_type = "application/json";

        mcp.RegisterResource(res, [](const json& url) -> ResourceContent {
            ResourceContent content;
            content.url = url;
            content.mime_type = "application/json";
            content.text = json({
                {"port" , MCP_CONFIG.GetServerPort()},
                {"log_level", MCP_CONFIG.GetLogLevel()}

            }).dump(2);
            return content;
        });
    }
    //***************************************注册提示词***************************************
    //代码审查
    {
        Prompt prompt;
        prompt.name = "code_review";
        prompt.description = "Generate code review prompt";
        prompt.arguments.push_back(PromptArgument{
            .name = "code",
            .required = true
        });
        prompt.arguments.push_back(PromptArgument{
            .name = "language",
            .required = true
        });

        mcp.RegisterPrompt(prompt, [](const json& args) -> std::vector<PromptMessage> {
            std::vector<PromptMessage> msgs;
            PromptMessage msg;
            msg.role = Role::User;
            msg.content = {
                {"type", "text"},
                {"text", "Please review this " + args.at("language").get<std::string>() +
                         " code:\n\n" + args.at("code").get<std::string>()}
            };
            msgs.push_back(msg);
            return msgs;
        });
    }
    MCP_LOG_INFO("MCP setup complete: {} tools, {} resources, {} prompts", mcp.ListTools().size(), mcp.ListResource().size(), mcp.ListPrompts().size());
}

//创建JSON-RPC调度器【绑定到MCP服务器中】
mcp::json_rpc::JsonRpcDispatcher create_dispatcher(McpServer& mcp_server) {
    mcp::json_rpc::JsonRpcDispatcher dispatcher;

    //init
    dispatcher.RegisterHandler("init", [&mcp_server](const json& /*params*/) -> json {
        MCP_LOG_INFO("Client Init");
        return mcp_server.GetInitResult().to_json();
    });
    //tools/list
    dispatcher.RegisterHandler("tools/list", [&mcp_server](const json& /*params*/) ->json {
       json tools_str = json::array();
        for (const auto& tool : mcp_server.ListTools()) {
            tools_str.push_back(tool.to_json());
        }
        return {{"tools", tools_str}};
    });

    //tools/call
    dispatcher.RegisterHandler("tools/call", [&mcp_server](const json& params) -> json {
        std::string name = params.at("name").get<std::string>();
        json arguments = params.value("arguments", json::object());
        MCP_LOG_INFO("Calling tool: {}", name);
        auto result = mcp_server.CallTool(name, arguments);
        return result.to_json();
    });
    //resources/list
    dispatcher.RegisterHandler("resources/list", [&mcp_server](const json& /*params*/) -> json {
       json resources_arr = json::array();
        for (const auto& resource : mcp_server.ListResource()) {
            resources_arr.push_back(resource.to_json());
        }
        return {{"resources", resources_arr}};
    });
    //resources/read
    dispatcher.RegisterHandler("resources/read", [&mcp_server](const json& params) -> json {
       std::string url = params.at("url").get<std::string>();
        MCP_LOG_INFO("Reading resource: {}", url);
        auto content = mcp_server.ReadResource(url);
        json contents_arr = json::array();
        contents_arr.push_back(content.to_json());
        return {{"content", contents_arr}};
    });
    //prompts/list
    dispatcher.RegisterHandler("prompts/list", [&mcp_server](const json& /*params*/) -> json {
       json prompt_arr = json::array();
        for (const auto& prompt : mcp_server.ListPrompts()) {
            prompt_arr.push_back(prompt.to_json());
        }
        return {{"prompts", prompt_arr}};
    });
    //prompts/get
    dispatcher.RegisterHandler("prompts/get", [&mcp_server](const json& params) -> json {
       std::string name = params.at("name").get<std::string>();
        json arguments = params.value("arguments", json::object());
        MCP_LOG_INFO("Getting prompt: {}", name);
        auto messages = mcp_server.GetPrompt(name, arguments);
        json messages_arr = json::array();
        for (const auto& msg : messages) {
            messages_arr.push_back(msg.to_json());
        }
        return {{"messages", messages_arr}};
    });
    return dispatcher;
}

//HHTP模式
void run_http_mode(McpServer& mcp_server, const std::string& host, int port) {
    MCP_LOG_INFO("Starting HTTP server on {}:{}", host, port);
    auto dispatcher = create_dispatcher(mcp_server);
    g_http_server = std::make_unique<mcp::json_rpc::HttpJsonRpcServer>(std::move(dispatcher), host, port);
    //SSE 事件队列
    struct {
        std::vector<json> events;
        std::mutex mutex;
        std::condition_variable cv;
    } event_queue;

    //设置MCP服务器的SSE回调
    mcp_server.SetSseCallback([&event_queue](const json& event) {
        std::lock_guard<std::mutex> lock(event_queue.mutex);
        event_queue.events.push_back(event);
        event_queue.cv.notify_all();
    });

    //注册SSE端点 - 服务器事件流
    g_http_server->RegisterSseEndpoint("/sse/events", [&mcp_server](const auto& send) {
        MCP_LOG_INFO("SSE events client connected");
        send(json({{"type", "connected"}, {"message", "Server events stream"}}));

        int count = 0;

        while (g_running.load()) {
            json status = {
                {"type", "server_status"},
                {"timestamp", std::time(nullptr)},
                {"tools_count", mcp_server.ListTools().size()},
                {"resources_count", mcp_server.ListResource().size()},
                {"prompts_count", mcp_server.ListPrompts().size()},
                {"uptime_seconds", count++}
            };
            send(status.dump());
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }

        MCP_LOG_INFO("SSE events client disconnected");
    });

    //注册 SSE端点 - 工具调用实时流
    g_http_server->RegisterSseEndpoint("/sse/tool_calls", [&event_queue](const auto& send) {
        MCP_LOG_INFO("SSE tool_calls client connected");
        send(json({{"type", "connected"}, {"message", "tool calls monitoring"}}).dump());
        while (g_running.load()) {
            std::unique_lock<std::mutex> lock(event_queue.mutex);

            //等待事件或超时
            event_queue.cv.wait_for(lock, std::chrono::seconds(1), [&event_queue] {
               return !event_queue.events.empty();//队列事件中不为空就醒来；最多等一秒
            });

            //发送所有待处理事件
            for (const auto& event : event_queue.events) {
                send(event.dump());
            }
            event_queue.events.clear();
        }
        MCP_LOG_INFO("SSE tool_calls client disconnected");
    });

    g_http_server->run();
    MCP_LOG_INFO("HTTP server stopped");
}


//stdio模式
void run_stdio_mode(McpServer& mcp_server) {
    MCP_LOG_INFO("Starting stdio server");

    auto dispatcher = create_dispatcher(mcp_server);
    mcp::json_rpc::StdioJsonRpcServer stdio_server(dispatcher);

    stdio_server.run();
    MCP_LOG_INFO("stdio server stopped");
}
//同时运行2中模式
void run_both_mode(McpServer& mcp_server, const std::string& host, int port) {
    MCP_LOG_INFO("Starting both HTTP and stdio servers");

    std::thread http_thread([&mcp_server, &host, port]() {
       run_http_mode(mcp_server, host, port);
    });
    run_stdio_mode(mcp_server);

    if (http_thread.joinable()) {
        http_thread.join();
    }

}
int main(int argc, char* argv[]) {
    //解析命令行参数
    std::string config_file = "../config/server.json";
    std::string mode= "http";
    std::string host;
    int port = 0;

    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        if (arg == "--config" && i + 1 < argc) {
            config_file = argv[++i];
        } else if (arg == "--mode" && i + 1 < argc) {
            mode = argv[++i];
        } else if (arg == "--host" && i + 1 < argc) {
            host = argv[++i];
        } else if (arg == "--port" && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else if (arg == "--help" || arg == "-h") {
            std::cout << "Usage: " << argv[0] << " [OPTIONS]\n"
                      << "Options:\n"
                      << "  --mode MODE      Server mode: http, stdio, or both (default: http)\n"
                      << "  --config FILE    Configuration file path\n"
                      << "  --host HOST      Server host (for HTTP mode)\n"
                      << "  --port PORT      Server port (for HTTP mode)\n"
                      << "  --help, -h       Show this help\n"
                      << "\nExamples:\n"
                      << "  " << argv[0] << " --mode http --port 8080\n"
                      << "  " << argv[0] << " --mode stdio\n"
                      << "  " << argv[0] << " --mode both --port 8080\n";
            return 0;
        }
    }
    // 验证模式
    if (mode != "http" && mode != "stdio" && mode != "both") {
        std::cerr << "Invalid mode: " << mode << " (must be http, stdio, or both)" << std::endl;
        return 1;
    }

    //加载配资
    if (!MCP_CONFIG.LoadFromFile(config_file)) {
        std::cerr << "Failed to load config from: " << config_file << std::endl;
    }

    if (host.empty()) {
        host = "0.0.0.0";
    }
    if (port == 0) {
        port = MCP_CONFIG.GetServerPort();
    }
    // 初始化日志
    MCP_LOG_INIT("mcp_server", MCP_CONFIG.GetLogFilePath(),
                 MCP_CONFIG.GetLogFileSize(), MCP_CONFIG.GetLogFileCount(),
                 MCP_CONFIG.GetLogConsoleOutput());
    MCP_LOG_SET_LEVEL(StringToLogLevel(MCP_CONFIG.GetLogLevel()));


    if (mode == "http" || mode == "both") {
        MCP_LOG_INFO("HTTP: {}:{}", host, port);
    }


    try {
        // 创建 MCP 服务器实例
        McpServer mcp_server("mcp-server", "1.0.0");

        // 设置能力
        ServerCapabilities capabilities;
        capabilities.tools = ServerCapabilities::ToolsCapability{false}; // 工具能力
        capabilities.resources = ServerCapabilities::ResourceCapability{false, false}; // 资源能力
        capabilities.prompts = ServerCapabilities::PromptCapability{false}; // 提示能力
        mcp_server.SetCapabilities(capabilities);

        // 注册 Tools, Resources, Prompts
        setup_mcp_server(mcp_server);

        // 设置信号处理
        std::signal(SIGINT, signal_handler);
        std::signal(SIGTERM, signal_handler);

        // 根据模式启动服务器
        if (mode == "http") {
            run_http_mode(mcp_server, host, port);
        } else if (mode == "stdio") {
            run_stdio_mode(mcp_server);
        } else if (mode == "both") {
            run_both_mode(mcp_server, host, port);
        }

        MCP_LOG_INFO("Server shutdown complete");

    } catch (const std::exception& e) {
        MCP_LOG_ERROR("Fatal error: {}", e.what());
        std::cerr << "Fatal error: " << e.what() << std::endl;
        return 1;
    }
    MCP_LOG_SHUTDOWN();
    return 0;
}