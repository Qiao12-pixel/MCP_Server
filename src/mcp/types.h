/**
 * @file types.h
 * @brief MCP核心类型定义
 * 包含 Tools, Resources, Prompts等核心类型
 * @author Joe
 * @date 26-4-16
 */


#ifndef TYPES_H
#define TYPES_H

#include <nlohmann/json.hpp>
#include <string>
#include <vector>
#include <optional>
#include <variant> // union联合体

namespace mcp {
    namespace mcp_inter {
        using json = nlohmann::json;
        // MCP 协议版本
        constexpr const char* LATEST_PROTOCOL_VERSION = "2024-11-05";
        constexpr const char* DEFAULT_NEGOTIATED_VERSION = "2024-11-05";

        // 进度令牌类型
        using ProgressToken = std::variant<std::string, int64_t>;

        // 光标类型（用于分页）
        using Cursor = std::string;

        // 角色类型
        enum class Role {
            User,
            Assistant
        };

        //=================================Tool=================================
        //工具输入
        //告诉别人“这个工具要怎么传参数”。
        struct ToolInputSchema {
            std::string type = "object";
            json properties;
            std::vector<std::string> required;

            json to_json() const;
            static ToolInputSchema from_json(const json& j);
        };

        //工具定义
        struct Tool {
            std::string name;
            std::string description;
            ToolInputSchema input_schema;

            json to_json() const;
            static Tool from_json(const json& j);
        };

        //工具调用结果内容[文本或图片]
        struct ContentItem {
            std::string type;
            std::optional<std::string> text;
            std::optional<std::string> data;
            std::optional<std::string> mime_type;
            std::optional<std::string> url;

            json to_json() const;
            static ContentItem from_json(const json& j);
        };

        //工具调用结果
        struct ToolResult {
            std::vector<ContentItem> content;
            bool is_error = false;

            json to_json() const;
            static ToolResult from_json(const json& j);
        };

        //=================================Resource===============================
        struct Resource {
            std::string url;
            std::string name;
            std::optional<std::string> description;
            std::optional<std::string> mime_type;//"application/json"

            json to_json() const;
            static Resource from_json(const json& j);
        };

        struct ResourceContent {
            std::string url;
            std::optional<std::string> mime_type;
            std::string text;
            std::optional<std::string> blob;// base64 编码的二进制内容\

            json to_json() const;
            static ResourceContent from_json(const json& j);
        };

        //=================================Prompt==================================
        //提示参数
        struct PromptArgument {
            std::string name;
            std::optional<std::string> description;
            bool required = false;

            json to_json() const;
            static PromptArgument from_json(const json& j);
        };
        //提示定义
        struct Prompt {
            std::string name;
            std::optional<std::string> description;
            std::vector<PromptArgument> arguments;

            json to_json() const;
            static Prompt from_json(const json& j);
        };

        //提示消息
        struct PromptMessage {
            Role role;
            json content;

            json to_json() const;
            static PromptMessage from_json(const json& j);
        };


        //============================服务器能力==============================
        struct ServerCapabilities {
            struct ToolsCapability {
                bool list_changed = false;//工具列表是否出现问题

                json to_json() const;
                static ToolsCapability from_json(const json& j);
            };

            struct ResourceCapability {
                bool subscribe = false;//是否订阅资源
                bool list_changed = false;//资源列表是否发生变化

                json to_json() const;
                static ResourceCapability from_json(const json& j);
            };

            struct PromptCapability {
                bool list_changed = false;

                json to_json() const;
                static PromptCapability from_json(const json& j);
            };

            std::optional<ToolsCapability> tools;
            std::optional<ResourceCapability> resources;
            std::optional<PromptCapability> prompts;
            std::optional<json> logging;//日志配置

            json to_json() const;
            static ServerCapabilities from_json(const json& j);
        };

        //=========================MCP协议服务器信息=============================
        struct ServerInfo {
            std::string name;
            std::string version;

            json to_json() const;
            static ServerInfo from_json(const json& j);
        };

        struct InitializeResult {
            std::string protocol_version;
            ServerCapabilities capabilities;
            ServerInfo server_info;

            json to_json() const;
            static InitializeResult from_json(const json& j);
        };
    }
}



#endif //TYPES_H
