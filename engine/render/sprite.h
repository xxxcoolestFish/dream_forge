/**
 * @file engine/render/sprite.h
 * @brief 2D 精灵渲染器 — ECS 批量渲染 + 独立精灵提交
 */

#pragma once

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace engine::ecs {
    class World;
}

namespace engine::render {

// 独立精灵描述（用于场景渲染等非 ECS 场景）
struct SpriteDesc
{
    uint16_t  textureHandle = UINT16_MAX;  // 纹理句柄（UINT16_MAX = 纯色）
    glm::vec2 position      { 0.0f, 0.0f };
    glm::vec2 size          { 50.0f, 50.0f };
    glm::vec4 tint          { 1.0f, 1.0f, 1.0f, 1.0f };
    float     rotation      = 0.0f;        // 弧度
    float     depth         = 0.5f;        // 0=远景, 1=前景
};

class SpriteRenderer
{
public:
    SpriteRenderer();
    ~SpriteRenderer();

    bool init();
    void shutdown();

    // --- ECS 批量渲染 ---
    void render(const engine::ecs::World& world,
                uint32_t screenWidth, uint32_t screenHeight);

    // --- 独立精灵提交 ---
    void submit(const SpriteDesc& desc);

    // 提交所有精灵并绘制（如果已经通过 submit 提交了精灵）
    void flush(uint32_t screenWidth, uint32_t screenHeight);

private:
    void drawSprite(const SpriteDesc& desc,
                    uint32_t screenWidth, uint32_t screenHeight);

    uint32_t m_vao          = 0;
    uint32_t m_vbo          = 0;
    uint32_t m_shaderProg   = 0;
    uint32_t m_uScreenSize  = 0;
    uint32_t m_uOffset      = 0;
    uint32_t m_uColor       = 0;
    uint32_t m_uSizeLoc     = 0; // uSize uniform location
    bool     m_initialized  = false;

    // 提交队列
    std::vector<SpriteDesc> m_submittedSprites;
};

} // namespace engine::render
