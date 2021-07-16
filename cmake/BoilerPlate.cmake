cmake_minimum_required(VERSION 3.9)

### setup options
## Include guard
if(NOT BOILERPLATE_LOADED)
    set(BOILERPLATE_LOADED ON)

option(FULL_WARNINGS "Enable full warnings" OFF)
option(ENABLE_SSE4 "Enable SSE 4" ${DEFAULT_ENABLE_SSE4})
option(FORCE32 "Force a 32bit compile on 64bit" OFF)

if("${CMAKE_SYSTEM}" MATCHES "Linux")
    set(LINUX ON)
endif()

if(EMSCRIPTEN)
    list(APPEND CMAKE_FIND_LIBRARY_SUFFIXES ".bc")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wno-warn-absolute-paths")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-warn-absolute-paths")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -g4 -s DEMANGLE_SUPPORT=1 -s ASSERTIONS=2")
endif()

if(FORCE32)
    if(LINUX AND NOT EMSCRIPTEN)
        set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -m32")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
    elseif(APPLE)
        set(CMAKE_OSX_ARCHITECTURES "i386")
    endif()
endif()

if(FULL_WARNINGS)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -Wall -Wextra")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")
endif()

if(NOT WIN32 AND NOT EMSCRIPTEN)
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -fno-strict-aliasing")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")
endif()

if(APPLE)
    if (NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
    endif()
endif()

if(EMSCRIPTEN)
elseif(LINUX)
    if(CMAKE_SIZEOF_VOID_P MATCHES "8" AND NOT(FORCE32) )
        set(LIB_RPATH_DIR           "lib64")
        set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)
        set(LINUX_X86_64 ON)
    else()
        set(LINUX_X86 ON)
        set(LIB_RPATH_DIR           "lib")
        set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)

        ### Ensure LargeFileSupport on 32bit linux
        set(CMAKE_C_FLAGS           "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE")
        set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64 -D_LARGEFILE64_SOURCE")

        ### Enable SSE instructions
        set(CMAKE_C_FLAGS           "${CMAKE_C_FLAGS} -msse -msse2 -msse3")
        set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -msse -msse2 -msse3")
        ### Enable SSE4 instructions
        if(ENABLE_SSE4)
            set(CMAKE_C_FLAGS           "${CMAKE_C_FLAGS} -msse4")
            set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -msse4")
        endif()
    endif()

    set_property(GLOBAL PROPERTY LIBRARY_RPATH_DIRECTORY ${LIB_RPATH_DIR})

    set(CMAKE_INSTALL_NAME_DIR              "")
    set(CMAKE_INSTALL_RPATH                 "\$ORIGIN/${LIB_RPATH_DIR}")
    set(CMAKE_BUILD_RPATH                   "\$ORIGIN/${LIB_RPATH_DIR}")
elseif(APPLE)
    set(BIN_RPATH "@executable_path/../Frameworks")

    set(CMAKE_INSTALL_NAME_DIR              "")
    set(CMAKE_INSTALL_RPATH                 ${BIN_RPATH})
    set(CMAKE_BUILD_RPATH                   ${BIN_RPATH})

    ### Enable SSE4 instructions
    if(ENABLE_SSE4)
        set(CMAKE_C_FLAGS           "${CMAKE_C_FLAGS} -msse4")
        set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -msse4")
    endif()
elseif(WIN32)
    if(CMAKE_CL_64)
        set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)
    endif()
elseif(ANDROID)
else()
    MESSAGE(FATAL_ERROR "Unhandled Platform")
endif()

### Export parent scope
if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(STATUS "Exporting variables to parent scope")

    set(_exposed
        LINUX LINUX_X86 LINUX_X86_64 FORCE32

        CMAKE_C_FLAGS CMAKE_CXX_FLAGS
        CMAKE_EXE_LINKER_FLAGS_DEBUG CMAKE_EXE_LINKER_FLAGS CMAKE_SHARED_LINKER_FLAGS CMAKE_MODULE_LINKER_FLAGS

        CMAKE_FIND_LIBRARY_SUFFIXES

        CMAKE_SKIP_BUILD_RPATH CMAKE_BUILD_WITH_INSTALL_RPATH
        CMAKE_INSTALL_RPATH CMAKE_INSTALL_RPATH_USE_LINK_PATH
    )

if(APPLE)
    list(APPEND _exposed CMAKE_OSX_ARCHITECTURES CMAKE_OSX_DEPLOYMENT_TARGET)
endif()

    foreach(_var ${_exposed})
        set(${_var} "${${_var}}" PARENT_SCOPE)
    endforeach()
endif()

## include guard
endif()
