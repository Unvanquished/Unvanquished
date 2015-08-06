# Daemon BSD Source Code
# Copyright (c) 2013-2014, Daemon Developers
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

cmake_minimum_required (VERSION 2.8)
if (POLICY CMP0017)
    cmake_policy(SET CMP0017 NEW)
endif()

# Need 2.8.12 for RPATH support
if (APPLE)
    cmake_minimum_required(VERSION 2.8.12)
endif()

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH})
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/cmake PARENT_SCOPE)

include(CMakeDependentOption)

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set(MSVC_INCREMENTAL_DEFAULT ON)
# Enable the creation of project folders for Visual Studio projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Default to Release builds. To use system CFLAGS only (for distro builds), set CMAKE_BUILD_TYPE to None
set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel.")

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

option(BUILD_CLIENT "Build Daemon client" 1)
option(BUILD_SERVER "Build Daemonded server" 1)
option(BUILD_GAME_NATIVE_EXE "Build native executable game logic" 1)
option(BUILD_GAME_NATIVE_DLL "Build native shared library game logic" 1)
if (CMAKE_SYSTEM_NAME STREQUAL CMAKE_HOST_SYSTEM_NAME)
    option(BUILD_GAME_NACL "Build NaCl game logic" 1)
else()
    set(BUILD_GAME_NACL 0)
endif()
option(BUILD_TTY_CLIENT "Build Daemon headless client" 1)

cmake_dependent_option(USE_VOIP "Compile with VOIP support" 0 "BUILD_CLIENT OR BUILD_SERVER OR BUILD_TTY_CLIENT" 0)
cmake_dependent_option(USE_SMP "Compile with support for running the renderer in a separate thread" 1 BUILD_CLIENT 0)
cmake_dependent_option(USE_GEOIP "Use libgeoip" 1 "BUILD_SERVER OR BUILD_CLIENT OR BUILD_TTY_CLIENT" 0)
option(USE_LTO "Use link-time optimization for release builds" 0)
cmake_dependent_option(USE_SLIM_LTO "Generate slim LTO objects, improves build times" 1 "USE_LTO AND ${CMAKE_CXX_COMPILER_ID} STREQUAL GNU" 0)
option(USE_HARDENING "Use stack protection and other hardening flags" 0)
option(USE_WERROR "Tell the compiler to make the build fail when warnings are present" 1)
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

set(NACL_RUNTIME_PATH "" CACHE STRING "Directory containing the NaCl binaries")

include(DaemonPlatform)

################################################################################
# Directories
################################################################################

# Dependencies version, this must match the number in external_deps/build.sh
set(DEPS_VERSION 3)

set(MOUNT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(MOUNT_DIR ${MOUNT_DIR} PARENT_SCOPE)
include_directories(${MOUNT_DIR})
set(OBJ_DIR ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY})
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

if (MSVC)
    set_c_cxx_flag("/MP")
    set_c_cxx_flag("/fp:fast")
    set_c_cxx_flag("/d2Zi+" RELWITHDEBINFO)
    if (ARCH STREQUAL "x86")
        set_c_cxx_flag("/arch:SSE2")
    endif()
    if (USE_LTO)
        set_c_cxx_flag("/GL" MINSIZEREL)
        set_c_cxx_flag("/GL" RELWITHDEBINFO)
        set_c_cxx_flag("/GL" RELEASE)
        set_linker_flag("/LTCG" MINSIZEREL)
        set_linker_flag("/LTCG" RELWITHDEBINFO)
        set_linker_flag("/LTCG" RELEASE)
    endif()
    set_linker_flag("/LARGEADDRESSAWARE")

    try_flag(WARNINGS   "/wd4068")

    # Turn off C4503:, e.g:
    # warning C4503: 'std::_Tree<std::_Tmap_traits<_Kty,_Ty,_Pr,_Alloc,false>>::_Insert_hint' : decorated name length exceeded, name was truncated
    # No issue will be caused from this error as long as no two symbols become identical by being truncated.
    # In practice this rarely happens and even the standard libraries are affected as in the example. So there really is not
    # much that can to done about it and the warnings about each truncation really just make it more likely
    # that other more real issues might get missed. So better to remove the distraction when it really is very unlikey to happen.
    set_c_cxx_flag("/wd4503")

    # Turn off warning C4996:, e.g:
    # warning C4996: 'open': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _open. See online help for details.    set_c_cxx_flag("/wd4996")
    # open seems far more popular than _open not to mention nicer. There doesn't seem to be any reason or will to change to _open.
    # So until there is a specific plan to tackle all of these type of warnings it's best to turn them off to the distraction.
    set_c_cxx_flag("/wd4996")
elseif (NACL)
    set_c_flag("-std=c11")
    set_cxx_flag("-std=gnu++11")

    set_c_cxx_flag("-ffast-math")
    set_c_cxx_flag("-fvisibility=hidden")
    set_c_cxx_flag("-stdlib=libc++")
    set_c_cxx_flag("--pnacl-allow-exceptions")

    set_c_cxx_flag("-O3 -DNDEBUG")
    set_c_cxx_flag("-Os -DNDEBUG"       MinSizeRel)
    set_c_cxx_flag("-O3 -DNDEBUG"       Release)
    set_c_cxx_flag("-O2 -DNDEBUG -g3"   RelWithDebInfo)
    set_c_cxx_flag("-O0          -g3"   Debug)

    set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} PARENT_SCOPE)
    set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} PARENT_SCOPE)
else()
    set_c_cxx_flag("-ffast-math")
    set_c_cxx_flag("-fno-strict-aliasing")

    # Set arch on x86 to SSE2 minimum and enable CMPXCHG16B
    if (ARCH STREQUAL "x86")
        set_c_cxx_flag("-m32")
        set_c_cxx_flag("-msse2")
        set_c_cxx_flag("-mtune=generic")
        try_c_cxx_flag_werror(MFPMATH_SSE "-mfpmath=sse")
    elseif (ARCH STREQUAL "x86_64")
        set_c_cxx_flag("-m64")
        set_c_cxx_flag("-mtune=generic")
        try_c_cxx_flag_werror(MCX16 "-mcx16")
    endif()

    # Use hidden symbol visibility if possible
    try_c_cxx_flag(FVISIBILITY_HIDDEN "-fvisibility=hidden")

    # Extra debug flags
    set_c_cxx_flag("-g3" DEBUG)
    set_c_cxx_flag("-g3" RELWITHDEBINFO)
    if (USE_DEBUG_OPTIMIZE)
        try_c_cxx_flag(OPTIMIZE_DEBUG "-Og" DEBUG)
    endif()

    # C++11 support
    try_cxx_flag(GNUXX11 "-std=gnu++11")
    if (NOT FLAG_GNUXX11)
        try_cxx_flag(GNUXX0X "-std=gnu++0x")
        if (NOT FLAG_GNUXX0X)
            message(FATAL_ERROR "C++11 not supported by compiler")
        endif()
    endif()

    # Use MSVC-compatible bitfield layout
    if (WIN32)
        set_c_cxx_flag("-mms-bitfields")
    endif()

    # Use libc++ on Mac because the shipped libstdc++ version is too old
    if (APPLE)
        set_c_cxx_flag("-stdlib=libc++")
        set_linker_flag("-stdlib=libc++")
    endif()

    # Hardening, don't set _FORTIFY_SOURCE in debug builds
    set_c_cxx_flag("-D_FORTIFY_SOURCE=2" RELEASE)
    set_c_cxx_flag("-D_FORTIFY_SOURCE=2" RELWITHDEBINFO)
    set_c_cxx_flag("-D_FORTIFY_SOURCE=2" MINSIZEREL)
    if (USE_HARDENING)
        try_c_cxx_flag(FSTACK_PROTECTOR_STRONG "-fstack-protector-strong")
        if (NOT FLAG_FSTACK_PROTECTOR_STRONG)
            try_c_cxx_flag(FSTACK_PROTECTOR_ALL "-fstack-protector-all")
        endif()
        try_c_cxx_flag(FNO_STRICT_OVERFLOW "-fno-strict-overflow")
        try_c_cxx_flag(WSTACK_PROTECTOR "-Wstack-protector")
        try_c_cxx_flag(FPIE "-fPIE")
        try_linker_flag(LINKER_PIE "-pie")
    endif()

    # Linker flags
    if (NOT APPLE)
        try_linker_flag(LINKER_O1 "-Wl,-O1")
        try_linker_flag(LINKER_SORT_COMMON "-Wl,--sort-common")
        try_linker_flag(LINKER_AS_NEEDED "-Wl,--as-needed")
        if (NOT USE_ADDRESS_SANITIZER)
            try_linker_flag(LINKER_NO_UNDEFINED "-Wl,--no-undefined")
        endif()
        try_linker_flag(LINKER_Z_RELRO "-Wl,-z,relro")
        try_linker_flag(LINKER_Z_NOW "-Wl,-z,now")
    endif()
    if (WIN32)
        try_linker_flag(LINKER_DYNAMICBASE "-Wl,--dynamicbase")
        try_linker_flag(LINKER_NXCOMPAT "-Wl,--nxcompat")
        try_linker_flag(LINKER_LARGE_ADDRESS_AWARE "-Wl,--large-address-aware")
    endif()

    # The -pthread flag sets some preprocessor defines,
    # it is also used to link with libpthread on Linux
    if (NOT APPLE)
        try_c_cxx_flag(PTHREAD "-pthread")
    endif()
    if (LINUX)
        set_linker_flag("-pthread")
    endif()

    # Warning options
    try_flag(WARNINGS           "-Wall")
    try_flag(WARNINGS           "-Wextra")
    if (USE_PEDANTIC)
        try_flag(WARNINGS       "-pedantic")
    endif()
    if (USE_WERROR)
        try_flag(WARNINGS       "-Werror")
        if (USE_PEDANTIC)
            try_flag(WARNINGS   "-pedantic-errors")
        endif()
    endif()
    # keep
    try_flag(WARNINGS           "-Wno-pragmas")
    try_flag(WARNINGS           "-Wno-unknown-pragmas")
    try_flag(WARNINGS           "-W${WARNMODE}old-style-cast")
    try_flag(WARNINGS           "-Woverloaded-virtual")
    try_flag(WARNINGS           "-Wstrict-null-sentinel")
    try_flag(WARNINGS           "-Wno-error=sign-promo")
    # remove
    try_flag(WARNINGS           "-W${WARNMODE}missing-field-initializers")
    try_flag(WARNINGS           "-W${WARNMODE}sign-compare")
    try_flag(WARNINGS           "-Wno-error=switch")
    try_flag(WARNINGS           "-Wno-error=unused-but-set-parameter")
    try_flag(WARNINGS           "-Wno-error=unused-but-set-variable")
    try_flag(WARNINGS           "-Wno-error=unused-function")
    try_flag(WARNINGS           "-Wno-error=unused-parameter")
    try_flag(WARNINGS           "-Wno-error=unused-variable")

    if (USE_ADDRESS_SANITIZER)
        set_cxx_flag("-fsanitize=address")
        set_linker_flag("-fsanitize=address")
    endif()

    # Link-time optimization
    if (USE_LTO)
        set_c_cxx_flag("-flto")
        set_linker_flag("-flto")

        # For LTO compilation we must send a copy of all compile flags to the linker
        set_linker_flag("${CMAKE_CXX_FLAGS}")

        # Use gcc-ar and gcc-ranlib instead of ar and ranlib so that we can use
        # slim LTO objects. This requires a recent version of GCC and binutils.
        if (${CMAKE_CXX_COMPILER_ID} STREQUAL GNU)
            if (USE_SLIM_LTO)
                string(REGEX MATCH "^([0-9]+.[0-9]+)" _version "${CMAKE_CXX_COMPILER_VERSION}")
                get_filename_component(COMPILER_BASENAME "${CMAKE_C_COMPILER}" NAME)
                if (COMPILER_BASENAME MATCHES "^(.+-)g?cc(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
                    set(TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
                endif()

                find_program(GCC_AR
                    NAMES
                        "${TOOLCHAIN_PREFIX}gcc-ar"
                        "${TOOLCHAIN_PREFIX}gcc-ar-${_version}"
                    DOC "gcc provided wrapper for ar which adds the --plugin option"
                )

                find_program(GCC_RANLIB
                    NAMES
                        "${TOOLCHAIN_PREFIX}gcc-ranlib"
                        "${TOOLCHAIN_PREFIX}gcc-ranlib-${_version}"
                    DOC "gcc provided wrapper for ranlib which adds the --plugin option"
                )

                mark_as_advanced(GCC_AR GCC_RANLIB)

                # Override standard ar and ranlib with the gcc- versions
                if (GCC_AR)
                    set(CMAKE_AR ${GCC_AR})
                endif()
                if (GCC_RANLIB)
                    set(CMAKE_RANLIB ${GCC_RANLIB})
                endif()

                try_c_cxx_flag(NO_FAT_LTO_OBJECTS "-fno-fat-lto-objects")
            else()
                try_c_cxx_flag(FAT_LTO_OBJECTS "-ffat-lto-objects")
            endif()
        endif()
    endif()

endif()

# Windows-specific definitions
if (WIN32)
    add_definitions(
        -DWINVER=0x501  # Minimum Windows version: XP
        -DWIN32         # Define WIN32 for compatibility (compiler defines _WIN32)
        -DNOMINMAX      # Define NOMINMAX to prevent conflics between std::min/max and the min/max macros in WinDef.h
        -DSTRICT        # Enable STRICT type checking for windows.h
    )
    set(CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "" "lib")
endif()
if (MSVC)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()

# Mac-specific definitions
if (APPLE)
    add_definitions(-DMACOS_X)
    set_linker_flag("-Wl,-no_pie")
    set(CMAKE_INSTALL_RPATH "@executable_path/;${DEPS_DIR};${DEPS_DIR}/lib")
    set(CMAKE_BUILD_WITH_INSTALL_RPATH ON)
endif()

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

    # Specify minimum OSX version
    if (APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET)
        set(Flags ${Flags} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET})
    endif()

    set(Header ${COMMON_DIR}/Common.h)

    # Compiler-specific PCH support
    if (CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
        add_custom_command(OUTPUT "${OBJ_DIR}/${Target}.h.gch"
            COMMAND ${CMAKE_CXX_COMPILER} ${Defs} ${Flags} -x c++-header ${Header} -o "${OBJ_DIR}/${Target}.h.gch"
            DEPENDS ${Header}
            IMPLICIT_DEPENDS CXX ${Header}
        )

        add_custom_target(${Target}-pch DEPENDS "${OBJ_DIR}/${Target}.h.gch")
        add_dependencies(${Target} ${Target}-pch)

        if (${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
            set_target_properties(${Target} PROPERTIES COMPILE_FLAGS "-include-pch ${OBJ_DIR}/${Target}.h.gch")
        else()
            set_target_properties(${Target} PROPERTIES COMPILE_FLAGS "-include ${OBJ_DIR}/${Target}.h -Winvalid-pch")
        endif()
    elseif (MSVC)
        set_source_files_properties(${PCH_FILE} PROPERTIES COMPILE_FLAGS "/Yc${Header}")
        set_target_properties(${Target} PROPERTIES COMPILE_FLAGS "/Yu${Header} /Fp${OBJ_DIR}/${Target}.pch /FI${Header}")
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

    # Force OpenAL Soft on Mac, otherwise the system framework will be selected
    if (APPLE)
        set(OPENAL_INCLUDE_DIR ${DEPS_DIR}/include/AL)
        set(OPENAL_LIBRARY ${DEPS_DIR}/lib/libopenal.dylib)
    endif()
endif()

# These variables are defined:
# LIBS_BASE, LIBS_ENGINE, LIBS_BASECLIENT, LIBS_CLIENT

# Base OS libs
if (WIN32)
    set(LIBS_BASE ${LIBS_BASE} winmm ws2_32)
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
    if (NOT NACL)
        set(LIBS_BASE ${LIBS_BASE} ${CMAKE_DL_LIBS})
    endif()
    if (NACL)
        find_library(NACL_EXCEPTION nacl_exception)
        set(LIBS_BASE ${LIBS_BASE} ${NACL_EXCEPTION})
    endif()
    if (NOT NACL)
        find_package(Threads REQUIRED)
        set(LIBS_BASE ${LIBS_BASE} ${CMAKE_THREAD_LIBS_INIT})
    endif()
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

if (NOT NACL)
    # Native client
    include(DaemonNacl)
    add_library(nacl-source-libs EXCLUDE_FROM_ALL ${NACLLIST_NATIVE})
    set_target_properties(nacl-source-libs PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
    set(LIBS_ENGINE ${LIBS_ENGINE} nacl-source-libs)

    if (NACL_RUNTIME_PATH)
        add_definitions("-DNACL_RUNTIME_PATH=${NACL_RUNTIME_PATH}")
    endif()

    # Curses, pdcurses on Windows and ncursesw on Unix
    # Not supported on mac because the included version is too old
    if (WIN32)
        set(LIBS_ENGINE ${LIBS_ENGINE} gdi32 comdlg32)

        add_definitions(-DPDC_WIDE -DPDC_FORCE_UTF8 -DUSE_CURSES)
        add_library(srclibs-pdcurses EXCLUDE_FROM_ALL ${PDCURSESLIST})
        set_target_properties(srclibs-pdcurses PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
        set(LIBS_ENGINE ${LIBS_ENGINE} srclibs-pdcurses)
        include_directories(${LIB_DIR}/pdcurses)
    elseif (NOT APPLE)
        add_definitions(-DUSE_CURSES)
        set(CURSES_NEED_NCURSES 1) # Tells FindCurses that ncurses is required
        find_package(CursesW REQUIRED)
        set(LIBS_ENGINE ${LIBS_ENGINE} ${CURSESW_LIBRARIES})
        include_directories(${CURSESW_INCLUDE_DIR})
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

if (BUILD_CLIENT)
    add_library(srclibs-openexr EXCLUDE_FROM_ALL ${OPENEXRLIST})
    set_target_properties(srclibs-openexr PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs")
    set(LIBS_CLIENT ${LIBS_CLIENT} srclibs-openexr)

    if (USE_VOIP)
        find_package(Speex REQUIRED)
        include_directories(${SPEEX_INCLUDE_DIR})
        set(LIBS_CLIENT ${LIBS_CLIENT} ${SPEEX_LIBRARIES})
    endif()

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

if (BUILD_CLIENT)
    add_executable(client WIN32 ${WIN_RC} ${PCH_FILE} ${COMMONLIST} ${ENGINELIST} ${SERVERLIST} ${CLIENTBASELIST} ${CLIENTLIST})
    if (USE_SMP)
        set_property(TARGET client APPEND PROPERTY COMPILE_DEFINITIONS SMP)
    endif()
    target_link_libraries(client ${LIBS_CLIENT} ${LIBS_CLIENTBASE} ${LIBS_ENGINE} ${LIBS_BASE})
    set_property(TARGET client APPEND PROPERTY COMPILE_DEFINITIONS BUILD_ENGINE BUILD_CLIENT)
    if (USE_VOIP)
        # Set it here so the tty client doesn't need to depend on speex
        set_property(TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_VOIP)
    endif()
    set_target_properties(client PROPERTIES OUTPUT_NAME "daemon" PREFIX "" FOLDER "engine")
    set_property(TARGET client APPEND PROPERTY INCLUDE_DIRECTORIES ${ENGINE_DIR})
    set_property(TARGET client APPEND PROPERTY COMPILE_FLAGS ${WARNINGS})
    ADD_PRECOMPILED_HEADER(client)
endif()

if (BUILD_SERVER)
    add_executable(server WIN32 ${WIN_RC} ${PCH_FILE} ${COMMONLIST} ${ENGINELIST} ${SERVERLIST} ${DEDSERVERLIST})
    target_link_libraries(server ${LIBS_ENGINE} ${LIBS_BASE})
    set_property(TARGET server APPEND PROPERTY COMPILE_DEFINITIONS BUILD_ENGINE BUILD_SERVER)
    set_target_properties(server PROPERTIES OUTPUT_NAME "daemonded" PREFIX "" FOLDER "engine")
    set_property(TARGET server APPEND PROPERTY INCLUDE_DIRECTORIES ${ENGINE_DIR})
    set_property(TARGET server APPEND PROPERTY COMPILE_FLAGS ${WARNINGS})
    ADD_PRECOMPILED_HEADER(server)
endif()

if (BUILD_TTY_CLIENT)
    add_executable(ttyclient WIN32 ${WIN_RC} ${PCH_FILE} ${COMMONLIST} ${ENGINELIST} ${SERVERLIST} ${CLIENTBASELIST} ${TTYCLIENTLIST})
    target_link_libraries(ttyclient ${LIBS_CLIENTBASE} ${LIBS_ENGINE} ${LIBS_BASE})
    set_property(TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS BUILD_ENGINE BUILD_TTY_CLIENT)
    set_target_properties(ttyclient PROPERTIES OUTPUT_NAME "daemon-tty" PREFIX "" FOLDER "engine")
    set_property(TARGET ttyclient APPEND PROPERTY INCLUDE_DIRECTORIES ${ENGINE_DIR})
    set_property(TARGET ttyclient APPEND PROPERTY COMPILE_FLAGS ${WARNINGS})
    ADD_PRECOMPILED_HEADER(ttyclient)
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
