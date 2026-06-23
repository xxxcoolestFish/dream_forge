/**
 * @file engine/render/render_target.cpp
 * @brief 渲染目标实现 — FBO 创建/销毁/绑定
 */

#include "engine/render/render_target.h"
#include "engine/render/gl_loader.h"

#include <spdlog/spdlog.h>

namespace engine::render {

RenderTarget::RenderTarget()
{
    spdlog::debug("RenderTarget: created");
}

RenderTarget::~RenderTarget()
{
    if (m_initialized)
    {
        shutdown();
    }
}

bool RenderTarget::init(uint32_t width, uint32_t height)
{
    if (width == 0 || height == 0)
    {
        spdlog::error("RenderTarget::init: invalid size {}x{}", width, height);
        return false;
    }

    m_width  = width;
    m_height = height;

    createTextures();

    m_initialized = true;
    spdlog::info("RenderTarget: initialized {}x{} (fbo={}, colorTex={})",
        m_width, m_height, m_fbo, m_colorTexture);
    return true;
}

void RenderTarget::createTextures()
{
    // 1. 创建 FBO
    gl::GenFramebuffers(1, &m_fbo);
    gl::BindFramebuffer(GL_FRAMEBUFFER, m_fbo);

    // 2. 创建颜色纹理附件
    gl::GenTextures(1, &m_colorTexture);
    gl::BindTexture(GL_TEXTURE_2D, m_colorTexture);
    gl::TexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(GL_RGBA),
        static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height),
        0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // 附加到 FBO
    gl::FramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0,
        GL_TEXTURE_2D, m_colorTexture, 0);

    // 3. 创建深度 Renderbuffer
    gl::GenRenderbuffers(1, &m_depthRbo);
    gl::BindRenderbuffer(GL_RENDERBUFFER, m_depthRbo);
    gl::RenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24,
        static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height));
    gl::FramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT,
        GL_RENDERBUFFER, m_depthRbo);

    // 4. 检查完整性
    GLenum status = gl::CheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE)
    {
        spdlog::error("RenderTarget: framebuffer not complete, status=0x{:X}", status);
    }

    // 解绑
    gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
    gl::BindTexture(GL_TEXTURE_2D, 0);
    gl::BindRenderbuffer(GL_RENDERBUFFER, 0);
}

void RenderTarget::shutdown()
{
    if (!m_initialized) return;

    if (m_depthRbo)
    {
        gl::DeleteRenderbuffers(1, &m_depthRbo);
        m_depthRbo = 0;
    }
    if (m_colorTexture)
    {
        gl::DeleteTextures(1, &m_colorTexture);
        m_colorTexture = 0;
    }
    if (m_fbo)
    {
        gl::DeleteFramebuffers(1, &m_fbo);
        m_fbo = 0;
    }

    m_initialized = false;
    spdlog::info("RenderTarget: shutdown");
}

void RenderTarget::bind()
{
    if (!m_initialized) return;
    gl::BindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    gl::Viewport(0, 0, static_cast<GLsizei>(m_width), static_cast<GLsizei>(m_height));
}

void RenderTarget::unbind()
{
    gl::BindFramebuffer(GL_FRAMEBUFFER, 0);
}

void RenderTarget::bindTexture(int unit) const
{
    if (!m_initialized) return;
    gl::ActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + unit));
    gl::BindTexture(GL_TEXTURE_2D, m_colorTexture);
}

void RenderTarget::resize(uint32_t width, uint32_t height)
{
    if (!m_initialized || (width == m_width && height == m_height)) return;

    // 销毁旧资源
    if (m_depthRbo)  { gl::DeleteRenderbuffers(1, &m_depthRbo); m_depthRbo = 0; }
    if (m_colorTexture) { gl::DeleteTextures(1, &m_colorTexture); m_colorTexture = 0; }
    if (m_fbo)       { gl::DeleteFramebuffers(1, &m_fbo); m_fbo = 0; }

    m_width  = width;
    m_height = height;

    createTextures();

    // 验证新 FBO
    gl::BindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    GLenum status = gl::CheckFramebufferStatus(GL_FRAMEBUFFER);
    gl::BindFramebuffer(GL_FRAMEBUFFER, 0);

    if (status == GL_FRAMEBUFFER_COMPLETE)
    {
        spdlog::info("RenderTarget: resized to {}x{}", width, height);
    }
    else
    {
        spdlog::error("RenderTarget: resize failed, fbo not complete (0x{:X})", status);
    }
}

} // namespace engine::render
