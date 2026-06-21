/**
 * @file engine/ecs/systems/dialogue_system.cpp
 * @brief E 键触发 NPC 对话，LLM 实时生成回复
 *
 * 状态机：Idle →（E键）→ WaitingResponse →（收到回复）→ Idle
 *
 * AI 请求由 engine.run() 主循环中的 pollResponse() 统一处理，
 * 本 System 只负责：检测 E 键 → 构建请求 → 解析响应 → 打印结果。
 */

#include "engine/ecs/systems/dialogue_system.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"
#include "engine/input/input_system.h"
#include "engine/ai/ai_client.h"

#include <GLFW/glfw3.h>
#include <spdlog/spdlog.h>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>
#include <cmath>
#include <typeinfo>

namespace engine::ecs {

DialogueSystem::DialogueSystem(input::InputSystem& input, ai::AiClient& aiClient)
    : System("DialogueSystem")
    , m_input(input)
    , m_aiClient(aiClient)
{
}

void DialogueSystem::onUpdate(World& world, double dt)
{
    try
    {
        // --- 轮询响应 ---
        if (m_waitingForResponse)
        {
        auto response = m_aiClient.pollResponse();
        if (response.has_value())
        {
            const auto& resp = *response;

            if (resp.empty() || !resp.contains("data"))
            {
                spdlog::warn("AI 服务未响应或返回错误。请确保 AI 服务已启动。");
                spdlog::info("启动命令：python ai_service/main.py --model qwen2.5:7b");
                m_waitingForResponse = false;
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
                {
                    spdlog::info("    {}. {}", i++, opt.get<std::string>());
                }
            }

            spdlog::info("  (按 E 继续对话)");
            m_waitingForResponse = false;
        }
        return;
    }

    // --- 检测 E 键 ---
    if (!m_input.isKeyPressed(GLFW_KEY_E)) return;

    // --- 找到玩家 ---
    auto playerView = world.view<Transform, Player>();
    if (playerView.begin() == playerView.end()) return;

    auto playerEntity = *playerView.begin();
    auto& playerTransform = world.getComponent<Transform>(playerEntity);

    // --- 找最近的 NPC ---
    Entity nearestNpc = kNullEntity;
    float   nearestDist = 100.0f;

    auto npcView = world.view<Transform, DialogueSpeaker>();
    for (auto entity : npcView)
    {
        if (entity == playerEntity) continue;
        auto& t = world.getComponent<Transform>(entity);
        float dx = t.position.x - playerTransform.position.x;
        float dy = t.position.y - playerTransform.position.y;
        float dist = std::sqrt(dx * dx + dy * dy);
        if (dist < nearestDist)
        {
            nearestDist = dist;
            nearestNpc = entity;
        }
    }

    if (nearestNpc == kNullEntity)
    {
        spdlog::info("附近没有可以对话的 NPC（距离 > 100 像素）");
        return;
    }

    // --- 构建请求 ---
    auto& npcSpeaker = world.getComponent<DialogueSpeaker>(nearestNpc);
    m_pendingNpcName = npcSpeaker.characterId;

    spdlog::info("[DIALOGUE] step 1: E key detected, NPC={}", m_pendingNpcName);
    spdlog::default_logger()->flush();

    // 先用最简单的请求测试，排除 json 构造问题
    nlohmann::json request;
    spdlog::info("[DIALOGUE] step 1b: json object created");
    spdlog::default_logger()->flush();

    request["type"] = "llm_dialogue";
    spdlog::info("[DIALOGUE] step 1c: type set");
    spdlog::default_logger()->flush();

    request["payload"] = nlohmann::json::object();
    request["payload"]["npc"] = nlohmann::json::object();
    request["payload"]["npc"]["name"] = npcSpeaker.characterId;
    request["payload"]["npc"]["personality"] = npcSpeaker.personalityPrompt;
    request["payload"]["player"] = nlohmann::json::object();
    request["payload"]["player"]["hp"] = 100;
    request["payload"]["history"] = nlohmann::json::array();

    spdlog::info("[DIALOGUE] step 1d: payload built");
    spdlog::default_logger()->flush();

    spdlog::info("[DIALOGUE] step 2: calling startRequest...");
    spdlog::default_logger()->flush();

    if (m_aiClient.startRequest(request))
    {
        spdlog::info("[DIALOGUE] step 3: startRequest OK");
        spdlog::default_logger()->flush();
        m_waitingForResponse = true;
    }
    else
    {
        spdlog::warn("[DIALOGUE] startRequest FAILED");
        spdlog::default_logger()->flush();
    }
    spdlog::info("[DIALOGUE] step 4: returning");
    spdlog::default_logger()->flush();
    }
    catch (const std::exception& e)
    {
        spdlog::error("DialogueSystem exception: {} ({})", e.what(), typeid(e).name());
        m_waitingForResponse = false;
    }
    catch (...)
    {
        spdlog::error("DialogueSystem unknown exception");
        m_waitingForResponse = false;
    }
}

} // namespace engine::ecs
