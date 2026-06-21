/**
 * @file engine/scene/scene.cpp
 * @brief Scene 实现
 */

#include "engine/scene/scene.h"

#include <algorithm>

namespace engine::scene {

nlohmann::json Scene::toJson() const
{
    nlohmann::json j;
    j["name"]        = name;
    j["description"] = description;
    j["version"]     = version;
    j["background"]  = {
        { "texture", backgroundTexture },
        { "size",    { backgroundSize.x, backgroundSize.y } }
    };

    nlohmann::json layersArr = nlohmann::json::array();
    for (const auto& layer : layers)
        layersArr.push_back(layer.toJson());
    j["layers"] = layersArr;

    nlohmann::json entsArr = nlohmann::json::array();
    for (const auto& ent : entities)
    {
        nlohmann::json ej;
        ej["type"]       = ent.type;
        ej["templateId"] = ent.templateId;
        ej["position"]   = { ent.position.x, ent.position.y, ent.position.z };
        ej["rotation"]   = ent.rotation;
        if (!ent.extra.empty()) ej["extra"] = ent.extra;
        entsArr.push_back(ej);
    }
    j["entities"] = entsArr;

    if (!ambianceAudio.empty())
        j["ambiance"] = ambianceAudio;

    return j;
}

Scene Scene::fromJson(const nlohmann::json& j)
{
    Scene scene;
    scene.name        = j.value("name", "Untitled");
    scene.description = j.value("description", "");
    scene.version     = j.value("version", "0.1.0");

    if (j.contains("background"))
    {
        scene.backgroundTexture = j["background"].value("texture", "");
        if (j["background"].contains("size"))
            scene.backgroundSize = glm::vec2(
                j["background"]["size"][0].get<float>(),
                j["background"]["size"][1].get<float>());
    }

    if (j.contains("layers") && j["layers"].is_array())
    {
        for (const auto& lj : j["layers"])
            scene.layers.push_back(Layer::fromJson(lj));
    }

    if (j.contains("entities") && j["entities"].is_array())
    {
        for (const auto& ej : j["entities"])
        {
            EntityPlacement ent;
            ent.type       = ej.value("type", "");
            ent.templateId = ej.value("templateId", "");
            if (ej.contains("position"))
            {
                float z = (ej["position"].size() > 2)
                    ? ej["position"][2].get<float>() : 0.0f;
                ent.position = glm::vec3(
                    ej["position"][0].get<float>(),
                    ej["position"][1].get<float>(),
                    z);
            }
            ent.rotation = ej.value("rotation", 0.0f);
            if (ej.contains("extra")) ent.extra = ej["extra"];
            scene.entities.push_back(ent);
        }
    }

    scene.ambianceAudio = j.value("ambiance", "");

    return scene;
}

void Scene::sortLayersByDepth()
{
    std::sort(layers.begin(), layers.end(),
        [](const Layer& a, const Layer& b) {
            return a.depth < b.depth; // 远的在前（depth 小 = 远）
        });
}

} // namespace engine::scene
