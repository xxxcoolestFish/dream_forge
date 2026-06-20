/**
 * @file engine/core/engine.cpp
 * @brief 引擎初始化实现 — Step 3：集成 ECS World
 */

#include "engine/core/engine.h"
#include "engine/core/game_loop.h"
#include "engine/render/render_backend.h"
#include "engine/ecs/world.h"
#include "engine/ecs/systems/movement_system.h"
#include "engine/input/input_system.h"

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
    GLFWwindow*   window        = nullptr;
    bool          initialized   = false;
    bool          shouldQuit    = false;

    std::unique_ptr<render::RenderBackend> renderBackend;
    std::unique_ptr<ecs::World>            ecsWorld;
    std::unique_ptr<input::InputSystem>    inputSystem;
    std::unique_ptr<GameLoop>              gameLoop;
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

    // 初始化顺序：Window → Render → Input → ECS → Resource
    // Input 在 ECS 之前，因为 System 可能依赖输入
    if (!initWindow())          return false;
    if (!initRenderBackend())   return false;
    if (!initInput())           return false;
    if (!initECS())             return false;
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

    if (!glfwInit())
    {
        spdlog::critical("Failed to initialize GLFW");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_FALSE);
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
    renderCfg.debugMode = true;

    if (!m_impl->renderBackend->init(m_impl->window, renderCfg))
    {
        spdlog::critical("Failed to initialize render backend");
        return false;
    }

    return true;
}

bool Engine::initECS()
{
    spdlog::info("[4/5] Initializing ECS World...");

    // 创建 ECS World
    m_impl->ecsWorld = std::make_unique<ecs::World>();

    // 注册 System（按依赖顺序）
    m_impl->ecsWorld->registerSystem(
        std::make_unique<ecs::MovementSystem>(*m_impl->inputSystem)
    );

    spdlog::info("  ECS World initialized with {} system(s).", 1);
    return true;
}

bool Engine::initInput()
{
    spdlog::info("[3/5] Initializing Input System...");

    m_impl->inputSystem = std::make_unique<input::InputSystem>();

    if (!m_impl->inputSystem->init(m_impl->window))
    {
        spdlog::critical("Failed to initialize input system");
        return false;
    }

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

    auto& render  = *m_impl->renderBackend;
    auto& world   = *m_impl->ecsWorld;
    auto& loop    = m_impl->gameLoop;

    // Step 3：使用简单的每帧 Update + Render 循环
    // Step 5 将升级为 GameLoop 类的固定时间步循环
    double lastTime = glfwGetTime();
    int    frameCount = 0;

    while (!m_impl->shouldQuit && !glfwWindowShouldClose(m_impl->window))
    {
        // 计算 delta time
        double currentTime = glfwGetTime();
        double dt = currentTime - lastTime;
        lastTime = currentTime;

        // 防止首帧 dt 过大
        if (dt > 0.1) dt = 0.016;

        // 轮询 GLFW 事件
        glfwPollEvents();

        // --- 输入轮询（必须在逻辑更新之前） ---
        if (m_impl->inputSystem)
        {
            m_impl->inputSystem->poll();

            // ESC 退出
            if (m_impl->inputSystem->isKeyPressed(GLFW_KEY_ESCAPE))
            {
                m_impl->shouldQuit = true;
                break;
            }
        }

        // --- 逻辑更新 ---
        world.updateSystems(dt);

        // --- 渲染 ---
        render.beginFrame();
        // TODO Step 5-6: 精灵渲染（根据 ECS 中的 Sprite + Transform 绘制）
        render.endFrame();
        glfwSwapBuffers(m_impl->window);

        // 每秒打印一次帧率
        frameCount++;
        static double fpsTimer = 0.0;
        fpsTimer += dt;
        if (fpsTimer >= 1.0)
        {
            spdlog::debug("FPS: {:.1f}", frameCount / fpsTimer);
            frameCount = 0;
            fpsTimer   = 0.0;
        }
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
m_impl->ecsWorld.reset();
    m_impl->inputSystem->shutdown();
    m_impl->inputSystem.reset();
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

ecs::World* Engine::ecsWorld() const
{
    return m_impl->ecsWorld.get();
}

input::InputSystem* Engine::inputSystem() const
{
    return m_impl->inputSystem.get();
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
