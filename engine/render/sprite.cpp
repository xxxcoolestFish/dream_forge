/**
 * @file engine/render/sprite.cpp
 * @brief 精灵绘制实现（当前为桩，Phase 1 Step 2 完善）
 */

#include "engine/render/sprite.h"
#include <spdlog/spdlog.h>

namespace engine::render {

struct SpriteRenderer::Impl
{
    bool initialized = false;
};

SpriteRenderer::SpriteRenderer()
    : m_impl(new Impl())
{
    spdlog::debug("SpriteRenderer created");
}

SpriteRenderer::~SpriteRenderer()
{
    if (m_impl->initialized)
    {
        shutdown();
    }
    delete m_impl;
}

bool SpriteRenderer::init()
{
    spdlog::info("SpriteRenderer initializing...");
    // TODO Step 2: 编译/加载 sprite shader
    // TODO Step 2: 创建顶点布局
    m_impl->initialized = true;
    spdlog::info("SpriteRenderer initialized.");
    return true;
}

void SpriteRenderer::shutdown()
{
    if (!m_impl->initialized) return;
    // TODO Step 2: 释放 shader、顶点缓冲等
    m_impl->initialized = false;
}

void SpriteRenderer::submit(const SpriteDesc& desc)
{
    // TODO Step 2: 构建顶点 → transient buffer → bgfx::submit()
    (void)desc;
}

void SpriteRenderer::flush()
{
    // TODO Step 2: 实际的绘制调用
}

} // namespace engine::render
