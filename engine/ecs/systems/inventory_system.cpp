/**
 * @file engine/ecs/systems/inventory_system.cpp
 * @brief InventorySystem 实现 — 物品栏 / 使用 / 装备 / 金钱
 */

#include "engine/ecs/systems/inventory_system.h"
#include "engine/ecs/event_types.h"
#include "engine/ecs/world.h"

#include <spdlog/spdlog.h>
#include <any>

namespace engine::ecs {

InventorySystem::InventorySystem(EventBus& eventBus)
    : System("InventorySystem")
    , m_eventBus(eventBus)
{}

void InventorySystem::onUpdate(World& /*world*/, double /*dt*/)
{
    // 操作全部事件驱动，无需每帧逻辑
}

// =========================================================================
// 装备槽
// =========================================================================

void InventorySystem::defineSlot(const std::string& key,
                                  const std::string& label, int maxCount)
{
    m_slots[key] = { key, label, maxCount };
}

const EquipmentSlotDef* InventorySystem::getSlotDef(const std::string& key) const
{
    auto it = m_slots.find(key);
    return (it != m_slots.end()) ? &it->second : nullptr;
}

// =========================================================================
// 物品原型
// =========================================================================

void InventorySystem::registerItemDef(const ItemDef& def)
{
    m_itemDefs[def.key] = def;
}

const ItemDef* InventorySystem::getItemDef(const std::string& key) const
{
    auto it = m_itemDefs.find(key);
    return (it != m_itemDefs.end()) ? &it->second : nullptr;
}

Entity InventorySystem::createItem(World& world, const std::string& itemKey)
{
    auto* def = getItemDef(itemKey);
    if (!def)
    {
        spdlog::warn("InventorySystem: item def '{}' not found", itemKey);
        return kNullEntity;
    }

    Entity e = world.createEntity();
    auto& item = world.addComponent<Item>(e);
    item.itemId       = def->key;
    item.name         = def->name;
    item.description  = def->description;
    item.equipSlot    = def->equipSlot;
    item.consumable   = def->consumable;
    item.useEffects   = def->useEffects;
    item.statModifiers = def->statModifiers;
    return e;
}

// =========================================================================
// 物品栏
// =========================================================================

bool InventorySystem::addItem(World& world, Entity owner, Entity item)
{
    if (!world.hasComponent<Inventory>(owner))
    {
        spdlog::warn("InventorySystem: entity {} has no Inventory", static_cast<uint32_t>(owner));
        return false;
    }
    if (!world.hasComponent<Item>(item))
    {
        spdlog::warn("InventorySystem: entity {} has no Item component", static_cast<uint32_t>(item));
        return false;
    }

    auto& inv = world.getComponent<Inventory>(owner);

    int limit = (m_configMaxSlots >= 0) ? m_configMaxSlots : m_configMaxSlots;
    if (m_configMaxSlots == 0)
    {
        spdlog::debug("InventorySystem: inventory disabled (maxSlots=0)");
        return false;
    }

    // maxSlots = -1 → 不限制
    if (m_configMaxSlots > 0 && static_cast<int>(inv.items.size()) >= m_configMaxSlots)
    {
        spdlog::debug("InventorySystem: inventory full ({}/{})", inv.items.size(), m_configMaxSlots);
        return false;
    }

    return inv.add(item);
}

// =========================================================================
// 使用
// =========================================================================

bool InventorySystem::useItem(World& world, Entity user, Entity item)
{
    if (!world.isValid(item) || !world.hasComponent<Item>(item))
        return false;

    auto& it = world.getComponent<Item>(item);
    if (!it.consumable)
    {
        spdlog::debug("InventorySystem: item '{}' is not consumable", it.itemId);
        return false;
    }

    // 应用 useEffects
    if (world.hasComponent<Stats>(user))
    {
        auto& stats = world.getComponent<Stats>(user);
        for (const auto& effect : it.useEffects)
        {
            if (effect.clampToMax)
                stats.modifyClamped(effect.stat, effect.modify);
            else
                stats.modify(effect.stat, effect.modify);
        }
    }

    // 发布事件
    ItemUsedData data{ user, item, it.itemId };
    m_eventBus.publishImmediate(ENGINE_EVENT("ItemUsed"), std::any(data));

    // 从背包移除
    if (world.hasComponent<Inventory>(user))
    {
        auto& inv = world.getComponent<Inventory>(user);
        inv.remove(item);
    }

    // 销毁物品实体
    world.destroyEntity(item);

    spdlog::debug("InventorySystem: used item '{}'", it.itemId);
    return true;
}

// =========================================================================
// 装备
// =========================================================================

int InventorySystem::countEquippedInSlot(World& world, Entity owner,
                                          const std::string& slot) const
{
    int count = 0;
    for (auto e : world.view<Equipped>())
    {
        auto& eq = world.getComponent<Equipped>(e);
        if (eq.equippedBy == owner && eq.slot == slot) count++;
    }
    return count;
}

Entity InventorySystem::findEquippedInSlot(World& world, Entity owner,
                                            const std::string& slot) const
{
    for (auto e : world.view<Equipped>())
    {
        auto& eq = world.getComponent<Equipped>(e);
        if (eq.equippedBy == owner && eq.slot == slot) return e;
    }
    return kNullEntity;
}

bool InventorySystem::equipItem(World& world, Entity owner, Entity item)
{
    if (!world.isValid(item) || !world.hasComponent<Item>(item))
        return false;

    auto& it = world.getComponent<Item>(item);
    if (it.equipSlot.empty())
    {
        spdlog::debug("InventorySystem: item '{}' is not equippable", it.itemId);
        return false;
    }

    // 检查装备槽合法性
    auto* slotDef = getSlotDef(it.equipSlot);
    if (!slotDef)
    {
        spdlog::warn("InventorySystem: equip slot '{}' not defined", it.equipSlot);
        return false;
    }

    // 已装备则跳过
    if (world.hasComponent<Equipped>(item))
        return true;

    // 槽满 → 卸下旧装备
    if (countEquippedInSlot(world, owner, it.equipSlot) >= slotDef->maxCount)
    {
        Entity old = findEquippedInSlot(world, owner, it.equipSlot);
        if (old != kNullEntity)
            unequipItemEntity(world, owner, old);
    }

    // 应用 statModifiers
    if (world.hasComponent<Stats>(owner))
    {
        auto& stats = world.getComponent<Stats>(owner);
        for (const auto& [sk, sv] : it.statModifiers)
            stats.modify(sk, sv);
    }

    // 标记为已装备
    auto& eq = world.addComponent<Equipped>(item);
    eq.equippedBy = owner;
    eq.slot       = it.equipSlot;

    ItemEquippedData data{ owner, item, it.equipSlot, true };
    m_eventBus.publishImmediate(ENGINE_EVENT("ItemEquipped"), std::any(data));

    spdlog::debug("InventorySystem: equipped '{}' to slot '{}'", it.itemId, it.equipSlot);
    return true;
}

bool InventorySystem::unequipItemEntity(World& world, Entity owner, Entity item)
{
    if (!world.isValid(item) || !world.hasComponent<Equipped>(item))
        return false;

    auto& eq = world.getComponent<Equipped>(item);
    auto& it = world.getComponent<Item>(item);

    // 移除 statModifiers
    if (world.hasComponent<Stats>(owner))
    {
        auto& stats = world.getComponent<Stats>(owner);
        for (const auto& [sk, sv] : it.statModifiers)
            stats.modify(sk, -sv);
    }

    std::string slot = eq.slot;
    world.removeComponent<Equipped>(item);

    ItemEquippedData data{ owner, item, slot, false };
    m_eventBus.publishImmediate(ENGINE_EVENT("ItemEquipped"), std::any(data));

    spdlog::debug("InventorySystem: unequipped '{}' from slot '{}'", it.itemId, slot);
    return true;
}

bool InventorySystem::unequipItem(World& world, Entity owner, const std::string& slot)
{
    Entity item = findEquippedInSlot(world, owner, slot);
    if (item == kNullEntity) return false;
    return unequipItemEntity(world, owner, item);
}

// =========================================================================
// 丢弃
// =========================================================================

bool InventorySystem::dropItem(World& world, Entity owner, Entity item)
{
    if (!world.isValid(item) || !world.hasComponent<Item>(item))
        return false;

    // 若已装备，先卸下
    if (world.hasComponent<Equipped>(item))
        unequipItemEntity(world, owner, item);

    // 从背包移除
    if (world.hasComponent<Inventory>(owner))
    {
        auto& inv = world.getComponent<Inventory>(owner);
        inv.remove(item);
    }

    // 不销毁实体，留给世界层（可在后续做掉落物渲染）
    return true;
}

// =========================================================================
// 金钱
// =========================================================================

bool InventorySystem::addMoney(World& world, Entity entity, int amount)
{
    if (!world.hasComponent<Inventory>(entity) || amount < 0)
        return false;

    auto& inv = world.getComponent<Inventory>(entity);
    int old = inv.money;
    if (!inv.addMoney(amount)) return false;

    MoneyChangedData data{ entity, old, inv.money };
    m_eventBus.publishImmediate(ENGINE_EVENT("MoneyChanged"), std::any(data));
    return true;
}

bool InventorySystem::spendMoney(World& world, Entity entity, int amount)
{
    if (!world.hasComponent<Inventory>(entity))
        return false;

    auto& inv = world.getComponent<Inventory>(entity);
    int old = inv.money;
    if (!inv.spendMoney(amount)) return false;

    MoneyChangedData data{ entity, old, inv.money };
    m_eventBus.publishImmediate(ENGINE_EVENT("MoneyChanged"), std::any(data));
    return true;
}

int InventorySystem::getMoney(World& world, Entity entity) const
{
    if (!world.hasComponent<Inventory>(entity)) return 0;
    return world.getComponent<Inventory>(entity).money;
}

} // namespace engine::ecs
