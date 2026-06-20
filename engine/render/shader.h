/**
 * @file engine/render/shader.h
 * @brief 简单的 OpenGL 着色器程序封装
 *
 * 负责编译顶点/片段着色器并链接为着色器程序。
 * Phase 1 着色器作为内嵌字符串，无需从文件加载。
 */

#pragma once

#include <cstdint>
#include <string>

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
    void setMat4(const std::string& name, const float* data) const;

    bool isValid() const { return m_programId != 0; }

private:
    uint32_t m_programId = 0;
    uint32_t compileSingle(uint32_t type, const std::string& source);
};

} // namespace engine::render
