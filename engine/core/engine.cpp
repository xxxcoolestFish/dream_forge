/**
 * @file engine/core/engine.cpp
 * @brief 引擎初始化实现 — Step 3：集成 ECS World
 */

#include "engine/core/engine.h"
#include "engine/core/game_loop.h"
#include "engine/render/render_backend.h"
#include "engine/render/sprite.h"
#include "engine/ui/ui_renderer.h"
#include "engine/render/scene/camera.h"
#include "engine/render/scene/scene_renderer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_loader.h"
#include "engine/ecs/world.h"
#include "engine/ai/ai_client.h"
#include "engine/ecs/systems/movement_system.h"
#include "engine/ecs/systems/dialogue_system.h"
#include "engine/ecs/systems/interaction_system.h"
#include "engine/input/input_system.h"
#include "engine/script/script_engine.h"
#include "engine/narrative/quest_manager.h"

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <typeinfo>

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
    std::unique_ptr<render::SpriteRenderer> spriteRenderer;
    std::unique_ptr<ecs::World>             ecsWorld;
    std::unique_ptr<input::InputSystem>     inputSystem;
    std::unique_ptr<ai::AiClient>           aiClient;
    std::unique_ptr<EventBus>               eventBus;
    std::unique_ptr<GameLoop>               gameLoop;

    // Phase 3: 场景渲染
    std::optional<scene::Scene>      activeScene;
    render::Camera                   sceneCamera;
    render::SceneRenderer            sceneRenderer;

    // Phase 4: UI
    std::unique_ptr<ui::UIRenderer>  uiRenderer;

    // Phase 5: 脚本
    std::unique_ptr<script::ScriptEngine> scriptEngine;

    // Phase 5: 叙事系统
    std::unique_ptr<narrative::QuestManager> questManager;
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

    // 0. 创建基础设施（EventBus, AiClient 等 System 依赖）
    m_impl->eventBus = std::make_unique<EventBus>();
    m_impl->aiClient = std::make_unique<ai::AiClient>();
    if (m_impl->aiClient->connect())
    {
        spdlog::info("  AI client connected to Python service.");
    }
    else
    {
        spdlog::warn("  AI client not connected (Python service not running).");
    }

    // 1. 创建 ECS World
    m_impl->ecsWorld = std::make_unique<ecs::World>();

    // 2. 注册 System
    m_impl->ecsWorld->registerSystem(
        std::make_unique<ecs::MovementSystem>(*m_impl->inputSystem)
    );
    m_impl->ecsWorld->registerSystem(
        std::make_unique<ecs::InteractionSystem>(*m_impl->inputSystem, *m_impl->eventBus)
    );
    m_impl->ecsWorld->registerSystem(
        std::make_unique<ecs::DialogueSystem>(*m_impl->aiClient, *m_impl->eventBus)
    );

    spdlog::info("  ECS World initialized with {} system(s).", 3);

    // 3. 精灵渲染器
    m_impl->spriteRenderer = std::make_unique<render::SpriteRenderer>();
    if (!m_impl->spriteRenderer->init())
    {
        spdlog::critical("Failed to initialize sprite renderer");
        return false;
    }

    // 4. UI 渲染器 + 字体 + 数据绑定
    m_impl->uiRenderer = std::make_unique<ui::UIRenderer>();
    m_impl->uiRenderer->setEcsWorld(m_impl->ecsWorld.get());
    if (!m_impl->uiRenderer->loadFont("C:/Windows/Fonts/consola.ttf", 22.0f))
    {
        spdlog::warn("Failed to load consola.ttf, trying arial.ttf...");
        m_impl->uiRenderer->loadFont("C:/Windows/Fonts/arial.ttf", 20.0f);
    }

    // 5. 任务管理器（ScriptEngine 之前创建，因为脚本需要绑定 Quest API）
    m_impl->questManager = std::make_unique<narrative::QuestManager>(
        m_impl->ecsWorld.get(),
        m_impl->eventBus.get());
    m_impl->questManager->loadDefinitions("assets/quests/");

    // 6. 脚本引擎
    m_impl->scriptEngine = std::make_unique<script::ScriptEngine>();
    m_impl->scriptEngine->init(
        m_impl->ecsWorld.get(),
        m_impl->eventBus.get(),
        m_impl->inputSystem.get(),
        m_impl->questManager.get());

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
        try
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

                // Phase 3: 方向键旋转相机（视差效果）
                if (m_impl->activeScene)
                {
                    auto& cam = m_impl->sceneCamera;
                    float hRot = 0.0f, vRot = 0.0f;
                    float rotSpeed = 30.0f; // 度/秒
                    if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_LEFT))  hRot -= rotSpeed * static_cast<float>(dt);
                    if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_RIGHT)) hRot += rotSpeed * static_cast<float>(dt);
                    if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_UP))    vRot -= rotSpeed * 0.3f * static_cast<float>(dt);
                    if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_DOWN))  vRot += rotSpeed * 0.3f * static_cast<float>(dt);
                    cam.setRotation(cam.horizontalAngle() + hRot, cam.verticalAngle() + vRot);
                }
            }

            // --- 脚本钩子（Lua onUpdate，在逻辑更新之前） ---
            if (m_impl->scriptEngine)
            {
                m_impl->scriptEngine->onUpdate(dt);
            }

            // --- 任务管理器更新 ---
            if (m_impl->questManager)
            {
                m_impl->questManager->update(dt);
            }

            // --- 逻辑更新 ---
            world.updateSystems(dt);

            // --- 渲染 ---
            render.beginFrame();

            // Phase 3: 先渲染场景层（背景 → 前景），再渲染 ECS 精灵（玩家、NPC）
            if (m_impl->activeScene && m_impl->spriteRenderer)
            {
                m_impl->sceneRenderer.render(
                    *m_impl->activeScene,
                    m_impl->sceneCamera,
                    *m_impl->spriteRenderer,
                    m_impl->config.windowWidth,
                    m_impl->config.windowHeight);
                m_impl->spriteRenderer->flush(
                    m_impl->config.windowWidth,
                    m_impl->config.windowHeight);
            }

            if (m_impl->spriteRenderer)
            {
                m_impl->spriteRenderer->render(
                    world,
                    m_impl->config.windowWidth,
                    m_impl->config.windowHeight);
            }

            // Phase 4: UI 渲染（最上层）+ 鼠标事件路由
            if (m_impl->uiRenderer)
            {
                m_impl->uiRenderer->update(static_cast<float>(dt));
                m_impl->uiRenderer->render(*m_impl->spriteRenderer,
                    m_impl->config.windowWidth,
                    m_impl->config.windowHeight);

                // 鼠标事件路由到 UI
                if (m_impl->inputSystem)
                {
                    if (m_impl->inputSystem->isMousePressed(
                            engine::input::MouseButton::Left))
                    {
                        float mx = static_cast<float>(m_impl->inputSystem->mouseX());
                        float my = static_cast<float>(m_impl->inputSystem->mouseY());
                        spdlog::debug("Mouse click at ({:.0f}, {:.0f})", mx, my);
                        m_impl->uiRenderer->onMouseDown(mx, my);
                    }
                }
            }

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
        catch (const std::exception& e)
        {
            spdlog::error("!!! Exception in game loop: {}", e.what());
            spdlog::error("!!! Type: {}", typeid(e).name());
            break;
        }
        catch (...)
        {
            spdlog::error("!!! Unknown exception in game loop");
            break;
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
    m_impl->questManager.reset();
    m_impl->scriptEngine->shutdown();
    m_impl->scriptEngine.reset();
    m_impl->uiRenderer.reset();
    m_impl->ecsWorld.reset();
    m_impl->aiClient->disconnect();
    m_impl->aiClient.reset();
    m_impl->spriteRenderer->shutdown();
    m_impl->spriteRenderer.reset();
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

EventBus* Engine::eventBus() const
{
    return m_impl->eventBus.get();
}

bool Engine::loadScene(const std::string& path)
{
    auto sceneOpt = scene::SceneLoader::loadFromFile(path);
    if (!sceneOpt)
    {
        spdlog::error("Engine::loadScene: failed to load '{}': {}",
            path, scene::SceneLoader::lastError());
        return false;
    }

    m_impl->activeScene = std::move(*sceneOpt);
    m_impl->sceneCamera.setScreenSize(
        static_cast<float>(m_impl->config.windowWidth),
        static_cast<float>(m_impl->config.windowHeight));
    m_impl->sceneCamera.setPosition(640.0f, 360.0f); // 默认看场景中心

    spdlog::info("Engine: scene '{}' loaded ({} layers)",
        m_impl->activeScene->name, m_impl->activeScene->layers.size());
    return true;
}

bool Engine::loadUI(const std::string& path)
{
    if (!m_impl->uiRenderer)
    {
        spdlog::error("Engine::loadUI: UIRenderer not initialized");
        return false;
    }
    return m_impl->uiRenderer->loadFromFile(path);
}

bool Engine::loadScript(const std::string& path)
{
    if (!m_impl->scriptEngine)
    {
        spdlog::error("Engine::loadScript: ScriptEngine not initialized");
        return false;
    }
    return m_impl->scriptEngine->loadFile(path);
}

script::ScriptEngine* Engine::scriptEngine() const
{
    return m_impl->scriptEngine.get();
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
