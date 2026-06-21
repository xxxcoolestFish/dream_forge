/**
 * @file engine/scene/layer.h
 * @brief 场景层 — 2.5D 视差渲染的基本单元
 *
 * 一个 Layer 代表 AI 生成图像中分割出的一个物体/区域：
 *   - 纹理（PNG 图片路径）
 *   - 深度值（0.0 = 远景天空, 1.0 = 前景）
 *   - 四边形顶点（像素坐标，相对于场景原点）
 *
 * 设计：
 *   - 纯数据结构，不依赖渲染 API
 *   - 可 JSON 序列化/反序列化
 *
 * 注意事项：
 *   - 深度值决定视差偏移量（近景偏移大，远景偏移小）
 *   - 顶点顺序：左下、右下、右上、左上（逆时针）
 */

#pragma once

#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <nlohmann/json.hpp>

namespace engine::scene {

struct LayerVertex
{
    glm::vec2 position;  // 像素坐标
    glm::vec2 texCoord;  // 纹理坐标 [0,1]

    static LayerVertex simple(float x, float y, float u, float v)
    {
        return { glm::vec2(x, y), glm::vec2(u, v) };
    }
};

struct Layer
{
    // 标识
    std::string id;          // 唯一标识（如 "tree_01"）
    std::string name;        // 显示名称

    // 纹理
    std::string texturePath; // 相对于 assets/textures/ 的路径

    // 深度（0.0 = 最远, 1.0 = 最近）
    float depth = 0.5f;

    // 四边形顶点（4个，左下、右下、右上、左上）
    // 如果为空，则使用 position + textureSize 自动生成
    std::vector<LayerVertex> vertices;

    // 简便构造：矩形层（自动生成顶点）
    static Layer rect(const std::string& id, const std::string& texPath,
                      float x, float y, float w, float h, float depth);

    // 变换偏移（场景组合时手动调整）
    glm::vec2 offset { 0.0f, 0.0f };
    float     rotation = 0.0f;   // 弧度
    float     scale    = 1.0f;

    // 是否可见
    bool visible = true;

    // JSON 序列化
    nlohmann::json toJson() const;
    static Layer fromJson(const nlohmann::json& j);
};

} // namespace engine::scene
