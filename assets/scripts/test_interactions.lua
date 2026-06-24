-- =============================================================================
-- assets/scripts/test_interactions.lua
-- 交互逻辑测试：物品使用 → 属性变化 / 装备 → 属性加成 / 金钱操作
-- =============================================================================

local player = engine.getPlayer()
local passed, failed = 0, 0

local function assert(cond, msg)
    if cond then
        passed = passed + 1
        print("  [PASS] " .. msg)
    else
        failed = failed + 1
        print("  [FAIL] " .. msg)
    end
end

print("=== Interaction Tests ===")

-- =============================================================================
-- 测试 1: stat 定义 + 默认值
-- =============================================================================
print("--- Test 1: stat.define defaults ---")
stat.define("test_hp", { default = 100, max = 100, min = 0, category = "vital" })
local stats = engine.getStats(player)
assert(stats:get("test_hp", 0) == 100, "default value applied")
assert(stats.maxValues["test_hp"] == 100, "maxValues set")
assert(stats.minValues["test_hp"] == 0,  "minValues set")

-- =============================================================================
-- 测试 2: setClamped / modifyClamped
-- =============================================================================
print("--- Test 2: clamping ---")
stats:setClamped("test_hp", 150)
assert(stats:get("test_hp", 0) == 100, "setClamped capped to max=100")

stats:setClamped("test_hp", -10)
assert(stats:get("test_hp", 0) == 0, "setClamped floored to min=0")

stats:setClamped("test_hp", 50)
stats:modifyClamped("test_hp", 80)
assert(stats:get("test_hp", 0) == 100, "modifyClamped capped to max")

stats:setClamped("test_hp", 5)
stats:modifyClamped("test_hp", -100)
assert(stats:get("test_hp", 0) == 0, "modifyClamped floored to min")

-- =============================================================================
-- 测试 3: isAtMax / isDepleted
-- =============================================================================
print("--- Test 3: isAtMax / isDepleted ---")
stats:setClamped("test_hp", 100)
assert(stats:isAtMax("test_hp"), "isAtMax=true at 100/100")
stats:setClamped("test_hp", 0)
assert(stats:isDepleted("test_hp"), "isDepleted=true at 0")
stats:setClamped("test_hp", 50)
assert(not stats:isAtMax("test_hp"), "isAtMax=false at 50/100")
assert(not stats:isDepleted("test_hp"), "isDepleted=false at 50")

-- =============================================================================
-- 测试 4: 物品使用 → 属性变化
-- =============================================================================
print("--- Test 4: item use → stat change ---")

-- 定义物品
item.define("test_potion", {
    name        = "测试药水",
    consumable  = true,
    useEffects  = { { stat = "test_hp", modify = 30, clampToMax = true } },
})

-- 创建实例
local potion = item.create("test_potion")
assert(potion ~= kNullEntity, "item created")

-- 放入背包
local inv = engine.getInventory(player)
inv:add(potion)
local itemCountBefore = #inv.items

-- 设 HP 为 50
stats:set("test_hp", 50)

-- 使用物品
local used = engine.useItem(player, potion)
assert(used, "useItem succeeded")
assert(stats:get("test_hp", 0) == 80, "HP: 50 + 30 = 80")

-- 物品从背包移除
assert(#inv.items == itemCountBefore - 1, "item removed from inventory")

-- 测试钳制：设 HP=90，使用后应到 100 而非 120
stats:set("test_hp", 90)
local potion2 = item.create("test_potion")
inv:add(potion2)
engine.useItem(player, potion2)
assert(stats:get("test_hp", 0) == 100, "useItem clamped to max=100")

-- 测试负数 effect
item.define("test_poison", {
    name        = "毒药",
    consumable  = true,
    useEffects  = { { stat = "test_hp", modify = -20, clampToMax = false } },
})
local poison = item.create("test_poison")
inv:add(poison)
stats:set("test_hp", 80)
engine.useItem(player, poison)
assert(stats:get("test_hp", 0) == 60, "negative effect: 80 - 20 = 60")

-- =============================================================================
-- 测试 5: 装备 → 属性加成
-- =============================================================================
print("--- Test 5: equip → stat bonus ---")
equipment.defineSlot("weapon", { label = "武器" })

stat.define("test_str", { default = 10, max = 99, min = 1 })
stats:set("test_str", 10)

item.define("test_sword", {
    name      = "测试剑",
    equipSlot = "weapon",
    statModifiers = { test_str = 5 },
})
local sword = item.create("test_sword")
inv:add(sword)

-- 装备
local ok = engine.equipItem(player, sword)
assert(ok, "equipItem succeeded")
assert(stats:get("test_str", 0) == 15, "str: 10 + 5 = 15")
assert(engine.hasEquipped(sword), "item has Equipped tag")
local eq = engine.getEquipped(sword)
assert(eq.slot == "weapon", "equipped to weapon slot")

-- 查询装备
local equippedSword = engine.findEquipped(player, "weapon")
assert(equippedSword == sword, "getEquipped returns correct entity")

-- 卸下
ok = engine.unequipItem(player, "weapon")
assert(ok, "unequipItem succeeded")
assert(stats:get("test_str", 0) == 10, "str: 15 - 5 = 10 (restored)")
assert(not engine.hasEquipped(sword), "Equipped tag removed")

-- 测试槽满自动卸下
local sword2 = item.create("test_sword")
inv:add(sword2)
engine.equipItem(player, sword)
engine.equipItem(player, sword2)  -- 应自动卸下 sword, 换上 sword2
-- sword(+5) 被卸下(-5), sword2(+5) 装备, 净效果 +5
assert(stats:get("test_str", 0) == 15, "slot full: old unequipped, new equipped")
eq = engine.getEquipped(sword2)
assert(eq ~= nil and eq.slot == "weapon", "new sword is equipped")

-- 清理
engine.unequipItem(player, "weapon")
engine.unequipItem(player, "weapon") -- also unequip sword2 if present

-- =============================================================================
-- 测试 6: 金钱操作
-- =============================================================================
print("--- Test 6: money ---")
inv.money = 100
assert(engine.getMoney(player) == 100, "getMoney = 100")
assert(engine.addMoney(player, 50), "addMoney 50 → 150")
assert(engine.getMoney(player) == 150, "money = 150")
assert(engine.spendMoney(player, 30), "spendMoney 30 → 120")
assert(engine.getMoney(player) == 120, "money = 120")
assert(not engine.spendMoney(player, 999), "spendMoney fails when insufficient")
assert(engine.getMoney(player) == 120, "money unchanged after failed spend")
assert(not engine.addMoney(player, -10), "addMoney rejects negative amount")

-- =============================================================================
-- 测试 7: 派生属性
-- =============================================================================
print("--- Test 7: derived stat ---")
stat.define("test_atk", {
    displayName = "攻击力",
    derived     = true,
    dependsOn   = { "test_str" },
})
stat.setCompute("test_atk", function(s) return (s:get("test_str", 0) or 0) * 3 end)

stats:set("test_str", 10)
-- 派生属性需要等一帧由 StatSystem 重算
-- 这里直接验证 compute 函数本身
local registry = _DERIVED_COMPUTE
assert(registry ~= nil, "compute registry exists")
assert(registry["test_atk"] ~= nil, "compute function registered")
print("  [INFO] derived stat compute function registered, StatSystem will recompute on next frame")

-- =============================================================================
-- 测试 8: 存档/读档
-- =============================================================================
print("--- Test 8: save/load ---")
inv.money = 250
stats:setClamped("test_hp", 75)
local saved = narrative.save("assets/test_save.json")
assert(saved, "save succeeded")

-- 修改数据
inv.money = 999
stats:setClamped("test_hp", 10)

-- 读档
local loaded = narrative.load("assets/test_save.json")
assert(loaded, "load succeeded")
assert(engine.getMoney(player) == 250, "money restored to 250")
assert(stats:get("test_hp", 0) == 75, "HP restored to 75")

-- =============================================================================
-- 测试 9: inventory 配置
-- =============================================================================
print("--- Test 9: inventory config ---")
inventory.configure({ maxSlots = 2 })
local oldMax = inv.maxSlots
-- 清除背包
while #inv.items > 0 do
    local e = inv.items[1]
    inv:remove(e)
    engine.destroyEntity(e)
end

local p1 = item.create("test_potion")
local p2 = item.create("test_potion")
local p3 = item.create("test_potion")
assert(inv:add(p1), "add item 1/2")
assert(inv:add(p2), "add item 2/2")
assert(inv:isFull(), "inventory full at 2/2")
assert(not inv:add(p3), "add item 3/3 rejected (full)")
-- 清理
inv:remove(p1); inv:remove(p2)
engine.destroyEntity(p1); engine.destroyEntity(p2); engine.destroyEntity(p3)

inventory.configure({ maxSlots = 20 })  -- restore

-- =============================================================================
-- 结果
-- =============================================================================
print("========================================")
print(string.format("Results: %d passed, %d failed", passed, failed))
print("========================================")
