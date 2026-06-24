# 物品系统 (Item)

开发者通过 `item` 表定义物品原型，通过 `engine` 表操作物品实例。

## item.define(key, config)

注册物品原型。

```lua
item.define("health_potion", {
    name        = "生命药水",
    description = "恢复30点生命值",
    consumable  = true,
    useEffects  = {
        { stat = "hp", modify = 30, clampToMax = true },
    },
})
```

### 可装备物品

```lua
item.define("iron_sword", {
    name        = "铁剑",
    description = "普通的铁剑",
    equipSlot   = "weapon",
    statModifiers = {
        attackPower = 5,
        strength   = 2,
    },
})
```

### 字段说明

| 字段 | 类型 | 必填 | 说明 |
|------|------|:--:|------|
| `name` | string | 否 | 显示名称 |
| `description` | string | 否 | 描述文本 |
| `consumable` | bool | 否 | 是否可消耗（使用后销毁） |
| `useEffects` | table | 否 | 使用效果列表 |
| `equipSlot` | string | 否 | 装备槽名（空=不可装备） |
| `statModifiers` | table | 否 | 装备时属性加成 |

### useEffects 结构

```lua
{ stat = "hp", modify = 30, clampToMax = true }
```

| 字段 | 说明 |
|------|------|
| `stat` | 目标属性 key |
| `modify` | 修改量（正=增加，负=减少） |
| `clampToMax` | 是否钳制到 max |

## item.create(key)

从原型创建物品实体。返回 entity ID。

```lua
local potion = item.create("health_potion")
```

## item.getDefinition(key)

查询物品原型。返回 table 或 nil。

## Item 组件

通过 `engine.getItem(entity)` 获取：

```lua
local item = engine.getItem(entity)
print(item.itemId)        -- "health_potion"
print(item.name)          -- "生命药水"
print(item.consumable)    -- true
print(item.equipSlot)     -- "" 或 "weapon"

-- useEffects 数组
for _, eff in ipairs(item.useEffects) do
    print(eff.stat, eff.modify)  -- "hp", 30
end

-- statModifiers map
for k, v in pairs(item.statModifiers) do
    print(k, v)  -- "attackPower", 5
end
```

## 事件

| 事件 | 触发时机 | 数据字段 |
|------|---------|---------|
| `ItemUsed` | 物品被使用 | `userEntity`, `itemEntity`, `itemId` |
