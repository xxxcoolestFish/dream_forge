/**
 * @file engine/core/event_bus.h
 * @brief 事件总线 — 子系统间解耦通信
 *
 * 负责：
 *   - 事件的订阅（subscribe）与发布（publish）
 *   - 类型安全的事件分发
 *   - 即时事件（immediate）和队列事件（queued）两种模式
 *
 * 设计：
 *   - 使用类型擦除（type erasure）存储回调，避免虚函数开销
 *   - EventType 是简单的 int ID，由宏生成（简化，避免 RTTI）
 *   - 单线程模型：事件在同一帧内同步分发
 *
 * 注意事项：
 *   - 回调中不要做耗时操作（AI调用、文件IO等），应提交异步任务
 *   - 回调中不要 recursively publish 过多事件，防止栈溢出
 */

#pragma once

#include <functional>
#include <vector>
#include <unordered_map>
#include <any>
#include <cstdint>

namespace engine {

// 事件类型ID
using EventTypeId = uint32_t;

// 事件容器
struct Event
{
    EventTypeId type;
    std::any    data;
};

// 事件回调签名
using EventCallback = std::function<void(const Event&)>;

class EventBus
{
public:
    EventBus();
    ~EventBus();

    // 订阅事件，返回订阅ID用于取消
    uint32_t subscribe(EventTypeId type, EventCallback callback);

    // 取消订阅
    void unsubscribe(EventTypeId type, uint32_t subscriptionId);

    // 发布即时事件（同步分发到所有订阅者）
    void publishImmediate(EventTypeId type, std::any data = {});

    // 发布队列事件（下一帧 processQueuedEvents() 时处理）
    void publishQueued(EventTypeId type, std::any data = {});

    // 处理所有队列中的事件
    void processQueuedEvents();

    // 清空所有订阅和队列
    void clear();

private:
    struct Subscription
    {
        uint32_t      id;
        EventCallback callback;
    };

    std::unordered_map<EventTypeId, std::vector<Subscription>> m_subscribers;
    std::vector<Event> m_queuedEvents;
    uint32_t m_nextSubscriptionId = 1;
};

// =========================================================================
// 生成唯一事件类型ID的宏（编译期）
// =========================================================================
// 用法：constexpr EventTypeId kMyEvent = ENGINE_EVENT("MyEvent");
// 实际上用编译期字符串哈希生成唯一ID

namespace detail {
    constexpr EventTypeId fnv1a32(const char* str, EventTypeId hash = 2166136261u)
    {
        return *str ? fnv1a32(str + 1, (hash ^ static_cast<EventTypeId>(*str)) * 16777619u)
                    : hash;
    }
}

#define ENGINE_EVENT(name) ::engine::detail::fnv1a32(name)

} // namespace engine
