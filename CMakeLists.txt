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
    GIT_REPOSITORY https://github.com/glfw/glfw.git
    GIT_TAG 3.4
)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(glfw)

# --- EnTT（ECS框架，header-only） ---
FetchContent_Declare(entt
    GIT_REPOSITORY https://github.com/skypjack/entt.git
    GIT_TAG v3.14.0
)
FetchContent_MakeAvailable(entt)

# --- GLM（数学库，header-only） ---
FetchContent_Declare(glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG 1.0.1
)
FetchContent_MakeAvailable(glm)

# --- spdlog（异步日志，header-only） ---
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.14.1
)
FetchContent_MakeAvailable(spdlog)

# --- nlohmann/json（序列化，header-only） ---
FetchContent_Declare(nlohmann_json
    GIT_REPOSITORY https://github.com/nlohmann/json.git
    GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

# --- bgfx（跨平台渲染库） ---
# 使用 bgfx.cmake 包装项目（bgfx 作者官方维护）
FetchContent_Declare(bgfx
    GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
    GIT_TAG 35e5a6bb8453b4d3e3a741bd35940c5ce5000f4a
)
set(BGFX_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BGFX_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(bgfx)

# -----------------------------------------------------------------------------
# 子目录
# -----------------------------------------------------------------------------
add_subdirectory(engine)
add_subdirectory(sample)
