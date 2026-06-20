/**
 * @file engine/render/render_backend.h
 * @brief 渲染后端抽象 — 基于 bgfx 的实现
 *
 * 负责：
 *   - bgfx 初始化与关闭
 *   - 渲染帧的开始（begin）和结束（end）
 *   - 视口和清理设置
 *
 * 设计：
 *   - 单一实现（bgfx），但接口设计为可替换
 *   - 使用 RAII 管理 bgfx 生命周期
 *
 * 注意事项：
 *   - bgfx 初始化需要有效的窗口句柄（从 GLFW 获取）
 *   - DX11 是当前 Windows 上的默认后端
 *   - 初始化顺序：GLFW 窗口 → bgfx 初始化
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
    bool     debugMode   = false;  // bgfx debug text 等
};

class RenderBackend
{
public:
    RenderBackend();
    ~RenderBackend();

    // 禁止拷贝
    RenderBackend(const RenderBackend&) = delete;
    RenderBackend& operator=(const RenderBackend&) = delete;

    // 初始化（需要 GLFW 窗口句柄）
    bool init(GLFWwindow* window, const RenderConfig& config);

    // 关闭
    void shutdown();

    // 帧操作
    void beginFrame();
    void endFrame();

    // 视口
    void setViewport(uint16_t x, uint16_t y, uint16_t width, uint16_t height);

    // 获取配置
    const RenderConfig& config() const { return m_config; }
    bool isInitialized() const { return m_initialized; }

    // bgfx 直接访问（高级使用，临时暴露）
    void* nativeHandle() const;

private:
    RenderConfig m_config;
    bool         m_initialized = false;
};

} // namespace engine::render
