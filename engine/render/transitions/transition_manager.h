/**
 * @file engine/render/transitions/transition_manager.h
 * @brief 屏幕转场管理 — Phase 6.6
 *
 * 当前支持：FadeIn（黑→画面）、FadeOut（画面→黑）
 * 通过全屏着色四边形叠加在 UI 之上（最高层）实现。
 */

#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace engine::render::transitions {

enum class TransitionType { FadeIn, FadeOut };

class TransitionManager
{
public:
    TransitionManager();
    ~TransitionManager();

    // 禁止拷贝
    TransitionManager(const TransitionManager&) = delete;
    TransitionManager& operator=(const TransitionManager&) = delete;

    bool init();
    void shutdown();

    // 启动转场
    void start(TransitionType type, float duration = 1.0f, glm::vec4 color = {0,0,0,1});

    // 是否正在转场
    bool isActive() const;

    // 每帧更新
    void update(float dt);

    // 渲染叠加四边形（在 UI 之后调用）
    void render(uint32_t screenWidth, uint32_t screenHeight);

    // 立即结束
    void skip();

private:
    uint32_t m_shaderProg = 0;
    uint32_t m_vao = 0;
    uint32_t m_vbo = 0;

    TransitionType m_type      = TransitionType::FadeIn;
    glm::vec4      m_color     = {0,0,0,1};
    float          m_duration  = 1.0f;
    float          m_elapsed   = 0.0f;
    bool           m_active    = false;
    bool           m_initialized = false;
};

} // namespace engine::render::transitions
