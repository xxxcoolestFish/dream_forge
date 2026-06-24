# 属性系统 (StatSystem)

开发者通过 `stat` 表定义游戏中的属性体系。引擎不预设任何属性名。

## stat.define(key, config)

注册一个属性定义。

```lua
stat.define("hp", {
    displayName   = "生命值",   -- 显示名称
    default       = 100,        -- 默认值
    max           = 100,        -- 最大值（-1 = 无上限）
    min           = 0,          -- 最小值
    regen         = 1.0,        -- 每秒恢复量（nil = 不恢复）
    regenCooldown = 5.0,        -- 受伤后冷却秒数
    onDepleted    = "onDeath",  -- 归零时发布的 EventBus 事件名
    category      = "vital",    -- 分类标签（供 UI 使用）
})
```

### 派生属性

不直接存储，通过公式从其他属性计算得出。仅依赖属性变化时重算，非每帧。

```lua
stat.define("attackPower", {
    displayName = "攻击力",
    derived     = true,
    dependsOn   = { "strength", "dexterity" },
    category    = "combat",
})

-- compute 函数单独设置
stat.setCompute("attackPower", function(stats)
    return (stats:get("strength") or 10) * 2
         + (stats:get("dexterity") or 5) * 0.5
end)
```

### 字段说明

| 字段 | 类型 | 必填 | 说明 |
|------|------|:--:|------|
| `displayName` | string | 否 | 显示名称 |
| `default` | number | 否 | 默认值（默认 0） |
| `max` | number | 否 | 最大值（0=禁用，-1=无上限） |
| `min` | number | 否 | 最小值 |
| `regen` | number | 否 | 每秒恢复量（0=不恢复） |
| `regenCooldown` | number | 否 | 受伤后恢复冷却（秒） |
| `onDepleted` | string | 否 | 归零事件名 |
| `category` | string | 否 | 分类标签 |
| `derived` | bool | 否 | 是否为派生属性 |
| `dependsOn` | table | 否 | 依赖的基础属性 key 列表 |

## stat.setCompute(key, fn)

为派生属性设置计算函数。

```lua
stat.setCompute("attackPower", function(stats)
    -- stats 是 Stats* 的 Lua userdata
    return stats:get("strength") * 2
end)
```

## stat.getDefinition(key)

查询属性定义。返回 table 或 nil。

```lua
local def = stat.getDefinition("hp")
if def then
    print(def.displayName)  -- "生命值"
    print(def.max)          -- 100
end
```

## Stats 组件方法

通过 `engine.getStats(entity)` 获取 Stats 对象后，可使用以下方法：

```lua
local stats = engine.getStats(player)

-- 读
local hp = stats:get("hp")            -- 获取值，不存在返回 0
local hp = stats:get("hp", 100)       -- 自定义默认值

-- 写（无钳制）
stats:set("hp", 200)
stats:modify("hp", -10)               -- 增加/减少

-- 写（钳制到 [min, max]）
stats:setClamped("hp", 200)           -- 自动钳制到 maxValues["hp"]
stats:modifyClamped("hp", 50)         -- 自动钳制

-- 查询
stats:isAtMax("hp")                   -- 是否已达最大值
stats:isDepleted("hp")                -- 是否已归零

-- 边界值
stats.maxValues["hp"] = 100           -- 设置最大值
stats.minValues["hp"] = 0             -- 设置最小值
```

## 事件

| 事件 | 触发时机 | 数据字段 |
|------|---------|---------|
| `StatChanged` | 任意属性值变化 | `entity`, `statKey`, `oldValue`, `newValue` |
| `StatDepleted` | 属性归零 | `entity`, `statKey` |
| `{onDepleted}` | 属性归零（自定义事件名） | `entity`, `statKey` |
