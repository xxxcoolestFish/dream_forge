/**
 * @file engine/ui/font.cpp
 * @brief 位图字体实现 — stb_truetype 加载 .ttf → 纹理
 */

#include "engine/ui/font.h"
#include "engine/render/gl_loader.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include <stb_truetype.h>

#include <spdlog/spdlog.h>
#include <fstream>
#include <vector>
#include <cstring>

namespace engine::ui {
namespace gl = engine::render::gl;

Font::Font()  = default;
Font::~Font() { release(); }

bool Font::load(const std::string& ttfPath, float pixelHeight)
{
    // 读取 .ttf 文件
    std::ifstream file(ttfPath, std::ios::binary | std::ios::ate);
    if (!file.is_open())
    {
        spdlog::error("Font::load: cannot open '{}'", ttfPath);
        return false;
    }

    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<unsigned char> ttfData(static_cast<size_t>(size));
    if (!file.read(reinterpret_cast<char*>(ttfData.data()), size))
    {
        spdlog::error("Font::load: read error");
        return false;
    }

    // 初始化 stb_truetype
    stbtt_fontinfo fontInfo;
    if (!stbtt_InitFont(&fontInfo, ttfData.data(), 0))
    {
        spdlog::error("Font::load: stbtt_InitFont failed");
        return false;
    }

    m_pixelHeight = pixelHeight;
    float scale = stbtt_ScaleForPixelHeight(&fontInfo, pixelHeight);

    // 获取字体度量
    int ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&fontInfo, &ascent, &descent, &lineGap);
    m_lineSpacing = (ascent - descent + lineGap) * scale;

    // 创建纹理图集
    m_texWidth  = 512;
    m_texHeight = 512;
    std::vector<unsigned char> texData(m_texWidth * m_texHeight, 0);

    int penX = 2, penY = 2;
    int maxRowH = 0;

    // 烘焙 ASCII 32-126
    for (int c = 32; c <= 126; ++c)
    {
        // 获取字形位图
        int w, h, xoff, yoff;
        unsigned char* bitmap = stbtt_GetCodepointBitmap(
            &fontInfo, scale, scale, c, &w, &h, &xoff, &yoff);

        if (!bitmap) continue;

        // 换行
        if (penX + w + 2 > static_cast<int>(m_texWidth))
        {
            penX = 2;
            penY += maxRowH + 2;
            maxRowH = 0;
        }

        if (penY + h + 2 > static_cast<int>(m_texHeight))
        {
            spdlog::warn("Font: texture atlas overflow at char '{}'", static_cast<char>(c));
            stbtt_FreeBitmap(bitmap, nullptr);
            break;
        }

        // 复制到图集
        for (int row = 0; row < h; ++row)
        {
            std::memcpy(
                texData.data() + (penY + row) * m_texWidth + penX,
                bitmap + row * w,
                static_cast<size_t>(w));
        }

        // 记录字形信息
        Glyph glyph;
        glyph.u0 = static_cast<float>(penX) / static_cast<float>(m_texWidth);
        glyph.v0 = static_cast<float>(penY) / static_cast<float>(m_texHeight);
        glyph.u1 = static_cast<float>(penX + w) / static_cast<float>(m_texWidth);
        glyph.v1 = static_cast<float>(penY + h) / static_cast<float>(m_texHeight);
        glyph.w  = static_cast<float>(w);
        glyph.h  = static_cast<float>(h);
        glyph.bearingX = static_cast<float>(xoff);
        glyph.bearingY = static_cast<float>(yoff);

        int advance;
        stbtt_GetCodepointHMetrics(&fontInfo, c, &advance, nullptr);
        glyph.advanceX = advance * scale;

        m_glyphs[static_cast<char>(c)] = glyph;

        stbtt_FreeBitmap(bitmap, nullptr);

        penX += w + 2;
        if (h > maxRowH) maxRowH = h;
    }

    // 创建 OpenGL 纹理
    gl::GenTextures(1, &m_textureId);
    gl::BindTexture(GL_TEXTURE_2D, m_textureId);
    gl::TexImage2D(GL_TEXTURE_2D, 0, GL_RED, static_cast<int>(m_texWidth),
        static_cast<int>(m_texHeight), 0, GL_RED, GL_UNSIGNED_BYTE,
        texData.data());
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    gl::TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    spdlog::info("Font: loaded '{}' ({} chars, {}x{} texture, lineH={:.1f})",
        ttfPath, m_glyphs.size(), m_texWidth, m_texHeight, m_lineSpacing);
    return true;
}

void Font::release()
{
    if (m_textureId)
    {
        gl::DeleteTextures(1, &m_textureId);
        m_textureId = 0;
    }
    m_glyphs.clear();
}

const Glyph* Font::glyph(char c) const
{
    auto it = m_glyphs.find(c);
    return (it != m_glyphs.end()) ? &it->second : nullptr;
}

float Font::textWidth(const std::string& text) const
{
    float w = 0;
    for (char c : text)
    {
        const Glyph* g = glyph(c);
        w += g ? g->advanceX : m_pixelHeight * 0.5f;
    }
    return w;
}

} // namespace engine::ui
