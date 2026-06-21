/**
 * @file sample/main.cpp
 * @brief Phase 3: 2.5D 视差场景渲染演示
 *
 * 操作：
 *   WASD     = 移动玩家（蓝色方块）
 *   方向键   = 旋转相机（↑↓←→）→ 不同深度层产生不同视差
 *   E        = 与 NPC 对话
 *   ESC      = 退出
 */

#include "engine/core/engine.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"

#include <spdlog/spdlog.h>

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
    spdlog::info("=== AI Game Frame — Phase 3: 2.5D Parallax Scene ===");

    engine::EngineConfig config;
    config.windowTitle  = "AI Game Frame — Phase 3 (2.5D Parallax)";
    config.windowWidth  = 1280;
    config.windowHeight = 720;

    engine::Engine engine;
    if (!engine.init(config))
    {
        spdlog::critical("Engine initialization failed.");
        return 1;
    }

    // --- ECS Entity ---
    auto* world = engine.ecsWorld();

    auto player = world->createEntity();
    world->addComponent<engine::ecs::Transform>(player);
    world->addComponent<engine::ecs::Sprite>(player);
    world->addComponent<engine::ecs::Stats>(player);
    world->addComponent<engine::ecs::Player>(player);
    auto& pt = world->getComponent<engine::ecs::Transform>(player);
    pt.position = glm::vec3(200.0f, 350.0f, 0.0f);
    world->getComponent<engine::ecs::Sprite>(player).tint = glm::vec4(0.2f, 0.6f, 1.0f, 1.0f);

    auto npc = world->createEntity();
    world->addComponent<engine::ecs::Transform>(npc);
    world->addComponent<engine::ecs::Sprite>(npc);
    world->addComponent<engine::ecs::DialogueSpeaker>(npc);
    world->addComponent<engine::ecs::Interactive>(npc);
    auto& nt = world->getComponent<engine::ecs::Transform>(npc);
    nt.position = glm::vec3(500.0f, 420.0f, 0.0f);
    world->getComponent<engine::ecs::Sprite>(npc).tint = glm::vec4(1.0f, 0.4f, 0.3f, 1.0f);
    auto& ns = world->getComponent<engine::ecs::DialogueSpeaker>(npc);
    ns.characterId = "老陈";
    ns.personalityPrompt = "暴躁但善良的老铁匠";
    auto& ni = world->getComponent<engine::ecs::Interactive>(npc);
    ni.interactionType = "talk";

    // --- Phase 3: 加载场景 ---
    if (!engine.loadScene("assets/scenes/test/parallax_test.scene"))
    {
        spdlog::warn("Scene not found, running without 2.5D background.");
    }

    spdlog::info("Controls: WASD=move | Arrows=rotate camera (parallax!) | E=talk | ESC=quit");
    spdlog::info("Try LEFT/RIGHT arrows to see near/far layers shift differently!");

    engine.run();
    engine.shutdown();

    return 0;
}
