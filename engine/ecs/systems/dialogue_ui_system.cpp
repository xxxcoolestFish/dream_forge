/**
 * @file engine/ecs/systems/dialogue_ui_system.cpp
 * @brief 对话 UI 系统实现
 */

#include "engine/ecs/systems/dialogue_ui_system.h"
#include "engine/ui/ui_renderer.h"
#include "engine/ui/widget.h"
#include "engine/ui/widgets/panel.h"
#include "engine/ui/widgets/button.h"
#include "engine/ui/widgets/text.h"
#include "engine/ui/widgets/flex_layout.h"
#include "engine/input/input_system.h"

#include <spdlog/spdlog.h>
#include <GLFW/glfw3.h>
#include <nlohmann/json.hpp>

namespace engine::ecs {

static constexpr EventTypeId kDialogueTextEvent      = ENGINE_EVENT("DialogueText");
static constexpr EventTypeId kDialogueChoiceEvent     = ENGINE_EVENT("DialogueChoice");
static constexpr EventTypeId kDialogueOptionSelected  = ENGINE_EVENT("DialogueOptionSelected");
static constexpr EventTypeId kDialogueEndEvent        = ENGINE_EVENT("DialogueEnd");

static constexpr float DLG_WIDTH  = 1200.0f;
static constexpr float DLG_HEIGHT = 130.0f;
static constexpr float SCREEN_W   = 1280.0f;
static constexpr float SCREEN_H   = 720.0f;

// =========================================================================
DialogueUISystem::DialogueUISystem(EventBus& eventBus, ui::UIRenderer& uiRenderer)
    : m_eventBus(eventBus)
    , m_uiRenderer(uiRenderer)
{
    m_textSubId   = m_eventBus.subscribe(kDialogueTextEvent,
        [this](const Event& e) { onDialogueText(e); });
    m_choiceSubId = m_eventBus.subscribe(kDialogueChoiceEvent,
        [this](const Event& e) { onDialogueChoice(e); });
    m_endSubId    = m_eventBus.subscribe(kDialogueEndEvent,
        [this](const Event&) { onDialogueEnd(Event{}); });

    spdlog::info("DialogueUISystem: subscribed to dialogue events");
}

DialogueUISystem::~DialogueUISystem()
{
    m_eventBus.unsubscribe(kDialogueTextEvent, m_textSubId);
    m_eventBus.unsubscribe(kDialogueChoiceEvent, m_choiceSubId);
    m_eventBus.unsubscribe(kDialogueEndEvent, m_endSubId);
}

// =========================================================================
// 首次构建对话 UI，挂载到 UIRenderer 根控件
// =========================================================================
void DialogueUISystem::buildUI(uint32_t, uint32_t)
{
    auto* root = m_uiRenderer.root();
    if (!root) return;

    // 检查是否已构建
    for (auto& c : root->children())
        if (c->id() == "dialogue_root") return;

    // 根容器
    nlohmann::json j;
    j["type"] = "Panel";
    j["id"]   = "dialogue_root";
    j["rect"] = {40.0f, SCREEN_H - DLG_HEIGHT - 16.0f - 150.0f, DLG_WIDTH, 220.0f};
    j["color"] = nlohmann::json::array({0, 0, 0, 0});
    j["children"] = nlohmann::json::array();

    // 对话背景
    nlohmann::json bg;
    bg["type"] = "Panel";
    bg["id"]   = "dlg_bg";
    bg["rect"] = {0, 0, DLG_WIDTH, DLG_HEIGHT};
    bg["color"] = nlohmann::json::array({10, 15, 25, 220});
    j["children"].push_back(bg);

    // 对话文本
    nlohmann::json txt;
    txt["type"] = "Text";
    txt["id"]   = "dlg_text";
    txt["text"] = " ";
    txt["rect"] = {16, 12, DLG_WIDTH - 32, DLG_HEIGHT - 24};
    txt["color"] = nlohmann::json::array({230, 230, 245});
    j["children"].push_back(txt);

    // 选项容器
    nlohmann::json opts;
    opts["type"] = "FlexLayout";
    opts["id"]   = "dlg_options";
    opts["rect"] = {0, DLG_HEIGHT + 8, DLG_WIDTH, 100};
    opts["direction"] = "vertical";
    opts["gap"]  = 4;
    opts["children"] = nlohmann::json::array();
    j["children"].push_back(opts);

    auto dlg = ui::Widget::fromJson(j);
    dlg->setVisible(false);

    // 保存关键子控件裸指针
    for (auto& child : dlg->children())
    {
        if (child->id() == "dlg_text")
            m_textWidget = child.get();
        else if (child->id() == "dlg_options")
            m_optionsContainer = child.get();
    }

    root->addChild(std::move(dlg));

    spdlog::debug("DialogueUISystem: UI built, text={} options={}",
                  (void*)m_textWidget, (void*)m_optionsContainer);
}

// =========================================================================
// 查找对话框根控件
// =========================================================================
static ui::Widget* findDialogRoot(ui::UIRenderer& ui)
{
    auto* root = ui.root();
    if (!root) return nullptr;
    for (auto& c : root->children())
        if (c->id() == "dialogue_root") return c.get();
    return nullptr;
}

// =========================================================================
// 键盘快捷键 1-4
// =========================================================================
void DialogueUISystem::update(input::InputSystem* input)
{
    if (!m_visible || !input || m_optionCount == 0) return;

    static const int keys[] = { GLFW_KEY_1, GLFW_KEY_2, GLFW_KEY_3, GLFW_KEY_4 };
    for (int i = 0; i < m_optionCount && i < 4; i++)
    {
        if (input->isKeyPressed(keys[i]))
        {
            spdlog::info("DialogueUISystem: key option {}", i + 1);
            DialogueOptionSelectedData d;
            d.optionIndex = i;
            m_eventBus.publishImmediate(kDialogueOptionSelected, std::any(d));
            return;
        }
    }
}

// =========================================================================
void DialogueUISystem::onDialogueText(const Event& event)
{
    const auto* data = std::any_cast<DialogueTextData>(&event.data);
    if (!data) return;

    buildUI(0, 0);

    auto* dlg = findDialogRoot(m_uiRenderer);
    if (!dlg) return;

    // 更新文本
    std::string fullText = data->speaker + "：" + data->text;
    for (auto& child : dlg->children())
    {
        if (auto* t = dynamic_cast<ui::Text*>(child.get()))
        {
            if (child->id() == "dlg_text")
            {
                t->setText(fullText);
                break;
            }
        }
    }

    // 清除旧选项
    m_optionButtons.clear();
    m_optionCount = 0;
    for (auto& child : dlg->children())
    {
        if (child->id() == "dlg_options")
        {
            child->children().clear();
            child->setVisible(false);
            break;
        }
    }

    dlg->setVisible(true);
    m_visible = true;
}

// =========================================================================
void DialogueUISystem::onDialogueChoice(const Event& event)
{
    const auto* data = std::any_cast<DialogueChoiceData>(&event.data);
    if (!data || !m_visible) return;

    auto* dlg = findDialogRoot(m_uiRenderer);
    if (!dlg) return;

    ui::Widget* optsContainer = nullptr;
    for (auto& child : dlg->children())
    {
        if (child->id() == "dlg_options")
        {
            optsContainer = child.get();
            break;
        }
    }
    if (!optsContainer) return;

    // 清除旧按钮
    optsContainer->children().clear();
    m_optionButtons.clear();
    m_optionCount = 0;

    for (size_t i = 0; i < data->optionTexts.size(); i++)
    {
        if (data->optionTexts[i].empty()) continue;

        nlohmann::json bj;
        bj["type"] = "Button";
        bj["id"]   = "dlg_opt_" + std::to_string(i);
        bj["text"] = std::to_string(i + 1) + ". " + data->optionTexts[i];
        bj["rect"] = {0, 0, DLG_WIDTH, 32};
        bj["color"] = nlohmann::json::array({40, 55, 80, 230});

        auto btnWidget = ui::Widget::fromJson(bj);
        auto* btn = dynamic_cast<ui::Button*>(btnWidget.get());
        if (btn)
        {
            int idx = static_cast<int>(i);
            btn->setOnClick([this, idx]() {
                spdlog::info("DialogueUISystem: mouse clicked option {}", idx + 1);
                DialogueOptionSelectedData d;
                d.optionIndex = idx;
                m_eventBus.publishImmediate(kDialogueOptionSelected, std::any(d));
            });
        }
        m_optionButtons.push_back(btnWidget.get());
        m_optionCount++;
        optsContainer->addChild(std::move(btnWidget));
    }

    optsContainer->setVisible(true);
    spdlog::info("DialogueUISystem: {} option(s) shown", m_optionCount);
}

// =========================================================================
void DialogueUISystem::onDialogueEnd(const Event&)
{
    m_visible = false;
    m_optionCount = 0;
    m_optionButtons.clear();

    auto* dlg = findDialogRoot(m_uiRenderer);
    if (!dlg) return;

    dlg->setVisible(false);
    for (auto& child : dlg->children())
    {
        if (child->id() == "dlg_options")
        {
            child->children().clear();
            child->setVisible(false);
            break;
        }
    }

    spdlog::info("DialogueUISystem: dialogue ended");
}

} // namespace engine::ecs
