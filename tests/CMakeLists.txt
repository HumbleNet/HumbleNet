CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

if(HUMBLENET_DESKTOP)
	list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/Catch2/lib/cmake)
	find_package(Catch2 REQUIRED)

	include(Catch)

	CreateTool(humblenet_test_webrtc
	FILES
		test_webrtc.cpp
	ASM_FLAGS
		ASSERTIONS=2
		DEMANGLE_SUPPORT=1
	DEFINES
		HUMBLENET_SERVER_URL=\"${HUMBLENET_SERVER_URL}\"
	FEATURES
		cxx_auto_type cxx_range_for
	LINK
		humblenet
		-g
	PROPERTIES
		FOLDER HumbleNet/Tests
	)

	CreateTool(humblenet_test_loader
	FILES
		test_loader.cpp
	LINK
		humblenet_loader
	DEPENDENCIES
		humblenet_shared
	PROPERTIES
		FOLDER HumbleNet/Tests
	)

	CreateTool(test_circular_buffer
	FILES
		test_circular_buffer.cpp
	FEATURES
		cxx_noexcept cxx_explicit_conversions
	PROPERTIES
		FOLDER HumbleNet/Tests
	LINK
		Catch2::Catch2
	)

	catch_discover_tests(test_circular_buffer)

	CreateTool(test_attributes
	FILES
		test_attributes.cpp
		../src/humblenet/src/humblenet_attributes.cpp
	INCLUDES
		../src/humblenet/include
		../src/humblenet/src
		../src/humblenet/humblepeer
	FEATURES
	PROPERTIES
		FOLDER HumbleNet/Tests
	LINK
		Catch2::Catch2
	)

	catch_discover_tests(test_attributes)

	CreateTool(test_humblepeer
	FILES
		test_humblepeer.cpp
	PROPERTIES
		FOLDER HumbleNet/Tests
	LINK
		Catch2::Catch2
		humblepeer
	)

	catch_discover_tests(test_humblepeer)

	CreateTool(test_server_p2p_connection
	FILES
		test_server_p2p_connection.cpp
		../src/peer-server/p2p_connection.cpp
	DEFINES
		HUMBLENET_STATIC
	PROPERTIES
		FOLDER HumbleNet/Tests
	LINK
		Catch2::Catch2
		humblepeer
		websockets
	)

	catch_discover_tests(test_server_p2p_connection)

	CreateTool(test_client_p2p_connection
	FILES
		test_client_p2p_connection.cpp
		../src/humblenet/src/humblenet_p2p_connection.cpp
		../src/humblenet/src/humblenet_events.cpp
		../src/humblenet/src/humblenet_core.cpp
		../src/humblenet/src/mutex.cpp
	INCLUDES
		../src/humblenet/src/
	DEFINES
		HUMBLENET_STATIC
	PROPERTIES
		FOLDER HumbleNet/Tests
	LINK
		Catch2::Catch2
		humblepeer
		${CMAKE_THREAD_LIBS_INIT}
	)

	catch_discover_tests(test_client_p2p_connection)
endif()

