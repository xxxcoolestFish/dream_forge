/**
 * @file engine/script/lua_bindings.h
 * @brief C++ → Lua 绑定注册（Tier 1: ECS直操 + Tier 2: 叙事API）
 *
 * 本文件声明所有绑定注册函数，实现在 lua_bindings.cpp。
 *
 * 绑定层次：
 *   Tier 1 — engine 全局表：createEntity, getTransform, getStats, ...
 *           直接操作 ECS World，给系统脚本用
 *   Tier 2 — narrative 全局表：say, startQuest, setFlag, ...
 *           叙事专用 API，给内容作者用（Phase 5.3+ 逐步实现）
 */

#pragma once

#include <sol/sol.hpp>

namespace engine::ecs { class World; }
namespace engine::input { class InputSystem; }
namespace engine::narrative { class QuestManager; }
class EventBus;

namespace engine::script {

// 注册所有绑定到指定 sol::state
void registerAllBindings(sol::state& lua,
                         ecs::World* world,
                         EventBus* eventBus,
                         input::InputSystem* input,
                         narrative::QuestManager* questManager = nullptr);

} // namespace engine::script
