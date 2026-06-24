/**
 * @file engine/script/lua_bindings.cpp
 * @brief C++ → Lua 绑定实现 — 两层 API 注册
 *
 * Tier 1 (engine 表): ECS 直操 — 给系统脚本
 * Tier 2 (narrative 表): 叙事 API — 给内容作者 (Phase 5.3+)
 */

#include "engine/script/lua_bindings.h"

#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"
#include "engine/core/event_bus.h"
#include "engine/input/input_system.h"
#include "engine/narrative/quest_manager.h"

#include <glm/glm.hpp>
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <sol/sol.hpp>

namespace engine::script {

// 引入所有 ECS 组件类型（它们在 engine::ecs 命名空间，本命名空间无法直接查找）
using engine::ecs::Transform;
using engine::ecs::Sprite;
using engine::ecs::Interactive;
using engine::ecs::Stats;
using engine::ecs::Inventory;
using engine::ecs::Item;
using engine::ecs::DialogueSpeaker;
using engine::ecs::DialogueState;
using engine::ecs::QuestProgress;
using engine::ecs::Relationship;
using engine::ecs::AICharacterContext;
using engine::ecs::Player;
using engine::ecs::Equipped;
using engine::ecs::Dead;
using engine::ecs::UseEffect;
using engine::ecs::kNullEntity;

// =========================================================================
// 辅助：运行时计算事件哈希（ENGINE_EVENT 的运行时版本）
// =========================================================================
static EventTypeId hashEventName(const std::string& name)
{
    return detail::fnv1a32(name.c_str());
}

// =========================================================================
// Tier 1: 每个 Component 的 C++ 访问器
// =========================================================================
// sol2 无法直接绑定模板函数 World::getComponent<T>，因此为每个组件提供
// 一个具名自由函数，在 Lua 中通过 engine.getXxx(entity) 调用。
// 参数使用 uint32_t 避免 entt::entity 枚举类型转换问题。
// =========================================================================

#define DEFINE_COMPONENT_ACCESSOR(FuncName, CompType)                   \
    static CompType* FuncName(ecs::World& w, uint32_t rawEnt) {        \
        auto e = static_cast<ecs::Entity>(rawEnt);                     \
        if (!w.isValid(e) || !w.hasComponent<CompType>(e))             \
            return nullptr;                                            \
        return &w.getComponent<CompType>(e);                           \
    }

DEFINE_COMPONENT_ACCESSOR(getTransform,        Transform)
DEFINE_COMPONENT_ACCESSOR(getSprite,           Sprite)
DEFINE_COMPONENT_ACCESSOR(getInteractive,      Interactive)
DEFINE_COMPONENT_ACCESSOR(getStats,            Stats)
DEFINE_COMPONENT_ACCESSOR(getInventory,        Inventory)
DEFINE_COMPONENT_ACCESSOR(getItem,             Item)
DEFINE_COMPONENT_ACCESSOR(getDialogueSpeaker,  DialogueSpeaker)
DEFINE_COMPONENT_ACCESSOR(getDialogueState,    DialogueState)
DEFINE_COMPONENT_ACCESSOR(getQuestProgress,    QuestProgress)
DEFINE_COMPONENT_ACCESSOR(getRelationship,     Relationship)
DEFINE_COMPONENT_ACCESSOR(getAICharacterContext, AICharacterContext)
DEFINE_COMPONENT_ACCESSOR(getEquipped, Equipped)
DEFINE_COMPONENT_ACCESSOR(getDead, Dead)

#undef DEFINE_COMPONENT_ACCESSOR

// Player 是标记组件，无数据可取，仅用于查询
static bool isPlayer(ecs::World& w, uint32_t rawEnt)
{
    auto e = static_cast<ecs::Entity>(rawEnt);
    return w.isValid(e) && w.hasComponent<Player>(e);
}

// =========================================================================
// Tier 1: World 操作辅助函数
// =========================================================================
#pragma warning(push)
#pragma warning(disable: 4702) // MSVC 误报：view 可能为空，代码可达
static uint32_t getPlayerEntity(ecs::World& w)
{
    auto view = w.view<Transform, Player>();
    for (auto e : view)
        return static_cast<uint32_t>(e);
    return static_cast<uint32_t>(kNullEntity);
}
#pragma warning(pop)

// =========================================================================
// 绑定注册主入口
// =========================================================================
void registerAllBindings(sol::state& lua,
                         ecs::World* world,
                         EventBus* eventBus,
                         input::InputSystem* input,
                         narrative::QuestManager* questManager)
{
    // --- 打开基础库（print, type, error 等） ---
    lua.open_libraries(sol::lib::base, sol::lib::string, sol::lib::math,
                       sol::lib::table, sol::lib::io, sol::lib::package);

    // =====================================================================
    // 1. glm 类型绑定（使用工厂函数 + property，避免 GLM 匿名 union 兼容问题）
    // =====================================================================

    // vec2 工厂
    lua.set_function("vec2", sol::overload(
        []() { return glm::vec2(0.f, 0.f); },
        [](float x, float y) { return glm::vec2(x, y); }
    ));
    lua.new_usertype<glm::vec2>("vec2_t",
        "x", sol::property([](const glm::vec2& v) { return v.x; },
                           [](glm::vec2& v, float val) { v.x = val; }),
        "y", sol::property([](const glm::vec2& v) { return v.y; },
                           [](glm::vec2& v, float val) { v.y = val; })
    );

    // vec3 工厂
    lua.set_function("vec3", sol::overload(
        []() { return glm::vec3(0.f, 0.f, 0.f); },
        [](float x, float y, float z) { return glm::vec3(x, y, z); }
    ));
    lua.new_usertype<glm::vec3>("vec3_t",
        "x", sol::property([](const glm::vec3& v) { return v.x; },
                           [](glm::vec3& v, float val) { v.x = val; }),
        "y", sol::property([](const glm::vec3& v) { return v.y; },
                           [](glm::vec3& v, float val) { v.y = val; }),
        "z", sol::property([](const glm::vec3& v) { return v.z; },
                           [](glm::vec3& v, float val) { v.z = val; })
    );

    // =====================================================================
    // 2. Component 类型绑定（Lua 可直接读写字段）
    // =====================================================================

    // --- Transform ---
    lua.new_usertype<Transform>("Transform",
        "position",    &Transform::position,
        "rotation",    &Transform::rotation,
        "scale",       &Transform::scale,
        "depthLayer",  &Transform::depthLayer
    );

    // --- Sprite ---
    lua.new_usertype<Sprite>("Sprite",
        "textureId",  &Sprite::textureId,
        "tint",       &Sprite::tint,
        "anchor",     &Sprite::anchor,
        "layerGroup", &Sprite::layerGroup,
        "visible",    &Sprite::visible
    );

    // --- Interactive ---
    lua.new_usertype<Interactive>("Interactive",
        "interactionType", &Interactive::interactionType,
        "cursorStyle",     &Interactive::cursorStyle,
        "enabled",         &Interactive::enabled
    );

    // --- Stats ---
    lua.new_usertype<Stats>("Stats",
        "values",       &Stats::values,
        "maxValues",    &Stats::maxValues,
        "minValues",    &Stats::minValues,
        "get",          &Stats::get,
        "set",          &Stats::set,
        "modify",       &Stats::modify,
        "setClamped",   &Stats::setClamped,
        "modifyClamped",&Stats::modifyClamped,
        "isAtMax",      &Stats::isAtMax,
        "isDepleted",   &Stats::isDepleted
    );

    // --- Inventory ---
    lua.new_usertype<Inventory>("Inventory",
        "items",     &Inventory::items,
        "maxSlots",  &Inventory::maxSlots,
        "money",     &Inventory::money,
        "add",       &Inventory::add,
        "remove",    &Inventory::remove,
        "isFull",    &Inventory::isFull,
        "addMoney",  &Inventory::addMoney,
        "spendMoney",&Inventory::spendMoney
    );

    // --- UseEffect ---
    lua.new_usertype<UseEffect>("UseEffect",
        "stat",       &UseEffect::stat,
        "modify",     &UseEffect::modify,
        "clampToMax", &UseEffect::clampToMax
    );

    // --- Item ---
    lua.new_usertype<Item>("Item",
        "itemId",       &Item::itemId,
        "name",         &Item::name,
        "description",  &Item::description,
        "equipSlot",    &Item::equipSlot,
        "consumable",   &Item::consumable,
        "useEffects",   &Item::useEffects,
        "statModifiers",&Item::statModifiers
    );

    // --- DialogueSpeaker ---
    lua.new_usertype<DialogueSpeaker>("DialogueSpeaker",
        "characterId",      &DialogueSpeaker::characterId,
        "personalityPrompt",&DialogueSpeaker::personalityPrompt,
        "friendliness",     &DialogueSpeaker::friendliness
    );

    // --- DialogueState ---
    lua.new_usertype<DialogueState>("DialogueState",
        "dialogueTreeId", &DialogueState::dialogueTreeId,
        "currentNodeId",  &DialogueState::currentNodeId,
        "visitedNodes",   &DialogueState::visitedNodes,
        "flags",          &DialogueState::flags
    );

    // --- QuestProgress ---
    lua.new_usertype<QuestProgress>("QuestProgress",
        "questId",    &QuestProgress::questId,
        "status",     &QuestProgress::status,
        "objectives", &QuestProgress::objectives
    );

    // --- Relationship ---
    lua.new_usertype<Relationship>("Relationship",
        "affinity", &Relationship::affinity
    );

    // --- AICharacterContext ---
    lua.new_usertype<AICharacterContext>("AICharacterContext",
        "backgroundStory",  &AICharacterContext::backgroundStory,
        "currentGoal",      &AICharacterContext::currentGoal,
        "emotionalState",   &AICharacterContext::emotionalState,
        "knowledgeDomains", &AICharacterContext::knowledgeDomains
    );

    lua.new_usertype<Equipped>("Equipped",
        "equippedBy", &Equipped::equippedBy,
        "slot",       &Equipped::slot
    );

    lua.new_usertype<Dead>("Dead",
        "deathTime", &Dead::deathTime
    );

    // =====================================================================
    // 3. engine 全局表 — Tier 1: ECS 直操
    // =====================================================================
    sol::table eng = lua["engine"].get_or_create<sol::table>();

    // --- Entity 操作 ---
    eng.set_function("createEntity",  [world]() -> uint32_t {
        return static_cast<uint32_t>(world->createEntity());
    });
    eng.set_function("destroyEntity", [world](uint32_t rawEnt) {
        world->destroyEntity(static_cast<ecs::Entity>(rawEnt));
    });
    eng.set_function("isValid",       [world](uint32_t rawEnt) {
        return world->isValid(static_cast<ecs::Entity>(rawEnt));
    });
    eng.set_function("getPlayer",     [world]() { return getPlayerEntity(*world); });

    // --- Component 判断 ---
    // 宏：为每种组件生成 hasXxx(entity) 函数
    #define DEF_HAS_COMP(FuncName, CompType) \
        eng.set_function(#FuncName, [world](uint32_t rawEnt) { \
            auto e = static_cast<ecs::Entity>(rawEnt); \
            return world->isValid(e) && world->hasComponent<CompType>(e); \
        })

    DEF_HAS_COMP(hasTransform,        Transform);
    DEF_HAS_COMP(hasSprite,           Sprite);
    DEF_HAS_COMP(hasInteractive,      Interactive);
    DEF_HAS_COMP(hasStats,            Stats);
    DEF_HAS_COMP(hasInventory,        Inventory);
    DEF_HAS_COMP(hasItem,             Item);
    DEF_HAS_COMP(hasDialogueSpeaker,  DialogueSpeaker);
    DEF_HAS_COMP(hasDialogueState,    DialogueState);
    DEF_HAS_COMP(hasQuestProgress,    QuestProgress);
    DEF_HAS_COMP(hasRelationship,     Relationship);
    DEF_HAS_COMP(hasAICharacterContext, AICharacterContext);
    DEF_HAS_COMP(hasEquipped,           Equipped);
    DEF_HAS_COMP(hasDead,               Dead);
    #undef DEF_HAS_COMP

    eng.set_function("isPlayer", [world](uint32_t rawEnt) {
        return isPlayer(*world, rawEnt);
    });

    // --- Component 获取（捕获 world 注入为第一个参数） ---
    eng.set_function("getTransform",   [world](uint32_t e) { return getTransform(*world, e); });
    eng.set_function("getSprite",      [world](uint32_t e) { return getSprite(*world, e); });
    eng.set_function("getInteractive", [world](uint32_t e) { return getInteractive(*world, e); });
    eng.set_function("getStats",       [world](uint32_t e) { return getStats(*world, e); });
    eng.set_function("getInventory",   [world](uint32_t e) { return getInventory(*world, e); });
    eng.set_function("getItem",        [world](uint32_t e) { return getItem(*world, e); });
    eng.set_function("getDialogueSpeaker",  [world](uint32_t e) { return getDialogueSpeaker(*world, e); });
    eng.set_function("getDialogueState",    [world](uint32_t e) { return getDialogueState(*world, e); });
    eng.set_function("getQuestProgress",    [world](uint32_t e) { return getQuestProgress(*world, e); });
    eng.set_function("getRelationship",     [world](uint32_t e) { return getRelationship(*world, e); });
    eng.set_function("getAICharacterContext", [world](uint32_t e) { return getAICharacterContext(*world, e); });
    eng.set_function("getEquipped",  [world](uint32_t e) { return getEquipped(*world, e); });
    eng.set_function("getDead",      [world](uint32_t e) { return getDead(*world, e); });

    // --- Component 添加 ---
    #define DEF_ADD_COMP(FuncName, CompType) \
        eng.set_function(#FuncName, [world](uint32_t rawEnt) -> CompType& { \
            return world->addComponent<CompType>(static_cast<ecs::Entity>(rawEnt)); \
        })

    DEF_ADD_COMP(addTransform,        Transform);
    DEF_ADD_COMP(addSprite,           Sprite);
    DEF_ADD_COMP(addInteractive,      Interactive);
    DEF_ADD_COMP(addStats,            Stats);
    DEF_ADD_COMP(addInventory,        Inventory);
    DEF_ADD_COMP(addItem,             Item);
    DEF_ADD_COMP(addDialogueSpeaker,  DialogueSpeaker);
    DEF_ADD_COMP(addDialogueState,    DialogueState);
    DEF_ADD_COMP(addQuestProgress,    QuestProgress);
    DEF_ADD_COMP(addRelationship,     Relationship);
    DEF_ADD_COMP(addAICharacterContext, AICharacterContext);
    DEF_ADD_COMP(addPlayer,           Player);
    DEF_ADD_COMP(addEquipped,         Equipped);
    DEF_ADD_COMP(addDead,             Dead);
    #undef DEF_ADD_COMP

    // --- Component 移除 ---
    #define DEF_REMOVE_COMP(FuncName, CompType) \
        eng.set_function(#FuncName, [world](uint32_t rawEnt) { \
            world->removeComponent<CompType>(static_cast<ecs::Entity>(rawEnt)); \
        })

    DEF_REMOVE_COMP(removeTransform,        Transform);
    DEF_REMOVE_COMP(removeSprite,           Sprite);
    DEF_REMOVE_COMP(removeInteractive,      Interactive);
    DEF_REMOVE_COMP(removeStats,            Stats);
    DEF_REMOVE_COMP(removeInventory,        Inventory);
    DEF_REMOVE_COMP(removeItem,             Item);
    DEF_REMOVE_COMP(removeDialogueSpeaker,  DialogueSpeaker);
    DEF_REMOVE_COMP(removeDialogueState,    DialogueState);
    DEF_REMOVE_COMP(removeQuestProgress,    QuestProgress);
    DEF_REMOVE_COMP(removeRelationship,     Relationship);
    DEF_REMOVE_COMP(removeAICharacterContext, AICharacterContext);
    DEF_REMOVE_COMP(removeEquipped,           Equipped);
    DEF_REMOVE_COMP(removeDead,               Dead);
    #undef DEF_REMOVE_COMP

    eng.set_function("removePlayer", [world](ecs::Entity e) {
        world->removeComponent<Player>(e);
    });

    // =====================================================================
    // 4. engine 全局表 — 输入系统
    // =====================================================================
    eng.set_function("isKeyPressed", [input](int keyCode) {
        return input->isKeyPressed(keyCode);
    });
    eng.set_function("isKeyHeld", [input](int keyCode) {
        return input->isKeyHeld(keyCode);
    });
    eng.set_function("isKeyReleased", [input](int keyCode) {
        return input->isKeyReleased(keyCode);
    });
    eng.set_function("mouseX", [input]() { return input->mouseX(); });
    eng.set_function("mouseY", [input]() { return input->mouseY(); });

    // =====================================================================
    // 5. engine 全局表 — 事件总线
    // =====================================================================
    eng.set_function("publish", [eventBus](const std::string& eventName) {
        auto typeId = hashEventName(eventName);
        eventBus->publishImmediate(typeId, {});
    });

    // 从 Lua 订阅事件 — 回调接收事件名（5.3 完善数据传递）
    eng.set_function("subscribe", [eventBus, &lua](const std::string& eventName,
                                                    sol::protected_function callback) {
        auto typeId = hashEventName(eventName);
        eventBus->subscribe(typeId, [cb = std::move(callback), eventName](const Event& /*event*/) {
            // 在游戏循环中同步调用 Lua 回调
            auto result = cb(eventName);
            if (!result.valid())
            {
                sol::error err = result;
                spdlog::warn("[Lua] Event callback error: {}", err.what());
            }
        });
    });

    // =====================================================================
    // 6. 常用键码常量
    // =====================================================================
    sol::table keys = lua["Keys"].get_or_create<sol::table>();
    keys["W"]     = GLFW_KEY_W;
    keys["A"]     = GLFW_KEY_A;
    keys["S"]     = GLFW_KEY_S;
    keys["D"]     = GLFW_KEY_D;
    keys["E"]     = GLFW_KEY_E;
    keys["ESC"]   = GLFW_KEY_ESCAPE;
    keys["SPACE"] = GLFW_KEY_SPACE;
    keys["LEFT"]  = GLFW_KEY_LEFT;
    keys["RIGHT"] = GLFW_KEY_RIGHT;
    keys["UP"]    = GLFW_KEY_UP;
    keys["DOWN"]  = GLFW_KEY_DOWN;
    keys["1"]     = GLFW_KEY_1;
    keys["2"]     = GLFW_KEY_2;
    keys["3"]     = GLFW_KEY_3;
    keys["4"]     = GLFW_KEY_4;
    keys["5"]     = GLFW_KEY_5;

    // =====================================================================
    // 7. narrative 全局表 — Tier 2: 叙事 API
    // =====================================================================
    sol::table narr = lua["narrative"].get_or_create<sol::table>();

    narr.set_function("_version", []() -> std::string { return "0.2.0"; });

    // 任务系统（需要 QuestManager）
    if (questManager)
    {
        narr.set_function("startQuest", [questManager, world](const std::string& questId) {
            // 查找玩家实体
            auto view = world->view<Transform, Player>();
            for (auto e : view)
            {
                questManager->startQuest(e, questId);
                return;
            }
        });

        narr.set_function("completeQuest", [questManager, world](const std::string& questId) {
            auto view = world->view<Transform, Player>();
            for (auto e : view)
                questManager->completeQuest(e, questId);
        });

        narr.set_function("failQuest", [questManager, world](const std::string& questId) {
            auto view = world->view<Transform, Player>();
            for (auto e : view)
                questManager->failQuest(e, questId);
        });

        narr.set_function("updateObjective",
            [questManager, world](const std::string& questId,
                                   const std::string& objectiveId, int delta) {
                auto view = world->view<Transform, Player>();
                for (auto e : view)
                    questManager->updateObjective(e, questId, objectiveId, delta);
            });

        narr.set_function("isQuestActive", [questManager, world](const std::string& questId) -> bool {
            auto view = world->view<Transform, Player>();
            for (auto e : view)
                return questManager->isQuestActive(e, questId);
            return false;
        });
    }
    else
    {
        spdlog::warn("ScriptEngine: QuestManager not available, quest API disabled");
    }

    // =====================================================================
    // 8. 常量
    // =====================================================================
    lua["kNullEntity"] = kNullEntity;

    spdlog::info("ScriptEngine: all C++ bindings registered ({} component types).", 14);
}

} // namespace engine::script
