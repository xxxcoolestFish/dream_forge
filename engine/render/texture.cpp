/**
 * @file engine/render/texture.cpp
 * @brief 纹理管理实现（当前为桩，Phase 1 Step 2 完善）
 */

#include "engine/render/texture.h"
#include <spdlog/spdlog.h>

namespace engine::render {

Texture::Texture()
{
}

Texture::~Texture()
{
    if (isValid())
    {
        release();
    }
}

bool Texture::loadFromFile(const std::string& path)
{
    spdlog::debug("Texture::loadFromFile: {}", path);
    // TODO Step 2: 使用 bimg 解码图像 → bgfx::createTexture2D()
    return false;
}

bool Texture::loadFromMemory(const uint8_t* data, uint32_t size)
{
    if (!data || size == 0)
    {
        spdlog::error("Texture::loadFromMemory: null or empty data");
        return false;
    }
    // TODO Step 2: 从内存创建纹理
    return false;
}

void Texture::release()
{
    if (!isValid()) return;
    // TODO Step 2: bgfx::destroy()
    m_handle = kInvalidTexture;
}

} // namespace engine::render
