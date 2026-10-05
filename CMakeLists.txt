cmake_minimum_required(VERSION 3.21)

#
# Options
#

# The channels of the adapters for a PC and the factory that finds them are written in C++ and
# need threads. Everything else is C: messages, the channel functions and the protocols.
# A firmware is built from that part alone.
option(UNI_CAN_C_ONLY "Build only the parts written in C: no adapter backends, no factory, no C++ compiler" OFF)

if(UNI_CAN_C_ONLY)
    project(uni.can LANGUAGES C)
else()
    project(uni.can LANGUAGES C CXX)
endif()

if(UNI_CAN_C_ONLY AND PROJECT_IS_TOP_LEVEL)
    message(FATAL_ERROR "UNI_CAN_C_ONLY is for use of Uni.CAN inside another project: the application and the tests need C++")
endif()



#
# Dependencies
#

# a project that brings Uni.Common itself, e.g. with Uni.HAL, keeps its own
if(NOT TARGET uni.common)
    include(3rdparty/uni_common.cmake)
endif()

if(NOT UNI_CAN_C_ONLY)
    find_package(Threads REQUIRED)

    include(3rdparty/libsocketcan.cmake)
endif()

if(PROJECT_IS_TOP_LEVEL)

    include(cmake/cpm.cmake)
	include(cmake/cpm-fmtlib.cmake)
    include(cmake/cpm-spdlog.cmake)
    include(cmake/cpm-nlohmann_json.cmake)
	include(cmake/cpm-catch2.cmake)
	include(cmake/cpm-uni.gui.cmake)
    include(cmake/cpm-magic_enum.cmake)
endif()



if(NOT UNI_CAN_C_ONLY)
    #
    # HMS IXXAT VCI4
    #
    if(WIN32)
        add_library(ixxat-vci SHARED IMPORTED)

        set_target_properties(ixxat-vci PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hms-ixxat/inc")

        if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
            set_target_properties(ixxat-vci PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hms-ixxat/bin64/vciapi.dll")
            set_target_properties(ixxat-vci PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hms-ixxat/lib64/Release/vciapi.lib")
        else()
            set_target_properties(ixxat-vci PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hms-ixxat/bin32/vciapi.dll")
            set_target_properties(ixxat-vci PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hms-ixxat/lib32/Release/vciapi.lib")
        endif()

        target_sources(ixxat-vci INTERFACE "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hms-ixxat/src/guid.c")
    endif()

    if(WIN32)
        include(cmake/3rdparty-marathon.cmake)
    endif()
    include(cmake/3rdparty-peak.cmake)
    include(cmake/3rdparty-vector.cmake)
    include(cmake/3rdparty-waveshare.cmake)
endif()



#
# Uni.CAN
#

add_library(uni.can STATIC)

# sources: the C part ...
file(GLOB uni.can_SRCS CONFIGURE_DEPENDS "src/*.c")
file(GLOB_RECURSE uni.can_HDRS CONFIGURE_DEPENDS "include/*.h" "include_private/*.h")
target_sources(uni.can PRIVATE ${uni.can_SRCS} ${uni.can_HDRS})

# ... and the adapter backends with their factory
if(NOT UNI_CAN_C_ONLY)
    file(GLOB_RECURSE uni.can_SRCS_CXX CONFIGURE_DEPENDS "src/*.cpp" "src/*.cc" "src/*.cxx")
    target_sources(uni.can PRIVATE ${uni.can_SRCS_CXX})
endif()

# includes
target_include_directories(uni.can PUBLIC include)
target_include_directories(uni.can PRIVATE src)

# defines
target_compile_definitions(uni.can PRIVATE NOMINMAX)

# properties
if(NOT UNI_CAN_C_ONLY)
    set_target_properties(uni.can PROPERTIES CXX_STANDARD 17)
    set_target_properties(uni.can PROPERTIES CXX_STANDARD_REQUIRED ON)
endif()

# link
target_link_libraries(uni.can PUBLIC uni.common)

if(NOT UNI_CAN_C_ONLY)
    if(WIN32)
        target_link_libraries(uni.can PUBLIC chai)
        target_link_libraries(uni.can PUBLIC ixxat-vci)
        target_link_libraries(uni.can PUBLIC pcanbasic)
        target_link_libraries(uni.can PUBLIC vxlapi)
        target_link_libraries(uni.can PUBLIC waveshare)
    endif()
    if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
        target_link_libraries(uni.can PUBLIC stdc++fs)
    endif()
    if(NOT WIN32)
        target_link_libraries(uni.can PUBLIC libsocketcan)
    endif()
endif()



#
# Uni.CAN GUI
#

if(PROJECT_IS_TOP_LEVEL)
	add_subdirectory(src_app)
endif()



#
# Tests
#

if(PROJECT_IS_TOP_LEVEL)
	enable_testing()
	add_subdirectory(src_tests)
endif()
