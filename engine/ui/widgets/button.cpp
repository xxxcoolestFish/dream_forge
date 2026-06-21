/**
 * @file engine/ui/widgets/button.cpp
 * @brief Button 实现
 */

#include "engine/ui/widgets/button.h"
#include "engine/render/sprite.h"

#include <spdlog/spdlog.h>

namespace engine::ui {
using engine::render::SpriteDesc;

Button::Button(const std::string& id) : Widget(id) {}

void Button::update(float dt)
{
    (void)dt;
    Widget::update(dt);
}

void Button::render(render::SpriteRenderer& renderer)
{
    if (!m_visible) return;

    Rect abs = absoluteRect();
    SpriteDesc desc;
    desc.position = { abs.x, abs.y };
    desc.size     = { abs.w, abs.h };
    Color c = m_hovered ? m_hoverColor : m_color;
    desc.tint = { c.r, c.g, c.b, c.a };
    renderer.submit(desc);

    // 文字暂时用日志输出（SDF 字体后续实现）
    Widget::render(renderer);
}

bool Button::onMouseDown(float x, float y)
{
    Rect abs = absoluteRect();
    if (abs.contains(x, y))
    {
        spdlog::info("Button '{}' clicked: \"{}\"", m_id, m_text);
        if (m_onClick) m_onClick();
        return true;
    }
    return false;
}

nlohmann::json Button::toJson() const
{
    auto j = Widget::toJson();
    j["type"] = "Button";
    j["text"] = m_text;
    j["color"] = {
        static_cast<int>(m_color.r * 255),
        static_cast<int>(m_color.g * 255),
        static_cast<int>(m_color.b * 255)
    };
    return j;
}

std::unique_ptr<Widget> Button::fromJson(const nlohmann::json& j)
{
    auto btn = std::make_unique<Button>();
    btn->setText(j.value("text", "Button"));
    if (j.contains("color") && j["color"].is_array())
    {
        btn->m_color = {
            j["color"][0].get<float>() / 255.0f,
            j["color"][1].get<float>() / 255.0f,
            j["color"][2].get<float>() / 255.0f,
            1.0f
        };
    }
    return btn;
}

} // namespace engine::ui
