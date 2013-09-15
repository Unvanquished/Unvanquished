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

set( CMAKE_BUILD_TYPE Release CACHE STRING "Build configuration (Debug, Release, RelWithDebInfo, MinSizeRel)" )

# convenience option for source distros
option( MINIMAL_FLAGS "try not to mess up system flags" 0)

project( Daemon C CXX ASM )

set( X86 0 )
if( WIN32 OR MSVC OR CMAKE_SYSTEM_PROCESSOR MATCHES "i.86" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" )
  set( X86 1 )
endif()

set( MOUNT_DIR ${CMAKE_SOURCE_DIR}/src )
set( OBJ_DIR ${CMAKE_BINARY_DIR}/CMakeFiles )

# Check if this is first run and if so set some default flags
if( NOT DEFINED SUBSEQUENT_RUN )
  set( SUBSEQUENT_RUN 1 CACHE INTERNAL "True if not first run" )

  if( MSVC )
    set( CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} /arch:SSE /fp:fast /Oy" CACHE STRING "Flags used by the compiler during release builds" FORCE )
  else()
    if(NOT MINIMAL_FLAGS)
      # "gcc -msse2" fails on non-x86
      if( X86 )
        set( CMAKE_USE_SSE2 "-msse2" )
      else()
        set( CMAKE_USE_SSE2 "" )
      endif()

      macro( try_compiler_option COMPILER_ID COMPILER_VERSION PROP FLAG_TRUE FLAG_FALSE )
        if( COMPILER_ID STREQUAL "" OR ( CMAKE_C_COMPILER_ID STREQUAL "${COMPILER_ID}" AND NOT ( CMAKE_C_COMPILER_VERSION VERSION_LESS "${COMPILER_VERSION}" ) ) )
          set( SAVED_FLAGS "${CMAKE_C_FLAGS}" )
          set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${FLAG_TRUE}" )
          try_compile( ${PROP}_TEST ${MOUNT_DIR}/tests ${MOUNT_DIR}/tests/test.c
                       CMAKE_FLAGS "-DCMAKE_C_FLAGS=${CMAKE_C_FLAGS}" )
          set( CMAKE_C_FLAGS "${SAVED_FLAGS}" )
          if( ${PROP}_TEST )
            message( STATUS "Using flag ${FLAG_TRUE}" )
            set( ${PROP} ${FLAG_TRUE} )
          else()
            set( ${PROP} ${FLAG_FALSE} )
          endif()
        else()
          set( ${PROP} ${FLAG_FALSE} )
        endif()
      endmacro()

      try_compiler_option( GNU   0.0 DEBUG_SYMBOLS_LEVEL -g3 -g  ) # gcc: macro expansion etc. in gdb
      try_compiler_option( GNU   4.8 DEBUG_OPTIMISE      -Og ""  ) # gcc 4.8: optimisation level for debug builds

      set( CMAKE_C_FLAGS "${CMAKE_USE_SSE2} -pipe" CACHE STRING "Flags used by the compiler during all build types" FORCE )
      set( CMAKE_C_FLAGS_DEBUG "${DEBUG_OPTIMISE} ${DEBUG_SYMBOLS_LEVEL}" CACHE STRING "Flags used by the compiler during debug builds." FORCE )
      set( CMAKE_C_FLAGS_MINSIZEREL "-DNDEBUG -Os" CACHE STRING "Flags used by the compiler during release minsize builds." FORCE )
      set( CMAKE_C_FLAGS_RELEASE "-DNDEBUG -O3 -ffast-math -fomit-frame-pointer -fno-strict-aliasing" CACHE STRING "Flags used by the compiler during release builds" FORCE )
      set( CMAKE_C_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELEASE} -g" CACHE STRING "Flags used by the compiler during Release with Debug Info builds." FORCE )
    else(NOT MINIMAL_FLAGS)
      set( CMAKE_C_FLAGS_DEBUG "" )
      set( CMAKE_C_FLAGS_RELEASE "-DNDEBUG" )
    endif(NOT MINIMAL_FLAGS)
  endif()

  set( CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS}" CACHE STRING "Flags used by the compiler during all build types" FORCE )
  set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG}" CACHE STRING "Flags used by the compiler during debug builds." FORCE )
  set( CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_C_FLAGS_MINSIZEREL}" CACHE STRING "Flags used by the compiler during release minsize builds." FORCE )
  set( CMAKE_CXX_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}" CACHE STRING "Flags used by the compiler during release builds" FORCE )
  set( CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_C_FLAGS_RELWITHDEBINFO}" CACHE STRING "Flags used by the compiler during Release with Debug Info builds." FORCE )
endif()

set( CMAKE_ASM_COMPILER ${CMAKE_C_COMPILER} )
set( CMAKE_ASM_COMPILE_OBJECT ${CMAKE_C_COMPILE_OBJECT} )
set( CMAKE_SHARED_LIBRARY_LINK_CXX_FLAGS "" ) # Need this to get rid of -rdynamic linker flag set by default by cmake
set( CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake" )


# Options
option( BUILD_CLIENT         "Build Daemon client"                         1 )
option( BUILD_SERVER         "Build Daemonded server"                      1 )
option( GAME_LIB             "Build game logic libraries"                  1 )
option( GAME_QVM             "Build game logic QVM files"                  1 )
option( BUILD_DAEMONMAP      "Build Mapping tool"                          0 )
option( BUILD_TTY_CLIENT     "Build Daemon headless client"                0 )

option( ENABLE_W_ALL         "Use -Wall"                                   0 )
option( ENABLE_WARNINGS      "Enable security & code-checking warning options" 1 )
option( ENABLE_HARDENING     "Enable security hardening options"           1 )

option( USE_GIT_DESCRIBE     "Use the revision information reported by Git's describe function" 1 )
option( ENABLE_BUNDLING      "When packaging automatically include all non-system libraries in package" 0 )

cmake_dependent_option( BUILD_OLD_RENDERER   "Build old renderer library"                            1 BUILD_CLIENT                   0 )
cmake_dependent_option( BUILD_NEW_RENDERER   "Build new renderer library"                            1 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_SMP              "Puts the renderer backend in another thread"           1 BUILD_NEW_RENDERER             0 )
cmake_dependent_option( USE_OPENAL           "Use OpenAL"                                            0 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_CODEC_VORBIS     "Compile with support for Vorbis"                       1 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_MUMBLE           "Compile with support for Mumble"                       1 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_VOIP             "Compile with VOIP support"                             1 "BUILD_CLIENT OR BUILD_SERVER" 0 )
cmake_dependent_option( USE_CIN_XVID         "Encode videos with XViD"                               1 BUILD_CLIENT                   0 )
cmake_dependent_option( USE_INTERNAL_SDL     "Use SDL files included with Unvanquished"              0 BUILD_CLIENT                   0 )

cmake_dependent_option( USE_CURSES           "Use the curses console"                                1 "BUILD_CLIENT OR BUILD_SERVER" 0 )
cmake_dependent_option( USE_PHYSICS          "Compile with Newton Physics"                           0 "BUILD_CLIENT OR BUILD_SERVER" 0 )
cmake_dependent_option( USE_INTERNAL_CRYPTO  "Use internal crypto (nettle)"                          0 "BUILD_CLIENT OR BUILD_SERVER" 0 )

cmake_dependent_option( USE_INTERNAL_JPEG    "Use JPEG files included with Unvanquished"             0
                                                                        "BUILD_OLD_RENDERER OR BUILD_NEW_RENDERER OR BUILD_DAEMONMAP" 0 )

cmake_dependent_option( USE_SDL_ICONV        "Compile with SDL's iconv implementation"               1 BUILD_CLIENT                   0 )

cmake_dependent_option( HAVE_BZIP2           "Compile with BZip2 support"                            0 "HAVE_BZIP2"                   0 )

cmake_dependent_option( BUILD_GAME_GPP       "Build Tremulous GPP game logic"              1 "GAME_LIB OR GAME_QVM" 0 )
cmake_dependent_option( DEBUG_VM_ALLOC       "Debug some allocations (VM as native code)"  0 GAME_LIB         0 )
cmake_dependent_option( GAME_LIB_LLVM        "Use LLVM, not shared libs, for game logic"   0 GAME_LIB         0 )

cmake_dependent_option( USE_OPENAL_DLOPEN    "Use dlopen to load OpenAL"                   1 USE_OPENAL       0 )
cmake_dependent_option( USE_CIN_THEORA       "Compile with support for Theora"             1 USE_CODEC_VORBIS 0 )
cmake_dependent_option( USE_INTERNAL_SPEEX   "Use internal speex"                          0 USE_VOIP         0 )
cmake_dependent_option( USE_INTERNAL_GLEW    "Use internal GLEW"                           0 BUILD_CLIENT     0 )
cmake_dependent_option( USE_INTERNAL_OPUS    "Use internal Opus"                           1 BUILD_CLIENT     0 )

cmake_dependent_option( USE_INTERNAL_WEBP    "Use internal WebP"                           0 BUILD_CLIENT     0 )

cmake_dependent_option( USE_GEOIP            "Use libgeoip"                                1 "BUILD_SERVER OR BUILD_CLIENT OR BUILD_TTY_CLIENT" 0 )

cmake_dependent_option( ENABLE_W_EXTRA       "Use -Wextra"                                 0 ENABLE_W_ALL     0 )

cmake_dependent_option( ENABLE_FSTACKPROT    "Enable stack protection if available"        1 ENABLE_HARDENING 0 )
cmake_dependent_option( ENABLE_RELRO         "Enable relro & bindnow if available"         1 ENABLE_HARDENING 0 )

mark_as_advanced( GAME_LIB_LLVM LLVM_CONFIG ENABLE_FSTACKPROT ENABLE_RELRO USE_GIT_DESCRIBE ENABLE_BUNDLING )

if( GAME_LIB_LLVM )
  find_program( LLVM_CONFIG llvm-config )
  execute_process( COMMAND ${LLVM_CONFIG} --libfiles
                   OUTPUT_VARIABLE LLVM_LIBS OUTPUT_STRIP_TRAILING_WHITESPACE )
  execute_process( COMMAND ${LLVM_CONFIG} --includedir OUTPUT_VARIABLE LLVM_INCLUDE_DIR OUTPUT_STRIP_TRAILING_WHITESPACE )
  execute_process( COMMAND ${LLVM_CONFIG} --version OUTPUT_VARIABLE LLVM_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE )
  execute_process( COMMAND ${LLVM_CONFIG} --libdir OUTPUT_VARIABLE LLVM_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE )
  separate_arguments( LLVM_LIBS )

  include( ${CMAKE_SOURCE_DIR}/cmake/CMakeLLVMInformation.cmake )
  include( ${CMAKE_SOURCE_DIR}/cmake/CMakeLLVMXXInformation.cmake )
  file( WRITE ${OBJ_DIR}/CMakeLLVMCompiler.cmake )
  file( WRITE ${OBJ_DIR}/CMakeLLVMXXCompiler.cmake )
  enable_language( LLVM )
  enable_language( LLVMXX )
  message( STATUS "Using LLVM" )
endif()

set( GPP_DIR ${MOUNT_DIR}/gamelogic )
set( LIB_DIR ${MOUNT_DIR}/libs ) # LIB_DIR is also defined in Visual_Studio32/64.cmake
set( SDL_DIR ${LIB_DIR}/libsdl2 )
set( JPEG_DIR ${LIB_DIR}/jpeg )
set( GLEW_DIR ${LIB_DIR}/glew )
set( WEBP_DIR ${LIB_DIR}/libwebp )

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
  elseif( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    set( BUILD_ARCH "x86_64" )
  elseif( CMAKE_SYSTEM_PROCESSOR STREQUAL "x86_64" AND CMAKE_SIZEOF_VOID_P EQUAL 4 )
    set( BUILD_ARCH "x32" )
  else()
    set( BUILD_ARCH ${CMAKE_SYSTEM_PROCESSOR} )
  endif()
endif()

if( USE_GIT_DESCRIBE )
  # Report the Git commit identification, if possible

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
  set( GIT_DESCRIBE ${GIT_DESCRIBE_REPORT} CACHE INTERNAL "commit id reported by Git" FORCE )
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

# Import CMakeLists.txt for internal libraries
if( USE_INTERNAL_SDL )
  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_BINARY_DIR}/Release/${GAMELIB_OUTPUT_DIR}" )
  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG "${CMAKE_BINARY_DIR}/Debug/${GAMELIB_OUTPUT_DIR}" )
  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${CMAKE_BINARY_DIR}/MinSizeRel/${GAMELIB_OUTPUT_DIR}" )
  set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${CMAKE_BINARY_DIR}/RelWithDebInfo/${GAMELIB_OUTPUT_DIR}" )
  add_subdirectory( ${SDL_DIR} ${OBJ_DIR}/libsdl2 )
  set( SDL_LIBRARY SDL2 SDL2main CACHE STRING "SDL library" FORCE )
  set( SDL_INCLUDE_DIR ${SDL_DIR}/include CACHE STRING "SDL include directory" FORCE )
endif()

if( USE_INTERNAL_JPEG )
  add_subdirectory( ${JPEG_DIR} ${OBJ_DIR}/libjpeg )
  set( JPEG_LIBRARY INTERNAL_JPEG CACHE STRING "JPEG library" FORCE )
  set( JPEG_INCLUDE_DIR ${JPEG_DIR} CACHE STRING "JPEG include directory" FORCE )
endif()

if( USE_INTERNAL_GLEW )
  add_subdirectory( ${GLEW_DIR} ${OBJ_DIR}/libglew )
  set( GLEW_LIBRARY INTERNAL_GLEW CACHE STRING "Glew library" FORCE )
  set( GLEW_INCLUDE_DIR ${GLEW_DIR}/include CACHE STRING "Glew include directory" FORCE )
endif()

if( USE_INTERNAL_WEBP )
  add_subdirectory( ${WEBP_DIR} ${OBJ_DIR}/libwebp )
  set( WEBP_LIBRARY INTERNAL_WEBP CACHE STRING "WebP library" FORCE )
  set( WEBP_INCLUDE_DIR ${WEBP_DIR} CACHE STRING "WebP include directory" FORCE )
endif()

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

if(NOT MINIMAL_FLAGS)
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
    try_compiler_flag_internal( ${PROP} ${FLAG} CXX cxx C++ )
  endmacro()

  macro( try_llvm_flag PROP FLAG )
    try_compiler_flag_internal( ${PROP} ${FLAG} LLVM cc C/LLVM )
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
  if( ENABLE_W_EXTRA )
    try_c_cxx_llvm_flag( HAVE_W_EXTRA            -Wextra )
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
      try_c_cxx_llvm_flag( HAVE_W_ERR_IMPLICIT_FN -Werror=implicit-function-declaration )
    endif()
    try_c_cxx_llvm_flag( HAVE_W_FMT_SECURITY     -Wformat-security )
    if( HAVE_W_FMT_SECURITY_C )
      try_c_cxx_llvm_flag( HAVE_W_ERR_FMT_SECURITY -Werror=format-security )
    endif()
    try_c_llvm_flag( HAVE_W_DECL_AFT_STMT        -Wdeclaration-after-statement )
    if( HAVE_W_DECL_AFT_STMT_C )
      try_c_llvm_flag( HAVE_W_ERR_DECL_AFT_STMT  -Werror=declaration-after-statement )
    endif()
  endif()

  # Hardening
  if( ENABLE_HARDENING )
    try_c_cxx_flag( HAVE_F_STACK_PROTECTOR -fstack-protector )
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

  if( NOT MSVC )
    # Can we get rid of unneeded external libraries?
    try_linker_flag( HAVE_AS_NEEDED -Wl,--as-needed )

    # If we can, fail if there are unresolved symbols
    try_linker_flag( HAVE_Z_DEFS -Wl,-z,defs )
  endif()

  if( ENABLE_FSTACKPROT )
    try_linker_flag( HAVE_FSTACKPROT "-fstack-protector --param=ssp-buffer-size=4" )
  endif()
  if( ENABLE_RELRO )
    try_linker_flag( HAVE_Z_RELRO_Z_NOW -Wl,-z,relro,-z,now )
  endif()
endif(NOT MINIMAL_FLAGS)

######################
# Define Build Files #
######################

set( SHAREDLIST_C
  ${MOUNT_DIR}/engine/qcommon/q_shared.c
  ${MOUNT_DIR}/engine/qcommon/q_unicode.c
  ${MOUNT_DIR}/engine/qcommon/q_math.c
)

set ( SHAREDLIST_H
  ${MOUNT_DIR}/engine/qcommon/q_shared.h
  ${MOUNT_DIR}/engine/qcommon/q_platform.h
  ${MOUNT_DIR}/engine/qcommon/q_unicode.h
  ${MOUNT_DIR}/engine/qcommon/unicode_data.h
  ${MOUNT_DIR}/engine/qcommon/logging.h
)

set ( SHAREDLIST
  ${SHAREDLIST_C}
  ${SHAREDLIST_H}
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
  ${MOUNT_DIR}/engine/renderer/tr_local.h
  ${MOUNT_DIR}/engine/renderer/tr_types.h
  ${MOUNT_DIR}/engine/renderer/tr_public.h
  ${MOUNT_DIR}/engine/renderer/tr_bonematrix.h
  ${MOUNT_DIR}/engine/renderer/tr_bonematrix.c
  ${MOUNT_DIR}/engine/renderer/anorms256.h
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
  ${MOUNT_DIR}/engine/renderer/tr_font.c
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
  ${MOUNT_DIR}/engine/qcommon/md4.c
  ${MOUNT_DIR}/engine/rendererGL/tr_local.h
  ${MOUNT_DIR}/engine/renderer/tr_public.h
  ${MOUNT_DIR}/engine/renderer/tr_bonematrix.h
  ${MOUNT_DIR}/engine/renderer/tr_bonematrix.c
  ${MOUNT_DIR}/engine/rendererGL/gl_shader.h
  ${MOUNT_DIR}/engine/rendererGL/tr_image.h
  ${MOUNT_DIR}/engine/rendererGL/tr_model_skel.h
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
  ${MOUNT_DIR}/engine/server/server.h
  ${MOUNT_DIR}/engine/server/g_api.h
  ${MOUNT_DIR}/engine/qcommon/print_translated.h
)

set( QCOMMON_ARCH "" )
if( BUILD_ARCH STREQUAL "i386" OR BUILD_ARCH STREQUAL "x32" OR BUILD_ARCH STREQUAL "x86_64" )
  set( QCOMMON_ARCH ${MOUNT_DIR}/engine/qcommon/vm_x86.c )
#elseif( ... )
endif()

set( QCOMMONLIST
  ${MOUNT_DIR}/engine/qcommon/cm_load.c
  ${MOUNT_DIR}/engine/qcommon/cm_trisoup.c
  ${MOUNT_DIR}/engine/qcommon/cm_patch.c
  ${MOUNT_DIR}/engine/qcommon/cm_plane.c
  ${MOUNT_DIR}/engine/qcommon/cm_polylib.c
  ${MOUNT_DIR}/engine/qcommon/cm_test.c
  ${MOUNT_DIR}/engine/qcommon/cm_trace.c
  ${MOUNT_DIR}/engine/qcommon/cm_local.h
  ${MOUNT_DIR}/engine/qcommon/cm_patch.h
  ${MOUNT_DIR}/engine/qcommon/cm_polylib.h
  ${MOUNT_DIR}/engine/qcommon/cm_public.h
  ${MOUNT_DIR}/engine/qcommon/cmd.c
  ${MOUNT_DIR}/engine/qcommon/common.c
  ${MOUNT_DIR}/engine/qcommon/crypto.c
  ${MOUNT_DIR}/engine/qcommon/crypto.h
  ${MOUNT_DIR}/engine/qcommon/cvar.c
  ${MOUNT_DIR}/engine/qcommon/cvar.h
  ${MOUNT_DIR}/engine/qcommon/files.c
  ${MOUNT_DIR}/engine/qcommon/qfiles.h
  ${MOUNT_DIR}/engine/qcommon/htable.c
  ${MOUNT_DIR}/engine/qcommon/htable.h
  ${MOUNT_DIR}/engine/qcommon/huffman.c
  ${MOUNT_DIR}/engine/qcommon/md4.c
  ${MOUNT_DIR}/engine/qcommon/md4.h
  ${MOUNT_DIR}/engine/qcommon/md5.c
  ${MOUNT_DIR}/engine/qcommon/msg.c
  ${MOUNT_DIR}/engine/qcommon/net_chan.c
  ${MOUNT_DIR}/engine/qcommon/net_ip.c
  ${MOUNT_DIR}/engine/qcommon/parse.c
  ${MOUNT_DIR}/engine/qcommon/translation.cpp
  ${MOUNT_DIR}/engine/qcommon/unzip.c
  ${MOUNT_DIR}/engine/qcommon/unzip.h
  ${MOUNT_DIR}/engine/qcommon/ioapi.c
  ${MOUNT_DIR}/engine/qcommon/vm.c
  ${MOUNT_DIR}/engine/qcommon/vm_local.h
  ${MOUNT_DIR}/engine/qcommon/vm_traps.h
  ${MOUNT_DIR}/engine/qcommon/vm_interpreted.c
  ${MOUNT_DIR}/engine/qcommon/qcommon.h
  ${MOUNT_DIR}/engine/qcommon/surfaceflags.h
  ${QCOMMON_ARCH}
)

# Visual studio can't handle inline assembly
if( MSVC )
  if( ${BUILD_ARCH} STREQUAL "i386" )
    set( ASM_APP ml /safeseh)
  else()
    set( ASM_APP ml64 -D idx64=1 /safeseh )
  endif()
  set( QCOMMONLIST ${QCOMMONLIST} ftola.obj snapvector.obj )
  # Work around - bugs in cmake prevent adding asm files to target in VS projects
  add_custom_command( OUTPUT ftola.obj COMMAND ${ASM_APP} /c ARGS "${MOUNT_DIR}/engine/asm/ftola.asm" MAIN_DEPENDENCY ${MOUNT_DIR}/engine/asm/ftola.asm )
  add_custom_command( OUTPUT snapvector.obj COMMAND ${ASM_APP} /c ARGS "${MOUNT_DIR}/engine/asm/snapvector.asm" MAIN_DEPENDENCY ${MOUNT_DIR}/engine/asm/snapvector.asm )
  if( BUILD_ARCH STREQUAL "x86_64" )
    set( QCOMMONLIST ${QCOMMONLIST} vm_x86_64.obj )
    add_custom_command( OUTPUT vm_x86_64.obj COMMAND ${ASM_APP} /c ARGS "${MOUNT_DIR}/engine/asm/vm_x86_64.asm" MAIN_DEPENDENCY ${MOUNT_DIR}/engine/asm/vm_x86_64.asm )
  endif()
elseif( BUILD_ARCH STREQUAL "i386" OR BUILD_ARCH STREQUAL "x32" OR BUILD_ARCH STREQUAL "x86_64" )
    set( QCOMMONLIST ${QCOMMONLIST}
    ${MOUNT_DIR}/engine/asm/ftola.c
    ${MOUNT_DIR}/engine/asm/snapvector.c
  )
endif()

if( GAME_LIB_LLVM )
  set( QCOMMONLIST ${QCOMMONLIST} ${MOUNT_DIR}/engine/qcommon/vm_llvm.cpp ${MOUNT_DIR}/engine/qcommon/vm_llvm.h )
endif()

if( USE_PHYSICS )
  set( QCOMMONLIST ${QCOMMONLIST} ${MOUNT_DIR}/engine/qcommon/cm_physics.cpp )
endif()

set( CLIENTLIST
  ${MOUNT_DIR}/engine/client/cg_api.h
  ${MOUNT_DIR}/engine/client/ui_api.h
  ${MOUNT_DIR}/engine/client/client.h
  ${MOUNT_DIR}/engine/client/cin_ogm.c
  ${MOUNT_DIR}/engine/client/cl_avi.c
  ${MOUNT_DIR}/engine/client/cl_cgame.c
  ${MOUNT_DIR}/engine/client/cl_cin.c
  ${MOUNT_DIR}/engine/client/cl_console.c
  ${MOUNT_DIR}/engine/client/cl_input.c
  ${MOUNT_DIR}/engine/client/cl_irc.c
  ${MOUNT_DIR}/engine/client/cl_keys.c
  ${MOUNT_DIR}/engine/client/keycodes.h
  ${MOUNT_DIR}/engine/client/keys.h
  ${MOUNT_DIR}/engine/client/cl_main.c
  ${MOUNT_DIR}/engine/client/cl_net_chan.c
  ${MOUNT_DIR}/engine/client/cl_parse.c
  ${MOUNT_DIR}/engine/client/cl_scrn.c
  ${MOUNT_DIR}/engine/client/cl_ui.c
  ${MOUNT_DIR}/engine/client/cl_logs.c
  ${MOUNT_DIR}/engine/client/snd_adpcm.c
  ${MOUNT_DIR}/engine/client/snd_codec.c
  ${MOUNT_DIR}/engine/client/snd_codec.h
  ${MOUNT_DIR}/engine/client/snd_local.h
  ${MOUNT_DIR}/engine/client/snd_public.h
  ${MOUNT_DIR}/engine/client/snd_codec_ogg.c
  ${MOUNT_DIR}/engine/client/snd_codec_wav.c
  ${MOUNT_DIR}/engine/client/snd_codec_opus.c
  ${MOUNT_DIR}/engine/client/snd_dma.c
  ${MOUNT_DIR}/engine/client/snd_openal.c
  ${MOUNT_DIR}/engine/client/snd_main.c
  ${MOUNT_DIR}/engine/client/snd_mem.c
  ${MOUNT_DIR}/engine/client/snd_mix.c
  ${MOUNT_DIR}/engine/client/snd_wavelet.c
  ${MOUNT_DIR}/engine/client/qal.c
  ${MOUNT_DIR}/engine/client/qal.h
)

if( USE_MUMBLE )
  set( CLIENTLIST ${CLIENTLIST} ${MOUNT_DIR}/engine/client/libmumblelink.c ${MOUNT_DIR}/engine/client/libmumblelink.h )
endif()

if( ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" )
  set( SDLBASELIST
    ${MOUNT_DIR}/engine/sys/sys_unix.c
    ${MOUNT_DIR}/engine/sys/sys_osx.m
    ${MOUNT_DIR}/engine/sys/sys_main.c
    ${MOUNT_DIR}/engine/sys/con_log.c
    ${MOUNT_DIR}/engine/sys/con_${CONSOLE}.c
	${MOUNT_DIR}/engine/sys/sys_local.h
  )
else()
  set( SDLBASELIST
    ${MOUNT_DIR}/engine/sys/sys_${PLATFORM}.c
    ${MOUNT_DIR}/engine/sys/sys_main.c
    ${MOUNT_DIR}/engine/sys/con_log.c
    ${MOUNT_DIR}/engine/sys/con_${CONSOLE}.c
	${MOUNT_DIR}/engine/sys/sys_local.h
  )
endif()

if ( WINDOWS )
	set( SDLBASELIST ${SDLBASELIST} ${MOUNT_DIR}/engine/sys/sys_win32.h )
endif()

# Append Visual Studio specific files to SDLBASELIST
if( MSVC )
  set( SDLBASELIST ${SDLBASELIST} ${MOUNT_DIR}/engine/sys/sys_sysinfo.c )
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
  ${GPP_DIR}/cgame/cg_main.c
  ${GPP_DIR}/game/bg_misc.c
  ${GPP_DIR}/game/bg_parse.c
  ${GPP_DIR}/game/bg_pmove.c
  ${GPP_DIR}/game/bg_slidemove.c
  ${GPP_DIR}/game/bg_lib.c
  ${GPP_DIR}/game/bg_alloc.c
  ${GPP_DIR}/game/bg_voice.c
  ${GPP_DIR}/cgame/cg_consolecmds.c
  ${GPP_DIR}/cgame/cg_buildable.c
  ${GPP_DIR}/cgame/cg_animation.c
  ${GPP_DIR}/cgame/cg_animmapobj.c
  ${GPP_DIR}/cgame/cg_draw.c
  ${GPP_DIR}/cgame/cg_drawtools.c
  ${GPP_DIR}/cgame/cg_ents.c
  ${GPP_DIR}/cgame/cg_event.c
  ${GPP_DIR}/cgame/cg_marks.c
  ${GPP_DIR}/cgame/cg_minimap.c
  ${GPP_DIR}/cgame/cg_players.c
  ${GPP_DIR}/cgame/cg_playerstate.c
  ${GPP_DIR}/cgame/cg_predict.c
  ${GPP_DIR}/cgame/cg_servercmds.c
  ${GPP_DIR}/cgame/cg_snapshot.c
  ${GPP_DIR}/cgame/cg_view.c
  ${GPP_DIR}/cgame/cg_weapons.c
  ${GPP_DIR}/cgame/cg_scanner.c
  ${GPP_DIR}/cgame/cg_attachment.c
  ${GPP_DIR}/cgame/cg_trails.c
  ${GPP_DIR}/cgame/cg_particles.c
  ${GPP_DIR}/cgame/cg_tutorial.c
  ${GPP_DIR}/cgame/cg_utils.c
  ${GPP_DIR}/ui/ui_shared.c
  ${GPP_DIR}/ui/ui_utf8.c
  ${SHAREDLIST_C}
)

set( CGAMEGPP_SO
  ${CGAMEGPP_BASE}
  ${SHAREDLIST_H}
  ${GPP_DIR}/cgame/cg_api.c
  ${GPP_DIR}/cgame/cgame.def
  ${MOUNT_DIR}/engine/client/cg_api.h
  ${MOUNT_DIR}/engine/qcommon/print_translated.h
)

set( CGAMEGPP_QVM
  ${GPP_DIR}/cgame/cg_api.asm
)

set( GAMEGPP_BASE
  ${GPP_DIR}/game/g_main.c
  ${GPP_DIR}/game/bg_misc.c
  ${GPP_DIR}/game/bg_parse.c
  ${GPP_DIR}/game/bg_pmove.c
  ${GPP_DIR}/game/bg_slidemove.c
  ${GPP_DIR}/game/bg_lib.c
  ${GPP_DIR}/game/bg_alloc.c
  ${GPP_DIR}/game/bg_voice.c
  ${GPP_DIR}/game/g_active.c
  ${GPP_DIR}/game/g_client.c
  ${GPP_DIR}/game/g_cmds.c
  ${GPP_DIR}/game/g_combat.c
  ${GPP_DIR}/game/g_entities.c
  ${GPP_DIR}/game/g_physics.c
  ${GPP_DIR}/game/g_buildable.c
  ${GPP_DIR}/game/g_missile.c
  ${GPP_DIR}/game/g_session.c
  ${GPP_DIR}/game/g_spawn_afx.c
  ${GPP_DIR}/game/g_spawn_ctrl.c
  ${GPP_DIR}/game/g_spawn_fx.c
  ${GPP_DIR}/game/g_spawn_game.c
  ${GPP_DIR}/game/g_spawn_generic.c
  ${GPP_DIR}/game/g_spawn_gfx.c
  ${GPP_DIR}/game/g_spawn_mover.c
  ${GPP_DIR}/game/g_spawn_position.c
  ${GPP_DIR}/game/g_spawn_sensor.c
  ${GPP_DIR}/game/g_spawn_shared.c
  ${GPP_DIR}/game/g_spawn.c
  ${GPP_DIR}/game/g_svcmds.c
  ${GPP_DIR}/game/g_team.c
  ${GPP_DIR}/game/g_utils.c
  ${GPP_DIR}/game/g_maprotation.c
  ${GPP_DIR}/game/g_weapon.c
  ${GPP_DIR}/game/g_admin.c
  ${GPP_DIR}/game/g_namelog.c
  ${SHAREDLIST_C}
)

set( GAMEGPP_SO
  ${GAMEGPP_BASE}
  ${SHAREDLIST_H}
  ${GPP_DIR}/game/g_api.c
  ${GPP_DIR}/game/game.def
  ${MOUNT_DIR}/engine/server/g_api.h
)

set( GAMEGPP_QVM
  ${GPP_DIR}/game/g_api.asm
)

set( UIGPP_BASE
  ${GPP_DIR}/ui/ui_main.c
  ${GPP_DIR}/ui/ui_atoms.c
  ${GPP_DIR}/ui/ui_shared.c
  ${GPP_DIR}/ui/ui_gameinfo.c
  ${GPP_DIR}/game/bg_misc.c
  ${GPP_DIR}/game/bg_parse.c
  ${GPP_DIR}/game/bg_lib.c
  ${GPP_DIR}/game/bg_alloc.c
  ${GPP_DIR}/ui/ui_utf8.c
  ${SHAREDLIST_C}
)

set( UIGPP_SO
  ${UIGPP_BASE}
  ${SHAREDLIST_H}
  ${GPP_DIR}/ui/ui_api.c
  ${GPP_DIR}/ui/ui.def
  ${MOUNT_DIR}/engine/client/ui_api.h
)

set( UIGPP_QVM
  ${GPP_DIR}/ui/ui_api.asm
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
if ( USE_INTERNAL_OPUS )
		set( OPUSLIST
		${LIB_DIR}/opus-1.0.2/celt/_kiss_fft_guts.h
		${LIB_DIR}/opus-1.0.2/celt/arch.h
		${LIB_DIR}/opus-1.0.2/celt/bands.c
		${LIB_DIR}/opus-1.0.2/celt/bands.h
		${LIB_DIR}/opus-1.0.2/celt/celt.c
		${LIB_DIR}/opus-1.0.2/celt/celt.h
		${LIB_DIR}/opus-1.0.2/celt/celt_lpc.c
		${LIB_DIR}/opus-1.0.2/celt/celt_lpc.h
		${LIB_DIR}/opus-1.0.2/celt/cwrs.c
		${LIB_DIR}/opus-1.0.2/celt/cwrs.h
		${LIB_DIR}/opus-1.0.2/celt/ecintrin.h
		${LIB_DIR}/opus-1.0.2/celt/entcode.c
		${LIB_DIR}/opus-1.0.2/celt/entcode.h
		${LIB_DIR}/opus-1.0.2/celt/entdec.c
		${LIB_DIR}/opus-1.0.2/celt/entdec.h
		${LIB_DIR}/opus-1.0.2/celt/entenc.c
		${LIB_DIR}/opus-1.0.2/celt/entenc.h
		${LIB_DIR}/opus-1.0.2/celt/fixed_debug.h
		${LIB_DIR}/opus-1.0.2/celt/fixed_generic.h
		${LIB_DIR}/opus-1.0.2/celt/float_cast.h
		${LIB_DIR}/opus-1.0.2/celt/kiss_fft.c
		${LIB_DIR}/opus-1.0.2/celt/kiss_fft.h
		${LIB_DIR}/opus-1.0.2/celt/laplace.c
		${LIB_DIR}/opus-1.0.2/celt/laplace.h
		${LIB_DIR}/opus-1.0.2/celt/mathops.c
		${LIB_DIR}/opus-1.0.2/celt/mathops.h
		${LIB_DIR}/opus-1.0.2/celt/mdct.c
		${LIB_DIR}/opus-1.0.2/celt/mdct.h
		${LIB_DIR}/opus-1.0.2/celt/mfrngcod.h
		${LIB_DIR}/opus-1.0.2/celt/modes.c
		${LIB_DIR}/opus-1.0.2/celt/modes.h
		${LIB_DIR}/opus-1.0.2/celt/os_support.h
		${LIB_DIR}/opus-1.0.2/celt/pitch.c
		${LIB_DIR}/opus-1.0.2/celt/pitch.h
		${LIB_DIR}/opus-1.0.2/celt/quant_bands.c
		${LIB_DIR}/opus-1.0.2/celt/quant_bands.h
		${LIB_DIR}/opus-1.0.2/celt/rate.c
		${LIB_DIR}/opus-1.0.2/celt/rate.h
		${LIB_DIR}/opus-1.0.2/celt/stack_alloc.h
		${LIB_DIR}/opus-1.0.2/celt/static_modes_fixed.h
		${LIB_DIR}/opus-1.0.2/celt/static_modes_float.h
		${LIB_DIR}/opus-1.0.2/celt/vq.c
		${LIB_DIR}/opus-1.0.2/celt/vq.h
		${LIB_DIR}/opus-1.0.2/include/opus.h
		${LIB_DIR}/opus-1.0.2/include/opus_custom.h
		${LIB_DIR}/opus-1.0.2/include/opus_defines.h
		${LIB_DIR}/opus-1.0.2/include/opus_multistream.h
		${LIB_DIR}/opus-1.0.2/include/opus_types.h
		${LIB_DIR}/opus-1.0.2/silk/A2NLSF.c
		${LIB_DIR}/opus-1.0.2/silk/ana_filt_bank_1.c
		${LIB_DIR}/opus-1.0.2/silk/API.h
		${LIB_DIR}/opus-1.0.2/silk/biquad_alt.c
		${LIB_DIR}/opus-1.0.2/silk/bwexpander.c
		${LIB_DIR}/opus-1.0.2/silk/bwexpander_32.c
		${LIB_DIR}/opus-1.0.2/silk/check_control_input.c
		${LIB_DIR}/opus-1.0.2/silk/CNG.c
		${LIB_DIR}/opus-1.0.2/silk/code_signs.c
		${LIB_DIR}/opus-1.0.2/silk/control.h
		${LIB_DIR}/opus-1.0.2/silk/control_audio_bandwidth.c
		${LIB_DIR}/opus-1.0.2/silk/control_codec.c
		${LIB_DIR}/opus-1.0.2/silk/control_SNR.c
		${LIB_DIR}/opus-1.0.2/silk/debug.c
		${LIB_DIR}/opus-1.0.2/silk/debug.h
		${LIB_DIR}/opus-1.0.2/silk/dec_API.c
		${LIB_DIR}/opus-1.0.2/silk/decode_core.c
		${LIB_DIR}/opus-1.0.2/silk/decode_frame.c
		${LIB_DIR}/opus-1.0.2/silk/decode_indices.c
		${LIB_DIR}/opus-1.0.2/silk/decode_parameters.c
		${LIB_DIR}/opus-1.0.2/silk/decode_pitch.c
		${LIB_DIR}/opus-1.0.2/silk/decode_pulses.c
		${LIB_DIR}/opus-1.0.2/silk/decoder_set_fs.c
		${LIB_DIR}/opus-1.0.2/silk/define.h
		${LIB_DIR}/opus-1.0.2/silk/enc_API.c
		${LIB_DIR}/opus-1.0.2/silk/encode_indices.c
		${LIB_DIR}/opus-1.0.2/silk/encode_pulses.c
		${LIB_DIR}/opus-1.0.2/silk/errors.h
		${LIB_DIR}/opus-1.0.2/silk/float/apply_sine_window_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/autocorrelation_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/burg_modified_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/bwexpander_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/corrMatrix_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/encode_frame_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/energy_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/find_LPC_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/find_LTP_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/find_pitch_lags_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/find_pred_coefs_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/inner_product_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/k2a_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/levinsondurbin_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/LPC_analysis_filter_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/LPC_inv_pred_gain_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/LTP_analysis_filter_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/LTP_scale_ctrl_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/main_FLP.h
		${LIB_DIR}/opus-1.0.2/silk/float/noise_shape_analysis_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/pitch_analysis_core_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/prefilter_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/process_gains_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/regularize_correlations_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/residual_energy_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/scale_copy_vector_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/scale_vector_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/schur_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/SigProc_FLP.h
		${LIB_DIR}/opus-1.0.2/silk/float/solve_LS_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/sort_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/structs_FLP.h
		${LIB_DIR}/opus-1.0.2/silk/float/warped_autocorrelation_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/float/wrappers_FLP.c
		${LIB_DIR}/opus-1.0.2/silk/gain_quant.c
		${LIB_DIR}/opus-1.0.2/silk/HP_variable_cutoff.c
		${LIB_DIR}/opus-1.0.2/silk/init_decoder.c
		${LIB_DIR}/opus-1.0.2/silk/init_encoder.c
		${LIB_DIR}/opus-1.0.2/silk/Inlines.h
		${LIB_DIR}/opus-1.0.2/silk/inner_prod_aligned.c
		${LIB_DIR}/opus-1.0.2/silk/interpolate.c
		${LIB_DIR}/opus-1.0.2/silk/lin2log.c
		${LIB_DIR}/opus-1.0.2/silk/log2lin.c
		${LIB_DIR}/opus-1.0.2/silk/LP_variable_cutoff.c
		${LIB_DIR}/opus-1.0.2/silk/LPC_analysis_filter.c
		${LIB_DIR}/opus-1.0.2/silk/LPC_inv_pred_gain.c
		${LIB_DIR}/opus-1.0.2/silk/MacroCount.h
		${LIB_DIR}/opus-1.0.2/silk/MacroDebug.h
		${LIB_DIR}/opus-1.0.2/silk/macros.h
		${LIB_DIR}/opus-1.0.2/silk/main.h
		${LIB_DIR}/opus-1.0.2/silk/NLSF2A.c
		${LIB_DIR}/opus-1.0.2/silk/NLSF_decode.c
		${LIB_DIR}/opus-1.0.2/silk/NLSF_del_dec_quant.c
		${LIB_DIR}/opus-1.0.2/silk/NLSF_encode.c
		${LIB_DIR}/opus-1.0.2/silk/NLSF_stabilize.c
		${LIB_DIR}/opus-1.0.2/silk/NLSF_unpack.c
		${LIB_DIR}/opus-1.0.2/silk/NLSF_VQ.c
		${LIB_DIR}/opus-1.0.2/silk/NLSF_VQ_weights_laroia.c
		${LIB_DIR}/opus-1.0.2/silk/NSQ.c
		${LIB_DIR}/opus-1.0.2/silk/NSQ_del_dec.c
		${LIB_DIR}/opus-1.0.2/silk/pitch_est_defines.h
		${LIB_DIR}/opus-1.0.2/silk/pitch_est_tables.c
		${LIB_DIR}/opus-1.0.2/silk/PLC.c
		${LIB_DIR}/opus-1.0.2/silk/PLC.h
		${LIB_DIR}/opus-1.0.2/silk/process_NLSFs.c
		${LIB_DIR}/opus-1.0.2/silk/quant_LTP_gains.c
		${LIB_DIR}/opus-1.0.2/silk/resampler.c
		${LIB_DIR}/opus-1.0.2/silk/resampler_down2.c
		${LIB_DIR}/opus-1.0.2/silk/resampler_down2_3.c
		${LIB_DIR}/opus-1.0.2/silk/resampler_private.h
		${LIB_DIR}/opus-1.0.2/silk/resampler_private_AR2.c
		${LIB_DIR}/opus-1.0.2/silk/resampler_private_down_FIR.c
		${LIB_DIR}/opus-1.0.2/silk/resampler_private_IIR_FIR.c
		${LIB_DIR}/opus-1.0.2/silk/resampler_private_up2_HQ.c
		${LIB_DIR}/opus-1.0.2/silk/resampler_rom.c
		${LIB_DIR}/opus-1.0.2/silk/resampler_rom.h
		${LIB_DIR}/opus-1.0.2/silk/resampler_structs.h
		${LIB_DIR}/opus-1.0.2/silk/shell_coder.c
		${LIB_DIR}/opus-1.0.2/silk/sigm_Q15.c
		${LIB_DIR}/opus-1.0.2/silk/SigProc_FIX.h
		${LIB_DIR}/opus-1.0.2/silk/sort.c
		${LIB_DIR}/opus-1.0.2/silk/stereo_decode_pred.c
		${LIB_DIR}/opus-1.0.2/silk/stereo_encode_pred.c
		${LIB_DIR}/opus-1.0.2/silk/stereo_find_predictor.c
		${LIB_DIR}/opus-1.0.2/silk/stereo_LR_to_MS.c
		${LIB_DIR}/opus-1.0.2/silk/stereo_MS_to_LR.c
		${LIB_DIR}/opus-1.0.2/silk/stereo_quant_pred.c
		${LIB_DIR}/opus-1.0.2/silk/structs.h
		${LIB_DIR}/opus-1.0.2/silk/sum_sqr_shift.c
		${LIB_DIR}/opus-1.0.2/silk/table_LSF_cos.c
		${LIB_DIR}/opus-1.0.2/silk/tables.h
		${LIB_DIR}/opus-1.0.2/silk/tables_gain.c
		${LIB_DIR}/opus-1.0.2/silk/tables_LTP.c
		${LIB_DIR}/opus-1.0.2/silk/tables_NLSF_CB_NB_MB.c
		${LIB_DIR}/opus-1.0.2/silk/tables_NLSF_CB_WB.c
		${LIB_DIR}/opus-1.0.2/silk/tables_other.c
		${LIB_DIR}/opus-1.0.2/silk/tables_pitch_lag.c
		${LIB_DIR}/opus-1.0.2/silk/tables_pulses_per_block.c
		${LIB_DIR}/opus-1.0.2/silk/tuning_parameters.h
		${LIB_DIR}/opus-1.0.2/silk/typedef.h
		${LIB_DIR}/opus-1.0.2/silk/VAD.c
		${LIB_DIR}/opus-1.0.2/silk/VQ_WMat_EC.c
		${LIB_DIR}/opus-1.0.2/src/opus.c
		${LIB_DIR}/opus-1.0.2/src/opus_decoder.c
		${LIB_DIR}/opus-1.0.2/src/opus_encoder.c
		${LIB_DIR}/opus-1.0.2/src/opus_multistream.c
		${LIB_DIR}/opus-1.0.2/src/opus_private.h
		${LIB_DIR}/opus-1.0.2/src/repacketizer.c
		${LIB_DIR}/opusfile-0.2/include/opusfile.h
		${LIB_DIR}/opusfile-0.2/src/http.c
		${LIB_DIR}/opusfile-0.2/src/info.c
		${LIB_DIR}/opusfile-0.2/src/internal.c
		${LIB_DIR}/opusfile-0.2/src/internal.h
		${LIB_DIR}/opusfile-0.2/src/opusfile.c
		${LIB_DIR}/opusfile-0.2/src/stream.c
	)
else()
	set(OPUSLIST)
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
  ${OBJ_DIR}/dagcheck.c
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

set( TINYGETTEXT_LIST
  ${MOUNT_DIR}/libs/tinygettext/dictionary_manager.hpp
  ${MOUNT_DIR}/libs/tinygettext/file_system.hpp
  ${MOUNT_DIR}/libs/tinygettext/iconv.cpp
  ${MOUNT_DIR}/libs/tinygettext/plural_forms.hpp
  ${MOUNT_DIR}/libs/tinygettext/tinygettext.cpp
  ${MOUNT_DIR}/libs/tinygettext/tinygettext.hpp
  ${MOUNT_DIR}/libs/tinygettext/dictionary.cpp
  ${MOUNT_DIR}/libs/tinygettext/dictionary.hpp
  ${MOUNT_DIR}/libs/tinygettext/dictionary_manager.cpp
  ${MOUNT_DIR}/libs/tinygettext/iconv.hpp
  ${MOUNT_DIR}/libs/tinygettext/language.cpp
  ${MOUNT_DIR}/libs/tinygettext/language.hpp
  ${MOUNT_DIR}/libs/tinygettext/log.cpp
  ${MOUNT_DIR}/libs/tinygettext/log.hpp
  ${MOUNT_DIR}/libs/tinygettext/log_stream.hpp
  ${MOUNT_DIR}/libs/tinygettext/plural_forms.cpp
  ${MOUNT_DIR}/libs/tinygettext/po_parser.cpp
  ${MOUNT_DIR}/libs/tinygettext/po_parser.hpp
  ${MOUNT_DIR}/libs/findlocale/findlocale.c
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

###############################
# Build Game/Cgame/Ui DLL/.so #
###############################


# Function to setup all the Game/Cgame/UI libraries
include( CMakeParseArguments )
function( GAMELIB )
  # ParseArguments setup
  set( oneValueArgs LIB_NAME OUTPUT_NAME OUTPUT_DIR )
  set( multiValueArgs COMPILE_DEF LIB_FILES QVM_FILES QVM_ASM )
  cmake_parse_arguments( GAMELIB "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN} )

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
    set( GAMELIB_COMPILE_DEF_NATIVE ${GAMELIB_COMPILE_DEF} )
    if( DEBUG_VM_ALLOC )
      set( GAMELIB_COMPILE_DEF_NATIVE ${GAMELIB_COMPILE_DEF} DEBUG_VM_ALLOC )
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
      COMPILE_DEFINITIONS "${GAMELIB_COMPILE_DEF_NATIVE}"
    )

    if( GAME_LIB_LLVM )
      file( MAKE_DIRECTORY "${Daemon_BINARY_DIR}/${GAMELIB_OUTPUT_DIR}/vm" )

      set( GAMELIB_SOURCE_NAME "gamelogic/${GAMELIB_OUTPUT_DIR}" )
      if( GAMELIB_OUTPUT_DIR STREQUAL "main" )
        set( GAMELIB_SOURCE_NAME "gamelogic/" )
      endif()

      set( GAMELIB_LLVM_FLAGS "-w -emit-llvm -DQ3_VM -nostdinc -I${GAMELIB_SOURCE_NAME} -Isrc/engine/qcommon -I${GAMELIB_SOURCE_NAME}/game -I${GAMELIB_SOURCE_NAME}/src/cgame -I${GAMELIB_SOURCE_NAME}/ui" )

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
    else()
      set_target_properties( ${GAMELIB_LIB_NAME} PROPERTIES OUTPUT_NAME "${GAMELIB_OUTPUT_NAME}" )
    endif()
  endif()

  if( GAME_QVM AND GAMELIB_QVM_FILES )
    set( QVM_DIR ${OBJ_DIR}/${GAMELIB_LIB_NAME}-qvm.dir )
    set( QVM_CPP_OPTIONS
      -I"${MOUNT_DIR}/gamelogic/game"
      -I"${MOUNT_DIR}/engine/qcommon"
      -D__inline=
      -DQ3_VM
    )
    foreach( DEF ${GAMELIB_COMPILE_DEF} )
      set( QVM_CPP_OPTIONS ${QVM_CPP_OPTIONS} -D${DEF} )
    endforeach()

    set( QVM_OBJECTS )
    set( GAMELIB_QVM_FILES ${GAMELIB_QVM_FILES} ${MOUNT_DIR}/engine/qcommon/q_static.c )
    foreach( QVM_FILE ${GAMELIB_QVM_FILES} )
      get_filename_component( QVM_FILENAME ${QVM_FILE} NAME_WE )
      get_filename_component( QVM_FILEDIR ${QVM_FILE} PATH )
      set( QVM_COMPONENT ${QVM_DIR}/${QVM_FILENAME} )
      set( QVM_OBJECTS ${QVM_OBJECTS} ${QVM_COMPONENT}.asm )

      add_custom_command( OUTPUT ${QVM_COMPONENT}.asm
        COMMAND lcc -target bytecode ${QVM_CPP_OPTIONS} ${QVM_FILE}
        DEPENDS lcc rcc cpp ${QVM_FILE}
        IMPLICIT_DEPENDS C ${QVM_FILE}
        WORKING_DIRECTORY ${QVM_DIR}
      )
    endforeach()

    # Ensure qvms end up in the correct folder for VS or Xcode projects
    if( XCODE_VERSION OR MSVC )
      set( QVM_OUTPUT ${CMAKE_BINARY_DIR}/$<CONFIGURATION>/${GAMELIB_OUTPUT_DIR}/vm/${GAMELIB_OUTPUT_NAME}.qvm )
    else()
      set( QVM_OUTPUT ${CMAKE_BINARY_DIR}/${GAMELIB_OUTPUT_DIR}/vm/${GAMELIB_OUTPUT_NAME}.qvm )
    endif()

    add_custom_command(
      OUTPUT ${CMAKE_BINARY_DIR}/${GAMELIB_OUTPUT_DIR}/vm/${GAMELIB_OUTPUT_NAME}.qvm
      COMMAND asm -o ${QVM_OUTPUT} ${QVM_OBJECTS} ${GAMELIB_QVM_ASM}
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
  OUTPUT_NAME game
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
if( GAME_LIB OR GAME_QVM )
  install( DIRECTORY ${CMAKE_BINARY_DIR}/main DESTINATION "Unvanquished" )
endif()

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
  set_property( TARGET renderer PROPERTY COMPILE_DEFINITIONS USE_REFENTITY_ANIMATIONSYSTEM )
  set_target_properties( renderer PROPERTIES OUTPUT_NAME "rendererGL" )

  if( NOT MSVC )
    target_link_libraries( renderer m )
  endif()

  # Setup CPack for target renderer
  install( TARGETS renderer DESTINATION "Unvanquished" )
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
  find_package( WebP REQUIRED )
  
  if( UNIX )
    find_package( X11 )
  endif()

  include_directories( ${PNG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${X11_INCLUDE_DIR} ${WEBP_INCLUDE_DIRS} )
  target_link_libraries( renderer ${PNG_LIBRARIES} ${SDL_LIBRARY} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${FREETYPE_LIBRARIES} ${WEBP_LIBRARIES} )

  # This section finds and provides OPTIONAL dependencies to target renderer
endif()

######################
# Build New Renderer #
######################

if( BUILD_NEW_RENDERER )
  # Setup target rendererGL
  add_library( rendererGL SHARED ${SHAREDLIST} ${OPENEXRLIST} ${RENDERERGLLIST} ${MOUNT_DIR}/engine/sys/sdl_glimp.c ${MOUNT_DIR}/engine/sys/sdl_gamma.c ${MOUNT_DIR}/engine/rendererGL/rendererGL.def )
  set_property( TARGET rendererGL PROPERTY COMPILE_DEFINITIONS USE_XREAL_RENDERER USE_REFENTITY_ANIMATIONSYSTEM )
  set_target_properties( rendererGL PROPERTIES OUTPUT_NAME "rendererGL3" )

  target_link_libraries( rendererGL ${OS_LIBRARIES} )

  # Setup CPack for target rendererGL
  install( TARGETS rendererGL DESTINATION "Unvanquished" )
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
  find_package( WebP REQUIRED )

  if( UNIX )
    find_package( X11 )
  endif()

  if( USE_SMP )
      set_property( TARGET rendererGL APPEND PROPERTY COMPILE_DEFINITIONS SMP )
  endif()

  include_directories( ${PNG_INCLUDE_DIR} ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${FREETYPE_INCLUDE_DIRS} ${GLEW_INCLUDE_DIRS} ${SDL_INCLUDE_DIR} ${X11_INCLUDE_DIR} ${WEBP_INCLUDE_DIRS} )
  target_link_libraries( rendererGL ${PNG_LIBRARIES} ${GLEW_LIBRARIES} ${OPENGL_LIBRARIES} ${JPEG_LIBRARIES} ${FREETYPE_LIBRARIES} ${SDL_LIBRARY} ${WEBP_LIBRARIES} )

  # This section finds and provides OPTIONAL dependencies to target rendererGL
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
    set( CURSES_LIBRARIES ${CURSESW_LIBRARIES} )
  else()
    find_package( Curses REQUIRED )
    set( CURSES_DEFINES USE_CURSES )
  endif()
endif()

##############
# Find GeoIP #
##############
function( find_package_geoip )
  if( USE_GEOIP )
    find_package( PkgConfig )
    if( PKG_CONFIG_FOUND )
      pkg_check_modules( GEOIP geoip )
      if ( GEOIP_FOUND )
        find_path( GEOIP_INCLUDE_DIRS GeoIP.h HINTS ${PC_GEOIP_INCLUDEDIR} ${PC_GEOIP_INCLUDE_DIRS} )
        find_library( GEOIP_LIBRARY NAMES GeoIP libGeoIP HINTS ${PC_GEOIP_LIBDIR} ${PC_GEOIP_LIBRARY_DIRS} )
        mark_as_advanced( GEOIP_INCLUDE_DIRS GEOIP_LIBRARY )
      endif()
    endif()
  endif()
  if ( NOT USE_GEOIP OR NOT PKG_CONFIG_FOUND )
    set( GEOIP_FOUND "" CACHE INTERNAL "" )
  endif()
endfunction()

################
# Build Client #
################

if( BUILD_CLIENT )
  # Setup target client
  add_executable( client ${SERVERLIST} ${CLIENTLIST} ${SDLBASELIST} ${SDLLIST} ${QCOMMONLIST} ${SHAREDLIST} ${CRYPTOLIST} ${TINYGETTEXT_LIST}
    ${DATABASELIST} ${MOUNT_DIR}/engine/qcommon/dl_main_curl.c ${SPEEXLIST} ${OPUSLIST} ${WIN_RC} )
  add_dependencies( client rendererGL )
  include_directories( ${NEWTON_INCLUDES} )
  target_link_libraries( client ${OS_LIBRARIES} ${NEWTON_LIBRARY} )
  set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_XREAL_RENDERER COMPAT_ET USE_REFENTITY_ANIMATIONSYSTEM HAVE_SDL USE_CODEC_OPUS )
  set_target_properties( client PROPERTIES OUTPUT_NAME "daemon" PREFIX "" LINKER_LANGUAGE CXX )

  # Setup CPack for target client
  install( TARGETS client DESTINATION "Unvanquished" )
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
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS ZLIB_WINAPI )
  endif()

  # This section finds and provides REQUIRED dependencies to target client
  find_package( OpenGL REQUIRED )
  find_package( SDL REQUIRED )
  find_package( Freetype REQUIRED )
  find_package( GLEW REQUIRED )
  find_package( GMP REQUIRED )
  find_package( ZLIB REQUIRED )
  find_package( CURL REQUIRED )
  find_package_geoip()

  if( GEOIP_FOUND )
    set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS HAVE_GEOIP )
    include_directories( ${GEOIP_INCLUDE_DIRS} )
    target_link_libraries( client ${GEOIP_LIBRARIES} )
  endif()

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
    else()
		target_link_libraries( client ${OPENAL_LIBRARY} )
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
    else()
      set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS HAVE_CONFIG_H )
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

  if( USE_INTERNAL_OPUS )
      if( NOT MSVC )
        set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS USE_ALLOCA FLOATING_POINT OPUS_BUILD )
      else()
        set_property( TARGET client APPEND PROPERTY COMPILE_DEFINITIONS HAVE_CONFIG_H )
        include_directories( ${LIB_DIR}/opus-1.0.2/include/win32 )
      endif()
      include_directories( ${LIB_DIR}/opusfile-0.2/include ${LIB_DIR}/opus-1.0.2/include ${LIB_DIR}/opus-1.0.2/celt ${LIB_DIR}/opus-1.0.2/silk ${LIB_DIR}/opus-1.0.2/silk/float )
  else()
      find_package( Opus REQUIRED )
      include_directories( ${OPUS_INCLUDE_DIRS} )
      target_link_libraries( client ${OPUS_LIBRARIES} )
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
  include_directories( ${OPENAL_INCLUDE_DIR} )
endif()

################
# Build Server #
################

if( BUILD_SERVER )
  # Setup the target Server
  add_executable( server ${SERVERLIST} ${SDLBASELIST}
    ${QCOMMONLIST} ${SHAREDLIST} ${DATABASELIST} ${CRYPTOLIST}
    ${MOUNT_DIR}/engine/null/null_client.c ${TINYGETTEXT_LIST}
    ${MOUNT_DIR}/engine/null/null_input.c ${MOUNT_DIR}/engine/null/null_snddma.c
    ${MOUNT_DIR}/engine/qcommon/dl_main_stubs.c ${WIN_RC} )
  include_directories( ${NEWTON_INCLUDES} )
  target_link_libraries( server ${OS_LIBRARIES} ${NEWTON_LIBRARY} )
  set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS DEDICATED COMPAT_ET )
  set_target_properties( server PROPERTIES OUTPUT_NAME "daemonded" PREFIX "" LINKER_LANGUAGE CXX )

  # Setup CPack for target server
  install( TARGETS server DESTINATION "Unvanquished" )
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
  find_package( GMP REQUIRED )
  find_package( ZLIB REQUIRED )
  find_package_geoip()

  if( GEOIP_FOUND )
    set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS HAVE_GEOIP )
    include_directories( ${GEOIP_INCLUDE_DIRS} )
    target_link_libraries( server ${GEOIP_LIBRARIES} )
  endif()

  if( USE_INTERNAL_CRYPTO )
    # BEFORE is required to prevent system headers from being used
    include_directories( BEFORE ${MOUNT_DIR}/libs/nettle )
  else()
    find_package( Nettle REQUIRED )
    include_directories( ${NETTLE_INCLUDE_DIRS} )
    target_link_libraries( server ${NETTLE_LIBRARIES} )
  endif()

  include_directories( ${GMP_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} )
  target_link_libraries( server ${GMP_LIBRARIES} ${ZLIB_LIBRARIES} )

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

  if( USE_SDL_ICONV )
    find_package( SDL REQUIRED )
    set_property( TARGET server APPEND PROPERTY COMPILE_DEFINITIONS HAVE_SDL )
    include_directories( ${SDL_INCLUDE_DIR} )
    target_link_libraries( server ${SDL_LIBRARY} )
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
  set_target_properties( daemonMap PROPERTIES OUTPUT_NAME "daemonmap" PREFIX "" )

  # Setup CPack for target daemonmap
  install( TARGETS daemonMap DESTINATION "Unvanquished" )
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
  find_package( GLib2 REQUIRED )
  target_link_libraries( daemonMap ${OPENGL_LIBRARIES} ${SDL_LIBRARY} ${JPEG_LIBRARIES} ${PNG_LIBRARIES} ${GLIB_LIBRARIES} )
  include_directories( ${OPENGL_INCLUDE_DIR} ${JPEG_INCLUDE_DIR} ${PNG_INCLUDE_DIR} ${SDL_INCLUDE_DIR} ${GLIB_INCLUDE_DIRS} )
endif()

####################
# Build tty client #
####################
if( BUILD_TTY_CLIENT )
  # Setup the target Server
  add_executable( ttyclient ${SERVERLIST} ${CLIENTLIST} ${SDLBASELIST}
    ${QCOMMONLIST} ${SHAREDLIST} ${DATABASELIST} ${CRYPTOLIST}
    ${TINYGETTEXT_LIST}
    ${MOUNT_DIR}/engine/qcommon/dl_main_stubs.c ${WIN_RC}
    ${MOUNT_DIR}/engine/null/null_input.c ${MOUNT_DIR}/engine/null/null_snddma.c ${MOUNT_DIR}/engine/null/null_renderer.c )
  include_directories( ${NEWTON_INCLUDES} )
  target_link_libraries( ttyclient ${OS_LIBRARIES} ${NEWTON_LIBRARY} )
  set_property( TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS COMPAT_ET BUILD_TTY_CLIENT REF_HARD_LINKED )
  set_target_properties( ttyclient PROPERTIES OUTPUT_NAME "daemon-tty" PREFIX "" LINKER_LANGUAGE CXX )

  # Setup CPack for target server
  install( TARGETS ttyclient DESTINATION "Unvanquished" )
  get_target_property( LOCATION ttyclient LOCATION )
  get_filename_component( OUTPUT_NAME ${LOCATION} NAME )
  set( APPS ${APPS} Unvanquished/${OUTPUT_NAME} )

  # Mac OS X has some trouble with PIC code under X86/32.
  if( CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND BUILD_ARCH STREQUAL "i386" )
    set_target_properties( ttyclient PROPERTIES COMPILE_FLAGS -mdynamic-no-pic LINK_FLAGS "-read_only_relocs suppress" )
  endif()

  # Visual Studio specific settings
  if( MSVC )
    set_property( TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS ZLIB_WINAPI )
  endif()

  # The following section finds the REQUIRED dependencies and makes them available to the target
  find_package( GMP REQUIRED )
  find_package( ZLIB REQUIRED )
  find_package_geoip()

  if( GEOIP_FOUND )
    set_property( TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS HAVE_GEOIP )
    include_directories( ${GEOIP_INCLUDE_DIRS} )
    target_link_libraries( ttyclient ${GEOIP_LIBRARIES} )
  endif()

  if( USE_INTERNAL_CRYPTO )
    # BEFORE is required to prevent system headers from being used
    include_directories( BEFORE ${MOUNT_DIR}/libs/nettle )
  else()
    find_package( Nettle REQUIRED )
    include_directories( ${NETTLE_INCLUDE_DIRS} )
    target_link_libraries( ttyclient ${NETTLE_LIBRARIES} )
  endif()

  include_directories( ${GMP_INCLUDE_DIR} ${ZLIB_INCLUDE_DIRS} )
  target_link_libraries( ttyclient ${GMP_LIBRARIES} ${ZLIB_LIBRARIES} )

  # The following section finds the OPTIONAL dependencies and makes them available to the target

  if( USE_CURSES )
    set_property( TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS ${CURSES_DEFINES} )
    include_directories( ${CURSES_INCLUDE_DIR} )
    target_link_libraries( ttyclient ${CURSES_LIBRARIES} )
  endif()

  if( USE_PHYSICS )
    find_package( TinyXML REQUIRED )
    set_property( TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS USE_PHYSICS )
    include_directories( ${TINYXML_INCLUDE_DIRS} )
    target_link_libraries( ttyclient ${TINYXML_LIBRARIES} )
  endif()

  if( HAVE_BZIP2 )
    find_package( BZip2 REQUIRED )
    set_property( TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS HAVE_BZIP2 )
    include_directories( ${BZIP2_INCLUDE_DIR} )
    target_link_libraries( ttyclient ${BZIP2_LIBRARY} )
  endif()

  if( USE_SDL_ICONV )
    find_package( SDL REQUIRED )
    set_property( TARGET ttyclient APPEND PROPERTY COMPILE_DEFINITIONS HAVE_SDL )
    include_directories( ${SDL_INCLUDE_DIR} )
    target_link_libraries( ttyclient ${SDL_LIBRARY} )
  endif()

  if( GAME_LIB_LLVM )
    include_directories( ${LLVM_INCLUDE_DIR} )
    target_link_libraries( ttyclient ${LLVM_LIBS} )
  endif()
endif()



###########################
#Configure Download Script#
###########################

if( NOT MSVC )
  execute_process( COMMAND sed -e "/^#define PRODUCT_VERSION/! d; s/.*\"\\(.*\\)\\.[^.]\"/\\1/" ${CMAKE_SOURCE_DIR}/src/engine/qcommon/q_shared.h
                   OUTPUT_VARIABLE PRODUCT_VERSION OUTPUT_STRIP_TRAILING_WHITESPACE )
  add_custom_target( download-pk3.sh ALL
    sed -e 's/VERSION=/&${PRODUCT_VERSION}/' ${CMAKE_SOURCE_DIR}/download-pk3.sh.in >${CMAKE_BINARY_DIR}/download-pk3.sh\; chmod +x ${CMAKE_BINARY_DIR}/download-pk3.sh
    DEPENDS ${CMAKE_SOURCE_DIR}/src/engine/qcommon/q_shared.h ${CMAKE_SOURCE_DIR}/download-pk3.sh.in
  )
endif()

#####################
#Configure Installer#
#####################
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
