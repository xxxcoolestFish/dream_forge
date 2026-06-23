/**
 * @file engine/render/scene/scene_renderer.h
 * @brief 场景渲染器 — 根据 Scene 数据 + Camera 进行视差渲染
 *
 * 负责：
 *   1. 渲染背景底图
 *   2. 按深度排序渲染各层（远→近），应用视差偏移
 *   3. （后续）前景特效、粒子
 *
 * 依赖：
 *   - engine::scene::Scene — 场景数据
 *   - engine::render::Camera — 相机状态
 *   - engine::render::SpriteRenderer — 精灵绘制
 *
 * 设计：
 *   - 每帧调用 render(scene, camera, spriteRenderer)
 *   - 不拥有纹理/资源，只读取 Scene 数据并调度绘制
 *
 * 注意事项：
 *   - 当前 Phase 3：每层一个 draw call
 *   - 后续优化：纹理图集 + 批处理
 */

#pragma once

#include <memory>
#include <cstdint>

namespace engine::scene {
    struct Scene;
    struct Layer;
}

namespace engine::render {

class Camera;
class SpriteRenderer;

class SceneRenderer
{
public:
    SceneRenderer();
    ~SceneRenderer();

    // 渲染整个场景（3D 透视模式）
    void render(const engine::scene::Scene& scene,
                const Camera& camera,
                SpriteRenderer& spriteRenderer);

    // 启用/禁用背景渲染
    void setDrawBackground(bool draw) { m_drawBackground = draw; }
    void setDrawLayers(bool draw)     { m_drawLayers = draw; }

private:
    // 渲染单个层（3D 空间）
    void renderLayer(const engine::scene::Layer& layer,
                     const Camera& camera,
                     SpriteRenderer& renderer);

    bool m_drawBackground = true;
    bool m_drawLayers     = true;
};

} // namespace engine::render
