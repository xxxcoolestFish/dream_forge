# =============================================================================
# Lua 5.4 静态库构建（本地源码）
# =============================================================================
# 源码位于 third_party/lua/，直接从本地构建，无需网络。
#
# 用法：include(cmake/Lua54.cmake) 后即可 target_link_libraries(... lua54)
# =============================================================================

if(NOT TARGET lua54)
    set(lua_SOURCE_DIR ${CMAKE_SOURCE_DIR}/third_party/lua)

    # Lua 5.4 核心源文件（即 liblua.a 的内容，不含 lua.c 和 luac.c）
    set(LUA_CORE_SOURCES
        ${lua_SOURCE_DIR}/lapi.c
        ${lua_SOURCE_DIR}/lauxlib.c
        ${lua_SOURCE_DIR}/lbaselib.c
        ${lua_SOURCE_DIR}/lcode.c
        ${lua_SOURCE_DIR}/lcorolib.c
        ${lua_SOURCE_DIR}/lctype.c
        ${lua_SOURCE_DIR}/ldblib.c
        ${lua_SOURCE_DIR}/ldebug.c
        ${lua_SOURCE_DIR}/ldo.c
        ${lua_SOURCE_DIR}/ldump.c
        ${lua_SOURCE_DIR}/lfunc.c
        ${lua_SOURCE_DIR}/lgc.c
        ${lua_SOURCE_DIR}/linit.c
        ${lua_SOURCE_DIR}/liolib.c
        ${lua_SOURCE_DIR}/llex.c
        ${lua_SOURCE_DIR}/lmathlib.c
        ${lua_SOURCE_DIR}/lmem.c
        ${lua_SOURCE_DIR}/loadlib.c
        ${lua_SOURCE_DIR}/lobject.c
        ${lua_SOURCE_DIR}/lopcodes.c
        ${lua_SOURCE_DIR}/loslib.c
        ${lua_SOURCE_DIR}/lparser.c
        ${lua_SOURCE_DIR}/lstate.c
        ${lua_SOURCE_DIR}/lstring.c
        ${lua_SOURCE_DIR}/lstrlib.c
        ${lua_SOURCE_DIR}/ltable.c
        ${lua_SOURCE_DIR}/ltablib.c
        ${lua_SOURCE_DIR}/ltm.c
        ${lua_SOURCE_DIR}/lundump.c
        ${lua_SOURCE_DIR}/lutf8lib.c
        ${lua_SOURCE_DIR}/lvm.c
        ${lua_SOURCE_DIR}/lzio.c
    )

    add_library(lua54 STATIC ${LUA_CORE_SOURCES})
    target_include_directories(lua54 PUBLIC ${lua_SOURCE_DIR})

    if(MSVC)
        target_compile_definitions(lua54 PRIVATE
            LUA_COMPAT_5_3
            _CRT_SECURE_NO_WARNINGS
        )
        target_compile_options(lua54 PRIVATE /W3 /wd4244 /wd4267)
    endif()

    message(STATUS "Lua 5.4 static library 'lua54' from ${lua_SOURCE_DIR}")
endif()
