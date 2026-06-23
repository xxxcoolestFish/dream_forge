/**
 * @file engine/render/render_target.h
 * @brief 渲染目标 — OpenGL FBO + 颜色纹理 + 深度缓冲封装
 *
 * Phase 6.3：使场景可渲染到纹理，支持后处理、转场等效果。
 *
 * 使用方式：
 *   RenderTarget rt;
 *   rt.init(1280, 720);
 *   rt.bind();
 *   // ... 渲染场景 ...
 *   rt.unbind();
 *   rt.bindTexture(0);  // 绑定颜色纹理到单元 0 供后处理 shader 采样
 */

#pragma once

#include <cstdint>

namespace engine::render {

class RenderTarget
{
public:
    RenderTarget();
    ~RenderTarget();

    // 禁止拷贝
    RenderTarget(const RenderTarget&) = delete;
    RenderTarget& operator=(const RenderTarget&) = delete;

    // 创建颜色纹理 + 深度 Renderbuffer 的 FBO
    bool init(uint32_t width, uint32_t height);
    void shutdown();

    // 绑定 / 解绑
    void bind();
    void unbind();

    // 绑定颜色纹理到指定纹理单元（供 shader 采样）
    void bindTexture(int unit = 0) const;

    // 窗口大小变化时调整
    void resize(uint32_t width, uint32_t height);

    // 查询
    uint32_t width()          const { return m_width; }
    uint32_t height()         const { return m_height; }
    uint32_t fboId()          const { return m_fbo; }
    uint32_t colorTextureId() const { return m_colorTexture; }
    bool isInitialized()      const { return m_initialized; }

private:
    uint32_t m_fbo          = 0;
    uint32_t m_colorTexture = 0;
    uint32_t m_depthRbo     = 0;
    uint32_t m_width        = 0;
    uint32_t m_height       = 0;
    bool     m_initialized  = false;

    void createTextures();
};

} // namespace engine::render
