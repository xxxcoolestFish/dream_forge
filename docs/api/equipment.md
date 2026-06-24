# 装备系统 (Equipment)

开发者通过 `equipment` 表定义装备槽，通过 `engine` 表操作装备。

## equipment.defineSlot(key, config)

定义装备槽。

```lua
equipment.defineSlot("weapon",    { label = "武器", maxCount = 1 })
equipment.defineSlot("head",      { label = "头部", maxCount = 1 })
equipment.defineSlot("body",      { label = "身体", maxCount = 1 })
equipment.defineSlot("accessory", { label = "饰品", maxCount = 2 })
```

### 字段说明

| 字段 | 类型 | 必填 | 说明 |
|------|------|:--:|------|
| `label` | string | 否 | 显示名称 |
| `maxCount` | int | 否 | 该槽最多装备件数（默认 1） |

## equipment.getSlots()

返回所有已定义的装备槽。

```lua
local slots = equipment.getSlots()
for key, info in pairs(slots) do
    print(key, info.label, info.maxCount)
end
```

## 装备操作

### engine.equipItem(owner, item) → bool

装备物品。自动检查槽位容量，满时先卸下旧装备。

```lua
local player = engine.getPlayer()
engine.equipItem(player, swordEntity)
```

装备时自动将 `Item.statModifiers` 中的加成应用到玩家 Stats。如铁剑 `{ attackPower=5, strength=2 }` 会直接修改玩家对应属性值。

### engine.unequipItem(owner, slot) → bool

卸下指定槽位的装备。

```lua
engine.unequipItem(player, "weapon")
```

卸下时反向扣除 statModifiers 加成。

### engine.findEquipped(owner, slot) → entityId

查询指定槽位当前装备的物品实体。

```lua
local weapon = engine.findEquipped(player, "weapon")
if weapon ~= kNullEntity then
    local item = engine.getItem(weapon)
    print("当前武器: " .. item.name)
end
```

## Equipped 组件

已装备的物品实体会被挂上 `Equipped` 标签：

```lua
if engine.hasEquipped(itemEntity) then
    local eq = engine.findEquipped(itemEntity)
    print("装备者: " .. eq.equippedBy)
    print("槽位: " .. eq.slot)
end
```

## 事件

| 事件 | 触发时机 | 数据字段 |
|------|---------|---------|
| `ItemEquipped` | 装备/卸下 | `entity`, `itemEntity`, `slot`, `equipped`(bool) |
