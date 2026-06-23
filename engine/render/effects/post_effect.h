/**
 * @file engine/render/effects/post_effect.h
 * @brief 后处理特效虚基类 — Phase 6.4
 *
 * 所有后处理特效（Bloom、Vignette、ColorGrading）继承此类。
 */

#pragma once

#include <cstdint>
#include <string>

namespace engine::render::effects {

class PostEffect
{
public:
    PostEffect(const std::string& name);
    virtual ~PostEffect();

    // 禁止拷贝
    PostEffect(const PostEffect&) = delete;
    PostEffect& operator=(const PostEffect&) = delete;

    // 初始化（创建 shader、FBO 等）
    virtual bool init(uint32_t screenWidth, uint32_t screenHeight) = 0;

    // 应用特效：从 inputTexture 读取 → 写入 outputFbo（0 = 默认帧缓冲）
    virtual void apply(uint32_t inputTexture, uint32_t outputFbo,
                       uint32_t screenWidth, uint32_t screenHeight) = 0;

    // 窗口大小变化
    virtual void resize(uint32_t w, uint32_t h) = 0;
    virtual void shutdown() = 0;

    // 开关
    void setEnabled(bool e) { m_enabled = e; }
    bool isEnabled() const  { return m_enabled; }
    const std::string& name() const { return m_name; }

protected:
    std::string m_name;
    bool m_enabled = true;
    bool m_initialized = false;

    // 辅助：编译 + 链接 shader
    static uint32_t compileShaderProgram(const char* vertSrc, const char* fragSrc);
};

} // namespace engine::render::effects
