/**
 * @file engine/core/game_loop.cpp
 * @brief 游戏主循环实现（当前为桩实现，Phase 1 Step 5 完善）
 */

#include "engine/core/game_loop.h"
#include <spdlog/spdlog.h>
#include <thread>

namespace engine {

struct GameLoop::Impl
{
    UpdateCallback updateCallback;
    RenderCallback renderCallback;
    bool           shouldExit    = false;
    double         currentFps    = 0.0;
    double         averageFps    = 0.0;
    int            frameCount    = 0;
    double         fpsAccumulator = 0.0;

    static constexpr double kFixedDt   = 1.0 / 60.0;  // 60 Hz 逻辑更新
    static constexpr int    kMaxFrames = 5;           // 最大跳帧数
};

GameLoop::GameLoop()
    : m_impl(new Impl())
{
    spdlog::debug("GameLoop created (fixed timestep: {:.4f}s)", Impl::kFixedDt);
}

GameLoop::~GameLoop()
{
    delete m_impl;
}

void GameLoop::setUpdateCallback(UpdateCallback cb)
{
    m_impl->updateCallback = std::move(cb);
}

void GameLoop::setRenderCallback(RenderCallback cb)
{
    m_impl->renderCallback = std::move(cb);
}

void GameLoop::run(std::function<bool()> shouldExit)
{
    spdlog::info("GameLoop starting...");

    // TODO Step 5: 实现固定时间步循环
    // 当前仅提供骨架：
    //
    // auto previousTime = std::chrono::high_resolution_clock::now();
    // double lag = 0.0;
    //
    // while (!shouldExit() && !m_impl->shouldExit)
    // {
    //     auto currentTime = std::chrono::high_resolution_clock::now();
    //     double elapsed = ...;
    //     lag += elapsed;
    //
    //     // 固定时间步逻辑更新
    //     while (lag >= kFixedDt && frames < kMaxFrames)
    //     {
    //         if (m_impl->updateCallback) m_impl->updateCallback(kFixedDt);
    //         lag -= kFixedDt;
    //     }
    //
    //     // 渲染（传入插值因子）
    //     double alpha = lag / kFixedDt;
    //     if (m_impl->renderCallback) m_impl->renderCallback(alpha);
    // }

    spdlog::info("GameLoop exited.");
}

void GameLoop::requestExit()
{
    m_impl->shouldExit = true;
}

double GameLoop::currentFPS() const
{
    return m_impl->currentFps;
}

double GameLoop::averageFPS() const
{
    return m_impl->averageFps;
}

} // namespace engine
