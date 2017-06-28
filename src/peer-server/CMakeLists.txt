CMAKE_MINIMUM_REQUIRED(VERSION 3.1)

CreateTool(peer-server
DIRS
	.
FEATURES
	cxx_auto_type cxx_range_for
LINK
	humblepeer
	sha1
	websockets
	${CMAKE_THREAD_LIBS_INIT}
PROPERTIES
	FOLDER HumbleNet/Server
)

source_group("Source" REGULAR_EXPRESSION ".")
source_group("" FILES CMakeLists.txt)