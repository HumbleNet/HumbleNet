CMAKE_MINIMUM_REQUIRED(VERSION 3.1)


if(HUMBLENET_NATIVE)
	list(APPEND PLATFORM_LIBRARIES
		webrtc_microstack websockets
		${CMAKE_THREAD_LIBS_INIT}
	)
endif()
if(NOT WIN32)
	list(APPEND PLATFORM_DEFINES
		_POSIX
	)
endif()

if(HUMBLENET_DESKTOP)
	add_custom_command(OUTPUT exported_osx.map
		COMMAND APIBuilder osx ${CMAKE_CURRENT_SOURCE_DIR}/HumbleNet_API.json ${CMAKE_CURRENT_BINARY_DIR}/exported_osx.map
		MAIN_DEPENDENCY HumbleNet_API.json
		DEPENDS APIBuilder
		COMMENT "Process API to OS X exported symbol list"
	)
	add_custom_command(OUTPUT exported_linux.map
		COMMAND APIBuilder linux ${CMAKE_CURRENT_SOURCE_DIR}/HumbleNet_API.json ${CMAKE_CURRENT_BINARY_DIR}/exported_linux.map
		MAIN_DEPENDENCY HumbleNet_API.json
		DEPENDS APIBuilder
		COMMENT "Process API to Linux exported symbol list"
	)
	add_custom_command(OUTPUT cert_pem.h
		COMMAND APIBuilder include ${CMAKE_CURRENT_SOURCE_DIR}/cert.pem ${CMAKE_CURRENT_BINARY_DIR}/cert_pem.h
		MAIN_DEPENDENCY cert.pem
		DEPENDS APIBuilder
		COMMENT "Converting cert.pem into a header"
	)
	if(APPLE)
		set(PLATFORM_EXPORT "-exported_symbols_list ${CMAKE_CURRENT_BINARY_DIR}/exported_osx.map")
		set(PLATFORM_EXPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/exported_osx.map)
	elseif(LINUX)
		set(PLATFORM_EXPORT "-Wl,--version-script=${CMAKE_CURRENT_BINARY_DIR}/exported_linux.map")
		set(PLATFORM_EXPORT_FILE ${CMAKE_CURRENT_BINARY_DIR}/exported_linux.map)
	endif()

	if(PLATFORM_EXPORT_FILE)
		set_source_files_properties(${PLATFORM_EXPORT_FILE}
			PROPERTIES
				HEADER_FILE_ONLY TRUE
		)
	endif()
	set(CERT_PEM_H ${CMAKE_CURRENT_BINARY_DIR}/cert_pem.h)
endif()

GenerateFlatBuffer(FLATBUFFER_SOURCES SCOPED_ENUMS humblepeer/humblepeer.fbs)

CreateLibrary(humblepeer
	DIRS
		humblepeer
	FILES
		${FLATBUFFER_SOURCES}
	REFERENCE
		humblepeer/*.fbs
	INCLUDES
		PUBLIC
			${CMAKE_CURRENT_BINARY_DIR}
			include
			humblepeer
		INTERFACE
			src
	FLAGS
		PRIVATE
			$<$<BOOL:${LINUX_X86_64}>:-fPIC>
	FEATURES
		cxx_auto_type cxx_range_for cxx_strong_enums
	LINK
		PUBLIC
			flatbuffers
		PRIVATE
			crc
			sha1
	PROPERTIES
		FOLDER HumbleNet/Library
)

set(CORE_HUMBLENET
	DIRS
		src
		include
	FILES
		${CERT_PEM_H}
	REFERENCE
		*.json
	INCLUDES
		PUBLIC
			include
		PRIVATE
			src
	FEATURES
		cxx_auto_type cxx_range_for cxx_noexcept cxx_explicit_conversions
	DEFINES
		${PLATFORM_DEFINES}
		PRIVATE
			$<$<BOOL:${HAVE_DLOPEN}>:HAVE_DLOPEN>
			HUMBLENET_LOAD_WEBRTC
	LINK
		humblepeer
		${PLATFORM_LIBRARIES}
		$<$<BOOL:${DL_LIBRARY}>:${DL_LIBRARY}>
	PROPERTIES
		FOLDER HumbleNet/Library
)

set(CORE_HUMBLENET_LOADER
	DIRS
		loader
		include
	FILES
		${CMAKE_CURRENT_BINARY_DIR}/humblenet_loader_procs.h
	INCLUDES
		PUBLIC
			include
		PRIVATE
			loader
			${CMAKE_CURRENT_BINARY_DIR}
	DEFINES
		PRIVATE
			$<$<BOOL:${HAVE_DLOPEN}>:HAVE_DLOPEN>
	FEATURES
		cxx_auto_type cxx_range_for
	LINK
		$<$<BOOL:${DL_LIBRARY}>:${DL_LIBRARY}>
	PROPERTIES
		FOLDER HumbleNet/Library
)

CreateLibrary(humblenet
	${CORE_HUMBLENET}
	DEFINES
		PUBLIC
			HUMBLENET_STATIC
	ASM_FLAGS
		EXTRA_EXPORTED_RUNTIME_METHODS='["cwrap"]'
	PROPERTIES
		OUTPUT_NAME humblenet_static
)

if(HUMBLENET_DESKTOP)
	set(amalgam_output ${CMAKE_BINARY_DIR}/humblenet_asmjs_amalgam.cpp)

	add_custom_target(humblenet_asmjs_amalgamation
		COMMAND Amalgamate
			-i src -i humblepeer -i include -i ${CMAKE_CURRENT_BINARY_DIR}
			-i ../../3rdparty/flatbuffers/include
			-i ../../3rdparty/crc
			-i ../../3rdparty/sha1
			humblenet_asmjs_amalgam_template.cpp
			${amalgam_output}
		WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
		BYPRODUCTS ${amalgam_output}
		SOURCES humblenet_asmjs_amalgam_template.cpp

		COMMENT "Building source amalgamation for emscripten humblenet"
	)
	set_target_properties(humblenet_asmjs_amalgamation
		PROPERTIES
			FOLDER "HumbleNet/Library"
	)

	add_custom_command(OUTPUT humblenet_loader_procs.h
		COMMAND APIBuilder loader ${CMAKE_CURRENT_SOURCE_DIR}/HumbleNet_API.json ${CMAKE_CURRENT_BINARY_DIR}/humblenet_loader_procs.h
		MAIN_DEPENDENCY HumbleNet_API.json
		DEPENDS APIBuilder
		COMMENT "Process API to humblenet_loader_procs.h"
	)

	add_custom_target(humblenet_cs
		COMMAND APIBuilder csharp ${CMAKE_CURRENT_SOURCE_DIR}/HumbleNet_API.json ${CMAKE_CURRENT_SOURCE_DIR}/humblenet_api.cs ${CMAKE_BINARY_DIR}/humblenet.cs
		MAIN_DEPENDENCY HumbleNet_API.json
		DEPENDS APIBuilder "${CMAKE_CURRENT_SOURCE_DIR}/humblenet_api.cs"
		BYPRODUCTS ${CMAKE_BINARY_DIR}/humblenet.cs
		SOURCES humblenet_api.cs
		COMMENT "Process API to humblenet.cs"
	)
	set_target_properties(humblenet_cs
		PROPERTIES
			FOLDER "HumbleNet/Library"
	)

	CreateModule(humblenet_unity_editor
	${CORE_HUMBLENET_LOADER}
	FILES
		${PLATFORM_EXPORT_FILE}
	DEFINES
		PRIVATE
			HUMBLENET_DLL_BUILD
	LINK
		${PLATFORM_EXPORT}
	PROPERTIES
		BUNDLE ON
	)

	CreateLibrary(humblenet_loader
	${CORE_HUMBLENET_LOADER}
	DEFINES
		PUBLIC
			HUMBLENET_STATIC
	)
endif()

if(NOT EMSCRIPTEN)

	if( TARGET chromium_webrtc )
		if(APPLE)
			set(WEBRTC_PLATFORM_EXPORT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/webrtc/exported_osx.map")
			set(WEBRTC_PLATFORM_EXPORT "-exported_symbols_list ${WEBRTC_PLATFORM_EXPORT_FILE}")
		elseif(LINUX)
			set(WEBRTC_PLATFORM_EXPORT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/webrtc/exported_linux.map")
			set(WEBRTC_PLATFORM_EXPORT "-Wl,--version-script=${WEBRTC_PLATFORM_EXPORT_FILE}")
		elseif(WIN32)
			set(WEBRTC_PLATFORM_EXPORT_FILE "${CMAKE_CURRENT_SOURCE_DIR}/webrtc/exported_win.def")
		endif()
		
		CreateSharedLibrary(webrtc_shared
		DIRS
			webrtc
		FILES
			${WEBRTC_PLATFORM_EXPORT_FILE}
		INCLUDES
			PRIVATE
				src
		DEFINES
				WEBRTC_DLL_BUILD
		FEATURES
			cxx_auto_type cxx_range_for
		PROPERTIES
			OUTPUT_NAME webrtc
			FOLDER HumbleNet/Library
		LINK
			chromium_webrtc
			${WEBRTC_PLATFORM_EXPORT}
		)
	endif()
	
	CreateSharedLibrary(humblenet_shared
	${CORE_HUMBLENET}
	FILES
		${PLATFORM_EXPORT_FILE}
	DEFINES
		PRIVATE
			HUMBLENET_DLL_BUILD
	LINK
		${PLATFORM_EXPORT}
	PROPERTIES
		OUTPUT_NAME humblenet
	)
endif()
if(APPLE)
	CreateModule(humblenet_unity_plugin
	${CORE_HUMBLENET}
	FILES
		${PLATFORM_EXPORT_FILE}
	DEFINES
		PRIVATE
			HUMBLENET_DLL_BUILD
	LINK
		${PLATFORM_EXPORT}
	PROPERTIES
		OUTPUT_NAME humblenet
		BUNDLE ON
	)
endif()
