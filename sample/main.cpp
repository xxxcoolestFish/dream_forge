/**
 * @file sample/main.cpp
 * @brief 示例游戏入口 — Phase 1 Step 3：ECS 管线验证
 *
 * 验证：
 *   1. GLFW 窗口 + OpenGL 3.3 渲染
 *   2. ECS World 创建 + MovementSystem 注册
 *   3. Entity 创建 + Component 挂载
 *   4. System 每帧执行（日志可见 MovementSystem 在更新 Entity 位置）
 */

#include "engine/core/engine.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"

#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    (void)argc; (void)argv;
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("=== AI Game Frame — Phase 1 Step 3: ECS Pipeline Test ===");

    // 配置引擎
    engine::EngineConfig config;
    config.windowTitle  = "AI Game Frame — Step 3 (ECS)";
    config.windowWidth  = 1280;
    config.windowHeight = 720;

    // 初始化引擎
    engine::Engine engine;
    if (!engine.init(config))
    {
        spdlog::critical("Engine initialization failed.");
        return 1;
    }

    // --- ECS 测试：创建 Entity ---
    auto* world = engine.ecsWorld();

    // 创建玩家 Entity
    auto player = world->createEntity();
    world->addComponent<engine::ecs::Transform>(player);
    world->addComponent<engine::ecs::Stats>(player);

    auto& playerTransform = world->getComponent<engine::ecs::Transform>(player);
    playerTransform.position = glm::vec3(100.0f, 100.0f, 0.0f);

    auto& playerStats = world->getComponent<engine::ecs::Stats>(player);
    playerStats.set("hp", 100.0f);
    playerStats.set("xp", 0.0f);

    spdlog::info("Player entity created (id={}): pos=({:.0f}, {:.0f}), HP={:.0f}",
        static_cast<uint32_t>(player),
        playerTransform.position.x,
        playerTransform.position.y,
        playerStats.get("hp"));

    // 创建 NPC Entity（仅 Transform）
    auto npc = world->createEntity();
    world->addComponent<engine::ecs::Transform>(npc);
    auto& npcTransform = world->getComponent<engine::ecs::Transform>(npc);
    npcTransform.position = glm::vec3(400.0f, 200.0f, 0.0f);

    spdlog::info("NPC entity created (id={}): pos=({:.0f}, {:.0f})",
        static_cast<uint32_t>(npc),
        npcTransform.position.x,
        npcTransform.position.y);

    spdlog::info("Starting main loop (entities will be moved by MovementSystem)...");
    spdlog::info("Press ESC or close window to exit.");

    // 进入主循环
    engine.run();

    // 退出前查看最终位置
    spdlog::info("Final player position: ({:.0f}, {:.0f})",
        playerTransform.position.x, playerTransform.position.y);
    spdlog::info("Final NPC position: ({:.0f}, {:.0f})",
        npcTransform.position.x, npcTransform.position.y);

    engine.shutdown();
    spdlog::info("=== Sample Game Exited Successfully ===");
    return 0;
}
