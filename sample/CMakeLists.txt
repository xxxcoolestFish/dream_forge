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

# 自动复制 libzmq DLL 到可执行文件目录
add_custom_command(TARGET sample_game POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "$<TARGET_FILE:libzmq>"
        "$<TARGET_FILE_DIR:sample_game>"
    COMMENT "Copying libzmq DLL → exe directory"
)
