/**
 * @file engine/core/engine.h
 * @brief 引擎初始化入口
 *
 * 负责：
 *   - 各子系统（渲染、ECS、输入、资源）的创建与初始化
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

namespace engine {

struct EngineConfig
{
    std::string windowTitle  = "AI Game Frame";
    int         windowWidth  = 1280;
    int         windowHeight = 720;
    bool        fullscreen   = false;
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

    // 请求退出
    void requestQuit();

private:
    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace engine
