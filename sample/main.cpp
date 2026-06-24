/**
 * @file sample/main.cpp
 * @brief Phase 6: "铁匠的委托" — 完整功能演示
 *
 * 展示全部 Phase 1-6 功能：
 *   2.5D 视差场景 + AI对话 + 对话树 + 任务系统
 *   纹理采样 + 音频引擎 + 后处理特效 + 粒子系统
 *   屏幕转场 + ImGui 编辑器
 *
 * 操作：
 *   WASD     = 移动玩家（蓝色方块）
 *   方向键   = 旋转相机 → 视差效果
 *   E        = 与 NPC 对话
 *   F1       = 切换 ImGui 编辑器
 *   ESC      = 退出
 */

#include "engine/core/engine.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"
#include "engine/audio/audio_engine.h"
#include "engine/render/particles/particle_system.h"
#include "engine/render/transitions/transition_manager.h"

#include <spdlog/spdlog.h>
#include <glm/glm.hpp>

#ifdef _WIN32
#include <windows.h>
#endif

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;

#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    spdlog::set_level(spdlog::level::debug);
    spdlog::info("=== AI Game Frame — Phase 6: Full Demo ===");

    engine::EngineConfig config;
    config.windowTitle  = "AI Game Frame — Phase 6";
    config.windowWidth  = 1280;
    config.windowHeight = 720;

    engine::Engine engine;
    if (!engine.init(config))
    {
        spdlog::critical("Engine initialization failed.");
        return 1;
    }

    // --- ECS 实体 ---
    auto* world = engine.ecsWorld();

    // 玩家
    auto player = world->createEntity();
    world->addComponent<engine::ecs::Transform>(player);
    world->addComponent<engine::ecs::Sprite>(player);
    world->addComponent<engine::ecs::Stats>(player);
    world->addComponent<engine::ecs::Player>(player);
    auto& pt = world->getComponent<engine::ecs::Transform>(player);
    pt.position = glm::vec3(640.0f, 360.0f, -100.0f);  // 3D: 场景中心前方
    world->getComponent<engine::ecs::Sprite>(player).tint = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f);
    auto& pStats = world->getComponent<engine::ecs::Stats>(player);
    pStats.values["hp"] = 100;
    pStats.values["xp"] = 0;
    pStats.values["level"] = 1;

    // 玩家背包 + 金币
    auto& inventory = world->addComponent<engine::ecs::Inventory>(player);
    inventory.money = 50;
    inventory.maxSlots = 20;

    // 示例物品：生命药水
    auto healthPotion = world->createEntity();
    {
        auto& item = world->addComponent<engine::ecs::Item>(healthPotion);
        item.itemId    = "health_potion";
        item.name      = "生命药水";
        item.description = "恢复30点生命值";
        item.consumable = true;
        item.useEffects.push_back({"hp", 30.0f, true});
    }
    inventory.add(healthPotion);

    // 示例物品：法力药水
    auto manaPotion = world->createEntity();
    {
        auto& item = world->addComponent<engine::ecs::Item>(manaPotion);
        item.itemId    = "mana_potion";
        item.name      = "法力药水";
        item.description = "恢复20点法力值";
        item.consumable = true;
        item.useEffects.push_back({"mp", 20.0f, true});
    }
    inventory.add(manaPotion);

    spdlog::info("Player inventory: {} gold, {} items", inventory.money, inventory.items.size());

    // NPC：铁匠老陈
    auto npc = world->createEntity();
    world->addComponent<engine::ecs::Transform>(npc);
    world->addComponent<engine::ecs::Sprite>(npc);
    world->addComponent<engine::ecs::DialogueSpeaker>(npc);
    world->addComponent<engine::ecs::Interactive>(npc);
    auto& nt = world->getComponent<engine::ecs::Transform>(npc);
    nt.position = glm::vec3(900.0f, 380.0f, -150.0f);  // 3D: 在场景深处
    world->getComponent<engine::ecs::Sprite>(npc).tint = glm::vec4(1.0f, 0.4f, 0.3f, 1.0f);
    auto& ns = world->getComponent<engine::ecs::DialogueSpeaker>(npc);
    ns.characterId = "老陈";
    ns.personalityPrompt = "暴躁但善良的老铁匠";
    auto& ni = world->getComponent<engine::ecs::Interactive>(npc);
    ni.interactionType = "talk";

    // --- 加载场景 + UI ---
    if (!engine.loadScene("assets/scenes/test/parallax_test.scene"))
    {
        spdlog::warn("Scene not found, running without 2.5D background.");
    }
    if (!engine.loadUI("assets/ui/test_hud.json"))
    {
        spdlog::warn("UI not found, running without HUD.");
    }

    // --- 启动粒子（铁匠铺烟囱火花） ---
    if (auto* ps = engine.ecsWorld() ? reinterpret_cast<engine::render::particles::ParticleSystem*>(nullptr) : nullptr)
    {
        (void)ps; // 粒子通过 Lua 发射更方便
    }

    // --- 启动转场（淡入） ---
    // TransitionManager 由 Engine 内部持有，此处通过 Lua 调用
    // engine.transitionToScene(...) 或 engine.startFadeIn(...)

    spdlog::info("=== Controls ===");
    spdlog::info("WASD=move on XZ plane | Arrows=move camera (dolly/strafe) | E=talk | F1=Editor | ESC=quit");
    spdlog::info("Lua API: engine.playSound(path) | engine.spawnParticles(preset,x,y) | engine.setBloom(on) | engine.setVignette(on)");

    engine.run();
    engine.shutdown();

    return 0;
}
