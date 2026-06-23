/**
 * @file engine/render/scene/camera.cpp
 * @brief 第一人称 3D 相机实现
 */

#include "engine/render/scene/camera.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace engine::render {

Camera::Camera()
{
    updateVectors();
    rebuildProjection();
}

void Camera::setPosition(const glm::vec3& pos)
{
    m_position = pos;
}

void Camera::updateVectors()
{
    glm::vec3 f;
    f.x = std::cos(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    f.y = std::sin(glm::radians(m_pitch));
    f.z = std::sin(glm::radians(m_yaw)) * std::cos(glm::radians(m_pitch));
    m_front = glm::normalize(f);

    m_right = glm::normalize(glm::cross(m_front, m_worldUp));
    m_up    = glm::normalize(glm::cross(m_right, m_front));
}

void Camera::processMouse(float deltaX, float deltaY, float sensitivity)
{
    m_yaw   += deltaX * sensitivity;
    m_pitch -= deltaY * sensitivity;  // 鼠标上推 = 抬头

    m_pitch = std::clamp(m_pitch, -89.0f, 89.0f);

    updateVectors();
}

void Camera::move(float forwardAmount, float rightAmount, float dt, float speed)
{
    // 归一化方向（避免斜向移动更快）
    float len = std::sqrt(forwardAmount * forwardAmount + rightAmount * rightAmount);
    if (len > 0.0f)
    {
        forwardAmount /= len;
        rightAmount   /= len;
    }

    float d = speed * dt;

    // 只在地面平面移动（忽略 front.y 分量，防止飞行）
    glm::vec3 flatFront = glm::normalize(glm::vec3(m_front.x, 0.0f, m_front.z));

    m_position += flatFront * forwardAmount * d;
    m_position += m_right * rightAmount * d;
}

void Camera::jump(float strength)
{
    if (m_onGround)
    {
        m_velocityY = strength;
        m_onGround = false;
    }
}

void Camera::updatePhysics(float dt)
{
    const float gravity = -600.0f;  // 重力加速度

    if (!m_onGround)
    {
        m_velocityY += gravity * dt;
        m_position.y += m_velocityY * dt;

        // 落地检测
        if (m_position.y <= m_groundY)
        {
            m_position.y = m_groundY;
            m_velocityY = 0.0f;
            m_onGround = true;
        }
    }
}

void Camera::setFov(float degrees)
{
    m_fov = std::clamp(degrees, 10.0f, 120.0f);
    rebuildProjection();
}

void Camera::setScreenSize(float width, float height)
{
    m_screenW = width;
    m_screenH = height;
    rebuildProjection();
}

void Camera::rebuildProjection()
{
    float aspect = m_screenW / std::max(m_screenH, 1.0f);
    m_projection = glm::perspective(glm::radians(m_fov), aspect, m_near, m_far);
}

glm::mat4 Camera::viewMatrix() const
{
    return glm::lookAt(m_position, m_position + m_front, m_up);
}

glm::mat4 Camera::projectionMatrix() const
{
    return m_projection;
}

glm::mat4 Camera::viewProjection() const
{
    return m_projection * viewMatrix();
}

float Camera::depthToZ(float depth) const
{
    return kSceneZNear + depth * (kSceneZFar - kSceneZNear);
}

glm::vec2 Camera::fillSizeAtZ(float z) const
{
    // 使用参考距离（不变），而非当前相机距离——这样透视投影自然产生近大远小
    float dist = std::abs(m_referenceZ - z);
    if (dist < 0.01f) dist = 0.01f;

    float aspect = m_screenW / std::max(m_screenH, 1.0f);
    float halfFovY = glm::radians(m_fov * 0.5f);

    float halfH = dist * std::tan(halfFovY);
    float h = halfH * 2.0f;
    float w = h * aspect;

    return { w, h };
}

} // namespace engine::render
