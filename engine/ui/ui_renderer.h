/**
 * @file engine/ui/ui_renderer.h
 * @brief UI 渲染器 — 管理控件树 + 路由鼠标事件
 *
 * 负责：
 *   - 加载 JSON UI 定义
 *   - 每帧 update + render
 *   - 鼠标事件分发（hitTest + onMouseDown/Up）
 */

#pragma once

#include "engine/ui/font.h"
#include "engine/ui/data_binding.h"

#include <memory>
#include <string>
#include <nlohmann/json.hpp>

struct GLFWwindow;
namespace engine::render { class SpriteRenderer; }
namespace engine::ui      { class Widget; }

namespace engine::ui {

class UIRenderer
{
public:
    UIRenderer();
    ~UIRenderer();

    // 从 JSON 字符串加载 UI
    bool loadFromString(const std::string& jsonStr);

    // 从 JSON 文件加载 UI
    bool loadFromFile(const std::string& path);

    // 加载字体
    bool loadFont(const std::string& ttfPath, float pixelHeight = 24.0f);

    // 设置 ECS World（用于数据绑定）
    void setEcsWorld(engine::ecs::World* world);

    // 每帧更新
    void update(float dt);

    // 渲染（在场景渲染之后、swap 之前调用）
    void render(render::SpriteRenderer& spriteRenderer,
                uint32_t screenWidth, uint32_t screenHeight);

    // 鼠标事件
    void onMouseDown(float x, float y);
    void onMouseUp(float x, float y);

    // 获取根控件
    Widget* root() const { return m_root.get(); }

private:
    void renderBackgroundWidgets(Widget* w, render::SpriteRenderer& sr);
    void renderTextWidgets(Widget* w, render::SpriteRenderer& sr);
    void resolveBindings(Widget* w);

    std::unique_ptr<Widget> m_root;
    std::shared_ptr<Font>   m_font;
    BindingContext          m_bindingContext;
};

} // namespace engine::ui
