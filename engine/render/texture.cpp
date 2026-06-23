/**
 * @file engine/render/texture.cpp
 * @brief 纹理管理实现 — Phase 6.1：stb_image 加载 + OpenGL 纹理
 */

#include "engine/render/texture.h"
#include "engine/render/gl_loader.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

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

    int w = 0, h = 0, channels = 0;
    // 强制加载为 RGBA8
    unsigned char* pixels = stbi_load(path.c_str(), &w, &h, &channels, 4);
    if (!pixels)
    {
        spdlog::error("Texture::loadFromFile: failed to load '{}': {}", path, stbi_failure_reason());
        return false;
    }

    bool ok = upload(pixels, static_cast<uint32_t>(w), static_cast<uint32_t>(h), 4);
    stbi_image_free(pixels);
    return ok;
}

bool Texture::loadFromMemory(const uint8_t* data, uint32_t size)
{
    if (!data || size == 0)
    {
        spdlog::error("Texture::loadFromMemory: null or empty data");
        return false;
    }

    int w = 0, h = 0, channels = 0;
    unsigned char* pixels = stbi_load_from_memory(
        data, static_cast<int>(size), &w, &h, &channels, 4);
    if (!pixels)
    {
        spdlog::error("Texture::loadFromMemory: {}", stbi_failure_reason());
        return false;
    }

    bool ok = upload(pixels, static_cast<uint32_t>(w), static_cast<uint32_t>(h), 4);
    stbi_image_free(pixels);
    return ok;
}

bool Texture::upload(const uint8_t* pixels, uint32_t width, uint32_t height, int channels)
{
    if (!pixels || width == 0 || height == 0)
    {
        spdlog::error("Texture::upload: invalid parameters");
        return false;
    }

    // 如果已有纹理，先释放
    if (isValid())
    {
        release();
    }

    gl::GenTextures(1, &m_handle);
    if (m_handle == 0)
    {
        spdlog::error("Texture::upload: glGenTextures failed");
        return false;
    }

    gl::BindTexture(GL_TEXTURE_2D, m_handle);

    // 上传像素数据
    GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
    gl::TexImage2D(GL_TEXTURE_2D, 0, static_cast<GLint>(format),
        static_cast<GLsizei>(width), static_cast<GLsizei>(height),
        0, format, GL_UNSIGNED_BYTE, pixels);

    // 设置默认采样参数
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    gl::BindTexture(GL_TEXTURE_2D, 0);

    m_width  = width;
    m_height = height;

    spdlog::info("Texture: uploaded {}x{} (GL id={}, {} channels)", width, height, m_handle, channels);
    return true;
}

void Texture::bind(int unit) const
{
    if (!isValid()) return;
    gl::ActiveTexture(static_cast<GLenum>(GL_TEXTURE0 + unit));
    gl::BindTexture(GL_TEXTURE_2D, m_handle);
}

void Texture::unbind() const
{
    gl::BindTexture(GL_TEXTURE_2D, 0);
}

void Texture::release()
{
    if (!isValid()) return;
    gl::DeleteTextures(1, &m_handle);
    spdlog::info("Texture: released GL id={}", m_handle);
    m_handle = kInvalidTexture;
    m_width  = 0;
    m_height = 0;
}

} // namespace engine::render
