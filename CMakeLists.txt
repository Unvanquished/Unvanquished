#
# CMakeLists.txt
#
# cmake required (http://www.cmake.org/)
#

cmake_minimum_required( VERSION 2.8 )

set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )
set( CMAKE_C_FLAGS "-msse2 -pipe" CACHE STRING "" )
set( CMAKE_C_FLAGS_DEBUG "-g" CACHE STRING "" )
set( CMAKE_C_FLAGS_MINSIZEREL "-DNDEBUG -Os" CACHE STRING "" )
set( CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3 -ffast-math -fomit-frame-pointer -fno-strict-aliasing" CACHE STRING "" )
set( CMAKE_C_FLAGS_RELWITHDEBINFO "-DNDEBUG -O3 -ffast-math -fomit-frame-pointer -fno-strict-aliasing -g" CACHE STRING "" )
set( CMAKE_CXX_FLAGS ${CMAKE_C_FLAGS} CACHE STRING "" )
set( CMAKE_CXX_FLAGS_DEBUG ${CMAKE_C_FLAGS_DEBUG} CACHE STRING "" )
set( CMAKE_CXX_FLAGS_MINSIZEREL ${CMAKE_C_FLAGS_MINSIZEREL} CACHE STRING "" )
set( CMAKE_CXX_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE} CACHE STRING "" )
set( CMAKE_CXX_FLAGS_RELWITHDEBINFO ${CMAKE_C_FLAGS_RELWITHDEBINFO} CACHE STRING "" )
set( CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)" )

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set( MSVC_INCREMENTAL_DEFAULT ON )

project( Daemon C CXX ASM )
set( CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER} )
set( CMAKE_ASM_COMPILE_OBJECT ${CMAKE_C_COMPILE_OBJECT} )
set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "" ) # Need this to get rid of -rdynamic linker flag set by default by cmake

# Options
option( BUILD_CLIENT         "Build Daemon client"                         1 )
option( BUILD_OLD_RENDERER   "Build old renderer library"                  1 )
option( BUILD_NEW_RENDERER   "Build new renderer library"                  1 )
option( BUILD_SERVER         "Build Daemonded server"                      1 )
option( BUILD_GAME_SO_ET     "Build game logic shared libraries for ET"    0 )
option( BUILD_GAME_SO_TREM   "Build game logic shared libs for Trem"       0 )
option( BUILD_GAME_SO_GPP    "Build GPP Tremulous game logic libs"         1 )
option( BUILD_MASTER         "Build master server"                         1 )
option( BUILD_DAEMONMAP      "Build Mapping tool"                          0 )
option( USE_OPENAL           "Use OpenAL"                                  1 )
if( USE_OPENAL )
  option( USE_OPENAL_DLOPEN  "Use dlopen to load OpenAL"                   1 )
endif()
option( USE_LLVM             "Use LLVM"                                    0 )
option( USE_CPUINFO          "Use CPUInfo (if compilation fails, then disable this)" 1 )
option( USE_CURSES           "Use the curses console"                      1 )
option( USE_CODEC_VORBIS     "Compile with support for Vorbis"             1 )
if( USE_CODEC_VORBIS )
  option( USE_CIN_THEORA     "Compile with support for Theora"             1 )
endif()
option( USE_GLSL_OPTIMIZER   "Compile with GLSL Optimizer"                 0 )
option( USE_MUMBLE           "Compile with support for Mumble"             1 )
option( USE_PHYSICS          "Compile with Newton Physics"                 0 )
option( USE_WEBP             "Compile with WebP loader"                    1 )
option( USE_MYSQL            "Compile with MYSQL support"                  0 )
option( USE_VOIP             "Compile with VOIP support"                   1 )
if( USE_VOIP )
  option( USE_INTERNAL_SPEEX "Use internal speex"                          0 )
endif()
option( USE_CIN_XVID         "Encode videos with XViD"                     1 )
option( USE_INTERNAL_CRYPTO  "Use internal crypto (nettle)"                0 )
option( NO_UNTRUSTED_PLUGINS "Don't load native plugins from the user dir" 1 )

option( ENABLE_W_ALL         "Use -Wall"                                   0 )
option( ENABLE_WARNINGS      "Enable various compiler warning options"     1 )
option( ENABLE_HARDENING     "Enable various hardening options"            1 )
if( ENABLE_HARDENING )
  option( ENABLE_FSTACKPROT  "Enable stack protection if available"        1 )
  option( ENABLE_RELRO       "Enable relro & bindnow if available"         1 )
endif()

# Package info
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Daemon Unvanquished client" )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt" )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/GPL.txt" )
include( CPack )

set( MOUNT_DIR ${CMAKE_SOURCE_DIR}/src )
set( GAME_DIR ${CMAKE_SOURCE_DIR}/src/gamelogic/etmain )
set( BASE_DIR ${CMAKE_SOURCE_DIR}/src/gamelogic/base )
set( GPP_DIR ${CMAKE_SOURCE_DIR}/src/gamelogic/gpp )
set( LIB_DIR ${CMAKE_SOURCE_DIR}/src/libs )

if( WIN32 )
  add_definitions( -D_CRT_SECURE_NO_WARNINGS )
endif()

set( CONSOLE "tty" )

# Determine platform
if( WIN32 )
  set( PLATFORM "win32" )
  set( CONSOLE "win32" )
elseif( UNIX )
  if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
    add_definitions( -DMACOS_X -DMACOS_X_ACCELERATION_HACK -DUSE_FILE32API -DMACOS_X_CURSOR_HACK )
  endif()
  set( PLATFORM "unix" )
endif()

# Determine arch.
if( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
  # CMAKE_SYSTEM_PROCESSOR always returns i386 for x86 and x86_64 on Mac
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( BUILD_ARCH "x86_64" )
  elseif( CMAKE_SIZEOF_VOID_P EQUAL 4 AND CMAKE_SYSTEM_PROCESSOR STREQUAL "i386" )
    set( BUILD_ARCH "i386" )
  elseif( CMAKE_SIZEOF_VOID_P EQUAL 4 AND CMAKE_SYSTEM_PROCESSOR STREQUAL "ppc" )
    set( BUILD_ARCH "ppc" )
  else()
    set( BUILD_ARCH "Unknown" )
  endif()
elseif( WIN32 )
  # CMAKE_SYSTEM_PROCESSOR is not reliable under windows
  if( CMAKE_SIZEOF_VOID_P EQUAL 8 )
    set( BUILD_ARCH "x86_64" )
  elseif( CMAKE_SIZEOF_VOID_P EQUAL 4 )
    set( BUILD_ARCH "i386" )
  endif()
else()
  # CMAKE_SYSTEM_PROCESSOR is reliable for *nix
  if( CMAKE_SYSTEM_PROCESSOR MATCHES "i.86" )
    set( BUILD_ARCH "i386" )
  elseif( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
    set( BUILD_ARCH "x86_64" )
  else()
    set( BUILD_ARCH ${CMAKE_SYSTEM_PROCESSOR} )
  endif()
endif()


#################
# Compile flags #
#################

macro( try_compiler_flag_internal PROP FLAG CC SUFFIX LABEL )
  set( SAVED_FLAGS "${CMAKE_${CC}_FLAGS}" )
  set( CMAKE_${CC}_FLAGS "${CMAKE_${CC}_FLAGS} ${FLAG}" )
  # test it on an executable
  if( NOT ${PROP}_${CC}_CACHED )
    try_compile( ${PROP}_${CC} ${MOUNT_DIR}/tests ${MOUNT_DIR}/tests/test.${SUFFIX}
                 CMAKE_FLAGS "-DCMAKE_${CC}_FLAGS=${CMAKE_${CC}_FLAGS}" )
  endif()
  if( ${PROP}_${CC} )
    if( NOT ${PROP}_${CC}_CACHED )
      message( STATUS "Using ${LABEL} flag ${FLAG}" )
    endif()
  else()
    if( NOT ${PROP}_${CC}_CACHED )
      message( STATUS "${LABEL} flag ${FLAG} is not available" )
    endif()
    set( CMAKE_${CC}_FLAGS "${SAVED_FLAGS}" )
  endif()
  set( ${PROP}_${CC}_CACHED TRUE CACHE INTERNAL "Set if ${LABEL} option ${FLAG} has been checked for." )
endmacro()

macro( try_c_flag PROP FLAG )
  try_compiler_flag_internal( ${PROP} ${FLAG} C c C )
endmacro()

macro( try_cxx_flag PROP FLAG )
  try_compiler_flag_internal( ${PROP} ${FLAG} CXX cxx C++)
endmacro()

macro( try_c_cxx_flag PROP FLAG )
  try_c_flag( ${PROP} ${FLAG} )
  try_cxx_flag( ${PROP} ${FLAG} )
endmacro()

# Various warnings
if( ENABLE_W_ALL )
  try_c_cxx_flag( HAVE_W_ALL              -Wall )
endif()
if( ENABLE_WARNINGS )
  try_c_cxx_flag( HAVE_W_FORMAT2          -Wformat=2 )
  if( NOT HAVE_W_FORMAT2_C )
    try_c_cxx_flag( HAVE_W_FORMAT         -Wformat )
  endif()
  try_c_flag    ( HAVE_W_0LEN_FMT         -Wno-format-zero-length )
  try_c_cxx_flag( HAVE_W_FORMAT_SECURITY  -Wformat-security )
  try_c_cxx_flag( HAVE_W_STRICT_ALIAS_2   -Wstrict-aliasing=2 )
  try_c_cxx_flag( HAVE_W_MISSING_FMT_ATTR -Wmissing-format-attribute )
  try_c_cxx_flag( HAVE_W_MISSING_NORETURN -Wmissing-noreturn )
  try_c_flag    ( HAVE_W_IMPLICIT_FN      -Wimplicit-function-declaration )
  if( HAVE_W_IMPLICIT_FN_C )
    try_c_cxx_flag( HAVE_W_ERR_IMPLICIT_FN  -Werror=implicit-function-declaration )
  endif()
  try_c_cxx_flag( HAVE_W_FMT_SECURITY     -Wformat-security )
  if( HAVE_W_FMT_SECURITY_C )
    try_c_cxx_flag( HAVE_W_ERR_FMT_SECURITY -Werror=format-security )
  endif()
endif()

# Hardening
if( ENABLE_HARDENING )
  try_c_cxx_flag( HAVE_F_STACK_PROTECTOR  -fstack-protector )
endif()

##############
# Link flags #
##############

macro( try_linker_flag PROP FLAG )
  set( SAVED_FLAGS "${CMAKE_EXE_LINKER_FLAGS}" )
  set( CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} ${FLAG}" )
  # test it on an executable
  if( NOT ${PROP}_CACHED )
    try_compile( ${PROP} ${MOUNT_DIR}/tests ${MOUNT_DIR}/tests/test.c
                 CMAKE_FLAGS -DCMAKE_EXE_LINKER_FLAGS=${CMAKE_EXE_LINKER_FLAGS} )
  endif()
  if( ${PROP} )
    if( NOT ${PROP}_CACHED )
      message( STATUS "Using linker flag ${FLAG}" )
    endif()
    # assume that it works for shared & static libs too
    set( CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} ${FLAG}" )
    set( CMAKE_MODULE_LINKER_FLAGS "${CMAKE_MODULE_LINKER_FLAGS} ${FLAG}" )
  else()
    if( NOT ${PROP}_CACHED )
      message( STATUS "Linker flag ${FLAG} is not available" )
    endif()
    set( CMAKE_EXE_LINKER_FLAGS "${SAVED_FLAGS}" )
  endif()
  set( ${PROP}_CACHED TRUE CACHE INTERNAL "Set if linker option ${FLAG} has been checked for." )
endmacro()

# Can we get rid of unneeded external libraries?
try_linker_flag( HAVE_AS_NEEDED -Wl,--as-needed )

# If we can, fail if there are unresolved symbols
try_linker_flag( HAVE_Z_DEFS -Wl,-z,defs )

if( ENABLE_HARDENING )
  if( ENABLE_FSTACKPROT )
    try_linker_flag( HAVE_FSTACKPROT "-fstack-protector --param=ssp-buffer-size=4" )
  endif()
  if( ENABLE_RELRO )
    try_linker_flag( HAVE_Z_RELRO_Z_NOW -Wl,-z,relro,-z,now )
  endif()
endif()

######################
# Define Build Files #
######################

set( SHAREDLIST
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
)

set( OPENEXRLIST
  ${MOUNT_DIR}/libs/openexr/half.cpp
)

set( RENDERERLIST
  ${MOUNT_DIR}/engine/renderer/tr_animation.c
  ${MOUNT_DIR}/engine/renderer/tr_animation_mdm.c
  ${MOUNT_DIR}/engine/renderer/tr_animation_mds.c
  ${MOUNT_DIR}/engine/renderer/tr_backend.c
  ${MOUNT_DIR}/engine/renderer/tr_bsp.c
  ${MOUNT_DIR}/engine/renderer/tr_cmds.c
  ${MOUNT_DIR}/engine/renderer/tr_cmesh.c
  ${MOUNT_DIR}/engine/renderer/tr_curve.c
  ${MOUNT_DIR}/engine/renderer/tr_decals.c
  ${MOUNT_DIR}/engine/renderer/tr_flares.c
  ${MOUNT_DIR}/engine/renderer/tr_font.c
  ${MOUNT_DIR}/engine/renderer/tr_image.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_webp.c
  ${MOUNT_DIR}/engine/renderer/tr_init.c
  ${MOUNT_DIR}/engine/renderer/tr_light.c
  ${MOUNT_DIR}/engine/renderer/tr_main.c
  ${MOUNT_DIR}/engine/renderer/tr_marks.c
  ${MOUNT_DIR}/engine/renderer/tr_mesh.c
  ${MOUNT_DIR}/engine/renderer/tr_model.c
  ${MOUNT_DIR}/engine/renderer/tr_model_md5.c
  ${MOUNT_DIR}/engine/renderer/tr_noise.c
  ${MOUNT_DIR}/engine/renderer/tr_scene.c
  ${MOUNT_DIR}/engine/renderer/tr_shade.c
  ${MOUNT_DIR}/engine/renderer/tr_shade_calc.c
  ${MOUNT_DIR}/engine/renderer/tr_shader.c
  ${MOUNT_DIR}/engine/renderer/tr_shadows.c
  ${MOUNT_DIR}/engine/renderer/tr_skin.c
  ${MOUNT_DIR}/engine/renderer/tr_sky.c
  ${MOUNT_DIR}/engine/renderer/tr_surface.c
  ${MOUNT_DIR}/engine/renderer/tr_world.c
)

set( RENDERERGLLIST
  ${MOUNT_DIR}/engine/rendererGL/gl_shader.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_animation_mdm.c
  ${MOUNT_DIR}/engine/rendererGL/tr_animation.c
  ${MOUNT_DIR}/engine/rendererGL/tr_backend.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_bsp.c
  ${MOUNT_DIR}/engine/rendererGL/tr_cmds.c
  ${MOUNT_DIR}/engine/rendererGL/tr_curve.c
  ${MOUNT_DIR}/engine/rendererGL/tr_decals.c
  ${MOUNT_DIR}/engine/rendererGL/tr_fbo.c
  ${MOUNT_DIR}/engine/rendererGL/tr_flares.c
  ${MOUNT_DIR}/engine/rendererGL/tr_fog.c
  ${MOUNT_DIR}/engine/rendererGL/tr_font.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_dds.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_exr.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_image_jpg.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_png.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_tga.c
  ${MOUNT_DIR}/engine/rendererGL/tr_image_webp.c
  ${MOUNT_DIR}/engine/rendererGL/tr_init.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_light.c
  ${MOUNT_DIR}/engine/rendererGL/tr_main.c
  ${MOUNT_DIR}/engine/rendererGL/tr_marks.c
  ${MOUNT_DIR}/engine/rendererGL/tr_mesh.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_md3.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_md5.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_mdc.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_mdm.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_psk.c
  ${MOUNT_DIR}/engine/rendererGL/tr_model_skel.c
  ${MOUNT_DIR}/engine/rendererGL/tr_noise.c
  ${MOUNT_DIR}/engine/rendererGL/tr_scene.c
  ${MOUNT_DIR}/engine/rendererGL/tr_shade.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_shade_calc.c
  ${MOUNT_DIR}/engine/rendererGL/tr_shader.c
  ${MOUNT_DIR}/engine/rendererGL/tr_shadows.c
  ${MOUNT_DIR}/engine/rendererGL/tr_skin.c
  ${MOUNT_DIR}/engine/rendererGL/tr_sky.cpp
  ${MOUNT_DIR}/engine/rendererGL/tr_surface.c
  ${MOUNT_DIR}/engine/rendererGL/tr_vbo.c
  ${MOUNT_DIR}/engine/rendererGL/tr_world.cpp
)

set( SERVERLIST
  ${MOUNT_DIR}/engine/server/sv_bot.c
  ${MOUNT_DIR}/engine/server/sv_ccmds.c
  ${MOUNT_DIR}/engine/server/sv_client.c
  ${MOUNT_DIR}/engine/server/sv_game.c
  ${MOUNT_DIR}/engine/server/sv_init.c
  ${MOUNT_DIR}/engine/server/sv_main.c
  ${MOUNT_DIR}/engine/server/sv_net_chan.c
  ${MOUNT_DIR}/engine/server/sv_snapshot.c
  ${MOUNT_DIR}/engine/server/sv_world.c
)

set( QCOMMONLIST
  ${MOUNT_DIR}/engine/asm/ftola.c
  ${MOUNT_DIR}/engine/asm/snapvector.c
  ${MOUNT_DIR}/engine/qcommon/cm_load.c
  ${MOUNT_DIR}/engine/qcommon/cm_trisoup.c
  ${MOUNT_DIR}/engine/qcommon/cm_patch.c
  ${MOUNT_DIR}/engine/qcommon/cm_polylib.c
  ${MOUNT_DIR}/engine/qcommon/cm_test.c
  ${MOUNT_DIR}/engine/qcommon/cm_trace.c
  ${MOUNT_DIR}/engine/qcommon/cmd.c
  ${MOUNT_DIR}/engine/qcommon/common.c
  ${MOUNT_DIR}/engine/qcommon/crypto.c
  ${MOUNT_DIR}/engine/qcommon/cvar.c
  ${MOUNT_DIR}/engine/qcommon/files.c
  ${MOUNT_DIR}/engine/qcommon/htable.c
  ${MOUNT_DIR}/engine/qcommon/huffman.c
  ${MOUNT_DIR}/engine/qcommon/md4.c
  ${MOUNT_DIR}/engine/qcommon/md5.c
  ${MOUNT_DIR}/engine/qcommon/msg.c
  ${MOUNT_DIR}/engine/qcommon/net_chan.c
  ${MOUNT_DIR}/engine/qcommon/net_ip.c
  ${MOUNT_DIR}/engine/qcommon/parse.c
  ${MOUNT_DIR}/engine/qcommon/unzip.c
  ${MOUNT_DIR}/engine/qcommon/ioapi.c
  ${MOUNT_DIR}/engine/qcommon/vm.c
  ${MOUNT_DIR}/engine/qcommon/vm_x86.c
  ${MOUNT_DIR}/engine/qcommon/vm_interpreted.c
)

if( USE_LLVM )
  set( QCOMMONLIST ${QCOMMONLIST} ${MOUNT_DIR}/engine/qcommon/vm_llvm.cpp )
  add_definitions( -DUSE_LLVM -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )
  find_program(LLVM_CONFIG llvm-config)
  execute_process(COMMAND ${LLVM_CONFIG} --libfiles
  			    OUTPUT_VARIABLE LLVM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${LLVM_CONFIG} --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${LLVM_CONFIG} --version OUTPUT_VARIABLE LLVM_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${LLVM_CONFIG} --libdir OUTPUT_VARIABLE LLVM_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(LLVM_LIBS)
endif()

if( USE_PHYSICS )
  set( QCOMMONLIST ${QCOMMONLIST} ${MOUNT_DIR}/engine/qcommon/cm_physics.cpp )
endif()

if( USE_MYSQL )
  set( DATABASELIST
    ${MOUNT_DIR}/engine/database/db_main.c
    ${MOUNT_DIR}/engine/database/db_mysql.c
  )
endif()

set( CLIENTLIST
  ${MOUNT_DIR}/engine/client/cin_ogm.c
  ${MOUNT_DIR}/engine/client/cl_avi.c
  ${MOUNT_DIR}/engine/client/cl_cgame.c
  ${MOUNT_DIR}/engine/client/cl_cin.c
  ${MOUNT_DIR}/engine/client/cl_console.c
  ${MOUNT_DIR}/engine/client/cl_input.c
  ${MOUNT_DIR}/engine/client/cl_irc.c
  ${MOUNT_DIR}/engine/client/cl_keys.c
  ${MOUNT_DIR}/engine/client/cl_main.c
  ${MOUNT_DIR}/engine/client/cl_net_chan.c
  ${MOUNT_DIR}/engine/client/cl_parse.c
  ${MOUNT_DIR}/engine/client/cl_scrn.c
  ${MOUNT_DIR}/engine/client/cl_ui.c
  ${MOUNT_DIR}/engine/client/snd_adpcm.c
  ${MOUNT_DIR}/engine/client/snd_codec.c
  ${MOUNT_DIR}/engine/client/snd_codec_ogg.c
  ${MOUNT_DIR}/engine/client/snd_codec_wav.c
  ${MOUNT_DIR}/engine/client/snd_dma.c
  ${MOUNT_DIR}/engine/client/snd_load.c
  ${MOUNT_DIR}/engine/client/snd_mem.c
  ${MOUNT_DIR}/engine/client/snd_mix.c
  ${MOUNT_DIR}/engine/client/snd_wavelet.c
)

if( USE_MUMBLE )
  set( CLIENTLIST ${CLIENTLIST} ${MOUNT_DIR}/engine/client/libmumblelink.c )
endif()

set( ALLIST
  ${MOUNT_DIR}/engine/snd_openal/qal.c
  ${MOUNT_DIR}/engine/snd_openal/snd_al_buffers.c
  ${MOUNT_DIR}/engine/snd_openal/snd_al_main.c
  ${MOUNT_DIR}/engine/snd_openal/snd_al_music.c
  ${MOUNT_DIR}/engine/snd_openal/snd_al_sources.c
  ${MOUNT_DIR}/engine/snd_openal/snd_al_stream.c
)

if( USE_CPUINFO )
  set( CPUINFOLIST
    ${MOUNT_DIR}/libs/cpuinfo/CPUInfo.c
  )
  add_definitions( -DUSE_CPUINFO )
endif()

if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
  set( SDLBASELIST
    ${MOUNT_DIR}/engine/sys/sys_unix.c
    ${MOUNT_DIR}/engine/sys/sys_osx.m
    ${MOUNT_DIR}/engine/sys/sys_main.c
    ${MOUNT_DIR}/engine/sys/con_log.c
    ${MOUNT_DIR}/engine/sys/con_${CONSOLE}.c
  )
else()
  set( SDLBASELIST
    ${MOUNT_DIR}/engine/sys/sys_${PLATFORM}.c
    ${MOUNT_DIR}/engine/sys/sys_main.c
    ${MOUNT_DIR}/engine/sys/con_log.c
    ${MOUNT_DIR}/engine/sys/con_${CONSOLE}.c
  )
endif()

if( USE_CURSES )
  set( SDLBASELIST ${SDLBASELIST} ${MOUNT_DIR}/engine/sys/con_curses.c )
endif()

set( SDLLIST
  ${MOUNT_DIR}/engine/sys/sdl_input.c
  ${MOUNT_DIR}/engine/sys/sdl_snd.c
)

set( QAGAMELIST
  ${GAME_DIR}/src/game/bg_animation.c
  ${GAME_DIR}/src/game/bg_animgroup.c
  ${GAME_DIR}/src/game/bg_campaign.c
  ${GAME_DIR}/src/game/bg_character.c
  ${GAME_DIR}/src/game/bg_classes.c
  ${GAME_DIR}/src/game/bg_misc.c
  ${GAME_DIR}/src/game/bg_pmove.c
  ${GAME_DIR}/src/game/bg_slidemove.c
  ${GAME_DIR}/src/game/bg_sscript.c
  ${GAME_DIR}/src/game/bg_stats.c
  ${GAME_DIR}/src/game/bg_tracemap.c
  ${GAME_DIR}/src/game/g_active.c
  ${GAME_DIR}/src/game/g_admin.c
  ${GAME_DIR}/src/game/g_alarm.c
  ${GAME_DIR}/src/game/g_antilag.c
  ${GAME_DIR}/src/game/g_buddy_list.c
  ${GAME_DIR}/src/game/g_character.c
  ${GAME_DIR}/src/game/g_client.c
  ${GAME_DIR}/src/game/g_cmds.c
  ${GAME_DIR}/src/game/g_cmds_ext.c
  ${GAME_DIR}/src/game/g_combat.c
  ${GAME_DIR}/src/game/g_config.c
  ${GAME_DIR}/src/game/g_fireteams.c
  ${GAME_DIR}/src/game/g_items.c
  ${GAME_DIR}/src/game/g_main.c
  ${GAME_DIR}/src/game/g_match.c
  ${GAME_DIR}/src/game/g_mem.c
  ${GAME_DIR}/src/game/g_misc.c
  ${GAME_DIR}/src/game/g_missile.c
  ${GAME_DIR}/src/game/g_mover.c
  ${GAME_DIR}/src/game/g_multiview.c
  ${GAME_DIR}/src/game/g_props.c
  ${GAME_DIR}/src/game/g_referee.c
  ${GAME_DIR}/src/game/g_script.c
  ${GAME_DIR}/src/game/g_script_actions.c
  ${GAME_DIR}/src/game/g_session.c
  ${GAME_DIR}/src/game/g_spawn.c
  ${GAME_DIR}/src/game/g_stats.c
  ${GAME_DIR}/src/game/g_svcmds.c
  ${GAME_DIR}/src/game/g_api.c
  ${GAME_DIR}/src/game/g_systemmsg.c
  ${GAME_DIR}/src/game/g_target.c
  ${GAME_DIR}/src/game/g_team.c
  ${GAME_DIR}/src/game/g_teammapdata.c
  ${GAME_DIR}/src/game/g_trigger.c
  ${GAME_DIR}/src/game/g_utils.c
  ${GAME_DIR}/src/game/g_vote.c
  ${GAME_DIR}/src/game/g_weapon.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
)

set( CGAMELIST
  ${GAME_DIR}/src/cgame/cg_main.c
  ${GAME_DIR}/src/game/bg_animation.c
  ${GAME_DIR}/src/game/bg_animgroup.c
  ${GAME_DIR}/src/game/bg_character.c
  ${GAME_DIR}/src/game/bg_classes.c
  ${GAME_DIR}/src/game/bg_misc.c
  ${GAME_DIR}/src/game/bg_pmove.c
  ${GAME_DIR}/src/game/bg_slidemove.c
  ${GAME_DIR}/src/game/bg_sscript.c
  ${GAME_DIR}/src/game/bg_stats.c
  ${GAME_DIR}/src/game/bg_tracemap.c
  ${GAME_DIR}/src/cgame/cg_atmospheric.c
  ${GAME_DIR}/src/cgame/cg_character.c
  ${GAME_DIR}/src/cgame/cg_commandmap.c
  ${GAME_DIR}/src/cgame/cg_consolecmds.c
  ${GAME_DIR}/src/cgame/cg_debriefing.c
  ${GAME_DIR}/src/cgame/cg_draw.c
  ${GAME_DIR}/src/cgame/cg_drawtools.c
  ${GAME_DIR}/src/cgame/cg_effects.c
  ${GAME_DIR}/src/cgame/cg_ents.c
  ${GAME_DIR}/src/cgame/cg_event.c
  ${GAME_DIR}/src/cgame/cg_fireteamoverlay.c
  ${GAME_DIR}/src/cgame/cg_fireteams.c
  ${GAME_DIR}/src/cgame/cg_flamethrower.c
  ${GAME_DIR}/src/cgame/cg_info.c
  ${GAME_DIR}/src/cgame/cg_limbopanel.c
  ${GAME_DIR}/src/cgame/cg_loadpanel.c
  ${GAME_DIR}/src/cgame/cg_localents.c
  ${GAME_DIR}/src/cgame/cg_marks.c
  ${GAME_DIR}/src/cgame/cg_missionbriefing.c
  ${GAME_DIR}/src/cgame/cg_multiview.c
  ${GAME_DIR}/src/cgame/cg_newDraw.c
  ${GAME_DIR}/src/cgame/cg_particles.c
  ${GAME_DIR}/src/cgame/cg_players.c
  ${GAME_DIR}/src/cgame/cg_playerstate.c
  ${GAME_DIR}/src/cgame/cg_polybus.c
  ${GAME_DIR}/src/cgame/cg_popupmessages.c
  ${GAME_DIR}/src/cgame/cg_predict.c
  ${GAME_DIR}/src/cgame/cg_scoreboard.c
  ${GAME_DIR}/src/cgame/cg_servercmds.c
  ${GAME_DIR}/src/cgame/cg_snapshot.c
  ${GAME_DIR}/src/cgame/cg_sound.c
  ${GAME_DIR}/src/cgame/cg_spawn.c
  ${GAME_DIR}/src/cgame/cg_statsranksmedals.c
  ${GAME_DIR}/src/cgame/cg_api.c
  ${GAME_DIR}/src/cgame/cg_trails.c
  ${GAME_DIR}/src/cgame/cg_view.c
  ${GAME_DIR}/src/cgame/cg_weapons.c
  ${GAME_DIR}/src/cgame/cg_window.c
  ${GAME_DIR}/src/ui/ui_shared.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
)

set( OMNIBOTLIST
  ${GAME_DIR}/omnibot/common/BotLoadLibrary.cpp
  ${GAME_DIR}/omnibot/et/g_etbot_interface.cpp
)

set( UILIST
  ${GAME_DIR}/src/ui/ui_atoms.c
  ${GAME_DIR}/src/ui/ui_gameinfo.c
  ${GAME_DIR}/src/ui/ui_loadpanel.c
  ${GAME_DIR}/src/ui/ui_main.c
  ${GAME_DIR}/src/ui/ui_players.c
  ${GAME_DIR}/src/ui/ui_shared.c
  ${GAME_DIR}/src/ui/ui_api.c
  ${GAME_DIR}/src/ui/ui_util.c
  ${GAME_DIR}/src/game/bg_campaign.c
  ${GAME_DIR}/src/game/bg_classes.c
  ${GAME_DIR}/src/game/bg_misc.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
)

set( CGAMETREM
  ${BASE_DIR}/src/game/bg_misc.c
  ${BASE_DIR}/src/game/bg_pmove.c
  ${BASE_DIR}/src/game/bg_slidemove.c
  ${BASE_DIR}/src/ui/ui_shared.c
  ${BASE_DIR}/src/cgame/cg_animation.c
  ${BASE_DIR}/src/cgame/cg_animmapobj.c
  ${BASE_DIR}/src/cgame/cg_attachment.c
  ${BASE_DIR}/src/cgame/cg_buildable.c
  ${BASE_DIR}/src/cgame/cg_consolecmds.c
  ${BASE_DIR}/src/cgame/cg_draw.c
  ${BASE_DIR}/src/cgame/cg_drawtools.c
  ${BASE_DIR}/src/cgame/cg_ents.c
  ${BASE_DIR}/src/cgame/cg_event.c
  ${BASE_DIR}/src/cgame/cg_main.c
  ${BASE_DIR}/src/cgame/cg_marks.c
  ${BASE_DIR}/src/cgame/cg_mem.c
  ${BASE_DIR}/src/cgame/cg_particles.c
  ${BASE_DIR}/src/cgame/cg_players.c
  ${BASE_DIR}/src/cgame/cg_playerstate.c
  ${BASE_DIR}/src/cgame/cg_predict.c
  ${BASE_DIR}/src/cgame/cg_ptr.c
  ${BASE_DIR}/src/cgame/cg_scanner.c
  ${BASE_DIR}/src/cgame/cg_servercmds.c
  ${BASE_DIR}/src/cgame/cg_snapshot.c
  ${BASE_DIR}/src/cgame/cg_trails.c
  ${BASE_DIR}/src/cgame/cg_tutorial.c
  ${BASE_DIR}/src/cgame/cg_view.c
  ${BASE_DIR}/src/cgame/cg_weapons.c
  ${BASE_DIR}/src/cgame/cg_api.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
)

set( GAMETREM
  ${BASE_DIR}/src/game/bg_misc.c
  ${BASE_DIR}/src/game/bg_pmove.c
  ${BASE_DIR}/src/game/bg_slidemove.c
  ${BASE_DIR}/src/game/g_active.c
  ${BASE_DIR}/src/game/g_buildable.c
  ${BASE_DIR}/src/game/g_client.c
  ${BASE_DIR}/src/game/g_cmds.c
  ${BASE_DIR}/src/game/g_combat.c
  ${BASE_DIR}/src/game/g_main.c
  ${BASE_DIR}/src/game/g_maprotation.c
  ${BASE_DIR}/src/game/g_mem.c
  ${BASE_DIR}/src/game/g_misc.c
  ${BASE_DIR}/src/game/g_missile.c
  ${BASE_DIR}/src/game/g_mover.c
  ${BASE_DIR}/src/game/g_physics.c
  ${BASE_DIR}/src/game/g_ptr.c
  ${BASE_DIR}/src/game/g_session.c
  ${BASE_DIR}/src/game/g_spawn.c
  ${BASE_DIR}/src/game/g_svcmds.c
  ${BASE_DIR}/src/game/g_target.c
  ${BASE_DIR}/src/game/g_team.c
  ${BASE_DIR}/src/game/g_trigger.c
  ${BASE_DIR}/src/game/g_utils.c
  ${BASE_DIR}/src/game/g_weapon.c
  ${BASE_DIR}/src/game/g_api.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
)

set( UITREM
  ${BASE_DIR}/src/ui/ui_atoms.c
  ${BASE_DIR}/src/ui/ui_gameinfo.c
  ${BASE_DIR}/src/ui/ui_main.c
  ${BASE_DIR}/src/ui/ui_players.c
  ${BASE_DIR}/src/ui/ui_shared.c
  ${BASE_DIR}/src/game/bg_misc.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
  ${BASE_DIR}/src/ui/ui_api.c
)

set( CGAMEGPP
  ${GPP_DIR}/src/cgame/cg_main.c
  ${GPP_DIR}/src/game/bg_misc.c
  ${GPP_DIR}/src/game/bg_pmove.c
  ${GPP_DIR}/src/game/bg_slidemove.c
  ${GPP_DIR}/src/game/bg_lib.c
  ${GPP_DIR}/src/game/bg_alloc.c
  ${GPP_DIR}/src/game/bg_voice.c
  ${GPP_DIR}/src/cgame/cg_consolecmds.c
  ${GPP_DIR}/src/cgame/cg_buildable.c
  ${GPP_DIR}/src/cgame/cg_animation.c
  ${GPP_DIR}/src/cgame/cg_animmapobj.c
  ${GPP_DIR}/src/cgame/cg_draw.c
  ${GPP_DIR}/src/cgame/cg_drawtools.c
  ${GPP_DIR}/src/cgame/cg_ents.c
  ${GPP_DIR}/src/cgame/cg_event.c
  ${GPP_DIR}/src/cgame/cg_marks.c
  ${GPP_DIR}/src/cgame/cg_players.c
  ${GPP_DIR}/src/cgame/cg_playerstate.c
  ${GPP_DIR}/src/cgame/cg_predict.c
  ${GPP_DIR}/src/cgame/cg_servercmds.c
  ${GPP_DIR}/src/cgame/cg_snapshot.c
  ${GPP_DIR}/src/cgame/cg_view.c
  ${GPP_DIR}/src/cgame/cg_weapons.c
  ${GPP_DIR}/src/cgame/cg_scanner.c
  ${GPP_DIR}/src/cgame/cg_attachment.c
  ${GPP_DIR}/src/cgame/cg_trails.c
  ${GPP_DIR}/src/cgame/cg_particles.c
  ${GPP_DIR}/src/cgame/cg_tutorial.c
  ${GPP_DIR}/src/ui/ui_shared.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
  ${GPP_DIR}/src/cgame/cg_api.c
)

set( GAMEGPP
  ${GPP_DIR}/src/game/g_main.c
  ${GPP_DIR}/src/game/bg_misc.c
  ${GPP_DIR}/src/game/bg_pmove.c
  ${GPP_DIR}/src/game/bg_slidemove.c
  ${GPP_DIR}/src/game/bg_lib.c
  ${GPP_DIR}/src/game/bg_alloc.c
  ${GPP_DIR}/src/game/bg_voice.c
  ${GPP_DIR}/src/game/g_active.c
  ${GPP_DIR}/src/game/g_client.c
  ${GPP_DIR}/src/game/g_cmds.c
  ${GPP_DIR}/src/game/g_combat.c
  ${GPP_DIR}/src/game/g_physics.c
  ${GPP_DIR}/src/game/g_buildable.c
  ${GPP_DIR}/src/game/g_misc.c
  ${GPP_DIR}/src/game/g_missile.c
  ${GPP_DIR}/src/game/g_mover.c
  ${GPP_DIR}/src/game/g_session.c
  ${GPP_DIR}/src/game/g_spawn.c
  ${GPP_DIR}/src/game/g_svcmds.c
  ${GPP_DIR}/src/game/g_target.c
  ${GPP_DIR}/src/game/g_team.c
  ${GPP_DIR}/src/game/g_trigger.c
  ${GPP_DIR}/src/game/g_utils.c
  ${GPP_DIR}/src/game/g_maprotation.c
  ${GPP_DIR}/src/game/g_weapon.c
  ${GPP_DIR}/src/game/g_admin.c
  ${GPP_DIR}/src/game/g_namelog.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
  ${GPP_DIR}/src/game/g_api.c
)

set( UIGPP
  ${GPP_DIR}/src/ui/ui_main.c
  ${GPP_DIR}/src/ui/ui_atoms.c
  ${GPP_DIR}/src/ui/ui_shared.c
  ${GPP_DIR}/src/ui/ui_gameinfo.c
  ${GPP_DIR}/src/game/bg_misc.c
  ${GPP_DIR}/src/game/bg_lib.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
  ${GPP_DIR}/src/ui/ui_api.c
)

set( DAEMONMAPLIST
  ${MOUNT_DIR}/tools/common/cmdlib.c
  ${MOUNT_DIR}/tools/common/imagelib.c
  ${MOUNT_DIR}/tools/common/inout.c
  ${MOUNT_DIR}/tools/common/mathlib.c
  ${MOUNT_DIR}/tools/common/md4.c
  ${MOUNT_DIR}/tools/common/mutex.c
  ${MOUNT_DIR}/tools/common/polylib.c
  ${MOUNT_DIR}/tools/common/scriplib.c
  ${MOUNT_DIR}/tools/common/threads.c
  ${MOUNT_DIR}/tools/common/unzip.c
  ${MOUNT_DIR}/tools/common/vfs.c
  ${MOUNT_DIR}/tools/common/ddslib.c
  ${MOUNT_DIR}/libs/picomodel/picointernal.c
  ${MOUNT_DIR}/libs/picomodel/picomodel.c
  ${MOUNT_DIR}/libs/picomodel/picomodules.c
  ${MOUNT_DIR}/libs/picomodel/pm_3ds.c
  ${MOUNT_DIR}/libs/picomodel/pm_ase.c
  ${MOUNT_DIR}/libs/picomodel/pm_fm.c
  ${MOUNT_DIR}/libs/picomodel/pm_lwo.c
  ${MOUNT_DIR}/libs/picomodel/pm_md2.c
  ${MOUNT_DIR}/libs/picomodel/pm_md3.c
  ${MOUNT_DIR}/libs/picomodel/pm_mdc.c
  ${MOUNT_DIR}/libs/picomodel/pm_ms3d.c
  ${MOUNT_DIR}/libs/picomodel/pm_obj.c
  ${MOUNT_DIR}/libs/picomodel/pm_terrain.c
  ${MOUNT_DIR}/libs/picomodel/lwo/clip.c
  ${MOUNT_DIR}/libs/picomodel/lwo/envelope.c
  ${MOUNT_DIR}/libs/picomodel/lwo/list.c
  ${MOUNT_DIR}/libs/picomodel/lwo/lwio.c
  ${MOUNT_DIR}/libs/picomodel/lwo/lwo2.c
  ${MOUNT_DIR}/libs/picomodel/lwo/lwob.c
  ${MOUNT_DIR}/libs/picomodel/lwo/pntspols.c
  ${MOUNT_DIR}/libs/picomodel/lwo/surface.c
  ${MOUNT_DIR}/libs/picomodel/lwo/vecmath.c
  ${MOUNT_DIR}/libs/picomodel/lwo/vmap.c
  ${MOUNT_DIR}/tools/owmap/brush.c
  ${MOUNT_DIR}/tools/owmap/fog.c
  ${MOUNT_DIR}/tools/owmap/brush_primit.c
  ${MOUNT_DIR}/tools/owmap/bsp.c
  ${MOUNT_DIR}/tools/owmap/bspfile_abstract.c
  ${MOUNT_DIR}/tools/owmap/bspfile_ibsp.c
  ${MOUNT_DIR}/tools/owmap/bspfile_rbsp.c
  ${MOUNT_DIR}/tools/owmap/bspfile_xbsp.c
  ${MOUNT_DIR}/tools/owmap/convert_ase.c
  ${MOUNT_DIR}/tools/owmap/convert_map.c
  ${MOUNT_DIR}/tools/owmap/decals.c
  ${MOUNT_DIR}/tools/owmap/facebsp.c
  ${MOUNT_DIR}/tools/owmap/gldraw.c
  ${MOUNT_DIR}/tools/owmap/model.c
  ${MOUNT_DIR}/tools/owmap/image.c
  ${MOUNT_DIR}/tools/owmap/leakfile.c
  ${MOUNT_DIR}/tools/owmap/light.c
  ${MOUNT_DIR}/tools/owmap/light_bounce.c
  ${MOUNT_DIR}/tools/owmap/light_trace.c
  ${MOUNT_DIR}/tools/owmap/light_ydnar.c
  ${MOUNT_DIR}/tools/owmap/lightmaps_ydnar.c
  ${MOUNT_DIR}/tools/owmap/main.c
  ${MOUNT_DIR}/tools/owmap/map.c
  ${MOUNT_DIR}/tools/owmap/mesh.c
  ${MOUNT_DIR}/tools/owmap/patch.c
  ${MOUNT_DIR}/tools/owmap/prtfile.c
  ${MOUNT_DIR}/tools/owmap/path_init.c
  ${MOUNT_DIR}/tools/owmap/portals.c
  ${MOUNT_DIR}/tools/owmap/shaders.c
  ${MOUNT_DIR}/tools/owmap/writebsp.c
  ${MOUNT_DIR}/tools/owmap/surface_abstract.c
  ${MOUNT_DIR}/tools/owmap/surface_extra.c
  ${MOUNT_DIR}/tools/owmap/surface_foliage.c
  ${MOUNT_DIR}/tools/owmap/surface_fur.c
  ${MOUNT_DIR}/tools/owmap/surface_meta.c
  ${MOUNT_DIR}/tools/owmap/tjunction.c
  ${MOUNT_DIR}/tools/owmap/tree.c
  ${MOUNT_DIR}/tools/owmap/vis.c
  ${MOUNT_DIR}/tools/owmap/visflow.c
  ${MOUNT_DIR}/libs/zlib/adler32.c
  ${MOUNT_DIR}/libs/zlib/compress.c
  ${MOUNT_DIR}/libs/zlib/crc32.c
  ${MOUNT_DIR}/libs/zlib/deflate.c
  ${MOUNT_DIR}/libs/zlib/gzio.c
  ${MOUNT_DIR}/libs/zlib/infback.c
  ${MOUNT_DIR}/libs/zlib/inffast.c
  ${MOUNT_DIR}/libs/zlib/inflate.c
  ${MOUNT_DIR}/libs/zlib/inftrees.c
  ${MOUNT_DIR}/libs/zlib/ioapi.c
  ${MOUNT_DIR}/libs/zlib/trees.c
  ${MOUNT_DIR}/libs/zlib/uncompr.c
  ${MOUNT_DIR}/libs/zlib/zutil.c
)

if( USE_VOIP AND USE_INTERNAL_SPEEX )
    set( SPEEXLIST
    ${MOUNT_DIR}/libs/libspeex/libspeex/bits.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/buffer.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/cb_search.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_10_16_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_10_32_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_20_32_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_5_256_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_5_64_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/exc_8_128_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/fftwrap.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/filterbank.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/filters.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/gain_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/gain_table_lbr.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/hexc_10_32_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/hexc_table.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/high_lsp_tables.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/jitter.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/kiss_fft.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/kiss_fftr.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/lpc.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/lsp.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/lsp_tables_nb.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/ltp.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/mdf.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/modes.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/modes_wb.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/nb_celp.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/preprocess.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/quant_lsp.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/resample.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/sb_celp.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/smallft.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/speex.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/speex_callbacks.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/speex_header.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/stereo.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/vbr.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/vq.c
    ${MOUNT_DIR}/libs/libspeex/libspeex/window.c
  )
endif()

if( USE_INTERNAL_CRYPTO )
  set( CRYPTOLIST
    ${MOUNT_DIR}/libs/nettle/nettle/bignum.c
    ${MOUNT_DIR}/libs/nettle/nettle/bignum-random.c
    ${MOUNT_DIR}/libs/nettle/nettle/buffer-init.c
    ${MOUNT_DIR}/libs/nettle/nettle/nettle-buffer.c
    ${MOUNT_DIR}/libs/nettle/nettle/nettle-internal.c
    ${MOUNT_DIR}/libs/nettle/nettle/rsa2sexp.c
    ${MOUNT_DIR}/libs/nettle/nettle/rsa.c
    ${MOUNT_DIR}/libs/nettle/nettle/rsa-decrypt.c
    ${MOUNT_DIR}/libs/nettle/nettle/rsa-encrypt.c
    ${MOUNT_DIR}/libs/nettle/nettle/rsa-keygen.c
    ${MOUNT_DIR}/libs/nettle/nettle/rsa-sign.c
    ${MOUNT_DIR}/libs/nettle/nettle/sexp2bignum.c
    ${MOUNT_DIR}/libs/nettle/nettle/sexp2rsa.c
    ${MOUNT_DIR}/libs/nettle/nettle/sexp.c
    ${MOUNT_DIR}/libs/nettle/nettle/sexp-format.c
  )
endif()

####################
# End of file list #
####################

if( USE_PHYSICS )
  set( NEWTON_INCLUDES
    ${MOUNT_DIR}/libs/libnewton/include
    ${MOUNT_DIR}/libs/libnewton/include/dAnimation
    ${MOUNT_DIR}/libs/libnewton/include/dContainers
    ${MOUNT_DIR}/libs/libnewton/include/dCustomJoints
    ${MOUNT_DIR}/libs/libnewton/include/dMath
  )
endif()

if( WIN32 )
  add_definitions( -DWINVER=0x501 )
  #set( OS_LIBRARIES m winmm wsock32 gdi32 ole32 ws2_32 psapi Iphlpapi advapi32 shell32 user32 )
  set( OS_LIBRARIES m winmm ws2_32 psapi libz )
  set( EXTRALIBS dinput dsound )
elseif( UNIX )
  if( CMAKE_SYSTEM_NAME STREQUAL "FreeBSD" )
    set( OS_LIBRARIES m z )
  elseif( CMAKE_SYSTEM_NAME STREQUAL "Darwin" )
    set( OS_LIBRARIES dl m z )
    # FIXME This looks wrong.
    set( OS_LIBRARIES ${OS_LIBRARIES} "-framework AGL -framework OpenGL -framework Carbon -framework IOKit" )
  else()
    set( OS_LIBRARIES ${CMAKE_DL_LIBS} m z rt )
  endif()
endif()

if( NO_UNTRUSTED_PLUGINS )
  add_definitions( -DNO_UNTRUSTED_PLUGINS )
endif()

###############################
# Build Game/Cgame/Ui DLL/.so #
###############################



if( BUILD_GAME_SO_ET )
  # Xcode places the game libraries in the incorrect location. This should fix it
  if( XCODE_VERSION )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "Release/etmain" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "Debug/etmain" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "MinSizeRel/etmain" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "RelWithDebInfo/etmain" )
  endif()

  add_library( game SHARED ${SHAREDLIST} ${QAGAMELIST} )
  set_property( TARGET game PROPERTY COMPILE_DEFINITIONS GAME GAMEDLL USE_REFENTITY_ANIMATIONSYSTEM )
  set_target_properties( game PROPERTIES
    OUTPUT_NAME "qagame.mp.${BUILD_ARCH}"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "etmain"
  )
  target_link_libraries( game -lm )
  add_library( cgame SHARED ${SHAREDLIST} ${CGAMELIST} )
  set_property( TARGET cgame PROPERTY COMPILE_DEFINITIONS CGAME CGAMEDLL USE_REFENTITY_ANIMATIONSYSTEM )
  set_target_properties( cgame PROPERTIES
    OUTPUT_NAME "cgame.mp.${BUILD_ARCH}"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "etmain"
  )
  target_link_libraries( cgame -lm )
  add_library( ui SHARED ${SHAREDLIST} ${UILIST} )
  set_property( TARGET ui PROPERTY COMPILE_DEFINITIONS UI UIDLL USE_REFENTITY_ANIMATIONSYSTEM )
  set_target_properties( ui PROPERTIES
    OUTPUT_NAME "ui.mp.${BUILD_ARCH}"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "etmain"
  )
  target_link_libraries( ui -lm )
  install( TARGETS game cgame ui
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
endif()

if( BUILD_GAME_SO_TREM )
  # Xcode places the game libraries in the incorrect location. This should fix it
  if( XCODE_VERSION )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "Release/base" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "Debug/base" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "MinSizeRel/base" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "RelWithDebInfo/base" )
  endif()

  add_library( tremgame SHARED ${GAMETREM} )
  set_property( TARGET tremgame PROPERTY COMPILE_DEFINITIONS GAME GAMEDLL USE_REFENTITY_ANIMATIONSYSTEM )
  target_link_libraries( tremgame -lm )
  set_target_properties( tremgame PROPERTIES
    OUTPUT_NAME "qagame.mp.${BUILD_ARCH}"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "base"
  )
  add_library( cgametrem SHARED ${CGAMETREM} )
  set_property( TARGET cgametrem PROPERTY COMPILE_DEFINITIONS CGAME CGAMEDLL USE_REFENTITY_ANIMATIONSYSTEM )
  target_link_libraries( cgametrem -lm )
  set_target_properties( cgametrem PROPERTIES
    OUTPUT_NAME "cgame.mp.${BUILD_ARCH}"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "base"
  )
  add_library( uitrem SHARED ${UITREM} )
  set_property( TARGET uitrem PROPERTY COMPILE_DEFINITIONS UI UIDLL USE_REFENTITY_ANIMATIONSYSTEM )
  target_link_libraries( uitrem -lm )
  set_target_properties( uitrem PROPERTIES
    OUTPUT_NAME "ui.mp.${BUILD_ARCH}"
    PREFIX ""
    LIBRARY_OUTPUT_DIRECTORY "base"
  )
endif()

if( BUILD_GAME_SO_GPP )
  if( XCODE_VERSION )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "Release/main" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "Debug/main" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "MinSizeRel/main" )
    set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "RelWithDebInfo/main" )
  endif()

  add_library( gppgame SHARED ${GAMEGPP} )
  set_property( TARGET gppgame PROPERTY COMPILE_DEFINITIONS GAME GAMEDLL GPP USE_REFENTITY_ANIMATIONSYSTEM )
  if( WIN32 )
    set_target_properties( gppgame PROPERTIES
      OUTPUT_NAME "qagame_mp_${BUILD_ARCH}"
      PREFIX ""
      RUNTIME_OUTPUT_DIRECTORY "main"
      ARCHIVE_OUTPUT_DIRECTORY "main"
    )
  else()
    set_target_properties( gppgame PROPERTIES
      OUTPUT_NAME "qagame.mp.${BUILD_ARCH}"
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY "main"
    )
    target_link_libraries( gppgame -lm )
  endif()
  add_library( gpptrem SHARED ${CGAMEGPP} )
  set_property( TARGET gpptrem PROPERTY COMPILE_DEFINITIONS CGAME CGAMEDLL GPP USE_REFENTITY_ANIMATIONSYSTEM )
  if( WIN32 )
    set_target_properties( gpptrem PROPERTIES
      OUTPUT_NAME "cgame_mp_${BUILD_ARCH}"
      PREFIX ""
      RUNTIME_OUTPUT_DIRECTORY "main"
      ARCHIVE_OUTPUT_DIRECTORY "main"
    )
  else()
    set_target_properties( gpptrem PROPERTIES
      OUTPUT_NAME "cgame.mp.${BUILD_ARCH}"
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY "main"
      CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "main"
    )
    target_link_libraries( gpptrem -lm )
  endif()
  add_library( gppui SHARED ${UIGPP} )
  set_property( TARGET gppui PROPERTY COMPILE_DEFINITIONS UI UIDLL GPP USE_REFENTITY_ANIMATIONSYSTEM )
  if( WIN32 )
    set_target_properties( gppui PROPERTIES
      OUTPUT_NAME "ui_mp_${BUILD_ARCH}"
      PREFIX ""
      RUNTIME_OUTPUT_DIRECTORY "main"
      ARCHIVE_OUTPUT_DIRECTORY "main"
    )
  else()
    set_target_properties( gppui PROPERTIES
      OUTPUT_NAME "ui.mp.${BUILD_ARCH}"
      PREFIX "/"
      LIBRARY_OUTPUT_DIRECTORY "main"
    )
    target_link_libraries( gppui -lm )
  endif()
endif()

# Disable fixes for game libraries under Xcode
if( XCODE_VERSION )
  unset( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE )
  unset( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG )
  unset( CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL )
  unset( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO )
endif()

###################
# Build Renderers #
###################

if( USE_WEBP )
  find_package( WebP REQUIRED )
endif()

if( USE_PHYSICS )
  if( LINUX )
    if( BUILD_ARCH STREQUAL "x86_64" )
      set( NEWTON_LIBRARY CACHE FILEPATH "${MOUNT_DIR}/libs/libnewton/libs/linux64/libNewton.a" )
    elseif( BUILD_ARCH STREQUAL "i386" )
      set( NEWTON_LIBRARY CACHE FILEPATH "${MOUNT_DIR}/libs/libnewton/libs/linux32/libNewton.a" )
    endif()
  else()
    set( NEWTON_LIBRARY CACHE FILEPATH "Location of the Newton library" )
  endif()
endif()

if( BUILD_OLD_RENDERER OR BUILD_NEW_RENDERER )
  find_package( OpenGL REQUIRED )
  find_package( SDL REQUIRED )
  find_package( JPEG REQUIRED )
  find_package( PNG REQUIRED )
  find_package( Freetype REQUIRED )
  find_package( GLEW REQUIRED )
  if( USE_GLSL_OPTIMIZER AND BUILD_NEW_RENDERER )
    include_directories( ${LIB_DIR}/glsl-optimizer/include )
    include_directories( ${LIB_DIR}/glsl-optimizer/src/mesa )
    include_directories( ${LIB_DIR}/glsl-optimizer/src/mapi )
    include_directories( ${LIB_DIR}/glsl-optimizer/src/glsl )
    file(GLOB glcpp-library_sources ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/*.c)
    file(GLOB glcpp-library_sources_remove ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/glcpp.c)
    list(REMOVE_ITEM glcpp-library_sources ${glcpp-library_sources_remove})
    set_source_files_properties( ${glcpp-library_sources} PROPERTIES COMPILE_FLAGS "-fPIC" )
    add_library(glcpp-library ${glcpp-library_sources})

    file(GLOB mesa_sources ${LIB_DIR}/glsl-optimizer/src/mesa/program/*.c)
    set_source_files_properties( ${mesa_sources} PROPERTIES COMPILE_FLAGS "-fPIC" )
    add_library(mesa ${mesa_sources})

    file(GLOB glsl_sources ${LIB_DIR}/glsl-optimizer/src/glsl/*.cpp ${LIB_DIR}/glsl-optimizer/src/glsl/*.c)
    file(GLOB glsl_sources_remove ${LIB_DIR}/glsl-optimizer/src/glsl/main.cpp ${LIB_DIR}/glsl-optimizer/src/glsl/builtin_stubs.cpp)
    list(REMOVE_ITEM glsl_sources ${glsl_sources_remove})
    set_source_files_properties( ${glsl_sources} PROPERTIES COMPILE_FLAGS "-fPIC" )
    add_library(glsl_optimizer ${glsl_sources})
    target_link_libraries(glsl_optimizer glcpp-library mesa)
  endif()
  if( BUILD_NEW_RENDERER )
    add_library( rendererGL SHARED ${SHAREDLIST} ${OPENEXRLIST} ${RENDERERGLLIST} ${MOUNT_DIR}/engine/sys/sdl_glimp.c ${MOUNT_DIR}/engine/sys/sdl_gamma.c )
    include_directories( ${PNG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR} ${SDL_INCLUDE_DIR} )
    set_property( TARGET rendererGL PROPERTY COMPILE_DEFINITIONS BUILD_FREETYPE FT_FREETYPE USE_XREAL_RENDERER USE_REFENTITY_ANIMATIONSYSTEM )
    if( USE_GLSL_OPTIMIZER )
      include_directories( ${LIB_DIR}/glsl-optimizer/include )
      target_link_libraries( rendererGL glsl_optimizer )
      set_property( TARGET rendererGL APPEND PROPERTY COMPILE_DEFINITIONS USE_GLSL_OPTIMIZER )
    endif()
    target_link_libraries( rendererGL ${OS_LIBRARIES} ${PNG_LIBRARIES} ${GLEW_LIBRARY} ${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${FREETYPE_LIBRARIES} ${SDL_LIBRARY} )
    if( WIN32 )
      set_target_properties( rendererGL PROPERTIES OUTPUT_NAME "rendererGL3${BUILD_ARCH}" PREFIX "" )
    else()
      set_target_properties( rendererGL PROPERTIES OUTPUT_NAME "librendererGL3${BUILD_ARCH}" PREFIX "" )
      target_link_libraries( rendererGL -lm )
    endif()
    if( WEBP_FOUND )
      set_property( TARGET rendererGL APPEND PROPERTY COMPILE_DEFINITIONS USE_WEBP )
      include_directories( ${WEBP_INCLUDE_DIRS} )
      target_link_libraries( rendererGL ${WEBP_LIBRARIES} )
    endif()
  endif()

  if( BUILD_OLD_RENDERER )
    add_library( renderer SHARED ${RENDERERLIST} ${MOUNT_DIR}/engine/sys/sdl_glimp.c ${MOUNT_DIR}/engine/sys/sdl_gamma.c ${SHAREDLIST} )
    include_directories( ${PNG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR} ${SDL_INCLUDE_DIR} )
    set_property( TARGET renderer PROPERTY COMPILE_DEFINITIONS BUILD_FREETYPE USE_REFENTITY_ANIMATIONSYSTEM )
    target_link_libraries( renderer ${PNG_LIBRARY} ${SDL_LIBRARY} ${GLEW_LIBRARY} ${OPENGL_LIBRARY} ${JPEG_LIBRARY} ${FREETYPE_LIBRARY} )
    if( WIN32 )
      set_target_properties( renderer PROPERTIES OUTPUT_NAME "rendererGL${BUILD_ARCH}" PREFIX "" )
    else()
      set_target_properties( renderer PROPERTIES OUTPUT_NAME "librendererGL${BUILD_ARCH}" PREFIX "" )
      target_link_libraries( renderer -lm )
    endif()
    if( WEBP_FOUND )
      set_property( TARGET renderer APPEND PROPERTY COMPILE_DEFINITIONS USE_WEBP )
      include_directories( ${WEBP_INCLUDE_DIR} )
      target_link_libraries( renderer ${WEBP_LIBRARY} )
    endif()
  endif()
endif()

################
# Build Client #
################

if( BUILD_CLIENT )
  # Setup target client
  add_executable( client ${SERVERLIST} ${CLIENTLIST} ${SDLBASELIST}
    ${CPUINFOLIST} ${SDLLIST} ${QCOMMONLIST} ${SHAREDLIST} ${CRYPTOLIST}
    ${DATABASELIST} ${MOUNT_DIR}/engine/qcommon/dl_main_curl.c ${SPEEXLIST} )
  add_dependencies( client rendererGL )
  include_directories( ${MOUNT_DIR}/libs/cpuinfo ${NEWTON_INCLUDES} )
  target_link_libraries( client ${OS_LIBRARIES} ${NEWTON_LIBRARY} )
  set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_XREAL_RENDERER COMPAT_ET USE_REFENTITY_ANIMATIONSYSTEM USE_CRYPTO  )
  set_target_properties( client PROPERTIES OUTPUT_NAME "daemon.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )
  install( TARGETS client
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

  # Mac OS X specific settings
  if( ${CMAKE_SYSTEM_NAME} STREQUAL "Darwin" )
    # Mac OS X does not provide a SDLMain.dylib so we must compile our own
    add_library( INTERNAL_SDLMain ${MOUNT_DIR}/engine/sys/SDLMain.m )
    target_link_libraries( client INTERNAL_SDLMain )
    # Mac OS X has some trouble with PIC code under X86/32.
    if( BUILD_ARCH STREQUAL "i386" )
      set_target_properties( client PROPERTIES COMPILE_FLAGS "-mdynamic-no-pic" LINK_FLAGS "-read_only_relocs suppress" )
    endif()
  endif()


  # This section finds and provides REQUIRED dependencies to target client
  find_package( OpenGL REQUIRED )
  find_package( SDL REQUIRED )
  find_package( Freetype REQUIRED )
  find_package( GLEW REQUIRED )
  find_package( GMP REQUIRED )
  find_package( ZLIB REQUIRED )
  find_package( CURL REQUIRED )

  if( USE_INTERNAL_CRYPTO )
    # BEFORE is required to prevent system headers from being used
    include_directories( BEFORE ${MOUNT_DIR}/libs/nettle )
  else()
    find_package( Nettle REQUIRED )
    include_directories( ${NETTLE_INCLUDE_DIRS} )
    target_link_libraries( client ${NETTLE_LIBRARIES} )
  endif()

    include_directories( ${OPENGL_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_DIR} ${GMP_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} )
    target_link_libraries( client ${OPENGL_LIBRARIES} ${SDL_LIBRARY} ${FREETYPE_LIBRARIES} ${GLEW_LIBRARY} ${GMP_LIBRARIES} ${ZLIB_LIBRARIES} ${CURL_LIBRARIES} )

  # This section finds and provides OPTIONAL dependencies to target client
  if( USE_OPENAL )
    find_package( OpenAL REQUIRED )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_OPENAL )
    if( USE_OPENAL_DLOPEN )
      set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_OPENAL_DLOPEN )
    endif()
  endif()

  if( USE_CURSES )
    set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
    find_package( Curses REQUIRED )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_CURSES )
    include_directories( ${CURSES_INCLUDE_DIR} )
    target_link_libraries( client ${CURSES_LIBRARIES} )
  endif()

  if( USE_VOIP )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_VOIP FLOATING_POINT )
    if( USE_INTERNAL_SPEEX )
      # BEFORE is required to prevent system headers being used
      include_directories( BEFORE ${MOUNT_DIR}/libs/libspeex/include )
    else()
      find_package( Speex REQUIRED )
      include_directories( ${SPEEX_INCLUDE_DIRS} )
      target_link_libraries( client ${SPEEX_LIBRARIES} )
    endif()
  endif()

  if( USE_CODEC_VORBIS )
    find_package( Vorbis REQUIRED )
    find_package( Ogg REQUIRED )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_CODEC_VORBIS )
    include_directories( ${VORBIS_INCLUDE_DIRS} ${OGG_INCLUDE_DIRS} )
    target_link_libraries( client ${VORBIS_LIBRARIES} ${OGG_LIBRARIES} )
  endif()

  if( USE_CIN_THEORA )
    find_package( Theora REQUIRED )
    if( USE_CODEC_VORBIS ) # Ogg (and by extension Vorbis) must be enabled to use Theora
      set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_CIN_THEORA )
      include_directories( ${THEORA_INCLUDE_DIRS} )
      target_link_libraries( client ${THEORA_LIBRARIES} )
    else()
      message( FATAL_ERROR "Vorbis must be enabled to use CIN_THEORA" )
    endif()
  endif()

  if( USE_MUMBLE )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_MUMBLE )
  endif()

  if( USE_PHYSICS )
    find_package( TinyXML REQUIRED )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_PHYSICS )
    add_dependencies( client newton )
    include_directories( ${TINYXML_INCLUDE_DIR} )
    target_link_libraries( client ${TINYXML_LIBRARY} ${NEWTON_LIBRARY} )
  endif()

  if( USE_MYSQL )
    find_package( MySQL REQUIRED )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS ET_MYSQL )
    include_directories( ${MYSQL_INCLUDE_DIR} )
    target_link_libraries( client ${MYSQL_LIBRARIES} )
  endif()

  if( USE_CIN_XVID )
    find_package( xvid REQUIRED )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_CIN_XVID )
    include_directories( ${XVID_INCLUDE_DIR} )
    target_link_libraries( client ${XVID_LIBRARY} )
  endif()

  if( USE_LLVM )
    include_directories( ${LLVM_INCLUDE_DIR} )
    target_link_libraries( client ${LLVM_LIBS} )
  endif()
endif()

################
# Build OpenAL #
################

if( USE_OPENAL )
  find_package( OpenAL REQUIRED )
  add_library( al SHARED ${ALLIST} )
  include_directories( ${OPENAL_INCLUDE_DIRS} )
  target_link_libraries( al ${OPENAL_LIBRARY} )
  set_property( TARGET al PROPERTY COMPILE_DEFINITIONS )
  set_target_properties( al PROPERTIES OUTPUT_NAME "snd_openal" PREFIX "" )
  if( USE_OPENAL_DLOPEN )
    set_property( TARGET al APPEND PROPERTY COMPILE_DEFINITIONS USE_OPENAL_DLOPEN )
    target_link_libraries( al ${CMAKE_DL_LIBS} )
  endif()
  if( USE_VOIP )
    set_property( TARGET al APPEND PROPERTY COMPILE_DEFINITIONS USE_VOIP FLOATING_POINT )
  endif()
endif()

################
# Build Server #
################

if( BUILD_SERVER )
  # Setup the target Server
  add_executable( server ${SERVERLIST} ${SDLBASELIST} ${CPUINFOLIST}
    ${QCOMMONLIST} ${SHAREDLIST} ${DATABASELIST} ${CRYPTOLIST}
    ${MOUNT_DIR}/engine/null/null_client.c
    ${MOUNT_DIR}/engine/null/null_input.c ${MOUNT_DIR}/engine/null/null_snddma.c
    ${MOUNT_DIR}/engine/qcommon/dl_main_stubs.c )
  include_directories( ${MOUNT_DIR}/libs/cpuinfo ${NEWTON_INCLUDES} )
  target_link_libraries( server ${OS_LIBRARIES} ${NEWTON_LIBRARY} )
  set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS DEDICATED COMPAT_ET USE_CRYPTO )
  set_target_properties( server PROPERTIES OUTPUT_NAME "daemonded.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )
  install( TARGETS server
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )

  # Mac OS X has some trouble with PIC code under X86/32.
  if( CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND BUILD_ARCH STREQUAL "i386" )
    set_target_properties( server PROPERTIES COMPILE_FLAGS -fno-pic LINK_FLAGS "-read_only_relocs suppress" )
  endif()

  # The following section finds the REQUIRED dependencies and makes them available to the target
    find_package( SDL REQUIRED )
    find_package( GMP REQUIRED )
    find_package( ZLIB REQUIRED )
    find_package( CURL REQUIRED )

    if( USE_INTERNAL_CRYPTO )
      # BEFORE is required to prevent system headers from being used
      include_directories( BEFORE ${MOUNT_DIR}/libs/nettle )
    else()
      find_package( Nettle REQUIRED )
      include_directories( ${NETTLE_INCLUDE_DIRS} )
      target_link_libraries( server ${NETTLE_LIBRARIES} )
    endif()

    include_directories( ${SDL_INCLUDE_DIR} ${GMP_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS} )
    target_link_libraries( server ${SDL_LIBRARY} ${GMP_LIBRARIES} ${ZLIB_LIBRARIES} ${CURL_LIBRARIES} )
  # The following section finds the OPTIONAL dependencies and makes them available to the target
  if( USE_MYSQL )
    find_package( MySQL REQUIRED )
    set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS ET_MYSQL )
    target_link_libraries( server ${MYSQL_LIBRARIES} )
  endif()

  if( USE_CURSES )
    set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
    find_package( Curses REQUIRED )
    set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS USE_CURSES )
    include_directories( ${CURSES_INCLUDE_DIR} )
    target_link_libraries( server ${CURSES_LIBRARIES} )
  endif()

  if( USE_VOIP )
    set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS USE_VOIP FLOATING_POINT )
  endif()

  if( USE_PHYSICS )
    find_package( TinyXML REQUIRED )
    set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS USE_PHYSICS )
    include_directories( ${TINYXML_INCLUDE_DIRS} )
    target_link_libraries( server ${TINYXML_LIBRARIES} )
  endif()

  if( USE_LLVM )
    include_directories( ${LLVM_INCLUDE_DIR} )
    target_link_libraries( server ${LLVM_LIBS} )
  endif()
endif()

###################
# Build DaemonMap #
###################
if( BUILD_DAEMONMAP )
  # Setup DaemonMap target
  add_executable( daemonMap ${DAEMONMAPLIST} )
  include_directories( ${LIB_DIR}/picomodel ${MOUNT_DIR}/tools/common )
  target_link_libraries( daemonMap ${OS_LIBRARIES} )
  set_property( TARGET daemonMap APPEND PROPERTY COMPILE_DEFINITIONS USE_OPENGL )
  set_target_properties( daemonMap PROPERTIES OUTPUT_NAME "daemonmap.${BUILD_ARCH}" PREFIX "" )
  install( TARGETS daemonMap
    RUNTIME DESTINATION bin
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
  )
  # This section finds and provides REQUIRED dependencies to DaemonMap target
  find_package( OpenGL REQUIRED )
  find_package( SDL REQUIRED )
  find_package( JPEG REQUIRED )
  find_package( PNG REQUIRED )
  find_package( ZLIB REQUIRED )
  find_package( GLib2 REQUIRED )
  target_link_libraries( daemonMap ${OPENGL_LIBRARIES} ${SDL_LIBRARY} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${ZLIB_LIBRARIES} ${GLIB_LIBRARIES} )
  include_directories( ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} ${GLIB_INCLUDE_DIRS} )
endif()
