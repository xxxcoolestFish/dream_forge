/**
 * @file engine/ecs/system.h
 * @brief System 基类 — 所有 ECS 系统的抽象
 *
 * System 包含游戏逻辑，每帧对匹配的 Component 集合进行操作。
 *
 * 使用方式：
 *   1. 继承 System 基类
 *   2. 实现 onUpdate(double dt)
 *   3. 通过 World::registerSystem() 注册
 *
 * 注意事项：
 *   - System 之间通过 EventBus 通信，不应直接互相调用
 *   - onUpdate 在固定时间步下被调用
 *   - 不要在 onUpdate 中做 IO 或阻塞操作
 */

#pragma once

#include <string>

namespace engine::ecs {

class World;

class System
{
public:
    explicit System(const std::string& name) : m_name(name) {}
    virtual ~System() = default;

    // 系统名称（用于调试和依赖排序）
    const std::string& name() const { return m_name; }

    // 初始化（在第一次 update 之前调用）
    virtual void onInit(World& world) {}

    // 每帧更新
    virtual void onUpdate(World& world, double dt) = 0;

    // 关闭清理
    virtual void onShutdown(World& world) {}

    // 依赖的其他 System 名称列表（确保先于自己执行）
    virtual std::vector<std::string> dependencies() const { return {}; }

private:
    std::string m_name;
};

} // namespace engine::ecs
