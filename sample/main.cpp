/**
 * @file sample/main.cpp
 * @brief 示例游戏入口 — Phase 1 Step 2：窗口 + OpenGL 清屏测试
 *
 * 验证：
 *   1. GLFW 窗口创建成功
 *   2. OpenGL 3.3 上下文初始化成功
 *   3. 渲染循环正常运行（清屏 + swap）
 *   4. ESC 或关闭按钮正常退出
 */

#include "engine/core/engine.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    spdlog::set_level(spdlog::level::debug);
    spdlog::info("=== AI Game Frame — Phase 1 Step 2: Window Test ===");

    // 配置引擎
    engine::EngineConfig config;
    config.windowTitle  = "AI Game Frame — Phase 1";
    config.windowWidth  = 1280;
    config.windowHeight = 720;
    config.fullscreen   = false;
    config.vsync        = true;

    // 创建引擎
    engine::Engine engine;

    if (!engine.init(config))
    {
        spdlog::critical("Engine initialization failed. Exiting.");
        return 1;
    }

    spdlog::info("Engine initialized. Press ESC or close window to exit.");

    // 进入主循环
    engine.run();

    // 正常退出
    engine.shutdown();
    spdlog::info("=== Sample Game Exited Successfully ===");
    return 0;
}
