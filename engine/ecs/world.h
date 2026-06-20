/**
 * @file engine/ecs/world.h
 * @brief ECS World — 基于 EnTT 的注册表封装
 *
 * 负责：
 *   - Entity 的创建与销毁
 *   - Component 的添加/移除/查询
 *   - System 的注册与执行调度
 *
 * 设计：
 *   - 封装 entt::registry，提供更简洁的 API
 *   - 单例模式（整个游戏只有一个 World）
 *   - System 按依赖图顺序执行
 *
 * 注意事项：
 *   - 不要在 System 执行期间创建/销毁 Entity（会导致迭代器失效）
 *     ——使用 deferred 机制延迟到 System 执行完毕后处理
 */

#pragma once

#include <entt/entt.hpp>
#include <memory>
#include <vector>
#include <string>

namespace engine::ecs {

class System; // 前向声明

class World
{
public:
    World();
    ~World();

    // --- Entity 操作 ---
    Entity createEntity();
    void   destroyEntity(Entity entity);
    bool   isValid(Entity entity) const;

    // --- Component 操作 ---
    template<typename T, typename... Args>
    T& addComponent(Entity entity, Args&&... args);

    template<typename T>
    void removeComponent(Entity entity);

    template<typename T>
    bool hasComponent(Entity entity) const;

    template<typename T>
    T& getComponent(Entity entity);

    template<typename T>
    const T& getComponent(Entity entity) const;

    // --- 批量查询 ---
    template<typename... Components>
    auto view();

    // --- System 管理 ---
    void registerSystem(std::unique_ptr<System> system);
    void updateSystems(double dt);

    // --- 直接访问底层 EnTT（用于高级操作）---
    entt::registry& raw();
    const entt::registry& raw() const;

private:
    std::unique_ptr<entt::registry> m_registry;
    std::vector<std::unique_ptr<System>> m_systems;
};

// =========================================================================
// 模板实现（必须在头文件中）
// =========================================================================

template<typename T, typename... Args>
T& World::addComponent(Entity entity, Args&&... args)
{
    return m_registry->emplace<T>(entity, std::forward<Args>(args)...);
}

template<typename T>
void World::removeComponent(Entity entity)
{
    m_registry->remove<T>(entity);
}

template<typename T>
bool World::hasComponent(Entity entity) const
{
    return m_registry->all_of<T>(entity);
}

template<typename T>
T& World::getComponent(Entity entity)
{
    return m_registry->get<T>(entity);
}

template<typename T>
const T& World::getComponent(Entity entity) const
{
    return m_registry->get<T>(entity);
}

template<typename... Components>
auto World::view()
{
    return m_registry->view<Components...>();
}

} // namespace engine::ecs
