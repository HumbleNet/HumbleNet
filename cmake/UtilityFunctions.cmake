cmake_minimum_required(VERSION 2.8.12)

### build up utility functions
include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

function(CheckCFlags outvar)
    foreach(flag ${ARGN})
        string(REGEX REPLACE "[^a-zA-Z0-9_]+" "_" cleanflag ${flag})
        check_c_compiler_flag(${flag} CHECK_C_FLAG_${cleanflag})
        if(CHECK_C_FLAG_${cleanflag})
            list(APPEND valid ${flag})
        endif()
    endforeach()
    set(${outvar} ${valid} PARENT_SCOPE)
endfunction()

function(CheckCXXFlags outvar)
    foreach(flag ${ARGN})
        string(REGEX REPLACE "[^a-zA-Z0-9_]+" "_" cleanflag ${flag})
        check_cxx_compiler_flag(${flag} CHECK_CXX_FLAG_${cleanflag})
        if(CHECK_CXX_FLAG_${cleanflag})
            list(APPEND valid ${flag})
        endif()
    endforeach()
    set(${outvar} ${valid} PARENT_SCOPE)
endfunction()

# Helper to ensures a scope has been set for certain target properties
macro(_SetDefaultScope var_name default_scope)
    list(GET ${var_name} 0 __setdefaultscope_temp)
    if(__setdefaultscope_temp STREQUAL "PRIVATE" OR __setdefaultscope_temp STREQUAL "PUBLIC" OR __setdefaultscope_temp STREQUAL "INTERFACE")
    else()
        list(INSERT ${var_name} 0 ${default_scope})
    endif()
    unset(__setdefaultscope_temp)
endmacro()

function(MakeCopyFileDepenency outvar file)
    if(IS_ABSOLUTE ${file})
        set(_input ${file})
    else()
        set(_input ${CMAKE_CURRENT_SOURCE_DIR}/${file})
    endif()
    if(ARGC GREATER "2")
        if(IS_ABSOLUTE ${ARGV2})
            set(_output ${ARGV2})
        else()
            set(_output ${CMAKE_CURRENT_BINARY_DIR}/${ARGV2})
        endif()
    else()
        get_filename_component(_outfile ${file} NAME)
        set(_output ${CMAKE_CURRENT_BINARY_DIR}/${_outfile})
    endif()

    add_custom_command(
        OUTPUT ${_output}
        COMMAND ${CMAKE_COMMAND} -E copy ${_input} ${_output}
        DEPENDS ${_input}
        COMMENT "Copying ${file}"
    )
    set(${outvar} ${_output} PARENT_SCOPE)
endfunction()

# magic function to handle the power functions below
function(_BuildDynamicTarget name type)
    set(_mode "files")
    foreach(dir ${ARGN})
        if(dir STREQUAL "EXCLUDE")
            set(_mode "excl")
        elseif(dir STREQUAL "DIRS")
            set(_mode "dirs")
        elseif(dir STREQUAL "FILES")
            set(_mode "files")
        elseif(dir STREQUAL "REFERENCE")
            set(_mode "reference")
        elseif(dir STREQUAL "INCLUDES")
            set(_mode "incl")
        elseif(dir STREQUAL "DEFINES")
            set(_mode "define")
        elseif(dir STREQUAL "PREFIX")
            set(_mode "prefix")
        elseif(dir STREQUAL "FLAGS")
            set(_mode "flags")
        elseif(dir STREQUAL "FEATURES")
            if(NOT COMMAND "target_compile_features")
                message(FATAL_ERROR "CMake 3.1+ is required to use this feature")
            endif()
            set(_mode ${dir})
        elseif(dir STREQUAL "LINK")
            set(_mode "link")
		elseif(dir STREQUAL "DEPENDENCIES")
			set(_mode "dependencies")
        elseif(dir STREQUAL "PROPERTIES")
            set(_mode "properties")
        elseif(dir STREQUAL "SHARED")
            set(type "shared")
        # Simple Copying files to build dir
        elseif(dir STREQUAL "COPY_FILES")
            set(_mode "copyfiles")
        # Emscripten handling
        elseif(dir STREQUAL "ASM_FLAG" OR dir STREQUAL "ASM_FLAGS")
            set(_mode "em_asmflag")
        elseif(dir STREQUAL "PRE_JS")
            set(_mode "em_prejs")
        elseif(dir STREQUAL "JS_LIBS")
            set(_mode "em_jslib")
        # The real work
        else()
            if(_mode STREQUAL "excl")
                if (dir MATCHES "\\/\\*\\*$")
                    string(LENGTH ${dir} dir_LENGTH)
                    math(EXPR dir_LENGTH "${dir_LENGTH} - 3")
                    string(SUBSTRING ${dir} 0 ${dir_LENGTH} dir)

                    file(GLOB_RECURSE _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
                        ${dir}/*.*
                    )
                else()
                    file(GLOB _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
                        ${dir}
                    )
                endif()
                if(_files)
                    list(REMOVE_ITEM _source_files
                        ${_files}
                    )
                endif()
                set(_files)
            elseif(_mode STREQUAL "files")
                if (dir MATCHES "\\*")
                    file(GLOB _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
                        ${dir}
                    )
                else()
                    set(_files ${dir})
                endif()
                if(_files)
                    list(APPEND _source_files
                        ${_files}
                    )
                endif()
                set(_files)
            elseif(_mode STREQUAL "incl")
                list(APPEND _include_dirs
                    ${dir}
                )
            elseif(_mode STREQUAL "define")
                list(APPEND _definitions
                    ${dir}
                )
            elseif(_mode STREQUAL "flags")
                list(APPEND _flags
                    ${dir}
                )
            elseif(_mode STREQUAL "FEATURES")
                list(APPEND _features
                    ${dir}
                )
            elseif(_mode STREQUAL "link")
                list(APPEND _link_libs
                    ${dir}
                )
            elseif(_mode STREQUAL "dependencies")
                list(APPEND _dependencies
                    ${dir}
                )
            elseif(_mode STREQUAL "properties")
                list(APPEND _properties
                    ${dir}
                )
            elseif(_mode STREQUAL "reference")
                if (dir MATCHES "\\*")
                    file(GLOB _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
                        ${dir}
                    )
                else()
                    set(_files ${dir})
                endif()
                if(_files)
                    list(APPEND _reference
                        ${_files}
                    )
                endif()
                set(_files)
            elseif(_mode STREQUAL "prefix")
                if(IS_ABSOLUTE ${dir})
                    list(APPEND _flags
                        PRIVATE "-include ${dir}"
                    )
                elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
                    list(APPEND _flags
                        PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/${dir}
                    )
                else()
                    message(FATAL_ERROR "could not find refix header")
                endif()
            elseif(_mode STREQUAL "dirs")
                if (dir STREQUAL ".")
                    set(dir ${CMAKE_CURRENT_SOURCE_DIR})
                endif()
                if (dir MATCHES "\\/\\*\\*$")
                    string(LENGTH ${dir} dir_LENGTH)
                    math(EXPR dir_LENGTH "${dir_LENGTH} - 3")
                    string(SUBSTRING ${dir} 0 ${dir_LENGTH} dir)

                    file(GLOB_RECURSE _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
                        ${dir}/*.c
                        ${dir}/*.cpp
                        ${dir}/*.cxx
                        ${dir}/*.cc
                        ${dir}/*.h
                        ${dir}/*.hpp
                        ${dir}/*.inl
                        ${dir}/*.m
                        ${dir}/*.mm
                    )
                else()
                    file(GLOB _files RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
                        ${dir}/*.c
                        ${dir}/*.cpp
                        ${dir}/*.cxx
                        ${dir}/*.cc
                        ${dir}/*.h
                        ${dir}/*.hpp
                        ${dir}/*.inl
                        ${dir}/*.m
                        ${dir}/*.mm
                    )
                endif()
                if(_files)
                    list(APPEND _source_files
                        ${_files}
                    )
                endif()
            # simple copy files
            elseif(_mode STREQUAL "copyfiles")
                MakeCopyFileDepenency(_copyfile_target
                    ${dir}
                )
                list(APPEND _source_files
                    ${_copyfile_target}
                )
                unset(_copyfile_target)
            # emscripten handling
            elseif(_mode STREQUAL "em_asmflag")
                list(APPEND _em_asmflag
                    "-s ${dir}"
                )
            elseif(_mode STREQUAL "em_prejs")
                list(APPEND _em_prejs
                    ${dir}
                )
            elseif(_mode STREQUAL "em_jslib")
                if(IS_ABSOLUTE ${dir})
                    list(APPEND _em_jslib
                        ${dir}
                    )
                elseif(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
                    list(APPEND _em_jslib
                        ${CMAKE_CURRENT_SOURCE_DIR}/${dir}
                    )
                else()
                    message(FATAL_ERROR "Could not find ${dir} in current source dir")
                endif()
            else()
                message(FATAL_ERROR "Unknown Mode ${_mode}")
            endif()
        endif()
    endforeach()
    if (NOT _source_files)
        message(FATAL_ERROR "Could not find any sources for ${name}")
    endif()
    if(_reference)
        list(APPEND _source_files ${_reference})
        set_source_files_properties(${_reference}
            PROPERTIES
                HEADER_FILE_ONLY TRUE
        )
    endif()
    if(type STREQUAL "lib")
        add_library(${name} STATIC EXCLUDE_FROM_ALL
            ${_source_files}
        )
    elseif(type STREQUAL "shared")
        add_library(${name} SHARED
            ${_source_files}
        )
        set_target_properties(${name} PROPERTIES
            MACOSX_RPATH ON
        )
    elseif(type STREQUAL "object")
        add_library(${name} OBJECT
            ${_source_files}
        )
    elseif(type STREQUAL "module")
        add_library(${name} MODULE
            ${_source_files}
        )
    elseif(type STREQUAL "tool")
        add_executable(${name}
            ${_source_files}
        )
        if(LINUX)
            get_property(is64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
            if(is64)
                set_target_properties(${name} PROPERTIES SUFFIX ".bin.x86_64")
            else()
                set_target_properties(${name} PROPERTIES SUFFIX ".bin.x86")
            endif()
            set_target_properties(${name} PROPERTIES
                LINK_FLAGS "-Wl,--allow-shlib-undefined" # Voodoo to ignore the libs that steam_api is linked to (will be resolved at runtime)
            )
        elseif(EMSCRIPTEN)
            set_target_properties(${name} PROPERTIES
                SUFFIX ".html"
            )
        endif()
    else()
        add_executable(${name} MACOSX_BUNDLE WIN32
            ${_source_files}
        )
        if(LINUX)
            get_property(is64 GLOBAL PROPERTY FIND_LIBRARY_USE_LIB64_PATHS)
            if(is64)
                set_target_properties(${name} PROPERTIES SUFFIX ".bin.x86_64")
            else()
                set_target_properties(${name} PROPERTIES SUFFIX ".bin.x86")
            endif()
            set_target_properties(${name} PROPERTIES
                LINK_FLAGS "-Wl,--allow-shlib-undefined" # Voodoo to ignore the libs that steam_api is linked to (will be resolved at runtime)
            )
        endif()
    endif()
    if(_include_dirs)
        _SetDefaultScope(_include_dirs PRIVATE)
        target_include_directories(${name} ${_include_dirs})
    endif()
    if(_definitions)
        _SetDefaultScope(_definitions PRIVATE)
        target_compile_definitions(${name} ${_definitions})
    endif()
    if(_features)
        _SetDefaultScope(_features PRIVATE)
        target_compile_features(${name} ${_features})
    endif()
    if(_link_libs)
        target_link_libraries(${name} ${_link_libs})
    endif()
    if(_dependencies)
        add_dependencies(${name} ${_dependencies})
    endif()
    if(_flags)
        _SetDefaultScope(_flags PRIVATE)
        target_compile_options(${name} ${_flags})
    endif()
    if(_properties)
        set_target_properties(${name} PROPERTIES
            ${_properties}
        )
    endif()
    if(EMSCRIPTEN)
        if(_em_jslib)
            em_link_js_library(${name} ${_em_jslib})
        endif()
        if(_em_prejs)
            em_link_pre_js(${name} ${_em_prejs})
        endif()
        if(_em_asmflag)
            target_link_libraries(${name}
                ${_em_asmflag}
            )
        endif()
    endif()
endfunction()

## These two power functions build up library and program targets
## 
## the parameters are simply the target name followed by a list of directories or other parameters
## parameters that can be specified
## FILES      followed by a list of explicit files/globs to add (or generated files).
## DIRS       followed by a list of directories ..  will glob in *.c, *.cpp, *.h, *.hpp, *.inl, *.m, *.mm
##                      If you end the string with ** it will pull in recursively.
## REFERENCE  followed by a list of explicit files/globs to add.
##                      These files will be available in the IDE, but will NOT be compiled. (ini files, txt files)
## EXCLUDE    followed by a list of files/globs to exclude
## INCLUDES   followed by a list of include directories.
##                      These use Generator expressions (see CMAKE documentation) default is PRIVATE scoped
## DEFINES    followed by a list of compiler defines.
##                      These use Generator expressions (see CMAKE documentation) default is PRIVATE scoped
## FLAGS      followed by a list of compiler flags.
##                      These use Generator expressions (see CMAKE documentation) default is PRIVATE scoped
## FEATURES   followed by a list of compiler features (cmake 3.1+).
##                      These use Generator expressions (see CMAKE documentation) default is PRIVATE scoped
## PREFIX     followed by a header. Basic prefix header support..  currently only GCC/Clang is supported.
## LINK       followed by a list of link targets.  Can use Generator expressions (see CMAKE documentation)
## PROPERTIES followed by a list of target properties.
## COPY_FILES followed by a list of files to copy to the build directory.
##                      If relative, assumes relative to source dir
##  Emscripten/asm.js ONLY flags
## ASM_FLAG   followed by a list of emscripten flags (without the -s.. e.g.  FULL_ES2=1)
## PRE_JS     followed by a list of js files to preinclude in the .js output
##                      If relative, assumes relative to source dir
## JS_LIBS    followed by a list of JS libs to include during emscripten link phase.
##                      If relative, assumes relative to source dir

function(CreateSharedLibrary name)
    _BuildDynamicTarget(${name} shared ${ARGN})
endfunction()

function(CreateObjectLibrary name)
    _BuildDynamicTarget(${name} object ${ARGN})
endfunction()

function(CreateModule name)
    _BuildDynamicTarget(${name} module ${ARGN})
endfunction()

function(CreateLibrary name)
    _BuildDynamicTarget(${name} lib ${ARGN})
endfunction()

function(CreateProgram name)
    _BuildDynamicTarget(${name} exe ${ARGN})
endfunction()

function(CreateTool name)
    _BuildDynamicTarget(${name} tool ${ARGN})
endfunction()

## Helper functions to copy libs
function(_CleanGeneratorExpressions var out_var debug_opt_var)
    set(debug_opt "")
    set(clean ${var})
    if (var MATCHES "\\$<")
        string(REGEX REPLACE "^\\$<(.+>|[^:]+):(.+)>$" "\\2" clean ${var})
        string(REGEX REPLACE "\\$<(.+>|[^:]+):(.+)>" "\\1" condition ${var})
        if (condition STREQUAL "$<CONFIG:DEBUG>")
            set(debug_opt "debug")
        elseif(condition STREQUAL "$<NOT:$<CONFIG:DEBUG>>")
            set(debug_opt "optimized")
        endif()
    endif()
    set(${debug_opt_var} ${debug_opt} PARENT_SCOPE)
    set(${out_var} ${clean} PARENT_SCOPE)
endfunction()

function(FindLinkedLibs target libs)
    get_target_property(lib_list ${target} INTERFACE_LINK_LIBRARIES)
    if(NOT lib_list)
        return()
    endif()

    #message(STATUS "Checking ${target} :: ${lib_list}")
    if (ARGV2 GREATER "0")
        set(_extra ON)
        math(EXPR level "${ARGV2} - 1")
    endif()

    foreach (lib ${lib_list})
        _CleanGeneratorExpressions(${lib} lib debug_opt)
        get_filename_component(ext ${lib} EXT)
        if(ext)
            string(SUBSTRING ${ext} 1 -1 ext2)
            get_filename_component(ext2 "${ext2}" EXT)
            if(ext2)
                set(ext ${ext2})
            endif()
        endif()
        if(TARGET ${lib})
            if(_extra)
                FindLinkedLibs(${lib} shared_libs ${level})
            endif()
        elseif(ext STREQUAL ".framework" OR ext STREQUAL CMAKE_SHARED_LIBRARY_SUFFIX OR ext STREQUAL CMAKE_IMPORT_LIBRARY_SUFFIX)
            if(debug_opt)
                list(APPEND shared_libs ${debug_opt})
            endif()
            list(APPEND shared_libs ${lib})
        else()
#            message(STATUS "Skipping ${lib} -- ${ext}")
        endif()
    endforeach()

#    message(STATUS "Target: ${target} Shared: ${shared_libs}")
    set(${libs} ${shared_libs} PARENT_SCOPE)
endfunction()

function(CopyAssets target)
    set(_script_file ${CMAKE_CURRENT_BINARY_DIR}/${target}_copyassets.cmake)
    file(WRITE ${_script_file}
        "set(assets \"${ARGN}\")\n"
        "\n"
        "get_filename_component(asset_dir \"\${APP_TARGET}\" DIRECTORY)\n"
        "if (APPLE) # an OS X Bundle\n"
        "  include(BundleUtilities)\n"
        "  get_bundle_and_executable(\"\${APP_TARGET}\" bundle executable valid)\n"
        "  if(valid)\n"
        "    set(asset_dir \"\${bundle}/Contents/Resources\")\n"
        "  endif()\n"
        "endif()\n"
        "file(MAKE_DIRECTORY \${asset_dir})\n"
        "foreach(_asset \${assets})\n"
        "  string(REGEX REPLACE \"^([^@]+)@?.*\" \"\\\\1\" _source \"\${_asset}\")\n"
        "  if (_asset MATCHES \"@\")\n"
        "    string(REGEX REPLACE \"^([^@]+)@(.*)$\" \"\\\\2\" _dest \"\${_asset}\")\n"
        "  else()\n"
        "    get_filename_component(_dest \"\${_asset}\" NAME)\n"
        "  endif()\n"
        "  configure_file(\"\${_source}\" \"\${asset_dir}/\${_dest}\" COPYONLY)\n"
        "endforeach()\n"
    )

    add_custom_command(TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -DAPP_TARGET="$<TARGET_FILE:${target}>" -P "${_script_file}"
        COMMENT "Copying Assets for ${target}"
    )
endfunction()

function(CopyDependentLibs target)
    if(EMSCRIPTEN)
        return()
    endif()
    set(_mode "lib")

    FindLinkedLibs(${target} __libs 2)
    list(APPEND _libs ${__libs})

    foreach(entry ${ARGN})
        if(entry STREQUAL "TARGETS")
            set(_mode "targets")
        elseif(entry STREQUAL "EXTRA_LIBS")
            set(_mode "extra")
        else()
            if(_mode STREQUAL "targets")
                FindLinkedLibs(${entry} __libs 2)
                list(APPEND _libs ${__libs})
                set(__libs)
            elseif(_mode STREQUAL "lib")
                list(APPEND _libs ${entry})
            elseif(_mode STREQUAL "extra")
                list(APPEND _extra_libs ${entry})
            else()
                message(FATAL_ERROR "Unknown mode ${_mode}")
            endif()
        endif()
    endforeach()


    # we don't sort extralibs as it may have debug;optimized entries in it

    # Fetch library rpath relative directory from global properties
    get_property(lib_rpath_dir GLOBAL PROPERTY LIBRARY_RPATH_DIRECTORY)
    if(NOT lib_rpath_dir)
        set(lib_rpath_dir "")
    endif()

    set(_SCRIPT_FILE "${CMAKE_CURRENT_BINARY_DIR}/${target}_copylibs.cmake")
    file(WRITE ${_SCRIPT_FILE}
        "# Generated Script file\n"
        "include(GetPrerequisites)\n"
        "set(source_libs \"${_libs}\")\n"
        "set(extra_libs \"${_extra_libs}\")\n"
        "\n"
        "if (APPLE) # an OS X Bundle\n"
        "  include(BundleUtilities)\n"
        "  get_bundle_and_executable(\"\${BUNDLE_APP}\" bundle executable valid)\n"
        "  if(valid)\n"
        "    set(dest \"\${bundle}/Contents/Frameworks\")\n"
        "    get_prerequisites(\${executable} lib_list 1 0 \"\" \"\")\n"
        "    foreach(lib \${lib_list})\n"
        "      get_filename_component(lib_file \"\${lib}\" NAME)\n"
        "      foreach(slib \${source_libs})\n"
        "         get_filename_component(slib_file \"\${slib}\" NAME)\n"
        "         if(lib_file STREQUAL slib_file)\n"
        "           file(COPY \"\${slib}\" DESTINATION \"\${dest}\")\n"
        "         endif()\n"
        "      endforeach()\n"
        "    endforeach()\n"
        "  else()\n"
        "    message(WARNING \"App Not found? \${BUNDLE_APP}\")\n"
        "  endif()\n"
        "else() # Not an OS X bundle\n"
        "  set(executable \"\${BUNDLE_APP}\")\n"
        "  get_filename_component(executable_dir \"\${executable}\" DIRECTORY)\n"
        "  get_prerequisites(\${executable} lib_list 1 0 \"\" \"\")\n"
        "  set(dest \${executable_dir}/\${LIB_RPATH_DIR})\n"
        "  file(MAKE_DIRECTORY \${dest})\n"
        "  set(_skipreq OFF)\n"
        "  foreach(lib \${lib_list} \${extra_libs})\n"
        "    if(_skipreq)\n"
        "      set(_skipreq OFF)\n"
        "    elseif( (lib STREQUAL \"debug\" AND NOT USE_DEBUG) OR (lib STREQUAL \"optimized\" AND USE_DEBUG) )\n"
        "      set(_skipreq ON)\n"
        "    elseif( (lib STREQUAL \"debug\" AND USE_DEBUG) OR (lib STREQUAL \"optimized\" AND NOT USE_DEBUG) )\n"
        "      # Splitting based on debug/optimized\n"
        "    else()\n"
        "      get_filename_component(lib_file \"\${lib}\" NAME)\n"
        "      set(_skip OFF)\n"
        "      foreach(slib \${source_libs} \${extra_libs})\n"
        "        if(_skip)\n"
        "          set(_skip OFF)\n"
        "        elseif( (slib STREQUAL \"debug\" AND NOT USE_DEBUG) OR (slib STREQUAL \"optimized\" AND USE_DEBUG) )\n"
        "          set(_skip ON)\n"
        "        elseif( (slib STREQUAL \"debug\" AND USE_DEBUG) OR (slib STREQUAL \"optimized\" AND NOT USE_DEBUG) )\n"
        "          # Splitting based on debug/optimized\n"
        "        else()\n"
        "          get_filename_component(slib_file \"\${slib}\" NAME)\n"
        "          if(lib_file STREQUAL slib_file)\n"
        "            message(STATUS \"Copying library: \${lib_file}\")\n"
        "            execute_process(COMMAND \${CMAKE_COMMAND} -E copy \"\${slib}\" \"\${dest}\")\n"
        "            break()\n"
        "          else()\n"
        "            get_filename_component(slib_dir \"\${slib}\" PATH)\n"
        "            set(slib_path \"\${slib_dir}/\${lib_file}\")\n"
        "            if(EXISTS \${slib_path})\n"
        "              message(STATUS \"Copying library: \${lib_file}\")\n"
        "              execute_process(COMMAND \${CMAKE_COMMAND} -E copy \"\${slib_path}\" \"\${dest}\")\n"
        "              break()\n"
        "            endif()\n"
        "          endif()\n"
        "        endif()\n" # _skip
        "      endforeach()\n" # source lib scan
        "    endif()\n" # _skipreq
        "  endforeach()\n" # required libs
        "endif()\n"
    )
    ADD_CUSTOM_COMMAND(TARGET ${target}
        POST_BUILD
        COMMAND ${CMAKE_COMMAND} -DBUNDLE_APP="$<TARGET_FILE:${target}>" -DLIB_RPATH_DIR="${lib_rpath_dir}" -DUSE_DEBUG=$<CONFIG:Debug> -P "${_SCRIPT_FILE}"
    )
endfunction()

if(EMSCRIPTEN)
    find_file(EM_FILE_PACKAGER
        file_packager.py
        HINTS ENV EMSCRIPTEN
        PATH_SUFFIXES tools
        NO_CMAKE_FIND_ROOT_PATH
    )
    find_program(EM_PYTHON
        NAMES python2 python
        NO_CMAKE_FIND_ROOT_PATH
    )
    function(EmscriptenCreatePackage output_file out_js_file)
        set(_data_file ${CMAKE_CURRENT_BINARY_DIR}/${output_file}.data)
        set(_preload_file ${CMAKE_CURRENT_BINARY_DIR}/${output_file}.data.js)

        set(_mode "none")
        foreach(entry ${ARGN})
            if(entry STREQUAL "PRELOAD")
                set(_mode "preload")
            elseif(entry STREQUAL "ARGS")
                set(_mode "args")
            else()
                if (_mode STREQUAL "preload")
                    list(APPEND preload_map ${entry})

                    string(REGEX REPLACE "^([^@]+)@?.*" "\\1" _path "${entry}")

                    if(NOT IS_ABSOLUTE ${_path})
                        set(_path ${CMAKE_CURRENT_BINARY_DIR}/${_path})
                    endif()

                    file(GLOB_RECURSE _files
                        ${_path}/*
                    )
                    list(APPEND _packaged_files ${_files})
                elseif(_mode STREQUAL "args")
                    list(APPEND extra_opts ${entry})
                else()
                    message(FATAL_ERROR "Specify a proper mode.  PRELOAD or ARGS")
                endif()
            endif()
        endforeach()

        if(NOT preload_map)
            message(FATAL_ERROR "Specify a preload map: ${preload_map}")
        endif()

        add_custom_command(
            OUTPUT
                ${_data_file}
                ${_preload_file}
            DEPENDS
                ${_packaged_files}
            WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
            COMMAND
                ${EM_PYTHON} ${EM_FILE_PACKAGER}
            ARGS
                ${output_file}.data
                --js-output=${output_file}.data.js
                --preload ${preload_map}
                ${extra_opts}
        )
        set_source_files_properties(${_data_file} ${_preload_file} PROPERTIES GENERATED TRUE)
        set(${out_js_file} ${_preload_file} PARENT_SCOPE)
    endfunction()
endif()
