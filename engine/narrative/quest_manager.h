/**
 * @file engine/narrative/quest_manager.h
 * @brief 任务管理器 — 任务生命周期管理 + 目标追踪
 *
 * 负责：
 *   - 加载 .quest JSON 定义
 *   - 开始/完成/失败任务（操作 QuestProgress 组件）
 *   - 更新目标进度 → 自动检测完成
 *   - 发放奖励（XP、物品、标记变更）
 *
 * 设计：
 *   - 由 Engine::Impl 持有，非 ECS System
 *   - 通过 EventBus 接收目标相关事件
 *   - 每帧 update() 检查位置类目标
 */

#pragma once

#include "engine/narrative/quest_definition.h"
#include "engine/ecs/entity.h"

#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

namespace engine { class EventBus; struct Event; }
namespace engine::ecs { class World; }

namespace engine::narrative {

class QuestManager
{
public:
    QuestManager(ecs::World* world, EventBus* eventBus);
    ~QuestManager();

    // 禁止拷贝
    QuestManager(const QuestManager&) = delete;
    QuestManager& operator=(const QuestManager&) = delete;

    // --- 定义加载 ---
    bool loadDefinitions(const std::string& directory);

    // --- 任务生命周期 ---
    bool startQuest(ecs::Entity player, const std::string& questId);
    bool completeQuest(ecs::Entity player, const std::string& questId);
    bool failQuest(ecs::Entity player, const std::string& questId);

    // --- 目标追踪 ---
    // 外部系统（物品拾取、对话、死亡等）调用此方法更新进度
    // delta > 0 增加进度，delta < 0 减少（通常为 +1）
    void updateObjective(ecs::Entity player, const std::string& questId,
                         const std::string& objectiveId, int delta = 1);

    // --- 查询 ---
    bool isQuestActive(ecs::Entity player, const std::string& questId) const;
    const QuestDefinition* getDefinition(const std::string& questId) const;
    std::vector<const QuestDefinition*> getActiveQuests(ecs::Entity player) const;

    // --- 每帧更新 ---
    void update(double dt);

private:
    ecs::World* m_world = nullptr;
    EventBus*   m_eventBus = nullptr;

    std::unordered_map<std::string, QuestDefinition> m_definitions;

    // 发放奖励
    void grantRewards(ecs::Entity player, const QuestDefinition& quest);

    // 事件处理
    uint32_t m_interactionSubId = 0;
    void onInteractionEvent(const struct engine::Event& event);
};

} // namespace engine::narrative
