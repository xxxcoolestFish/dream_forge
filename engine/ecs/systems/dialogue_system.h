/**
 * @file engine/ecs/systems/dialogue_system.h
 * @brief 对话系统 — E 键触发 NPC LLM 对话
 */

#pragma once

#include "engine/ecs/system.h"
#include <string>

namespace engine::input { class InputSystem; }
namespace engine::ai    { class AiClient; }

namespace engine::ecs {

class DialogueSystem : public System
{
public:
    DialogueSystem(input::InputSystem& input, ai::AiClient& aiClient);

    void onUpdate(World& world, double dt) override;

private:
    input::InputSystem& m_input;
    ai::AiClient&       m_aiClient;
    bool                m_waitingForResponse = false;
    std::string         m_pendingNpcName;
    double              m_responseTimer = 0.0;
};

} // namespace engine::ecs
