/**
 * @file engine/ui/widget.cpp
 * @brief Widget 基类实现
 */

#include "engine/ui/widget.h"
#include "engine/render/sprite.h"

#include <spdlog/spdlog.h>
#include <algorithm>

namespace engine::ui {

// 静态工厂注册表
std::unordered_map<std::string, Widget::WidgetFactory> Widget::s_factories;

// =========================================================================
// 构造 / 析构
// =========================================================================
Widget::Widget(const std::string& id) : m_id(id) {}
Widget::~Widget() = default;

// =========================================================================
// 树结构
// =========================================================================
void Widget::addChild(std::unique_ptr<Widget> child)
{
    child->m_parent = this;
    m_children.push_back(std::move(child));
}

// =========================================================================
// 布局
// =========================================================================
Rect Widget::absoluteRect() const
{
    Rect abs = m_rect;
    const Widget* p = m_parent;
    while (p)
    {
        abs.x += p->m_rect.x;
        abs.y += p->m_rect.y;
        p = p->m_parent;
    }
    return abs;
}

glm::vec2 Widget::preferredSize() const
{
    return { m_rect.w, m_rect.h };
}

// =========================================================================
// 更新 / 渲染
// =========================================================================
void Widget::update(float dt)
{
    for (auto& child : m_children)
        child->update(dt);
}

void Widget::render(render::SpriteRenderer& renderer)
{
    if (!m_visible) return;
    for (auto& child : m_children)
        child->render(renderer);
}

// =========================================================================
// 鼠标事件
// =========================================================================
Widget* Widget::hitTest(float x, float y)
{
    if (!m_visible) return nullptr;

    Rect abs = absoluteRect();
    if (!abs.contains(x, y)) return nullptr;

    // 后添加的 child 在上层（倒序遍历）
    for (auto it = m_children.rbegin(); it != m_children.rend(); ++it)
    {
        Widget* hit = (*it)->hitTest(x, y);
        if (hit) return hit;
    }
    return this;
}

bool Widget::onMouseDown(float x, float y) { return false; }
bool Widget::onMouseUp(float x, float y)   { return false; }

// =========================================================================
// JSON
// =========================================================================
nlohmann::json Widget::toJson() const
{
    nlohmann::json j;
    j["type"] = "Widget";
    j["id"]   = m_id;
    j["rect"] = { m_rect.x, m_rect.y, m_rect.w, m_rect.h };
    j["visible"] = m_visible;

    if (!m_children.empty())
    {
        nlohmann::json kids = nlohmann::json::array();
        for (auto& c : m_children) kids.push_back(c->toJson());
        j["children"] = kids;
    }
    return j;
}

std::unique_ptr<Widget> Widget::fromJson(const nlohmann::json& j)
{
    std::string type = j.value("type", "Widget");

    // 查工厂表
    auto it = s_factories.find(type);
    if (it != s_factories.end())
    {
        auto w = it->second(j);
        if (w)
        {
            w->m_id = j.value("id", "");
            if (j.contains("rect"))
            {
                w->m_rect.x = j["rect"][0].get<float>();
                w->m_rect.y = j["rect"][1].get<float>();
                w->m_rect.w = j["rect"][2].get<float>();
                w->m_rect.h = j["rect"][3].get<float>();
            }
            w->m_visible = j.value("visible", true);

            if (j.contains("children"))
                for (auto& cj : j["children"])
                    w->addChild(Widget::fromJson(cj));

            return w;
        }
    }

    spdlog::warn("Widget::fromJson: unknown type '{}'", type);
    return std::make_unique<Widget>(j.value("id", "unknown"));
}

void Widget::registerType(const std::string& type, WidgetFactory factory)
{
    s_factories[type] = std::move(factory);
}

// =========================================================================
// Color
// =========================================================================
Color Color::fromHex(uint32_t hex)
{
    return {
        ((hex >> 24) & 0xFF) / 255.0f,
        ((hex >> 16) & 0xFF) / 255.0f,
        ((hex >> 8)  & 0xFF) / 255.0f,
        (hex & 0xFF) / 255.0f
    };
}

} // namespace engine::ui
