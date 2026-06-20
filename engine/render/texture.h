/**
 * @file engine/render/texture.h
 * @brief 纹理资源管理
 *
 * 负责：
 *   - 从文件/内存加载纹理
 *   - 纹理句柄管理
 *   - 纹理资源释放
 *
 * 注意事项：
 *   - 使用 bgfx 的纹理句柄
 *   - 纹理数据在渲染线程创建，加载时注意线程安全
 *   - 支持 PNG、JPG、DDS 等常见格式（通过 bimg 解码）
 */

#pragma once

#include <cstdint>
#include <string>
#include <memory>

namespace engine::render {

// 纹理句柄（封装 bgfx 纹理句柄）
using TextureHandle = uint16_t;
constexpr TextureHandle kInvalidTexture = UINT16_MAX;

class Texture
{
public:
    Texture();
    ~Texture();

    // 从文件加载
    bool loadFromFile(const std::string& path);

    // 从内存加载
    bool loadFromMemory(const uint8_t* data, uint32_t size);

    // 释放纹理
    void release();

    // 获取信息
    TextureHandle handle() const { return m_handle; }
    uint32_t width()  const { return m_width; }
    uint32_t height() const { return m_height; }
    bool isValid()    const { return m_handle != kInvalidTexture; }

private:
    TextureHandle m_handle = kInvalidTexture;
    uint32_t      m_width  = 0;
    uint32_t      m_height = 0;
};

} // namespace engine::render
