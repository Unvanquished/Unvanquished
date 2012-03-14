/*
* ET <-> *Lua* interface source file.
*
* This code is taken from ETPub. All credits go to their team especially quad and pheno!
* http://etpub.org
*
* Find the ETPub Lua code by doing a text seach for "*LUA*"
*
* TheDushan 04/2011
*
*/

#ifndef _G_LUA_H
#define _G_LUA_H

#include "../../../../engine/qcommon/q_shared.h"
#include "g_local.h"

#include "lua.h"
#include "lauxlib.h"
#include "lualib.h"

#define LUA_NUM_VM          16
#define LUA_MAX_FSIZE       1024 * 1024 // 1MB

#define FIELD_INT           0
#define FIELD_STRING        1
#define FIELD_FLOAT         2
#define FIELD_ENTITY        3
#define FIELD_VEC3          4
#define FIELD_INT_ARRAY     5
#define FIELD_TRAJECTORY    6
#define FIELD_FLOAT_ARRAY   7

#define FIELD_FLAG_GENTITY  1 // marks a gentity_s field
#define FIELD_FLAG_GCLIENT  2 // marks a gclient_s field
#define FIELD_FLAG_NOPTR    4
#define FIELD_FLAG_READONLY 8 // read-only access

// define HOSTARCH and EXTENSION depending on host architecture
#ifdef __linux__
#define HOSTARCH  "UNIX"
#define EXTENSION "so"
#elif defined WIN32
#define HOSTARCH  "WIN32"
#define EXTENSION "dll"
#endif

// macros to register predefined constants
#define lua_registerglobal(L, n, v)           ( lua_pushstring(L, v), lua_setglobal(L, n))
#define lua_regconstinteger(L, n)             ( lua_pushstring(L, #n), lua_pushinteger(L, n), lua_settable(L, -3))
#define lua_regconststring(L, n)              ( lua_pushstring(L, #n), lua_pushstring(L, n), lua_settable(L, -3))

// macros to add gentity and gclient fields
#define _et_gentity_addfield(n, t, f)         { #n, t, offsetof(struct gentity_s, n), FIELD_FLAG_GENTITY + f }
#define _et_gentity_addfieldalias(n, a, t, f) { #n, t, offsetof(struct gentity_s, a), FIELD_FLAG_GENTITY + f }
#define _et_gclient_addfield(n, t, f)         { #n, t, offsetof(struct gclient_s, n), FIELD_FLAG_GCLIENT + f }
#define _et_gclient_addfieldalias(n, a, t, f) { #n, t, offsetof(struct gclient_s, a), FIELD_FLAG_GCLIENT + f }

typedef struct
{
	int       id;
	char      file_name[ MAX_QPATH ];
	char      mod_name[ MAX_CVAR_VALUE_STRING ];
	char      mod_signature[ 41 ];
	char      *code;
	int       code_size;
	int       err;
	lua_State *L;
} lua_vm_t;

typedef struct
{
	const char    *name;
	int           type;
	unsigned long mapping;
	int           flags;
} gentity_field_t;

extern lua_vm_t *lVM[ LUA_NUM_VM ];

// API
qboolean        G_LuaInit();
qboolean        G_LuaCall( lua_vm_t *vm, char *func, int nargs, int nresults );
qboolean        G_LuaGetNamedFunction( lua_vm_t *vm, char *name );
qboolean        G_LuaStartVM( lua_vm_t *vm );
void            G_LuaStopVM( lua_vm_t *vm );
void            G_LuaShutdown();
void            G_LuaStatus( gentity_t *ent );
lua_vm_t         *G_LuaGetVM( lua_State *L );

// Callbacks
void            G_LuaHook_InitGame( int levelTime, int randomSeed, int restart );
void            G_LuaHook_ShutdownGame( int restart );
void            G_LuaHook_RunFrame( int levelTime );
qboolean        G_LuaHook_ClientConnect( int clientNum, qboolean firstTime, qboolean isBot, char *reason );
void            G_LuaHook_ClientDisconnect( int clientNum );
void            G_LuaHook_ClientBegin( int clientNum );
void            G_LuaHook_ClientUserinfoChanged( int clientNum );
void            G_LuaHook_ClientSpawn( int clientNum, qboolean revived, qboolean teamChange, qboolean restoreHealth );
qboolean        G_LuaHook_ClientCommand( int clientNum, char *command );
qboolean        G_LuaHook_ConsoleCommand( char *command );
qboolean        G_LuaHook_UpgradeSkill( int cno, skillType_t skill );
qboolean        G_LuaHook_SetPlayerSkill( int cno, skillType_t skill );
void            G_LuaHook_Print( char *text );

#endif /* ifndef _G_LUA_H */
