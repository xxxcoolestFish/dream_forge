/**
 * @file engine/render/sprite.h
 * @brief 2D 精灵渲染器 — 将 ECS 中 Sprite+Transform 渲染到屏幕
 *
 * Phase 1：简单彩色矩形渲染（无纹理），每个 Entity 一个 draw call。
 * 后续 Phase：批处理、纹理支持、深度排序。
 */

#pragma once

#include <cstdint>
#include <glm/glm.hpp>

namespace engine::ecs {
    class World;
    struct Transform;
    struct Sprite;
}

namespace engine::render {

class SpriteRenderer
{
public:
    SpriteRenderer();
    ~SpriteRenderer();

    // 初始化（编译着色器、创建 VAO/VBO）
    bool init();

    // 关闭
    void shutdown();

    // 每帧渲染所有可见精灵
    void render(const engine::ecs::World& world,
                uint32_t screenWidth, uint32_t screenHeight);

private:
    uint32_t m_vao        = 0;  // 顶点数组
    uint32_t m_vbo        = 0;  // 顶点缓冲
    uint32_t m_shaderProg = 0;  // 着色器程序
    uint32_t m_uScreenSize = 0; // screen size uniform location
    uint32_t m_uOffset     = 0; // position offset uniform
    uint32_t m_uColor      = 0; // color uniform
    bool     m_initialized = false;
};

} // namespace engine::render
