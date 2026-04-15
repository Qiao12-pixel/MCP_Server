//
// Created by Joe on 26-4-14.
//

#ifndef JSONRPC_SERIALIZATION_H
#define JSONRPC_SERIALIZATION_H

#include <nlohmann/json.hpp>
using json = nlohmann::json;
#include "jsonrpc.h"


namespace mcp {
    namespace json_rpc {
        /**
         * @brief JSON-RPC 2.0 类型的 nlohmann::json 序列化定义
         */
        void to_json(json& j, const json_rpc::JsonRpcRequest& r);
        void from_json(const json& j, json_rpc::JsonRpcRequest& r);

        void to_json(json& j, const json_rpc::JsonRpcError& e);
        void from_json(const json& j, json_rpc::JsonRpcError& e);

        void to_json(json& j, const json_rpc::JsonRpcResponse& r);
        void from_json(const json& j, json_rpc::JsonRpcResponse& r);
    }
}



#endif //JSONRPC_SERIALIZATION_H
