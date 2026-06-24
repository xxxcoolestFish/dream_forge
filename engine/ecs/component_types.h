/**
 * @file engine/ecs/component_types.h
 * @brief 所有 ECS Component 的 POD 结构定义
 *
 * 设计原则：
 *   - Component 是纯数据结构（POD），不包含逻辑
 *   - 使用键值对（unordered_map）存储可变属性，避免每个属性都成为一个字段
 *   - 所有 Component 都需要默认构造函数
 *
 * 注意事项：
 *   - Component 的数据布局影响缓存性能，常用属性放前面
 *   - 避免在 Component 中使用 std::string 存储大量文本（用 ID 引用）
 */

#pragma once

#include "engine/ecs/entity.h"

#include <string>
#include <unordered_map>
#include <vector>
#include <set>
#include <algorithm>
#include <cstdint>
#include <glm/glm.hpp>

namespace engine::ecs {

// =========================================================================
// Transform — 空间变换
// =========================================================================
struct Transform
{
    glm::vec3 position  { 0.0f, 0.0f, 0.0f };  // 世界坐标 (z = 深度)
    glm::vec3 rotation  { 0.0f, 0.0f, 0.0f };  // 欧拉角（度）
    glm::vec3 scale     { 1.0f, 1.0f, 1.0f };
    float     depthLayer = 0.5f;                 // 深度层级 0.0(远景) ~ 1.0(前景)
};

// =========================================================================
// Sprite — 精灵渲染
// =========================================================================
struct Sprite
{
    uint32_t textureId   = 0;      // 纹理句柄
    glm::vec4 tint       { 1.0f }; // 色调
    glm::vec2 anchor     { 0.5f }; // 锚点 (0,0)=左下, (1,1)=右上
    int       layerGroup = 0;      // 层级组（同组之间深度排序）
    bool      visible    = true;
};

// =========================================================================
// Interactive — 可交互标记
// =========================================================================
struct Interactive
{
    // 点击检测多边形（相对于实体位置）
    std::vector<glm::vec2> hitPolygon;
    std::string            interactionType;  // "talk", "pickup", "use", "examine"
    std::string            cursorStyle;      // 悬停时的光标样式
    bool                   enabled = true;
};

// =========================================================================
// Stats — 数值属性（键值对）
// =========================================================================
struct Stats
{
    std::unordered_map<std::string, float> values;
    std::unordered_map<std::string, float> maxValues;
    std::unordered_map<std::string, float> minValues;

    float get(const std::string& key, float defaultVal = 0.0f) const
    {
        auto it = values.find(key);
        return (it != values.end()) ? it->second : defaultVal;
    }

    void set(const std::string& key, float value)
    {
        values[key] = value;
    }

    void modify(const std::string& key, float delta)
    {
        values[key] += delta;
    }

    // 钳制写 — 自动限制在 [minValues[key], maxValues[key]]
    void setClamped(const std::string& key, float value)
    {
        auto itMin = minValues.find(key);
        auto itMax = maxValues.find(key);
        if (itMin != minValues.end() && value < itMin->second) value = itMin->second;
        if (itMax != maxValues.end() && value > itMax->second) value = itMax->second;
        values[key] = value;
    }

    void modifyClamped(const std::string& key, float delta)
    {
        float cur = get(key, 0.0f);
        setClamped(key, cur + delta);
    }

    bool isAtMax(const std::string& key) const
    {
        auto itMax = maxValues.find(key);
        if (itMax == maxValues.end()) return false;
        return get(key) >= itMax->second;
    }

    bool isDepleted(const std::string& key) const
    {
        auto itMin = minValues.find(key);
        float minVal = (itMin != minValues.end()) ? itMin->second : 0.0f;
        return get(key) <= minVal;
    }
};

// =========================================================================
// Inventory — 物品栏
// =========================================================================
struct Inventory
{
    std::vector<Entity> items;
    uint32_t            maxSlots = 20;
    int                 money    = 0;

    bool add(Entity item)
    {
        if (items.size() >= maxSlots) return false;
        items.push_back(item);
        return true;
    }

    bool remove(Entity item)
    {
        auto it = std::find(items.begin(), items.end(), item);
        if (it == items.end()) return false;
        items.erase(it);
        return true;
    }

    bool isFull() const { return items.size() >= maxSlots; }

    bool addMoney(int amount)
    {
        if (amount < 0) return false;
        money += amount;
        return true;
    }

    bool spendMoney(int amount)
    {
        if (amount < 0 || money < amount) return false;
        money -= amount;
        return true;
    }
};

// =========================================================================
// UseEffect — 物品使用效果（挂载在 Item 上）
// =========================================================================
struct UseEffect
{
    std::string stat;        // 目标属性名
    float       modify  = 0.0f;
    bool        clampToMax = true;
};

// =========================================================================
// Item — 物品属性
// =========================================================================
struct Item
{
    std::string itemId;
    std::string name;
    std::string description;
    std::string equipSlot;  // "" = 不可装备且不可消耗, "head", "body", "weapon"...
    std::unordered_map<std::string, float> statModifiers; // 装备时属性修正
    bool        consumable = false;
    std::vector<UseEffect> useEffects;   // 使用时的效果列表
};

// =========================================================================
// DialogueSpeaker — AI对话角色
// =========================================================================
struct DialogueSpeaker
{
    std::string characterId;
    std::string personalityPrompt;  // 性格描述 prompt
    float       friendliness = 0.0f; // -1.0 ~ 1.0
    std::vector<std::string> memoryTags;
};

// =========================================================================
// DialogueState — 对话状态
// =========================================================================
struct DialogueState
{
    uint32_t dialogueTreeId = 0;
    uint32_t currentNodeId  = 0;
    std::vector<uint32_t> visitedNodes;
    std::unordered_map<std::string, bool> flags;
};

// =========================================================================
// QuestProgress — 任务进度
// =========================================================================
struct QuestProgress
{
    std::string questId;
    std::string status;  // "inactive", "active", "completed", "failed"
    std::unordered_map<std::string, int> objectives; // 目标ID → 进度
};

// =========================================================================
// Relationship — NPC关系
// =========================================================================
struct Relationship
{
    std::unordered_map<Entity, float> affinity; // NPC Entity → 好感度 (-1.0 ~ 1.0)
};

// =========================================================================
// AICharacterContext — AI角色上下文
// =========================================================================
struct AICharacterContext
{
    std::string backgroundStory;
    std::string currentGoal;
    std::string emotionalState;
    std::vector<std::string> knowledgeDomains;
};

// =========================================================================
// Player — 玩家标记（标记玩家 Entity，供 MovementSystem 等识别）
// =========================================================================
struct Player
{
    int dummy = 0; // EnTT 对空 struct 的 get<> 返回 void，加占位字段
};

// =========================================================================
// Equipped — 装备标记（物品 Entity 身上，标记已被装备）
// =========================================================================
struct Equipped
{
    Entity      equippedBy = kNullEntity;
    std::string slot;
};

// =========================================================================
// Dead — 死亡标记（HP 归零等致命状态）
// =========================================================================
struct Dead
{
    float deathTime = 0.0f;
};

} // namespace engine::ecs
