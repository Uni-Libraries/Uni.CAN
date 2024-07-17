CPMAddPackage(
        NAME libsocketcan
        GITHUB_REPOSITORY linux-can/libsocketcan
        GIT_SHALLOW 1
        VERSION 0.0.12
        DOWNLOAD_ONLY True
)

if(libsocketcan_ADDED)
    add_library(libsocketcan STATIC)
    target_include_directories(libsocketcan PUBLIC "${libsocketcan_SOURCE_DIR}/include")
    target_sources(libsocketcan PRIVATE "${libsocketcan_SOURCE_DIR}/src/libsocketcan.c")
endif()
