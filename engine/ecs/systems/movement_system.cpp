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
    (void)world;
    (void)dt;
    // 玩家移动由 FPS 相机直接处理，MovementSystem 仅保留给后续 AI/NPC 使用
}

} // namespace engine::ecs
