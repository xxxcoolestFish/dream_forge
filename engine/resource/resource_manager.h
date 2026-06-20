/**
 * @file engine/resource/resource_manager.h
 * @brief 资源管理器 — 异步加载 + LRU缓存
 *
 * 负责：
 *   - 资源（纹理、场景、脚本、音频等）的生命周期管理
 *   - 异步加载（不阻塞渲染线程）
 *   - LRU 缓存（内存上限管理）
 *
 * 设计：
 *   - 模板接口 load<T>(path) → std::future<T>
 *   - 内部维护资源句柄 → 数据的映射
 *   - 引用计数确保使用中的资源不被淘汰
 *
 * 注意事项：
 *   - Phase 1 实现最简单版本（同步加载 + 无缓存淘汰）
 *   - 后续 Phase 再添加异步 + LRU
 *   - 资源路径相对于 assets/ 目录
 */

#pragma once

#include <string>
#include <memory>
#include <unordered_map>
#include <functional>

namespace engine::resource {

// 资源基类
class Resource
{
public:
    virtual ~Resource() = default;
    virtual bool load(const std::string& path) = 0;
    virtual void unload() = 0;
    virtual bool isLoaded() const = 0;
    virtual size_t memoryUsage() const = 0;
};

// 资源句柄
using ResourceHandle = uint32_t;
constexpr ResourceHandle kInvalidResource = 0;

class ResourceManager
{
public:
    ResourceManager();
    ~ResourceManager();

    // 注册资源类型（工厂函数）
    template<typename T>
    void registerType(const std::string& typeName);

    // 加载资源
    ResourceHandle load(const std::string& typeName, const std::string& path);

    // 获取资源
    template<typename T>
    T* get(ResourceHandle handle);

    // 释放资源
    void release(ResourceHandle handle);

    // 释放所有未使用资源
    void garbageCollect();

    // 内存管理
    void   setMaxMemory(size_t maxBytes);
    size_t currentMemoryUsage() const;
    size_t maxMemory() const;

private:
    struct Impl;
    Impl* m_impl = nullptr;
};

} // namespace engine::resource
