/**
 * @file engine/ui/widgets/flex_layout.cpp
 * @brief FlexLayout 实现
 */

#include "engine/ui/widgets/flex_layout.h"
#include <algorithm>
#include <numeric>

namespace engine::ui {

FlexLayout::FlexLayout(const std::string& id) : Widget(id) {}

void FlexLayout::setChildGrow(Widget& child, float grow)
{
    // 通过 id 存储在 map 中
    static std::unordered_map<Widget*, float>* s_growMap = nullptr;
    if (!s_growMap) s_growMap = new std::unordered_map<Widget*, float>();
    (*s_growMap)[&child] = grow;
}

float FlexLayout::childGrow(const Widget& child) const
{
    // 从静态 map 中查找
    static std::unordered_map<Widget*, float>* s_growMap = nullptr;
    if (!s_growMap) return 1.0f;
    auto it = s_growMap->find(const_cast<Widget*>(&child));
    return (it != s_growMap->end()) ? it->second : 1.0f;
}

void FlexLayout::update(float dt)
{
    layoutChildren();
    Widget::update(dt);
}

glm::vec2 FlexLayout::preferredSize() const
{
    float total = 0;
    int count = 0;
    for (auto& child : m_children)
    {
        if (!child->visible()) continue;
        glm::vec2 pref = child->preferredSize();
        total += (m_direction == LayoutDirection::Vertical) ? pref.y : pref.x;
        count++;
    }
    float gaps = m_gap * static_cast<float>(std::max(0, count - 1));
    float size = total + gaps + 2.0f * m_padding;
    return (m_direction == LayoutDirection::Vertical)
        ? glm::vec2(m_rect.w, size) : glm::vec2(size, m_rect.h);
}

void FlexLayout::layoutChildren()
{
    if (m_children.empty()) return;

    // 收集可见子控件
    std::vector<Widget*> visible;
    float totalGrow = 0;
    float totalPref = 0;
    for (auto& child : m_children)
    {
        if (!child->visible()) continue;
        visible.push_back(child.get());
        totalGrow += childGrow(*child);
        glm::vec2 pref = child->preferredSize();
        totalPref += (m_direction == LayoutDirection::Vertical) ? pref.y : pref.x;
    }
    if (visible.empty()) return;

    bool isVertical = (m_direction == LayoutDirection::Vertical);
    float containerSize = (isVertical ? m_rect.h : m_rect.w) - 2.0f * m_padding;
    float totalGap = m_gap * static_cast<float>(visible.size() - 1);

    // 分配空间
    float extraSpace = containerSize - totalPref - totalGap;
    if (extraSpace < 0) extraSpace = 0;

    // 计算起始偏移（justify-content）
    float offset = m_padding;
    if (m_justify == JustifyContent::Center)
        offset += extraSpace * 0.5f;
    else if (m_justify == JustifyContent::End)
        offset += extraSpace;

    float spaceBetweenExtra = 0;
    if (m_justify == JustifyContent::SpaceBetween && visible.size() > 1)
        spaceBetweenExtra = extraSpace / static_cast<float>(visible.size() - 1);
    else if (m_justify == JustifyContent::SpaceAround && visible.size() > 0)
        spaceBetweenExtra = extraSpace / static_cast<float>(visible.size() + 1);

    if (m_justify == JustifyContent::SpaceAround)
        offset += spaceBetweenExtra;

    for (auto* child : visible)
    {
        glm::vec2 pref = child->preferredSize();
        float grow = childGrow(*child);
        float itemSize = (isVertical ? pref.y : pref.x);
        // flex-grow 分配额外空间
        if (totalGrow > 0)
            itemSize += extraSpace * grow / totalGrow;

        Rect r;
        if (isVertical)
        {
            r.x = m_padding;
            r.y = offset;
            r.w = m_rect.w - 2.0f * m_padding;
            r.h = std::max(10.0f, itemSize);
        }
        else
        {
            r.x = offset;
            r.y = m_padding;
            r.w = std::max(10.0f, itemSize);
            r.h = m_rect.h - 2.0f * m_padding;
        }
        child->setRect(r);
        offset += (isVertical ? r.h : r.w) + m_gap + spaceBetweenExtra;
    }
}

nlohmann::json FlexLayout::toJson() const
{
    auto j = Widget::toJson();
    j["type"] = "FlexLayout";
    j["direction"] = (m_direction == LayoutDirection::Vertical) ? "vertical" : "horizontal";
    j["gap"]     = m_gap;
    j["padding"] = m_padding;
    switch (m_justify) {
        case JustifyContent::Center: j["justify"] = "center"; break;
        case JustifyContent::End: j["justify"] = "end"; break;
        case JustifyContent::SpaceBetween: j["justify"] = "space-between"; break;
        case JustifyContent::SpaceAround: j["justify"] = "space-around"; break;
        default: j["justify"] = "start"; break;
    }
    switch (m_align) {
        case AlignItems::Center: j["align"] = "center"; break;
        case AlignItems::End: j["align"] = "end"; break;
        default: j["align"] = "stretch"; break;
    }
    return j;
}

std::unique_ptr<Widget> FlexLayout::fromJson(const nlohmann::json& j)
{
    auto layout = std::make_unique<FlexLayout>();

    std::string dir = j.value("direction", "vertical");
    layout->m_direction = (dir == "horizontal")
        ? LayoutDirection::Horizontal : LayoutDirection::Vertical;

    std::string justify = j.value("justify", "start");
    if (justify == "center") layout->m_justify = JustifyContent::Center;
    else if (justify == "end") layout->m_justify = JustifyContent::End;
    else if (justify == "space-between") layout->m_justify = JustifyContent::SpaceBetween;
    else if (justify == "space-around") layout->m_justify = JustifyContent::SpaceAround;

    std::string align = j.value("align", "stretch");
    if (align == "center") layout->m_align = AlignItems::Center;
    else if (align == "end") layout->m_align = AlignItems::End;

    layout->m_gap     = j.value("gap", 8.0f);
    layout->m_padding = j.value("padding", 12.0f);

    // 递归加载子控件，支持 flex-grow
    if (j.contains("children"))
    {
        for (auto& cj : j["children"])
        {
            auto child = Widget::fromJson(cj);
            if (child && cj.contains("grow"))
                FlexLayout::setChildGrow(*child, cj.value("grow", 1.0f));
            if (child)
                layout->addChild(std::move(child));
        }
    }

    return layout;
}

} // namespace engine::ui
