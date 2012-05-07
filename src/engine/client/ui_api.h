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
#include "../renderer/tr_types.h"

#define UI_API_VERSION 4

typedef enum
{
  UI_ERROR,
  UI_PRINT,
  UI_MILLISECONDS,
  UI_CVAR_REGISTER,
  UI_CVAR_UPDATE,
  UI_CVAR_SET,
  UI_CVAR_VARIABLEVALUE,
  UI_CVAR_VARIABLESTRINGBUFFER,
  UI_CVAR_LATCHEDVARIABLESTRINGBUFFER,
  UI_CVAR_SETVALUE,
  UI_CVAR_RESET,
  UI_CVAR_CREATE,
  UI_CVAR_INFOSTRINGBUFFER,
  UI_ARGC,
  UI_ARGV,
  UI_CMD_EXECUTETEXT,
  UI_ADDCOMMAND,
  UI_FS_FOPENFILE,
  UI_FS_READ,
  UI_FS_WRITE,
  UI_FS_FCLOSEFILE,
  UI_FS_DELETEFILE,
  UI_FS_GETFILELIST,
  UI_FS_SEEK,
  UI_R_REGISTERMODEL,
  UI_R_REGISTERSKIN,
  UI_R_REGISTERSHADERNOMIP,
  UI_R_CLEARSCENE,
  UI_R_ADDREFENTITYTOSCENE,
  UI_R_ADDPOLYTOSCENE,
  UI_R_ADDPOLYSTOSCENE,
  UI_R_ADDLIGHTTOSCENE,
  UI_R_ADDCORONATOSCENE,
  UI_R_RENDERSCENE,
  UI_R_SETCOLOR,
  UI_R_SETCLIPREGION,
  UI_R_DRAW2DPOLYS,
  UI_R_DRAWSTRETCHPIC,
  UI_R_DRAWROTATEDPIC,
  UI_R_MODELBOUNDS,
  UI_UPDATESCREEN,
  UI_CM_LERPTAG,
  UI_S_REGISTERSOUND,
  UI_S_STARTLOCALSOUND,
  UI_S_FADESTREAMINGSOUND,
  UI_S_FADEALLSOUNDS,
  UI_KEY_KEYNUMTOSTRINGBUF,
  UI_KEY_GETBINDINGBUF,
  UI_KEY_SETBINDING,
  UI_KEY_BINDINGTOKEYS,
  UI_KEY_ISDOWN,
  UI_KEY_GETOVERSTRIKEMODE,
  UI_KEY_SETOVERSTRIKEMODE,
  UI_KEY_CLEARSTATES,
  UI_KEY_GETCATCHER,
  UI_KEY_SETCATCHER,
  UI_GETCLIPBOARDDATA,
  UI_GETCLIENTSTATE,
  UI_GETGLCONFIG,
  UI_GETCONFIGSTRING,
  UI_LAN_LOADCACHEDSERVERS,
  UI_LAN_SAVECACHEDSERVERS,
  UI_LAN_ADDSERVER,
  UI_LAN_REMOVESERVER,
  UI_LAN_GETPINGQUEUECOUNT,
  UI_LAN_CLEARPING,
  UI_LAN_GETPING,
  UI_LAN_GETPINGINFO,
  UI_LAN_GETSERVERCOUNT,
  UI_LAN_GETSERVERADDRESSSTRING,
  UI_LAN_GETSERVERINFO,
  UI_LAN_GETSERVERPING,
  UI_LAN_MARKSERVERVISIBLE,
  UI_LAN_SERVERISVISIBLE,
  UI_LAN_UPDATEVISIBLEPINGS,
  UI_LAN_RESETPINGS,
  UI_LAN_SERVERSTATUS,
  UI_LAN_SERVERISINFAVORITELIST,
  UI_GETNEWS,
  UI_LAN_COMPARESERVERS,
  UI_MEMORY_REMAINING,
  UI_GET_CDKEY,
  UI_SET_CDKEY,
  UI_R_REGISTERFONT,
  UI_MEMSET,
  UI_MEMCPY,
  UI_STRNCPY,
  UI_SIN,
  UI_COS,
  UI_ATAN2,
  UI_SQRT,
  UI_FLOOR,
  UI_CEIL,
  UI_PARSE_ADD_GLOBAL_DEFINE,
  UI_PARSE_LOAD_SOURCE,
  UI_PARSE_FREE_SOURCE,
  UI_PARSE_READ_TOKEN,
  UI_PARSE_SOURCE_FILE_AND_LINE,
  UI_PC_ADD_GLOBAL_DEFINE,
  UI_PC_REMOVE_ALL_GLOBAL_DEFINES,
  UI_PC_LOAD_SOURCE,
  UI_PC_FREE_SOURCE,
  UI_PC_READ_TOKEN,
  UI_PC_SOURCE_FILE_AND_LINE,
  UI_PC_UNREAD_TOKEN,
  UI_S_STOPBACKGROUNDTRACK,
  UI_S_STARTBACKGROUNDTRACK,
  UI_REAL_TIME,
  UI_CIN_PLAYCINEMATIC,
  UI_CIN_STOPCINEMATIC,
  UI_CIN_RUNCINEMATIC,
  UI_CIN_DRAWCINEMATIC,
  UI_CIN_SETEXTENTS,
  UI_R_REMAP_SHADER,
  UI_CL_GETLIMBOSTRING,
  UI_CL_TRANSLATE_STRING,
  UI_CHECKAUTOUPDATE,
  UI_GET_AUTOUPDATE,
  UI_OPENURL,
  UI_GETHUNKDATA,
#if defined( USE_REFENTITY_ANIMATIONSYSTEM )
  UI_R_REGISTERANIMATION,
  UI_R_BUILDSKELETON,
  UI_R_BLENDSKELETON,
  UI_R_BONEINDEX,
  UI_R_ANIMNUMFRAMES,
  UI_R_ANIMFRAMERATE,
#endif
  UI_GETTEXT = 300,
  UI_R_LOADFACE,
  UI_R_FREEFACE,
  UI_R_LOADGLYPH,
  UI_R_FREEGLYPH,
  UI_R_GLYPH,
  UI_R_FREECACHEDGLYPHS
} uiImport_t;

typedef struct
{
	connstate_t connState;
	int         connectPacketCount;
	int         clientNum;
	char        servername[ MAX_STRING_CHARS ];
	char        updateInfoString[ MAX_STRING_CHARS ];
	char        messageString[ MAX_STRING_CHARS ];
} uiClientState_t;

typedef enum
{
  UIMENU_NONE,
  UIMENU_MAIN,
  UIMENU_INGAME,
  UIMENU_NEED_CD,
  UIMENU_BAD_CD_KEY,
  UIMENU_TEAM,
  UIMENU_POSTGAME,
  UIMENU_HELP,

  UIMENU_WM_QUICKMESSAGE,
  UIMENU_WM_QUICKMESSAGEALT,

  UIMENU_WM_FTQUICKMESSAGE,
  UIMENU_WM_FTQUICKMESSAGEALT,

  UIMENU_WM_TAPOUT,
  UIMENU_WM_TAPOUT_LMS,

  UIMENU_WM_AUTOUPDATE,

  UIMENU_INGAME_OMNIBOTMENU,
} uiMenuCommand_t;

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

typedef enum
{
  UI_GETAPIVERSION = 0, // system reserved

  UI_INIT,
//  void    UI_Init( void );

  UI_SHUTDOWN,
//  void    UI_Shutdown( void );

  UI_KEY_EVENT,
//  void    UI_KeyEvent( int key );

  UI_MOUSE_EVENT,
//  void    UI_MouseEvent( int dx, int dy );

  UI_REFRESH,
//  void    UI_Refresh( int time );

  UI_IS_FULLSCREEN,
//  qboolean UI_IsFullscreen( void );

  UI_SET_ACTIVE_MENU,
//  void    UI_SetActiveMenu( uiMenuCommand_t menu );

  UI_GET_ACTIVE_MENU,
//  void    UI_GetActiveMenu( void );

  UI_CONSOLE_COMMAND,
//  qboolean UI_ConsoleCommand( void );

  UI_DRAW_CONNECT_SCREEN,
//  void    UI_DrawConnectScreen( qboolean overlay );
  UI_HASUNIQUECDKEY,
// if !overlay, the background will be drawn, otherwise it will be
// overlayed over whatever the cgame has drawn.
// a GetClientState syscall will be made to get the current strings
  UI_CHECKEXECKEY, // NERVE - SMF

  UI_WANTSBINDKEYS,

// void UI_ReportHighScoreResponse( void );
  UI_REPORT_HIGHSCORE_RESPONSE,

//When the client has been authorized, send a response, and the token
// void UI_Authorized( int response )
  UI_AUTHORIZED,

// when the client gets an error message from the server
  UI_SERVER_ERRORMESSAGE,

  UI_MOUSE_POSITION,
  //  int   UI_MousePosition( void );

  UI_SET_MOUSE_POSITION,
  //  void  UI_SetMousePosition( int x, int y );
} uiExport_t;

void        trap_Cvar_CopyValue_i( const char *in_var, const char *out_var );
void        trap_Error( const char *string );
void        trap_Print( const char *string );
int         trap_Milliseconds( void );
void        trap_Cvar_Register( vmCvar_t *cvar, const char *var_name, const char *value, int flags );
void        trap_Cvar_Update( vmCvar_t *cvar );
void        trap_Cvar_Set( const char *var_name, const char *value );
float       trap_Cvar_VariableValue( const char *var_name );
void        trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void        trap_Cvar_LatchedVariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void        trap_Cvar_SetValue( const char *var_name, float value );
void        trap_Cvar_Reset( const char *name );
void        trap_Cvar_Create( const char *var_name, const char *var_value, int flags );
void        trap_Cvar_InfoStringBuffer( int bit, char *buffer, int bufsize );
int         trap_Argc( void );
void        trap_Argv( int n, char *buffer, int bufferLength );
void        trap_Cmd_ExecuteText( int exec_when, const char *text );
void        trap_AddCommand( const char *cmdName );
int         trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void        trap_FS_Read( void *buffer, int len, fileHandle_t f );
void        trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void        trap_FS_FCloseFile( fileHandle_t f );
int         trap_FS_Delete( const char *filename );
int         trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
int         trap_FS_Seek( fileHandle_t f, long offset, int origin );
qhandle_t   trap_R_RegisterModel( const char *name );
qhandle_t   trap_R_RegisterSkin( const char *name );
qhandle_t   trap_R_RegisterShaderNoMip( const char *name );
void        trap_R_ClearScene( void );
void        trap_R_AddRefEntityToScene( const refEntity_t *re );
void        trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts );
void        trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );
void        trap_R_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags );
void        trap_R_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible );
void        trap_R_RenderScene( const refdef_t *fd );
void        trap_R_SetColor( const float *rgba );
void        trap_R_SetClipRegion( const float *region );
void        trap_R_Add2dPolys( polyVert_t *verts, int numverts, qhandle_t hShader );
void        trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void        trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );
void        trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
void        trap_UpdateScreen( void );
int         trap_CM_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
sfxHandle_t trap_S_RegisterSound( const char *sample, qboolean compressed );
void        trap_S_StartLocalSound( sfxHandle_t sfx, int channelNum );
void        trap_S_FadeAllSound( float targetvol, int time, qboolean stopsound );
void        trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void        trap_Key_GetBindingBuf( int keynum, char *buf, int buflen );
void        trap_Key_SetBinding( int keynum, const char *binding );
void        trap_Key_KeysForBinding( const char *binding, int *key1, int *key2 );
qboolean    trap_Key_IsDown( int keynum );
qboolean    trap_Key_GetOverstrikeMode( void );
void        trap_Key_SetOverstrikeMode( qboolean state );
void        trap_Key_ClearStates( void );
int         trap_Key_GetCatcher( void );
void        trap_Key_SetCatcher( int catcher );
void        trap_GetClipboardData( char *buf, int bufsize );
void        trap_GetClientState( uiClientState_t *state );
void        trap_GetGlconfig( glconfig_t *glconfig );
int         trap_GetConfigString( int index, char *buff, int buffsize );
void        trap_LAN_LoadCachedServers( void );
void        trap_LAN_SaveCachedServers( void );
int         trap_LAN_AddServer( int source, const char *name, const char *addr );
void        trap_LAN_RemoveServer( int source, const char *addr );
int         trap_LAN_GetPingQueueCount( void );
void        trap_LAN_ClearPing( int n );
void        trap_LAN_GetPing( int n, char *buf, int buflen, int *pingtime );
void        trap_LAN_GetPingInfo( int n, char *buf, int buflen );
int         trap_LAN_GetServerCount( int source );
void        trap_LAN_GetServerAddressString( int source, int n, char *buf, int buflen );
void        trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen );
int         trap_LAN_GetServerPing( int source, int n );
void        trap_LAN_MarkServerVisible( int source, int n, qboolean visible );
int         trap_LAN_ServerIsVisible( int source, int n );
qboolean    trap_LAN_UpdateVisiblePings( int source );
void        trap_LAN_ResetPings( int n );
int         trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen );
qboolean    trap_LAN_ServerIsInFavoriteList( int source, int n );
qboolean    trap_GetNews( qboolean force );
int         trap_LAN_CompareServers( int source, int sortKey, int sortDir, int s1, int s2 );
int         trap_MemoryRemaining( void );
void        trap_GetCDKey( char *buf, int buflen );
void        trap_SetCDKey( char *buf );
void        trap_R_RegisterFont( const char *fontName, int pointSize, fontInfo_t *font );
void        trap_R_LoadFace(const char *fileName, int pointSize, const char *name, face_t *face);
void        trap_R_FreeFace(face_t *face);
void        trap_R_LoadGlyph(face_t *face, const char *str, int img, glyphInfo_t *glyphInfo);
void        trap_R_FreeGlyph(face_t *face, int img, glyphInfo_t *glyphInfo);
void        trap_R_Glyph(fontInfo_t *font, face_t *face, const char *str, glyphInfo_t *glyph);
void        trap_R_FreeCachedGlyphs(face_t *face);
int         trap_Parse_AddGlobalDefine( char *define );
int         trap_Parse_LoadSource( const char *filename );
int         trap_Parse_FreeSource( int handle );
int         trap_Parse_ReadToken( int handle, pc_token_t *pc_token );
int         trap_Parse_SourceFileAndLine( int handle, char *filename, int *line );
int         trap_PC_AddGlobalDefine( char *define );
int         trap_PC_RemoveAllGlobalDefines( void );
int         trap_PC_LoadSource( const char *filename );
int         trap_PC_FreeSource( int handle );
int         trap_PC_ReadToken( int handle, pc_token_t *pc_token );
int         trap_PC_SourceFileAndLine( int handle, char *filename, int *line );
int         trap_PC_UnReadToken( int handle );
void        trap_S_StopBackgroundTrack( void );
void        trap_S_StartBackgroundTrack( const char *intro, const char *loop );
int         trap_RealTime( qtime_t *qtime );
int         trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits );
e_status    trap_CIN_StopCinematic( int handle );
e_status    trap_CIN_RunCinematic( int handle );
void        trap_CIN_DrawCinematic( int handle );
void        trap_CIN_SetExtents( int handle, int x, int y, int w, int h );
void        trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );
qboolean    trap_GetLimboString( int index, char *buf );
char        *trap_TranslateString( const char *string ) __attribute__( ( format_arg( 1 ) ) );
void        trap_CheckAutoUpdate( void );
void        trap_GetAutoUpdate( void );
void        trap_openURL( const char *s );
void        trap_GetHunkData( int *hunkused, int *hunkexpected );
