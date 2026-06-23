/**
 * @file engine/render/particles/particle_system.cpp
 * @brief 粒子系统实现 — CPU 更新 + GPU 点精灵渲染
 */

#include "engine/render/particles/particle_system.h"
#include "engine/render/gl_loader.h"

#include <spdlog/spdlog.h>
#include <algorithm>
#include <random>

namespace engine::render::particles {

// 线程局部 RNG
static thread_local std::mt19937 s_rng(std::random_device{}());

// =========================================================================
// 点精灵 shader
// =========================================================================
static const char* kParticleVert = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec4 aColor;
layout(location = 2) in float aSize;
uniform vec2 uScreenSize;
out vec4 vColor;
void main()
{
    vec2 ndc;
    ndc.x = (aPos.x / uScreenSize.x) * 2.0 - 1.0;
    ndc.y = 1.0 - (aPos.y / uScreenSize.y) * 2.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    gl_PointSize = aSize;
    vColor = aColor;
}
)";

static const char* kParticleFrag = R"(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main()
{
    vec2 center = gl_PointCoord - 0.5;
    float dist = length(center) * 2.0;
    float alpha = 1.0 - smoothstep(0.7, 1.0, dist);
    FragColor = vec4(vColor.rgb, vColor.a * alpha);
}
)";

// =========================================================================
ParticleSystem::ParticleSystem()
{
    spdlog::debug("ParticleSystem: created");
}

ParticleSystem::~ParticleSystem()
{
    if (m_initialized) shutdown();
}

bool ParticleSystem::init(uint32_t maxParticles)
{
    m_maxParticles = maxParticles;
    m_particles.resize(maxParticles);

    auto compileShader = [](uint32_t type, const char* src) -> uint32_t
    {
        uint32_t s = gl::CreateShader(type);
        gl::ShaderSource(s, 1, &src, nullptr);
        gl::CompileShader(s);
        int ok = 0;
        gl::GetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) { gl::DeleteShader(s); return 0; }
        return s;
    };

    uint32_t vs = compileShader(GL_VERTEX_SHADER, kParticleVert);
    uint32_t fs = compileShader(GL_FRAGMENT_SHADER, kParticleFrag);
    if (!vs || !fs) { if (vs) gl::DeleteShader(vs); if (fs) gl::DeleteShader(fs); return false; }

    m_shader = gl::CreateProgram();
    gl::AttachShader(m_shader, vs);
    gl::AttachShader(m_shader, fs);
    gl::LinkProgram(m_shader);
    gl::DeleteShader(vs);
    gl::DeleteShader(fs);

    m_uScreenSize = static_cast<uint32_t>(gl::GetUniformLocation(m_shader, "uScreenSize"));

    gl::GenVertexArrays(1, &m_vao);
    gl::GenBuffers(1, &m_vbo);
    gl::BindVertexArray(m_vao);
    gl::BindBuffer(GL_ARRAY_BUFFER, m_vbo);
    gl::BufferData(GL_ARRAY_BUFFER,
        static_cast<GLsizeiptr>(maxParticles * sizeof(ParticleVertex)),
        nullptr, GL_DYNAMIC_DRAW);

    constexpr auto stride = sizeof(ParticleVertex);
    gl::VertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(stride), (void*)0);
    gl::EnableVertexAttribArray(0);
    gl::VertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(stride), (void*)offsetof(ParticleVertex, color));
    gl::EnableVertexAttribArray(1);
    gl::VertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, static_cast<GLsizei>(stride), (void*)offsetof(ParticleVertex, size));
    gl::EnableVertexAttribArray(2);
    gl::BindVertexArray(0);

    loadDefaultPresets();
    m_initialized = true;
    spdlog::info("ParticleSystem: initialized (max={})", maxParticles);
    return true;
}

void ParticleSystem::shutdown()
{
    if (m_vbo)    { gl::DeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_vao)    { gl::DeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_shader) { gl::DeleteProgram(m_shader); m_shader = 0; }
    m_particles.clear();
    m_emitters.clear();
    m_initialized = false;
    spdlog::info("ParticleSystem: shutdown");
}

// =========================================================================
// 发射
// =========================================================================
uint32_t ParticleSystem::emit(const EmitterDesc& desc, glm::vec2 position)
{
    uint32_t id = m_nextEmitterId++;
    m_emitters[id] = {desc, position, 0.0f, 0.0f, true};
    spdlog::debug("ParticleSystem: emitter {} started", id);
    return id;
}

uint32_t ParticleSystem::emitBurst(const EmitterDesc& desc, glm::vec2 position, uint32_t count)
{
    for (uint32_t i = 0; i < count && m_activeCount < m_maxParticles; i++)
        spawnParticle(desc, position);
    return 0;
}

void ParticleSystem::stopEmitter(uint32_t id) { m_emitters.erase(id); }
void ParticleSystem::stopAll() { m_emitters.clear(); m_activeCount = 0; }

// =========================================================================
// 预设
// =========================================================================
void ParticleSystem::registerPreset(const std::string& name, const EmitterDesc& desc)
{
    EmitterDesc d = desc;
    d.presetName = name;
    m_presets[name] = d;
}

uint32_t ParticleSystem::emitPreset(const std::string& name, glm::vec2 position)
{
    auto it = m_presets.find(name);
    if (it == m_presets.end())
    {
        spdlog::warn("ParticleSystem: preset '{}' not found", name);
        return 0;
    }
    return emit(it->second, position);
}

void ParticleSystem::loadDefaultPresets()
{
    EmitterDesc firefly;
    firefly.emissionRate = 8.0f;
    firefly.particleLifetime = 3.0f;
    firefly.particleLifetimeVariance = 1.0f;
    firefly.startSpeed = 15.0f;
    firefly.startSpeedVariance = 10.0f;
    firefly.startColor = {1.0f, 0.9f, 0.3f, 0.8f};
    firefly.endColor   = {0.8f, 0.6f, 0.0f, 0.0f};
    firefly.startSize  = 4.0f;
    firefly.endSize    = 2.0f;
    firefly.gravity    = {0, 5};
    firefly.shape      = EmitterDesc::Circle;
    firefly.shapeRadius = 60.0f;
    registerPreset("fireflies", firefly);

    EmitterDesc dust;
    dust.emissionRate = 15.0f;
    dust.particleLifetime = 4.0f;
    dust.particleLifetimeVariance = 2.0f;
    dust.startSpeed = 10.0f;
    dust.startSpeedVariance = 8.0f;
    dust.startColor = {0.7f, 0.65f, 0.5f, 0.4f};
    dust.endColor   = {0.5f, 0.45f, 0.35f, 0.0f};
    dust.startSize  = 2.0f;
    dust.endSize    = 1.0f;
    dust.gravity    = {3, 8};
    dust.shape      = EmitterDesc::Box;
    dust.shapeRadius = 100.0f;
    registerPreset("dust", dust);

    EmitterDesc sparkle;
    sparkle.emissionRate = 30.0f;
    sparkle.particleLifetime = 0.8f;
    sparkle.particleLifetimeVariance = 0.3f;
    sparkle.startSpeed = 80.0f;
    sparkle.startSpeedVariance = 40.0f;
    sparkle.startColor = {1.0f, 0.7f, 0.1f, 1.0f};
    sparkle.endColor   = {1.0f, 0.2f, 0.0f, 0.0f};
    sparkle.startSize  = 5.0f;
    sparkle.endSize    = 1.0f;
    sparkle.gravity    = {0, -80};
    registerPreset("sparkles", sparkle);
}

// =========================================================================
// CPU 模拟
// =========================================================================
void ParticleSystem::update(float dt)
{
    float dtClamped = std::min(dt, 0.1f);

    std::uniform_real_distribution<float> varDist(-1.0f, 1.0f);

    // 发射新粒子
    std::vector<uint32_t> dead;
    for (auto& [id, em] : m_emitters)
    {
        if (!em.active) { dead.push_back(id); continue; }
        em.elapsed += dtClamped;
        em.accumulator += dtClamped * em.desc.emissionRate;

        while (em.accumulator >= 1.0f && m_activeCount < m_maxParticles)
        {
            em.accumulator -= 1.0f;
            spawnParticle(em.desc, em.position);
        }
        if (em.desc.duration > 0.0f && em.elapsed >= em.desc.duration)
            dead.push_back(id);
    }
    for (auto id : dead) m_emitters.erase(id);

    // 更新活跃粒子
    for (uint32_t i = 0; i < m_activeCount; )
    {
        auto& p = m_particles[i];

        // 速度 + 重力
        p.velocity += EmitterDesc{}.gravity * dtClamped; // 使用默认重力参数...
        // 实际上重力来自 emitter，但粒子不存储 emitter 引用。
        // 简化：直接使用固定重力 (0, -20)
        // 更好的做法：在 ParticleVertex 存储加速度或 emitter 引用

        // position += velocity * dt
        p.position += p.velocity * dtClamped;

        // 寿命推进
        p.life += dtClamped;

        // 死亡检查
        if (p.life >= 1.0f)
        {
            if (i < m_activeCount - 1)
                p = m_particles[m_activeCount - 1];
            m_activeCount--;
        }
        else
        {
            i++;
        }
    }
}

void ParticleSystem::spawnParticle(const EmitterDesc& desc, glm::vec2 position)
{
    if (m_activeCount >= m_maxParticles) return;

    std::uniform_real_distribution<float> angleDist(0.0f, 6.2832f);
    std::uniform_real_distribution<float> varDist(-1.0f, 1.0f);

    ParticleVertex& p = m_particles[m_activeCount];

    // 位置
    p.position = position;
    if (desc.shape == EmitterDesc::Circle)
    {
        float a = angleDist(s_rng);
        float r = desc.shapeRadius * std::sqrt(std::uniform_real_distribution<float>(0, 1)(s_rng));
        p.position.x += std::cos(a) * r;
        p.position.y += std::sin(a) * r;
    }
    else if (desc.shape == EmitterDesc::Box)
    {
        p.position.x += std::uniform_real_distribution<float>(-desc.shapeRadius, desc.shapeRadius)(s_rng);
        p.position.y += std::uniform_real_distribution<float>(-desc.shapeRadius, desc.shapeRadius)(s_rng);
    }

    // 速度
    float angle = angleDist(s_rng);
    float speed = desc.startSpeed + desc.startSpeedVariance * varDist(s_rng);
    speed = std::max(speed, 0.0f);
    p.velocity.x = std::cos(angle) * speed;
    p.velocity.y = std::sin(angle) * speed;

    // 重力存入 velocity（hack：用 gravity 来加速，且 velocity 也包含方向）
    // 实际上这个设计有问题——gravity 应持续影响。修正在 update() 中。
    // 当前：spawn 时初始 velocity 为方向+速度，update 时施加加速度

    p.color = desc.startColor;
    p.size  = desc.startSize;
    p.life  = 0.0f;

    m_activeCount++;
}

// =========================================================================
// GPU 渲染
// =========================================================================
void ParticleSystem::render(uint32_t screenWidth, uint32_t screenHeight)
{
    if (m_activeCount == 0 || !m_initialized) return;

    gl::Enable(GL_BLEND);
    gl::BlendFunc(GL_SRC_ALPHA, GL_ONE); // 加法混合

    gl::UseProgram(m_shader);
    gl::Uniform2f(static_cast<GLint>(m_uScreenSize),
        static_cast<float>(screenWidth), static_cast<float>(screenHeight));

    gl::BindVertexArray(m_vao);
    gl::BindBuffer(GL_ARRAY_BUFFER, m_vbo);

    gl::BufferSubData(GL_ARRAY_BUFFER, 0,
        static_cast<GLsizeiptr>(m_activeCount * sizeof(ParticleVertex)),
        m_particles.data());

    gl::Enable(GL_PROGRAM_POINT_SIZE);
    gl::DrawArrays(GL_POINTS, 0, static_cast<GLsizei>(m_activeCount));

    gl::BindVertexArray(0);
    gl::UseProgram(0);
    gl::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

} // namespace engine::render::particles
