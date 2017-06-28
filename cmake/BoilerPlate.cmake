cmake_minimum_required(VERSION 2.8.12)

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
    option(USE_LIBCPP "Use the LLVM libc++ instead of GCC libstdc++ on OS X" ON)
    if(USE_LIBCPP)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
        set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
        set(CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} -stdlib=libc++")
    endif()
    if (NOT CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "10.7")
    endif()
endif()

if(EMSCRIPTEN)
    set(PLATFORM_PREFIX             "emscripten")
    set(CMAKE_EXECUTABLE_SUFFIX     ".js")
elseif(LINUX)
    set(PLATFORM_PREFIX             "linux")
    if(CMAKE_SIZEOF_VOID_P MATCHES "8" AND NOT(FORCE32) )
        set(LIB_RPATH_DIR           "lib64")
        set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS ON)
        set(LINUX_X86_64 ON)
    else()
        set(LINUX_X86 ON)
        set(LIB_RPATH_DIR           "lib")
        set_property(GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS OFF)

        ### Ensure LargeFileSupport on 32bit linux
        set(CMAKE_C_FLAGS           "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")
        set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -D_FILE_OFFSET_BITS=64")

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

    set(CMAKE_SKIP_BUILD_RPATH              TRUE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH      TRUE)
    set(CMAKE_INSTALL_RPATH                 "\$ORIGIN/${LIB_RPATH_DIR}")
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH   FALSE)
elseif(APPLE)
    set(PLATFORM_PREFIX             "macosx")

    set(BIN_RPATH "@executable_path/../Frameworks")

    set(CMAKE_SKIP_BUILD_RPATH              TRUE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH      TRUE)
    set(CMAKE_INSTALL_RPATH                 ${BIN_RPATH})
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH   FALSE)

    ### Enable SSE4 instructions
    if(ENABLE_SSE4)
        set(CMAKE_C_FLAGS           "${CMAKE_C_FLAGS} -msse4")
        set(CMAKE_CXX_FLAGS         "${CMAKE_CXX_FLAGS} -msse4")
    endif()
elseif(WIN32)
    set(PLATFORM_PREFIX             "win32")
else()
    MESSAGE(FATAL_ERROR "Unhandled Platform")
endif()

### Export parent scope
if(NOT CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(STATUS "Exporting variables to parent scope")

    set(LINUX                               ${LINUX} PARENT_SCOPE)
    set(LINUX_X86                           ${LINUX_X86} PARENT_SCOPE)
    set(LINUX_X86_64                        ${LINUX_X86_64} PARENT_SCOPE)
    set(FORCE32                             ${FORCE32} PARENT_SCOPE)
    set(PLATFORM_PREFIX                     ${PLATFORM_PREFIX} PARENT_SCOPE)

    set(CMAKE_INCLUDE_CURRENT_DIR           ${CMAKE_INCLUDE_CURRENT_DIR} PARENT_SCOPE)

if(APPLE)
    set(CMAKE_OSX_ARCHITECTURES             ${CMAKE_OSX_ARCHITECTURES} PARENT_SCOPE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET         ${CMAKE_OSX_DEPLOYMENT_TARGET} PARENT_SCOPE)
endif()

    set(CMAKE_C_FLAGS                       ${CMAKE_C_FLAGS} PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS                     ${CMAKE_CXX_FLAGS} PARENT_SCOPE)

    set(CMAKE_SKIP_BUILD_RPATH              ${CMAKE_SKIP_BUILD_RPATH} PARENT_SCOPE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH      ${CMAKE_BUILD_WITH_INSTALL_RPATH} PARENT_SCOPE)
    set(CMAKE_INSTALL_RPATH                 ${CMAKE_INSTALL_RPATH} PARENT_SCOPE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH   ${CMAKE_INSTALL_RPATH_USE_LINK_PATH} PARENT_SCOPE)
    set(CMAKE_EXECUTABLE_SUFFIX             ${CMAKE_EXECUTABLE_SUFFIX} PARENT_SCOPE)
endif()

## include guard
endif()
