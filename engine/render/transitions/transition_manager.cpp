/**
 * @file engine/render/transitions/transition_manager.cpp
 * @brief 转场管理实现 — 全屏着色四边形淡入淡出
 */

#include "engine/render/transitions/transition_manager.h"
#include "engine/render/gl_loader.h"

#include <spdlog/spdlog.h>
#include <vector>

namespace engine::render::transitions {

// 简单全屏四边形 shader（仅颜色 + alpha）
static const char* kTransVert = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
}
)";

static const char* kTransFrag = R"(
#version 330 core
uniform vec4 uColor;
out vec4 FragColor;
void main()
{
    FragColor = uColor;
}
)";

TransitionManager::TransitionManager()  = default;
TransitionManager::~TransitionManager() { if (m_initialized) shutdown(); }

bool TransitionManager::init()
{
    auto compileShader = [](uint32_t type, const char* src) -> uint32_t
    {
        uint32_t s = gl::CreateShader(type);
        gl::ShaderSource(s, 1, &src, nullptr);
        gl::CompileShader(s);
        int ok = 0;
        gl::GetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { gl::DeleteShader(s); return 0; }
        return s;
    };

    uint32_t vs = compileShader(GL_VERTEX_SHADER, kTransVert);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, kTransFrag);
    if (!vs || !fs) { if (vs) gl::DeleteShader(vs); if (fs) gl::DeleteShader(fs); return false; }

    m_shaderProg = gl::CreateProgram();
    gl::AttachShader(m_shaderProg, vs);
    gl::AttachShader(m_shaderProg, fs);
    gl::LinkProgram(m_shaderProg);
    gl::DeleteShader(vs);
    gl::DeleteShader(fs);

    float vertices[] = {
        -1, 1,  -1,-1,   1,-1,
        -1, 1,   1,-1,   1, 1,
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
    spdlog::info("TransitionManager: initialized");
    return true;
}

void TransitionManager::shutdown()
{
    if (m_vbo)    { gl::DeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_vao)    { gl::DeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_shaderProg) { gl::DeleteProgram(m_shaderProg); m_shaderProg = 0; }
    m_initialized = false;
    m_active = false;
}

void TransitionManager::start(TransitionType type, float duration, glm::vec4 color)
{
    m_type     = type;
    m_duration = std::max(duration, 0.01f);
    m_color    = color;
    m_elapsed  = 0.0f;
    m_active   = true;
    spdlog::info("TransitionManager: start {} (duration={:.1f}s)",
        (type == TransitionType::FadeIn) ? "FadeIn" : "FadeOut", m_duration);
}

bool TransitionManager::isActive() const { return m_active; }

void TransitionManager::update(float dt)
{
    if (!m_active) return;
    m_elapsed += dt;
    if (m_elapsed >= m_duration)
    {
        m_elapsed = m_duration;
        m_active  = false; // 转场结束
    }
}

void TransitionManager::render(uint32_t, uint32_t)
{
    if (!m_active || !m_initialized) return;

    float t = m_elapsed / m_duration;
    float alpha;

    if (m_type == TransitionType::FadeIn)
        alpha = 1.0f - t; // 从不透明 → 透明
    else
        alpha = t;         // 从透明 → 不透明

    if (alpha <= 0.0f) return;

    gl::UseProgram(m_shaderProg);
    gl::BindVertexArray(m_vao);

    int loc = gl::GetUniformLocation(m_shaderProg, "uColor");
    gl::Uniform4f(loc, m_color.r, m_color.g, m_color.b, m_color.a * alpha);

    gl::DrawArrays(GL_TRIANGLES, 0, 6);
    gl::BindVertexArray(0);
    gl::UseProgram(0);
}

void TransitionManager::skip()
{
    m_active  = false;
    m_elapsed = m_duration;
}

} // namespace engine::render::transitions
