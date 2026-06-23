/**
 * @file engine/render/texture.h
 * @brief 纹理资源管理 — Phase 6.1：OpenGL 纹理加载 + 绑定
 *
 * 负责：
 *   - 从文件/内存加载 PNG/JPG 等格式（通过 stb_image）
 *   - OpenGL 纹理对象创建与管理
 *   - 纹理绑定与参数设置
 *
 * 注意事项：
 *   - TextureHandle 直接使用 OpenGL 纹理 ID（uint32_t, 0 = 无效）
 *   - 当前仅支持 2D 纹理
 */

#pragma once

#include <cstdint>
#include <string>

namespace engine::render {

// 纹理句柄（直接使用 OpenGL 纹理 ID，0 表示无效纹理）
using TextureHandle = uint32_t;
constexpr TextureHandle kInvalidTexture = 0;

class Texture
{
public:
    Texture();
    ~Texture();

    // 从文件加载
    bool loadFromFile(const std::string& path);

    // 从内存加载（PNG/JPG 等编码数据）
    bool loadFromMemory(const uint8_t* data, uint32_t size);

    // 从原始像素数据上传（RGBA8）
    bool upload(const uint8_t* pixels, uint32_t width, uint32_t height, int channels);

    // 绑定到指定纹理单元
    void bind(int unit = 0) const;
    void unbind() const;

    // 释放纹理
    void release();

    // 获取信息
    TextureHandle handle() const { return m_handle; }
    uint32_t glId()       const { return m_handle; }
    uint32_t width()      const { return m_width; }
    uint32_t height()     const { return m_height; }
    bool isValid()        const { return m_handle != kInvalidTexture; }

private:
    TextureHandle m_handle = kInvalidTexture;
    uint32_t      m_width  = 0;
    uint32_t      m_height = 0;
};

} // namespace engine::render
