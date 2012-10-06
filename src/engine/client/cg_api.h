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

#include "../qcommon/q_shared.h"
#include "../qcommon/vm_traps.h"
#include "../renderer/tr_types.h"

#define CGAME_IMPORT_API_VERSION 3

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

	int           numEntities; // all of the entities that need to be presented
	entityState_t entities[ MAX_ENTITIES_IN_SNAPSHOT ]; // at the time of this snapshot

	int           numServerCommands; // text based server commands to execute when this
	int           serverCommandSequence; // snapshot becomes current
} snapshot_t;

typedef enum cgameEvent_e
{
  CGAME_EVENT_NONE,
  CGAME_EVENT_GAMEVIEW,
  CGAME_EVENT_SPEAKEREDITOR,
  CGAME_EVENT_CAMPAIGNBREIFING,
  CGAME_EVENT_DEMO,
  CGAME_EVENT_FIRETEAMMSG,
  CGAME_EVENT_MULTIVIEW
} cgameEvent_t;

typedef enum cgameImport_s
{
  CG_PRINT = FIRST_VM_SYSCALL,
  CG_ERROR,
  CG_MILLISECONDS,
  CG_CVAR_REGISTER,
  CG_CVAR_UPDATE,
  CG_CVAR_SET,
  CG_CVAR_VARIABLESTRINGBUFFER,
  CG_CVAR_LATCHEDVARIABLESTRINGBUFFER,
  CG_CVAR_VARIABLEINTEGERVALUE,
  CG_ARGC,
  CG_ARGV,
  CG_ARGS,
  CG_LITERAL_ARGS,
  CG_GETDEMOSTATE,
  CG_GETDEMOPOS,
  CG_FS_FOPENFILE,
  CG_FS_READ,
  CG_FS_WRITE,
  CG_FS_FCLOSEFILE,
  CG_FS_GETFILELIST,
  CG_FS_DELETEFILE,
  CG_SENDCONSOLECOMMAND,
  CG_ADDCOMMAND,
  CG_REMOVECOMMAND,
  CG_SENDCLIENTCOMMAND,
  CG_UPDATESCREEN,
  CG_CM_LOADMAP,
  CG_CM_NUMINLINEMODELS,
  CG_CM_INLINEMODEL,
  CG_CM_TEMPBOXMODEL,
  CG_CM_TEMPCAPSULEMODEL,
  CG_CM_POINTCONTENTS,
  CG_CM_TRANSFORMEDPOINTCONTENTS,
  CG_CM_BOXTRACE,
  CG_CM_TRANSFORMEDBOXTRACE,
  CG_CM_CAPSULETRACE,
  CG_CM_TRANSFORMEDCAPSULETRACE,
  CG_CM_BISPHERETRACE,
  CG_CM_TRANSFORMEDBISPHERETRACE,
  CG_CM_MARKFRAGMENTS,
  CG_R_PROJECTDECAL,
  CG_R_CLEARDECALS,
  CG_S_STARTSOUND,
  CG_S_STARTSOUNDEX,
  CG_S_STARTLOCALSOUND,
  CG_S_CLEARLOOPINGSOUNDS,
  CG_S_CLEARSOUNDS,
  CG_S_ADDLOOPINGSOUND,
  CG_S_ADDREALLOOPINGSOUND,
  CG_S_STOPLOOPINGSOUND,
  CG_S_STOPSTREAMINGSOUND,
  CG_S_UPDATEENTITYPOSITION,
  CG_S_GETVOICEAMPLITUDE,
  CG_S_GETSOUNDLENGTH,
  CG_S_GETCURRENTSOUNDTIME,
  CG_S_RESPATIALIZE,
  CG_S_REGISTERSOUND,
  CG_S_STARTBACKGROUNDTRACK,
  CG_S_FADESTREAMINGSOUND,
  CG_S_STARTSTREAMINGSOUND,
  CG_R_LOADWORLDMAP,
  CG_R_REGISTERMODEL,
  CG_R_REGISTERSKIN,
  CG_R_GETSKINMODEL,
  CG_R_GETMODELSHADER,
  CG_R_REGISTERSHADER,
  CG_R_REGISTERFONT,
  CG_R_REGISTERSHADERNOMIP,
//#if defined( USE_REFLIGHT )
  CG_R_REGISTERSHADERLIGHTATTENUATION,
//#endif
  CG_R_CLEARSCENE,
  CG_R_ADDREFENTITYTOSCENE,
//#if defined( USE_REFLIGHT )
  CG_R_ADDREFLIGHTSTOSCENE,
//#endif
  CG_R_ADDPOLYTOSCENE,
  CG_R_ADDPOLYSTOSCENE,
  CG_R_ADDPOLYBUFFERTOSCENE,
  CG_R_ADDLIGHTTOSCENE,
  CG_R_ADDADDITIVELIGHTTOSCENE,
  CG_FS_SEEK,
  CG_R_ADDCORONATOSCENE,
  CG_R_SETFOG,
  CG_R_SETGLOBALFOG,
  CG_R_RENDERSCENE,
  CG_R_SAVEVIEWPARMS,
  CG_R_RESTOREVIEWPARMS,
  CG_R_SETCOLOR,
  CG_R_SETCLIPREGION,
  CG_R_DRAWSTRETCHPIC,
  CG_R_DRAWROTATEDPIC,
  CG_R_DRAWSTRETCHPIC_GRADIENT,
  CG_R_DRAW2DPOLYS,
  CG_R_MODELBOUNDS,
  CG_R_LERPTAG,
  CG_GETGLCONFIG,
  CG_GETGAMESTATE,
  CG_GETCURRENTSNAPSHOTNUMBER,
  CG_GETSNAPSHOT,
  CG_GETSERVERCOMMAND,
  CG_GETCURRENTCMDNUMBER,
  CG_GETUSERCMD,
  CG_SETUSERCMDVALUE,
  CG_SETCLIENTLERPORIGIN,
  CG_MEMORY_REMAINING,
  CG_KEY_ISDOWN,
  CG_KEY_GETCATCHER,
  CG_KEY_SETCATCHER,
  CG_KEY_GETKEY,
  CG_KEY_GETOVERSTRIKEMODE,
  CG_KEY_SETOVERSTRIKEMODE,
  CG_PC_ADD_GLOBAL_DEFINE,
  CG_PC_LOAD_SOURCE,
  CG_PC_FREE_SOURCE,
  CG_PC_READ_TOKEN,
  CG_PC_SOURCE_FILE_AND_LINE,
  CG_PC_UNREAD_TOKEN,
  CG_S_STOPBACKGROUNDTRACK,
  CG_REAL_TIME,
  CG_SNAPVECTOR,
  CG_CIN_PLAYCINEMATIC,
  CG_CIN_STOPCINEMATIC,
  CG_CIN_RUNCINEMATIC,
  CG_CIN_DRAWCINEMATIC,
  CG_CIN_SETEXTENTS,
  CG_R_REMAP_SHADER,
  CG_LOADCAMERA,
  CG_STARTCAMERA,
  CG_STOPCAMERA,
  CG_GETCAMERAINFO,
  CG_GET_ENTITY_TOKEN,
  CG_INGAME_POPUP,
  CG_INGAME_CLOSEPOPUP,
  CG_KEY_GETBINDINGBUF,
  CG_KEY_SETBINDING,
  CG_PARSE_ADD_GLOBAL_DEFINE,
  CG_PARSE_LOAD_SOURCE,
  CG_PARSE_FREE_SOURCE,
  CG_PARSE_READ_TOKEN,
  CG_PARSE_SOURCE_FILE_AND_LINE,
  CG_KEY_KEYNUMTOSTRINGBUF,
  CG_KEY_BINDINGTOKEYS,
  CG_TRANSLATE_STRING,
  CG_S_FADEALLSOUNDS,
  CG_R_INPVS,
  CG_GETHUNKDATA,
  CG_R_LOADDYNAMICSHADER,
  CG_R_RENDERTOTEXTURE,
  CG_R_GETTEXTUREID,
  CG_R_FINISH,
  CG_GETDEMONAME,
  CG_R_LIGHTFORPOINT,
  CG_S_SOUNDDURATION,
//#if defined( USE_REFENTITY_ANIMATIONSYSTEM )
  CG_R_REGISTERANIMATION,
  CG_R_CHECKSKELETON,
  CG_R_BUILDSKELETON,
  CG_R_BLENDSKELETON,
  CG_R_BONEINDEX,
  CG_R_ANIMNUMFRAMES,
  CG_R_ANIMFRAMERATE,
//#endif
  CG_COMPLETE_CALLBACK,
  CG_REGISTER_BUTTON_COMMANDS,
  CG_GETCLIPBOARDDATA,
  CG_QUOTESTRING,
  CG_GETTEXT,
  CG_R_GLYPH,
  CG_R_GLYPHCHAR,
  CG_R_UREGISTERFONT,
  CG_PGETTEXT
} cgameImport_t;

typedef enum
{
  CG_INIT,
//  void CG_Init( int serverMessageNum, int serverCommandSequence )
  // called when the level loads or when the renderer is restarted
  // all media should be registered at this time
  // cgame will display loading status by calling SCR_Update, which
  // will call CG_DrawInformation during the loading process
  // reliableCommandSequence will be 0 on fresh loads, but higher for
  // demos, tourney restarts, or vid_restarts

  CG_SHUTDOWN,
//  void (*CG_Shutdown)( void );
  // oportunity to flush and close any open files

  CG_CONSOLE_COMMAND,
//  qboolean (*CG_ConsoleCommand)( void );
  // a console command has been issued locally that is not recognized by the
  // main game system.
  // use Cmd_Argc() / Cmd_Argv() to read the command, return qfalse if the
  // command is not known to the game

  CG_DRAW_ACTIVE_FRAME,
//  void (*CG_DrawActiveFrame)( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
  // Generates and draws a game scene and status information at the given time.
  // If demoPlayback is set, local movement prediction will not be enabled

  CG_CONSOLE_TEXT,
//	void (*CG_ConsoleText)( void );
  //  pass text that has been printed to the console to cgame
  //  use Cmd_Argc() / Cmd_Argv() to read it

  CG_CROSSHAIR_PLAYER,
//  int (*CG_CrosshairPlayer)( void );

  CG_LAST_ATTACKER,
//  int (*CG_LastAttacker)( void );

  CG_KEY_EVENT,
//  void    (*CG_KeyEvent)( int key, qboolean down );

  CG_MOUSE_EVENT,
//  void    (*CG_MouseEvent)( int dx, int dy );
  CG_EVENT_HANDLING,
//  void (*CG_EventHandling)(int type, qboolean fForced);

  CG_VOIP_STRING,
// char *(*CG_VoIPString)( void );
// returns a string of comma-delimited clientnums based on cl_voipSendTarget

  CG_COMPLETE_COMMAND
// char (*CG_CompleteCommand)( int argNum );
// will callback on all availible completions
// use Cmd_Argc() / Cmd_Argv() to read the command
} cgameExport_t;

void            trap_Print( const char *string );
void            trap_Error( const char *string ) NORETURN;
int             trap_Milliseconds( void );
void            trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void            trap_Cvar_Update( vmCvar_t *vmCvar );
void            trap_Cvar_Set( const char *var_name, const char *value );
void            trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void            trap_Cvar_LatchedVariableStringBuffer( const char *var_name, char *buffer, int bufsize );
int             trap_Cvar_VariableIntegerValue( const char *var_name );
int             trap_Argc( void );
void            trap_Argv( int n, char *buffer, int bufferLength );
void            trap_Args( char *buffer, int bufferLength );
void            trap_LiteralArgs( char *buffer, int bufferLength );
int             trap_GetDemoState( void );
int             trap_GetDemoPos( void );
int             trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void            trap_FS_Read( void *buffer, int len, fileHandle_t f );
void            trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void            trap_FS_FCloseFile( fileHandle_t f );
int             trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
int             trap_FS_Delete( const char *filename );
void            trap_SendConsoleCommand( const char *text );
void            trap_AddCommand( const char *cmdName );
void            trap_RemoveCommand( const char *cmdName );
void            trap_SendClientCommand( const char *s );
void            trap_UpdateScreen( void );
void            trap_CM_LoadMap( const char *mapname );
int             trap_CM_NumInlineModels( void );
clipHandle_t    trap_CM_InlineModel( int index );
clipHandle_t    trap_CM_TempBoxModel( const vec3_t mins, const vec3_t maxs );
clipHandle_t    trap_CM_TempCapsuleModel( const vec3_t mins, const vec3_t maxs );
int             trap_CM_PointContents( const vec3_t p, clipHandle_t model );
int             trap_CM_TransformedPointContents( const vec3_t p, clipHandle_t model, const vec3_t origin, const vec3_t angles );
void            trap_CM_BoxTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask );
void            trap_CM_TransformedBoxTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles );
void            trap_CM_CapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask );
void            trap_CM_TransformedCapsuleTrace( trace_t *results, const vec3_t start, const vec3_t end, const vec3_t mins, const vec3_t maxs, clipHandle_t model, int brushmask, const vec3_t origin, const vec3_t angles );
void            trap_CM_BiSphereTrace( trace_t *results, const vec3_t start, const vec3_t end, float startRad, float endRad, clipHandle_t model, int mask );
void            trap_CM_TransformedBiSphereTrace( trace_t *results, const vec3_t start, const vec3_t end, float startRad, float endRad, clipHandle_t model, int mask, const vec3_t origin );
int             trap_CM_MarkFragments( int numPoints, const vec3_t *points, const vec3_t projection, int maxPoints, vec3_t pointBuffer, int maxFragments, markFragment_t *fragmentBuffer );
void            trap_R_ProjectDecal( qhandle_t hShader, int numPoints, vec3_t *points, vec4_t projection, vec4_t color, int lifeTime, int fadeTime );
void            trap_R_ClearDecals( void );
void            trap_S_StartSound( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx );
void            trap_S_StartSoundEx( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int flags );
void            trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );

//void trap_S_ClearLoopingSounds(void);
void            trap_S_ClearLoopingSounds( qboolean killall );
void            trap_S_ClearSounds( qboolean killmusic );

//void trap_S_AddLoopingSound(const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int volume, int soundTime);
void            trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );

//void trap_S_AddRealLoopingSound(const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx, int range, int volume, int soundTime);
void            trap_S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
void            trap_S_StopLoopingSound( int entityNum );
void            trap_S_StopStreamingSound( int entityNum );
int             trap_S_SoundDuration( sfxHandle_t handle );
void            trap_S_UpdateEntityPosition( int entityNum, const vec3_t origin );
int             trap_S_GetVoiceAmplitude( int entityNum );
int             trap_S_GetSoundLength( sfxHandle_t sfx );
int             trap_S_GetCurrentSoundTime( void );

void            trap_S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[ 3 ], int inwater );
sfxHandle_t     trap_S_RegisterSound( const char *sample, qboolean compressed );
void            trap_S_StartBackgroundTrack( const char *intro, const char *loop );
void            trap_S_FadeBackgroundTrack( float targetvol, int time, int num );
int             trap_S_StartStreamingSound( const char *intro, const char *loop, int entnum, int channel, int attenuation );
void            trap_R_LoadWorldMap( const char *mapname );
qhandle_t       trap_R_RegisterModel( const char *name );
qhandle_t       trap_R_RegisterSkin( const char *name );
qboolean        trap_R_GetSkinModel( qhandle_t skinid, const char *type, char *name );
qhandle_t       trap_R_GetShaderFromModel( qhandle_t modelid, int surfnum, int withlightmap );
qhandle_t       trap_R_RegisterShader( const char *name );
void            trap_R_RegisterFont( const char *fontName, const char *fallbackName, int pointSize, fontMetrics_t * );
void            trap_R_Glyph( fontHandle_t, const char *str, glyphInfo_t *glyph );
void            trap_R_GlyphChar( fontHandle_t, int ch, glyphInfo_t *glyph );
void            trap_R_UnregisterFont( fontHandle_t );

qhandle_t       trap_R_RegisterShaderNoMip( const char *name );
qhandle_t       trap_R_RegisterShaderLightAttenuation( const char *name );
void            trap_R_ClearScene( void );
void            trap_R_AddRefEntityToScene( const refEntity_t *re );
void            trap_R_AddRefLightToScene( const refLight_t *light );
void            trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts );
void            trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );
void            trap_R_AddPolyBufferToScene( polyBuffer_t *pPolyBuffer );
void            trap_R_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags );
void            trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void            trap_GS_FS_Seek( fileHandle_t f, long offset, fsOrigin_t origin );
void            trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible );
void            trap_R_SetFog( int fogvar, int var1, int var2, float r, float g, float b, float density );
void            trap_R_SetGlobalFog( qboolean restore, int duration, float r, float g, float b, float depthForOpaque );
void            trap_R_RenderScene( const refdef_t *fd );
void            trap_R_SaveViewParms( void );
void            trap_R_RestoreViewParms( void );
void            trap_R_SetColor( const float *rgba );
void            trap_R_SetClipRegion( const float *region );
void            trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void            trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );
void            trap_R_DrawStretchPicGradient( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, const float *gradientColor, int gradientType );
void            trap_R_Add2dPolys( polyVert_t *verts, int numverts, qhandle_t hShader );
void            trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int             trap_R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
void            trap_GetGlconfig( glconfig_t *glconfig );
void            trap_GetGameState( gameState_t *gamestate );
void            trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );
qboolean        trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );
qboolean        trap_GetServerCommand( int serverCommandNumber );
int             trap_GetCurrentCmdNumber( void );
qboolean        trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );
void            trap_SetUserCmdValue( int stateValue, int flags, float sensitivityScale, int mpIdentClient );
void            trap_SetClientLerpOrigin( float x, float y, float z );
int             trap_MemoryRemaining( void );
qboolean        trap_Key_IsDown( int keynum );
int             trap_Key_GetCatcher( void );
void            trap_Key_SetCatcher( int catcher );
int             trap_Key_GetKey( const char *binding );
qboolean        trap_Key_GetOverstrikeMode( void );
void            trap_Key_SetOverstrikeMode( qboolean state );
int             trap_PC_AddGlobalDefine( const char *define );
int             trap_PC_LoadSource( const char *filename );
int             trap_PC_FreeSource( int handle );
int             trap_PC_ReadToken( int handle, pc_token_t *pc_token );
int             trap_PC_SourceFileAndLine( int handle, char *filename, int *line );
int             trap_PC_UnReadToken( int handle );
void            trap_S_StopBackgroundTrack( void );
int             trap_RealTime( qtime_t *qtime );
void            trap_SnapVector( float *v );
int             trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits );
e_status        trap_CIN_StopCinematic( int handle );
e_status        trap_CIN_RunCinematic( int handle );
void            trap_CIN_DrawCinematic( int handle );
void            trap_CIN_SetExtents( int handle, int x, int y, int w, int h );
void            trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );
qboolean        trap_loadCamera( int camNum, const char *name );
void            trap_startCamera( int camNum, int time );
void            trap_stopCamera( int camNum );
qboolean        trap_getCameraInfo( int camNum, int time, vec3_t *origin, vec3_t *angles, float *fov );
qboolean        trap_GetEntityToken( char *buffer, int bufferSize );
void            trap_UI_Popup( int arg0 );
void            trap_UI_ClosePopup( const char *arg0 );
void            trap_Key_GetBindingBuf( int keynum, char *buf, int buflen );
void            trap_Key_SetBinding( int keynum, const char *binding );
int             trap_Parse_AddGlobalDefine( const char *define );
int             trap_Parse_LoadSource( const char *filename );
int             trap_Parse_FreeSource( int handle );
int             trap_Parse_ReadToken( int handle, pc_token_t *pc_token );
int             trap_Parse_SourceFileAndLine( int handle, char *filename, int *line );
void            trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void            trap_Key_KeysForBinding( const char *binding, int *key1, int *key2 );
void            trap_CG_TranslateString( const char *string, char *buf );
void            trap_S_FadeAllSound( float targetvol, int time, qboolean stopsounds );
qboolean        trap_R_inPVS( const vec3_t p1, const vec3_t p2 );
void            trap_GetHunkData( int *hunkused, int *hunkexpected );
qboolean        trap_R_LoadDynamicShader( const char *shadername, const char *shadertext );
void            trap_R_RenderToTexture( int textureid, int x, int y, int w, int h );
int             trap_R_GetTextureId( const char *name );
void            trap_R_Finish( void );
void            trap_GetDemoName( char *buffer, int size );
void            trap_S_StartSoundVControl( vec3_t origin, int entityNum, int entchannel, sfxHandle_t sfx, int volume );
int             trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )
qhandle_t       trap_R_RegisterAnimation( const char *name );
int             trap_R_CheckSkeleton( refSkeleton_t *skel, qhandle_t hModel, qhandle_t hAnim );
int             trap_R_BuildSkeleton( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac, qboolean clearOrigin );
int             trap_R_BlendSkeleton( refSkeleton_t *skel, const refSkeleton_t *blend, float frac );
int             trap_R_BoneIndex( qhandle_t hModel, const char *boneName );
int             trap_R_AnimNumFrames( qhandle_t hAnim );
int             trap_R_AnimFrameRate( qhandle_t hAnim );

#endif
void            trap_CompleteCallback( const char *complete );

void            trap_RegisterButtonCommands( const char *cmds );

void            trap_GetClipboardData( char *, int, clipboard_t );
void            trap_QuoteString( const char *, char*, int );
void            trap_Gettext( char *buffer, const char *msgid, int bufferLength );
void            trap_Pgettext( char *buffer, const char *ctxt, const char *msgid, int bufferLength );
