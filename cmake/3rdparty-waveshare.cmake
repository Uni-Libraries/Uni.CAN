if(WIN32)
	add_library(waveshare SHARED IMPORTED)

    set_target_properties(waveshare PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/waveshare-windows/include")

	if("${CMAKE_SIZEOF_VOID_P}" STREQUAL "8")
		set_target_properties(waveshare PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/waveshare-windows/bin64/ControlCAN.dll")
		set_target_properties(waveshare PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/waveshare-windows/lib64/ControlCAN.lib")
	else()
		set_target_properties(waveshare PROPERTIES IMPORTED_LOCATION "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/waveshare-windows/bin32/ControlCAN.dll")
		set_target_properties(waveshare PROPERTIES IMPORTED_IMPLIB   "${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/waveshare-windows/lib32/ControlCAN.lib")
	endif()
endif()

