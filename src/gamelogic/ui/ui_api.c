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

#include "../../engine/client/ui_api.h"

static intptr_t ( QDECL *syscallVM )( intptr_t arg, ... ) = ( intptr_t ( QDECL * )( intptr_t, ... ) ) - 1;

EXTERN_C Q_EXPORT void dllEntry( intptr_t ( QDECL *syscallptr )( intptr_t arg, ... ) )
{
	syscallVM = syscallptr;
}

int PASSFLOAT( float x )
{
	floatint_t fi;
	fi.f = x;
	return fi.i;
}

void trap_SyscallABIVersion( int major, int minor )
{
        syscallVM( TRAP_VERSION, major, minor );
}

void trap_Cvar_CopyValue_i( const char *in_var, const char *out_var )
{
	int v1 = trap_Cvar_VariableValue( in_var );
	trap_Cvar_Set( out_var, va( "%i", v1 ) );
}

//00.
//Com_Error(ERR_DROP, "%s", (char *)VMA(1));
void NORETURN trap_Error( const char *string )
{
	syscallVM( UI_ERROR, string );
	exit(1); // silence warning
}

//01.
//Com_Printf_(("%s"), (char *)VMA(1));
void trap_Print( const char *string )
{
	syscallVM( UI_PRINT, string );
}

//02.
//return Sys_Milliseconds();
int trap_Milliseconds( void )
{
	return syscallVM( UI_MILLISECONDS );
}

//03.
//Cvar_Register(VMA(1), VMA(2), VMA(3), args[4]);
void trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags )
{
	syscallVM( UI_CVAR_REGISTER, cvar, var_name, value, flags );
}

//04.
//Cvar_Update(VMA(1));
void trap_Cvar_Update( vmCvar_t *cvar )
{
	syscallVM( UI_CVAR_UPDATE, cvar );
}

//05.
//Cvar_Set(VMA(1), VMA(2));
void trap_Cvar_Set( const char *var_name, const char *value )
{
	syscallVM( UI_CVAR_SET, var_name, value );
}

//06.
//return FloatAsInt(Cvar_VariableValue(VMA(1)));
float trap_Cvar_VariableValue( const char *var_name )
{
	floatint_t fi;
	fi.i = syscallVM( UI_CVAR_VARIABLEVALUE, var_name );
	return fi.f;
}

//07.
//Cvar_VariableStringBuffer(VMA(1), VMA(2), args[3]);
void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize )
{
	syscallVM( UI_CVAR_VARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

//08.
//Cvar_VariableStringBuffer(VMA(1), VMA(2), args[3]);
void trap_Cvar_LatchedVariableStringBuffer( const char *var_name, char *buffer, int bufsize )
{
	syscallVM( UI_CVAR_LATCHEDVARIABLESTRINGBUFFER, var_name, buffer, bufsize );
}

//09.
//Cvar_SetValue(VMA(1), VMF(2));
void trap_Cvar_SetValue( const char *var_name, float value )
{
	syscallVM( UI_CVAR_SETVALUE, var_name, PASSFLOAT( value ) );
}

//10.
//Cvar_Reset(VMA(1));
void trap_Cvar_Reset( const char *name )
{
	syscallVM( UI_CVAR_RESET, name );
}

//11.
//Cvar_Get(VMA(1), VMA(2), args[3]);
void trap_Cvar_Create( const char *var_name, const char *var_value, int flags )
{
	syscallVM( UI_CVAR_CREATE, var_name, var_value, flags );
}

//12.
//Cvar_InfoStringBuffer(args[1], VMA(2), args[3]);
void trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize )
{
	syscallVM( UI_CVAR_INFOSTRINGBUFFER, bit, buffer, bufsize );
}

//13.
//return Cmd_Argc();
int trap_Argc( void )
{
	return syscallVM( UI_ARGC );
}

//14.
//Cmd_ArgvBuffer(args[1], VMA(2), args[3]);
void trap_Argv( int n, char *buffer, int bufferLength )
{
	syscallVM( UI_ARGV, n, buffer, bufferLength );
}

//15.
//Cbuf_ExecuteText(args[1], VMA(2));
void trap_Cmd_ExecuteText( int exec_when, const char *text )
{
	syscallVM( UI_CMD_EXECUTETEXT, exec_when, text );
}

//16.
//Cmd_AddCommand(VMA(1), NULL);
void trap_AddCommand( const char *cmdName )
{
	syscallVM( UI_ADDCOMMAND, cmdName );
}

//17.
//return FS_FOpenFileByMode(VMA(1), VMA(2), args[3]);
int trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode )
{
	return syscallVM( UI_FS_FOPENFILE, qpath, f, mode );
}

//18.
//FS_Read(VMA(1), args[2], args[3]);
void trap_FS_Read( void *buffer, int len, fileHandle_t f )
{
	syscallVM( UI_FS_READ, buffer, len, f );
}

//19.
//FS_Write(VMA(1), args[2], args[3]);
void trap_FS_Write( const void *buffer, int len, fileHandle_t f )
{
	syscallVM( UI_FS_WRITE, buffer, len, f );
}

//20.
//FS_FCloseFile(args[1]);
void trap_FS_FCloseFile( fileHandle_t f )
{
	syscallVM( UI_FS_FCLOSEFILE, f );
}

//21.
//return FS_Delete(VMA(1));
int trap_FS_Delete( const char *filename )
{
	return syscallVM( UI_FS_DELETEFILE, filename );
}

//22.
//return FS_GetFileList(VMA(1), VMA(2), VMA(3), args[4]);
int trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize )
{
	return syscallVM( UI_FS_GETFILELIST, path, extension, listbuf, bufsize );
}

//23.
//return FS_Seek(args[1], args[2], args[3]);
int trap_FS_Seek( fileHandle_t f, long offset, int origin )
{
	return syscallVM( UI_FS_SEEK, f, offset, origin );
}

//24.
//return re.RegisterModel(VMA(1));
qhandle_t trap_R_RegisterModel( const char *name )
{
	return syscallVM( UI_R_REGISTERMODEL, name );
}

//25.
//return re.RegisterSkin(VMA(1));
qhandle_t trap_R_RegisterSkin( const char *name )
{
	return syscallVM( UI_R_REGISTERSKIN, name );
}

//26.
//return re.RegisterShader(VMA(1));
qhandle_t trap_R_RegisterShader( const char *name, RegisterShaderFlags_t flags )
{
	return syscallVM( UI_R_REGISTERSHADER, name, flags );
}

//27.
//re.ClearScene();
void trap_R_ClearScene( void )
{
	syscallVM( UI_R_CLEARSCENE );
}

//28.
//re.AddRefEntityToScene(VMA(1));
void trap_R_AddRefEntityToScene( const refEntity_t *re )
{
	syscallVM( UI_R_ADDREFENTITYTOSCENE, re );
}

//29.
//re.AddPolyToScene(args[1], args[2], VMA(3));
void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts )
{
	syscallVM( UI_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}

//30.
//re.AddPolysToScene(args[1], args[2], VMA(3), args[4]);
//Was missing for some unknown reason - even in original ET-GPL source code
void trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys )
{
	syscallVM( UI_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, numPolys );
}

//31.
//re.AddLightToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), args[7], args[8]);
void trap_R_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags )
{
	syscallVM( UI_R_ADDLIGHTTOSCENE, org, PASSFLOAT( radius ), PASSFLOAT( intensity ), PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), hShader, flags );
}

//32.
//re.AddCoronaToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
void trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible )
{
	syscallVM( UI_R_ADDCORONATOSCENE, org, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( scale ), id, visible );
}

//33.
//re.RenderScene(VMA(1));
void trap_R_RenderScene( const refdef_t *fd )
{
	syscallVM( UI_R_RENDERSCENE, fd );
}

//34.
//re.SetColor(VMA(1));
void trap_R_SetColor( const float *rgba )
{
	syscallVM( UI_R_SETCOLOR, rgba );
}

//35.
//re.SetClipRegion( VMA(1) );
void trap_R_SetClipRegion( const float *region )
{
	syscallVM( UI_R_SETCLIPREGION, region );
}

//36.
//re.Add2dPolys(VMA(1), args[2], args[3]);
void trap_R_Add2dPolys( polyVert_t *verts, int numverts, qhandle_t hShader )
{
	syscallVM( UI_R_DRAW2DPOLYS, verts, numverts, hShader );
}

//37.
//re.DrawStretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader )
{
	syscallVM( UI_R_DRAWSTRETCHPIC, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader );
}

//38.
//re.DrawRotatedPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], VMF(10));
void trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle )
{
	syscallVM( UI_R_DRAWROTATEDPIC, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader, PASSFLOAT( angle ) );
}

//39.
//re.ModelBounds(args[1], VMA(2), VMA(3));
void trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs )
{
	syscallVM( UI_R_MODELBOUNDS, model, mins, maxs );
}

//40.
//SCR_UpdateScreen();
void trap_UpdateScreen( void )
{
	syscallVM( UI_UPDATESCREEN );
}

//41.
//return re.LerpTag(VMA(1), VMA(2), VMA(3), args[4]);
int trap_CM_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex )
{
	return syscallVM( UI_CM_LERPTAG, tag, refent, tagName, startIndex );  // NEFVE - SMF - fixed
}

//42.
//return S_RegisterSound(VMA(1), args[2]);
sfxHandle_t trap_S_RegisterSound( const char *sample, qboolean compressed )
{
	int i = syscallVM( UI_S_REGISTERSOUND, sample, compressed );
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
	syscallVM( UI_S_STARTLOCALSOUND, sfx, channelNum );
}

//44.
//S_FadeStreamingSound(VMF(1), args[2], args[3]);
void trap_S_FadeBackgroundTrack( float targetvol, int time, int num )
{
	syscallVM( UI_S_FADESTREAMINGSOUND, PASSFLOAT( targetvol ), time, num );
}

//45.
//S_FadeAllSounds(VMF(1), args[2], args[3]);
void trap_S_FadeAllSound( float targetvol, int time, qboolean stopsound )
{
	syscallVM( UI_S_FADEALLSOUNDS, PASSFLOAT( targetvol ), time, stopsound );
}

//46.
//Key_KeynumToStringBuf(args[1], VMA(2), args[3]);
void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen )
{
	syscallVM( UI_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen );
}

//47.
//Key_GetBindingBuf(args[1], VMA(2), args[3]);
void trap_Key_GetBindingBuf( int keynum, int team, char *buf, int buflen )
{
	syscallVM( UI_KEY_GETBINDINGBUF, keynum, team, buf, buflen );
}

//48.
//Key_SetBinding(args[1], VMA(2));
void trap_Key_SetBinding( int keynum, int team, const char *binding )
{
	syscallVM( UI_KEY_SETBINDING, keynum, team, binding );
}

//50.
//return Key_IsDown(args[1]);
qboolean trap_Key_IsDown( int keynum )
{
	return syscallVM( UI_KEY_ISDOWN, keynum );
}

//51.
//return Key_GetOverstrikeMode();
qboolean trap_Key_GetOverstrikeMode( void )
{
	return syscallVM( UI_KEY_GETOVERSTRIKEMODE );
}

//52.
//Key_SetOverstrikeMode(args[1]);
void trap_Key_SetOverstrikeMode( qboolean state )
{
	syscallVM( UI_KEY_SETOVERSTRIKEMODE, state );
}

//53.
//Key_ClearStates();
void trap_Key_ClearStates( void )
{
	syscallVM( UI_KEY_CLEARSTATES );
}

//54.
//return Key_GetCatcher();
int trap_Key_GetCatcher( void )
{
	return syscallVM( UI_KEY_GETCATCHER );
}

//55.
//Key_SetCatcher(args[1]);
void trap_Key_SetCatcher( int catcher )
{
	syscallVM( UI_KEY_SETCATCHER, catcher );
}

//56.
//GetClipboardData(VMA(1), args[2], args[3]);
void trap_GetClipboardData( char *buf, int bufsize, clipboard_t clip )
{
	syscallVM( UI_GETCLIPBOARDDATA, buf, bufsize, clip );
}

//57.
//GetClientState(VMA(1));
void trap_GetClientState( uiClientState_t *state )
{
	syscallVM( UI_GETCLIENTSTATE, state );
}

//58.
//CL_GetGlconfig(VMA(1));
void trap_GetGlconfig( glconfig_t *glconfig )
{
	syscallVM( UI_GETGLCONFIG, glconfig );
}

//59.
//return GetConfigString(args[1], VMA(2), args[3]);
int trap_GetConfigString( int index, char *buff, int buffsize )
{
	return syscallVM( UI_GETCONFIGSTRING, index, buff, buffsize );
}

//60.
//LAN_LoadCachedServers();
void trap_LAN_LoadCachedServers( void )
{
	syscallVM( UI_LAN_LOADCACHEDSERVERS );
}

//61.
//LAN_SaveServersToCache();
void trap_LAN_SaveCachedServers( void )
{
	syscallVM( UI_LAN_SAVECACHEDSERVERS );
}

//62.
//return LAN_AddServer(args[1], VMA(2), VMA(3));
int trap_LAN_AddServer( int source, const char *name, const char *addr )
{
	return syscallVM( UI_LAN_ADDSERVER, source, name, addr );
}

//63.
//LAN_RemoveServer(args[1], VMA(2));
void trap_LAN_RemoveServer( int source, const char *addr )
{
	syscallVM( UI_LAN_REMOVESERVER, source, addr );
}

//64.
//return LAN_GetPingQueueCount();
int trap_LAN_GetPingQueueCount( void )
{
	return syscallVM( UI_LAN_GETPINGQUEUECOUNT );
}

//65.
//LAN_ClearPing(args[1]);
void trap_LAN_ClearPing( int n )
{
	syscallVM( UI_LAN_CLEARPING, n );
}

//66.
//LAN_GetPing(args[1], VMA(2), args[3], VMA(4));
void trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime )
{
	syscallVM( UI_LAN_GETPING, n, buf, buflen, pingtime );
}

//67.
//LAN_GetPingInfo(args[1], VMA(2), args[3]);
void trap_LAN_GetPingInfo( int n, char *buf, int buflen )
{
	syscallVM( UI_LAN_GETPINGINFO, n, buf, buflen );
}

//68.
//return LAN_GetServerCount(args[1]);
int trap_LAN_GetServerCount( int source )
{
	return syscallVM( UI_LAN_GETSERVERCOUNT, source );
}

//69.
//LAN_GetServerAddressString(args[1], args[2], VMA(3), args[4]);
void trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen )
{
	syscallVM( UI_LAN_GETSERVERADDRESSSTRING, source, n, buf, buflen );
}

//70.
//LAN_GetServerInfo(args[1], args[2], VMA(3), args[4]);
void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen )
{
	syscallVM( UI_LAN_GETSERVERINFO, source, n, buf, buflen );
}

//71.
//return LAN_GetServerPing(args[1], args[2]);
int trap_LAN_GetServerPing( int source, int n )
{
	return syscallVM( UI_LAN_GETSERVERPING, source, n );
}

//72.
//LAN_MarkServerVisible(args[1], args[2], args[3]);
void trap_LAN_MarkServerVisible( int source, int n, qboolean visible )
{
	syscallVM( UI_LAN_MARKSERVERVISIBLE, source, n, visible );
}

//73.
//return LAN_ServerIsVisible(args[1], args[2]);
int trap_LAN_ServerIsVisible( int source, int n )
{
	return syscallVM( UI_LAN_SERVERISVISIBLE, source, n );
}

//74.
//return LAN_UpdateVisiblePings(args[1]);
qboolean trap_LAN_UpdateVisiblePings( int source )
{
	return syscallVM( UI_LAN_UPDATEVISIBLEPINGS, source );
}

//75.
//LAN_ResetPings(args[1]);
void trap_LAN_ResetPings( int n )
{
	syscallVM( UI_LAN_RESETPINGS, n );
}

//76.
//return LAN_GetServerStatus(VMA(1), VMA(2), args[3]);
int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen )
{
	return syscallVM( UI_LAN_SERVERSTATUS, serverAddress, serverStatus, maxLen );
}

//77.
//return LAN_ServerIsInFavoriteList(args[1], args[2]);
qboolean trap_LAN_ServerIsInFavoriteList( int source, int n )
{
	return syscallVM( UI_LAN_SERVERISINFAVORITELIST, source, n );
}

//78.
//return GetNews(args[1]);
qboolean trap_GetNews( qboolean force )
{
	return syscallVM( UI_GETNEWS, force );
}

//79.
//return LAN_CompareServers(args[1], args[2], args[3], args[4], args[5]);
int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 )
{
	return syscallVM( UI_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2 );
}

//80.
//return Hunk_MemoryRemaining();
int trap_MemoryRemaining( void )
{
	return syscallVM( UI_MEMORY_REMAINING );
}

//83.
//re.RegisterFont(VMA(1), args[2], VMA(3));
void trap_R_RegisterFont( const char *fontName, const char *fallbackName, int pointSize, fontMetrics_t *font )
{
	syscallVM( UI_R_REGISTERFONT, fontName, fallbackName, pointSize, font );
}

//93.
//return Parse_AddGlobalDefine(VMA(1));
int trap_Parse_AddGlobalDefine( const char *define )
{
	return syscallVM( UI_PARSE_ADD_GLOBAL_DEFINE, define );
}

//94.
//return Parse_LoadSourceHandle( VMA(1) );
int trap_Parse_LoadSource( const char *filename )
{
	return syscallVM( UI_PARSE_LOAD_SOURCE, filename );
}

//95.
//return Parse_FreeSourceHandle( args[1] );
int trap_Parse_FreeSource( int handle )
{
	return syscallVM( UI_PARSE_FREE_SOURCE, handle );
}

//96.
//return Parse_ReadTokenHandle( args[1], VMA(2) );
int trap_Parse_ReadToken( int handle, pc_token_t *pc_token )
{
	return syscallVM( UI_PARSE_READ_TOKEN, handle, pc_token );
}

//97.
//return Parse_SourceFileAndLine( args[1], VMA(2), VMA(3) );
int trap_Parse_SourceFileAndLine( int handle, char *filename, int *line )
{
	return syscallVM( UI_PARSE_SOURCE_FILE_AND_LINE, handle, filename, line );
}

//98.
//return botlib_export->PC_AddGlobalDefine(VMA(1));
int trap_PC_AddGlobalDefine( const char *define )
{
	return syscallVM( UI_PARSE_ADD_GLOBAL_DEFINE, define );
}

//99.
//botlib_export->PC_RemoveAllGlobalDefines();
int trap_PC_RemoveAllGlobalDefines( void )
{
	return syscallVM( UI_PC_REMOVE_ALL_GLOBAL_DEFINES );
}

//100.
//return botlib_export->PC_LoadSourceHandle(VMA(1));
int trap_PC_LoadSource( const char *filename )
{
	return syscallVM( UI_PARSE_LOAD_SOURCE, filename );
}

//101.
//return botlib_export->PC_FreeSourceHandle(args[1]);
int trap_PC_FreeSource( int handle )
{
	return syscallVM( UI_PARSE_FREE_SOURCE, handle );
}

//102.
//return botlib_export->PC_ReadTokenHandle(args[1], VMA(2));
int trap_PC_ReadToken( int handle, pc_token_t *pc_token )
{
	return syscallVM( UI_PARSE_READ_TOKEN, handle, pc_token );
}

//103.
//return botlib_export->PC_SourceFileAndLine(args[1], VMA(2), VMA(3));
int trap_PC_SourceFileAndLine( int handle, char *filename, int *line )
{
	return syscallVM( UI_PARSE_SOURCE_FILE_AND_LINE, handle, filename, line );
}

//104.
//botlib_export->PC_UnreadLastTokenHandle(args[1]);
int trap_PC_UnReadToken( int handle )
{
	return syscallVM( UI_PC_UNREAD_TOKEN, handle );
}

//105.
//S_StopBackgroundTrack();
void trap_S_StopBackgroundTrack( void )
{
	syscallVM( UI_S_STOPBACKGROUNDTRACK );
}

//106.
//S_StartBackgroundTrack(VMA(1), VMA(2), args[3]);
void trap_S_StartBackgroundTrack( const char *intro, const char *loop )
{
	syscallVM( UI_S_STARTBACKGROUNDTRACK, intro, loop );
}

//107.
//return Com_RealTime(VMA(1));
int trap_RealTime( qtime_t *qtime )
{
	return syscallVM( UI_REAL_TIME, qtime );
}

//108.
//return CIN_PlayCinematic(VMA(1), args[2], args[3], args[4], args[5], args[6]);
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits )
{
	return syscallVM( UI_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits );
}

//109.
//return CIN_StopCinematic(args[1]);
e_status trap_CIN_StopCinematic( int handle )
{
	return (e_status) syscallVM( UI_CIN_STOPCINEMATIC, handle );
}

//110.
//return CIN_RunCinematic(args[1]);
e_status trap_CIN_RunCinematic( int handle )
{
	return (e_status) syscallVM( UI_CIN_RUNCINEMATIC, handle );
}

//111.
//CIN_DrawCinematic(args[1]);
void trap_CIN_DrawCinematic( int handle )
{
	syscallVM( UI_CIN_DRAWCINEMATIC, handle );
}

//112.
//CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h )
{
	syscallVM( UI_CIN_SETEXTENTS, handle, x, y, w, h );
}

//113.
//re.RemapShader(VMA(1), VMA(2), VMA(3));
void trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset )
{
	syscallVM( UI_R_REMAP_SHADER, oldShader, newShader, timeOffset );
}

//119.
//Com_GetHunkInfo(VMA(1), VMA(2));
void trap_GetHunkData( int *hunkused, int *hunkexpected )
{
	syscallVM( UI_GETHUNKDATA, hunkused, hunkexpected );
}

//120.
qhandle_t trap_R_RegisterAnimation( const char *name )
{
	return syscallVM( UI_R_REGISTERANIMATION, name );
}

//121.
int trap_R_BuildSkeleton( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac, qboolean clearOrigin )
{
	return syscallVM( UI_R_BUILDSKELETON, skel, anim, startFrame, endFrame, PASSFLOAT( frac ), clearOrigin );
}

//122.
int trap_R_BlendSkeleton( refSkeleton_t *skel, const refSkeleton_t *blend, float frac )
{
	return syscallVM( UI_R_BLENDSKELETON, skel, blend, PASSFLOAT( frac ) );
}

//123.
int trap_R_BoneIndex( qhandle_t hModel, const char *boneName )
{
	return syscallVM( UI_R_BONEINDEX, hModel, boneName );
}

//124.
int trap_R_AnimNumFrames( qhandle_t hAnim )
{
	return syscallVM( UI_R_ANIMNUMFRAMES, hAnim );
}

int trap_R_AnimFrameRate( qhandle_t hAnim )
{
	return syscallVM( UI_R_ANIMFRAMERATE, hAnim );
}

//125.
void trap_R_Glyph( fontHandle_t font, const char *str, glyphInfo_t *glyph )
{
  syscallVM( UI_R_GLYPH, font, str, glyph );
}

//126.
void trap_R_GlyphChar( fontHandle_t font, int ch, glyphInfo_t *glyph )
{
  syscallVM( UI_R_GLYPHCHAR, font, ch, glyph );
}

//127.
void trap_R_UnregisterFont( fontHandle_t font )
{
  syscallVM( UI_R_UREGISTERFONT, font );
}

//127.
void trap_QuoteString( const char *str, char *buffer, int size )
{
	syscallVM( UI_QUOTESTRING, str, buffer, size );
}

//128.
void trap_Gettext( char *buffer, const char *msgid, int bufferLength )
{
	syscallVM( UI_GETTEXT, buffer, msgid, bufferLength );
}

//129.
void trap_Pgettext( char *buffer, const char *ctxt, const char *msgid, int bufferLength )
{
	syscallVM( UI_PGETTEXT, buffer, ctxt, msgid, bufferLength );
}

void trap_GettextPlural( char *buffer, const char *msgid, const char *msgid2, int number, int bufferLength )
{
	syscallVM( UI_GETTEXT_PLURAL, buffer, msgid, msgid2, number, bufferLength );
}
