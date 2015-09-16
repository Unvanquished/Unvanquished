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

cmake_minimum_required(VERSION 2.8)
project(Unvanquished C CXX)

set(Daemon_OUT ${CMAKE_CURRENT_BINARY_DIR})
add_subdirectory(daemon daemon_build)
include(DaemonGame)

set(LIB_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs)
include_directories(${LIB_DIR})

set(GAMELOGIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
include_directories(${GAMELOGIC_DIR})
include(src.cmake)


option(BUILD_CGAME "Build client-side gamelogic" 1)
option(BUILD_SGAME "Build server-side gamelogic" 1)

if (BUILD_CGAME AND (BUILD_GAME_NATIVE_DLL OR BUILD_GAME_NATIVE_EXE OR NACL))
    if (NACL)
        set(FREETYPE_LIBRARY ${DEPS_DIR}/pnacl_deps/lib/libfreetype.a)
        set(FREETYPE_LIBRARIES ${FREETYPE_LIBRARY} ${DEPS_DIR}/pnacl_deps/lib/libpng16.a)
        set(FREETYPE_INCLUDE_DIRS ${DEPS_DIR}/pnacl_deps/include/freetype2)
        set(LUA_INCLUDE_DIR ${DEPS_DIR}/pnacl_deps/include)
        set(LUA_LIBRARY ${DEPS_DIR}/pnacl_deps/lib/liblua.a)
    else()
        find_package(Freetype REQUIRED)
        find_package(Lua REQUIRED)
    endif()
    include_directories(${FREETYPE_INCLUDE_DIRS} ${LUA_INCLUDE_DIR})
    add_subdirectory(${LIB_DIR}/libRocket)
    set_target_properties(ROCKET_LIB PROPERTIES FOLDER "libs")
    include_directories(${ROCKET_INCLUDE_DIRS})
endif()

# Warning options
# keep
try_flag(WARNINGS           "/wd4068")
try_flag(WARNINGS           "-Wno-pragmas")
try_flag(WARNINGS           "-Wno-unknown-pragmas")
try_flag(WARNINGS           "-W${WARNMODE}old-style-cast")
try_flag(WARNINGS           "-Woverloaded-virtual")
try_flag(WARNINGS           "-Wstrict-null-sentinel")
try_flag(WARNINGS           "-Wno-error=sign-promo")

if (NOT BE_VERBOSE)
    try_flag(WARNINGS       "-Wno-error=strict-overflow")
endif()

# remove
try_flag(WARNINGS           "-Wno-error=maybe-uninitialized")
try_flag(WARNINGS           "-W${WARNMODE}missing-field-initializers")
try_flag(WARNINGS           "-W${WARNMODE}sign-compare")
try_flag(WARNINGS           "-Wno-error=switch")
try_flag(WARNINGS           "-Wno-error=unused-but-set-variable")
try_flag(WARNINGS           "-Wno-error=unused-function")
try_flag(WARNINGS           "-W${WARNMODE}unused-parameter")
try_flag(WARNINGS           "-Wno-error=unused-private-field")

if (BUILD_SGAME)
    GAMEMODULE(NAME sgame
        DEFINITIONS
            BUILD_SGAME
        FLAGS
            ${WARNINGS}
        FILES
            ${SGAMELIST}
    )
endif()

if (BUILD_CGAME)
  GAMEMODULE(NAME cgame
    DEFINITIONS
        BUILD_CGAME
        ROCKET_STATIC_LIB
    FLAGS
        ${WARNINGS}
    FILES
        ${CGAMELIST}
    LIBS
        ROCKET_LIB
        ${FREETYPE_LIBRARIES}
        ${LUA_LIBRARY}
  )
endif()
