/**
 * @file engine/ui/font.h
 * @brief 位图字体 — stb_truetype 加载 .ttf → 纹理图集
 *
 * 预烘焙 ASCII 32-126 字符到一张 OpenGL 纹理。
 * 提供字形度量数据供 Text 控件使用。
 */

#pragma once

#include <string>
#include <unordered_map>
#include <glm/glm.hpp>

namespace engine::ui {

// 单个字形信息
struct Glyph
{
    float u0, v0, u1, v1;  // 纹理坐标
    float w, h;             // 字形宽高（像素）
    float bearingX, bearingY; // 基线偏移
    float advanceX;          // 水平步进
};

class Font
{
public:
    Font();
    ~Font();

    // 从 .ttf 文件加载，生成指定像素大小的字形
    bool load(const std::string& ttfPath, float pixelHeight = 24.0f);

    // 释放
    void release();

    // 获取字形
    const Glyph* glyph(char c) const;

    // 获取字形的纹理
    uint32_t textureId() const { return m_textureId; }

    // 字体度量
    float pixelHeight() const { return m_pixelHeight; }
    float lineSpacing()  const { return m_lineSpacing; }

    // 计算字符串宽度
    float textWidth(const std::string& text) const;

private:
    float                  m_pixelHeight  = 24.0f;
    float                  m_lineSpacing  = 28.0f;
    uint32_t               m_textureId    = 0;
    uint32_t               m_texWidth     = 512;
    uint32_t               m_texHeight    = 512;
    std::unordered_map<char, Glyph> m_glyphs;
};

} // namespace engine::ui
