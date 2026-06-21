/**
 * @file engine/ui/widgets/flex_layout.h
 * @brief CSS Flexbox 布局 — 替代 BoxLayout
 *
 * 支持属性：
 *   - flex-direction: row | column
 *   - justify-content: start | center | end | space-between | space-around
 *   - align-items: start | center | end | stretch
 *   - gap: 子控件间距
 *   - padding: 内边距
 *   - 每个子控件可设置 flex-grow（JSON: "grow": 1）
 */

#pragma once
#include "engine/ui/widget.h"

namespace engine::ui {

enum class JustifyContent { Start, Center, End, SpaceBetween, SpaceAround };
enum class AlignItems      { Start, Center, End, Stretch };

class FlexLayout : public Widget
{
public:
    FlexLayout(const std::string& id = "");

    void setDirection(LayoutDirection d) { m_direction = d; }
    void setJustify(JustifyContent j)    { m_justify = j; }
    void setAlign(AlignItems a)          { m_align = a; }
    void setGap(float g)                 { m_gap = g; }
    void setPadding(float p)             { m_padding = p; }

    // 设置子控件的 flex-grow（由 JSON 加载时调用）
    static void setChildGrow(Widget& child, float grow);

    void update(float dt) override;
    glm::vec2 preferredSize() const override;
    nlohmann::json toJson() const override;
    static std::unique_ptr<Widget> fromJson(const nlohmann::json& j);

private:
    void layoutChildren();
    float childGrow(const Widget& child) const;

    LayoutDirection m_direction = LayoutDirection::Vertical;
    JustifyContent  m_justify   = JustifyContent::Start;
    AlignItems      m_align     = AlignItems::Stretch;
    float m_gap     = 8.0f;
    float m_padding = 12.0f;

    // 存储 flex-grow 值（不在 Widget 基类中，避免污染接口）
    std::unordered_map<std::string, float> m_growMap;
};

} // namespace engine::ui
