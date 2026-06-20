/**
 * @file engine/render/sprite.h
 * @brief 精灵绘制
 *
 * 负责：
 *   - 单个精灵的绘制（一个带纹理的四边形）
 *   - 批处理精灵的提交
 *
 * 当前 Phase 1 阶段：简单单精灵绘制
 * 后续阶段：精灵批处理（instance rendering）、排序
 *
 * 注意事项：
 *   - 使用 bgfx 的 transient vertex buffer 提交
 *   - 所有精灵使用同一个 shader 程序
 */

#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace engine::render {

struct SpriteDesc
{
    uint16_t  textureHandle = UINT16_MAX;
    glm::vec2 position      { 0.0f, 0.0f };    // 屏幕坐标
    glm::vec2 size          { 100.0f, 100.0f }; // 像素
    glm::vec2 anchor        { 0.5f, 0.5f };    // 锚点
    glm::vec4 tint          { 1.0f, 1.0f, 1.0f, 1.0f };
    float     rotation      = 0.0f;             // 弧度
    float     depth         = 0.0f;             // 0.0(最远) ~ 1.0(最近)
};

class SpriteRenderer
{
public:
    SpriteRenderer();
    ~SpriteRenderer();

    // 初始化（加载 shader 等）
    bool init();

    // 关闭
    void shutdown();

    // 提交一个精灵到渲染队列
    void submit(const SpriteDesc& desc);

    // 执行所有提交的绘制
    void flush();

private:
    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace engine::render
