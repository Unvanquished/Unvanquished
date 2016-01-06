/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2011  Dusan Jocic <dusanjocic@msn.com>

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

#ifndef CGAPI_H
#define CGAPI_H


#include "engine/qcommon/q_shared.h"
#include "engine/renderer/tr_types.h"
#include "common/cm/cm_public.h"

#define CGAME_API_VERSION 2

#define CMD_BACKUP               64
#define CMD_MASK                 ( CMD_BACKUP - 1 )
// allow a lot of command backups for very fast systems
// multiple commands may be combined into a single packet, so this
// needs to be larger than PACKET_BACKUP

#define MAX_ENTITIES_IN_SNAPSHOT 512

// snapshots are a view of the server at a given time

// Snapshots are generated at regular time intervals by the server,
// but they may not be sent if a client's rate level is exceeded, or
// they may be dropped by the network.
typedef struct
{
	int           snapFlags; // SNAPFLAG_RATE_DELAYED, etc
	int           ping;

	int           serverTime; // server time the message is valid for (in msec)

	byte          areamask[ MAX_MAP_AREA_BYTES ]; // portalarea visibility bits

	playerState_t ps; // complete information about the current player at this time

	// all of the entities that need to be presented at the time of this snapshot
	std::vector<entityState_t> entities;

	// text based server commands to execute when this snapshot becomes current
	std::vector<std::string> serverCommands;
} snapshot_t;

typedef enum {
	ROCKET_STRING,
	ROCKET_FLOAT,
	ROCKET_INT,
	ROCKET_COLOR
} rocketVarType_t;

typedef enum {
	ROCKETMENU_MAIN,
	ROCKETMENU_CONNECTING,
	ROCKETMENU_LOADING,
	ROCKETMENU_DOWNLOADING,
	ROCKETMENU_INGAME_MENU,
	ROCKETMENU_TEAMSELECT,
	ROCKETMENU_HUMANSPAWN,
	ROCKETMENU_ALIENSPAWN,
	ROCKETMENU_ALIENBUILD,
	ROCKETMENU_HUMANBUILD,
	ROCKETMENU_ARMOURYBUY,
	ROCKETMENU_ALIENEVOLVE,
	ROCKETMENU_CHAT,
	ROCKETMENU_BEACONS,
	ROCKETMENU_ERROR,
	ROCKETMENU_NUM_TYPES
} rocketMenuType_t;

typedef enum {
	RP_QUAKE = 1 << 0,
	RP_EMOTICONS = 1 << 1,
} rocketInnerRMLParseTypes_t;

typedef struct
{
	connstate_t connState;
	int         connectPacketCount;
	int         clientNum;
	char        servername[ MAX_STRING_CHARS ];
	char        updateInfoString[ MAX_STRING_CHARS ];
	char        messageString[ MAX_STRING_CHARS ];
} cgClientState_t;

typedef enum
{
	SORT_HOST,
	SORT_MAP,
	SORT_CLIENTS,
	SORT_PING,
	SORT_GAME,
	SORT_FILTERS,
	SORT_FAVOURITES
} serverSortField_t;

enum class MouseMode
{
	Deltas,       // The input is sent as movement deltas, the cursor is hidden
	CustomCursor, // The input is sent as positions, the cursor should be rendered by the game
	SystemCursor, // The input is sent as positions, the cursor should be rendered by the system
};

void            trap_Print( const char *string );
void NORETURN   trap_Error( const char *string );
int             trap_Milliseconds();
void            trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void            trap_Cvar_Update( vmCvar_t *vmCvar );
void            trap_Cvar_Set( const char *var_name, const char *value );
void            trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
int             trap_Cvar_VariableIntegerValue( const char *var_name );
float           trap_Cvar_VariableValue( const char *var_name );
void            trap_Cvar_AddFlags( const char *var_name, int flags );
int             trap_Argc();
void            trap_Argv( int n, char *buffer, int bufferLength );
void            trap_EscapedArgs( char *buffer, int bufferLength );
void            trap_LiteralArgs( char *buffer, int bufferLength );
int             trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
int             trap_FS_Read( void *buffer, int len, fileHandle_t f );
int             trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void            trap_FS_FCloseFile( fileHandle_t f );
int             trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
int             trap_FS_GetFileListRecursive( const char *path, const char *extension, char *listbuf, int bufsize );
int             trap_FS_Delete( const char *filename );
bool        trap_FS_LoadPak( const char *pak, const char *prefix );
void            trap_FS_LoadAllMapMetadata();
void            trap_SendConsoleCommand( const char *text );
void            trap_AddCommand( const char *cmdName );
void            trap_RemoveCommand( const char *cmdName );
void            trap_SendClientCommand( const char *s );
void            trap_UpdateScreen();
#define trap_CM_LoadMap(a) CM_LoadMap(a)
#define trap_CM_NumInlineModels CM_NumInlineModels
#define trap_CM_InlineModel CM_InlineModel
#define trap_CM_TempBoxModel(...) CM_TempBoxModel(__VA_ARGS__, false)
#define trap_CM_TempCapsuleModel(...) CM_TempBoxModel(__VA_ARGS__, true)
#define trap_CM_PointContents CM_PointContents
#define trap_CM_TransformedPointContents CM_TransformedPointContents
#define trap_CM_BoxTrace(...) CM_BoxTrace(__VA_ARGS__, TT_AABB)
#define trap_CM_TransformedBoxTrace(...) CM_TransformedBoxTrace(__VA_ARGS__, TT_AABB)
#define trap_CM_CapsuleTrace(...) CM_BoxTrace(__VA_ARGS__, TT_CAPSULE)
#define trap_CM_TransformedCapsuleTrace(...) CM_TransformedBoxTrace(__VA_ARGS__, TT_CAPSULE)
#define trap_CM_BiSphereTrace CM_BiSphereTrace
#define trap_CM_TransformedBiSphereTrace CM_TransformedBiSphereTrace
#define trap_CM_DistanceToModel CM_DistanceToModel
int             trap_CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );
void            trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
void            trap_S_StartSoundEx( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags );
void            trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void            trap_S_ClearLoopingSounds( bool killall );
void            trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void            trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void            trap_S_StopLoopingSound( int entityNum );
void            trap_S_StopStreamingSound( int entityNum );
int             trap_S_SoundDuration( sfxHandle_t handle );
void            trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );
int             trap_S_GetVoiceAmplitude( int entityNum );
int             trap_S_GetSoundLength( sfxHandle_t sfx );
int             trap_S_GetCurrentSoundTime();

void            trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[ 3 ], int inwater );
sfxHandle_t     trap_S_RegisterSound( const char *sample, bool compressed );
void            trap_S_StartBackgroundTrack( const char *intro, const char *loop );
int             trap_S_StartStreamingSound( const char *intro, const char *loop, int entnum, int channel, int attenuation );
void            trap_R_LoadWorldMap( const char *mapname );
qhandle_t       trap_R_RegisterModel( const char *name );
qhandle_t       trap_R_RegisterSkin( const char *name );
qhandle_t       trap_R_RegisterShader( const char *name, RegisterShaderFlags_t flags );
void            trap_R_RegisterFont( const char *fontName, const char *fallbackName, int pointSize, fontMetrics_t * );

void            trap_R_ClearScene();
void            trap_R_AddRefEntityToScene( const refEntity_t *re );
void            trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts );
void            trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );
void            trap_R_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags );
void            trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void            trap_R_Add2dPolysIndexedToScene( polyVert_t *polys, int numVerts, int *indexes, int numIndexes, int trans_x, int trans_y, qhandle_t shader );
int             trap_FS_Seek( fileHandle_t f, int offset, fsOrigin_t origin );
int             trap_FS_Tell( fileHandle_t f );
int             trap_FS_FileLength( fileHandle_t f );
void            trap_R_RenderScene( const refdef_t *fd );
void            trap_R_ClearColor();
void            trap_R_SetColor( const Color::Color &rgba );
void            trap_R_SetClipRegion( const float *region );
void            trap_R_ResetClipRegion();
void            trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void            trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );
void            trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int             trap_R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
void            trap_R_GetTextureSize( qhandle_t handle, int *x, int *y );
qhandle_t       trap_R_GenerateTexture( const byte *data, int x, int y );
void            trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );
bool        trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );
int             trap_GetCurrentCmdNumber();
bool        trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );
void            trap_SetUserCmdValue( int stateValue, int flags, float sensitivityScale );
int             trap_Key_GetCatcher();
void            trap_Key_SetCatcher( int catcher );
void            trap_Key_SetBinding( int keyNum, int team, const char *cmd );
void            trap_Key_ClearCmdButtons( void );
void            trap_Key_ClearStates( void );
std::vector<int> trap_Key_KeysDown( const std::vector<int>& keys );
void            trap_SetMouseMode( MouseMode mode );
void            trap_S_StopBackgroundTrack();
int             trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits );
e_status        trap_CIN_StopCinematic( int handle );
e_status        trap_CIN_RunCinematic( int handle );
void            trap_CIN_DrawCinematic( int handle );
void            trap_CIN_SetExtents( int handle, int x, int y, int w, int h );
void            trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );
bool        trap_GetEntityToken( char *buffer, int bufferSize );
void            trap_UI_Popup( int arg0 );
void            trap_UI_ClosePopup( const char *arg0 );
std::vector<std::vector<int>> trap_Key_GetKeynumForBinds(int team, std::vector<std::string> binds);
int             trap_Parse_AddGlobalDefine( const char *define );
int             trap_Parse_LoadSource( const char *filename );
int             trap_Parse_FreeSource( int handle );
bool             trap_Parse_ReadToken( int handle, pc_token_t *pc_token );
int             trap_Parse_SourceFileAndLine( int handle, char *filename, int *line );
void            trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void            trap_CG_TranslateString( const char *string, char *buf );
bool        trap_R_inPVS( const vec3_t p1, const vec3_t p2 );
bool        trap_R_inPVVS( const vec3_t p1, const vec3_t p2 );
bool        trap_R_LoadDynamicShader( const char *shadername, const char *shadertext );
int             trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );

qhandle_t       trap_R_RegisterAnimation( const char *name );
int             trap_R_BuildSkeleton( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac, bool clearOrigin );
int             trap_R_BlendSkeleton( refSkeleton_t *skel, const refSkeleton_t *blend, float frac );
int             trap_R_BoneIndex( qhandle_t hModel, const char *boneName );
int             trap_R_AnimNumFrames( qhandle_t hAnim );
int             trap_R_AnimFrameRate( qhandle_t hAnim );

void            trap_CompleteCallback( const char *complete );

void            trap_RegisterButtonCommands( const char *cmds );

void            trap_GetClipboardData( char *, int);
void            trap_QuoteString( const char *, char*, int );
void            trap_Gettext( char *buffer, const char *msgid, int bufferLength );
void            trap_Pgettext( char *buffer, const char *ctxt, const char *msgid, int bufferLength );
void            trap_GettextPlural( char *buffer, const char *msgid, const char *msgid2, int number, int bufferLength );

void            trap_notify_onTeamChange( int newTeam );

qhandle_t       trap_RegisterVisTest();
void            trap_AddVisTestToScene( qhandle_t hTest, const vec3_t pos,
					float depthAdjust, float area );
float           trap_CheckVisibility( qhandle_t hTest );
void            trap_UnregisterVisTest( qhandle_t hTest );
void            trap_SetColorGrading( int slot, qhandle_t hShader );
void            trap_R_ScissorEnable( bool enable );
void            trap_R_ScissorSet( int x, int y, int w, int h );
int             trap_LAN_GetServerCount( int source );
void            trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen );
int             trap_LAN_GetServerPing( int source, int n );
void            trap_LAN_MarkServerVisible( int source, int n, bool visible );
int             trap_LAN_ServerIsVisible( int source, int n );
bool        trap_LAN_UpdateVisiblePings( int source );
void            trap_LAN_ResetPings( int n );
int             trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen );
void            trap_LAN_ResetServerStatus();
bool        trap_GetNews( bool force );
void            trap_R_GetShaderNameFromHandle( const qhandle_t shader, char *out, int len );
void            trap_PrepareKeyUp();
void            trap_R_SetAltShaderTokens( const char * );
void            trap_S_UpdateEntityVelocity( int entityNum, const vec3_t velocity );
void            trap_S_SetReverb( int slotNum, const char* presetName, float ratio );
void            trap_S_BeginRegistration();
void            trap_S_EndRegistration();
void            trap_CrashDump(const uint8_t* data, size_t size);
#endif
