/**
 * @file engine/narrative/story_flags.h
 * @brief 全局剧情标记存储 — 键值对（bool/int/float/string）
 *
 * 用于：
 *   - 对话树条件分支（flag:met_chen == true）
 *   - 任务前置条件
 *   - 剧情结果追踪
 *
 * 设计：
 *   - 全局单例，由 Engine::Impl 持有
 *   - 支持 toJson/fromJson 存档
 */

#pragma once

#include <string>
#include <variant>
#include <unordered_map>
#include <nlohmann/json_fwd.hpp>

namespace engine::narrative {

class StoryFlags
{
public:
    using FlagValue = std::variant<bool, int, float, std::string>;

    // --- 设置 ---
    void setBool(const std::string& key, bool value);
    void setInt(const std::string& key, int value);
    void setFloat(const std::string& key, float value);
    void setString(const std::string& key, const std::string& value);

    // --- 获取 ---
    bool        getBool(const std::string& key, bool defaultVal = false) const;
    int         getInt(const std::string& key, int defaultVal = 0) const;
    float       getFloat(const std::string& key, float defaultVal = 0.0f) const;
    std::string getString(const std::string& key, const std::string& defaultVal = "") const;

    // 获取原始值（用于条件求值器）
    const FlagValue* get(const std::string& key) const;

    // --- 查询 ---
    bool has(const std::string& key) const;
    void clear();
    size_t size() const { return m_flags.size(); }

    // --- 序列化 ---
    nlohmann::json toJson() const;
    void fromJson(const nlohmann::json& j);

private:
    std::unordered_map<std::string, FlagValue> m_flags;
};

} // namespace engine::narrative
