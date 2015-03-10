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
# Directories
################################################################################

# Dependencies version, this must match the number in external_deps/build.sh
set( DEPS_VERSION 3 )

set( CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake ${CMAKE_MODULE_PATH} )
set( MOUNT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src )
set( ENGINE_MOUNT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/daemon/src )
set( OBJ_DIR ${CMAKE_CURRENT_BINARY_DIR}${CMAKE_FILES_DIRECTORY} )
set( ENGINE_DIR ${ENGINE_MOUNT_DIR}/engine )
set( COMMON_DIR ${ENGINE_MOUNT_DIR}/common )
set( GAMELOGIC_DIR ${MOUNT_DIR}/gamelogic )
set( LIB_DIR ${MOUNT_DIR}/libs )
if( CMAKE_CFG_INTDIR STREQUAL "." )
  set( FULL_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR} )
else()
  set( FULL_OUTPUT_DIR ${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR} )
endif()
if( MSVC )
  set( DEPS_EXT ".zip" )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/msvc64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/msvc32-${DEPS_VERSION} )
  endif()
elseif( WIN32 )
  set( DEPS_EXT ".zip" )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/mingw64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/mingw32-${DEPS_VERSION} )
  endif()
elseif( APPLE )
  set( DEPS_EXT ".tar.bz2" )
  if( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/macosx64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/macosx32-${DEPS_VERSION} )
  endif()
elseif( LINUX )
  set( DEPS_EXT ".tar.bz2" )
  # Our minimal NaCl .debs put the files in /usr/lib/nacl, so check that first
  if( EXISTS "/usr/lib/nacl/nacl_loader" )
    set( DEPS_DIR "/usr/lib/nacl" )
  elseif( ARCH STREQUAL "x86_64" )
    set( DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/linux64-${DEPS_VERSION} )
  else()
    set( DEPS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/external_deps/linux32-${DEPS_VERSION} )
  endif()
endif()
set( PNACL_TOOLCHAIN ${DEPS_DIR}/pnacl/bin )
if( WIN32 )
  set( PNACL_BIN_EXT ".bat" )
else()
  set( PNACL_BIN_EXT "" )
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
  set_linker_flag( "/LARGEADDRESSAWARE" )

  # Turn off C4503:, e.g:
  # warning C4503: 'std::_Tree<std::_Tmap_traits<_Kty,_Ty,_Pr,_Alloc,false>>::_Insert_hint' : decorated name length exceeded, name was truncated
  # No issue will be caused from this error as long as no two symbols become identical by being truncated.
  # In practice this rarely happens and even the standard libraries are affected as in the example. So there really is not
  # much that can to done about it and the warnings about each truncation really just make it more likely
  # that other more real issues might get missed. So better to remove the distraction when it really is very unlikey to happen.
  set_c_cxx_flag( "/wd4503" )

  # Turn off warning C4996:, e.g:
  # warning C4996: 'open': The POSIX name for this item is deprecated. Instead, use the ISO C++ conformant name: _open. See online help for details.  set_c_cxx_flag( "/wd4996" )
  # open seems far more popular than _open not to mention nicer. There doesn't seem to be any reason or will to change to _open.
  # So until there is a specific plan to tackle all of these type of warnings it's best to turn them off to the distraction.
  set_c_cxx_flag( "/wd4996" )

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
    try_linker_flag( LINKER_LARGE_ADDRESS_AWARE "-Wl,--large-address-aware" )
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
  set( CMAKE_INSTALL_RPATH "@executable_path/;${DEPS_DIR};${DEPS_DIR}/lib" )
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
# Git revision info
################################################################################

if( EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git" )
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
    if( CMAKE_COMPILER_IS_GNUCXX OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" )
      add_custom_command( OUTPUT "${OBJ_DIR}/${Target}.h.gch"
        COMMAND ${CMAKE_CXX_COMPILER} ${Defs} ${Flags} -x c++-header ${Header} -o "${OBJ_DIR}/${Target}.h.gch"
        DEPENDS ${Header}
        IMPLICIT_DEPENDS CXX ${Header}
      )

      add_custom_target( ${Target}-pch DEPENDS "${OBJ_DIR}/${Target}.h.gch" )
      add_dependencies( ${Target} ${Target}-pch )

      if( ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" )
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

# Import external dependencies
if( DEPS_DIR )
  # Download them if they not available
  if( NOT EXISTS ${DEPS_DIR} )
    get_filename_component( BASENAME ${DEPS_DIR} NAME )
    set( REMOTE "http://dl.unvanquished.net/deps/${BASENAME}${DEPS_EXT}" )
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
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/external_deps RESULT_VARIABLE EXTRACT_RESULT )
    if( NOT EXTRACT_RESULT EQUAL 0 )
      message( FATAL_ERROR "Could not extract ${BASENAME}${DEPS_EXT}" )
    endif()
  endif()

  # Add to paths
  set( CMAKE_FIND_ROOT_PATH ${DEPS_DIR} ${CMAKE_FIND_ROOT_PATH} )
  set( CMAKE_INCLUDE_PATH ${DEPS_DIR} ${DEPS_DIR}/include ${CMAKE_INCLUDE_PATH} )
  set( CMAKE_FRAMEWORK_PATH ${DEPS_DIR} ${CMAKE_FRAMEWORK_PATH} )
  set( CMAKE_LIBRARY_PATH ${DEPS_DIR}/lib ${CMAKE_LIBRARY_PATH} )
endif()

# These variables are defined:
# LIBS_BASE
# LIBSRC_BASE, LIBSRC_NACL

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

# Minizip
set( LIBSRC_BASE ${LIBSRC_BASE} ${MINIZIPLIST} )

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

if( NACL_RUNTIME_PATH )
  add_definitions( "-DNACL_RUNTIME_PATH=${NACL_RUNTIME_PATH}" )
endif()

# zlib
find_package( ZLIB REQUIRED )
include_directories( ${ZLIB_INCLUDE_DIRS} )
set( LIBS_BASE ${LIBS_BASE} ${ZLIB_LIBRARIES} )

# Builds the libs for which we include the source
# Set by `add_subdirectory( daemon )`
# add_library( base-source-libs EXCLUDE_FROM_ALL ${LIBSRC_BASE} )
# add_library( nacl-source-libs EXCLUDE_FROM_ALL ${LIBSRC_NACL} )
# set_target_properties( base-source-libs nacl-source-libs PROPERTIES POSITION_INDEPENDENT_CODE 1 )

set( LIBS_BASE base-source-libs ${LIBS_BASE} )

################################################################################
# Game logic
################################################################################

# Function to setup all the Game/Cgame/UI libraries
include( CMakeParseArguments )
function( GAMEMODULE )
  # ParseArguments setup
  set( oneValueArgs NAME )
  set( multiValueArgs COMPILE_DEF FILES )
  cmake_parse_arguments( GAMEMODULE "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

  if( BUILD_GAME_NATIVE_DLL )
    add_library( ${GAMEMODULE_NAME}-native-dll MODULE ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST} )
    target_link_libraries( ${GAMEMODULE_NAME}-native-dll nacl-source-libs ${LIBS_BASE} )
    set_target_properties( ${GAMEMODULE_NAME}-native-dll PROPERTIES
      PREFIX ""
      COMPILE_DEFINITIONS "${GAMEMODULE_COMPILE_DEF};BUILD_VM_IN_PROCESS"
    )
    ADD_PRECOMPILED_HEADER( ${GAMEMODULE_NAME}-native-dll ${COMMON_DIR}/Common.h )
  endif()

  if( BUILD_GAME_NATIVE_EXE )
    add_executable( ${GAMEMODULE_NAME}-native-exe ${PCH_FILE} ${GAMEMODULE_FILES} ${SHAREDLIST} )
    target_link_libraries( ${GAMEMODULE_NAME}-native-exe nacl-source-libs ${LIBS_BASE} )
    set_target_properties( ${GAMEMODULE_NAME}-native-exe PROPERTIES
      COMPILE_DEFINITIONS "${GAMEMODULE_COMPILE_DEF}"
    )
    ADD_PRECOMPILED_HEADER( ${GAMEMODULE_NAME}-native-exe ${COMMON_DIR}/Common.h )
  endif()

  if( BUILD_GAME_NACL )
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
    foreach( FILE ${GAMEMODULE_FILES} ${SHAREDLIST} ${NACLLIST_MODULE} ${ZLIBLIST} ${LIBSRC_BASE} )
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
        ${OBJECTS} -lnacl_exception
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

endfunction()

GAMEMODULE( NAME sgame
  COMPILE_DEF BUILD_VM BUILD_SGAME
  FILES ${SGAMELIST}
)

GAMEMODULE( NAME cgame
  COMPILE_DEF BUILD_VM BUILD_CGAME
  FILES ${CGAMELIST}
)
