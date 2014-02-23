/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef G_TRAPCALLS_H_
#define G_TRAPCALLS_H_

void             trap_Print( const char *string );
void NORETURN    trap_Error( const char *string );
int              trap_Milliseconds( void );
void             trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags );
void             trap_Cvar_Set( const char *var_name, const char *value );
void             trap_Cvar_Update( vmCvar_t *cvar );
int              trap_Cvar_VariableIntegerValue( const char *var_name );
void             trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
int              trap_Argc( void );
void             trap_Argv( int n, char *buffer, int bufferLength );
void             trap_SendConsoleCommand( int exec_when, const char *text );
int              trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void             trap_FS_Read( void *buffer, int len, fileHandle_t f );
int              trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void             trap_FS_Rename( const char *from, const char *to );
void             trap_FS_FCloseFile( fileHandle_t f );
int              trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
void             trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGClient );
void             trap_DropClient( int clientNum, const char *reason );
void             trap_SendServerCommand( int clientNum, const char *text );
void             trap_SetConfigstring( int num, const char *string );
void             trap_LinkEntity( gentity_t *ent );
void             trap_UnlinkEntity( gentity_t *ent );
int              trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *list, int maxcount );
qboolean         trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
qboolean         trap_EntityContactCapsule( const vec3_t mins, const vec3_t maxs, const gentity_t *ent );
void             trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
void             trap_TraceNoEnts( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
void             trap_TraceCapsule( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
void             trap_TraceCapsuleNoEnts( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask );
int              trap_PointContents( const vec3_t point, int passEntityNum );
void             trap_SetBrushModel( gentity_t *ent, const char *name );
qboolean         trap_InPVS( const vec3_t p1, const vec3_t p2 );
qboolean         trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 );
void             trap_SetConfigstringRestrictions( int num, const clientList_t *clientList );
void             trap_GetConfigstring( int num, char *buffer, int bufferSize );
void             trap_SetConfigstringRestrictions( int num, const clientList_t *clientList );
void             trap_SetUserinfo( int num, const char *buffer );
void             trap_GetUserinfo( int num, char *buffer, int bufferSize );
void             trap_GetServerinfo( char *buffer, int bufferSize );
void             trap_AdjustAreaPortalState( gentity_t *ent, qboolean open );
qboolean         trap_AreasConnected( int area1, int area2 );
int              trap_BotAllocateClient( int clientNum );
void             trap_BotFreeClient( int clientNum );
void             trap_GetUsercmd( int clientNum, usercmd_t *cmd );
qboolean         trap_GetEntityToken( char *buffer, int bufferSize );
int              trap_GMTime( qtime_t *qtime );
void             trap_SendGameStat( const char *data );
void             trap_AddCommand( const char *cmdName );
void             trap_RemoveCommand( const char *cmdName );
qboolean         trap_GetTag( int clientNum, int tagFileNumber, const char *tagName, orientation_t *ori );
qboolean         trap_LoadTag( const char *filename );
int              trap_GetSoundLength( sfxHandle_t sfxHandle );
int              trap_Parse_AddGlobalDefine( const char *define );
int              trap_Parse_LoadSource( const char *filename );
int              trap_Parse_FreeSource( int handle );
int              trap_Parse_ReadToken( int handle, pc_token_t *pc_token );
int              trap_Parse_SourceFileAndLine( int handle, char *filename, int *line );
int              trap_BotGetServerCommand( int clientNum, char *message, int size );
void             trap_SendMessage( int clientNum, char *buf, int buflen );
messageStatus_t  trap_MessageStatus( int clientNum );

int              trap_RSA_GenerateMessage( const char *public_key, char *cleartext, char *encrypted );

void             trap_QuoteString( const char *str, char *buf, int size );

void             trap_GenFingerprint( const char *pubkey, int size, char *buffer, int bufsize );
void             trap_GetPlayerPubkey( int clientNum, char *pubkey, int size );

void             trap_GetTimeString( char *buffer, int size, const char *format, const qtime_t *tm );
qboolean         trap_FindPak( const char *name );

qboolean         trap_BotSetupNav( const botClass_t *botClass, qhandle_t *navHandle );
void             trap_BotShutdownNav( void );
void             trap_BotSetNavMesh( int botClientNum, qhandle_t navHandle );
qboolean         trap_BotFindRoute( int botClientNum, const botRouteTarget_t *target, qboolean allowPartial );
qboolean         trap_BotUpdatePath( int botClientNum, const botRouteTarget_t *target, botNavCmd_t *cmd );
qboolean         trap_BotNavTrace( int botClientNum, botTrace_t *botTrace, const vec3_t start, const vec3_t end );
void             trap_BotFindRandomPoint( int botClientNum, vec3_t point );
qboolean         trap_BotFindRandomPointInRadius( int botClientNum, const vec3_t origin, vec3_t point, float radius );
void             trap_BotEnableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
void             trap_BotDisableArea( const vec3_t origin, const vec3_t mins, const vec3_t maxs );
void             trap_BotAddObstacle( const vec3_t mins, const vec3_t maxs, qhandle_t *handle );
void             trap_BotRemoveObstacle( qhandle_t handle );
void             trap_BotUpdateObstacles( void );

#endif // G_TRAPCALLS_H_
