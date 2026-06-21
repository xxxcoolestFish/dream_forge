/**
 * @file engine/ui/widgets/text.cpp
 * @brief Text 控件实现
 */

#include "engine/ui/widgets/text.h"
#include "engine/render/gl_loader.h"
#include "engine/render/sprite.h"

#include <spdlog/spdlog.h>
#include <vector>

namespace engine::ui {
namespace gl = engine::render::gl;

// 带纹理的文字着色器
static const char* kTextVertex = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
uniform vec2 uScreenSize;
out vec2 vTexCoord;
void main()
{
    vec2 ndc;
    ndc.x = (aPos.x / uScreenSize.x) * 2.0 - 1.0;
    ndc.y = 1.0 - (aPos.y / uScreenSize.y) * 2.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

static const char* kTextFragment = R"(
#version 330 core
in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform vec4 uColor;
out vec4 FragColor;
void main()
{
    float alpha = texture(uTexture, vTexCoord).r;
    FragColor = vec4(uColor.rgb, uColor.a * alpha);
}
)";

Text::Text(const std::string& id) : Widget(id) {}

static std::shared_ptr<Font> s_defaultFont;

void Text::setDefaultFont(std::shared_ptr<Font> font) { s_defaultFont = std::move(font); }
std::shared_ptr<Font> Text::defaultFont() { return s_defaultFont; }

bool Text::initShader()
{
    if (m_shaderReady) return true;

    auto compile = [](uint32_t type, const char* src) -> uint32_t {
        uint32_t s = gl::CreateShader(type);
        gl::ShaderSource(s, 1, &src, nullptr);
        gl::CompileShader(s);
        int ok = 0; gl::GetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { gl::DeleteShader(s); return 0; }
        return s;
    };

    uint32_t vs = compile(GL_VERTEX_SHADER, kTextVertex);
    uint32_t fs = compile(GL_FRAGMENT_SHADER, kTextFragment);
    if (!vs || !fs) return false;

    m_textShader = gl::CreateProgram();
    gl::AttachShader(m_textShader, vs);
    gl::AttachShader(m_textShader, fs);
    gl::LinkProgram(m_textShader);
    gl::DeleteShader(vs);
    gl::DeleteShader(fs);

    m_uTexScreenSize = static_cast<uint32_t>(
        gl::GetUniformLocation(m_textShader, "uScreenSize"));

    gl::GenVertexArrays(1, &m_textVAO);
    gl::GenBuffers(1, &m_textVBO);

    m_shaderReady = true;
    return true;
}

glm::vec2 Text::preferredSize() const
{
    if (m_font && !m_text.empty())
        return { m_font->textWidth(m_text), m_font->pixelHeight() };
    return { static_cast<float>(m_text.size()) * 12.0f, 20.0f };
}

void Text::render(render::SpriteRenderer& renderer)
{
    if (!m_visible) return;
    if (!m_font) m_font = s_defaultFont;
    if (!m_font) return;
    renderText(renderer);
    Widget::render(renderer);
}

void Text::renderText(render::SpriteRenderer& /*renderer*/)
{
    if (!initShader()) return;

    Rect abs = absoluteRect();
    float penX = abs.x;
    float penY = abs.y + m_font->pixelHeight();

    gl::UseProgram(m_textShader);

    // 屏幕尺寸（从 absoluteRect 不能获取，使用固定值）
    // 实际应从 engine 获取，这里暂用常用分辨率
    float sw = 1280.0f, sh = 720.0f;
    gl::Uniform2f(static_cast<GLint>(m_uTexScreenSize), sw, sh);

    int colorLoc = gl::GetUniformLocation(m_textShader, "uColor");
    gl::Uniform4f(colorLoc, m_color.r, m_color.g, m_color.b, m_color.a);

    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(GL_TEXTURE_2D, m_font->textureId());

    gl::BindVertexArray(m_textVAO);

    for (char c : m_text)
    {
        const Glyph* g = m_font->glyph(c);
        if (!g) { penX += m_font->pixelHeight() * 0.5f; continue; }

        float x0 = penX + g->bearingX;
        float y0 = penY - g->bearingY;
        float x1 = x0 + g->w;
        float y1 = y0 + g->h;

        float vertices[] = {
            x0, y0, g->u0, g->v0,
            x1, y0, g->u1, g->v0,
            x1, y1, g->u1, g->v1,
            x0, y0, g->u0, g->v0,
            x1, y1, g->u1, g->v1,
            x0, y1, g->u0, g->v1,
        };

        gl::BindBuffer(GL_ARRAY_BUFFER, m_textVBO);
        gl::BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);

        gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
        gl::EnableVertexAttribArray(0);
        gl::VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
        gl::EnableVertexAttribArray(1);

        gl::DrawArrays(GL_TRIANGLES, 0, 6);

        penX += g->advanceX;
    }

    gl::BindVertexArray(0);
    gl::UseProgram(0);
}

nlohmann::json Text::toJson() const
{
    auto j = Widget::toJson();
    j["type"] = "Text";
    j["text"] = m_text;
    j["color"] = {
        static_cast<int>(m_color.r * 255),
        static_cast<int>(m_color.g * 255),
        static_cast<int>(m_color.b * 255)
    };
    return j;
}

std::unique_ptr<Widget> Text::fromJson(const nlohmann::json& j)
{
    auto t = std::make_unique<Text>();
    t->setText(j.value("text", ""));
    if (j.contains("color") && j["color"].is_array())
    {
        t->m_color = {
            j["color"][0].get<float>() / 255.0f,
            j["color"][1].get<float>() / 255.0f,
            j["color"][2].get<float>() / 255.0f,
            1.0f
        };
    }
    return t;
}

} // namespace engine::ui
