/**
 * @file engine/narrative/condition_evaluator.h
 * @brief 条件表达式求值器 — 解析 "flag:key == value" 等条件字符串
 *
 * 支持的表达式格式：
 *   flag:KEY == BOOL|INT|STRING
 *   quest:ID == active|completed|failed
 *   and:COND1,COND2,...
 *   or:COND1,COND2,...
 *   not:COND
 *   true / false (字面量)
 *
 * 运算符：==  !=  >=  <=  >  <
 */

#pragma once

#include <string>
#include <functional>

namespace engine::narrative {

class StoryFlags;
class QuestManager;

// 外部查询回调：检查一个 flag 的值
// 返回 variant<bool,int,float,string> 或空
using FlagQueryFn = std::function<bool(const std::string& key, bool& out)>;
using QuestStatusFn = std::function<bool(const std::string& questId, std::string& outStatus)>;

class ConditionEvaluator
{
public:
    ConditionEvaluator() = default;

    // 设置外部查询回调
    void setFlagQuery(FlagQueryFn fn)   { m_flagQuery = std::move(fn); }
    void setQuestQuery(QuestStatusFn fn) { m_questQuery = std::move(fn); }

    // 求值条件表达式
    bool evaluate(const std::string& condition) const;

private:
    FlagQueryFn    m_flagQuery;
    QuestStatusFn  m_questQuery;

    // 解析单个条件子句（不含 and/or/not）
    bool evalAtomic(const std::string& cond) const;
    // 比较两个字符串值
    bool compareStr(const std::string& left, const std::string& op,
                    const std::string& right) const;
};

} // namespace engine::narrative
