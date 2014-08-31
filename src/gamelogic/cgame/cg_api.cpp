/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2014, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/

#include "cg_local.h"

#include "../shared/VMMain.h"
#include "../shared/CommonProxies.h"

// Symbols required by the shqred VMMqin code

int VM::VM_API_VERSION = CGAME_API_VERSION;

void VM::VMInit() {
    // Nothing to do right now
}

void VM::VMHandleSyscall(uint32_t id, IPC::Reader reader) {
    // Nothing here, TODO
}

// Definition of the VM->Engine calls

#define syscallVM(...) 0

int trap_GetDemoState( void )
{
}

int trap_GetDemoPos( void )
{
}

qboolean trap_FS_LoadPak( const char *pak )
{
}

void trap_EscapedArgs( char *buffer, int bufferLength )
{
}

void trap_LiteralArgs( char *buffer, int bufferLength )
{
}

float trap_Cvar_VariableValue( const char *var_name )
{
}

//23.
//CL_AddReliableCommand(VMA(1));
void trap_SendClientCommand( const char *s )
{
    syscallVM( CG_SENDCLIENTCOMMAND, s );
}

//24.
//SCR_UpdateScreen();
void trap_UpdateScreen( void )
{
    syscallVM( CG_UPDATESCREEN );
}

//38.
//return re.MarkFragments(args[1], VMA(2), VMA(3), args[4], VMA(5), args[6], VMA(7));
int trap_CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer )
{
    return syscallVM( CG_CM_MARKFRAGMENTS, numPoints, points, projection, maxPoints, pointBuffer, maxFragments, fragmentBuffer );
}

//39.
//re.ProjectDecal(args[1], args[2], VMA(3), VMA(4), VMA(5), args[6], args[7]);
void trap_R_ProjectDecal( qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime )
{
    syscallVM( CG_R_PROJECTDECAL, hShader, numPoints, points, projection, color, lifeTime, fadeTime );
}

//40.
//re.ClearDecals();
void trap_R_ClearDecals( void )
{
    syscallVM( CG_R_CLEARDECALS );
}

//41.
//S_StartSound(VMA(1), args[2], args[3], args[4], args[5]);
void trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx )
{
    syscallVM( CG_S_STARTSOUND, origin, entityNum, entchannel, sfx, 127 );
}

//43.
//S_StartLocalSound(args[1], args[2], args[3]);
void trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum )
{
    syscallVM( CG_S_STARTLOCALSOUND, sfx, channelNum, 127 );
}

//44.
//S_ClearLoopingSounds();
//S_ClearLoopingSounds(args[1]);
void trap_S_ClearLoopingSounds( qboolean killall )
{
    syscallVM( CG_S_CLEARLOOPINGSOUNDS, killall );
}

//45.
// if(args[1] == 0) {
//	S_ClearSounds(qtrue, qfalse);
//} else if(args[1] == 1) {
//	S_ClearSounds(qtrue, qtrue);
//}
void trap_S_ClearSounds( qboolean killmusic )
{
    syscallVM( CG_S_CLEARSOUNDS, killmusic );
}

//46.
//S_AddLoopingSound(VMA(1), VMA(2), args[3], args[4], args[5], args[6]);
//S_AddLoopingSound( args[1], VMA(2), VMA(3), args[4] );
void trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
    syscallVM( CG_S_ADDLOOPINGSOUND, entityNum, origin, velocity, sfx );
}

//47.
//S_AddRealLoopingSound(VMA(1), VMA(2), args[3], args[4], args[5], args[6]);
//S_AddRealLoopingSound( args[1], VMA(2), VMA(3), args[4] );
void trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
    syscallVM( CG_S_ADDREALLOOPINGSOUND, entityNum, origin, velocity, sfx );
}

//48.
//S_StopLoopingSound( args[1] );
void trap_S_StopLoopingSound( int entityNum )
{
    syscallVM( CG_S_STOPLOOPINGSOUND, entityNum );
}

//49.
//S_StopEntStreamingSound(args[1]);
void trap_S_StopStreamingSound( int entityNum )
{
    syscallVM( CG_S_STOPSTREAMINGSOUND, entityNum );
}

//50.
//S_UpdateEntityPosition(args[1], VMA(2));
void trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
    syscallVM( CG_S_UPDATEENTITYPOSITION, entityNum, origin );
}

//54.
//S_Respatialize(args[1], VMA(2), VMA(3), args[4]);
void trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[ 3 ], int inwater )
{
    syscallVM( CG_S_RESPATIALIZE, entityNum, origin, axis, inwater );
}

//55.
//return S_RegisterSound(VMA(1), args[2]);
sfxHandle_t trap_S_RegisterSound( const char *sample, qboolean compressed )
{
    //CG_DrawInformation(qtrue);
    Q_UNUSED(compressed);
    return syscallVM( CG_S_REGISTERSOUND, sample, qfalse /* compressed */ );
}

//56.
//S_StartBackgroundTrack(VMA(1), VMA(2), args[3]);
//S_StartBackgroundTrack(VMA(1), VMA(2));
void trap_S_StartBackgroundTrack( const char *intro, const char *loop )
{
    syscallVM( CG_S_STARTBACKGROUNDTRACK, intro, loop );
}

//57.
//S_FadeStreamingSound(VMF(1), args[2], args[3]);
void trap_S_FadeBackgroundTrack( float targetvol, int time, int num )
{
    syscallVM( CG_S_FADESTREAMINGSOUND, PASSFLOAT( targetvol ), time, num );
}

//58.
//return S_StartStreamingSound(VMA(1), VMA(2), args[3], args[4], args[5]);
int trap_S_StartStreamingSound( const char *intro, const char *loop, int entnum, int channel, int attenuation )
{
    return syscallVM( CG_S_STARTSTREAMINGSOUND, intro, loop, entnum, channel, attenuation );
}

//59.
//re.LoadWorld(VMA(1));
void trap_R_LoadWorldMap( const char *mapname )
{
    //CG_DrawInformation(qtrue);

    syscallVM( CG_R_LOADWORLDMAP, mapname );
}

//60.
//return re.RegisterModel(VMA(1));
qhandle_t trap_R_RegisterModel( const char *name )
{
    //CG_DrawInformation(qtrue);

    return syscallVM( CG_R_REGISTERMODEL, name );
}

//61.
//return re.RegisterSkin(VMA(1));
qhandle_t trap_R_RegisterSkin( const char *name )
{
    //CG_DrawInformation(qtrue);

    return syscallVM( CG_R_REGISTERSKIN, name );
}

//62.
//return re.GetSkinModel(args[1], VMA(2), VMA(3));
qboolean trap_R_GetSkinModel( qhandle_t skinid, const char *type, char *name )
{
    return syscallVM( CG_R_GETSKINMODEL, skinid, type, name );
}

//63.
//return re.GetShaderFromModel(args[1], args[2], args[3]);
qhandle_t trap_R_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap )
{
    return syscallVM( CG_R_GETMODELSHADER, modelid, surfnum, withlightmap );
}

//64.
//return re.RegisterShader(VMA(1), args[2]);
qhandle_t trap_R_RegisterShader( const char *name, RegisterShaderFlags_t flags )
{
    //CG_DrawInformation(qtrue);

    return syscallVM( CG_R_REGISTERSHADER, name, flags );
}

//65.
//re.RegisterFont(VMA(1), args[2], VMA(3));
void trap_R_RegisterFont( const char *fontName, const char *fallbackName, int pointSize, fontMetrics_t *font )
{
    //CG_DrawInformation(qtrue);

    /**/
    syscallVM( CG_R_REGISTERFONT, fontName, fallbackName, pointSize, font );
}

//68.
//re.ClearScene();
void trap_R_ClearScene( void )
{
    syscallVM( CG_R_CLEARSCENE );
}

//69.
//re.AddRefEntityToScene(VMA(1));
void trap_R_AddRefEntityToScene( const refEntity_t *re )
{
    syscallVM( CG_R_ADDREFENTITYTOSCENE, re );
}

//70.
//re.AddRefLightToScene(VMA(1));
void trap_R_AddRefLightToScene( const refLight_t *light )
{
    syscallVM( CG_R_ADDREFLIGHTSTOSCENE, light );
}

//71.
//re.AddPolyToScene(args[1], args[2], VMA(3));
void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts )
{
    syscallVM( CG_R_ADDPOLYTOSCENE, hShader, numVerts, verts );
}

//72.
//re.AddPolysToScene(args[1], args[2], VMA(3), args[4]);
void trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys )
{
    syscallVM( CG_R_ADDPOLYSTOSCENE, hShader, numVerts, verts, numPolys );
}

//73.
//re.AddPolyBufferToScene(VMA(1));
void trap_R_AddPolyBufferToScene( polyBuffer_t *pPolyBuffer )
{
    syscallVM( CG_R_ADDPOLYBUFFERTOSCENE, pPolyBuffer );
}

//74.
//re.AddLightToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), args[7], args[8]);
void trap_R_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags )
{
    syscallVM( CG_R_ADDLIGHTTOSCENE, org, PASSFLOAT( radius ), PASSFLOAT( intensity ), PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), hShader, flags );
}

//75.
//re.AddAdditiveLightToScene( VMA(1), VMF(2), VMF(3), VMF(4), VMF(5) );
void trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b )
{
    syscallVM( CG_R_ADDADDITIVELIGHTTOSCENE, org, PASSFLOAT( intensity ), PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ) );
}

//76.
//return FS_Seek( args[1], args[2], args[3] );
void trap_GS_FS_Seek( fileHandle_t f, long offset, fsOrigin_t origin )
{
    syscallVM( CG_FS_SEEK, f, offset, origin );
}

//77.
//re.AddCoronaToScene(VMA(1), VMF(2), VMF(3), VMF(4), VMF(5), args[6], args[7]);
void trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible )
{
    syscallVM( CG_R_ADDCORONATOSCENE, org, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( scale ), id, visible );
}

//78.
//re.SetFog(args[1], args[2], args[3], VMF(4), VMF(5), VMF(6), VMF(7));
void trap_R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density )
{
    syscallVM( CG_R_SETFOG, fogvar, var1, var2, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( density ) );
}

//79.
//re.SetGlobalFog(args[1], args[2], VMF(3), VMF(4), VMF(5), VMF(6));
void trap_R_SetGlobalFog( qboolean restore, int duration, float r, float g, float b, float depthForOpaque )
{
    syscallVM( CG_R_SETGLOBALFOG, restore, duration, PASSFLOAT( r ), PASSFLOAT( g ), PASSFLOAT( b ), PASSFLOAT( depthForOpaque ) );
}

//80.
//re.RenderScene(VMA(1));
void trap_R_RenderScene( const refdef_t *fd )
{
    syscallVM( CG_R_RENDERSCENE, fd );
}

//81.
//re.SaveViewParms();
void trap_R_SaveViewParms( void )
{
    syscallVM( CG_R_SAVEVIEWPARMS );
}

//82.
//re.RestoreViewParms();
void trap_R_RestoreViewParms( void )
{
    syscallVM( CG_R_RESTOREVIEWPARMS );
}

//83.
//re.SetColor(VMA(1));
void trap_R_SetColor( const float *rgba )
{
    syscallVM( CG_R_SETCOLOR, rgba );
}

//84.
//re.SetClipRegion( VMA(1) );
void trap_R_SetClipRegion( const float *region )
{
    syscallVM( CG_R_SETCLIPREGION, region );
}

//85.
//re.DrawStretchPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9]);
void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader )
{
    syscallVM( CG_R_DRAWSTRETCHPIC, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader );
}

//86.
//re.DrawRotatedPic(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], VMF(10));
void trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle )
{
    syscallVM( CG_R_DRAWROTATEDPIC, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader, PASSFLOAT( angle ) );
}

//87.
//re.DrawStretchPicGradient(VMF(1), VMF(2), VMF(3), VMF(4), VMF(5), VMF(6), VMF(7), VMF(8), args[9], VMA(10), args[11]);
void trap_R_DrawStretchPicGradient( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType )
{
    syscallVM( CG_R_DRAWSTRETCHPIC_GRADIENT, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( w ), PASSFLOAT( h ), PASSFLOAT( s1 ), PASSFLOAT( t1 ), PASSFLOAT( s2 ), PASSFLOAT( t2 ), hShader, gradientColor, gradientType );
}

//88.
//re.Add2dPolys(VMA(1), args[2], args[3]);
void trap_R_Add2dPolys( polyVert_t *verts, int numverts, qhandle_t hShader )
{
    syscallVM( CG_R_DRAW2DPOLYS, verts, numverts, hShader );
}

//89.
//re.ModelBounds(args[1], VMA(2), VMA(3));
void trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs )
{
    syscallVM( CG_R_MODELBOUNDS, model, mins, maxs );
}

//90.
//return re.LerpTag(VMA(1), VMA(2), VMA(3), args[4]);
int trap_R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex )
{
    return syscallVM( CG_R_LERPTAG, tag, refent, tagName, startIndex );
}

//91.
//CL_GetGlconfig(VMA(1));
void trap_GetGlconfig( glconfig_t *glconfig )
{
    syscallVM( CG_GETGLCONFIG, glconfig );
}

//92.
//CL_GetGameState(VMA(1));
void trap_GetGameState( gameState_t *gamestate )
{
    syscallVM( CG_GETGAMESTATE, gamestate );
}

void trap_GetClientState( cgClientState_t *cstate )
{
    syscallVM( CG_GETCLIENTSTATE, cstate );
}

//93.
//CL_GetCurrentSnapshotNumber(VMA(1), VMA(2));
void trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime )
{
    syscallVM( CG_GETCURRENTSNAPSHOTNUMBER, snapshotNumber, serverTime );
}

//94.
//return CL_GetSnapshot(args[1], VMA(2));
qboolean trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot )
{
    return syscallVM( CG_GETSNAPSHOT, snapshotNumber, snapshot );
}

//95.
//return CL_GetServerCommand(args[1]);
qboolean trap_GetServerCommand( int serverCommandNumber )
{
    return syscallVM( CG_GETSERVERCOMMAND, serverCommandNumber );
}

//96.
//return CL_GetCurrentCmdNumber();
int trap_GetCurrentCmdNumber( void )
{
    return syscallVM( CG_GETCURRENTCMDNUMBER );
}

//97.
//return CL_GetUserCmd(args[1], VMA(2));
qboolean trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd )
{
    return syscallVM( CG_GETUSERCMD, cmdNumber, ucmd );
}

//98.
//CL_SetUserCmdValue(args[1], args[2], VMF(3), args[4]);
void trap_SetUserCmdValue( int stateValue, int flags, float sensitivityScale, int mpIdentClient )
{
    syscallVM( CG_SETUSERCMDVALUE, stateValue, flags, PASSFLOAT( sensitivityScale ), mpIdentClient );
}

//99.
//CL_SetClientLerpOrigin(VMF(1), VMF(2), VMF(3));
void trap_SetClientLerpOrigin( float x, float y, float z )
{
    syscallVM( CG_SETCLIENTLERPORIGIN, PASSFLOAT( x ), PASSFLOAT( y ), PASSFLOAT( z ) );
}

//100.
//return Hunk_MemoryRemaining();
int trap_MemoryRemaining( void )
{
    return syscallVM( CG_MEMORY_REMAINING );
}

//101.
//return Key_IsDown(args[1]);
qboolean trap_Key_IsDown( int keynum )
{
    return syscallVM( CG_KEY_ISDOWN, keynum );
}

//102.
//return Key_GetCatcher();
int trap_Key_GetCatcher( void )
{
    return syscallVM( CG_KEY_GETCATCHER );
}

//103.
//Key_SetCatcher(args[1]);
void trap_Key_SetCatcher( int catcher )
{
    syscallVM( CG_KEY_SETCATCHER, catcher );
}

//104.
//return Key_GetKey(VMA(1));
int trap_Key_GetKey( const char *binding )
{
    return syscallVM( CG_KEY_GETKEY, binding );
}

//105.
//return Key_GetOverstrikeMode();
qboolean trap_Key_GetOverstrikeMode( void )
{
    return syscallVM( CG_KEY_GETOVERSTRIKEMODE );
}

//106.
//Key_SetOverstrikeMode(args[1]);
void trap_Key_SetOverstrikeMode( qboolean state )
{
    syscallVM( CG_KEY_SETOVERSTRIKEMODE, state );
}

//107.
//return (int)memset(VMA(1), args[2], args[3]);

//108.
//return (int)memcpy(VMA(1), VMA(2), args[3]);

//109.
//return (int)strncpy(VMA(1), VMA(2), args[3]);

//110
//return FloatAsInt(sin(VMF(1)));

//111
//return FloatAsInt(cos(VMF(1)));

//112
//return FloatAsInt(atan2(VMF(1), VMF(2)));

//113
//return FloatAsInt(sqrt(VMF(1)));

//114
//return FloatAsInt(floor(VMF(1)));

//115
//return FloatAsInt(ceil(VMF(1)));

//116
//return FloatAsInt(acos(VMF(1)));

//117
//return botlib_export->PC_AddGlobalDefine(VMA(1));
int trap_PC_AddGlobalDefine( const char *define )
{
    return syscallVM( CG_PC_ADD_GLOBAL_DEFINE, define );
}

//118
//return botlib_export->PC_LoadSourceHandle(VMA(1));
int trap_PC_LoadSource( const char *filename )
{
    return syscallVM( CG_PC_LOAD_SOURCE, filename );
}

//119
//return botlib_export->PC_FreeSourceHandle(args[1]);
int trap_PC_FreeSource( int handle )
{
    return syscallVM( CG_PC_FREE_SOURCE, handle );
}

//120
//return botlib_export->PC_ReadTokenHandle(args[1], VMA(2));
int trap_PC_ReadToken( int handle, pc_token_t *pc_token )
{
    return syscallVM( CG_PC_READ_TOKEN, handle, pc_token );
}

//121
//return botlib_export->PC_SourceFileAndLine(args[1], VMA(2), VMA(3));
int trap_PC_SourceFileAndLine( int handle, char *filename, int *line )
{
    return syscallVM( CG_PC_SOURCE_FILE_AND_LINE, handle, filename, line );
}

//122.
//botlib_export->PC_UnreadLastTokenHandle(args[1]);
int trap_PC_UnReadToken( int handle )
{
    return syscallVM( CG_PC_UNREAD_TOKEN, handle );
}

//123
//S_StopBackgroundTrack();
void trap_S_StopBackgroundTrack( void )
{
    syscallVM( CG_S_STOPBACKGROUNDTRACK );
}

//124.
//return Com_RealTime(VMA(1));
int trap_RealTime( qtime_t *qtime )
{
    return syscallVM( CG_REAL_TIME, qtime );
}

//125.
//Q_SnapVector(VMA(1));
void trap_SnapVector( float *v )
{
    syscallVM( CG_SNAPVECTOR, v );
}

//126.
//return CIN_PlayCinematic(VMA(1), args[2], args[3], args[4], args[5], args[6]);
int trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits )
{
    return syscallVM( CG_CIN_PLAYCINEMATIC, arg0, xpos, ypos, width, height, bits );
}

//127.
//return CIN_StopCinematic(args[1]);
e_status trap_CIN_StopCinematic( int handle )
{
    return (e_status) syscallVM( CG_CIN_STOPCINEMATIC, handle );
}

//128.
//return CIN_RunCinematic(args[1]);
e_status trap_CIN_RunCinematic( int handle )
{
    return (e_status) syscallVM( CG_CIN_RUNCINEMATIC, handle );
}

//129.
//CIN_DrawCinematic(args[1]);
void trap_CIN_DrawCinematic( int handle )
{
    syscallVM( CG_CIN_DRAWCINEMATIC, handle );
}

//130.
//CIN_SetExtents(args[1], args[2], args[3], args[4], args[5]);
void trap_CIN_SetExtents( int handle, int x, int y, int w, int h )
{
    syscallVM( CG_CIN_SETEXTENTS, handle, x, y, w, h );
}

//131.
//re.RemapShader(VMA(1), VMA(2), VMA(3));
void trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset )
{
    syscallVM( CG_R_REMAP_SHADER, oldShader, newShader, timeOffset );
}

//138.
//return re.GetEntityToken(VMA(1), args[2]);
qboolean trap_GetEntityToken( char *buffer, int bufferSize )
{
    return syscallVM( CG_GET_ENTITY_TOKEN, buffer, bufferSize );
}

//139.
//if(cls.state == CA_ACTIVE && !clc.demoplaying) {
//	if(uivm) {
//		VM_Call(uivm, UI_SET_ACTIVE_MENU, args[1]);
//	}
//}
void trap_UI_Popup( int arg0 )
{
    syscallVM( CG_INGAME_POPUP, arg0 );
}

//140.
void trap_UI_ClosePopup( const char *arg0 )
{
    syscallVM( CG_INGAME_CLOSEPOPUP, arg0 );
}

//141.
//Key_GetBindingBuf(args[1], VMA(2), args[3]);
void trap_Key_GetBindingBuf( int keynum, int team, char *buf, int buflen )
{
    syscallVM( CG_KEY_GETBINDINGBUF, keynum, team, buf, buflen );
}

//142.
//Key_SetBinding(args[1], VMA(2));
void trap_Key_SetBinding( int keynum, int team, const char *binding )
{
    syscallVM( CG_KEY_SETBINDING, keynum, team, binding );
}

//143.
//return Parse_AddGlobalDefine(VMA(1));
int trap_Parse_AddGlobalDefine( const char *define )
{
    return syscallVM( CG_PARSE_ADD_GLOBAL_DEFINE, define );
}

//144.
//return Parse_LoadSourceHandle(VMA(1));
int trap_Parse_LoadSource( const char *filename )
{
    return syscallVM( CG_PARSE_LOAD_SOURCE, filename );
}

//145.
//return Parse_FreeSourceHandle(args[1]);
int trap_Parse_FreeSource( int handle )
{
    return syscallVM( CG_PARSE_FREE_SOURCE, handle );
}

//146.
//return Parse_ReadTokenHandle( args[1], VMA(2) );
int trap_Parse_ReadToken( int handle, pc_token_t *pc_token )
{
    return syscallVM( CG_PARSE_READ_TOKEN, handle, pc_token );
}

//147.
//return Parse_SourceFileAndLine(args[1], VMA(2), VMA(3));
int trap_Parse_SourceFileAndLine( int handle, char *filename, int *line )
{
    return syscallVM( CG_PARSE_SOURCE_FILE_AND_LINE, handle, filename, line );
}

//148.
//Key_KeynumToStringBuf(args[1], VMA(2), args[3]);
void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen )
{
    syscallVM( CG_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen );
}

//151.
//S_FadeAllSounds(VMF(1), args[2], args[3]);
void trap_S_FadeAllSound( float targetvol, int time, qboolean stopsounds )
{
    syscallVM( CG_S_FADEALLSOUNDS, PASSFLOAT( targetvol ), time, stopsounds );
}

//152.
//return re.inPVS(VMA(1), VMA(2));
qboolean trap_R_inPVS( const vec3_t p1, const vec3_t p2 )
{
    return syscallVM( CG_R_INPVS, p1, p2 );
}

//153.
//Com_GetHunkInfo(VMA(1), VMA(2));
void trap_GetHunkData( int *hunkused, int *hunkexpected )
{
    syscallVM( CG_GETHUNKDATA, hunkused, hunkexpected );
}

//157.
//return re.LoadDynamicShader(VMA(1), VMA(2));
qboolean trap_R_LoadDynamicShader( const char *shadername, const char *shadertext )
{
    return syscallVM( CG_R_LOADDYNAMICSHADER, shadername, shadertext );
}

//158.
//re.RenderToTexture(args[1], args[2], args[3], args[4], args[5]);
void trap_R_RenderToTexture( int textureid, int x, int y, int w, int h )
{
    syscallVM( CG_R_RENDERTOTEXTURE, textureid, x, y, w, h );
}

//160.
//re.Finish();
void trap_R_Finish( void )
{
    syscallVM( CG_R_FINISH );
}

//161.
//CL_DemoName( VMA(1), args[2] );
void trap_GetDemoName( char *buffer, int size )
{
    syscallVM( CG_GETDEMONAME, buffer, size );
}

//162.
void trap_S_StartSoundVControl( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int volume )
{
    syscallVM( CG_S_STARTSOUND, origin, entityNum, entchannel, sfx, volume );
}

//163.
int trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir )
{
    return syscallVM( CG_R_LIGHTFORPOINT, point, ambientLight, directedLight, lightDir );
}

//165.
qhandle_t trap_R_RegisterAnimation( const char *name )
{
    return syscallVM( CG_R_REGISTERANIMATION, name );
}

//166.
int trap_R_CheckSkeleton( refSkeleton_t *skel, qhandle_t hModel, qhandle_t hAnim )
{
    return syscallVM( CG_R_CHECKSKELETON, skel, hModel, hAnim );
}

//167.
int trap_R_BuildSkeleton( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac, qboolean clearOrigin )
{
    return syscallVM( CG_R_BUILDSKELETON, skel, anim, startFrame, endFrame, PASSFLOAT( frac ), clearOrigin );
}

//168.
int trap_R_BlendSkeleton( refSkeleton_t *skel, const refSkeleton_t *blend, float frac )
{
    return syscallVM( CG_R_BLENDSKELETON, skel, blend, PASSFLOAT( frac ) );
}

//169.
int trap_R_BoneIndex( qhandle_t hModel, const char *boneName )
{
    return syscallVM( CG_R_BONEINDEX, hModel, boneName );
}

//170.
int trap_R_AnimNumFrames( qhandle_t hAnim )
{
    return syscallVM( CG_R_ANIMNUMFRAMES, hAnim );
}

//171.
int trap_R_AnimFrameRate( qhandle_t hAnim )
{
    return syscallVM( CG_R_ANIMFRAMERATE, hAnim );
}

//172.
void trap_CompleteCallback( const char *complete )
{
    syscallVM( CG_COMPLETE_CALLBACK, complete );
}

//173.
void trap_RegisterButtonCommands( const char *cmds )
{
    syscallVM( CG_REGISTER_BUTTON_COMMANDS, cmds );
}

//174.
void trap_R_Glyph( fontHandle_t font, const char *str, glyphInfo_t *glyph )
{
    syscallVM( CG_R_GLYPH, font, str, glyph );
}

//175.
void trap_R_GlyphChar( fontHandle_t font, int ch, glyphInfo_t *glyph )
{
    syscallVM( CG_R_GLYPHCHAR, font, ch, glyph );
}

//176.
void trap_R_UnregisterFont( fontHandle_t font )
{
    syscallVM( CG_R_UREGISTERFONT, font );
}

//177.
//GetClipboardData(VMA(1), args[2], args[3]);
void trap_GetClipboardData( char *buf, int bufsize, clipboard_t clip )
{
    syscallVM( CG_GETCLIPBOARDDATA, buf, bufsize, clip );
}

//178.
void trap_QuoteString( const char *str, char *buffer, int size )
{
    syscallVM( CG_QUOTESTRING, str, buffer, size );
}

//179.
void trap_Gettext( char *buffer, const char *msgid, int bufferLength )
{
    syscallVM( CG_GETTEXT, buffer, msgid, bufferLength );
}

//180.
void trap_Pgettext( char *buffer, const char *ctxt, const char *msgid, int bufferLength )
{
    syscallVM( CG_PGETTEXT, buffer, ctxt, msgid, bufferLength );
}

void trap_GettextPlural( char *buffer, const char *msgid, const char *msgid2, int number, int bufferLength )
{
    syscallVM( CG_GETTEXT_PLURAL, buffer, msgid, msgid2, number, bufferLength );
}

//181.
//return re.inPVVS(VMA(1), VMA(2));
qboolean trap_R_inPVVS( const vec3_t p1, const vec3_t p2 )
{
    return syscallVM( CG_R_INPVVS, p1, p2 );
}

void trap_notify_onTeamChange( int newTeam )
{
    syscallVM( CG_NOTIFY_TEAMCHANGE, newTeam );
}

qhandle_t trap_RegisterVisTest( void )
{
    return syscallVM( CG_REGISTERVISTEST );
}

void trap_AddVisTestToScene( qhandle_t hTest, vec3_t pos, float depthAdjust, float area )
{
    syscallVM( CG_ADDVISTESTTOSCENE, hTest, pos, PASSFLOAT( depthAdjust ),
            PASSFLOAT( area ) );
}

float trap_CheckVisibility( qhandle_t hTest )
{
    return syscallVM( CG_CHECKVISIBILITY, hTest );
}

void trap_UnregisterVisTest( qhandle_t hTest )
{
    syscallVM( CG_UNREGISTERVISTEST, hTest );
}

void trap_SetColorGrading( int slot, qhandle_t hShader )
{
    syscallVM( CG_SETCOLORGRADING, slot, hShader );
}

float trap_CM_DistanceToModel( const vec3_t loc, clipHandle_t model )
{
    return syscallVM( CG_CM_DISTANCETOMODEL, loc, model );
}

void trap_R_ScissorEnable( qboolean enable )
{
    syscallVM( CG_R_SCISSOR_ENABLE, enable );
}

void trap_R_ScissorSet( int x, int y, int w, int h )
{
    syscallVM( CG_R_SCISSOR_SET, x, y, w, h );
}

void trap_LAN_LoadCachedServers( void )
{
    syscallVM( CG_LAN_LOADCACHEDSERVERS );
}

void trap_LAN_SaveCachedServers( void )
{
    syscallVM( CG_LAN_SAVECACHEDSERVERS );
}

int trap_LAN_AddServer( int source, const char *name, const char *addr )
{
    return syscallVM( CG_LAN_ADDSERVER, source, name, addr );
}

void trap_LAN_RemoveServer( int source, const char *addr )
{
    syscallVM( CG_LAN_REMOVESERVER, source, addr );
}

int trap_LAN_GetPingQueueCount( void )
{
    return syscallVM( CG_LAN_GETPINGQUEUECOUNT );
}

void trap_LAN_ClearPing( int n )
{
    syscallVM( CG_LAN_CLEARPING, n );
}

void trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime )
{
    syscallVM( CG_LAN_GETPING, n, buf, buflen, pingtime );
}

void trap_LAN_GetPingInfo( int n, char *buf, int buflen )
{
    syscallVM( CG_LAN_GETPINGINFO, n, buf, buflen );
}

int trap_LAN_GetServerCount( int source )
{
    return syscallVM( CG_LAN_GETSERVERCOUNT, source );
}

void trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen )
{
    syscallVM( CG_LAN_GETSERVERADDRESSSTRING, source, n, buf, buflen );
}

void trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen )
{
    syscallVM( CG_LAN_GETSERVERINFO, source, n, buf, buflen );
}

int trap_LAN_GetServerPing( int source, int n )
{
    return syscallVM( CG_LAN_GETSERVERPING, source, n );
}

void trap_LAN_MarkServerVisible( int source, int n, qboolean visible )
{
    syscallVM( CG_LAN_MARKSERVERVISIBLE, source, n, visible );
}

int trap_LAN_ServerIsVisible( int source, int n )
{
    return syscallVM( CG_LAN_SERVERISVISIBLE, source, n );
}

qboolean trap_LAN_UpdateVisiblePings( int source )
{
    return syscallVM( CG_LAN_UPDATEVISIBLEPINGS, source );
}

void trap_LAN_ResetPings( int n )
{
    syscallVM( CG_LAN_RESETPINGS, n );
}

int trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen )
{
    return syscallVM( CG_LAN_SERVERSTATUS, serverAddress, serverStatus, maxLen );
}

qboolean trap_LAN_ServerIsInFavoriteList( int source, int n )
{
    return syscallVM( CG_LAN_SERVERISINFAVORITELIST, source, n );
}

qboolean trap_GetNews( qboolean force )
{
    return syscallVM( CG_GETNEWS, force );
}

int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 )
{
    return syscallVM( CG_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2 );
}

void trap_R_GetShaderNameFromHandle( const qhandle_t shader, char *out, int len )
{
    syscallVM( CG_R_GETSHADERNAMEFROMHANDLE, shader, out, len );
}

void trap_PrepareKeyUp( void )
{
    syscallVM( CG_PREPAREKEYUP );
}

void trap_R_SetAltShaderTokens( const char *str )
{
    syscallVM( CG_R_SETALTSHADERTOKENS, str );
}

void trap_S_UpdateEntityVelocity( int entityNum, const vec3_t velocity )
{
    syscallVM( CG_S_UPDATEENTITYVELOCITY, entityNum, velocity );
}

void trap_S_SetReverb( int slotNum, const char* name, float ratio )
{
    syscallVM( CG_S_SETREVERB, slotNum, name, PASSFLOAT(ratio) );
}

void trap_S_BeginRegistration( void )
{
    syscallVM( CG_S_BEGINREGISTRATION );
}

void trap_S_EndRegistration( void )
{
    syscallVM( CG_S_ENDREGISTRATION );
}

void trap_Rocket_Init( void )
{
    syscallVM( CG_ROCKET_INIT );
}

void trap_Rocket_Shutdown( void )
{
    syscallVM( CG_ROCKET_SHUTDOWN );
}

void trap_Rocket_LoadDocument( const char *path )
{
    syscallVM( CG_ROCKET_LOADDOCUMENT, path );
}

void trap_Rocket_LoadCursor( const char *path )
{
    syscallVM( CG_ROCKET_LOADCURSOR, path );
}

void trap_Rocket_DocumentAction( const char *name, const char *action )
{
    syscallVM( CG_ROCKET_DOCUMENTACTION, name, action );
}

qboolean trap_Rocket_GetEvent( void )
{
    return syscallVM( CG_ROCKET_GETEVENT );
}

void trap_Rocket_DeleteEvent( void )
{
    syscallVM( CG_ROCKET_DELELTEEVENT );
}

void trap_Rocket_RegisterDataSource( const char *name )
{
    syscallVM( CG_ROCKET_REGISTERDATASOURCE, name );
}

void trap_Rocket_DSAddRow( const char *name, const char *table, const char *data )
{
    syscallVM( CG_ROCKET_DSADDROW, name, table, data );
}

void trap_Rocket_DSChangeRow( const char *name, const char *table, int row, const char *data )
{
    syscallVM( CG_ROCKET_DSCHANGEROW, name, table, row, data );
}

void trap_Rocket_DSRemoveRow( const char *name, const char *table, int row )
{
    syscallVM( CG_ROCKET_DSREMOVEROW, name, table, row );
}

void trap_Rocket_DSClearTable( const char *name, const char *table )
{
    syscallVM( CG_ROCKET_DSCLEARTABLE, name, table );
}

void trap_Rocket_SetInnerRML( const char *RML, int parseFlags )
{
    syscallVM( CG_ROCKET_SETINNERRML, RML, parseFlags );
}

void trap_Rocket_GetAttribute( const char *attribute, char *out, int length )
{
    syscallVM( CG_ROCKET_GETATTRIBUTE, attribute, out, length );
}

void trap_Rocket_SetAttribute( const char *attribute, const char *value )
{
    syscallVM( CG_ROCKET_SETATTRIBUTE, attribute, value );
}

void trap_Rocket_GetProperty( const char *name, void *out, int len, rocketVarType_t type )
{
    syscallVM( CG_ROCKET_GETPROPERTY, name, out, len, type );
}

void trap_Rocket_SetProperty( const char *property, const char *value )
{
    syscallVM( CG_ROCKET_SETPROPERYBYID, property, value );
}
void trap_Rocket_GetEventParameters( char *params, int length )
{
    syscallVM( CG_ROCKET_GETEVENTPARAMETERS, params, length );
}
void trap_Rocket_RegisterDataFormatter( const char *name )
{
    syscallVM( CG_ROCKET_REGISTERDATAFORMATTER, name );
}

void trap_Rocket_DataFormatterRawData( int handle, char *name, int nameLength, char *data, int dataLength )
{
    syscallVM( CG_ROCKET_DATAFORMATTERRAWDATA, handle, name, nameLength, data, dataLength );
}

void trap_Rocket_DataFormatterFormattedData( int handle, const char *data, qboolean parseQuake )
{
    syscallVM( CG_ROCKET_DATAFORMATTERFORMATTEDDATA, handle, data, parseQuake );
}

void trap_Rocket_RegisterElement( const char *tag )
{
    syscallVM( CG_ROCKET_REGISTERELEMENT, tag );
}

void trap_Rocket_SetElementDimensions( float x, float y )
{
    syscallVM( CG_ROCKET_SETELEMENTDIMENSIONS, PASSFLOAT( x ), PASSFLOAT( y ) );
}

void trap_Rocket_GetElementTag( char *tag, int length )
{
    syscallVM( CG_ROCKET_GETELEMENTTAG, tag, length );
}

void trap_Rocket_GetElementAbsoluteOffset( float *x, float *y )
{
    syscallVM( CG_ROCKET_GETELEMENTABSOLUTEOFFSET, x, y );
}

void trap_Rocket_QuakeToRML( const char *in, char *out, int length )
{
    syscallVM( CG_ROCKET_QUAKETORML, in, out, length );
}

void trap_Rocket_SetClass( const char *in, qboolean activate )
{
    syscallVM( CG_ROCKET_SETCLASS, in, activate );
}

void trap_Rocket_InitializeHuds( int size )
{
    syscallVM( CG_ROCKET_INITHUDS, size );
}

void trap_Rocket_LoadUnit( const char *path )
{
    syscallVM( CG_ROCKET_LOADUNIT, path );
}

void trap_Rocket_AddUnitToHud( int weapon, const char *id )
{
    syscallVM( CG_ROCKET_ADDUNITTOHUD, weapon, id );
}

void trap_Rocket_ShowHud( int weapon )
{
    syscallVM( CG_ROCKET_SHOWHUD, weapon );
}

void trap_Rocket_ClearHud( int weapon )
{
    syscallVM( CG_ROCKET_CLEARHUD, weapon );
}

void trap_Rocket_AddTextElement( const char *text, const char *Class, float x, float y )
{
    syscallVM( CG_ROCKET_ADDTEXT, text, Class, PASSFLOAT( x ), PASSFLOAT( y ) );
}

void trap_Rocket_ClearText( void )
{
    syscallVM( CG_ROCKET_CLEARTEXT );
}

void trap_Rocket_RegisterProperty( const char *name, const char *defaultValue, qboolean inherited, qboolean force_layout, const char *parseAs )
{
    syscallVM( CG_ROCKET_REGISTERPROPERTY, name, defaultValue, inherited, force_layout, parseAs );
}

void trap_Rocket_ShowScoreboard( const char *name, qboolean show )
{
    syscallVM( CG_ROCKET_SHOWSCOREBOARD, name, show );
}

void trap_Rocket_SetDataSelectIndex( int index )
{
    syscallVM( CG_ROCKET_SETDATASELECTINDEX, index );
}

