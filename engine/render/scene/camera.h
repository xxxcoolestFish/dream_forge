/**
 * @file engine/render/scene/camera.h
 * @brief 2.5D 相机 — 控制场景视角和视差计算
 *
 * 相机围绕场景中心旋转，不同深度层产生不同的视差偏移量。
 * 旋转限制：水平 ±15°、垂直 ±5°。
 *
 * 视差原理：
 *   深度因子 d (0=远景, 1=前景)
 *   相机偏移角度 (hAngle, vAngle)
 *   视差偏移 = d * maxParallax * angle
 *   近景偏移大（前景移动多），远景偏移小（天空几乎不动）
 *
 * 使用方式：
 *   Camera cam;
 *   cam.setPosition(100, 200);        // 相机看向的位置
 *   cam.setRotation(5.0f, -2.0f);     // 轻微旋转
 *   glm::vec2 offset = cam.parallaxOffset(layerDepth);  // 每层偏移
 */

#pragma once

#include <glm/glm.hpp>

namespace engine::render {

class Camera
{
public:
    Camera();

    // --- 位置 ---
    void setPosition(float x, float y);
    glm::vec2 position() const { return m_position; }

    // --- 旋转（角度制，受限制） ---
    // horizontal: -15° ~ +15°
    // vertical:   -5° ~ +5°
    void setRotation(float horizontalDeg, float verticalDeg);
    float horizontalAngle() const { return m_hAngle; }
    float verticalAngle()   const { return m_vAngle; }

    // --- 缩放 ---
    void  setZoom(float zoom);
    float zoom() const { return m_zoom; }

    // --- 视差偏移计算 ---
    // 给定深度值 (0~1)，返回该层在屏幕上的偏移量（像素）
    glm::vec2 parallaxOffset(float depth) const;

    // 将世界坐标转换为屏幕坐标（考虑相机位置、旋转、视差）
    glm::vec2 worldToScreen(glm::vec2 worldPos, float depth) const;

    // --- 屏幕尺寸（每帧设置） ---
    void setScreenSize(float width, float height);

    // --- 最大视差偏移量（像素） ---
    void  setMaxParallax(float pixels);
    float maxParallax() const { return m_maxParallax; }

private:
    glm::vec2 m_position { 0.0f, 0.0f };

    float m_hAngle = 0.0f;      // 水平角（度）
    float m_vAngle = 0.0f;      // 垂直角（度）

    static constexpr float kMaxHAngle = 15.0f;
    static constexpr float kMaxVAngle = 5.0f;

    float m_zoom        = 1.0f;
    float m_maxParallax = 30.0f;  // 最大视差偏移（像素）

    float m_screenWidth  = 1280.0f;
    float m_screenHeight = 720.0f;
};

} // namespace engine::render
