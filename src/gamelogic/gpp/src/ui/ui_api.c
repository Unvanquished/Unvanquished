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

#include "../../../../engine/client/ui_api.h"

#define MAX_VA_STRING 32000

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

void trap_Cvar_CopyValue_i( const char *in_var, const char *out_var )
{
	int v1 = trap_Cvar_VariableValue( in_var );
	trap_Cvar_Set( out_var, va( "%i", v1 ) );
}

//00.
//Com_Error(ERR_DROP, "%s", (char *)VMA(1));
void trap_Error( const char *string )
{
	syscall( UI_ERROR, string );
}

//01.
//Com_Printf("%s", (char *)VMA(1));
void trap_Print( const char *string )
{
	syscall( UI_PRINT, string );
}

//02.
//return Sys_Milliseconds();
int trap_Milliseconds( void )
{
	return syscall( UI_MILLISECONDS );
}

//03.
//Cvar_Register(VMA(1), VMA(2), VMA(3), args[4]);
void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags )
{
	syscall( UI_CVAR_REGISTER, cvar, var_name, value, flags );
}

//04.
//Cvar_Update(VMA(1));
void trap_Cvar_Update( vmCvar_t *cvar )
{
	syscall( UI_CVAR_UPDATE, cvar );
}

//05.
//Cvar_Set(VMA(1), VMA(2));
void trap_Cvar_Set( const char *var_name, const char *value )
{
	syscall( UI_CVAR_SET, var_name, value );
}

//06.
//return FloatAsInt(Cvar_VariableValue(VMA(1)));
float trap_Cvar_VariableValue( const char *var_name )
{
	int temp;
	temp = syscall( UI_CVAR_VARIABLEVALUE, var_name );
	return ( * ( float * ) &temp );
}

//07.
//Cvar_VariableStringBuffer(VMA(1), VMA(2), args[3]);
void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize )
{
	syscall( UI_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

//08.
//Cvar_VariableStringBuffer(VMA(1), VMA(2), args[3]);
void trap_Cvar_LatchedVariableStringBuffer( const char *var_name, char *buffer, int bufsize )
{
	syscall( UI_CVAR_LATCHEDVARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

//09.
//Cvar_SetValue(VMA(1), VMF(2));
void trap_Cvar_SetValue( const char *var_name, float value )
{
	syscall( UI_CVAR_SETVALUE, var_name, PASSFLOAT( value ) );
}

//10.
//Cvar_Reset(VMA(1));
void trap_Cvar_Reset( const char *name )
{
	syscall( UI_CVAR_RESET, name );
}

//11.
//Cvar_Get(VMA(1), VMA(2), args[3]);
void trap_Cvar_Create( const char *var_name, const char *var_value, int flags )
{
	syscall( UI_CVAR_CREATE, var_name, var_value, flags );
}

//12.
//Cvar_InfoStringBuffer(args[1], VMA(2), args[3]);
void trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize )
{
	syscall( UI_CVAR_INFOSTRINGBUFFER, bit, buffer, bufsize );
}

//13.
//return Cmd_Argc();
int trap_Argc( void )
{
	return syscall( UI_ARGC );
}

//14.
//Cmd_ArgvBuffer(args[1], VMA(2), args[3]);
void trap_Argv( int n, char *buffer, int bufferLength )
{
	syscall( UI_ARGV, n, buffer, bufferLength );
}

//15.
//Cbuf_ExecuteText(args[1], VMA(2));
void trap_Cmd_ExecuteText( int exec_when, const char *text )
{
	syscall( UI_CMD_EXECUTETEXT, exec_when, text );
}

//16.
//Cmd_AddCommand(VMA(1), NULL);
void trap_AddCommand( const char *cmdName )
{
	syscall( UI_ADDCOMMAND, cmdName );
}

//17.
//return FS_FOpenFileByMode(VMA(1), VMA(2), args[3]);
int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode )
{
	return syscall( UI_FS_FOPENFILE, qpath, f, mode );
}

//18.
//FS_Read2(VMA(1), args[2], args[3]);
void trap_FS_Read( void *buffer, int len, fileHandle_t f )
{
	syscall( UI_FS_READ, buffer, len, f );
}

//19.
//FS_Write(VMA(1), args[2], args[3]);
void trap_FS_Write( const void *buffer, int len, fileHandle_t f )
{
	syscall( UI_FS_WRITE, buffer, len, f );
}

//20.
//FS_FCloseFile(args[1]);
void trap_FS_FCloseFile( fileHandle_t f )
{
	syscall( UI_FS_FCLOSEFILE, f );
}

//21.
//return FS_Delete(VMA(1));
int trap_FS_Delete( const char *filename )
{
	return syscall( UI_FS_DELETEFILE, filename );
}

//22.
//return FS_GetFileList(VMA(1), VMA(2), VMA(3), args[4]);
int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize )
{
	return syscall( UI_FS_GETFILELIST, path, extension, listbuf, bufsize );
}

//23.
//return FS_Seek(args[1], args[2], args[3]);
int trap_FS_Seek( fileHandle_t f, long offset, int origin )
{
	return syscall( UI_FS_SEEK, f, offset, origin );
}

//24.
//return re.RegisterModel(VMA(1));
qhandle_t trap_R_RegisterModel( const char *name )
{
	return syscall( UI_R_REGISTERMODEL, name );
}

//25.
//return re.RegisterSkin(VMA(1));
qhandle_t trap_R_RegisterSkin( const char *name )
{
	return syscall( UI_R_REGISTERSKIN, name );
}

//26.
//return re.RegisterShaderNoMip(VMA(1));
qhandle_t trap_R_RegisterShaderNoMip( const char *name )
{
	return syscall( UI_R_REGISTERSHADERNOMIP, name );
}

//27.
//re.ClearScene();
void trap_R_ClearScene( void )
{
	syscall( UI_R_CLEARSCENE );
}

//28.
//re.AddRefEntityToScene(VMA(1));
void trap_R_AddRefEntityToScene( const refEntity_t *re )
{
	syscall( UI_R_ADDREFENTITYTOSCENE, re );
}

//29.
//re.AddPolyToScene(args[1], args[2], VMA(3));
void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts )
{
	syscall( UI_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}

//30.
//re.AddPolysToScene(args[1], args[2], VMA(3), args[4]);
//Was missing for some unknown reason - even in original ET-GPL source code
void trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys )
{
	syscall( UI_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, numPolys );
}

//31.
//re.AddLightToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), args[7], args[8]);
void trap_R_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags )
{
	syscall( UI_R_ADDLIGHTTOSCENE, org, PASSFLOAT( radius ), PASSFLOAT( intensity ), PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), hShader, flags );
}

//32.
//re.AddCoronaToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
void trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible )
{
	syscall( UI_R_ADDCORONATOSCENE, org, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( scale ), id, visible );
}

//33.
//re.RenderScene(VMA(1));
void trap_R_RenderScene( const refdef_t *fd )
{
	syscall( UI_R_RENDERSCENE, fd );
}

//34.
//re.SetColor(VMA(1));
void trap_R_SetColor( const float *rgba )
{
	syscall( UI_R_SETCOLOR, rgba );
}

//35.
//re.SetClipRegion( VMA(1) );
void trap_R_SetClipRegion( const float *region )
{
	syscall( UI_R_SETCLIPREGION, region );
}

//36.
//re.Add2dPolys(VMA(1), args[2], args[3]);
void trap_R_Add2dPolys( polyVert_t *verts, int numverts, qhandle_t hShader )
{
	syscall( UI_R_DRAW2DPOLYS, verts, numverts, hShader );
}

//37.
//re.DrawStretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader )
{
	syscall( UI_R_DRAWSTRETCHPIC, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader );
}

//38.
//re.DrawRotatedPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], VMF(10));
void trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle )
{
	syscall( UI_R_DRAWROTATEDPIC, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader, PASSFLOAT( angle ) );
}

//39.
//re.ModelBounds(args[1], VMA(2), VMA(3));
void trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs )
{
	syscall( UI_R_MODELBOUNDS, model, mins, maxs );
}

//40.
//SCR_UpdateScreen();
void trap_UpdateScreen( void )
{
	syscall( UI_UPDATESCREEN );
}

//41.
//return re.LerpTag(VMA(1), VMA(2), VMA(3), args[4]);
int trap_CM_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex )
{
	return syscall( UI_CM_LERPTAG, tag, refent, tagName, 0 );  // NEFVE - SMF - fixed
}

//42.
//return S_RegisterSound(VMA(1), args[2]);
sfxHandle_t trap_S_RegisterSound( const char *sample, qboolean compressed )
{
	int i = syscall( UI_S_REGISTERSOUND, sample, qfalse /* compressed */ );
#ifdef DEBUG

	if ( i == 0 )
	{
		Com_Printf( "^1Warning: Failed to load sound: %s\n", sample );
	}

#endif
	return i;
}

//43.
//S_StartLocalSound(args[1], args[2], args[3]);
void trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum )
{
	syscall( UI_S_STARTLOCALSOUND, sfx, channelNum );
}

//44.
//S_FadeStreamingSound(VMF(1), args[2], args[3]);
void trap_S_FadeBackgroundTrack( float targetvol, int time, int num )
{
	syscall( UI_S_FADESTREAMINGSOUND, PASSFLOAT( targetvol ), time, num );
}

//45.
//S_FadeAllSounds(VMF(1), args[2], args[3]);
void trap_S_FadeAllSound( float targetvol, int time, qboolean stopsound )
{
	syscall( UI_S_FADEALLSOUNDS, PASSFLOAT( targetvol ), time, stopsound );
}

//46.
//Key_KeynumToStringBuf(args[1], VMA(2), args[3]);
void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen )
{
	syscall( UI_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen );
}

//47.
//Key_GetBindingBuf(args[1], VMA(2), args[3]);
void trap_Key_GetBindingBuf( int keynum, char *buf, int buflen )
{
	syscall( UI_KEY_GETBINDINGBUF, keynum, buf, buflen );
}

//48.
//Key_SetBinding(args[1], VMA(2));
void trap_Key_SetBinding( int keynum, const char *binding )
{
	syscall( UI_KEY_SETBINDING, keynum, binding );
}

//49.
//Key_GetBindingByString(VMA(1), VMA(2), VMA(3));
void trap_Key_KeysForBinding( const char *binding, int *key1, int *key2 )
{
	syscall( UI_KEY_BINDINGTOKEYS, binding, key1, key2 );
}

//50.
//return Key_IsDown(args[1]);
qboolean trap_Key_IsDown( int keynum )
{
	return syscall( UI_KEY_ISDOWN, keynum );
}

//51.
//return Key_GetOverstrikeMode();
qboolean trap_Key_GetOverstrikeMode( void )
{
	return syscall( UI_KEY_GETOVERSTRIKEMODE );
}

//52.
//Key_SetOverstrikeMode(args[1]);
void trap_Key_SetOverstrikeMode( qboolean state )
{
	syscall( UI_KEY_SETOVERSTRIKEMODE, state );
}

//53.
//Key_ClearStates();
void trap_Key_ClearStates( void )
{
	syscall( UI_KEY_CLEARSTATES );
}

//54.
//return Key_GetCatcher();
int trap_Key_GetCatcher( void )
{
	return syscall( UI_KEY_GETCATCHER );
}

//55.
//Key_SetCatcher(args[1]);
void trap_Key_SetCatcher( int catcher )
{
	syscall( UI_KEY_SETCATCHER, catcher );
}

//56.
//GetClipboardData(VMA(1), args[2], args[3]);
void trap_GetClipboardData( char *buf, int bufsize, clipboard_t clip )
{
	syscall( UI_GETCLIPBOARDDATA, buf, bufsize, clip );
}

//57.
//GetClientState(VMA(1));
void trap_GetClientState( uiClientState_t *state )
{
	syscall( UI_GETCLIENTSTATE, state );
}

//58.
//CL_GetGlconfig(VMA(1));
void trap_GetGlconfig( glconfig_t *glconfig )
{
	syscall( UI_GETGLCONFIG, glconfig );
}

//59.
//return GetConfigString(args[1], VMA(2), args[3]);
int trap_GetConfigString( int index, char *buff, int buffsize )
{
	return syscall( UI_GETCONFIGSTRING, index, buff, buffsize );
}

//60.
//LAN_LoadCachedServers();
void trap_LAN_LoadCachedServers( void )
{
	syscall( UI_LAN_LOADCACHEDSERVERS );
}

//61.
//LAN_SaveServersToCache();
void trap_LAN_SaveCachedServers( void )
{
	syscall( UI_LAN_SAVECACHEDSERVERS );
}

//62.
//return LAN_AddServer(args[1], VMA(2), VMA(3));
int trap_LAN_AddServer( int source, const char *name, const char *addr )
{
	return syscall( UI_LAN_ADDSERVER, source, name, addr );
}

//63.
//LAN_RemoveServer(args[1], VMA(2));
void trap_LAN_RemoveServer( int source, const char *addr )
{
	syscall( UI_LAN_REMOVESERVER, source, addr );
}

//64.
//return LAN_GetPingQueueCount();
int trap_LAN_GetPingQueueCount( void )
{
	return syscall( UI_LAN_GETPINGQUEUECOUNT );
}

//65.
//LAN_ClearPing(args[1]);
void trap_LAN_ClearPing( int n )
{
	syscall( UI_LAN_CLEARPING, n );
}

//66.
//LAN_GetPing(args[1], VMA(2), args[3], VMA(4));
void trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime )
{
	syscall( UI_LAN_GETPING, n, buf, buflen, pingtime );
}

//67.
//LAN_GetPingInfo(args[1], VMA(2), args[3]);
void trap_LAN_GetPingInfo( int n, char *buf, int buflen )
{
	syscall( UI_LAN_GETPINGINFO, n, buf, buflen );
}

//68.
//return LAN_GetServerCount(args[1]);
int trap_LAN_GetServerCount( int source )
{
	return syscall( UI_LAN_GETSERVERCOUNT, source );
}

//69.
//LAN_GetServerAddressString(args[1], args[2], VMA(3), args[4]);
void trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen )
{
	syscall( UI_LAN_GETSERVERADDRESSSTRING, source, n, buf, buflen );
}

//70.
//LAN_GetServerInfo(args[1], args[2], VMA(3), args[4]);
void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen )
{
	syscall( UI_LAN_GETSERVERINFO, source, n, buf, buflen );
}

//71.
//return LAN_GetServerPing(args[1], args[2]);
int trap_LAN_GetServerPing( int source, int n )
{
	return syscall( UI_LAN_GETSERVERPING, source, n );
}

//72.
//LAN_MarkServerVisible(args[1], args[2], args[3]);
void trap_LAN_MarkServerVisible( int source, int n, qboolean visible )
{
	syscall( UI_LAN_MARKSERVERVISIBLE, source, n, visible );
}

//73.
//return LAN_ServerIsVisible(args[1], args[2]);
int trap_LAN_ServerIsVisible( int source, int n )
{
	return syscall( UI_LAN_SERVERISVISIBLE, source, n );
}

//74.
//return LAN_UpdateVisiblePings(args[1]);
qboolean trap_LAN_UpdateVisiblePings( int source )
{
	return syscall( UI_LAN_UPDATEVISIBLEPINGS, source );
}

//75.
//LAN_ResetPings(args[1]);
void trap_LAN_ResetPings( int n )
{
	syscall( UI_LAN_RESETPINGS, n );
}

//76.
//return LAN_GetServerStatus(VMA(1), VMA(2), args[3]);
int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen )
{
	return syscall( UI_LAN_SERVERSTATUS, serverAddress, serverStatus, maxLen );
}

//77.
//return LAN_ServerIsInFavoriteList(args[1], args[2]);
qboolean trap_LAN_ServerIsInFavoriteList( int source, int n )
{
	return syscall( UI_LAN_SERVERISINFAVORITELIST, source, n );
}

//78.
//return GetNews(args[1]);
qboolean trap_GetNews( qboolean force )
{
	return syscall( UI_GETNEWS, force );
}

//79.
//return LAN_CompareServers(args[1], args[2], args[3], args[4], args[5]);
int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 )
{
	return syscall( UI_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2 );
}

//80.
//return Hunk_MemoryRemaining();
int trap_MemoryRemaining( void )
{
	return syscall( UI_MEMORY_REMAINING );
}

//81.
//CLUI_GetCDKey(VMA(1), args[2]);
void trap_GetCDKey( char *buf, int buflen )
{
	syscall( UI_GET_CDKEY, buf, buflen );
}

//82.
//CLUI_SetCDKey(VMA(1));
void trap_SetCDKey( char *buf )
{
	syscall( UI_SET_CDKEY, buf );
}

//83.
//re.RegisterFont(VMA(1), args[2], VMA(3));
void trap_R_RegisterFont( const char *fontName, const char *fallbackName, int pointSize, fontInfo_t *font )
{
	syscall( UI_R_REGISTERFONT, fontName, fallbackName, pointSize, font );
}

//84.
//UI_MEMSET
//return (intptr_t)memset( VMA( 1 ), args[2], args[3] );

//85.
//UI_MEMCPY
//return (intptr_t)memcpy( VMA( 1 ), VMA( 2 ), args[3] );

//86.
//UI_STRNCPY
//return (intptr_t)strncpy( VMA( 1 ), VMA( 2 ), args[3] );

//87.
//UI_SIN
//return FloatAsInt(sin(VMF(1)));

//88.
//UI_COS
//return FloatAsInt(cos(VMF(1)));

//89.
//UI_ATAN2
//return FloatAsInt(atan2(VMF(1), VMF(2)));

//90.
//UI_SQRT
//return FloatAsInt(sqrt(VMF(1)));

//91.
//UI_FLOOR
//return FloatAsInt(floor(VMF(1)));

//92.
//UI_CEIL
//return FloatAsInt(ceil(VMF(1)));

//93.
//return Parse_AddGlobalDefine(VMA(1));
int trap_Parse_AddGlobalDefine( char *define )
{
	return syscall( UI_PARSE_ADD_GLOBAL_DEFINE, define );
}

//94.
//return Parse_LoadSourceHandle( VMA(1) );
int trap_Parse_LoadSource( const char *filename )
{
	return syscall( UI_PARSE_LOAD_SOURCE, filename );
}

//95.
//return Parse_FreeSourceHandle( args[1] );
int trap_Parse_FreeSource( int handle )
{
	return syscall( UI_PARSE_FREE_SOURCE, handle );
}

//96.
//return Parse_ReadTokenHandle( args[1], VMA(2) );
int trap_Parse_ReadToken( int handle, pc_token_t *pc_token )
{
	return syscall( UI_PARSE_READ_TOKEN, handle, pc_token );
}

//97.
//return Parse_SourceFileAndLine( args[1], VMA(2), VMA(3) );
int trap_Parse_SourceFileAndLine( int handle, char *filename, int *line )
{
	return syscall( UI_PARSE_SOURCE_FILE_AND_LINE, handle, filename, line );
}

//98.
//return botlib_export->PC_AddGlobalDefine(VMA(1));
int trap_PC_AddGlobalDefine( char *define )
{
	return syscall( UI_PC_ADD_GLOBAL_DEFINE, define );
}

//99.
//botlib_export->PC_RemoveAllGlobalDefines();
int trap_PC_RemoveAllGlobalDefines( void )
{
	return syscall( UI_PC_REMOVE_ALL_GLOBAL_DEFINES );
}

//100.
//return botlib_export->PC_LoadSourceHandle(VMA(1));
int trap_PC_LoadSource( const char *filename )
{
	return syscall( UI_PC_LOAD_SOURCE, filename );
}

//101.
//return botlib_export->PC_FreeSourceHandle(args[1]);
int trap_PC_FreeSource( int handle )
{
	return syscall( UI_PC_FREE_SOURCE, handle );
}

//102.
//return botlib_export->PC_ReadTokenHandle(args[1], VMA(2));
int trap_PC_ReadToken( int handle, pc_token_t *pc_token )
{
	return syscall( UI_PC_READ_TOKEN, handle, pc_token );
}

//103.
//return botlib_export->PC_SourceFileAndLine(args[1], VMA(2), VMA(3));
int trap_PC_SourceFileAndLine( int handle, char *filename, int *line )
{
	return syscall( UI_PC_SOURCE_FILE_AND_LINE, handle, filename, line );
}

//104.
//botlib_export->PC_UnreadLastTokenHandle(args[1]);
int trap_PC_UnReadToken( int handle )
{
	return syscall( UI_PC_UNREAD_TOKEN, handle );
}

//105.
//S_StopBackgroundTrack();
void trap_S_StopBackgroundTrack( void )
{
	syscall( UI_S_STOPBACKGROUNDTRACK );
}

//106.
//S_StartBackgroundTrack(VMA(1), VMA(2), args[3]);
void trap_S_StartBackgroundTrack( const char *intro, const char *loop )
{
	syscall( UI_S_STARTBACKGROUNDTRACK, intro, loop );
}

//107.
//return Com_RealTime(VMA(1));
int trap_RealTime( qtime_t *qtime )
{
	return syscall( UI_REAL_TIME, qtime );
}

//108.
//return CIN_PlayCinematic(VMA(1), args[2], args[3], args[4], args[5], args[6]);
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits )
{
	return syscall( UI_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits );
}

//109.
//return CIN_StopCinematic(args[1]);
e_status trap_CIN_StopCinematic( int handle )
{
	return syscall( UI_CIN_STOPCINEMATIC, handle );
}

//110.
//return CIN_RunCinematic(args[1]);
e_status trap_CIN_RunCinematic( int handle )
{
	return syscall( UI_CIN_RUNCINEMATIC, handle );
}

//111.
//CIN_DrawCinematic(args[1]);
void trap_CIN_DrawCinematic( int handle )
{
	syscall( UI_CIN_DRAWCINEMATIC, handle );
}

//112.
//CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h )
{
	syscall( UI_CIN_SETEXTENTS, handle, x, y, w, h );
}

//113.
//re.RemapShader(VMA(1), VMA(2), VMA(3));
void trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset )
{
	syscall( UI_R_REMAP_SHADER, oldShader, newShader, timeOffset );
}

//114.
//return CL_GetLimboString(args[1], VMA(2));
qboolean trap_GetLimboString( int index, char *buf )
{
	return syscall( UI_CL_GETLIMBOSTRING, index, buf );
}

//115.
//CL_TranslateString(VMA(1), VMA(2));
char *trap_TranslateString( const char *string )
{
	static char staticbuf[ 2 ][ MAX_VA_STRING ];
	static int  bufcount = 0;
	char        *buf;

	buf = staticbuf[ bufcount++ % 2 ];

#ifdef LOCALIZATION_SUPPORT
	syscall( UI_CL_TRANSLATE_STRING, string, buf );
#else
	Q_strncpyz( buf, string, MAX_VA_STRING );
#endif // LOCALIZATION_SUPPORT
	return buf;
}

//116.
//CL_CheckAutoUpdate();
void trap_CheckAutoUpdate( void )
{
	syscall( UI_CHECKAUTOUPDATE );
}

//117.
//CL_GetAutoUpdate();
void trap_GetAutoUpdate( void )
{
	syscall( UI_GET_AUTOUPDATE );
}

//118.
//CL_OpenURL((const char *)VMA(1));
void trap_openURL( const char *s )
{
	syscall( UI_OPENURL, s );
}

//119.
//Com_GetHunkInfo(VMA(1), VMA(2));
void trap_GetHunkData( int *hunkused, int *hunkexpected )
{
	syscall( UI_GETHUNKDATA, hunkused, hunkexpected );
}

//120.
#if defined( USE_REFENTITY_ANIMATIONSYSTEM )
qhandle_t trap_R_RegisterAnimation( const char *name )
{
	return syscall( UI_R_REGISTERANIMATION, name );
}

//121.
int trap_R_BuildSkeleton( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac, qboolean clearOrigin )
{
	return syscall( UI_R_BUILDSKELETON, skel, anim, startFrame, endFrame, PASSFLOAT( frac ), clearOrigin );
}

//122.
int trap_R_BlendSkeleton( refSkeleton_t *skel, const refSkeleton_t *blend, float frac )
{
	return syscall( UI_R_BLENDSKELETON, skel, blend, PASSFLOAT( frac ) );
}

//123.
int trap_R_BoneIndex( qhandle_t hModel, const char *boneName )
{
	return syscall( UI_R_BONEINDEX, hModel, boneName );
}

//124.
int trap_R_AnimNumFrames( qhandle_t hAnim )
{
	return syscall( UI_R_ANIMNUMFRAMES, hAnim );
}

int trap_R_AnimFrameRate( qhandle_t hAnim )
{
	return syscall( UI_R_ANIMFRAMERATE, hAnim );
}

//125.
void trap_R_Glyph( fontInfo_t *font, const char *str, glyphInfo_t *glyph )
{
  syscall( UI_R_GLYPH, font, str, glyph );
}

//126.
void trap_R_GlyphChar( fontInfo_t *font, int ch, glyphInfo_t *glyph )
{
  syscall( UI_R_GLYPHCHAR, font, ch, glyph );
}

//127.
void trap_R_UnregisterFont( fontInfo_t *font )
{
  syscall( UI_R_UREGISTERFONT, font );
}

#endif
