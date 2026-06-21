/**
 * @file engine/narrative/story_flags.cpp
 * @brief StoryFlags 实现
 */

#include "engine/narrative/story_flags.h"
#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

namespace engine::narrative {

// =========================================================================
// 设置
// =========================================================================
void StoryFlags::setBool(const std::string& key, bool value)
{
    m_flags[key] = value;
    spdlog::debug("StoryFlags: {} = {} (bool)", key, value ? "true" : "false");
}

void StoryFlags::setInt(const std::string& key, int value)
{
    m_flags[key] = value;
    spdlog::debug("StoryFlags: {} = {} (int)", key, value);
}

void StoryFlags::setFloat(const std::string& key, float value)
{
    m_flags[key] = value;
    spdlog::debug("StoryFlags: {} = {:.2f} (float)", key, value);
}

void StoryFlags::setString(const std::string& key, const std::string& value)
{
    m_flags[key] = value;
    spdlog::debug("StoryFlags: {} = \"{}\" (string)", key, value);
}

// =========================================================================
// 获取
// =========================================================================
bool StoryFlags::getBool(const std::string& key, bool defaultVal) const
{
    auto it = m_flags.find(key);
    if (it == m_flags.end()) return defaultVal;
    if (std::holds_alternative<bool>(it->second))
        return std::get<bool>(it->second);
    return defaultVal;
}

int StoryFlags::getInt(const std::string& key, int defaultVal) const
{
    auto it = m_flags.find(key);
    if (it == m_flags.end()) return defaultVal;
    if (std::holds_alternative<int>(it->second))
        return std::get<int>(it->second);
    if (std::holds_alternative<float>(it->second))
        return static_cast<int>(std::get<float>(it->second));
    return defaultVal;
}

float StoryFlags::getFloat(const std::string& key, float defaultVal) const
{
    auto it = m_flags.find(key);
    if (it == m_flags.end()) return defaultVal;
    if (std::holds_alternative<float>(it->second))
        return std::get<float>(it->second);
    if (std::holds_alternative<int>(it->second))
        return static_cast<float>(std::get<int>(it->second));
    return defaultVal;
}

std::string StoryFlags::getString(const std::string& key, const std::string& defaultVal) const
{
    auto it = m_flags.find(key);
    if (it == m_flags.end()) return defaultVal;
    if (std::holds_alternative<std::string>(it->second))
        return std::get<std::string>(it->second);
    return defaultVal;
}

const StoryFlags::FlagValue* StoryFlags::get(const std::string& key) const
{
    auto it = m_flags.find(key);
    return (it != m_flags.end()) ? &it->second : nullptr;
}

bool StoryFlags::has(const std::string& key) const
{
    return m_flags.find(key) != m_flags.end();
}

void StoryFlags::clear()
{
    m_flags.clear();
}

// =========================================================================
// 序列化
// =========================================================================
nlohmann::json StoryFlags::toJson() const
{
    nlohmann::json j;
    for (const auto& [key, value] : m_flags)
    {
        std::visit([&](auto&& v) {
            using T = std::decay_t<decltype(v)>;
            if constexpr (std::is_same_v<T, bool>)
                j[key] = v;
            else if constexpr (std::is_same_v<T, int>)
                j[key] = v;
            else if constexpr (std::is_same_v<T, float>)
                j[key] = v;
            else if constexpr (std::is_same_v<T, std::string>)
                j[key] = v;
        }, value);
    }
    return j;
}

void StoryFlags::fromJson(const nlohmann::json& j)
{
    m_flags.clear();
    for (auto& [key, val] : j.items())
    {
        if (val.is_boolean())
            m_flags[key] = val.get<bool>();
        else if (val.is_number_integer())
            m_flags[key] = val.get<int>();
        else if (val.is_number_float())
            m_flags[key] = val.get<float>();
        else if (val.is_string())
            m_flags[key] = val.get<std::string>();
    }
}

} // namespace engine::narrative
