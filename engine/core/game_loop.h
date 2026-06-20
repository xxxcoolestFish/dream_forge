/**
 * @file engine/core/game_loop.h
 * @brief 游戏主循环
 *
 * 负责：
 *   - 固定时间步逻辑更新（ECS Systems）
 *   - 可变时间步渲染
 *   - 帧率统计
 *
 * 循环顺序（每帧）：
 *   Input → Script → ECS Update → AI Poll → Render
 *
 * 使用固定时间步（fixed timestep）模式：
 *   - 逻辑更新以固定 dt（如 1/60s）执行
 *   - 渲染以可变帧率执行
 *   - 逻辑追上渲染，允许跳帧但限制最大跳过帧数
 */

#pragma once

#include <functional>
#include <chrono>

namespace engine {

class GameLoop
{
public:
    using UpdateCallback = std::function<void(double dt)>;
    using RenderCallback = std::function<void(double alpha)>;

    GameLoop();
    ~GameLoop();

    // 设置回调
    void setUpdateCallback(UpdateCallback cb);
    void setRenderCallback(RenderCallback cb);

    // 运行循环（阻塞直到 shouldExit 返回 true）
    void run(std::function<bool()> shouldExit);

    // 请求退出
    void requestExit();

    // 帧率信息
    double currentFPS() const;
    double averageFPS() const;

private:
    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace engine
