/**
 * @file engine/ai/ai_client.cpp
 * @brief AI 客户端 — zmq::poll 非阻塞模式，绝不在主循环中阻塞
 */

#include "engine/ai/ai_client.h"

#include <zmq.hpp>
#include <spdlog/spdlog.h>

namespace engine::ai {

struct AiClient::Impl
{
    std::unique_ptr<zmq::context_t> context;
    std::unique_ptr<zmq::socket_t>  socket;
    std::string                     address;

    enum class State { Idle, ReadyToSend, WaitingForResponse };
    State state = State::Idle;
    json  requestData;
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

        // 关键：所有操作都不阻塞
        int timeout = 1; // 1ms 超时
        m_impl->socket->set(zmq::sockopt::rcvtimeo, timeout);
        m_impl->socket->set(zmq::sockopt::sndtimeo, timeout);
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
        spdlog::error("AiClient: ZeroMQ error: {}", e.what());
        m_connected = false;
        return false;
    }
}

void AiClient::disconnect()
{
    if (!m_connected) return;
    try {
        m_impl->socket->close();
        m_impl->socket.reset();
        m_impl->context->close();
        m_impl->context.reset();
    } catch (...) {}
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
// --- Step 1: 发送（非阻塞） ---
        if (impl.state == Impl::State::ReadyToSend)
        {
            std::string msgStr = impl.requestData.dump();

            zmq::pollitem_t items[] = {
                { *impl.socket, 0, ZMQ_POLLOUT, 0 }
            };
            int rc = zmq::poll(items, 1, 0);

            if (rc > 0 && (items[0].revents & ZMQ_POLLOUT))
            {
                zmq::message_t message(msgStr.size());
                memcpy(message.data(), msgStr.data(), msgStr.size());
                auto sendResult = impl.socket->send(message, zmq::send_flags::dontwait);
                impl.state = Impl::State::WaitingForResponse;
                spdlog::info("AiClient: >>> SENT request to AI service ({} bytes)", msgStr.size());
            }
            else
            {
                // 每60帧打印一次，避免刷屏
                static int pollOutFailCount = 0;
                if (++pollOutFailCount % 60 == 0)
                    spdlog::warn("AiClient: waiting for socket ready... (POLLOUT not ready, retry {})", pollOutFailCount);
            }
        }

        // --- Step 2: 接收（非阻塞） ---
        if (impl.state == Impl::State::WaitingForResponse)
        {
            zmq::pollitem_t items[] = {
                { *impl.socket, 0, ZMQ_POLLIN, 0 }
            };
            int rc = zmq::poll(items, 1, 0);

            if (rc > 0 && (items[0].revents & ZMQ_POLLIN))
            {
                zmq::message_t reply;
                auto recvResult = impl.socket->recv(reply, zmq::recv_flags::dontwait);

                if (recvResult.has_value())
                {
                    std::string replyStr(static_cast<char*>(reply.data()), reply.size());
                    impl.state = Impl::State::Idle;
                    spdlog::info("AiClient: <<< RECEIVED response from AI service ({} bytes)", replyStr.size());
                    return json::parse(replyStr);
                }
            }
            else
            {
                static int pollInFailCount = 0;
                if (++pollInFailCount % 120 == 0)
                    spdlog::info("AiClient: waiting for LLM response... ({}s elapsed)", pollInFailCount / 60);
            }
        }
    }
    catch (const zmq::error_t& e)
    {
        spdlog::warn("AiClient: ZMQ error: {} (state={})",
            e.what(), static_cast<int>(impl.state));
        impl.state = Impl::State::Idle;
    }
    catch (const json::parse_error& e)
    {
        spdlog::warn("AiClient: JSON parse error: {}", e.what());
        impl.state = Impl::State::Idle;
    }
    catch (const std::exception& e)
    {
        spdlog::error("AiClient: unexpected error: {}", e.what());
        impl.state = Impl::State::Idle;
    }

    return std::nullopt;
}

bool AiClient::hasPendingRequest() const
{
    return m_impl->state != Impl::State::Idle;
}

} // namespace engine::ai
