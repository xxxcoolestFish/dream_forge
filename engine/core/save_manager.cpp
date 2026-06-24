/**
 * @file engine/core/save_manager.cpp
 * @brief SaveManager 实现 — nlohmann/json 序列化
 */

#include "engine/core/save_manager.h"
#include "engine/ecs/world.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>
#include <fstream>

using json = nlohmann::json;

namespace engine {

// =========================================================================
// 序列化辅助
// =========================================================================

static json serializeItem(ecs::World& world, ecs::Entity e)
{
    auto& item = world.getComponent<ecs::Item>(e);
    json j;
    j["itemId"]       = item.itemId;
    j["name"]         = item.name;
    j["description"]  = item.description;
    j["equipSlot"]    = item.equipSlot;
    j["consumable"]   = item.consumable;

    // useEffects
    json ueArr = json::array();
    for (auto& ue : item.useEffects)
    {
        ueArr.push_back({
            {"stat",       ue.stat},
            {"modify",     ue.modify},
            {"clampToMax", ue.clampToMax}
        });
    }
    j["useEffects"] = ueArr;

    // statModifiers
    json sm;
    for (auto& [k, v] : item.statModifiers)
        sm[k] = v;
    j["statModifiers"] = sm;

    // Equipped 标记
    j["equipped"] = world.hasComponent<ecs::Equipped>(e);
    if (j["equipped"])
    {
        auto& eq = world.getComponent<ecs::Equipped>(e);
        j["equippedSlot"] = eq.slot;
    }

    return j;
}

static json serializeStats(const ecs::Stats& stats)
{
    json j;
    j["values"]    = stats.values;
    j["maxValues"] = stats.maxValues;
    j["minValues"] = stats.minValues;
    return j;
}

// =========================================================================
// 反序列化辅助
// =========================================================================

static ecs::Entity deserializeItem(ecs::World& world, const json& j)
{
    ecs::Entity e = world.createEntity();
    auto& item = world.addComponent<ecs::Item>(e);

    item.itemId      = j.value("itemId", "");
    item.name        = j.value("name", "");
    item.description = j.value("description", "");
    item.equipSlot   = j.value("equipSlot", "");
    item.consumable  = j.value("consumable", false);

    if (j.contains("useEffects") && j["useEffects"].is_array())
    {
        for (auto& ue : j["useEffects"])
        {
            item.useEffects.push_back({
                ue.value("stat", ""),
                ue.value("modify", 0.0f),
                ue.value("clampToMax", true)
            });
        }
    }

    if (j.contains("statModifiers"))
    {
        for (auto& [k, v] : j["statModifiers"].items())
            item.statModifiers[k] = v.get<float>();
    }

    if (j.value("equipped", false))
    {
        auto& eq = world.addComponent<ecs::Equipped>(e);
        eq.slot = j.value("equippedSlot", "");
        // equippedBy 在后续设置
    }

    return e;
}

static void deserializeStats(ecs::Stats& stats, const json& j)
{
    if (j.contains("values"))
        for (auto& [k, v] : j["values"].items())
            stats.values[k] = v.get<float>();
    if (j.contains("maxValues"))
        for (auto& [k, v] : j["maxValues"].items())
            stats.maxValues[k] = v.get<float>();
    if (j.contains("minValues"))
        for (auto& [k, v] : j["minValues"].items())
            stats.minValues[k] = v.get<float>();
}

// =========================================================================
// 主接口
// =========================================================================

bool SaveManager::save(const std::string& filepath,
                       ecs::World& world,
                       const json& storyFlags) const
{
    json root;
    root["version"] = 1;

    // 找玩家
    ecs::Entity player = ecs::kNullEntity;
    for (auto e : world.view<ecs::Player>())
    {
        player = e;
        break;
    }

    // --- 玩家数据 ---
    json pj;

    if (world.hasComponent<ecs::Transform>(player))
    {
        auto& t = world.getComponent<ecs::Transform>(player);
        pj["transform"] = {
            {"x", t.position.x}, {"y", t.position.y}, {"z", t.position.z}
        };
    }

    if (world.hasComponent<ecs::Stats>(player))
        pj["stats"] = serializeStats(world.getComponent<ecs::Stats>(player));

    if (world.hasComponent<ecs::Inventory>(player))
    {
        auto& inv = world.getComponent<ecs::Inventory>(player);
        pj["inventory"]["money"] = inv.money;

        json itemsArr = json::array();
        for (auto itemE : inv.items)
        {
            if (world.isValid(itemE) && world.hasComponent<ecs::Item>(itemE))
                itemsArr.push_back(serializeItem(world, itemE));
        }
        pj["inventory"]["items"] = itemsArr;
    }

    root["player"] = pj;
    root["storyFlags"] = storyFlags;

    // --- 写文件 ---
    std::ofstream ofs(filepath);
    if (!ofs)
    {
        spdlog::error("SaveManager: cannot open '{}' for writing", filepath);
        return false;
    }
    ofs << root.dump(2);
    ofs.close();

    spdlog::info("SaveManager: saved to '{}'", filepath);
    return true;
}

bool SaveManager::load(const std::string& filepath,
                       ecs::World& world,
                       json& outStoryFlags) const
{
    std::ifstream ifs(filepath);
    if (!ifs)
    {
        spdlog::error("SaveManager: cannot open '{}' for reading", filepath);
        return false;
    }

    json root;
    try { root = json::parse(ifs); }
    catch (const json::parse_error& e)
    {
        spdlog::error("SaveManager: JSON parse error: {}", e.what());
        return false;
    }
    ifs.close();

    if (!root.contains("player"))
    {
        spdlog::error("SaveManager: missing 'player' section in save file");
        return false;
    }

    auto& pj = root["player"];

    // 找玩家
    ecs::Entity player = ecs::kNullEntity;
    for (auto e : world.view<ecs::Player>())
    {
        player = e;
        break;
    }
    if (player == ecs::kNullEntity)
    {
        spdlog::error("SaveManager: no player entity found in world");
        return false;
    }

    // --- 清理旧物品 ---
    if (world.hasComponent<ecs::Inventory>(player))
    {
        auto& inv = world.getComponent<ecs::Inventory>(player);
        for (auto itemE : inv.items)
            world.destroyEntity(itemE);
        inv.items.clear();
    }

    // --- 恢复 Stats ---
    if (pj.contains("stats") && world.hasComponent<ecs::Stats>(player))
        deserializeStats(world.getComponent<ecs::Stats>(player), pj["stats"]);

    // --- 恢复 Transform ---
    if (pj.contains("transform") && world.hasComponent<ecs::Transform>(player))
    {
        auto& t = world.getComponent<ecs::Transform>(player);
        auto& tj = pj["transform"];
        t.position.x = tj.value("x", t.position.x);
        t.position.y = tj.value("y", t.position.y);
        t.position.z = tj.value("z", t.position.z);
    }

    // --- 恢复 Inventory + Items ---
    if (pj.contains("inventory"))
    {
        auto& invj = pj["inventory"];

        // 确保有 Inventory 组件
        if (!world.hasComponent<ecs::Inventory>(player))
            world.addComponent<ecs::Inventory>(player);

        auto& inv = world.getComponent<ecs::Inventory>(player);
        inv.money = invj.value("money", 0);

        if (invj.contains("items") && invj["items"].is_array())
        {
            for (auto& ij : invj["items"])
            {
                ecs::Entity itemE = deserializeItem(world, ij);
                if (itemE != ecs::kNullEntity)
                {
                    inv.items.push_back(itemE);
                    // 设置 Equipped 的 equippedBy
                    if (world.hasComponent<ecs::Equipped>(itemE))
                        world.getComponent<ecs::Equipped>(itemE).equippedBy = player;
                }
            }
        }
    }

    // --- 恢复 StoryFlags ---
    outStoryFlags.clear();
    if (root.contains("storyFlags"))
        outStoryFlags = root["storyFlags"];

    spdlog::info("SaveManager: loaded from '{}'", filepath);
    return true;
}

} // namespace engine
