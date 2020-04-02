CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
set(CMAKE_USER_MAKE_RULES_OVERRIDE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/c_flag_overrides.cmake)
set(CMAKE_USER_MAKE_RULES_OVERRIDE_CXX ${CMAKE_CURRENT_SOURCE_DIR}/cmake/cxx_flag_overrides.cmake)

PROJECT(humblenet)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

if(APPLE)
	set(CMAKE_OSX_ARCHITECTURES x86_64 CACHE STRING "" FORCE)
	set(CMAKE_OSX_DEPLOYMENT_TARGET 10.9 CACHE STRING "" FORCE)
endif()

include(BoilerPlate)
include(UtilityFunctions)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})

if(WIN32 OR LINUX OR APPLE)
	set(HUMBLENET_NATIVE ON)
	set(HUMBLENET_DESKTOP ON)
endif()

option(HUMBLENET_SERVER "Build the humblenet Server" ${HUMBLENET_DESKTOP})
option(HUMBLENET_TESTS "Build the humblenet tests" ON)
option(HUMBLENET_SAMPLES "Build the humblenet samples" ON)

if(LINUX OR APPLE)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fvisibility=hidden")
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden")
endif()

option(ENABLE_COVERAGE "Enable Code coverage" OFF)

if(ENABLE_COVERAGE)
	set(CMAKE_REQUIRED_LINK_OPTIONS "--coverage")
	check_cxx_compiler_flag("--coverage" CHECK_CXX_COVERAGE_FLAGS)
	unset(CMAKE_REQUIRED_LINK_OPTIONS)
	if (CHECK_CXX_COVERAGE_FLAGS)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} --coverage")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} --coverage")
	endif()
endif()

find_program(FLATC_EXECUTABLE
	NAMES flatc
	NO_CMAKE_FIND_ROOT_PATH
)

if(NOT FLATC_EXECUTABLE)
	message(WARNING "Flatc not found.  Flatbuffers headers will not be rebuilt.  Please install flatc.")
endif()

function(GenerateFlatBuffer outvar)
	set(_extra_args "")
	foreach(_fbs ${ARGN})
		if (_fbs STREQUAL "SCOPED_ENUMS")
			list(APPEND _extra_args --scoped-enums)
		else()
			get_filename_component(_full_fbs ${_fbs} ABSOLUTE)
			if(NOT EXISTS ${_full_fbs})
				message(FATAL_ERROR "Could not found input files ${_fbs}")
			endif()
			get_filename_component(_base_name ${_fbs} NAME_WE)
			set(_output ${CMAKE_CURRENT_BINARY_DIR}/${_base_name}_generated.h)
			list(APPEND _generated_sources ${_output})
			if (FLATC_EXECUTABLE)
				set_source_files_properties(${_output} PROPERTIES GENERATED ON)
				add_custom_command(
					OUTPUT ${_output}
					COMMAND ${FLATC_EXECUTABLE}
					ARGS -o ${CMAKE_CURRENT_BINARY_DIR} ${_extra_args} --cpp ${_full_fbs}
					DEPENDS ${_full_fbs}
					COMMENT "Generating flatbuffer source for ${_fbs}"
				)
			endif()
		endif()
	endforeach()
	set(${outvar} ${_generated_sources} PARENT_SCOPE)
endfunction()

add_library(flatbuffers INTERFACE)
target_include_directories(flatbuffers INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/flatbuffers/include)


if(HUMBLENET_NATIVE)
	include(ExternalProject)
	include(SelectLibraryConfigurations)
	if(APPLE OR WIN32 OR LINUX)
		set(DEFAULT_USE_BORING_SSL ON)
	endif()
	option(USE_BORINGSSL "Use BoringSSL instead of OpenSSL" ${DEFAULT_USE_BORING_SSL})

	if(LINUX_X86)
		set(LINUX_ARCH_CONFIG
			-DOVERRIDE_ARCH:STRING=x86
			-DCMAKE_ASM_FLAGS:STRING=-m32
			-DCMAKE_C_FLAGS:STRING=-m32
			-DCMAKE_CXX_FLAGS:STRING=-m32
		)
	elseif(LINUX_X86_64)
		set(LINUX_ARCH_CONFIG
			"-DCMAKE_C_FLAGS:STRING=-m64 -fPIC"
			"-DCMAKE_CXX_FLAGS:STRING=-m64 -fPIC"
		)
	endif()

	if(USE_BORINGSSL)
		find_program(GO_EXECUTABLE go)
		if(WIN32)
			find_program(CMAKE_ASM_NASM_COMPILER
				NAMES yasm vsyasm
				HINTS ENV YASMPATH
			)
		endif()
		ExternalProject_Add(BoringSSL-build
			SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/boringssl
			CMAKE_CACHE_ARGS
				-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}
				-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
				-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
				-DGO_EXECUTABLE:FILEPATH=${GO_EXECUTABLE}
				-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
				-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
				-DCMAKE_ASM_NASM_COMPILER:FILEPATH=${CMAKE_ASM_NASM_COMPILER}
				-DCMAKE_USER_MAKE_RULES_OVERRIDE:FILEPATH=${CMAKE_USER_MAKE_RULES_OVERRIDE}
				-DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX:FILEPATH=${CMAKE_USER_MAKE_RULES_OVERRIDE_CXX}
				-DOPENSSL_NO_ASM:BOOL=ON
				-DBUILD_TESTS:BOOL=OFF
				-DBUILD_TOOLS:BOOL=OFF
				${LINUX_ARCH_CONFIG}
		)

		ExternalProject_Get_Property(BoringSSL-build INSTALL_DIR BINARY_DIR)

		if (CMAKE_CONFIGURATION_TYPES)
			SET(BoringSSL_crypto_LIBRARY_DEBUG ${BINARY_DIR}/crypto/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}crypto${CMAKE_STATIC_LIBRARY_SUFFIX})
			SET(BoringSSL_crypto_LIBRARY_RELEASE ${BINARY_DIR}/crypto/Release/${CMAKE_STATIC_LIBRARY_PREFIX}crypto${CMAKE_STATIC_LIBRARY_SUFFIX})
			select_library_configurations(BoringSSL_crypto)
			SET(BoringSSL_ssl_LIBRARY_DEBUG ${BINARY_DIR}/ssl/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}ssl${CMAKE_STATIC_LIBRARY_SUFFIX})
			SET(BoringSSL_ssl_LIBRARY_RELEASE ${BINARY_DIR}/ssl/Release/${CMAKE_STATIC_LIBRARY_PREFIX}ssl${CMAKE_STATIC_LIBRARY_SUFFIX})
			select_library_configurations(BoringSSL_ssl)
			set(LWS_BORING_LIBS ${BoringSSL_ssl_LIBRARY} ${BoringSSL_crypto_LIBRARY})
			set(LWS_BORING_INTERFACE
				$<$<CONFIG:Debug>:${BoringSSL_ssl_LIBRARY_DEBUG}>
				$<$<NOT:$<CONFIG:Debug>>:${BoringSSL_ssl_LIBRARY_RELEASE}>
				$<$<CONFIG:Debug>:${BoringSSL_crypto_LIBRARY_DEBUG}>
				$<$<NOT:$<CONFIG:Debug>>:${BoringSSL_crypto_LIBRARY_RELEASE}>
			)
		else()
			set(BoringSSL_crypto_LIBRARY
				"${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}crypto${CMAKE_STATIC_LIBRARY_SUFFIX}"
			)
			set(BoringSSL_ssl_LIBRARY
				"${INSTALL_DIR}/lib/${EP_BUILD_SUFFIX}${CMAKE_STATIC_LIBRARY_PREFIX}ssl${CMAKE_STATIC_LIBRARY_SUFFIX}"
			)
			set(LWS_BORING_LIBS ${BoringSSL_ssl_LIBRARY} ${BoringSSL_crypto_LIBRARY})
			set(LWS_BORING_INTERFACE ${LWS_BORING_LIBS})
		endif()

		set(BoringSSL_INCLUDE_DIR
			${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/boringssl/include
		)

		set(LWS_BORING_CONFIG
			-DLWS_USE_BORINGSSL:BOOL=ON
			-DLWS_BORINGSSL_LIB:STRING=${LWS_BORING_LIBS}
			-DLWS_BORINGSSL_INCLUDE_DIRS:STRING=${BoringSSL_INCLUDE_DIR}
		)
	endif()

	add_subdirectory(3rdparty/webrtc		webrtc)

	add_subdirectory(3rdparty/chromium_webrtc		chromium_webrtc)

	if(NOT WIN32)
		find_package(ZLIB)
		set(LWS_ZLIB_CONFIG
			-DZLIB_INCLUDE_DIR:STRING=${ZLIB_INCLUDE_DIR}
			-DZLIB_LIBRARY_RELEASE:STRING=${ZLIB_LIBRARY}
			-DZLIB_LIBRARY:STRING=${ZLIB_LIBRARY}
		)
	endif()

	ExternalProject_Add(websockets-build
		SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/3rdparty/libwebsockets
		CMAKE_CACHE_ARGS
			-DCMAKE_OSX_ARCHITECTURES:STRING=${CMAKE_OSX_ARCHITECTURES}
			-DCMAKE_OSX_DEPLOYMENT_TARGET:STRING=${CMAKE_OSX_DEPLOYMENT_TARGET}
			-DCMAKE_INSTALL_PREFIX:PATH=<INSTALL_DIR>
			-DCMAKE_C_COMPILER:FILEPATH=${CMAKE_C_COMPILER}
			-DCMAKE_CXX_COMPILER:FILEPATH=${CMAKE_CXX_COMPILER}
			-DCMAKE_USER_MAKE_RULES_OVERRIDE:FILEPATH=${CMAKE_USER_MAKE_RULES_OVERRIDE}
			-DCMAKE_USER_MAKE_RULES_OVERRIDE_CXX:FILEPATH=${CMAKE_USER_MAKE_RULES_OVERRIDE_CXX}
			-DLWS_WITHOUT_TESTAPPS:BOOL=ON
			${LWS_ZLIB_CONFIG}
			${LWS_BORING_CONFIG}
			${LINUX_ARCH_CONFIG}
	)
	if(USE_BORINGSSL)
		ExternalProject_Add_StepDependencies(websockets-build configure BoringSSL-build)
	endif()

	ExternalProject_Get_Property(websockets-build INSTALL_DIR BINARY_DIR)

	file(MAKE_DIRECTORY ${INSTALL_DIR}/include)


	if(WIN32)
		# win32 adds a _static to the .lib
		set(WEBSOCKET_SUFFIX _static)
		# win32 setup ZLIB target
		CreateLibrary(ZLIB
			DIRS
				3rdparty/libwebsockets/win32port/zlib/
			EXCLUDE
				3rdparty/libwebsockets/win32port/zlib/gzio.c
		)
		set(ZLIB_LIBRARY ZLIB)
	endif()

	add_library(websockets STATIC IMPORTED)
	set_target_properties(websockets
		PROPERTIES
			INTERFACE_INCLUDE_DIRECTORIES ${INSTALL_DIR}/include
	)

	if (CMAKE_CONFIGURATION_TYPES)
		set_target_properties(websockets
			PROPERTIES
				IMPORTED_LOCATION ${BINARY_DIR}/lib/Release/${CMAKE_STATIC_LIBRARY_PREFIX}websockets${WEBSOCKET_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
				IMPORTED_LOCATION_DEBUG ${BINARY_DIR}/lib/Debug/${CMAKE_STATIC_LIBRARY_PREFIX}websockets${WEBSOCKET_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
		)
	else()
		set_target_properties(websockets
			PROPERTIES
				IMPORTED_LOCATION ${INSTALL_DIR}/lib/${CMAKE_STATIC_LIBRARY_PREFIX}websockets${WEBSOCKET_SUFFIX}${CMAKE_STATIC_LIBRARY_SUFFIX}
		)
	endif()


	set_target_properties(websockets
		PROPERTIES
			INTERFACE_LINK_LIBRARIES "${ZLIB_LIBRARY};${LWS_BORING_INTERFACE}"
	)

	add_dependencies(websockets websockets-build)
endif()

if(NOT EMSCRIPTEN)
	find_package(Threads)
endif()

include(CheckFunctionExists)
include(CheckLibraryExists)

check_function_exists(clock_gettime HAVE_CLOCK_GETTIME)
if(NOT HAVE_CLOCK_GETTIME)
	find_library(RT_LIBRARY NAMES rt)
	if (RT_LIBRARY)
		unset(HAVE_CLOCK_GETTIME CACHE)
		check_library_exists(${RT_LIBRARY} clock_gettime "time.h" HAVE_CLOCK_GETTIME)
	endif()
endif()

check_function_exists(dlopen HAVE_DLOPEN)
if(NOT HAVE_DLOPEN)
	find_library(DL_LIBRARY NAMES dl)
	if (DL_LIBRARY)
		unset(HAVE_DLOPEN CACHE)
		check_library_exists(${DL_LIBRARY} dlopen "dlfcn.h" HAVE_DLOPEN)
	endif()
endif()

if(HUMBLENET_DESKTOP)
	add_subdirectory(3rdparty/Amalgamate	amalgamate-build)
	add_subdirectory(3rdparty/json-parser	json-parser)
	add_subdirectory(src/api_builder		apibuilder-build)
endif()
add_subdirectory(3rdparty/crc				crc)
add_subdirectory(3rdparty/sha1				sha1)
add_subdirectory(src/humblenet				humblenet)
if(HUMBLENET_SERVER)
	add_subdirectory(src/peer-server		peer-server-build)
endif()
if(HUMBLENET_TESTS)
	add_subdirectory(tests					test)
endif()

if(HUMBLENET_SAMPLES)
	add_subdirectory(samples				samples)
endif()