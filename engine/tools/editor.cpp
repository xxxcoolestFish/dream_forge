/**
 * @file engine/tools/editor.cpp
 * @brief ImGui 编辑器实现 — 实体列表 + 属性检视器
 */

#include "engine/tools/editor.h"
#include "engine/ecs/world.h"
#include "engine/ecs/component_types.h"
#include "engine/input/input_system.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>

#include <spdlog/spdlog.h>
#include <sstream>

namespace engine::tools {

Editor::Editor(ecs::World* world, input::InputSystem* input)
    : m_world(world)
    , m_input(input)
{
    spdlog::debug("Editor: created");
}

Editor::~Editor()
{
    if (m_initialized) shutdown();
}

bool Editor::init(GLFWwindow* window)
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr; // 不保存 imgui.ini

    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    m_initialized = true;
    spdlog::info("Editor: ImGui initialized (OpenGL3 + GLFW)");
    return true;
}

void Editor::shutdown()
{
    if (!m_initialized) return;
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    m_initialized = false;
    m_active = false;
    spdlog::info("Editor: shutdown");
}

void Editor::setActive(bool a)
{
    m_active = a;
    if (m_initialized)
        spdlog::info("Editor: {}", a ? "ON (F1)" : "OFF");
}

void Editor::update(float dt)
{
    (void)dt;
    // F1 切换编辑器
    if (m_input && m_input->isKeyPressed(GLFW_KEY_F1))
    {
        setActive(!m_active);
    }

    if (!m_active) return;

    // 开始 ImGui 帧
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Editor::render()
{
    if (!m_active || !m_initialized) return;

    drawEntityList();
    drawInspector();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

// =========================================================================
// 实体列表窗口
// =========================================================================
void Editor::drawEntityList()
{
    ImGui::SetNextWindowSize({300, 400}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Entity List");

    if (!m_world)
    {
        ImGui::TextUnformatted("(no world)");
        ImGui::End();
        return;
    }

    auto& registry = m_world->raw();

    // 列出所有有 Transform 的实体
    auto view = registry.view<ecs::Transform>();
    int count = 0;
    for (auto entity : view)
    {
        uint32_t eid = static_cast<uint32_t>(entity);

        const auto* stats = registry.try_get<ecs::Stats>(entity);
        const auto* player = registry.try_get<ecs::Player>(entity);

        std::string label;
        if (player)
            label = "Player";
        else if (stats)
            label = "Entity";
        else
            label = "Entity";

        std::stringstream ss;
        ss << label << " #" << eid;
        std::string name = ss.str();

        if (ImGui::Selectable(name.c_str(), m_selectedEntity == eid))
        {
            m_selectedEntity = eid;
        }
        count++;
    }

    if (count == 0)
        ImGui::TextUnformatted("(no entities)");

    ImGui::End();
}

// =========================================================================
// 属性检视器窗口
// =========================================================================
void Editor::drawInspector()
{
    ImGui::SetNextWindowSize({350, 400}, ImGuiCond_FirstUseEver);
    ImGui::Begin("Inspector");

    if (m_selectedEntity == 0 || !m_world)
    {
        ImGui::TextUnformatted("Select an entity to inspect.");
        ImGui::End();
        return;
    }

    auto& registry = m_world->raw();
    entt::entity ent = static_cast<entt::entity>(m_selectedEntity);

    auto* transform = registry.try_get<ecs::Transform>(ent);
    auto* sprite    = registry.try_get<ecs::Sprite>(ent);
    auto* stats     = registry.try_get<ecs::Stats>(ent);

    if (!transform)
    {
        ImGui::TextUnformatted("Entity has no Transform component.");
        ImGui::End();
        return;
    }

    if (ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        float pos[3] = { transform->position.x, transform->position.y, transform->position.z };
        if (ImGui::DragFloat3("Position", pos, 1.0f))
        {
            transform->position.x = pos[0];
            transform->position.y = pos[1];
            transform->position.z = pos[2];
        }

        float rot[3] = { transform->rotation.x, transform->rotation.y, transform->rotation.z };
        if (ImGui::DragFloat3("Rotation", rot, 1.0f))
        {
            transform->rotation.x = rot[0];
            transform->rotation.y = rot[1];
            transform->rotation.z = rot[2];
        }

        float scale[3] = { transform->scale.x, transform->scale.y, transform->scale.z };
        if (ImGui::DragFloat3("Scale", scale, 0.1f, 0.1f, 10.0f))
        {
            transform->scale.x = scale[0];
            transform->scale.y = scale[1];
            transform->scale.z = scale[2];
        }

        float depth = transform->depthLayer;
        if (ImGui::DragFloat("Depth Layer", &depth, 0.01f, 0.0f, 1.0f))
            transform->depthLayer = depth;
    }

    if (sprite && ImGui::CollapsingHeader("Sprite"))
    {
        float tint[4] = { sprite->tint.r, sprite->tint.g, sprite->tint.b, sprite->tint.a };
        if (ImGui::ColorEdit4("Tint", tint))
        {
            sprite->tint.r = tint[0];
            sprite->tint.g = tint[1];
            sprite->tint.b = tint[2];
            sprite->tint.a = tint[3];
        }
        ImGui::Text("Texture ID: %u", sprite->textureId);
    }

    if (stats && ImGui::CollapsingHeader("Stats"))
    {
        // 编辑已有属性值
        std::vector<std::string> keysToRemove;
        std::vector<std::pair<std::string,std::pair<float,float>>> newValues;

        for (auto& [key, value] : stats->values)
        {
            float val = value;
            char buf[64];
            snprintf(buf, sizeof(buf), "##%s_val", key.c_str());
            if (ImGui::DragFloat(key.c_str(), &val, 0.5f))
                newValues.push_back({key, {val, val}});
            // check max
            auto maxIt = stats->maxValues.find(key);
            if (maxIt != stats->maxValues.end())
            {
                float mx = maxIt->second;
                snprintf(buf, sizeof(buf), "##%s_max", key.c_str());
                if (ImGui::DragFloat((key + " max").c_str(), &mx, 0.5f))
                    stats->maxValues[key] = mx;
            }
        }

        // 应用修改
        for (auto& [k, v] : newValues)
            stats->values[k] = v.first;

        // 添加新属性
        static char newKey[64] = "";
        static float newVal = 0.0f;
        ImGui::InputText("New stat", newKey, sizeof(newKey));
        ImGui::DragFloat("Value", &newVal, 0.5f);
        if (ImGui::Button("Add Stat") && newKey[0] != '\0')
        {
            stats->values[std::string(newKey)] = newVal;
            newKey[0] = '\0';
            newVal = 0.0f;
        }
    }

    ImGui::End();
}

} // namespace engine::tools
