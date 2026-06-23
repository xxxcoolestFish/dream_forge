/**
 * @file engine/render/sprite.cpp
 * @brief 2D 精灵渲染器实现 — Phase 6.1：纹理采样支持
 *
 * 变更：
 *   - 顶点着色器新增 UV 传递（vTexCoord = aPos，单位四边形自然映射 0-1）
 *   - 片段着色器新增 uUseTexture 开关 + sampler2D 纹理采样
 *   - drawSprite() 根据 SpriteDesc.textureHandle 自动切换纯色/纹理模式
 */

#include "engine/render/sprite.h"
#include "engine/render/gl_loader.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"

#include <glm/gtc/matrix_transform.hpp>
#include <spdlog/spdlog.h>
#include <vector>

namespace engine::render {

// =========================================================================
// 内嵌着色器 — Phase 6.1：添加 UV 传递 + 纹理采样
// =========================================================================

static const char* kVertexShader = R"(
#version 330 core
layout(location = 0) in vec2 aPos;

uniform vec2 uScreenSize;
uniform vec2 uOffset;
uniform vec2 uSize;

out vec2 vTexCoord;

void main()
{
    // 单位四边形位置 (0,0)-(1,1) 直接作为纹理坐标
    vTexCoord = aPos;

    vec2 ndc;
    ndc.x = ((aPos.x * uSize.x + uOffset.x) / uScreenSize.x) * 2.0 - 1.0;
    ndc.y = 1.0 - ((aPos.y * uSize.y + uOffset.y) / uScreenSize.y) * 2.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
}
)";

static const char* kFragmentShader = R"(
#version 330 core

uniform vec4  uColor;
uniform bool  uUseTexture;
uniform sampler2D uTexture;

in  vec2 vTexCoord;
out vec4 FragColor;

void main()
{
    vec4 baseColor = uColor;
    if (uUseTexture)
    {
        baseColor *= texture(uTexture, vTexCoord);
    }
    FragColor = baseColor;
}
)";

// =========================================================================
// 构造 / 析构
// =========================================================================
SpriteRenderer::SpriteRenderer()
{
    spdlog::debug("SpriteRenderer created");
}

SpriteRenderer::~SpriteRenderer()
{
    if (m_initialized) shutdown();
}

// =========================================================================
// 初始化
// =========================================================================
bool SpriteRenderer::init()
{
    spdlog::info("SpriteRenderer initializing...");

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
            spdlog::error("SpriteRenderer: shader compile error: {}", log.data());
            gl::DeleteShader(s);
            return 0;
        }
        return s;
    };

    uint32_t vs = compileShader(GL_VERTEX_SHADER, kVertexShader);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, kFragmentShader);
    if (!vs || !fs) { if (vs) gl::DeleteShader(vs); if (fs) gl::DeleteShader(fs); return false; }

    m_shaderProg = gl::CreateProgram();
    gl::AttachShader(m_shaderProg, vs);
    gl::AttachShader(m_shaderProg, fs);
    gl::LinkProgram(m_shaderProg);

    int linked = 0;
    gl::GetProgramiv(m_shaderProg, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        int logLen = 0;
        gl::GetProgramiv(m_shaderProg, GL_INFO_LOG_LENGTH, &logLen);
        std::vector<char> log(static_cast<size_t>(logLen) + 1);
        gl::GetProgramInfoLog(m_shaderProg, logLen, nullptr, log.data());
        spdlog::error("SpriteRenderer: shader link failed: {}", log.data());
        gl::DeleteProgram(m_shaderProg); m_shaderProg = 0;
        return false;
    }
    gl::DeleteShader(vs);
    gl::DeleteShader(fs);

    // 获取所有 uniform 位置
    m_uScreenSize = static_cast<uint32_t>(gl::GetUniformLocation(m_shaderProg, "uScreenSize"));
    m_uOffset     = static_cast<uint32_t>(gl::GetUniformLocation(m_shaderProg, "uOffset"));
    m_uColor      = static_cast<uint32_t>(gl::GetUniformLocation(m_shaderProg, "uColor"));
    m_uSizeLoc    = static_cast<uint32_t>(gl::GetUniformLocation(m_shaderProg, "uSize"));
    m_uUseTexture = static_cast<uint32_t>(gl::GetUniformLocation(m_shaderProg, "uUseTexture"));
    m_uTexture    = static_cast<uint32_t>(gl::GetUniformLocation(m_shaderProg, "uTexture"));

    // 单位四边形顶点数据（2 三角形 = 6 顶点）
    float vertices[] = {
        0.0f, 0.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        0.0f, 0.0f,  1.0f, 1.0f,  0.0f, 1.0f,
    };
    gl::GenVertexArrays(1, &m_vao);
    gl::GenBuffers(1, &m_vbo);
    gl::BindVertexArray(m_vao);
    gl::BindBuffer(GL_ARRAY_BUFFER, m_vbo);
    gl::BufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
    gl::EnableVertexAttribArray(0);
    gl::BindVertexArray(0);

    // 3D shader
    if (!init3DShader())
    {
        spdlog::warn("SpriteRenderer: 3D shader init failed, 3D rendering disabled.");
    }

    m_initialized = true;
    spdlog::info("SpriteRenderer initialized (2D shader={}, 3D shader={}, vao={})",
        m_shaderProg, m_shader3D, m_vao);
    return true;
}

void SpriteRenderer::shutdown()
{
    if (!m_initialized) return;
    if (m_vao) { gl::DeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo) { gl::DeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_shaderProg) { gl::DeleteProgram(m_shaderProg); m_shaderProg = 0; }
    if (m_shader3D)   { gl::DeleteProgram(m_shader3D);   m_shader3D = 0; }
    m_initialized = false;
    spdlog::info("SpriteRenderer shutdown.");
}

// =========================================================================
// ECS 批量渲染（纯色模式，ECS Sprite 组件暂无纹理字段）
// =========================================================================
void SpriteRenderer::render(const engine::ecs::World& world,
                             uint32_t screenWidth, uint32_t screenHeight)
{
    if (!m_initialized) return;

    auto view = world.raw().view<ecs::Transform, ecs::Sprite>();
    if (view.begin() == view.end()) return;

    gl::UseProgram(m_shaderProg);
    gl::BindVertexArray(m_vao);
    gl::Uniform2f(static_cast<GLint>(m_uScreenSize),
        static_cast<float>(screenWidth), static_cast<float>(screenHeight));
    gl::Uniform1i(static_cast<GLint>(m_uUseTexture), 0); // ECS 路径纯色模式

    for (auto entity : view)
    {
        const auto& transform = world.raw().get<ecs::Transform>(entity);
        const auto& sprite    = world.raw().get<ecs::Sprite>(entity);
        if (!sprite.visible) continue;

        gl::Uniform2f(static_cast<GLint>(m_uOffset),
            transform.position.x, transform.position.y);
        gl::Uniform4f(static_cast<GLint>(m_uColor),
            sprite.tint.r, sprite.tint.g, sprite.tint.b, sprite.tint.a);
        gl::Uniform2f(static_cast<GLint>(m_uSizeLoc), 50.0f, 50.0f);

        gl::DrawArrays(GL_TRIANGLES, 0, 6);
    }

    gl::BindVertexArray(0);
    gl::UseProgram(0);
}

// =========================================================================
// 独立精灵提交
// =========================================================================
void SpriteRenderer::submit(const SpriteDesc& desc)
{
    m_submittedSprites.push_back(desc);
}

void SpriteRenderer::flush(uint32_t screenWidth, uint32_t screenHeight)
{
    if (!m_initialized || m_submittedSprites.empty()) return;

    gl::UseProgram(m_shaderProg);
    gl::BindVertexArray(m_vao);
    gl::Uniform2f(static_cast<GLint>(m_uScreenSize),
        static_cast<float>(screenWidth), static_cast<float>(screenHeight));

    for (const auto& desc : m_submittedSprites)
    {
        drawSprite(desc, screenWidth, screenHeight);
    }

    gl::BindVertexArray(0);
    gl::UseProgram(0);

    m_submittedSprites.clear();
}

void SpriteRenderer::drawSprite(const SpriteDesc& desc,
                                 uint32_t screenWidth, uint32_t screenHeight)
{
    (void)screenWidth; (void)screenHeight;

    // 位置 + 尺寸
    gl::Uniform2f(static_cast<GLint>(m_uOffset),
        desc.position.x, desc.position.y);
    gl::Uniform2f(static_cast<GLint>(m_uSizeLoc),
        desc.size.x, desc.size.y);

    // 颜色
    gl::Uniform4f(static_cast<GLint>(m_uColor),
        desc.tint.r, desc.tint.g, desc.tint.b, desc.tint.a);

    // 纹理采样
    bool hasTexture = (desc.textureHandle != kInvalidTexture);
    gl::Uniform1i(static_cast<GLint>(m_uUseTexture), hasTexture ? 1 : 0);

    if (hasTexture)
    {
        // 绑定纹理到单元 0
        gl::ActiveTexture(GL_TEXTURE0);
        gl::BindTexture(GL_TEXTURE_2D, desc.textureHandle);
        gl::Uniform1i(static_cast<GLint>(m_uTexture), 0);
    }

    gl::DrawArrays(GL_TRIANGLES, 0, 6);

    // 解绑纹理
    if (hasTexture)
    {
        gl::BindTexture(GL_TEXTURE_2D, 0);
    }
}

// =========================================================================
// 3D MVP 渲染路径 — Phase 7
// =========================================================================

static const char* kVertex3D = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
uniform mat4 uModel;
uniform mat4 uVP;
out vec2 vTexCoord;
void main()
{
    vTexCoord = aPos;
    gl_Position = uVP * uModel * vec4(aPos.x, aPos.y, 0.0, 1.0);
}
)";

static const char* kFragment3D = R"(
#version 330 core
uniform vec4      uColor;
uniform bool      uUseTexture;
uniform sampler2D uTexture;
in  vec2 vTexCoord;
out vec4 FragColor;
void main()
{
    vec4 base = uColor;
    if (uUseTexture)
        base *= texture(uTexture, vTexCoord);
    FragColor = base;
}
)";

bool SpriteRenderer::init3DShader()
{
    auto compileOne = [](uint32_t type, const char* src) -> uint32_t
    {
        uint32_t s = gl::CreateShader(type);
        gl::ShaderSource(s, 1, &src, nullptr);
        gl::CompileShader(s);
        int ok = 0;
        gl::GetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { gl::DeleteShader(s); return 0; }
        return s;
    };

    uint32_t vs = compileOne(GL_VERTEX_SHADER, kVertex3D);
    uint32_t fs = compileOne(GL_FRAGMENT_SHADER, kFragment3D);
    if (!vs || !fs) { if (vs) gl::DeleteShader(vs); if (fs) gl::DeleteShader(fs); return false; }

    m_shader3D = gl::CreateProgram();
    gl::AttachShader(m_shader3D, vs);
    gl::AttachShader(m_shader3D, fs);
    gl::LinkProgram(m_shader3D);
    gl::DeleteShader(vs);
    gl::DeleteShader(fs);

    m_uMVP      = static_cast<uint32_t>(gl::GetUniformLocation(m_shader3D, "uVP"));
    m_uModel3D  = static_cast<uint32_t>(gl::GetUniformLocation(m_shader3D, "uModel"));
    m_uColor3D  = static_cast<uint32_t>(gl::GetUniformLocation(m_shader3D, "uColor"));
    m_uUseTex3D = static_cast<uint32_t>(gl::GetUniformLocation(m_shader3D, "uUseTexture"));
    m_uTex3D    = static_cast<uint32_t>(gl::GetUniformLocation(m_shader3D, "uTexture"));

    return m_shader3D != 0;
}

void SpriteRenderer::submit3D(const SpriteDesc& desc)
{
    m_submitted3DSprites.push_back(desc);
}

void SpriteRenderer::flush3D(const glm::mat4& viewProj)
{
    if (!m_initialized || !m_shader3D || m_submitted3DSprites.empty()) return;

    gl::UseProgram(m_shader3D);
    gl::BindVertexArray(m_vao);

    // VP 矩阵每帧传一次
    gl::UniformMatrix4fv(static_cast<GLint>(m_uMVP), 1, GL_FALSE, &viewProj[0][0]);

    for (const auto& desc : m_submitted3DSprites)
    {
        // 构建模型矩阵：缩放→平移（暂时跳过旋转）
        glm::mat4 model(1.0f);
        model = glm::translate(model, desc.position);
        model = glm::scale(model, glm::vec3(desc.size.x, desc.size.y, 1.0f));

        gl::UniformMatrix4fv(static_cast<GLint>(m_uModel3D), 1, GL_FALSE, &model[0][0]);
        gl::Uniform4f(static_cast<GLint>(m_uColor3D),
            desc.tint.r, desc.tint.g, desc.tint.b, desc.tint.a);

        bool hasTex = (desc.textureHandle != kInvalidTexture);
        gl::Uniform1i(static_cast<GLint>(m_uUseTex3D), hasTex ? 1 : 0);
        if (hasTex)
        {
            gl::ActiveTexture(GL_TEXTURE0);
            gl::BindTexture(GL_TEXTURE_2D, desc.textureHandle);
            gl::Uniform1i(static_cast<GLint>(m_uTex3D), 0);
        }

        gl::DrawArrays(GL_TRIANGLES, 0, 6);

        if (hasTex)
            gl::BindTexture(GL_TEXTURE_2D, 0);
    }

    gl::BindVertexArray(0);
    gl::UseProgram(0);
    m_submitted3DSprites.clear();
}

void SpriteRenderer::render3D(const engine::ecs::World& world,
                               const glm::mat4& viewProj)
{
    if (!m_initialized || !m_shader3D) return;

    auto view = world.raw().view<ecs::Transform, ecs::Sprite>();
    if (view.begin() == view.end()) return;

    gl::UseProgram(m_shader3D);
    gl::BindVertexArray(m_vao);
    gl::UniformMatrix4fv(static_cast<GLint>(m_uMVP), 1, GL_FALSE, &viewProj[0][0]);
    gl::Uniform1i(static_cast<GLint>(m_uUseTex3D), 0);

    for (auto entity : view)
    {
        const auto& t = world.raw().get<ecs::Transform>(entity);
        const auto& s = world.raw().get<ecs::Sprite>(entity);
        if (!s.visible) continue;

        glm::mat4 model(1.0f);
        model = glm::translate(model, t.position);
        model = glm::scale(model, glm::vec3(50.0f, 50.0f, 1.0f));

        gl::UniformMatrix4fv(static_cast<GLint>(m_uModel3D), 1, GL_FALSE, &model[0][0]);
        gl::Uniform4f(static_cast<GLint>(m_uColor3D),
            s.tint.r, s.tint.g, s.tint.b, s.tint.a);
        gl::DrawArrays(GL_TRIANGLES, 0, 6);
    }

    gl::BindVertexArray(0);
    gl::UseProgram(0);
}

} // namespace engine::render
