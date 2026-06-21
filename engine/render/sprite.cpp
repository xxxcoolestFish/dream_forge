/**
 * @file engine/render/sprite.cpp
 * @brief 2D 精灵渲染器实现 — ECS 批量 + 独立提交
 */

#include "engine/render/sprite.h"
#include "engine/render/gl_loader.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"

#include <spdlog/spdlog.h>
#include <vector>

namespace engine::render {

// =========================================================================
// 内嵌着色器
// =========================================================================

static const char* kVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform vec2 uScreenSize;
uniform vec2 uOffset;
uniform vec2 uSize;
void main()
{
    vec2 ndc;
    ndc.x = ((aPos.x * uSize.x + uOffset.x) / uScreenSize.x) * 2.0 - 1.0;
    ndc.y = 1.0 - ((aPos.y * uSize.y + uOffset.y) / uScreenSize.y) * 2.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)";

static const char* kFragmentShader = R"(
#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main()
{
    FragColor = uColor;
}
)";

// =========================================================================
// 构造 / 析构
// =========================================================================
SpriteRenderer::SpriteRenderer()
{
    spdlog::debug("SpriteRenderer created");
}

SpriteRenderer::~SpriteRenderer()
{
    if (m_initialized) shutdown();
}

// =========================================================================
// 初始化
// =========================================================================
bool SpriteRenderer::init()
{
    spdlog::info("SpriteRenderer initializing...");

    auto compileShader = [](uint32_t type, const char* src) -> uint32_t
    {
        uint32_t s = gl::CreateShader(type);
        gl::ShaderSource(s, 1, &src, nullptr);
        gl::CompileShader(s);
        int ok = 0;
        gl::GetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            int len = 0;
            gl::GetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
            std::vector<char> log(static_cast<size_t>(len) + 1);
            gl::GetShaderInfoLog(s, len, nullptr, log.data());
            spdlog::error("SpriteRenderer: shader compile error: {}", log.data());
            gl::DeleteShader(s);
            return 0;
        }
        return s;
    };

    uint32_t vs = compileShader(GL_VERTEX_SHADER, kVertexShader);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    if (!vs || !fs) { if (vs) gl::DeleteShader(vs); if (fs) gl::DeleteShader(fs); return false; }

    m_shaderProg = gl::CreateProgram();
    gl::AttachShader(m_shaderProg, vs);
    gl::AttachShader(m_shaderProg, fs);
    gl::LinkProgram(m_shaderProg);

    int linked = 0;
    gl::GetProgramiv(m_shaderProg, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        spdlog::error("SpriteRenderer: shader link failed");
        gl::DeleteProgram(m_shaderProg); m_shaderProg = 0;
        return false;
    }
    gl::DeleteShader(vs);
    gl::DeleteShader(fs);

    m_uScreenSize = static_cast<uint32_t>(gl::GetUniformLocation(m_shaderProg, "uScreenSize"));
    m_uOffset     = static_cast<uint32_t>(gl::GetUniformLocation(m_shaderProg, "uOffset"));
    m_uColor      = static_cast<uint32_t>(gl::GetUniformLocation(m_shaderProg, "uColor"));
    m_uSizeLoc    = static_cast<uint32_t>(gl::GetUniformLocation(m_shaderProg, "uSize"));

    float vertices[] = {
        0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
        0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f,
    };
    gl::GenVertexArrays(1, &m_vao);
    gl::GenBuffers(1, &m_vbo);
    gl::BindVertexArray(m_vao);
    gl::BindBuffer(GL_ARRAY_BUFFER, m_vbo);
    gl::BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    gl::EnableVertexAttribArray(0);
    gl::BindVertexArray(0);

    m_initialized = true;
    spdlog::info("SpriteRenderer initialized (shader={}, vao={})", m_shaderProg, m_vao);
    return true;
}

void SpriteRenderer::shutdown()
{
    if (!m_initialized) return;
    if (m_vao) { gl::DeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo) { gl::DeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_shaderProg) { gl::DeleteProgram(m_shaderProg); m_shaderProg = 0; }
    m_initialized = false;
    spdlog::info("SpriteRenderer shutdown.");
}

// =========================================================================
// ECS 批量渲染
// =========================================================================
void SpriteRenderer::render(const engine::ecs::World& world,
                             uint32_t screenWidth, uint32_t screenHeight)
{
    if (!m_initialized) return;

    auto view = world.raw().view<ecs::Transform, ecs::Sprite>();
    if (view.begin() == view.end()) return;

    gl::UseProgram(m_shaderProg);
    gl::BindVertexArray(m_vao);
    gl::Uniform2f(static_cast<GLint>(m_uScreenSize),
        static_cast<float>(screenWidth), static_cast<float>(screenHeight));

    for (auto entity : view)
    {
        const auto& transform = world.raw().get<ecs::Transform>(entity);
        const auto& sprite    = world.raw().get<ecs::Sprite>(entity);
        if (!sprite.visible) continue;

        gl::Uniform2f(static_cast<GLint>(m_uOffset),
            transform.position.x, transform.position.y);
        gl::Uniform4f(static_cast<GLint>(m_uColor),
            sprite.tint.r, sprite.tint.g, sprite.tint.b, sprite.tint.a);
        gl::Uniform2f(static_cast<GLint>(m_uSizeLoc), 50.0f, 50.0f);

        gl::DrawArrays(GL_TRIANGLES, 0, 6);
    }

    gl::BindVertexArray(0);
    gl::UseProgram(0);
}

// =========================================================================
// 独立精灵提交
// =========================================================================
void SpriteRenderer::submit(const SpriteDesc& desc)
{
    m_submittedSprites.push_back(desc);
}

void SpriteRenderer::flush(uint32_t screenWidth, uint32_t screenHeight)
{
    if (!m_initialized || m_submittedSprites.empty()) return;

    gl::UseProgram(m_shaderProg);
    gl::BindVertexArray(m_vao);
    gl::Uniform2f(static_cast<GLint>(m_uScreenSize),
        static_cast<float>(screenWidth), static_cast<float>(screenHeight));

    for (const auto& desc : m_submittedSprites)
    {
        drawSprite(desc, screenWidth, screenHeight);
    }

    gl::BindVertexArray(0);
    gl::UseProgram(0);

    m_submittedSprites.clear();
}

void SpriteRenderer::drawSprite(const SpriteDesc& desc,
                                 uint32_t screenWidth, uint32_t screenHeight)
{
    (void)screenWidth; (void)screenHeight;
    gl::Uniform2f(static_cast<GLint>(m_uOffset),
        desc.position.x, desc.position.y);
    gl::Uniform4f(static_cast<GLint>(m_uColor),
        desc.tint.r, desc.tint.g, desc.tint.b, desc.tint.a);
    gl::Uniform2f(static_cast<GLint>(m_uSizeLoc),
        desc.size.x, desc.size.y);
    gl::DrawArrays(GL_TRIANGLES, 0, 6);
}

} // namespace engine::render
