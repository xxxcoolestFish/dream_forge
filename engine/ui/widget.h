/**
 * @file engine/ui/widget.h
 * @brief UI 控件基类 — 控件树 + 布局 + 事件
 *
 * 设计：
 *   - 树形结构（parent / children）
 *   - 矩形区域（相对父控件）
 *   - 虚方法：update / render / onMouseDown / onMouseUp
 *   - 从 JSON 构造
 *
 * 使用方式：
 *   auto root = Widget::fromJson(json);
 *   root->update(dt);
 *   root->render(spriteRenderer);
 */

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <functional>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace engine::render { class SpriteRenderer; }

namespace engine::ui {

// 矩形（像素坐标）
struct Rect
{
    float x = 0, y = 0, w = 100, h = 100;

    bool contains(float px, float py) const
    {
        return px >= x && px <= x + w && py >= y && py <= y + h;
    }
};

// 颜色
struct Color
{
    float r = 1, g = 1, b = 1, a = 1;
    static Color fromHex(uint32_t hex);
};

// 布局方向
enum class LayoutDirection { Vertical, Horizontal };

// 控件基类
class Widget
{
public:
    Widget(const std::string& id = "");
    virtual ~Widget();

    // --- 树结构 ---
    Widget* parent() const { return m_parent; }
    void addChild(std::unique_ptr<Widget> child);
    const std::vector<std::unique_ptr<Widget>>& children() const { return m_children; }

    // --- 标识 ---
    const std::string& id() const { return m_id; }
    void setId(const std::string& id) { m_id = id; }

    // --- 布局 ---
    void  setRect(const Rect& rect) { m_rect = rect; }
    const Rect& rect() const { return m_rect; }
    Rect  absoluteRect() const;           // 屏幕绝对坐标
    virtual glm::vec2 preferredSize() const;

    // --- 可见性 ---
    void setVisible(bool v) { m_visible = v; }
    bool visible() const { return m_visible; }

    // --- 每帧更新 ---
    virtual void update(float dt);

    // --- 渲染 ---
    virtual void render(render::SpriteRenderer& renderer);

    // --- 鼠标事件 ---
    virtual Widget* hitTest(float x, float y);
    virtual bool onMouseDown(float x, float y);
    virtual bool onMouseUp(float x, float y);

    // --- JSON ---
    virtual nlohmann::json toJson() const;
    static std::unique_ptr<Widget> fromJson(const nlohmann::json& j);

    // --- 工厂注册 ---
    using WidgetFactory = std::function<std::unique_ptr<Widget>(const nlohmann::json&)>;
    static void registerType(const std::string& type, WidgetFactory factory);

protected:
    std::string m_id;
    Widget*     m_parent = nullptr;
    Rect        m_rect;
    bool        m_visible = true;
    std::vector<std::unique_ptr<Widget>> m_children;

    static std::unordered_map<std::string, WidgetFactory> s_factories;
};

} // namespace engine::ui
