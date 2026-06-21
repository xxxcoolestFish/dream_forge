/**
 * @file engine/ecs/systems/interaction_system.cpp
 * @brief 交互系统实现
 */

#include "engine/ecs/systems/interaction_system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"
#include "engine/input/input_system.h"
#include "engine/core/event_bus.h"
#include "engine/ecs/event_types.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <cmath>

namespace engine::ecs {

// 交互事件类型（编译期生成唯一 ID）
static constexpr EventTypeId kInteractionEvent = ENGINE_EVENT("Interaction");

InteractionSystem::InteractionSystem(input::InputSystem& input, EventBus& eventBus)
    : System("InteractionSystem")
    , m_input(input)
    , m_eventBus(eventBus)
{
}

void InteractionSystem::onUpdate(World& world, double dt)
{
    (void)dt;

    // 检测 E 键
    if (!m_input.isKeyPressed(GLFW_KEY_E)) return;

    // 找到玩家
    auto playerView = world.view<Transform, Player>();
    if (playerView.begin() == playerView.end()) return;

    auto playerEntity = *playerView.begin();
    auto& playerTransform = world.getComponent<Transform>(playerEntity);

    // 找最近的交互实体（100 像素内）
    Entity  nearestEntity = kNullEntity;
    float   nearestDist   = 100.0f;
    std::string nearestType;

    auto interactiveView = world.view<Transform, Interactive>();
    for (auto entity : interactiveView)
    {
        if (entity == playerEntity) continue;

        auto& t = world.getComponent<Transform>(entity);
        auto& inter = world.getComponent<Interactive>(entity);

        if (!inter.enabled) continue;

        float dx = t.position.x - playerTransform.position.x;
        float dy = t.position.y - playerTransform.position.y;
        float dist = std::sqrt(dx * dx + dy * dy);

        if (dist < nearestDist)
        {
            nearestDist = dist;
            nearestEntity = entity;
            nearestType = inter.interactionType;
        }
    }

    if (nearestEntity == kNullEntity)
    {
        spdlog::info("附近没有可交互的对象。");
        return;
    }

    // 发布交互事件（使用共享的 InteractionData）
    InteractionData data;
    data.sourceEntity = playerEntity;
    data.targetEntity = nearestEntity;
    data.type = nearestType;

    spdlog::info("Interaction: player={} -> target={} (type={})",
        static_cast<uint32_t>(data.sourceEntity),
        static_cast<uint32_t>(data.targetEntity),
        data.type);

    m_eventBus.publishImmediate(kInteractionEvent, std::move(data));
}

} // namespace engine::ecs
