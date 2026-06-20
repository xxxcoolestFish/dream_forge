/**
 * @file engine/render/render_backend.h
 * @brief 渲染后端抽象 — Phase 1 使用 OpenGL 3.3
 *
 * 负责：
 *   - OpenGL 上下文初始化（GLFW 窗口提供）
 *   - 渲染帧的开始（clear）和结束（swap buffers）
 *   - 视口设置
 *
 * 设计：
 *   - 接口与具体渲染API解耦，后续可切换为 bgfx/DX11/Vulkan
 *   - Phase 1 使用 OpenGL 3.3 Core Profile
 *
 * 注意事项：
 *   - OpenGL 上下文由 GLFW 创建，RenderBackend 不负责创建窗口
 *   - 使用 Core Profile，不依赖已废弃的固定管线
 */

#pragma once

#include <cstdint>
#include <string>

struct GLFWwindow;

namespace engine::render {

struct RenderConfig
{
    uint32_t width       = 1280;
    uint32_t height      = 720;
    bool     vsync       = true;
    bool     debugMode   = false;   // 是否输出 OpenGL 调试信息
};

class RenderBackend
{
public:
    RenderBackend();
    ~RenderBackend();

    // 禁止拷贝
    RenderBackend(const RenderBackend&) = delete;
    RenderBackend& operator=(const RenderBackend&) = delete;

    // 初始化 OpenGL 上下文（需要已创建好的 GLFW 窗口）
    bool init(GLFWwindow* window, const RenderConfig& config);

    // 关闭
    void shutdown();

    // 帧操作
    void beginFrame();
    void endFrame();

    // 视口（窗口大小变化时调用）
    void setViewport(int x, int y, int width, int height);

    // 清屏颜色
    void setClearColor(float r, float g, float b, float a = 1.0f);

    // 获取配置
    const RenderConfig& config() const { return m_config; }
    bool isInitialized() const { return m_initialized; }

private:
    RenderConfig m_config;
    bool         m_initialized = false;
};

} // namespace engine::render
