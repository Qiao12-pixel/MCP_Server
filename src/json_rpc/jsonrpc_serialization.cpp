//
// Created by Joe on 26-4-14.
//

#include "jsonrpc_serialization.h"
#include "jsonrpc.h"

namespace mcp {
    namespace json_rpc {
        /**
         * @brief 将c++结构体进行序列化; c++ struct --> json
         * @param j
         * @param r
         */
        void to_json(json &j, const json_rpc::JsonRpcRequest &r) {
            j = json{{"jsonrpc", r.jsonrpc}, {"method", r.method}};
            if (r.id.has_value()) {
                j["id"] = *r.id;
            } if (r.params.has_value()) {
                j["params"] = *r.params;
            }
        }

        /**
         * @brief 将json反序列化; json---> c++ struct
         * @param j
         * @param r
         */
        void from_json(const json &j, json_rpc::JsonRpcRequest &r) {
            // r.jsonrpc = j.at("jsonrpc").get<std::string>();//从 JSON 对象 j 中提取名为 "jsonrpc" 的字段，并将其转换为 C++ 的 std::string 类型，最后赋值给结构体成员 r.jsonrpc。
            // r.method = j.at("method").get<std::string>();
            j.at("jsonrpc").get_to(r.jsonrpc);
            j.at("method").get_to(r.method);
            if (j.contains("id")) {
                r.id = j.at("id");
                // j.at("id").get_to(*r.id);
            } else {
                r.id.reset();//防止旧的id存在
            } if (j.contains("params")) {
                r.params = j.at("params");
                // j.at("params").get_to(*r.params);
            } else {
                r.params.reset();
            }
        }

        void to_json(json &j, const json_rpc::JsonRpcResponse &r) {
            // j = json{{"jsonrpc", "2.0"}, {"id", r.id}};
            j = json{{"jsonrpc", r.jsonrpc}, {"id", r.id}};
            if (r.result.has_value()) {
                j["result"] = *r.result;
            } else if (r.error.has_value()) {
                j["error"] = *r.error;
            }
        }

        void from_json(const json &j, json_rpc::JsonRpcResponse &r) {
            // r.jsonrpc = j.at("jsonrpc").get<std::string>();
            // r.id = j.at("id");
            j.at("jsonrpc").get_to(r.jsonrpc);
            j.at("id").get_to(r.id);

            if (j.contains("error")) {
                r.error = j.at("error").get<JsonRpcError>();
                // j.at("error").get_to(*r.error);
                r.result.reset();
            } else if (j.contains("result")) {
                r.result = j.at("result");
                // j.at("result").get_to(*r.result);
                r.error.reset();
            }
        }

        void to_json(json &j, const json_rpc::JsonRpcError &e) {
            j = json{{"code", e.code}, {"message", e.message}};
            if (e.data) {
                j["data"] = *e.data;
            }
        }

        void from_json(const json &j, json_rpc::JsonRpcError &e) {
            // e.code = j.at("code").get<int>();
            j.at("code").get_to(e.code);
            // e.message = j.at("message").get<std::string>();
            j.at("message").get_to(e.message);
            if (j.contains("data")) {
                e.data = j.at("data");
            } else {
                e.data.reset();
            }
        }








    }
}