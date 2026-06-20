/**
 * @file engine/core/engine.cpp
 * @brief 引擎初始化实现（当前为桩实现，Phase 1 Step 5 完善）
 */

#include "engine/core/engine.h"
#include <spdlog/spdlog.h>

namespace engine {

struct Engine::Impl
{
    EngineConfig config;
    bool         initialized = false;
    bool         shouldQuit  = false;
};

Engine::Engine()
    : m_impl(new Impl())
{
    spdlog::info("Engine instance created");
}

Engine::~Engine()
{
    if (m_impl && m_impl->initialized)
    {
        shutdown();
    }
    delete m_impl;
}

bool Engine::init(const EngineConfig& config)
{
    m_impl->config = config;

    spdlog::info("Engine initializing...");
    spdlog::info("  Window: {}x{}, title: '{}'",
        config.windowWidth, config.windowHeight, config.windowTitle);

    // TODO Step 2: 创建 GLFW 窗口
    // TODO Step 2: 初始化 bgfx 渲染后端
    // TODO Step 3: 初始化 ECS World
    // TODO Step 4: 初始化输入系统
    // TODO Step 4: 初始化资源管理器

    m_impl->initialized = true;
    spdlog::info("Engine initialization complete.");
    return true;
}

void Engine::run()
{
    if (!m_impl->initialized)
    {
        spdlog::error("Engine::run() called before init()");
        return;
    }

    spdlog::info("Engine entering main loop...");
    // TODO Step 5: 进入游戏主循环
    spdlog::info("Engine main loop exited.");
}

void Engine::shutdown()
{
    if (!m_impl->initialized) return;

    spdlog::info("Engine shutting down...");
    // TODO: 逆序销毁子系统
    m_impl->initialized = false;
    spdlog::info("Engine shutdown complete.");
}

void Engine::requestQuit()
{
    m_impl->shouldQuit = true;
}

} // namespace engine
