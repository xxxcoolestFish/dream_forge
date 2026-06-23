/**
 * @file engine/render/post_process.h
 * @brief 后处理基座 — 全屏四边形渲染 + 输入纹理采样
 *
 * Phase 6.3：提供基础的全屏四边形直通渲染（将 FBO → 屏幕）。
 * Phase 6.4：扩展为特效链（Bloom/Vignette/ColorGrading）。
 */

#pragma once

#include <cstdint>

namespace engine::render {

class RenderTarget;

class PostProcess
{
public:
    PostProcess();
    ~PostProcess();

    // 禁止拷贝
    PostProcess(const PostProcess&) = delete;
    PostProcess& operator=(const PostProcess&) = delete;

    bool init();
    void shutdown();

    // 应用后处理：将 RenderTarget 的颜色纹理渲染为全屏四边形
    void apply(const RenderTarget& source,
               uint32_t screenWidth, uint32_t screenHeight);

private:
    uint32_t m_shaderProg = 0;
    uint32_t m_vao        = 0;
    uint32_t m_vbo        = 0;
    uint32_t m_uTexture   = 0;
    bool     m_initialized = false;
};

} // namespace engine::render
