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

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/daemon/cmake ${CMAKE_MODULE_PATH} )

include( CMakeDependentOption )

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set( MSVC_INCREMENTAL_DEFAULT ON )
# Enable the creation of project folders for Visual Studio projects
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

# Default to Release builds. To use system CFLAGS only (for distro builds), set CMAKE_BUILD_TYPE to None
set( CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." )

project( Unvanquished C CXX )

set( DAEMON_OUT ${CMAKE_CURRENT_BINARY_DIR} )
add_subdirectory( daemon daemon_build )

################################################################################
# Directories
################################################################################

set( OBJ_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY} )
set( GAMELOGIC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src )

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

set( GAMESHAREDLIST
  ${GAMELOGIC_DIR}/shared/bg_gameplay.h
  ${GAMELOGIC_DIR}/shared/bg_local.h
  ${GAMELOGIC_DIR}/shared/bg_misc.cpp
  ${GAMELOGIC_DIR}/shared/bg_parse.cpp
  ${GAMELOGIC_DIR}/shared/bg_pmove.cpp
  ${GAMELOGIC_DIR}/shared/bg_public.h
  ${GAMELOGIC_DIR}/shared/bg_slidemove.cpp
  ${GAMELOGIC_DIR}/shared/bg_alloc.cpp
  ${GAMELOGIC_DIR}/shared/bg_voice.cpp
  ${GAMELOGIC_DIR}/shared/bg_teamprogress.cpp
  ${GAMELOGIC_DIR}/shared/bg_utilities.cpp
)

set( CGAMELIST
  ${GAMESHAREDLIST}
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
  ${GAMELOGIC_DIR}/cgame/cg_attachment.cpp
  ${GAMELOGIC_DIR}/cgame/cg_trails.cpp
  ${GAMELOGIC_DIR}/cgame/cg_particles.cpp
  ${GAMELOGIC_DIR}/cgame/cg_tutorial.cpp
  ${GAMELOGIC_DIR}/cgame/cg_ui.h
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
  ${GAMELOGIC_DIR}/cgame/Filter.h
  ${Daemon_SOURCE_DIR}/src/engine/client/cg_api.h
  ${Daemon_SOURCE_DIR}/src/engine/client/cg_msgdef.h
  ${Daemon_SOURCE_DIR}/src/engine/qcommon/print_translated.h
)

set( SGAMELIST
  ${GAMESHAREDLIST}
  ${GAMELOGIC_DIR}/sgame/sg_active.cpp
  ${GAMELOGIC_DIR}/sgame/sg_api.cpp
  ${GAMELOGIC_DIR}/sgame/sg_buildpoints.cpp
  ${GAMELOGIC_DIR}/sgame/sg_client.cpp
  ${GAMELOGIC_DIR}/sgame/sg_cmds.cpp
  ${GAMELOGIC_DIR}/sgame/sg_combat.cpp
  ${GAMELOGIC_DIR}/sgame/sg_definitions.h
  ${GAMELOGIC_DIR}/sgame/sg_momentum.cpp
  ${GAMELOGIC_DIR}/sgame/sg_entities.cpp
  ${GAMELOGIC_DIR}/sgame/sg_entities.h
  ${GAMELOGIC_DIR}/sgame/sg_extern.h
  ${GAMELOGIC_DIR}/sgame/sg_local.h
  ${GAMELOGIC_DIR}/sgame/sg_public.h
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
  ${GAMELOGIC_DIR}/sgame/sg_spawn.h
  ${GAMELOGIC_DIR}/sgame/sg_struct.h
  ${GAMELOGIC_DIR}/sgame/sg_svcmds.cpp
  ${GAMELOGIC_DIR}/sgame/sg_team.cpp
  ${GAMELOGIC_DIR}/sgame/sg_trapcalls.h
  ${GAMELOGIC_DIR}/sgame/sg_typedef.h
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
  ${Daemon_SOURCE_DIR}/src/engine/server/sg_api.h
  ${Daemon_SOURCE_DIR}/src/engine/server/sg_msgdef.h
)

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
