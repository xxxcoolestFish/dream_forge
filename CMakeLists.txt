# =============================================================================
# AI驱动互动叙事游戏框架 — 根构建文件
# =============================================================================
# 本文件负责：
#   1. 项目声明与全局编译选项
#   2. 第三方依赖的获取（FetchContent）
#   3. 子目录（engine, sample）的递归构建
#
# 注意事项：
#   - 当前仅支持 Windows + MSVC 2022+
#   - 引擎编译为静态库（engine.lib）
#   - 所有依赖通过 FetchContent 获取，无需手动安装
# =============================================================================

cmake_minimum_required(VERSION 3.27)

# -----------------------------------------------------------------------------
# 项目声明
# -----------------------------------------------------------------------------
project(AI_Game_Frame
    VERSION 0.1.0
    DESCRIPTION "AI驱动的2.5D互动叙事游戏框架"
    LANGUAGES CXX
)

# -----------------------------------------------------------------------------
# C++20 标准 — 全局要求
# -----------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# -----------------------------------------------------------------------------
# MSVC 编译选项（Windows）
# -----------------------------------------------------------------------------
if(MSVC)
    add_compile_options(
        /W4              # 高警告级别
        /utf-8           # 源文件UTF-8编码
        /MP              # 多处理器编译
        /Zc:preprocessor # 符合标准的预处理器
        /Zc:__cplusplus  # 正确的 __cplusplus 宏值
    )
    # 将特定警告降级（第三方库引发的）
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd4201") # nameless struct/union (EnTT使用)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd5321") # 多字节UTF-8字符 (sol2注释中的emoji)
endif()

# -----------------------------------------------------------------------------
# 全局 Include 路径
# -----------------------------------------------------------------------------
# 引擎源码根目录加入 include 路径，方便 #include "engine/ecs/..."
include_directories(${CMAKE_CURRENT_SOURCE_DIR})

# -----------------------------------------------------------------------------
# 第三方依赖 — FetchContent
# -----------------------------------------------------------------------------
include(FetchContent)

# 公共选项：关闭测试、示例、安装（减少编译时间）
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

# --- GLFW 3.4（窗口创建 + 输入） ---
FetchContent_Declare(glfw
    GIT_REPOSITORY https://gitcode.com/glfw/glfw.git
    GIT_TAG 3.4
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# --- EnTT（ECS框架，header-only） ---
# gitclone浅克隆不包含tag，使用commit hash对应v3.14.0
FetchContent_Declare(entt
    GIT_REPOSITORY https://gitclone.com/github.com/skypjack/entt.git
    GIT_TAG 1333fa53129e7cfded5a9640c4336a254049917b
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)
FetchContent_MakeAvailable(entt)

# --- GLM（数学库，header-only） ---
# gitclone浅克隆不包含tag，使用commit hash对应1.0.1
FetchContent_Declare(glm
    GIT_REPOSITORY https://gitclone.com/github.com/g-truc/glm.git
    GIT_TAG 6f14f4792a0cde5d0cf2c910506724d61cb95834
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)
FetchContent_MakeAvailable(glm)

# --- spdlog（异步日志，header-only） ---
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://gitcode.com/gabime/spdlog.git
    GIT_TAG v1.14.1
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)
FetchContent_MakeAvailable(spdlog)

# --- nlohmann/json（序列化，header-only） ---
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://gitcode.com/nlohmann/json.git
    GIT_TAG v3.11.3
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)
FetchContent_MakeAvailable(nlohmann_json)

# --- OpenGL（Windows 系统自带 opengl32.lib） ---
# Phase 1 使用原生 OpenGL 3.3；后续 Phase 切换 bgfx 时替换此段
find_package(OpenGL REQUIRED)

# --- stb_image（单头文件图像加载库，public domain） ---
FetchContent_Declare(stb
    GIT_REPOSITORY https://gitcode.com/nothings/stb.git
    GIT_TAG master
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)
FetchContent_MakeAvailable(stb)

# --- Lua 5.4（脚本引擎，静态库） ---
include(cmake/Lua54.cmake)

# --- sol2（Lua C++ 绑定，header-only） ---
include(cmake/sol2.cmake)

# --- ZeroMQ（C++ ↔ Python IPC 通信） ---
# libzmq: C 库；cppzmq: C++ 头文件封装
FetchContent_Declare(libzmq
    GIT_REPOSITORY https://gitcode.com/zeromq/libzmq.git
    GIT_TAG master
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)
set(BUILD_SHARED ON CACHE BOOL "" FORCE)
set(BUILD_STATIC OFF CACHE BOOL "" FORCE)
set(ZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(WITH_PERF_TOOL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(libzmq)

FetchContent_Declare(cppzmq
    GIT_REPOSITORY https://gitcode.com/zeromq/cppzmq.git
    GIT_TAG master
    GIT_SHALLOW ON
    GIT_PROGRESS ON
)
set(CPPZMQ_BUILD_TESTS OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(cppzmq)

# --- Phase 6.2: SoLoud（音频引擎） ---
include(cmake/SoLoud.cmake)

# -----------------------------------------------------------------------------
# 子目录
# -----------------------------------------------------------------------------
add_subdirectory(engine)
add_subdirectory(sample)

# CPack 安装目标（必须在 add_subdirectory 之后）
install(TARGETS sample_game RUNTIME DESTINATION bin)
install(DIRECTORY assets/ DESTINATION assets)

# --- Phase 6.8: ImGui 编辑器 ---
include(cmake/ImGui.cmake)

# --- Phase 6.7: CPack 打包 ---
include(cmake/Packaging.cmake)
