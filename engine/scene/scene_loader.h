/**
 * @file engine/scene/scene_loader.h
 * @brief 场景加载器 — 从 .scene JSON 文件加载 Scene 对象
 *
 * 负责：
 *   - 文件读取 → JSON 解析 → Scene 构造
 *   - 路径解析（相对于 assets/scenes/ 目录）
 *   - 加载失败时返回错误信息而非抛异常
 *
 * 使用方式：
 *   auto result = SceneLoader::load("tavern/tavern.scene");
 *   if (result) { Scene& scene = *result; ... }
 */

#pragma once

#include "engine/scene/scene.h"

#include <string>
#include <optional>
#include <memory>

namespace engine::scene {

class SceneLoader
{
public:
    // 从 JSON 文件加载场景
    // 返回 nullopt 表示加载失败，错误信息通过 error() 获取
    static std::optional<Scene> loadFromFile(const std::string& path);

    // 从 JSON 字符串加载场景（测试用）
    static std::optional<Scene> loadFromString(const std::string& jsonStr);

    // 保存场景到文件
    static bool saveToFile(const Scene& scene, const std::string& path);

    // 最后一次错误信息
    static const std::string& lastError();

private:
    static std::string s_lastError;
};

} // namespace engine::scene
