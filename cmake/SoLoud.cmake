# =============================================================================
# SoLoud 音频引擎构建配置
# =============================================================================
# 使用 FetchContent 从 gitcode 镜像下载，手动枚举源文件构建静态库。
# 模式与 cmake/Lua54.cmake 一致。
#
# SoLoud 是宽松的 zlib/libpng 许可，可自由用于商业项目。
# =============================================================================

if(NOT TARGET soloud)
    FetchContent_Declare(soloud_src
        GIT_REPOSITORY https://gitcode.com/jarikomppa/soloud.git
        GIT_TAG        master
        GIT_SHALLOW    ON
    )
    FetchContent_MakeAvailable(soloud_src)

    # SoLoud 核心源文件
    set(SOLOUD_CORE_SOURCES
        ${soloud_src_SOURCE_DIR}/src/core/soloud.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_audiosource.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_bus.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_core_3d.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_core_basicops.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_core_faderops.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_core_filterops.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_core_getters.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_core_setters.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_core_voicegroup.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_core_voiceops.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_fader.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_fft.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_fft_lut.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_file.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_filter.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_misc.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_queue.cpp
        ${soloud_src_SOURCE_DIR}/src/core/soloud_thread.cpp
    )

    # 音频文件格式支持（WAV + MP3 + FLAC + OGG via stb_vorbis）
    set(SOLOUD_AUDIOSOURCE_SOURCES
        ${soloud_src_SOURCE_DIR}/src/audiosource/wav/soloud_wav.cpp
        ${soloud_src_SOURCE_DIR}/src/audiosource/wav/dr_impl.cpp
        ${soloud_src_SOURCE_DIR}/src/audiosource/wav/stb_vorbis.c
    )

    # Windows 后端（WinMM，零额外依赖）
    set(SOLOUD_BACKEND_SOURCES
        ${soloud_src_SOURCE_DIR}/src/backend/winmm/soloud_winmm.cpp
    )

    add_library(soloud STATIC
        ${SOLOUD_CORE_SOURCES}
        ${SOLOUD_AUDIOSOURCE_SOURCES}
        ${SOLOUD_BACKEND_SOURCES}
    )

    target_include_directories(soloud PUBLIC
        ${soloud_src_SOURCE_DIR}/include
    )

    if(MSVC)
        target_compile_definitions(soloud PRIVATE
            _CRT_SECURE_NO_WARNINGS
            WITH_WINMM
        )
        target_compile_options(soloud PRIVATE /W3 /wd4244 /wd4267 /wd4305)
    endif()
endif()
