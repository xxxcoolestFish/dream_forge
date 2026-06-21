/**
 * @file engine/narrative/quest_definition.h
 * @brief 任务系统数据结构 — 纯数据，JSON 可序列化
 *
 * 复用已有组件：
 *   - QuestProgress（engine/ecs/component_types.h）存储运行时进度
 *   - 每个活跃任务对应一个 QuestProgress 组件实例
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace engine::narrative {

// =========================================================================
// 任务目标
// =========================================================================
struct QuestObjective
{
    std::string objectiveId;   // 唯一 ID（如 "collect_herbs"）
    std::string description;   // 显示文本（如 "收集银叶草"）
    std::string type;          // "collect_item" | "talk_to_npc" | "reach_location" | "kill_enemy"
    std::string target;        // 目标 ID（物品名/NPC名/位置名/敌人名）
    int         required = 1;  // 需要的数量
};

// =========================================================================
// 任务奖励
// =========================================================================
struct QuestRewards
{
    int         xp    = 0;
    std::vector<std::string> items;                           // 物品 ID 列表
    std::unordered_map<std::string, bool>   flags;            // 剧情标记
    std::unordered_map<std::string, float>  affinityChanges;  // NPC → 好感度变化
};

// =========================================================================
// 任务阶段
// =========================================================================
struct QuestStage
{
    std::string stageId;       // "active" | "completed" | "failed"
    std::string journalEntry;  // 日志文本
};

// =========================================================================
// 任务定义（JSON 加载后不可变）
// =========================================================================
struct QuestDefinition
{
    std::string questId;
    std::string name;
    std::string description;
    std::string giverNpc;
    std::vector<QuestObjective> objectives;
    QuestRewards                rewards;
    std::vector<QuestStage>     stages;

    // 按 stageId 查找阶段
    const QuestStage* findStage(const std::string& stageId) const
    {
        for (auto& s : stages)
            if (s.stageId == stageId) return &s;
        return nullptr;
    }

    // 按 objectiveId 查找目标
    const QuestObjective* findObjective(const std::string& objId) const
    {
        for (auto& o : objectives)
            if (o.objectiveId == objId) return &o;
        return nullptr;
    }
};

} // namespace engine::narrative
