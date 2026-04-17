/**
 * @file types.cpp
 * @brief MCP协议核心类型实现
 * @author Joe
 * @date 26-4-16
 */


#include "types.h"
namespace mcp {
    namespace mcp {

        //=================================Tool=================================
        json ToolInputSchema::to_json() const {
            json j = {
                {"type", type},
                {"properties", properties}
            };

            if (!required.empty()) {
                j["required"] = required;
            }
            return j;
        }
        ToolInputSchema ToolInputSchema::from_json(const json &j) {
            ToolInputSchema schema;
            schema.type = j.value("type", "object");
            schema.properties = j.value("properties", json::object());
            if (j.contains("required")) {
                schema.required = j["required"].get<std::vector<std::string>>();
            }
            return schema;
        }

        json Tool::to_json() const {
            return {
                {"name", name},
                {"description", description},
                {"input_schema", input_schema.to_json()}
            };
        }

        Tool Tool::from_json(const json &j) {
            Tool tool;
            tool.name = j.at("name").get<std::string>();
            tool.description = j.value("description", "");
            tool.input_schema = ToolInputSchema::from_json(j.at("input_schema"));
            return tool;
        }

        json ContentItem::to_json() const {
            json j = {
                {"type", type}
            };
            if (text.has_value()) {
                j["text"] = *text;
            } if (data.has_value()) {
                j["data"] = *data;
            } if (mime_type.has_value()) {
                j["mime_type"] = *mime_type;
            } if (url.has_value()) {
                j["url"] = *url;
            }

            return j;
        }

        ContentItem ContentItem::from_json(const json &j) {
            ContentItem item;
            item.type = j.at("type").get<std::string>();

            if (j.contains("text")) {
                item.text = j["text"].get<std::string>();
            } if (j.contains("data")) {
                item.data = j["data"].get<std::string>();
            } if (j.contains("mime_type")) {
                item.mime_type = j["mime_type"].get<std::string>();
            } if (j.contains(("url"))) {
                item.url = j["url"].get<std::string>();
            }
            return item;
        }
        json ToolResult::to_json() const {
            json content_arr = json::array();

            for (const auto& item : content) {
                content_arr.push_back(item.to_json());
            }
            json j = {"content", content_arr};
            if (is_error) {
                j["is_error"] = is_error;
            }

            return j;
        }

        ToolResult ToolResult::from_json(const json &j) {
            ToolResult result;
            if (j.contains("content")) {
                for (const auto& item_json : j["content"]) {
                    result.content.push_back(ContentItem::from_json(item_json));
                }
            }
            result.is_error = j.value("is_error", false);
            return result;
        }

        //=================================Resource===============================
        json Resource::to_json() const {
            json j = {
                {"url", url},
                {"name", name}
            };
            if (description.has_value()) {
                j["description"] = *description;
            } if (mime_type.has_value()) {
                j["mime_type"] = *mime_type;
            }
            return j;
        }

        Resource Resource::from_json(const json &j) {
            Resource resource;
            resource.url = j.at("url").get<std::string>();
            resource.name = j.at("name").get<std::string>();

            if (j.contains("description")) {
                resource.description = j["description"].get<std::string>();
            } if (j.contains("mime_type")) {
                resource.mime_type = j["mime_type"].get<std::string>();
            }
            return resource;
        }

        json ResourceContent::to_json() const {
            json j = {
                {"url", url},
                {"text", text}
            };
            if (mime_type.has_value()) {
                j["mime_type"] = *mime_type;
            } if (blob.has_value()) {
                j["blob"] = *blob;
            }
            return j;
        }


        ResourceContent ResourceContent::from_json(const json &j) {
            ResourceContent content;
            content.url = j.at("url").get<std::string>();
            content.text = j.at("text").get<std::string>();

            if (j.contains("mime_type")) {
                content.mime_type = j["mime_type"].get<std::string>();
            } if (j.contains("blob")) {
                content.blob = j["blob"].get<std::string>();
            }
            return content;
        }

        //=================================Prompt==================================
        json PromptArgument::to_json() const {
            json j = {
                {"name", name},
                {"required", required}
            };
            if (description.has_value()) {
                j["description"] = *description;
            }
            return j;

        }

        PromptArgument PromptArgument::from_json(const json &j) {
            PromptArgument argument;
            argument.name = j.at("name").get<std::string>();
            argument.required = j.value("required", false);

            if (j.contains("description")) {
                argument.description = j["description"].get<std::string>();
            }

            return argument;
        }

        json Prompt::to_json() const {
            json j = {
                {"name", name}
            };
            if (description.has_value()) {
                j["description"] = *description;
            } if (!arguments.empty()) {//vector-->json
                json args_arr = json::array();
                for (const auto& args : arguments) {
                    args_arr.push_back(args.to_json());
                }
                j["arguments"] = args_arr;
            }
            return j;
        }

        Prompt Prompt::from_json(const json &j) {
            Prompt prompt;
            prompt.name = j.at("name").get<std::string>();
            if (j.contains("description")) {
                prompt.description = j["description"].get<std::string>();
            } if (j.contains("arguments")) {
                for (const auto& arg_json : j["arguments"]) {
                    prompt.arguments.push_back(PromptArgument::from_json(arg_json));
                }
            }
            return prompt;
        }


        json PromptMessage::to_json() const {
            return {
                {"role", role == Role::User ? "user" : "assistant"},
                {"content", content}
            };
        }

        PromptMessage PromptMessage::from_json(const json &j) {
            PromptMessage msg;
            std::string role_str = j.at("role").get<std::string>();
            msg.role = (role_str == "user") ? Role::User : Role::Assistant;
            msg.content = j.at("content");
            return msg;
        }


        //=================================Capability==================================
        json ServerCapabilities::ToolsCapability::to_json() const {
            return {{"list_changed", list_changed}};
        }
        ServerCapabilities::ToolsCapability ServerCapabilities::ToolsCapability::from_json(const json &j) {
            ServerCapabilities::ToolsCapability tools_capability;
            tools_capability.list_changed = j.value("list_changed", false);
            return tools_capability;
        }

        json ServerCapabilities::ResourceCapability::to_json() const {
            return {
                {"subscribe", subscribe},
                {"list_changed", list_changed}
            };
        }
        ServerCapabilities::ResourceCapability ServerCapabilities::ResourceCapability::from_json(const json &j) {
            ServerCapabilities::ResourceCapability re_cap;
            re_cap.subscribe = j.value("subscribe", false);
            re_cap.list_changed = j.value("list_changed", false);
            return re_cap;
        }

        json ServerCapabilities::PromptCapability::to_json() const {
            return {{"list_changed", list_changed}};
        }

        ServerCapabilities::PromptCapability ServerCapabilities::PromptCapability::from_json(const json &j) {
            ServerCapabilities::PromptCapability pro_cap;
            pro_cap.list_changed = j.value("list_changed", false);
            return pro_cap;
        }

        json ServerCapabilities::to_json() const {
            json j = json::object();
            if (tools.has_value()) {
                j["tools"] = tools->to_json();
            } if (resources.has_value()) {
                j["resources"] = resources->to_json();
            } if (prompts.has_value()) {
                j["prompts"] = prompts->to_json();
            } if (logging.has_value()) {
                j["logging"] = *logging;
            }
            return j;
        }
        ServerCapabilities ServerCapabilities::from_json(const json &j) {
            ServerCapabilities ser_cap;
            if (j.contains("tools")) {
                ser_cap.tools = ToolsCapability::from_json(j["tools"]);
            } if (j.contains("resources")) {
                ser_cap.resources = ResourceCapability::from_json(j["resources"]);
            } if (j.contains("prompts")) {
                ser_cap.prompts = PromptCapability::from_json(j["prompts"]);
            } if (j.contains("logging")) {
                ser_cap.logging = j["logging"];
            }
            return ser_cap;
        }

        json ServerInfo::to_json() const {
            return {
                    {"name", name},
                    {"version", version}
            };

        }
        ServerInfo ServerInfo::from_json(const json &j) {
            ServerInfo info;
            info.name = j.at("name").get<std::string>();
            info.version = j.at("version").get<std::string>();

            return info;
        }

        json InitializeResult::to_json() const {
            return {
                {"protocol_version", protocol_version},
                {"capabilities", capabilities.to_json()},
                {"server_info", server_info.to_json()}
            };
        }
        InitializeResult InitializeResult::from_json(const json &j) {
            InitializeResult result;
            result.protocol_version = j.at("protocol_version").get<std::string>();
            result.capabilities = ServerCapabilities::from_json(j["capabilities"]);
            result.server_info = ServerInfo::from_json(j["server_info"]);
            return result;
        }

















    }
}
