/**
 * @file engine/narrative/dialogue_tree.cpp
 * @brief 对话树加载与查询实现
 */

#include "engine/narrative/dialogue_tree.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <stdexcept>

namespace engine::narrative {

// =========================================================================
// JSON → DialogueNodeType 映射表
// =========================================================================
static const std::unordered_map<std::string, DialogueNodeType> kTypeMap = {
    { "text",         DialogueNodeType::Text },
    { "choice",       DialogueNodeType::Choice },
    { "condition",    DialogueNodeType::Condition },
    { "set_flag",     DialogueNodeType::SetFlag },
    { "ai_fallback",  DialogueNodeType::AIFallback },
    { "end",          DialogueNodeType::End },
};

// =========================================================================
// 加载
// =========================================================================
bool DialogueTree::loadFromJson(const nlohmann::json& j)
{
    if (!j.contains("treeId") || !j.contains("nodes"))
    {
        spdlog::error("DialogueTree: missing 'treeId' or 'nodes' in JSON");
        return false;
    }

    m_treeId      = j["treeId"].get<std::string>();
    m_startNodeId = j.value("startNodeId", 1u);
    m_nodes.clear();

    const auto& nodesJson = j["nodes"];
    if (!nodesJson.is_object())
    {
        spdlog::error("DialogueTree: 'nodes' must be a JSON object");
        return false;
    }

    for (auto& [key, nodeJson] : nodesJson.items())
    {
        uint32_t nodeId = 0;
        try
        {
            nodeId = static_cast<uint32_t>(std::stoul(key));
        }
        catch (const std::exception& e)
        {
            spdlog::error("DialogueTree: invalid node ID '{}': {}", key, e.what());
            return false;
        }

        DialogueNode node;
        if (!parseNode(nodeId, nodeJson, node))
            return false;

        m_nodes[nodeId] = std::move(node);
    }

    spdlog::info("DialogueTree '{}': loaded {} nodes (start={})",
                 m_treeId, m_nodes.size(), m_startNodeId);
    return true;
}

// =========================================================================
// 查询
// =========================================================================
const DialogueNode* DialogueTree::getNode(uint32_t id) const
{
    auto it = m_nodes.find(id);
    if (it != m_nodes.end())
        return &it->second;
    return nullptr;
}

// =========================================================================
// 解析单个节点
// =========================================================================
bool DialogueTree::parseNode(uint32_t id, const nlohmann::json& j, DialogueNode& out)
{
    out.nodeId = id;

    // --- 类型 ---
    std::string typeStr = j.value("type", "");
    auto typeIt = kTypeMap.find(typeStr);
    if (typeIt == kTypeMap.end())
    {
        spdlog::error("DialogueTree: unknown node type '{}' (node {})", typeStr, id);
        return false;
    }
    out.type = typeIt->second;

    // --- 按类型解析 ---
    switch (out.type)
    {
    case DialogueNodeType::Text:
        out.speaker  = j.value("speaker", "???");
        out.text     = j.value("text", "");
        out.nextNode = j.value("next", 0u);
        break;

    case DialogueNodeType::Choice:
        out.choicePrompt = j.value("prompt", "");
        if (j.contains("options") && j["options"].is_array())
        {
            for (const auto& opt : j["options"])
            {
                DialogueOption dopt;
                dopt.text     = opt.value("text", "...");
                dopt.nextNode = opt.value("next", 0u);
                dopt.condition = opt.value("condition", "");
                out.options.push_back(std::move(dopt));
            }
        }
        break;

    case DialogueNodeType::Condition:
        out.conditionExpr = j.value("condition", "");
        out.trueNode  = j.value("trueNode", 0u);
        out.falseNode = j.value("falseNode", 0u);
        break;

    case DialogueNodeType::SetFlag:
        out.flagKey   = j.value("flag", "");
        out.flagValue = j.value("value", true);
        out.nextNode  = j.value("next", 0u);
        break;

    case DialogueNodeType::AIFallback:
        if (j.contains("context"))
            out.aiContextJson = j["context"].dump();
        break;

    case DialogueNodeType::End:
        // 无额外字段
        break;
    }

    return true;
}

} // namespace engine::narrative
