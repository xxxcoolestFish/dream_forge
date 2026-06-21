/**
 * @file engine/script/script_engine.h
 * @brief Lua 脚本引擎 — 封装 sol::state，提供脚本加载与执行
 *
 * 负责：
 *   - Lua VM 生命周期管理
 *   - C++ → Lua 绑定注册（委托给 lua_bindings.cpp）
 *   - 脚本文件加载与执行
 *   - 游戏循环钩子（onUpdate）
 *
 * 设计：
 *   - 单实例，由 Engine::Impl 持有
 *   - 绑定分为两层：Tier 1（ECS直操）+ Tier 2（叙事API）
 *   - 所有 C++ 对象以指针/引用暴露给 Lua，生命周期由 C++ 管理
 *
 * 注意事项：
 *   - Lua 脚本在游戏循环中同步执行，不要在脚本中做耗时操作
 *   - Entity 在 Lua 中表现为整数（uint32_t）
 *   - 不要将 C++ 临时对象的引用传递给 Lua
 */

#pragma once

#include <sol/sol.hpp>
#include <string>

namespace engine::ecs { class World; }
namespace engine::input { class InputSystem; }
namespace engine::narrative { class QuestManager; }
namespace engine { class EventBus; }

namespace engine::script {

class ScriptEngine
{
public:
    ScriptEngine();
    ~ScriptEngine();

    // 禁止拷贝
    ScriptEngine(const ScriptEngine&) = delete;
    ScriptEngine& operator=(const ScriptEngine&) = delete;

    // --- 初始化 ---
    // 必须在所有其他方法之前调用，注册所有 C++ 绑定
    void init(ecs::World* world, EventBus* eventBus, input::InputSystem* input,
              narrative::QuestManager* questManager = nullptr);

    // 反初始化，清理 Lua 状态
    void shutdown();

    bool isInitialized() const { return m_initialized; }

    // --- 脚本加载 ---
    // 从文件加载并执行 Lua 脚本，返回是否成功
    bool loadFile(const std::string& path);

    // 直接执行一段 Lua 代码，用于内联脚本和测试
    bool executeString(const std::string& code);

    // --- 游戏循环钩子 ---
    // 每帧调用，执行 Lua 全局函数 onUpdate(dt)
    void onUpdate(double dt);

    // --- 访问底层 sol::state ---
    sol::state& luaState() { return m_lua; }
    const sol::state& luaState() const { return m_lua; }

private:
    sol::state     m_lua;
    ecs::World*    m_world    = nullptr;
    EventBus*      m_eventBus = nullptr;
    input::InputSystem* m_input = nullptr;
    bool           m_initialized = false;

    // 注册所有 C++ → Lua 绑定
    void registerAllBindings();
};

} // namespace engine::script
