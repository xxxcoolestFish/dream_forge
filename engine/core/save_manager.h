/**
 * @file engine/core/save_manager.h
 * @brief 存档管理器 — 玩家状态 JSON 序列化/反序列化
 *
 * 职责：
 *   - 将玩家实体（Stats/Inventory/Transform）序列化为 JSON 文件
 *   - 从 JSON 恢复玩家状态
 *   - 附带 StoryFlags 持久化
 *
 * 设计：
 *   - 非 ECS System，纯工具类
 *   - 使用 nlohmann/json（已有依赖）
 *   - 物品实体完全序列化（Item + Equipped），加载时重建
 */

#pragma once

#include <string>
#include <nlohmann/json_fwd.hpp>

namespace engine::ecs { class World; }

namespace engine {

class SaveManager
{
public:
    /// 存档：序列化玩家 + StoryFlags → JSON 文件
    bool save(const std::string& filepath,
              ecs::World& world,
              const nlohmann::json& storyFlags) const;

    /// 读档：从 JSON 文件恢复玩家 + StoryFlags
    bool load(const std::string& filepath,
              ecs::World& world,
              nlohmann::json& outStoryFlags) const;
};

} // namespace engine
