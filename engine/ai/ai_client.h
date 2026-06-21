/**
 * @file engine/ai/ai_client.h
 * @brief AI 服务客户端
 */

#pragma once

#include <string>
#include <memory>
#include <optional>
#include <future>
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

    // 异步发送请求（返回 future，在后台线程执行）
    // 调用 pollResponse() 检查是否完成
    bool startRequest(const json& request);

    // 检查请求是否完成，如果完成返回响应
    std::optional<json> pollResponse();

    bool hasPendingRequest() const;
    bool isConnected() const { return m_connected; }

private:
    struct Impl;
    std::unique_ptr<Impl> m_impl;
    bool m_connected = false;
};

} // namespace engine::ai
