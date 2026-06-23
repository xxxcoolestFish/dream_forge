/**
 * @file engine/render/effects/bloom_effect.h
 * @brief 泛光特效 — 亮部提取 + 模糊 + 合成
 *
 * 3-pass 实现：
 *   1. 亮度提取（1/2 分辨率）
 *   2. 双 pass 高斯模糊（水平 + 垂直）
 *   3. 叠加合成
 */

#pragma once

#include "engine/render/effects/post_effect.h"
#include "engine/render/render_target.h"
#include <memory>

namespace engine::render::effects {

class BloomEffect : public PostEffect
{
public:
    BloomEffect();
    ~BloomEffect() override;

    bool init(uint32_t screenWidth, uint32_t screenHeight) override;
    void apply(uint32_t inputTexture, uint32_t outputFbo,
               uint32_t screenWidth, uint32_t screenHeight) override;
    void resize(uint32_t w, uint32_t h) override;
    void shutdown() override;

    // 参数
    void setThreshold(float t)  { m_threshold = t; }
    void setIntensity(float i)  { m_intensity = i; }
    float threshold() const     { return m_threshold; }
    float intensity() const     { return m_intensity; }

private:
    float m_threshold = 0.7f;
    float m_intensity = 1.0f;

    // Shader 程序
    uint32_t m_extractProg  = 0;   // 亮部提取
    uint32_t m_blurProg     = 0;   // 高斯模糊（水平+垂直复用）
    uint32_t m_compositeProg = 0;  // 叠加合成

    // 半分辨率 RenderTarget
    std::unique_ptr<RenderTarget> m_halfRes;
    std::unique_ptr<RenderTarget> m_blurTemp;

    uint32_t m_halfW = 0;
    uint32_t m_halfH = 0;

    // Uniform 位置
    uint32_t m_uExtractThreshold = 0;
    uint32_t m_uBlurDirection    = 0;
    uint32_t m_uBlurTexSize      = 0;
    uint32_t m_uCompositeIntensity = 0;
};

} // namespace engine::render::effects
