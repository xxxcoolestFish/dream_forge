/**
 * @file engine/render/effects/effect_chain.h
 * @brief 特效链 — 管理多个后处理特效的序列化应用
 *
 * 内部使用 ping-pong FBO 支持多 pass 特效。
 * 每个特效读取 input 纹理 → 输出到 output FBO。
 */

#pragma once

#include <cstdint>
#include <memory>
#include <vector>
#include <string>

#include "engine/render/render_target.h"

namespace engine::render::effects {

class PostEffect;

class EffectChain
{
public:
    EffectChain();
    ~EffectChain();

    // 禁止拷贝
    EffectChain(const EffectChain&) = delete;
    EffectChain& operator=(const EffectChain&) = delete;

    bool init(uint32_t screenWidth, uint32_t screenHeight);
    void shutdown();

    // 添加特效（接管所有权）
    void addEffect(std::unique_ptr<PostEffect> effect);

    // 按名称查找
    PostEffect* findEffect(const std::string& name);

    // 处理：inputTexture → chain → 默认帧缓冲（或指定 FBO）
    void process(uint32_t inputTexture, uint32_t screenWidth, uint32_t screenHeight);

    // 窗口 resize
    void resize(uint32_t w, uint32_t h);

private:
    std::vector<std::unique_ptr<PostEffect>> m_effects;

    // Ping-pong 渲染目标（按需创建）
    std::unique_ptr<RenderTarget> m_ping;
    std::unique_ptr<RenderTarget> m_pong;

    // 全屏四边形 VAO（所有特效共享）
    uint32_t m_fullscreenVao = 0;
    uint32_t m_fullscreenVbo = 0;

    // 直通 shader（无特效时使用）
    uint32_t m_passthroughProg = 0;
    uint32_t m_uPassthroughTex = 0;

    uint32_t m_screenWidth  = 0;
    uint32_t m_screenHeight = 0;
    bool     m_initialized  = false;

    void ensurePingPong(uint32_t w, uint32_t h);
    void drawFullscreenQuad();
};

} // namespace engine::render::effects
