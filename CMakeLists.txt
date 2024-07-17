cmake_minimum_required(VERSION 3.21)

project(uni.can)


#
# CPM
#

include(cmake/cpm.cmake)
include(cmake/cpm-uni.common.cmake)

if(PROJECT_IS_TOP_LEVEL)
	include(cmake/cpm-catch2.cmake)
endif()

if(NOT WIN32)
	include(cmake/cpm-libsocketcan.cmake)
endif()



#
# Marathon-Chai
#

if(WIN32)
	add_library(chai SHARED IMPORTED)

    set_target_properties(chai PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/marathon-chai/include")

	if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
		set_target_properties(chai PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/marathon-chai/bin64/chai.dll")
		set_target_properties(chai PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/marathon-chai/lib64/chai.lib")
	else()
		set_target_properties(chai PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/marathon-chai/bin32/chai.dll")
		set_target_properties(chai PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/marathon-chai/lib32/chai.lib")
	endif()
endif()



#
# HMS IXXAT VCI4
#

if(MSVC)
	add_library(ixxat-vci SHARED IMPORTED)

	set_target_properties(ixxat-vci PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hms-ixxat/inc")

	if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
		set_target_properties(ixxat-vci PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hms-ixxat/bin64/vciapi.dll")
		set_target_properties(ixxat-vci PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hms-ixxat/lib64/Release/vciapi.lib")
	else()
		set_target_properties(ixxat-vci PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hms-ixxat/bin32/vciapi.dll")
		set_target_properties(ixxat-vci PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/hms-ixxat/lib32/Release/vciapi.lib")
	endif()
endif()



#
# Uni.CAN
#

add_library(uni.can STATIC)

file(GLOB_RECURSE uni.can_SRCS CONFIGURE_DEPENDS "src/*.c" "src/*.cpp" "src/*.cc" "src/*.cxx")
file(GLOB_RECURSE uni.can_HDRS CONFIGURE_DEPENDS "include/*.h" "include_private/*.h")

target_sources(uni.can PRIVATE ${uni.can_SRCS} ${uni.can_HDRS})

target_include_directories(uni.can PUBLIC include)
target_include_directories(uni.can PRIVATE include_private)
target_include_directories(uni.can PRIVATE src)

set_target_properties(uni.can PROPERTIES CXX_STANDARD 17)
set_target_properties(uni.can PROPERTIES CXX_STANDARD_REQUIRED ON)

target_link_libraries(uni.can PUBLIC uni.common)

if(WIN32)
	target_link_libraries(uni.can PUBLIC chai)
	if(MSVC)
		target_link_libraries(uni.can PUBLIC ixxat-vci)
	endif()
endif()

if(CMAKE_COMPILER_IS_GNUCC AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0)
	target_link_libraries(uni.can PUBLIC stdc++fs)
endif()

if(NOT WIN32)
	target_link_libraries(uni.can PUBLIC libsocketcan)
endif()


#
# Tests
#

if(PROJECT_IS_TOP_LEVEL)
	add_subdirectory(src_tests)
endif()
