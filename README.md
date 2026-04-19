
# MCP_Server

一个基于 C++17 实现的轻量 MCP Server，支持 `Tools`、`Resources`、`Prompts`、HTTP JSON-RPC、`stdio` 模式，以及基于 SSE 的实时事件流。

项目里还包含一个 Python 客户端示例 [`client/ollama_mcp_client.py`](/Users/joe/CLionProjects/MCP_Server/client/ollama_mcp_client.py)，可以把本地 Ollama 模型和 MCP Server 串起来，形成一个最小可用的“LLM + 工具调用”智能体闭环。

## 功能概览

- 支持 MCP 基础能力：
  - `tools/list`
  - `tools/call`
  - `resources/list`
  - `resources/read`
  - `prompts/list`
  - `prompts/get`
- 支持三种运行模式：
  - `http`
  - `stdio`
  - `both`
- 支持 SSE 实时事件推送：
  - `/sse/events`
  - `/sse/tool_calls`
- 内置工具：
  - `echo`
  - `calculate`
  - `get_time`
  - `get_weather`
  - `write_file`
- 内置资源：
  - `system://info`
  - `config://server`
- 内置 Prompt：
  - `code_review`
- 提供 Ollama Python 客户端示例，支持：
  - 工具发现
  - 工具调用
  - SSE 工具事件监控
  - 自然语言问答

## 项目结构

```text
MCP_Server/
├── main.cpp
├── client/
│   └── ollama_mcp_client.py
├── config/
│   └── server.json
├── src/
│   ├── config/
│   ├── json_rpc/
│   ├── logger/
│   └── mcp/
├── tests/
├── conanfile.txt
└── CMakeLists.txt
```

## 技术栈

- C++17
- CMake
- Conan
- `nlohmann_json`
- `spdlog`
- `cpp-httplib`
- `libcurl`
- `gtest`
- Python `requests`
- Ollama

## 架构说明

这个项目的核心链路可以理解成：

```text
用户
  ↓
ollama_mcp_client.py
  ↓
Ollama / 本地 LLM
  ↓
判断是否调用工具
  ↓
MCP Server
  ↓
具体工具执行
  ↓
工具结果返回
  ↓
LLM 组织最终回复
```

其中：

- `main.cpp` 负责注册工具、资源、Prompt，并启动 HTTP/stdio 服务
- `src/json_rpc/` 负责 JSON-RPC 请求分发与序列化
- `src/mcp/` 负责 MCP 核心类型和工具/资源/Prompt 的注册与调用
- `client/ollama_mcp_client.py` 是一个带工具调用的简单 Agent Client
- SSE 是实时事件流通道，当前主要用于工具调用状态观测，而不是主业务返回通道

## 当前内置能力

### Tools

#### `echo`

输入一个字符串，原样返回。

请求参数：

```json
{
  "message": "Hello"
}
```

#### `calculate`

支持基础四则运算，当前支持：

- `add`
- `subtract`
- `multiply`
- `divide`

请求参数：

```json
{
  "operation": "add",
  "a": "10",
  "b": "20"
}
```

说明：

- `a` 和 `b` 既支持 JSON number，也支持数字字符串
- 除零会返回工具错误

#### `get_time`

返回当前系统时间。

#### `get_weather`

根据城市名查询天气，使用 Open-Meteo Geocoding API + Forecast API。

当前返回的是封装后的 JSON 文本，而不是上游原始响应。返回字段类似：

```json
{
  "city": "Zhanjiang",
  "country": "China",
  "observation_time": "2026-04-19T15:00",
  "temperature_c": 28.7,
  "condition": "Overcast",
  "humidity_percent": 72,
  "wind_speed_kmh": 11.2,
  "wind_direction_degrees": 136,
  "wind_direction": "SE",
  "weather_code": 3,
  "source": "Open-Meteo Geocoding API + Forecast API"
}
```

说明：

- 推荐优先使用英文城市名，例如 `Zhanjiang`
- 当前中文城市名不一定能稳定命中地理编码

#### `write_file`

向指定路径写入内容。

请求参数：

```json
{
  "path": "/tmp/demo.txt",
  "content": "hello"
}
```

### Resources

#### `system://info`

返回系统信息文本。

#### `config://server`

返回当前服务器配置 JSON。

### Prompts

#### `code_review`

输入代码和语言，返回一个代码审查 Prompt 消息数组。

## HTTP 接口

### JSON-RPC 入口

- `POST /jsonrpc`

### SSE 入口

- `GET /sse/events`
- `GET /sse/tool_calls`

### 健康检查

- `GET /health`

### 根路径

- `GET /`

## 配置文件

默认配置文件在 [`config/server.json`](/Users/joe/CLionProjects/MCP_Server/config/server.json)：

```json
{
  "server": {
    "port": 8080
  },
  "logging": {
    "log_file_path": "../logs/server.log",
    "log_level": "debug",
    "log_file_size": 52428800,
    "log_file_count": 5,
    "log_console_output": true
  }
}
```

## 构建

### 1. 安装依赖

项目依赖定义在 [`conanfile.txt`](/Users/joe/CLionProjects/MCP_Server/conanfile.txt)。

```bash
conan install . --output-folder=build/Debug --build=missing -s build_type=Debug
```

### 2. 生成并编译

```bash
cmake -S . -B cmake-build-debug -DCMAKE_BUILD_TYPE=Debug
cmake --build cmake-build-debug
```

如果你使用的是 CLion，也可以直接用 IDE 打开项目并构建。

## 运行服务端

可执行文件示例：

[`cmake-build-debug/MCP_Server`](/Users/joe/CLionProjects/MCP_Server/cmake-build-debug/MCP_Server)

### HTTP 模式

```bash
/Users/joe/CLionProjects/MCP_Server/cmake-build-debug/MCP_Server --mode http --port 8080
```

### stdio 模式

```bash
/Users/joe/CLionProjects/MCP_Server/cmake-build-debug/MCP_Server --mode stdio
```

### 同时运行 HTTP + stdio

```bash
/Users/joe/CLionProjects/MCP_Server/cmake-build-debug/MCP_Server --mode both --port 8080
```

### 查看帮助

```bash
/Users/joe/CLionProjects/MCP_Server/cmake-build-debug/MCP_Server --help
```

## `curl` 调试示例

### 列出工具

```bash
curl -s http://127.0.0.1:8080/jsonrpc \
  -H 'Content-Type: application/json' \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"tools/list",
    "params":{}
  }' | jq
```

### 调用天气工具

```bash
curl -s http://127.0.0.1:8080/jsonrpc \
  -H 'Content-Type: application/json' \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"tools/call",
    "params":{
      "name":"get_weather",
      "arguments":{"city":"Zhanjiang"}
    }
  }' | jq
```

只看封装后的天气字段：

```bash
curl -s http://127.0.0.1:8080/jsonrpc \
  -H 'Content-Type: application/json' \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"tools/call",
    "params":{
      "name":"get_weather",
      "arguments":{"city":"Zhanjiang"}
    }
  }' | jq -r '.result.content[0].text | fromjson'
```

### 调用计算工具

```bash
curl -s http://127.0.0.1:8080/jsonrpc \
  -H 'Content-Type: application/json' \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"tools/call",
    "params":{
      "name":"calculate",
      "arguments":{
        "operation":"add",
        "a":"10",
        "b":"20"
      }
    }
  }' | jq
```

### 读取资源

```bash
curl -s http://127.0.0.1:8080/jsonrpc \
  -H 'Content-Type: application/json' \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"resources/read",
    "params":{"url":"config://server"}
  }' | jq
```

### 获取 Prompt

```bash
curl -s http://127.0.0.1:8080/jsonrpc \
  -H 'Content-Type: application/json' \
  -d '{
    "jsonrpc":"2.0",
    "id":1,
    "method":"prompts/get",
    "params":{
      "name":"code_review",
      "arguments":{
        "language":"cpp",
        "code":"int main(){return 0;}"
      }
    }
  }' | jq
```

## SSE 调试示例

### 监听服务事件

```bash
curl -N http://127.0.0.1:8080/sse/events
```

### 监听工具调用事件

```bash
curl -N http://127.0.0.1:8080/sse/tool_calls
```

`/sse/tool_calls` 会实时输出类似事件：

```json
{"type":"tool_call_start","tool":"get_weather","arguments":{"city":"Zhanjiang"}}
{"type":"tool_call_end","tool":"get_weather","success":true}
```

## Ollama 客户端

示例客户端在 [`client/ollama_mcp_client.py`](/Users/joe/CLionProjects/MCP_Server/client/ollama_mcp_client.py)。

它做的事情包括：

- 从 MCP Server 拉取工具列表
- 转成 Ollama 的 tool calling 格式
- 接收模型返回的 `tool_calls`
- 调用 MCP Server 的 `tools/call`
- 把工具结果再喂回模型
- 通过 SSE 监听工具执行事件

### 运行前准备

先确保：

1. Ollama 已启动
2. 本地已经有可用模型
3. MCP Server 正在运行

例如：

```bash
ollama serve
ollama pull qwen3.5:4b
```

### 运行客户端

```bash
python3 /Users/joe/CLionProjects/MCP_Server/client/ollama_mcp_client.py
```

### 指定模型

默认模型是 `qwen3.5:4b`。也可以通过环境变量覆盖：

```bash
OLLAMA_MODEL='qwen3.5:4b' python3 /Users/joe/CLionProjects/MCP_Server/client/ollama_mcp_client.py
```

### 客户端示例输入

```text
帮我查看一下 Zhanjiang 的天气
calculate "add" "10" "20"
tools
```

## 测试

当前仓库包含的自动化测试主要覆盖：

- 配置加载
- 日志初始化

运行测试：

```bash
ctest --test-dir /Users/joe/CLionProjects/MCP_Server/cmake-build-debug --output-on-failure
```

## 当前限制

- 天气查询更适合英文城市名，中文地名匹配仍可能失败
- `ollama_mcp_client.py` 目前更偏向单轮工具调用示例，还不是完整的多步 Agent Loop
- 天气工具依赖外部 Open-Meteo API，离线环境不可用
- `write_file` 目前没有额外的路径安全限制，使用时需要自行注意
- 测试覆盖面还不完整，天气工具和 HTTP 链路的自动化测试仍可继续补充

## 后续可扩展方向

- 多步工具循环
- 工具失败后的自动重试
- 更完整的 Agent 状态管理
- 更好的中文城市名归一化
- 天气工具单元测试和集成测试
- 前端可视化监控面板

## 项目截图

Server:
<img width="1512" height="948" alt="image" src="https://github.com/user-attachments/assets/46fb3f14-b2c1-43c7-9790-c462ad467b1b" />


Client:
<img width="1843" height="649" alt="image" src="https://github.com/user-attachments/assets/d1789bd9-d144-48c2-b6e3-2924617b1a5b" />

