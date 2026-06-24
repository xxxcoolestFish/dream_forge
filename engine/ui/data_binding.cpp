/**
 * @file engine/ui/data_binding.cpp
 * @brief 数据绑定实现 — 通用 stat key 解析
 */

#include "engine/ui/data_binding.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"

#include <spdlog/spdlog.h>
#include <sstream>
#include <iomanip>

namespace engine::ui {

Binding Binding::fromJson(const nlohmann::json& j)
{
    Binding b;
    b.target = j.value("target", "text");
    b.source = j.value("source", "static");
    b.path   = j.value("path", "");
    b.format = j.value("format", "{}");
    return b;
}

BindingContext::BindingContext()  = default;
BindingContext::~BindingContext() = default;

void BindingContext::setEcsWorld(engine::ecs::World* world)
{
    m_ecsWorld = world;
}

std::string BindingContext::resolve(const Binding& binding) const
{
    std::string value;

    if (binding.source == "ecs")
    {
        value = resolveEcs(binding.path);
    }
    else if (binding.source == "static")
    {
        value = binding.path;
    }
    else
    {
        return binding.path;
    }

    if (!binding.format.empty() && binding.format != "{}")
    {
        std::string result = binding.format;
        size_t pos = result.find("{0}");
        if (pos != std::string::npos)
            result.replace(pos, 3, value);
        else
        {
            try
            {
                double num = std::stod(value);
                char buf[128];
                snprintf(buf, sizeof(buf), binding.format.c_str(), num);
                result = buf;
            }
            catch (...)
            {
                result = value;
            }
        }
        return result;
    }

    return value;
}

std::string BindingContext::resolveEcs(const std::string& path) const
{
    if (!m_ecsWorld) return "0";

    // 路径格式: "EntityKey.field"
    //   "Player.hp"     → Stats::get("hp")
    //   "Player.money"  → Inventory::money
    //   "Player.items"  → Inventory::items.size()
    //   "Player.pos_x"  → Transform::position.x
    // field 为任意已注册的 stat key 均可工作

    size_t dotPos = path.find('.');
    if (dotPos == std::string::npos) return "0";

    std::string entityKey = path.substr(0, dotPos);
    std::string field     = path.substr(dotPos + 1);

    auto playerView = m_ecsWorld->view<engine::ecs::Player>();
    if (playerView.begin() == playerView.end()) return "0";

    auto entity = *playerView.begin();

    std::ostringstream oss;
    oss << std::fixed << std::setprecision(0);

    // 位置字段
    if (field == "pos_x" || field == "x")
    {
        if (m_ecsWorld->hasComponent<engine::ecs::Transform>(entity))
            oss << m_ecsWorld->getComponent<engine::ecs::Transform>(entity).position.x;
    }
    else if (field == "pos_y" || field == "y")
    {
        if (m_ecsWorld->hasComponent<engine::ecs::Transform>(entity))
            oss << m_ecsWorld->getComponent<engine::ecs::Transform>(entity).position.y;
    }
    // 金钱
    else if (field == "money")
    {
        if (m_ecsWorld->hasComponent<engine::ecs::Inventory>(entity))
            oss << m_ecsWorld->getComponent<engine::ecs::Inventory>(entity).money;
    }
    // 物品数量
    else if (field == "items")
    {
        if (m_ecsWorld->hasComponent<engine::ecs::Inventory>(entity))
            oss << m_ecsWorld->getComponent<engine::ecs::Inventory>(entity).items.size();
    }
    // 通用: 任意 Stats key（不再硬编码 hp/xp/level）
    else if (m_ecsWorld->hasComponent<engine::ecs::Stats>(entity))
    {
        auto& stats = m_ecsWorld->getComponent<engine::ecs::Stats>(entity);
        if (stats.values.find(field) != stats.values.end())
            oss << stats.get(field, 0.0f);
        else
            oss << "0";
    }
    else
    {
        oss << "0";
    }

    return oss.str();
}

} // namespace engine::ui
