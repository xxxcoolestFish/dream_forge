/**
 * @file engine/ai/ai_client.cpp
 * @brief AI 客户端实现 — 非阻塞轮询模式，无后台线程
 *
 * 设计：
 *   startRequest() → 标记请求就绪
 *   sendAndPoll() → 每帧调用：发送请求（如未发送）+ 非阻塞检查响应
 *
 * 这避免了后台线程的竞态问题，且不会阻塞 240FPS 渲染循环。
 */

#include "engine/ai/ai_client.h"

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <spdlog/spdlog.h>

namespace engine::ai {

struct AiClient::Impl
{
    std::unique_ptr<zmq::context_t> context;
    std::unique_ptr<zmq::socket_t>  socket;
    std::string                     address;

    // 请求状态机
    enum class State { Idle, ReadyToSend, WaitingForResponse };
    State   state     = State::Idle;
    json    requestData;
    int     timeoutMs = 5000;
};

AiClient::AiClient()
    : m_impl(std::make_unique<Impl>())
{
    spdlog::debug("AiClient created");
}

AiClient::~AiClient()
{
    disconnect();
}

bool AiClient::connect(const std::string& address)
{
    if (m_connected) return true;

    try
    {
        m_impl->context = std::make_unique<zmq::context_t>(1);
        m_impl->socket  = std::make_unique<zmq::socket_t>(*m_impl->context, zmq::socket_type::req);

        // 设置发送/接收超时（非阻塞模式下实际用于阻塞模式的 fallback）
        int timeout = 3000;
        m_impl->socket->set(zmq::sockopt::rcvtimeo, timeout);
        m_impl->socket->set(zmq::sockopt::sndtimeo, timeout);
        // 设置 linger 为 0，关闭时立即丢弃未发送的消息
        int linger = 0;
        m_impl->socket->set(zmq::sockopt::linger, linger);

        m_impl->socket->connect(address);
        m_impl->address = address;

        m_connected = true;
        spdlog::info("AiClient connected to Python AI service at {}", address);
        return true;
    }
    catch (const zmq::error_t& e)
    {
        spdlog::error("AiClient: ZeroMQ connect error: {}", e.what());
        m_connected = false;
        return false;
    }
}

void AiClient::disconnect()
{
    if (!m_connected) return;

    try
    {
        m_impl->socket->close();
        m_impl->socket.reset();
        m_impl->context->close();
        m_impl->context.reset();
    }
    catch (...) {}

    m_connected = false;
    spdlog::info("AiClient disconnected.");
}

bool AiClient::startRequest(const json& request)
{
    if (!m_connected) return false;
    if (m_impl->state != Impl::State::Idle) return false;

    m_impl->requestData = request;
    m_impl->state = Impl::State::ReadyToSend;
    return true;
}

std::optional<json> AiClient::sendAndPoll()
{
    if (!m_connected) return std::nullopt;

    auto& impl = *m_impl;

    try
    {
        // --- Step 1: 发送请求（如果就绪） ---
        if (impl.state == Impl::State::ReadyToSend)
        {
            std::string msgStr = impl.requestData.dump();
            zmq::message_t message(msgStr.size());
            memcpy(message.data(), msgStr.data(), msgStr.size());
            impl.socket->send(message, zmq::send_flags::none);
            impl.state = Impl::State::WaitingForResponse;
        }

        // --- Step 2: 非阻塞检查响应 ---
        if (impl.state == Impl::State::WaitingForResponse)
        {
            zmq::message_t reply;
            // 非阻塞接收：如果没有消息可用，立即返回
            auto recvResult = impl.socket->recv(reply, zmq::recv_flags::dontwait);

            if (recvResult.has_value())
            {
                std::string replyStr(static_cast<char*>(reply.data()), reply.size());
                impl.state = Impl::State::Idle;
                return json::parse(replyStr);
            }
        }
    }
    catch (const zmq::error_t& e)
    {
        // ZMQ 错误（如超时、连接断开等）
        spdlog::warn("AiClient::sendAndPoll: ZMQ error: {}", e.what());
        impl.state = Impl::State::Idle;
        return json::object(); // 返回空响应
    }
    catch (const json::parse_error& e)
    {
        spdlog::warn("AiClient::sendAndPoll: JSON parse error: {}", e.what());
        impl.state = Impl::State::Idle;
        return json::object();
    }

    return std::nullopt;
}

bool AiClient::hasPendingRequest() const
{
    return m_impl->state != Impl::State::Idle;
}

} // namespace engine::ai
