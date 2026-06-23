/**
 * @file engine/render/effects/vignette_effect.h
 * @brief 暗角特效 — 径向渐暗
 */

#pragma once

#include "engine/render/effects/post_effect.h"

namespace engine::render::effects {

class VignetteEffect : public PostEffect
{
public:
    VignetteEffect();
    ~VignetteEffect() override;

    bool init(uint32_t screenWidth, uint32_t screenHeight) override;
    void apply(uint32_t inputTexture, uint32_t outputFbo,
               uint32_t screenWidth, uint32_t screenHeight) override;
    void resize(uint32_t w, uint32_t h) override;
    void shutdown() override;

    // 参数：radius 越大暗角越小（0.3=强暗角, 1.0=几乎无）
    void setRadius(float r)    { m_radius = r; }
    void setSoftness(float s)  { m_softness = s; }
    float radius() const       { return m_radius; }
    float softness() const     { return m_softness; }

private:
    float m_radius   = 0.6f;
    float m_softness = 0.4f;
    uint32_t m_shaderProg = 0;
};

} // namespace engine::render::effects
