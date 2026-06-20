# =============================================================================
# 示例游戏可执行文件
# =============================================================================
add_executable(sample_game
    main.cpp
)

set_target_properties(sample_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)

target_link_libraries(sample_game PRIVATE engine)
