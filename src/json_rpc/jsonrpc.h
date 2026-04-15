//
// Created by Joe on 26-4-14.
//

#ifndef JSONRPC_H
#define JSONRPC_H
#include <iostream>
#include <nlohmann/json.hpp>
namespace mcp {
    namespace json_rpc {
        using json = nlohmann::json;
        /**
         * @brief JSON-RPC 2.0基础类型
         */
        struct JsonRpcError {
            int code;
            std::string message;
            std::optional<json> data;
        };

        /**
         * @brief JSON-RPC 2.0请求类型
         */
        struct JsonRpcRequest {
            std::string jsonrpc = "2.0";//JSON-RPC 2.0 规范要求每个请求必须包含这个精确的字段和值。通过默认初始化，你在创建一个新请求时就不需要手动赋值了，减少了出错概率。
            std::optional<json> id;//如果 id 有值，它是一个 Request（请求），服务器必须回复。如果 id 为空（std::nullopt），它是一个 Notification（通知），服务器不得回复。
            std::string method;//核心功能： 告诉服务器要执行的具体操作。
            std::optional<json> params;//并不是所有方法都需要参数（例如 tools/list 可能不需要参数）。
        };

        /**
         * Json-RPC 2.0响应类型
         */
        struct JsonRpcResponse {
            std::string jsonrpc = "2.0";
            json id;
            std::optional<json> result;
            std::optional<JsonRpcError> error;
        };

        class JsonRpcDispatcher {
        public:

            using Handler = std::function<json(const json& params)>;

            void RegisterHandler(const std::string& method, Handler handler);

            bool HasHandler(const std::string& method) const;

            json Call(const std::string& method, const json& params) const;

        private:

            std::unordered_map<std::string, Handler> m_handler_;
        };
        /**
         * @brief stdio + Content-Length 封包的 JSON-RPC 服务器
         */
        class StdioJsonRpcServer {
        public:
            explicit StdioJsonRpcServer(JsonRpcDispatcher dispatcher);
            StdioJsonRpcServer(JsonRpcDispatcher dispatcher, std::istream& in, std::ostream& out);

            void run();

        private:
            JsonRpcDispatcher m_dispatcher_;
            std::istream& m_in_ = std::cin;

            std::ostream& m_out_ = std::cout;

            bool ReadMessage(std::string& out_body);

            void WriteMessage(const json& msg);

            JsonRpcResponse HandleRequst(const JsonRpcRequest& req);
        };
        namespace jsonrpc_errc {
            // 应用自定义错误建议使用 -32000 ~ -32099
            constexpr int ParseError = -32700;
            constexpr int InvalidRequest = -32600;
            constexpr int MethodNotFound = -32601;
            constexpr int InvalidParams = -32602;
            constexpr int InternalError = -32603;
        }
    }
}
#endif //JSONRPC_H
