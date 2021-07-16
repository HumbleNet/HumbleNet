PROJECT(Amalgamate)

if(APPLE)
	set_source_files_properties(
		juce_core_amalgam.cpp
		PROPERTIES
			HEADER_FILE_ONLY ON
	)
	set(PLATFORM_LIBRARIES
		"-framework AppKit"
		"-framework Carbon"
	)
else()
	set(EXCLUDED_SOURCES juce_core_amalgam.mm)
	set(PLATFORM_LIBRARIES "")
endif()

CheckCXXFlags(AMALGAMATE_FLAGS -Wno-deprecated-register -Wno-int-to-void-pointer-cast)

CreateTool(Amalgamate
DIRS
	.
EXCLUDE
	${EXCLUDED_SOURCES}
FEATURES
	cxx_nullptr
	cxx_rvalue_references
FLAGS
	${AMALGAMATE_FLAGS}
DEFINES
	PRIVATE
		NDEBUG
		$<$<BOOL:${WIN32}>:_CRT_SECURE_NO_WARNINGS>
LINK
	${CMAKE_THREAD_LIBS_INIT}
	${CMAKE_DL_LIBS}
	$<$<BOOL:${RT_LIBRARY}>:${RT_LIBRARY}>
	${PLATFORM_LIBRARIES}
PROPERTIES
	FOLDER 3rdParty/Tools
)
