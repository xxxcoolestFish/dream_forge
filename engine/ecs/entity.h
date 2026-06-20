/**
 * @file engine/ecs/entity.h
 * @brief Entity 类型定义
 *
 * Entity 是一个轻量的标识符，不对应任何数据。
 * 所有数据以 Component 形式存储，由 System 处理。
 *
 * 使用 EnTT 的 entity_type（uint64_t 或 uint32_t 取决于配置）。
 */

#pragma once

#include <entt/entt.hpp>

namespace engine::ecs {

// Entity 句柄类型
using Entity = entt::entity;

// 无效/空 Entity 常量
constexpr Entity kNullEntity = entt::null;

} // namespace engine::ecs
