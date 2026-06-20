/**
 * @file engine/input/input_system.h
 * @brief 输入系统 — 原始输入 → 动作映射 → 事件发布
 *
 * 负责：
 *   - GLFW 输入回调 → 内部状态
 *   - 动作映射（按键 → 游戏动作名称）
 *   - 通过 EventBus 发布输入事件
 *
 * 设计：
 *   - 轮询模式（每帧查询），不使用 GLFW 回调（避免回调中的线程问题）
 *   - 动作映射由 JSON 配置驱动（keybindings.json）
 *
 * 支持的动作类型：
 *   - "pressed": 按键刚按下（仅一帧）
 *   - "released": 按键刚释放（仅一帧）
 *   - "held": 按键持续按下
 *
 * 注意事项：
 *   - 每帧开始时调用 poll() 更新输入状态
 *   - 动作映射在引擎初始化时从配置文件加载
 */

#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <functional>

struct GLFWwindow;

namespace engine::input {

// 鼠标按钮
enum class MouseButton : uint8_t
{
    Left = 0,
    Right,
    Middle,
    Count
};

// 按键状态
enum class KeyState : uint8_t
{
    Released = 0,
    Pressed,
    Held,
    Repeat
};

// 输入事件
struct InputEvent
{
    std::string action;     // 动作名称（如 "move_forward", "interact"）
    KeyState    state;      // 按键状态
    float       value;      // 模拟值（0.0 ~ 1.0，用于手柄等）
};

class InputSystem
{
public:
    InputSystem();
    ~InputSystem();

    // 初始化（绑定到 GLFW 窗口）
    bool init(GLFWwindow* window);

    // 关闭
    void shutdown();

    // 每帧轮询（必须在游戏循环开始处调用）
    void poll();

    // 按键查询
    KeyState keyState(int keyCode) const;
    bool     isKeyPressed(int keyCode) const;
    bool     isKeyHeld(int keyCode) const;
    bool     isKeyReleased(int keyCode) const;

    // 鼠标查询
    KeyState mouseButtonState(MouseButton button) const;
    bool     isMousePressed(MouseButton button) const;

    // 鼠标位置
    double mouseX() const;
    double mouseY() const;
    double mouseDeltaX() const;
    double mouseDeltaY() const;
    double scrollDeltaX() const;
    double scrollDeltaY() const;

    // 动作映射
    void loadActionMappings(const std::string& configPath);
    std::vector<InputEvent> getActionEvents() const;

    // 光标模式
    void setCursorVisible(bool visible);
    bool isCursorVisible() const;

private:
    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace engine::input
