###################
# Build QVM stuff #
###################

project( qcc C )

add_executable( lburg ${Q3LBURG_LIST} )
get_target_property( LBURG_EXE lburg LOCATION )

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/CMakeFiles/dagcheck.c
  COMMAND ${LBURG_EXE} "${MOUNT_DIR}/tools/lcc/src/dagcheck.md" >"${CMAKE_BINARY_DIR}/CMakeFiles/dagcheck.c"
  DEPENDS lburg
  MAIN_DEPENDENCY ${MOUNT_DIR}/tools/lcc/src/dagcheck.md
)

include_directories( ${MOUNT_DIR}/tools/lcc/src ${MOUNT_DIR}/tools/lcc/cpp )

add_executable( cpp ${Q3CPP_LIST} )
set_property( TARGET cpp APPEND PROPERTY COMPILE_DEFINITIONS TEMPDIR="${CMAKE_BINARY_DIR}" SYSTEM= )
set_property( TARGET cpp APPEND PROPERTY OUTPUT_NAME q3cpp )

add_executable( rcc ${Q3RCC_LIST} )
set_property( TARGET rcc APPEND PROPERTY COMPILE_DEFINITIONS TEMPDIR="${CMAKE_BINARY_DIR}" SYSTEM= )
set_property( TARGET rcc APPEND PROPERTY OUTPUT_NAME q3rcc )

add_executable( lcc ${Q3LCC_LIST} )
set_property( TARGET lcc APPEND PROPERTY COMPILE_DEFINITIONS TEMPDIR="${CMAKE_BINARY_DIR}" SYSTEM= )
set_property( TARGET lcc APPEND PROPERTY OUTPUT_NAME q3lcc )

add_executable( asm ${Q3ASM_LIST} )
set_property( TARGET asm APPEND PROPERTY COMPILE_DEFINITIONS TEMPDIR="${CMAKE_BINARY_DIR}" SYSTEM= )
set_property( TARGET asm APPEND PROPERTY OUTPUT_NAME q3asm )
