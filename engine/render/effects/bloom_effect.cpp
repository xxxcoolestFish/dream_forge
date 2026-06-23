/**
 * @file engine/render/effects/bloom_effect.cpp
 * @brief 泛光特效实现
 */

#include "engine/render/effects/bloom_effect.h"
#include "engine/render/render_target.h"
#include "engine/render/gl_loader.h"

#include <spdlog/spdlog.h>

namespace engine::render::effects {

// =========================================================================
// 顶点 shader（全屏四边形 — 所有 pass 共用）
// =========================================================================
static const char* kFullscreenVert = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
out vec2 vTexCoord;
void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aPos * 0.5 + 0.5;  // NDC [-1,1] → UV [0,1]
}
)";

// =========================================================================
// 亮部提取 shader
// =========================================================================
static const char* kExtractFrag = R"(
#version 330 core
in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform float uThreshold;
out vec4 FragColor;

void main()
{
    vec3 color = texture(uTexture, vTexCoord).rgb;
    float brightness = dot(color, vec3(0.2126, 0.7152, 0.0722));
    if (brightness > uThreshold)
    {
        FragColor = vec4(color, 1.0);
    }
    else
    {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
)";

// =========================================================================
// 分离式高斯模糊 shader（水平 + 垂直）
// =========================================================================
static const char* kBlurFrag = R"(
#version 330 core
in vec2 vTexCoord;
uniform sampler2D uTexture;
uniform int   uDirection;    // 0=水平, 1=垂直
uniform vec2  uTexSize;      // 纹理尺寸（用于计算像素偏移）

out vec4 FragColor;

void main()
{
    vec2 texelSize = 1.0 / uTexSize;
    float weights[5] = float[](0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);
    vec3 result = texture(uTexture, vTexCoord).rgb * weights[0];

    vec2 dir = (uDirection == 0) ? vec2(texelSize.x, 0.0) : vec2(0.0, texelSize.y);

    for (int i = 1; i < 5; i++)
    {
        result += texture(uTexture, vTexCoord + dir * float(i)).rgb * weights[i];
        result += texture(uTexture, vTexCoord - dir * float(i)).rgb * weights[i];
    }

    FragColor = vec4(result, 1.0);
}
)";

// =========================================================================
// 叠加合成 shader
// =========================================================================
static const char* kCompositeFrag = R"(
#version 330 core
in vec2 vTexCoord;
uniform sampler2D uSceneTexture;
uniform sampler2D uBloomTexture;
uniform float     uIntensity;
out vec4 FragColor;

void main()
{
    vec3 scene = texture(uSceneTexture, vTexCoord).rgb;
    vec3 bloom = texture(uBloomTexture, vTexCoord).rgb;
    FragColor = vec4(scene + bloom * uIntensity, 1.0);
}
)";

// =========================================================================
BloomEffect::BloomEffect()
    : PostEffect("Bloom")
{
}

BloomEffect::~BloomEffect()
{
    if (m_initialized) shutdown();
}

bool BloomEffect::init(uint32_t screenWidth, uint32_t screenHeight)
{
    spdlog::info("BloomEffect: initializing ({}x{})", screenWidth, screenHeight);

    // 编译 shader
    m_extractProg   = compileShaderProgram(kFullscreenVert, kExtractFrag);
    m_blurProg      = compileShaderProgram(kFullscreenVert, kBlurFrag);
    m_compositeProg = compileShaderProgram(kFullscreenVert, kCompositeFrag);
    if (!m_extractProg || !m_blurProg || !m_compositeProg)
    {
        spdlog::error("BloomEffect: shader compilation failed");
        return false;
    }

    // Uniform 位置
    m_uExtractThreshold    = static_cast<uint32_t>(gl::GetUniformLocation(m_extractProg, "uThreshold"));
    m_uBlurDirection       = static_cast<uint32_t>(gl::GetUniformLocation(m_blurProg, "uDirection"));
    m_uBlurTexSize         = static_cast<uint32_t>(gl::GetUniformLocation(m_blurProg, "uTexSize"));
    m_uCompositeIntensity  = static_cast<uint32_t>(gl::GetUniformLocation(m_compositeProg, "uIntensity"));

    resize(screenWidth, screenHeight);
    m_initialized = true;
    spdlog::info("BloomEffect: initialized (threshold={:.2f}, intensity={:.2f})", m_threshold, m_intensity);
    return true;
}

void BloomEffect::resize(uint32_t w, uint32_t h)
{
    m_halfW = w / 2;
    m_halfH = h / 2;

    if (m_halfRes) m_halfRes->resize(m_halfW, m_halfH);
    else { m_halfRes = std::make_unique<RenderTarget>(); m_halfRes->init(m_halfW, m_halfH); }

    if (m_blurTemp) m_blurTemp->resize(m_halfW, m_halfH);
    else { m_blurTemp = std::make_unique<RenderTarget>(); m_blurTemp->init(m_halfW, m_halfH); }
}

void BloomEffect::apply(uint32_t inputTexture, uint32_t outputFbo,
                         uint32_t screenWidth, uint32_t screenHeight)
{
    if (!m_initialized || !m_enabled) return;

    (void)screenWidth; (void)screenHeight;

    // ---- Pass 1: 亮部提取到半分辨率 ----
    gl::BindFramebuffer(GL_FRAMEBUFFER, m_halfRes->fboId());
    gl::Viewport(0, 0, static_cast<GLsizei>(m_halfW), static_cast<GLsizei>(m_halfH));
    gl::UseProgram(m_extractProg);
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(GL_TEXTURE_2D, inputTexture);
    gl::Uniform1i(static_cast<GLint>(gl::GetUniformLocation(m_extractProg, "uTexture")), 0);
    gl::Uniform1f(static_cast<GLint>(m_uExtractThreshold), m_threshold);
    // 使用 EffectChain 的全屏 VAO — 此处使用内联绘制
    gl::BindVertexArray(0); // VAO 由外部绑定
    // 手动绘制全屏四边形（当前 post effect 的 apply 由 EffectChain 处理 VAO）
    // 这里我们直接用 effect chain 统一管理 VAO，因此假定 VAO 已绑定
    // 简单起见，为每个 effect apply 创建独立的 fullscreen 绘制
    gl::DrawArrays(GL_TRIANGLES, 0, 6);

    // ---- Pass 2a: 水平模糊 ----
    gl::BindFramebuffer(GL_FRAMEBUFFER, m_blurTemp->fboId());
    gl::Viewport(0, 0, static_cast<GLsizei>(m_halfW), static_cast<GLsizei>(m_halfH));
    gl::UseProgram(m_blurProg);
    m_halfRes->bindTexture(0);
    gl::Uniform1i(static_cast<GLint>(gl::GetUniformLocation(m_blurProg, "uTexture")), 0);
    gl::Uniform1i(static_cast<GLint>(m_uBlurDirection), 0);
    gl::Uniform2f(static_cast<GLint>(m_uBlurTexSize),
        static_cast<float>(m_halfW), static_cast<float>(m_halfH));
    gl::DrawArrays(GL_TRIANGLES, 0, 6);

    // ---- Pass 2b: 垂直模糊 ----
    gl::BindFramebuffer(GL_FRAMEBUFFER, m_halfRes->fboId());
    m_blurTemp->bindTexture(0);
    gl::Uniform1i(static_cast<GLint>(gl::GetUniformLocation(m_blurProg, "uTexture")), 0);
    gl::Uniform1i(static_cast<GLint>(m_uBlurDirection), 1);
    gl::DrawArrays(GL_TRIANGLES, 0, 6);

    // ---- Pass 3: 叠加合成 ----
    gl::BindFramebuffer(GL_FRAMEBUFFER, outputFbo);
    gl::Viewport(0, 0, static_cast<GLsizei>(screenWidth), static_cast<GLsizei>(screenHeight));
    gl::UseProgram(m_compositeProg);

    // 绑定场景纹理（单元 0）+ Bloom 纹理（单元 1）
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(GL_TEXTURE_2D, inputTexture);
    gl::Uniform1i(static_cast<GLint>(gl::GetUniformLocation(m_compositeProg, "uSceneTexture")), 0);

    gl::ActiveTexture(GL_TEXTURE1);
    m_halfRes->bindTexture(1);
    gl::Uniform1i(static_cast<GLint>(gl::GetUniformLocation(m_compositeProg, "uBloomTexture")), 1);

    gl::Uniform1f(static_cast<GLint>(m_uCompositeIntensity), m_intensity);
    gl::DrawArrays(GL_TRIANGLES, 0, 6);

    // 清理
    gl::ActiveTexture(GL_TEXTURE0);
    gl::BindTexture(GL_TEXTURE_2D, 0);
    gl::ActiveTexture(GL_TEXTURE1);
    gl::BindTexture(GL_TEXTURE_2D, 0);
    gl::UseProgram(0);
    gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
}

void BloomEffect::shutdown()
{
    if (m_extractProg)   { gl::DeleteProgram(m_extractProg);   m_extractProg = 0; }
    if (m_blurProg)      { gl::DeleteProgram(m_blurProg);      m_blurProg = 0; }
    if (m_compositeProg) { gl::DeleteProgram(m_compositeProg); m_compositeProg = 0; }
    m_halfRes.reset();
    m_blurTemp.reset();
    m_initialized = false;
    spdlog::info("BloomEffect: shutdown");
}

} // namespace engine::render::effects
