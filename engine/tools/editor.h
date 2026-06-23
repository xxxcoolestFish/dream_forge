/**
 * @file engine/tools/editor.h
 * @brief 引擎内编辑器 — Phase 6.8
 *
 * F1 切换编辑模式。提供实体列表 + 属性检视器。
 * 使用 Dear ImGui 即时模式 UI。
 */

#pragma once

#include <memory>

struct GLFWwindow;

namespace engine::ecs { class World; }
namespace engine::input { class InputSystem; }

namespace engine::tools {

class Editor
{
public:
    Editor(ecs::World* world, input::InputSystem* input);
    ~Editor();

    // 禁止拷贝
    Editor(const Editor&) = delete;
    Editor& operator=(const Editor&) = delete;

    bool init(GLFWwindow* window);
    void shutdown();

    void update(float dt);
    void render();

    bool isActive() const { return m_active; }
    void setActive(bool a);

    // 编辑器消耗输入 → 游戏不应响应
    bool consumesInput() const { return m_active; }

private:
    ecs::World*          m_world = nullptr;
    input::InputSystem*  m_input = nullptr;
    bool                 m_active = false;
    bool                 m_initialized = false;

    uint32_t m_selectedEntity = 0; // entt::entity as uint32_t

    void drawEntityList();
    void drawInspector();
};

} // namespace engine::tools
