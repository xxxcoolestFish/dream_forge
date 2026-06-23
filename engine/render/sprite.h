/**
 * @file engine/render/sprite.h
 * @brief 2D/3D 精灵渲染器 — ECS 批量渲染 + 独立精灵提交
 *
 * Phase 7: 新增 3D MVP 渲染路径，原有 2D 像素坐标路径保留给 UI 使用。
 */

#pragma once

#include "engine/render/texture.h"

#include <cstdint>
#include <vector>
#include <glm/glm.hpp>

namespace engine::ecs {
    class World;
}

namespace engine::render {

// 独立精灵描述（2D/3D 通用）
struct SpriteDesc
{
    uint32_t  textureHandle = kInvalidTexture;
    glm::vec3 position      { 0.0f, 0.0f, 0.0f };  // 3D 世界坐标
    glm::vec2 size          { 50.0f, 50.0f };
    glm::vec4 tint          { 1.0f, 1.0f, 1.0f, 1.0f };
    float     rotation      = 0.0f;
    float     depth         = 0.5f;                 // 保留给 2D 模式
};

class SpriteRenderer
{
public:
    SpriteRenderer();
    ~SpriteRenderer();

    bool init();
    void shutdown();

    // --- ECS 批量渲染（2D 像素坐标，UI 兼容） ---
    void render(const engine::ecs::World& world,
                uint32_t screenWidth, uint32_t screenHeight);

    // --- ECS 批量渲染（3D，深度测试） ---
    void render3D(const engine::ecs::World& world,
                  const glm::mat4& viewProj);

    // --- 独立精灵提交（2D） ---
    void submit(const SpriteDesc& desc);
    void flush(uint32_t screenWidth, uint32_t screenHeight);

    // --- 独立精灵提交（3D） ---
    void submit3D(const SpriteDesc& desc);
    void flush3D(const glm::mat4& viewProj);

private:
    void drawSprite(const SpriteDesc& desc,
                    uint32_t screenWidth, uint32_t screenHeight);

    // 3D shader 程序
    uint32_t m_shader3D     = 0;
    uint32_t m_uMVP         = 0;
    uint32_t m_uModel3D     = 0;
    uint32_t m_uColor3D     = 0;
    uint32_t m_uUseTex3D    = 0;
    uint32_t m_uTex3D       = 0;

    // 2D shader (unchanged)
    uint32_t m_vao          = 0;
    uint32_t m_vbo          = 0;
    uint32_t m_shaderProg   = 0;
    uint32_t m_uScreenSize   = 0;
    uint32_t m_uOffset       = 0;
    uint32_t m_uColor        = 0;
    uint32_t m_uSizeLoc      = 0;
    uint32_t m_uUseTexture   = 0;
    uint32_t m_uTexture      = 0;
    bool     m_initialized   = false;

    // 提交队列
    std::vector<SpriteDesc> m_submittedSprites;
    std::vector<SpriteDesc> m_submitted3DSprites;

    bool init3DShader();
};

} // namespace engine::render
