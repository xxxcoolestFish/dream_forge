/**
 * @file engine/render/render_backend.cpp
 * @brief OpenGL 3.3 渲染后端实现
 *
 * Phase 1 使用原生 OpenGL。当网络环境允许时，可切换为 bgfx 实现，
 * 仅需替换此 .cpp 文件，无需修改任何上层代码。
 */

#include "engine/render/render_backend.h"
#include "engine/render/gl_loader.h"

// GLFW 不自动包含任何 OpenGL 头文件（gl_loader.h 已处理）
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <cstdlib>

namespace engine::render {

RenderBackend::RenderBackend()
{
    spdlog::debug("RenderBackend created (OpenGL 3.3 backend)");
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

    spdlog::info("RenderBackend initializing (OpenGL 3.3 Core Profile)...");
    spdlog::info("  Resolution: {}x{}, vsync={}", config.width, config.height, config.vsync);

    // --- 1. 设置 OpenGL 上下文 ---
    glfwMakeContextCurrent(window);

    // --- 2. 加载 OpenGL 3.3 函数指针 ---
    if (!gl::load(window))
    {
        spdlog::critical("Failed to load OpenGL functions");
        return false;
    }

    spdlog::info("  OpenGL Version: {}", (const char*)gl::GetString(GL_VERSION));
    spdlog::info("  GPU: {}", (const char*)gl::GetString(GL_RENDERER));

    // --- 3. 设置默认渲染状态 ---
    gl::Viewport(0, 0, static_cast<int>(config.width), static_cast<int>(config.height));
    gl::ClearColor(0.1f, 0.1f, 0.15f, 1.0f);  // 深蓝灰背景

    // 启用混合（透明度支持）
    gl::Enable(GL_BLEND);
    gl::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // 垂直同步
    glfwSwapInterval(config.vsync ? 1 : 0);

    m_initialized = true;
    spdlog::info("RenderBackend initialized successfully.");
    return true;
}

void RenderBackend::shutdown()
{
    if (!m_initialized) return;

    spdlog::info("RenderBackend shutting down...");
    // OpenGL 资源由 GLFW 窗口销毁时自动清理
    m_initialized = false;
    spdlog::info("RenderBackend shutdown complete.");
}

void RenderBackend::beginFrame()
{
    gl::Clear(GL_COLOR_BUFFER_BIT);
}

void RenderBackend::endFrame()
{
    // swap 由 Engine::run() 中的 glfwSwapBuffers 完成
}

void RenderBackend::setViewport(int x, int y, int width, int height)
{
    gl::Viewport(x, y, width, height);
}

void RenderBackend::setClearColor(float r, float g, float b, float a)
{
    gl::ClearColor(r, g, b, a);
}

} // namespace engine::render
