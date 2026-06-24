/**
 * @file engine/ecs/systems/stat_system.h
 * @brief 属性系统 — 统一管理所有实体的 Stats 组件
 *
 * 职责：
 *   - 属性定义注册表（stat.define() → StatDef）
 *   - 每帧：钳制 / 自然恢复 / 归零检测 / 派生属性重算 / 变更通知
 *   - 派生属性依赖图 + dirty 标记（变更时重算）
 *   - 归零时发布 onDepleted 事件 + Dead 标签
 *
 * 设计：
 *   - 不预设任何属性名。开发者通过 Lua stat.define() 定义属性体系
 *   - 缓存上帧值做差异检测，不要求所有写操作都经过本系统
 */

#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/component_types.h"
#include "engine/core/event_bus.h"

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <string>

#include <sol/sol.hpp>

namespace engine::ecs {

// =========================================================================
// StatDef — 单个属性的定义（开发者通过 Lua stat.define() 注册）
// =========================================================================
struct StatDef
{
    std::string key;
    std::string displayName;
    float       defaultValue = 0.0f;
    float       maxValue     = 0.0f;
    float       minValue     = 0.0f;
    float       regenRate    = 0.0f;   // 每秒恢复量，0 = 不恢复
    float       regenCooldown = 0.0f;  // 受伤后冷却秒数
    std::string onDepleted;            // 归零时发布的事件名
    std::string category;
    bool        derived    = false;
    std::vector<std::string> dependsOn;  // 依赖的基础属性
};

class StatSystem : public System
{
public:
    explicit StatSystem(EventBus& eventBus);
    ~StatSystem() override = default;

    void onUpdate(World& world, double dt) override;

    // ---- 属性注册表（从 Lua stat.define 调用） ----
    void registerDefinition(const StatDef& def);

    /// 将注册表中的定义应用到指定实体（初始化默认值 + min/max）
    void applyDefinitions(World& world, Entity entity);

    const StatDef* getDefinition(const std::string& key) const;
    const std::unordered_map<std::string, StatDef>& allDefinitions() const { return m_defs; }

    /// 按分类筛选属性定义
    std::vector<const StatDef*> getByCategory(const std::string& category) const;

    // ---- 派生属性 Lua 回调 ----
    void setLuaState(class sol::state* L) { m_lua = L; }

    // ---- 属性操作（钳制 + 通知） ----
    void modifyStat(World& world, Entity entity, const std::string& key, float delta);

private:
    EventBus& m_eventBus;
    sol::state* m_lua = nullptr;

    // 属性注册表
    std::unordered_map<std::string, StatDef> m_defs;

    // 派生属性：依赖图（baseKey → {derivedKey...}）
    std::unordered_map<std::string, std::unordered_set<std::string>> m_depGraph;

    // 派生属性：compute 函数引用（Lua sol::protected_function 的生命期由 sol::state 管理，
    // 此处存储 key，实际调用时从 Lua 全局表 _DERIVED_COMPUTE[key] 获取）
    std::unordered_set<std::string> m_derivedKeys;

    // 恢复状态追踪：entity → (statKey → cooldown + accumulatedTime)
    struct RegenState { float cooldown = 0.0f; float accum = 0.0f; };
    std::unordered_map<Entity, std::unordered_map<std::string, RegenState>> m_regen;

    // 上帧值缓存：entity → (statKey → value)，用于差异检测
    std::unordered_map<Entity, std::unordered_map<std::string, float>> m_prevValues;

    // 被标记为 dirty 的派生属性：entity → {statKey...}
    std::unordered_map<Entity, std::unordered_set<std::string>> m_dirtyDerived;

    // ---- 内部步骤 ----
    void processRegen(World& world, double dt);
    void processClamp(World& world);
    void processDepletion(World& world);
    void processDerived(World& world);
    void detectAndPublishChanges(World& world);

    void markDependentsDirty(Entity entity, const std::string& baseKey);
    bool recomputeDerived(World& world, Entity entity, const std::string& key);
};

} // namespace engine::ecs
