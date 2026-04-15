/**
 * @file stdio_jsonrpc.cpp
 * @brief 基于stdio的JSON- RPC2.0服务器
 * 通过标准输入输出通信，使用Content-Length头分割信息
 * @author Joe
 * @date 26-4-15
 */
#include "jsonrpc.h"
#include "jsonrpc_serialization.h"
#include "../logger/logger.h"

#include <iostream>
#include <sstream>
#include <limits>
#include <algorithm>
#include <cctype>
#include <stdexcept>


namespace mcp {
    namespace json_rpc {
        //==========================JsonRpcDispatcher - 方法调度器============================

        /**
         * @brief 注册RPC方法处理器
         * @param method
         * @param handler
         */
        void JsonRpcDispatcher::RegisterHandler(const std::string &method, Handler handler) {
            m_handler_[method] = std::move(handler);
        }

        /**
         * @brief 检查方法是否已经注册
         * @param method
         * @return
         */
        bool JsonRpcDispatcher::HasHandler(const std::string &method) const {
            return m_handler_.find(method) != m_handler_.end();
        }

        /**
         * @brief 调用已经注册的方法
         * @param method
         * @param params
         * @return
         */
        json JsonRpcDispatcher::Call(const std::string &method, const json &params) const {
            auto it = m_handler_.find(method);
            if (it == m_handler_.end()) {
                throw std::runtime_error("Method not found");
            }
            return it->second(params);
        }
        //======================================StdioJsonRpcServer======================================
        StdioJsonRpcServer::StdioJsonRpcServer(JsonRpcDispatcher dispatcher) : m_dispatcher_(std::move(dispatcher)) {

        }

        StdioJsonRpcServer::StdioJsonRpcServer(JsonRpcDispatcher dispatcher, std::istream &in, std::ostream &out)
            : m_dispatcher_(std::move(dispatcher)), m_in_(in), m_out_(out){

        }

        /**
         * @brief 读取一条完整的JSON_RPC信息
         * @param out_body
         * @return
         */
        bool StdioJsonRpcServer::ReadMessage(std::string &out_body) {
            /**
             * Content-Length: 128\r\n
             * Content-Type: application/json; charset=utf-8\r\n\r\n
             */
            out_body.clear();

            std::string line;
            size_t content_length = 0;
            bool found_content_length = false;

            //读取头部，大小写不敏感，允许额外头部（Content-Type 等）
            while (std::getline(m_in_, line)) {
                // 每行末尾如果带有回车（Windows 风格 CRLF），手动去掉
                if (!line.empty() && line.back() == '\r') {
                    line.pop_back();
                }
                // 空行表示头部结束，后面就是消息体
                if (line.empty()) {
                    break;//头部结束
                }
                auto colon = line.find(":");
                if (colon == std::string::npos) {//std::string::npos找不到的标志
                    // 没有冒号视为非法头，跳过
                    continue;
                }
                std::string key = line.substr(0, colon);
                std::string value = line.substr(colon + 1);
                // 去掉 value 前导空格，避免“Content-Length:  42”这种情况
                size_t pos = value.find_first_not_of(' ');//从字符串的开头（默认位置 0）开始，找到第一个「不是空格 ' '」的字符的下标
                if (pos != std::string::npos) {
                    MCP_LOG_DEBUG("Value: {}", value);
                    value = value.substr(pos);
                }

                // key 转小写以实现大小写不敏感
                std::transform(key.begin(), key.end(), key.begin(), [](unsigned char c) {
                    return std::tolower(c);
                });

                if (key == "content-length") {
                    try {
                        content_length = static_cast<size_t>(std::stoul(value));//字符串转成无符号长整数
                        found_content_length = true;
                    } catch (...) {
                        MCP_LOG_ERROR("Invalid Content-Length: {}", value);
                        return false;
                    }
                } else if (key == "content-type") {
                    // Content-Type 头部仅记录日志，保持兼容
                    MCP_LOG_DEBUG("Content-Type: {}", value);
                } else {
                    // 其他自定义头部：记录后忽略
                    MCP_LOG_DEBUG("Ignore header: {}: {}", key, value);
                }
            }
            MCP_LOG_INFO("Found Content-Length: {}", found_content_length);
            MCP_LOG_INFO("Content-Length: {}", content_length);

            // 改进：区分 "未找到 Content-Length" 和 "长度为 0"
            if (!found_content_length) {
                return false;
            }

            if (content_length == 0) {
                // 允许空消息体
                return true;
            }
            //读取结构体
            out_body.resize(content_length);
            size_t total_read = 0;

            while (total_read < content_length) {
                const std::streamsize to_read = static_cast<std::streamsize>(content_length - total_read);
                m_in_.read(&out_body[total_read], to_read);

                const std::streamsize just_read = m_in_.gcount();
                if (just_read <= 0) {
                    break;
                }

                total_read += static_cast<size_t>(just_read);

                if (!m_in_.good() && !m_in_.eof()) {
                    break;
                }
            }

            // 改进：添加日志，便于调试
            if (total_read != content_length) {
                MCP_LOG_ERROR("Incomplete message: expected {} bytes, got {}", content_length, total_read);
                return false;
            }

            return true;

        }

        /**
         * @brief 写入信息｜｜添加Content-Length头
         * @param msg
         */
        void StdioJsonRpcServer::WriteMessage(const json &msg) {
            std::string payload = msg.dump();
            m_out_ << "Content-Length: " << payload.size() << "\r\n\r\n";//所有的 Header（报头）已经说完了，后面紧跟着的就是 Body（数据正文）了。”
            m_out_ << payload;
            m_out_.flush();
        }

        /**
         * @brief 处理单个JSON_RPC请求
         * 错误码: -32700(Parse), -32600(Invalid), -32601(NotFound), -32602(Params), -32603(Internal)
         * @param req
         * @return
         */
        JsonRpcResponse StdioJsonRpcServer::HandleRequst(const JsonRpcRequest &req) {
            JsonRpcResponse resp;
            resp.jsonrpc = "2.0";
            if (req.id.has_value()) {
                resp.id = *req.id;
            } else {
                resp.id = nullptr;
            }

            try {
                //验证请求格式
                if (req.jsonrpc != "2.0") {
                    throw std::invalid_argument("Invalid Request: Jsonrpc must be 2.0");
                } if (req.method.empty()) {
                    throw std::invalid_argument("Invalid Request: method missing");
                }

                const std::string method = req.method;
                json params = req.params.has_value() ? *req.params : json::object();//新建一个空 json 对象 {}

                MCP_LOG_DEBUG("Handling method: {}", method);

                //检查方法是否存在
                if (!m_dispatcher_.HasHandler(method)) {
                    resp.error = JsonRpcError{
                        jsonrpc_errc::MethodNotFound,
                        "Method not found",
                        std::nullopt
                    };
                    return resp;
                }
                //调用方法处理器
                try {
                    json result = m_dispatcher_.Call(method, params);
                    resp.result = std::move(result);
                    resp.error.reset();
                } catch (const std::invalid_argument& ex) {
                    resp.result.reset();
                    resp.error = JsonRpcError{
                        jsonrpc_errc::InvalidParams,
                        ex.what(),
                        std::nullopt
                    };
                } catch (const std::exception& ex) {
                    resp.result.reset();
                    resp.error = JsonRpcError{
                        jsonrpc_errc::InternalError,
                        ex.what(),
                        std::nullopt
                    };
                }
            } catch (const std::invalid_argument& ex) {
                resp.result.reset();
                resp.error = JsonRpcError{
                    jsonrpc_errc::InvalidRequest,
                    ex.what(),
                    std::nullopt
                };
            } catch (const json::parse_error& ex) {
                resp.result.reset();
                resp.error = JsonRpcError{
                    jsonrpc_errc::ParseError,
                    ex.what(),
                    std::nullopt
                };
            } catch (const std::exception& ex) {
                resp.result.reset();
                resp.error = {
                    jsonrpc_errc::InternalError,
                    ex.what(),
                    std::nullopt
                };
            }
            return resp;
        }

        /**
         * @brief 运行服务器主循环【阻塞】，从stdin读取请求并输出到stdio
         */
        void StdioJsonRpcServer::run() {
            MCP_LOG_INFO("JSON-RPC stdio Server starting...");

            std::string body;

            while (true) {
                if (!ReadMessage(body)) {
                    if (m_in_.eof()) {
                        MCP_LOG_INFO("stdio EOF reached, existing");
                        break;
                    }
                    continue;
                }

                //解析并处理
                try {
                    json j = json::parse(body);

                    //尝试解析成请求对象；若结构不合法，返回InvalidRequst;
                    JsonRpcRequest req;
                    try {
                        req = j.get<JsonRpcRequest>();
                    } catch (const std::exception& ex) {
                        JsonRpcResponse resp;
                        resp.id = j.contains("id") ? j["id"] : nullptr;
                        resp.error = JsonRpcError{
                            jsonrpc_errc::InvalidRequest,
                            ex.what(),
                            std::nullopt
                        };
                        WriteMessage(json(resp));
                        continue;
                    }

                    const bool is_notification = !req.id.has_value();
                    auto resp = HandleRequst(req);

                    if (!is_notification) {
                        WriteMessage(json(resp));
                    }
                } catch (const json::parse_error& ex) {
                     MCP_LOG_ERROR("JSON parse error: {}", ex.what());
                    JsonRpcResponse resp;
                    resp.id = nullptr;
                    resp.error = JsonRpcError {
                        jsonrpc_errc::ParseError,
                        ex.what(),
                        std::nullopt
                    };
                }
            }
        }
    }
}