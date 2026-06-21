/**
 * @file engine/scene/scene.h
 * @brief 场景 — 有序 Layer 集合 + Entity 放置信息
 *
 * 一个 Scene 代表一个完整的游戏场景（如 "tavern"、"forest"）：
 *   - 背景底图（修复后的背景）
 *   - 有序 Layer 列表（从远到近排列）
 *   - Entity 初始放置列表（NPC、物品等）
 *   - 元数据（名称、氛围音乐等）
 *
 * 设计：
 *   - 纯数据结构，不依赖渲染 API
 *   - 可 JSON 序列化为 .scene 文件
 *   - Layer 顺序 = 渲染顺序（远→近）
 *
 * 与 ECS 的关系：
 *   - Scene 加载后，Entity 放置信息用于在 ECS World 中创建 Entity
 *   - Layer 列表传递给 SceneRenderer 进行绘制
 */

#pragma once

#include "engine/scene/layer.h"

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace engine::scene {

// Entity 初始放置信息
struct EntityPlacement
{
    std::string type;        // "npc", "item", "door", "player_spawn"
    std::string templateId;  // 对应的角色/物品模板 ID
    glm::vec3   position { 0.0f, 0.0f, 0.0f };
    float       rotation = 0.0f;  // 弧度
    nlohmann::json extra;   // 额外属性
};

struct Scene
{
    // 元数据
    std::string name;
    std::string description;
    std::string version = "0.1.0";

    // 背景
    std::string backgroundTexture;   // 背景底图路径
    glm::vec2   backgroundSize { 1920.0f, 1080.0f };  // 背景图尺寸

    // 层（按深度从远到近排列）
    std::vector<Layer> layers;

    // Entity 放置
    std::vector<EntityPlacement> entities;

    // 氛围
    std::string ambianceAudio;       // 氛围音频路径（可选）

    // JSON 序列化
    nlohmann::json toJson() const;
    static Scene fromJson(const nlohmann::json& j);

    // 按深度排序层（远→近）
    void sortLayersByDepth();
};

} // namespace engine::scene
