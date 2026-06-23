/**
 * @file engine/render/effects/vignette_effect.cpp
 * @brief 暗角特效实现
 */

#include "engine/render/effects/vignette_effect.h"
#include "engine/render/gl_loader.h"

#include <spdlog/spdlog.h>

namespace engine::render::effects {

static const char* kVignetteVert = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vTexCoord;
void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aPos * 0.5 + 0.5;
}
)";

static const char* kVignetteFrag = R"(
#version 330 core
in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform float uRadius;
uniform float uSoftness;
out vec4 FragColor;

void main()
{
    vec3 color = texture(uTexture, vTexCoord).rgb;

    // 径向距离（屏幕中心）
    vec2 center = vTexCoord - 0.5;
    float dist = length(center) * 1.414; // 对角线归一化

    // smoothstep 暗角
    float vignette = 1.0 - smoothstep(uRadius, uRadius + uSoftness, dist);

    FragColor = vec4(color * vignette, 1.0);
}
)";

VignetteEffect::VignetteEffect()
    : PostEffect("Vignette")
{
}

VignetteEffect::~VignetteEffect()
{
    if (m_initialized) shutdown();
}

bool VignetteEffect::init(uint32_t, uint32_t)
{
    m_shaderProg = compileShaderProgram(kVignetteVert, kVignetteFrag);
    if (!m_shaderProg) return false;

    m_initialized = true;
    spdlog::info("VignetteEffect: initialized (radius={:.2f}, softness={:.2f})", m_radius, m_softness);
    return true;
}

void VignetteEffect::apply(uint32_t inputTexture, uint32_t outputFbo,
                            uint32_t screenWidth, uint32_t screenHeight)
{
    if (!m_initialized || !m_enabled) return;

    gl::BindFramebuffer(GL_FRAMEBUFFER, outputFbo);
    gl::Viewport(0, 0, static_cast<GLsizei>(screenWidth), static_cast<GLsizei>(screenHeight));
    gl::UseProgram(m_shaderProg);

    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(GL_TEXTURE_2D, inputTexture);
    gl::Uniform1i(static_cast<GLint>(gl::GetUniformLocation(m_shaderProg, "uTexture")), 0);
    gl::Uniform1f(static_cast<GLint>(gl::GetUniformLocation(m_shaderProg, "uRadius")), m_radius);
    gl::Uniform1f(static_cast<GLint>(gl::GetUniformLocation(m_shaderProg, "uSoftness")), m_softness);

    gl::DrawArrays(GL_TRIANGLES, 0, 6);

    gl::BindTexture(GL_TEXTURE_2D, 0);
    gl::UseProgram(0);
    gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
}

void VignetteEffect::resize(uint32_t, uint32_t)
{
    // 无需改变（shader 基于相对坐标）
}

void VignetteEffect::shutdown()
{
    if (m_shaderProg) { gl::DeleteProgram(m_shaderProg); m_shaderProg = 0; }
    m_initialized = false;
    spdlog::info("VignetteEffect: shutdown");
}

} // namespace engine::render::effects
