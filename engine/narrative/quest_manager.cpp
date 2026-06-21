/**
 * @file engine/narrative/quest_manager.cpp
 * @brief 任务管理器实现
 */

#include "engine/narrative/quest_manager.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"
#include "engine/ecs/event_types.h"
#include "engine/core/event_bus.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace engine::narrative {

// 引入 ECS 类型
using ecs::Entity;
using ecs::kNullEntity;
using ecs::QuestProgress;
using ecs::Stats;
using ecs::Inventory;
using ecs::Item;
using ecs::DialogueSpeaker;
using ecs::Relationship;
using ecs::InteractionData;

// =========================================================================
// 构造 / 析构
// =========================================================================
QuestManager::QuestManager(ecs::World* world, EventBus* eventBus)
    : m_world(world)
    , m_eventBus(eventBus)
{
    // 订阅交互事件（用于 talk_to_npc 类型目标）
    m_interactionSubId = m_eventBus->subscribe(
        ENGINE_EVENT("Interaction"),
        [this](const engine::Event& event) { onInteractionEvent(event); });

    spdlog::debug("QuestManager: created");
}

QuestManager::~QuestManager()
{
    if (m_eventBus)
        m_eventBus->unsubscribe(ENGINE_EVENT("Interaction"), m_interactionSubId);
    spdlog::debug("QuestManager: destroyed");
}

// =========================================================================
// 加载任务定义目录
// =========================================================================
bool QuestManager::loadDefinitions(const std::string& directory)
{
    namespace fs = std::filesystem;

    std::error_code ec;
    if (!fs::exists(directory, ec) || !fs::is_directory(directory, ec))
    {
        spdlog::warn("QuestManager: directory '{}' not found", directory);
        return false;
    }

    int count = 0;
    for (const auto& entry : fs::directory_iterator(directory, ec))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
            continue;

        std::ifstream file(entry.path());
        if (!file.is_open()) continue;

        std::stringstream buf;
        buf << file.rdbuf();

        try
        {
            auto j = nlohmann::json::parse(buf.str());
            QuestDefinition def;
            def.questId      = j["questId"].get<std::string>();
            def.name         = j.value("name", def.questId);
            def.description  = j.value("description", "");
            def.giverNpc     = j.value("giverNpc", "");

            // 目标
            if (j.contains("objectives"))
            {
                for (const auto& obj : j["objectives"])
                {
                    QuestObjective qo;
                    qo.objectiveId = obj["objectiveId"].get<std::string>();
                    qo.description = obj.value("description", "");
                    qo.type        = obj.value("type", "collect_item");
                    qo.target      = obj.value("target", "");
                    qo.required    = obj.value("required", 1);
                    def.objectives.push_back(qo);
                }
            }

            // 奖励
            if (j.contains("rewards"))
            {
                const auto& r = j["rewards"];
                def.rewards.xp = r.value("xp", 0);
                if (r.contains("items"))
                    for (const auto& item : r["items"])
                        def.rewards.items.push_back(item.get<std::string>());
                if (r.contains("flags"))
                    for (auto& [k, v] : r["flags"].items())
                        def.rewards.flags[k] = v.get<bool>();
                if (r.contains("affinity"))
                    for (auto& [k, v] : r["affinity"].items())
                        def.rewards.affinityChanges[k] = v.get<float>();
            }

            // 阶段
            if (j.contains("stages"))
            {
                for (const auto& s : j["stages"])
                {
                    QuestStage qs;
                    qs.stageId      = s["stageId"].get<std::string>();
                    qs.journalEntry = s.value("journalEntry", "");
                    def.stages.push_back(qs);
                }
            }

            m_definitions[def.questId] = std::move(def);
            count++;
        }
        catch (const std::exception& e)
        {
            spdlog::error("QuestManager: parse error '{}': {}",
                          entry.path().string(), e.what());
        }
    }

    if (count > 0)
        spdlog::info("QuestManager: loaded {} quest definition(s) from '{}'", count, directory);
    return count > 0;
}

// =========================================================================
// 开始任务
// =========================================================================
bool QuestManager::startQuest(ecs::Entity player, const std::string& questId)
{
    if (!m_world || !m_world->isValid(player)) return false;

    auto defIt = m_definitions.find(questId);
    if (defIt == m_definitions.end())
    {
        spdlog::warn("QuestManager: unknown quest '{}'", questId);
        return false;
    }

    // 检查是否已激活
    if (isQuestActive(player, questId))
    {
        spdlog::debug("QuestManager: quest '{}' already active", questId);
        return false;
    }

    // 为每个活跃任务创建一个 Quest 实体（因为每个 Entity 只能有一个 QuestProgress 组件）
    auto questEntity = m_world->createEntity();
    auto& qp = m_world->addComponent<QuestProgress>(questEntity);
    qp.questId = questId;
    qp.status  = "active";

    // 初始化目标进度
    for (const auto& obj : defIt->second.objectives)
        qp.objectives[obj.objectiveId] = 0;

    spdlog::info("QuestManager: quest '{}' ({}) started for player", questId, defIt->second.name);

    // 发布事件通知（后续 DialogueUISystem 可监听）
    // 预留：m_eventBus->publishImmediate(ENGINE_EVENT("QuestStarted"), ...);

    return true;
}

// =========================================================================
// 完成任务
// =========================================================================
bool QuestManager::completeQuest(ecs::Entity player, const std::string& questId)
{
    // 查找该任务的 Quest 实体
    Entity questEnt = kNullEntity;
    for (auto e : m_world->view<QuestProgress>())
    {
        auto& qp = m_world->getComponent<QuestProgress>(e);
        if (qp.questId == questId && qp.status == "active")
        {
            questEnt = e;
            break;
        }
    }

    if (questEnt == kNullEntity)
    {
        spdlog::warn("QuestManager: no active quest '{}' to complete", questId);
        return false;
    }

    auto& qp = m_world->getComponent<QuestProgress>(questEnt);
    qp.status = "completed";

    auto defIt = m_definitions.find(questId);
    if (defIt != m_definitions.end())
    {
        grantRewards(player, defIt->second);
        spdlog::info("QuestManager: quest '{}' ({}) completed!", questId, defIt->second.name);
    }

    return true;
}

// =========================================================================
// 失败任务
// =========================================================================
bool QuestManager::failQuest(ecs::Entity player, const std::string& questId)
{
    for (auto e : m_world->view<QuestProgress>())
    {
        auto& qp = m_world->getComponent<QuestProgress>(e);
        if (qp.questId == questId && qp.status == "active")
        {
            qp.status = "failed";
            spdlog::info("QuestManager: quest '{}' failed", questId);
            return true;
        }
    }
    return false;
}

// =========================================================================
// 更新目标进度
// =========================================================================
void QuestManager::updateObjective(ecs::Entity player, const std::string& questId,
                                    const std::string& objectiveId, int delta)
{
    (void)player;
    for (auto e : m_world->view<QuestProgress>())
    {
        auto& qp = m_world->getComponent<QuestProgress>(e);
        if (qp.questId == questId && qp.status == "active")
        {
            qp.objectives[objectiveId] += delta;
            int current = qp.objectives[objectiveId];

            spdlog::debug("QuestManager: objective '{}/{}' = {}/{}",
                          questId, objectiveId, current,
                          m_definitions[questId].findObjective(objectiveId)->required);

            // 检查是否所有目标都完成
            const auto* def = getDefinition(questId);
            if (def)
            {
                bool allDone = true;
                for (const auto& obj : def->objectives)
                {
                    if (qp.objectives[obj.objectiveId] < obj.required)
                    {
                        allDone = false;
                        break;
                    }
                }
                if (allDone)
                    completeQuest(player, questId);
            }
            return;
        }
    }
}

// =========================================================================
// 查询
// =========================================================================
bool QuestManager::isQuestActive(ecs::Entity player, const std::string& questId) const
{
    (void)player;
    for (auto e : m_world->view<QuestProgress>())
    {
        auto& qp = m_world->getComponent<QuestProgress>(e);
        if (qp.questId == questId && qp.status == "active")
            return true;
    }
    return false;
}

const QuestDefinition* QuestManager::getDefinition(const std::string& questId) const
{
    auto it = m_definitions.find(questId);
    return (it != m_definitions.end()) ? &it->second : nullptr;
}

std::vector<const QuestDefinition*> QuestManager::getActiveQuests(ecs::Entity player) const
{
    (void)player;
    std::vector<const QuestDefinition*> result;
    for (auto e : m_world->view<QuestProgress>())
    {
        auto& qp = m_world->getComponent<QuestProgress>(e);
        if (qp.status == "active")
        {
            auto it = m_definitions.find(qp.questId);
            if (it != m_definitions.end())
                result.push_back(&it->second);
        }
    }
    return result;
}

// =========================================================================
// 每帧更新
// =========================================================================
void QuestManager::update(double dt)
{
    // 位置类目标检查（后续实现）
    (void)dt;
}

// =========================================================================
// 发放奖励
// =========================================================================
void QuestManager::grantRewards(ecs::Entity player, const QuestDefinition& quest)
{
    if (!m_world || !m_world->isValid(player)) return;

    const auto& rewards = quest.rewards;

    // XP
    if (rewards.xp > 0 && m_world->hasComponent<ecs::Stats>(player))
    {
        auto& stats = m_world->getComponent<ecs::Stats>(player);
        stats.modify("xp", static_cast<float>(rewards.xp));
        spdlog::info("QuestManager: granted {} XP (total: {:.0f})",
                     rewards.xp, stats.get("xp"));
    }

    // 物品（添加到 Inventory）
    if (!rewards.items.empty() && m_world->hasComponent<ecs::Inventory>(player))
    {
        auto& inv = m_world->getComponent<ecs::Inventory>(player);
        for (const auto& itemId : rewards.items)
        {
            // 创建物品实体
            auto itemEnt = m_world->createEntity();
            auto& item = m_world->addComponent<ecs::Item>(itemEnt);
            item.itemId = itemId;
            item.name = itemId;
            inv.add(itemEnt);
        }
        spdlog::info("QuestManager: granted {} item(s)", rewards.items.size());
    }

    // 标记（StoryFlags 在 5.5 集成）
    if (!rewards.flags.empty())
    {
        spdlog::debug("QuestManager: {} flag(s) pending (5.5 integration)",
                      rewards.flags.size());
    }

    // 好感度（Relationship 组件）
    if (!rewards.affinityChanges.empty() && m_world->hasComponent<ecs::Relationship>(player))
    {
        auto& rel = m_world->getComponent<ecs::Relationship>(player);
        for (const auto& [npcId, delta] : rewards.affinityChanges)
        {
            // 查找 NPC entity（简化：用第一个匹配的）
            for (auto npcEnt : m_world->view<ecs::DialogueSpeaker>())
            {
                auto& sp = m_world->getComponent<ecs::DialogueSpeaker>(npcEnt);
                if (sp.characterId == npcId)
                {
                    rel.affinity[npcEnt] += delta;
                    break;
                }
            }
        }
    }
}

// =========================================================================
// 事件处理 — talk_to_npc 目标检测
// =========================================================================
void QuestManager::onInteractionEvent(const engine::Event& event)
{
    const auto* data = std::any_cast<ecs::InteractionData>(&event.data);
    if (!data) return;

    if (data->type != "talk") return;

    // 查找 talk_to_npc 类型的目标
    Entity player = data->sourceEntity;
    std::string npcName;

    if (m_world->hasComponent<ecs::DialogueSpeaker>(data->targetEntity))
    {
        npcName = m_world->getComponent<ecs::DialogueSpeaker>(data->targetEntity).characterId;
    }

    if (npcName.empty()) return;

    // 遍历所有活跃任务，检查 talk_to_npc 目标
    for (auto e : m_world->view<ecs::QuestProgress>())
    {
        auto& qp = m_world->getComponent<ecs::QuestProgress>(e);
        if (qp.status != "active") continue;

        const auto* def = getDefinition(qp.questId);
        if (!def) continue;

        for (const auto& obj : def->objectives)
        {
            if (obj.type == "talk_to_npc" && obj.target == npcName)
            {
                updateObjective(player, qp.questId, obj.objectiveId, 1);
            }
        }
    }
}

} // namespace engine::narrative
