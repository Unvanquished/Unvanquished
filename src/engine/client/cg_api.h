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


#include "../qcommon/q_shared.h"
#include "../renderer/tr_types.h"
#include "../../common/cm/cm_public.h"

#define CGAME_API_VERSION 1

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

typedef enum cgameImport_s
{
  // Misc
  CG_GETDEMOSTATE,
  CG_FS_SEEK,
  CG_GETDEMOPOS,
  CG_SENDCLIENTCOMMAND,
  CG_UPDATESCREEN,
  CG_CM_MARKFRAGMENTS,
  CG_REAL_TIME,
  CG_GETGLCONFIG,
  CG_GETGAMESTATE,
  CG_GETCLIENTSTATE,
  CG_GETCURRENTSNAPSHOTNUMBER,
  CG_GETSNAPSHOT,
  CG_GETSERVERCOMMAND,
  CG_GETCURRENTCMDNUMBER,
  CG_GETUSERCMD,
  CG_SETUSERCMDVALUE,
  CG_SETCLIENTLERPORIGIN,
  CG_GET_ENTITY_TOKEN,
  CG_GETDEMONAME,
  CG_REGISTER_BUTTON_COMMANDS,
  CG_GETCLIPBOARDDATA,
  CG_QUOTESTRING,
  CG_GETTEXT,
  CG_PGETTEXT,
  CG_GETTEXT_PLURAL,
  CG_NOTIFY_TEAMCHANGE,
  CG_PREPAREKEYUP,
  CG_GETNEWS,

  // Sound
  CG_S_STARTSOUND,
  CG_S_STARTLOCALSOUND,
  CG_S_CLEARLOOPINGSOUNDS,
  CG_S_ADDLOOPINGSOUND,
  CG_S_STOPLOOPINGSOUND,
  CG_S_UPDATEENTITYPOSITION,
  CG_S_RESPATIALIZE,
  CG_S_REGISTERSOUND,
  CG_S_STARTBACKGROUNDTRACK,
  CG_S_STOPBACKGROUNDTRACK,
  CG_S_UPDATEENTITYVELOCITY,
  CG_S_SETREVERB,
  CG_S_BEGINREGISTRATION,
  CG_S_ENDREGISTRATION,

  // Renderer
  CG_R_SETALTSHADERTOKENS,
  CG_R_GETSHADERNAMEFROMHANDLE,
  CG_R_SCISSOR_ENABLE,
  CG_R_SCISSOR_SET,
  CG_R_INPVVS,
  CG_R_LOADWORLDMAP,
  CG_R_REGISTERMODEL,
  CG_R_REGISTERSKIN,
  CG_R_REGISTERSHADER,
  CG_R_REGISTERFONT,
  CG_R_CLEARSCENE,
  CG_R_ADDREFENTITYTOSCENE,
  CG_R_ADDPOLYTOSCENE,
  CG_R_ADDPOLYSTOSCENE,
  CG_R_ADDLIGHTTOSCENE,
  CG_R_ADDADDITIVELIGHTTOSCENE,
  CG_R_RENDERSCENE,
  CG_R_SETCOLOR,
  CG_R_SETCLIPREGION,
  CG_R_DRAWSTRETCHPIC,
  CG_R_DRAWROTATEDPIC,
  CG_R_MODELBOUNDS,
  CG_R_LERPTAG,
  CG_R_REMAP_SHADER,
  CG_R_INPVS,
  CG_R_LIGHTFORPOINT,
  CG_R_REGISTERANIMATION,
  CG_R_BUILDSKELETON,
  CG_R_BLENDSKELETON,
  CG_R_BONEINDEX,
  CG_R_ANIMNUMFRAMES,
  CG_R_ANIMFRAMERATE,
  CG_REGISTERVISTEST,
  CG_ADDVISTESTTOSCENE,
  CG_CHECKVISIBILITY,
  CG_UNREGISTERVISTEST,
  CG_SETCOLORGRADING,

  // Keys
  CG_KEY_GETCATCHER,
  CG_KEY_SETCATCHER,
  CG_KEY_GETBINDINGBUF,
  CG_KEY_KEYNUMTOSTRINGBUF,

  // Lan
  CG_LAN_GETSERVERCOUNT,
  CG_LAN_GETSERVERINFO,
  CG_LAN_GETSERVERPING,
  CG_LAN_MARKSERVERVISIBLE,
  CG_LAN_SERVERISVISIBLE,
  CG_LAN_UPDATEVISIBLEPINGS,
  CG_LAN_RESETPINGS,
  CG_LAN_SERVERSTATUS,

  // Rocket
  CG_ROCKET_INIT,
  CG_ROCKET_SHUTDOWN,
  CG_ROCKET_LOADDOCUMENT,
  CG_ROCKET_LOADCURSOR,
  CG_ROCKET_DOCUMENTACTION,
  CG_ROCKET_GETEVENT,
  CG_ROCKET_DELELTEEVENT,
  CG_ROCKET_REGISTERDATASOURCE,
  CG_ROCKET_DSADDROW,
  CG_ROCKET_DSCLEARTABLE,
  CG_ROCKET_SETINNERRML,
  CG_ROCKET_GETATTRIBUTE,
  CG_ROCKET_SETATTRIBUTE,
  CG_ROCKET_GETPROPERTY,
  CG_ROCKET_SETPROPERYBYID,
  CG_ROCKET_GETEVENTPARAMETERS,
  CG_ROCKET_REGISTERDATAFORMATTER,
  CG_ROCKET_DATAFORMATTERRAWDATA,
  CG_ROCKET_DATAFORMATTERFORMATTEDDATA,
  CG_ROCKET_REGISTERELEMENT,
  CG_ROCKET_GETELEMENTTAG,
  CG_ROCKET_GETELEMENTABSOLUTEOFFSET,
  CG_ROCKET_QUAKETORML,
  CG_ROCKET_SETCLASS,
  CG_ROCKET_INITHUDS,
  CG_ROCKET_LOADUNIT,
  CG_ROCKET_ADDUNITTOHUD,
  CG_ROCKET_SHOWHUD,
  CG_ROCKET_CLEARHUD,
  CG_ROCKET_ADDTEXT,
  CG_ROCKET_CLEARTEXT,
  CG_ROCKET_REGISTERPROPERTY,
  CG_ROCKET_SHOWSCOREBOARD,
  CG_ROCKET_SETDATASELECTINDEX
} cgameImport_t;

// All Miscs

// GetDemoStateMsg TODO send it at the beginning of the frame
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETDEMOSTATE>>,
	IPC::Reply<int>
> GetDemoStateMsg;
// FSSeekMsg
typedef IPC::Message<IPC::Id<VM::QVM, CG_FS_SEEK>, int, long, int> FSSeekMsg;
// GetDemoPosMsg TODO send it at the beginning of the frame
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETDEMOPOS>>,
	IPC::Reply<int>
> GetDemoPosMsg;
// SendClientCommandMsg TODO really sync?
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_SENDCLIENTCOMMAND>, std::string>
> SendClientCommandMsg;
// UpdateScreenMsg TODO really sync?
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_UPDATESCREEN>>
> UpdateScreenMsg;
// CMMarkFragmentsMsg TODO can move to VM too ?
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_CM_MARKFRAGMENTS>, std::vector<std::array<float, 3>>, std::array<float, 3>, int, int>,
	IPC::Reply<std::vector<std::array<float, 3>>, std::vector<markFragment_t>>
> CMMarkFragmentsMsg;
// RealTimeMsg TODO do with nacl API
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_REAL_TIME>>,
	IPC::Reply<int, qtime_t>
> RealTimeMsg;
// GetGLConfigMsg TODO send it only once?
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETGLCONFIG>>,
	IPC::Reply<glconfig_t>
> GetGLConfigMsg;
// GetGameStateMsg TODO send it only once or per frame?
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETGAMESTATE>>,
	IPC::Reply<gameState_t>
> GetGameStateMsg;
// GetClientStateMsg TODO send it only once or per frame?
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETCLIENTSTATE>>,
	IPC::Reply<cgClientState_t>
> GetClientStateMsg;
// TODO send all snapshots at the beginning of the frame
// GetCurrentSnapshotNumberMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETCURRENTSNAPSHOTNUMBER>>,
	IPC::Reply<int, int>
> GetCurrentSnapshotNumberMsg;
// GetSnapshotMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETSNAPSHOT>, int>,
	IPC::Reply<bool, snapshot_t>
> GetSnapshotMsg;
// GetServerCommandMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETSERVERCOMMAND>, int>,
	IPC::Reply<bool>
> GetServerCommandMsg;
// GetCurrentCmdNumberMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETCURRENTCMDNUMBER>>,
	IPC::Reply<int>
> GetCurrentCmdNumberMsg;
// GetUserCmdMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETUSERCMD>, int>,
	IPC::Reply<bool, usercmd_t>
> GetUserCmdMsg;
// SetUserCmdValueMsg TODO check it is async
typedef IPC::Message<IPC::Id<VM::QVM, CG_SETUSERCMDVALUE>, int, int, float, int> SetUserCmdValueMsg;
// SetClientLerpOriginMsg TODO check it is async
typedef IPC::Message<IPC::Id<VM::QVM, CG_SETCLIENTLERPORIGIN>, float, float, float> SetClientLerpOriginMsg;
// GetEntityTokenMsg TODO what?
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GET_ENTITY_TOKEN>, int>,
	IPC::Reply<bool, std::string>
> GetEntityTokenMsg;
// GetDemoNameMsg TODO send only once per frame?
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETDEMONAME>, int>,
	IPC::Reply<std::string>
> GetDemoNameMsg;
// RegisterButtonCommandsMsg TODO check it is async
typedef IPC::Message<IPC::Id<VM::QVM, CG_REGISTER_BUTTON_COMMANDS>, std::string> RegisterButtonCommandsMsg;
// GetClipboardDataMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETCLIPBOARDDATA>, int, int>,
	IPC::Reply<std::string>
> GetClipboardDataMsg;
// QuoteStringMsg TODO using Command.h for that ?
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_QUOTESTRING>, int, std::string>,
	IPC::Reply<std::string>
> QuoteStringMsg;
// GettextMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETTEXT>, int, std::string>,
	IPC::Reply<std::string>
> GettextMsg;
// PGettextMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_PGETTEXT>, int, std::string, std::string>,
	IPC::Reply<std::string>
> PGettextMsg;
// GettextPluralMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETTEXT_PLURAL>, int, std::string, std::string, int>,
	IPC::Reply<std::string>
> GettextPluralMsg;
// NotifyTeamChangeMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_NOTIFY_TEAMCHANGE>, int>
> NotifyTeamChangeMsg;
// PrepareKeyUpMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_PREPAREKEYUP>>
> PrepareKeyUpMsg;
// GetNewsMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_GETNEWS>, bool>,
	IPC::Reply<bool>
> GetNewsMsg;

// All Sounds

namespace Audio {
	// StartSoundMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_STARTSOUND>, std::array<float, 3>, int, int> StartSoundMsg;
	// StartLocalSoundMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_STARTLOCALSOUND>, int> StartLocalSoundMsg;
	// ClearLoopingSoundsMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_CLEARLOOPINGSOUNDS>> ClearLoopingSoundsMsg;
	// AddLoopingSoundMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_ADDLOOPINGSOUND>, int, int> AddLoopingSoundMsg;
	// StopLoopingSoundMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_STOPLOOPINGSOUND>, int> StopLoopingSoundMsg;
	// UpdateEntityPositionMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_UPDATEENTITYPOSITION>, int, std::array<float, 3>> UpdateEntityPositionMsg;
	// RespatializeMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_RESPATIALIZE>, int, std::array<float, 9>> RespatializeMsg;
	// RegisterSoundMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_S_REGISTERSOUND>, std::string>,
		IPC::Reply<int>
	> RegisterSoundMsg;
	// StartBackgroundTrackMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_STARTBACKGROUNDTRACK>, std::string, std::string> StartBackgroundTrackMsg;
	// StopBackgroundTrackMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_STOPBACKGROUNDTRACK>> StopBackgroundTrackMsg;
	// UpdateEntityVelocityMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_UPDATEENTITYVELOCITY>, int, std::array<float, 3>> UpdateEntityVelocityMsg;
	// SetReverbMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_SETREVERB>, int, std::string, float> SetReverbMsg;
	// BeginRegistrationMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_BEGINREGISTRATION>> BeginRegistrationMsg;
	// EndRegistrationMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_S_ENDREGISTRATION>> EndRegistrationMsg;
}

namespace Render {
	// SetAltShaderTokenMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_SETALTSHADERTOKENS>, std::string> SetAltShaderTokenMsg;
	// GetShaderNameFromHandleMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_GETSHADERNAMEFROMHANDLE>, int, int>,
		IPC::Reply<std::string>
	> GetShaderNameFromHandleMsg;
	// ScissorEnableMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_SCISSOR_ENABLE>, bool> ScissorEnableMsg;
	// ScissorSetMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_SCISSOR_SET>, int, int, int, int> ScissorSetMsg;
	// InPVVSMsg //TODO not a renderer call, handle in CM in the VM?
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_INPVVS>, std::array<float, 3>, std::array<float, 3>>,
		IPC::Reply<bool>
	> InPVVSMsg;
	// LoadWorldMapMsg //TODO is it realy async?
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_LOADWORLDMAP>, std::string> LoadWorldMapMsg;
	// RegisterModelMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_REGISTERMODEL>, std::string>,
		IPC::Reply<int>
	> RegisterModelMsg;
	// RegisterSkinMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_REGISTERSKIN>, std::string>,
		IPC::Reply<int>
	> RegisterSkinMsg;
	// RegisterShaderMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_REGISTERSHADER>, std::string, int>,
		IPC::Reply<int>
	> RegisterShaderMsg;
	// RegisterFontMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_REGISTERFONT>, std::string, std::string, int>,
		IPC::Reply<fontMetrics_t>
	> RegisterFontMsg;
	// ClearSceneMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_CLEARSCENE>> ClearSceneMsg;
	// AddRefEntityToSceneMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_ADDREFENTITYTOSCENE>, refEntity_t> AddRefEntityToSceneMsg;
	// AddPolyToSceneMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_ADDPOLYTOSCENE>, int, std::vector<polyVert_t>> AddPolyToSceneMsg;
	// AddPolysToSceneMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_ADDPOLYSTOSCENE>, int, std::vector<polyVert_t>, int> AddPolysToSceneMsg;
	// AddLightToSceneMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_ADDLIGHTTOSCENE>, std::array<float, 3>, float, float, float, float, float, int, int> AddLightToSceneMsg;
	// AddAdditiveLightToSceneMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_ADDADDITIVELIGHTTOSCENE>, std::array<float, 3>, float, float, float, float> AddAdditiveLightToSceneMsg;
	// RenderSceneMsg //TODO check async
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_RENDERSCENE>, refdef_t> RenderSceneMsg;
	// SetColorMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_SETCOLOR>, std::array<float, 4>> SetColorMsg;
	// SetClipRegionMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_SETCLIPREGION>, std::array<float, 4>> SetClipRegionMsg;
	// DrawStretchPicMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_DRAWSTRETCHPIC>, float, float, float, float, float, float, float, float, int> DrawStretchPicMsg;
	// DrawRotatedPicMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_DRAWROTATEDPIC>, float, float, float, float, float, float, float, float, int, float> DrawRotatedPicMsg;
	// ModelBoundsMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_MODELBOUNDS>, int>,
		IPC::Reply<std::array<float, 3>, std::array<float, 3>>
	> ModelBoundsMsg;
	// LerpTagMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_LERPTAG>, refEntity_t, std::string, int>,
		IPC::Reply<orientation_t, int>
	> LerpTagMsg;
	// RemapShaderMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_R_REMAP_SHADER>, std::string, std::string, std::string> RemapShaderMsg;
	// InPVSMsg //TODO not a renderer call, handle in CM in the VM?
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_INPVS>, std::array<float, 3>, std::array<float, 3>>,
		IPC::Reply<bool>
	> InPVSMsg;
	// LightForPointMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_LIGHTFORPOINT>, std::array<float, 3>>,
		IPC::Reply<std::array<float, 3>, std::array<float, 3>, std::array<float, 3>, int>
	> LightForPointMsg;
	// RegisterAnimationMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_REGISTERANIMATION>, std::string>,
		IPC::Reply<int>
	> RegisterAnimationMsg;
	// BuildSkeletonMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_BUILDSKELETON>, int, int, int, float, bool>,
		IPC::Reply<refSkeleton_t, int>
	> BuildSkeletonMsg;
	// BlendSkeletonMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_BLENDSKELETON>, refSkeleton_t, refSkeleton_t, float>,
		IPC::Reply<refSkeleton_t, int>
	> BlendSkeletonMsg;
	// BoneIndexMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_BONEINDEX>, int, std::string>,
		IPC::Reply<int>
	> BoneIndexMsg;
	// AnimNumFramesMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_ANIMNUMFRAMES>, int>,
		IPC::Reply<int>
	> AnimNumFramesMsg;
	// AnimFrameRateMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_R_ANIMFRAMERATE>, int>,
		IPC::Reply<int>
	> AnimFrameRateMsg;
	// RegisterVisTestMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_REGISTERVISTEST>>,
		IPC::Reply<int>
	> RegisterVisTestMsg;
	// AddVisTextToSceneMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_ADDVISTESTTOSCENE>, int, std::array<float, 3>, float, float> AddVisTestToSceneMsg;
	// CheckVisibilityMsg
	typedef IPC::SyncMessage<
		IPC::Message<IPC::Id<VM::QVM, CG_CHECKVISIBILITY>, int>,
		IPC::Reply<float>
	> CheckVisibilityMsg;
	// UnregisterVisTestMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_UNREGISTERVISTEST>, int> UnregisterVisTestMsg;
	// SetColorGradingMsg
	typedef IPC::Message<IPC::Id<VM::QVM, CG_SETCOLORGRADING>, int, int> SetColorGradingMsg;
}

typedef enum
{
  CG_STATIC_INIT,

  CG_INIT,
//  void CG_Init( int serverMessageNum, int serverCommandSequence )
  // called when the level loads or when the renderer is restarted
  // all media should be registered at this time
  // cgame will display loading status by calling SCR_Update, which
  // will call CG_DrawInformation during the loading process
  // reliableCommandSequence will be 0 on fresh loads, but higher for
  // demos or vid_restarts

  CG_SHUTDOWN,
//  void (*CG_Shutdown)( void );
  // oportunity to flush and close any open files

  CG_DRAW_ACTIVE_FRAME,
//  void (*CG_DrawActiveFrame)( int serverTime, stereoFrame_t stereoView, qboolean demoPlayback );
  // Generates and draws a game scene and status information at the given time.
  // If demoPlayback is set, local movement prediction will not be enabled

  CG_CROSSHAIR_PLAYER,
//  int (*CG_CrosshairPlayer)( void );

  CG_KEY_EVENT,
//  void    (*CG_KeyEvent)( int key, qboolean down );

  CG_MOUSE_EVENT,
//  void    (*CG_MouseEvent)( int dx, int dy );

  CG_VOIP_STRING,
// char *(*CG_VoIPString)( void );
// returns a string of comma-delimited clientnums based on cl_voipSendTarget

  CG_INIT_CVARS,
// registers cvars only then shuts down; call instead of CG_INIT for this purpose

  CG_ROCKET_VM_INIT,
// Inits libRocket in the game.

  CG_ROCKET_FRAME,
// Rocket runs through a frame, including event processing

  CG_ROCKET_FORMAT_DATA,
// Rocket wants some data formatted

  CG_ROCKET_RENDER_ELEMENT,
// Rocket wants an element renderered

  CG_ROCKET_PROGRESSBAR_VALUE
// Rocket wants to query the value of a progress bar
} cgameExport_t;

// CGameStaticInitMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_STATIC_INIT>>
> CGameStaticInitMsg;
// CGameInitMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_INIT>, int, int, int, bool>
> CGameInitMsg;
// CGameShutdownMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_SHUTDOWN>>
> CGameShutdownMsg;
// CGameDrawActiveFrameMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_DRAW_ACTIVE_FRAME>, int, stereoFrame_t, bool>
> CGameDrawActiveFrameMsg;
// CGameCrosshairPlayerMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_CROSSHAIR_PLAYER>>,
	IPC::Reply<int>
> CGameCrosshairPlayerMsg;
// CGameKeyEventMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_KEY_EVENT>, int, bool>
> CGameKeyEventMsg;
// CGameMouseEventMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_MOUSE_EVENT>, int, int>
> CGameMouseEventMsg;
// CGameVoipStringMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_VOIP_STRING>>,
	IPC::Reply<std::vector<std::string>>
> CGameVoipStringMsg;
// CGameInitCvarsMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_INIT_CVARS>>
> CGameInitCvarsMsg;

//TODO Check all rocket calls
// CGameRocketInitMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_VM_INIT>>
> CGameRocketInitMsg;
// CGameRocketFrameMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_FRAME>>
> CGameRocketFrameMsg;
// CGameRocketFormatDataMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_FORMAT_DATA>, int>
> CGameRocketFormatDataMsg;
// CGameRocketRenderElementMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_RENDER_ELEMENT>>
> CGameRocketRenderElementMsg;
// CGameRocketProgressbarValueMsg
typedef IPC::SyncMessage<
	IPC::Message<IPC::Id<VM::QVM, CG_ROCKET_PROGRESSBAR_VALUE>>,
	IPC::Reply<float>
> CGameRocketProgressbarValueMsg;


void            trap_Print( const char *string );
void NORETURN   trap_Error( const char *string );
int             trap_Milliseconds( void );
void            trap_Cvar_Register( vmCvar_t *vmCvar, const char *varName, const char *defaultValue, int flags );
void            trap_Cvar_Update( vmCvar_t *vmCvar );
void            trap_Cvar_Set( const char *var_name, const char *value );
void            trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );
void            trap_Cvar_LatchedVariableStringBuffer( const char *var_name, char *buffer, int bufsize );
int             trap_Cvar_VariableIntegerValue( const char *var_name );
float           trap_Cvar_VariableValue( const char *var_name );
int             trap_Argc( void );
void            trap_Argv( int n, char *buffer, int bufferLength );
void            trap_EscapedArgs( char *buffer, int bufferLength );
void            trap_LiteralArgs( char *buffer, int bufferLength );
int             trap_GetDemoState( void );
int             trap_GetDemoPos( void );
int             trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void            trap_FS_Read( void *buffer, int len, fileHandle_t f );
void            trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void            trap_FS_FCloseFile( fileHandle_t f );
int             trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
int             trap_FS_Delete( const char *filename );
qboolean            trap_FS_LoadPak( const char *pak );
void            trap_SendConsoleCommand( const char *text );
void            trap_AddCommand( const char *cmdName );
void            trap_RemoveCommand( const char *cmdName );
void            trap_SendClientCommand( const char *s );
void            trap_UpdateScreen( void );
#define trap_CM_LoadMap(a) CM_LoadMap(a, nullptr, false)
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
void            trap_S_ClearLoopingSounds( qboolean killall );
void            trap_S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx );
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
int             trap_S_StartStreamingSound( const char *intro, const char *loop, int entnum, int channel, int attenuation );
void            trap_R_LoadWorldMap( const char *mapname );
qhandle_t       trap_R_RegisterModel( const char *name );
qhandle_t       trap_R_RegisterSkin( const char *name );
qhandle_t       trap_R_RegisterShader( const char *name, RegisterShaderFlags_t flags );
void            trap_R_RegisterFont( const char *fontName, const char *fallbackName, int pointSize, fontMetrics_t * );

void            trap_R_ClearScene( void );
void            trap_R_AddRefEntityToScene( const refEntity_t *re );
void            trap_R_AddPolyToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts );
void            trap_R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys );
void            trap_R_AddLightToScene( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags );
void            trap_R_AddAdditiveLightToScene( const vec3_t org, float intensity, float r, float g, float b );
void            trap_GS_FS_Seek( fileHandle_t f, long offset, fsOrigin_t origin );
void            trap_R_RenderScene( const refdef_t *fd );
void            trap_R_SetColor( const float *rgba );
void            trap_R_SetClipRegion( const float *region );
void            trap_R_DrawStretchPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader );
void            trap_R_DrawRotatedPic( float x, float y, float w, float h, float s1, float t1, float s2, float t2, qhandle_t hShader, float angle );
void            trap_R_ModelBounds( clipHandle_t model, vec3_t mins, vec3_t maxs );
int             trap_R_LerpTag( orientation_t *tag, const refEntity_t *refent, const char *tagName, int startIndex );
void            trap_GetGlconfig( glconfig_t *glconfig );
void            trap_GetGameState( gameState_t *gamestate );
void            trap_GetClientState( cgClientState_t *cstate );
void            trap_GetCurrentSnapshotNumber( int *snapshotNumber, int *serverTime );
qboolean        trap_GetSnapshot( int snapshotNumber, snapshot_t *snapshot );
qboolean        trap_GetServerCommand( int serverCommandNumber );
int             trap_GetCurrentCmdNumber( void );
qboolean        trap_GetUserCmd( int cmdNumber, usercmd_t *ucmd );
void            trap_SetUserCmdValue( int stateValue, int flags, float sensitivityScale, int mpIdentClient );
void            trap_SetClientLerpOrigin( float x, float y, float z );
int             trap_Key_GetCatcher( void );
void            trap_Key_SetCatcher( int catcher );
void            trap_S_StopBackgroundTrack( void );
int             trap_RealTime( qtime_t *qtime );
int             trap_CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits );
e_status        trap_CIN_StopCinematic( int handle );
e_status        trap_CIN_RunCinematic( int handle );
void            trap_CIN_DrawCinematic( int handle );
void            trap_CIN_SetExtents( int handle, int x, int y, int w, int h );
void            trap_R_RemapShader( const char *oldShader, const char *newShader, const char *timeOffset );
qboolean        trap_GetEntityToken( char *buffer, int bufferSize );
void            trap_UI_Popup( int arg0 );
void            trap_UI_ClosePopup( const char *arg0 );
void            trap_Key_GetBindingBuf( int keynum, int team, char *buf, int buflen );
int             trap_Parse_AddGlobalDefine( const char *define );
int             trap_Parse_LoadSource( const char *filename );
int             trap_Parse_FreeSource( int handle );
int             trap_Parse_ReadToken( int handle, pc_token_t *pc_token );
int             trap_Parse_SourceFileAndLine( int handle, char *filename, int *line );
void            trap_Key_KeynumToStringBuf( int keynum, char *buf, int buflen );
void            trap_CG_TranslateString( const char *string, char *buf );
qboolean        trap_R_inPVS( const vec3_t p1, const vec3_t p2 );
qboolean        trap_R_inPVVS( const vec3_t p1, const vec3_t p2 );
qboolean        trap_R_LoadDynamicShader( const char *shadername, const char *shadertext );
void            trap_GetDemoName( char *buffer, int size );
int             trap_R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir );

qhandle_t       trap_R_RegisterAnimation( const char *name );
int             trap_R_BuildSkeleton( refSkeleton_t *skel, qhandle_t anim, int startFrame, int endFrame, float frac, qboolean clearOrigin );
int             trap_R_BlendSkeleton( refSkeleton_t *skel, const refSkeleton_t *blend, float frac );
int             trap_R_BoneIndex( qhandle_t hModel, const char *boneName );
int             trap_R_AnimNumFrames( qhandle_t hAnim );
int             trap_R_AnimFrameRate( qhandle_t hAnim );

void            trap_CompleteCallback( const char *complete );

void            trap_RegisterButtonCommands( const char *cmds );

void            trap_GetClipboardData( char *, int, clipboard_t );
void            trap_QuoteString( const char *, char*, int );
void            trap_Gettext( char *buffer, const char *msgid, int bufferLength );
void            trap_Pgettext( char *buffer, const char *ctxt, const char *msgid, int bufferLength );
void            trap_GettextPlural( char *buffer, const char *msgid, const char *msgid2, int number, int bufferLength );

void            trap_notify_onTeamChange( int newTeam );

qhandle_t       trap_RegisterVisTest();
void            trap_AddVisTestToScene( qhandle_t hTest, vec3_t pos,
					float depthAdjust, float area );
float           trap_CheckVisibility( qhandle_t hTest );
void            trap_UnregisterVisTest( qhandle_t hTest );
void            trap_SetColorGrading( int slot, qhandle_t hShader );
void            trap_R_ScissorEnable( qboolean enable );
void            trap_R_ScissorSet( int x, int y, int w, int h );
int             trap_LAN_GetServerCount( int source );
void            trap_LAN_GetServerInfo( int source, int n, char *buf, int buflen );
int             trap_LAN_GetServerPing( int source, int n );
void            trap_LAN_MarkServerVisible( int source, int n, qboolean visible );
int             trap_LAN_ServerIsVisible( int source, int n );
qboolean        trap_LAN_UpdateVisiblePings( int source );
void            trap_LAN_ResetPings( int n );
int             trap_LAN_ServerStatus( const char *serverAddress, char *serverStatus, int maxLen );
qboolean        trap_GetNews( qboolean force );
void            trap_R_GetShaderNameFromHandle( const qhandle_t shader, char *out, int len );
void            trap_PrepareKeyUp( void );
void            trap_R_SetAltShaderTokens( const char * );
void            trap_S_UpdateEntityVelocity( int entityNum, const vec3_t velocity );
void            trap_S_SetReverb( int slotNum, const char* presetName, float ratio );
void            trap_S_BeginRegistration( void );
void            trap_S_EndRegistration( void );
void            trap_Rocket_Init( void );
void            trap_Rocket_Shutdown( void );
void            trap_Rocket_LoadDocument( const char *path );
void            trap_Rocket_LoadCursor( const char *path );
void            trap_Rocket_DocumentAction( const char *name, const char *action );
qboolean        trap_Rocket_GetEvent( void );
void            trap_Rocket_DeleteEvent( void );
void            trap_Rocket_RegisterDataSource( const char *name );
void            trap_Rocket_DSAddRow( const char *name, const char *table, const char *data );
void            trap_Rocket_DSClearTable( const char *name, const char *table );
void            trap_Rocket_SetInnerRML( const char *RML, int parseFlags );
void            trap_Rocket_GetAttribute( const char *attribute, char *out, int length );
void            trap_Rocket_SetAttribute( const char *attribute, const char *value );
void            trap_Rocket_GetProperty( const char *name, void *out, int len, rocketVarType_t type );
void            trap_Rocket_SetProperty( const char *property, const char *value );
void            trap_Rocket_GetEventParameters( char *params, int length );
void            trap_Rocket_RegisterDataFormatter( const char *name );
void            trap_Rocket_DataFormatterRawData( int handle, char *name, int nameLength, char *data, int dataLength );
void            trap_Rocket_DataFormatterFormattedData( int handle, const char *data, qboolean parseQuake );
void            trap_Rocket_RegisterElement( const char *tag );
void            trap_Rocket_GetElementTag( char *tag, int length );
void            trap_Rocket_GetElementAbsoluteOffset( float *x, float *y );
void            trap_Rocket_QuakeToRML( const char *in, char *out, int length );
void            trap_Rocket_SetClass( const char *in, qboolean activate );
void            trap_Rocket_InitializeHuds( int size );
void            trap_Rocket_LoadUnit( const char *path );
void            trap_Rocket_AddUnitToHud( int weapon, const char *id );
void            trap_Rocket_ShowHud( int weapon );
void            trap_Rocket_ClearHud( int weapon );
void            trap_Rocket_AddTextElement( const char *text, const char *Class, float x, float y );
void            trap_Rocket_ClearText( void );
void            trap_Rocket_RegisterProperty( const char *name, const char *defaultValue, qboolean inherited, qboolean force_layout, const char *parseAs );
void            trap_Rocket_ShowScoreboard( const char *name, qboolean show );
void            trap_Rocket_SetDataSelectIndex( int index );
#endif
