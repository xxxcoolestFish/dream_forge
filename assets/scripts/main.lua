-- =============================================================================
-- assets/scripts/main.lua — 游戏主脚本（示例）
-- =============================================================================

print("=== Game main.lua loaded! ===")

-- =============================================================================
-- 示例：修改玩家初始属性（通过 ECS 绑定）
-- =============================================================================
local player = engine.getPlayer()
if player ~= kNullEntity then
    local stats = engine.getStats(player)
    if stats then
        stats:set("hp", 120)
        stats:set("xp", 0)
        stats:set("level", 1)
        print(string.format("Player stats set: HP=%.0f, XP=%.0f, Level=%.0f",
            stats:get("hp"), stats:get("xp"), stats:get("level")))
    else
        print("Player has no Stats component")
    end
else
    print("No player entity found yet (created in C++)")
end

-- =============================================================================
-- 示例：创建并操作实体
-- =============================================================================
local pos = vec3(300, 300, 0)
print(string.format("vec3 test: (%f, %f, %f)", pos.x, pos.y, pos.z))

local testEnt = engine.createEntity()
print("Created test entity: " .. testEnt)

engine.addTransform(testEnt)
local t = engine.getTransform(testEnt)
if t then
    t.position = vec3(400, 250, 0.5)
    t.depthLayer = 0.8
    print(string.format("Test entity depth: %.1f", t.depthLayer))
end
engine.destroyEntity(testEnt)

-- =============================================================================
-- 每帧调用
-- =============================================================================
function onUpdate(dt)
    -- 后续 Phase 在此添加逻辑
end

-- =============================================================================
-- 引擎关闭
-- =============================================================================
function onShutdown()
    print("Game script shutting down. Goodbye!")
end

print("=== main.lua setup complete ===")
