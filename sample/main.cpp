/**
 * @file sample/main.cpp
 * @brief 示例游戏入口 — Phase 1 最小可运行骨架
 *
 * 当前阶段仅验证：
 *   1. 编译链完整（CMake + 所有依赖）
 *   2. 引擎库链接成功
 *   3. 程序可以正常启动和退出
 *
 * 后续 Step 中将逐步添加窗口、渲染、ECS 等功能。
 */

#include <spdlog/spdlog.h>

int main(int argc, char* argv[])
{
    spdlog::info("AI Game Frame — Sample Game Starting...");

    // TODO Step 2: 创建窗口 + 初始化渲染后端
    // TODO Step 3: 初始化 ECS World
    // TODO Step 4: 初始化输入系统
    // TODO Step 5: 进入游戏主循环

    spdlog::info("Sample Game Exiting.");
    return 0;
}
