-- =============================================================================
-- assets/scripts/main.lua — 属性 + 物品 + 装备框架演示
-- =============================================================================

print("=== Game main.lua loaded! ===")

-- 1. Stats
stat.define("hp",      { displayName="生命值", default=100, max=100, min=0, regen=1.0, regenCooldown=5.0, onDepleted="onDeath", category="vital" })
stat.define("mp",      { displayName="法力值", default=50,  max=50,  min=0, regen=0.5, category="vital" })
stat.define("stamina", { displayName="体力",   default=100, max=100, min=0, regen=2.0, category="vital" })
stat.define("xp",      { displayName="经验值", default=0,   max=-1,  category="progression" })
stat.define("level",   { displayName="等级",   default=1,   max=999, min=1, category="progression" })
stat.define("strength",{ displayName="力量",   default=10,  max=99,  min=1, category="attribute" })
stat.define("attackPower", { displayName="攻击力", derived=true, dependsOn={"strength"}, category="combat" })
stat.setCompute("attackPower", function(s) return (s:get("strength", 0) or 10) * 2 end)
print("  Stats defined")

-- 2. Inventory
inventory.configure({ maxSlots = 20 })

-- 3. Items
item.define("health_potion", { name="生命药水", description="恢复30点生命值", consumable=true, useEffects={{stat="hp",modify=30,clampToMax=true}} })
item.define("mana_potion",   { name="法力药水", description="恢复20点法力值", consumable=true, useEffects={{stat="mp",modify=20,clampToMax=true}} })
item.define("iron_sword",    { name="铁剑",     description="普通的铁剑", equipSlot="weapon", statModifiers={attackPower=5,strength=2} })
print("  Items defined")

-- 4. Equipment
equipment.defineSlot("weapon",    { label="武器", maxCount=1 })
equipment.defineSlot("head",      { label="头部", maxCount=1 })
equipment.defineSlot("body",      { label="身体", maxCount=1 })
equipment.defineSlot("accessory", { label="饰品", maxCount=2 })
print("  Equipment defined")

-- 5. Player
local player = engine.getPlayer()
if player ~= kNullEntity then
    local stats = engine.getStats(player)
    if stats then
        print(string.format("Player: HP=%.0f/%.0f MP=%.0f Lv=%.0f Str=%.0f Att=%d",
            stats:get("hp", 0), stats.maxValues["hp"] or 0,
            stats:get("mp", 0), stats:get("level", 0),
            stats:get("strength", 0), stats:get("attackPower", 0)))
    end
    local inv = engine.getInventory(player)
    if inv then print(string.format("  Money=%d Items=%d", inv.money, #inv.items)) end
end

function onUpdate(dt) end
function onShutdown() print("Goodbye!") end
print("=== setup complete ===")
