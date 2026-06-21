/**
 * @file engine/ecs/systems/interaction_system.h
 * @brief 交互系统 — 检测玩家交互意图，通过 EventBus 发布事件
 *
 * 职责：
 *   - 检测 E 键按下
 *   - 找到玩家附近可交互的 Entity（Interactive 组件 + 距离检测）
 *   - 发布 InteractionEvent 到 EventBus
 *
 * 与 DialogueSystem 解耦：
 *   - DialogueSystem 订阅 InteractionEvent，收到后触发 LLM 对话
 *   - 未来 ItemSystem、DoorSystem 也可订阅同一事件
 */

#pragma once

#include "engine/ecs/system.h"

namespace engine::input { class InputSystem; }
namespace engine { class EventBus; }

namespace engine::ecs {

class InteractionSystem : public System
{
public:
    InteractionSystem(input::InputSystem& input, EventBus& eventBus);

    void onUpdate(World& world, double dt) override;

private:
    input::InputSystem& m_input;
    EventBus&           m_eventBus;
};

} // namespace engine::ecs
