/**
 * @file engine/render/effects/post_effect.cpp
 * @brief 后处理特效基类实现
 */

#include "engine/render/effects/post_effect.h"
#include "engine/render/gl_loader.h"

#include <spdlog/spdlog.h>
#include <vector>

namespace engine::render::effects {

PostEffect::PostEffect(const std::string& name)
    : m_name(name)
{
}

PostEffect::~PostEffect() = default;

uint32_t PostEffect::compileShaderProgram(const char* vertSrc, const char* fragSrc)
{
    auto compileOne = [](uint32_t type, const char* src) -> uint32_t
    {
        uint32_t s = gl::CreateShader(type);
        gl::ShaderSource(s, 1, &src, nullptr);
        gl::CompileShader(s);
        int ok = 0;
        gl::GetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok)
        {
            int len = 0;
            gl::GetShaderiv(s, GL_INFO_LOG_LENGTH, &len);
            std::vector<char> log(static_cast<size_t>(len) + 1);
            gl::GetShaderInfoLog(s, len, nullptr, log.data());
            spdlog::error("PostEffect: shader compile error: {}", log.data());
            gl::DeleteShader(s);
            return 0;
        }
        return s;
    };

    uint32_t vs = compileOne(GL_VERTEX_SHADER, vertSrc);
    uint32_t fs = compileOne(GL_FRAGMENT_SHADER, fragSrc);
    if (!vs || !fs) { if (vs) gl::DeleteShader(vs); if (fs) gl::DeleteShader(fs); return 0; }

    uint32_t prog = gl::CreateProgram();
    gl::AttachShader(prog, vs);
    gl::AttachShader(prog, fs);
    gl::LinkProgram(prog);

    int linked = 0;
    gl::GetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        int len = 0;
        gl::GetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        std::vector<char> log(static_cast<size_t>(len) + 1);
        gl::GetProgramInfoLog(prog, len, nullptr, log.data());
        spdlog::error("PostEffect: shader link failed: {}", log.data());
        gl::DeleteProgram(prog);
        prog = 0;
    }

    gl::DeleteShader(vs);
    gl::DeleteShader(fs);
    return prog;
}

} // namespace engine::render::effects
