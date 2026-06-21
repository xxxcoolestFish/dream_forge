/**
 * @file engine/script/script_engine.cpp
 * @brief ScriptEngine 实现
 */

#include "engine/script/script_engine.h"
#include "engine/script/lua_bindings.h"
#include "engine/ecs/world.h"
#include "engine/core/event_bus.h"
#include "engine/input/input_system.h"

#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>

namespace engine::script {

ScriptEngine::ScriptEngine()
{
    spdlog::debug("ScriptEngine: instance created");
}

ScriptEngine::~ScriptEngine()
{
    if (m_initialized)
        shutdown();
}

void ScriptEngine::init(ecs::World* world, EventBus* eventBus, input::InputSystem* input)
{
    m_world    = world;
    m_eventBus = eventBus;
    m_input    = input;

    spdlog::info("ScriptEngine: initializing Lua VM...");

    // 注册所有 C++ → Lua 绑定
    registerAllBindings();

    // 标记已初始化，允许后续脚本加载
    m_initialized = true;

    // 加载引擎内置的 init.lua
    // 该脚本提供 Lua 侧的辅助函数和默认配置
    // 尝试从多个可能的位置加载
    static const char* kInitPaths[] = {
        "assets/scripts/init.lua",
        "engine/script/init.lua",
    };

    bool initLoaded = false;
    for (auto* path : kInitPaths)
    {
        if (loadFile(path))
        {
            spdlog::info("ScriptEngine: loaded engine init script '{}'", path);
            initLoaded = true;
            break;
        }
    }
    if (!initLoaded)
    {
        spdlog::warn("ScriptEngine: engine init script not found (expected at assets/scripts/init.lua). "
                     "Lua VM is functional but without defaults.");
    }

    spdlog::info("ScriptEngine: initialization complete");
}

void ScriptEngine::shutdown()
{
    if (!m_initialized) return;

    spdlog::debug("ScriptEngine: shutting down...");

    // 调用 Lua 的 onShutdown 钩子（如果存在）
    sol::protected_function onShutdown = m_lua["onShutdown"];
    if (onShutdown.valid())
    {
        auto result = onShutdown();
        if (!result.valid())
        {
            sol::error err = result;
            spdlog::warn("ScriptEngine: onShutdown error: {}", err.what());
        }
    }

    // 清理 Lua 状态（析构 sol::state 会自动回收所有 Lua 对象）
    m_lua = sol::state(); // 重建一个新的空状态
    m_initialized = false;
    spdlog::debug("ScriptEngine: shutdown complete");
}

bool ScriptEngine::loadFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        spdlog::debug("ScriptEngine: cannot open '{}'", path);
        return false;
    }

    std::stringstream buf;
    buf << file.rdbuf();
    std::string code = buf.str();

    return executeString(code);
}

bool ScriptEngine::executeString(const std::string& code)
{
    if (!m_initialized)
    {
        spdlog::error("ScriptEngine: not initialized, cannot execute script");
        return false;
    }

    try
    {
        auto result = m_lua.script(code, sol::script_pass_on_error);
        if (!result.valid())
        {
            sol::error err = result;
            spdlog::error("ScriptEngine: Lua execution error: {}", err.what());
            return false;
        }
        return true;
    }
    catch (const std::exception& e)
    {
        spdlog::error("ScriptEngine: C++ exception during Lua execution: {}", e.what());
        return false;
    }
    catch (...)
    {
        spdlog::error("ScriptEngine: unknown exception during Lua execution");
        return false;
    }
}

void ScriptEngine::onUpdate(double dt)
{
    if (!m_initialized) return;

    // 调用 Lua 全局函数 onUpdate(dt)
    sol::protected_function onUpdate = m_lua["onUpdate"];
    if (onUpdate.valid())
    {
        auto result = onUpdate(dt);
        if (!result.valid())
        {
            sol::error err = result;
            spdlog::warn("ScriptEngine: onUpdate error: {}", err.what());
        }
    }
}

void ScriptEngine::registerAllBindings()
{
    engine::script::registerAllBindings(m_lua, m_world, m_eventBus, m_input);
}

} // namespace engine::script
