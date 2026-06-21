/**
 * @file engine/ui/widgets/panel.h
 * @brief 面板控件 — 纯色矩形背景
 */

#pragma once
#include "engine/ui/widget.h"

namespace engine::ui {

class Panel : public Widget
{
public:
    Panel(const std::string& id = "");

    void setColor(const Color& c) { m_color = c; }
    const Color& color() const { return m_color; }

    void render(render::SpriteRenderer& renderer) override;
    nlohmann::json toJson() const override;
    static std::unique_ptr<Widget> fromJson(const nlohmann::json& j);

private:
    Color m_color = Color::fromHex(0x333333);
};

} // namespace engine::ui
