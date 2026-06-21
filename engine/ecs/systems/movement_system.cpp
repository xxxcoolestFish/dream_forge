/**
 * @file engine/ecs/systems/movement_system.cpp
 * @brief 移动系统 — 键盘 WASD 驱动 Player Entity 移动
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
    // 读取输入方向
    float moveX = 0.0f;
    float moveY = 0.0f;

    if (m_input.isKeyHeld(GLFW_KEY_W) || m_input.isKeyHeld(GLFW_KEY_UP))    moveY -= 1.0f;
    if (m_input.isKeyHeld(GLFW_KEY_S) || m_input.isKeyHeld(GLFW_KEY_DOWN))  moveY += 1.0f;
    if (m_input.isKeyHeld(GLFW_KEY_A) || m_input.isKeyHeld(GLFW_KEY_LEFT))  moveX -= 1.0f;
    if (m_input.isKeyHeld(GLFW_KEY_D) || m_input.isKeyHeld(GLFW_KEY_RIGHT)) moveX += 1.0f;

    // 归一化
    float length = std::sqrt(moveX * moveX + moveY * moveY);
    if (length > 0.0f)
    {
        moveX /= length;
        moveY /= length;
    }
    else
    {
        return; // 无输入，不更新
    }

    const float speed = 150.0f; // 像素/秒

    // 只移动拥有 Player 组件的 Entity
    auto view = world.view<Transform, Player>();
    for (auto entity : view)
    {
        auto& t = world.getComponent<Transform>(entity);
        t.position.x += moveX * speed * static_cast<float>(dt);
        t.position.y += moveY * speed * static_cast<float>(dt);
    }

    // 每秒打印一次位置
    static double logTimer = 0.0;
    logTimer += dt;
    if (logTimer >= 1.0 && view.begin() != view.end())
    {
        auto& first = world.getComponent<Transform>(*view.begin());
        spdlog::debug("Player pos=({:.0f}, {:.0f})", first.position.x, first.position.y);
        logTimer = 0.0;
    }
}

} // namespace engine::ecs
