/**
 * @file engine/scene/layer.cpp
 * @brief Layer 实现
 */

#include "engine/scene/layer.h"

namespace engine::scene {

Layer Layer::rect(const std::string& id, const std::string& texPath,
                   float x, float y, float w, float h, float depth)
{
    Layer layer;
    layer.id          = id;
    layer.name        = id;
    layer.texturePath = texPath;
    layer.depth       = depth;
    layer.vertices    = {
        LayerVertex::simple(x,     y,     0.0f, 0.0f), // 左下
        LayerVertex::simple(x + w, y,     1.0f, 0.0f), // 右下
        LayerVertex::simple(x + w, y + h, 1.0f, 1.0f), // 右上
        LayerVertex::simple(x,     y + h, 0.0f, 1.0f), // 左上
    };
    return layer;
}

nlohmann::json Layer::toJson() const
{
    nlohmann::json j;
    j["id"]          = id;
    j["name"]        = name;
    j["texture"]     = texturePath;
    j["depth"]       = depth;
    j["offset"]      = { offset.x, offset.y };
    j["rotation"]    = rotation;
    j["scale"]       = scale;
    j["visible"]     = visible;

    nlohmann::json verts = nlohmann::json::array();
    for (const auto& v : vertices)
    {
        verts.push_back({
            { "px", v.position.x }, { "py", v.position.y },
            { "u",  v.texCoord.x }, { "v",  v.texCoord.y }
        });
    }
    j["vertices"] = verts;

    return j;
}

Layer Layer::fromJson(const nlohmann::json& j)
{
    Layer layer;
    layer.id          = j.value("id", "");
    layer.name        = j.value("name", layer.id);
    layer.texturePath = j.value("texture", "");
    layer.depth       = j.value("depth", 0.5f);

    if (j.contains("offset") && j["offset"].is_array())
        layer.offset = glm::vec2(j["offset"][0].get<float>(), j["offset"][1].get<float>());

    layer.rotation = j.value("rotation", 0.0f);
    layer.scale    = j.value("scale", 1.0f);
    layer.visible  = j.value("visible", true);

    if (j.contains("vertices") && j["vertices"].is_array())
    {
        for (const auto& v : j["vertices"])
        {
            LayerVertex lv;
            lv.position = glm::vec2(v.value("px", 0.0f), v.value("py", 0.0f));
            lv.texCoord = glm::vec2(v.value("u", 0.0f),  v.value("v", 0.0f));
            layer.vertices.push_back(lv);
        }
    }

    return layer;
}

} // namespace engine::scene
