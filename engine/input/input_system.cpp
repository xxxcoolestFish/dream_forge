/**
 * @file engine/input/input_system.cpp
 * @brief 输入系统实现（当前为桩，Phase 1 Step 4 完善）
 */

#include "engine/input/input_system.h"
#include <spdlog/spdlog.h>

namespace engine::input {

struct InputSystem::Impl
{
    GLFWwindow* window = nullptr;
    bool        initialized = false;

    // 当前帧和上一帧的键盘状态
    uint8_t keysCurrent[512] = {};
    uint8_t keysPrevious[512] = {};

    // 鼠标状态
    double mouseX       = 0.0;
    double mouseY       = 0.0;
    double prevMouseX   = 0.0;
    double prevMouseY   = 0.0;
    double scrollX      = 0.0;
    double scrollY      = 0.0;
    bool   cursorVisible = true;

    // 动作映射
    struct ActionMapping
    {
        std::string actionName;
        int         keyCode;
    };
    std::vector<ActionMapping> mappings;
};

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

bool InputSystem::init(GLFWwindow* window)
{
    if (!window)
    {
        spdlog::error("InputSystem::init: null window handle");
        return false;
    }
    m_impl->window = window;
    m_impl->initialized = true;
    spdlog::info("InputSystem initialized.");
    return true;
}

void InputSystem::shutdown()
{
    m_impl->initialized = false;
    spdlog::info("InputSystem shutdown.");
}

void InputSystem::poll()
{
    // TODO Step 4: 从 GLFW 获取输入状态
    // 拷贝上一帧状态
    // std::memcpy(m_impl->keysPrevious, m_impl->keysCurrent, sizeof(m_impl->keysPrevious));
    // 更新当前帧状态
    // glfwGetKey / glfwGetMouseButton 等
}

KeyState InputSystem::keyState(int keyCode) const
{
    // TODO Step 4: 根据 current/previous 判断 Pressed/Held/Released
    (void)keyCode;
    return KeyState::Released;
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

KeyState InputSystem::mouseButtonState(MouseButton button) const
{
    (void)button;
    return KeyState::Released;
}

bool InputSystem::isMousePressed(MouseButton button) const
{
    return mouseButtonState(button) == KeyState::Pressed;
}

double InputSystem::mouseX() const           { return m_impl->mouseX; }
double InputSystem::mouseY() const           { return m_impl->mouseY; }
double InputSystem::mouseDeltaX() const      { return m_impl->mouseX - m_impl->prevMouseX; }
double InputSystem::mouseDeltaY() const      { return m_impl->mouseY - m_impl->prevMouseY; }
double InputSystem::scrollDeltaX() const     { return m_impl->scrollX; }
double InputSystem::scrollDeltaY() const     { return m_impl->scrollY; }

void InputSystem::loadActionMappings(const std::string& configPath)
{
    // TODO Step 4: 从 JSON 加载动作映射配置
    spdlog::debug("InputSystem::loadActionMappings: {}", configPath);
}

std::vector<InputEvent> InputSystem::getActionEvents() const
{
    // TODO Step 4: 根据当前输入状态 + 动作映射生成事件
    return {};
}

void InputSystem::setCursorVisible(bool visible)
{
    m_impl->cursorVisible = visible;
    // TODO Step 4: glfwSetInputMode(..., GLFW_CURSOR, ...)
}

bool InputSystem::isCursorVisible() const
{
    return m_impl->cursorVisible;
}

} // namespace engine::input
