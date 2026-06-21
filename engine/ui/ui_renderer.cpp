/**
 * @file engine/ui/ui_renderer.cpp
 * @brief UIRenderer 实现
 */

#include "engine/ui/ui_renderer.h"
#include "engine/ui/widget.h"
#include "engine/ui/widgets/panel.h"
#include "engine/ui/widgets/button.h"
#include "engine/ui/widgets/box_layout.h"
#include "engine/ui/widgets/flex_layout.h"
#include "engine/ui/widgets/text.h"
#include "engine/render/sprite.h"
#include <typeinfo>

#include <spdlog/spdlog.h>
#include <fstream>
#include <sstream>

namespace engine::ui {

// 注册所有内建控件类型
static bool registerWidgetTypes()
{
    static bool done = false;
    if (!done)
    {
        Widget::registerType("Panel",      Panel::fromJson);
        Widget::registerType("Button",     Button::fromJson);
        Widget::registerType("Text",       Text::fromJson);
        Widget::registerType("BoxLayout",  BoxLayout::fromJson);
        Widget::registerType("FlexLayout", FlexLayout::fromJson);
        done = true;
    }
    return done;
}

UIRenderer::UIRenderer()
{
    registerWidgetTypes();
    spdlog::debug("UIRenderer created");
}

UIRenderer::~UIRenderer()
{
    spdlog::debug("UIRenderer destroyed");
}

bool UIRenderer::loadFromString(const std::string& jsonStr)
{
    try
    {
        nlohmann::json j = nlohmann::json::parse(jsonStr);
        m_root = Widget::fromJson(j);
        spdlog::info("UIRenderer: loaded UI (root='{}', type='{}', rect=[{},{},{},{}])",
            m_root->id(),
            j.value("type", "?"),
            m_root->rect().x, m_root->rect().y,
            m_root->rect().w, m_root->rect().h);
        return true;
    }
    catch (const std::exception& e)
    {
        spdlog::error("UIRenderer: load failed: {}", e.what());
        return false;
    }
}

bool UIRenderer::loadFromFile(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
    {
        spdlog::error("UIRenderer: cannot open '{}'", path);
        return false;
    }
    std::stringstream buf;
    buf << file.rdbuf();
    return loadFromString(buf.str());
}

bool UIRenderer::loadFont(const std::string& ttfPath, float pixelHeight)
{
    m_font = std::make_shared<Font>();
    if (m_font->load(ttfPath, pixelHeight))
    {
        Text::setDefaultFont(m_font);
        return true;
    }
    m_font.reset();
    return false;
}

void UIRenderer::setEcsWorld(engine::ecs::World* world)
{
    m_bindingContext.setEcsWorld(world);
}

void UIRenderer::update(float dt)
{
    if (!m_root) return;

    try
    {
        m_root->update(dt);
        if (m_bindingContext.hasEcs())
            resolveBindings(m_root.get());
    }
    catch (const std::exception& e)
    {
        spdlog::error("UIRenderer::update error: {}", e.what());
    }
    catch (...)
    {
        spdlog::error("UIRenderer::update unknown error");
    }
}

void UIRenderer::resolveBindings(Widget* w)
{
    if (auto* text = dynamic_cast<Text*>(w))
        text->resolveBinding(m_bindingContext);
    for (auto& child : w->children())
        resolveBindings(child.get());
}

void UIRenderer::render(render::SpriteRenderer& spriteRenderer,
                         uint32_t screenWidth, uint32_t screenHeight)
{
    if (!m_root) { spdlog::debug("UIRenderer::render: no root widget"); return; }

    static bool firstRender = true;
    if (firstRender)
    {
        spdlog::info("UIRenderer: first render (root='{}', screen={}x{})",
            m_root->id(), screenWidth, screenHeight);
        firstRender = false;
    }

    try
    {
        renderBackgroundWidgets(m_root.get(), spriteRenderer);
        spriteRenderer.flush(screenWidth, screenHeight);
        renderTextWidgets(m_root.get(), spriteRenderer);
    }
    catch (const std::exception& e)
    {
        spdlog::error("UIRenderer::render error: {}", e.what());
    }
    catch (...)
    {
        spdlog::error("UIRenderer::render unknown error");
    }
}

void UIRenderer::renderBackgroundWidgets(Widget* w, render::SpriteRenderer& sr)
{
    if (dynamic_cast<Text*>(w)) return;

    // 对于叶子控件（Panel/Button），调用 render 提交精灵
    // 对于容器控件（FlexLayout/BoxLayout），只递归不调用 render
    bool isContainer = (dynamic_cast<FlexLayout*>(w) || dynamic_cast<BoxLayout*>(w));
    if (!isContainer)
        w->render(sr);

    for (auto& child : w->children())
        renderBackgroundWidgets(child.get(), sr);
}

void UIRenderer::renderTextWidgets(Widget* w, render::SpriteRenderer& sr)
{
    if (auto* text = dynamic_cast<Text*>(w))
        text->render(sr); // Text 直接 OpenGL 绘制
    for (auto& child : w->children())
        renderTextWidgets(child.get(), sr);
}

void UIRenderer::onMouseDown(float x, float y)
{
    if (!m_root) return;
    Widget* hit = m_root->hitTest(x, y);
    if (hit)
    {
        spdlog::info("UI click: ({:.0f},{:.0f}) -> widget '{}' (type={})",
            x, y, hit->id(),
            typeid(*hit).name());
        hit->onMouseDown(x, y);
    }
    else
    {
        spdlog::debug("UI click: ({:.0f},{:.0f}) -> no hit", x, y);
    }
}

void UIRenderer::onMouseUp(float x, float y)
{
    if (!m_root) return;
    Widget* hit = m_root->hitTest(x, y);
    if (hit) hit->onMouseUp(x, y);
}

} // namespace engine::ui
