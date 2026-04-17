/**
 * @file mcp_server.cpp
 * @brief 
 * @author Joe
 * @date 26-4-16
 */


#include "mcp_server.h"
#include <stdexcept>
#include "../logger/logger.h"

namespace mcp {
    namespace mcp {
        McpServer::McpServer(const std::string &name, const std::string &version) {
            m_server_info_.name = name;
            m_server_info_.version = version;

            //设置默认能力
            m_capabilities_.tools = ServerCapabilities::ToolsCapability{false};
            m_capabilities_.resources = ServerCapabilities::ResourceCapability{false, false};
            m_capabilities_.prompts = ServerCapabilities::PromptCapability{false};
        }

        InitializeResult McpServer::GetInitResult() const {
            InitializeResult result;
            result.protocol_version = LATEST_PROTOCOL_VERSION;
            result.capabilities = m_capabilities_;
            result.server_info = m_server_info_;
            return result;
        }

        void McpServer::SetCapabilities(const ServerCapabilities &capabilities) {
            m_capabilities_ = capabilities;
        }
        /*********************************工具类*********************************/
        void McpServer::RegisterTool(const Tool &tool, ToolHandler handler) {
            std::lock_guard<std::mutex> lock(m_tools_mutex_);
            if (m_tools_.find(tool.name) != m_tools_.end()) {
                throw std::runtime_error("Tool already registered: " + tool.name);
            }
            MCP_LOG_INFO("Use tool name: {}", tool.name);
            m_tools_[tool.name] = tool;
            m_tool_handlers_[tool.name] = std::move(handler);
        }

        std::vector<Tool> McpServer::ListTools() const {
            std::lock_guard<std::mutex> lock(m_tools_mutex_);

            std::vector<Tool> result;
            result.reserve(m_tools_.size());

            for (const auto& [name, tool] : m_tools_) {
                result.push_back(tool);
            }
            return result;
        }
        bool McpServer::HasTool(const std::string &name) const {
            std::lock_guard<std::mutex> lock(m_tools_mutex_);
            return m_tools_.find(name) != m_tools_.end();
        }

        ToolResult McpServer::CallTool(const std::string &name, const json &arguments) {
            std::lock_guard<std::mutex> lock(m_tools_mutex_);
            auto it = m_tool_handlers_.find(name);
            if (it == m_tool_handlers_.end()) {
                throw std::runtime_error("Tool not found: " + name);
            }
            MCP_LOG_INFO("Use Tool: {}", name);


            //推送工具调用事件
            {
                std::lock_guard<std::mutex> sse_lock(m_sse_mutex_);
                if (m_sse_callback_) {
                    m_sse_callback_(json({
                        {"type", "tool_call_start"},
                        {"tool", name},
                        {"arguments", arguments},
                        {"timestamp", std::time(nullptr)}
                    }));
                }
            }

            try {
                auto result = it->second(arguments);
                //推送工具调用成功事件
                {
                    std::lock_guard<std::mutex> sse_lock(m_sse_mutex_);
                    if (m_sse_callback_) {
                        m_sse_callback_(json({
                            {"type", "tool_call_end"},
                            {"tool", name},
                            {"success", !result.is_error},
                            {"timestamp", std::time(nullptr)}
                        }));
                    }
                }
                return result;
            } catch (const std::exception& e) {
                {
                    //推送工具调用失败事件
                    std::lock_guard<std::mutex> sse_lock(m_sse_mutex_);
                    if (m_sse_callback_) {
                        m_sse_callback_(json({
                            {"type", "tool_call_error"},
                            {"tool", name},
                            {"error", e.what()},
                            {"timestamp", std::time(nullptr)}
                        }));
                    }
                }
                ToolResult error_result;
                error_result.is_error = true;
                ContentItem item;
                item.type = "text";
                item.text = std::string("Error calling tool: ") + e.what();
                error_result.content.push_back(item);
                return error_result;
            }
        }

        /************************************资源类*****************************/
        void McpServer::RegisterResource(const Resource &resource, ResourceProvider provider) {
            std::lock_guard<std::mutex> lock(m_resources_mutex_);
            if (m_resources_.find(resource.url) != m_resources_.end()) {
                throw std::runtime_error("Resource already resigtered: " + resource.url);
            }
            MCP_LOG_INFO("Use m_resource_ {}", resource.url);

            m_resources_[resource.url] = resource;
            m_resource_providers_[resource.url] = std::move(provider);//resource_providers_["config://server"] = 某个 lambda】存在时获取这个资源的函数
        }

        std::vector<Resource> McpServer::ListResource() const {
            std::lock_guard<std::mutex> lock(m_resources_mutex_);

            std::vector<Resource> result;
            result.reserve(m_resources_.size());

            for (const auto& [url, resource] : m_resources_) {
                result.push_back(resource);
            }

            return result;
        }
        bool McpServer::HasResource(const std::string &url) const {
            std::lock_guard<std::mutex> lock(m_resources_mutex_);
            return m_resources_.find(url) != m_resources_.end();
        }

        ResourceContent McpServer::ReadResource(const std::string &url) {
            std::lock_guard<std::mutex> lock(m_resources_mutex_);

            auto it = m_resource_providers_.find(url);//获取取得资源的方法
            if (it == m_resource_providers_.end()) {
                throw std::runtime_error("Resource not found: " + url);
            }
            MCP_LOG_INFO("Use Resouece: {}", url);

            return it->second(url);
        }


        /************************************提示词类*****************************/
        void McpServer::RegisterPrompt(const Prompt &prompt, PromptGenerator generator) {
            std::lock_guard<std::mutex> lock(m_prompts_mutex_);

            if (m_prompts_.find(prompt.name) != m_prompts_.end()) {
                throw std::runtime_error("Prompt already registered: " + prompt.name);
            }
            MCP_LOG_INFO("Use Prompt: {}", prompt.name);

            m_prompts_[prompt.name] = prompt;
            m_prompt_generators_[prompt.name] = std::move(generator);
        }
        std::vector<Prompt> McpServer::ListPrompts() const {
            std::lock_guard<std::mutex> lock(m_prompts_mutex_);

            std::vector<Prompt> result;
            result.reserve(m_prompts_.size());

            for (const auto& [name, prompt] : m_prompts_) {
                result.push_back(prompt);
            }
            return result;
        }
        bool McpServer::HasPrompt(const std::string &name) const {
            std::lock_guard<std::mutex> lock(m_prompts_mutex_);
            return m_prompts_.find(name) != m_prompts_.end();
        }
        std::vector<PromptMessage> McpServer::GetPrompt(const std::string &name, const json &arguments) {
            std::lock_guard<std::mutex> lock(m_prompts_mutex_);

            auto it = m_prompt_generators_.find(name);
            if (it == m_prompt_generators_.end()) {
                throw std::runtime_error("Prompt not found: " + name);
            }
            MCP_LOG_INFO("Use Prompt: {}", name);
            return it->second(arguments);
        }


        void McpServer::SetSseCallback(SseEventCallback callback) {
            std::lock_guard<std::mutex> lock(m_sse_mutex_);
            m_sse_callback_ = std::move(callback);
        }






    }
}