# Unvanquished BSD Source Code
# Copyright (c) 2013-2014, Unvanquished Developers
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

cmake_minimum_required(VERSION 3.12)
project(Unvanquished C CXX)

# On Mac, default to x86_64 unless user requests something else
if (NOT CMAKE_OSX_ARCHITECTURES)
    set(CMAKE_OSX_ARCHITECTURES x86_64)
endif()

# When we build in the source dir, the daemon executable cannot be create
# as there is already a daemon directory, so we disallow building in the
# source dir.
if(CMAKE_SOURCE_DIR STREQUAL CMAKE_BINARY_DIR)
    message(FATAL_ERROR "Building in the source directory is not supported, try building in a 'build' directory. You may have to delete the CMakeCache.txt file and CMakeFiles directory that are next to the CMakeLists.txt.")
endif()

set(SUBMODULE_LIST
  "libs/RmlUi"
)

set(DAEMON_DIR_STRING "Path to the Daemon Engine source.")
if(NOT DAEMON_DIR)
  set(DAEMON_DIR "${CMAKE_CURRENT_SOURCE_DIR}/daemon" CACHE STRING "${DAEMON_DIR_STRING}")
  LIST(APPEND SUBMODULE_LIST "daemon")
else()
  set(DAEMON_DIR "${DAEMON_DIR}" CACHE STRING "${DAEMON_DIR_STRING}")
endif()

foreach(SUBMODULE_PATH ${SUBMODULE_LIST})
  # Check if we need to initialize submodule
  file(GLOB DENG_RESULT "${CMAKE_CURRENT_SOURCE_DIR}/${SUBMODULE_PATH}")
  list(LENGTH DENG_RESULT DENG_RES_LEN)
  if(DENG_RES_LEN EQUAL 0)
    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
      find_package(Git REQUIRED)
      if(GIT_FOUND)
        execute_process(
         COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive -- "${SUBMODULE_PATH}"
         WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
      endif()
    endif()
  endif()
endforeach()

set(Daemon_OUT ${CMAKE_CURRENT_BINARY_DIR})
add_subdirectory("${DAEMON_DIR}" daemon_build)
include(DaemonGame)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
include_directories(${LIB_DIR})

set(GAMELOGIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${GAMELOGIC_DIR})
include(src.cmake)


option(BUILD_CGAME "Build client-side gamelogic" 1)
option(BUILD_SGAME "Build server-side gamelogic" 1)

# Whether we are building any gamelogic with the current toolchain
if ((BUILD_GAME_NATIVE_DLL OR BUILD_GAME_NATIVE_EXE OR NACL) AND (BUILD_CGAME OR BUILD_SGAME))
    set(BUILDING_ANY_GAMELOGIC 1)
endif()

#########################################
# Externally built library dependencies #
#########################################
if (NACL)
    set(FREETYPE_LIBRARY ${DEPS_DIR}/pnacl_deps/lib/libfreetype.a)
    set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY} ${DEPS_DIR}/pnacl_deps/lib/libpng16.a)
    set(FREETYPE_INCLUDE_DIRS ${DEPS_DIR}/pnacl_deps/include/freetype2)
    set(LUA_INCLUDE_DIR ${DEPS_DIR}/pnacl_deps/include)
    set(LUA_LIBRARY ${DEPS_DIR}/pnacl_deps/lib/liblua.a)
elseif (BUILDING_ANY_GAMELOGIC)
    find_package(Lua REQUIRED)

    if (BUILD_CGAME)
        # Freetype (RmlUi dependency)
        find_package(Freetype REQUIRED)
        if (APPLE OR WIN32)
            find_package(ZLIB REQUIRED)
            set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARIES} ${ZLIB_LIBRARIES})
        endif()
    endif()
endif()
include_directories(${LUA_INCLUDE_DIR})

##################################
# Libraries we build from source #
##################################

if (BUILDING_ANY_GAMELOGIC AND BUILD_CGAME)
    # RmlUi
    include_directories(${FREETYPE_INCLUDE_DIRS})
    include(${CMAKE_CURRENT_SOURCE_DIR}/rmlui.cmake)
    set_target_properties(RMLUI_LIB PROPERTIES FOLDER "libs")
    include_directories(${RMLUI_INCLUDE_DIRS})
endif()

# Configuration for Recast
set(RC_MAX_LAYERS 63 CACHE STRING "Must be 255 or smaller.")
set(RC_MAX_NEIS 16 CACHE STRING "")
# This does not set the cache variable but rather a normal variable that shadows it.
set(NACL_VM_INHERITED_OPTIONS ${NACL_VM_INHERITED_OPTIONS} RC_MAX_LAYERS RC_MAX_NEIS)

if (BUILDING_ANY_GAMELOGIC)
    # Fastlz
    add_library(srclibs-fastlz EXCLUDE_FROM_ALL ${FASTLZLIST})
    set_target_properties(srclibs-fastlz PROPERTIES POSITION_INDEPENDENT_CODE ${GAME_PIE} FOLDER "libs")

    # Detour
    add_library(srclibs-detour EXCLUDE_FROM_ALL ${DETOURLIST})
    set_target_properties(srclibs-detour PROPERTIES POSITION_INDEPENDENT_CODE ${GAME_PIE} FOLDER "libs")
    include_directories(${LIB_DIR}/recastnavigation/Detour/Include
                        ${LIB_DIR}/recastnavigation/DetourTileCache/Include
                        ${LIB_DIR}/recastnavigation/DebugUtils/Include
                        ${LIB_DIR}/recastnavigation/DetourCrowd/Include)

    # Recast
    if (RC_MAX_LAYERS GREATER 255)
        message(FATAL_ERROR "RC_MAX_LAYERS must be 255 or smaller")
    endif()

    add_definitions(-DRC_MAX_LAYERS_DEF=${RC_MAX_LAYERS})
    add_definitions(-DRC_MAX_NEIS_DEF=${RC_MAX_NEIS})

    add_library(srclibs-recast EXCLUDE_FROM_ALL ${RECASTLIST})
    set_target_properties(srclibs-recast PROPERTIES POSITION_INDEPENDENT_CODE ${GAME_PIE} FOLDER "libs")
    include_directories(
        ${LIB_DIR}/recastnavigation/Recast/Include
        ${LIB_DIR}/recastnavigation/RecastDemo/Include)

    # GLM (header-only)
    include_directories(${LIB_DIR}/glm)
    add_definitions(-DGLM_FORCE_EXPLICIT_CTOR)

    # tinygettext
    add_library(srclibs-tinygettext EXCLUDE_FROM_ALL ${TINYGETTEXTLIST})
    set_target_properties(srclibs-tinygettext PROPERTIES POSITION_INDEPENDENT_CODE ${GAME_PIE} FOLDER "libs")
    target_include_directories(srclibs-tinygettext PUBLIC ${LIB_DIR}/tinygettext/include)
    target_compile_definitions(srclibs-tinygettext PUBLIC TINYGETTEXT_UTF8_ONLY)
endif()

################################################
# Warning options for Unvanquished source code #
################################################

if (BUILDING_ANY_GAMELOGIC)
    # keep
    try_flag(WARNINGS           "-Wno-pragmas")
    try_flag(WARNINGS           "-Wno-unknown-pragmas")
    try_flag(WARNINGS           "-W${WARNMODE}old-style-cast")
    try_flag(WARNINGS           "-Woverloaded-virtual")
    try_flag(WARNINGS           "-Wstrict-null-sentinel")

    # Since OSX uses newer compilers, they complain about some of header only libs.
    # So, ignore deprecation warnings on OSX.
    if (APPLE)
        # Do not error for GLM vsprintf deprecation warnings.
        try_flag(WARNINGS "-Wno-error=deprecated-declarations")
    endif()

    # remove
    try_flag(WARNINGS           "-Wno-maybe-uninitialized")
    try_flag(WARNINGS "/wd4127")  # conditional expression is constant
    try_flag(WARNINGS "/wd4458")  # declaration of 'XXX' hides class member
    try_flag(WARNINGS "/wd4459")  # declaration of 'XXX' hides global declaration
    try_flag(WARNINGS "/wd4701")  # potentially uninitialized local variable 'XXX' used
    try_flag(WARNINGS "/wd26812") # The enum 'XXX' is unscoped. Prefer 'enum class' over 'enum'
endif()

if (NACL)
    # https://stackoverflow.com/a/48413842
    set(WARNINGS ${WARNINGS} "-Wno-missing-braces")
endif()

#####################
# Gamelogic modules #
#####################

if (BUILD_SGAME)
    include(tools/cbse/CBSE.cmake)

    # Avoid generating CBSE in the NaCl subproject.
    if (FORK EQUAL 2)
        CBSEFileList("${GAMELOGIC_DIR}/sgame/" sgame_GENERATED_CBSE)
    else()
        CBSE("sgame"
             "${GAMELOGIC_DIR}/sgame/entities.yaml"
             "${GAMELOGIC_DIR}/sgame/")
    endif()

    GAMEMODULE(NAME sgame
        DEFINITIONS
            BUILD_SGAME
        FLAGS
            ${WARNINGS}
        FILES
            ${SGAMELIST}
            ${sgame_GENERATED_CBSE}
        LIBS
            ${LUA_LIBRARY}
            srclibs-detour
            srclibs-recast
            srclibs-fastlz
    )

    if (BUILD_GAME_NACL AND NOT (FORK EQUAL 2))
        include(ExternalProject)
        foreach(NACL_VMS_PROJECT ${NACL_VMS_PROJECTS})
            ExternalProject_Add_Step(${NACL_VMS_PROJECT} cbse-trigger
                COMMENT "Finished CBSE generation, ready to build NaCl ${NACL_VMS_PROJECT}"
                DEPENDS ${sgame_GENERATED_CBSE}
                DEPENDERS build)
        endforeach()
    endif()
endif()

if (BUILD_CGAME)
  GAMEMODULE(NAME cgame
    DEFINITIONS
        BUILD_CGAME
        RMLUI_STATIC_LIB
    FLAGS
        ${WARNINGS}
    FILES
        ${CGAMELIST}
    LIBS
        RMLUI_LIB
        ${FREETYPE_LIBRARIES}
        ${LUA_LIBRARY}
        srclibs-tinygettext
        srclibs-detour
        srclibs-recast
        srclibs-fastlz
  )
endif()
