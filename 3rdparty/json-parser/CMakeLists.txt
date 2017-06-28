CMAKE_MINIMUM_REQUIRED(VERSION 3.1)
PROJECT(json-parser)

CreateLibrary(jsonparser
DIRS
	.
INCLUDES
	PUBLIC
		.
PROPERTIES
	FOLDER 3rdParty/Library
)
