/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "g_local.h"
//#include "../../../src/engine/server/g_api.h"

static intptr_t ( QDECL *syscall )( intptr_t arg, ... ) = ( intptr_t ( QDECL * )( intptr_t, ... ) ) - 1;

void dllEntry( intptr_t ( QDECL *syscallptr )( intptr_t arg, ... ) )
{
	syscall = syscallptr;
}

int PASSFLOAT( float x )
{
	float floatTemp;

	floatTemp = x;
	return * ( int * ) &floatTemp;
}

//00.
//Com_Printf("%s", (char *)VMA(1));
void trap_Print( const char *fmt )
{
	syscall( G_PRINT, fmt );
}

//01.
//Com_Error(ERR_DROP, "%s", (char *)VMA(1));
void trap_Error( const char *fmt )
{
	syscall( G_ERROR, fmt );
}

//02.
//return Sys_Milliseconds();
int trap_Milliseconds( void )
{
	return syscall( G_MILLISECONDS );
}

//03.
//Cvar_Register(VMA(1), VMA(2), VMA(3), args[4]);
void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags )
{
	syscall( G_CVAR_REGISTER, cvar, var_name, value, flags );
}

//04.
//Cvar_Update(VMA(1));
void trap_Cvar_Update( vmCvar_t *cvar )
{
	syscall( G_CVAR_UPDATE, cvar );
}

//05.
//Cvar_Set((const char *)VMA(1), (const char *)VMA(2));
void trap_Cvar_Set( const char *var_name, const char *value )
{
	syscall( G_CVAR_SET, var_name, value );
}

//06.
//return Cvar_VariableIntegerValue((const char *)VMA(1));
int trap_Cvar_VariableIntegerValue( const char *var_name )
{
	return syscall( G_CVAR_VARIABLE_INTEGER_VALUE, var_name );
}

//07.
//Cvar_VariableStringBuffer(VMA(1), VMA(2), args[3]);
void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize )
{
	syscall( G_CVAR_VARIABLE_STRING_BUFFER, var_name, buffer, bufsize );
}

//08.
//Cvar_LatchedVariableStringBuffer(VMA(1), VMA(2), args[3]);
void trap_Cvar_LatchedVariableStringBuffer( const char *var_name, char *buffer, int bufsize )
{
	syscall( G_CVAR_LATCHEDVARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

//09.
//return Cmd_Argc();
int trap_Argc( void )
{
	return syscall( G_ARGC );
}

//10.
//Cmd_ArgvBuffer(args[1], VMA(2), args[3]);
void trap_Argv( int n, char *buffer, int bufferLength )
{
	syscall( G_ARGV, n, buffer, bufferLength );
}

//11.
//Cbuf_ExecuteText(args[1], VMA(2));
void trap_SendConsoleCommand( int exec_when, const char *text )
{
	syscall( G_SEND_CONSOLE_COMMAND, exec_when, text );
}

//12.
//return FS_FOpenFileByMode(VMA(1), VMA(2), args[3]);
int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode )
{
	return syscall( G_FS_FOPEN_FILE, qpath, f, mode );
}

//13.
//FS_Read2(VMA(1), args[2], args[3]);
void trap_FS_Read( void *buffer, int len, fileHandle_t f )
{
	syscall( G_FS_READ, buffer, len, f );
}

//14.
//return FS_Write(VMA(1), args[2], args[3]);
int trap_FS_Write( const void *buffer, int len, fileHandle_t f )
{
	return syscall( G_FS_WRITE, buffer, len, f );
}

//15.
//FS_Rename(VMA(1), VMA(2));
int trap_FS_Rename( const char *from, const char *to )
{
	return syscall( G_FS_RENAME, from, to );
}

//16.
//FS_FCloseFile(args[1]);
void trap_FS_FCloseFile( fileHandle_t f )
{
	syscall( G_FS_FCLOSE_FILE, f );
}

//17.
//return FS_GetFileList(VMA(1), VMA(2), VMA(3), args[4]);
int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize )
{
	return syscall( G_FS_GETFILELIST, path, extension, listbuf, bufsize );
}

//18.
//SV_LocateGameData(VMA(1), args[2], args[3], VMA(4), args[5]);
void trap_LocateGameData( gentity_t *gEnts, int numGEntities, int sizeofGEntity_t, playerState_t *clients, int sizeofGClient )
{
	syscall( G_LOCATE_GAME_DATA, gEnts, numGEntities, sizeofGEntity_t, clients, sizeofGClient );
}

//19.
//SV_GameDropClient(args[1], VMA(2), args[3]);
void trap_DropClient( int clientNum, const char *reason, int length )
{
	syscall( G_DROP_CLIENT, clientNum, reason, length );
}

//20.
//SV_GameSendServerCommand(args[1], VMA(2));
void trap_SendServerCommand( int clientNum, const char *text )
{
	if ( strlen( text ) > 1022 )
	{
		G_LogPrintf( "%s: trap_SendServerCommand( %d, ... ) length exceeds 1022.\n", GAME_VERSION, clientNum );
		G_LogPrintf( "%s: text [%.950s]... truncated\n", GAME_VERSION, text );
		return;
	}

	syscall( G_SEND_SERVER_COMMAND, clientNum, text );
}

//21.
//SV_LinkEntity(VMA(1));
void trap_LinkEntity( gentity_t *ent )
{
	syscall( G_LINKENTITY, ent );
}

//22.
//SV_UnlinkEntity(VMA(1));
void trap_UnlinkEntity( gentity_t *ent )
{
	syscall( G_UNLINKENTITY, ent );
}

//23.
//return SV_AreaEntities(VMA(1), VMA(2), VMA(3), args[4]);
int trap_EntitiesInBox( const vec3_t mins, const vec3_t maxs, int *list, int maxcount )
{
	return syscall( G_ENTITIES_IN_BOX, mins, maxs, list, maxcount );
}

//24.
//return SV_EntityContact(VMA(1), VMA(2), VMA(3), /* int capsule */ qfalse);
qboolean trap_EntityContact( const vec3_t mins, const vec3_t maxs, const gentity_t *ent )
{
	return syscall( G_ENTITY_CONTACT, mins, maxs, ent );
}

//25.
//return SV_EntityContact(VMA(1), VMA(2), VMA(3), /* int capsule */ qtrue);
qboolean trap_EntityContactCapsule( const vec3_t mins, const vec3_t maxs, const gentity_t *ent )
{
	return syscall( G_ENTITY_CONTACTCAPSULE, mins, maxs, ent );
}

//26.
//SV_Trace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /* int capsule */ qfalse);
void trap_Trace( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask )
{
	syscall( G_TRACE, results, start, mins, maxs, end, passEntityNum, contentmask );
}

//same as 26, call G_TRACE
void trap_TraceNoEnts( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask )
{
	syscall( G_TRACE, results, start, mins, maxs, end, -2, contentmask );
}

//27.
//SV_Trace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], /* int capsule */ qtrue);
void trap_TraceCapsule( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask )
{
	syscall( G_TRACECAPSULE, results, start, mins, maxs, end, passEntityNum, contentmask );
}

//same sa 27, call G_TRACEAPSULE
void trap_TraceCapsuleNoEnts( trace_t *results, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int passEntityNum, int contentmask )
{
	syscall( G_TRACECAPSULE, results, start, mins, maxs, end, -2, contentmask );
}

//28.
//return SV_PointContents(VMA(1), args[2]);
int trap_PointContents( const vec3_t point, int passEntityNum )
{
	return syscall( G_POINT_CONTENTS, point, passEntityNum );
}

//29.
//SV_SetBrushModel(VMA(1), VMA(2));
void trap_SetBrushModel( gentity_t *ent, const char *name )
{
	syscall( G_SET_BRUSH_MODEL, ent, name );
}

//30.
//return SV_inPVS(VMA(1), VMA(2));
qboolean trap_InPVS( const vec3_t p1, const vec3_t p2 )
{
	return syscall( G_IN_PVS, p1, p2 );
}

//31.
//return SV_inPVSIgnorePortals(VMA(1), VMA(2));
qboolean trap_InPVSIgnorePortals( const vec3_t p1, const vec3_t p2 )
{
	return syscall( G_IN_PVS_IGNORE_PORTALS, p1, p2 );
}

//32.
//SV_SetConfigstring(args[1], VMA(2));
void trap_SetConfigstring( int num, const char *string )
{
	syscall( G_SET_CONFIGSTRING, num, string );
}

//33.
//SV_GetConfigstring(args[1], VMA(2), args[3]);
void trap_GetConfigstring( int num, char *buffer, int bufferSize )
{
	syscall( G_GET_CONFIGSTRING, num, buffer, bufferSize );
}

//34.
//SV_SetConfigstringRestrictions( args[1], VMA(2) );
void trap_SetConfigstringRestrictions( int num, const clientList_t *clientList )
{
	syscall( G_SET_CONFIGSTRING_RESTRICTIONS, num, clientList );
}

//35.
//SV_SetUserinfo(args[1], VMA(2));
void trap_SetUserinfo( int num, const char *buffer )
{
	syscall( G_SET_USERINFO, num, buffer );
}

//36.
//SV_GetUserinfo(args[1], VMA(2), args[3]);
void trap_GetUserinfo( int num, char *buffer, int bufferSize )
{
	syscall( G_GET_USERINFO, num, buffer, bufferSize );
}

//37.
//SV_GetServerinfo(VMA(1), args[2]);
void trap_GetServerinfo( char *buffer, int bufferSize )
{
	syscall( G_GET_SERVERINFO, buffer, bufferSize );
}

//38.
//SV_AdjustAreaPortalState(VMA(1), args[2]);
void trap_AdjustAreaPortalState( gentity_t *ent, qboolean open )
{
	syscall( G_ADJUST_AREA_PORTAL_STATE, ent, open );
}

//39.
//return CM_AreasConnected(args[1], args[2]);
qboolean trap_AreasConnected( int area1, int area2 )
{
	return syscall( G_AREAS_CONNECTED, area1, area2 );
}

//40.
//SV_UpdateSharedConfig( args[1], VMA(2) );
void trap_UpdateSharedConfig( unsigned int port, const char *rconpass )
{
	syscall( G_UPDATE_SHARED_CONFIG, port, rconpass );
	return;
}

//41.
//return SV_BotAllocateClient(args[1]);
int trap_BotAllocateClient( int clientNum )
{
	return syscall( G_BOT_ALLOCATE_CLIENT, clientNum );
}

//42.
//SV_BotFreeClient(args[1]);
void trap_BotFreeClient( int clientNum )
{
	syscall( G_BOT_FREE_CLIENT, clientNum );
}

//43.
//SV_GetUsercmd(args[1], VMA(2));
void trap_GetUsercmd( int clientNum, usercmd_t *cmd )
{
	syscall( G_GET_USERCMD, clientNum, cmd );
}

//44.
qboolean trap_GetEntityToken( char *buffer, int bufferSize )
{
	return syscall( G_GET_ENTITY_TOKEN, buffer, bufferSize );
}

//45.
//return Com_RealTime(VMA(1));
int trap_RealTime( qtime_t *qtime )
{
	return syscall( G_REAL_TIME, qtime );
}

//46.
//Q_SnapVector(VMA(1));
void trap_SnapVector( float *v )
{
	syscall( G_SNAPVECTOR, v );
	return;
}

//47.
//SV_MasterGameStat( VMA(1) );
void trap_SendGameStat( const char *data )
{
	syscall( G_SEND_GAMESTAT, data );
	return;
}

//48.
//Cmd_AddCommand( VMA(1), NULL );
void trap_AddCommand( const char *cmdName )
{
	syscall( G_ADDCOMMAND, cmdName );
}

//49.
//Cmd_RemoveCommand( VMA(1) );
void trap_RemoveCommand( const char *cmdName )
{
	syscall( G_REMOVECOMMAND, cmdName );
}

//50.
//return SV_GetTag(args[1], args[2], VMA(3), VMA(4));
qboolean trap_GetTag( int clientNum, int tagFileNumber, char *tagName, orientation_t *ori )
{
	return syscall( G_GETTAG, clientNum, tagFileNumber, tagName, ori );
}

//51.
//return SV_LoadTag(VMA(1));
qboolean trap_LoadTag( const char *filename )
{
	return syscall( G_REGISTERTAG, filename );
}

//52.
//return S_RegisterSound(VMA(1), args[2]);
sfxHandle_t trap_RegisterSound( const char *sample, qboolean compressed )
{
	return syscall( G_REGISTERSOUND, sample, compressed );
}

//53.
//return S_GetSoundLength(args[1]);
int trap_GetSoundLength( sfxHandle_t sfxHandle )
{
	return syscall( G_GET_SOUND_LENGTH, sfxHandle );
}

//54.
//return Parse_AddGlobalDefine( VMA(1) );
int trap_Parse_AddGlobalDefine( const char *define )
{
	return syscall( G_PARSE_ADD_GLOBAL_DEFINE, define );
}

//55.
//return Parse_LoadSourceHandle( VMA(1) );
int trap_Parse_LoadSource( const char *filename )
{
	return syscall( G_PARSE_LOAD_SOURCE, filename );
}

//56.
//return Parse_FreeSourceHandle( args[1] );
int trap_Parse_FreeSource( int handle )
{
	return syscall( G_PARSE_FREE_SOURCE, handle );
}

//57.
//return Parse_ReadTokenHandle( args[1], VMA(2) );
int trap_Parse_ReadToken( int handle, pc_token_t *pc_token )
{
	return syscall( G_PARSE_READ_TOKEN, handle, pc_token );
}

//58.
//return Parse_SourceFileAndLine( args[1], VMA(2), VMA(3) );
int trap_Parse_SourceFileAndLine( int handle, char *filename, int *line )
{
	return syscall( G_PARSE_SOURCE_FILE_AND_LINE, handle, filename, line );
}

//59.
//return SV_BotGetConsoleMessage(args[1], VMA(2), args[3]);
int trap_BotGetServerCommand( int clientNum, char *message, int size )
{
	return syscall( BOT_GET_CONSOLE_MESSAGE, clientNum, message, size );
}

//60.
//CMod_PhysicsAddEntity(VMA(1));
void trap_AddPhysicsEntity( gentity_t *ent )
{
#if defined ( USE_PHYSICS )
	syscall( G_ADD_PHYSICS_ENTITY, ent );
#endif
}

//61
//CMod_PhysicsAddStatic(VMA(1));
void trap_AddPhysicsStatic( gentity_t *ent )
{
#if defined ( USE_PHYSICS )
	syscall( G_ADD_PHYSICS_STATIC, ent );
#endif
}

//62.
//memset(VMA(1), args[2], args[3]);

//63.
//memcpy(VMA(1), VMA(2), args[3]);

//64.
//return (intptr_t)strncpy( VMA( 1 ), VMA( 2 ), args[3] );

//65.
//return FloatAsInt(sin(VMF(1)));

//66.
//return FloatAsInt(cos(VMF(1)));

//67.
//return FloatAsInt(atan2(VMF(1), VMF(2)));

//68.
//return FloatAsInt(sqrt(VMF(1)));

//69.
//AxisMultiply(VMA(1), VMA(2), VMA(3));

//70.
//AngleVectors(VMA(1), VMA(2), VMA(3), VMA(4));

//71.
//PerpendicularVector(VMA(1), VMA(2));

//72.
//return FloatAsInt(floor(VMF(1)));

//73.
//return FloatAsInt(ceil(VMF(1)));

//74.
//SV_SendBinaryMessage(args[1], VMA(2), args[3]);
void trap_SendMessage( int clientNum, char *buf, int buflen )
{
	syscall( G_SENDMESSAGE, clientNum, buf, buflen );
}

//75.
//return SV_BinaryMessageStatus(args[1]);
messageStatus_t trap_MessageStatus( int clientNum )
{
	return syscall( G_MESSAGESTATUS, clientNum );
}

//86.
//return SV_RSAGenMsg( VMA(1), VMA(2), VMA(3) );
int trap_RSA_GenerateMessage( const char *public_key, const char *cleartext, char *encrypted )
{
	return syscall( G_RSA_GENMSG, public_key, cleartext, encrypted );
}

//87.
void trap_QuoteString( const char *str, char *buffer, int size )
{
	syscall( G_QUOTESTRING, str, buffer, size );
}
