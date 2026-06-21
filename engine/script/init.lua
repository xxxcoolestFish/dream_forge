-- =============================================================================
-- engine/script/init.lua — 引擎内置 Lua 初始化脚本
-- =============================================================================
-- 提供 Lua 侧的辅助函数、默认配置和全局变量。
-- 在 ScriptEngine::init() 中自动加载。
-- =============================================================================

print("[ScriptEngine] init.lua loaded — engine Lua runtime ready")

-- =========================================================================
-- 全局常量
-- =========================================================================
VERSION = "0.1.0"

-- =========================================================================
-- 辅助函数
-- =========================================================================

-- 安全地打印调试信息
function debugPrint(...)
    print("[Lua]", ...)
end

-- 检查实体是否有效且拥有某组件
function entityHas(entity, componentName)
    local checkFn = engine["has" .. componentName]
    if checkFn == nil then
        debugPrint("Warning: unknown component type '" .. componentName .. "'")
        return false
    end
    return checkFn(entity)
end

-- 获取实体上某组件（如果存在）
function entityGet(entity, componentName)
    local getFn = engine["get" .. componentName]
    if getFn == nil then
        debugPrint("Warning: unknown component type '" .. componentName .. "'")
        return nil
    end
    return getFn(entity)
end

-- =========================================================================
-- 游戏循环钩子（默认空实现，由 assets/scripts/main.lua 覆盖）
-- =========================================================================

function onUpdate(dt)
    -- 由游戏脚本覆盖
end

function onShutdown()
    -- 由游戏脚本覆盖
end

debugPrint("init.lua setup complete")

-- =========================================================================
-- 尝试加载用户主脚本
-- =========================================================================
local mainOk, mainErr = pcall(function()
    -- 用户脚本不一定存在，不存在不报错
    local f = io.open("assets/scripts/main.lua", "r")
    if f then
        f:close()
        dofile("assets/scripts/main.lua")
        debugPrint("main.lua loaded successfully")
    end
end)

if not mainOk then
    debugPrint("main.lua load error: " .. tostring(mainErr))
end
