/**
 * @file engine/ecs/systems/movement_system.h
 * @brief 移动系统 — 第一个可运行的 ECS System
 *
 * 每帧遍历所有拥有 Transform 组件的 Entity，执行移动逻辑。
 * Step 3：使用固定方向和速度演示 ECS 管线。
 * Step 4：接入 InputSystem，改为键盘驱动移动。
 */

#pragma once

#include "engine/ecs/system.h"

namespace engine::ecs {

class MovementSystem : public System
{
public:
    MovementSystem();

    void onUpdate(World& world, double dt) override;
};

} // namespace engine::ecs
