# =============================================================================
# Dear ImGui 构建配置 — Phase 6.8
# =============================================================================
# ImGui 即时模式 GUI 库，用于引擎内编辑器（实体列表 + 属性检视器）。
# =============================================================================

if(NOT TARGET imgui)
    FetchContent_Declare(imgui_src
        GIT_REPOSITORY https://gitcode.com/ocornut/imgui.git
        GIT_TAG        v1.91.0
        GIT_SHALLOW    ON
    )
    FetchContent_MakeAvailable(imgui_src)

    set(IMGUI_SOURCES
        ${imgui_src_SOURCE_DIR}/imgui.cpp
        ${imgui_src_SOURCE_DIR}/imgui_draw.cpp
        ${imgui_src_SOURCE_DIR}/imgui_tables.cpp
        ${imgui_src_SOURCE_DIR}/imgui_widgets.cpp
        ${imgui_src_SOURCE_DIR}/backends/imgui_impl_glfw.cpp
        ${imgui_src_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp
    )

    add_library(imgui STATIC ${IMGUI_SOURCES})
    target_include_directories(imgui PUBLIC
        ${imgui_src_SOURCE_DIR}
        ${imgui_src_SOURCE_DIR}/backends
    )
    target_link_libraries(imgui PUBLIC glfw)

    if(MSVC)
        target_compile_options(imgui PRIVATE /W3)
    endif()
endif()
