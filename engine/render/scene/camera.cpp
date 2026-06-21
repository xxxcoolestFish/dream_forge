/**
 * @file engine/render/scene/camera.cpp
 * @brief 2.5D 相机实现
 */

#include "engine/render/scene/camera.h"

#include <algorithm>
#include <cmath>
#include <glm/gtc/constants.hpp>

namespace engine::render {

Camera::Camera()
{
}

void Camera::setPosition(float x, float y)
{
    m_position = { x, y };
}

void Camera::setRotation(float horizontalDeg, float verticalDeg)
{
    m_hAngle = std::clamp(horizontalDeg, -kMaxHAngle, kMaxHAngle);
    m_vAngle = std::clamp(verticalDeg,   -kMaxVAngle, kMaxVAngle);
}

void Camera::setZoom(float zoom)
{
    m_zoom = std::max(0.1f, zoom);
}

glm::vec2 Camera::parallaxOffset(float depth) const
{
    // 深度 0 = 远景（不动）, 深度 1 = 前景（偏移最大）
    float factor = std::clamp(depth, 0.0f, 1.0f);

    // 将角度归一化到 [-1, 1]
    float hNorm = m_hAngle / kMaxHAngle;
    float vNorm = m_vAngle / kMaxVAngle;

    return glm::vec2(
        hNorm * m_maxParallax * factor,
        vNorm * m_maxParallax * factor
    );
}

glm::vec2 Camera::worldToScreen(glm::vec2 worldPos, float depth) const
{
    // 世界坐标 → 相对于相机 → 屏幕中心
    glm::vec2 relative = worldPos - m_position;

    // 视差偏移（不同深度的物体偏移量不同）
    glm::vec2 parallax = parallaxOffset(depth);

    // 屏幕中心 + 相对位置 + 视差偏移
    return glm::vec2(
        m_screenWidth  * 0.5f + relative.x + parallax.x,
        m_screenHeight * 0.5f + relative.y + parallax.y
    );
}

void Camera::setScreenSize(float width, float height)
{
    m_screenWidth  = width;
    m_screenHeight = height;
}

void Camera::setMaxParallax(float pixels)
{
    m_maxParallax = std::max(0.0f, pixels);
}

} // namespace engine::render
