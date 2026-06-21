/**
 * @file engine/ui/widgets/text.h
 * @brief 文本控件 — 使用 Font 渲染字符串
 */

#pragma once
#include "engine/ui/widget.h"
#include "engine/ui/font.h"
#include "engine/ui/data_binding.h"
#include <memory>

namespace engine::ui {

class Text : public Widget
{
public:
    Text(const std::string& id = "");

    void setText(const std::string& t) { m_text = t; m_originalText = t; }
    const std::string& text() const { return m_text; }

    // 数据绑定
    void setBinding(const Binding& b) { m_binding = b; m_hasBinding = true; }
    void resolveBinding(const class BindingContext& ctx);

    void setColor(const Color& c) { m_color = c; }
    void setFont(std::shared_ptr<Font> font) { m_font = std::move(font); }

    // 全局默认字体（UIRenderer 加载后设置）
    static void setDefaultFont(std::shared_ptr<Font> font);
    static std::shared_ptr<Font> defaultFont();

    glm::vec2 preferredSize() const override;
    void update(float dt) override;
    void render(render::SpriteRenderer& renderer) override;

    nlohmann::json toJson() const override;
    static std::unique_ptr<Widget> fromJson(const nlohmann::json& j);

private:
    std::string   m_text;
    std::string   m_originalText;  // 原始文本（绑定前）
    Color   m_color { 1, 1, 1, 1 };
    Binding m_binding;
    bool    m_hasBinding = false;
    std::shared_ptr<Font> m_font;

    // 文字渲染着色器（内联编译）
    uint32_t m_textShader  = 0;
    uint32_t m_textVAO     = 0;
    uint32_t m_textVBO     = 0;
    uint32_t m_uTexScreenSize = 0;
    bool     m_shaderReady = false;

    bool initShader();
    void renderText(render::SpriteRenderer& renderer);
};

} // namespace engine::ui
