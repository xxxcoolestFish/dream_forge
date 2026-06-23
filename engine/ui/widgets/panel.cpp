/**
 * @file engine/ui/widgets/panel.cpp
 * @brief Panel 实现
 */

#include "engine/ui/widgets/panel.h"
#include "engine/render/sprite.h"
#include <spdlog/spdlog.h>

namespace engine::ui {
using engine::render::SpriteDesc;

Panel::Panel(const std::string& id) : Widget(id) {}

void Panel::render(render::SpriteRenderer& renderer)
{
    if (!m_visible) return;

    Rect abs = absoluteRect();
    SpriteDesc desc;
    desc.position = glm::vec3(abs.x, abs.y, 0.0f);
    desc.size     = { abs.w, abs.h };
    desc.tint     = { m_color.r, m_color.g, m_color.b, m_color.a };
    renderer.submit(desc);

    static bool once = false;
    if (!once) {
        spdlog::info("Panel '{}': abs=({:.0f},{:.0f},{:.0f},{:.0f}) color=({:.2f},{:.2f},{:.2f})",
            m_id, abs.x, abs.y, abs.w, abs.h, m_color.r, m_color.g, m_color.b);
        once = true;
    }

    Widget::render(renderer);
}

nlohmann::json Panel::toJson() const
{
    auto j = Widget::toJson();
    j["type"] = "Panel";
    j["color"] = {
        static_cast<int>(m_color.r * 255),
        static_cast<int>(m_color.g * 255),
        static_cast<int>(m_color.b * 255),
        static_cast<int>(m_color.a * 255)
    };
    return j;
}

std::unique_ptr<Widget> Panel::fromJson(const nlohmann::json& j)
{
    auto p = std::make_unique<Panel>();
    if (j.contains("color") && j["color"].is_array())
    {
        p->m_color = {
            j["color"][0].get<float>() / 255.0f,
            j["color"][1].get<float>() / 255.0f,
            j["color"][2].get<float>() / 255.0f,
            (j["color"].size() > 3 ? j["color"][3].get<float>() / 255.0f : 1.0f)
        };
    }
    return p;
}

} // namespace engine::ui
