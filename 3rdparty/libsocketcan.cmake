
add_library(libsocketcan STATIC)
target_include_directories(libsocketcan PUBLIC "${CMAKE_CURRENT_LIST_DIR}/libsocketcan/include")
target_sources(libsocketcan PRIVATE "${CMAKE_CURRENT_LIST_DIR}/libsocketcan/src/libsocketcan.c")
