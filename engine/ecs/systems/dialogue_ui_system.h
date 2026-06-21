/**
 * @file engine/ecs/systems/dialogue_ui_system.h
 * @brief 对话 UI 系统 — 将对话文本/选项渲染到屏幕上
 *
 * Phase 5.6：程序化构建对话 UI 控件树，监听 DialogueText/Choice/End 事件。
 *
 * 设计：
 *   - 由 Engine::Impl 持有，非注册到 World 的 ECS System
 *   - 通过 EventBus 接收事件，通过 UIRenderer 渲染 UI
 *   - 控件树：Panel（背景）→  Text（对话内容）+ FlexLayout（选项按钮）
 */

#pragma once

#include "engine/core/event_bus.h"
#include "engine/ecs/event_types.h"

#include <memory>
#include <string>
#include <vector>

namespace engine::ui { class UIRenderer; class Widget; }
namespace engine::input { class InputSystem; }

namespace engine::ecs {

class DialogueUISystem
{
public:
    DialogueUISystem(EventBus& eventBus, ui::UIRenderer& uiRenderer);
    ~DialogueUISystem();

    // 禁止拷贝
    DialogueUISystem(const DialogueUISystem&) = delete;
    DialogueUISystem& operator=(const DialogueUISystem&) = delete;

    // 每帧更新（处理键盘输入 1-4 选择选项）
    void update(input::InputSystem* input);

private:
    EventBus&        m_eventBus;
    ui::UIRenderer&  m_uiRenderer;

    // 订阅 ID
    uint32_t m_textSubId   = 0;
    uint32_t m_choiceSubId = 0;
    uint32_t m_endSubId    = 0;

    // 对话框控件
    std::unique_ptr<ui::Widget> m_dialogueBox;  // 根 Panel
    ui::Widget* m_textWidget = nullptr;          // 对话文本
    ui::Widget* m_optionsContainer = nullptr;    // 选项容器
    std::vector<ui::Widget*> m_optionButtons;    // 选项按钮列表

    bool m_visible = false;
    int  m_optionCount = 0;

    // 构建控件树（懒加载，首次使用时调用）
    void buildUI(uint32_t screenW, uint32_t screenH);

    // 事件处理
    void onDialogueText(const Event& event);
    void onDialogueChoice(const Event& event);
    void onDialogueEnd(const Event& event);
};

} // namespace engine::ecs
