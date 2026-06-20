/**
 * @file engine/ecs/systems/movement_system.h
 * @brief 移动系统 — 键盘 WASD 驱动 Entity 移动
 *
 * Step 3：固定方向演示 ECS 管线。
 * Step 4：接入 InputSystem，WASD 键盘控制。
 */

#pragma once

#include "engine/ecs/system.h"

namespace engine::input { class InputSystem; }

namespace engine::ecs {

class MovementSystem : public System
{
public:
    explicit MovementSystem(input::InputSystem& input);

    void onUpdate(World& world, double dt) override;

private:
    input::InputSystem& m_input;
};

} // namespace engine::ecs
