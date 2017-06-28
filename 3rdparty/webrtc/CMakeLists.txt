CMAKE_MINIMUM_REQUIRED(VERSION 2.8.12)
PROJECT(webrtc_microstack)

if(USE_BORINGSSL)
	set(OPENSSL_INCLUDE_DIR ${BoringSSL_INCLUDE_DIR})
	set(OPENSSL_LIBRARIES ${BoringSSL_ssl_LIBRARY} ${BoringSSL_crypto_LIBRARY})
else()
	find_package(OpenSSL)
endif()

CreateLibrary(webrtc_microstack
DIRS
	core
	Microstack
EXCLUDE
	Microstack/sha1.c
INCLUDES
	PUBLIC
		core
		Microstack
		${OPENSSL_INCLUDE_DIR}
DEFINES
	$<$<NOT:$<BOOL:${WIN32}>>:_POSIX>
	PUBLIC
		$<$<BOOL:${WIN32}>:WINSOCK2 MICROSTACK_NO_STDAFX>
FLAGS
	$<$<BOOL:${LINUX_X86_64}>:-fPIC>
LINK
	${CMAKE_THREAD_LIBS_INIT}
	${OPENSSL_LIBRARIES}
	${CMAKE_DL_LIBS}
	$<$<BOOL:${WIN32}>:Iphlpapi.lib>
	$<$<BOOL:${RT_LIBRARY}>:${RT_LIBRARY}>
PROPERTIES
	FOLDER 3rdParty/Library
)

if(USE_BORINGSSL)
	add_dependencies(webrtc_microstack BoringSSL-build)
endif()
