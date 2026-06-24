/**
 * @file engine/ecs/systems/inventory_system.h
 * @brief 物品系统 — 物品栏、使用、装备、金钱
 *
 * 职责：
 *   - 物品栏配置（maxSlots: 0=禁用, N=限制, -1=无限）
 *   - 物品使用（consumable + useEffects → 修改 Stats）
 *   - 装备/卸下（equipSlot + statModifiers → Equipped 标签）
 *   - 装备槽自定义（equipment.defineSlot）
 *   - 金钱管理
 *   - 物品原型注册（item.define / item.create）
 *
 * 设计：
 *   - 所有操作通过 World 的 ECS 组件完成，不持有额外状态
 *   - 同一装备槽最多 maxCount 件，超限时自动卸下旧装备
 */

#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/component_types.h"
#include "engine/core/event_bus.h"

#include <unordered_map>
#include <string>

namespace engine::ecs {

// =========================================================================
// EquipmentSlotDef — 装备槽定义
// =========================================================================
struct EquipmentSlotDef
{
    std::string key;
    std::string label;
    int         maxCount = 1;  // 该槽最多同时装备几件
};

// =========================================================================
// ItemDef — 物品原型（Lua item.define 注册）
// =========================================================================
struct ItemDef
{
    std::string key;
    std::string name;
    std::string description;
    std::string equipSlot;
    bool        consumable = false;
    std::vector<UseEffect>                    useEffects;
    std::unordered_map<std::string, float>    statModifiers;
};

class InventorySystem : public System
{
public:
    explicit InventorySystem(EventBus& eventBus);
    ~InventorySystem() override = default;

    void onUpdate(World& world, double dt) override;

    // ---- 物品栏配置 ----
    void configure(int maxSlots) { m_configMaxSlots = maxSlots; }
    int  maxSlots() const { return m_configMaxSlots; }

    // ---- 装备槽定义 ----
    void defineSlot(const std::string& key, const std::string& label, int maxCount = 1);
    const EquipmentSlotDef* getSlotDef(const std::string& key) const;
    const std::unordered_map<std::string, EquipmentSlotDef>& allSlots() const { return m_slots; }

    // ---- 物品原型注册 ----
    void registerItemDef(const ItemDef& def);
    const ItemDef* getItemDef(const std::string& key) const;

    /// 从原型创建物品 Entity（返回 Entity，失败返回 kNullEntity）
    Entity createItem(World& world, const std::string& itemKey);

    // ---- 物品操作 ----
    /// 将物品放入背包（检查 maxSlots）
    bool addItem(World& world, Entity owner, Entity item);

    /// 使用物品（触发 useEffects，消耗品销毁）
    bool useItem(World& world, Entity user, Entity item);

    /// 装备物品
    bool equipItem(World& world, Entity owner, Entity item);

    /// 卸下指定槽位的装备
    bool unequipItem(World& world, Entity owner, const std::string& slot);

    /// 卸下指定物品
    bool unequipItemEntity(World& world, Entity owner, Entity item);

    /// 丢弃物品
    bool dropItem(World& world, Entity owner, Entity item);

    // ---- 金钱 ----
    bool addMoney(World& world, Entity entity, int amount);
    bool spendMoney(World& world, Entity entity, int amount);
    int  getMoney(World& world, Entity entity) const;

    // ---- 查询 ----
    Entity findEquippedInSlot(World& world, Entity owner, const std::string& slot) const;

private:
    EventBus& m_eventBus;

    int  m_configMaxSlots = 20;  // -1 = 无限

    std::unordered_map<std::string, EquipmentSlotDef> m_slots;
    std::unordered_map<std::string, ItemDef>          m_itemDefs;

    int countEquippedInSlot(World& world, Entity owner, const std::string& slot) const;
};

} // namespace engine::ecs
