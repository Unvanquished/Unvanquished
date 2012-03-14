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
		G_LogPrintf( "%s: trap_SendServerCommand( %d, ... ) length exceeds 1022.\n", GAMEVERSION, clientNum );
		G_LogPrintf( "%s: text [%.950s]... truncated\n", GAMEVERSION, text );
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
//SV_SetConfigstring(args[1], VMA(2), qfalse);
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
//return BotImport_DebugPolygonCreate(args[1], args[2], VMA(3));
int trap_DebugPolygonCreate( int color, int numPoints, vec3_t *points )
{
	return syscall( G_DEBUG_POLYGON_CREATE, color, numPoints, points );
}

//46.
//BotImport_DebugPolygonDelete(args[1]);
void trap_DebugPolygonDelete( int id )
{
	syscall( G_DEBUG_POLYGON_DELETE, id );
}

//47.
//return Com_RealTime(VMA(1));
int trap_RealTime( qtime_t *qtime )
{
	return syscall( G_REAL_TIME, qtime );
}

//48.
//Q_SnapVector(VMA(1));
void trap_SnapVector( float *v )
{
	syscall( G_SNAPVECTOR, v );
	return;
}

//49.
//SV_MasterGameStat( VMA(1) );
void trap_SendGameStat( const char *data )
{
	syscall( G_SEND_GAMESTAT, data );
	return;
}

//50.
//Cmd_AddCommand( VMA(1), NULL );
void trap_AddCommand( const char *cmdName )
{
	syscall( G_ADDCOMMAND, cmdName );
}

//51.
//Cmd_RemoveCommand( VMA(1) );
void trap_RemoveCommand( const char *cmdName )
{
	syscall( G_REMOVECOMMAND, cmdName );
}

//52.
//return SV_GetTag(args[1], args[2], VMA(3), VMA(4));
qboolean trap_GetTag( int clientNum, int tagFileNumber, char *tagName, orientation_t *ori )
{
	return syscall( G_GETTAG, clientNum, tagFileNumber, tagName, ori );
}

//53.
//return SV_LoadTag(VMA(1));
qboolean trap_LoadTag( const char *filename )
{
	return syscall( G_REGISTERTAG, filename );
}

//54.
//return S_RegisterSound(VMA(1), args[2]);
sfxHandle_t trap_RegisterSound( const char *sample, qboolean compressed )
{
	return syscall( G_REGISTERSOUND, sample, compressed );
}

//55.
//return S_GetSoundLength(args[1]);
int trap_GetSoundLength( sfxHandle_t sfxHandle )
{
	return syscall( G_GET_SOUND_LENGTH, sfxHandle );
}

//56.
//return Parse_AddGlobalDefine( VMA(1) );
int trap_Parse_AddGlobalDefine( char *define )
{
	return syscall( G_PARSE_ADD_GLOBAL_DEFINE, define );
}

//57.
//return Parse_LoadSourceHandle( VMA(1) );
int trap_Parse_LoadSource( const char *filename )
{
	return syscall( G_PARSE_LOAD_SOURCE, filename );
}

//58.
//return Parse_FreeSourceHandle( args[1] );
int trap_Parse_FreeSource( int handle )
{
	return syscall( G_PARSE_FREE_SOURCE, handle );
}

//59.
//return Parse_ReadTokenHandle( args[1], VMA(2) );
int trap_Parse_ReadToken( int handle, pc_token_t *pc_token )
{
	return syscall( G_PARSE_READ_TOKEN, handle, pc_token );
}

//60.
//return Parse_SourceFileAndLine( args[1], VMA(2), VMA(3) );
int trap_Parse_SourceFileAndLine( int handle, char *filename, int *line )
{
	return syscall( G_PARSE_SOURCE_FILE_AND_LINE, handle, filename, line );
}

//61.
//return SV_BotLibSetup();
int trap_BotLibSetup( void )
{
	return syscall( BOTLIB_SETUP );
}

//62.
//return SV_BotLibShutdown();
int trap_BotLibShutdown( void )
{
	return syscall( BOTLIB_SHUTDOWN );
}

//63.
//return botlib_export->BotLibVarSet(VMA(1), VMA(2));
int trap_BotLibVarSet( char *var_name, char *value )
{
	return syscall( BOTLIB_LIBVAR_SET, var_name, value );
}

//64.
//return botlib_export->BotLibVarGet(VMA(1), VMA(2), args[3]);
int trap_BotLibVarGet( char *var_name, char *value, int size )
{
	return syscall( BOTLIB_LIBVAR_GET, var_name, value, size );
}

//65.
//return botlib_export->PC_AddGlobalDefine(VMA(1));
int trap_BotLibDefine( char *string )
{
	return syscall( BOTLIB_PC_ADD_GLOBAL_DEFINE, string );
}

//66.
//return botlib_export->PC_LoadSourceHandle(VMA(1));
int trap_PC_LoadSource( const char *filename )
{
	return syscall( BOTLIB_PC_LOAD_SOURCE, filename );
}

//67.
//return botlib_export->PC_FreeSourceHandle(args[1]);
int trap_PC_FreeSource( int handle )
{
	return syscall( BOTLIB_PC_FREE_SOURCE, handle );
}

//68.
//return botlib_export->PC_ReadTokenHandle(args[1], VMA(2));
int trap_PC_ReadToken( int handle, pc_token_t *pc_token )
{
	return syscall( BOTLIB_PC_READ_TOKEN, handle, pc_token );
}

//69.
//return botlib_export->PC_SourceFileAndLine(args[1], VMA(2), VMA(3));
int trap_PC_SourceFileAndLine( int handle, char *filename, int *line )
{
	return syscall( BOTLIB_PC_SOURCE_FILE_AND_LINE, handle, filename, line );
}

//70.
//botlib_export->PC_UnreadLastTokenHandle(args[1]);
int trap_PC_UnReadToken( int handle )
{
	return syscall( BOTLIB_PC_UNREAD_TOKEN, handle );
}

//71.
//return botlib_export->BotLibStartFrame(VMF(1));
int trap_BotLibStartFrame( float time )
{
	return syscall( BOTLIB_START_FRAME, PASSFLOAT( time ) );
}

//72.
//return botlib_export->BotLibLoadMap(VMA(1));
int trap_BotLibLoadMap( const char *mapname )
{
	return syscall( BOTLIB_LOAD_MAP, mapname );
}

//73.
//return botlib_export->BotLibUpdateEntity(args[1], VMA(2));
int trap_BotLibUpdateEntity( int ent, void /* struct bot_updateentity_s */ *bue )
{
	return syscall( BOTLIB_UPDATENTITY, ent, bue );
}

//74.
//return botlib_export->Test(args[1], VMA(2), VMA(3), VMA(4));
int trap_BotLibTest( int parm0, char *parm1, vec3_t parm2, vec3_t parm3 )
{
	return syscall( BOTLIB_TEST, parm0, parm1, parm2, parm3 );
}

//75.
//return SV_BotGetSnapshotEntity(args[1], args[2]);
int trap_BotGetSnapshotEntity( int clientNum, int sequence )
{
	return syscall( BOTLIB_GET_SNAPSHOT_ENTITY, clientNum, sequence );
}

//76.
//return SV_BotGetConsoleMessage(args[1], VMA(2), args[3]);
int trap_BotGetServerCommand( int clientNum, char *message, int size )
{
	return syscall( BOTLIB_GET_CONSOLE_MESSAGE, clientNum, message, size );
}

//77.
//SV_ClientThink(&svs.clients[args[1]], VMA(2));
void trap_BotUserCommand( int clientNum, usercmd_t *ucmd )
{
	syscall( BOTLIB_USER_COMMAND, clientNum, ucmd );
}

//78.
//botlib_export->aas.AAS_EntityInfo(args[1], VMA(2));
void trap_AAS_EntityInfo( int entnum, void /* struct aas_entityinfo_s */ *info )
{
	syscall( BOTLIB_AAS_ENTITY_INFO, entnum, info );
}

//79.
//return botlib_export->aas.AAS_Initialized();
int trap_AAS_Initialized( void )
{
	return syscall( BOTLIB_AAS_INITIALIZED );
}

//80.
//botlib_export->aas.AAS_PresenceTypeBoundingBox(args[1], VMA(2), VMA(3));
void trap_AAS_PresenceTypeBoundingBox( int presencetype, vec3_t mins, vec3_t maxs )
{
	syscall( BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX, presencetype, mins, maxs );
}

//81.
//return FloatAsInt(botlib_export->aas.AAS_Time());
float trap_AAS_Time( void )
{
	int temp;
	temp = syscall( BOTLIB_AAS_TIME );
	return ( * ( float * ) &temp );
}

//82.
//botlib_export->aas.AAS_SetCurrentWorld(args[1]);
void trap_AAS_SetCurrentWorld( int index )
{
	syscall( BOTLIB_AAS_SETCURRENTWORLD, index );
}

//83.
//return botlib_export->aas.AAS_PointAreaNum(VMA(1));
int trap_AAS_PointAreaNum( vec3_t point )
{
	return syscall( BOTLIB_AAS_POINT_AREA_NUM, point );
}

//84.
//return botlib_export->aas.AAS_TraceAreas(VMA(1), VMA(2), VMA(3), VMA(4), args[5]);
int trap_AAS_TraceAreas( vec3_t start, vec3_t end, int *areas, vec3_t *points, int maxareas )
{
	return syscall( BOTLIB_AAS_TRACE_AREAS, start, end, areas, points, maxareas );
}

//85.
//return botlib_export->aas.AAS_BBoxAreas(VMA(1), VMA(2), VMA(3), args[4]);
int trap_AAS_BBoxAreas( vec3_t absmins, vec3_t absmaxs, int *areas, int maxareas )
{
	return syscall( BOTLIB_AAS_BBOX_AREAS, absmins, absmaxs, areas, maxareas );
}

//86.
//botlib_export->aas.AAS_AreaCenter(args[1], VMA(2));
void trap_AAS_AreaCenter( int areanum, vec3_t center )
{
	syscall( BOTLIB_AAS_AREA_CENTER, areanum, center );
}

//87.
//return botlib_export->aas.AAS_AreaWaypoint(args[1], VMA(2));
qboolean trap_AAS_AreaWaypoint( int areanum, vec3_t center )
{
	return syscall( BOTLIB_AAS_AREA_WAYPOINT, areanum, center );
}

//88.
//return botlib_export->aas.AAS_PointContents(VMA(1));
int trap_AAS_PointContents( vec3_t point )
{
	return syscall( BOTLIB_AAS_POINT_CONTENTS, point );
}

//89.
//return botlib_export->aas.AAS_NextBSPEntity(args[1]);
int trap_AAS_NextBSPEntity( int ent )
{
	return syscall( BOTLIB_AAS_NEXT_BSP_ENTITY, ent );
}

//90.
//return botlib_export->aas.AAS_ValueForBSPEpairKey(args[1], VMA(2), VMA(3), args[4]);
int trap_AAS_ValueForBSPEpairKey( int ent, char *key, char *value, int size )
{
	return syscall( BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY, ent, key, value, size );
}

//91.
//return botlib_export->aas.AAS_VectorForBSPEpairKey(args[1], VMA(2), VMA(3));
int trap_AAS_VectorForBSPEpairKey( int ent, char *key, vec3_t v )
{
	return syscall( BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY, ent, key, v );
}

//92.
//return botlib_export->aas.AAS_FloatForBSPEpairKey(args[1], VMA(2), VMA(3));
int trap_AAS_FloatForBSPEpairKey( int ent, char *key, float *value )
{
	return syscall( BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY, ent, key, value );
}

//93.
//return botlib_export->aas.AAS_IntForBSPEpairKey(args[1], VMA(2), VMA(3));
int trap_AAS_IntForBSPEpairKey( int ent, char *key, int *value )
{
	return syscall( BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY, ent, key, value );
}

//94.
//return botlib_export->aas.AAS_AreaReachability(args[1]);
int trap_AAS_AreaReachability( int areanum )
{
	return syscall( BOTLIB_AAS_AREA_REACHABILITY, areanum );
}

//95.
//return botlib_export->aas.AAS_AreaLadder(args[1]);
int trap_AAS_AreaLadder( int areanum )
{
	return syscall( BOTLIB_AAS_AREA_LADDER, areanum );
}

//96.
//return botlib_export->aas.AAS_AreaTravelTimeToGoalArea(args[1], VMA(2), args[3], args[4]);
int trap_AAS_AreaTravelTimeToGoalArea( int areanum, vec3_t origin, int goalareanum, int travelflags )
{
	return syscall( BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA, areanum, origin, goalareanum, travelflags );
}

//97.
//return botlib_export->aas.AAS_Swimming(VMA(1));
int trap_AAS_Swimming( vec3_t origin )
{
	return syscall( BOTLIB_AAS_SWIMMING, origin );
}

//98.
//return botlib_export->aas.AAS_PredictClientMovement(VMA(1), args[2], VMA(3), args[4], args[5], VMA(6), VMA(7), args[8], args[9], VMF(10), args[11], args[12], args[13]);
int trap_AAS_PredictClientMovement( void /* struct aas_clientmove_s */ *move, int entnum, vec3_t origin, int presencetype, int onground, vec3_t velocity, vec3_t cmdmove, int cmdframes, int maxframes, float frametime, int stopevent, int stopareanum, int visualize )
{
	return syscall( BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT, move, entnum, origin, presencetype, onground, velocity, cmdmove, cmdframes, maxframes, PASSFLOAT( frametime ), stopevent, stopareanum, visualize );
}

//99.
//botlib_export->aas.AAS_RT_ShowRoute(VMA(1), args[2], args[3]);
void trap_AAS_RT_ShowRoute( vec3_t srcpos, int  srcnum, int destnum )
{
	syscall( BOTLIB_AAS_RT_SHOWROUTE, srcpos, srcnum, destnum );
}

//100.
//return botlib_export->aas.AAS_NearestHideArea(args[1], VMA(2), args[3], args[4], VMA(5), args[6], args[7], VMF(8), VMA(9));
int trap_AAS_NearestHideArea( int srcnum, vec3_t origin, int areanum, int enemynum, vec3_t enemyorigin, int enemyareanum, int travelflags, float maxdist, vec3_t distpos )
{
	return syscall( BOTLIB_AAS_NEARESTHIDEAREA, srcnum, origin, areanum, enemynum, enemyorigin, enemyareanum, travelflags, PASSFLOAT( maxdist ), distpos );
}

//101.
//return botlib_export->aas.AAS_ListAreasInRange(VMA(1), args[2], VMF(3), args[4], VMA(5), args[6]);
int trap_AAS_ListAreasInRange( vec3_t srcpos, int srcarea, float range, int travelflags, float **outareas, int maxareas )
{
	return syscall( BOTLIB_AAS_LISTAREASINRANGE, srcpos, srcarea, PASSFLOAT( range ), travelflags, outareas, maxareas );
}

//102.
//return botlib_export->aas.AAS_AvoidDangerArea(VMA(1), args[2], VMA(3), args[4], VMF(5), args[6]);
int trap_AAS_AvoidDangerArea( vec3_t srcpos, int srcarea, vec3_t dangerpos, int dangerarea, float range, int travelflags )
{
	return syscall( BOTLIB_AAS_AVOIDDANGERAREA, srcpos, srcarea, dangerpos, dangerarea, PASSFLOAT( range ), travelflags );
}

//103.
//return botlib_export->aas.AAS_Retreat(VMA(1), args[2], VMA(3), args[4], VMA(5), args[6], VMF(7), VMF(8), args[9]);
int trap_AAS_Retreat( int *dangerSpots, int dangerSpotCount, vec3_t srcpos, int srcarea, vec3_t dangerpos, int dangerarea, float range, float dangerRange, int travelflags )
{
	return syscall( BOTLIB_AAS_RETREAT, dangerSpots, dangerSpotCount, srcpos, srcarea, dangerpos, dangerarea, PASSFLOAT( range ), PASSFLOAT( dangerRange ), travelflags );
}

//104.
//botlib_export->aas.AAS_SetAASBlockingEntity(VMA(1), VMA(2), args[3]);
void trap_AAS_SetAASBlockingEntity( vec3_t absmin, vec3_t absmax, int blocking )
{
	syscall( BOTLIB_AAS_SETAASBLOCKINGENTITY, absmin, absmax, blocking );
}

//105.
//botlib_export->aas.AAS_RecordTeamDeathArea(VMA(1), args[2], args[3], args[4], args[5]);
void trap_AAS_RecordTeamDeathArea( vec3_t srcpos, int srcarea, int team, int teamCount, int travelflags )
{
	syscall( BOTLIB_AAS_RECORDTEAMDEATHAREA, srcpos, srcarea, team, teamCount, travelflags );
}

//106.
//botlib_export->ea.EA_Say(args[1], VMA(2));
void trap_EA_Say( int client, char *str )
{
	syscall( BOTLIB_EA_SAY, client, str );
}

//107.
//botlib_export->ea.EA_SayTeam(args[1], VMA(2));
void trap_EA_SayTeam( int client, char *str )
{
	syscall( BOTLIB_EA_SAY_TEAM, client, str );
}

//108.
//botlib_export->ea.EA_UseItem(args[1], VMA(2));
void trap_EA_UseItem( int client, char *it )
{
	syscall( BOTLIB_EA_USE_ITEM, client, it );
}

//109.
//botlib_export->ea.EA_DropItem(args[1], VMA(2));
void trap_EA_DropItem( int client, char *it )
{
	syscall( BOTLIB_EA_DROP_ITEM, client, it );
}

//110.
//botlib_export->ea.EA_UseInv(args[1], VMA(2));
void trap_EA_UseInv( int client, char *inv )
{
	syscall( BOTLIB_EA_USE_INV, client, inv );
}

//111.
//botlib_export->ea.EA_DropInv(args[1], VMA(2));
void trap_EA_DropInv( int client, char *inv )
{
	syscall( BOTLIB_EA_DROP_INV, client, inv );
}

//112.
//botlib_export->ea.EA_Gesture(args[1]);
void trap_EA_Gesture( int client )
{
	syscall( BOTLIB_EA_GESTURE, client );
}

//113.
//botlib_export->ea.EA_Command(args[1], VMA(2));
void trap_EA_Command( int client, char *command )
{
	syscall( BOTLIB_EA_COMMAND, client, command );
}

//114.
//botlib_export->ea.EA_SelectWeapon(args[1], args[2]);
void trap_EA_SelectWeapon( int client, int weapon )
{
	syscall( BOTLIB_EA_SELECT_WEAPON, client, weapon );
}

//115.
//botlib_export->ea.EA_Talk(args[1]);
void trap_EA_Talk( int client )
{
	syscall( BOTLIB_EA_TALK, client );
}

//116.
//botlib_export->ea.EA_Attack(args[1]);
void trap_EA_Attack( int client )
{
	syscall( BOTLIB_EA_ATTACK, client );
}

//117.
//botlib_export->ea.EA_Reload(args[1]);
void trap_EA_Reload( int client )
{
	syscall( BOTLIB_EA_RELOAD, client );
}

//118.
//botlib_export->ea.EA_Use(args[1]);
void trap_EA_Activate( int client )
{
	syscall( BOTLIB_EA_USE, client );
}

//119.
//botlib_export->ea.EA_Respawn(args[1]);
void trap_EA_Respawn( int client )
{
	syscall( BOTLIB_EA_RESPAWN, client );
}

//120.
//botlib_export->ea.EA_Jump(args[1]);
void trap_EA_Jump( int client )
{
	syscall( BOTLIB_EA_JUMP, client );
}

//121.
//botlib_export->ea.EA_DelayedJump(args[1]);
void trap_EA_DelayedJump( int client )
{
	syscall( BOTLIB_EA_DELAYED_JUMP, client );
}

//122.
//botlib_export->ea.EA_Crouch(args[1]);
void trap_EA_Crouch( int client )
{
	syscall( BOTLIB_EA_CROUCH, client );
}

//123.
//botlib_export->ea.EA_Walk(args[1]);
void trap_EA_Walk( int client )
{
	syscall( BOTLIB_EA_WALK, client );
}

//124.
//botlib_export->ea.EA_MoveUp(args[1]);
void trap_EA_MoveUp( int client )
{
	syscall( BOTLIB_EA_MOVE_UP, client );
}

//125.
//botlib_export->ea.EA_MoveDown(args[1]);
void trap_EA_MoveDown( int client )
{
	syscall( BOTLIB_EA_MOVE_DOWN, client );
}

//126.
//botlib_export->ea.EA_MoveForward(args[1]);
void trap_EA_MoveForward( int client )
{
	syscall( BOTLIB_EA_MOVE_FORWARD, client );
}

//127.
//botlib_export->ea.EA_MoveBack(args[1]);
void trap_EA_MoveBack( int client )
{
	syscall( BOTLIB_EA_MOVE_BACK, client );
}

//128.
//botlib_export->ea.EA_MoveLeft(args[1]);
void trap_EA_MoveLeft( int client )
{
	syscall( BOTLIB_EA_MOVE_LEFT, client );
}

//129.
//botlib_export->ea.EA_MoveRight(args[1]);
void trap_EA_MoveRight( int client )
{
	syscall( BOTLIB_EA_MOVE_RIGHT, client );
}

//130.
//botlib_export->ea.EA_Move(args[1], VMA(2), VMF(3));
void trap_EA_Move( int client, vec3_t dir, float speed )
{
	syscall( BOTLIB_EA_MOVE, client, dir, PASSFLOAT( speed ) );
}

//131.
//botlib_export->ea.EA_View(args[1], VMA(2));
void trap_EA_View( int client, vec3_t viewangles )
{
	syscall( BOTLIB_EA_VIEW, client, viewangles );
}

//132.
//botlib_export->ea.EA_Prone(args[1]);
void trap_EA_Prone( int client )
{
	syscall( BOTLIB_EA_PRONE, client );
}

//133.
//botlib_export->ea.EA_EndRegular(args[1], VMF(2));
void trap_EA_EndRegular( int client, float thinktime )
{
	syscall( BOTLIB_EA_END_REGULAR, client, PASSFLOAT( thinktime ) );
}

//134.
//botlib_export->ea.EA_GetInput(args[1], VMF(2), VMA(3));
void trap_EA_GetInput( int client, float thinktime, void /* struct bot_input_s */ *input )
{
	syscall( BOTLIB_EA_GET_INPUT, client, PASSFLOAT( thinktime ), input );
}

//135.
//botlib_export->ea.EA_ResetInput(args[1], VMA(2));
void trap_EA_ResetInput( int client, void *init )
{
	syscall( BOTLIB_EA_RESET_INPUT, client, init );
}

//136.
//return botlib_export->ai.BotLoadCharacter(VMA(1), args[2]);
int trap_BotLoadCharacter( char *charfile, int skill )
{
	return syscall( BOTLIB_AI_LOAD_CHARACTER, charfile, skill );
}

//137.
//botlib_export->ai.BotFreeCharacter(args[1]);
void trap_BotFreeCharacter( int character )
{
	syscall( BOTLIB_AI_FREE_CHARACTER, character );
}

//138.
//return FloatAsInt(botlib_export->ai.Characteristic_Float(args[1], args[2]));
float trap_Characteristic_Float( int character, int index )
{
	int temp;
	temp = syscall( BOTLIB_AI_CHARACTERISTIC_FLOAT, character, index );
	return ( * ( float * ) &temp );
}

//139.
//return FloatAsInt(botlib_export->ai.Characteristic_BFloat(args[1], args[2], VMF(3), VMF(4)));
float trap_Characteristic_BFloat( int character, int index, float min, float max )
{
	int temp;
	temp = syscall( BOTLIB_AI_CHARACTERISTIC_BFLOAT, character, index, PASSFLOAT( min ), PASSFLOAT( max ) );
	return ( * ( float * ) &temp );
}

//140.
//return botlib_export->ai.Characteristic_Integer(args[1], args[2]);
int trap_Characteristic_Integer( int character, int index )
{
	return syscall( BOTLIB_AI_CHARACTERISTIC_INTEGER, character, index );
}

//141.
//return botlib_export->ai.Characteristic_BInteger(args[1], args[2], args[3], args[4]);
int trap_Characteristic_BInteger( int character, int index, int min, int max )
{
	return syscall( BOTLIB_AI_CHARACTERISTIC_BINTEGER, character, index, min, max );
}

//142.
//botlib_export->ai.Characteristic_String(args[1], args[2], VMA(3), args[4]);
void trap_Characteristic_String( int character, int index, char *buf, int size )
{
	syscall( BOTLIB_AI_CHARACTERISTIC_STRING, character, index, buf, size );
}

//143.
//return botlib_export->ai.BotAllocChatState();
int trap_BotAllocChatState( void )
{
	return syscall( BOTLIB_AI_ALLOC_CHAT_STATE );
}

//144.
//botlib_export->ai.BotFreeChatState(args[1]);
void trap_BotFreeChatState( int handle )
{
	syscall( BOTLIB_AI_FREE_CHAT_STATE, handle );
}

//145.
//botlib_export->ai.BotQueueConsoleMessage(args[1], args[2], VMA(3));
void trap_BotQueueConsoleMessage( int chatstate, int type, char *message )
{
	syscall( BOTLIB_AI_QUEUE_CONSOLE_MESSAGE, chatstate, type, message );
}

//146.
//botlib_export->ai.BotRemoveConsoleMessage(args[1], args[2]);
void trap_BotRemoveConsoleMessage( int chatstate, int handle )
{
	syscall( BOTLIB_AI_REMOVE_CONSOLE_MESSAGE, chatstate, handle );
}

//147.
//return botlib_export->ai.BotNextConsoleMessage(args[1], VMA(2));
int trap_BotNextConsoleMessage( int chatstate, void /* struct bot_consolemessage_s */ *cm )
{
	return syscall( BOTLIB_AI_NEXT_CONSOLE_MESSAGE, chatstate, cm );
}

//148.
//return botlib_export->ai.BotNumConsoleMessages(args[1]);
int trap_BotNumConsoleMessages( int chatstate )
{
	return syscall( BOTLIB_AI_NUM_CONSOLE_MESSAGE, chatstate );
}

//149.
//botlib_export->ai.BotInitialChat(args[1], VMA(2), args[3], VMA(4), VMA(5), VMA(6), VMA(7), VMA(8), VMA(9), VMA(10), VMA(11));
void trap_BotInitialChat( int chatstate, char *type, int mcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 )
{
	syscall( BOTLIB_AI_INITIAL_CHAT, chatstate, type, mcontext, var0, var1, var2, var3, var4, var5, var6, var7 );
}

//150.
//return botlib_export->ai.BotNumInitialChats(args[1], VMA(2));
int     trap_BotNumInitialChats( int chatstate, char *type )
{
	return syscall( BOTLIB_AI_NUM_INITIAL_CHATS, chatstate, type );
}

//151.
//return botlib_export->ai.BotReplyChat(args[1], VMA(2), args[3], args[4], VMA(5), VMA(6), VMA(7), VMA(8), VMA(9), VMA(10), VMA(11), VMA(12));
int trap_BotReplyChat( int chatstate, char *message, int mcontext, int vcontext, char *var0, char *var1, char *var2, char *var3, char *var4, char *var5, char *var6, char *var7 )
{
	return syscall( BOTLIB_AI_REPLY_CHAT, chatstate, message, mcontext, vcontext, var0, var1, var2, var3, var4, var5, var6, var7 );
}

//152.
//return botlib_export->ai.BotChatLength(args[1]);
int trap_BotChatLength( int chatstate )
{
	return syscall( BOTLIB_AI_CHAT_LENGTH, chatstate );
}

//153.
//botlib_export->ai.BotEnterChat(args[1], args[2], args[3]);
void trap_BotEnterChat( int chatstate, int client, int sendto )
{
	//disabled
	return;
	syscall( BOTLIB_AI_ENTER_CHAT, chatstate, client, sendto );
}

//154.
//botlib_export->ai.BotGetChatMessage(args[1], VMA(2), args[3]);
void trap_BotGetChatMessage( int chatstate, char *buf, int size )
{
	syscall( BOTLIB_AI_GET_CHAT_MESSAGE, chatstate, buf, size );
}

//155.
//return botlib_export->ai.StringContains(VMA(1), VMA(2), args[3]);
int trap_StringContains( char *str1, char *str2, int casesensitive )
{
	return syscall( BOTLIB_AI_STRING_CONTAINS, str1, str2, casesensitive );
}

//156.
//return botlib_export->ai.BotFindMatch(VMA(1), VMA(2), args[3]);
int trap_BotFindMatch( char *str, void /* struct bot_match_s */ *match, unsigned long int context )
{
	return syscall( BOTLIB_AI_FIND_MATCH, str, match, context );
}

//157.
//botlib_export->ai.BotMatchVariable(VMA(1), args[2], VMA(3), args[4]);
void trap_BotMatchVariable( void /* struct bot_match_s */ *match, int variable, char *buf, int size )
{
	syscall( BOTLIB_AI_MATCH_VARIABLE, match, variable, buf, size );
}

//158.
//botlib_export->ai.UnifyWhiteSpaces(VMA(1));
void trap_UnifyWhiteSpaces( char *string )
{
	syscall( BOTLIB_AI_UNIFY_WHITE_SPACES, string );
}

//159.
//botlib_export->ai.BotReplaceSynonyms(VMA(1), args[2]);
void trap_BotReplaceSynonyms( char *string, unsigned long int context )
{
	syscall( BOTLIB_AI_REPLACE_SYNONYMS, string, context );
}

//160.
//return botlib_export->ai.BotLoadChatFile(args[1], VMA(2), VMA(3));
int trap_BotLoadChatFile( int chatstate, char *chatfile, char *chatname )
{
	return syscall( BOTLIB_AI_LOAD_CHAT_FILE, chatstate, chatfile, chatname );
}

//161.
//botlib_export->ai.BotSetChatGender(args[1], args[2]);
void trap_BotSetChatGender( int chatstate, int gender )
{
	syscall( BOTLIB_AI_SET_CHAT_GENDER, chatstate, gender );
}

//162.
//botlib_export->ai.BotSetChatName(args[1], VMA(2));
void trap_BotSetChatName( int chatstate, char *name )
{
	syscall( BOTLIB_AI_SET_CHAT_NAME, chatstate, name );
}

//162.
//botlib_export->ai.BotResetGoalState(args[1]);
void trap_BotResetGoalState( int goalstate )
{
	syscall( BOTLIB_AI_RESET_GOAL_STATE, goalstate );
}

//163.
//botlib_export->ai.BotResetAvoidGoals(args[1]);
void trap_BotResetAvoidGoals( int goalstate )
{
	syscall( BOTLIB_AI_RESET_AVOID_GOALS, goalstate );
}

//164.
//botlib_export->ai.BotRemoveFromAvoidGoals(args[1], args[2]);
void trap_BotRemoveFromAvoidGoals( int goalstate, int number )
{
	syscall( BOTLIB_AI_REMOVE_FROM_AVOID_GOALS, goalstate, number );
}

//165.
//botlib_export->ai.BotPushGoal(args[1], VMA(2));
void trap_BotPushGoal( int goalstate, void /* struct bot_goal_s */ *goal )
{
	syscall( BOTLIB_AI_PUSH_GOAL, goalstate, goal );
}

//166.
//botlib_export->ai.BotPopGoal(args[1]);
void trap_BotPopGoal( int goalstate )
{
	syscall( BOTLIB_AI_POP_GOAL, goalstate );
}

//167.
//botlib_export->ai.BotEmptyGoalStack(args[1]);
void trap_BotEmptyGoalStack( int goalstate )
{
	syscall( BOTLIB_AI_EMPTY_GOAL_STACK, goalstate );
}

//168.
//botlib_export->ai.BotDumpAvoidGoals(args[1]);
void trap_BotDumpAvoidGoals( int goalstate )
{
	syscall( BOTLIB_AI_DUMP_AVOID_GOALS, goalstate );
}

//169.
//botlib_export->ai.BotDumpGoalStack(args[1]);
void trap_BotDumpGoalStack( int goalstate )
{
	syscall( BOTLIB_AI_DUMP_GOAL_STACK, goalstate );
}

//170.
//botlib_export->ai.BotGoalName(args[1], VMA(2), args[3]);
void trap_BotGoalName( int number, char *name, int size )
{
	syscall( BOTLIB_AI_GOAL_NAME, number, name, size );
}

//171.
//return botlib_export->ai.BotGetTopGoal(args[1], VMA(2));
int trap_BotGetTopGoal( int goalstate, void /* struct bot_goal_s */ *goal )
{
	return syscall( BOTLIB_AI_GET_TOP_GOAL, goalstate, goal );
}

//172.
//return botlib_export->ai.BotGetSecondGoal(args[1], VMA(2));
int trap_BotGetSecondGoal( int goalstate, void /* struct bot_goal_s */ *goal )
{
	return syscall( BOTLIB_AI_GET_SECOND_GOAL, goalstate, goal );
}

//173.
//return botlib_export->ai.BotChooseLTGItem(args[1], VMA(2), VMA(3), args[4]);
int trap_BotChooseLTGItem( int goalstate, vec3_t origin, int *inventory, int travelflags )
{
	return syscall( BOTLIB_AI_CHOOSE_LTG_ITEM, goalstate, origin, inventory, travelflags );
}

//174.
//return botlib_export->ai.BotChooseNBGItem(args[1], VMA(2), VMA(3), args[4], VMA(5), VMF(6));
int trap_BotChooseNBGItem( int goalstate, vec3_t origin, int *inventory, int travelflags, void /* struct bot_goal_s */ *ltg, float maxtime )
{
	return syscall( BOTLIB_AI_CHOOSE_NBG_ITEM, goalstate, origin, inventory, travelflags, ltg, PASSFLOAT( maxtime ) );
}

//175.
//return botlib_export->ai.BotTouchingGoal(VMA(1), VMA(2));
int trap_BotTouchingGoal( vec3_t origin, void /* struct bot_goal_s */ *goal )
{
	return syscall( BOTLIB_AI_TOUCHING_GOAL, origin, goal );
}

//176.
//return botlib_export->ai.BotItemGoalInVisButNotVisible(args[1], VMA(2), VMA(3), VMA(4));
int trap_BotItemGoalInVisButNotVisible( int viewer, vec3_t eye, vec3_t viewangles, void /* struct bot_goal_s */ *goal )
{
	return syscall( BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE, viewer, eye, viewangles, goal );
}

//177.
//return botlib_export->ai.BotGetLevelItemGoal(args[1], VMA(2), VMA(3));
int trap_BotGetLevelItemGoal( int index, char *classname, void /* struct bot_goal_s */ *goal )
{
	return syscall( BOTLIB_AI_GET_LEVEL_ITEM_GOAL, index, classname, goal );
}

//178.
//return botlib_export->ai.BotGetNextCampSpotGoal(args[1], VMA(2));
int trap_BotGetNextCampSpotGoal( int num, void /* struct bot_goal_s */ *goal )
{
	return syscall( BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL, num, goal );
}

//179.
//return botlib_export->ai.BotGetMapLocationGoal(VMA(1), VMA(2));
int trap_BotGetMapLocationGoal( char *name, void /* struct bot_goal_s */ *goal )
{
	return syscall( BOTLIB_AI_GET_MAP_LOCATION_GOAL, name, goal );
}

//180.
//return FloatAsInt(botlib_export->ai.BotAvoidGoalTime(args[1], args[2]));
float trap_BotAvoidGoalTime( int goalstate, int number )
{
	int temp;
	temp = syscall( BOTLIB_AI_AVOID_GOAL_TIME, goalstate, number );
	return ( * ( float * ) &temp );
}

//181.
//botlib_export->ai.BotInitLevelItems();
void trap_BotInitLevelItems( void )
{
	syscall( BOTLIB_AI_INIT_LEVEL_ITEMS );
}

//182.
//botlib_export->ai.BotUpdateEntityItems();
void trap_BotUpdateEntityItems( void )
{
	syscall( BOTLIB_AI_UPDATE_ENTITY_ITEMS );
}

//183.
//return botlib_export->ai.BotLoadItemWeights(args[1], VMA(2));
int trap_BotLoadItemWeights( int goalstate, char *filename )
{
	return syscall( BOTLIB_AI_LOAD_ITEM_WEIGHTS, goalstate, filename );
}

//184.
//botlib_export->ai.BotFreeItemWeights(args[1]);
void trap_BotFreeItemWeights( int goalstate )
{
	syscall( BOTLIB_AI_FREE_ITEM_WEIGHTS, goalstate );
}

//185.
//botlib_export->ai.BotInterbreedGoalFuzzyLogic(args[1], args[2], args[3]);
void trap_BotInterbreedGoalFuzzyLogic( int parent1, int parent2, int child )
{
	syscall( BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC, parent1, parent2, child );
}

//186.
//botlib_export->ai.BotSaveGoalFuzzyLogic(args[1], VMA(2));
void trap_BotSaveGoalFuzzyLogic( int goalstate, char *filename )
{
	syscall( BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC, goalstate, filename );
}

//187.
//botlib_export->ai.BotMutateGoalFuzzyLogic(args[1], VMF(2));
void trap_BotMutateGoalFuzzyLogic( int goalstate, float range )
{
	syscall( BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC, goalstate, range );
}

//188.
//return botlib_export->ai.BotAllocGoalState(args[1]);
int trap_BotAllocGoalState( int state )
{
	return syscall( BOTLIB_AI_ALLOC_GOAL_STATE, state );
}

//189.
//botlib_export->ai.BotFreeGoalState(args[1]);
void trap_BotFreeGoalState( int handle )
{
	syscall( BOTLIB_AI_FREE_GOAL_STATE, handle );
}

//190.
//botlib_export->ai.BotResetMoveState(args[1]);
void trap_BotResetMoveState( int movestate )
{
	syscall( BOTLIB_AI_RESET_MOVE_STATE, movestate );
}

//191.
//botlib_export->ai.BotMoveToGoal(VMA(1), args[2], VMA(3), args[4]);
void trap_BotMoveToGoal( void /* struct bot_moveresult_s */ *result, int movestate, void /* struct bot_goal_s */ *goal, int travelflags )
{
	syscall( BOTLIB_AI_MOVE_TO_GOAL, result, movestate, goal, travelflags );
}

//192.
//return botlib_export->ai.BotMoveInDirection(args[1], VMA(2), VMF(3), args[4]);
int trap_BotMoveInDirection( int movestate, vec3_t dir, float speed, int type )
{
	return syscall( BOTLIB_AI_MOVE_IN_DIRECTION, movestate, dir, PASSFLOAT( speed ), type );
}

//193.
//botlib_export->ai.BotResetAvoidReach(args[1]);
void trap_BotResetAvoidReach( int movestate )
{
	syscall( BOTLIB_AI_RESET_AVOID_REACH, movestate );
}

//194.
//botlib_export->ai.BotResetLastAvoidReach(args[1]);
void trap_BotResetLastAvoidReach( int movestate )
{
	syscall( BOTLIB_AI_RESET_LAST_AVOID_REACH, movestate );
}

//195.
//return botlib_export->ai.BotReachabilityArea(VMA(1), args[2]);
int trap_BotReachabilityArea( vec3_t origin, int testground )
{
	return syscall( BOTLIB_AI_REACHABILITY_AREA, origin, testground );
}

//196.
//return botlib_export->ai.BotMovementViewTarget(args[1], VMA(2), args[3], VMF(4), VMA(5));
int trap_BotMovementViewTarget( int movestate, void /* struct bot_goal_s */ *goal, int travelflags, float lookahead, vec3_t target )
{
	return syscall( BOTLIB_AI_MOVEMENT_VIEW_TARGET, movestate, goal, travelflags, PASSFLOAT( lookahead ), target );
}

//197.
//return botlib_export->ai.BotPredictVisiblePosition(VMA(1), args[2], VMA(3), args[4], VMA(5));
int trap_BotPredictVisiblePosition( vec3_t origin, int areanum, void /* struct bot_goal_s */ *goal, int travelflags, vec3_t target )
{
	return syscall( BOTLIB_AI_PREDICT_VISIBLE_POSITION, origin, areanum, goal, travelflags, target );
}

//198.
//return botlib_export->ai.BotAllocMoveState();
int trap_BotAllocMoveState( void )
{
	return syscall( BOTLIB_AI_ALLOC_MOVE_STATE );
}

//199.
//botlib_export->ai.BotFreeMoveState(args[1]);
void trap_BotFreeMoveState( int handle )
{
	syscall( BOTLIB_AI_FREE_MOVE_STATE, handle );
}

//200.
//botlib_export->ai.BotInitMoveState(args[1], VMA(2));
void trap_BotInitMoveState( int handle, void /* struct bot_initmove_s */ *initmove )
{
	syscall( BOTLIB_AI_INIT_MOVE_STATE, handle, initmove );
}

//201.
//botlib_export->ai.BotInitAvoidReach(args[1]);
void trap_BotInitAvoidReach( int handle )
{
	syscall( BOTLIB_AI_INIT_AVOID_REACH, handle );
}

//202.
//return botlib_export->ai.BotChooseBestFightWeapon(args[1], VMA(2));
int trap_BotChooseBestFightWeapon( int weaponstate, int *inventory )
{
	return syscall( BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON, weaponstate, inventory );
}

//203.
//botlib_export->ai.BotGetWeaponInfo(args[1], args[2], VMA(3));
void trap_BotGetWeaponInfo( int weaponstate, int weapon, void /* struct weaponinfo_s */ *weaponinfo )
{
	syscall( BOTLIB_AI_GET_WEAPON_INFO, weaponstate, weapon, weaponinfo );
}

//204.
//return botlib_export->ai.BotLoadWeaponWeights(args[1], VMA(2));
int trap_BotLoadWeaponWeights( int weaponstate, char *filename )
{
	return syscall( BOTLIB_AI_LOAD_WEAPON_WEIGHTS, weaponstate, filename );
}

//205.
//return botlib_export->ai.BotAllocWeaponState();
int trap_BotAllocWeaponState( void )
{
	return syscall( BOTLIB_AI_ALLOC_WEAPON_STATE );
}

//206.
//botlib_export->ai.BotFreeWeaponState(args[1]);
void trap_BotFreeWeaponState( int weaponstate )
{
	syscall( BOTLIB_AI_FREE_WEAPON_STATE, weaponstate );
}

//207.
//botlib_export->ai.BotResetWeaponState(args[1])
void trap_BotResetWeaponState( int weaponstate )
{
	syscall( BOTLIB_AI_RESET_WEAPON_STATE, weaponstate );
}

//208.
//return botlib_export->ai.GeneticParentsAndChildSelection(args[1], VMA(2), VMA(3), VMA(4), VMA(5));
int trap_GeneticParentsAndChildSelection( int numranks, float *ranks, int *parent1, int *parent2, int *child )
{
	return syscall( BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION, numranks, ranks, parent1, parent2, child );
}

//209.
//CMod_PhysicsAddEntity(VMA(1));
void trap_AddPhysicsEntity( gentity_t *ent )
{
#if defined ( USE_PHYSICS )
	syscall( G_ADD_PHYSICS_ENTITY, ent );
#endif
}

//210
//CMod_PhysicsAddStatic(VMA(1));
void trap_AddPhysicsStatic( gentity_t *ent )
{
#if defined ( USE_PHYSICS )
	syscall( G_ADD_PHYSICS_STATIC, ent );
#endif
}

//211.
//memset(VMA(1), args[2], args[3]);

//212.
//memcpy(VMA(1), VMA(2), args[3]);

//213.
//return (intptr_t)strncpy( VMA( 1 ), VMA( 2 ), args[3] );

//214.
//return FloatAsInt(sin(VMF(1)));

//215.
//return FloatAsInt(cos(VMF(1)));

//216.
//return FloatAsInt(atan2(VMF(1), VMF(2)));

//217.
//return FloatAsInt(sqrt(VMF(1)));

//218.
//AxisMultiply(VMA(1), VMA(2), VMA(3));

//219.
//AngleVectors(VMA(1), VMA(2), VMA(3), VMA(4));

//220.
//PerpendicularVector(VMA(1), VMA(2));

//221.
//return FloatAsInt(floor(VMF(1)));

//222.
//return FloatAsInt(ceil(VMF(1)));

//223.
//SV_SendBinaryMessage(args[1], VMA(2), args[3]);
void trap_SendMessage( int clientNum, char *buf, int buflen )
{
	syscall( G_SENDMESSAGE, clientNum, buf, buflen );
}

//224.
//return SV_BinaryMessageStatus(args[1]);
messageStatus_t trap_MessageStatus( int clientNum )
{
	return syscall( G_MESSAGESTATUS, clientNum );
}

#if defined( ET_MYSQL )
//225.
//return OW_RunQuery( VMA(1) );
int trap_SQL_RunQuery( const char *query )
{
	return syscall( G_SQL_RUNQUERY, query );
}

//226.
//OW_FinishQuery( args[1] );
void trap_SQL_FinishQuery( int queryid )
{
	syscall( G_SQL_FINISHQUERY, queryid );
	return;
}

//227.
//return OW_NextRow( args[1] );
qboolean trap_SQL_NextRow( int queryid )
{
	return syscall( G_SQL_NEXTROW, queryid );
}

//228.
//return OW_RowCount( args[1] );
int trap_SQL_RowCount( int queryid )
{
	return syscall( G_SQL_ROWCOUNT, queryid );
}

//229.
//OW_GetFieldByID( args[1], args[2], VMA(3), args[4]  );
void trap_SQL_GetFieldbyID( int queryid, int fieldid, char *buffer, int len )
{
	syscall( G_SQL_GETFIELDBYID, queryid, fieldid, buffer, len );
	return;
}

//230
//OW_GetFieldByName( args[1], VMA(2), VMA(3), args[4] );
void trap_SQL_GetFieldbyName( int queryid, const char *name, char *buffer, int len )
{
	syscall( G_SQL_GETFIELDBYNAME, queryid, name, buffer, len );
	return;
}

//231.
//return OW_GetFieldByID_int( args[1], args[2] );
int trap_SQL_GetFieldbyID_int( int queryid, int fieldid )
{
	return syscall( G_SQL_GETFIELDBYID_INT, queryid, fieldid );
}

//232.
//return OW_GetFieldByName_int( args[1], VMA(2) );
int trap_SQL_GetFieldbyName_int( int queryid, const char *name )
{
	return syscall( G_SQL_GETFIELDBYNAME_INT, queryid, name );
}

//233.
//return OW_FieldCount( args[1] );
int trap_SQL_FieldCount( int queryid )
{
	return syscall( G_SQL_FIELDCOUNT, queryid );
}

//234.
//OW_CleanString( VMA(1), VMA(2), args[3] );
void trap_SQL_CleanString( const char *in, char *out, int len )
{
	syscall( G_SQL_CLEANSTRING, in, out, len );
	return;
}

#endif

//235.
//return SV_RSAGenMsg( VMA(1), VMA(2), VMA(3) );
int trap_RSA_GenerateMessage( const char *public_key, char *cleartext, char *encrypted )
{
	return syscall( G_RSA_GENMSG, public_key, cleartext, encrypted );
}
