/**
 * @file engine/render/scene/scene_renderer.cpp
 * @brief 场景渲染器实现
 */

#include "engine/render/scene/scene_renderer.h"
#include "engine/render/scene/camera.h"
#include "engine/scene/scene.h"
#include "engine/scene/layer.h"
#include "engine/render/sprite.h"
#include "engine/scene/layer.h"

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
                            SpriteRenderer& spriteRenderer,
                            uint32_t screenWidth,
                            uint32_t screenHeight)
{
    // TODO Phase 3 Step 2: 渲染背景底图
    (void)m_drawBackground;

    if (m_drawLayers)
    {
        // 场景层已按深度排序（远→近），直接遍历
        for (const auto& layer : scene.layers)
        {
            if (!layer.visible) continue;
            renderLayer(layer, camera, spriteRenderer, screenWidth, screenHeight);
        }
    }
}

void SceneRenderer::renderLayer(const engine::scene::Layer& layer,
                                 const Camera& camera,
                                 SpriteRenderer& renderer,
                                 uint32_t screenWidth, uint32_t screenHeight)
{
    // 计算视差偏移后的屏幕位置
    glm::vec2 screenPos = camera.worldToScreen(
        layer.offset, layer.depth);

    // 如果层有顶点，计算包围盒
    // 当前简单实现：使用层的 offset 作为中心，纹理尺寸来自 SpriteRenderer
    // TODO: 从纹理获取实际尺寸，当前使用默认尺寸

    SpriteDesc desc;
    desc.textureHandle = UINT16_MAX; // TODO: 通过纹理管理器加载
    desc.position      = screenPos;
    desc.size          = { 100.0f, 100.0f }; // 默认尺寸，后续从纹理获取
    desc.depth         = layer.depth;
    desc.rotation      = layer.rotation;
    desc.tint          = { 1.0f, 1.0f, 1.0f, 1.0f };

    // 按深度映射到色调（便于调试：远=蓝，近=红）
    desc.tint = {
        0.3f + layer.depth * 0.7f,   // R: 0.3→1.0
        0.3f + (1.0f - layer.depth) * 0.7f, // G: 1.0→0.3
        0.5f,                         // B
        1.0f
    };

    renderer.submit(desc);
}

} // namespace engine::render
