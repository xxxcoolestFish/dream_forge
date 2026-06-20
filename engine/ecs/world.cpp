/**
 * @file engine/ecs/world.cpp
 * @brief ECS World 实现
 */

#include "engine/ecs/world.h"
#include "engine/ecs/system.h"
#include <spdlog/spdlog.h>
#include <algorithm>

namespace engine::ecs {

World::World()
    : m_registry(std::make_unique<entt::registry>())
{
    spdlog::debug("ECS World created");
}

World::~World()
{
    spdlog::debug("ECS World destroyed ({} entities, {} systems)",
        m_registry->alive(), m_systems.size());
}

Entity World::createEntity()
{
    return m_registry->create();
}

void World::destroyEntity(Entity entity)
{
    if (m_registry->valid(entity))
    {
        m_registry->destroy(entity);
    }
}

bool World::isValid(Entity entity) const
{
    return m_registry->valid(entity);
}

void World::registerSystem(std::unique_ptr<System> system)
{
    if (!system)
    {
        spdlog::warn("World::registerSystem: null system ignored");
        return;
    }

    spdlog::debug("Registering system: {}", system->name());
    system->onInit(*this);
    m_systems.push_back(std::move(system));
}

void World::updateSystems(double dt)
{
    // TODO: 按依赖图排序后执行
    // 当前简单按注册顺序执行
    for (auto& system : m_systems)
    {
        system->onUpdate(*this, dt);
    }
}

entt::registry& World::raw()
{
    return *m_registry;
}

const entt::registry& World::raw() const
{
    return *m_registry;
}

} // namespace engine::ecs
