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

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH} )

include( CMakeDependentOption )

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set( MSVC_INCREMENTAL_DEFAULT ON )

# Default to Release builds. To use system CFLAGS only (for distro builds), set CMAKE_BUILD_TYPE to None
set( CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." )

if( DAEMON_OUT )
  set( CMAKE_CURRENT_BINARY_DIR ${DAEMON_OUT} )
  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} )
endif()

project( Daemon C CXX )

################################################################################
# Configuration options
################################################################################

option( BUILD_CLIENT "Build Daemon client" 1 )
option( BUILD_SERVER "Build Daemonded server" 1 )
option( BUILD_TTY_CLIENT "Build Daemon headless client" 1 )

cmake_dependent_option( USE_VOIP "Compile with VOIP support" 0 "BUILD_CLIENT OR BUILD_SERVER OR BUILD_TTY_CLIENT" 0 )
cmake_dependent_option( USE_SMP "Compile with support for running the renderer in a separate thread" 1 BUILD_CLIENT 0 )
cmake_dependent_option( USE_GEOIP "Use libgeoip" 1 "BUILD_SERVER OR BUILD_CLIENT OR BUILD_TTY_CLIENT" 0 )
cmake_dependent_option( USE_CON_TTY "Use tty console output" 1 "NOT WIN32; BUILD_SERVER OR BUILD_CLIENT OR BUILD_TTY_CLIENT" 0 )

set( NACL_RUNTIME_PATH "" CACHE STRING "Directory containing the NaCl binaries" )

################################################################################
# Determine platform + architecture
################################################################################

include( DaemonUtils )

################################################################################
# Directories
################################################################################

set( MOUNT_DIR ${Daemon_SOURCE_DIR}/src )
set( OBJ_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY} )
set( COMMON_DIR ${MOUNT_DIR}/common )
set( ENGINE_DIR ${MOUNT_DIR}/engine )
set( GAMELOGIC_DIR ${MOUNT_DIR}/gamelogic )
set( LIB_DIR ${MOUNT_DIR}/libs )

include( DaemonExternals )

################################################################################
# Compile and link flags
################################################################################

include( DaemonFlags )

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

# daemon = [client, ttyclient, server]
# gamelogic = [cgame, sgame]

# Used by daemon and gamelogic
set( SHAREDLIST
  ${ENGINE_DIR}/qcommon/q_shared.cpp
  ${ENGINE_DIR}/qcommon/q_unicode.cpp
  ${ENGINE_DIR}/qcommon/q_math.cpp
  ${ENGINE_DIR}/qcommon/q_shared.h
  ${ENGINE_DIR}/qcommon/q_unicode.h
  ${ENGINE_DIR}/qcommon/unicode_data.h
  ${ENGINE_DIR}/qcommon/logging.h
)

# Used by daemon and gamelogic
set( COMMONLIST
  ${COMMON_DIR}/Command.cpp
  ${COMMON_DIR}/Common.h
  ${COMMON_DIR}/Command.h
  ${COMMON_DIR}/Compiler.h
  ${COMMON_DIR}/Cvar.cpp
  ${COMMON_DIR}/Cvar.h
  ${COMMON_DIR}/DisjointSets.h
  ${COMMON_DIR}/Endian.h
  ${COMMON_DIR}/FileSystem.cpp
  ${COMMON_DIR}/FileSystem.h
  ${COMMON_DIR}/LineEditData.cpp
  ${COMMON_DIR}/LineEditData.h
  ${COMMON_DIR}/Log.cpp
  ${COMMON_DIR}/Log.h
  ${COMMON_DIR}/Math.h
  ${COMMON_DIR}/Optional.h
  ${COMMON_DIR}/Platform.h
  ${COMMON_DIR}/String.cpp
  ${COMMON_DIR}/String.h
  ${COMMON_DIR}/System.cpp
  ${COMMON_DIR}/System.h
  ${COMMON_DIR}/Serialize.h
  ${COMMON_DIR}/Util.h
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
  ${COMMON_DIR}/IPC/CommandBuffer.cpp
  ${COMMON_DIR}/IPC/CommandBuffer.h
  ${COMMON_DIR}/IPC/Common.h
  ${COMMON_DIR}/IPC/CommonSyscalls.h
  ${COMMON_DIR}/IPC/Primitives.cpp
  ${COMMON_DIR}/IPC/Primitives.h
  ${COMMON_DIR}/math/Vector.h
)

# Used by gamelogic
set( PROVIDEDLIST
  ${COMMONLIST}
  ${SHAREDLIST}
  ${GAMELOGIC_DIR}/shared/CommandBufferClient.cpp
  ${GAMELOGIC_DIR}/shared/CommandBufferClient.h
  ${GAMELOGIC_DIR}/shared/CommonProxies.cpp
  ${GAMELOGIC_DIR}/shared/CommonProxies.h
  ${GAMELOGIC_DIR}/shared/VMMain.cpp
  ${GAMELOGIC_DIR}/shared/VMMain.h
)
set( PROVIDEDLIST ${PROVIDEDLIST} PARENT_SCOPE )

# Used by client
set( RENDERERLIST
  ${ENGINE_DIR}/sys/sdl_icon.h
  ${ENGINE_DIR}/sys/sdl_glimp.cpp
  ${ENGINE_DIR}/renderer/gl_shader.cpp
  ${ENGINE_DIR}/renderer/gl_shader.h
  ${ENGINE_DIR}/renderer/iqm.h
  ${ENGINE_DIR}/renderer/shaders.cpp
  ${ENGINE_DIR}/renderer/tr_animation.cpp
  ${ENGINE_DIR}/renderer/tr_backend.cpp
  ${ENGINE_DIR}/renderer/tr_bsp.cpp
  ${ENGINE_DIR}/renderer/tr_cmds.cpp
  ${ENGINE_DIR}/renderer/tr_curve.cpp
  ${ENGINE_DIR}/renderer/tr_decals.cpp
  ${ENGINE_DIR}/renderer/tr_fbo.cpp
  ${ENGINE_DIR}/renderer/tr_flares.cpp
  ${ENGINE_DIR}/renderer/tr_font.cpp
  ${ENGINE_DIR}/renderer/tr_image_crn.cpp
  ${ENGINE_DIR}/renderer/tr_image_dds.cpp
  ${ENGINE_DIR}/renderer/tr_image_exr.cpp
  ${ENGINE_DIR}/renderer/tr_image_jpg.cpp
  ${ENGINE_DIR}/renderer/tr_image_ktx.cpp
  ${ENGINE_DIR}/renderer/tr_image_png.cpp
  ${ENGINE_DIR}/renderer/tr_image_tga.cpp
  ${ENGINE_DIR}/renderer/tr_image_webp.cpp
  ${ENGINE_DIR}/renderer/tr_image.cpp
  ${ENGINE_DIR}/renderer/tr_image.h
  ${ENGINE_DIR}/renderer/tr_init.cpp
  ${ENGINE_DIR}/renderer/tr_light.cpp
  ${ENGINE_DIR}/renderer/tr_local.h
  ${ENGINE_DIR}/renderer/tr_main.cpp
  ${ENGINE_DIR}/renderer/tr_marks.cpp
  ${ENGINE_DIR}/renderer/tr_mesh.cpp
  ${ENGINE_DIR}/renderer/tr_model_iqm.cpp
  ${ENGINE_DIR}/renderer/tr_model_md3.cpp
  ${ENGINE_DIR}/renderer/tr_model_md5.cpp
  ${ENGINE_DIR}/renderer/tr_model_skel.cpp
  ${ENGINE_DIR}/renderer/tr_model_skel.h
  ${ENGINE_DIR}/renderer/tr_model.cpp
  ${ENGINE_DIR}/renderer/tr_noise.cpp
  ${ENGINE_DIR}/renderer/tr_public.h
  ${ENGINE_DIR}/renderer/tr_scene.cpp
  ${ENGINE_DIR}/renderer/tr_shade_calc.cpp
  ${ENGINE_DIR}/renderer/tr_shade.cpp
  ${ENGINE_DIR}/renderer/tr_shader.cpp
  ${ENGINE_DIR}/renderer/tr_skin.cpp
  ${ENGINE_DIR}/renderer/tr_sky.cpp
  ${ENGINE_DIR}/renderer/tr_surface.cpp
  ${ENGINE_DIR}/renderer/tr_types.h
  ${ENGINE_DIR}/renderer/tr_vbo.cpp
  ${ENGINE_DIR}/renderer/tr_world.cpp
  ${LIB_DIR}/openexr/half.cpp
)

# Used by daemon
set( SERVERLIST
  ${ENGINE_DIR}/botlib/bot_api.h
  ${ENGINE_DIR}/botlib/bot_convert.cpp
  ${ENGINE_DIR}/botlib/bot_load.cpp
  ${ENGINE_DIR}/botlib/bot_local.cpp
  ${ENGINE_DIR}/botlib/bot_nav.cpp
  ${ENGINE_DIR}/botlib/bot_navdraw.h
  ${ENGINE_DIR}/botlib/bot_types.h
  ${ENGINE_DIR}/botlib/nav.h
  ${ENGINE_DIR}/server/sv_bot.cpp
  ${ENGINE_DIR}/server/sv_ccmds.cpp
  ${ENGINE_DIR}/server/sv_client.cpp
  ${ENGINE_DIR}/server/sv_sgame.cpp
  ${ENGINE_DIR}/server/sv_init.cpp
  ${ENGINE_DIR}/server/sv_main.cpp
  ${ENGINE_DIR}/server/sv_net_chan.cpp
  ${ENGINE_DIR}/server/sv_snapshot.cpp
  ${ENGINE_DIR}/server/server.h
  ${ENGINE_DIR}/server/sg_api.h
  ${ENGINE_DIR}/server/sg_msgdef.h
  ${ENGINE_DIR}/qcommon/print_translated.h
)

# Used by daemon
set( ENGINELIST
  ${COMMONLIST}
  ${SHAREDLIST}
  ${ENGINE_DIR}/framework/BaseCommands.cpp
  ${ENGINE_DIR}/framework/BaseCommands.h
  ${ENGINE_DIR}/framework/CommandSystem.cpp
  ${ENGINE_DIR}/framework/CommandSystem.h
  ${ENGINE_DIR}/framework/CvarSystem.cpp
  ${ENGINE_DIR}/framework/CvarSystem.h
  ${ENGINE_DIR}/framework/CommandBufferHost.cpp
  ${ENGINE_DIR}/framework/CommandBufferHost.h
  ${ENGINE_DIR}/framework/CommonVMServices.cpp
  ${ENGINE_DIR}/framework/CommonVMServices.h
  ${ENGINE_DIR}/framework/ConsoleField.cpp
  ${ENGINE_DIR}/framework/ConsoleField.h
  ${ENGINE_DIR}/framework/ConsoleHistory.cpp
  ${ENGINE_DIR}/framework/ConsoleHistory.h
  ${ENGINE_DIR}/framework/LogSystem.cpp
  ${ENGINE_DIR}/framework/LogSystem.h
  ${ENGINE_DIR}/framework/Resource.cpp
  ${ENGINE_DIR}/framework/Resource.h
  ${ENGINE_DIR}/framework/System.cpp
  ${ENGINE_DIR}/framework/System.h
  ${ENGINE_DIR}/framework/VirtualMachine.cpp
  ${ENGINE_DIR}/framework/VirtualMachine.h
  ${ENGINE_DIR}/qcommon/cmd.cpp
  ${ENGINE_DIR}/qcommon/common.cpp
  ${ENGINE_DIR}/qcommon/crypto.cpp
  ${ENGINE_DIR}/qcommon/crypto.h
  ${ENGINE_DIR}/qcommon/cvar.cpp
  ${ENGINE_DIR}/qcommon/cvar.h
  ${ENGINE_DIR}/qcommon/files.cpp
  ${ENGINE_DIR}/qcommon/qfiles.h
  ${ENGINE_DIR}/qcommon/huffman.cpp
  ${ENGINE_DIR}/qcommon/md5.cpp
  ${ENGINE_DIR}/qcommon/msg.cpp
  ${ENGINE_DIR}/qcommon/net_chan.cpp
  ${ENGINE_DIR}/qcommon/net_ip.cpp
  ${ENGINE_DIR}/qcommon/parse.cpp
  ${ENGINE_DIR}/qcommon/translation.cpp
  ${ENGINE_DIR}/sys/con_log.cpp
  ${ENGINE_DIR}/qcommon/qcommon.h
  ${ENGINE_DIR}/qcommon/surfaceflags.h
)

if( USE_CON_TTY )
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/con_tty.cpp
  )
else()
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/con_passive.cpp
  )
endif()

if( NOT APPLE )
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/con_curses.cpp
  )
endif()

# Used by client and ttyclient
set( CLIENTBASELIST
  ${ENGINELIST}
  ${SERVERLIST}
  ${ENGINE_DIR}/botlib/bot_debug.cpp
  ${ENGINE_DIR}/botlib/bot_nav_edit.cpp
  ${ENGINE_DIR}/client/cg_api.h
  ${ENGINE_DIR}/client/cg_msgdef.h
  ${ENGINE_DIR}/client/client.h
  ${ENGINE_DIR}/client/cin_ogm.cpp
  ${ENGINE_DIR}/client/cl_avi.cpp
  ${ENGINE_DIR}/client/cl_cgame.cpp
  ${ENGINE_DIR}/client/cl_cin.cpp
  ${ENGINE_DIR}/client/cl_console.cpp
  ${ENGINE_DIR}/client/cl_input.cpp
  ${ENGINE_DIR}/client/cl_irc.cpp
  ${ENGINE_DIR}/client/cl_keys.cpp
  ${ENGINE_DIR}/client/keycodes.h
  ${ENGINE_DIR}/client/keys.h
  ${ENGINE_DIR}/client/cl_main.cpp
  ${ENGINE_DIR}/client/cl_parse.cpp
  ${ENGINE_DIR}/client/cl_scrn.cpp
  ${ENGINE_DIR}/client/dl_main.cpp
  ${ENGINE_DIR}/client/libmumblelink.cpp
  ${ENGINE_DIR}/client/libmumblelink.h
)

# Used by client
set( CLIENTLIST
  ${CLIENTBASELIST}
  ${RENDERERLIST}
  ${ENGINE_DIR}/audio/ALObjects.cpp
  ${ENGINE_DIR}/audio/ALObjects.h
  ${ENGINE_DIR}/audio/Audio.cpp
  ${ENGINE_DIR}/audio/Audio.h
  ${ENGINE_DIR}/audio/AudioData.h
  ${ENGINE_DIR}/audio/AudioPrivate.h
  ${ENGINE_DIR}/audio/Emitter.cpp
  ${ENGINE_DIR}/audio/Emitter.h
  ${ENGINE_DIR}/audio/Sample.cpp
  ${ENGINE_DIR}/audio/Sample.h
  ${ENGINE_DIR}/audio/Sound.cpp
  ${ENGINE_DIR}/audio/Sound.h
  ${ENGINE_DIR}/audio/SoundCodec.h
  ${ENGINE_DIR}/audio/SoundCodec.cpp
  ${ENGINE_DIR}/audio/OggCodec.cpp
  ${ENGINE_DIR}/audio/OpusCodec.cpp
  ${ENGINE_DIR}/audio/WavCodec.cpp
  ${ENGINE_DIR}/client/rocket_dataformatter.cpp
  ${ENGINE_DIR}/client/rocket_textelements.cpp
  ${ENGINE_DIR}/client/rocket_documents.cpp
  ${ENGINE_DIR}/client/rocket_datagrid.cpp
  ${ENGINE_DIR}/client/rocket_element.cpp
  ${ENGINE_DIR}/client/rocket_events.cpp
  ${ENGINE_DIR}/client/rocket_keys.cpp
  ${ENGINE_DIR}/client/rocket_hud.cpp
  ${ENGINE_DIR}/client/rocket.cpp

  ${ENGINE_DIR}/client/rocketSelectableDataGrid.h
  ${ENGINE_DIR}/client/rocketConditionalElement.h
  ${ENGINE_DIR}/client/rocketConsoleTextElement.h
  ${ENGINE_DIR}/client/rocketFormControlSelect.h
  ${ENGINE_DIR}/client/rocketCvarInlineElement.h
  ${ENGINE_DIR}/client/rocketDataSourceSingle.h
  ${ENGINE_DIR}/client/rocketFormControlInput.h
  ${ENGINE_DIR}/client/rocketElementDocument.h
  ${ENGINE_DIR}/client/rocketEventInstancer.h
  ${ENGINE_DIR}/client/rocketIncludeElement.h
  ${ENGINE_DIR}/client/rocketDataFormatter.h
  ${ENGINE_DIR}/client/rocketFocusManager.h
  ${ENGINE_DIR}/client/rocketProgressBar.h
  ${ENGINE_DIR}/client/rocketDataSelect.h
  ${ENGINE_DIR}/client/rocketDataSource.h
  ${ENGINE_DIR}/client/rocketCircleMenu.h
  ${ENGINE_DIR}/client/rocketColorInput.h
  ${ENGINE_DIR}/client/rocketChatField.h
  ${ENGINE_DIR}/client/rocketKeyBinder.h
  ${ENGINE_DIR}/client/rocketMiscText.h
  ${ENGINE_DIR}/client/rocketDataGrid.h
  ${ENGINE_DIR}/client/rocketElement.h
  ${ENGINE_DIR}/client/rocket.h

  ${ENGINE_DIR}/sys/sdl_input.cpp
)

# Used by ttyclient
set( TTYCLIENTLIST
  ${CLIENTBASELIST}
  ${ENGINE_DIR}/null/null_input.cpp
  ${ENGINE_DIR}/null/null_renderer.cpp
  ${ENGINE_DIR}/null/NullAudio.cpp
  ${ENGINE_DIR}/null/null_rocket.cpp
)

# Used by server
set( DEDSERVERLIST
  ${ENGINELIST}
  ${SERVERLIST}
  ${ENGINE_DIR}/null/null_input.cpp
  ${ENGINE_DIR}/null/null_client.cpp
)

# Add include dirs
include_directories( ${MOUNT_DIR} )

################################################################################
# Libraries
################################################################################

include( DaemonLibs )

# Force OpenAL Soft on Mac, otherwise the system framework will be selected
if( APPLE )
  set( OPENAL_INCLUDE_DIR ${DEPS_DIR}/include/AL )
  set( OPENAL_LIBRARY ${DEPS_DIR}/lib/libopenal.dylib )
endif()

# These variables are defined:
# LIBS_BASE, LIBS_ENGINE, LIBS_CLIENT
# LIBSRC_NACL, LIBSRC_ENGINE

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
  find_library( LIBICONV iconv )
  if( LIBICONV )
    set( LIBS_ENGINE ${LIBS_ENGINE} ${LIBICONV} )
  endif()
  mark_as_advanced( LIBM LIBRT LIBICONV )
  set( LIBS_BASE ${LIBS_BASE} ${CMAKE_DL_LIBS} )
  find_package( Threads REQUIRED )
  set( LIBS_BASE ${LIBS_BASE} ${CMAKE_THREAD_LIBS_INIT} )
  if( APPLE )
    set( LIBS_CLIENT ${LIBS_CLIENT} "-framework Carbon" "-framework IOKit" "-framework Cocoa" )
  endif()
endif()

# Minizip
add_library( minizip ${MINIZIPLIST} )
set_target_properties( minizip PROPERTIES POSITION_INDEPENDENT_CODE 1 )
set( LIBS_BASE ${LIBS_BASE} minizip )

# Tinygettext
set( LIBSRC_ENGINE ${LIBSRC_ENGINE} ${TINYGETTEXTLIST} )

# Detour
set( LIBSRC_ENGINE ${LIBSRC_ENGINE} ${DETOURLIST} )

# GeoIP
if( USE_GEOIP )
  find_package( GeoIP REQUIRED )
  add_definitions( -DHAVE_GEOIP )
  set( LIBS_ENGINE ${LIBS_ENGINE} ${GeoIP_LIBRARIES} )
  include_directories( ${GeoIP_INCLUDE_DIRS} )
endif()

# SDL, required for all targets on win32 because of iconv
if( BUILD_CLIENT OR ( WIN32 AND (BUILD_TTY_CLIENT OR BUILD_SERVER ) ) )
  find_package( SDL2 REQUIRED )
  include_directories( ${SDL2_INCLUDE_DIR} )
  if( WIN32 )
    set( LIBS_ENGINE ${LIBS_ENGINE} ${SDL2_LIBRARY} )
  else()
    set( LIBS_CLIENT ${LIBS_CLIENT} ${SDL2_LIBRARY} )
  endif()

  # Used by tinygettext to use SDL_iconv instead of base iconv
  if( WIN32 )
    add_definitions( -DHAVE_SDL )
  endif()
endif()
mark_as_advanced( SDL2MAIN_LIBRARY SDL2_LIBRARY SDL2_INCLUDE_DIR )

# libRocket
if( BUILD_CLIENT )
  set( ROCKET_DIR ${LIB_DIR}/libRocket )
  include_directories( ${ROCKET_DIR}/Include ${ROCKET_DIR}/Contrib/Include )
  add_subdirectory( ${ROCKET_DIR} )
  set_target_properties(ROCKET_LIB PROPERTIES FOLDER "libs")
  add_definitions( -DSTATIC_LIB )
  set( LIBS_CLIENT ${LIBS_CLIENT} ROCKET_LIB )
endif()

# zlib
find_package( ZLIB REQUIRED )
include_directories( ${ZLIB_INCLUDE_DIRS} )
set( LIBS_BASE ${LIBS_BASE} ${ZLIB_LIBRARIES} )

# Nettle
find_package( Nettle REQUIRED )
include_directories( ${NETTLE_INCLUDE_DIR} )
set( LIBS_ENGINE ${LIBS_ENGINE} ${NETTLE_LIBRARIES} )

# GMP
find_package( GMP REQUIRED )
include_directories( ${GMP_INCLUDE_DIR} )
set( LIBS_ENGINE ${LIBS_ENGINE} ${GMP_LIBRARIES} )

# Speex
if( BUILD_CLIENT AND USE_VOIP )
  find_package( Speex REQUIRED )
  include_directories( ${SPEEX_INCLUDE_DIR} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${SPEEX_LIBRARIES} )
endif()

# Ogg
if( BUILD_CLIENT )
  find_package( Ogg REQUIRED )
  include_directories( ${OGG_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${OGG_LIBRARIES} )
endif()

# Vorbis
if( BUILD_CLIENT )
  find_package( Vorbis REQUIRED )
  include_directories( ${VORBIS_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${VORBIS_LIBRARIES} )
endif()

# Opus
if( BUILD_CLIENT )
  find_package( Opus REQUIRED )
  include_directories( ${OPUS_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${OPUS_LIBRARIES} )
endif()

# Theora
if( BUILD_CLIENT )
  find_package( Theora REQUIRED )
  include_directories( ${THEORA_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${THEORA_LIBRARIES} )
endif()

# WebP
if( BUILD_CLIENT )
  find_package( WebP REQUIRED )
  include_directories( ${WEBP_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${WEBP_LIBRARIES} )
endif()

# JPEG
if( BUILD_CLIENT )
  find_package( JPEG REQUIRED )
  include_directories( ${JPEG_INCLUDE_DIR} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${JPEG_LIBRARIES} )
endif()

# PNG
if( BUILD_CLIENT )
  find_package( PNG REQUIRED )
  include_directories( ${PNG_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${PNG_LIBRARIES} )
endif()

# Freetype
if( BUILD_CLIENT )
  find_package( Freetype REQUIRED )
  include_directories( ${FREETYPE_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${FREETYPE_LIBRARIES} )
endif()

# GLEW
if( BUILD_CLIENT )
  find_package( GLEW REQUIRED )
  include_directories( ${GLEW_INCLUDE_DIR} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${GLEW_LIBRARIES} )
endif()

# OpenGL
if( BUILD_CLIENT )
  find_package( OpenGL REQUIRED )
  include_directories( ${OPENGL_INCLUDE_DIR} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${OPENGL_LIBRARIES} )
endif()

# OpenAL
if( BUILD_CLIENT )
  find_package( OpenAL REQUIRED )
  include_directories( ${OPENAL_INCLUDE_DIR} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${OPENAL_LIBRARY} )
endif()

# CURL
if( BUILD_CLIENT OR BUILD_TTY_CLIENT )
  find_package( CURL REQUIRED )
  include_directories( ${CURL_INCLUDE_DIRS} )
  set( LIBS_CLIENTBASE ${LIBS_CLIENTBASE} ${CURL_LIBRARIES} )
endif()

# Curses, pdcurses on Windows and ncursesw on Unix
# Not supported on mac because the included version is too old
if( WIN32 )
  add_definitions( -DPDC_WIDE -DPDC_FORCE_UTF8 -DUSE_CURSES )
  include_directories( ${LIB_DIR}/pdcurses )
  set( LIBSRC_ENGINE ${LIBSRC_ENGINE} ${PDCURSESLIST} )
  set( LIBS_ENGINE ${LIBS_ENGINE} gdi32 comdlg32 )
elseif( NOT APPLE )
  add_definitions( -DUSE_CURSES )
  set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
  find_package( CursesW REQUIRED )
  include_directories( ${CURSESW_INCLUDE_DIR} )
  set( LIBS_ENGINE ${LIBS_ENGINE} ${CURSESW_LIBRARIES} )
endif()

# Builds the libs for which we include the source
add_library( engine-source-libs EXCLUDE_FROM_ALL ${LIBSRC_ENGINE} )
add_library( nacl-source-libs EXCLUDE_FROM_ALL ${LIBSRC_NACL} )
set_target_properties( engine-source-libs nacl-source-libs PROPERTIES POSITION_INDEPENDENT_CODE 1 FOLDER "libs" )

set( LIBS_ENGINE engine-source-libs nacl-source-libs ${LIBS_ENGINE} )

################################################################################
# Engine
################################################################################

set( WIN_RC ${ENGINE_DIR}/sys/daemon.rc )

if( BUILD_CLIENT )
  add_executable( client WIN32 ${PCH_FILE} ${CLIENTLIST} ${WIN_RC} )
  if( USE_SMP )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS SMP )
  endif()
  target_link_libraries( client ${LIBS_CLIENT} ${LIBS_CLIENTBASE} ${LIBS_ENGINE} ${LIBS_BASE} )
  set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS BUILD_ENGINE BUILD_CLIENT )
  if( USE_VOIP )
    # Set it here so the tty client doesn't need to depend on speex
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_VOIP )
  endif()
  set_target_properties( client PROPERTIES OUTPUT_NAME "daemon" PREFIX "" FOLDER "engine" )
  set_property( TARGET client APPEND PROPERTY INCLUDE_DIRECTORIES ${ENGINE_DIR} )
  ADD_PRECOMPILED_HEADER( client ${COMMON_DIR}/Common.h )
endif()

if( BUILD_SERVER )
  add_executable( server WIN32 ${PCH_FILE} ${DEDSERVERLIST} ${WIN_RC} )
  target_link_libraries( server ${LIBS_ENGINE} ${LIBS_BASE} )
  set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS BUILD_ENGINE BUILD_SERVER )
  set_target_properties( server PROPERTIES OUTPUT_NAME "daemonded" PREFIX "" FOLDER "engine" )
  set_property( TARGET server APPEND PROPERTY INCLUDE_DIRECTORIES ${ENGINE_DIR} )
  ADD_PRECOMPILED_HEADER( server ${COMMON_DIR}/Common.h )
endif()

if( BUILD_TTY_CLIENT )
  add_executable( ttyclient WIN32 ${PCH_FILE} ${TTYCLIENTLIST} ${WIN_RC} )
  target_link_libraries( ttyclient ${LIBS_CLIENTBASE} ${LIBS_ENGINE} ${LIBS_BASE} )
  set_property( TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS BUILD_ENGINE BUILD_TTY_CLIENT )
  set_target_properties( ttyclient PROPERTIES OUTPUT_NAME "daemon-tty" PREFIX "" FOLDER "engine" )
  set_property( TARGET ttyclient APPEND PROPERTY INCLUDE_DIRECTORIES ${ENGINE_DIR} )
  ADD_PRECOMPILED_HEADER( ttyclient ${COMMON_DIR}/Common.h )
endif()

################################################################################
# Runtime dependencies
################################################################################

if( BUILD_CLIENT OR BUILD_SERVER OR BUILD_TTY_CLIENT )
  add_custom_target( runtime_deps )
  set_target_properties( runtime_deps PROPERTIES FOLDER "CMakePlumbing" )

  # The NaCl loader and IRT are required to load .nexe files
  add_custom_command( TARGET runtime_deps PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/sel_ldr${CMAKE_EXECUTABLE_SUFFIX} ${FULL_OUTPUT_DIR}/nacl_loader${CMAKE_EXECUTABLE_SUFFIX} )
  add_custom_command( TARGET runtime_deps PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/irt_core-${ARCH}.nexe ${FULL_OUTPUT_DIR}/irt_core-${ARCH}.nexe )

  # Linux uses a bootstrap program to reserve address space
  if( LINUX )
    add_custom_command( TARGET runtime_deps PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/nacl_helper_bootstrap ${FULL_OUTPUT_DIR}/nacl_helper_bootstrap )
  endif()

  # Win32 requires nacl_loader64.exe in order to run on Win64
  if( WIN32 AND ARCH STREQUAL x86 )
    add_custom_command( TARGET runtime_deps PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/sel_ldr64${CMAKE_EXECUTABLE_SUFFIX} ${FULL_OUTPUT_DIR}/nacl_loader64${CMAKE_EXECUTABLE_SUFFIX} )
    add_custom_command( TARGET runtime_deps PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/irt_core-x86_64.nexe ${FULL_OUTPUT_DIR}/irt_core-x86_64.nexe )
  endif()

  # Mac requires some libraries from external_deps
  if( APPLE )
    add_custom_command( TARGET runtime_deps PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${DEPS_DIR}/SDL2.framework ${FULL_OUTPUT_DIR}/SDL2.framework )
    file( GLOB RUNTIME_LIBS ${DEPS_DIR}/lib/*.dylib )
    foreach( RUNTIME_LIB ${RUNTIME_LIBS} )
      add_custom_command( TARGET runtime_deps PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RUNTIME_LIB} ${FULL_OUTPUT_DIR} )
    endforeach()
  endif()

  # Windows requires some libraries from external_deps
  if( WIN32 )
    file( GLOB RUNTIME_LIBS ${DEPS_DIR}/bin/*.dll )
    foreach( RUNTIME_LIB ${RUNTIME_LIBS} )
      add_custom_command( TARGET runtime_deps PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${RUNTIME_LIB} ${FULL_OUTPUT_DIR} )
    endforeach()
  endif()

  if( BUILD_CLIENT )
    add_dependencies( client runtime_deps )
  endif()
  if( BUILD_SERVER )
    add_dependencies( server runtime_deps )
  endif()
  if( BUILD_TTY_CLIENT )
    add_dependencies( ttyclient runtime_deps )
  endif()
endif()
