/**
 * @file engine/resource/resource_manager.cpp
 * @brief 资源管理器实现（当前为桩，Phase 1 Step 4 完善）
 */

#include "engine/resource/resource_manager.h"
#include <spdlog/spdlog.h>

namespace engine::resource {

struct ResourceManager::Impl
{
    size_t maxMemoryBytes = 256 * 1024 * 1024; // 256 MB 默认上限
    size_t currentMemory  = 0;
    // TODO: 资源映射表、LRU链表等
};

ResourceManager::ResourceManager()
    : m_impl(new Impl())
{
    spdlog::debug("ResourceManager created (max memory: {} MB)",
        m_impl->maxMemoryBytes / (1024 * 1024));
}

ResourceManager::~ResourceManager()
{
    delete m_impl;
}

ResourceHandle ResourceManager::load(const std::string& typeName, const std::string& path)
{
    // TODO Step 4: 根据 typeName 调用对应工厂函数加载资源
    spdlog::debug("ResourceManager::load: type={}, path={}", typeName, path);
    return kInvalidResource;
}

void ResourceManager::release(ResourceHandle handle)
{
    (void)handle;
}

void ResourceManager::garbageCollect()
{
    // TODO: LRU 淘汰
}

void ResourceManager::setMaxMemory(size_t maxBytes)
{
    m_impl->maxMemoryBytes = maxBytes;
}

size_t ResourceManager::currentMemoryUsage() const
{
    return m_impl->currentMemory;
}

size_t ResourceManager::maxMemory() const
{
    return m_impl->maxMemoryBytes;
}

} // namespace engine::resource
