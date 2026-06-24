/**
 * @file engine/ecs/systems/stat_system.cpp
 * @brief StatSystem 实现 — 属性钳制/恢复/派生/归零
 */

#include "engine/ecs/systems/stat_system.h"
#include "engine/ecs/event_types.h"
#include "engine/ecs/world.h"

#include <spdlog/spdlog.h>
#include <sol/sol.hpp>

namespace engine::ecs {

StatSystem::StatSystem(EventBus& eventBus)
    : System("StatSystem")
    , m_eventBus(eventBus)
{}

void StatSystem::registerDefinition(const StatDef& def)
{
    m_defs[def.key] = def;

    if (def.derived)
    {
        m_derivedKeys.insert(def.key);
        for (const auto& dep : def.dependsOn)
            m_depGraph[dep].insert(def.key);
    }
}

void StatSystem::applyDefinitions(World& world, Entity entity)
{
    if (!world.hasComponent<Stats>(entity)) return;

    auto& stats = world.getComponent<Stats>(entity);
    for (const auto& [key, def] : m_defs)
    {
        if (def.derived) continue;  // 派生属性由 compute 函数赋值

        // 设置 min/max（若未手动设置）
        if (stats.minValues.find(key) == stats.minValues.end())
            stats.minValues[key] = def.minValue;
        if (stats.maxValues.find(key) == stats.maxValues.end())
            stats.maxValues[key] = def.maxValue;

        // 初始化默认值（若 key 尚不存在）
        if (stats.values.find(key) == stats.values.end())
            stats.values[key] = def.defaultValue;
    }

    // 初始化缓存
    auto& prev = m_prevValues[entity];
    for (const auto& [k, v] : stats.values)
        prev[k] = v;
}

const StatDef* StatSystem::getDefinition(const std::string& key) const
{
    auto it = m_defs.find(key);
    return (it != m_defs.end()) ? &it->second : nullptr;
}

void StatSystem::modifyStat(World& world, Entity entity,
                            const std::string& key, float delta)
{
    if (!world.hasComponent<Stats>(entity)) return;
    auto& stats = world.getComponent<Stats>(entity);
    float oldVal = stats.get(key);
    stats.modifyClamped(key, delta);
    float newVal = stats.get(key);

    if (newVal != oldVal)
    {
        // 受伤冷却
        if (newVal < oldVal)
        {
            auto defIt = m_defs.find(key);
            if (defIt != m_defs.end() && defIt->second.regenCooldown > 0.0f)
                m_regen[entity][key].cooldown = defIt->second.regenCooldown;
        }

        markDependentsDirty(entity, key);
    }
}

// =========================================================================
// 内部步骤
// =========================================================================

void StatSystem::processRegen(World& world, double dt)
{
    float dtf = static_cast<float>(dt);
    for (auto e : world.view<Stats>())
    {
        auto& stats = world.getComponent<Stats>(e);
        for (auto& [key, def] : m_defs)
        {
            if (def.derived || def.regenRate <= 0.0f) continue;
            if (stats.values.find(key) == stats.values.end()) continue;

            auto& rs = m_regen[e][key];
            if (rs.cooldown > 0.0f)
            {
                rs.cooldown -= dtf;
                if (rs.cooldown > 0.0f) continue;
                rs.cooldown = 0.0f;
            }

            float cur  = stats.get(key);
            float maxV = def.maxValue;
            if (maxV <= 0.0f || cur < maxV)
                stats.setClamped(key, cur + def.regenRate * dtf);
        }
    }
}

void StatSystem::processClamp(World& world)
{
    for (auto e : world.view<Stats>())
    {
        auto& stats = world.getComponent<Stats>(e);
        for (auto& [key, val] : stats.values)
            stats.setClamped(key, val);  // 确保所有值在 [min, max] 内
    }
}

void StatSystem::processDepletion(World& world)
{
    for (auto e : world.view<Stats>())
    {
        auto& stats = world.getComponent<Stats>(e);
        auto& prev = m_prevValues[e];

        for (auto& [key, def] : m_defs)
        {
            if (def.derived || def.onDepleted.empty()) continue;
            if (stats.values.find(key) == stats.values.end()) continue;

            float val = stats.get(key);
            float old = (prev.count(key)) ? prev[key] : def.defaultValue;
            float minV = def.minValue;

            if (val <= minV && old > minV)
            {
                // 本帧刚归零
                StatDepletedData data{ e, key };
                auto eventId = ::engine::detail::fnv1a32(def.onDepleted.c_str());
                m_eventBus.publishImmediate(eventId, std::any(data));

                if (!world.hasComponent<Dead>(e))
                    world.addComponent<Dead>(e);

                spdlog::debug("StatSystem: entity {} stat '{}' depleted", static_cast<uint32_t>(e), key);
            }
        }
    }
}

void StatSystem::processDerived(World& world)
{
    for (auto e : world.view<Stats>())
    {
        auto it = m_dirtyDerived.find(e);
        if (it == m_dirtyDerived.end() || it->second.empty()) continue;

        auto& stats = world.getComponent<Stats>(e);
        for (const auto& dk : it->second)
            recomputeDerived(world, e, dk);

        it->second.clear();
    }
}

void StatSystem::detectAndPublishChanges(World& world)
{
    for (auto e : world.view<Stats>())
    {
        auto& stats = world.getComponent<Stats>(e);
        auto& prev = m_prevValues[e];

        for (auto& [key, val] : stats.values)
        {
            float oldVal = prev[key];  // 新 key 默认 0
            if (val == oldVal) continue;

            StatChangedData data{ e, key, oldVal, val };
            m_eventBus.publishImmediate(ENGINE_EVENT("StatChanged"), std::any(data));

            if (val < oldVal)
            {
                auto defIt = m_defs.find(key);
                if (defIt != m_defs.end() && defIt->second.regenCooldown > 0.0f)
                    m_regen[e][key].cooldown = defIt->second.regenCooldown;
            }

            markDependentsDirty(e, key);
            prev[key] = val;
        }
    }
}

void StatSystem::markDependentsDirty(Entity entity, const std::string& baseKey)
{
    auto git = m_depGraph.find(baseKey);
    if (git == m_depGraph.end()) return;

    for (const auto& derivedKey : git->second)
        m_dirtyDerived[entity].insert(derivedKey);
}

bool StatSystem::recomputeDerived(World& world, Entity entity, const std::string& key)
{
    if (!m_lua) return false;

    if (!world.hasComponent<Stats>(entity)) return false;
    auto& stats = world.getComponent<Stats>(entity);

    sol::state& lua = *m_lua;

    // 查找 Lua 全局注册表中的 compute 函数: _DERIVED_COMPUTE["key"]
    sol::table registry = lua["_DERIVED_COMPUTE"];
    if (!registry.valid()) return false;

    sol::protected_function compute = registry[key];
    if (!compute.valid()) return false;

    // 将 Stats* 作为 userdata 传给 compute 函数
    auto result = compute(&stats);
    if (!result.valid())
    {
        sol::error err = result;
        spdlog::warn("StatSystem: derived compute '{}' error: {}", key, err.what());
        return false;
    }

    float newVal = result.get<float>();
    stats.setClamped(key, newVal);
    return true;
}

// =========================================================================
// 每帧入口
// =========================================================================

void StatSystem::onUpdate(World& world, double dt)
{
    // 0. 自动初始化新出现的 Stats 实体
    for (auto e : world.view<Stats>())
    {
        if (m_prevValues.find(e) == m_prevValues.end())
            applyDefinitions(world, e);
    }

    // 1-5. 管线
    processRegen(world, dt);
    processClamp(world);
    processDepletion(world);
    processDerived(world);
    detectAndPublishChanges(world);
}

} // namespace engine::ecs
