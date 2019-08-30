CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

set(HUMBLENET_SERVER_URL "ws://localhost:8080/ws" CACHE STRING "The peer server to use for the test apps")

if(FLATC_EXECUTABLE)
	GenerateFlatBuffer(test_peer_flatbuffers test_peer.fbs)

	CreateTool(humblenet_test_peer
	FILES
		test_peer.cpp
		${test_peer_flatbuffers}
	REFERENCE
		test_peer.fbs
	DEFINES
		HUMBLENET_SERVER_URL=\"${HUMBLENET_SERVER_URL}\"
	FEATURES
		cxx_auto_type cxx_range_for
	INCLUDES
		${CMAKE_CURRENT_BINARY_DIR}
	LINK
		humblenet
		flatbuffers
		-g
	PROPERTIES
		FOLDER HumbleNet/Tests
	)

	list(APPEND TEST_TARGETS
		humblenet_test_peer
	)
endif()

if(HUMBLENET_DESKTOP)
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
	list(APPEND TEST_TARGETS
		humblenet_test_webrtc
	)

	CreateTool(humblenet_test_loader
	FILES
		test_loader.cpp
	LINK
		humblenet_loader
	PROPERTIES
		FOLDER HumbleNet/Tests
	)
	list(APPEND TEST_TARGETS
		humblenet_test_loader
	)

	add_dependencies(humblenet_test_loader humblenet_shared)
endif()

CreateTool(humblenet_hello_world
FILES
	hello_world.cpp
DEFINES
	HUMBLENET_SERVER_URL=\"${HUMBLENET_SERVER_URL}\"
FEATURES
	cxx_auto_type cxx_range_for
LINK
	humblenet
PROPERTIES
	FOLDER HumbleNet/Tests
)
list(APPEND TEST_TARGETS
	humblenet_hello_world
)

if(TEST_TARGETS)
	add_custom_target(all_tests)
	set_target_properties(all_tests PROPERTIES FOLDER HumbleNet/Tests)
	add_dependencies(all_tests ${TEST_TARGETS})
endif()
