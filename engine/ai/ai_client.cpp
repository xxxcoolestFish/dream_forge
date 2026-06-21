/**
 * @file engine/ai/ai_client.cpp
 * @brief AI 客户端 — std::async 后台线程 + future 轮询
 *
 * 设计：
 *   startRequest() → 在 std::async 中同步发送+接收（阻塞后台线程）
 *   pollResponse() → 检查 future 是否就绪，非阻塞
 *
 * 后台线程阻塞不影响 240FPS 主循环。
 */

#include "engine/ai/ai_client.h"

#include <zmq.hpp>
#include <zmq_addon.hpp>
#include <spdlog/spdlog.h>
#include <thread>

namespace engine::ai {

struct AiClient::Impl
{
    std::unique_ptr<zmq::context_t> context;
    std::string                     address;

    // 后台请求
    std::future<json> pendingFuture;
    bool              requestInFlight = false;

    // 在后台线程中执行同步请求
    static json doRequest(zmq::context_t* ctx, const std::string& addr,
                          const json& requestData)
    {
        try
        {
            zmq::socket_t socket(*ctx, zmq::socket_type::req);
            socket.set(zmq::sockopt::rcvtimeo, 30000); // 30秒超时（LLM需要时间）
            socket.set(zmq::sockopt::sndtimeo, 5000);
            socket.set(zmq::sockopt::linger, 0);
            socket.connect(addr);

            // 发送
            std::string msgStr = requestData.dump();
            zmq::message_t message(msgStr.size());
            memcpy(message.data(), msgStr.data(), msgStr.size());
            socket.send(message, zmq::send_flags::none);

            // 接收
            zmq::message_t reply;
            auto result = socket.recv(reply, zmq::recv_flags::none);

            socket.close();

            if (result.has_value())
            {
                std::string replyStr(static_cast<char*>(reply.data()), reply.size());
                return json::parse(replyStr);
            }

            return json::object(); // timeout
        }
        catch (const zmq::error_t& e)
        {
            spdlog::warn("AiClient::doRequest: ZMQ error: {}", e.what());
            json err;
            err["status"] = "error";
            err["error"] = std::string("ZMQ: ") + e.what();
            return err;
        }
        catch (const std::exception& e)
        {
            spdlog::warn("AiClient::doRequest: error: {}", e.what());
            json err;
            err["status"] = "error";
            err["error"] = e.what();
            return err;
        }
    }
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
        m_impl->address = address;
        m_connected = true;
        spdlog::info("AiClient ready, address={}", address);
        return true;
    }
    catch (const zmq::error_t& e)
    {
        spdlog::error("AiClient: ZeroMQ context error: {}", e.what());
        return false;
    }
}

void AiClient::disconnect()
{
    if (!m_connected) return;

    // 等待后台请求完成
    if (m_impl->requestInFlight)
    {
        try { m_impl->pendingFuture.wait_for(std::chrono::seconds(2)); }
        catch (...) {}
    }

    try { m_impl->context->close(); }
    catch (...) {}

    m_impl->context.reset();
    m_connected = false;
    spdlog::info("AiClient disconnected.");
}

bool AiClient::startRequest(const json& request)
{
    if (!m_connected) return false;
    if (m_impl->requestInFlight) return false;

    spdlog::info("AiClient: launching async request...");

    // 在后台线程执行同步请求
    zmq::context_t* ctx = m_impl->context.get();
    std::string addr = m_impl->address;

    m_impl->pendingFuture = std::async(std::launch::async,
        [ctx, addr, request]() -> json {
            return Impl::doRequest(ctx, addr, request);
        });

    m_impl->requestInFlight = true;
    return true;
}

std::optional<json> AiClient::pollResponse()
{
    if (!m_impl->requestInFlight) return std::nullopt;

    // 检查 future 是否完成（非阻塞）
    auto status = m_impl->pendingFuture.wait_for(std::chrono::milliseconds(0));

    if (status == std::future_status::ready)
    {
        m_impl->requestInFlight = false;
        try
        {
            json result = m_impl->pendingFuture.get();
            spdlog::info("AiClient: response received");
            return result;
        }
        catch (const std::exception& e)
        {
            spdlog::error("AiClient: future exception: {}", e.what());
            return json::object();
        }
    }

    return std::nullopt;
}

bool AiClient::hasPendingRequest() const
{
    return m_impl->requestInFlight;
}

} // namespace engine::ai
