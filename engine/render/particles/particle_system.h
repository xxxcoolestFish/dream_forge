/**
 * @file engine/render/particles/particle_system.h
 * @brief GPU 点精灵粒子系统 — Phase 6.5
 *
 * CPU 端更新粒子生命周期，GPU 端通过 GL_POINTS + 圆形 mask 渲染。
 * 支持持续发射、爆发、预设（萤火虫/尘埃/火花）。
 */

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace engine::render::particles {

// =========================================================================
// 粒子数据（CPU 端，每帧上传到 VBO）
// =========================================================================
struct ParticleVertex
{
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;
    float     size = 4.0f;
    float     life = 1.0f;    // 归一化寿命 [0,1], >1 = 死亡
};

// =========================================================================
// 发射器描述
// =========================================================================
struct EmitterDesc
{
    enum Shape { Point, Circle, Box } shape = Point;
    float shapeRadius = 10.0f;

    float emissionRate = 20.0f;           // 每秒粒子数
    float particleLifetime = 2.0f;        // 秒
    float particleLifetimeVariance = 0.5f;
    float startSpeed = 50.0f;
    float startSpeedVariance = 20.0f;
    glm::vec4 startColor{1, 1, 1, 1};
    glm::vec4 endColor{1, 1, 1, 0};
    float startSize = 6.0f;
    float endSize = 1.0f;
    glm::vec2 gravity{0, -20};
    bool  oneShot = false;                // true=单次爆发
    float duration = -1.0f;              // -1=无限（连续发射）

    // 辅助
    std::string presetName;               // 预设名（用于查找）
};

// =========================================================================
// 粒子系统
// =========================================================================
class ParticleSystem
{
public:
    ParticleSystem();
    ~ParticleSystem();

    // 禁止拷贝
    ParticleSystem(const ParticleSystem&) = delete;
    ParticleSystem& operator=(const ParticleSystem&) = delete;

    bool init(uint32_t maxParticles = 4096);
    void shutdown();

    // 发射粒子
    uint32_t emit(const EmitterDesc& desc, glm::vec2 position);
    uint32_t emitBurst(const EmitterDesc& desc, glm::vec2 position, uint32_t count);
    void stopEmitter(uint32_t emitterId);
    void stopAll();

    // 每帧更新（CPU 模拟）
    void update(float dt);

    // GPU 渲染（GL_POINTS）
    void render(uint32_t screenWidth, uint32_t screenHeight);

    // 预设
    void registerPreset(const std::string& name, const EmitterDesc& desc);
    uint32_t emitPreset(const std::string& name, glm::vec2 position);

private:
    // 粒子数组
    std::vector<ParticleVertex> m_particles;
    uint32_t m_maxParticles = 0;
    uint32_t m_activeCount  = 0;

    // GPU 资源
    uint32_t m_vbo     = 0;
    uint32_t m_vao     = 0;
    uint32_t m_shader  = 0;
    uint32_t m_uScreenSize = 0;

    // 发射器
    struct EmitterState
    {
        EmitterDesc desc;
        glm::vec2   position;
        float       accumulator = 0.0f;
        float       elapsed = 0.0f;
        bool        active = true;
    };
    std::unordered_map<uint32_t, EmitterState> m_emitters;
    uint32_t m_nextEmitterId = 1;

    // 预设
    std::unordered_map<std::string, EmitterDesc> m_presets;

    bool m_initialized = false;

    void spawnParticle(const EmitterDesc& desc, glm::vec2 position);
    void loadDefaultPresets();
};

} // namespace engine::render::particles
