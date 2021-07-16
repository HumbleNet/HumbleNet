
find_package(nlohmann_json REQUIRED)

CreateTool(APIBuilder
DIRS
	.
INCLUDES
FEATURES
	cxx_auto_type cxx_range_for
LINK
	nlohmann_json::nlohmann_json
PROPERTIES
	FOLDER HumbleNet/Tools
)