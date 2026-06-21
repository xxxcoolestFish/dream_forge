/**
 * @file engine/ui/widgets/box_layout.cpp
 * @brief BoxLayout 实现
 */

#include "engine/ui/widgets/box_layout.h"

namespace engine::ui {

BoxLayout::BoxLayout(const std::string& id) : Widget(id) {}

void BoxLayout::update(float dt)
{
    layoutChildren();
    Widget::update(dt);
}

void BoxLayout::layoutChildren()
{
    float offset = m_padding;
    for (auto& child : m_children)
    {
        if (!child->visible()) continue;

        glm::vec2 pref = child->preferredSize();

        Rect r;
        if (m_direction == LayoutDirection::Vertical)
        {
            r.x = m_padding;
            r.y = offset;
            r.w = m_rect.w - 2.0f * m_padding;
            r.h = pref.y > 0 ? pref.y : 30.0f; // 默认高度
        }
        else
        {
            r.x = offset;
            r.y = m_padding;
            r.w = pref.x > 0 ? pref.x : 80.0f;
            r.h = m_rect.h - 2.0f * m_padding;
        }

        child->setRect(r);
        offset += (m_direction == LayoutDirection::Vertical ? r.h : r.w) + m_spacing;
    }
}

nlohmann::json BoxLayout::toJson() const
{
    auto j = Widget::toJson();
    j["type"] = "BoxLayout";
    j["direction"] = (m_direction == LayoutDirection::Vertical) ? "vertical" : "horizontal";
    j["spacing"] = m_spacing;
    j["padding"] = m_padding;
    return j;
}

std::unique_ptr<Widget> BoxLayout::fromJson(const nlohmann::json& j)
{
    auto layout = std::make_unique<BoxLayout>();
    std::string dir = j.value("direction", "vertical");
    layout->m_direction = (dir == "horizontal")
        ? LayoutDirection::Horizontal : LayoutDirection::Vertical;
    layout->m_spacing = j.value("spacing", 8.0f);
    layout->m_padding = j.value("padding", 12.0f);
    return layout;
}

} // namespace engine::ui
