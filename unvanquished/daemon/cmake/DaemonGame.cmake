################################################################################
# Configuration options
################################################################################

set( NACL_RUNTIME_PATH "" CACHE STRING "Directory containing the NaCl binaries" )

################################################################################
# Determine platform + architecture
################################################################################

include( DaemonUtils )

################################################################################
# Directories
################################################################################

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
# File lists
################################################################################

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
    foreach( FILE ${GAMEMODULE_FILES} ${SHAREDLIST} ${NACLLIST_MODULE} ${ZLIBLIST} ${MINIZIPLIST} )
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
