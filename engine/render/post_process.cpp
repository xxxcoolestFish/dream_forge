/**
 * @file engine/render/post_process.cpp
 * @brief 后处理基座实现 — 全屏四边形直通渲染
 *
 * Phase 6.3: 简单采样 FBO 颜色纹理绘到屏幕。
 * Phase 6.4: 替换为特效链。
 */

#include "engine/render/post_process.h"
#include "engine/render/render_target.h"
#include "engine/render/gl_loader.h"

#include <spdlog/spdlog.h>
#include <vector>

namespace engine::render {

// =========================================================================
// 全屏四边形 shader（直通：采样纹理 → 输出）
// =========================================================================
static const char* kFullscreenVert = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main()
{
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

static const char* kFullscreenFrag = R"(
#version 330 core
in vec2 vTexCoord;
uniform sampler2D uScreenTexture;
out vec4 FragColor;
void main()
{
    FragColor = texture(uScreenTexture, vTexCoord);
}
)";

// =========================================================================
PostProcess::PostProcess()
{
    spdlog::debug("PostProcess: created");
}

PostProcess::~PostProcess()
{
    if (m_initialized) shutdown();
}

bool PostProcess::init()
{
    spdlog::info("PostProcess: initializing (passthrough)...");

    // 编译 shader
    auto compileShader = [](uint32_t type, const char* src) -> uint32_t
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
            spdlog::error("PostProcess: shader compile error: {}", log.data());
            gl::DeleteShader(s);
            return 0;
        }
        return s;
    };

    uint32_t vs = compileShader(GL_VERTEX_SHADER, kFullscreenVert);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, kFullscreenFrag);
    if (!vs || !fs) { if (vs) gl::DeleteShader(vs); if (fs) gl::DeleteShader(fs); return false; }

    m_shaderProg = gl::CreateProgram();
    gl::AttachShader(m_shaderProg, vs);
    gl::AttachShader(m_shaderProg, fs);
    gl::LinkProgram(m_shaderProg);

    int linked = 0;
    gl::GetProgramiv(m_shaderProg, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        spdlog::error("PostProcess: shader link failed");
        gl::DeleteProgram(m_shaderProg); m_shaderProg = 0;
        return false;
    }
    gl::DeleteShader(vs);
    gl::DeleteShader(fs);

    m_uTexture = static_cast<uint32_t>(gl::GetUniformLocation(m_shaderProg, "uScreenTexture"));

    // 全屏四边形顶点（NDC pos + UV）
    float vertices[] = {
        // pos(x,y)      tex(u,v)
        -1.0f,  1.0f,    0.0f, 1.0f,   // 左上
        -1.0f, -1.0f,    0.0f, 0.0f,   // 左下
         1.0f, -1.0f,    1.0f, 0.0f,   // 右下

        -1.0f,  1.0f,    0.0f, 1.0f,   // 左上
         1.0f, -1.0f,    1.0f, 0.0f,   // 右下
         1.0f,  1.0f,    1.0f, 1.0f,   // 右上
    };

    gl::GenVertexArrays(1, &m_vao);
    gl::GenBuffers(1, &m_vbo);
    gl::BindVertexArray(m_vao);
    gl::BindBuffer(GL_ARRAY_BUFFER, m_vbo);
    gl::BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // pos
    gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    gl::EnableVertexAttribArray(0);
    // texcoord
    gl::VertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    gl::EnableVertexAttribArray(1);

    gl::BindVertexArray(0);

    m_initialized = true;
    spdlog::info("PostProcess: initialized (passthrough shader={})", m_shaderProg);
    return true;
}

void PostProcess::shutdown()
{
    if (!m_initialized) return;
    if (m_vbo) { gl::DeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_vao) { gl::DeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_shaderProg) { gl::DeleteProgram(m_shaderProg); m_shaderProg = 0; }
    m_initialized = false;
    spdlog::info("PostProcess: shutdown");
}

void PostProcess::apply(const RenderTarget& source,
                         uint32_t screenWidth, uint32_t screenHeight)
{
    if (!m_initialized) return;

    gl::Viewport(0, 0, static_cast<GLsizei>(screenWidth), static_cast<GLsizei>(screenHeight));
    gl::UseProgram(m_shaderProg);
    gl::BindVertexArray(m_vao);

    // 绑定 FBO 颜色纹理到单元 0
    source.bindTexture(0);
    gl::Uniform1i(static_cast<GLint>(m_uTexture), 0);

    gl::DrawArrays(GL_TRIANGLES, 0, 6);

    gl::BindVertexArray(0);
    gl::UseProgram(0);
}

} // namespace engine::render
