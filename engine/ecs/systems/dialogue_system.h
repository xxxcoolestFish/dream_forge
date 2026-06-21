/**
 * @file engine/ecs/systems/dialogue_system.h
 * @brief 对话系统 — 基于对话树的 NPC 对话管理（Phase 5.3 重写）
 *
 * 重写内容：
 *   - 加载 JSON 对话树（DialogueTree）
 *   - 事件驱动：接收 InteractionEvent → 遍历对话树 → 发布 DialogueText/Choice/End 事件
 *   - AI 回退：ai_fallback 节点 → 构建真实 ECS 上下文 → 调用 AiClient → 发布文本
 *   - 使用已有的 DialogueState 组件追踪对话进度
 *
 * 依赖：World, EventBus, AiClient, DialogueTree
 */

#pragma once

#include "engine/ecs/system.h"
#include "engine/ecs/event_types.h"
#include "engine/core/event_bus.h"
#include "engine/narrative/dialogue_tree.h"

#include <unordered_map>
#include <string>

namespace engine::ai { class AiClient; }
namespace engine::ecs { class World; }

namespace engine::ecs {

class DialogueSystem : public System
{
public:
    DialogueSystem(ai::AiClient& aiClient, EventBus& eventBus);
    ~DialogueSystem() override = default;

    void onInit(World& world) override;
    void onUpdate(World& world, double dt) override;

    // 加载对话树文件目录
    bool loadTrees(const std::string& directory);

private:
    ai::AiClient& m_aiClient;
    EventBus&     m_eventBus;
    World*        m_world = nullptr;

    // 订阅 ID
    uint32_t m_interactionSubId    = 0;
    uint32_t m_optionSelectedSubId = 0;

    // 已加载的对话树（treeId → DialogueTree）
    std::unordered_map<std::string, narrative::DialogueTree> m_trees;

    // --- AI 请求状态 ---
    bool        m_waitingForAI    = false;
    std::string m_pendingNpcName;
    Entity      m_pendingNpcEntity = kNullEntity;
    double      m_aiTimer         = 0.0;

    // --- 事件处理 ---
    void onInteraction(const Event& event);
    void onOptionSelected(const Event& event);

    // --- 节点求值 ---
    void evaluateNode(World& world, Entity npcEntity);

    // --- 对话树管理 ---
    bool ensureTreeLoaded(const std::string& treeId);
};

} // namespace engine::ecs
