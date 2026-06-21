# =============================================================================
# 引擎静态库
# =============================================================================
# 本文件负责：
#   1. 编译 engine.lib 静态库
#   2. 链接所有第三方依赖
#   3. 子模块源文件管理
#
# 目录对应：
#   engine/core/     → 引擎初始化、游戏循环、事件总线
#   engine/ecs/      → EnTT封装
#   engine/render/   → bgfx渲染后端
#   engine/input/    → 输入系统
#   engine/resource/ → 资源管理
#   engine/script/   → Lua绑定（Phase 5 集成）
# =============================================================================

# -----------------------------------------------------------------------------
# 引擎源文件 — 按模块分组，按 Phase 逐步添加
# -----------------------------------------------------------------------------

# --- Phase 1: core ---
set(ENGINE_CORE_SOURCES
    core/engine.cpp
    core/game_loop.cpp
    core/event_bus.cpp
)

# --- Phase 1: ecs ---
set(ENGINE_ECS_SOURCES
    ecs/world.cpp
    ecs/system.cpp
    ecs/systems/movement_system.cpp
    ecs/systems/interaction_system.cpp
    ecs/systems/dialogue_system.cpp
)

# --- Phase 1: render ---
set(ENGINE_RENDER_SOURCES
    render/gl_loader.cpp
    render/render_backend.cpp
    render/shader.cpp
    render/sprite.cpp
    render/texture.cpp
    render/scene/camera.cpp
    render/scene/scene_renderer.cpp
)

# --- Phase 4: ui ---
set(ENGINE_UI_SOURCES
    ui/widget.cpp
    ui/font.cpp
    ui/ui_renderer.cpp
    ui/widgets/panel.cpp
    ui/widgets/button.cpp
    ui/widgets/text.cpp
    ui/widgets/box_layout.cpp
    ui/widgets/flex_layout.cpp
)

# --- Phase 3: scene ---
set(ENGINE_SCENE_SOURCES
    scene/layer.cpp
    scene/scene.cpp
    scene/scene_loader.cpp
)

# --- Phase 1: input ---
set(ENGINE_INPUT_SOURCES
    input/input_system.cpp
)

# --- Phase 1: resource ---
set(ENGINE_RESOURCE_SOURCES
    resource/resource_manager.cpp
)

# --- Phase 2: ai ---
set(ENGINE_AI_SOURCES
    ai/ai_client.cpp
)

# -----------------------------------------------------------------------------
# 库声明
# -----------------------------------------------------------------------------
add_library(engine STATIC
    ${ENGINE_CORE_SOURCES}
    ${ENGINE_ECS_SOURCES}
    ${ENGINE_RENDER_SOURCES}
    ${ENGINE_UI_SOURCES}
    ${ENGINE_SCENE_SOURCES}
    ${ENGINE_INPUT_SOURCES}
    ${ENGINE_RESOURCE_SOURCES}
    ${ENGINE_AI_SOURCES}
)

# 输出目录统一
set_target_properties(engine PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
)

# -----------------------------------------------------------------------------
# 链接依赖
# -----------------------------------------------------------------------------
target_link_libraries(engine PUBLIC
    glfw
    EnTT::EnTT
    glm::glm
    spdlog::spdlog
    nlohmann_json::nlohmann_json
    OpenGL::GL
    libzmq
    cppzmq
)

# 引擎需要 GLFW 头文件公开暴露（输入系统中引用 GLFW 类型）
# 同时明确指定所有 FetchContent 依赖的 include 路径
target_include_directories(engine PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${entt_SOURCE_DIR}/src
    ${glm_SOURCE_DIR}
    ${spdlog_SOURCE_DIR}/include
    ${nlohmann_json_SOURCE_DIR}/include
    ${stb_SOURCE_DIR}
    ${libzmq_SOURCE_DIR}/include
    ${cppzmq_SOURCE_DIR}
)

# -----------------------------------------------------------------------------
# 引擎编译选项
# -----------------------------------------------------------------------------
target_compile_definitions(engine PRIVATE
    ENGINE_BUILD
)

if(MSVC)
    # 引擎代码严格警告
    target_compile_options(engine PRIVATE /W4)
endif()
