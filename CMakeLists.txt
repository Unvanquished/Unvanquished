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

include( CMakeDependentOption )

# This line has to appear before 'PROJECT' in order to be able to disable incremental linking
set( MSVC_INCREMENTAL_DEFAULT ON )

# Default to Release builds. To use system CFLAGS only (for distro builds), set CMAKE_BUILD_TYPE to None
set( CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: None(CMAKE_CXX_FLAGS or CMAKE_C_FLAGS used) Debug Release RelWithDebInfo MinSizeRel." )

project( Daemon C CXX )

################################################################################
# Configuration options
################################################################################

option( BUILD_CLIENT "Build Daemon client" 1 )
option( BUILD_SERVER "Build Daemonded server" 1 )
option( BUILD_GAME_NACL_NATIVE_EXE "Build native executable NaCl game logic" 1 )
option( BUILD_GAME_NACL_NATIVE_DLL "Build native shared library NaCl game logic" 1 )
option( BUILD_GAME_NACL "Build NaCl game logic" 1 )
option( BUILD_GAME_QVM_NATIVE "Build native game logic " 1 )
option( BUILD_GAME_QVM "Build QVM game logic" 1 )
option( BUILD_TTY_CLIENT "Build Daemon headless client" 1 )

cmake_dependent_option( USE_VOIP "Compile with VOIP support" 0 "BUILD_CLIENT OR BUILD_SERVER OR BUILD_TTY_CLIENT" 0 )
cmake_dependent_option( USE_SMP "Compile with support for running the renderer in a separate thread" 1 BUILD_CLIENT 0 )
cmake_dependent_option( USE_GEOIP "Use libgeoip" 1 "BUILD_SERVER OR BUILD_CLIENT OR BUILD_TTY_CLIENT" 0 )
option( USE_LTO "Use link-time optimization for release builds" 0 )
cmake_dependent_option( USE_SLIM_LTO "Generate slim LTO objects, improves build times" 1 "USE_LTO AND ${CMAKE_CXX_COMPILER_ID} STREQUAL GNU" 0 )
option( USE_HARDENING "Use stack protection and other hardening flags" 0 )
option( USE_SDL2 "Enable SDL 2.0 support (incompatible with SDL 1.2)" 1 )
option( USE_WEXTRA "Enable extra compilation warnings" 0 )
option( USE_PEDANTIC "Tell the compiler to be pedantic" 0 )
option( USE_DEBUG_OPTIMIZE "Try to optimize the debug build" 1 )
option( USE_PRECOMPILED_HEADER "Improve build times by using a precompiled header" 1 )
option( USE_ADDRESS_SANITIZER "Try to use the address sanitizer" 0 )

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
# Clang prints a warning when if it doesn't support a flag, so use -Werror to detect
macro( try_cxx_flag_werror PROP FLAG )
  set( CMAKE_REQUIRED_FLAGS "-Werror" )
  check_CXX_compiler_flag( ${FLAG} FLAG_${PROP} )
  set( CMAKE_REQUIRED_FLAGS "" )
  if( FLAG_${PROP} )
    set_cxx_flag( ${FLAG} ${ARGV2} )
  endif()
endmacro()
macro( try_c_cxx_flag_werror PROP FLAG )
  try_cxx_flag_werror( ${PROP} ${FLAG} ${ARGV2} )
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
  if( USE_LTO )
    set_c_cxx_flag( "/GL" MINSIZEREL )
    set_c_cxx_flag( "/GL" RELWITHDEBINFO )
    set_c_cxx_flag( "/GL" RELEASE )
    set_linker_flag( "/LTCG" MINSIZEREL )
    set_linker_flag( "/LTCG" RELWITHDEBINFO )
    set_linker_flag( "/LTCG" RELEASE )
  endif()
else()
  set_c_cxx_flag( "-ffast-math" )
  set_c_cxx_flag( "-fno-strict-aliasing" )

  # Set arch on x86 to SSE2 minimum and enable CMPXCHG16B
  if( ARCH STREQUAL "x86" )
    set_c_cxx_flag( "-m32" )
    set_c_cxx_flag( "-msse2" )
    set_c_cxx_flag( "-mtune=generic" )
    try_c_cxx_flag_werror( MFPMATH_SSE "-mfpmath=sse" )
  elseif( ARCH STREQUAL "x86_64" )
    set_c_cxx_flag( "-m64" )
    set_c_cxx_flag( "-mtune=generic" )
    try_c_cxx_flag_werror( MCX16 "-mcx16" )
  endif()

  # Use hidden symbol visibility if possible
  try_c_cxx_flag( FVISIBILITY_HIDDEN "-fvisibility=hidden" )

  # Extra debug flags
  set_c_cxx_flag( "-g3" DEBUG )
  set_c_cxx_flag( "-g3" RELWITHDEBINFO )
  if( USE_DEBUG_OPTIMIZE )
    try_c_cxx_flag( OPTIMIZE_DEBUG "-Og" DEBUG )
  endif()

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
    set_linker_flag( "-stdlib=libc++" )
  endif()

  # Hardening, don't set _FORTIFY_SOURCE in debug builds
  set_c_cxx_flag( "-D_FORTIFY_SOURCE=2" RELEASE )
  set_c_cxx_flag( "-D_FORTIFY_SOURCE=2" RELWITHDEBINFO )
  set_c_cxx_flag( "-D_FORTIFY_SOURCE=2" MINSIZEREL )
  if( USE_HARDENING )
    try_c_cxx_flag( FSTACK_PROTECTOR_STRONG "-fstack-protector-strong" )
    if( NOT FLAG_FSTACK_PROTECTOR_STRONG )
      try_c_cxx_flag( FSTACK_PROTECTOR_ALL "-fstack-protector-all" )
    endif()
    try_c_cxx_flag( FNO_STRICT_OVERFLOW "-fno-strict-overflow" )
    try_c_cxx_flag( WSTACK_PROTECTOR "-Wstack-protector" )
    try_c_cxx_flag( FPIE "-fPIE" )
    try_linker_flag( LINKER_PIE "-pie" )
  endif()

  # Linker flags
  if( NOT APPLE )
    try_linker_flag( LINKER_O1 "-Wl,-O1" )
    try_linker_flag( LINKER_SORT_COMMON "-Wl,--sort-common" )
    try_linker_flag( LINKER_AS_NEEDED "-Wl,--as-needed" )
    if( NOT USE_ADDRESS_SANITIZER )
      try_linker_flag( LINKER_NO_UNDEFINED "-Wl,--no-undefined" )
    endif()
    try_linker_flag( LINKER_Z_RELRO "-Wl,-z,relro" )
    try_linker_flag( LINKER_Z_NOW "-Wl,-z,now" )
  endif()
  if( WIN32 )
    try_linker_flag( LINKER_DYNAMICBASE "-Wl,--dynamicbase" )
    try_linker_flag( LINKER_NXCOMPAT "-Wl,--nxcompat" )
  endif()

  # The -pthread flag sets some preprocessor defines, it is also used to link
  # with libpthread on Linux
  try_c_cxx_flag( PTHREAD "-pthread" )
  if( LINUX )
    set_linker_flag( "-pthread" )
  endif()

  # Warning options
  set_c_cxx_flag( "-Wall" )
  if( USE_WEXTRA )
    set_c_cxx_flag( "-Wextra" )
  else()
    set_c_cxx_flag( "-Wno-sign-compare" )
    set_c_cxx_flag( "-Wno-write-strings" )
  endif()

  if( USE_PEDANTIC )
    set_c_cxx_flag( "-pedantic" )
  endif()

  if( USE_ADDRESS_SANITIZER )
    set_cxx_flag( "-fsanitize=address" )
    set_linker_flag( "-fsanitize=address" )
  endif()

  # Link-time optimization
  if( USE_LTO )
    set_c_cxx_flag( "-flto" )
    set_linker_flag( "-flto" )

    # For LTO compilation we must send a copy of all compile flags to the linker
    set_linker_flag( "${CMAKE_CXX_FLAGS}" )

    # Use gcc-ar and gcc-ranlib instead of ar and ranlib so that we can use
    # slim LTO objects. This requires a recent version of GCC and binutils.
    if( ${CMAKE_CXX_COMPILER_ID} STREQUAL GNU )
      if( USE_SLIM_LTO )
        string(REGEX MATCH "^([0-9]+.[0-9]+)" _version
           "${CMAKE_CXX_COMPILER_VERSION}")
        get_filename_component(COMPILER_BASENAME "${CMAKE_C_COMPILER}" NAME)
        if (COMPILER_BASENAME MATCHES "^(.+-)g?cc(-[0-9]+\\.[0-9]+\\.[0-9]+)?(\\.exe)?$")
          set(TOOLCHAIN_PREFIX ${CMAKE_MATCH_1})
        endif()

        find_program(GCC_AR NAMES
          "${TOOLCHAIN_PREFIX}gcc-ar"
          "${TOOLCHAIN_PREFIX}gcc-ar-${_version}"
          DOC "gcc provided wrapper for ar which adds the --plugin option"
        )

        find_program(GCC_RANLIB NAMES
          "${TOOLCHAIN_PREFIX}gcc-ranlib"
          "${TOOLCHAIN_PREFIX}gcc-ranlib-${_version}"
          DOC "gcc provided wrapper for ranlib which adds the --plugin option"
        )

        mark_as_advanced( GCC_AR GCC_RANLIB )

        # Override standard ar and ranlib with the gcc- versions
        if( GCC_AR )
          set( CMAKE_AR ${GCC_AR} )
        endif()
        if( GCC_RANLIB )
          set( CMAKE_RANLIB ${GCC_RANLIB} )
        endif()

        try_c_cxx_flag( NO_FAT_LTO_OBJECTS "-fno-fat-lto-objects" )
      else()
        try_c_cxx_flag( FAT_LTO_OBJECTS "-ffat-lto-objects" )
      endif()
    endif()
  endif()

endif()

# Windows-specific definitions
if( WIN32 )
  # Minimum Windows version: XP
  # Define WIN32 for compatibility (compiler defines _WIN32)
  # Define NOMINMAX to prevent conflics between std::min/max and the min/max macros in WinDef.h
  # Enable STRICT type checking for windows.h
  add_definitions( -DWINVER=0x501 -DWIN32 -DNOMINMAX -DSTRICT )
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

################################################################################
# Determine python version and set env variable
################################################################################

if( BUILD_GAME_NACL AND NOT CMAKE_HOST_WIN32 )
  set( PythonInterp_FIND_VERSION 2 )
  find_package( PythonInterp 2 )
  if( NOT PYTHONINTERP_FOUND )
    message( "Please set the PNACLPYTHON environment variable to your Python2 executable")
  endif()
  set( PNACLPYTHON_PREFIX env "PNACLPYTHON=${PYTHON_EXECUTABLE}" )
endif()

################################################################################
# Directories
################################################################################

# Dependencies version, this must match the number in external_deps/build.sh
set( DEPS_VERSION 2 )

set( CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
set( MOUNT_DIR ${CMAKE_SOURCE_DIR}/src )
set( OBJ_DIR ${CMAKE_BINARY_DIR}${CMAKE_FILES_DIRECTORY} )
set( ENGINE_DIR ${MOUNT_DIR}/engine )
set( COMMON_DIR ${MOUNT_DIR}/common )
set( GAMELOGIC_DIR ${MOUNT_DIR}/gamelogic )
set( TOOLS_DIR ${MOUNT_DIR}/tools )
set( LCC_DIR ${TOOLS_DIR}/lcc )
set( LIB_DIR ${MOUNT_DIR}/libs )
set( ROCKET_DIR ${LIB_DIR}/libRocket )
if( CMAKE_CFG_INTDIR STREQUAL "." )
  set( FULL_OUTPUT_DIR ${CMAKE_BINARY_DIR} )
else()
  set( FULL_OUTPUT_DIR ${CMAKE_BINARY_DIR}/${CMAKE_CFG_INTDIR} )
endif()
if( MSVC )
  set( DEPS_EXT ".zip" )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/msvc64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/msvc32-${DEPS_VERSION} )
  endif()
elseif( WIN32 )
  set( DEPS_EXT ".zip" )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/mingw64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/mingw32-${DEPS_VERSION} )
  endif()
elseif( APPLE )
  set( DEPS_EXT ".tar.bz2" )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/macosx64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/macosx32-${DEPS_VERSION} )
  endif()
elseif( LINUX )
  set( DEPS_EXT ".tar.bz2" )
  # Our minimal NaCl .debs put the files in /usr/lib/nacl, so check that first
  if( EXISTS "/usr/lib/nacl/sel_ldr" )
    set( DEPS_DIR "/usr/lib/nacl" )
  elseif( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/linux64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${CMAKE_SOURCE_DIR}/external_deps/linux32-${DEPS_VERSION} )
  endif()
endif()
if( LINUX AND ARCH STREQUAL "x86_64" )
  set( PNACL_TOOLCHAIN ${DEPS_DIR}/pnacl/bin64 )
else()
  set( PNACL_TOOLCHAIN ${DEPS_DIR}/pnacl/bin )
endif()
if( WIN32 )
  set( PNACL_BIN_EXT ".bat" )
else()
  set( PNACL_BIN_EXT "" )
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
# Support for precompiled headers
################################################################################

# MSVC requires that an extra file be added to a project
if( USE_PRECOMPILED_HEADER AND MSVC )
  file( WRITE ${OBJ_DIR}/PrecompiledHeader.cpp "" )
  set( PCH_FILE ${OBJ_DIR}/PrecompiledHeader.cpp )
endif()

function( ADD_PRECOMPILED_HEADER Target Header )
  if( USE_PRECOMPILED_HEADER AND NOT CMAKE_VERSION VERSION_LESS 2.8.10 )
    # Get the common compile flags
    set( Flags ${CMAKE_CXX_FLAGS} )
    get_target_property( Type ${Target} TYPE )
    if( Type STREQUAL MODULE_LIBRARY )
        set( Flags ${Flags} ${CMAKE_SHARED_MODULE_CXX_FLAGS} )
    endif()
    separate_arguments( Flags )

    # Get the per-configuration compile flags
    foreach( Config Debug Release RelWithDebInfo MinSizeRel )
      string( TOUPPER ${Config} CONFIG )
      set( ConfigFlags ${CMAKE_CXX_FLAGS_${CONFIG}} )
      separate_arguments( ConfigFlags )
      foreach( Flag ${ConfigFlags} )
        set( Flags ${Flags} $<$<CONFIG:${Config}>:${Flag}> )
      endforeach()
    endforeach()

    # Get preprocessor options for the target and directory (global)
    get_directory_property( DirCompileDefs COMPILE_DEFINITIONS )
    get_directory_property( DirIncludeDirs INCLUDE_DIRECTORIES )
    get_target_property( TargetCompileDefs ${Target} COMPILE_DEFINITIONS )
    set( Defs )
    foreach( Def ${TargetCompileDefs} )
      set( Defs ${Defs} -D${Def} )
    endforeach()
    foreach( Def ${DirCompileDefs} )
      set( Defs ${Defs} -D${Def} )
    endforeach()
    foreach( Def ${DirIncludeDirs} )
      set( Defs ${Defs} -I${Def} )
    endforeach()

    # Specify minimum OSX version
    if( APPLE AND CMAKE_OSX_DEPLOYMENT_TARGET )
      set( Flags ${Flags} -mmacosx-version-min=${CMAKE_OSX_DEPLOYMENT_TARGET} )
    endif()

    # Compiler-specific PCH support
    if( CMAKE_COMPILER_IS_GNUCXX OR "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
      add_custom_command( OUTPUT "${OBJ_DIR}/${Target}.h.gch"
        COMMAND ${CMAKE_CXX_COMPILER} ${Defs} ${Flags} -x c++-header ${Header} -o "${OBJ_DIR}/${Target}.h.gch"
        DEPENDS ${Header}
        IMPLICIT_DEPENDS CXX ${Header}
      )

      add_custom_target( ${Target}-pch DEPENDS "${OBJ_DIR}/${Target}.h.gch" )
      add_dependencies( ${Target} ${Target}-pch )

      if( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
        set_target_properties( ${Target} PROPERTIES COMPILE_FLAGS "-include-pch ${OBJ_DIR}/${Target}.h.gch" )
      else()
        set_target_properties( ${Target} PROPERTIES COMPILE_FLAGS "-include ${OBJ_DIR}/${Target}.h -Winvalid-pch" )
      endif()
    elseif( MSVC )
      set_source_files_properties( ${PCH_FILE} PROPERTIES COMPILE_FLAGS "/Yc${Header}" )
      set_target_properties( ${Target} PROPERTIES COMPILE_FLAGS "/Yu${Header} /Fp${OBJ_DIR}/${Target}.pch /FI${Header}" )
    endif()
  endif()
endfunction()

################################################################################
# File lists
################################################################################

set( SHAREDLIST_H
  ${ENGINE_DIR}/qcommon/q_shared.h
  ${ENGINE_DIR}/qcommon/q_unicode.h
  ${ENGINE_DIR}/qcommon/unicode_data.h
  ${ENGINE_DIR}/qcommon/logging.h
)

set( SHAREDLIST_C
  ${ENGINE_DIR}/qcommon/q_shared.c
  ${ENGINE_DIR}/qcommon/q_unicode.c
  ${ENGINE_DIR}/qcommon/q_math.c
  ${SHAREDLIST_H}
)

set( SHAREDLIST
  ${ENGINE_DIR}/qcommon/q_shared.cpp
  ${ENGINE_DIR}/qcommon/q_unicode.cpp
  ${ENGINE_DIR}/qcommon/q_math.cpp
  ${SHAREDLIST_H}
)

set( COMMONLIST
  ${COMMON_DIR}/Command.cpp
  ${COMMON_DIR}/Command.h
  ${COMMON_DIR}/Cvar.cpp
  ${COMMON_DIR}/Cvar.h
  ${COMMON_DIR}/FileSystem.cpp
  ${COMMON_DIR}/FileSystem.h
  ${COMMON_DIR}/IPC.h
  ${COMMON_DIR}/LineEditData.cpp
  ${COMMON_DIR}/LineEditData.h
  ${COMMON_DIR}/Log.cpp
  ${COMMON_DIR}/Log.h
  ${COMMON_DIR}/IPC.cpp
  ${COMMON_DIR}/IPC.h
  ${COMMON_DIR}/String.cpp
  ${COMMON_DIR}/String.h
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
)

set( RENDERERLIST
  ${ENGINE_DIR}/sys/sdl2_compat.cpp
  ${ENGINE_DIR}/sys/sdl_glimp.cpp
  ${ENGINE_DIR}/renderer/gl_shader.cpp
  ${ENGINE_DIR}/renderer/tr_animation.cpp
  ${ENGINE_DIR}/renderer/tr_backend.cpp
  ${ENGINE_DIR}/renderer/tr_bsp.cpp
  ${ENGINE_DIR}/renderer/tr_cmds.cpp
  ${ENGINE_DIR}/renderer/tr_curve.cpp
  ${ENGINE_DIR}/renderer/tr_decals.cpp
  ${ENGINE_DIR}/renderer/tr_fbo.cpp
  ${ENGINE_DIR}/renderer/tr_flares.cpp
  ${ENGINE_DIR}/renderer/tr_fog.cpp
  ${ENGINE_DIR}/renderer/tr_font.cpp
  ${ENGINE_DIR}/renderer/tr_image.cpp
  ${ENGINE_DIR}/renderer/tr_image_crn.cpp
  ${ENGINE_DIR}/renderer/tr_image_dds.cpp
  ${ENGINE_DIR}/renderer/tr_image_exr.cpp
  ${ENGINE_DIR}/renderer/tr_image_jpg.cpp
  ${ENGINE_DIR}/renderer/tr_image_ktx.cpp
  ${ENGINE_DIR}/renderer/tr_image_png.cpp
  ${ENGINE_DIR}/renderer/tr_image_tga.cpp
  ${ENGINE_DIR}/renderer/tr_image_webp.cpp
  ${ENGINE_DIR}/renderer/tr_init.cpp
  ${ENGINE_DIR}/renderer/tr_light.cpp
  ${ENGINE_DIR}/renderer/tr_main.cpp
  ${ENGINE_DIR}/renderer/tr_marks.cpp
  ${ENGINE_DIR}/renderer/tr_mesh.cpp
  ${ENGINE_DIR}/renderer/tr_model.cpp
  ${ENGINE_DIR}/renderer/tr_model_md3.cpp
  ${ENGINE_DIR}/renderer/tr_model_md5.cpp
  ${ENGINE_DIR}/renderer/tr_model_iqm.cpp
  ${ENGINE_DIR}/renderer/tr_model_skel.cpp
  ${ENGINE_DIR}/renderer/tr_noise.cpp
  ${ENGINE_DIR}/renderer/tr_scene.cpp
  ${ENGINE_DIR}/renderer/tr_shade.cpp
  ${ENGINE_DIR}/renderer/tr_shade_calc.cpp
  ${ENGINE_DIR}/renderer/tr_shader.cpp
  ${ENGINE_DIR}/renderer/tr_shadows.cpp
  ${ENGINE_DIR}/renderer/tr_skin.cpp
  ${ENGINE_DIR}/renderer/tr_sky.cpp
  ${ENGINE_DIR}/renderer/tr_surface.cpp
  ${ENGINE_DIR}/renderer/tr_vbo.cpp
  ${ENGINE_DIR}/renderer/tr_world.cpp
  ${LIB_DIR}/openexr/half.cpp
  ${ENGINE_DIR}/renderer/tr_local.h
  ${ENGINE_DIR}/renderer/tr_public.h
  ${ENGINE_DIR}/renderer/gl_shader.h
  ${ENGINE_DIR}/renderer/tr_image.h
  ${ENGINE_DIR}/renderer/tr_model_skel.h
)

set( SERVERLIST
  ${ENGINE_DIR}/botlib/bot_convert.cpp
  ${ENGINE_DIR}/botlib/bot_local.cpp
  ${ENGINE_DIR}/botlib/bot_nav.cpp
  ${ENGINE_DIR}/botlib/bot_load.cpp
  ${ENGINE_DIR}/server/sv_bot.cpp
  ${ENGINE_DIR}/server/sv_ccmds.cpp
  ${ENGINE_DIR}/server/sv_client.cpp
  ${ENGINE_DIR}/server/sv_game.cpp
  ${ENGINE_DIR}/server/sv_init.cpp
  ${ENGINE_DIR}/server/sv_main.cpp
  ${ENGINE_DIR}/server/sv_net_chan.cpp
  ${ENGINE_DIR}/server/sv_snapshot.cpp
  ${ENGINE_DIR}/server/server.h
  ${ENGINE_DIR}/server/g_api.h
  ${ENGINE_DIR}/qcommon/print_translated.h
)

set( ENGINELIST
  ${ENGINE_DIR}/framework/BaseCommands.cpp
  ${ENGINE_DIR}/framework/BaseCommands.h
  ${ENGINE_DIR}/framework/CommandSystem.cpp
  ${ENGINE_DIR}/framework/CommandSystem.h
  ${ENGINE_DIR}/framework/CvarSystem.cpp
  ${ENGINE_DIR}/framework/CvarSystem.h
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
  ${ENGINE_DIR}/qcommon/vm.cpp
  ${ENGINE_DIR}/qcommon/vm_interpreted.cpp
  ${ENGINE_DIR}/qcommon/vm_local.h
  ${ENGINE_DIR}/qcommon/vm_traps.h
  ${ENGINE_DIR}/sys/sys_main.cpp
  ${ENGINE_DIR}/sys/con_log.cpp
  ${ENGINE_DIR}/qcommon/qcommon.h
  ${ENGINE_DIR}/qcommon/surfaceflags.h
  ${COMMONLIST}
)

if( ARCH STREQUAL "x86" OR ARCH STREQUAL "x86_64" )
  set( ENGINELIST ${ENGINELIST} ${ENGINE_DIR}/qcommon/vm_x86.cpp )
  if( MSVC AND CMAKE_CL_64 )
    add_custom_command( OUTPUT vm_x86_64.obj COMMAND ml64 ARGS /c "${ENGINE_DIR}/qcommon/vm_x86_64.asm" MAIN_DEPENDENCY ${ENGINE_DIR}/qcommon/vm_x86_64.asm )
    set( ENGINELIST ${ENGINELIST} vm_x86_64.obj )
  endif()
endif()

if( WIN32 )
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/sys_win32.cpp
    ${ENGINE_DIR}/sys/sys_win32.h
    ${ENGINE_DIR}/sys/con_passive.cpp
  )
else()
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/sys_unix.cpp
    ${ENGINE_DIR}/sys/con_tty.cpp
  )
endif()

if( NOT APPLE )
  set( ENGINELIST ${ENGINELIST}
    ${ENGINE_DIR}/sys/con_curses.cpp
  )
endif()

set( CLIENTBASELIST
  ${ENGINE_DIR}/botlib/bot_debug.cpp
  ${ENGINE_DIR}/botlib/bot_nav_edit.cpp
  ${ENGINE_DIR}/client/cg_api.h
  ${ENGINE_DIR}/client/ui_api.h
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
  ${ENGINE_DIR}/client/cl_net_chan.cpp
  ${ENGINE_DIR}/client/cl_parse.cpp
  ${ENGINE_DIR}/client/cl_scrn.cpp
  ${ENGINE_DIR}/client/dl_main.cpp
  ${ENGINE_DIR}/client/libmumblelink.cpp
  ${ENGINE_DIR}/client/libmumblelink.h
)

set( CLIENTLIST
  ${ENGINE_DIR}/audio/ALObjects.cpp
  ${ENGINE_DIR}/audio/ALObjects.h
  ${ENGINE_DIR}/audio/Audio.cpp
  ${ENGINE_DIR}/audio/Audio.h
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
)

set( TTYCLIENTLIST
    ${ENGINE_DIR}/null/null_input.cpp
    ${ENGINE_DIR}/null/null_renderer.cpp
    ${ENGINE_DIR}/null/NullAudio.cpp
    ${ENGINE_DIR}/null/null_rocket.cpp
)

set( DEDSERVERLIST
    ${ENGINE_DIR}/null/null_input.cpp
    ${ENGINE_DIR}/null/null_client.cpp
)

set( SDLLIST
  ${ENGINE_DIR}/sys/sdl_input.cpp
  ${ENGINE_DIR}/sys/sdl2_compat.cpp
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
  ${GAMELOGIC_DIR}/shared/bg_utilities.c
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
  ${GAMELOGIC_DIR}/cgame/cg_rocket.c
  ${GAMELOGIC_DIR}/cgame/cg_rocket_draw.c
  ${GAMELOGIC_DIR}/cgame/cg_rocket_events.c
  ${GAMELOGIC_DIR}/cgame/cg_rocket_datasource.c
  ${GAMELOGIC_DIR}/cgame/cg_rocket_progressbar.c
  ${GAMELOGIC_DIR}/cgame/cg_rocket_dataformatter.c
  ${GAMELOGIC_DIR}/cgame/cg_gameinfo.c
  ${GAMELOGIC_DIR}/cgame/cg_parseutils.c
  ${ENGINE_DIR}/client/cg_api.h
  ${ENGINE_DIR}/qcommon/print_translated.h
)

set( CGAMENATIVELIST
  ${GAMELOGIC_DIR}/cgame/cg_api.c
)

set( CGAMEQVMLIST
  ${GAMELOGIC_DIR}/shared/bg_lib.c
  ${ENGINE_DIR}/qcommon/q_static.c
  ${SHAREDLIST_C}
)

set( CGAMEASMLIST
  ${GAMELOGIC_DIR}/cgame/cg_api.asm
)

set( GAMELIST
  ${GAMELOGIC_DIR}/game/g_main.cpp
  ${GAMELOGIC_DIR}/shared/bg_misc.c
  ${GAMELOGIC_DIR}/shared/bg_parse.c
  ${GAMELOGIC_DIR}/shared/bg_pmove.c
  ${GAMELOGIC_DIR}/shared/bg_slidemove.c
  ${GAMELOGIC_DIR}/shared/bg_alloc.c
  ${GAMELOGIC_DIR}/shared/bg_voice.c
  ${GAMELOGIC_DIR}/shared/bg_teamprogress.c
  ${GAMELOGIC_DIR}/shared/bg_utilities.c
  ${GAMELOGIC_DIR}/shared/CommonProxies.cpp
  ${GAMELOGIC_DIR}/shared/CommonProxies.h
  ${GAMELOGIC_DIR}/shared/VMMain.h
  ${GAMELOGIC_DIR}/game/g_active.cpp
  ${GAMELOGIC_DIR}/game/g_client.cpp
  ${GAMELOGIC_DIR}/game/g_cmds.cpp
  ${GAMELOGIC_DIR}/game/g_combat.cpp
  ${GAMELOGIC_DIR}/game/g_confidence.cpp
  ${GAMELOGIC_DIR}/game/g_entities.cpp
  ${GAMELOGIC_DIR}/game/g_physics.cpp
  ${GAMELOGIC_DIR}/game/g_buildable.cpp
  ${GAMELOGIC_DIR}/game/g_missile.cpp
  ${GAMELOGIC_DIR}/game/g_session.cpp
  ${GAMELOGIC_DIR}/game/g_spawn_afx.cpp
  ${GAMELOGIC_DIR}/game/g_spawn_ctrl.cpp
  ${GAMELOGIC_DIR}/game/g_spawn_fx.cpp
  ${GAMELOGIC_DIR}/game/g_spawn_game.cpp
  ${GAMELOGIC_DIR}/game/g_spawn_generic.cpp
  ${GAMELOGIC_DIR}/game/g_spawn_gfx.cpp
  ${GAMELOGIC_DIR}/game/g_spawn_mover.cpp
  ${GAMELOGIC_DIR}/game/g_spawn_position.cpp
  ${GAMELOGIC_DIR}/game/g_spawn_sensor.cpp
  ${GAMELOGIC_DIR}/game/g_spawn_shared.cpp
  ${GAMELOGIC_DIR}/game/g_spawn.cpp
  ${GAMELOGIC_DIR}/game/g_svcmds.cpp
  ${GAMELOGIC_DIR}/game/g_team.cpp
  ${GAMELOGIC_DIR}/game/g_utils.cpp
  ${GAMELOGIC_DIR}/game/g_maprotation.cpp
  ${GAMELOGIC_DIR}/game/g_weapon.cpp
  ${GAMELOGIC_DIR}/game/g_admin.cpp
  ${GAMELOGIC_DIR}/game/g_namelog.cpp
  ${GAMELOGIC_DIR}/game/g_bot.cpp
  ${GAMELOGIC_DIR}/game/g_bot_ai.cpp
  ${GAMELOGIC_DIR}/game/g_bot_nav.cpp
  ${GAMELOGIC_DIR}/game/g_bot_parse.cpp
  ${GAMELOGIC_DIR}/game/g_bot_util.cpp
  ${GAMELOGIC_DIR}/game/g_cm_world.cpp
  ${GAMELOGIC_DIR}/game/g_cm_world.h
  ${ENGINE_DIR}/server/g_api.h
)

set( GAMENACLLIST
  ${GAMELOGIC_DIR}/game/g_api.cpp
  ${COMMONLIST}
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
  ${GAMELOGIC_DIR}/shared/bg_utilities.c
  ${GAMELOGIC_DIR}/ui/ui_utf8.c
  ${ENGINE_DIR}/client/ui_api.h
)

set( UINATIVELIST
  ${GAMELOGIC_DIR}/ui/ui_api.c
)

set( UIQVMLIST
  ${GAMELOGIC_DIR}/shared/bg_lib.c
  ${ENGINE_DIR}/qcommon/q_static.c
  ${SHAREDLIST_C}
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
    set( REMOTE "http://unvnet.net/unvanquished_deps/${BASENAME}${DEPS_EXT}" )
    message( STATUS "Downloading dependencies from '${REMOTE}'" )
    file( DOWNLOAD ${REMOTE} ${OBJ_DIR}/${BASENAME}${DEPS_EXT} SHOW_PROGRESS
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
    execute_process( COMMAND ${CMAKE_COMMAND} -E tar xzf ${OBJ_DIR}/${BASENAME}${DEPS_EXT}
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/external_deps RESULT_VARIABLE EXTRACT_RESULT )
    if( NOT EXTRACT_RESULT EQUAL 0 )
      message( FATAL_ERROR "Could not extract ${BASENAME}${DEPS_EXT}" )
    endif()
  endif()

  # Add to paths
  set( CMAKE_FIND_ROOT_PATH ${DEPS_DIR} ${CMAKE_FIND_ROOT_PATH} )
  set( CMAKE_INCLUDE_PATH ${DEPS_DIR} ${DEPS_DIR}/include ${CMAKE_INCLUDE_PATH} )
  set( CMAKE_FRAMEWORK_PATH ${DEPS_DIR} ${CMAKE_FRAMEWORK_PATH} )
  set( CMAKE_LIBRARY_PATH ${DEPS_DIR}/lib ${CMAKE_LIBRARY_PATH} )

  # Force OpenAL Soft on Mac, otherwise the system framework will be selected
  if( APPLE )
    set( OPENAL_INCLUDE_DIR ${DEPS_DIR}/include/AL )
    set( OPENAL_LIBRARY ${DEPS_DIR}/lib/libopenal.dylib )
  endif()
endif()

# These variables are defined:
# LIBS_BASE, LIBS_ENGINE, LIBS_BASECLIENT, LIBS_CLIENT
# LIBSRC_BASE, LIBSRC_NACL, LIBSRC_ENGINE

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
set( LIBSRC_BASE ${LIBSRC_BASE} ${MINIZIPLIST} )

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
set( LIBSRC_NACL ${LIBSRC_NACL} ${NACLLIST_NATIVE} )

# SDL, required for all targets on win32 because of iconv
if( BUILD_CLIENT OR ( WIN32 AND (BUILD_TTY_CLIENT OR BUILD_SERVER ) ) )
  if( USE_SDL2 )
    find_package( SDL2 REQUIRED )
    include_directories( ${SDL2_INCLUDE_DIR} )
    if( WIN32 )
      set( LIBS_ENGINE ${LIBS_ENGINE} ${SDL2_LIBRARY} )
    else()
      set( LIBS_CLIENT ${LIBS_CLIENT} ${SDL2_LIBRARY} )
    endif()
  else()
    find_package( SDL REQUIRED )
    include_directories( ${SDL_INCLUDE_DIR} )
    if( WIN32 )
      set( LIBS_ENGINE ${LIBS_ENGINE} ${SDL_LIBRARY} )
    else()
      set( LIBS_CLIENT ${LIBS_CLIENT} ${SDL_LIBRARY} )
    endif()
  endif()

  # Used by tinygettext to use SDL_iconv instead of base iconv
  if( WIN32 )
    add_definitions( -DHAVE_SDL )
  endif()
endif()
mark_as_advanced( SDLMAIN_LIBRARY SDL_LIBRARY SDL_INCLUDE_DIR MINGW32_LIBRARY )
mark_as_advanced( SDL2MAIN_LIBRARY SDL2_LIBRARY SDL2_INCLUDE_DIR )

# libRocket
if( BUILD_CLIENT )
  include_directories( ${ROCKET_DIR}/Include ${ROCKET_DIR}/Contrib/Include )
  add_subdirectory( ${ROCKET_DIR} )
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

# X11
if( LINUX AND BUILD_CLIENT )
  find_package( X11 REQUIRED )
  include_directories( ${X11_INCLUDE_DIRS} )
  set( LIBS_CLIENT ${LIBS_CLIENT} ${X11_LIBRARIES} )
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
add_library( base-source-libs EXCLUDE_FROM_ALL ${LIBSRC_BASE} )
add_library( nacl-source-libs EXCLUDE_FROM_ALL ${LIBSRC_NACL} )
add_library( engine-source-libs EXCLUDE_FROM_ALL ${LIBSRC_ENGINE} )
set_target_properties( base-source-libs nacl-source-libs engine-source-libs PROPERTIES POSITION_INDEPENDENT_CODE 1 )

set( LIBS_BASE base-source-libs ${LIBS_BASE} )
set( LIBS_ENGINE engine-source-libs nacl-source-libs ${LIBS_ENGINE} )

################################################################################
# Game logic
################################################################################

# Function to setup all the Game/Cgame/UI libraries
include( CMakeParseArguments )
function( GAMEMODULE )
  # ParseArguments setup
  set( oneValueArgs NAME HAS_NACL HAS_QVM )
  set( multiValueArgs COMPILE_DEF FILES NACL_FILES NATIVE_FILES QVM_FILES QVM_ASM )
  cmake_parse_arguments( GAMEMODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if( GAMEMODULE_HAS_NACL AND BUILD_GAME_NACL_NATIVE_DLL )
    add_library( ${GAMEMODULE_NAME}-nacl-native-dll MODULE ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST} ${GAMEMODULE_NACL_FILES} )
    target_link_libraries( ${GAMEMODULE_NAME}-nacl-native-dll nacl-source-libs ${LIBS_BASE} )
    set_target_properties( ${GAMEMODULE_NAME}-nacl-native-dll PROPERTIES
      PREFIX ""
      COMPILE_DEFINITIONS "${GAMEMODULE_COMPILE_DEF}"
    )
    ADD_PRECOMPILED_HEADER( ${GAMEMODULE_NAME}-nacl-native-dll ${COMMON_DIR}/Common.h )
  endif()

  if( GAMEMODULE_HAS_NACL AND BUILD_GAME_NACL_NATIVE_EXE )
    add_executable( ${GAMEMODULE_NAME}-nacl-native-exe ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST} ${GAMEMODULE_NACL_FILES} )
    target_link_libraries( ${GAMEMODULE_NAME}-nacl-native-exe nacl-source-libs ${LIBS_BASE} )
    set_target_properties( ${GAMEMODULE_NAME}-nacl-native-exe PROPERTIES
      COMPILE_DEFINITIONS "${GAMEMODULE_COMPILE_DEF}"
    )
    ADD_PRECOMPILED_HEADER( ${GAMEMODULE_NAME}-nacl-native-exe ${COMMON_DIR}/Common.h )
  endif()

  if( GAMEMODULE_HAS_QVM AND BUILD_GAME_QVM_NATIVE )
    set( QVM_NATIVE_COMPILE_DEF ${GAMEMODULE_COMPILE_DEF} QVM_ABI)
    add_library( ${GAMEMODULE_NAME}-qvm-native MODULE ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST} ${GAMEMODULE_NATIVE_FILES} )
    target_link_libraries( ${GAMEMODULE_NAME}-qvm-native ${LIBS_BASE} )
    set_target_properties( ${GAMEMODULE_NAME}-qvm-native PROPERTIES
      PREFIX ""
      COMPILE_DEFINITIONS "${QVM_NATIVE_COMPILE_DEF}"
    )
    ADD_PRECOMPILED_HEADER( ${GAMEMODULE_NAME}-qvm-native ${COMMON_DIR}/Common.h )
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
      -I${LIB_DIR}/zlib
      -DNACL_BUILD_ARCH=x86 # Not actually used
      -DNACL_BUILD_SUBARCH=64 # Not actually used
      -Wall
      -ffast-math
      -fvisibility=hidden
      -stdlib=libc++
      --pnacl-allow-exceptions
    )
    set( NACL_TRANSLATE_OPTIONS
      --allow-llvm-bitcode-input
      --pnacl-allow-exceptions
    )
    if( CMAKE_VERSION VERSION_LESS 2.8.10 )
      # Use some default flags if cmake doesn't support generator expressions
      set( NACL_COMPILE_OPTIONS
        ${NACL_COMPILE_OPTIONS}
        -O3
        -DNDEBUG
      )
      set( NACL_TRANSLATE_OPTIONS
        ${NACL_TRANSLATE_OPTIONS}
        -O3
      )
    else()
      set( NACL_COMPILE_OPTIONS
        ${NACL_COMPILE_OPTIONS}
        $<$<CONFIG:None>:-O3>
        $<$<CONFIG:None>:-DNDEBUG>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Release>:-DNDEBUG>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:Debug>:-g3>
        $<$<CONFIG:RelWithDebInfo>:-O2>
        $<$<CONFIG:RelWithDebInfo>:-DNDEBUG>
        $<$<CONFIG:RelWithDebInfo>:-g3>
        $<$<CONFIG:MinSizeRel>:-Os>
        $<$<CONFIG:MinSizeRel>:-DNDEBUG>
      )
      set( NACL_TRANSLATE_OPTIONS
        ${NACL_TRANSLATE_OPTIONS}
        $<$<CONFIG:None>:-O3>
        $<$<CONFIG:Release>:-O3>
        $<$<CONFIG:Debug>:-O0>
        $<$<CONFIG:RelWithDebInfo>:-O2>
        $<$<CONFIG:MinSizeRel>:-O2>
      )
    endif()
    if( USE_WEXTRA )
      set( NACL_COMPILE_OPTIONS ${NACL_COMPILE_OPTIONS} "-Wextra" )
    else()
      set( NACL_COMPILE_OPTIONS ${NACL_COMPILE_OPTIONS} "-Wno-sign-compare" )
      set( NACL_COMPILE_OPTIONS ${NACL_COMPILE_OPTIONS} "-Wno-write-strings" )
    endif()
    if( USE_PEDANTIC )
      set( NACL_COMPILE_OPTIONS ${NACL_COMPILE_OPTIONS} "-pedantic" )
    endif()
    foreach( DEF ${GAMEMODULE_COMPILE_DEF} )
      set( NACL_COMPILE_OPTIONS ${NACL_COMPILE_OPTIONS} -D${DEF} )
    endforeach()

    # Compile C files with clang and C++ files with clang++
    set( OBJECTS )
    foreach( FILE ${GAMEMODULE_FILES} ${GAMEMODULE_NACL_FILES} ${SHAREDLIST} ${NACLLIST_MODULE} ${ZLIBLIST} ${LIBSRC_BASE} )
      get_filename_component( FILENAME ${FILE} NAME_WE )
      get_filename_component( FILEDIR ${FILE} PATH )
      get_filename_component( EXT ${FILE} EXT )
      set( OBJ ${NACL_DIR}/${FILENAME}.o )

      if( EXT STREQUAL ".c" AND FILEDIR MATCHES ${LIB_DIR} )
        add_custom_command( OUTPUT ${OBJ}
          COMMAND ${PNACLPYTHON_PREFIX} ${PNACL_TOOLCHAIN}/pnacl-clang${PNACL_BIN_EXT}
            -std=gnu89
            ${NACL_COMPILE_OPTIONS}
            -c
            -o ${OBJ}
            ${FILE}
          DEPENDS ${FILE}
          IMPLICIT_DEPENDS C ${FILE}
        )
        set( OBJECTS ${OBJECTS} ${OBJ} )
      elseif( EXT STREQUAL ".c" OR EXT STREQUAL ".cpp" OR EXT STREQUAL ".cc" )
        add_custom_command( OUTPUT ${OBJ}
          COMMAND ${PNACLPYTHON_PREFIX} ${PNACL_TOOLCHAIN}/pnacl-clang++${PNACL_BIN_EXT}
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

    # Generate PNaCL platform-independent executable
    add_custom_command(
      OUTPUT ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
      COMMAND ${PNACLPYTHON_PREFIX} ${PNACL_TOOLCHAIN}/pnacl-clang++${PNACL_BIN_EXT}
        ${NACL_COMPILE_OPTIONS}
        -o ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
        ${OBJECTS}
      DEPENDS ${OBJECTS}
    )

    # Generate NaCl executables for x86 and x86_64
    add_custom_command(
      OUTPUT ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86.nexe
      COMMAND ${PNACLPYTHON_PREFIX} ${PNACL_TOOLCHAIN}/pnacl-translate${PNACL_BIN_EXT}
        ${NACL_TRANSLATE_OPTIONS}
        -arch i686
        -o ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86.nexe
        ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
      DEPENDS ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
    )
    add_custom_command(
      OUTPUT ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86_64.nexe
      COMMAND ${PNACLPYTHON_PREFIX} ${PNACL_TOOLCHAIN}/pnacl-translate${PNACL_BIN_EXT}
        ${NACL_TRANSLATE_OPTIONS}
        -arch x86-64
        -o ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86_64.nexe
        ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
      DEPENDS ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
    )

    # Create stripped versions of the NaCl executables
    add_custom_command(
      OUTPUT ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86-stripped.nexe
      COMMAND ${PNACLPYTHON_PREFIX} ${PNACL_TOOLCHAIN}/pnacl-strip${PNACL_BIN_EXT}
        -s
        -o ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86-stripped.nexe
        ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86.nexe
      DEPENDS ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86.nexe
    )
    add_custom_command(
      OUTPUT ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86_64-stripped.nexe
      COMMAND ${PNACLPYTHON_PREFIX} ${PNACL_TOOLCHAIN}/pnacl-strip${PNACL_BIN_EXT}
        -s
        -o ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86_64-stripped.nexe
        ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86_64.nexe
      DEPENDS ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86_64.nexe
    )

    # Dummy target to get CMake to execute the commands
    add_custom_target( ${GAMEMODULE_NAME}-nacl ALL
      DEPENDS ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}.pexe
        ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86.nexe
        ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86_64.nexe
        ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86-stripped.nexe
        ${FULL_OUTPUT_DIR}/${GAMEMODULE_NAME}-x86_64-stripped.nexe
    )

    # Create the build directory before building
    add_custom_command( TARGET ${GAMEMODULE_NAME}-nacl PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${FULL_OUTPUT_DIR}
      COMMAND ${CMAKE_COMMAND} -E make_directory ${NACL_DIR}
    )
  endif()

  if( GAMEMODULE_HAS_QVM AND BUILD_GAME_QVM )
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
    add_custom_command( OUTPUT ${CMAKE_BINARY_DIR}/vm/${GAMEMODULE_NAME}.qvm
      COMMAND asm -o ${CMAKE_BINARY_DIR}/vm/${GAMEMODULE_NAME}.qvm ${OBJECTS} ${GAMEMODULE_QVM_ASM}
      DEPENDS asm ${OBJECTS} ${GAMEMODULE_QVM_ASM}
    )

    # Dummy target to get CMake to execute the commands
    add_custom_target( ${GAMEMODULE_NAME}-qvm ALL
      DEPENDS ${CMAKE_BINARY_DIR}/vm/${GAMEMODULE_NAME}.qvm
    )

    # Create the build directory before building
    add_custom_command( TARGET ${GAMEMODULE_NAME}-qvm PRE_BUILD
      COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/vm
      COMMAND ${CMAKE_COMMAND} -E make_directory ${QVM_DIR}
    )
  endif()
endfunction()

if( BUILD_GAME_QVM )
  add_subdirectory( ${LCC_DIR} ${CMAKE_BINARY_DIR}/lcc )
endif()

GAMEMODULE( NAME game
  COMPILE_DEF BUILD_VM BUILD_GAME
  HAS_NACL 1
  HAS_QVM 0
  FILES ${GAMELIST}
  NACL_FILES ${GAMENACLLIST}
  NATIVE_FILES ${GAMENATIVELIST}
  QVM_FILES ${GAMEQVMLIST}
  QVM_ASM ${GAMEASMLIST}
)

GAMEMODULE( NAME cgame
  COMPILE_DEF BUILD_VM BUILD_CGAME
  HAS_NACL 0
  HAS_QVM 1
  FILES ${CGAMELIST}
  NACL_FILES ${CGAMENACLLIST}
  NATIVE_FILES ${CGAMENATIVELIST}
  QVM_FILES ${CGAMEQVMLIST}
  QVM_ASM ${CGAMEASMLIST}
)

################################################################################
# Engine
################################################################################

if( BUILD_CLIENT )
  add_executable( client WIN32 ${PCH_FILE} ${ENGINELIST} ${SHAREDLIST} ${CLIENTLIST} ${CLIENTBASELIST} ${RENDERERLIST} ${SERVERLIST} ${SDLLIST} ${WIN_RC} )
  if( USE_SMP )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS SMP )
  endif()
  target_link_libraries( client ${LIBS_CLIENT} ${LIBS_CLIENTBASE} ${LIBS_ENGINE} ${LIBS_BASE} )
  set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS BUILD_ENGINE BUILD_CLIENT )
  if( USE_VOIP )
    # Set it here so the tty client doesn't need to depend on speex
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_VOIP )
  endif()
  set_target_properties( client PROPERTIES OUTPUT_NAME "daemon" PREFIX "" )
  ADD_PRECOMPILED_HEADER( client ${COMMON_DIR}/Common.h )
endif()

if( BUILD_SERVER )
  add_executable( server WIN32 ${PCH_FILE} ${SERVERLIST} ${DEDSERVERLIST} ${ENGINELIST} ${SHAREDLIST} ${WIN_RC} )
  target_link_libraries( server ${LIBS_ENGINE} ${LIBS_BASE} )
  set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS BUILD_ENGINE BUILD_SERVER )
  set_target_properties( server PROPERTIES OUTPUT_NAME "daemonded" PREFIX "" )
  ADD_PRECOMPILED_HEADER( server ${COMMON_DIR}/Common.h )
endif()

if( BUILD_TTY_CLIENT )
  add_executable( ttyclient WIN32 ${PCH_FILE} ${SERVERLIST} ${CLIENTBASELIST} ${TTYCLIENTLIST} ${ENGINELIST} ${SHAREDLIST} ${WIN_RC} )
  target_link_libraries( ttyclient ${LIBS_CLIENTBASE} ${LIBS_ENGINE} ${LIBS_BASE} )
  set_property( TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS BUILD_ENGINE BUILD_TTY_CLIENT )
  set_target_properties( ttyclient PROPERTIES OUTPUT_NAME "daemon-tty" PREFIX "" )
  ADD_PRECOMPILED_HEADER( ttyclient ${COMMON_DIR}/Common.h )
endif()

################################################################################
# NaCl SDK files
################################################################################

if( LINUX )
  add_custom_target( nacl_sdk ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/sel_ldr${CMAKE_EXECUTABLE_SUFFIX} ${FULL_OUTPUT_DIR}/sel_ldr${CMAKE_EXECUTABLE_SUFFIX}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/irt_core-${ARCH}.nexe ${FULL_OUTPUT_DIR}/irt_core-${ARCH}.nexe
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/nacl_helper_bootstrap ${FULL_OUTPUT_DIR}/nacl_helper_bootstrap )
elseif(WIN32 AND ARCH STREQUAL x86)
  # Win32 requires sel_ldr64.exe in order to run on Win64
  add_custom_target( nacl_sdk ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/sel_ldr${CMAKE_EXECUTABLE_SUFFIX} ${FULL_OUTPUT_DIR}/sel_ldr${CMAKE_EXECUTABLE_SUFFIX}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/irt_core-${ARCH}.nexe ${FULL_OUTPUT_DIR}/irt_core-${ARCH}.nexe
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/sel_ldr64${CMAKE_EXECUTABLE_SUFFIX} ${FULL_OUTPUT_DIR}/sel_ldr64${CMAKE_EXECUTABLE_SUFFIX}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/irt_core-x86_64.nexe ${FULL_OUTPUT_DIR}/irt_core-x86_64.nexe )
else()
  add_custom_target( nacl_sdk ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/sel_ldr${CMAKE_EXECUTABLE_SUFFIX} ${FULL_OUTPUT_DIR}/sel_ldr${CMAKE_EXECUTABLE_SUFFIX}
    COMMAND ${CMAKE_COMMAND} -E copy_if_different ${DEPS_DIR}/irt_core-${ARCH}.nexe ${FULL_OUTPUT_DIR}/irt_core-${ARCH}.nexe )
endif()
