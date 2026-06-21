/**
 * @file engine/ai/ai_client.h
 * @brief AI 服务客户端 — 通过 ZeroMQ 与 Python AI 进程通信
 */

#pragma once

#include <string>
#include <memory>
#include <optional>
#include <nlohmann/json.hpp>

namespace engine::ai {

using json = nlohmann::json;

class AiClient
{
public:
    AiClient();
    ~AiClient();

    AiClient(const AiClient&) = delete;
    AiClient& operator=(const AiClient&) = delete;

    bool connect(const std::string& address = "tcp://127.0.0.1:5555");
    void disconnect();

    // 发送请求并立即返回（不阻塞主循环）
    // 请求进入"待发送"状态，下一帧 sendAndPoll 会实际发送
    bool startRequest(const json& request);

    // 每帧调用：发送待发请求 + 非阻塞轮询响应
    // 如果有响应，返回响应内容；否则返回 nullopt
    std::optional<json> sendAndPoll();

    // 是否有待处理的请求
    bool hasPendingRequest() const;

    bool isConnected() const { return m_connected; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_connected = false;
};

} // namespace engine::ai
