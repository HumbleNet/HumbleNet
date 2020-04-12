CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

if(HUMBLENET_DESKTOP)
	list(APPEND CMAKE_PREFIX_PATH ${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/Catch2/lib/cmake)
	find_package(Catch2 REQUIRED)

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
	DEPENDENCIES
		humblenet_shared
	PROPERTIES
		FOLDER HumbleNet/Tests
	)
	list(APPEND TEST_TARGETS
		humblenet_test_loader
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

	list(APPEND TEST_TARGETS
		test_circular_buffer
	)
endif()
