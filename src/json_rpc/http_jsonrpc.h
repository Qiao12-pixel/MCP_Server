//
// Created by Joe on 26-4-14.
//

#ifndef HTTP_JSONRPC_H
#define HTTP_JSONRPC_H

#include "jsonrpc.h"
#include <string>
#include <functional>
#include <memory>
#include <atomic>


namespace mcp {
    namespace json_rpc {


        class HttpJsonRpcServer {
        public:
            explicit HttpJsonRpcServer(JsonRpcDispatcher dispatcher);

            HttpJsonRpcServer(JsonRpcDispatcher dispatcher, const std::string& host, int port);

            ~HttpJsonRpcServer();

            HttpJsonRpcServer(const HttpJsonRpcServer&) = delete;
            HttpJsonRpcServer& operator=(const HttpJsonRpcServer&) = delete;

            void run();

            void stop();

            bool is_running() const {
                return m_running_.load();
            }

            const std::string& get_host() const {
                return m_host_;
            }

            int get_port() const {
                return m_port_;
            }

            using SseCallback = std::function<void(const std::function<void(const std::string&)>&)>;

            void RegisterSseEndpoint(const std::string& path, SseCallback callback);

        private:
            JsonRpcDispatcher m_dispatcher_;
            std::string m_host_;
            int m_port_;
            std::atomic<bool> m_running_{false};

            /**
             * @brief 利用pimpl模式隐藏httplib实现细节
             */
            class Impl;
            std::unique_ptr<Impl> m_pimpl_;
            std::string HandleRequest(const std::string& request_body);
        };
    }
}



#endif //HTTP_JSONRPC_H
