if(WIN32)
	add_library(vxlapi SHARED IMPORTED)

    set_target_properties(vxlapi PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/vxlapi-windows/bin")

	if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
		set_target_properties(vxlapi PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/vxlapi-windows/bin/vxlapi64.dll")
		set_target_properties(vxlapi PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/vxlapi-windows/bin/vxlapi64.lib")
	else()
		set_target_properties(vxlapi PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/vxlapi-windows/bin/vxlapi.dll")
		set_target_properties(vxlapi PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/vxlapi-windows/bin/vxlapi.lib")
	endif()
endif()

