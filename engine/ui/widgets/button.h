/**
 * @file engine/ui/widgets/button.h
 * @brief 按钮控件 — 可点击的矩形 + 文字标签
 */

#pragma once
#include "engine/ui/widget.h"

namespace engine::ui {

class Button : public Widget
{
public:
    Button(const std::string& id = "");

    void setText(const std::string& t) { m_text = t; }
    const std::string& text() const { return m_text; }

    void setColor(const Color& c) { m_color = c; }
    void setHoverColor(const Color& c) { m_hoverColor = c; }

    // 回调
    using ClickCallback = std::function<void()>;
    void setOnClick(ClickCallback cb) { m_onClick = std::move(cb); }

    void update(float dt) override;
    void render(render::SpriteRenderer& renderer) override;
    bool onMouseDown(float x, float y) override;
    nlohmann::json toJson() const override;
    static std::unique_ptr<Widget> fromJson(const nlohmann::json& j);

private:
    std::string m_text;
    Color m_color      = Color::fromHex(0x555555);
    Color m_hoverColor = Color::fromHex(0x777777);
    bool  m_hovered = false;
    ClickCallback m_onClick;
};

} // namespace engine::ui
