# =============================================================================
# CPack 打包配置 — Phase 6.7
# =============================================================================
# 生成 NSIS 安装器（Windows）+ ZIP 便携版
# 使用：cmake --build build --target package
# =============================================================================

set(CPACK_PACKAGE_NAME           "AI_Game_Frame")
set(CPACK_PACKAGE_VENDOR         "AI Game Frame Team")
set(CPACK_PACKAGE_VERSION        "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "AI-driven 2.5D interactive narrative game framework")

# Windows: NSIS 安装器 + ZIP
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME      "AI Game Frame Demo")
    set(CPACK_NSIS_INSTALL_ROOT      "$PROGRAMFILES64")
    set(CPACK_NSIS_CREATE_ICONS_EXTRA
        "CreateShortCut '$DESKTOP\\\\AI Game Frame.lnk' '$INSTDIR\\\\bin\\\\sample_game.exe'")
    set(CPACK_NSIS_DELETE_ICONS_EXTRA
        "Delete '$DESKTOP\\\\AI Game Frame.lnk'")
else()
    set(CPACK_GENERATOR "TGZ;ZIP")
endif()

include(CPack)
