/**
 * @file engine/input/input_system.cpp
 * @brief 输入系统 — GLFW轮询 + 按键状态机 + 动作映射
 */

#include "engine/input/input_system.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <cstring>
#include <algorithm>

namespace engine::input {

// =========================================================================
// 内部实现
// =========================================================================
struct InputSystem::Impl
{
    GLFWwindow* window      = nullptr;
    bool        initialized = false;

    // 键盘状态：两帧缓冲（用于判断 Pressed/Released）
    static constexpr int kMaxKeys = 512;
    uint8_t keysCurrent[kMaxKeys]  = {};
    uint8_t keysPrevious[kMaxKeys] = {};

    // 鼠标状态
    static constexpr int kMaxMouse = 8;
    uint8_t mouseCurrent[kMaxMouse]  = {};
    uint8_t mousePrevious[kMaxMouse] = {};

    double mouseX        = 0.0;
    double mouseY        = 0.0;
    double prevMouseX    = 0.0;
    double prevMouseY    = 0.0;
    double scrollX       = 0.0;
    double scrollY       = 0.0;
    double prevScrollX   = 0.0;
    double prevScrollY   = 0.0;

    bool cursorVisible = true;

    // 动作映射
    struct ActionMapping
    {
        std::string action;
        int         key;
    };
    std::vector<ActionMapping> mappings;

    // 从 GLFW key code 获取状态
    KeyState computeKeyState(int key) const
    {
        if (key < 0 || key >= kMaxKeys) return KeyState::Released;

        bool prev = (keysPrevious[key] == GLFW_PRESS);
        bool curr = (keysCurrent[key] == GLFW_PRESS);

        if (!prev && curr)  return KeyState::Pressed;
        if (prev && curr)   return KeyState::Held;
        if (prev && !curr)  return KeyState::Released;
        return KeyState::Released;
    }

    KeyState computeMouseState(int button) const
    {
        if (button < 0 || button >= kMaxMouse) return KeyState::Released;

        bool prev = (mousePrevious[button] == GLFW_PRESS);
        bool curr = (mouseCurrent[button] == GLFW_PRESS);

        if (!prev && curr)  return KeyState::Pressed;
        if (prev && curr)   return KeyState::Held;
        if (prev && !curr)  return KeyState::Released;
        return KeyState::Released;
    }
};

// =========================================================================
// 构造 / 析构
// =========================================================================
InputSystem::InputSystem()
    : m_impl(new Impl())
{
    spdlog::debug("InputSystem created");
}

InputSystem::~InputSystem()
{
    if (m_impl->initialized)
    {
        shutdown();
    }
    delete m_impl;
}

// =========================================================================
// 初始化
// =========================================================================
bool InputSystem::init(GLFWwindow* window)
{
    if (!window)
    {
        spdlog::error("InputSystem::init: null window handle");
        return false;
    }

    m_impl->window = window;

    // 初始化鼠标位置
    glfwGetCursorPos(window, &m_impl->mouseX, &m_impl->mouseY);
    m_impl->prevMouseX = m_impl->mouseX;
    m_impl->prevMouseY = m_impl->mouseY;

    // 默认动作映射（硬编码，后续从JSON加载）
    m_impl->mappings.push_back({ "move_up",    GLFW_KEY_W });
    m_impl->mappings.push_back({ "move_down",  GLFW_KEY_S });
    m_impl->mappings.push_back({ "move_left",  GLFW_KEY_A });
    m_impl->mappings.push_back({ "move_right", GLFW_KEY_D });
    m_impl->mappings.push_back({ "interact",   GLFW_KEY_E });
    m_impl->mappings.push_back({ "quit",       GLFW_KEY_ESCAPE });

    m_impl->initialized = true;
    spdlog::info("InputSystem initialized ({} action mappings).", m_impl->mappings.size());
    return true;
}

void InputSystem::shutdown()
{
    m_impl->initialized = false;
    spdlog::info("InputSystem shutdown.");
}

// =========================================================================
// 每帧轮询
// =========================================================================
void InputSystem::poll()
{
    if (!m_impl->initialized) return;

    auto& impl = *m_impl;
    GLFWwindow* w = impl.window;

    // --- 键盘：拷贝上一帧状态 → 读取当前帧 ---
    std::memcpy(impl.keysPrevious, impl.keysCurrent, sizeof(impl.keysPrevious));
    for (int i = 0; i < Impl::kMaxKeys; ++i)
    {
        impl.keysCurrent[i] = static_cast<uint8_t>(glfwGetKey(w, i));
    }

    // --- 鼠标按键 ---
    std::memcpy(impl.mousePrevious, impl.mouseCurrent, sizeof(impl.mousePrevious));
    for (int i = 0; i < Impl::kMaxMouse; ++i)
    {
        impl.mouseCurrent[i] = static_cast<uint8_t>(glfwGetMouseButton(w, i));
    }

    // --- 鼠标位置 ---
    impl.prevMouseX = impl.mouseX;
    impl.prevMouseY = impl.mouseY;
    glfwGetCursorPos(w, &impl.mouseX, &impl.mouseY);

    // --- 滚轮 ---
    impl.prevScrollX = impl.scrollX;
    impl.prevScrollY = impl.scrollY;
    // 滚轮由回调设置（当前不实现回调版本，直接清零）
    impl.scrollX = 0.0;
    impl.scrollY = 0.0;
}

// =========================================================================
// 按键查询
// =========================================================================
KeyState InputSystem::keyState(int keyCode) const
{
    return m_impl->computeKeyState(keyCode);
}

bool InputSystem::isKeyPressed(int keyCode) const
{
    return keyState(keyCode) == KeyState::Pressed;
}

bool InputSystem::isKeyHeld(int keyCode) const
{
    KeyState s = keyState(keyCode);
    return s == KeyState::Held || s == KeyState::Pressed;
}

bool InputSystem::isKeyReleased(int keyCode) const
{
    return keyState(keyCode) == KeyState::Released;
}

// =========================================================================
// 鼠标查询
// =========================================================================
KeyState InputSystem::mouseButtonState(MouseButton button) const
{
    return m_impl->computeMouseState(static_cast<int>(button));
}

bool InputSystem::isMousePressed(MouseButton button) const
{
    return mouseButtonState(button) == KeyState::Pressed;
}

double InputSystem::mouseX() const       { return m_impl->mouseX; }
double InputSystem::mouseY() const       { return m_impl->mouseY; }
double InputSystem::mouseDeltaX() const  { return m_impl->mouseX - m_impl->prevMouseX; }
double InputSystem::mouseDeltaY() const  { return m_impl->mouseY - m_impl->prevMouseY; }
double InputSystem::scrollDeltaX() const { return m_impl->scrollX; }
double InputSystem::scrollDeltaY() const { return m_impl->scrollY; }

// =========================================================================
// 光标
// =========================================================================
void InputSystem::setCursorVisible(bool visible)
{
    m_impl->cursorVisible = visible;
    if (m_impl->window)
    {
        glfwSetInputMode(m_impl->window, GLFW_CURSOR,
            visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_DISABLED);
    }
}

bool InputSystem::isCursorVisible() const
{
    return m_impl->cursorVisible;
}

// =========================================================================
// 动作映射
// =========================================================================
void InputSystem::loadActionMappings(const std::string& configPath)
{
    // TODO: 从 JSON 文件加载按键绑定
    spdlog::debug("InputSystem::loadActionMappings: {} (using built-in defaults)", configPath);
}

std::vector<InputEvent> InputSystem::getActionEvents() const
{
    std::vector<InputEvent> events;

    for (const auto& mapping : m_impl->mappings)
    {
        KeyState state = keyState(mapping.key);

        // 只报告有意义的动作（Pressed / Released）
        if (state == KeyState::Pressed || state == KeyState::Released)
        {
            InputEvent ev;
            ev.action = mapping.action;
            ev.state  = state;
            ev.value  = (state == KeyState::Pressed) ? 1.0f : 0.0f;
            events.push_back(ev);
        }
        else if (state == KeyState::Held)
        {
            InputEvent ev;
            ev.action = mapping.action;
            ev.state  = KeyState::Held;
            ev.value  = 1.0f;
            events.push_back(ev);
        }
    }

    return events;
}

} // namespace engine::input
