/**
 * @file engine/ecs/systems/movement_system.cpp
 * @brief 移动系统 — 键盘 WASD 驱动 Entity 移动
 */

#include "engine/ecs/systems/movement_system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"
#include "engine/input/input_system.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <cmath>

namespace engine::ecs {

MovementSystem::MovementSystem(input::InputSystem& input)
    : System("MovementSystem")
    , m_input(input)
{
}

void MovementSystem::onUpdate(World& world, double dt)
{
    // 从输入读取移动方向（WASD + 方向键）
    float moveX = 0.0f;
    float moveY = 0.0f;

    if (m_input.isKeyHeld(GLFW_KEY_W) || m_input.isKeyHeld(GLFW_KEY_UP))    moveY += 1.0f;
    if (m_input.isKeyHeld(GLFW_KEY_S) || m_input.isKeyHeld(GLFW_KEY_DOWN))  moveY -= 1.0f;
    if (m_input.isKeyHeld(GLFW_KEY_A) || m_input.isKeyHeld(GLFW_KEY_LEFT))  moveX -= 1.0f;
    if (m_input.isKeyHeld(GLFW_KEY_D) || m_input.isKeyHeld(GLFW_KEY_RIGHT)) moveX += 1.0f;

    // 归一化
    float length = std::sqrt(moveX * moveX + moveY * moveY);
    if (length > 0.0f)
    {
        moveX /= length;
        moveY /= length;
    }

    // 无输入时不移动
    if (length == 0.0f) return;

    const float speed = 150.0f; // 像素/秒

    // 移动所有 Entity（Step 4：均受键盘控制；后续用 Player 组件标记）
    auto view = world.view<Transform>();
    for (auto entity : view)
    {
        auto& t = world.getComponent<Transform>(entity);
        t.position.x += moveX * speed * static_cast<float>(dt);
        t.position.y += moveY * speed * static_cast<float>(dt);
    }

    // 每秒打印一次位置（减少日志噪音）
    static double logTimer = 0.0;
    logTimer += dt;
    if (logTimer >= 1.0 && view.size() > 0)
    {
        auto& first = world.getComponent<Transform>(*view.begin());
        spdlog::debug("Movement: {} entities, first pos=({:.0f}, {:.0f})",
            view.size(), first.position.x, first.position.y);
        logTimer = 0.0;
    }
}

} // namespace engine::ecs
