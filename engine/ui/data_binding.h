/**
 * @file engine/ui/data_binding.h
 * @brief 数据绑定系统 — 连接 UI 控件与 ECS 数据
 *
 * 支持的绑定源：
 *   - ecs: 读取 ECS 组件字段
 *   - static: 静态常量
 *
 * JSON 示例：
 *   {
 *     "type": "Text",
 *     "text": "HP: 100",
 *     "binding": {
 *       "target": "text",        // 绑定到控件的哪个属性
 *       "source": "ecs",         // ecs | static
 *       "path": "player.hp",     // ECS: component_entity_key.field
 *       "format": "HP: {:.0f}"
 *     }
 *   }
 */

#pragma once

#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>

namespace engine::ecs { class World; }

namespace engine::ui {

// 单条绑定
struct Binding
{
    std::string target;  // 控件属性名 ("text", "color", "visible")
    std::string source;  // 数据源 ("ecs", "static")
    std::string path;    // 数据路径
    std::string format;  // 格式化字符串

    static Binding fromJson(const nlohmann::json& j);
};

// 绑定结果（数据源 → 值）
class BindingContext
{
public:
    BindingContext();
    ~BindingContext();

    // 设置 ECS World（从外部注入）
    void setEcsWorld(engine::ecs::World* world);

    // 解析绑定的值
    std::string resolve(const Binding& binding) const;

    // 检查是否有有效的 ECS 数据源
    bool hasEcs() const { return m_ecsWorld != nullptr; }

private:
    engine::ecs::World* m_ecsWorld = nullptr;

    // 解析 ECS 路径："component.entity_key.field"
    // e.g., "Player::hp" → find entity with Player component → get Stats → get "hp"
    std::string resolveEcs(const std::string& path) const;
};

} // namespace engine::ui
