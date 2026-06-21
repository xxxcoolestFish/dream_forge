/**
 * @file engine/ecs/systems/dialogue_system.h
 * @brief 对话系统 — 订阅 InteractionEvent，触发 LLM 对话
 *
 * 解耦设计：
 *   - InteractionSystem 负责检测 E 键 + 找最近交互实体
 *   - DialogueSystem 订阅 InteractionEvent，收到后触发 LLM
 *   - 两者通过 EventBus 通信，互不依赖
 */

#pragma once

#include "engine/ecs/system.h"
#include "engine/core/event_bus.h"
#include <string>

namespace engine::ai    { class AiClient; }
namespace engine::ecs   { class World; }

namespace engine::ecs {

class DialogueSystem : public System
{
public:
    DialogueSystem(ai::AiClient& aiClient, EventBus& eventBus);

    void onInit(World& world) override;
    void onUpdate(World& world, double dt) override;

private:
    void onInteraction(const Event& event);

    ai::AiClient& m_aiClient;
    EventBus&     m_eventBus;

    bool          m_waitingForResponse = false;
    std::string   m_pendingNpcName;
    double        m_responseTimer = 0.0;

    uint32_t      m_subscriptionId = 0;
    World*        m_world = nullptr;
};

} // namespace engine::ecs
