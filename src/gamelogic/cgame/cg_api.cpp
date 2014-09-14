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

// TODO non-syscalls, implement them at some point

void trap_EscapedArgs( char *buffer, int bufferLength )
{
}

void trap_LiteralArgs( char *buffer, int bufferLength )
{
}

float trap_Cvar_VariableValue( const char *var_name )
{
}

qboolean trap_FS_LoadPak( const char *pak )
{
}

// All Miscs

int trap_GetDemoState( void )
{
	int state;
	VM::SendMsg<GetDemoStateMsg>(state);
	return state;
}

void trap_GS_FS_Seek( fileHandle_t f, long offset, fsOrigin_t origin )
{
	VM::SendMsg<FSSeekMsg>(f, offset, origin);
}


int trap_GetDemoPos( void )
{
	int pos;
	VM::SendMsg<GetDemoPosMsg>(pos);
	return pos;
}

void trap_SendClientCommand( const char *s )
{
	VM::SendMsg<SendClientCommandMsg>(s);
}

void trap_UpdateScreen( void )
{
	VM::SendMsg<UpdateScreenMsg>();
}

int trap_CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer )
{
	std::vector<std::array<float, 3>> mypoints(numPoints);
	std::array<float, 3> myproj;
	memcpy((float*)mypoints.data(), points, sizeof(float) * 3 * numPoints);
	VectorCopy(projection, myproj.data());

	std::vector<std::array<float, 3>> mypointBuffer;
	std::vector<markFragment_t> myfragmentBuffer;
	VM::SendMsg<CMMarkFragmentsMsg>(mypoints, myproj, maxPoints, maxFragments, mypointBuffer, myfragmentBuffer);

	memcpy(pointBuffer, mypointBuffer.data(), sizeof(float) * 3 * maxPoints);
	memcpy(fragmentBuffer, myfragmentBuffer.data(), sizeof(markFragment_t) * maxFragments);
	return 0;
}

int trap_RealTime( qtime_t *qtime )
{
	int res;
	VM::SendMsg<RealTimeMsg>(res, *qtime);
	return res;
}

void trap_GetGlconfig( glconfig_t *glconfig )
{
	VM::SendMsg<GetGLConfigMsg>(*glconfig);
}

void trap_GetGameState( gameState_t *gamestate )
{
	VM::SendMsg<GetGameStateMsg>(*gamestate);
}

void trap_GetClientState( cgClientState_t *cstate )
{
	VM::SendMsg<GetClientStateMsg>(*cstate);
}

void trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime )
{
	VM::SendMsg<GetCurrentSnapshotNumberMsg>(*snapshotNumber, *serverTime);
}

qboolean trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot )
{
	bool res;
	VM::SendMsg<GetSnapshotMsg>(snapshotNumber, res, *snapshot);
	return res;
}

qboolean trap_GetServerCommand( int serverCommandNumber )
{
	bool res;
	VM::SendMsg<GetServerCommandMsg>(serverCommandNumber, res);
	return res;
}

int trap_GetCurrentCmdNumber( void )
{
	int res;
	VM::SendMsg<GetCurrentCmdNumberMsg>(res);
	return res;
}

qboolean trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd )
{
	bool res;
	VM::SendMsg<GetUserCmdMsg>(cmdNumber, res, *ucmd);
	return res;
}

void trap_SetUserCmdValue( int stateValue, int flags, float sensitivityScale, int mpIdentClient )
{
	VM::SendMsg<SetUserCmdValueMsg>(stateValue, flags, sensitivityScale, mpIdentClient);
}

void trap_SetClientLerpOrigin( float x, float y, float z )
{
	VM::SendMsg<SetClientLerpOriginMsg>(x, y, z);
}

qboolean trap_GetEntityToken( char *buffer, int bufferSize )
{
	bool res;
	std::string token;
	VM::SendMsg<GetEntityTokenMsg>(bufferSize, res, token);
	Q_strncpyz(buffer, token.c_str(), bufferSize);
	return res;
}

void trap_GetDemoName( char *buffer, int size )
{
	std::string name;
	VM::SendMsg<GetDemoNameMsg>(size, name);
	Q_strncpyz(buffer, name.c_str(), size);
}

void trap_RegisterButtonCommands( const char *cmds )
{
	VM::SendMsg<RegisterButtonCommandsMsg>(cmds);
}

void trap_GetClipboardData( char *buf, int bufsize, clipboard_t clip )
{
	std::string data;
	VM::SendMsg<GetClipboardDataMsg>(bufsize, clip, data);
	Q_strncpyz(buf, data.c_str(), bufsize);
}

void trap_QuoteString( const char *str, char *buffer, int size )
{
	std::string quoted;
	VM::SendMsg<QuoteStringMsg>(size, str, quoted);
	Q_strncpyz(buffer, quoted.c_str(), size);
}

void trap_Gettext( char *buffer, const char *msgid, int bufferLength )
{
	std::string result;
	VM::SendMsg<GettextMsg>(bufferLength, msgid, result);
	Q_strncpyz(buffer, result.c_str(), bufferLength);
}

void trap_Pgettext( char *buffer, const char *ctxt, const char *msgid, int bufferLength )
{
	std::string result;
	VM::SendMsg<PGettextMsg>(bufferLength, ctxt, msgid, result);
	Q_strncpyz(buffer, result.c_str(), bufferLength);
}

void trap_GettextPlural( char *buffer, const char *msgid, const char *msgid2, int number, int bufferLength )
{
	std::string result;
	VM::SendMsg<GettextPluralMsg>(bufferLength, msgid, msgid2, number, result);
	Q_strncpyz(buffer, result.c_str(), bufferLength);
}

void trap_notify_onTeamChange( int newTeam )
{
	VM::SendMsg<NotifyTeamChangeMsg>(newTeam);
}

void trap_PrepareKeyUp( void )
{
	VM::SendMsg<PrepareKeyUpMsg>();
}

qboolean trap_GetNews( qboolean force )
{
	bool res;
	VM::SendMsg<GetNewsMsg>(force, res);
	return res;
}

// All Sounds

void trap_S_StartSound( vec3_t origin, int entityNum, int, sfxHandle_t sfx )
{
	std::array<float, 3> myorigin;
	VectorCopy(origin, myorigin.data());
	VM::SendMsg<Audio::StartSoundMsg>(myorigin, entityNum, sfx);
}

void trap_S_StartLocalSound( sfxHandle_t sfx, int )
{
	VM::SendMsg<Audio::StartLocalSoundMsg>(sfx);
}

void trap_S_ClearLoopingSounds( qboolean )
{
	VM::SendMsg<Audio::ClearLoopingSoundsMsg>();
}

void trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
	if (origin) {
		trap_S_UpdateEntityPosition(entityNum, origin);
	}
	if (velocity) {
		trap_S_UpdateEntityVelocity(entityNum, velocity);
	}
	VM::SendMsg<Audio::AddLoopingSoundMsg>(entityNum, sfx);
}

void trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
	trap_S_AddLoopingSound(entityNum, origin, velocity, sfx);
}

void trap_S_StopLoopingSound( int entityNum )
{
	VM::SendMsg<Audio::StopLoopingSoundMsg>(entityNum);
}

void trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	std::array<float, 3> myposition;
	VectorCopy(origin, myposition.data());
	VM::SendMsg<Audio::UpdateEntityPositionMsg>(entityNum, myposition);
}

void trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[ 3 ], int )
{
	if (origin) {
		trap_S_UpdateEntityPosition(entityNum, origin);
	}
	std::array<float, 9> myaxis;
	memcpy(myaxis.data(), axis, sizeof(float) * 9);
	VM::SendMsg<Audio::RespatializeMsg>(entityNum, myaxis);
}

sfxHandle_t trap_S_RegisterSound( const char *sample, qboolean)
{
	int sfx;
	VM::SendMsg<Audio::RegisterSoundMsg>(sample, sfx);
	return sfx;
}

void trap_S_StartBackgroundTrack( const char *intro, const char *loop )
{
	VM::SendMsg<Audio::StartBackgroundTrackMsg>(intro, loop);
}

void trap_S_StopBackgroundTrack( void )
{
	VM::SendMsg<Audio::StopBackgroundTrackMsg>();
}

void trap_S_UpdateEntityVelocity( int entityNum, const vec3_t velocity )
{
	std::array<float, 3> myvelocity;
	VectorCopy(velocity, myvelocity.data());
	VM::SendMsg<Audio::UpdateEntityVelocityMsg>(entityNum, myvelocity);
}

void trap_S_SetReverb( int slotNum, const char* name, float ratio )
{
	VM::SendMsg<Audio::SetReverbMsg>(slotNum, name, ratio);
}

void trap_S_BeginRegistration( void )
{
	VM::SendMsg<Audio::BeginRegistrationMsg>();
}

void trap_S_EndRegistration( void )
{
	VM::SendMsg<Audio::EndRegistrationMsg>();
}

// All renderer

void trap_R_SetAltShaderTokens( const char *str )
{
	VM::SendMsg<Render::SetAltShaderTokenMsg>(str);
}

void trap_R_GetShaderNameFromHandle( const qhandle_t shader, char *out, int len )
{
	std::string result;
	VM::SendMsg<Render::GetShaderNameFromHandleMsg>(shader, len, result);
	Q_strncpyz(out, result.c_str(), len);
}

void trap_R_ScissorEnable( qboolean enable )
{
	VM::SendMsg<Render::ScissorEnableMsg>(enable);
}

void trap_R_ScissorSet( int x, int y, int w, int h )
{
	VM::SendMsg<Render::ScissorSetMsg>(x, y, w, h);
}

qboolean trap_R_inPVVS( const vec3_t p1, const vec3_t p2 )
{
	bool res;
	std::array<float, 3> myp1, myp2;
	VectorCopy(p1, myp1.data());
	VectorCopy(p2, myp2.data());
	VM::SendMsg<Render::InPVVSMsg>(myp1, myp2, res);
	return res;
}

void trap_R_LoadWorldMap( const char *mapname )
{
	VM::SendMsg<Render::LoadWorldMapMsg>(mapname);
}

qhandle_t trap_R_RegisterModel( const char *name )
{
	int handle;
	VM::SendMsg<Render::RegisterModelMsg>(name, handle);
	return handle;
}

qhandle_t trap_R_RegisterSkin( const char *name )
{
	int handle;
	VM::SendMsg<Render::RegisterSkinMsg>(name, handle);
	return handle;
}

qhandle_t trap_R_RegisterShader( const char *name, RegisterShaderFlags_t flags )
{
	int handle;
	VM::SendMsg<Render::RegisterShaderMsg>(name, flags, handle);
	return handle;
}

void trap_R_RegisterFont( const char *fontName, const char *fallbackName, int pointSize, fontMetrics_t *font )
{
	VM::SendMsg<Render::RegisterFontMsg>(fontName, fallbackName, pointSize, *font);
}

void trap_R_ClearScene( void )
{
	VM::SendMsg<Render::ClearSceneMsg>();
}

void trap_R_AddRefEntityToScene( const refEntity_t *re )
{
	VM::SendMsg<Render::AddRefEntityToSceneMsg>(*re);
}

void trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts )
{
	std::vector<polyVert_t> myverts(numVerts);
	memcpy(myverts.data(), verts, numVerts * sizeof(polyVert_t));
	VM::SendMsg<Render::AddPolyToSceneMsg>(hShader, myverts);
}

void trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys )
{
	std::vector<polyVert_t> myverts(numVerts);
	memcpy(myverts.data(), verts, numVerts * sizeof(polyVert_t));
	VM::SendMsg<Render::AddPolysToSceneMsg>(hShader, myverts, numPolys);
}

void trap_R_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags )
{
	std::array<float, 3> myorg;
	VectorCopy(org, myorg.data());
	VM::SendMsg<Render::AddLightToSceneMsg>(myorg, radius, intensity, r, g, b, hShader, flags);
}

void trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b )
{
	std::array<float, 3> myorg;
	VectorCopy(org, myorg.data());
	VM::SendMsg<Render::AddAdditiveLightToSceneMsg>(myorg, intensity, r, g, b);
}

void trap_R_RenderScene( const refdef_t *fd )
{
	VM::SendMsg<Render::RenderSceneMsg>(*fd);
}

void trap_R_SetColor( const float *rgba )
{
	std::array<float, 4> myrgba;
	memcpy(myrgba.data(), rgba, 4 * sizeof(float));
	VM::SendMsg<Render::SetColorMsg>(myrgba);
}

void trap_R_SetClipRegion( const float *region )
{
	std::array<float, 4> myregion;
	memcpy(myregion.data(), region, 4 * sizeof(float));
	VM::SendMsg<Render::SetClipRegionMsg>(myregion);
}

void trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader )
{
	VM::SendMsg<Render::DrawStretchPicMsg>(x, y, w, h, s1, t1, s2, t2, hShader);
}

void trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle )
{
	VM::SendMsg<Render::DrawRotatedPicMsg>(x, y, w, h, s1, t1, s2, t2, hShader, angle);
}

void trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs )
{
	std::array<float, 3> mymins, mymaxs;
	VM::SendMsg<Render::ModelBoundsMsg>(model, mymins, mymaxs);
	VectorCopy(mymins.data(), mins);
	VectorCopy(mymaxs.data(), maxs);
}

int trap_R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex )
{
	int result;
	VM::SendMsg<Render::LerpTagMsg>(*refent, tagName, startIndex, *tag, result);
	return result;
}

void trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset )
{
	VM::SendMsg<Render::RemapShaderMsg>(oldShader, newShader, timeOffset);
}

qboolean trap_R_inPVS( const vec3_t p1, const vec3_t p2 )
{
	bool res;
	std::array<float, 3> myp1, myp2;
	VectorCopy(p1, myp1.data());
	VectorCopy(p2, myp2.data());
	VM::SendMsg<Render::InPVSMsg>(myp1, myp2, res);
	return res;
}

int trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir )
{
	int result;
	std::array<float, 3> mypoint, myambient, mydirected, mydir;
	VectorCopy(point, mypoint.data());
	VM::SendMsg<Render::LightForPointMsg>(mypoint, myambient, mydirected, mydir, result);
	VectorCopy(myambient.data(), ambientLight);
	VectorCopy(mydirected.data(), directedLight);
	VectorCopy(mydir.data(), lightDir);
	return result;
}

qhandle_t trap_R_RegisterAnimation( const char *name )
{
	int handle;
	VM::SendMsg<Render::RegisterAnimationMsg>(name, handle);
	return handle;
}

int trap_R_BuildSkeleton( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac, qboolean clearOrigin )
{
	int result;
	VM::SendMsg<Render::BuildSkeletonMsg>(anim, startFrame, endFrame, frac, clearOrigin, *skel, result);
	return result;
}

int trap_R_BlendSkeleton( refSkeleton_t *skel, const refSkeleton_t *blend, float frac )
{
	int result;
	VM::SendMsg<Render::BlendSkeletonMsg>(*skel, *blend, frac, *skel, result);
	return result;
}

int trap_R_BoneIndex( qhandle_t hModel, const char *boneName )
{
	int index;
	VM::SendMsg<Render::BoneIndexMsg>(hModel, boneName, index);
	return index;
}

int trap_R_AnimNumFrames( qhandle_t hAnim )
{
	int n;
	VM::SendMsg<Render::AnimNumFramesMsg>(hAnim, n);
	return n;
}

int trap_R_AnimFrameRate( qhandle_t hAnim )
{
	int n;
	VM::SendMsg<Render::AnimFrameRateMsg>(hAnim, n);
	return n;
}

qhandle_t trap_RegisterVisTest( void )
{
	int handle;
	VM::SendMsg<Render::RegisterVisTestMsg>(handle);
	return handle;
}

void trap_AddVisTestToScene( qhandle_t hTest, vec3_t pos, float depthAdjust, float area )
{
	std::array<float, 3> mypos;
	VectorCopy(pos, mypos.data());
	VM::SendMsg<Render::AddVisTestToSceneMsg>(hTest, mypos, depthAdjust, area);
}

float trap_CheckVisibility( qhandle_t hTest )
{
	float result;
	VM::SendMsg<Render::CheckVisibilityMsg>(hTest, result);
	return result;
}

void trap_UnregisterVisTest( qhandle_t hTest )
{
	VM::SendMsg<Render::UnregisterVisTestMsg>(hTest);
}

void trap_SetColorGrading( int slot, qhandle_t hShader )
{
	VM::SendMsg<Render::SetColorGradingMsg>(slot, hShader);
}

//172.
void trap_CompleteCallback( const char *complete )
{
    syscallVM( CG_COMPLETE_CALLBACK, complete );
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

//148.
//Key_KeynumToStringBuf(args[1], VMA(2), args[3]);
void trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen )
{
    syscallVM( CG_KEY_KEYNUMTOSTRINGBUF, keynum, buf, buflen );
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

int trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 )
{
    return syscallVM( CG_LAN_COMPARESERVERS, source, sortKey, sortDir, s1, s2 );
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

