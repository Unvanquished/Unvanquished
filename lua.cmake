set(LUA_DIR ${CMAKE_CURRENT_SOURCE_DIR}/libs/lua)

set(LUA_INCLUDE_DIR ${LUA_DIR})

set(LUA_SRC_FILES
	${LUA_DIR}/lapi.c
	${LUA_DIR}/lapi.h
	${LUA_DIR}/lauxlib.c
	${LUA_DIR}/lauxlib.h
	${LUA_DIR}/lbaselib.c
	${LUA_DIR}/lcode.c
	${LUA_DIR}/lcode.h
	${LUA_DIR}/lcorolib.c
	${LUA_DIR}/lctype.c
	${LUA_DIR}/lctype.h
	${LUA_DIR}/ldblib.c
	${LUA_DIR}/ldebug.c
	${LUA_DIR}/ldebug.h
	${LUA_DIR}/ldo.c
	${LUA_DIR}/ldo.h
	${LUA_DIR}/ldump.c
	${LUA_DIR}/lfunc.c
	${LUA_DIR}/lfunc.h
	${LUA_DIR}/lgc.c
	${LUA_DIR}/lgc.h
	${LUA_DIR}/linit.c
	${LUA_DIR}/liolib.c
	${LUA_DIR}/ljumptab.h
	${LUA_DIR}/llex.c
	${LUA_DIR}/llex.h
	${LUA_DIR}/llimits.h
	${LUA_DIR}/lmathlib.c
	${LUA_DIR}/lmem.c
	${LUA_DIR}/lmem.h
	${LUA_DIR}/loadlib.c
	${LUA_DIR}/lobject.c
	${LUA_DIR}/lobject.h
	${LUA_DIR}/lopcodes.c
	${LUA_DIR}/lopcodes.h
	${LUA_DIR}/lopnames.h
	${LUA_DIR}/loslib.c
	${LUA_DIR}/lparser.c
	${LUA_DIR}/lparser.h
	${LUA_DIR}/lprefix.h
	${LUA_DIR}/lstate.c
	${LUA_DIR}/lstate.h
	${LUA_DIR}/lstring.c
	${LUA_DIR}/lstring.h
	${LUA_DIR}/lstrlib.c
	${LUA_DIR}/ltable.c
	${LUA_DIR}/ltable.h
	${LUA_DIR}/ltablib.c
#	${LUA_DIR}/ltests.c
#	${LUA_DIR}/ltests.h
	${LUA_DIR}/ltm.c
	${LUA_DIR}/ltm.h
#	${LUA_DIR}/lua.c
	${LUA_DIR}/luaconf.h
	${LUA_DIR}/lua.h
	${LUA_DIR}/lualib.h
	${LUA_DIR}/lundump.c
	${LUA_DIR}/lundump.h
	${LUA_DIR}/lutf8lib.c
	${LUA_DIR}/lvm.c
	${LUA_DIR}/lvm.h
	${LUA_DIR}/lzio.c
	${LUA_DIR}/lzio.h
#	${LUA_DIR}/onelua.c
)

set(LUA_LIBRARY srclibs-lua)

add_library(${LUA_LIBRARY} STATIC ${LUA_SRC_FILES})

unset(LUA_DEFINITIONS)

if (NACL AND USE_NACL_SAIGO)
	# Workaround for this Saigo Clang internal error:
	# > fatal error: error in backend: Cannot select: t21: ch = brind t20, t19
	# > error: Not enough scratch registers when expanding indirect jump.
	# See: https://github.com/Unvanquished/Unvanquished/pull/3195#issuecomment-2484530883
	list(APPEND LUA_DEFINITIONS LUA_USE_JUMPTABLE=0)
endif()

set_target_properties(${LUA_LIBRARY} PROPERTIES
	COMPILE_DEFINITIONS "${LUA_DEFINITIONS}"
	FOLDER "libs"
)
