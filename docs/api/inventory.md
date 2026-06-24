# 物品栏系统 (Inventory)

开发者通过 `inventory` 表配置物品栏规则，通过 `engine` 表操作背包和金钱。

## inventory.configure(config)

配置物品栏全局规则。

```lua
inventory.configure({
    maxSlots = 20,   -- 0=不使用物品栏, N=限制N个, -1=无限存储
})
```

## 物品操作

### engine.useItem(user, item) → bool

使用物品。触发 `useEffects`，消耗品使用后销毁。

```lua
local player = engine.getPlayer()
local ok = engine.useItem(player, potionEntity)
```

### engine.addItem(owner, item) → bool

将物品放入背包（受 maxSlots 限制）。一般通过 `Inventory:add()` 完成：

```lua
local inv = engine.getInventory(player)
inv:add(itemEntity)
```

## 金钱操作

金钱存储在 `Inventory` 组件上，非独立 stat。

### engine.addMoney(entity, amount) → bool

```lua
engine.addMoney(player, 100)    -- 加 100 金币
engine.addMoney(player, -50)    -- 返回 false（不允许负数）
```

### engine.spendMoney(entity, amount) → bool

```lua
if engine.spendMoney(player, 200) then
    print("购买成功")
else
    print("金币不足")
end
```

### engine.getMoney(entity) → int

```lua
local gold = engine.getMoney(player)
print("金币: " .. gold)
```

## Inventory 组件

通过 `engine.getInventory(entity)` 获取：

```lua
local inv = engine.getInventory(player)

-- 物品列表（entity ID 数组）
for _, itemId in ipairs(inv.items) do
    local item = engine.getItem(itemId)
    print(item.name)
end

-- 金钱
print(inv.money)
inv:addMoney(50)
inv:spendMoney(30)

-- 容量
print(inv.maxSlots)
inv:add(newItemEntity)       -- 添加物品
inv:remove(itemEntity)       -- 移除物品
print(inv:isFull())          -- 是否已满
```

## 事件

| 事件 | 触发时机 | 数据字段 |
|------|---------|---------|
| `MoneyChanged` | 金钱变动 | `entity`, `oldAmount`, `newAmount` |
