/**
 * @file engine/render/shader.cpp
 * @brief 着色器编译和链接实现 — Phase 6.1：uniform 方法全部实现
 */

#include "engine/render/shader.h"
#include "engine/render/gl_loader.h"

#include <spdlog/spdlog.h>
#include <vector>

namespace engine::render {

Shader::Shader()  = default;
Shader::~Shader() { release(); }

bool Shader::compile(const std::string& vertexSrc, const std::string& fragmentSrc)
{
    uint32_t vs = compileSingle(GL_VERTEX_SHADER, vertexSrc);
    if (vs == 0) return false;

    uint32_t fs = compileSingle(GL_FRAGMENT_SHADER, fragmentSrc);
    if (fs == 0) { gl::DeleteShader(vs); return false; }

    // 链接程序
    m_programId = gl::CreateProgram();
    gl::AttachShader(m_programId, vs);
    gl::AttachShader(m_programId, fs);
    gl::LinkProgram(m_programId);

    // 检查链接状态
    int linked = 0;
    gl::GetProgramiv(m_programId, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        int logLen = 0;
        gl::GetProgramiv(m_programId, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(static_cast<size_t>(logLen) + 1);
        gl::GetProgramInfoLog(m_programId, logLen, nullptr, log.data());
        spdlog::error("Shader::compile: link failed: {}", log.data());
        gl::DeleteProgram(m_programId);
        m_programId = 0;
    }

    // 清理着色器（已链接到程序，不再需要）
    gl::DeleteShader(vs);
    gl::DeleteShader(fs);

    spdlog::debug("Shader: program {} linked successfully", m_programId);
    return m_programId != 0;
}

uint32_t Shader::compileSingle(uint32_t type, const std::string& source)
{
    uint32_t shader = gl::CreateShader(type);
    const char* src = source.c_str();
    gl::ShaderSource(shader, 1, &src, nullptr);
    gl::CompileShader(shader);

    int compiled = 0;
    gl::GetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        int logLen = 0;
        gl::GetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(static_cast<size_t>(logLen) + 1);
        gl::GetShaderInfoLog(shader, logLen, nullptr, log.data());
        const char* typeName = (type == GL_VERTEX_SHADER) ? "vertex" : "fragment";
        spdlog::error("Shader::compile: {} shader failed: {}", typeName, log.data());
        gl::DeleteShader(shader);
        return 0;
    }

    return shader;
}

void Shader::release()
{
    if (m_programId)
    {
        gl::DeleteProgram(m_programId);
        m_programId = 0;
    }
}

void Shader::bind() const
{
    gl::UseProgram(m_programId);
}

void Shader::unbind() const
{
    gl::UseProgram(0);
}

int Shader::uniformLocation(const std::string& name) const
{
    if (!m_programId) return -1;
    return gl::GetUniformLocation(m_programId, name.c_str());
}

void Shader::setInt(const std::string& name, int value) const
{
    int loc = uniformLocation(name);
    if (loc >= 0) gl::Uniform1i(loc, value);
}

void Shader::setFloat(const std::string& name, float value) const
{
    int loc = uniformLocation(name);
    if (loc >= 0) gl::Uniform1f(loc, value);
}

void Shader::setVec2(const std::string& name, float x, float y) const
{
    int loc = uniformLocation(name);
    if (loc >= 0) gl::Uniform2f(loc, x, y);
}

void Shader::setVec2(const std::string& name, const glm::vec2& v) const
{
    setVec2(name, v.x, v.y);
}

void Shader::setVec4(const std::string& name, float x, float y, float z, float w) const
{
    int loc = uniformLocation(name);
    if (loc >= 0) gl::Uniform4f(loc, x, y, z, w);
}

void Shader::setVec4(const std::string& name, const glm::vec4& v) const
{
    setVec4(name, v.x, v.y, v.z, v.w);
}

void Shader::setMat4(const std::string& name, const float* data) const
{
    int loc = uniformLocation(name);
    if (loc >= 0) gl::UniformMatrix4fv(loc, 1, GL_FALSE, data);
}

} // namespace engine::render
