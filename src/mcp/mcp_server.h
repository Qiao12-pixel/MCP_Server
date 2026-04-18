/**
 * @file mcp_server.h
 * @brief MCP_Server服务器核心类
 * 管理Tools, Resources, Prompts的注册和调用
 * @author Joe
 * @date 26-4-16
 */


#ifndef MCP_SERVER_H
#define MCP_SERVER_H
#include "types.h"
#include <functional>
#include <unordered_map>
#include <memory>
#include <mutex>

namespace mcp {
    namespace mcp_inter {
        class McpServer {
        public:
            // ===== Tool 相关类型 =====

            // 工具处理器函数类型
            using ToolHandler = std::function<ToolResult(const json& arguments)>;
            // 资源内容提供器函数类型
            using ResourceProvider = std::function<ResourceContent(const std::string& url)>;
            // 提示词生成器函数类型
            using PromptGenerator = std::function<std::vector<PromptMessage>(const json& arguments)>;
            //构造函数
            McpServer(const std::string& name, const std::string& version);
            //获取初始化结果
            InitializeResult GetInitResult() const;
            //设置服务器能力

            /**************************************工具相关**********************************************/
            void SetCapabilities(const ServerCapabilities& capabilities);
            //注册工具
            void RegisterTool(const Tool& tool, ToolHandler handler);
            //列出所有工具
            std::vector<Tool> ListTools() const;
            //检查工具是否存在
            bool HasTool(const std::string& name) const;
            //调用工具
            ToolResult CallTool(const std::string& name, const json& arguments);
            /******************************************************************************************/
            /**************************************工具相关**********************************************/
            //注册资源
            void RegisterResource(const Resource& resource, ResourceProvider provider);
            //列出所有资源
            std::vector<Resource> ListResource() const;
            //判断资源是否存在
            bool HasResource(const std::string& url) const;
            //读取资源
            ResourceContent ReadResource(const std::string& url);
            /******************************************************************************************/
            /**************************************prompt相关**********************************************/
            //注册提示词
            void RegisterPrompt(const Prompt& prompt, PromptGenerator generator);
            //列出所有提示词
            std::vector<Prompt> ListPrompts() const;
            //检查提示词是否存在
            bool HasPrompt(const std::string& name) const;
            //获取提示词
            std::vector<PromptMessage> GetPrompt(const std::string& name, const json& arguments);
            /******************************************************************************************/
            //注册SSE回调
            using SseEventCallback = std::function<void(const json&)>;
            void SetSseCallback(SseEventCallback callback);

        private:
            // 服务器信息
            ServerInfo m_server_info_;

            // 服务器能力
            ServerCapabilities m_capabilities_;

            // Tools 存储，用来存储各种工具
            std::unordered_map<std::string, Tool> m_tools_;
            std::unordered_map<std::string, ToolHandler> m_tool_handlers_;
            mutable std::mutex m_tools_mutex_;

            // Resources 存储
            std::unordered_map<std::string, Resource> m_resources_;
            std::unordered_map<std::string, ResourceProvider> m_resource_providers_;
            mutable std::mutex m_resources_mutex_;

            // Prompts 存储
            std::unordered_map<std::string, Prompt> m_prompts_;
            std::unordered_map<std::string, PromptGenerator> m_prompt_generators_;
            mutable std::mutex m_prompts_mutex_;

            // SSE 事件回调
            SseEventCallback m_sse_callback_;
            mutable std::mutex m_sse_mutex_;







        };
    }
}



#endif //MCP_SERVER_H
