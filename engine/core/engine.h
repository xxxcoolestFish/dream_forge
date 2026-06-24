/**
 * @file engine/core/engine.h
 * @brief 引擎初始化入口
 *
 * 负责：
 *   - 各子系统（窗口、渲染、ECS、输入、资源）的创建与初始化
 *   - 引擎生命周期管理（init / run / shutdown）
 *   - 子系统间的协调
 *
 * 使用方式：
 *   Engine engine;
 *   engine.init(config);
 *   engine.run();   // 进入主循环，阻塞直到窗口关闭
 *   engine.shutdown();
 *
 * 注意事项：
 *   - init() 中子系统初始化顺序有依赖：Window → Render → ECS → Input → Resource
 *   - shutdown() 按逆序销毁
 */

#pragma once

#include <string>
#include <memory>

struct GLFWwindow;

namespace engine::ecs { class World; }
namespace engine::input { class InputSystem; }
namespace engine::script { class ScriptEngine; }
namespace engine::audio { class AudioEngine; }
namespace engine::ecs { class StatSystem; }
namespace engine::ecs { class InventorySystem; }

namespace engine {

struct EngineConfig
{
    std::string windowTitle  = "AI Game Frame";
    int         windowWidth  = 1280;
    int         windowHeight = 720;
    bool        fullscreen   = false;
    bool        vsync        = true;
};

class Engine
{
public:
    Engine();
    ~Engine();

    // 禁止拷贝
    Engine(const Engine&) = delete;
    Engine& operator=(const Engine&) = delete;

    // 生命周期
    bool init(const EngineConfig& config);
    void run();
    void shutdown();

    // 获取窗口句柄（供子系统和外部使用）
    GLFWwindow* window() const;

    // 获取 ECS World（供脚本层和测试使用）
    class ecs::World* ecsWorld() const;

    // 获取输入系统
    class input::InputSystem* inputSystem() const;

    // 获取事件总线
    class EventBus* eventBus() const;

    // Phase 3: 加载并激活场景（可选的 2.5D 背景场景）
    bool loadScene(const std::string& path);

    // Phase 4: 加载 UI 布局
    bool loadUI(const std::string& path);

    // Phase 5: 加载并执行 Lua 脚本
    bool loadScript(const std::string& path);

    // Phase 5: 获取脚本引擎
    class script::ScriptEngine* scriptEngine() const;

    // Phase 6.2: 获取音频引擎
    class audio::AudioEngine* audioEngine() const;

    // Phase 8: 属性系统 + 物品系统
    class ecs::StatSystem* statSystem() const;
    class ecs::InventorySystem* inventorySystem() const;

    // 请求退出
    void requestQuit();
    bool shouldQuit() const;

private:
    // 初始化顺序严格
    bool initWindow();
    bool initRenderBackend();
    bool initECS();
    bool initInput();
    bool initResource();

    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace engine
