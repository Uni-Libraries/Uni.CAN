cmake_minimum_required(VERSION 3.21)

project(uni.can)


#
# CPM
#

include(cmake/cpm.cmake)
include(cmake/cpm-uni.common.cmake)
if(NOT WIN32)
	include(cmake/cpm-libsocketcan.cmake)
endif()

if(PROJECT_IS_TOP_LEVEL)
	include(cmake/cpm-fmtlib.cmake)
    include(cmake/cpm-spdlog.cmake)
    include(cmake/cpm-nlohmann_json.cmake)
	include(cmake/cpm-catch2.cmake)
	include(cmake/cpm-uni.gui.cmake)
    include(cmake/cpm-magic_enum.cmake)
endif()



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

include(cmake/3rdparty-marathon.cmake)
include(cmake/3rdparty-peak.cmake)
include(cmake/3rdparty-vector.cmake)
include(cmake/3rdparty-waveshare.cmake)



#
# Uni.CAN
#

add_library(uni.can STATIC)

# sources
file(GLOB_RECURSE uni.can_SRCS CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "src/*.cc" "src/*.cxx")
file(GLOB_RECURSE uni.can_HDRS CONFIGURE_DEPENDS "include/*.h" "include_private/*.h")
target_sources(uni.can PRIVATE ${uni.can_SRCS} ${uni.can_HDRS})

# includes
target_include_directories(uni.can PUBLIC include)
target_include_directories(uni.can PRIVATE src)

# defines
target_compile_definitions(uni.can PRIVATE NOMINMAX)

# properties
set_target_properties(uni.can PROPERTIES CXX_STANDARD 17)
set_target_properties(uni.can PROPERTIES CXX_STANDARD_REQUIRED ON)

# link
target_link_libraries(uni.can PUBLIC uni.common)

target_link_libraries(uni.can PUBLIC chai)
if(WIN32)
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
