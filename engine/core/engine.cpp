/**
 * @file engine/core/engine.cpp
 * @brief 引擎初始化实现
 */

#include "engine/core/engine.h"
#include "engine/render/render_backend.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>

namespace engine {

// =========================================================================
// PIMPL 内部实现
// =========================================================================
struct Engine::Impl
{
    EngineConfig  config;
    GLFWwindow*   window       = nullptr;
    bool          initialized  = false;
    bool          shouldQuit   = false;

    std::unique_ptr<render::RenderBackend> renderBackend;
};

// =========================================================================
// 构造 / 析构
// =========================================================================
Engine::Engine()
    : m_impl(std::make_unique<Impl>())
{
    spdlog::info("Engine instance created");
}

Engine::~Engine()
{
    if (m_impl && m_impl->initialized)
    {
        shutdown();
    }
}

// =========================================================================
// 初始化
// =========================================================================
bool Engine::init(const EngineConfig& config)
{
    m_impl->config = config;

    spdlog::info("========================================");
    spdlog::info("  AI Game Frame Engine v0.1.0");
    spdlog::info("========================================");
    spdlog::info("Window: {}x{}, title: '{}'",
        config.windowWidth, config.windowHeight, config.windowTitle);

    // 初始化顺序不可改变：Window → Render → ECS → Input → Resource
    if (!initWindow())          return false;
    if (!initRenderBackend())   return false;
    // if (!initECS())             return false;   // Step 3
    // if (!initInput())           return false;   // Step 4
    // if (!initResource())        return false;   // Step 4

    m_impl->initialized = true;
    spdlog::info("Engine initialization complete.");
    return true;
}

// =========================================================================
// 初始化子步骤
// =========================================================================

bool Engine::initWindow()
{
    spdlog::info("[1/5] Initializing GLFW window...");

    // 初始化 GLFW 库
    if (!glfwInit())
    {
        spdlog::critical("Failed to initialize GLFW");
        return false;
    }

    // OpenGL 3.3 Core Profile
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE);

    // 窗口属性
    glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

    GLFWmonitor* monitor = m_impl->config.fullscreen
        ? glfwGetPrimaryMonitor()
        : nullptr;

    m_impl->window = glfwCreateWindow(
        m_impl->config.windowWidth,
        m_impl->config.windowHeight,
        m_impl->config.windowTitle.c_str(),
        monitor,
        nullptr
    );

    if (!m_impl->window)
    {
        spdlog::critical("Failed to create GLFW window");
        glfwTerminate();
        return false;
    }

    // 窗口关闭回调
    glfwSetWindowUserPointer(m_impl->window, this);
    glfwSetWindowCloseCallback(m_impl->window, [](GLFWwindow* w) {
        auto* engine = static_cast<Engine*>(glfwGetWindowUserPointer(w));
        if (engine) engine->requestQuit();
    });

    spdlog::info("  GLFW window created successfully.");
    return true;
}

bool Engine::initRenderBackend()
{
    spdlog::info("[2/5] Initializing Render Backend...");

    m_impl->renderBackend = std::make_unique<render::RenderBackend>();

    render::RenderConfig renderCfg;
    renderCfg.width     = m_impl->config.windowWidth;
    renderCfg.height    = m_impl->config.windowHeight;
    renderCfg.vsync     = m_impl->config.vsync;
    renderCfg.debugMode = true;  // Phase 1 开发阶段启用调试

    if (!m_impl->renderBackend->init(m_impl->window, renderCfg))
    {
        spdlog::critical("Failed to initialize render backend");
        return false;
    }

    return true;
}

bool Engine::initECS()
{
    spdlog::info("[3/5] Initializing ECS World... (TODO Step 3)");
    return true;
}

bool Engine::initInput()
{
    spdlog::info("[4/5] Initializing Input System... (TODO Step 4)");
    return true;
}

bool Engine::initResource()
{
    spdlog::info("[5/5] Initializing Resource Manager... (TODO Step 4)");
    return true;
}

// =========================================================================
// 主循环
// =========================================================================
void Engine::run()
{
    if (!m_impl->initialized)
    {
        spdlog::error("Engine::run() called before init()");
        return;
    }

    spdlog::info("Engine entering main loop...");

    auto& render = *m_impl->renderBackend;

    while (!m_impl->shouldQuit && !glfwWindowShouldClose(m_impl->window))
    {
        // 轮询 GLFW 事件
        glfwPollEvents();

        // 渲染一帧
        render.beginFrame();
        // TODO Step 3-5: ECS update, Sprite rendering
        render.endFrame();

        // 交换缓冲区
        glfwSwapBuffers(m_impl->window);
    }

    spdlog::info("Engine main loop exited.");
}

// =========================================================================
// 关闭
// =========================================================================
void Engine::shutdown()
{
    if (!m_impl->initialized) return;

    spdlog::info("Engine shutting down...");

    // 逆序销毁子系统
    // m_impl->resourceManager.reset();  // Step 4
    // m_impl->inputSystem.reset();      // Step 4
    // m_impl->ecsWorld.reset();         // Step 3
    m_impl->renderBackend->shutdown();
    m_impl->renderBackend.reset();

    if (m_impl->window)
    {
        glfwDestroyWindow(m_impl->window);
        m_impl->window = nullptr;
    }
    glfwTerminate();

    m_impl->initialized = false;
    spdlog::info("Engine shutdown complete.");
}

// =========================================================================
// 查询
// =========================================================================
GLFWwindow* Engine::window() const
{
    return m_impl->window;
}

void Engine::requestQuit()
{
    m_impl->shouldQuit = true;
    spdlog::info("Quit requested.");
}

bool Engine::shouldQuit() const
{
    return m_impl->shouldQuit;
}

} // namespace engine
