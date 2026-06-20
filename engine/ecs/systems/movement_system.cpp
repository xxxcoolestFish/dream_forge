/**
 * @file engine/ecs/systems/movement_system.cpp
 * @brief 移动系统实现
 */

#include "engine/ecs/systems/movement_system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"

#include <spdlog/spdlog.h>
#include <glm/glm.hpp>

namespace engine::ecs {

MovementSystem::MovementSystem()
    : System("MovementSystem")
{
}

void MovementSystem::onUpdate(World& world, double dt)
{
    // 遍历所有拥有 Transform 组件的 Entity
    auto view = world.view<Transform>();

    for (auto entity : view)
    {
        auto& transform = world.getComponent<Transform>(entity);

        // Step 3：固定方向移动（右下方），演示 ECS 管线
        // Step 4 将接入 InputSystem，改为键盘驱动
        const float speed = 60.0f; // 像素/秒
        transform.position.x += speed * static_cast<float>(dt);
        transform.position.y += speed * 0.3f * static_cast<float>(dt);

        // 首次运行时打印一次，验证 System 执行
        static bool firstRun = true;
        if (firstRun)
        {
            spdlog::info("MovementSystem: first update — moving {} entities", view.size());
            firstRun = false;
        }
    }
}

} // namespace engine::ecs
