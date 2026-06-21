/**
 * @file engine/ai/ai_client.cpp
 * @brief AI 客户端 ZeroMQ 实现
 */

#include "engine/ai/ai_client.h"

#include <zmq.hpp>
#include <spdlog/spdlog.h>
#include <thread>
#include <chrono>
#include <mutex>

namespace engine::ai {

struct AiClient::Impl
{
    std::unique_ptr<zmq::context_t> context;
    std::unique_ptr<zmq::socket_t>  socket;
    std::string                     address;

    // 异步请求队列保护
    std::mutex     mutex;
    bool           requestPending = false;
    json           pendingRequest;
    std::optional<json> pendingResponse;
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
    if (m_connected)
    {
        spdlog::warn("AiClient: already connected");
        return true;
    }

    try
    {
        m_impl->context = std::make_unique<zmq::context_t>(1);
        m_impl->socket  = std::make_unique<zmq::socket_t>(*m_impl->context, zmq::socket_type::req);

        // 设置超时
        int timeout = 3000; // 3秒
        m_impl->socket->set(zmq::sockopt::rcvtimeo, timeout);
        m_impl->socket->set(zmq::sockopt::sndtimeo, timeout);

        m_impl->socket->connect(address);
        m_impl->address = address;

        m_connected = true;
        spdlog::info("AiClient connected to Python AI service at {}", address);
        return true;
    }
    catch (const zmq::error_t& e)
    {
        spdlog::error("AiClient: ZeroMQ error: {}", e.what());
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
    catch (...)
    {
        // 关闭时忽略异常
    }

    m_connected = false;
    spdlog::info("AiClient disconnected.");
}

json AiClient::sendRequest(const json& request, int timeoutMs)
{
    if (!m_connected)
    {
        spdlog::error("AiClient::sendRequest: not connected");
        return json::object();
    }

    try
    {
        // 序列化为 JSON 字符串
        std::string msgStr = request.dump();

        zmq::message_t message(msgStr.size());
        memcpy(message.data(), msgStr.data(), msgStr.size());
        m_impl->socket->send(message, zmq::send_flags::none);

        // 接收响应
        zmq::message_t reply;
        auto result = m_impl->socket->recv(reply, zmq::recv_flags::none);

        if (!result)
        {
            spdlog::warn("AiClient::sendRequest: no response (timeout)");
            return json::object();
        }

        std::string replyStr(static_cast<char*>(reply.data()), reply.size());
        return json::parse(replyStr);
    }
    catch (const zmq::error_t& e)
    {
        spdlog::error("AiClient::sendRequest: ZeroMQ error: {}", e.what());
        return json::object();
    }
    catch (const json::parse_error& e)
    {
        spdlog::error("AiClient::sendRequest: JSON parse error: {}", e.what());
        return json::object();
    }
}

bool AiClient::sendAsync(const json& request)
{
    if (!m_connected) return false;

    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (m_impl->requestPending)
    {
        spdlog::warn("AiClient::sendAsync: previous request still pending");
        return false;
    }

    m_impl->pendingRequest = request;
    m_impl->requestPending = true;
    m_impl->pendingResponse.reset();

    // 在后台线程发送（REQ/REP 必须发送后立即等待）
    // 简单实现：直接用同步 sendRequest 在独立线程
    auto& impl = *m_impl;
    std::thread([&impl]()
    {
        try
        {
            std::string msgStr = impl.pendingRequest.dump();
            zmq::message_t message(msgStr.size());
            memcpy(message.data(), msgStr.data(), msgStr.size());
            impl.socket->send(message, zmq::send_flags::none);

            zmq::message_t reply;
            auto result = impl.socket->recv(reply, zmq::recv_flags::none);

            std::lock_guard<std::mutex> lock(impl.mutex);
            if (result)
            {
                std::string replyStr(static_cast<char*>(reply.data()), reply.size());
                impl.pendingResponse = json::parse(replyStr);
            }
            else
            {
                impl.pendingResponse = json::object();
            }
            impl.requestPending = false;
        }
        catch (...)
        {
            std::lock_guard<std::mutex> lock(impl.mutex);
            impl.pendingResponse = json::object();
            impl.requestPending = false;
        }
    }).detach();

    return true;
}

std::optional<json> AiClient::pollResponse()
{
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    if (!m_impl->requestPending && m_impl->pendingResponse.has_value())
    {
        auto resp = std::move(m_impl->pendingResponse);
        m_impl->pendingResponse.reset();
        return resp;
    }
    return std::nullopt;
}

bool AiClient::launchService(const std::string& pythonExe, const std::string& scriptPath)
{
    // TODO: 启动 Python 子进程
    spdlog::info("AiClient::launchService: launching {} {}", pythonExe, scriptPath);
    return false; // 当前由用户手动启动
}

} // namespace engine::ai
