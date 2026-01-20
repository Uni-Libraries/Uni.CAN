if(WIN32)
    add_executable(uni.can.app WIN32)
else()
    add_executable(uni.can.app)
endif()

target_sources(uni.can.app PRIVATE
    main.cpp
    app_state.cpp

    can_mgr.cpp
    can_mgr_state.cpp

    window_can_connect.cpp
    window_can_rx.cpp
    window_can_tx.cpp
    topbar.cpp

    protoplexer_dictionary.cpp
)

target_compile_features(uni.can.app PRIVATE cxx_std_23)

if(MINGW)
    target_link_libraries(uni.can.app PRIVATE mingw32)
endif()

target_link_libraries(uni.can.app PRIVATE uni.can)
target_link_libraries(uni.can.app PRIVATE Threads::Threads)
target_link_libraries(uni.can.app PRIVATE magic_enum)
target_link_libraries(uni.can.app PRIVATE uni.gui)
target_link_libraries(uni.can.app PRIVATE nlohmann_json)
target_link_libraries(uni.can.app PRIVATE fmt::fmt)

# Copy ProtoPlexer dictionary JSON next to executable for runtime loading.
# Run only if the source file exists.
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/protoplexer_dict.json")
    add_custom_command(TARGET uni.can.app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/protoplexer_dict.json
            $<TARGET_FILE_DIR:uni.can.app>/protoplexer_dict.json
    )
endif()

if(WIN32)
    add_custom_command(TARGET uni.can.app POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_RUNTIME_DLLS:uni.can.app> $<TARGET_FILE_DIR:uni.can.app>
            COMMAND_EXPAND_LISTS
    )
    install(
        FILES $<TARGET_RUNTIME_DLLS:uni.can.app>
        DESTINATION bin
    )
endif()

install(
    TARGETS uni.can.app
    RUNTIME DESTINATION bin
)

