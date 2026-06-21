/**
 * @file engine/ui/widgets/box_layout.h
 * @brief 盒子布局 — 垂直/水平排列子控件
 *
 * 类似 CSS Flexbox 的简化版：
 *   - 方向：Vertical（垂直）或 Horizontal（水平）
 *   - 间距：子控件之间的像素间距
 *   - 内边距：容器边缘到子控件的距离
 */

#pragma once
#include "engine/ui/widget.h"

namespace engine::ui {

class BoxLayout : public Widget
{
public:
    BoxLayout(const std::string& id = "");

    void setDirection(LayoutDirection d) { m_direction = d; }
    void setSpacing(float s) { m_spacing = s; }
    void setPadding(float p) { m_padding = p; }

    void update(float dt) override;
    nlohmann::json toJson() const override;
    static std::unique_ptr<Widget> fromJson(const nlohmann::json& j);

private:
    void layoutChildren();

    LayoutDirection m_direction = LayoutDirection::Vertical;
    float m_spacing = 8.0f;
    float m_padding = 12.0f;
};

} // namespace engine::ui
