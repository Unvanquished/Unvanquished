###################
# Build QVM stuff #
###################

project( qcc C )

set( Q3LCCETCDIR ${LCC_DIR}/etc )

set( Q3LBURG_LIST
  ${LCC_DIR}/lburg/gram.c
  ${LCC_DIR}/lburg/lburg.c
)

set( Q3RCC_LIST
  ${LCC_DIR}/src/alloc.c
  ${LCC_DIR}/src/bind.c
  ${LCC_DIR}/src/bytecode.c
  ${LCC_DIR}/src/dag.c
  ${LCC_DIR}/src/decl.c
  ${LCC_DIR}/src/enode.c
  ${LCC_DIR}/src/error.c
  ${LCC_DIR}/src/event.c
  ${LCC_DIR}/src/expr.c
  ${LCC_DIR}/src/gen.c
  ${LCC_DIR}/src/init.c
  ${LCC_DIR}/src/inits.c
  ${LCC_DIR}/src/input.c
  ${LCC_DIR}/src/lex.c
  ${LCC_DIR}/src/list.c
  ${LCC_DIR}/src/main.c
  ${LCC_DIR}/src/null.c
  ${LCC_DIR}/src/output.c
  ${LCC_DIR}/src/prof.c
  ${LCC_DIR}/src/profio.c
  ${LCC_DIR}/src/simp.c
  ${LCC_DIR}/src/stmt.c
  ${LCC_DIR}/src/string.c
  ${LCC_DIR}/src/sym.c
  ${LCC_DIR}/src/symbolic.c
  ${LCC_DIR}/src/trace.c
  ${LCC_DIR}/src/tree.c
  ${LCC_DIR}/src/types.c
  ${OBJ_DIR}/dagcheck.c
)

set( Q3CPP_LIST
  ${LCC_DIR}/cpp/cpp.c
  ${LCC_DIR}/cpp/lex.c
  ${LCC_DIR}/cpp/nlist.c
  ${LCC_DIR}/cpp/tokens.c
  ${LCC_DIR}/cpp/macro.c
  ${LCC_DIR}/cpp/eval.c
  ${LCC_DIR}/cpp/include.c
  ${LCC_DIR}/cpp/hideset.c
  ${LCC_DIR}/cpp/getopt.c
  ${LCC_DIR}/cpp/unix.c
)

set( Q3LCC_LIST
  ${LCC_DIR}/etc/lcc.c
  ${LCC_DIR}/etc/bytecode.c
)

set( Q3ASM_LIST
  ${LCC_DIR}/asm/q3asm.c
  ${LCC_DIR}/asm/cmdlib.c
)

add_executable( lburg ${Q3LBURG_LIST} )
get_target_property( LBURG_EXE lburg LOCATION )

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/CMakeFiles/dagcheck.c
  COMMAND ${LBURG_EXE} "${LCC_DIR}/src/dagcheck.md" >"${CMAKE_BINARY_DIR}/CMakeFiles/dagcheck.c"
  DEPENDS lburg
  MAIN_DEPENDENCY ${LCC_DIR}/src/dagcheck.md
)

include_directories( ${LCC_DIR}/src ${LCC_DIR}/cpp )

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
