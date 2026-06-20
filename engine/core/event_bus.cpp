/**
 * @file engine/core/event_bus.cpp
 * @brief 事件总线实现
 */

#include "engine/core/event_bus.h"
#include <spdlog/spdlog.h>

namespace engine {

EventBus::EventBus()
{
    spdlog::debug("EventBus created");
}

EventBus::~EventBus()
{
    spdlog::debug("EventBus destroyed");
}

uint32_t EventBus::subscribe(EventTypeId type, EventCallback callback)
{
    uint32_t id = m_nextSubscriptionId++;
    m_subscribers[type].push_back({ id, std::move(callback) });
    return id;
}

void EventBus::unsubscribe(EventTypeId type, uint32_t subscriptionId)
{
    auto it = m_subscribers.find(type);
    if (it == m_subscribers.end()) return;

    auto& vec = it->second;
    vec.erase(
        std::remove_if(vec.begin(), vec.end(),
            [subscriptionId](const Subscription& s) { return s.id == subscriptionId; }),
        vec.end()
    );
}

void EventBus::publishImmediate(EventTypeId type, std::any data)
{
    Event event{ type, std::move(data) };

    auto it = m_subscribers.find(type);
    if (it == m_subscribers.end()) return;

    for (auto& sub : it->second)
    {
        if (sub.callback)
        {
            sub.callback(event);
        }
    }
}

void EventBus::publishQueued(EventTypeId type, std::any data)
{
    m_queuedEvents.push_back({ type, std::move(data) });
}

void EventBus::processQueuedEvents()
{
    if (m_queuedEvents.empty()) return;

    // 取出所有队列事件（避免在处理过程中新增事件导致无限循环）
    auto events = std::move(m_queuedEvents);
    m_queuedEvents.clear();

    for (const auto& event : events)
    {
        auto it = m_subscribers.find(event.type);
        if (it == m_subscribers.end()) continue;

        for (auto& sub : it->second)
        {
            if (sub.callback)
            {
                sub.callback(event);
            }
        }
    }
}

void EventBus::clear()
{
    m_subscribers.clear();
    m_queuedEvents.clear();
}

} // namespace engine
