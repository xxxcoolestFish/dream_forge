/**
 * @file engine/core/engine.cpp
 * @brief 引擎初始化实现 — Step 3：集成 ECS World
 */

#include "engine/core/engine.h"
#include "engine/core/game_loop.h"
#include "engine/render/render_backend.h"
#include "engine/render/sprite.h"
#include "engine/render/gl_loader.h"
#include "engine/ui/ui_renderer.h"
#include "engine/render/scene/camera.h"
#include "engine/render/scene/scene_renderer.h"
#include "engine/scene/scene.h"
#include "engine/scene/scene_loader.h"
#include "engine/ecs/world.h"
#include "engine/ai/ai_client.h"
#include "engine/ecs/systems/movement_system.h"
#include "engine/ecs/systems/dialogue_system.h"
#include "engine/ecs/systems/dialogue_ui_system.h"
#include "engine/ecs/systems/interaction_system.h"
#include "engine/input/input_system.h"
#include "engine/script/script_engine.h"
#include "engine/audio/audio_engine.h"
#include "engine/render/render_target.h"
#include "engine/render/post_process.h"
#include "engine/render/effects/effect_chain.h"
#include "engine/render/effects/bloom_effect.h"
#include "engine/render/effects/vignette_effect.h"
#include "engine/render/particles/particle_system.h"
#include "engine/render/transitions/transition_manager.h"
#include "engine/tools/editor.h"
#include "engine/narrative/quest_manager.h"
#include "engine/narrative/story_flags.h"
#include "engine/narrative/condition_evaluator.h"

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
    std::unique_ptr<narrative::QuestManager>  questManager;
    std::unique_ptr<narrative::StoryFlags>   storyFlags;
    std::unique_ptr<narrative::ConditionEvaluator> conditionEval;
    std::unique_ptr<ecs::DialogueUISystem>   dialogueUI;

    // Phase 6.3/6.4: FBO + 后处理特效链
    std::unique_ptr<render::RenderTarget> mainRenderTarget;
    std::unique_ptr<render::effects::EffectChain> effectChain;

    // Phase 6.5/6.6/6.8: 粒子 + 转场 + 编辑器
    std::unique_ptr<render::particles::ParticleSystem> particleSystem;
    std::unique_ptr<render::transitions::TransitionManager> transitionManager;
    std::unique_ptr<tools::Editor> editor;

    // Phase 6.2: 音频
    std::unique_ptr<audio::AudioEngine> audioEngine;
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
    // 初始光标捕获（ESC 切换）
    if (m_impl->inputSystem)
        m_impl->inputSystem->setCursorVisible(false);

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
    auto dialogueSys = std::make_unique<ecs::DialogueSystem>(*m_impl->aiClient, *m_impl->eventBus);
    ecs::DialogueSystem* dialogueSysPtr = dialogueSys.get();
    m_impl->ecsWorld->registerSystem(std::move(dialogueSys));

    spdlog::info("  ECS World initialized with {} system(s).", 3);

    // 3. 精灵渲染器
    m_impl->spriteRenderer = std::make_unique<render::SpriteRenderer>();
    if (!m_impl->spriteRenderer->init())
    {
        spdlog::critical("Failed to initialize sprite renderer");
        return false;
    }

    // 3b. FBO 渲染目标 + 后处理基座（在精灵渲染器之后）
    m_impl->mainRenderTarget = std::make_unique<render::RenderTarget>();
    if (!m_impl->mainRenderTarget->init(
            static_cast<uint32_t>(m_impl->config.windowWidth),
            static_cast<uint32_t>(m_impl->config.windowHeight)))
    {
        spdlog::warn("Main RenderTarget init failed — disabling post-processing.");
        m_impl->mainRenderTarget.reset();
    }
    else
    {
        m_impl->effectChain = std::make_unique<render::effects::EffectChain>();
        if (!m_impl->effectChain->init(
                static_cast<uint32_t>(m_impl->config.windowWidth),
                static_cast<uint32_t>(m_impl->config.windowHeight)))
        {
            spdlog::warn("EffectChain init failed.");
            m_impl->effectChain.reset();
            m_impl->mainRenderTarget.reset();
        }
        else
        {
            // 添加默认特效（默认关闭，由 Lua 或配置开启）
            auto bloom = std::make_unique<render::effects::BloomEffect>();
            if (bloom->init(
                    static_cast<uint32_t>(m_impl->config.windowWidth),
                    static_cast<uint32_t>(m_impl->config.windowHeight)))
            {
                bloom->setEnabled(false);  // 默认关闭
                bloom->setIntensity(0.5f);
                m_impl->effectChain->addEffect(std::move(bloom));
            }

            auto vignette = std::make_unique<render::effects::VignetteEffect>();
            if (vignette->init(
                    static_cast<uint32_t>(m_impl->config.windowWidth),
                    static_cast<uint32_t>(m_impl->config.windowHeight)))
            {
                vignette->setEnabled(false);  // 默认关闭
                m_impl->effectChain->addEffect(std::move(vignette));
            }
        }
    }

    // 4. UI 渲染器 + 字体 + 数据绑定
    m_impl->uiRenderer = std::make_unique<ui::UIRenderer>();
    m_impl->uiRenderer->setEcsWorld(m_impl->ecsWorld.get());
    if (!m_impl->uiRenderer->loadFont("C:/Windows/Fonts/consola.ttf", 22.0f))
    {
        spdlog::warn("Failed to load consola.ttf, trying arial.ttf...");
        m_impl->uiRenderer->loadFont("C:/Windows/Fonts/arial.ttf", 20.0f);
    }

    // 4b. 对话 UI 系统（UI 渲染器之后，订阅对话事件）
    m_impl->dialogueUI = std::make_unique<ecs::DialogueUISystem>(
        *m_impl->eventBus,
        *m_impl->uiRenderer);

    // 5. 叙事系统（ScriptEngine 之前，脚本需要绑定叙事 API）
    m_impl->questManager = std::make_unique<narrative::QuestManager>(
        m_impl->ecsWorld.get(),
        m_impl->eventBus.get());
    m_impl->questManager->loadDefinitions("assets/quests/");

    m_impl->storyFlags = std::make_unique<narrative::StoryFlags>();

    // 创建条件求值器，连接 StoryFlags 和 QuestManager 作为数据源
    m_impl->conditionEval = std::make_unique<narrative::ConditionEvaluator>();
    m_impl->conditionEval->setFlagQuery(
        [flags = m_impl->storyFlags.get()](const std::string& key, bool& out) {
            if (!flags->has(key)) return false;
            out = flags->getBool(key);
            return true;
        });
    m_impl->conditionEval->setQuestQuery(
        [qm = m_impl->questManager.get()](const std::string& questId, std::string& out) {
            // 通过 QuestManager 检查任务状态（查找活跃/已完成任务）
            // 简化：检查是否有该 questId 的定义
            if (qm->getDefinition(questId))
            {
                out = "completed"; // 后续完善状态查询
                return true;
            }
            return false;
        });

    // 将条件求值器注入 DialogueSystem
    dialogueSysPtr->setConditionEvaluator(m_impl->conditionEval.get());

    // 6. 脚本引擎
    m_impl->scriptEngine = std::make_unique<script::ScriptEngine>();
    m_impl->scriptEngine->init(
        m_impl->ecsWorld.get(),
        m_impl->eventBus.get(),
        m_impl->inputSystem.get(),
        m_impl->questManager.get());

    // 注册剧情标记绑定（StoryFlags 在 init 之后才创建，手动注入）
    {
        sol::table narr = m_impl->scriptEngine->luaState()["narrative"];
        narr.set_function("setFlag",
            [sf = m_impl->storyFlags.get()](const std::string& key, bool val) {
                sf->setBool(key, val);
            });
        narr.set_function("getFlag",
            [sf = m_impl->storyFlags.get()](const std::string& key) -> bool {
                return sf->getBool(key, false);
            });
        narr.set_function("hasFlag",
            [sf = m_impl->storyFlags.get()](const std::string& key) -> bool {
                return sf->has(key);
            });
    }

    // 6.5: 粒子系统
    m_impl->particleSystem = std::make_unique<render::particles::ParticleSystem>();
    if (!m_impl->particleSystem->init(4096))
    {
        spdlog::warn("ParticleSystem init failed.");
        m_impl->particleSystem.reset();
    }

    // 6.6: 转场管理器
    m_impl->transitionManager = std::make_unique<render::transitions::TransitionManager>();
    if (!m_impl->transitionManager->init())
    {
        spdlog::warn("TransitionManager init failed.");
        m_impl->transitionManager.reset();
    }

    // 6.8: ImGui 编辑器
    m_impl->editor = std::make_unique<tools::Editor>(
        m_impl->ecsWorld.get(), m_impl->inputSystem.get());
    if (!m_impl->editor->init(m_impl->window))
    {
        spdlog::warn("Editor init failed.");
        m_impl->editor.reset();
    }

    // 7. 音频引擎
    m_impl->audioEngine = std::make_unique<audio::AudioEngine>();
    if (!m_impl->audioEngine->init())
    {
        spdlog::warn("AudioEngine initialization failed — audio disabled.");
        m_impl->audioEngine.reset();
    }
    else
    {
        // 注册音频 Lua 绑定
        sol::table eng = m_impl->scriptEngine->luaState()["engine"];
        eng.set_function("playSound",
            [ae = m_impl->audioEngine.get()](const std::string& path, sol::object vol) {
                float v = vol.is<float>() ? vol.as<float>() : 1.0f;
                return ae->playSound(path, v);
            });
        eng.set_function("playMusic",
            [ae = m_impl->audioEngine.get()](const std::string& path, sol::object vol, sol::object loop) {
                float v = vol.is<float>() ? vol.as<float>() : 0.7f;
                bool  l = loop.is<bool>() ? loop.as<bool>() : true;
                ae->playMusic(path, v, l);
            });
        eng.set_function("stopMusic",
            [ae = m_impl->audioEngine.get()]() { ae->stopMusic(); });
        eng.set_function("setMasterVolume",
            [ae = m_impl->audioEngine.get()](float vol) { ae->setMasterVolume(vol); });
    }

    // 注册粒子 Lua 绑定
    if (m_impl->particleSystem)
    {
        sol::table eng = m_impl->scriptEngine->luaState()["engine"];
        eng.set_function("spawnParticles",
            [ps = m_impl->particleSystem.get()](const std::string& preset, float x, float y) {
                return ps->emitPreset(preset, {x, y});
            });
    }

    // 注册后期特效 Lua 绑定
    if (m_impl->effectChain)
    {
        sol::table eng = m_impl->scriptEngine->luaState()["engine"];

        eng.set_function("setBloom",
            [chain = m_impl->effectChain.get()](bool enabled, sol::object intensity) {
                auto* bloom = dynamic_cast<render::effects::BloomEffect*>(chain->findEffect("Bloom"));
                if (bloom)
                {
                    bloom->setEnabled(enabled);
                    if (intensity.is<float>()) bloom->setIntensity(intensity.as<float>());
                }
            });

        eng.set_function("setVignette",
            [chain = m_impl->effectChain.get()](bool enabled, sol::object radius) {
                auto* vignette = dynamic_cast<render::effects::VignetteEffect*>(chain->findEffect("Vignette"));
                if (vignette)
                {
                    vignette->setEnabled(enabled);
                    if (radius.is<float>()) vignette->setRadius(radius.as<float>());
                }
            });
    }

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

                // ESC 切换光标捕获
                if (m_impl->inputSystem->isKeyPressed(GLFW_KEY_ESCAPE))
                {
                    bool cursorVis = m_impl->inputSystem->isCursorVisible();
                    m_impl->inputSystem->setCursorVisible(!cursorVis);
                    if (!cursorVis)
                        spdlog::info("Cursor captured — FPS mode. ESC to release.");
                    else
                        spdlog::info("Cursor released. ESC to re-capture.");
                }

                // FPS 相机：鼠标旋转 + WASD 移动
                auto& cam = m_impl->sceneCamera;
                bool cursorCaptured = !m_impl->inputSystem->isCursorVisible();

                if (cursorCaptured && m_impl->activeScene)
                {
                    // 鼠标旋转视角
                    float mdX = static_cast<float>(m_impl->inputSystem->mouseDeltaX());
                    float mdY = static_cast<float>(m_impl->inputSystem->mouseDeltaY());
                    if (mdX != 0.0f || mdY != 0.0f)
                        cam.processMouse(mdX, mdY);

                    // WASD 移动
                    float fwd = 0.0f, rgt = 0.0f;
                    if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_W)) fwd += 1.0f;
                    if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_S)) fwd -= 1.0f;
                    if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_A)) rgt -= 1.0f;
                    if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_D)) rgt += 1.0f;

                    if (fwd != 0.0f || rgt != 0.0f)
                        cam.move(fwd, rgt, static_cast<float>(dt));
                }

                // 物理更新（重力 + 落地）
                cam.updatePhysics(static_cast<float>(dt));

                // 同步玩家实体到相机位置
                {
                    auto view = world.view<ecs::Player>();
                    for (auto e : view)
                    {
                        auto& t = world.getComponent<ecs::Transform>(e);
                        glm::vec3 cp = cam.position();
                        t.position = cp;
                    }
                }

                // 跳跃（空格）
                if (m_impl->inputSystem->isKeyPressed(GLFW_KEY_SPACE))
                    cam.jump();

                // 方向键：自由飞行模式（始终可用，用于调试浏览）
                if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_UP))    cam.move( 1.0f, 0.0f, static_cast<float>(dt), 300.0f);
                if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_DOWN))  cam.move(-1.0f, 0.0f, static_cast<float>(dt), 300.0f);
                if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_LEFT))  cam.move( 0.0f,-1.0f, static_cast<float>(dt), 300.0f);
                if (m_impl->inputSystem->isKeyHeld(GLFW_KEY_RIGHT)) cam.move( 0.0f, 1.0f, static_cast<float>(dt), 300.0f);
            }

            // --- 编辑器更新（F1 切换等，最优先） ---
            if (m_impl->editor)
            {
                m_impl->editor->update(static_cast<float>(dt));
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

            // --- 粒子系统更新 ---
            if (m_impl->particleSystem)
            {
                m_impl->particleSystem->update(static_cast<float>(dt));
            }

            // --- 转场更新 ---
            if (m_impl->transitionManager)
            {
                m_impl->transitionManager->update(static_cast<float>(dt));
            }

            // --- 音频引擎更新 ---
            if (m_impl->audioEngine)
            {
                m_impl->audioEngine->update(static_cast<float>(dt));
            }

            // --- 逻辑更新 ---
            world.updateSystems(dt);

            // --- 对话 UI 更新（键盘输入处理） ---
            if (m_impl->dialogueUI && m_impl->inputSystem)
            {
                m_impl->dialogueUI->update(m_impl->inputSystem.get());
            }

            // --- 渲染 ---
            const uint32_t sw = static_cast<uint32_t>(m_impl->config.windowWidth);
            const uint32_t sh = static_cast<uint32_t>(m_impl->config.windowHeight);
            const bool useFbo = (m_impl->mainRenderTarget != nullptr && m_impl->effectChain != nullptr);

            // Phase 6.3: 场景 → FBO（如果可用）
            if (useFbo)
            {
                m_impl->mainRenderTarget->bind();
            }
            render.beginFrame();  // 清除颜色+深度

            // 3D 场景渲染（深度测试开）
            render::gl::Enable(GL_DEPTH_TEST);

            if (m_impl->activeScene && m_impl->spriteRenderer)
            {
                m_impl->sceneRenderer.render(
                    *m_impl->activeScene,
                    m_impl->sceneCamera,
                    *m_impl->spriteRenderer);
            }

            if (m_impl->spriteRenderer)
            {
                m_impl->spriteRenderer->render3D(
                    world,
                    m_impl->sceneCamera.viewProjection());
            }

            if (m_impl->particleSystem)
            {
                m_impl->particleSystem->render(
                    m_impl->config.windowWidth,
                    m_impl->config.windowHeight);
            }

            render::gl::Disable(GL_DEPTH_TEST);

            // Phase 6.3/6.4: 解绑 FBO → 后处理特效链
            if (useFbo)
            {
                m_impl->mainRenderTarget->unbind();
                // 清除默认帧缓冲
                render::gl::Clear(GL_COLOR_BUFFER_BIT);
                m_impl->effectChain->process(
                    m_impl->mainRenderTarget->colorTextureId(), sw, sh);
            }

            // Phase 4: UI 渲染（最上层，始终在后处理之后）
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

            // 编辑器叠加（UI 之上，转场之下）
            if (m_impl->editor)
            {
                m_impl->editor->render();
            }

            // 转场叠加（最顶层）
            if (m_impl->transitionManager)
            {
                m_impl->transitionManager->render(
                    m_impl->config.windowWidth, m_impl->config.windowHeight);
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
    if (m_impl->audioEngine)
    {
        m_impl->audioEngine->shutdown();
        m_impl->audioEngine.reset();
    }
    m_impl->effectChain.reset();
    m_impl->mainRenderTarget.reset();
    m_impl->editor.reset();
    m_impl->transitionManager.reset();
    m_impl->particleSystem.reset();
    m_impl->dialogueUI.reset();
    m_impl->conditionEval.reset();
    m_impl->storyFlags.reset();
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
    // FPS 相机：初始站在场景前方，面朝场景（-Z 方向）
    m_impl->sceneCamera.setScreenSize(
        static_cast<float>(m_impl->config.windowWidth),
        static_cast<float>(m_impl->config.windowHeight));
    m_impl->sceneCamera.setPosition(glm::vec3(640.0f, 360.0f, 150.0f));
    m_impl->sceneCamera.setReferenceZ(150.0f);  // 层尺寸以此 Z 为基准，固定不变

    spdlog::info("Engine: scene '{}' loaded ({} layers, 3D perspective)",
        m_impl->activeScene->name, m_impl->activeScene->layers.size());

    // 场景环境音
    if (m_impl->audioEngine && !m_impl->activeScene->ambianceAudio.empty())
    {
        m_impl->audioEngine->playAmbiance(m_impl->activeScene->ambianceAudio);
    }

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

audio::AudioEngine* Engine::audioEngine() const
{
    return m_impl->audioEngine.get();
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
