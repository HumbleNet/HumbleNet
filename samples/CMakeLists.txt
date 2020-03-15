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
		FOLDER HumbleNet/Samples
	)

	list(APPEND SAMPLE_TARGETS
		humblenet_test_peer
	)
endif()

CreateTool(lobby_demo
FILES
	lobby_demo.cpp
DEFINES
	HUMBLENET_SERVER_URL=\"${HUMBLENET_SERVER_URL}\"
FEATURES
	cxx_auto_type cxx_range_for
LINK
	humblenet
	-g
PROPERTIES
	FOLDER HumbleNet/Samples
)

list(APPEND SAMPLE_TARGETS
	lobby_demo
)

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
	FOLDER HumbleNet/Samples
)
list(APPEND SAMPLE_TARGETS
	humblenet_hello_world
)

if(SAMPLE_TARGETS)
	add_custom_target(all_samples)
	set_target_properties(all_samples PROPERTIES FOLDER HumbleNet/Samples)
	add_dependencies(all_samples ${SAMPLE_TARGETS})
endif()
