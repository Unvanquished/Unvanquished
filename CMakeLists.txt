#
# CMakeLists.txt
#
# cmake required (http://www.cmake.org/)
#

cmake_minimum_required( VERSION 2.8 )
if( POLICY CMP0017 )
  cmake_policy( SET CMP0017 NEW )
endif()

# Need 2.8.12 for RPATH support
if( APPLE )
  cmake_minimum_required( VERSION 2.8.12 )
endif()

include( CMakeDependentOption )

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set( MSVC_INCREMENTAL_DEFAULT ON )

# Default to Release builds. To use system CFLAGS only (for distro builds), set CMAKE_BUILD_TYPE to None
set( CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." )

project( Daemon C CXX )

################################################################################
# Configuration options
################################################################################

option( ENABLE_LTO "Use link-time optimization for release builds" 0 )
option( ENABLE_HARDENING "Use stack protection and other hardening flags" 0 )

option( BUILD_CLIENT "Build Daemon client" 1 )
option( BUILD_SERVER "Build Daemonded server" 1 )
option( BUILD_GAME_NACL_NATIVE "Build native NaCl game logic" 0)
option( BUILD_GAME_NACL "Build NaCl game logic" 0)
option( BUILD_GAME_QVM_NATIVE "Build native game logic " 1)
option( BUILD_GAME_QVM "Build QVM game logic" 1)
option( BUILD_TTY_CLIENT "Build Daemon headless client" 1 )
cmake_dependent_option( BUILD_OLD_RENDERER "Build old renderer library" 1 BUILD_CLIENT 0 )
cmake_dependent_option( BUILD_NEW_RENDERER "Build new renderer library" 1 BUILD_CLIENT 0 )

cmake_dependent_option( USE_MUMBLE "Compile with support for Mumble" 1 BUILD_CLIENT 0 )
cmake_dependent_option( USE_VOIP "Compile with VOIP support" 1 "BUILD_CLIENT OR BUILD_SERVER OR BUILD_TTY_CLIENT" 0 )
cmake_dependent_option( USE_CURSES "Use the curses console" 1 "BUILD_CLIENT OR BUILD_SERVER OR BUILD_TTY_CLIENT" 0 )
cmake_dependent_option( USE_OPENAL_DLOPEN "Use dlopen to load OpenAL" 0 BUILD_CLIENT 0 )
cmake_dependent_option( USE_SMP "Compile with support for running the renderer in a separate thread" 1 BUILD_CLIENT 0 )
cmake_dependent_option( USE_GEOIP "Use libgeoip" 1 "BUILD_SERVER OR BUILD_CLIENT OR BUILD_TTY_CLIENT" 0 )
option( USE_SDL2 "Enable SDL 2.0 support (incompatible with SDL 1.2)" 1 )
option( USE_WEXTRA "Enable extra compilation warnings" 0 )
option( USE_PEDANTIC "Tell the compiler to be pedantic" 0 )

set( NACL_SDK_PATH "" CACHE PATH "Path to native client SDK" )
if( BUILD_GAME_NACL AND NOT NACL_SDK_PATH )
  message( SEND_ERROR "You need to set the path to the native client SDK" )
endif()

################################################################################
# Determine platform
################################################################################

# When adding a new platform, look at all the places WIN32, APPLE and LINUX are used
if( CMAKE_SYSTEM_NAME MATCHES "Linux" )
  set( LINUX ON )
elseif( NOT WIN32 AND NOT APPLE )
  message( FATAL_ERROR "Platform not supported" )
endif()

################################################################################
# Determine architecture
################################################################################

# When adding a new architecture, look at all the places ARCH is used
if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
  set( ARCH "x86_64" )
else()
  set( ARCH "x86" )
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
macro( try_cxx_flag PROP FLAG )
  check_CXX_compiler_flag( ${FLAG} FLAG_${PROP} )
  if( FLAG_${PROP} )
    set_cxx_flag( ${FLAG} ${ARGV2} )
  endif()
endmacro()
macro( try_c_cxx_flag PROP FLAG )
  # Only try the flag once on the C++ compiler
  try_cxx_flag( ${PROP} ${FLAG} ${ARGV2} )
  if( FLAG_${PROP} )
    set_c_flag( ${FLAG} ${ARGV2} )
  endif()
endmacro()
macro( try_linker_flag PROP FLAG )
  # Check it with the C compiler
  set( CMAKE_REQUIRED_FLAGS ${FLAG} )
  check_C_compiler_flag( ${FLAG} FLAG_${PROP} )
  set( CMAKE_REQUIRED_FLAGS "" )
  if( FLAG_${PROP} )
    set_linker_flag( ${FLAG} ${ARGN} )
  endif()
endmacro()

if( MSVC )
  set_c_cxx_flag( "/MP" )
  set_c_cxx_flag( "/fp:fast" )
  set_c_cxx_flag( "/d2Zi+" RELWITHDEBINFO )
  if( ARCH STREQUAL "x86" )
    set_c_cxx_flag( "/arch:SSE2" )
  endif()
  if( ENABLE_LTO )
    set_c_cxx_flag( "/GL" RELEASE )
    set_linker_flag( "/LTCG" )
  endif()
else()
  set_c_cxx_flag( "-ffast-math" )
  set_c_cxx_flag( "-fno-strict-aliasing" )
  set_c_cxx_flag( "-fomit-frame-pointer" RELEASE )
  set_c_cxx_flag( "-fomit-frame-pointer" MINSIZEREL )

  # Set arch on x86 to SSE2 minimum and enable CMPXCHG16B
  if( ARCH STREQUAL "x86" )
    set_c_cxx_flag( "-m32" )
    set_c_cxx_flag( "-msse2" )
    set_c_cxx_flag( "-mtune=generic" )

    # Clang prints a warning when if it doesn't support -mfpmath=sse
    set( CMAKE_REQUIRED_FLAGS "-Werror" )
    set_c_cxx_flag( "-mfpmath=sse" )
    set( CMAKE_REQUIRED_FLAGS "" )
  elseif( ARCH STREQUAL "x86_64" )
    set_c_cxx_flag( "-m64" )
    set_c_cxx_flag( "-mtune=generic" )

    # Clang prints a warning when if it doesn't support -mcx16
    set( CMAKE_REQUIRED_FLAGS "-Werror" )
    try_c_cxx_flag( MCX16 "-mcx16" )
    set( CMAKE_REQUIRED_FLAGS "" )
  endif()

  # Use hidden symbol visibility if possible
  try_c_cxx_flag( FVISIBILITY_HIDDEN "-fvisibility=hidden" )

  # Extra debug flags
  set_c_cxx_flag( "-g3" DEBUG )
  set_c_cxx_flag( "-g3" RELWITHDEBINFO )
  try_c_cxx_flag( OPTIMIZE_DEBUG "-Og" DEBUG )

  # C++11 support
  try_cxx_flag( GNUXX11 "-std=gnu++11" )
  if( NOT FLAG_GNUXX11 )
    try_cxx_flag( GNUXX0X "-std=gnu++0x" )
    if( NOT FLAG_GNUXX0X )
      message( FATAL_ERROR "C++11 not supported by compiler" )
    endif()
  endif()

  # Use MSVC-compatible bitfield layout
  if( WIN32 )
    set_c_cxx_flag( "-mms-bitfields" )
  endif()

  # Use libc++ on Mac because the shipped libstdc++ version is too old
  if( APPLE )
    set_c_cxx_flag( "-stdlib=libc++" )
  endif()

  # Hardening, don't set _FORTIFY_SOURCE in debug builds
  set_c_cxx_flag( "-D_FORTIFY_SOURCE=2" RELEASE )
  set_c_cxx_flag( "-D_FORTIFY_SOURCE=2" RELWITHDEBINFO )
  set_c_cxx_flag( "-D_FORTIFY_SOURCE=2" MINSIZEREL )
  if( ENABLE_HARDENING )
    try_c_cxx_flag( FNO_STRICT_OVERFLOW "-fno-strict-overflow" )
    try_c_cxx_flag( FSTACK_PROTECTOR_ALL "-fstack-protector-all" )
    try_c_cxx_flag( PARAM_SSP_BUFFER_SIZE "--param ssp-buffer-size=1" )
    try_c_cxx_flag( WSTACK_PROTECTOR "-Wstack-protector" )
    try_c_cxx_flag( FPIE "-fPIE" )
    try_linker_flag( LINKER_PIE "-pie" )
  endif()

  # Linker flags
  try_linker_flag( LINKER_O1 "-Wl,-O1" )
  try_linker_flag( LINKER_SORT_COMMON "-Wl,--sort-common" )
  try_linker_flag( LINKER_AS_NEEDED "-Wl,--as-needed" )
  try_linker_flag( LINKER_NO_UNDEFINED "-Wl,--no-undefined" )
  try_linker_flag( LINKER_Z_RELRO "-Wl,-z,relro" )
  try_linker_flag( LINKER_Z_NOW "-Wl,-z,now" )
  if( WIN32 )
    try_linker_flag( LINKER_DYNAMICBASE "-Wl,--dynamicbase" )
    try_linker_flag( LINKER_NXCOMPAT "-Wl,--nxcompat" )
  endif()

  # LTO support, only used on release builds
  if( ENABLE_LTO )
    # This flag is also used when linking
    set_c_cxx_flag( "-flto" RELEASE )
  endif()

  # The -pthread flag sets some preprocessor defines
  try_c_cxx_flag( PTHREAD "-pthread" )

  # Warning options
  set_c_cxx_flag( "-Wall" )
  if( USE_WEXTRA )
    set_c_cxx_flag( "-Wextra" )
  else()
    set_c_cxx_flag( "-Wno-sign-compare" )
    set_c_cxx_flag( "-Wno-write-strings" )
  endif()

  if( USE_PEDANTIC )
    set_c_cxx_flag("-pedantic")
  endif()

endif()

# Windows-specific definitions
if( WIN32 )
  # Minimum Windows version: XP
  # Define WIN32 for compatibility (compiler defines _WIN32)
  add_definitions( -DWINVER=0x501 -DWIN32 )
  set( CMAKE_FIND_LIBRARY_PREFIXES ${CMAKE_FIND_LIBRARY_PREFIXES} "" "lib" )
endif()
if( MSVC )
  add_definitions( -D_CRT_SECURE_NO_WARNINGS )
endif()

# Mac-specific definitions
if( APPLE )
  add_definitions( -DMACOS_X )
  set_linker_flag( "-Wl,-no_pie" )
  set( CMAKE_INSTALL_RPATH "@executable_path/" )
  set( CMAKE_BUILD_WITH_INSTALL_RPATH ON )
endif()

# Mumble link support
if( USE_MUMBLE )
  add_definitions( -DUSE_MUMBLE )
endif()

################################################################################
# Directories
################################################################################

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
set( MOUNT_DIR ${CMAKE_SOURCE_DIR}/src )
set( OBJ_DIR ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY} )
set( ENGINE_DIR ${MOUNT_DIR}/engine )
set( COMMON_DIR ${MOUNT_DIR}/common )
set( GAMELOGIC_DIR ${MOUNT_DIR}/gamelogic )
set( TOOLS_DIR ${MOUNT_DIR}/tools )
set( LCC_DIR ${TOOLS_DIR}/lcc )
set( LIB_DIR ${MOUNT_DIR}/libs )
if( CMAKE_HOST_WIN32 )
  set( PNACL_TOOLCHAIN ${NACL_SDK_PATH}/toolchain/win_pnacl/bin )
  set( PNACL_BIN_EXT ".bat" )
elseif( CMAKE_HOST_APPLE )
  set( PNACL_TOOLCHAIN ${NACL_SDK_PATH}/toolchain/mac_pnacl/bin )
  set( PNACL_BIN_EXT "" )
elseif( CMAKE_HOST_SYSTEM_NAME MATCHES "Linux" )
  if( CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "x86_64" OR CMAKE_HOST_SYSTEM_PROCESSOR STREQUAL "amd64" )
    set( PNACL_TOOLCHAIN ${NACL_SDK_PATH}/toolchain/linux_pnacl/bin64 )
  else()
    set( PNACL_TOOLCHAIN ${NACL_SDK_PATH}/toolchain/linux_pnacl/bin )
  endif()
  set( PNACL_BIN_EXT "" )
endif()
if( MSVC )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/msvc64 )
  else()
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/msvc32 )
  endif()
elseif( WIN32 )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/mingw64 )
  else()
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/mingw32 )
  endif()
elseif( APPLE )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/macosx64 )
  else()
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/macosx32 )
  endif()
endif()

################################################################################
# Git revision info
################################################################################

if( EXISTS "${CMAKE_SOURCE_DIR}/.git" )
  find_package( Git )
  if( GIT_FOUND )
    execute_process( COMMAND ${GIT_EXECUTABLE} describe --tags --long --match v* --dirty=+dirty OUTPUT_VARIABLE GIT_DESCRIBE_REPORT OUTPUT_STRIP_TRAILING_WHITESPACE )
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
  ${ENGINE_DIR}/qcommon/q_shared.h
  ${ENGINE_DIR}/qcommon/q_unicode.h
  ${ENGINE_DIR}/qcommon/unicode_data.h
  ${ENGINE_DIR}/qcommon/logging.h
)

set( RENDERERLIST
  ${ENGINE_DIR}/sys/sdl2_compat.c
  ${ENGINE_DIR}/sys/sdl_glimp.c
  ${ENGINE_DIR}/sys/sdl_gamma.c
  ${ENGINE_DIR}/renderer/tr_animation.c
  ${ENGINE_DIR}/renderer/tr_animation_mds.c
  ${ENGINE_DIR}/renderer/tr_backend.c
  ${ENGINE_DIR}/renderer/tr_bonematrix.c
  ${ENGINE_DIR}/renderer/tr_bonematrix.h
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
  ${ENGINE_DIR}/renderer/tr_local.h
  ${ENGINE_DIR}/renderer/tr_types.h
  ${ENGINE_DIR}/renderer/tr_public.h
  ${ENGINE_DIR}/renderer/anorms256.h
)

set( RENDERERGLLIST
  ${ENGINE_DIR}/sys/sdl2_compat.c
  ${ENGINE_DIR}/sys/sdl_glimp.c
  ${ENGINE_DIR}/sys/sdl_gamma.c
  ${ENGINE_DIR}/rendererGL/gl_shader.cpp
  ${ENGINE_DIR}/rendererGL/tr_animation.c
  ${ENGINE_DIR}/rendererGL/tr_backend.cpp
  ${ENGINE_DIR}/renderer/tr_bonematrix.c
  ${ENGINE_DIR}/renderer/tr_bonematrix.h
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
  ${ENGINE_DIR}/rendererGL/tr_local.h
  ${ENGINE_DIR}/renderer/tr_public.h
  ${ENGINE_DIR}/rendererGL/gl_shader.h
  ${ENGINE_DIR}/rendererGL/tr_image.h
  ${ENGINE_DIR}/rendererGL/tr_model_skel.h
)

set( SERVERLIST
  ${ENGINE_DIR}/botlib/bot_convert.cpp
  ${ENGINE_DIR}/botlib/bot_local.cpp
  ${ENGINE_DIR}/botlib/bot_nav.cpp
  ${ENGINE_DIR}/botlib/bot_load.cpp
  ${ENGINE_DIR}/server/sv_bot.c
  ${ENGINE_DIR}/server/sv_ccmds.c
  ${ENGINE_DIR}/server/sv_client.c
  ${ENGINE_DIR}/server/sv_game.c
  ${ENGINE_DIR}/server/sv_init.c
  ${ENGINE_DIR}/server/sv_main.c
  ${ENGINE_DIR}/server/sv_net_chan.c
  ${ENGINE_DIR}/server/sv_snapshot.c
  ${ENGINE_DIR}/server/sv_world.c
  ${ENGINE_DIR}/server/server.h
  ${ENGINE_DIR}/server/g_api.h
  ${ENGINE_DIR}/qcommon/print_translated.h
)

set( ENGINELIST
  ${COMMON_DIR}/Command.cpp
  ${COMMON_DIR}/Command.h
  ${COMMON_DIR}/Cvar.cpp
  ${COMMON_DIR}/Cvar.h
  ${COMMON_DIR}/LineEditData.cpp
  ${COMMON_DIR}/LineEditData.h
  ${COMMON_DIR}/Log.cpp
  ${COMMON_DIR}/Log.h
  ${COMMON_DIR}/String.cpp
  ${COMMON_DIR}/String.h
  ${ENGINE_DIR}/framework/BaseCommands.cpp
  ${ENGINE_DIR}/framework/BaseCommands.h
  ${ENGINE_DIR}/framework/CommandSystem.cpp
  ${ENGINE_DIR}/framework/CommandSystem.h
  ${ENGINE_DIR}/framework/CvarSystem.cpp
  ${ENGINE_DIR}/framework/CvarSystem.h
  ${ENGINE_DIR}/framework/ConsoleField.cpp
  ${ENGINE_DIR}/framework/ConsoleField.h
  ${ENGINE_DIR}/framework/ConsoleHistory.cpp
  ${ENGINE_DIR}/framework/ConsoleHistory.h
  ${ENGINE_DIR}/framework/LogSystem.cpp
  ${ENGINE_DIR}/framework/LogSystem.h
  ${ENGINE_DIR}/framework/VirtualMachine.cpp
  ${ENGINE_DIR}/framework/VirtualMachine.h
  ${ENGINE_DIR}/qcommon/cm_load.c
  ${ENGINE_DIR}/qcommon/cm_trisoup.c
  ${ENGINE_DIR}/qcommon/cm_patch.c
  ${ENGINE_DIR}/qcommon/cm_plane.c
  ${ENGINE_DIR}/qcommon/cm_polylib.c
  ${ENGINE_DIR}/qcommon/cm_test.c
  ${ENGINE_DIR}/qcommon/cm_trace.c
  ${ENGINE_DIR}/qcommon/cm_local.h
  ${ENGINE_DIR}/qcommon/cm_patch.h
  ${ENGINE_DIR}/qcommon/cm_polylib.h
  ${ENGINE_DIR}/qcommon/cm_public.h
  ${ENGINE_DIR}/qcommon/cmd.c
  ${ENGINE_DIR}/qcommon/common.c
  ${ENGINE_DIR}/qcommon/crypto.c
  ${ENGINE_DIR}/qcommon/crypto.h
  ${ENGINE_DIR}/qcommon/cvar.c
  ${ENGINE_DIR}/qcommon/cvar.h
  ${ENGINE_DIR}/qcommon/files.c
  ${ENGINE_DIR}/qcommon/qfiles.h
  ${ENGINE_DIR}/qcommon/htable.c
  ${ENGINE_DIR}/qcommon/htable.h
  ${ENGINE_DIR}/qcommon/huffman.c
  ${ENGINE_DIR}/qcommon/md4.c
  ${ENGINE_DIR}/qcommon/md4.h
  ${ENGINE_DIR}/qcommon/md5.c
  ${ENGINE_DIR}/qcommon/msg.c
  ${ENGINE_DIR}/qcommon/net_chan.c
  ${ENGINE_DIR}/qcommon/net_ip.c
  ${ENGINE_DIR}/qcommon/parse.c
  ${ENGINE_DIR}/qcommon/translation.cpp
  ${ENGINE_DIR}/qcommon/vm.c
  ${ENGINE_DIR}/qcommon/vm_interpreted.c
  ${ENGINE_DIR}/qcommon/vm_local.h
  ${ENGINE_DIR}/qcommon/vm_traps.h
  ${ENGINE_DIR}/sys/sys_main.c
  ${ENGINE_DIR}/sys/con_log.c
  ${ENGINE_DIR}/qcommon/qcommon.h
  ${ENGINE_DIR}/qcommon/surfaceflags.h
)

if( ARCH STREQUAL "x86" OR ARCH STREQUAL "x86_64" )
  set( ENGINELIST ${ENGINELIST} ${ENGINE_DIR}/qcommon/vm_x86.c )
  if( MSVC AND CMAKE_CL_64 )
    add_custom_command( OUTPUT vm_x86_64.obj COMMAND ml64 ARGS /c "${ENGINE_DIR}/qcommon/vm_x86_64.asm" MAIN_DEPENDENCY ${ENGINE_DIR}/qcommon/vm_x86_64.asm )
    set( ENGINELIST ${ENGINELIST} vm_x86_64.obj )
  endif()
endif()

if( WIN32 )
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/sys_win32.c
    ${ENGINE_DIR}/sys/sys_win32.h
    ${ENGINE_DIR}/sys/con_passive.c
  )
else()
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/sys_unix.c
    ${ENGINE_DIR}/sys/con_tty.c
  )
endif()

if( APPLE )
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/sys_osx.mm
  )
endif()

if( USE_CURSES )
  set( ENGINELIST ${ENGINELIST} ${ENGINE_DIR}/sys/con_curses.c )
endif()

set( CLIENTLIST
  ${ENGINE_DIR}/botlib/bot_debug.cpp
  ${ENGINE_DIR}/botlib/bot_nav_edit.cpp
  ${ENGINE_DIR}/client/cg_api.h
  ${ENGINE_DIR}/client/ui_api.h
  ${ENGINE_DIR}/client/client.h
  ${ENGINE_DIR}/client/cin_ogm.c
  ${ENGINE_DIR}/client/cl_avi.c
  ${ENGINE_DIR}/client/cl_cgame.c
  ${ENGINE_DIR}/client/cl_cin.c
  ${ENGINE_DIR}/client/cl_console.c
  ${ENGINE_DIR}/client/cl_input.c
  ${ENGINE_DIR}/client/cl_irc.c
  ${ENGINE_DIR}/client/cl_keys.c
  ${ENGINE_DIR}/client/keycodes.h
  ${ENGINE_DIR}/client/keys.h
  ${ENGINE_DIR}/client/cl_main.c
  ${ENGINE_DIR}/client/cl_net_chan.c
  ${ENGINE_DIR}/client/cl_parse.c
  ${ENGINE_DIR}/client/cl_scrn.c
  ${ENGINE_DIR}/client/cl_ui.c
  ${ENGINE_DIR}/client/cl_logs.c
  ${ENGINE_DIR}/client/dl_main.c
  ${ENGINE_DIR}/audio/snd_codec.c
  ${ENGINE_DIR}/audio/snd_codec.h
  ${ENGINE_DIR}/audio/snd_local.h
  ${ENGINE_DIR}/audio/snd_public.h
  ${ENGINE_DIR}/audio/snd_codec_ogg.c
  ${ENGINE_DIR}/audio/snd_codec_opus.c
  ${ENGINE_DIR}/audio/snd_codec_wav.c
  ${ENGINE_DIR}/audio/snd_openal.c
  ${ENGINE_DIR}/audio/snd_main.c
  ${ENGINE_DIR}/audio/qal.c
  ${ENGINE_DIR}/audio/qal.h
)

if( USE_MUMBLE )
  set( CLIENTLIST ${CLIENTLIST}
    ${ENGINE_DIR}/client/libmumblelink.c
    ${ENGINE_DIR}/client/libmumblelink.h
  )
endif()

set( SDLLIST
  ${ENGINE_DIR}/sys/sdl_input.c
  ${ENGINE_DIR}/sys/sdl2_compat.c
  ${ENGINE_DIR}/sys/sys_local.h
)

set( CGAMELIST
  ${GAMELOGIC_DIR}/cgame/cg_main.c #keep it first because vmMain must be the first function compiled in qvms
  ${GAMELOGIC_DIR}/shared/bg_misc.c
  ${GAMELOGIC_DIR}/shared/bg_parse.c
  ${GAMELOGIC_DIR}/shared/bg_pmove.c
  ${GAMELOGIC_DIR}/shared/bg_slidemove.c
  ${GAMELOGIC_DIR}/shared/bg_alloc.c
  ${GAMELOGIC_DIR}/shared/bg_voice.c
  ${GAMELOGIC_DIR}/shared/bg_teamprogress.c
  ${GAMELOGIC_DIR}/cgame/cg_consolecmds.c
  ${GAMELOGIC_DIR}/cgame/cg_buildable.c
  ${GAMELOGIC_DIR}/cgame/cg_animation.c
  ${GAMELOGIC_DIR}/cgame/cg_animmapobj.c
  ${GAMELOGIC_DIR}/cgame/cg_draw.c
  ${GAMELOGIC_DIR}/cgame/cg_drawtools.c
  ${GAMELOGIC_DIR}/cgame/cg_ents.c
  ${GAMELOGIC_DIR}/cgame/cg_event.c
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
  ${ENGINE_DIR}/client/cg_api.h
  ${ENGINE_DIR}/qcommon/print_translated.h
)

set( CGAMENATIVELIST
  ${GAMELOGIC_DIR}/cgame/cg_api.c
)

set( CGAMEQVMLIST
  ${GAMELOGIC_DIR}/shared/bg_lib.c
  ${ENGINE_DIR}/qcommon/q_static.c
  ${SHAREDLIST}
)

set( CGAMEASMLIST
  ${GAMELOGIC_DIR}/cgame/cg_api.asm
)

set( GAMELIST
  ${GAMELOGIC_DIR}/game/g_main.c #keep it first because vmMain must be the first function compiled in qvms
  ${GAMELOGIC_DIR}/shared/bg_misc.c
  ${GAMELOGIC_DIR}/shared/bg_parse.c
  ${GAMELOGIC_DIR}/shared/bg_pmove.c
  ${GAMELOGIC_DIR}/shared/bg_slidemove.c
  ${GAMELOGIC_DIR}/shared/bg_alloc.c
  ${GAMELOGIC_DIR}/shared/bg_voice.c
  ${GAMELOGIC_DIR}/shared/bg_teamprogress.c
  ${GAMELOGIC_DIR}/game/g_active.c
  ${GAMELOGIC_DIR}/game/g_client.c
  ${GAMELOGIC_DIR}/game/g_cmds.c
  ${GAMELOGIC_DIR}/game/g_combat.c
  ${GAMELOGIC_DIR}/game/g_confidence.c
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
  ${GAMELOGIC_DIR}/game/g_bot.c
  ${GAMELOGIC_DIR}/game/g_bot_ai.c
  ${GAMELOGIC_DIR}/game/g_bot_nav.c
  ${GAMELOGIC_DIR}/game/g_bot_parse.c
  ${GAMELOGIC_DIR}/game/g_bot_util.c
  ${ENGINE_DIR}/server/g_api.h
)

set( GAMENACLLIST
  ${GAMELOGIC_DIR}/game/g_api.cpp
)

set( GAMENATIVELIST
  ${GAMELOGIC_DIR}/game/g_api.c
)

set( GAMEQVMLIST
  ${GAMELOGIC_DIR}/shared/bg_lib.c
  ${ENGINE_DIR}/qcommon/q_static.c
  ${SHAREDLIST}
)

set( GAMEASMLIST
  ${GAMELOGIC_DIR}/game/g_api.asm
)

set( UILIST
  ${GAMELOGIC_DIR}/ui/ui_main.c #keep it first because vmMain must be the first function compiled in qvms
  ${GAMELOGIC_DIR}/ui/ui_atoms.c
  ${GAMELOGIC_DIR}/ui/ui_shared.c
  ${GAMELOGIC_DIR}/ui/ui_gameinfo.c
  ${GAMELOGIC_DIR}/shared/bg_misc.c
  ${GAMELOGIC_DIR}/shared/bg_parse.c
  ${GAMELOGIC_DIR}/shared/bg_alloc.c
  ${GAMELOGIC_DIR}/shared/bg_teamprogress.c
  ${GAMELOGIC_DIR}/ui/ui_utf8.c
  ${ENGINE_DIR}/client/ui_api.h
)

set( UINATIVELIST
  ${GAMELOGIC_DIR}/ui/ui_api.c
)

set( UIQVMLIST
  ${GAMELOGIC_DIR}/shared/bg_lib.c
  ${ENGINE_DIR}/qcommon/q_static.c
  ${SHAREDLIST}
)

set( UIASMLIST
  ${GAMELOGIC_DIR}/ui/ui_api.asm
)

set( WIN_RC ${ENGINE_DIR}/sys/daemon.rc )

set( MINIZIPLIST
  ${LIB_DIR}/minizip/ioapi.c
  ${LIB_DIR}/minizip/ioapi.h
  ${LIB_DIR}/minizip/unzip.c
  ${LIB_DIR}/minizip/unzip.h
)

set( TINYGETTEXTLIST
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

set( PDCURSESLIST
  ${LIB_DIR}/pdcurses/pdcurses/addch.c
  ${LIB_DIR}/pdcurses/pdcurses/addchstr.c
  ${LIB_DIR}/pdcurses/pdcurses/addstr.c
  ${LIB_DIR}/pdcurses/pdcurses/attr.c
  ${LIB_DIR}/pdcurses/pdcurses/beep.c
  ${LIB_DIR}/pdcurses/pdcurses/bkgd.c
  ${LIB_DIR}/pdcurses/pdcurses/border.c
  ${LIB_DIR}/pdcurses/pdcurses/clear.c
  ${LIB_DIR}/pdcurses/pdcurses/color.c
  ${LIB_DIR}/pdcurses/pdcurses/debug.c
  ${LIB_DIR}/pdcurses/pdcurses/delch.c
  ${LIB_DIR}/pdcurses/pdcurses/deleteln.c
  ${LIB_DIR}/pdcurses/pdcurses/deprec.c
  ${LIB_DIR}/pdcurses/pdcurses/getch.c
  ${LIB_DIR}/pdcurses/pdcurses/getstr.c
  ${LIB_DIR}/pdcurses/pdcurses/getyx.c
  ${LIB_DIR}/pdcurses/pdcurses/inch.c
  ${LIB_DIR}/pdcurses/pdcurses/inchstr.c
  ${LIB_DIR}/pdcurses/pdcurses/initscr.c
  ${LIB_DIR}/pdcurses/pdcurses/inopts.c
  ${LIB_DIR}/pdcurses/pdcurses/insch.c
  ${LIB_DIR}/pdcurses/pdcurses/insstr.c
  ${LIB_DIR}/pdcurses/pdcurses/instr.c
  ${LIB_DIR}/pdcurses/pdcurses/kernel.c
  ${LIB_DIR}/pdcurses/pdcurses/mouse.c
  ${LIB_DIR}/pdcurses/pdcurses/move.c
  ${LIB_DIR}/pdcurses/pdcurses/outopts.c
  ${LIB_DIR}/pdcurses/pdcurses/overlay.c
  ${LIB_DIR}/pdcurses/pdcurses/pad.c
  ${LIB_DIR}/pdcurses/pdcurses/panel.c
  ${LIB_DIR}/pdcurses/pdcurses/printw.c
  ${LIB_DIR}/pdcurses/pdcurses/refresh.c
  ${LIB_DIR}/pdcurses/pdcurses/scanw.c
  ${LIB_DIR}/pdcurses/pdcurses/scr_dump.c
  ${LIB_DIR}/pdcurses/pdcurses/scroll.c
  ${LIB_DIR}/pdcurses/pdcurses/slk.c
  ${LIB_DIR}/pdcurses/pdcurses/termattr.c
  ${LIB_DIR}/pdcurses/pdcurses/terminfo.c
  ${LIB_DIR}/pdcurses/pdcurses/touch.c
  ${LIB_DIR}/pdcurses/pdcurses/util.c
  ${LIB_DIR}/pdcurses/pdcurses/window.c
  ${LIB_DIR}/pdcurses/win32a/pdcclip.c
  ${LIB_DIR}/pdcurses/win32a/pdcdisp.c
  ${LIB_DIR}/pdcurses/win32a/pdcgetsc.c
  ${LIB_DIR}/pdcurses/win32a/pdckbd.c
  ${LIB_DIR}/pdcurses/win32a/pdcscrn.c
  ${LIB_DIR}/pdcurses/win32a/pdcsetsc.c
  ${LIB_DIR}/pdcurses/win32a/pdcutil.c
)

set( FASTLZLIST
  ${MOUNT_DIR}/libs/fastlz/fastlz.c
)

set( DETOURLIST
  ${FASTLZLIST}
  ${MOUNT_DIR}/libs/detour/DetourAlloc.cpp
  ${MOUNT_DIR}/libs/detour/DetourCommon.cpp
  ${MOUNT_DIR}/libs/detour/DetourNavMeshBuilder.cpp
  ${MOUNT_DIR}/libs/detour/DetourNavMesh.cpp
  ${MOUNT_DIR}/libs/detour/DetourNavMeshQuery.cpp
  ${MOUNT_DIR}/libs/detour/DetourNode.cpp
  ${MOUNT_DIR}/libs/detour/DetourPathCorridor.cpp
  ${MOUNT_DIR}/libs/detour/DetourDebugDraw.cpp
  ${MOUNT_DIR}/libs/detour/DebugDraw.cpp
  ${MOUNT_DIR}/libs/detour/DetourTileCache.cpp
  ${MOUNT_DIR}/libs/detour/DetourTileCacheBuilder.cpp
)

set( RECASTLIST
  ${MOUNT_DIR}/libs/recast/RecastAlloc.cpp
  ${MOUNT_DIR}/libs/recast/RecastArea.cpp
  ${MOUNT_DIR}/libs/recast/RecastContour.cpp
  ${MOUNT_DIR}/libs/recast/Recast.cpp
  ${MOUNT_DIR}/libs/recast/RecastFilter.cpp
  ${MOUNT_DIR}/libs/recast/RecastLayers.cpp
  ${MOUNT_DIR}/libs/recast/RecastMesh.cpp
  ${MOUNT_DIR}/libs/recast/RecastMeshDetail.cpp
  ${MOUNT_DIR}/libs/recast/RecastRasterization.cpp
  ${MOUNT_DIR}/libs/recast/RecastRegion.cpp
  ${MOUNT_DIR}/libs/recast/ChunkyTriMesh.cpp
)

if( APPLE )
  set( NACLLIST_NATIVE
    ${LIB_DIR}/nacl/nacl.cpp
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/posix/nacl_imc_posix.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/osx/nacl_imc.cc
  )
elseif( LINUX )
  set( NACLLIST_NATIVE
    ${LIB_DIR}/nacl/nacl.cpp
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/posix/nacl_imc_posix.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/linux/nacl_imc.cc
  )
elseif( WIN32 )
  set( NACLLIST_NATIVE
    ${LIB_DIR}/nacl/nacl.cpp
    ${LIB_DIR}/nacl/native_client/src/shared/imc/nacl_imc_common.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/win/nacl_imc.cc
    ${LIB_DIR}/nacl/native_client/src/shared/imc/win/nacl_shm.cc
  )
endif()

set( NACLLIST_MODULE
  ${LIB_DIR}/nacl/nacl.cpp
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

# HACK: Force the compilation of EVERYTHING in C++
file( GLOB_RECURSE EVERYTHING_C "src/engine/*.c" "src/gamelogic/*.c" )
set_source_files_properties( ${EVERYTHING_C} PROPERTIES LANGUAGE CXX )

################################################################################
# Libraries
################################################################################

# Import external dependencies
if( DEPS_DIR )
  # Download them if they not available
  if( NOT EXISTS ${DEPS_DIR} )
    get_filename_component( BASENAME ${DEPS_DIR} NAME )
    set( REMOTE "http://vanquished.zapto.org/~amanieu/unvanquished_deps/${BASENAME}.zip" )
    message( STATUS "Downloading dependencies from '${REMOTE}'" )
    file( DOWNLOAD ${REMOTE} ${OBJ_DIR}/${BASENAME}.zip SHOW_PROGRESS
      STATUS DOWNLOAD_RESULT LOG DOWNLOAD_LOG )
    list( GET DOWNLOAD_RESULT 0 DOWNLOAD_STATUS )
    list( GET DOWNLOAD_RESULT 1 DOWNLOAD_STRING )
    if( NOT DOWNLOAD_STATUS EQUAL 0 )
      message( FATAL_ERROR "Error downloading '${REMOTE}':
        Status code: ${DOWNLOAD_STATUS}
        Error string: ${DOWNLOAD_STRING}
        Download log: ${DOWNLOAD_LOG}" )
    endif()
    message( STATUS "Download completed successfully" )

    # Extract the downloaded archive
    execute_process( COMMAND ${CMAKE_COMMAND} -E tar xzf ${OBJ_DIR}/${BASENAME}.zip
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external_deps RESULT_VARIABLE EXTRACT_RESULT )
    if( NOT EXTRACT_RESULT EQUAL 0 )
      message( FATAL_ERROR "Could not extract ${BASENAME}.zip" )
    endif()
  endif()

  # Add to paths
  set( CMAKE_FIND_ROOT_PATH ${DEPS_DIR} ${CMAKE_FIND_ROOT_PATH} )
  set( CMAKE_INCLUDE_PATH ${DEPS_DIR} ${DEPS_DIR}/include ${CMAKE_INCLUDE_PATH} )
  set( CMAKE_FRAMEWORK_PATH ${DEPS_DIR} ${CMAKE_FRAMEWORK_PATH} )
  set( CMAKE_LIBRARY_PATH ${DEPS_DIR}/lib ${CMAKE_LIBRARY_PATH} )
endif()

# These variables are defined
# LIBS_BASE, LIBSRC_BASE
# LIBS_ENGINE, LIBSRC_ENGINE
# LIBS_RENDERER, LIBSRC_RENDERER
# LIBS_BASECLIENT, LIBSRC_BASECLIENT
# LIBS_CLIENT, LIBSRC_CLIENT

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
  set( LIBS_ENGINE ${LIBS_ENGINE} ${CMAKE_DL_LIBS} )
  find_package( Threads REQUIRED )
  set( LIBS_BASE ${LIBS_BASE} ${CMAKE_THREAD_LIBS_INIT} )
  if( APPLE )
    set( LIBS_CLIENT ${LIBS_CLIENT} "-framework Carbon" "-framework IOKit" "-framework Cocoa" )
  endif()
endif()

# Minizip
set( LIBSRC_ENGINE ${LIBSRC_ENGINE} ${MINIZIPLIST} )

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

# Native client
if( APPLE )
  add_definitions( -DNACL_WINDOWS=0 -DNACL_LINUX=0 -DNACL_ANDROID=0 -DNACL_OSX=1 )
elseif( LINUX )
  add_definitions( -DNACL_WINDOWS=0 -DNACL_LINUX=1 -DNACL_ANDROID=0 -DNACL_OSX=0 )
elseif( WIN32 )
  add_definitions( -DNACL_WINDOWS=1 -DNACL_LINUX=0 -DNACL_ANDROID=0 -DNACL_OSX=0 )
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
set( LIBSRC_ENGINE ${LIBSRC_ENGINE} ${NACLLIST_NATIVE} )

# SDL, required for all targets on win32 because of iconv
if( BUILD_CLIENT OR BUILD_NEW_RENDERER OR BUILD_OLD_RENDERER OR ( WIN32 AND (BUILD_TTY_CLIENT OR BUILD_SERVER ) ) )
  if( USE_SDL2 )
    find_package( SDL2 REQUIRED )
    include_directories( ${SDL2_INCLUDE_DIR} )
    if( WIN32 )
      set( LIBS_ENGINE ${LIBS_ENGINE} ${SDL2_LIBRARY} )
    else()
      set( LIBS_CLIENT ${LIBS_CLIENT} ${SDL2_LIBRARY} )
    endif()
    set( LIBS_RENDERER ${LIBS_RENDERER} ${SDL2_LIBRARY} )
    set( LIBS_RENDERERGL ${LIBS_RENDERERGL} ${SDL2_LIBRARY} )
  else()
    find_package( SDL REQUIRED )
    include_directories( ${SDL_INCLUDE_DIR} )
    if( WIN32 )
      set( LIBS_ENGINE ${LIBS_ENGINE} ${SDL_LIBRARY} )
    else()
      set( LIBS_CLIENT ${LIBS_CLIENT} ${SDL_LIBRARY} )
    endif()
    set( LIBS_RENDERER ${LIBS_RENDERER} ${SDL_LIBRARY} )
    set( LIBS_RENDERERGL ${LIBS_RENDERERGL} ${SDL_LIBRARY} )
    if( APPLE )
      # Mac OS X does not provide a SDLMain.dylib so we must compile our own
      add_library( INTERNAL_SDLMain ${ENGINE_DIR}/sys/SDLMain.mm )
      set( LIBS_CLIENT ${LIBS_CLIENT} INTERNAL_SDLMain )
    endif()
  endif()

  # Used by tinygettext to use SDL_iconv instead of base iconv
  if( WIN32 )
    add_definitions( -DHAVE_SDL )
  endif()
endif()
mark_as_advanced( SDLMAIN_LIBRARY SDL_LIBRARY SDL_INCLUDE_DIR MINGW32_LIBRARY )
mark_as_advanced( SDL2MAIN_LIBRARY SDL2_LIBRARY SDL2_INCLUDE_DIR )

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
if( BUILD_NEW_RENDERER OR BUILD_OLD_RENDERER )
  find_package( WebP REQUIRED )
  include_directories( ${WEBP_INCLUDE_DIRS} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${WEBP_LIBRARIES} )
endif()

# JPEG
if( BUILD_NEW_RENDERER OR BUILD_OLD_RENDERER )
  find_package( JPEG REQUIRED )
  include_directories( ${JPEG_INCLUDE_DIR} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${JPEG_LIBRARIES} )
endif()

# PNG
if( BUILD_NEW_RENDERER OR BUILD_OLD_RENDERER )
  find_package( PNG REQUIRED )
  include_directories( ${PNG_INCLUDE_DIRS} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${PNG_LIBRARIES} )
endif()

# Freetype
if( BUILD_NEW_RENDERER OR BUILD_OLD_RENDERER )
  find_package( Freetype REQUIRED )
  include_directories( ${FREETYPE_INCLUDE_DIRS} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${FREETYPE_LIBRARIES} )
endif()

# GLEW
if( BUILD_NEW_RENDERER OR BUILD_OLD_RENDERER )
  find_package( GLEW REQUIRED )
  include_directories( ${GLEW_INCLUDE_DIR} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${GLEW_LIBRARIES} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${GLEW_LIBRARIES} )
endif()

# OpenGL
if( BUILD_NEW_RENDERER OR BUILD_OLD_RENDERER )
  find_package( OpenGL REQUIRED )
  include_directories( ${OPENGL_INCLUDE_DIR} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${OPENGL_LIBRARIES} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${OPENGL_LIBRARIES} )
endif()

# X11
if( LINUX AND ( BUILD_CLIENT OR BUILD_NEW_RENDERER OR BUILD_OLD_RENDERER ) )
  find_package( X11 REQUIRED )
  include_directories( ${X11_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${X11_LIBRARIES} )
  set( LIBS_RENDERER ${LIBS_RENDERER} ${X11_LIBRARIES} )
endif()

# OpenAL
if( BUILD_CLIENT )
  find_package( OpenAL REQUIRED )
  include_directories( ${OPENAL_INCLUDE_DIR} )
  if( USE_OPENAL_DLOPEN )
    add_definitions( -DUSE_OPENAL_DLOPEN )
  else()
    set( LIBS_CLIENT ${LIBS_CLIENT} ${OPENAL_LIBRARY} )
  endif()
endif()

# CURL
if( BUILD_CLIENT OR BUILD_TTY_CLIENT )
  find_package( CURL REQUIRED )
  include_directories( ${CURL_INCLUDE_DIRS} )
  set( LIBS_CLIENTBASE ${LIBS_CLIENTBASE} ${CURL_LIBRARIES} )
endif()

# Curses, pdcurses on Windows and ncursesw on Unix
if( USE_CURSES )
  add_definitions( -DUSE_CURSES -DUSE_CURSES_W )
  if( WIN32 )
    add_definitions( -DPDC_WIDE -DPDC_FORCE_UTF8 )
    include_directories( ${LIB_DIR}/pdcurses )
    set( LIBSRC_ENGINE ${LIBSRC_ENGINE} ${PDCURSESLIST} )
    set( LIBS_ENGINE ${LIBS_ENGINE} gdi32 comdlg32 )
  else()
    set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
    find_package( CursesW REQUIRED )
    include_directories( ${CURSESW_INCLUDE_DIR} )
    set( LIBS_ENGINE ${LIBS_ENGINE} ${CURSESW_LIBRARIES} )
  endif()
endif()

################################################################################
# Game logic
################################################################################

# Function to setup all the Game/Cgame/UI libraries
include( CMakeParseArguments )
function( GAMEMODULE )
  # ParseArguments setup
  set( oneValueArgs NAME OUTPUT_DIR HAS_NACL )
  set( multiValueArgs COMPILE_DEF FILES NACL_FILES NATIVE_FILES QVM_FILES QVM_ASM )
  cmake_parse_arguments( GAMEMODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )
  if( CMAKE_CFG_INTDIR STREQUAL "." )
    set( GAMEMODULE_FULL_OUTPUT_DIR ${CMAKE_BINARY_DIR}/${GAMEMODULE_OUTPUT_DIR} )
  else()
    set( GAMEMODULE_FULL_OUTPUT_DIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR}/${GAMEMODULE_OUTPUT_DIR} )
  endif()

  if( GAMEMODULE_HAS_NACL AND BUILD_GAME_NACL_NATIVE )
    add_executable( ${GAMEMODULE_NAME}-nacl-native ${GAMEMODULE_FILES} ${SHAREDLIST} ${GAMEMODULE_NACL_FILES} ${NACLLIST_NATIVE} )
    target_link_libraries( ${GAMEMODULE_NAME}-nacl-native ${LIBS_BASE} )
    set_target_properties( ${GAMEMODULE_NAME}-nacl-native PROPERTIES
      OUTPUT_NAME ${GAMEMODULE_NAME}
      RUNTIME_OUTPUT_DIRECTORY ${GAMEMODULE_OUTPUT_DIR}
      COMPILE_DEFINITIONS ${GAMEMODULE_COMPILE_DEF}
    )
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
      string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UP )
      set_target_properties( ${GAMEMODULE_NAME}-nacl-native PROPERTIES RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UP} ${OUTPUTCONFIG}/${GAMEMODULE_OUTPUT_DIR} )
    endforeach()
  endif()

  if( BUILD_GAME_QVM_NATIVE )
    set( QVM_NATIVE_COMPILE_DEF ${GAMEMODULE_COMPILE_DEF} QVM_ABI)
    add_library( ${GAMEMODULE_NAME}-qvm-native MODULE ${GAMEMODULE_FILES} ${SHAREDLIST} ${GAMEMODULE_NATIVE_FILES} )
    target_link_libraries( ${GAMEMODULE_NAME}-qvm-native ${LIBS_BASE} )
    set_target_properties( ${GAMEMODULE_NAME}-qvm-native PROPERTIES
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY ${GAMEMODULE_OUTPUT_DIR}
	  COMPILE_DEFINITIONS "${QVM_NATIVE_COMPILE_DEF}"
    )
    foreach( OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES} )
      string( TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG_UP )
      set_target_properties( ${GAMEMODULE_NAME}-qvm-native PROPERTIES LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG_UP} ${OUTPUTCONFIG}/${GAMEMODULE_OUTPUT_DIR} )
    endforeach()
  endif()

  if( GAMEMODULE_HAS_NACL AND BUILD_GAME_NACL )
    if( CMAKE_CFG_INTDIR STREQUAL "." )
      set( NACL_DIR ${OBJ_DIR}/${GAMEMODULE_NAME}-nacl.dir )
    else()
      set( NACL_DIR ${OBJ_DIR}/${GAMEMODULE_NAME}-nacl.dir/${CMAKE_CFG_INTDIR} )
    endif()

    # Determine compiler and translator options
    set( NACL_COMPILE_OPTIONS
      -I${LIB_DIR}/nacl
      -DNACL_BUILD_ARCH=x86 # Not actually used
      -DNACL_BUILD_SUBARCH=64 # Not actually used
      -Wall
      -ffast-math
      -fvisibility=hidden
      --pnacl-allow-exceptions
      $<$<CONFIG:None>:-O3>
      $<$<CONFIG:None>:-fomit-frame-pointer>
      $<$<CONFIG:None>:-DNDEBUG>
      $<$<CONFIG:Release>:-O3>
      $<$<CONFIG:Release>:-fomit-frame-pointer>
      $<$<CONFIG:Release>:-DNDEBUG>
      $<$<CONFIG:Debug>:-O0>
      $<$<CONFIG:Debug>:-g3>
      $<$<CONFIG:RelWithDebInfo>:-O2>
      $<$<CONFIG:RelWithDebInfo>:-DNDEBUG>
      $<$<CONFIG:RelWithDebInfo>:-g3>
      $<$<CONFIG:MinSizeRel>:-Os>
      $<$<CONFIG:MinSizeRel>:-fomit-frame-pointer>
      $<$<CONFIG:MinSizeRel>:-DNDEBUG>
    )
    set( NACL_TRANSLATE_OPTIONS
      --allow-llvm-bitcode-input
      --pnacl-allow-exceptions
      $<$<CONFIG:None>:-O3>
      $<$<CONFIG:Release>:-O3>
      $<$<CONFIG:Debug>:-O0>
      $<$<CONFIG:RelWithDebInfo>:-O2>
      $<$<CONFIG:MinSizeRel>:-O2>
    )
    foreach( DEF ${GAMEMODULE_COMPILE_DEF} )
      set( NACL_COMPILE_OPTIONS ${NACL_COMPILE_OPTIONS} -D${DEF} )
    endforeach()

    # Compile C files with clang and C++ files with clang++
    set( OBJECTS )
    foreach( FILE ${GAMEMODULE_FILES} ${GAMEMODULE_NACL_FILES} ${SHAREDLIST} ${NACLLIST_MODULE} )
      get_filename_component( FILENAME ${FILE} NAME_WE )
      get_filename_component( FILEDIR ${FILE} PATH )
      get_filename_component( EXT ${FILE} EXT )
      set( OBJ ${NACL_DIR}/${FILENAME}.o )

      if( EXT STREQUAL ".c" OR EXT STREQUAL ".cpp" OR EXT STREQUAL ".cc" )
        add_custom_command( OUTPUT ${OBJ}
          COMMAND ${PNACL_TOOLCHAIN}/pnacl-clang++${PNACL_BIN_EXT}
            -x c++ -std=gnu++11
            ${NACL_COMPILE_OPTIONS}
            -c
            -o ${OBJ}
            ${FILE}
          DEPENDS ${FILE}
          IMPLICIT_DEPENDS CXX ${FILE}
        )
        set( OBJECTS ${OBJECTS} ${OBJ} )
      endif()
    endforeach()

    # Generate PNaCL platform-independant executable
    add_custom_command(
      OUTPUT ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
      COMMAND ${PNACL_TOOLCHAIN}/pnacl-clang++${PNACL_BIN_EXT}
        ${NACL_COMPILE_OPTIONS}
        -o ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
        ${OBJECTS}
      DEPENDS ${OBJECTS}
    )

    # Generate NaCl executables for x86 and x86_64
    add_custom_command(
      OUTPUT ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86.nexe
      COMMAND ${PNACL_TOOLCHAIN}/pnacl-translate${PNACL_BIN_EXT}
        ${NACL_TRANSLATE_OPTIONS}
        -arch i686
        -o ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86.nexe
        ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
      DEPENDS ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
    )
    add_custom_command(
      OUTPUT ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86_64.nexe
      COMMAND ${PNACL_TOOLCHAIN}/pnacl-translate${PNACL_BIN_EXT}
        ${NACL_TRANSLATE_OPTIONS}
        -arch x86-64
        -o ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86_64.nexe
        ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
      DEPENDS ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
    )

    # Dummy target to get CMake to execute the commands
    add_custom_target( ${GAMEMODULE_NAME}-nacl ALL
      DEPENDS ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
        ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86.nexe
        ${GAMEMODULE_FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86_64.nexe
    )
  endif()

  if( BUILD_GAME_QVM )
    if( CMAKE_CFG_INTDIR STREQUAL "." )
      set( QVM_DIR ${OBJ_DIR}/${GAMEMODULE_NAME}-qvm.dir )
    else()
      set( QVM_DIR ${OBJ_DIR}/${GAMEMODULE_NAME}-qvm.dir/${CMAKE_CFG_INTDIR} )
    endif()

    set( QVM_COMPILE_OPTIONS -I${MOUNT_DIR}/gamelogic/game -I${MOUNT_DIR}/engine/qcommon -DQ3_VM -DQVM_ABI)
    foreach( DEF ${GAMEMODULE_COMPILE_DEF} )
      set( QVM_COMPILE_OPTIONS ${QVM_COMPILE_OPTIONS} -D${DEF} )
    endforeach()

    set( OBJECTS )
    set( QVM_FILES ${GAMEMODULE_FILES} ${GAMEMODULE_QVM_FILES} )
    foreach( QVM_FILE ${QVM_FILES} )
      get_filename_component( QVM_FILENAME ${QVM_FILE} NAME_WE )
      get_filename_component( QVM_FILEDIR ${QVM_FILE} PATH )
      get_filename_component( QVM_FILEEXT ${QVM_FILE} EXT )
      set( ASM_FILE ${QVM_DIR}/${QVM_FILENAME}.asm )

      if( QVM_FILEEXT STREQUAL ".c" )
        add_custom_command( OUTPUT ${ASM_FILE}
			COMMAND lcc -target bytecode ${QVM_COMPILE_OPTIONS} ${QVM_FILE} -o ${ASM_FILE}
          DEPENDS lcc rcc cpp ${QVM_FILE}
          IMPLICIT_DEPENDS C ${QVM_FILE}
          WORKING_DIRECTORY ${QVM_DIR}
        )

        set( OBJECTS ${OBJECTS} ${ASM_FILE} )
      endif()
    endforeach()

    # Link *.asm into a qvm
    add_custom_command( OUTPUT ${GAMEMODULE_FULL_OUTPUT_DIR}/vm/${GAMEMODULE_NAME}.qvm
      COMMAND asm -o ${GAMEMODULE_FULL_OUTPUT_DIR}/vm/${GAMEMODULE_NAME}.qvm ${OBJECTS} ${GAMEMODULE_QVM_ASM}
      DEPENDS asm ${OBJECTS} ${GAMEMODULE_QVM_ASM}
    )

    # Dummy target to get CMake to execute the commands
    add_custom_target( ${GAMEMODULE_NAME}-qvm ALL
      DEPENDS ${GAMEMODULE_FULL_OUTPUT_DIR}/vm/${GAMEMODULE_NAME}.qvm
    )
  endif()
endfunction()

if( BUILD_GAME_QVM )
  add_subdirectory( ${LCC_DIR} ${CMAKE_BINARY_DIR}/lcc )
endif()

GAMEMODULE( NAME game
  OUTPUT_DIR main
  COMPILE_DEF GAME
  HAS_NACL 1
  FILES ${GAMELIST}
  NACL_FILES ${GAMENACLLIST}
  NATIVE_FILES ${GAMENATIVELIST}
  QVM_FILES ${GAMEQVMLIST}
  QVM_ASM ${GAMEASMLIST}
)

GAMEMODULE( NAME cgame
  OUTPUT_DIR main
  COMPILE_DEF CGAME
  HAS_NACL 0
  FILES ${CGAMELIST}
  NACL_FILES ${CGAMENACLLIST}
  NATIVE_FILES ${CGAMENATIVELIST}
  QVM_FILES ${CGAMEQVMLIST}
  QVM_ASM ${CGAMEASMLIST}
)

GAMEMODULE( NAME ui
  OUTPUT_DIR main
  COMPILE_DEF UI
  FILES ${UILIST}
  HAS_NACL 0
  NACL_FILES ${UINACLLIST}
  NATIVE_FILES ${UINATIVELIST}
  QVM_FILES ${UIQVMLIST}
  QVM_ASM ${UIASMLIST}
)

################################################################################
# Engine
################################################################################

if( BUILD_CLIENT )
  add_executable( client WIN32 ${SERVERLIST} ${CLIENTLIST} ${SDLLIST} ${ENGINELIST} ${SHAREDLIST}
    ${LIBSRC_BASE} ${LIBSRC_ENGINE} ${LIBSRC_CLIENTBASE} ${LIBSRC_CLIENT} ${WIN_RC} )
  target_link_libraries( client ${LIBS_CLIENT} ${LIBS_CLIENTBASE} ${LIBS_ENGINE} ${LIBS_BASE} )
  if( USE_VOIP )
    # Set it here so the tty client doesn't need to depend on speex
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_VOIP )
  endif()
  set_target_properties( client PROPERTIES OUTPUT_NAME "daemon" PREFIX "" )
  if( USE_SMP )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS SMP )
  endif()
endif()

if( BUILD_SERVER )
  add_executable( server WIN32 ${SERVERLIST} ${ENGINELIST} ${SHAREDLIST} ${LIBSRC_BASE} ${LIBSRC_ENGINE}
    ${ENGINE_DIR}/null/null_client.c  ${ENGINE_DIR}/null/null_input.c
    ${ENGINE_DIR}/null/null_snddma.c ${WIN_RC} )
  target_link_libraries( server ${LIBS_ENGINE} ${LIBS_BASE} )
  set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS DEDICATED )
  set_target_properties( server PROPERTIES OUTPUT_NAME "daemonded" PREFIX "" )
endif()

if( BUILD_TTY_CLIENT )
  add_executable( ttyclient WIN32 ${SERVERLIST} ${CLIENTLIST} ${ENGINELIST} ${SHAREDLIST}
    ${LIBSRC_BASE} ${LIBSRC_ENGINE} ${LIBSRC_CLIENTBASE} ${WIN_RC}
    ${ENGINE_DIR}/null/null_input.c ${ENGINE_DIR}/null/null_snddma.c ${ENGINE_DIR}/null/null_renderer.c )
  target_link_libraries( ttyclient ${LIBS_CLIENTBASE} ${LIBS_ENGINE} ${LIBS_BASE} )
  set_property( TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS BUILD_TTY_CLIENT REF_HARD_LINKED )
  set_target_properties( ttyclient PROPERTIES OUTPUT_NAME "daemon-tty" PREFIX "" )
endif()

################################################################################
# Renderers
################################################################################

if( BUILD_OLD_RENDERER )
  add_library( renderer MODULE ${RENDERERLIST} ${SHAREDLIST} ${LIBSRC_BASE} ${LIBSRC_RENDERER} )
  target_link_libraries( renderer ${LIBS_RENDERER} ${LIBS_BASE} )
  set_target_properties( renderer PROPERTIES OUTPUT_NAME "rendererGL" PREFIX "" )
endif()

if( BUILD_NEW_RENDERER )
  add_library( rendererGL MODULE ${RENDERERGLLIST} ${SHAREDLIST} ${LIBSRC_BASE} ${LIBSRC_RENDERER} )
  set_property( TARGET rendererGL APPEND PROPERTY COMPILE_DEFINITIONS USE_XREAL_RENDERER )
  target_link_libraries( rendererGL ${LIBS_RENDERER} ${LIBS_BASE} )
  set_target_properties( rendererGL PROPERTIES OUTPUT_NAME "rendererGL3" PREFIX "" )
  if( USE_SMP )
    set_property( TARGET rendererGL APPEND PROPERTY COMPILE_DEFINITIONS SMP )
  endif()
endif()
