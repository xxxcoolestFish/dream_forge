/**
 * @file engine/ai/ai_client.h
 * @brief AI 服务客户端 — 通过 ZeroMQ 与 Python AI 进程通信
 *
 * 负责：
 *   - 发送请求到 Python AI 服务（LLM对话、图像生成等）
 *   - 非阻塞轮询响应
 *   - JSON 消息序列化
 *
 * 使用方式：
 *   AiClient client;
 *   client.connect("tcp://127.0.0.1:5555");
 *   client.sendRequest({...});       // 异步发送
 *   auto resp = client.pollResponse(); // 每帧轮询
 *
 * 设计：
 *   - ZeroMQ REQ/REP 模式
 *   - 所有消息为 JSON 格式
 *   - 请求-响应一一对应（REQ/REP 保证）
 */

#pragma once

#include <string>
#include <memory>
#include <future>
#include <nlohmann/json.hpp>

namespace engine::ai {

using json = nlohmann::json;

class AiClient
{
public:
    AiClient();
    ~AiClient();

    // 禁止拷贝
    AiClient(const AiClient&) = delete;
    AiClient& operator=(const AiClient&) = delete;

    // 连接到 Python AI 服务
    bool connect(const std::string& address = "tcp://127.0.0.1:5555");

    // 断开连接
    void disconnect();

    // 发送请求（同步，阻塞当前线程直到收到响应）
    // 适用于简单请求；长时间 LLM 调用时在后台线程使用
    json sendRequest(const json& request, int timeoutMs = 5000);

    // 异步发送请求（不等待响应，通过 pollResponse 轮询）
    bool sendAsync(const json& request);

    // 轮询响应（如果有则返回，否则返回 nullopt）
    std::optional<json> pollResponse();

    // 连接状态
    bool isConnected() const { return m_connected; }

    // 启动 Python AI 服务进程（开发用）
    bool launchService(const std::string& pythonExe = "python",
                       const std::string& scriptPath = "");

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_connected = false;
};

} // namespace engine::ai
