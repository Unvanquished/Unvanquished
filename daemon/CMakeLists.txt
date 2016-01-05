# Daemon BSD Source Code
# Copyright (c) 2013-2016, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#  * Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
#  * Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    documentation and/or other materials provided with the distribution.
#  * Neither the name of the <organization> nor the
#    names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
# (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
# LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
# ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
# SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

cmake_minimum_required (VERSION 3.0)
if (POLICY CMP0017)
    cmake_policy(SET CMP0017 NEW)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake PARENT_SCOPE)

include(CMakeDependentOption)

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set(MSVC_INCREMENTAL_DEFAULT ON)
# Enable the creation of project folders for Visual Studio projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Default to Release builds. To use system CFLAGS only (for distro builds), set CMAKE_BUILD_TYPE to None
if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

if (Daemon_OUT)
    set(CMAKE_CURRENT_BINARY_DIR ${Daemon_OUT})
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${Daemon_OUT})
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${Daemon_OUT})
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${Daemon_OUT})
endif()

project(Daemon C CXX)

################################################################################
# Configuration options
################################################################################

if(NOT DAEMON_EXTERNAL_APP)
    option(BUILD_CLIENT "Build Daemon client" 1)
    option(BUILD_SERVER "Build Daemonded server" 1)
    option(BUILD_GAME_NATIVE_EXE "Build native executable game logic" 1)
    option(BUILD_GAME_NATIVE_DLL "Build native shared library game logic" 1)
    if (CMAKE_SYSTEM_NAME STREQUAL CMAKE_HOST_SYSTEM_NAME)
        option(BUILD_GAME_NACL "Build PNaCl game logic" 1)
        option(BUILD_GAME_NACL_NEXE "Build NaCl game logic" 1)
    else()
        set(BUILD_GAME_NACL 0)
        set(BUILD_GAME_NACL_NEXE 0)
    endif()
    option(BUILD_TTY_CLIENT "Build Daemon headless client" 1)

    set(NACL_RUNTIME_PATH "" CACHE STRING "Directory containing the NaCl binaries")

    if (NOT APPLE AND NOT WIN32)
	option(USE_CURSES "Compile with curses support" 1)
    else()
	option(USE_CURSES "Compile with curses support" 0)
    endif()
    cmake_dependent_option(USE_SMP "Compile with support for running the renderer in a separate thread" 1 BUILD_CLIENT 0)
    cmake_dependent_option(USE_GEOIP "Use libgeoip" 1 "BUILD_SERVER OR BUILD_CLIENT OR BUILD_TTY_CLIENT" 0)
    option(USE_BREAKPAD "Use Breakpad crash logging" 0)
endif()

option(USE_LTO "Use link-time optimization for release builds" 0)
cmake_dependent_option(USE_SLIM_LTO "Generate slim LTO objects, improves build times" 1 "USE_LTO AND ${CMAKE_CXX_COMPILER_ID} STREQUAL GNU" 0)
option(USE_HARDENING "Use stack protection and other hardening flags" 0)
option(USE_WERROR "Tell the compiler to make the build fail when warnings are present" 0)
option(USE_PEDANTIC "Tell the compiler to be pedantic" 0)
option(USE_DEBUG_OPTIMIZE "Try to optimize the debug build" 1)
option(USE_PRECOMPILED_HEADER "Improve build times by using a precompiled header" 1)
option(USE_ADDRESS_SANITIZER "Try to use the address sanitizer" 0)
option(BE_VERBOSE "Tell the compiler to report all warnings" 0)
if (BE_VERBOSE)
    set(WARNMODE "no-error=")
else()
    set(WARNMODE "no-")
endif()
set(WARNMODE ${WARNMODE} PARENT_SCOPE)

include(CMakeParseArguments)
include(DaemonPlatform)

################################################################################
# Directories
################################################################################

# Dependencies version, this must match the number in external_deps/build.sh
set(DEPS_VERSION 4)

set(MOUNT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(MOUNT_DIR ${MOUNT_DIR} PARENT_SCOPE)
include_directories(${MOUNT_DIR})
set(OBJ_DIR ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY})
set(OBJ_DIR ${OBJ_DIR} PARENT_SCOPE)
set(ENGINE_DIR ${MOUNT_DIR}/engine)
set(ENGINE_DIR ${ENGINE_DIR} PARENT_SCOPE)
set(COMMON_DIR ${MOUNT_DIR}/common)
set(COMMON_DIR ${COMMON_DIR} PARENT_SCOPE)
set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
set(LIB_DIR ${LIB_DIR} PARENT_SCOPE)
include_directories(${LIB_DIR})
if (CMAKE_CFG_INTDIR STREQUAL ".")
    set(FULL_OUTPUT_DIR ${CMAKE_BINARY_DIR})
else()
    set(FULL_OUTPUT_DIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR})
endif()
if (MSVC)
    set(DEPS_EXT ".zip")
    if (ARCH STREQUAL "x86_64")
        set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/msvc64-${DEPS_VERSION})
    else()
        set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/msvc32-${DEPS_VERSION})
    endif()
elseif (WIN32)
    set(DEPS_EXT ".zip")
    if (ARCH STREQUAL "x86_64")
        set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/mingw64-${DEPS_VERSION})
    else()
        set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/mingw32-${DEPS_VERSION})
    endif()
elseif (APPLE)
    set(DEPS_EXT ".tar.bz2")
    if (ARCH STREQUAL "x86_64")
        set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/macosx64-${DEPS_VERSION})
    else()
        set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/macosx32-${DEPS_VERSION})
    endif()
elseif (LINUX)
    set(DEPS_EXT ".tar.bz2")
    # Our minimal NaCl .debs put the files in /usr/lib/nacl, so check that first
    if (EXISTS "/usr/lib/nacl/nacl_loader")
        set(DEPS_DIR "/usr/lib/nacl")
    elseif (ARCH STREQUAL "x86_64")
        set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/linux64-${DEPS_VERSION})
    else()
        set(DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/linux32-${DEPS_VERSION})
    endif()
endif()
set(DEPS_DIR ${DEPS_DIR} PARENT_SCOPE)

################################################################################
# Compile and link flags
################################################################################

include(DaemonFlags)

# Warning options
# keep
try_flag(WARNINGS           "-Wno-pragmas")
try_flag(WARNINGS           "-Wno-unknown-pragmas")
try_flag(WARNINGS           "-W${WARNMODE}old-style-cast")
try_flag(WARNINGS           "-Woverloaded-virtual")
try_flag(WARNINGS           "-Wstrict-null-sentinel")
try_flag(WARNINGS           "-Wno-error=sign-promo")
try_flag(WARNINGS           "-Wno-error=extern-c-compat")
# remove
try_flag(WARNINGS           "-Wno-error=maybe-uninitialized")
try_flag(WARNINGS           "-W${WARNMODE}missing-field-initializers")
try_flag(WARNINGS           "-W${WARNMODE}sign-compare")
try_flag(WARNINGS           "-Wno-error=switch")
try_flag(WARNINGS           "-Wno-error=unused-but-set-parameter")
try_flag(WARNINGS           "-Wno-error=unused-but-set-variable")
try_flag(WARNINGS           "-Wno-error=unused-function")
try_flag(WARNINGS           "-Wno-error=unused-parameter")
try_flag(WARNINGS           "-Wno-error=unused-variable")

################################################################################
# Git revision info
################################################################################

if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
    find_package(Git)
    if (GIT_FOUND)
        execute_process(
            COMMAND ${GIT_EXECUTABLE} describe --tags --long --match v* --dirty=+dirty
            OUTPUT_VARIABLE GIT_DESCRIBE_REPORT
            OUTPUT_STRIP_TRAILING_WHITESPACE
        )
        message(STATUS "git reported ${GIT_DESCRIBE_REPORT}")
        # this may fail with annotated non-release tags
        if (GIT_DESCRIBE_REPORT MATCHES "-0-g.......$")
            set(GIT_DESCRIBE_REPORT)
        endif()
    endif()
endif()

if (GIT_DESCRIBE_REPORT)
    set(DESIRED_REVISION_H_CONTENTS "#define REVISION \"${GIT_DESCRIBE_REPORT}\"\n")
endif()

if (EXISTS "${OBJ_DIR}/revision.h")
    file(READ "${OBJ_DIR}/revision.h" ACTUAL_REVISION_H_CONTENTS)
    if (NOT "${ACTUAL_REVISION_H_CONTENTS}" STREQUAL "${DESIRED_REVISION_H_CONTENTS}")
        file(WRITE "${OBJ_DIR}/revision.h" "${DESIRED_REVISION_H_CONTENTS}")
    endif()
else()
    file(WRITE "${OBJ_DIR}/revision.h" "${DESIRED_REVISION_H_CONTENTS}")
endif()

include_directories("${OBJ_DIR}")

################################################################################
# Group the sources by folder to have folder show in Visual Studio
################################################################################

macro(group_dir_sources dir)
    file(GLOB_RECURSE files ${dir}/*)
    string(LENGTH ${dir}/ dir_length)
    foreach(file ${files})
        if (NOT IS_DIRECTORY ${dir}/${file})
            get_filename_component(group_name ${file} DIRECTORY)
            string(SUBSTRING ${group_name} ${dir_length} -1 group_name)
            string(REPLACE "/" "\\" group_name ${group_name})
            source_group(${group_name} FILES ${file})
        endif()
    endforeach()
endmacro()

if (MSVC)
    group_dir_sources(${MOUNT_DIR})
endif()

################################################################################
# Support for precompiled headers
################################################################################

# MSVC requires that an extra file be added to a project
if (USE_PRECOMPILED_HEADER AND MSVC)
    file(WRITE ${OBJ_DIR}/PrecompiledHeader.cpp "")
    set(PCH_FILE ${OBJ_DIR}/PrecompiledHeader.cpp)
    set(PCH_FILE ${PCH_FILE} PARENT_SCOPE)
endif()

function(ADD_PRECOMPILED_HEADER Target)
    if (NOT USE_PRECOMPILED_HEADER OR CMAKE_VERSION VERSION_LESS 2.8.10)
        return()
    endif()

    # Get the common compile flags
    set(Flags ${CMAKE_CXX_FLAGS})
    get_target_property(Type ${Target} TYPE)
    if (Type STREQUAL MODULE_LIBRARY)
        set(Flags ${Flags} ${CMAKE_SHARED_MODULE_CXX_FLAGS})
    endif()
    separate_arguments(Flags)

    # Get the per-configuration compile flags
    foreach(Config Debug Release RelWithDebInfo MinSizeRel)
        string(TOUPPER ${Config} CONFIG)
        set(ConfigFlags ${CMAKE_CXX_FLAGS_${CONFIG}})
        separate_arguments(ConfigFlags)
        foreach(Flag ${ConfigFlags})
            set(Flags ${Flags} $<$<CONFIG:${Config}>:${Flag}>)
        endforeach()
    endforeach()

    # Get preprocessor options for the target and directory (global)
    get_directory_property(DirCompileDefs COMPILE_DEFINITIONS)
    get_directory_property(DirIncludeDirs INCLUDE_DIRECTORIES)
    get_target_property(TargetCompileDefs ${Target} COMPILE_DEFINITIONS)
    get_target_property(TargetIncludeDirs ${Target} INCLUDE_DIRECTORIES)
    set(Defs)
    foreach(Def ${TargetCompileDefs})
        set(Defs ${Defs} -D${Def})
    endforeach()
    foreach(Def ${DirCompileDefs})
        set(Defs ${Defs} -D${Def})
    endforeach()
    foreach(Def ${DirIncludeDirs})
        set(Defs ${Defs} -I${Def})
    endforeach()
    foreach(Def ${TargetIncludeDirs})
        set(Defs ${Defs} -I${Def})
    endforeach()

    # Specify minimum OSX version
    if (APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
        set(Flags ${Flags} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
    endif()

    set(Header ${COMMON_DIR}/Common.h)

    # Compiler-specific PCH support
    if (CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        add_custom_command(OUTPUT "${OBJ_DIR}/${Target}.h.gch"
            COMMAND ${PNACLPYTHON_PREFIX2} ${CMAKE_CXX_COMPILER} ${Defs} ${Flags} -x c++-header ${Header} -o "${OBJ_DIR}/${Target}.h.gch"
            DEPENDS ${Header}
            IMPLICIT_DEPENDS CXX ${Header}
        )

        add_custom_target(${Target}-pch DEPENDS "${OBJ_DIR}/${Target}.h.gch")
        add_dependencies(${Target} ${Target}-pch)

        # PNaCl clang doesn't support -include-pch properly
        if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" AND NOT NACL)
            set_property(TARGET ${Target} APPEND PROPERTY COMPILE_OPTIONS "-include-pch;${OBJ_DIR}/${Target}.h.gch")
        else()
            set_property(TARGET ${Target} APPEND PROPERTY COMPILE_OPTIONS "-include;${OBJ_DIR}/${Target}.h;-Winvalid-pch")
        endif()
    elseif (MSVC)
        set_source_files_properties(${PCH_FILE} PROPERTIES COMPILE_FLAGS "/Yc${Header}")
        set_property(TARGET ${Target} APPEND PROPERTY COMPILE_FLAGS "/Yu${Header} /Fp${OBJ_DIR}/${Target}.pch /FI${Header}")
    endif()
endfunction()

################################################################################
# File lists
################################################################################

include(srclibs.cmake)
include(src.cmake)

################################################################################
# Libraries
################################################################################

# Import external dependencies
if (DEPS_DIR)
    # Download them if they not available
    if (NOT EXISTS ${DEPS_DIR})
        get_filename_component(BASENAME ${DEPS_DIR} NAME)
        set(REMOTE "http://dl.unvanquished.net/deps/${BASENAME}${DEPS_EXT}")
        message(STATUS "Downloading dependencies from '${REMOTE}'")
        file(DOWNLOAD ${REMOTE} ${OBJ_DIR}/${BASENAME}${DEPS_EXT}
            SHOW_PROGRESS
            STATUS DOWNLOAD_RESULT
            LOG DOWNLOAD_LOG
        )
        list(GET DOWNLOAD_RESULT 0 DOWNLOAD_STATUS)
        list(GET DOWNLOAD_RESULT 1 DOWNLOAD_STRING)
        if (NOT DOWNLOAD_STATUS EQUAL 0)
            message(FATAL_ERROR "Error downloading '${REMOTE}':
                Status code: ${DOWNLOAD_STATUS}
                Error string: ${DOWNLOAD_STRING}
                Download log: ${DOWNLOAD_LOG}"
            )
        endif()
        message(STATUS "Download completed successfully")

        # Extract the downloaded archive
        execute_process(
            COMMAND ${CMAKE_COMMAND} -E tar xzf ${OBJ_DIR}/${BASENAME}${DEPS_EXT}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external_deps
            RESULT_VARIABLE EXTRACT_RESULT
        )
        if (NOT EXTRACT_RESULT EQUAL 0)
            message(FATAL_ERROR "Could not extract ${BASENAME}${DEPS_EXT}")
        endif()
    endif()

    # Add to paths
    set(CMAKE_FIND_ROOT_PATH ${DEPS_DIR} ${CMAKE_FIND_ROOT_PATH})
    set(CMAKE_INCLUDE_PATH ${DEPS_DIR} ${DEPS_DIR}/include ${CMAKE_INCLUDE_PATH})
    set(CMAKE_FRAMEWORK_PATH ${DEPS_DIR} ${CMAKE_FRAMEWORK_PATH})
    set(CMAKE_LIBRARY_PATH ${DEPS_DIR}/lib ${CMAKE_LIBRARY_PATH})
    # Also set parent scope so the top level CMakeLists can find precompiled deps
    set(CMAKE_FIND_ROOT_PATH ${DEPS_DIR} ${CMAKE_FIND_ROOT_PATH} PARENT_SCOPE)
    set(CMAKE_INCLUDE_PATH ${DEPS_DIR} ${DEPS_DIR}/include ${CMAKE_INCLUDE_PATH} PARENT_SCOPE)
    set(CMAKE_FRAMEWORK_PATH ${DEPS_DIR} ${CMAKE_FRAMEWORK_PATH} PARENT_SCOPE)
    set(CMAKE_LIBRARY_PATH ${DEPS_DIR}/lib ${CMAKE_LIBRARY_PATH} PARENT_SCOPE)

    # Force OpenAL Soft on Mac, otherwise the system framework will be selected
    if (APPLE)
        set(OPENAL_INCLUDE_DIR ${DEPS_DIR}/include/AL)
        set(OPENAL_LIBRARY ${DEPS_DIR}/lib/libopenal.dylib)
    endif()
endif()

# These variables are defined:
# LIBS_BASE, LIBS_ENGINE_BASE LIBS_ENGINE, LIBS_BASECLIENT, LIBS_CLIENT

# Native client
include(DaemonNacl)
if (NACL)
    add_library(srclibs-nacl-module EXCLUDE_FROM_ALL ${NACLLIST_MODULE})
    set_target_properties(srclibs-nacl-module PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
    set(LIBS_BASE ${LIBS_BASE} srclibs-nacl-module)
else()
    add_library(srclibs-nacl-native EXCLUDE_FROM_ALL ${NACLLIST_NATIVE})
    set_target_properties(srclibs-nacl-native PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
    set(LIBS_BASE ${LIBS_BASE} srclibs-nacl-native)
endif()

# Base OS libs
if (WIN32)
    set(LIBS_BASE ${LIBS_BASE} winmm ws2_32)
elseif (NACL)
    find_library(NACL_EXCEPTION nacl_exception)
    find_library(NACL_MINIDUMP minidump_generator)
    set(LIBS_BASE ${LIBS_BASE} ${NACL_MINIDUMP} ${NACL_EXCEPTION} )
else()
    find_library(LIBM m)
    if (LIBM)
        set(LIBS_BASE ${LIBS_BASE} ${LIBM})
    endif()
    find_library(LIBRT rt)
    if (LIBRT)
        set(LIBS_BASE ${LIBS_BASE} ${LIBRT})
    endif()
    find_library(LIBICONV iconv)
    if (LIBICONV)
        set(LIBS_ENGINE ${LIBS_ENGINE} ${LIBICONV})
    endif()
    mark_as_advanced(LIBM LIBRT LIBICONV)
    set(LIBS_BASE ${LIBS_BASE} ${CMAKE_DL_LIBS})
    find_package(Threads REQUIRED)
    set(LIBS_BASE ${LIBS_BASE} ${CMAKE_THREAD_LIBS_INIT})
    if (APPLE)
        set(LIBS_CLIENT ${LIBS_CLIENT} "-framework Carbon" "-framework IOKit" "-framework Cocoa")
    endif()
endif()

# Minizip
add_library(srclibs-minizip EXCLUDE_FROM_ALL ${MINIZIPLIST})
set_target_properties(srclibs-minizip PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
set(LIBS_BASE ${LIBS_BASE} srclibs-minizip)

# zlib
if (NOT NACL)
    find_package(ZLIB REQUIRED)
else()
    add_library(srclibs-zlib EXCLUDE_FROM_ALL ${ZLIBLIST})
    set_target_properties(srclibs-zlib PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
    set(ZLIB_INCLUDE_DIRS ${LIB_DIR}/zlib)
    set(ZLIB_LIBRARIES srclibs-zlib)
endif()
set(LIBS_BASE ${LIBS_BASE} ${ZLIB_LIBRARIES})
include_directories(${ZLIB_INCLUDE_DIRS})

set(LIBS_BASE ${LIBS_BASE} PARENT_SCOPE)

# Curses, pdcurses on Windows and ncursesw on Unix
# Not supported on mac because the included version is too old
if (NOT NACL)
    if (WIN32)
        set(LIBS_ENGINE_BASE ${LIBS_ENGINE_BASE} gdi32 comdlg32)

        add_definitions(-DPDC_WIDE -DPDC_FORCE_UTF8 -DUSE_CURSES)
        add_library(srclibs-pdcurses EXCLUDE_FROM_ALL ${PDCURSESLIST})
        set_target_properties(srclibs-pdcurses PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
        set(LIBS_ENGINE_BASE ${LIBS_ENGINE_BASE} srclibs-pdcurses)
        include_directories(${LIB_DIR}/pdcurses)
    elseif (NOT APPLE)
        add_definitions(-DUSE_CURSES)
        set(CURSES_NEED_NCURSES 1) # Tells FindCurses that ncurses is required
        find_package(CursesW REQUIRED)
        set(LIBS_ENGINE_BASE ${LIBS_ENGINE_BASE} ${CURSESW_LIBRARIES})
        include_directories(${CURSESW_INCLUDE_DIR})
    endif()
endif()

if (BUILD_CLIENT OR BUILD_TTY_CLIENT OR BUILD_SERVER)
    if (NACL_RUNTIME_PATH)
        add_definitions("-DNACL_RUNTIME_PATH=${NACL_RUNTIME_PATH}")
    endif()

    # Tinygettext
    add_library(srclibs-tinygettext EXCLUDE_FROM_ALL ${TINYGETTEXTLIST})
    set_target_properties(srclibs-tinygettext PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
    set(LIBS_ENGINE ${LIBS_ENGINE} srclibs-tinygettext)

    # Fastlz
    add_library(srclibs-fastlz EXCLUDE_FROM_ALL ${FASTLZLIST})
    set_target_properties(srclibs-fastlz PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
    set(LIBS_ENGINE ${LIBS_ENGINE} srclibs-fastlz)

    # Detour
    add_library(srclibs-detour EXCLUDE_FROM_ALL ${DETOURLIST})
    set_target_properties(srclibs-detour PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
    set(LIBS_ENGINE ${LIBS_ENGINE} srclibs-detour)

    # GeoIP
    if (USE_GEOIP)
        find_package(GeoIP REQUIRED)
        add_definitions(-DHAVE_GEOIP)
        set(LIBS_ENGINE ${LIBS_ENGINE} ${GeoIP_LIBRARIES})
        include_directories(${GeoIP_INCLUDE_DIRS})
    endif()

    # Nettle
    find_package(Nettle REQUIRED)
    include_directories(${NETTLE_INCLUDE_DIR})
    set(LIBS_ENGINE ${LIBS_ENGINE} ${NETTLE_LIBRARIES})

    # GMP
    find_package(GMP REQUIRED)
    include_directories(${GMP_INCLUDE_DIR})
    set(LIBS_ENGINE ${LIBS_ENGINE} ${GMP_LIBRARIES})
endif()

if (BUILD_CLIENT OR BUILD_TTY_CLIENT)
    # CURL
    find_package(CURL REQUIRED)
    include_directories(${CURL_INCLUDE_DIRS})
    set(LIBS_CLIENTBASE ${LIBS_CLIENTBASE} ${CURL_LIBRARIES})

    # Mumble link
    add_library(srclibs-mumblelink EXCLUDE_FROM_ALL ${MUMBLELINKLIST})
    set_target_properties(srclibs-mumblelink PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
    set(LIBS_CLIENTBASE ${LIBS_CLIENTBASE} srclibs-mumblelink)
endif()

# SDL, required for all targets on win32 because of iconv
if (BUILD_CLIENT OR (WIN32 AND (BUILD_TTY_CLIENT OR BUILD_SERVER)))
    find_package(SDL2 REQUIRED)
    include_directories(${SDL2_INCLUDE_DIR})
    if (WIN32)
        set(LIBS_ENGINE ${LIBS_ENGINE} ${SDL2_LIBRARY})
    else()
        set(LIBS_CLIENT ${LIBS_CLIENT} ${SDL2_LIBRARY})
    endif()

    # Used by tinygettext to use SDL_iconv instead of base iconv
    if (WIN32)
        add_definitions(-DHAVE_SDL)
    endif()
    mark_as_advanced(SDL2MAIN_LIBRARY SDL2_LIBRARY SDL2_INCLUDE_DIR)
endif()

# Breakpad
if (USE_BREAKPAD)
    add_definitions(-DUSE_BREAKPAD)
    include_directories(${LIB_DIR}/breakpad/src)

    if (WIN32)
        add_library(srclibs-breakpad-common EXCLUDE_FROM_ALL ${BREAKPAD_COMMON_LIST})
        add_library(srclibs-breakpad-crash_generation_server EXCLUDE_FROM_ALL ${BREAKPAD_CRASHGENERATIONSERVER_LIST})
        add_library(srclibs-breakpad-exception_handler EXCLUDE_FROM_ALL ${BREAKPAD_EXCEPTIONHANDLER_LIST})
        add_library(srclibs-breakpad-crash_generation_client EXCLUDE_FROM_ALL ${BREAKPAD_CRASHGENERATIONCLIENT_LIST})

        set(BREAKPAD_LIBRARIES
            srclibs-breakpad-common
            srclibs-breakpad-crash_generation_server
            srclibs-breakpad-exception_handler
            srclibs-breakpad-crash_generation_client
        )
        foreach(breaklib ${BREAKPAD_LIBRARIES})
            set_target_properties(${breaklib} PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")

            # Breakpad library assumes wide char versions of Windows API functions
            target_compile_definitions(${breaklib} PRIVATE UNICODE)

            if (CMAKE_BUILD_TYPE MATCHES Debug)
                target_compile_definitions(${breaklib} PRIVATE _DEBUG)
            endif()
        endforeach(breaklib)

        add_executable(crash_server src/engine/crash_server/crash_server_windows.cpp)
        target_link_libraries(crash_server
            srclibs-breakpad-crash_generation_server srclibs-breakpad-common)
        target_compile_definitions(crash_server PRIVATE UNICODE)

        set(LIBS_ENGINE ${LIBS_ENGINE} srclibs-breakpad-exception_handler
            srclibs-breakpad-crash_generation_client srclibs-breakpad-common)

    elseif (LINUX)
        add_library(srclibs-breakpad EXCLUDE_FROM_ALL ${BREAKPAD_LIST})
        set_target_properties(srclibs-breakpad PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
        set(LIBS_ENGINE ${LIBS_ENGINE} srclibs-breakpad)

        add_executable(crash_server src/engine/crash_server/crash_server_linux.cpp)
        target_link_libraries(crash_server srclibs-breakpad)
    endif()
endif()

if (BUILD_CLIENT)
    add_library(srclibs-openexr EXCLUDE_FROM_ALL ${OPENEXRLIST})
    set_target_properties(srclibs-openexr PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
    set(LIBS_CLIENT ${LIBS_CLIENT} srclibs-openexr)

    find_package(Ogg REQUIRED)
    include_directories(${OGG_INCLUDE_DIRS})
    set(LIBS_CLIENT ${LIBS_CLIENT} ${OGG_LIBRARIES})

    find_package(Vorbis REQUIRED)
    include_directories(${VORBIS_INCLUDE_DIRS})
    set(LIBS_CLIENT ${LIBS_CLIENT} ${VORBIS_LIBRARIES})

    find_package(Opus REQUIRED)
    include_directories(${OPUS_INCLUDE_DIRS})
    set(LIBS_CLIENT ${LIBS_CLIENT} ${OPUS_LIBRARIES})

    find_package(Theora REQUIRED)
    include_directories(${THEORA_INCLUDE_DIRS})
    set(LIBS_CLIENT ${LIBS_CLIENT} ${THEORA_LIBRARIES})

    find_package(WebP REQUIRED)
    include_directories(${WEBP_INCLUDE_DIRS})
    set(LIBS_CLIENT ${LIBS_CLIENT} ${WEBP_LIBRARIES})

    find_package(JPEG REQUIRED)
    include_directories(${JPEG_INCLUDE_DIR})
    set(LIBS_CLIENT ${LIBS_CLIENT} ${JPEG_LIBRARIES})

    find_package(PNG REQUIRED)
    include_directories(${PNG_INCLUDE_DIRS})
    set(LIBS_CLIENT ${LIBS_CLIENT} ${PNG_LIBRARIES})

    find_package(Freetype REQUIRED)
    include_directories(${FREETYPE_INCLUDE_DIRS})
    set(LIBS_CLIENT ${LIBS_CLIENT} ${FREETYPE_LIBRARIES})

    find_package(GLEW REQUIRED)
    include_directories(${GLEW_INCLUDE_DIR})
    set(LIBS_CLIENT ${LIBS_CLIENT} ${GLEW_LIBRARIES})

    find_package(OpenGL REQUIRED)
    include_directories(${OPENGL_INCLUDE_DIR})
    set(LIBS_CLIENT ${LIBS_CLIENT} ${OPENGL_LIBRARIES})

    find_package(OpenAL REQUIRED)
    include_directories(${OPENAL_INCLUDE_DIR})
    set(LIBS_CLIENT ${LIBS_CLIENT} ${OPENAL_LIBRARY})
endif()

################################################################################
# Engine
################################################################################
if (NOT NACL)
    add_library(engine-lib ${PCH_FILE} ${COMMONLIST} ${ENGINELIST})
    target_link_libraries(engine-lib ${LIBS_BASE} ${LIBS_ENGINE_BASE})
    set_property(TARGET engine-lib APPEND PROPERTY COMPILE_DEFINITIONS BUILD_ENGINE)
    set_property(TARGET engine-lib APPEND PROPERTY INCLUDE_DIRECTORIES ${ENGINE_DIR} ${MOUNT_DIR} ${LIB_DIR})
    set_property(TARGET engine-lib APPEND PROPERTY COMPILE_OPTIONS ${WARNINGS})
    ADD_PRECOMPILED_HEADER(engine-lib)
endif()

function(AddApplication)
    set(oneValueArgs Target ExecutableName)
    set(multiValueArgs Definitions Flags Files Libs)
    cmake_parse_arguments(A "" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    add_executable(${A_Target} ${A_Files} ${PCH_FILE})
    target_link_libraries(${A_Target} engine-lib ${A_Libs})

    set_property(TARGET ${A_Target} APPEND PROPERTY COMPILE_OPTIONS ${A_Flags})
    set_property(TARGET ${A_Target} APPEND PROPERTY INCLUDE_DIRECTORIES ${ENGINE_DIR} ${MOUNT_DIR} ${LIB_DIR})
    set_property(TARGET ${A_Target} APPEND PROPERTY COMPILE_DEFINITIONS ${A_Definitions})
    set_target_properties(${A_Target} PROPERTIES OUTPUT_NAME "${A_ExecutableName}" PREFIX "" FOLDER "engine")

    message(STATUS ${A_Target})
    ADD_PRECOMPILED_HEADER(${A_Target})
endfunction()

if (BUILD_CLIENT)
    set(Definitions
        BUILD_ENGINE BUILD_CLIENT
    )
    if (USE_SMP)
        list(APPEND Definitions USE_SMP)
    endif()
    AddApplication(
        Target client
        ExecutableName daemon
        Definitions ${Definitions}
        Flags ${WARNINGS}
        Files WIN32 ${WIN_RC} ${QCOMMONLIST} ${SERVERLIST} ${CLIENTBASELIST} ${CLIENTLIST}
        Libs ${LIBS_CLIENT} ${LIBS_CLIENTBASE} ${LIBS_ENGINE}
    )
endif()

if (BUILD_SERVER)
    AddApplication(
        Target server
        ExecutableName daemonded
        Definitions BUILD_ENGINE BUILD_SERVER
        Flags ${WARNINGS}
        Files WIN32 ${WIN_RC} ${QCOMMONLIST} ${SERVERLIST} ${DEDSERVERLIST}
        Libs ${LIBS_ENGINE}
    )
endif()

if (BUILD_TTY_CLIENT)
    AddApplication(
        Target ttyclient
        ExecutableName daemon-tty
        Definitions BUILD_ENGINE BUILD_TTY_CLIENT
        Flags ${WARNINGS}
        Files WIN32 ${WIN_RC} ${QCOMMONLIST} ${SERVERLIST} ${CLIENTBASELIST} ${TTYCLIENTLIST}
        Libs ${LIBS_CLIENTBASE} ${LIBS_ENGINE}
    )
endif()

################################################################################
# Runtime dependencies
################################################################################

if (BUILD_CLIENT OR BUILD_SERVER OR BUILD_TTY_CLIENT)
    add_custom_target(runtime_deps)
    set_target_properties(runtime_deps PROPERTIES FOLDER "CMakePlumbing")

    # The NaCl loader and IRT are required to load .nexe files
    add_custom_command(TARGET runtime_deps PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DEPS_DIR}/sel_ldr${CMAKE_EXECUTABLE_SUFFIX}
            ${FULL_OUTPUT_DIR}/nacl_loader${CMAKE_EXECUTABLE_SUFFIX}
    )
    add_custom_command(TARGET runtime_deps PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${DEPS_DIR}/irt_core-${ARCH}.nexe
            ${FULL_OUTPUT_DIR}/irt_core-${ARCH}.nexe
    )

    # Linux uses a bootstrap program to reserve address space
    if (LINUX)
        add_custom_command(TARGET runtime_deps PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DEPS_DIR}/nacl_helper_bootstrap
                ${FULL_OUTPUT_DIR}/nacl_helper_bootstrap
        )
    endif()

    # Win32 requires nacl_loader64.exe in order to run on Win64
    if (WIN32 AND ARCH STREQUAL x86)
        add_custom_command(TARGET runtime_deps PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DEPS_DIR}/sel_ldr64${CMAKE_EXECUTABLE_SUFFIX}
                ${FULL_OUTPUT_DIR}/nacl_loader64${CMAKE_EXECUTABLE_SUFFIX}
        )
        add_custom_command(TARGET runtime_deps PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${DEPS_DIR}/irt_core-x86_64.nexe
                ${FULL_OUTPUT_DIR}/irt_core-x86_64.nexe
        )
    endif()

    # Mac requires some libraries from external_deps
    if (APPLE)
        add_custom_command(TARGET runtime_deps PRE_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${DEPS_DIR}/SDL2.framework
                ${FULL_OUTPUT_DIR}/SDL2.framework
        )
        file(GLOB RUNTIME_LIBS ${DEPS_DIR}/lib/*.dylib)
        foreach(RUNTIME_LIB ${RUNTIME_LIBS})
            add_custom_command(TARGET runtime_deps PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${RUNTIME_LIB}
                    ${FULL_OUTPUT_DIR}
            )
        endforeach()
    endif()

    # Windows requires some libraries from external_deps
    if (WIN32)
        file(GLOB RUNTIME_LIBS ${DEPS_DIR}/bin/*.dll)
        foreach(RUNTIME_LIB ${RUNTIME_LIBS})
            add_custom_command(TARGET runtime_deps PRE_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${RUNTIME_LIB}
                    ${FULL_OUTPUT_DIR}
            )
        endforeach()
    endif()

    if (BUILD_CLIENT)
        add_dependencies(client runtime_deps)
    endif()
    if (BUILD_SERVER)
        add_dependencies(server runtime_deps)
    endif()
    if (BUILD_TTY_CLIENT)
        add_dependencies(ttyclient runtime_deps)
    endif()
endif()
