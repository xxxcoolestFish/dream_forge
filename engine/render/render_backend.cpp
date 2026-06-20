/**
 * @file engine/render/render_backend.cpp
 * @brief 渲染后端实现（当前为桩，Phase 1 Step 2 完善）
 */

#include "engine/render/render_backend.h"
#include <spdlog/spdlog.h>

namespace engine::render {

RenderBackend::RenderBackend()
{
    spdlog::debug("RenderBackend created");
}

RenderBackend::~RenderBackend()
{
    if (m_initialized)
    {
        shutdown();
    }
}

bool RenderBackend::init(GLFWwindow* window, const RenderConfig& config)
{
    if (!window)
    {
        spdlog::error("RenderBackend::init: null window handle");
        return false;
    }

    m_config = config;
    spdlog::info("RenderBackend initializing: {}x{}, vsync={}, debug={}",
        config.width, config.height, config.vsync, config.debugMode);

    // TODO Step 2: bgfx::init() 调用
    // TODO Step 2: 设置 DX11 后端
    // TODO Step 2: 创建默认视口

    m_initialized = true;
    spdlog::info("RenderBackend initialized.");
    return true;
}

void RenderBackend::shutdown()
{
    if (!m_initialized) return;
    spdlog::info("RenderBackend shutting down...");
    // TODO Step 2: bgfx::shutdown()
    m_initialized = false;
}

void RenderBackend::beginFrame()
{
    // TODO Step 2: bgfx::touch() + 设置视口
}

void RenderBackend::endFrame()
{
    // TODO Step 2: bgfx::frame()
}

void RenderBackend::setViewport(uint16_t x, uint16_t y, uint16_t width, uint16_t height)
{
    // TODO Step 2: bgfx::setViewRect()
}

void* RenderBackend::nativeHandle() const
{
    // TODO Step 2: 返回 bgfx 上下文
    return nullptr;
}

} // namespace engine::render
