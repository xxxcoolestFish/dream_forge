/**
 * @file engine/narrative/condition_evaluator.cpp
 * @brief 条件表达式求值器实现
 */

#include "engine/narrative/condition_evaluator.h"
#include <spdlog/spdlog.h>
#include <algorithm>
#include <sstream>

namespace engine::narrative {

// =========================================================================
// 辅助：去除首尾空格
// =========================================================================
static std::string trim(const std::string& s)
{
    size_t start = s.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(" \t\r\n");
    return s.substr(start, end - start + 1);
}

// =========================================================================
// 辅助：按分隔符拆分字符串
// =========================================================================
static std::vector<std::string> splitBy(const std::string& s, char delim)
{
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;
    while (std::getline(ss, item, delim))
        result.push_back(trim(item));
    return result;
}

// =========================================================================
// 主入口：evaluate("flag:met_chen == true")
// =========================================================================
bool ConditionEvaluator::evaluate(const std::string& condition) const
{
    std::string cond = trim(condition);
    if (cond.empty() || cond == "true") return true;
    if (cond == "false") return false;

    // and:COND1,COND2,...
    if (cond.rfind("and:", 0) == 0)
    {
        std::string inner = cond.substr(4);
        auto parts = splitBy(inner, ',');
        for (const auto& part : parts)
        {
            if (!evaluate(part))
                return false;
        }
        return true;
    }

    // or:COND1,COND2,...
    if (cond.rfind("or:", 0) == 0)
    {
        std::string inner = cond.substr(3);
        auto parts = splitBy(inner, ',');
        for (const auto& part : parts)
        {
            if (evaluate(part))
                return true;
        }
        return false;
    }

    // not:COND
    if (cond.rfind("not:", 0) == 0)
    {
        std::string inner = cond.substr(4);
        return !evaluate(inner);
    }

    return evalAtomic(cond);
}

// =========================================================================
// 原子条件求值：flag:KEY OP VALUE 或 quest:ID OP STATUS
// =========================================================================
bool ConditionEvaluator::evalAtomic(const std::string& cond) const
{
    // 查找比较运算符（按长度降序匹配，避免 == 被 = 匹配）
    static const char* kOps[] = { "==", "!=", ">=", "<=", ">", "<" };
    std::string foundOp;
    size_t opPos = std::string::npos;

    for (const char* op : kOps)
    {
        size_t pos = cond.find(op);
        if (pos != std::string::npos)
        {
            foundOp = op;
            opPos = pos;
            break;
        }
    }

    if (opPos == std::string::npos)
    {
        spdlog::warn("ConditionEvaluator: no operator found in '{}'", cond);
        return false;
    }

    std::string left  = trim(cond.substr(0, opPos));
    std::string right = trim(cond.substr(opPos + foundOp.size()));

    // flag:KEY
    if (left.rfind("flag:", 0) == 0)
    {
        std::string key = left.substr(5);
        if (!m_flagQuery)
        {
            spdlog::warn("ConditionEvaluator: no flag query callback set");
            return false;
        }
        bool flagVal = false;
        if (!m_flagQuery(key, flagVal))
            return false; // flag 不存在
        return compareStr(flagVal ? "true" : "false", foundOp, right);
    }

    // quest:ID
    if (left.rfind("quest:", 0) == 0)
    {
        std::string questId = left.substr(6);
        if (!m_questQuery)
        {
            spdlog::warn("ConditionEvaluator: no quest query callback set");
            return false;
        }
        std::string status;
        if (!m_questQuery(questId, status))
            return false; // quest 不存在
        return compareStr(status, foundOp, right);
    }

    spdlog::warn("ConditionEvaluator: unknown condition prefix in '{}'", left);
    return false;
}

// =========================================================================
// 字符串比较
// =========================================================================
bool ConditionEvaluator::compareStr(const std::string& left, const std::string& op,
                                     const std::string& right) const
{
    // 尝试作为数值比较
    bool leftIsNum = false, rightIsNum = false;
    float leftNum = 0.0f, rightNum = 0.0f;

    try { leftNum = std::stof(left); leftIsNum = true; } catch (...) {}
    try { rightNum = std::stof(right); rightIsNum = true; } catch (...) {}

    if (leftIsNum && rightIsNum)
    {
        if (op == "==") return leftNum == rightNum;
        if (op == "!=") return leftNum != rightNum;
        if (op == ">=") return leftNum >= rightNum;
        if (op == "<=") return leftNum <= rightNum;
        if (op == ">")  return leftNum > rightNum;
        if (op == "<")  return leftNum < rightNum;
        return false;
    }

    // 字符串比较
    if (op == "==") return left == right;
    if (op == "!=") return left != right;

    spdlog::warn("ConditionEvaluator: cannot compare '{}' {} '{}'", left, op, right);
    return false;
}

} // namespace engine::narrative
