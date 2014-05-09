# Create DukTape

find_package( C99 REQUIRED )
# Remove -ffast-math
STRING( REPLACE "-ffast-math" "" CMAKE_C_FLAGS ${CMAKE_C_FLAGS} )
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${C99_C_FLAGS}")

set( DUK_SRC
	${LIB_DIR}/duktape/duktape.c
	${LIB_DIR}/duktape/duktape.h
)

add_library( DUK_LIB STATIC ${DUK_SRC} )
