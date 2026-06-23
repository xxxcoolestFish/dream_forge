/**
 * @file engine/render/scene/scene_renderer.cpp
 * @brief 场景渲染器 — Phase 7: 3D 透视层渲染
 */

#include "engine/render/scene/scene_renderer.h"
#include "engine/render/scene/camera.h"
#include "engine/scene/scene.h"
#include "engine/scene/layer.h"
#include "engine/render/sprite.h"

#include <spdlog/spdlog.h>

namespace engine::render {

SceneRenderer::SceneRenderer()
{
    spdlog::debug("SceneRenderer created");
}

SceneRenderer::~SceneRenderer()
{
    spdlog::debug("SceneRenderer destroyed");
}

void SceneRenderer::render(const engine::scene::Scene& scene,
                            const Camera& camera,
                            SpriteRenderer& spriteRenderer)
{
    if (m_drawLayers)
    {
        for (const auto& layer : scene.layers)
        {
            if (!layer.visible) continue;
            renderLayer(layer, camera, spriteRenderer);
        }
    }

    // 提交并绘制
    spriteRenderer.flush3D(camera.viewProjection());
}

void SceneRenderer::renderLayer(const engine::scene::Layer& layer,
                                 const Camera& camera,
                                 SpriteRenderer& renderer)
{
    // 将深度值映射为 Z 坐标
    float z = camera.depthToZ(layer.depth);

    // 在该 Z 处填满视锥体所需的四边形尺寸
    glm::vec2 fillSize = camera.fillSizeAtZ(z);

    // 应用层的 scale
    glm::vec2 layerSize = fillSize * layer.scale;

    // 层的世界坐标: offset.x 为 X, offset.y 为 Y, Z = depthToZ
    glm::vec3 worldPos(
        layer.offset.x,
        layer.offset.y,
        z
    );

    SpriteDesc desc;
    desc.textureHandle = kInvalidTexture;
    desc.position      = worldPos;
    desc.size          = layerSize;
    desc.rotation      = layer.rotation;

    // 按深度映射到色调（便于调试：远=冷蓝, 近=暖红）
    float t = layer.depth;
    desc.tint = {
        0.25f + t * 0.75f,
        0.3f  + (1.0f - t) * 0.7f,
        0.55f + (1.0f - t) * 0.3f,
        1.0f
    };

    renderer.submit3D(desc);
}

} // namespace engine::render
