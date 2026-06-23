/**
 * @file engine/render/scene/camera.h
 * @brief 第一人称 3D 相机 — 鼠标旋转视角 + WASD 移动
 *
 * Phase 7: FPS 相机。玩家看哪里，相机就看哪里。
 *
 * 坐标系：
 *   X = 右, Y = 上, Z = 朝向观察者
 *   front = 相机前方向量（从 yaw/pitch 计算）
 *   viewMatrix = lookAt(position, position + front, worldUp)
 */

#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace engine::render {

class Camera
{
public:
    Camera();

    // --- 位置 ---
    void setPosition(const glm::vec3& pos);
    glm::vec3 position() const { return m_position; }
    glm::vec3 front()    const { return m_front; }

    // --- 鼠标旋转 ---
    void processMouse(float deltaX, float deltaY, float sensitivity = 0.1f);

    // --- 键盘移动（相对于相机朝向） ---
    void move(float forwardAmount, float rightAmount, float dt, float speed = 150.0f);

    // --- 跳跃 ---
    void jump(float strength = 300.0f);
    void updatePhysics(float dt);  // 重力 + 落地检测
    bool isOnGround() const { return m_onGround; }

    // --- 视角 ---
    void setFov(float degrees);
    float fov() const { return m_fov; }

    // --- 屏幕 ---
    void setScreenSize(float width, float height);

    // --- 矩阵 ---
    glm::mat4 viewMatrix()       const;
    glm::mat4 projectionMatrix() const;
    glm::mat4 viewProjection()   const;

    // --- 深度映射 ---
    float depthToZ(float depth) const;
    glm::vec2 fillSizeAtZ(float z) const;

    // 参考 Z（场景加载时相机的初始 Z，层尺寸基于此，不随相机移动改变）
    void setReferenceZ(float refZ) { m_referenceZ = refZ; }

    // 场景层 Z 范围
    static constexpr float kSceneZNear = -30.0f;
    static constexpr float kSceneZFar  = -500.0f;

private:
    glm::vec3 m_position { 640.0f, 360.0f, 200.0f };
    glm::vec3 m_front    { 0.0f, 0.0f, -1.0f };
    glm::vec3 m_up       { 0.0f, 1.0f, 0.0f };
    glm::vec3 m_worldUp  { 0.0f, 1.0f, 0.0f };
    glm::vec3 m_right    { 1.0f, 0.0f, 0.0f };

    float m_yaw    = -90.0f;   // 度，默认朝 -Z（场景方向）
    float m_pitch  = 0.0f;

    float m_fov    = 60.0f;
    float m_near   = 1.0f;
    float m_far    = 1000.0f;
    float m_screenW    = 1280.0f;
    float m_screenH    = 720.0f;
    float m_referenceZ = 150.0f;

    // 跳跃物理
    float m_velocityY = 0.0f;
    float m_groundY   = 360.0f;  // 地面高度
    bool  m_onGround  = true;

    void updateVectors();
    void rebuildProjection();
    glm::mat4 m_projection { 1.0f };
};

} // namespace engine::render
