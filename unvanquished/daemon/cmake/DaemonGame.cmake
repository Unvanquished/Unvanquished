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
