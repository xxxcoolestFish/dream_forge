/**
 * @file engine/render/effects/effect_chain.cpp
 * @brief 特效链实现 — ping-pong FBO + 特效序列化
 */

#include "engine/render/effects/effect_chain.h"
#include "engine/render/effects/post_effect.h"
#include "engine/render/render_target.h"
#include "engine/render/gl_loader.h"

#include <spdlog/spdlog.h>

namespace engine::render::effects {

// =========================================================================
// 直通 shader（无特效时的默认全屏四边形）
// =========================================================================
static const char* kPassthroughVert = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vTexCoord;
void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aPos * 0.5 + 0.5;
}
)";

static const char* kPassthroughFrag = R"(
#version 330 core
in vec2 vTexCoord;
uniform sampler2D uScreenTexture;
out vec4 FragColor;
void main()
{
    FragColor = texture(uScreenTexture, vTexCoord);
}
)";

EffectChain::EffectChain()
{
    spdlog::debug("EffectChain: created");
}

EffectChain::~EffectChain()
{
    if (m_initialized) shutdown();
}

bool EffectChain::init(uint32_t screenWidth, uint32_t screenHeight)
{
    m_screenWidth  = screenWidth;
    m_screenHeight = screenHeight;

    // 创建全屏四边形 VAO（NDC 坐标, 无 UV — shader 用 gl_FragCoord 计算）
    float vertices[] = {
        -1.0f,  1.0f,   -1.0f, -1.0f,   1.0f, -1.0f,
        -1.0f,  1.0f,    1.0f, -1.0f,   1.0f,  1.0f,
    };
    gl::GenVertexArrays(1, &m_fullscreenVao);
    gl::GenBuffers(1, &m_fullscreenVbo);
    gl::BindVertexArray(m_fullscreenVao);
    gl::BindBuffer(GL_ARRAY_BUFFER, m_fullscreenVbo);
    gl::BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    gl::EnableVertexAttribArray(0);
    gl::BindVertexArray(0);

    // 编译直通 shader（无特效时的后备）
    {
        auto compileOne = [](uint32_t type, const char* src) -> uint32_t
        {
            uint32_t s = gl::CreateShader(type);
            gl::ShaderSource(s, 1, &src, nullptr);
            gl::CompileShader(s);
            int ok = 0;
            gl::GetShaderiv(s, GL_COMPILE_STATUS, &ok);
            if (!ok) { gl::DeleteShader(s); return 0; }
            return s;
        };
        uint32_t vs = compileOne(GL_VERTEX_SHADER, kPassthroughVert);
        uint32_t fs = compileOne(GL_FRAGMENT_SHADER, kPassthroughFrag);
        if (vs && fs)
        {
            m_passthroughProg = gl::CreateProgram();
            gl::AttachShader(m_passthroughProg, vs);
            gl::AttachShader(m_passthroughProg, fs);
            gl::LinkProgram(m_passthroughProg);
            gl::DeleteShader(vs);
            gl::DeleteShader(fs);
            m_uPassthroughTex = static_cast<uint32_t>(gl::GetUniformLocation(m_passthroughProg, "uScreenTexture"));
        }
    }

    m_initialized = true;
    spdlog::info("EffectChain: initialized {}x{}", screenWidth, screenHeight);
    return true;
}

void EffectChain::shutdown()
{
    for (auto& effect : m_effects)
    {
        effect->shutdown();
    }
    m_effects.clear();

    m_ping.reset();
    m_pong.reset();

    if (m_passthroughProg) { gl::DeleteProgram(m_passthroughProg); m_passthroughProg = 0; }
    if (m_fullscreenVbo) { gl::DeleteBuffers(1, &m_fullscreenVbo); m_fullscreenVbo = 0; }
    if (m_fullscreenVao) { gl::DeleteVertexArrays(1, &m_fullscreenVao); m_fullscreenVao = 0; }

    m_initialized = false;
    spdlog::info("EffectChain: shutdown");
}

void EffectChain::addEffect(std::unique_ptr<PostEffect> effect)
{
    if (!effect) return;
    spdlog::info("EffectChain: added effect '{}'", effect->name());
    m_effects.push_back(std::move(effect));
}

PostEffect* EffectChain::findEffect(const std::string& name)
{
    for (auto& e : m_effects)
    {
        if (e->name() == name) return e.get();
    }
    return nullptr;
}

void EffectChain::ensurePingPong(uint32_t w, uint32_t h)
{
    if (!m_ping)
    {
        m_ping = std::make_unique<RenderTarget>();
        m_ping->init(w, h);
    }
    if (!m_pong)
    {
        m_pong = std::make_unique<RenderTarget>();
        m_pong->init(w, h);
    }
}

void EffectChain::drawFullscreenQuad()
{
    gl::BindVertexArray(m_fullscreenVao);
    gl::DrawArrays(GL_TRIANGLES, 0, 6);
    gl::BindVertexArray(0);
}

void EffectChain::process(uint32_t inputTexture, uint32_t screenWidth, uint32_t screenHeight)
{
    if (!m_initialized) return;

    // 收集启用的特效
    std::vector<PostEffect*> active;
    for (auto& e : m_effects)
    {
        if (e->isEnabled()) active.push_back(e.get());
    }

    if (active.empty())
    {
        // 无特效：直通渲染 FBO 纹理 → 屏幕
        if (m_passthroughProg)
        {
            gl::Viewport(0, 0, static_cast<GLsizei>(screenWidth), static_cast<GLsizei>(screenHeight));
            gl::UseProgram(m_passthroughProg);
            gl::BindVertexArray(m_fullscreenVao);
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(GL_TEXTURE_2D, inputTexture);
            gl::Uniform1i(static_cast<GLint>(m_uPassthroughTex), 0);
            gl::DrawArrays(GL_TRIANGLES, 0, 6);
            gl::BindVertexArray(0);
            gl::UseProgram(0);
            gl::BindTexture(GL_TEXTURE_2D, 0);
        }
        return;
    }

    ensurePingPong(screenWidth, screenHeight);

    // 特效链：input → effect0 → ping, ping → effect1 → pong, ...
    uint32_t srcTexture = inputTexture;
    RenderTarget* dstTarget = m_ping.get();

    for (size_t i = 0; i < active.size(); i++)
    {
        uint32_t dstFbo = dstTarget->fboId();

        // 最后一个特效输出到默认帧缓冲
        if (i == active.size() - 1)
        {
            dstFbo = 0;
        }

        active[i]->apply(srcTexture, dstFbo, screenWidth, screenHeight);

        // 切换 src → 刚写入的纹理
        if (i < active.size() - 1)
        {
            srcTexture = dstTarget->colorTextureId();
            dstTarget = (dstTarget == m_ping.get()) ? m_pong.get() : m_ping.get();
            // 绑定目标 FBO 的 GL 上下文
            gl::BindFramebuffer(GL_FRAMEBUFFER, dstTarget->fboId());
            gl::Clear(GL_COLOR_BUFFER_BIT);
            gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // 解绑所有纹理（避免冲突）
        gl::BindTexture(GL_TEXTURE_2D, 0);
    }
}

void EffectChain::resize(uint32_t w, uint32_t h)
{
    m_screenWidth  = w;
    m_screenHeight = h;

    if (m_ping) m_ping->resize(w, h);
    if (m_pong) m_pong->resize(w, h);

    for (auto& e : m_effects)
    {
        e->resize(w, h);
    }
}

} // namespace engine::render::effects
