/**
 * @file engine/render/shader.h
 * @brief 简单的 OpenGL 着色器程序封装
 *
 * 负责编译顶点/片段着色器并链接为着色器程序。
 * 提供 uniform 设置接口（int/float/vec2/vec4/mat4）。
 *
 * Phase 6.1：uniformLocation/setMat4/setInt/setFloat/setVec4 已全部实现。
 */

#pragma once

#include <cstdint>
#include <string>
#include <glm/glm.hpp>

namespace engine::render {

class Shader
{
public:
    Shader();
    ~Shader();

    // 从字符串编译着色器
    bool compile(const std::string& vertexSrc, const std::string& fragmentSrc);

    // 释放着色器
    void release();

    // 使用此着色器
    void bind() const;
    void unbind() const;

    // 获取 uniform 位置
    int uniformLocation(const std::string& name) const;

    // 设置 uniform
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec2(const std::string& name, float x, float y) const;
    void setVec2(const std::string& name, const glm::vec2& v) const;
    void setVec4(const std::string& name, float x, float y, float z, float w) const;
    void setVec4(const std::string& name, const glm::vec4& v) const;
    void setMat4(const std::string& name, const float* data) const;

    bool isValid() const { return m_programId != 0; }

private:
    uint32_t m_programId = 0;
    uint32_t compileSingle(uint32_t type, const std::string& source);
};

} // namespace engine::render
