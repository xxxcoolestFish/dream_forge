/**
 * @file engine/ecs/systems/dialogue_system.cpp
 * @brief 对话系统实现 — 对话树遍历 + AI 回退（Phase 5.3 重写）
 */

#include "engine/ecs/systems/dialogue_system.h"
#include "engine/ecs/world.h"
#include "engine/narrative/condition_evaluator.h"
#include "engine/ecs/component_types.h"
#include "engine/ai/ai_client.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace engine::ecs {

// 引入叙事系统类型
using engine::narrative::DialogueNode;
using engine::narrative::DialogueNodeType;

// =========================================================================
// 事件 ID（编译期计算）
// =========================================================================
static constexpr EventTypeId kInteractionEvent       = ENGINE_EVENT("Interaction");
static constexpr EventTypeId kDialogueTextEvent      = ENGINE_EVENT("DialogueText");
static constexpr EventTypeId kDialogueChoiceEvent     = ENGINE_EVENT("DialogueChoice");
static constexpr EventTypeId kDialogueOptionSelected  = ENGINE_EVENT("DialogueOptionSelected");
static constexpr EventTypeId kDialogueEndEvent        = ENGINE_EVENT("DialogueEnd");

// =========================================================================
// 构造
// =========================================================================
DialogueSystem::DialogueSystem(ai::AiClient& aiClient, EventBus& eventBus)
    : System("DialogueSystem")
    , m_aiClient(aiClient)
    , m_eventBus(eventBus)
{
}

// =========================================================================
// 初始化
// =========================================================================
void DialogueSystem::onInit(World& world)
{
    m_world = &world;

    // 订阅交互事件
    m_interactionSubId = m_eventBus.subscribe(kInteractionEvent,
        [this](const Event& event) { onInteraction(event); });

    // 订阅选项选中事件
    m_optionSelectedSubId = m_eventBus.subscribe(kDialogueOptionSelected,
        [this](const Event& event) { onOptionSelected(event); });

    // 自动加载对话树目录
    loadTrees("assets/dialogues/");

    spdlog::info("DialogueSystem: initialized, {} tree(s) loaded", m_trees.size());
}

// =========================================================================
// 每帧更新
// =========================================================================
void DialogueSystem::onUpdate(World& world, double dt)
{
    (void)world;

    if (m_waitingForAI)
    {
        m_aiTimer += dt;
        auto response = m_aiClient.pollResponse();

        if (response.has_value())
        {
            m_waitingForAI = false;
            m_aiTimer = 0.0;

            const auto& resp = *response;
            std::string aiText;

            if (resp.contains("data") && resp["data"].contains("text"))
                aiText = resp["data"]["text"].get<std::string>();
            else if (resp.contains("error"))
                aiText = std::string("...") + resp.value("error", "无响应");
            else
                aiText = "...（" + m_pendingNpcName + " 沉默了片刻）";

            // 发布 AI 生成的文本
            DialogueTextData textData;
            textData.speaker  = m_pendingNpcName;
            textData.text     = aiText;
            textData.npcEntity = m_pendingNpcEntity;

            m_eventBus.publishImmediate(kDialogueTextEvent, std::any(textData));

            // AI 回退后直接结束对话（后续可改为继续到指定节点）
            DialogueEndData endData;
            m_eventBus.publishImmediate(kDialogueEndEvent, std::any(endData));

            // 重置 NPC 的对话状态
            if (m_world->isValid(m_pendingNpcEntity) &&
                m_world->hasComponent<DialogueState>(m_pendingNpcEntity))
            {
                auto& ds = m_world->getComponent<DialogueState>(m_pendingNpcEntity);
                ds.currentNodeId = 0;
            }
        }
        else if (m_aiTimer > 8.0)
        {
            // 超时：显示提示并结束
            m_waitingForAI = false;
            m_aiTimer = 0.0;

            spdlog::warn("DialogueSystem: AI response timeout for '{}'", m_pendingNpcName);

            DialogueTextData textData;
            textData.speaker  = m_pendingNpcName;
            textData.text     = "..." + m_pendingNpcName + " 似乎在想什么。";
            textData.npcEntity = m_pendingNpcEntity;

            m_eventBus.publishImmediate(kDialogueTextEvent, std::any(textData));

            DialogueEndData endData;
            m_eventBus.publishImmediate(kDialogueEndEvent, std::any(endData));
        }
        else if (m_aiTimer > 4.0 && static_cast<int>(m_aiTimer) % 2 == 0)
        {
            spdlog::info("  {} 正在思考...", m_pendingNpcName);
        }
    }
}

// =========================================================================
// 加载对话树目录
// =========================================================================
bool DialogueSystem::loadTrees(const std::string& directory)
{
    namespace fs = std::filesystem;

    std::error_code ec;
    if (!fs::exists(directory, ec) || !fs::is_directory(directory, ec))
    {
        spdlog::warn("DialogueSystem: dialogue directory '{}' not found", directory);
        return false;
    }

    int count = 0;
    for (const auto& entry : fs::directory_iterator(directory, ec))
    {
        if (!entry.is_regular_file() || entry.path().extension() != ".json")
            continue;

        std::ifstream file(entry.path());
        if (!file.is_open())
            continue;

        std::stringstream buf;
        buf << file.rdbuf();

        try
        {
            auto j = nlohmann::json::parse(buf.str());
            narrative::DialogueTree tree;
            if (tree.loadFromJson(j))
            {
                m_trees[tree.treeId()] = std::move(tree);
                count++;
            }
        }
        catch (const std::exception& e)
        {
            spdlog::error("DialogueSystem: failed to parse '{}': {}",
                          entry.path().string(), e.what());
        }
    }

    if (count > 0)
        spdlog::info("DialogueSystem: loaded {} dialogue tree(s) from '{}'", count, directory);
    return count > 0;
}

// =========================================================================
// 交互事件处理
// =========================================================================
void DialogueSystem::onInteraction(const Event& event)
{
    if (m_waitingForAI)
    {
        spdlog::debug("DialogueSystem: ignoring interaction (waiting for AI)");
        return;
    }

    const auto* data = std::any_cast<InteractionData>(&event.data);
    if (!data) return;

    if (data->type != "talk") return;

    Entity npcEnt = data->targetEntity;
    if (!m_world || !m_world->isValid(npcEnt)) return;
    if (!m_world->hasComponent<DialogueSpeaker>(npcEnt)) return;

    auto& speaker = m_world->getComponent<DialogueSpeaker>(npcEnt);

    // 确定对话树 ID（characterId + "_intro"）
    std::string treeId = speaker.characterId + "_intro";

    // 确保已有对话树加载
    if (!ensureTreeLoaded(treeId))
    {
        spdlog::warn("DialogueSystem: no dialogue tree '{}' for NPC '{}'",
                     treeId, speaker.characterId);

        // 回退到纯 AI 对话
        spdlog::info("DialogueSystem: falling back to pure AI dialogue for '{}'",
                     speaker.characterId);

        // 构建简单的 AI 回退请求
        nlohmann::json request;
        request["type"] = "llm_dialogue";
        request["payload"]["npc"]["name"] = speaker.characterId;
        request["payload"]["npc"]["personality"] = speaker.personalityPrompt;
        request["payload"]["npc"]["background"] = "这个世界的一位居民。";
        request["payload"]["history"] = nlohmann::json::array();

        // 读取玩家真实数据
        auto playerView = m_world->view<Transform, Player>();
        for (auto pe : playerView)
        {
            if (m_world->hasComponent<Stats>(pe))
            {
                auto& stats = m_world->getComponent<Stats>(pe);
                request["payload"]["player"]["hp"] = stats.get("hp", 100);
                request["payload"]["player"]["level"] = stats.get("level", 1);
                break;
            }
        }

        m_pendingNpcName   = speaker.characterId;
        m_pendingNpcEntity = npcEnt;

        if (m_aiClient.startRequest(request))
        {
            m_waitingForAI = true;
            spdlog::info("与 {} 对话中（AI 模式）...", m_pendingNpcName);
        }
        else
        {
            spdlog::warn("无法发送对话请求（AI 服务未启动？）");

            DialogueTextData td;
            td.speaker  = speaker.characterId;
            td.text     = "..." + speaker.characterId + " 暂时无法回应。";
            td.npcEntity = npcEnt;
            m_eventBus.publishImmediate(kDialogueTextEvent, std::any(td));
        }
        return;
    }

    // 获取/创建 DialogueState
    if (!m_world->hasComponent<DialogueState>(npcEnt))
    {
        m_world->addComponent<DialogueState>(npcEnt);
    }
    auto& ds = m_world->getComponent<DialogueState>(npcEnt);

    // 如果当前无活跃对话，从头开始
    if (ds.currentNodeId == 0)
    {
        auto& tree = m_trees[treeId];
        ds.dialogueTreeId = 1; // 标记为活跃
        ds.currentNodeId  = tree.startNodeId();
        ds.visitedNodes.clear();
    }

    spdlog::info("DialogueSystem: processing node {} for NPC '{}'",
                 ds.currentNodeId, speaker.characterId);

    evaluateNode(*m_world, npcEnt);
}

// =========================================================================
// 选项选中事件处理
// =========================================================================
void DialogueSystem::onOptionSelected(const Event& event)
{
    const auto* data = std::any_cast<DialogueOptionSelectedData>(&event.data);
    if (!data) return;

    // 需要找到当前对话中的 NPC（通过查询 DialogueState 组件）
    // 简化方案：遍历所有有 DialogueState 的实体，找到 current != 0 的那个
    Entity activeNpc = kNullEntity;
    const DialogueNode* choiceNode = nullptr;
    std::string treeId;

    for (auto e : m_world->view<DialogueState>())
    {
        auto& ds = m_world->getComponent<DialogueState>(e);
        if (ds.currentNodeId != 0 && m_world->hasComponent<DialogueSpeaker>(e))
        {
            auto& speaker = m_world->getComponent<DialogueSpeaker>(e);
            treeId = speaker.characterId + "_intro";
            if (ensureTreeLoaded(treeId))
            {
                auto& tree = m_trees[treeId];
                choiceNode = tree.getNode(ds.currentNodeId);
                if (choiceNode && choiceNode->type == narrative::DialogueNodeType::Choice)
                {
                    activeNpc = e;
                    break;
                }
            }
        }
    }

    if (activeNpc == kNullEntity || !choiceNode)
    {
        spdlog::warn("DialogueSystem: option selected but no active choice dialogue");
        return;
    }

    int idx = data->optionIndex;
    if (idx < 0 || idx >= static_cast<int>(choiceNode->options.size()))
    {
        spdlog::warn("DialogueSystem: invalid option index {}", idx);
        return;
    }

    uint32_t nextId = choiceNode->options[idx].nextNode;

    spdlog::info("DialogueSystem: player chose option {}, advancing to node {}",
                 idx, nextId);

    auto& ds = m_world->getComponent<DialogueState>(activeNpc);

    if (nextId == 0)
    {
        // 对话结束
        ds.currentNodeId = 0;
        DialogueEndData endData;
        m_eventBus.publishImmediate(kDialogueEndEvent, std::any(endData));
    }
    else
    {
        ds.visitedNodes.push_back(ds.currentNodeId);
        ds.currentNodeId = nextId;
        evaluateNode(*m_world, activeNpc);
    }
}

// =========================================================================
// 节点求值 — 核心逻辑
// =========================================================================
void DialogueSystem::evaluateNode(World& world, Entity npcEntity)
{
    if (!world.hasComponent<DialogueState>(npcEntity)) return;
    if (!world.hasComponent<DialogueSpeaker>(npcEntity)) return;

    auto& ds = world.getComponent<DialogueState>(npcEntity);
    auto& speaker = world.getComponent<DialogueSpeaker>(npcEntity);

    std::string treeId = speaker.characterId + "_intro";
    if (!ensureTreeLoaded(treeId)) return;

    auto& tree = m_trees[treeId];
    const auto* node = tree.getNode(ds.currentNodeId);

    if (!node)
    {
        spdlog::warn("DialogueSystem: node {} not found in tree '{}'",
                     ds.currentNodeId, treeId);
        ds.currentNodeId = 0;
        m_eventBus.publishImmediate(kDialogueEndEvent, std::any(DialogueEndData{}));
        return;
    }

    spdlog::debug("DialogueSystem: evaluating node {} (type={})",
                  node->nodeId, static_cast<int>(node->type));

    switch (node->type)
    {
    // =====================================================================
    case narrative::DialogueNodeType::Text:
    {
        DialogueTextData td;
        td.speaker   = node->speaker;
        td.text      = node->text;
        td.npcEntity = npcEntity;
        m_eventBus.publishImmediate(kDialogueTextEvent, std::any(td));

        ds.visitedNodes.push_back(ds.currentNodeId);

        if (node->nextNode == 0)
        {
            ds.currentNodeId = 0;
            m_eventBus.publishImmediate(kDialogueEndEvent, std::any(DialogueEndData{}));
        }
        else
        {
            ds.currentNodeId = node->nextNode;
            // 不自动推进：等待玩家按 E 继续
        }
        break;
    }

    // =====================================================================
    case narrative::DialogueNodeType::Choice:
    {
        DialogueChoiceData cd;
        cd.optionTexts.reserve(node->options.size());
        for (const auto& opt : node->options)
        {
            // 检查选项条件（使用 ConditionEvaluator，回退到本地 flags）
            bool visible = true;
            if (!opt.condition.empty())
            {
                if (m_conditionEval)
                    visible = m_conditionEval->evaluate(opt.condition);
                else
                {
                    // 回退：简单 flag: 检查
                    auto eqPos = opt.condition.find("==");
                    if (eqPos != std::string::npos)
                    {
                        std::string left = opt.condition.substr(0, eqPos);
                        while (!left.empty() && left.back() == ' ') left.pop_back();
                        if (left.find("flag:") == 0)
                        {
                            auto flagIt = ds.flags.find(left.substr(5));
                            visible = (flagIt != ds.flags.end() && flagIt->second);
                        }
                    }
                }
            }
            if (visible)
                cd.optionTexts.push_back(opt.text);
            else
                cd.optionTexts.push_back(""); // 不可见选项留空
        }

        m_eventBus.publishImmediate(kDialogueChoiceEvent, std::any(cd));
        // 不推进节点：等待玩家选择
        break;
    }

    // =====================================================================
    case narrative::DialogueNodeType::Condition:
    {
        // 使用 ConditionEvaluator 求值条件表达式
        bool result = false;
        if (m_conditionEval)
        {
            result = m_conditionEval->evaluate(node->conditionExpr);
        }
        else
        {
            // 回退：简单 flag:key == bool 检查
            const auto& expr = node->conditionExpr;
            auto eqPos = expr.find("==");
            if (eqPos != std::string::npos)
            {
                std::string left = expr.substr(0, eqPos);
                while (!left.empty() && left.back() == ' ') left.pop_back();
                std::string right = expr.substr(eqPos + 2);
                while (!right.empty() && right.front() == ' ') right.erase(0, 1);
                if (left.find("flag:") == 0)
                {
                    auto flagIt = ds.flags.find(left.substr(5));
                    result = (flagIt != ds.flags.end() && flagIt->second == (right == "true"));
                }
            }
        }

        ds.visitedNodes.push_back(ds.currentNodeId);
        ds.currentNodeId = result ? node->trueNode : node->falseNode;

        if (ds.currentNodeId == 0)
        {
            m_eventBus.publishImmediate(kDialogueEndEvent, std::any(DialogueEndData{}));
        }
        else
        {
            evaluateNode(world, npcEntity); // 递归求值下一个节点
        }
        break;
    }

    // =====================================================================
    case narrative::DialogueNodeType::SetFlag:
    {
        ds.flags[node->flagKey] = node->flagValue;
        spdlog::debug("DialogueSystem: set flag '{}' = {}",
                      node->flagKey, node->flagValue ? "true" : "false");

        ds.visitedNodes.push_back(ds.currentNodeId);
        ds.currentNodeId = node->nextNode;

        if (ds.currentNodeId == 0)
        {
            m_eventBus.publishImmediate(kDialogueEndEvent, std::any(DialogueEndData{}));
        }
        else
        {
            evaluateNode(world, npcEntity);
        }
        break;
    }

    // =====================================================================
    case narrative::DialogueNodeType::AIFallback:
    {
        // 构建 LLM 请求，包含真实 ECS 上下文
        nlohmann::json request;
        request["type"] = "llm_dialogue";
        request["payload"]["npc"]["name"] = speaker.characterId;
        request["payload"]["npc"]["personality"] = speaker.personalityPrompt;

        // 读取 AICharacterContext（如果存在）
        if (world.hasComponent<AICharacterContext>(npcEntity))
        {
            auto& aiCtx = world.getComponent<AICharacterContext>(npcEntity);
            request["payload"]["npc"]["background"] = aiCtx.backgroundStory;
            request["payload"]["npc"]["mood"] = aiCtx.emotionalState;
        }
        else
        {
            request["payload"]["npc"]["background"] = "这个世界的一位居民。";
        }

        // AI 上下文提示
        if (!node->aiContextJson.empty())
        {
            try
            {
                request["payload"]["context"] = nlohmann::json::parse(node->aiContextJson);
            }
            catch (...) {}
        }

        // 读取玩家真实数据
        auto playerView = world.view<Transform, Player>();
        for (auto pe : playerView)
        {
            if (world.hasComponent<Stats>(pe))
            {
                auto& stats = world.getComponent<Stats>(pe);
                request["payload"]["player"]["hp"] = stats.get("hp", 100);
                request["payload"]["player"]["level"] = stats.get("level", 1);
                break;
            }
        }

        // 对话历史
        auto& historyArr = request["payload"]["history"];
        historyArr = nlohmann::json::array();
        for (auto nodeId : ds.visitedNodes)
        {
            const auto* histNode = tree.getNode(nodeId);
            if (histNode && histNode->type == narrative::DialogueNodeType::Text)
            {
                nlohmann::json entry;
                entry["speaker"] = histNode->speaker;
                entry["text"]    = histNode->text;
                historyArr.push_back(entry);
            }
        }

        m_pendingNpcName   = speaker.characterId;
        m_pendingNpcEntity = npcEntity;

        if (m_aiClient.startRequest(request))
        {
            m_waitingForAI = true;
            spdlog::info("DialogueSystem: AI fallback for '{}'...", m_pendingNpcName);
        }
        else
        {
            // AI 不可用，显示默认文本
            DialogueTextData td;
            td.speaker   = speaker.characterId;
            td.text      = "...（" + speaker.characterId + " 暂时无法回应）";
            td.npcEntity = npcEntity;
            m_eventBus.publishImmediate(kDialogueTextEvent, std::any(td));

            ds.currentNodeId = 0;
            m_eventBus.publishImmediate(kDialogueEndEvent, std::any(DialogueEndData{}));
        }
        break;
    }

    // =====================================================================
    case narrative::DialogueNodeType::End:
    {
        ds.currentNodeId = 0;
        m_eventBus.publishImmediate(kDialogueEndEvent, std::any(DialogueEndData{}));
        break;
    }
    } // switch
}

// =========================================================================
// 确保对话树已加载
// =========================================================================
bool DialogueSystem::ensureTreeLoaded(const std::string& treeId)
{
    if (m_trees.find(treeId) != m_trees.end())
        return true;

    // 尝试按文件名加载
    std::string path = "assets/dialogues/" + treeId + ".json";
    std::ifstream file(path);
    if (file.is_open())
    {
        std::stringstream buf;
        buf << file.rdbuf();
        try
        {
            auto j = nlohmann::json::parse(buf.str());
            narrative::DialogueTree tree;
            if (tree.loadFromJson(j))
            {
                m_trees[tree.treeId()] = std::move(tree);
                return true;
            }
        }
        catch (const std::exception& e)
        {
            spdlog::error("DialogueSystem: parse error '{}': {}", path, e.what());
        }
    }

    return false;
}

} // namespace engine::ecs
