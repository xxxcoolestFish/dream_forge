/**
 * @file engine/scene/scene_loader.cpp
 * @brief SceneLoader 实现
 */

#include "engine/scene/scene_loader.h"

#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>
#include <nlohmann/json.hpp>

namespace engine::scene {

std::string SceneLoader::s_lastError;

std::optional<Scene> SceneLoader::loadFromFile(const std::string& path)
{
    spdlog::info("SceneLoader: loading '{}'", path);

    std::ifstream file(path);
    if (!file.is_open())
    {
        s_lastError = "Cannot open file: " + path;
        spdlog::error("SceneLoader: {}", s_lastError);
        return std::nullopt;
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return loadFromString(buffer.str());
}

std::optional<Scene> SceneLoader::loadFromString(const std::string& jsonStr)
{
    try
    {
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        Scene scene = Scene::fromJson(j);
        scene.sortLayersByDepth();

        spdlog::info("SceneLoader: loaded '{}' ({} layers, {} entities)",
            scene.name, scene.layers.size(), scene.entities.size());

        return scene;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        s_lastError = std::string("JSON parse error: ") + e.what();
        spdlog::error("SceneLoader: {}", s_lastError);
        return std::nullopt;
    }
    catch (const std::exception& e)
    {
        s_lastError = std::string("Error: ") + e.what();
        spdlog::error("SceneLoader: {}", s_lastError);
        return std::nullopt;
    }
}

bool SceneLoader::saveToFile(const Scene& scene, const std::string& path)
{
    try
    {
        nlohmann::json j = scene.toJson();
        std::ofstream file(path);
        if (!file.is_open())
        {
            s_lastError = "Cannot write file: " + path;
            return false;
        }
        file << j.dump(2); // 缩进 2 空格
        spdlog::info("SceneLoader: saved '{}' to '{}'", scene.name, path);
        return true;
    }
    catch (const std::exception& e)
    {
        s_lastError = std::string("Save error: ") + e.what();
        spdlog::error("SceneLoader: {}", s_lastError);
        return false;
    }
}

const std::string& SceneLoader::lastError()
{
    return s_lastError;
}

} // namespace engine::scene
