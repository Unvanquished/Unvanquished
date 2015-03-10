# Daemon BSD Source Code
# Copyright (c) 2013-2014, Daemon Developers
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in the
#      documentation and/or other materials provided with the distribution.
#    * Neither the name of the <organization> nor the
#      names of its contributors may be used to endorse or promote products
#      derived from this software without specific prior written permission.
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

cmake_minimum_required( VERSION 2.8 )
if( POLICY CMP0017 )
  cmake_policy( SET CMP0017 NEW )
endif()

# Need 2.8.12 for RPATH support
if( APPLE )
  cmake_minimum_required( VERSION 2.8.12 )
endif()

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/daemon/cmake ${CMAKE_MODULE_PATH} )
set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH} )

include( CMakeDependentOption )

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set( MSVC_INCREMENTAL_DEFAULT ON )

# Default to Release builds. To use system CFLAGS only (for distro builds), set CMAKE_BUILD_TYPE to None
set( CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." )

project( Unvanquished C CXX )

add_subdirectory( daemon )

################################################################################
# Configuration options
################################################################################

option( BUILD_GAME_NATIVE_EXE "Build native executable game logic" 1 )
option( BUILD_GAME_NATIVE_DLL "Build native shared library game logic" 1 )
option( BUILD_GAME_NACL "Build NaCl game logic" 1 )

option( USE_LTO "Use link-time optimization for release builds" 0 )
cmake_dependent_option( USE_SLIM_LTO "Generate slim LTO objects, improves build times" 1 "USE_LTO AND ${CMAKE_CXX_COMPILER_ID} STREQUAL GNU" 0 )
option( USE_HARDENING "Use stack protection and other hardening flags" 0 )
option( USE_WEXTRA "Enable extra compilation warnings" 0 )
option( USE_PEDANTIC "Tell the compiler to be pedantic" 0 )
option( USE_DEBUG_OPTIMIZE "Try to optimize the debug build" 1 )
option( USE_PRECOMPILED_HEADER "Improve build times by using a precompiled header" 1 )
option( USE_ADDRESS_SANITIZER "Try to use the address sanitizer" 0 )

set( NACL_RUNTIME_PATH "" CACHE STRING "Directory containing the NaCl binaries" )

################################################################################
# Determine platform + architecture
################################################################################

include( DaemonUtils )

################################################################################
# Directories
################################################################################

set( MOUNT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src )
set( OBJ_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY} )
set( ENGINE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/daemon/src/engine )
set( COMMON_DIR ${CMAKE_CURRENT_SOURCE_DIR}/daemon/src/common )
set( GAMELOGIC_DIR ${MOUNT_DIR}/gamelogic )
set( LIB_DIR ${MOUNT_DIR}/libs )

include( DaemonExternals )

################################################################################
# Compile and link flags
################################################################################

include( DaemonFlags )

################################################################################
# Determine python version and set env variable for NaCl
################################################################################

set( PNACL_TOOLCHAIN ${DEPS_DIR}/pnacl/bin )
if( WIN32 )
  set( PNACL_BIN_EXT ".bat" )
else()
  set( PNACL_BIN_EXT "" )
endif()

if( BUILD_GAME_NACL AND NOT CMAKE_HOST_WIN32 )
  set( PythonInterp_FIND_VERSION 2 )
  find_package( PythonInterp 2 )
  if( NOT PYTHONINTERP_FOUND )
    message( "Please set the PNACLPYTHON environment variable to your Python2 executable")
  endif()
  set( PNACLPYTHON_PREFIX env "PNACLPYTHON=${PYTHON_EXECUTABLE}" )
endif()

################################################################################
# Git revision info
################################################################################

include( DaemonGitRev )

################################################################################
# Support for precompiled headers
################################################################################

include( DaemonPCH )

################################################################################
# File lists
################################################################################

set( SHAREDLIST
  ${ENGINE_DIR}/qcommon/q_shared.cpp
  ${ENGINE_DIR}/qcommon/q_unicode.cpp
  ${ENGINE_DIR}/qcommon/q_math.cpp
  ${ENGINE_DIR}/qcommon/q_shared.h
  ${ENGINE_DIR}/qcommon/q_unicode.h
  ${ENGINE_DIR}/qcommon/unicode_data.h
  ${ENGINE_DIR}/qcommon/logging.h
)

set( COMMONLIST
  ${COMMON_DIR}/Command.cpp
  ${COMMON_DIR}/Command.h
  ${COMMON_DIR}/Cvar.cpp
  ${COMMON_DIR}/Cvar.h
  ${COMMON_DIR}/DisjointSets.h
  ${COMMON_DIR}/FileSystem.cpp
  ${COMMON_DIR}/FileSystem.h
  ${COMMON_DIR}/LineEditData.cpp
  ${COMMON_DIR}/LineEditData.h
  ${COMMON_DIR}/Log.cpp
  ${COMMON_DIR}/Log.h
  ${COMMON_DIR}/String.cpp
  ${COMMON_DIR}/String.h
  ${COMMON_DIR}/System.cpp
  ${COMMON_DIR}/System.h
  ${COMMON_DIR}/Serialize.h
  ${COMMON_DIR}/cm/cm_load.cpp
  ${COMMON_DIR}/cm/cm_trisoup.cpp
  ${COMMON_DIR}/cm/cm_patch.cpp
  ${COMMON_DIR}/cm/cm_plane.cpp
  ${COMMON_DIR}/cm/cm_polylib.cpp
  ${COMMON_DIR}/cm/cm_test.cpp
  ${COMMON_DIR}/cm/cm_trace.cpp
  ${COMMON_DIR}/cm/cm_local.h
  ${COMMON_DIR}/cm/cm_patch.h
  ${COMMON_DIR}/cm/cm_polylib.h
  ${COMMON_DIR}/cm/cm_public.h
  ${COMMON_DIR}/IPC/Channel.h
  ${COMMON_DIR}/IPC/CommonSyscalls.h
  ${COMMON_DIR}/IPC/CommandBuffer.cpp
  ${COMMON_DIR}/IPC/CommandBuffer.h
  ${COMMON_DIR}/IPC/Primitives.cpp
  ${COMMON_DIR}/IPC/Primitives.h
)

set( GAMESHAREDLIST
  ${GAMELOGIC_DIR}/shared/bg_misc.cpp
  ${GAMELOGIC_DIR}/shared/bg_parse.cpp
  ${GAMELOGIC_DIR}/shared/bg_pmove.cpp
  ${GAMELOGIC_DIR}/shared/bg_slidemove.cpp
  ${GAMELOGIC_DIR}/shared/bg_alloc.cpp
  ${GAMELOGIC_DIR}/shared/bg_voice.cpp
  ${GAMELOGIC_DIR}/shared/bg_teamprogress.cpp
  ${GAMELOGIC_DIR}/shared/bg_utilities.cpp
  ${GAMELOGIC_DIR}/shared/CommandBufferClient.cpp
  ${GAMELOGIC_DIR}/shared/CommandBufferClient.h
  ${GAMELOGIC_DIR}/shared/CommonProxies.cpp
  ${GAMELOGIC_DIR}/shared/CommonProxies.h
  ${GAMELOGIC_DIR}/shared/VMMain.cpp
  ${GAMELOGIC_DIR}/shared/VMMain.h
)

set( CGAMELIST
  ${GAMELOGIC_DIR}/cgame/cg_api.cpp
  ${GAMELOGIC_DIR}/cgame/cg_consolecmds.cpp
  ${GAMELOGIC_DIR}/cgame/cg_buildable.cpp
  ${GAMELOGIC_DIR}/cgame/cg_animation.cpp
  ${GAMELOGIC_DIR}/cgame/cg_animmapobj.cpp
  ${GAMELOGIC_DIR}/cgame/cg_draw.cpp
  ${GAMELOGIC_DIR}/cgame/cg_drawtools.cpp
  ${GAMELOGIC_DIR}/cgame/cg_ents.cpp
  ${GAMELOGIC_DIR}/cgame/cg_event.cpp
  ${GAMELOGIC_DIR}/cgame/cg_main.cpp
  ${GAMELOGIC_DIR}/cgame/cg_marks.cpp
  ${GAMELOGIC_DIR}/cgame/cg_minimap.cpp
  ${GAMELOGIC_DIR}/cgame/cg_players.cpp
  ${GAMELOGIC_DIR}/cgame/cg_playerstate.cpp
  ${GAMELOGIC_DIR}/cgame/cg_predict.cpp
  ${GAMELOGIC_DIR}/cgame/cg_servercmds.cpp
  ${GAMELOGIC_DIR}/cgame/cg_snapshot.cpp
  ${GAMELOGIC_DIR}/cgame/cg_view.cpp
  ${GAMELOGIC_DIR}/cgame/cg_weapons.cpp
  ${GAMELOGIC_DIR}/cgame/cg_scanner.cpp
  ${GAMELOGIC_DIR}/cgame/cg_attachment.cpp
  ${GAMELOGIC_DIR}/cgame/cg_trails.cpp
  ${GAMELOGIC_DIR}/cgame/cg_particles.cpp
  ${GAMELOGIC_DIR}/cgame/cg_tutorial.cpp
  ${GAMELOGIC_DIR}/cgame/cg_utils.cpp
  ${GAMELOGIC_DIR}/cgame/cg_beacon.cpp
  ${GAMELOGIC_DIR}/cgame/cg_rocket.cpp
  ${GAMELOGIC_DIR}/cgame/cg_rocket_draw.cpp
  ${GAMELOGIC_DIR}/cgame/cg_rocket_events.cpp
  ${GAMELOGIC_DIR}/cgame/cg_rocket_datasource.cpp
  ${GAMELOGIC_DIR}/cgame/cg_rocket_progressbar.cpp
  ${GAMELOGIC_DIR}/cgame/cg_rocket_dataformatter.cpp
  ${GAMELOGIC_DIR}/cgame/cg_gameinfo.cpp
  ${GAMELOGIC_DIR}/cgame/cg_parseutils.cpp
  ${ENGINE_DIR}/client/cg_api.h
  ${ENGINE_DIR}/client/cg_msgdef.h
  ${ENGINE_DIR}/qcommon/print_translated.h
  ${GAMESHAREDLIST}
  ${COMMONLIST}
)

set( SGAMELIST
  ${GAMELOGIC_DIR}/sgame/sg_active.cpp
  ${GAMELOGIC_DIR}/sgame/sg_api.cpp
  ${GAMELOGIC_DIR}/sgame/sg_buildpoints.cpp
  ${GAMELOGIC_DIR}/sgame/sg_client.cpp
  ${GAMELOGIC_DIR}/sgame/sg_cmds.cpp
  ${GAMELOGIC_DIR}/sgame/sg_combat.cpp
  ${GAMELOGIC_DIR}/sgame/sg_momentum.cpp
  ${GAMELOGIC_DIR}/sgame/sg_entities.cpp
  ${GAMELOGIC_DIR}/sgame/sg_physics.cpp
  ${GAMELOGIC_DIR}/sgame/sg_buildable.cpp
  ${GAMELOGIC_DIR}/sgame/sg_main.cpp
  ${GAMELOGIC_DIR}/sgame/sg_missile.cpp
  ${GAMELOGIC_DIR}/sgame/sg_session.cpp
  ${GAMELOGIC_DIR}/sgame/sg_spawn_afx.cpp
  ${GAMELOGIC_DIR}/sgame/sg_spawn_ctrl.cpp
  ${GAMELOGIC_DIR}/sgame/sg_spawn_fx.cpp
  ${GAMELOGIC_DIR}/sgame/sg_spawn_game.cpp
  ${GAMELOGIC_DIR}/sgame/sg_spawn_generic.cpp
  ${GAMELOGIC_DIR}/sgame/sg_spawn_gfx.cpp
  ${GAMELOGIC_DIR}/sgame/sg_spawn_mover.cpp
  ${GAMELOGIC_DIR}/sgame/sg_spawn_position.cpp
  ${GAMELOGIC_DIR}/sgame/sg_spawn_sensor.cpp
  ${GAMELOGIC_DIR}/sgame/sg_spawn_shared.cpp
  ${GAMELOGIC_DIR}/sgame/sg_spawn.cpp
  ${GAMELOGIC_DIR}/sgame/sg_svcmds.cpp
  ${GAMELOGIC_DIR}/sgame/sg_team.cpp
  ${GAMELOGIC_DIR}/sgame/sg_utils.cpp
  ${GAMELOGIC_DIR}/sgame/sg_maprotation.cpp
  ${GAMELOGIC_DIR}/sgame/sg_weapon.cpp
  ${GAMELOGIC_DIR}/sgame/sg_admin.cpp
  ${GAMELOGIC_DIR}/sgame/sg_namelog.cpp
  ${GAMELOGIC_DIR}/sgame/sg_bot.cpp
  ${GAMELOGIC_DIR}/sgame/sg_bot_ai.cpp
  ${GAMELOGIC_DIR}/sgame/sg_bot_nav.cpp
  ${GAMELOGIC_DIR}/sgame/sg_bot_parse.cpp
  ${GAMELOGIC_DIR}/sgame/sg_bot_util.cpp
  ${GAMELOGIC_DIR}/sgame/sg_cm_world.cpp
  ${GAMELOGIC_DIR}/sgame/sg_cm_world.h
  ${GAMELOGIC_DIR}/sgame/Beacon.cpp
  ${GAMELOGIC_DIR}/sgame/Clustering.cpp
  ${ENGINE_DIR}/server/sg_api.h
  ${ENGINE_DIR}/server/sg_msgdef.h
  ${GAMESHAREDLIST}
  ${COMMONLIST}
)

set( MINIZIPLIST
  ${LIB_DIR}/minizip/ioapi.c
  ${LIB_DIR}/minizip/ioapi.h
  ${LIB_DIR}/minizip/unzip.c
  ${LIB_DIR}/minizip/unzip.h
)

# This is only used for the NaCl modules. For all other targets, the host zlib
# is used (sourced from external_deps if not available on target platform).
set( ZLIBLIST
  ${LIB_DIR}/zlib/adler32.c
  ${LIB_DIR}/zlib/compress.c
  ${LIB_DIR}/zlib/crc32.c
  ${LIB_DIR}/zlib/crc32.h
  ${LIB_DIR}/zlib/deflate.c
  ${LIB_DIR}/zlib/deflate.h
  ${LIB_DIR}/zlib/gzclose.c
  ${LIB_DIR}/zlib/gzguts.h
  ${LIB_DIR}/zlib/gzlib.c
  ${LIB_DIR}/zlib/gzread.c
  ${LIB_DIR}/zlib/gzwrite.c
  ${LIB_DIR}/zlib/infback.c
  ${LIB_DIR}/zlib/inffast.c
  ${LIB_DIR}/zlib/inffast.h
  ${LIB_DIR}/zlib/inffixed.h
  ${LIB_DIR}/zlib/inflate.c
  ${LIB_DIR}/zlib/inflate.h
  ${LIB_DIR}/zlib/inftrees.c
  ${LIB_DIR}/zlib/inftrees.h
  ${LIB_DIR}/zlib/trees.c
  ${LIB_DIR}/zlib/trees.h
  ${LIB_DIR}/zlib/uncompr.c
  ${LIB_DIR}/zlib/zconf.h
  ${LIB_DIR}/zlib/zlib.h
  ${LIB_DIR}/zlib/zutil.c
  ${LIB_DIR}/zlib/zutil.h
)

if( APPLE )
  set( NACLLIST_NATIVE
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/posix/nacl_imc_posix.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/osx/nacl_imc.cc
  )
elseif( LINUX )
  set( NACLLIST_NATIVE
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/posix/nacl_imc_posix.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/linux/nacl_imc.cc
  )
elseif( WIN32 )
  set( NACLLIST_NATIVE
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/win/nacl_imc.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/win/nacl_shm.cc
  )
endif()

set( NACLLIST_MODULE
  ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
  ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl/nacl_imc.cc
  ${LIB_DIR}/nacl/native_client/src/untrusted/nacl/imc_accept.c
  ${LIB_DIR}/nacl/native_client/src/untrusted/nacl/imc_connect.c
  ${LIB_DIR}/nacl/native_client/src/untrusted/nacl/imc_makeboundsock.c
  ${LIB_DIR}/nacl/native_client/src/untrusted/nacl/imc_mem_obj_create.c
  ${LIB_DIR}/nacl/native_client/src/untrusted/nacl/imc_recvmsg.c
  ${LIB_DIR}/nacl/native_client/src/untrusted/nacl/imc_sendmsg.c
  ${LIB_DIR}/nacl/native_client/src/untrusted/nacl/imc_socketpair.c
)

################################################################################
# Libraries
################################################################################

include( DaemonLibs )

# Base OS libs
if( WIN32 )
  set( LIBS_BASE ${LIBS_BASE} winmm ws2_32 )
else()
  find_library( LIBM m )
  if( LIBM )
    set( LIBS_BASE ${LIBS_BASE} ${LIBM} )
  endif()
  find_library( LIBRT rt )
  if( LIBRT )
    set( LIBS_BASE ${LIBS_BASE} ${LIBRT} )
  endif()
  mark_as_advanced( LIBM LIBRT )
  set( LIBS_BASE ${LIBS_BASE} ${CMAKE_DL_LIBS} )
  find_package( Threads REQUIRED )
  set( LIBS_BASE ${LIBS_BASE} ${CMAKE_THREAD_LIBS_INIT} )
endif()

# zlib
find_package( ZLIB REQUIRED )
include_directories( ${ZLIB_INCLUDE_DIRS} )
set( LIBS_BASE ${LIBS_BASE} ${ZLIB_LIBRARIES} )

# Minizip -- added by daemon
#add_library( minizip EXCLUDE_FROM_ALL ${MINIZIPLIST} )
#set_target_properties( minizip PROPERTIES POSITION_INDEPENDENT_CODE 1 )
set( LIBS_BASE minizip ${LIBS_BASE} )

################################################################################
# Game logic
################################################################################

include( DaemonGame )

GAMEMODULE( NAME sgame
  COMPILE_DEF BUILD_VM BUILD_SGAME
  FILES ${SGAMELIST}
)

GAMEMODULE( NAME cgame
  COMPILE_DEF BUILD_VM BUILD_CGAME
  FILES ${CGAMELIST}
)
