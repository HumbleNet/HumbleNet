CMAKE_MINIMUM_REQUIRED(VERSION 3.0.2)
PROJECT(chromium_webrtc)

set(WEBRTC_ROOT "" CACHE PATH "Location of the chromium WebRTC source directory")

if( NOT WEBRTC_ROOT )
    return()
endif()

if( NOT EXISTS "${WEBRTC_ROOT}/webrtc/api/peerconnection.h" )
    message( STATUS "Could not fine ${WEBRTC_ROOT}/webrtc/api/peerconnection.h" )
    return()
endif()


if( APPLE )
    if( EXISTS "${WEBRTC_ROOT}/xcodebuild/" )
        set(WEBRTC_BIN_DIR "${WEBRTC_ROOT}/xcodebuild/" )
    elseif( EXISTS "${WEBRTC_ROOT}/out/" )
        set(WEBRTC_BIN_DIR "${WEBRTC_ROOT}/out" )
    endif()
    file( GLOB_RECURSE WEBRTC_LIBRARY_DEBUG ${WEBRTC_BIN_DIR}/Debug/*.a )
    file( GLOB_RECURSE EXCLUDES_DEBUG ${WEBRTC_BIN_DIR}/Debug/*test*.a )
    file( GLOB_RECURSE WEBRTC_LIBRARY_RELEASE ${WEBRTC_BIN_DIR}/Release/*.a )
    file( GLOB_RECURSE EXCLUDES_RELEASE ${WEBRTC_ROOT_BIN_DIR}/Release/*test*.a )
elseif( LINUX )
    if( NOT EXISTS "${WEBRTC_ROOT}/out" )
        # auto prepare based on current arch..
        # GYP_GENERATOR_FLAGS="output_dir=$PWD/src/out/x86_64" GYP_DEFINES="host_arch=x86_64 ztarget_arch=ia32" ../gclient gclient runhooks
    endif()
    if( LINUX_X86 AND EXISTS "${WEBRTC_ROOT}/out/x86/Release" )
        set(WEBRTC_BIN_DIR "${WEBRTC_ROOT}/out/x86" )
    elseif( LINUX_X86_64 AND EXISTS "${WEBRTC_ROOT}/out/x86_64/Release" )
        set(WEBRTC_BIN_DIR "${WEBRTC_ROOT}/out/x86_64" )
    elseif( EXISTS "${WEBRTC_ROOT}/out/Release" )
        set(WEBRTC_BIN_DIR "${WEBRTC_ROOT}/out" )
    endif()
    file( GLOB_RECURSE WEBRTC_LIBRARY_DEBUG ${WEBRTC_BIN_DIR}/Debug/*.a )
    file( GLOB_RECURSE EXCLUDES_DEBUG ${WEBRTC_BIN_DIR}/Debug/*test*.a )
    file( GLOB_RECURSE WEBRTC_LIBRARY_RELEASE ${WEBRTC_BIN_DIR}/Release/*.a )
    file( GLOB_RECURSE EXCLUDES_RELEASE ${WEBRTC_ROOT_BIN_DIR}/Release/*test*.a )
elseif( WIN32 )
    if( NOT EXISTS "${WEBRTC_ROOT}/out" )
        message(FATAL_ERROR "No out directory.  Have you run gclient sync?")
    endif()
    set(WEBRTC_BIN_DIR "${WEBRTC_ROOT}/out" )
    if ( CMAKE_CL_64 )
        file( GLOB_RECURSE WEBRTC_LIBRARY_DEBUG ${WEBRTC_BIN_DIR}/Debug_x64/*.lib )
        file( GLOB_RECURSE EXCLUDES_DEBUG ${WEBRTC_BIN_DIR}/Debug_x64/*test*.lib )
        file( GLOB_RECURSE WEBRTC_LIBRARY_RELEASE ${WEBRTC_BIN_DIR}/Release_x64/*.lib )
        file( GLOB_RECURSE EXCLUDES_RELEASE ${WEBRTC_ROOT_BIN_DIR}/Release_x64/*test*.lib )
    else()
        file( GLOB_RECURSE WEBRTC_LIBRARY_DEBUG ${WEBRTC_BIN_DIR}/Debug/*.lib )
        file( GLOB_RECURSE EXCLUDES_DEBUG ${WEBRTC_BIN_DIR}/Debug/*test*.lib )
        file( GLOB_RECURSE WEBRTC_LIBRARY_RELEASE ${WEBRTC_BIN_DIR}/Release/*.lib )
        file( GLOB_RECURSE EXCLUDES_RELEASE ${WEBRTC_ROOT_BIN_DIR}/Release/*test*.lib )
    endif()
else()
    message(FATAL_ERROR "Not compatible with current arch")
endif()

set( WEBRTC_INCLUDE_DIR "${WEBRTC_ROOT}/" )

if( EXCLUDES_DEBUG )
    list(REMOVE_ITEM WEBRTC_LIBRARY_DEBUG ${EXCLUDES_DEBUG})
endif()

if( EXCLUDES_RELEASE )
    list(REMOVE_ITEM WEBRTC_LIBRARY_RELEASE ${EXCLUDES_RELEASE})
endif()

include(SelectLibraryConfigurations)

select_library_configurations(WEBRTC)

if( NOT WEBRTC_LIBRARY_RELEASE AND NOT WEBRTC_LIBRARY_DEBUG )
    message( FATAL_ERROR "You must build chromium webrtc first: \
    cd ${WEBRTC_ROOT}   \
    webrtc/build/gyp_webrtc  # -f xcode --generator-output=   \
    ninja -C out/Debug  \
    ninja -C out/Release        \
    \ 
or for xcode projects: (must disable all the mac && ia32 checks to get ia32 to build)\
    webrtc/build/gyp_webrtc -f xcode --generator-output=xcodeproj \
    \
target arch can be controlled by adding\
    -Dtarget_arch=<arch>\
    \
for more info see https://www.chromium.org/developers/gyp-environment-variables \
    ")
endif()

if( NOT WEBRTC_LIBRARY_RELEASE )
    message( "WARNING: unable to locate release libraries, only debug builds will be possible")
endif()

CreateLibrary( chromium_webrtc_faked
    FILES
        ${WEBRTC_ROOT}/webrtc/api/test/fakeaudiocapturemodule.cc
    FEATURES
        cxx_auto_type
    INCLUDES
        PRIVATE
            ${WEBRTC_INCLUDE_DIR}
    FLAGS
        PRIVATE
            $<$<BOOL:${LINUX_X86_64}>:-fPIC;-fno-rtti>
            $<$<BOOL:${APPLE}>:-fno-rtti>
    DEFINES
        $<$<BOOL:${WIN32}>:WEBRTC_WIN;_CRT_SECURE_NO_WARNINGS;NOMINMAX;_UNICODE;UNICODE>
        $<$<BOOL:${LINUX}>:WEBRTC_POSIX>
        $<$<BOOL:${APPLE}>:WEBRTC_POSIX>
)

LIST( APPEND WEBRTC_LIBRARIES chromium_webrtc_faked )

if( LINUX )
    LIST( INSERT WEBRTC_LIBRARIES 0 "-Wl,--start-group" )
    LIST( APPEND WEBRTC_LIBRARIES "-Wl,--end-group" )

    LIST( APPEND WEBRTC_LIBRARIES X11 )
elseif( WIN32 )
    LIST( APPEND WEBRTC_LIBRARIES Secur32 dmoguids Msdmo wmcodecdspuuid )
elseif( APPLE )
    find_library( CORE_AUDIO CoreAudio )
    find_library( AUDIO_TOOLBOX AudioToolbox )
    find_library( FOUNDATION Foundation )
    find_library( APPLICATION_SERVICES ApplicationServices )

    LIST( APPEND WEBRTC_LIBRARIES ${CORE_AUDIO} ${AUDIO_TOOLBOX} ${FOUNDATION} ${APPLICATION_SERVICES} )
endif()

add_library( chromium_webrtc INTERFACE )

target_link_libraries( chromium_webrtc
    INTERFACE 
        ${WEBRTC_LIBRARIES}
)
target_include_directories( chromium_webrtc
    INTERFACE
        ${WEBRTC_INCLUDE_DIR}
)
target_compile_definitions( chromium_webrtc
    INTERFACE
        $<$<BOOL:${WIN32}>:WEBRTC_WIN;_CRT_SECURE_NO_WARNINGS;NOMINMAX;_UNICODE;UNICODE>
        $<$<BOOL:${LINUX}>:WEBRTC_POSIX>
        $<$<BOOL:${APPLE}>:WEBRTC_POSIX>
)
