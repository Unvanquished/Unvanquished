#
# CMakeLists.txt
#
# cmake required (http://www.cmake.org/)
#

cmake_minimum_required( VERSION 2.8 )
if( POLICY CMP0017 )
  cmake_policy( SET CMP0017 NEW )
endif()

include( CMakeDependentOption )

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set( MSVC_INCREMENTAL_DEFAULT ON )

# Default to Release builds. To use system CFLAGS only (for distro builds), set CMAKE_BUILD_TYPE to None
if( NOT CMAKE_BUILD_TYPE )
   set( CMAKE_BUILD_TYPE "Release" )
endif()

project( Daemon C CXX )

################################################################################
# Configuration options
################################################################################

# Build targets
option( BUILD_CLIENT         "Build Daemon client"                         1 )
option( BUILD_SERVER         "Build Daemonded server"                      1 )
option( BUILD_GAME_NATIVE    "Build native game logic "                    0 )
option( BUILD_GAME_NACL      "Build NaCl game logic"                       0 )
option( BUILD_TTY_CLIENT     "Build Daemon headless client"                0 )
cmake_dependent_option( BUILD_OLD_RENDERER   "Build old renderer library"                            1 BUILD_CLIENT                   0 )
cmake_dependent_option( BUILD_NEW_RENDERER   "Build new renderer library"                            1 BUILD_CLIENT                   0 )

option( ENABLE_LTO           "Use link-time optimization for Release builds"                            0 )
option( ENABLE_BUNDLING      "When packaging automatically include all non-system libraries in package" 0 )
option( ENABLE_HARDENING     "Use stack protection"                                                     0 )
option( ENABLE_QVM_COMPAT    "Configure the engine to use QVMs instead of NaCl"                         0 )

cmake_dependent_option( USE_MUMBLE           "Compile with support for Mumble"                       1 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_VOIP             "Compile with VOIP support"                             1 "BUILD_CLIENT OR BUILD_SERVER" 0 )
cmake_dependent_option( USE_CURSES           "Use the curses console"                                1 "BUILD_CLIENT OR BUILD_SERVER OR BUILD_TTY_CLIENT" 0 )

################################################################################
# Determine platform
################################################################################

# When adding a new platform, look at all the places WIN32, MAC and LINUX are used
if( WIN32 )
  set( PLATFORM "win32" )
elseif( CMAKE_SYSTEM_NAME MATCHES "Darwin" )
  set( MAC ON )
  set( PLATFORM "mac" )
elseif( CMAKE_SYSTEM_NAME MATCHES "Linux" )
  set( LINUX ON )
  set( PLATFORM "linux" )
else()
  message( FATAL_ERROR "Platform not supported" )
endif()

################################################################################
# Determine architecture
################################################################################

# When adding a new architecture, look at all the places ARCH is used
if( WIN32 )
  # CMAKE_SYSTEM_PROCESSOR is not reliable under windows
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( ARCH "x86_64" )
  elseif( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    set( ARCH "x86" )
  else()
    set( ARCH ${CMAKE_SYSTEM_PROCESSOR} )
  endif()
else()
  if( CMAKE_SYSTEM_PROCESSOR MATCHES "i.86" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86" )
    # Sometimes CMAKE_SYSTEM_PROCESSOR returns x86 on x86_64
    if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
      set( ARCH "x86_64" )
    else()
      set( ARCH "x86" )
    endif()
  elseif( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "amd64" )
   set( ARCH "x86_64" )
  else()
    set( ARCH ${CMAKE_SYSTEM_PROCESSOR} )
  endif()
endif()

################################################################################
# Compile and link flags
################################################################################

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Set flag without checking, optional argument specifies build type
macro( set_c_flag FLAG )
  if( ${ARGC} GREATER 1 )
    set( CMAKE_C_FLAGS_${ARGV1} "${CMAKE_C_FLAGS_${ARGV1}} ${FLAG}" )
  else()
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG}" )
  endif()
endmacro()
macro( set_cxx_flag FLAG )
  if( ${ARGC} GREATER 1 )
    set( CMAKE_CXX_FLAGS_${ARGV1} "${CMAKE_CXX_FLAGS_${ARGV1}} ${FLAG}" )
  else()
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${FLAG}" )
  endif()
endmacro()
macro( set_c_cxx_flag FLAG )
  set_c_flag( ${FLAG} ${ARGN} )
  set_cxx_flag( ${FLAG} ${ARGN} )
endmacro()
macro( set_linker_flag FLAG )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}" )
  set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${FLAG}" )
  set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${FLAG}" )
endmacro()

# Try flag and set if it works, optional argument specifies build type
macro( try_c_flag PROP FLAG )
  check_C_compiler_flag( ${FLAG} FLAG_${PROP} )
  if( FLAG_${PROP} )
    set_c_flag( ${FLAG} ${ARGN} )
  endif()
endmacro()
macro( try_cxx_flag PROP FLAG )
  check_CXX_compiler_flag( ${FLAG} FLAG_${PROP} )
  if( FLAG_${PROP} )
    set_cxx_flag( ${FLAG} ${ARGV2} )
  endif()
endmacro()
macro( try_c_cxx_flag PROP FLAG )
  # Only try the flag once on the C compiler
  try_c_flag( ${PROP} ${FLAG} ${ARGV2} )
  if( FLAG_${PROP} )
    set_cxx_flag( ${FLAG} ${ARGV2} )
  endif()
endmacro()
macro( try_linker_flag PROP FLAG )
  # Check it with the C compiler
  set( CMAKE_REQUIRED_FLAGS ${FLAG} )
  check_C_compiler_flag( ${FLAG} FLAG_${PROP} )
  if( FLAG_${PROP} )
    set_linker_flag( ${FLAG} ${ARGN} )
  endif()
  set( CMAKE_REQUIRED_FLAGS "" )
endmacro()

if( MSVC )
  set_c_cxx_flag( "/fp:fast" )
  if( ARCH STREQUAL "x86" )
    set_c_cxx_flag( "/arch:SSE2" )
  endif()
  if( ENABLE_LTO )
    set_c_cxx_flag( "/GL" RELEASE )
    set_linker_flag( "/LTCG" )
  endif()
else()
  # Amanieu: Look into enabling strict aliasing
  set_c_cxx_flag( "-ffast-math" )
  set_c_cxx_flag( "-fno-strict-aliasing" )
  set_c_cxx_flag( "-fomit-frame-pointer" RELEASE )

  # Set arch on x86 to SSE2 minimum and enable CMPXCHG16B
  if( ARCH STREQUAL "x86" )
    set_c_cxx_flag( "-m32" )
    set_c_cxx_flag( "-msse2" )
    set_c_cxx_flag( "-mtune=generic" )
    set_c_cxx_flag( "-mfpmath=sse" )
  elseif( ARCH STREQUAL "x86_64" )
    set_c_cxx_flag( "-m64" )
    set_c_cxx_flag( "-mtune=generic" )
    try_c_cxx_flag( MCX16 "-mcx16" )
  endif()

  # Use hidden symbol visibility if possible
  try_c_cxx_flag( FVISIBILITY_HIDDEN "-fvisibility=hidden" )

  # Extra debug flags
  set_c_cxx_flag( "-g3" )
  try_c_cxx_flag( OPTIMIZE_DEBUG "-Og" DEBUG )

  # C++11 support
  try_cxx_flag( GNUXX11 "-std=gnu++11" )
  if( NOT FLAG_GNUXX11 )
    try_cxx_flag( GNUXX0X "-std=gnu++0x" )
    if( NOT FLAG_GNUXX0X )
      message( FATAL_ERROR "C++11 not supported by compiler" )
    endif()
  endif()
  
  # Use libc++ on Mac because the shipped libstdc++ version is too old
  if( MAC )
    set_c_cxx_flag( "-stdlib=libc++")
  endif()

  # Hardening
  add_definitions( -D_FORTIFY_SOURCE=2 )
  if( ENABLE_HARDENING )
    try_c_cxx_flag( FSTACK_PROTECTOR "-fstack-protector" )
  endif()

  # Linker flags
  try_linker_flag( LINKER_O1 "-Wl,-O1" )
  try_linker_flag( LINKER_SORT_COMMON "-Wl,--sort-common" )
  try_linker_flag( LINKER_AS_NEEDED "-Wl,--as-needed" )
  try_linker_flag( LINKER_NO_UNDEFINED "-Wl,--no-undefined" )
  try_linker_flag( LINKER_Z_RELRO "-Wl,-z,relro" )

  # LTO support, only used on release builds
  if( ENABLE_LTO )
    # This flag is also used when linking
    set_c_cxx_flag( "-flto" RELEASE )
  endif()

  # The -pthread flag sets some preprocessor defines
  try_c_cxx_flag( PTHREAD "-pthread" )

  # Warning options
  set_c_cxx_flag( "-Wall" )
  set_c_cxx_flag( "-Wno-sign-compare" )
  set_c_cxx_flag( "-Wno-write-strings" )
endif()

# Windows-specific definitions
if( WIN32 )
  # Minimum Windows version: XP
  # Define WIN32 for compatibility (compiler defines _WIN32)
  add_definitions( -DWINVER=0x501 -DWIN32 )
endif()
if( MSVC )
  add_definitions( -D_CRT_SECURE_NO_WARNINGS )
endif()

# QVM compatibility
if( ENABLE_QVM_COMPAT )
  add_definitions( -DQVM_COMPAT )
endif()

################################################################################
# Directories
################################################################################

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
set( MOUNT_DIR ${CMAKE_SOURCE_DIR}/src )
set( OBJ_DIR ${CMAKE_BINARY_DIR}/CMakeFiles )
set( ENGINE_DIR ${MOUNT_DIR}/engine )
set( SHARED_DIR ${MOUNT_DIR}/shared )
set( GAMELOGIC_DIR ${MOUNT_DIR}/gamelogic )
set( LIB_DIR ${MOUNT_DIR}/libs )

################################################################################
# Git revision info
################################################################################

if( EXISTS "${CMAKE_SOURCE_DIR}/.git" )
  find_package( Git )
  if( GIT_FOUND )
    execute_process( COMMAND git describe --tags --long --match v* --dirty=+dirty OUTPUT_VARIABLE GIT_DESCRIBE_REPORT OUTPUT_STRIP_TRAILING_WHITESPACE )
    message( STATUS "git reported ${GIT_DESCRIBE_REPORT}" )
    # this may fail with annotated non-release tags
    if( GIT_DESCRIBE_REPORT MATCHES "-0-g.......$" )
      set( GIT_DESCRIBE_REPORT )
    endif()
  endif()
endif()

if( GIT_DESCRIBE_REPORT )
  set( DESIRED_REVISION_H_CONTENTS "#define REVISION \"${GIT_DESCRIBE_REPORT}\"\n" )
endif()

if( EXISTS "${OBJ_DIR}/revision.h" )
  file( READ "${OBJ_DIR}/revision.h" ACTUAL_REVISION_H_CONTENTS )
  if( NOT "${ACTUAL_REVISION_H_CONTENTS}" STREQUAL "${DESIRED_REVISION_H_CONTENTS}" )
    file( WRITE "${OBJ_DIR}/revision.h" "${DESIRED_REVISION_H_CONTENTS}" )
  endif()
else()
  file( WRITE "${OBJ_DIR}/revision.h" "${DESIRED_REVISION_H_CONTENTS}" )
endif()

include_directories( "${OBJ_DIR}" )

################################################################################
# File lists
################################################################################

set( SHAREDLIST
  ${ENGINE_DIR}/qcommon/q_shared.c
  ${ENGINE_DIR}/qcommon/q_unicode.c
  ${ENGINE_DIR}/qcommon/q_math.c
)

set( RENDERERLIST
  ${ENGINE_DIR}/renderer/tr_animation.c
  ${ENGINE_DIR}/renderer/tr_animation_mdm.c
  ${ENGINE_DIR}/renderer/tr_animation_mds.c
  ${ENGINE_DIR}/renderer/tr_backend.c
  ${ENGINE_DIR}/renderer/tr_bonematrix.c
  ${ENGINE_DIR}/renderer/tr_bsp.c
  ${ENGINE_DIR}/renderer/tr_cmds.c
  ${ENGINE_DIR}/renderer/tr_cmesh.c
  ${ENGINE_DIR}/renderer/tr_curve.c
  ${ENGINE_DIR}/renderer/tr_decals.c
  ${ENGINE_DIR}/renderer/tr_flares.c
  ${ENGINE_DIR}/renderer/tr_font.c
  ${ENGINE_DIR}/renderer/tr_image.c
  ${ENGINE_DIR}/renderer/tr_init.c
  ${ENGINE_DIR}/renderer/tr_light.c
  ${ENGINE_DIR}/renderer/tr_main.c
  ${ENGINE_DIR}/renderer/tr_marks.c
  ${ENGINE_DIR}/renderer/tr_mesh.c
  ${ENGINE_DIR}/renderer/tr_model.c
  ${ENGINE_DIR}/renderer/tr_model_md5.c
  ${ENGINE_DIR}/renderer/tr_noise.c
  ${ENGINE_DIR}/renderer/tr_scene.c
  ${ENGINE_DIR}/renderer/tr_shade.c
  ${ENGINE_DIR}/renderer/tr_shade_calc.c
  ${ENGINE_DIR}/renderer/tr_shader.c
  ${ENGINE_DIR}/renderer/tr_shadows.c
  ${ENGINE_DIR}/renderer/tr_skin.c
  ${ENGINE_DIR}/renderer/tr_sky.c
  ${ENGINE_DIR}/renderer/tr_surface.c
  ${ENGINE_DIR}/renderer/tr_world.c
)

set( RENDERERGLLIST
  ${ENGINE_DIR}/rendererGL/gl_shader.cpp
  ${ENGINE_DIR}/rendererGL/tr_animation_mdm.c
  ${ENGINE_DIR}/rendererGL/tr_animation.c
  ${ENGINE_DIR}/rendererGL/tr_backend.cpp
  ${ENGINE_DIR}/renderer/tr_bonematrix.c
  ${ENGINE_DIR}/rendererGL/tr_bsp.c
  ${ENGINE_DIR}/rendererGL/tr_cmds.c
  ${ENGINE_DIR}/rendererGL/tr_curve.c
  ${ENGINE_DIR}/rendererGL/tr_decals.c
  ${ENGINE_DIR}/rendererGL/tr_fbo.c
  ${ENGINE_DIR}/rendererGL/tr_flares.c
  ${ENGINE_DIR}/rendererGL/tr_fog.c
  ${ENGINE_DIR}/renderer/tr_font.c
  ${ENGINE_DIR}/rendererGL/tr_image.c
  ${ENGINE_DIR}/rendererGL/tr_image_dds.c
  ${ENGINE_DIR}/rendererGL/tr_image_exr.cpp
  ${ENGINE_DIR}/rendererGL/tr_image_jpg.c
  ${ENGINE_DIR}/rendererGL/tr_image_png.c
  ${ENGINE_DIR}/rendererGL/tr_image_tga.c
  ${ENGINE_DIR}/rendererGL/tr_image_webp.c
  ${ENGINE_DIR}/rendererGL/tr_init.cpp
  ${ENGINE_DIR}/rendererGL/tr_light.c
  ${ENGINE_DIR}/rendererGL/tr_main.c
  ${ENGINE_DIR}/rendererGL/tr_marks.c
  ${ENGINE_DIR}/rendererGL/tr_mesh.c
  ${ENGINE_DIR}/rendererGL/tr_model.c
  ${ENGINE_DIR}/rendererGL/tr_model_md3.c
  ${ENGINE_DIR}/rendererGL/tr_model_md5.c
  ${ENGINE_DIR}/rendererGL/tr_model_mdc.c
  ${ENGINE_DIR}/rendererGL/tr_model_mdm.c
  ${ENGINE_DIR}/rendererGL/tr_model_psk.c
  ${ENGINE_DIR}/rendererGL/tr_model_skel.c
  ${ENGINE_DIR}/rendererGL/tr_noise.c
  ${ENGINE_DIR}/rendererGL/tr_scene.c
  ${ENGINE_DIR}/rendererGL/tr_shade.cpp
  ${ENGINE_DIR}/rendererGL/tr_shade_calc.c
  ${ENGINE_DIR}/rendererGL/tr_shader.c
  ${ENGINE_DIR}/rendererGL/tr_shadows.c
  ${ENGINE_DIR}/rendererGL/tr_skin.c
  ${ENGINE_DIR}/rendererGL/tr_sky.cpp
  ${ENGINE_DIR}/rendererGL/tr_surface.c
  ${ENGINE_DIR}/rendererGL/tr_vbo.c
  ${ENGINE_DIR}/rendererGL/tr_world.cpp
  ${ENGINE_DIR}/qcommon/md4.c
  ${LIB_DIR}/openexr/half.cpp
)

set( SERVERLIST
  ${ENGINE_DIR}/server/sv_bot.c
  ${ENGINE_DIR}/server/sv_ccmds.c
  ${ENGINE_DIR}/server/sv_client.c
  ${ENGINE_DIR}/server/sv_game.c
  ${ENGINE_DIR}/server/sv_init.c
  ${ENGINE_DIR}/server/sv_main.c
  ${ENGINE_DIR}/server/sv_net_chan.c
  ${ENGINE_DIR}/server/sv_snapshot.c
  ${ENGINE_DIR}/server/sv_world.c
)

set( ENGINELIST
  ${SHARED_DIR}/Command.cpp
  ${SHARED_DIR}/LineEditData.cpp
  ${SHARED_DIR}/String.cpp
  ${ENGINE_DIR}/framework/BaseCommands.cpp
  ${ENGINE_DIR}/framework/CommandSystem.cpp
  ${ENGINE_DIR}/framework/ConsoleField.cpp
  ${ENGINE_DIR}/framework/ConsoleHistory.cpp
  ${ENGINE_DIR}/qcommon/cm_load.c
  ${ENGINE_DIR}/qcommon/cm_trisoup.c
  ${ENGINE_DIR}/qcommon/cm_patch.c
  ${ENGINE_DIR}/qcommon/cm_plane.c
  ${ENGINE_DIR}/qcommon/cm_polylib.c
  ${ENGINE_DIR}/qcommon/cm_test.c
  ${ENGINE_DIR}/qcommon/cm_trace.c
  ${ENGINE_DIR}/qcommon/cmd.c
  ${ENGINE_DIR}/qcommon/common.c
  ${ENGINE_DIR}/qcommon/crypto.c
  ${ENGINE_DIR}/qcommon/cvar.c
  ${ENGINE_DIR}/qcommon/files.c
  ${ENGINE_DIR}/qcommon/htable.c
  ${ENGINE_DIR}/qcommon/huffman.c
  ${ENGINE_DIR}/qcommon/md4.c
  ${ENGINE_DIR}/qcommon/md5.c
  ${ENGINE_DIR}/qcommon/msg.c
  ${ENGINE_DIR}/qcommon/net_chan.c
  ${ENGINE_DIR}/qcommon/net_ip.c
  ${ENGINE_DIR}/qcommon/parse.c
  ${ENGINE_DIR}/qcommon/translation.cpp
  ${ENGINE_DIR}/qcommon/unzip.c
  ${ENGINE_DIR}/qcommon/ioapi.c
  ${ENGINE_DIR}/sys/sys_main.c
  ${ENGINE_DIR}/sys/con_log.c
)

if( ENABLE_QVM_COMPAT )
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/qcommon/vm.c
    ${ENGINE_DIR}/qcommon/vm_interpreted.c
  )
  if( ARCH STREQUAL "x86" OR ARCH STREQUAL "x86_64" )
    set( ENGINELIST ${ENGINELIST} ${ENGINE_DIR}/qcommon/vm_x86.c )
  endif()
else()
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/qcommon/nacl.cpp
    ${ENGINE_DIR}/qcommon/vm.cpp
  )
endif()

if( WIN32 )
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/sys_win32.c
    ${ENGINE_DIR}/sys/con_win32.c
  )
else()
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/sys_unix.c
    ${ENGINE_DIR}/sys/con_tty.c
  )
endif()

if( MAC )
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/sys_osx.mm
  )
endif()

if( USE_CURSES )
  set( ENGINELIST ${ENGINELIST} ${ENGINE_DIR}/sys/con_curses.c )
endif()

set( CLIENTLIST
  ${ENGINE_DIR}/client/cin_ogm.c
  ${ENGINE_DIR}/client/cl_avi.c
  ${ENGINE_DIR}/client/cl_cgame.c
  ${ENGINE_DIR}/client/cl_cin.c
  ${ENGINE_DIR}/client/cl_console.c
  ${ENGINE_DIR}/client/cl_input.c
  ${ENGINE_DIR}/client/cl_irc.c
  ${ENGINE_DIR}/client/cl_keys.c
  ${ENGINE_DIR}/client/cl_main.c
  ${ENGINE_DIR}/client/cl_net_chan.c
  ${ENGINE_DIR}/client/cl_parse.c
  ${ENGINE_DIR}/client/cl_scrn.c
  ${ENGINE_DIR}/client/cl_ui.c
  ${ENGINE_DIR}/client/cl_logs.c
  ${ENGINE_DIR}/client/dl_main.c
  ${ENGINE_DIR}/client/snd_adpcm.c
  ${ENGINE_DIR}/client/snd_codec.c
  ${ENGINE_DIR}/client/snd_codec_ogg.c
  ${ENGINE_DIR}/client/snd_codec_wav.c
  ${ENGINE_DIR}/client/snd_dma.c
  ${ENGINE_DIR}/client/snd_openal.c
  ${ENGINE_DIR}/client/snd_main.c
  ${ENGINE_DIR}/client/snd_mem.c
  ${ENGINE_DIR}/client/snd_mix.c
  ${ENGINE_DIR}/client/snd_wavelet.c
  ${ENGINE_DIR}/client/qal.c
)

if( USE_MUMBLE )
  set( CLIENTLIST ${CLIENTLIST} ${ENGINE_DIR}/client/libmumblelink.c )
endif()

set( SDLLIST
  ${ENGINE_DIR}/sys/sdl_input.c
  ${ENGINE_DIR}/sys/sdl_snd.c
)

set( CGAMEGPP_BASE
  ${GAMELOGIC_DIR}/game/bg_misc.c
  ${GAMELOGIC_DIR}/game/bg_parse.c
  ${GAMELOGIC_DIR}/game/bg_pmove.c
  ${GAMELOGIC_DIR}/game/bg_slidemove.c
  ${GAMELOGIC_DIR}/game/bg_alloc.c
  ${GAMELOGIC_DIR}/game/bg_voice.c
  ${GAMELOGIC_DIR}/cgame/cg_consolecmds.c
  ${GAMELOGIC_DIR}/cgame/cg_buildable.c
  ${GAMELOGIC_DIR}/cgame/cg_animation.c
  ${GAMELOGIC_DIR}/cgame/cg_animmapobj.c
  ${GAMELOGIC_DIR}/cgame/cg_draw.c
  ${GAMELOGIC_DIR}/cgame/cg_drawtools.c
  ${GAMELOGIC_DIR}/cgame/cg_ents.c
  ${GAMELOGIC_DIR}/cgame/cg_event.c
  ${GAMELOGIC_DIR}/cgame/cg_main.c
  ${GAMELOGIC_DIR}/cgame/cg_marks.c
  ${GAMELOGIC_DIR}/cgame/cg_minimap.c
  ${GAMELOGIC_DIR}/cgame/cg_players.c
  ${GAMELOGIC_DIR}/cgame/cg_playerstate.c
  ${GAMELOGIC_DIR}/cgame/cg_predict.c
  ${GAMELOGIC_DIR}/cgame/cg_servercmds.c
  ${GAMELOGIC_DIR}/cgame/cg_snapshot.c
  ${GAMELOGIC_DIR}/cgame/cg_view.c
  ${GAMELOGIC_DIR}/cgame/cg_weapons.c
  ${GAMELOGIC_DIR}/cgame/cg_scanner.c
  ${GAMELOGIC_DIR}/cgame/cg_attachment.c
  ${GAMELOGIC_DIR}/cgame/cg_trails.c
  ${GAMELOGIC_DIR}/cgame/cg_particles.c
  ${GAMELOGIC_DIR}/cgame/cg_tutorial.c
  ${GAMELOGIC_DIR}/cgame/cg_utils.c
  ${GAMELOGIC_DIR}/ui/ui_shared.c
  ${GAMELOGIC_DIR}/ui/ui_utf8.c
  ${GAMELOGIC_DIR}/cgame/cg_api.c
)

set( GAMEGPP_BASE
  ${GAMELOGIC_DIR}/game/g_main.c
  ${GAMELOGIC_DIR}/game/bg_misc.c
  ${GAMELOGIC_DIR}/game/bg_parse.c
  ${GAMELOGIC_DIR}/game/bg_pmove.c
  ${GAMELOGIC_DIR}/game/bg_slidemove.c
  ${GAMELOGIC_DIR}/game/bg_alloc.c
  ${GAMELOGIC_DIR}/game/bg_voice.c
  ${GAMELOGIC_DIR}/game/g_active.c
  ${GAMELOGIC_DIR}/game/g_client.c
  ${GAMELOGIC_DIR}/game/g_cmds.c
  ${GAMELOGIC_DIR}/game/g_combat.c
  ${GAMELOGIC_DIR}/game/g_entities.c
  ${GAMELOGIC_DIR}/game/g_physics.c
  ${GAMELOGIC_DIR}/game/g_buildable.c
  ${GAMELOGIC_DIR}/game/g_missile.c
  ${GAMELOGIC_DIR}/game/g_session.c
  ${GAMELOGIC_DIR}/game/g_spawn_afx.c
  ${GAMELOGIC_DIR}/game/g_spawn_ctrl.c
  ${GAMELOGIC_DIR}/game/g_spawn_fx.c
  ${GAMELOGIC_DIR}/game/g_spawn_game.c
  ${GAMELOGIC_DIR}/game/g_spawn_generic.c
  ${GAMELOGIC_DIR}/game/g_spawn_gfx.c
  ${GAMELOGIC_DIR}/game/g_spawn_mover.c
  ${GAMELOGIC_DIR}/game/g_spawn_position.c
  ${GAMELOGIC_DIR}/game/g_spawn_sensor.c
  ${GAMELOGIC_DIR}/game/g_spawn_shared.c
  ${GAMELOGIC_DIR}/game/g_spawn.c
  ${GAMELOGIC_DIR}/game/g_svcmds.c
  ${GAMELOGIC_DIR}/game/g_team.c
  ${GAMELOGIC_DIR}/game/g_utils.c
  ${GAMELOGIC_DIR}/game/g_maprotation.c
  ${GAMELOGIC_DIR}/game/g_weapon.c
  ${GAMELOGIC_DIR}/game/g_admin.c
  ${GAMELOGIC_DIR}/game/g_namelog.c
  ${GAMELOGIC_DIR}/game/g_api.cpp
)

set( UIGPP_BASE
  ${GAMELOGIC_DIR}/ui/ui_main.c
  ${GAMELOGIC_DIR}/ui/ui_atoms.c
  ${GAMELOGIC_DIR}/ui/ui_shared.c
  ${GAMELOGIC_DIR}/ui/ui_gameinfo.c
  ${GAMELOGIC_DIR}/game/bg_misc.c
  ${GAMELOGIC_DIR}/game/bg_parse.c
  ${GAMELOGIC_DIR}/game/bg_alloc.c
  ${GAMELOGIC_DIR}/ui/ui_utf8.c
  ${GAMELOGIC_DIR}/ui/ui_api.c
)

set( SPEEXLIST
  ${LIB_DIR}/libspeex/libspeex/bits.c
  ${LIB_DIR}/libspeex/libspeex/buffer.c
  ${LIB_DIR}/libspeex/libspeex/cb_search.c
  ${LIB_DIR}/libspeex/libspeex/exc_10_16_table.c
  ${LIB_DIR}/libspeex/libspeex/exc_10_32_table.c
  ${LIB_DIR}/libspeex/libspeex/exc_20_32_table.c
  ${LIB_DIR}/libspeex/libspeex/exc_5_256_table.c
  ${LIB_DIR}/libspeex/libspeex/exc_5_64_table.c
  ${LIB_DIR}/libspeex/libspeex/exc_8_128_table.c
  ${LIB_DIR}/libspeex/libspeex/fftwrap.c
  ${LIB_DIR}/libspeex/libspeex/filterbank.c
  ${LIB_DIR}/libspeex/libspeex/filters.c
  ${LIB_DIR}/libspeex/libspeex/gain_table.c
  ${LIB_DIR}/libspeex/libspeex/gain_table_lbr.c
  ${LIB_DIR}/libspeex/libspeex/hexc_10_32_table.c
  ${LIB_DIR}/libspeex/libspeex/hexc_table.c
  ${LIB_DIR}/libspeex/libspeex/high_lsp_tables.c
  ${LIB_DIR}/libspeex/libspeex/jitter.c
  ${LIB_DIR}/libspeex/libspeex/kiss_fft.c
  ${LIB_DIR}/libspeex/libspeex/kiss_fftr.c
  ${LIB_DIR}/libspeex/libspeex/lpc.c
  ${LIB_DIR}/libspeex/libspeex/lsp.c
  ${LIB_DIR}/libspeex/libspeex/lsp_tables_nb.c
  ${LIB_DIR}/libspeex/libspeex/ltp.c
  ${LIB_DIR}/libspeex/libspeex/mdf.c
  ${LIB_DIR}/libspeex/libspeex/modes.c
  ${LIB_DIR}/libspeex/libspeex/modes_wb.c
  ${LIB_DIR}/libspeex/libspeex/nb_celp.c
  ${LIB_DIR}/libspeex/libspeex/preprocess.c
  ${LIB_DIR}/libspeex/libspeex/quant_lsp.c
  ${LIB_DIR}/libspeex/libspeex/resample.c
  ${LIB_DIR}/libspeex/libspeex/sb_celp.c
  ${LIB_DIR}/libspeex/libspeex/smallft.c
  ${LIB_DIR}/libspeex/libspeex/speex.c
  ${LIB_DIR}/libspeex/libspeex/speex_callbacks.c
  ${LIB_DIR}/libspeex/libspeex/speex_header.c
  ${LIB_DIR}/libspeex/libspeex/stereo.c
  ${LIB_DIR}/libspeex/libspeex/vbr.c
  ${LIB_DIR}/libspeex/libspeex/vq.c
  ${LIB_DIR}/libspeex/libspeex/window.c
)

set( NETTLELIST
  ${LIB_DIR}/nettle/nettle/bignum.c
  ${LIB_DIR}/nettle/nettle/bignum-random.c
  ${LIB_DIR}/nettle/nettle/buffer-init.c
  ${LIB_DIR}/nettle/nettle/nettle-buffer.c
  ${LIB_DIR}/nettle/nettle/nettle-internal.c
  ${LIB_DIR}/nettle/nettle/rsa2sexp.c
  ${LIB_DIR}/nettle/nettle/rsa.c
  ${LIB_DIR}/nettle/nettle/rsa-decrypt.c
  ${LIB_DIR}/nettle/nettle/rsa-encrypt.c
  ${LIB_DIR}/nettle/nettle/rsa-keygen.c
  ${LIB_DIR}/nettle/nettle/rsa-sign.c
  ${LIB_DIR}/nettle/nettle/sexp2bignum.c
  ${LIB_DIR}/nettle/nettle/sexp2rsa.c
  ${LIB_DIR}/nettle/nettle/sexp.c
  ${LIB_DIR}/nettle/nettle/sexp-format.c
)

set( ZLIBLIST
  ${LIB_DIR}/zlib/adler32.c
  ${LIB_DIR}/zlib/compress.c
  ${LIB_DIR}/zlib/crc32.c
  ${LIB_DIR}/zlib/deflate.c
  ${LIB_DIR}/zlib/gzio.c
  ${LIB_DIR}/zlib/infback.c
  ${LIB_DIR}/zlib/inffast.c
  ${LIB_DIR}/zlib/inflate.c
  ${LIB_DIR}/zlib/inftrees.c
  ${LIB_DIR}/zlib/ioapi.c
  ${LIB_DIR}/zlib/trees.c
  ${LIB_DIR}/zlib/uncompr.c
  ${LIB_DIR}/zlib/zutil.c
)

set( WIN_RC ${ENGINE_DIR}/sys/daemon.rc )

set( TINYGETTEXT_LIST
  ${LIB_DIR}/tinygettext/dictionary_manager.hpp
  ${LIB_DIR}/tinygettext/file_system.hpp
  ${LIB_DIR}/tinygettext/iconv.cpp
  ${LIB_DIR}/tinygettext/plural_forms.hpp
  ${LIB_DIR}/tinygettext/tinygettext.cpp
  ${LIB_DIR}/tinygettext/tinygettext.hpp
  ${LIB_DIR}/tinygettext/dictionary.cpp
  ${LIB_DIR}/tinygettext/dictionary.hpp
  ${LIB_DIR}/tinygettext/dictionary_manager.cpp
  ${LIB_DIR}/tinygettext/iconv.hpp
  ${LIB_DIR}/tinygettext/language.cpp
  ${LIB_DIR}/tinygettext/language.hpp
  ${LIB_DIR}/tinygettext/log.cpp
  ${LIB_DIR}/tinygettext/log.hpp
  ${LIB_DIR}/tinygettext/log_stream.hpp
  ${LIB_DIR}/tinygettext/plural_forms.cpp
  ${LIB_DIR}/tinygettext/po_parser.cpp
  ${LIB_DIR}/tinygettext/po_parser.hpp
  ${LIB_DIR}/findlocale/findlocale.c
)

if( MAC )
  set( NACL_LIST_NATIVE
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/posix/nacl_imc_posix.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/osx/nacl_imc.cc
  )
elseif( LINUX )
  set( NACL_LIST_NATIVE
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/posix/nacl_imc_posix.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/linux/nacl_imc.cc
  )
elseif( WIN32 )
  set( NACL_LIST_NATIVE
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/win/nacl_imc.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/win/nacl_shm.cc
  )
endif()

set( NACL_LIST_MODULE
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

#HACK: Force the compilation of EVERYTHING in C++
file(GLOB_RECURSE EVERYTHING_C "src/engine/*.c")
set_source_files_properties( ${EVERYTHING_C} PROPERTIES LANGUAGE CXX )

################################################################################
# Libraries
################################################################################

# Amanieu: Need to add proper if wrappers so that compilation doesn't fail because of a dep we don't need

# These variables are defined
# LIBS_BASE, LIBSRC_BASE
# LIBS_ENGINE, LIBSRC_ENGINE
# LIBS_RENDERER, LIBSRC_RENDERER
# LIBS_BASECLIENT, LIBSRC_BASECLIENT
# LIBS_CLIENT, LIBSRC_CLIENT

# We provide some pre-built packages for Windows, make find_package optional
if( WIN32 )
  set( OPTIONAL_WIN32 )
else()
  set( OPTIONAL_WIN32 REQUIRED )
endif()

# C99 int types for MSVC
if( MSVC )
  include_directories( ${LIB_DIR}/msinttypes )
endif()

# Base OS libs
if( WIN32 )
  # Amanieu: Check if psapi is really needed
  set( LIBS_BASE ${LIBS_BASE} winmm ws2_32 psapi )
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
    set( LIBS_BASE ${LIBS_BASE} ${LIBICONV} )
  endif()
  mark_as_advanced( LIBM LIBRT LIBICONV )
  set( LIBS_BASE ${LIBS_BASE} ${CMAKE_DL_LIBS} )
  find_package( Threads REQUIRED )
  set( LIBS_BASE ${LIBS_BASE} ${CMAKE_THREAD_LIBS_INIT} )
  if( MAC )
    set( LIBS_CLIENT ${LIBS_CLIENT} "-framework Carbon" "-framework IOKit" "-framework Cocoa" )
  endif()
endif()

# zlib
find_package( ZLIB )
if( ZLIB_FOUND )
  include_directories( ${ZLIB_INCLUDE_DIRS} )
  set( LIBS_BASE ${LIBS_BASE} ${ZLIB_LIBRARIES} )
else()
  include_directories( ${LIB_DIR}/zlib )
  set( LIBSRC_BASE ${LIBSRC_BASE} ${ZLIBLIST} )
endif()

# Tinygettext
set( LIBSRC_ENGINE ${LIBSRC_ENGINE} ${TINYGETTEXT_LIST} )

# Native client
if( MAC )
  add_definitions( -DNACL_WIN32=0 -DNACL_LINUX=0 -DNACL_ANDRIOD=0 -DNACL_OSX=1 )
elseif( LINUX )
  add_definitions( -DNACL_WIN32=0 -DNACL_LINUX=1 -DNACL_ANDRIOD=0 -DNACL_OSX=0 )
elseif( WIN32 )
  add_definitions( -DNACL_WIN32=1 -DNACL_LINUX=0 -DNACL_ANDRIOD=0 -DNACL_OSX=0 )
endif()
if( ARCH STREQUAL "x86" OR ARCH STREQUAL "x86_64" )
  add_definitions( -DNACL_BUILD_ARCH=x86 )
else()
  add_definitions( -DNACL_BUILD_ARCH=${ARCH} )
endif()
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  add_definitions( -DNACL_BUILD_SUBARCH=64 )
else()
  add_definitions( -DNACL_BUILD_SUBARCH=32 )
endif()
include_directories( ${LIB_DIR}/nacl )
if( NOT ENABLE_QVM_COMPAT )
  set( LIBSRC_ENGINE ${LIBSRC_ENGINE} ${NACL_LIST_NATIVE} )
endif()

# SDL, required for all targets on win32 because of iconv
if( BUILD_CLIENT OR WIN32 )
  find_package( SDL ${OPTIONAL_WIN32} )
  mark_as_advanced( SDLMAIN_LIBRARY SDL_LIBRARY SDL_INCLUDE_DIR )
  if( SDL_FOUND )
    include_directories( ${SDL_INCLUDE_DIR} )
    set( LIBS_CLIENT ${LIBS_CLIENT} ${SDL_LIBRARY} )
    set( LIBS_RENDERER ${LIBS_RENDERER} ${SDL_LIBRARY} )
    set( LIBS_RENDERERGL ${LIBS_RENDERERGL} ${SDL_LIBRARY} )
    if( MAC )
      # Mac OS X does not provide a SDLMain.dylib so we must compile our own
      add_library( INTERNAL_SDLMain ${ENGINE_DIR}/sys/SDLMain.mm )
      set( LIBS_CLIENT ${LIBS_CLIENT} INTERNAL_SDLMain )
    endif()
  else()
    add_subdirectory( ${LIB_DIR}/libsdl ${OBJ_DIR}/libsdl )
    include_directories( ${LIB_DIR}/libsdl/include )
    set( LIBS_CLIENT ${LIBS_CLIENT} INTERNAL_SDL INTERNAL_SDLMain )
    set( LIBS_RENDERER ${LIBS_RENDERER} INTERNAL_SDL )
    set( LIBS_RENDERERGL ${LIBS_RENDERERGL} INTERNAL_SDL )
  endif()

  # Used by tinygettext to use SDL_iconv instead of base iconv
  if( WIN32 )
    add_definitions( -DHAVE_SDL )
  endif()
endif()

# GMP
find_package( GMP ${OPTIONAL_WIN32} )
if( GMP_FOUND )
  include_directories( ${GMP_INCLUDE_DIR} )
  set( LIBS_ENGINE ${LIBS_ENGINE} ${GMP_LIBRARIES} )
else()
  include_directories( ${LIB_DIR}/gmp/include )
  if( ARCH STREQUAL "x86" )
    set( LIBS_ENGINE ${LIBS_ENGINE} ${LIB_DIR}/GMP/libs/win32/mpir.lib )
  elseif( ARCH STREQUAL "x86_64" )
    set( LIBS_ENGINE ${LIBS_ENGINE} ${LIB_DIR}/GMP/libs/win64/mpir.lib )
  endif()
endif()

# Nettle
find_package( Nettle )
if( NETTLE_FOUND )
  include_directories( ${NETTLE_INCLUDE_DIR} )
  set( LIBS_ENGINE ${LIBS_ENGINE} ${NETTLE_LIBRARIES} )
else()
  include_directories( ${LIB_DIR}/nettle )
  set( LIBSRC_ENGINE ${LIBSRC_ENGINE} ${NETTLELIST} )
endif()

# Speex
if( USE_VOIP )
  find_package( Speex )
  if( SPEEX_FOUND )
    include_directories( ${SPEEX_INCLUDE_DIR} )
    set( LIBS_CLIENT ${LIBS_CLIENT} ${SPEEX_LIBRARIES} )
  else()
    add_definitions( -DFLOATING_POINT )
    include_directories( ${LIB_DIR}/libspeex/include )
    set( LIBSRC_CLIENT ${LIBSRC_CLIENT} ${SPEEXLIST} )
  endif()
endif()

# Mumble
if( USE_MUMBLE )
  add_definitions( -DUSE_MUMBLE )
endif()

# Ogg
find_package( Ogg ${OPTIONAL_WIN32} )
if( OGG_FOUND )
  include_directories( ${OGG_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${OGG_LIBRARIES} )
else()
  include_directories( ${LIB_DIR}/libogg/include )
  if( ARCH STREQUAL "x86" )
    set( LIBS_CLIENT ${LIBS_CLIENT} ${LIB_DIR}/libogg/libs/win32/release/libogg.lib )
  elseif( ARCH STREQUAL "x86_64" )
    set( LIBS_CLIENT ${LIBS_CLIENT} ${LIB_DIR}/libogg/libs/win64/release/libogg.lib )
  endif()
endif()

# Vorbis
add_definitions( -DUSE_CODEC_VORBIS )
find_package( Vorbis ${OPTIONAL_WIN32} )
if( VORBIS_FOUND )
  include_directories( ${VORBIS_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${VORBIS_LIBRARIES} )
else()
  include_directories( ${LIB_DIR}/libvorbis/include )
  if( ARCH STREQUAL "x86" )
    set( LIBS_CLIENT ${LIBS_CLIENT}
      ${LIB_DIR}/libvorbis/libs/Win32/Release/libvorbis.lib
      ${LIB_DIR}/libvorbis/libs/Win32/Release/libvorbisfile.lib
    )
  elseif( ARCH STREQUAL "x86_64" )
    set( LIBS_CLIENT ${LIBS_CLIENT}
      ${LIB_DIR}/libvorbis/libs/x64/Release/libvorbis.lib
      ${LIB_DIR}/libvorbis/libs/x64/Release/libvorbisfile.lib
    )
  endif()
endif()

# Theora
add_definitions( -DUSE_CIN_THEORA )
find_package( Theora ${OPTIONAL_WIN32} )
if( THEORA_FOUND )
  include_directories( ${THEORA_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${THEORA_LIBRARIES} )
else()
  include_directories( ${LIB_DIR}/libtheora/include )
  if( ARCH STREQUAL "x86" )
    set( LIBS_CLIENT ${LIBS_CLIENT} ${LIB_DIR}/libtheora/libs/win32/release/libtheora.lib )
  elseif( ARCH STREQUAL "x86_64" )
    set( LIBS_CLIENT ${LIBS_CLIENT} ${LIB_DIR}/libtheora/libs/win64/release/libtheora.lib )
  endif()
endif()

# WebP
find_package( WebP )
if( WEBP_FOUND )
  include_directories( ${WEBP_INCLUDE_DIRS} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${WEBP_LIBRARIES} )
else()
  add_subdirectory( ${LIB_DIR}/libwebp ${OBJ_DIR}/libwebp )
  include_directories( ${LIB_DIR}/libwebp )
  set( LIBS_RENDERER ${LIBS_RENDERER} INTERNAL_WEBP )
endif()

# JPEG
find_package( JPEG )
if( JPEG_FOUND )
  include_directories( ${JPEG_INCLUDE_DIR} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${JPEG_LIBRARIES} )
else()
  add_subdirectory( ${LIB_DIR}/jpeg ${OBJ_DIR}/libjpeg )
  include_directories( ${LIB_DIR}/jpeg )
  set( LIBSRC_RENDERER ${LIBSRC_RENDERER} ${JPEGLIST} )
endif()

# PNG
find_package( PNG ${OPTIONAL_WIN32} )
if( PNG_FOUND )
  include_directories( ${PNG_INCLUDE_DIRS} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${PNG_LIBRARIES} )
else()
  include_directories( ${LIB_DIR}/libpng )
  if( ARCH STREQUAL "x86" )
    set( LIBS_RENDERER ${LIBS_RENDERER} ${LIB_DIR}/libpng/libs/win32/libpng.lib )
  elseif( ARCH STREQUAL "x86_64" )
    set( LIBS_RENDERER ${LIBS_RENDERER} ${LIB_DIR}/libpng/libs/win32/libpng.lib )
  endif()
endif()

# Freetype
find_package( Freetype ${OPTIONAL_WIN32} )
if( FREETYPE_FOUND )
  include_directories( ${FREETYPE_INCLUDE_DIRS} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${FREETYPE_LIBRARIES} )
else()
  include_directories( ${LIB_DIR}/freetype/include/freetype2 )
  if( ARCH STREQUAL "x86" )
    set( LIBS_RENDERER ${LIBS_RENDERER} ${LIB_DIR}/freetype/lib/freetype.lib )
  elseif( ARCH STREQUAL "x86_64" )
    set( LIBS_RENDERER ${LIBS_RENDERER} ${LIB_DIR}/freetype/lib/x64/freetype.lib )
  endif()
endif()

# GLEW
find_package( GLEW )
if( GLEW_FOUND )
  include_directories( ${GLEW_INCLUDE_DIR} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${GLEW_LIBRARIES} )
else()
  add_subdirectory( ${LIB_DIR}/glew ${OBJ_DIR}/libglew )
  include_directories( ${LIB_DIR}/glew/include )
  set( LIBS_RENDERER ${LIBS_RENDERER} INTERNAL_GLEW )
endif()

# OpenGL
find_package( OpenGL REQUIRED )
include_directories( ${OPENGL_INCLUDE_DIR} )
set( LIBS_RENDERER ${LIBS_RENDERER} ${OPENGL_LIBRARIES} )

# X11
if( LINUX )
  find_package( X11 REQUIRED )
  include_directories( ${X11_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${X11_LIBRARIES} )
endif()

# OpenAL
find_package( OpenAL )
if( OPENAL_FOUND )
  include_directories( ${OPENAL_INCLUDE_DIR} )
else()
  include_directories( ${LIB_DIR}/openal/include )
endif()


# CURL
find_package( CURL ${OPTIONAL_WIN32} )
if( CURL_FOUND )
  include_directories( ${CURL_INCLUDE_DIRS} )
  set( LIBS_CLIENTBASE ${LIBS_CLIENTBASE} ${CURL_LIBRARIES} )
else()
  include_directories( ${LIB_DIR}/curl-7.21.6/include )
  if( ARCH STREQUAL "x86" )
    set( LIBS_CLIENTBASE ${LIBS_CLIENTBASE} ${LIB_DIR}/curl-7.21.6/lib/win32/release/libcurl_imp.lib )
  elseif( ARCH STREQUAL "x86_64" )
    set( LIBS_CLIENTBASE ${LIBS_CLIENTBASE} ${LIB_DIR}/curl-7.21.6/lib/win64/release/libcurl_imp.lib )
  endif()
endif()

# Curses, disabled for Windows
if( USE_CURSES AND NOT WIN32 )
  set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
  find_package( CursesW )
  if( CURSESW_FOUND )
    add_definitions( -DUSE_CURSES -DUSE_CURSES_W )
    include_directories( ${CURSESW_INCLUDE_DIR} )
    set( LIBS_ENGINE ${LIBS_ENGINE} ${CURSESW_LIBRARIES} )
  else()
    find_package( Curses REQUIRED )
    add_definitions( -DUSE_CURSES )
    include_directories( ${CURSES_INCLUDE_DIR} )
    set( LIBS_ENGINE ${LIBS_ENGINE} ${CURSES_LIBRARIES} )
  endif()
endif()

################################################################################
# Installer
################################################################################

# Setup CPack
set( CPACK_PACKAGE_FILE_NAME "Unvanquished" )
set( CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpack/OS_X/Unvanquished.icns" )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Daemon Unvanquished client" )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.md" )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/GPL.txt" )
# Mac OS X Bundle specific settings
set( CPACK_BUNDLE_NAME ${CPACK_PACKAGE_FILE_NAME} )
set( CPACK_BUNDLE_ICON ${CPACK_PACKAGE_ICON} )
set( CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/cpack/OS_X/Info.plist" )

# Don't worry about installing targets individually just copy entire folder over
# This allows maps/.pk3s to be easily added to the installer
install( DIRECTORY ${CMAKE_BINARY_DIR}/main DESTINATION "Unvanquished" )

################################################################################
# Game logic
################################################################################

# Function to setup all the Game/Cgame/UI libraries
include( CMakeParseArguments )
function( GAMELIB )
  # ParseArguments setup
  set( oneValueArgs LIB_NAME OUTPUT_DIR )
  set( multiValueArgs COMPILE_DEF LIB_FILES )
  cmake_parse_arguments( GAMELIB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if( BUILD_GAME_NATIVE )
    # Xcode places the game libraries in the incorrect location. This should fix it
    if( XCODE_VERSION OR MSVC )
      set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "Release/${GAMELIB_OUTPUT_DIR}" )
      set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "Debug/${GAMELIB_OUTPUT_DIR}" )
      set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "MinSizeRel/${GAMELIB_OUTPUT_DIR}" )
      set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "RelWithDebInfo/${GAMELIB_OUTPUT_DIR}" )
    endif()
    set( GAMELIB_COMPILE_DEF_NATIVE ${GAMELIB_COMPILE_DEF} )

    # HACK: Compile shared files as C, uncomment this when building game
    #set_source_files_properties( ${SHAREDLIST} PROPERTIES LANGUAGE C )
    add_executable( ${GAMELIB_LIB_NAME} ${GAMELIB_LIB_FILES} ${SHAREDLIST} ${NACL_LIST_NATIVE} ${ENGINE_DIR}/qcommon/nacl.cpp )
    target_link_libraries( ${GAMELIB_LIB_NAME} ${LIBS_BASE} )
    set_target_properties( ${GAMELIB_LIB_NAME} PROPERTIES
      RUNTIME_OUTPUT_DIRECTORY ${GAMELIB_OUTPUT_DIR}
      COMPILE_DEFINITIONS "${GAMELIB_COMPILE_DEF}"
    )
  endif()

endfunction()

GAMELIB( LIB_NAME game
  OUTPUT_DIR main
  COMPILE_DEF GAME
  LIB_FILES ${GAMEGPP_BASE}
)

################################################################################
# Engine
################################################################################

if( BUILD_CLIENT )
  # Setup target client
  add_executable( client ${SERVERLIST} ${CLIENTLIST} ${SDLLIST} ${ENGINELIST} ${SHAREDLIST}
    ${LIBSRC_BASE} ${LIBSRC_ENGINE} ${LIBSRC_CLIENTBASE} ${LIBSRC_CLIENT} ${WIN_RC} )
  target_link_libraries( client ${LIBS_BASE} ${LIBS_ENGINE} ${LIBS_CLIENTBASE} ${LIBS_CLIENT} )
  if( USE_VOIP )
    # Set it here so the tty client doesn't try to handle voip
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_VOIP )
  endif()
  set_target_properties( client PROPERTIES OUTPUT_NAME "daemon" PREFIX "" ENABLE_EXPORTS ON )

  # Setup CPack for target client
  install( TARGETS client DESTINATION "Unvanquished" )
  get_target_property( LOCATION client LOCATION )
  get_filename_component( OUTPUT_NAME ${LOCATION} NAME )
  set( APPS ${APPS} Unvanquished/${OUTPUT_NAME} )

  # Amanieu: Not sure if this is required
  #if( MINGW )
  #  target_link_libraries( client ${SDLMAIN_LIBRARY} )
  #endif()
endif()

if( BUILD_SERVER )
  # Setup the target Server
  add_executable( server ${SERVERLIST} ${ENGINELIST} ${SHAREDLIST} ${LIBSRC_BASE} ${LIBSRC_ENGINE}
    ${ENGINE_DIR}/null/null_client.c  ${ENGINE_DIR}/null/null_input.c
    ${ENGINE_DIR}/null/null_snddma.c ${WIN_RC} )
  target_link_libraries( server ${LIBS_BASE} ${LIBS_ENGINE} )
  set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS DEDICATED )
  set_target_properties( server PROPERTIES OUTPUT_NAME "daemonded" PREFIX "" )

  # Setup CPack for target server
  install( TARGETS server DESTINATION "Unvanquished" )
  get_target_property( LOCATION server LOCATION )
  get_filename_component( OUTPUT_NAME ${LOCATION} NAME )
  set( APPS ${APPS} Unvanquished/${OUTPUT_NAME} )
endif()

if( BUILD_TTY_CLIENT )
  # Setup the target ttyclient
  add_executable( ttyclient ${SERVERLIST} ${CLIENTLIST} ${ENGINELIST} ${SHAREDLIST}
    ${LIBSRC_BASE} ${LIBSRC_ENGINE} ${LIBSRC_CLIENTBASE} ${WIN_RC}
    ${ENGINE_DIR}/null/null_input.c ${ENGINE_DIR}/null/null_snddma.c ${ENGINE_DIR}/null/null_renderer.c )
  target_link_libraries( ttyclient ${LIBS_BASE} ${LIBS_ENGINE} ${LIBS_CLIENTBASE} )
  set_property( TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS BUILD_TTY_CLIENT REF_HARD_LINKED )
  set_target_properties( ttyclient PROPERTIES OUTPUT_NAME "daemon-tty" PREFIX "" )

  # Setup CPack for target ttyclient
  install( TARGETS ttyclient DESTINATION "Unvanquished" )
  get_target_property( LOCATION ttyclient LOCATION )
  get_filename_component( OUTPUT_NAME ${LOCATION} NAME )
  set( APPS ${APPS} Unvanquished/${OUTPUT_NAME} )
endif()

################################################################################
# Renderers
################################################################################

if( BUILD_OLD_RENDERER )
  # Setup target renderer
  add_library( renderer MODULE ${RENDERERLIST} ${SHAREDLIST} ${LIBSRC_BASE} ${LIBSRC_RENDERER}
    ${ENGINE_DIR}/sys/sdl_glimp.c ${ENGINE_DIR}/sys/sdl_gamma.c )
  target_link_libraries( renderer ${LIBS_BASE} ${LIBS_RENDERER} client )
  set_target_properties( renderer PROPERTIES OUTPUT_NAME "rendererGL" )

  # Setup CPack for target renderer
  install( TARGETS renderer DESTINATION "Unvanquished" )
  get_target_property( LOCATION renderer LOCATION )
  get_filename_component( OUTPUT_NAME ${LOCATION} NAME )
  set( PLUGINS ${PLUGINS} "\${CMAKE_INSTALL_PREFIX}/Unvanquished/${OUTPUT_NAME}" )
endif()

if( BUILD_NEW_RENDERER )
  # Setup target rendererGL
  add_library( rendererGL MODULE ${RENDERERGLLIST} ${SHAREDLIST} ${LIBSRC_BASE} ${LIBSRC_RENDERER}
    ${ENGINE_DIR}/sys/sdl_glimp.c ${ENGINE_DIR}/sys/sdl_gamma.c )
  set_property( TARGET rendererGL APPEND PROPERTY COMPILE_DEFINITIONS USE_XREAL_RENDERER )
  target_link_libraries( rendererGL ${LIBS_BASE} ${LIBS_RENDERER} client )
  set_target_properties( rendererGL PROPERTIES OUTPUT_NAME "rendererGL3" )

  # Setup CPack for target rendererGL
  install( TARGETS rendererGL DESTINATION "Unvanquished" )
  get_target_property( LOCATION rendererGL LOCATION )
  get_filename_component( OUTPUT_NAME ${LOCATION} NAME )
  set( PLUGINS ${PLUGINS} "\${CMAKE_INSTALL_PREFIX}/Unvanquished/${OUTPUT_NAME}" )
endif()

################################################################################
# Installer
################################################################################

if( NOT MSVC )
  execute_process( COMMAND sed -e "/^#define PRODUCT_VERSION/! d; s/.*\"\\(.*\\)\\.[^.]\"/\\1/" ${CMAKE_SOURCE_DIR}/src/engine/qcommon/q_shared.h
                   OUTPUT_VARIABLE PRODUCT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE )
  add_custom_target( download-pk3.sh ALL
    sed -e 's/VERSION=/&${PRODUCT_VERSION}/' ${CMAKE_SOURCE_DIR}/download-pk3.sh.in >${CMAKE_BINARY_DIR}/download-pk3.sh\; chmod +x ${CMAKE_BINARY_DIR}/download-pk3.sh
    DEPENDS ${CMAKE_SOURCE_DIR}/src/engine/qcommon/q_shared.h ${CMAKE_SOURCE_DIR}/download-pk3.sh.in
  )
endif()

# This section requires a lot of escaping as these values are parsed then passed
# to CPack. Be very very careful when modifying values

if( ENABLE_BUNDLING )
  # Automatically copy over any dependencies
  foreach( APP ${APPS} )
    install( CODE "
       include( BundleUtilities )
       message( \"\${CMAKE_INSTALL_PREFIX}/${APP}\" )
       fixup_bundle( \"\${CMAKE_INSTALL_PREFIX}/${APP}\" \"${PLUGINS}\" \"${CMAKE_LIBRARY_PATH}\" )
       " )
  endforeach()
endif()

# include( CPack ) must be at the end of CMakeLists.txt
include( CPack )
