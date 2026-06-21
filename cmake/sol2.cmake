# =============================================================================
# sol2 — Lua C++ 绑定库（header-only，本地源码）
# =============================================================================
# 源码位于 third_party/sol2/，直接从本地读取，无需网络。
#
# 用法：include(cmake/sol2.cmake) 后，include 路径自动包含 sol2 头文件。
# =============================================================================

set(sol2_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/sol2)

message(STATUS "sol2 header-only library from ${sol2_SOURCE_DIR}/include")
