//
// Created by Joe on 26-4-14.
//

#include "http_jsonrpc.h"
#include "jsonrpc_serialization.h"
#include "../config/config.h"
#include "../logger/logger.h"

#include <httplib.h>
#include <nlohmann/json.hpp>

namespace mcp {
    namespace json_rpc {
        class HttpJsonRpcServer::Impl {
        public:
            httplib::Server server;

            /**
             * @brief 设置日志回调
             * server.set_logger([](const httplib::Request& req, const httplib::Response& res) {
             *     MCP_LOG_INFO("HTTP {} {} -> {}", req.method, req.path, res.status);
             * });
             */
            Impl() {
                server.set_error_handler([](const httplib::Request& /*req*/, httplib::Response&res) {
                    json error_response = {
                        {"jsonrpc", "2.0"},
                        {"error", {
                            {"code", -32603},
                            {"message", "Internal server error"}
                        }},
                        {"id", nullptr}
                    };
                    res.set_content(error_response.dump(), "application/json");
                });
            }
        };

        /**
         * @brief 使用配置文件
         * @param dispatcher 调度器
         */
        HttpJsonRpcServer::HttpJsonRpcServer(JsonRpcDispatcher dispatcher)
            :HttpJsonRpcServer(std::move(dispatcher), "0.0.0.0", MCP_CONFIG.GetServerPort()){
            MCP_LOG_INFO("HTTP JSON_RPC server initialized from config");
        }

        HttpJsonRpcServer::HttpJsonRpcServer(JsonRpcDispatcher dispatcher, const std::string &host, int port)
            : m_dispatcher_(std::move(dispatcher)), m_host_(host), m_port_(port), m_pimpl_(std::make_unique<Impl>()) {
            MCP_LOG_INFO("HTTP JSON_RPC server created on {}:{}", m_host_, m_port_);
            //注册POST/jsonrpc端点
            m_pimpl_->server.Post("/jsonrpc", [this](const httplib::Request& req, httplib::Response& res) {
                MCP_LOG_INFO("Received JSON_RPC request, body size: {}", req.body.size());

                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
                res.set_header("Access-Control-Allow-Headers", "Content-Type");

                try {
                    std::string response = HandleRequest(req.body);
                    res.set_content(response, "application/json");
                    res.status = 200;

                } catch (const std::exception& e) {
                    MCP_LOG_ERROR("Error handling request: {}", e.what());
                    json error_response = {
                        {"jsonrpc", "2.0"},
                        {"error", {
                            {"code", -32603},
                            {"message", e.what()}
                        }},
                        {"id", nullptr}
                    };
                    res.set_content(error_response.dump(), "application/json");
                    res.status = 500;
                }
            });
            //处理Options请求
            m_pimpl_->server.Options("/jsonrpc", [](const httplib::Request& /*req*/, httplib::Response& res) {
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_header("Access-Control-Allow-Methods", "POST, OPTIONS");
                res.set_header("Access-Control-Allow-Headers", "Content-Type");
                res.status = 204;
            });

            //注册健康节点
            m_pimpl_->server.Get("/health", [](const httplib::Request& /*req*/,httplib::Response& res) {
                json health = {
                    {"status", "ok"},
                    {"service", "mcp-http-jsonrpc"}
                };
                res.set_content(health.dump(), "application/json");
            });

            //注册根节点
            m_pimpl_->server.Get("/", [this](const httplib::Request& /*req*/, httplib::Response& res) {
                json info = {
                    {"service", "MCP HTTP JSON-RPC Server"},
                    {"version", "1.0.0"},
                    {"endpoints", {
                        {{"path", "/jsonrpc"}, {"method", "POST"}, {"description", "JSON-RPC 2.0 endpoint"}},
                        {{"path", "/health"}, {"method", "GET"}, {"description", "Health check"}},
                        {{"path", "/sse/events"}, {"method", "GET"}, {"description", "Server status event stream (SSE)"}},
                        {{"path", "/sse/tool_calls"}, {"method", "GET"}, {"description", "Tool call monitoring stream (SSE)"}},
                        {{"path", "/"}, {"method", "GET"}, {"description", "Server information"}}
                    }}
                };
            });
        }



        void HttpJsonRpcServer::RegisterSseEndpoint(const std::string &path, SseCallback callback) {
                m_pimpl_->server.Get(path, [callback](const httplib::Request& /*req*/, httplib::Response& res) {
                res.set_header("Content-Type", "text/event-stream");
                res.set_header("Cache-Control", "no-cache");
                res.set_header("Connection", "keep-alive");
                res.set_header("Access-Control-Allow-Origin", "*");
                res.set_header("X-Accel-Buffering", "no");

                res.set_chunked_content_provider(
                    "text/event-stream",
                    [callback](size_t /*offset*/, httplib::DataSink& sink) {
                        auto send_event = [&sink](const std::string& data) {
                            std::string event = "data: " + data + "\n\n";
                            sink.write(event.c_str(), event.size());
                        };
                        try {
                            callback(send_event);
                        } catch (const std::exception& e) {
                            MCP_LOG_ERROR("SSE callback error: {}", e.what());
                        }
                        return true;
                    }
                );
            });
        }
        HttpJsonRpcServer::~HttpJsonRpcServer() {
            stop();
        }

        void HttpJsonRpcServer::run() {
            if (m_running_.exchange(true)) {
                MCP_LOG_WARN("Server is already running...");
                return;
            }
            MCP_LOG_INFO("Starting HTTP JSON-RPC server on {}:{}", m_host_, m_port_);

            //启动服务器[阻塞]
            if (!m_pimpl_->server.listen(m_host_, m_port_)) {
                m_running_ = false;
                throw std::runtime_error("Failed to start HTTP server on " + m_host_ + ":" + std::to_string(m_port_));
            }

            MCP_LOG_INFO("HTTP JSON_RPC server stopped");
            m_running_ = false;
        }

        void HttpJsonRpcServer::stop() {
            if (!m_running_.exchange(false)) {
                return;
            }
            MCP_LOG_INFO("Stopping HTTP JSON-RPC server....");
            m_pimpl_->server.stop();
        }

        /**
         * @brief 将客户端发送的request_body进行解析
         * @param request_body
         * @return
         */
        std::string HttpJsonRpcServer::HandleRequest(const std::string &request_body) {
            MCP_LOG_DEBUG("Request body: {}", request_body);

            try {
                json request_json = json::parse(request_body);
                //检查是否为批量请求
                if (request_json.is_array()) {
                    json batch_response = json::array();

                    for (const auto& single_req_json : request_json) {
                        try {
                            //从json解析请求
                            JsonRpcRequest req;
                            single_req_json.at("jsonrpc").get_to(req.jsonrpc);
                            single_req_json.at("method").get_to(req.method);

                            if (single_req_json.contains("id")) {
                                // single_req_json.at("id").get_to(*req.id);//会存在解引用失败
                                req.id = single_req_json["id"];
                            } if (single_req_json.contains("params")) {
                                // single_req_json.at("params").get_to(*req.params);//会存在解引用失败
                                req.params = single_req_json["params"];
                            }

                            //处理请求-无需响应
                            if (!req.id.has_value()) {//id没有值
                                //通知请求，无需响应
                                if (m_dispatcher_.HasHandler(req.method)) {
                                    m_dispatcher_.Call(req.method, req.params.value_or(json::object()));//如果 params 盒子里有东西，就用它；如果盒子是空的，就塞给它一个“空对象” {}。
                                }
                                continue;
                            }

                            //处理请求-要有响应===>构造响应 response
                            JsonRpcResponse resp;
                            resp.jsonrpc = "2.0";
                            resp.id = *req.id;

                            try {
                                if (!m_dispatcher_.HasHandler(req.method)) {
                                    resp.error = JsonRpcError {
                                        jsonrpc_errc::MethodNotFound,
                                        "Merhod not found: " + req.method,
                                        std::nullopt
                                    };
                                } else {
                                    resp.result = m_dispatcher_.Call(req.method, req.params.value_or(json::object()));
                                }
                            } catch (const std::exception& e) {
                                resp.error = JsonRpcError {
                                    jsonrpc_errc::InternalError,
                                    e.what(),
                                    std::nullopt
                                };
                            }
                            //序列化响应

                            json resp_json = {
                                {"jsonrpc", "2.0"},
                                {"id", resp.id}
                            };

                            if (resp.result.has_value()) {
                                resp_json["result"] = *resp.result;
                            } else if (resp.error.has_value()) {//第一次判断是为了决定“回不回错误消息”。
                                                                //第二次判断是为了决定“错误消息够不够详细”并防止程序崩溃。
                                resp_json["error"] = {
                                    {"code", resp.error->code},
                                    {"message", resp.error->message}
                                };
                                if (resp.error->data.has_value()) {
                                    resp_json["error"]["data"] = *resp.error->data;
                                }
                            }

                            batch_response.emplace_back(resp_json);
                        } catch (const std::exception& e) {
                            MCP_LOG_ERROR("Error in batch request: {}", e.what());
                            json error_resp = {
                                {"jsonrpc", "2.0"},
                                {"error", {
                                    {"code", jsonrpc_errc::InternalError},
                                    {"message", e.what()}
                                }},
                                {"id", nullptr}
                            };
                            batch_response.push_back(error_resp);
                        }
                    }
                    std::string response = batch_response.dump();
                    MCP_LOG_DEBUG("Batch response: {}", response);
                    return response;
                }

                //处理单个请求
                JsonRpcRequest request;
                request_json.at("jsonrpc").get_to(request.jsonrpc);
                request_json.at("method").get_to(request.method);//at 如果键不存在，抛出异常。

                if (request_json.contains("id")) {
                    // request_json.at("id").get_to(*request.id);//存在解引用失败的情况
                    request.id = request_json["id"];//在 const json 对象上，如果键不存在会抛出异常；但在非 const 对象上，如果键不存在，它会创建一个 null 节点。
                } if (request_json.contains("params")) {
                    request.params = request_json["params"];
                }
                //如果是无id（notify）执行后不返回响应
                if (!request.id.has_value()) {
                    if (m_dispatcher_.HasHandler(request.method)) {
                        m_dispatcher_.Call(request.method, request.params.value_or(json::object()));
                    }
                    MCP_LOG_DEBUG("Norification request, no response");
                    return "";
                }

                //构造响应---有id
                JsonRpcResponse response;
                response.jsonrpc = "2.0";
                response.id = *request.id;

                try {
                    if (!m_dispatcher_.HasHandler(request.method)) {
                        response.error = {
                            jsonrpc_errc::MethodNotFound,
                            "Method not found" + request.method,
                            std::nullopt
                        };
                    } else {
                        response.result = m_dispatcher_.Call(request.method, request.params.value_or(json::object()));
                    }
                } catch (const std::exception& e) {
                    response.error = JsonRpcError {
                        jsonrpc_errc::InternalError,
                        e.what(),
                        std::nullopt

                    };
                }

                //序列化响应
                json response_json = {
                    {"jsonrpc", "2.0"},
                    {"id", *request.id}

                };
                if (response.result.has_value()) {
                    response_json["result"] = *response.result;
                } else if (response.error.has_value()) {
                    response_json["error"] = {
                        {"code", response.error->code},
                        {"message", response.error->message}

                    };
                    if (response.error->data.has_value()) {
                        response_json["error"]["data"] = *response.error->data;
                    }
                }

                std::string response_str = response_json.dump();
                MCP_LOG_DEBUG("Response: {}", response_str);

                return response_str;
            } catch (const json::parse_error& e) {
                MCP_LOG_ERROR("JSON parse error", e.what());
                json error_response = {
                    {"jsonrpc", "2.0"},
                    {"error",
                        {"code", jsonrpc_errc::ParseError},
                        {"message", std::string("Parse error:") + e.what()}
                    },
                    {"id", nullptr}
                };
                return error_response.dump();
            } catch (const std::exception& e) {
                MCP_LOG_ERROR("Error handling request: {}", e.what());
                json error_response = {
                    {"jsonrpc", "2.0"},
                    {"error",
                        {"code", jsonrpc_errc::InternalError},
                        {"message", std::string("Internal error: ") + e.what()}
                    },
                    {"data", nullptr}
                };
                return error_response.dump();
            }
        }
    }
}