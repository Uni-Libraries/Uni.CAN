if(WIN32)
	add_library(pcanbasic SHARED IMPORTED)

    set_target_properties(pcanbasic PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/pcanbasic-windows/Include")

	if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
		set_target_properties(pcanbasic PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/pcanbasic-windows/x64/PCANBasic.dll")
		set_target_properties(pcanbasic PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/pcanbasic-windows/x64/VC_LIB/PCANBasic.lib")
	else()
		set_target_properties(pcanbasic PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/pcanbasic-windows/x86/PCANBasic.dll")
		set_target_properties(pcanbasic PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/pcanbasic-windows/x86/VC_LIB/PCANBasic.lib")
	endif()
endif()

