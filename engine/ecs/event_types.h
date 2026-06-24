/**
 * @file engine/ecs/event_types.h
 * @brief ECS 系统间事件数据结构 — 统一的事件数据定义
 *
 * 消除此前在 interaction_system.cpp 和 dialogue_system.cpp 中
 * 重复定义的 InteractionData 问题。所有系统使用本文件的数据结构。
 */

#pragma once

#include "engine/ecs/entity.h"
#include <string>
#include <vector>

namespace engine::ecs {

// =========================================================================
// InteractionEvent — 玩家与实体交互（E 键触发）
// =========================================================================
// 发布者：InteractionSystem
// 订阅者：DialogueSystem
struct InteractionData
{
    Entity      sourceEntity;  // 交互发起者（通常是玩家）
    Entity      targetEntity;  // 被交互的实体（NPC/物品）
    std::string type;          // "talk", "pickup", "use", "examine"
};

// =========================================================================
// DialogueTextEvent — NPC 对话文本显示
// =========================================================================
// 发布者：DialogueSystem
// 订阅者：DialogueUISystem（Phase 5.6）
struct DialogueTextData
{
    std::string speaker;  // 说话者名称
    std::string text;     // 对话内容
    Entity      npcEntity; // NPC 实体（用于查询组件）
};

// =========================================================================
// DialogueChoiceEvent — 玩家选项显示
// =========================================================================
// 发布者：DialogueSystem
// 订阅者：DialogueUISystem（Phase 5.6）
struct DialogueChoiceData
{
    std::vector<std::string> optionTexts; // 选项文本列表
};

// =========================================================================
// DialogueOptionSelectedEvent — 玩家选择了某个选项
// =========================================================================
// 发布者：DialogueUISystem（Phase 5.6）
// 订阅者：DialogueSystem
struct DialogueOptionSelectedData
{
    int optionIndex; // 选中的选项索引（0-based）
};

// =========================================================================
// DialogueEndEvent — 对话结束
// =========================================================================
// 发布者：DialogueSystem
// 订阅者：DialogueUISystem（Phase 5.6）
struct DialogueEndData
{
    // 当前无需额外数据，占位供后续扩展
};

// =========================================================================
// StatChangedEvent — 实体属性值变化
// =========================================================================
// 发布者：StatSystem
// 订阅者：脚本层、UI、任务系统
struct StatChangedData
{
    Entity      entity;
    std::string statKey;
    float       oldValue = 0.0f;
    float       newValue = 0.0f;
};

// =========================================================================
// StatDepletedEvent — 实体属性归零（HP=0 等）
// =========================================================================
// 发布者：StatSystem
// 订阅者：脚本层（死亡处理等）
struct StatDepletedData
{
    Entity      entity;
    std::string statKey;
};

// =========================================================================
// ItemUsedEvent — 物品被使用/消耗
// =========================================================================
// 发布者：InventorySystem
// 订阅者：脚本层、任务系统（"使用药水"类目标）
struct ItemUsedData
{
    Entity      userEntity;
    Entity      itemEntity;
    std::string itemId;
};

// =========================================================================
// ItemEquippedEvent — 装备/卸下物品
// =========================================================================
// 发布者：InventorySystem
// 订阅者：脚本层、UI
struct ItemEquippedData
{
    Entity      entity;     // 装备者
    Entity      itemEntity; // 物品实体
    std::string slot;
    bool        equipped = true; // true=穿上, false=脱下
};

// =========================================================================
// MoneyChangedEvent — 金钱变动
// =========================================================================
// 发布者：InventorySystem
// 订阅者：脚本层、UI
struct MoneyChangedData
{
    Entity   entity;
    int      oldAmount = 0;
    int      newAmount = 0;
};

} // namespace engine::ecs
