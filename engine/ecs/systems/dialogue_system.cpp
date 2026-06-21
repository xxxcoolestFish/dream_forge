/**
 * @file engine/ecs/systems/dialogue_system.cpp
 * @brief 对话系统 — EventBus 驱动
 */

#include "engine/ecs/systems/dialogue_system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"
#include "engine/ai/ai_client.h"

#include <spdlog/spdlog.h>
#include <nlohmann/json.hpp>

namespace engine::ecs {

// 与 InteractionSystem 使用相同的事件 ID
static constexpr EventTypeId kInteractionEvent = ENGINE_EVENT("Interaction");

struct InteractionData
{
    Entity      sourceEntity;
    Entity      targetEntity;
    std::string type;
};

DialogueSystem::DialogueSystem(ai::AiClient& aiClient, EventBus& eventBus)
    : System("DialogueSystem")
    , m_aiClient(aiClient)
    , m_eventBus(eventBus)
{
}

void DialogueSystem::onInit(World& world)
{
    m_world = &world;

    // 订阅交互事件
    m_subscriptionId = m_eventBus.subscribe(kInteractionEvent,
        [this](const Event& event) {
            onInteraction(event);
        });

    spdlog::debug("DialogueSystem subscribed to InteractionEvent (id={})", m_subscriptionId);
}

void DialogueSystem::onUpdate(World& world, double dt)
{
    (void)world;

    // --- 轮询 AI 响应 ---
    if (m_waitingForResponse)
    {
        m_responseTimer += dt;
        auto response = m_aiClient.pollResponse();
        if (response.has_value())
        {
            const auto& resp = *response;

            if (resp.empty() || !resp.contains("data"))
            {
                spdlog::warn("AI 服务未响应。请确保 AI 服务已启动。");
                m_waitingForResponse = false;
                m_responseTimer = 0.0;
                return;
            }

            const auto& data = resp["data"];
            std::string text = data.value("text", "(NPC 沉默不语...)");

            spdlog::info("");
            spdlog::info("╔══════════════════════════════════════╗");
            spdlog::info("║  {} 说：", m_pendingNpcName);
            spdlog::info("║  \"{}\"", text);
            spdlog::info("╚══════════════════════════════════════╝");
            spdlog::info("");

            if (data.contains("options") && data["options"].is_array())
            {
                spdlog::info("  可选回复：");
                int i = 1;
                for (const auto& opt : data["options"])
                    spdlog::info("    {}. {}", i++, opt.get<std::string>());
            }

            spdlog::info("  (按 E 继续对话)");
            m_waitingForResponse = false;
            m_responseTimer = 0.0;
        }
        else if (m_responseTimer > 5.0)
        {
            spdlog::info("  等待 AI 服务响应中... ({}s)", static_cast<int>(m_responseTimer));
            m_responseTimer = 0.0;
        }
    }
}

void DialogueSystem::onInteraction(const Event& event)
{
    if (m_waitingForResponse) return; // 正在等待上一个回复

    const auto* data = std::any_cast<InteractionData>(&event.data);
    if (!data) return;

    // 只处理 "talk" 类型的交互
    if (data->type != "talk") return;

    // 获取 NPC 的 DialogueSpeaker 组件
    if (!m_world || !m_world->isValid(data->targetEntity)) return;
    if (!m_world->hasComponent<DialogueSpeaker>(data->targetEntity)) return;

    auto& speaker = m_world->getComponent<DialogueSpeaker>(data->targetEntity);
    m_pendingNpcName = speaker.characterId;

    // 构建 LLM 请求
    nlohmann::json request;
    request["type"] = "llm_dialogue";
    request["payload"]["npc"]["name"] = speaker.characterId;
    request["payload"]["npc"]["personality"] = speaker.personalityPrompt;
    request["payload"]["npc"]["background"] = "这个世界的一位居民。";
    request["payload"]["player"]["hp"] = 100;
    request["payload"]["player"]["level"] = 1;
    request["payload"]["history"] = nlohmann::json::array();

    spdlog::info("与 {} 对话中...", m_pendingNpcName);

    if (m_aiClient.startRequest(request))
    {
        m_waitingForResponse = true;
    }
    else
    {
        spdlog::warn("无法发送对话请求（AI 服务未启动？）");
    }
}

} // namespace engine::ecs
