/**
 * @file engine/narrative/dialogue_tree.h
 * @brief 对话树 — JSON 定义的 NPC 对话节点图
 *
 * 负责：
 *   - 从 JSON 加载对话节点定义
 *   - 提供节点查询接口
 *   - 节点类型：text / choice / condition / set_flag / ai_fallback / end
 *
 * 设计：
 *   - 纯数据结构，不依赖 ECS 或渲染
 *   - 节点 ID 使用 uint32_t，在 JSON 中以字符串键存储
 *   - 条件表达式字符串由 ConditionEvaluator（5.5）运行时求值
 */

#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <nlohmann/json_fwd.hpp>

namespace engine::narrative {

// =========================================================================
// 节点类型枚举
// =========================================================================
enum class DialogueNodeType : uint8_t
{
    Text,        // NPC 说一段话 → 自动推进到 next 节点
    Choice,      // 玩家从选项中选择 → 跟随选中分支
    Condition,   // 条件分支 → 根据条件跳转 trueNode 或 falseNode
    SetFlag,     // 设置标记 → 自动推进到 next 节点
    AIFallback,  // 调用 AI 生成回复 → 显示后继续
    End          // 对话结束
};

// =========================================================================
// 单个选项（Choice 节点中使用）
// =========================================================================
struct DialogueOption
{
    std::string text;          // 选项显示文本
    uint32_t    nextNode = 0;  // 选中后跳转到的节点 ID
    std::string condition;     // 可选：显示条件（为空表示始终可见）
};

// =========================================================================
// 对话节点
// =========================================================================
struct DialogueNode
{
    DialogueNodeType type = DialogueNodeType::Text;
    uint32_t         nodeId = 0;

    // --- Text 节点字段 ---
    std::string speaker;  // 说话者名称
    std::string text;     // 对话文本
    uint32_t    nextNode = 0; // 文本显示完毕后跳转（0 = 结束）

    // --- Choice 节点字段 ---
    std::string              choicePrompt;  // 提示语（如"你想说什么？"）
    std::vector<DialogueOption> options;

    // --- Condition 节点字段 ---
    std::string conditionExpr; // 条件表达式（"flag:met_chen == true"）
    uint32_t    trueNode  = 0;
    uint32_t    falseNode = 0;

    // --- SetFlag 节点字段 ---
    std::string flagKey;
    bool        flagValue = true;

    // --- AIFallback 节点字段 ---
    std::string aiContextJson; // 传给 AI 的上下文提示 JSON
};

// =========================================================================
// 对话树
// =========================================================================
class DialogueTree
{
public:
    DialogueTree() = default;

    // 从 JSON 对象加载对话树
    bool loadFromJson(const nlohmann::json& j);

    // 查询节点
    const DialogueNode* getNode(uint32_t id) const;

    // 属性
    const std::string& treeId()      const { return m_treeId; }
    uint32_t           startNodeId() const { return m_startNodeId; }
    bool               isEmpty()     const { return m_nodes.empty(); }
    size_t             nodeCount()   const { return m_nodes.size(); }

private:
    std::string m_treeId;
    uint32_t    m_startNodeId = 0;
    std::unordered_map<uint32_t, DialogueNode> m_nodes;

    // 解析单个节点
    bool parseNode(uint32_t id, const nlohmann::json& j, DialogueNode& out);
};

} // namespace engine::narrative
