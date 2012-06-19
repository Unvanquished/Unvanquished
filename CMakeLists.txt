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
option( BUILD_SERVER         "Build Daemonded server"                      1 )
option( GAME_LIB             "Build game logic libraries"                  1 )
option( GAME_QVM             "Build game logic QVM files"                  1 )
option( BUILD_MASTER         "Build master server"                         1 )
option( BUILD_DAEMONMAP      "Build Mapping tool"                          0 )

option( ENABLE_W_ALL         "Use -Wall"                                   0 )
option( ENABLE_WARNINGS      "Enable various compiler warning options"     1 )
option( ENABLE_HARDENING     "Enable security hardening options"            1 )

cmake_dependent_option( BUILD_OLD_RENDERER   "Build old renderer library"                            1 BUILD_CLIENT                   0 )
cmake_dependent_option( BUILD_NEW_RENDERER   "Build new renderer library"                            1 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_OPENAL           "Use OpenAL"                                            1 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_CODEC_VORBIS     "Compile with support for Vorbis"                       1 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_MUMBLE           "Compile with support for Mumble"                       1 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_WEBP             "Compile with WebP loader"                              1 BUILD_CLIENT                   0 ) # technically, any renderer, not client
cmake_dependent_option( USE_VOIP             "Compile with VOIP support"                             1 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_CIN_XVID         "Encode videos with XViD"                               1 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_INTERNAL_SDL     "Use SDL files included with Unvanquished"              0 BUILD_CLIENT                   0 )

cmake_dependent_option( USE_CPUINFO          "Use CPUInfo (if compilation fails, then disable this)" 1 "BUILD_CLIENT OR BUILD_SERVER" 0 )
cmake_dependent_option( USE_CURSES           "Use the curses console"                                1 "BUILD_CLIENT OR BUILD_SERVER" 0 )
cmake_dependent_option( USE_PHYSICS          "Compile with Newton Physics"                           0 "BUILD_CLIENT OR BUILD_SERVER" 0 )
cmake_dependent_option( USE_INTERNAL_CRYPTO  "Use internal crypto (nettle)"                          0 "BUILD_CLIENT OR BUILD_SERVER" 0 )
cmake_dependent_option( NO_UNTRUSTED_PLUGINS "Don't load native plugins from the user dir"           1 "BUILD_CLIENT OR BUILD_SERVER" 0 )

cmake_dependent_option( USE_INTERNAL_JPEG    "Use JPEG files included with Unvanquished"             0
                                                                        "BUILD_OLD_RENDERER OR BUILD_NEW_RENDERER OR BUILD_DAEMONMAP" 0 )

cmake_dependent_option( USE_GLSL_OPTIMIZER   "Compile with GLSL Optimizer"                           1 BUILD_NEW_RENDERER             0 )

cmake_dependent_option( HAVE_BZIP2           "Compile with BZip2 support"                            0 "HAVE_BZIP2"                   0 )

cmake_dependent_option( BUILD_GAME_GPP       "Build Tremulous GPP game logic"              1 "GAME_LIB OR GAME_QVM" 0 )
cmake_dependent_option( QVM_DEBUG            "Enable (some) QVM debugging"                 1 GAME_QVM         0 )
cmake_dependent_option( GAME_LIB_LLVM        "Use LLVM, not shared libs, for game logic"   0 GAME_LIB         0 )

cmake_dependent_option( USE_OPENAL_DLOPEN    "Use dlopen to load OpenAL"                   1 USE_OPENAL       0 )
cmake_dependent_option( USE_CIN_THEORA       "Compile with support for Theora"             1 USE_CODEC_VORBIS 0 )
cmake_dependent_option( USE_INTERNAL_SPEEX   "Use internal speex"                          0 USE_VOIP         0 )

cmake_dependent_option( ENABLE_FSTACKPROT    "Enable stack protection if available"        1 ENABLE_HARDENING 0 )
cmake_dependent_option( ENABLE_RELRO         "Enable relro & bindnow if available"         1 ENABLE_HARDENING 0 )

mark_as_advanced( GAME_LIB_LLVM LLVM_CONFIG ENABLE_FSTACKPROT ENABLE_RELRO )

if( GAME_LIB_LLVM )
  find_program(LLVM_CONFIG llvm-config)
  execute_process(COMMAND ${LLVM_CONFIG} --libfiles
                  OUTPUT_VARIABLE LLVM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${LLVM_CONFIG} --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${LLVM_CONFIG} --version OUTPUT_VARIABLE LLVM_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE)
  execute_process(COMMAND ${LLVM_CONFIG} --libdir OUTPUT_VARIABLE LLVM_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE)
  separate_arguments(LLVM_LIBS)

  include( ${CMAKE_SOURCE_DIR}/cmake/CMakeLLVMInformation.cmake )
  include( ${CMAKE_SOURCE_DIR}/cmake/CMakeLLVMXXInformation.cmake )
  file( WRITE ${Daemon_BINARY_DIR}/CMakeFiles/CMakeLLVMCompiler.cmake )
  file( WRITE ${Daemon_BINARY_DIR}/CMakeFiles/CMakeLLVMXXCompiler.cmake )
  enable_language( LLVM )
  enable_language( LLVMXX )
  message( STATUS "Using LLVM" )
endif()

set( MOUNT_DIR ${CMAKE_SOURCE_DIR}/src )
set( GPP_DIR ${CMAKE_SOURCE_DIR}/src/gamelogic/gpp )
set( LIB_DIR ${CMAKE_SOURCE_DIR}/src/libs ) # LIB_DIR is also defined in Visual_Studio32/64.cmake
set( SDL_DIR ${LIB_DIR}/libsdl )
set( JPEG_DIR ${LIB_DIR}/jpeg )
set( GLEW_DIR ${LIB_DIR}/glew )

add_definitions( -DUSING_CMAKE ) # Let compiler know we are using CMake so files generated by CMake are included.
if( WIN32 )
  add_definitions( -D_CRT_SECURE_NO_WARNINGS )
  set( CMAKE_SHARED_LIBRARY_PREFIX "" )
endif()

if( MSVC )
  add_definitions( -D_WINDOWS -DWIN32 )
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

# Report the git commit identification, if possible

if( EXISTS "${CMAKE_SOURCE_DIR}/.git" )
  find_package( Git )
  if( GIT_FOUND )
    execute_process( COMMAND git describe --tags --long --dirty=+dirty OUTPUT_VARIABLE GIT_DESCRIBE_REPORT OUTPUT_STRIP_TRAILING_WHITESPACE )
    message( STATUS "git reported ${GIT_DESCRIBE_REPORT}" )
    # this may fail with annotated non-release tags
    if( GIT_DESCRIBE_REPORT MATCHES "-0-g.......$" )
      set( GIT_DESCRIBE_REPORT )
    endif()
  endif()
endif()

if( NOT GIT_DESCRIBE STREQUAL GIT_DESCRIBE_REPORT )
  if( GIT_DESCRIBE_REPORT )
    file( WRITE "${CMAKE_BINARY_DIR}/CMakeFiles/git_version.h" "#define GIT_VERSION \"${GIT_DESCRIBE_REPORT}\"" )
  else()
    file( WRITE "${CMAKE_BINARY_DIR}/CMakeFiles/git_version.h" )
  endif()
  set( GIT_DESCRIBE ${GIT_DESCRIBE_REPORT} CACHE INTERNAL "commit id reported by git" FORCE )
endif()

include_directories( "${CMAKE_BINARY_DIR}/CMakeFiles" )

# Import CMakeLists.txt for internal libraries
if( USE_INTERNAL_SDL )
  add_subdirectory( ${SDL_DIR} )
  set( SDL_LIBRARY INTERNAL_SDL INTERNAL_SDLMain CACHE STRING "SDL library" FORCE )
  set( SDL_INCLUDE_DIR ${SDL_DIR}/include CACHE STRING "SDL include directory" FORCE )
endif()

if( USE_INTERNAL_JPEG )
  add_subdirectory( ${JPEG_DIR} )
  set( JPEG_LIBRARY INTERNAL_JPEG CACHE STRING "JPEG library" FORCE )
  set( JPEG_INCLUDE_DIR ${JPEG_DIR} CACHE STRING "JPEG include directory" FORCE )
endif()

if( USE_INTERNAL_GLEW )
  add_subdirectory( ${GLEW_DIR} )
  set( GLEW_LIBRARY INTERNAL_GLEW CACHE STRING "Glew library" FORCE )
  set( GLEW_INCLUDE_DIR ${GLEW_DIR}/include CACHE STRING "Glew include directory" FORCE )
endif()

# Setup CPack
set( CPACK_PACKAGE_FILE_NAME "Unvanquished" )
set( CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/cpack/OS_X/Unvanquished.icns" )
set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "Daemon Unvanquished client" )
set( CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_CURRENT_SOURCE_DIR}/README.txt" )
set( CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/GPL.txt" )
# Mac OS X Bundle specific settings
set( CPACK_BUNDLE_NAME ${CPACK_PACKAGE_FILE_NAME} )
set( CPACK_BUNDLE_ICON ${CPACK_PACKAGE_ICON} )
if( ${BUILD_ARCH} STREQUAL "i386" )
  set( CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cpack/OS_X/x86/info.plist")
else()
  set( CPACK_BUNDLE_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cpack/OS_X/x86_64/info.plist")
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

macro( try_llvm_flag PROP FLAG )
  try_compiler_flag_internal( ${PROP} ${FLAG} LLVM cc C/LLVM)
endmacro()

macro( try_c_llvm_flag PROP FLAG )
  try_c_flag( ${PROP} ${FLAG} )
  if( GAME_LIB_LLVM )
    try_llvm_flag( ${PROP} ${FLAG} )
  endif()
endmacro()

macro( try_c_cxx_flag PROP FLAG )
  try_c_flag( ${PROP} ${FLAG} )
  try_cxx_flag( ${PROP} ${FLAG} )
endmacro()

macro( try_c_cxx_llvm_flag PROP FLAG )
  try_c_flag( ${PROP} ${FLAG} )
  try_cxx_flag( ${PROP} ${FLAG} )
  if( GAME_LIB_LLVM )
    try_llvm_flag( ${PROP} ${FLAG} )
  endif()
endmacro()

# Various warnings
if( ENABLE_W_ALL )
  try_c_cxx_llvm_flag( HAVE_W_ALL              -Wall )
endif()
if( ENABLE_WARNINGS )
  try_c_cxx_llvm_flag( HAVE_W_FORMAT2          -Wformat=2 )
  if( NOT HAVE_W_FORMAT2_C )
    try_c_cxx_llvm_flag( HAVE_W_FORMAT         -Wformat )
  endif()
  try_c_llvm_flag    ( HAVE_W_0LEN_FMT         -Wno-format-zero-length )
  try_c_cxx_llvm_flag( HAVE_W_FORMAT_SECURITY  -Wformat-security )
  try_c_cxx_llvm_flag( HAVE_W_STRICT_ALIAS_2   -Wstrict-aliasing=2 )
  try_c_cxx_llvm_flag( HAVE_W_MISSING_FMT_ATTR -Wmissing-format-attribute )
  try_c_cxx_llvm_flag( HAVE_W_MISSING_NORETURN -Wmissing-noreturn )
  try_c_llvm_flag    ( HAVE_W_IMPLICIT_FN      -Wimplicit-function-declaration )
  if( HAVE_W_IMPLICIT_FN_C )
    try_c_cxx_llvm_flag( HAVE_W_ERR_IMPLICIT_FN  -Werror=implicit-function-declaration )
  endif()
  try_c_cxx_llvm_flag( HAVE_W_FMT_SECURITY     -Wformat-security )
  if( HAVE_W_FMT_SECURITY_C )
    try_c_cxx_llvm_flag( HAVE_W_ERR_FMT_SECURITY -Werror=format-security )
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

if( ENABLE_FSTACKPROT )
  try_linker_flag( HAVE_FSTACKPROT "-fstack-protector --param=ssp-buffer-size=4" )
endif()
if( ENABLE_RELRO )
  try_linker_flag( HAVE_Z_RELRO_Z_NOW -Wl,-z,relro,-z,now )
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

# Visual studio can't handle inline assembly
if( MSVC )
  if( ${BUILD_ARCH} STREQUAL "i386" )
    set( ASM_APP ml )
  else()
    set( ASM_APP ml64 )
  endif()
  set( QCOMMONLIST ${QCOMMONLIST} ftola.obj snapvector.obj )
  # Work around - bugs in cmake prevent adding asm files to target in VS projects
  add_custom_command( OUTPUT ftola.obj COMMAND ${ASM_APP} /c ARGS "${MOUNT_DIR}/engine/asm/ftola.asm" MAIN_DEPENDENCY ${MOUNT_DIR}/engine/asm/ftola.asm )
  add_custom_command( OUTPUT snapvector.obj COMMAND ${ASM_APP} /c ARGS "${MOUNT_DIR}/engine/asm/snapvector.asm" MAIN_DEPENDENCY ${MOUNT_DIR}/engine/asm/snapvector.asm )
  if( BUILD_ARCH STREQUAL "x86_64" )
    set( QCOMMONLIST ${QCOMMONLIST} vm_x86_64.obj )
    add_custom_command( OUTPUT vm_x86_64.obj COMMAND ${ASM_APP} /c ARGS "${MOUNT_DIR}/engine/asm/vm_x86_64.asm" MAIN_DEPENDENCY ${MOUNT_DIR}/engine/asm/vm_x86_64.asm )
  endif()
else()
    set( QCOMMONLIST ${QCOMMONLIST}
    ${MOUNT_DIR}/engine/asm/ftola.c
    ${MOUNT_DIR}/engine/asm/snapvector.c
  )
endif()

if( GAME_LIB_LLVM )
  set( QCOMMONLIST ${QCOMMONLIST} ${MOUNT_DIR}/engine/qcommon/vm_llvm.cpp )
endif()

if( USE_PHYSICS )
  set( QCOMMONLIST ${QCOMMONLIST} ${MOUNT_DIR}/engine/qcommon/cm_physics.cpp )
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

# needed for LLVM & QVM
set( GAME_VM_COMMON
  ${MOUNT_DIR}/engine/qcommon/q_static.c
)

# Any .def files must come at the end of the file list otherwise they will be ignored
set( CGAMEGPP_BASE
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
  ${GPP_DIR}/src/ui/ui_utf8.c
  ${MOUNT_DIR}/engine/qcommon/./q_math.c
  ${MOUNT_DIR}/engine/qcommon/./q_shared.c
)

set( CGAMEGPP_SO
  ${CGAMEGPP_BASE}
  ${GPP_DIR}/src/cgame/cg_api.c
  ${GPP_DIR}/src/cgame/cgame.def
)

set( CGAMEGPP_QVM
  ${GPP_DIR}/src/cgame/cg_api.asm
)

set( GAMEGPP_BASE
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
  ${MOUNT_DIR}/engine/qcommon/./q_math.c
  ${MOUNT_DIR}/engine/qcommon/./q_shared.c
)

set( GAMEGPP_SO
  ${GAMEGPP_BASE}
  ${GPP_DIR}/src/game/g_api.c
  ${GPP_DIR}/src/game/game.def
)

set( GAMEGPP_QVM
  ${GPP_DIR}/src/game/g_api.asm
)

set( UIGPP_BASE
  ${GPP_DIR}/src/ui/ui_main.c
  ${GPP_DIR}/src/ui/ui_atoms.c
  ${GPP_DIR}/src/ui/ui_shared.c
  ${GPP_DIR}/src/ui/ui_gameinfo.c
  ${GPP_DIR}/src/game/bg_misc.c
  ${GPP_DIR}/src/game/bg_lib.c
  ${MOUNT_DIR}/engine/qcommon/./q_math.c
  ${MOUNT_DIR}/engine/qcommon/./q_shared.c
  ${GPP_DIR}/src/ui/ui_utf8.c
)

set( UIGPP_SO
  ${UIGPP_BASE}
  ${GPP_DIR}/src/ui/ui_api.c
  ${GPP_DIR}/src/ui/ui.def
)

set( UIGPP_QVM
  ${GPP_DIR}/src/ui/ui_api.asm
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

set( GLCPPLIST
  ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/glcpp-lex.c
  ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/glcpp-parse.c
  ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/pp.c
)

set( MESALIST
  ${LIB_DIR}/glsl-optimizer/src/mesa/program/hash_table.c
  ${LIB_DIR}/glsl-optimizer/src/mesa/program/symbol_table.c
)

set( GLSLLIST
  ${LIB_DIR}/glsl-optimizer/src/glsl/loop_analysis.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_jumps.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ast_function.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ast_type.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_hv_accept.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_types.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_reader.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/hir_field_selection.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_dead_functions.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_variable.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_redundant_jumps.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_tree_grafting.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_instructions.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_dead_code_local.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_function_detect_recursion.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_if_to_cond_assign.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_symbol_table.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_rvalue_visitor.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/loop_unroll.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_if_simplification.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ast_expr.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_validate.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_vector.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_variable_refcount.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_set_program_inouts.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_noise.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_swizzle_swizzle.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_parser.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_structure_splitting.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_dead_code.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_import_prototypes.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_basic_block.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/s_expression.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_texture_projection.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_constant_variable.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ast_to_hir.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/linker.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_copy_propagation.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_noop_swizzle.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/standalone_scaffolding.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_print_visitor.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_function_can_inline.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_discard.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/loop_controls.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_constant_propagation.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_function_inlining.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/link_functions.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_vec_index_to_swizzle.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/builtin_function.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_function.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_mat_op_to_vec.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_print_glsl_visitor.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_optimizer.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_vec_index_to_cond_assign.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_copy_propagation_elements.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_unused_structs.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_constant_folding.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_lexer.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_clone.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/lower_variable_index_to_cond_assign.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_hierarchical_visitor.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_algebraic.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_expression_flattening.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/ir_constant_expression.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glsl_parser_extras.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/opt_discard_simplification.cpp
  ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/glcpp-parse.c
  ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/glcpp-lex.c
  ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/pp.c
  ${LIB_DIR}/glsl-optimizer/src/glsl/glcpp/glcpp.c
  ${LIB_DIR}/glsl-optimizer/src/glsl/ralloc.c
  ${LIB_DIR}/glsl-optimizer/src/glsl/strtod.c
)

if( WIN32 )
  set( WIN_RC ${MOUNT_DIR}/engine/sys/daemon.rc )
endif()

set( Q3LCCETCDIR ${MOUNT_DIR}/tools/lcc/etc )

set( Q3LBURG_LIST
  ${MOUNT_DIR}/tools/lcc/lburg/gram.c
  ${MOUNT_DIR}/tools/lcc/lburg/lburg.c
)

set( Q3RCC_LIST
  ${MOUNT_DIR}/tools/lcc/src/alloc.c
  ${MOUNT_DIR}/tools/lcc/src/bind.c
  ${MOUNT_DIR}/tools/lcc/src/bytecode.c
  ${MOUNT_DIR}/tools/lcc/src/dag.c
  ${MOUNT_DIR}/tools/lcc/src/decl.c
  ${MOUNT_DIR}/tools/lcc/src/enode.c
  ${MOUNT_DIR}/tools/lcc/src/error.c
  ${MOUNT_DIR}/tools/lcc/src/event.c
  ${MOUNT_DIR}/tools/lcc/src/expr.c
  ${MOUNT_DIR}/tools/lcc/src/gen.c
  ${MOUNT_DIR}/tools/lcc/src/init.c
  ${MOUNT_DIR}/tools/lcc/src/inits.c
  ${MOUNT_DIR}/tools/lcc/src/input.c
  ${MOUNT_DIR}/tools/lcc/src/lex.c
  ${MOUNT_DIR}/tools/lcc/src/list.c
  ${MOUNT_DIR}/tools/lcc/src/main.c
  ${MOUNT_DIR}/tools/lcc/src/null.c
  ${MOUNT_DIR}/tools/lcc/src/output.c
  ${MOUNT_DIR}/tools/lcc/src/prof.c
  ${MOUNT_DIR}/tools/lcc/src/profio.c
  ${MOUNT_DIR}/tools/lcc/src/simp.c
  ${MOUNT_DIR}/tools/lcc/src/stmt.c
  ${MOUNT_DIR}/tools/lcc/src/string.c
  ${MOUNT_DIR}/tools/lcc/src/sym.c
  ${MOUNT_DIR}/tools/lcc/src/symbolic.c
  ${MOUNT_DIR}/tools/lcc/src/trace.c
  ${MOUNT_DIR}/tools/lcc/src/tree.c
  ${MOUNT_DIR}/tools/lcc/src/types.c
  ${CMAKE_BINARY_DIR}/CMakeFiles/dagcheck.c
)

set( Q3CPP_LIST
  ${MOUNT_DIR}/tools/lcc/cpp/cpp.c
  ${MOUNT_DIR}/tools/lcc/cpp/lex.c
  ${MOUNT_DIR}/tools/lcc/cpp/nlist.c
  ${MOUNT_DIR}/tools/lcc/cpp/tokens.c
  ${MOUNT_DIR}/tools/lcc/cpp/macro.c
  ${MOUNT_DIR}/tools/lcc/cpp/eval.c
  ${MOUNT_DIR}/tools/lcc/cpp/include.c
  ${MOUNT_DIR}/tools/lcc/cpp/hideset.c
  ${MOUNT_DIR}/tools/lcc/cpp/getopt.c
  ${MOUNT_DIR}/tools/lcc/cpp/unix.c
)

set( Q3LCC_LIST
  ${MOUNT_DIR}/tools/lcc/etc/lcc.c
  ${MOUNT_DIR}/tools/lcc/etc/bytecode.c
)

set( Q3ASM_LIST
  ${MOUNT_DIR}/tools/lcc/asm/q3asm.c
  ${MOUNT_DIR}/tools/lcc/asm/cmdlib.c
)

####################
# End of file list #
####################

if( GAME_LIB_LLVM )
  add_definitions( -DUSE_LLVM -D__STDC_LIMIT_MACROS -D__STDC_CONSTANT_MACROS )
endif()

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
  if( MSVC )
    set( OS_LIBRARIES winmm ws2_32 psapi )
    if( ${BUILD_ARCH} STREQUAL "x86_64" )
      add_definitions( -D__WIN64__ )
    endif()
  else()
    set( OS_LIBRARIES m winmm ws2_32 psapi z )
  endif()
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


# Function to setup all the Game/Cgame/UI libraries
include( CMakeParseArguments )
function( GAMELIB )
  # ParseArguments setup
  set( oneValueArgs LIB_NAME OUTPUT_NAME OUTPUT_DIR )
  set( multiValueArgs COMPILE_DEF LIB_FILES QVM_FILES QVM_ASM )
  cmake_parse_arguments(GAMELIB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if( GAME_LIB )
    # Xcode places the game libraries in the incorrect location. This should fix it
    if( XCODE_VERSION OR MSVC )
      set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELEASE "Release/${GAMELIB_OUTPUT_DIR}" )
      set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_DEBUG "Debug/${GAMELIB_OUTPUT_DIR}" )
      set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL "MinSizeRel/${GAMELIB_OUTPUT_DIR}" )
      set( CMAKE_LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO "RelWithDebInfo/${GAMELIB_OUTPUT_DIR}" )
      set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "Release/${GAMELIB_OUTPUT_DIR}" )
      set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "Debug/${GAMELIB_OUTPUT_DIR}" )
      set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "MinSizeRel/${GAMELIB_OUTPUT_DIR}" )
      set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "RelWithDebInfo/${GAMELIB_OUTPUT_DIR}" )
    endif()

    if( GAME_LIB_LLVM )
      add_library( ${GAMELIB_LIB_NAME} SHARED ${GAMELIB_LIB_FILES} ${GAME_VM_COMMON} )
    else()
      add_library( ${GAMELIB_LIB_NAME} SHARED ${GAMELIB_LIB_FILES} )
    endif()
    if( NOT MSVC )
      target_link_libraries( ${GAMELIB_LIB_NAME} m )
    endif()
    set_target_properties( ${GAMELIB_LIB_NAME} PROPERTIES
      PREFIX ""
      LIBRARY_OUTPUT_DIRECTORY ${GAMELIB_OUTPUT_DIR}
      RUNTIME_OUTPUT_DIRECTORY ${GAMELIB_OUTPUT_DIR}
      ARCHIVE_OUTPUT_DIRECTORY ${GAMELIB_OUTPUT_DIR}
      COMPILE_DEFINITIONS "${GAMELIB_COMPILE_DEF}"
    )

    if( GAME_LIB_LLVM )
      file( MAKE_DIRECTORY "${Daemon_BINARY_DIR}/${GAMELIB_OUTPUT_DIR}/vm" )

      set( GAMELIB_SOURCE_NAME "src/gamelogic/${GAMELIB_OUTPUT_DIR}" )
      if( GAMELIB_OUTPUT_DIR STREQUAL "main" )
        set( GAMELIB_SOURCE_NAME "src/gamelogic/gpp" )
      endif()

      set( GAMELIB_LLVM_FLAGS "-w -emit-llvm -DQ3_VM -nostdinc -I${GAMELIB_SOURCE_NAME} -Isrc/engine/qcommon -I${GAMELIB_SOURCE_NAME}/src/game -I${GAMELIB_SOURCE_NAME}/src/cgame -I${GAMELIB_SOURCE_NAME}/src/ui" )

      set( GAMELIB_LIB_CXX )
      foreach( FILE ${GAMELIB_LIB_FILES} )
        if( FILE MATCHES "\\.cpp$" )
          set( GAMELIB_LIB_CXX ${GAMELIB_LIB_CXX} ${FILE} )
        endif()
      endforeach()

      set_source_files_properties( ${GAMELIB_LIB_FILES} ${GAME_VM_COMMON} PROPERTIES LANGUAGE LLVM )
      set_source_files_properties( ${GAMELIB_LIB_CXX} PROPERTIES LANGUAGE LLVMXX )

      # First target is for at least i386
      set_target_properties( ${GAMELIB_LIB_NAME} PROPERTIES
        PREFIX "vm/"
        SUFFIX ".bc"
        COMPILE_FLAGS "-m32 ${GAMELIB_LLVM_FLAGS}"
        OUTPUT_NAME "${GAMELIB_OUTPUT_NAME}_32"
        LINKER_LANGUAGE LLVM
        LINK_FLAGS -emit-llvm
      )
      # And a second target for at least amd64
      add_library( ${GAMELIB_LIB_NAME}_64 SHARED ${GAMELIB_LIB_FILES} ${GAME_VM_COMMON} )
      target_link_libraries( ${GAMELIB_LIB_NAME}_64 m )
      set_target_properties( ${GAMELIB_LIB_NAME}_64 PROPERTIES
        PREFIX "vm/"
        SUFFIX ".bc"
        COMPILE_FLAGS "-m64 ${GAMELIB_LLVM_FLAGS}"
        OUTPUT_NAME "${GAMELIB_OUTPUT_NAME}_64"
        LINKER_LANGUAGE LLVM
        LINK_FLAGS -emit-llvm
        LIBRARY_OUTPUT_DIRECTORY ${GAMELIB_OUTPUT_DIR}
        RUNTIME_OUTPUT_DIRECTORY ${GAMELIB_OUTPUT_DIR}
        ARCHIVE_OUTPUT_DIRECTORY ${GAMELIB_OUTPUT_DIR}
        COMPILE_DEFINITIONS "${GAMELIB_COMPILE_DEF}"
      )
      # May need more here for different architectures

    elseif( WIN32 )
      set_target_properties( ${GAMELIB_LIB_NAME} PROPERTIES OUTPUT_NAME "${GAMELIB_OUTPUT_NAME}_mp_${BUILD_ARCH}" )
    else()
      set_target_properties( ${GAMELIB_LIB_NAME} PROPERTIES OUTPUT_NAME "${GAMELIB_OUTPUT_NAME}.mp.${BUILD_ARCH}" )
    endif()
  endif()

  if( GAME_QVM AND GAMELIB_QVM_FILES )
    set( QVM_DIR ${CMAKE_BINARY_DIR}/CMakeFiles/${GAMELIB_LIB_NAME}-qvm.dir )
    set( QVM_CPP_OPTIONS
      -I"${MOUNT_DIR}/gamelogic/gpp/src/game"
      -I"${MOUNT_DIR}/engine/qcommon"
      -D__inline=
      -DQ3_VM
    )
    if( QVM_DEBUG )
      set( QVM_CPP_OPTIONS ${QVM_CPP_OPTIONS} -D_DEBUG )
    endif()
    foreach( DEF ${GAMELIB_COMPILE_DEF} )
      set( QVM_CPP_OPTIONS ${QVM_CPP_OPTIONS} -D${DEF} )
    endforeach()

    set( QVM_OBJECTS )
    set( GAMELIB_QVM_FILES ${GAMELIB_QVM_FILES} ${MOUNT_DIR}/engine/qcommon/q_static.c )
    foreach( QVM_FILE ${GAMELIB_QVM_FILES} )
      get_filename_component( QVM_FILENAME ${QVM_FILE} NAME_WE )
      get_filename_component( QVM_FILEDIR  ${QVM_FILE} PATH )
      set( QVM_COMPONENT ${QVM_DIR}/${QVM_FILENAME} )
      set( QVM_OBJECTS ${QVM_OBJECTS} ${QVM_COMPONENT}.asm )

      add_custom_command( OUTPUT ${QVM_COMPONENT}.asm
        COMMAND lcc -target bytecode ${QVM_CPP_OPTIONS} ${QVM_FILE}
        DEPENDS lcc rcc cpp ${QVM_FILE}
        MAIN_DEPENDENCY ${QVM_COMPONENT}.asm
        IMPLICIT_DEPENDS C ${QVM_FILE}
        WORKING_DIRECTORY ${QVM_DIR}
      )
    endforeach()

    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/${GAMELIB_OUTPUT_DIR}/vm/${GAMELIB_OUTPUT_NAME}.qvm
      COMMAND asm -o ${CMAKE_BINARY_DIR}/${GAMELIB_OUTPUT_DIR}/vm/${GAMELIB_OUTPUT_NAME}.qvm ${QVM_OBJECTS} ${GAMELIB_QVM_ASM}
      DEPENDS asm ${QVM_OBJECTS} ${GAMELIB_QVM_ASM}
    )

    add_custom_target( ${GAMELIB_LIB_NAME}-qvm ALL
      DEPENDS ${CMAKE_BINARY_DIR}/${GAMELIB_OUTPUT_DIR}/vm/${GAMELIB_OUTPUT_NAME}.qvm
    )

  endif()
endfunction()

if( GAME_QVM )
  add_subdirectory( ${MOUNT_DIR}/tools/lcc ${CMAKE_BINARY_DIR}/lcc )
endif()

GAMELIB( LIB_NAME gppgame
  OUTPUT_NAME qagame
  OUTPUT_DIR main
  COMPILE_DEF GAME GAMEDLL GPP USE_REFENTITY_ANIMATIONSYSTEM
  LIB_FILES ${GAMEGPP_SO} ${GPP_DIR}
  QVM_FILES ${GAMEGPP_BASE}
  QVM_ASM ${GAMEGPP_QVM}
)

GAMELIB( LIB_NAME gpptrem
  OUTPUT_NAME cgame
  OUTPUT_DIR main
  COMPILE_DEF CGAME CGAMEDLL GPP USE_REFENTITY_ANIMATIONSYSTEM
  LIB_FILES ${CGAMEGPP_SO}
  QVM_FILES ${CGAMEGPP_BASE}
  QVM_ASM ${CGAMEGPP_QVM}
)

GAMELIB( LIB_NAME gppui
  OUTPUT_NAME ui
  OUTPUT_DIR main
  COMPILE_DEF UI UIDLL GPP USE_REFENTITY_ANIMATIONSYSTEM
  LIB_FILES ${UIGPP_SO}
  QVM_FILES ${UIGPP_BASE}
  QVM_ASM ${UIGPP_QVM}
)

# Don't worry about installing targets individually just copy entire folder over
# This allows maps/.pk3s to be easily added to the installer
install( DIRECTORY ${CMAKE_BINARY_DIR}/main DESTINATION "Unvanquished" )

if( USE_PHYSICS )
  if( LINUX )
    if( BUILD_ARCH STREQUAL "x86_64" )
      set( NEWTON_LIBRARY CACHE FILEPATH "${MOUNT_DIR}/libs/libnewton/libs/linux64/libNewton.a" )
    elseif( BUILD_ARCH STREQUAL "i386" )
      set( NEWTON_LIBRARY CACHE FILEPATH "${MOUNT_DIR}/libs/libnewton/libs/linux32/libNewton.a" )
    endif()
  elseif( MSVC )
    if( BUILD_ARCH STREQUAL "x86_64" )
      set( NEWTON_LIBRARY CACHE FILEPATH "${MOUNT_DIR}/libs/libnewton/libs/win64/newton.lib" )
    elseif( BUILD_ARCH STREQUAL "i386" )
      set( NEWTON_LIBRARY CACHE FILEPATH "${MOUNT_DIR}/libs/libnewton/libs/win32/newton.lib" )
    endif()
  else()
    set( NEWTON_LIBRARY CACHE FILEPATH "Location of the Newton library" )
  endif()
endif()

######################
# Build Old Renderer #
######################

if( BUILD_OLD_RENDERER )
  # Setup target renderer
  add_library( renderer SHARED ${RENDERERLIST} ${MOUNT_DIR}/engine/sys/sdl_glimp.c ${MOUNT_DIR}/engine/sys/sdl_gamma.c ${SHAREDLIST} ${MOUNT_DIR}/engine/rendererGL/rendererGL.def )
  set_property( TARGET renderer PROPERTY COMPILE_DEFINITIONS BUILD_FREETYPE USE_REFENTITY_ANIMATIONSYSTEM )
  if( NOT WIN32 AND NOT (${BUILD_ARCH} STREQUAL "i386") )
    set_target_properties( renderer PROPERTIES OUTPUT_NAME "rendererGL${BUILD_ARCH}" )
  else()
    set_target_properties( renderer PROPERTIES OUTPUT_NAME "rendererGLx86" )
  endif()
  if( NOT MSVC )
    target_link_libraries( renderer m )
  endif()

  # Setup CPack for target renderer
  install( TARGETS renderer DESTINATION "Unvanquished")
  get_target_property( LOCATION renderer LOCATION )
  get_filename_component( OUTPUT_NAME ${LOCATION} NAME )
  set( PLUGINS ${PLUGINS} "\${CMAKE_INSTALL_PREFIX}/Unvanquished/${OUTPUT_NAME}" )

  # This section finds and provides REQUIRED dependencies to target renderer
  find_package( OpenGL REQUIRED )
  find_package( SDL REQUIRED )
  find_package( JPEG REQUIRED )
  find_package( PNG REQUIRED )
  find_package( Freetype REQUIRED )
  find_package( GLEW REQUIRED )

  include_directories( ${PNG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} )
  target_link_libraries( renderer ${PNG_LIBRARIES} ${SDL_LIBRARY} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${FREETYPE_LIBRARIES} )

  # This section finds and provides OPTIONAL dependencies to target renderer
  if( USE_WEBP )
    find_package( WebP REQUIRED )
    set_property( TARGET renderer APPEND PROPERTY COMPILE_DEFINITIONS USE_WEBP )
    include_directories( ${WEBP_INCLUDE_DIRS} )
    target_link_libraries( renderer ${WEBP_LIBRARIES} )
  endif()
endif()

######################
# Build New Renderer #
######################

if( BUILD_NEW_RENDERER )
  # Setup target rendererGL
  add_library( rendererGL SHARED ${SHAREDLIST} ${OPENEXRLIST} ${RENDERERGLLIST} ${MOUNT_DIR}/engine/sys/sdl_glimp.c ${MOUNT_DIR}/engine/sys/sdl_gamma.c ${MOUNT_DIR}/engine/rendererGL/rendererGL.def )
  set_property( TARGET rendererGL PROPERTY COMPILE_DEFINITIONS BUILD_FREETYPE FT_FREETYPE USE_XREAL_RENDERER USE_REFENTITY_ANIMATIONSYSTEM )
  if( NOT WIN32 AND NOT (${BUILD_ARCH} STREQUAL "i386") )
    set_target_properties( rendererGL PROPERTIES OUTPUT_NAME "rendererGL3${BUILD_ARCH}" )
  else()
    set_target_properties( rendererGL PROPERTIES OUTPUT_NAME "rendererGL3x86" )
  endif()
  target_link_libraries( rendererGL ${OS_LIBRARIES} )

  # Setup CPack for target rendererGL
  install( TARGETS rendererGL DESTINATION "Unvanquished")
  get_target_property( LOCATION rendererGL LOCATION )
  get_filename_component( OUTPUT_NAME ${LOCATION} NAME )
  set( PLUGINS ${PLUGINS} "\${CMAKE_INSTALL_PREFIX}/Unvanquished/${OUTPUT_NAME}" )

  # This section finds and provides REQUIRED dependencies to target rendererGL
  find_package( OpenGL REQUIRED )
  find_package( SDL REQUIRED )
  find_package( JPEG REQUIRED )
  find_package( PNG REQUIRED )
  find_package( Freetype REQUIRED )
  find_package( GLEW REQUIRED )

  include_directories( ${PNG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} )
  target_link_libraries( rendererGL ${PNG_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${FREETYPE_LIBRARIES} ${SDL_LIBRARY} )

  # This section finds and provides OPTIONAL dependencies to target rendererGL
  if( USE_WEBP )
    find_package( WebP REQUIRED )
    set_property( TARGET rendererGL APPEND PROPERTY COMPILE_DEFINITIONS USE_WEBP )
    include_directories( ${WEBP_INCLUDE_DIRS} )
    target_link_libraries( rendererGL ${WEBP_LIBRARIES} )
  endif()

  if( USE_GLSL_OPTIMIZER )
    include_directories( ${LIB_DIR}/glsl-optimizer/include )
    target_link_libraries( rendererGL glsl_optimizer )
    set_property( TARGET rendererGL APPEND PROPERTY COMPILE_DEFINITIONS USE_GLSL_OPTIMIZER )
  endif()
endif()

########################
# Build GLSL Optimizer #
########################

if( USE_GLSL_OPTIMIZER )
  include_directories( ${LIB_DIR}/glsl-optimizer/include )
  include_directories( ${LIB_DIR}/glsl-optimizer/src/mesa )
  include_directories( ${LIB_DIR}/glsl-optimizer/src/mapi )
  include_directories( ${LIB_DIR}/glsl-optimizer/src/glsl )

  add_library( glcpp-library ${GLCPPLIST} )
  set_target_properties( glcpp-library PROPERTIES COMPILE_FLAGS "-fPIC" )

  add_library( mesa ${MESALIST} )
  set_target_properties( mesa PROPERTIES COMPILE_FLAGS "-fPIC" )

  add_library( glsl_optimizer ${GLSLLIST} )
  set_target_properties( glsl_optimizer PROPERTIES COMPILE_FLAGS "-fPIC" )
  target_link_libraries( glsl_optimizer glcpp-library mesa )
endif()

###############
# Find Curses #
###############

if( USE_CURSES )
  set( CURSES_NEED_NCURSES 1 ) # Tells FindCurses that ncurses is required
  find_package( CursesW )
  if( CURSESW_FOUND )
    set( CURSES_DEFINES USE_CURSES USE_CURSES_W )
    set( CURSES_INCLUDE_DIR ${CURSESW_INCLUDE_DIR} )
    set( CURSES_LIBRARIES   ${CURSESW_LIBRARIES} )
  else()
    find_package( Curses REQUIRED )
    set( CURSES_DEFINES USE_CURSES )
  endif()
endif()

################
# Build Client #
################

if( BUILD_CLIENT )
  # Setup target client
  add_executable( client ${SERVERLIST} ${CLIENTLIST} ${SDLBASELIST}
    ${CPUINFOLIST} ${SDLLIST} ${QCOMMONLIST} ${SHAREDLIST} ${CRYPTOLIST}
    ${DATABASELIST} ${MOUNT_DIR}/engine/qcommon/dl_main_curl.c ${SPEEXLIST} ${WIN_RC} )
  add_dependencies( client rendererGL )
  include_directories( ${MOUNT_DIR}/libs/cpuinfo ${NEWTON_INCLUDES} )
  target_link_libraries( client ${OS_LIBRARIES} ${NEWTON_LIBRARY} )
  set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_XREAL_RENDERER COMPAT_ET USE_REFENTITY_ANIMATIONSYSTEM USE_CRYPTO  )
  set_target_properties( client PROPERTIES OUTPUT_NAME "daemon.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )

  # Setup CPack for target client
  install( TARGETS client DESTINATION "Unvanquished")
  get_target_property( LOCATION client LOCATION )
  get_filename_component( OUTPUT_NAME ${LOCATION} NAME )
  set( APPS ${APPS} Unvanquished/${OUTPUT_NAME} )

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
  
  # Visual Studio specific settings
  if( MSVC )
    include_directories( ${MOUNT_DIR}/libs/msinttypes )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS ZLIB_WINAPI HAVE_CONFIG_H )
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
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS ${CURSES_DEFINES} )
    include_directories( ${CURSES_INCLUDE_DIR} )
    target_link_libraries( client ${CURSES_LIBRARIES} )
  endif()

  if( USE_VOIP )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_VOIP )
    if( NOT MSVC )
      set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS FLOATING_POINT )
    endif()
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
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_CODEC_VORBIS )
    include_directories( ${VORBIS_INCLUDE_DIRS} )
    target_link_libraries( client ${VORBIS_LIBRARIES} )
  endif()

  if( USE_CIN_THEORA )
    find_package( Theora REQUIRED )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_CIN_THEORA )
    include_directories( ${THEORA_INCLUDE_DIRS} )
    target_link_libraries( client ${THEORA_LIBRARIES} )
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

  if( USE_CIN_XVID )
    find_package( xvid REQUIRED )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_CIN_XVID )
    include_directories( ${XVID_INCLUDE_DIR} )
    target_link_libraries( client ${XVID_LIBRARY} )
  endif()

  if( HAVE_BZIP2 )
    find_package( BZip2 REQUIRED )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS HAVE_BZIP2 )
    include_directories( ${BZIP2_INCLUDE_DIR} )
    target_link_libraries( client ${BZIP2_LIBRARY} )
  endif()

  if( GAME_LIB_LLVM )
    include_directories( ${LLVM_INCLUDE_DIR} )
    target_link_libraries( client ${LLVM_LIBS} )
  endif()

  if( MINGW )
    target_link_libraries( client ${SDLMAIN_LIBRARY} )
  endif()
endif()

################
# Build OpenAL #
################

if( USE_OPENAL )
  find_package( OpenAL REQUIRED )
  add_library( al SHARED ${ALLIST} )
  include_directories( ${OPENAL_INCLUDE_DIR} )
  target_link_libraries( al ${OPENAL_LIBRARY} )
  set_property( TARGET al PROPERTY COMPILE_DEFINITIONS )
  set_target_properties( al PROPERTIES OUTPUT_NAME "snd_openal" PREFIX "" )

  # Setup CPack for target al
  install( TARGETS al DESTINATION "Unvanquished")
  get_target_property( LOCATION al LOCATION )
  get_filename_component( OUTPUT_NAME ${LOCATION} NAME )
  set( PLUGINS ${PLUGINS} "\${CMAKE_INSTALL_PREFIX}/Unvanquished/${OUTPUT_NAME}" )

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
    ${MOUNT_DIR}/engine/qcommon/dl_main_stubs.c ${WIN_RC} )
  include_directories( ${MOUNT_DIR}/libs/cpuinfo ${NEWTON_INCLUDES} )
  target_link_libraries( server ${OS_LIBRARIES} ${NEWTON_LIBRARY} )
  set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS DEDICATED COMPAT_ET USE_CRYPTO )
  set_target_properties( server PROPERTIES OUTPUT_NAME "daemonded.${BUILD_ARCH}" PREFIX "" LINKER_LANGUAGE CXX )

  # Setup CPack for target server
  install( TARGETS server DESTINATION "Unvanquished")
  get_target_property( LOCATION server LOCATION )
  get_filename_component( OUTPUT_NAME ${LOCATION} NAME )
  set( APPS ${APPS} Unvanquished/${OUTPUT_NAME} )

  # Mac OS X has some trouble with PIC code under X86/32.
  if( CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND BUILD_ARCH STREQUAL "i386" )
    set_target_properties( server PROPERTIES COMPILE_FLAGS -mdynamic-no-pic LINK_FLAGS "-read_only_relocs suppress" )
  endif()
  
  # Visual Studio specific settings
  if( MSVC )
    set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS ZLIB_WINAPI )
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

  if( USE_CURSES )
    set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS ${CURSES_DEFINES} )
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

  if( HAVE_BZIP2 )
    find_package( BZip2 REQUIRED )
    set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS HAVE_BZIP2 )
    include_directories( ${BZIP2_INCLUDE_DIR} )
    target_link_libraries( server ${BZIP2_LIBRARY} )
  endif()

  if( GAME_LIB_LLVM )
    include_directories( ${LLVM_INCLUDE_DIR} )
    target_link_libraries( server ${LLVM_LIBS} )
  endif()
endif()

###################
# Build DaemonMap #
###################
if( BUILD_DAEMONMAP )
  # Setup DaemonMap target
  add_executable( daemonMap ${DAEMONMAPLIST} ${WIN_RC} )
  include_directories( ${LIB_DIR}/picomodel ${MOUNT_DIR}/tools/common )
  target_link_libraries( daemonMap ${OS_LIBRARIES} )
  set_property( TARGET daemonMap APPEND PROPERTY COMPILE_DEFINITIONS USE_OPENGL )
  set_target_properties( daemonMap PROPERTIES OUTPUT_NAME "daemonmap.${BUILD_ARCH}" PREFIX "" )

  # Setup CPack for target daemonmap
  install( TARGETS daemonMap DESTINATION "Unvanquished")
  get_target_property( OUTPUT_NAME daemonMap OUTPUT_NAME )
  set( APPS ${APPS} Unvanquished/${OUTPUT_NAME} )

  # Visual Studio specific settings
  if( MSVC )
    set_property( TARGET daemonMap APPEND PROPERTY COMPILE_DEFINITIONS ZLIB_WINAPI )
  endif()
  
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

#####################
#Configure Installer#
#####################
# This section requires a lot of escaping as these values are parsed then passed
# to CPack. Be very very careful when modifying values

# SDL framework doesn't work with bundleutilities so detect if it's being used
# and copy it over manually
foreach( LIB ${SDL_LIBRARY} )
  if( ${LIB} MATCHES "SDL.framework")
    install( DIRECTORY ${LIB} DESTINATION Frameworks )
  endif()
endforeach()

# Automatically copy over any dependencies
foreach( APP ${APPS} )
  install(CODE "
     include(BundleUtilities)
     message( \"\${CMAKE_INSTALL_PREFIX}/${APP}\" )
     fixup_bundle(\"\${CMAKE_INSTALL_PREFIX}/${APP}\"   \"${PLUGINS}\"   \"${CMAKE_LIBRARY_PATH}\")
     " )
endforeach()

# include( CPack ) must be at the end of CMakeLists.txt
include( CPack )
