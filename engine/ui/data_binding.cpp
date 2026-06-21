/**
 * @file engine/ui/data_binding.cpp
 * @brief 数据绑定实现
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
        value = binding.path; // 静态值就是 path 本身
    }
    else
    {
        return binding.path; // 未知来源，返回原始路径
    }

    // 应用格式化
    if (!binding.format.empty() && binding.format != "{}")
    {
        // 简单格式化：将 {0} 替换为值
        std::string result = binding.format;
        size_t pos = result.find("{0}");
        if (pos != std::string::npos)
            result.replace(pos, 3, value);
        else
        {
            // 尝试 {:.0f} 等浮点格式
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

    // 路径格式："component_key.field"
    // e.g., "Player.hp" → 找 Player 组件 → Stats 组件 → hp 字段
    // e.g., "Player.xp"
    // e.g., "Player.position.x"

    size_t dotPos = path.find('.');
    if (dotPos == std::string::npos) return "0";

    std::string entityKey = path.substr(0, dotPos);
    std::string field     = path.substr(dotPos + 1);

    // 查找带 Player 组件的 Entity
    auto playerView = m_ecsWorld->view<engine::ecs::Player>();
    if (playerView.begin() == playerView.end()) return "0";

    auto entity = *playerView.begin();

    // 根据字段名读取组件
    if (field == "hp" || field == "xp" || field == "level")
    {
        if (m_ecsWorld->hasComponent<engine::ecs::Stats>(entity))
        {
            auto& stats = m_ecsWorld->getComponent<engine::ecs::Stats>(entity);
            float val = stats.get(field, 0.0f);
            std::ostringstream oss;
            oss << std::fixed << std::setprecision(0) << val;
            return oss.str();
        }
    }
    else if (field == "pos_x" || field == "x")
    {
        auto& t = m_ecsWorld->getComponent<engine::ecs::Transform>(entity);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << t.position.x;
        return oss.str();
    }
    else if (field == "pos_y" || field == "y")
    {
        auto& t = m_ecsWorld->getComponent<engine::ecs::Transform>(entity);
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(0) << t.position.y;
        return oss.str();
    }

    return "0";
}

} // namespace engine::ui
