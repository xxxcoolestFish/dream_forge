-- =============================================================================
-- test_binding.lua — 测试新增的 UI 配合接口
-- =============================================================================

local passed, failed = 0, 0
local function assert(cond, msg)
    if cond then passed = passed + 1; print("  [PASS] " .. msg)
    else        failed = failed + 1; print("  [FAIL] " .. msg) end
end

print("=== Binding API Tests ===")

-- =============================================================================
-- Test 1: stat.getAllDefinitions
-- =============================================================================
print("--- Test 1: getAllDefinitions ---")
print("  defining bind_hp...")
stat.define("bind_hp",  { displayName="HP",  default=100, max=100, category="vital" })
print("  defining bind_mp...")
stat.define("bind_mp",  { displayName="MP",  default=50,  max=50,  category="vital" })
print("  defining bind_xp...")
stat.define("bind_xp",  { displayName="EXP", default=0,   max=-1,  category="progression" })
print("  calling getAllDefinitions...")

local all = stat.getAllDefinitions()
assert(all["bind_hp"] ~= nil, "bind_hp in allDefinitions")
assert(all["bind_mp"] ~= nil, "bind_mp in allDefinitions")
assert(all["bind_xp"] ~= nil, "bind_xp in allDefinitions")
assert(all["bind_hp"].displayName == "HP", "displayName = HP")
assert(all["bind_hp"].max == 100, "max = 100")

-- =============================================================================
-- Test 2: stat.getByCategory
-- =============================================================================
print("--- Test 2: getByCategory ---")
local vital = stat.getByCategory("vital")
local prog  = stat.getByCategory("progression")
assert(vital["bind_hp"] ~= nil, "vital contains bind_hp")
assert(vital["bind_mp"] ~= nil, "vital contains bind_mp")
assert(vital["bind_xp"] == nil, "vital does NOT contain bind_xp")
assert(prog["bind_xp"] ~= nil, "progression contains bind_xp")

-- =============================================================================
-- Test 3: engine.resolveBinding (stat)
-- =============================================================================
print("--- Test 3: resolveBinding ---")
local player = engine.getPlayer()
local stats  = engine.getStats(player)
stats:set("bind_hp", 75)
stats:set("bind_mp", 30)

assert(engine.resolveBinding(player, "stat.bind_hp") == "75", "resolveBinding stat.bind_hp = 75")
assert(engine.resolveBinding(player, "stat.bind_mp") == "30", "resolveBinding stat.bind_mp = 30")
assert(engine.resolveBinding(player, "stat.bind_xp") == "0",  "resolveBinding stat.bind_xp = 0")

-- =============================================================================
-- Test 4: engine.resolveBinding (money / inventory)
-- =============================================================================
print("--- Test 4: resolveBinding money/inventory ---")
local inv = engine.getInventory(player)
inv.money = 150
assert(engine.resolveBinding(player, "money") == "150", "resolveBinding money = 150")
assert(engine.resolveBinding(player, "inventory_count") ~= "0", "resolveBinding inventory_count > 0")

-- =============================================================================
-- Test 5: 模拟 UI 场景 — 遍历 vital 类属性自动生成显示
-- =============================================================================
print("--- Test 5: UI auto-generation simulation ---")
local vitals = stat.getByCategory("vital")
for key, def in pairs(vitals) do
    local val = engine.resolveBinding(player, "stat." .. key)
    local maxVal = def.max
    if maxVal == -1 then maxVal = "∞" end
    print(string.format("  %s: %s/%s", def.displayName, val, tostring(maxVal)))
end
assert(true, "UI auto-gen completed without error")

-- =============================================================================
-- Test 6: engine.resolveBinding (equipped slot)
-- =============================================================================
print("--- Test 6: resolveBinding equipped ---")
equipment.defineSlot("weapon", { label="武器" })
item.define("bind_sword", { name="测试剑", equipSlot="weapon" })
local sword = item.create("bind_sword")
inv:add(sword)
engine.equipItem(player, sword)

local eqName = engine.resolveBinding(player, "equipped.weapon")
assert(eqName == "测试剑", "resolveBinding equipped.weapon = '测试剑'")

engine.unequipItem(player, "weapon")
assert(engine.resolveBinding(player, "equipped.weapon") == "-", "empty slot returns '-'")

-- =============================================================================
-- Result
-- =============================================================================
print("========================================")
print(string.format("Results: %d passed, %d failed", passed, failed))
print("========================================")
