/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

// client.h -- primary header for client

#ifndef CLIENT_H
#define CLIENT_H

#include "qcommon/q_shared.h"
#include "qcommon/qcommon.h"
#include "renderer/tr_public.h"
#include "keys.h"
#include "audio/Audio.h"
#include "client/cg_api.h"
#include "framework/VirtualMachine.h"
#include "framework/CommonVMServices.h"
#include "framework/CommandBufferHost.h"
#include "common/IPC/CommandBuffer.h"

#define RETRANSMIT_TIMEOUT 3000 // time between connection packet retransmits

// snapshots are a view of the server at a given time
struct clSnapshot_t
{
	bool      valid; // cleared if delta parsing was invalid
	int           snapFlags; // rate delayed and dropped commands

	int           serverTime; // server time the message is valid for (in msec)

	int           messageNum; // copied from netchan->incoming_sequence
	int           deltaNum; // messageNum the delta is from
	int           ping; // time from when cmdNum-1 was sent to time packet was reeceived
	byte          areamask[ MAX_MAP_AREA_BYTES ]; // portalarea visibility bits

	int           cmdNum; // the next cmdNum the server is expecting
	playerState_t ps; // complete information about the current player at this time

	int           serverCommandNum; // execute all commands up to this before
	// making the snapshot current

	std::vector<entityState_t> entities;
};

// Arnout: for double tapping
struct doubleTap_t
{
	int pressedTime[Util::ordinal(dtType_t::DT_NUM)];
	int releasedTime[Util::ordinal(dtType_t::DT_NUM)];

	int lastdoubleTap;
};

/*
=============================================================================

the clientActive_t structure is wiped completely at every
new gameState_t, potentially several times during an established connection

=============================================================================
*/

struct outPacket_t
{
	int p_cmdNumber; // cl.cmdNumber when packet was sent
	int p_serverTime; // usercmd->serverTime when packet was sent
	int p_realtime; // cls.realtime when packet was sent
};

#define MAX_PARSE_ENTITIES 2048

extern int g_console_field_width;

struct clientActive_t
{
	GameStateCSs gameState; // configstrings
	int timeoutcount; // it requres several frames in a timeout condition
	// to disconnect, preventing debugging breaks from
	// causing immediate disconnects on continue
	clSnapshot_t snap; // latest received from server

	int          serverTime; // may be paused during play
	int          oldServerTime; // to prevent time from flowing bakcwards
	int          oldFrameServerTime; // to check tournament restarts
	int          serverTimeDelta; // cl.serverTime = cls.realtime + cl.serverTimeDelta
	// this value changes as net lag varies
	bool     extrapolatedSnapshot; // set if any cgame frame has been forced to extrapolate
	// cleared when CL_AdjustTimeDelta looks at it
	bool     newSnapshots; // set on parse of any valid packet

	char         mapname[ MAX_QPATH ]; // extracted from CS_SERVERINFO

	int          mouseDx[ 2 ], mouseDy[ 2 ]; // added to by mouse events
	int          mouseIndex;
	int          joystickAxis[Util::ordinal(joystickAxis_t::MAX_JOYSTICK_AXIS)]; // set by joystick events

	// cgame communicates a few values to the client system
	int    cgameUserCmdValue; // current weapon to add to usercmd_t
	int    cgameFlags; // flags that can be set by the gamecode
	float  cgameSensitivity;

	// cmds[cmdNumber] is the predicted command, [cmdNumber-1] is the last
	// properly generated command
	usercmd_t cmds[ CMD_BACKUP ]; // each message will send several old cmds
	int       cmdNumber; // incremented each frame, because multiple
	// frames may need to be packed into a single packet

	// Arnout: double tapping
	doubleTap_t doubleTap;

	outPacket_t outPackets[ PACKET_BACKUP ]; // information about each packet we have sent out

	// the client maintains its own idea of view angles, which are
	// sent to the server each frame.  It is cleared to 0 upon entering each level.
	// the server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes
	vec3_t viewangles;

	int    serverId; // included in each client message so the server
	// can tell if it is for a prior map_restart
	// big stuff at end of structure so most offsets are 15 bits or less
	clSnapshot_t  snapshots[ PACKET_BACKUP ];

	entityState_t entityBaselines[ MAX_GENTITIES ]; // for delta compression when not in previous frame
};

extern clientActive_t cl;

/*
=============================================================================

the clientConnection_t structure is wiped when disconnecting from a server,
either to go to a full screen console, play a demo, or connect to a different server

A connection can be to either a server through the network layer or a
demo through a file.

=============================================================================
*/

struct clientConnection_t
{
	int      clientNum;
	int      lastPacketSentTime; // for retransmits during connection
	int      lastPacketTime; // for timeouts

	netadr_t serverAddress;
	int      connectTime; // for connection retransmits
	int      connectPacketCount; // for display on connection dialog
	char     serverMessage[ MAX_STRING_TOKENS ]; // for display on connection dialog

	std::string challenge; // from the server to use for connecting
	int      checksumFeed; // from the server for checksum calculations

	// these are our reliable messages that go to the server
	int  reliableSequence;
	int  reliableAcknowledge; // the last one the server has executed
	// TTimo - NOTE: incidentally, reliableCommands[0] is never used (always start at reliableAcknowledge+1)
	char reliableCommands[ MAX_RELIABLE_COMMANDS ][ MAX_TOKEN_CHARS ];

	// unreliable binary data to send to server
	int      binaryMessageLength;
	char     binaryMessage[ MAX_BINARY_MESSAGE ];
	bool binaryMessageOverflowed;

	// server message (unreliable) and command (reliable) sequence
	// numbers are NOT cleared at level changes, but continue to
	// increase as long as the connection is valid

	// message sequence is used by both the network layer and the
	// delta compression layer
	int serverMessageSequence;

	// reliable messages received from server
	int  serverCommandSequence;
	int  lastExecutedServerCommand; // last server command grabbed or executed with CL_GetServerCommand
	char serverCommands[ MAX_RELIABLE_COMMANDS ][ MAX_TOKEN_CHARS ];

	// file transfer from server
	fileHandle_t download;
	int          downloadNumber;
	int          downloadBlock; // block we are waiting for
	int          downloadCount; // how many bytes we got
	int          downloadSize; // how many bytes we got
	int          downloadFlags; // misc download behaviour flags sent by the server
	char         downloadList[ MAX_INFO_STRING ]; // list of paks we need to download

	// www downloading
	bool bWWWDl; // we have a www download going
	bool bWWWDlAborting; // disable the CL_WWWDownload until server gets us a gamestate (used for aborts)
	char     redirectedList[ MAX_INFO_STRING ]; // list of files that we downloaded through a redirect since last FS_ComparePaks
	char     badChecksumList[ MAX_INFO_STRING ]; // list of files for which wwwdl redirect is broken (wrong checksum)
	char     newsString[ MAX_NEWS_STRING ];

	// demo information
	char         demoName[ MAX_QPATH ];
	bool     demorecording;
	bool     demoplaying;
	bool     demowaiting; // don't record until a non-delta message is received
	bool     firstDemoFrameSkipped;
	fileHandle_t demofile;

	bool     waverecording;
	fileHandle_t wavefile;
	int          wavetime;

	int          timeDemoFrames; // counter of rendered frames
	int          timeDemoStart; // cls.realtime before first frame
	int          timeDemoBaseTime; // each frame will be at this time + frameNum * 50

	// big stuff at end of structure so most offsets are 15 bits or less
	netchan_t netchan;
};

extern clientConnection_t clc;

/*
==================================================================

the clientStatic_t structure is never wiped, and is used even when
no client connection is active at all

==================================================================
*/

struct ping_t
{
	netadr_t adr;
	int      start;
	int      time;
	char     info[ MAX_INFO_STRING ];
};

#define MAX_FEATLABEL_CHARS 32
struct serverInfo_t
{
	netadr_t adr;
	char     hostName[ MAX_NAME_LENGTH ];
	int      load;
	char     mapName[ MAX_NAME_LENGTH ];
	char     game[ MAX_NAME_LENGTH ];
	char     label[ MAX_FEATLABEL_CHARS ]; // for featured servers, nullptr otherwise
	netadrtype_t netType;
	int      clients;
	int      bots;
	int      maxClients;
	int      minPing;
	int      maxPing;
	int      ping;
	bool visible;
	int      friendlyFire; // NERVE - SMF
	int      needpass;
	char     gameName[ MAX_NAME_LENGTH ]; // Arnout
};

struct clientStatic_t
{
	connstate_t state; // connection status
	int         keyCatchers; // bit flags

	char        servername[ MAX_OSPATH ]; // name of server from original connect
	char        reconnectCmd[ MAX_STRING_CHARS ]; // command to be used on reconnection

	// when the server clears the hunk, all of these must be restarted
	bool rendererStarted;
	bool soundStarted;
	bool soundRegistered;
	bool uiStarted;
	bool cgameStarted;

	int      framecount;
	int      frametime; // msec since last frame

	int      realtime; // ignores pause
	int      realFrametime; // ignoring pause, so console always works

	// master server sequence information
	int          numMasterPackets;
	unsigned int receivedMasterPackets; // bitfield

	int          numlocalservers;
	serverInfo_t localServers[ MAX_OTHER_SERVERS ];

	int          numglobalservers;
	serverInfo_t globalServers[ MAX_GLOBAL_SERVERS ];
	// additional global servers
	int          numGlobalServerAddresses;
	netadr_t     globalServerAddresses[ MAX_GLOBAL_SERVERS ];

	unsigned     numserverLinks;
	netadr_t     serverLinks[ MAX_GLOBAL_SERVERS ];

	int          numfavoriteservers;
	serverInfo_t favoriteServers[ MAX_OTHER_SERVERS ];

	int          pingUpdateSource; // source currently pinging or updating

	int          masterNum;

	// update server info
	netadr_t updateServer;
	char     updateChallenge[ MAX_TOKEN_CHARS ];
	char     updateInfoString[ MAX_INFO_STRING ];

	// rendering info
	glconfig_t  glconfig;
	glconfig2_t glconfig2;
	qhandle_t   charSetShader;
	qhandle_t   whiteShader;
	qhandle_t   consoleShader;
	qhandle_t   consoleShader2; // NERVE - SMF - merged from WolfSP
	bool    useLegacyConsoleFont;
	bool    useLegacyConsoleFace;
	fontInfo_t *consoleFont;

	// www downloading
	// in the static stuff since this may have to survive server disconnects
	// if new stuff gets added, CL_ClearStaticDownload code needs to be updated for clear up
	bool bWWWDlDisconnected; // keep going with the download after server disconnect
	char     downloadName[ MAX_OSPATH ];
	char     downloadTempName[ MAX_OSPATH ]; // in wwwdl mode, this is OS path (it's a qpath otherwise)
	char     originalDownloadName[ MAX_QPATH ]; // if we get a redirect, keep a copy of the original file path
	bool downloadRestart; // if true, we need to do another FS_Restart because we downloaded a pak
};

extern clientStatic_t cls;

//=============================================================================

class CGameVM: public VM::VMBase {
public:
	CGameVM();
	void Start();

	void CGameStaticInit();
	void CGameInit(int serverMessageNum, int clientNum);
	void CGameShutdown();
	void CGameDrawActiveFrame(int serverTime, bool demoPlayback);
	int CGameCrosshairPlayer();
	void CGameKeyEvent(int key, bool down);
	void CGameMouseEvent(int dx, int dy);
    void CGameMousePosEvent(int x, int y);
	void CGameTextInputEvent(int c);
	void CGameFocusEvent(bool focus);
	//std::vector<std::string> CGameVoipString();
	//void CGameInitCvars();

	void CGameRocketInit();
	void CGameRocketFrame();
	void CGameConsoleLine(const std::string& str);

private:
	virtual void Syscall(uint32_t id, Util::Reader reader, IPC::Channel& channel) OVERRIDE FINAL;
	void QVMSyscall(int index, Util::Reader& reader, IPC::Channel& channel);

	std::unique_ptr<VM::CommonVMServices> services;

    class CmdBuffer: public IPC::CommandBufferHost {
        public:
            CmdBuffer(std::string name);
            virtual void HandleCommandBufferSyscall(int major, int minor, Util::Reader& reader) OVERRIDE FINAL;
    };

    CmdBuffer cmdBuffer;
};

extern CGameVM                cgvm;

extern refexport_t            re; // interface to refresh library

extern struct rsa_public_key  public_key;

extern struct rsa_private_key private_key;

//
// cvars
//
extern cvar_t *cl_nodelta;
extern cvar_t *cl_noprint;
extern cvar_t *cl_maxpackets;
extern cvar_t *cl_packetdup;
extern cvar_t *cl_shownet;
extern cvar_t *cl_shownuments; // DHM - Nerve
extern cvar_t *cl_showSend;
extern cvar_t *cl_showServerCommands; // NERVE - SMF
extern cvar_t *cl_timeNudge;
extern cvar_t *cl_showTimeDelta;
extern cvar_t *cl_freezeDemo;

extern cvar_t *cl_yawspeed;
extern cvar_t *cl_pitchspeed;
extern cvar_t *cl_run;
extern cvar_t *cl_anglespeedkey;

extern cvar_t *cl_doubletapdelay;

extern cvar_t *cl_sensitivity;
extern cvar_t *cl_freelook;

extern cvar_t *cl_xbox360ControllerAvailable;

extern cvar_t *cl_mouseAccel;
extern cvar_t *cl_mouseAccelOffset;
extern cvar_t *cl_mouseAccelStyle;
extern cvar_t *cl_showMouseRate;

extern cvar_t *m_pitch;
extern cvar_t *m_yaw;
extern cvar_t *m_forward;
extern cvar_t *m_side;
extern cvar_t *m_filter;

extern cvar_t *j_pitch;
extern cvar_t *j_yaw;
extern cvar_t *j_forward;
extern cvar_t *j_side;
extern cvar_t *j_up;
extern cvar_t *j_pitch_axis;
extern cvar_t *j_yaw_axis;
extern cvar_t *j_forward_axis;
extern cvar_t *j_side_axis;
extern cvar_t *j_up_axis;

extern cvar_t *cl_IRC_connect_at_startup;
extern cvar_t *cl_IRC_server;
extern cvar_t *cl_IRC_channel;
extern cvar_t *cl_IRC_port;
extern cvar_t *cl_IRC_override_nickname;
extern cvar_t *cl_IRC_nickname;
extern cvar_t *cl_IRC_kick_rejoin;
extern cvar_t *cl_IRC_reconnect_delay;

extern cvar_t *cl_timedemo;

extern cvar_t *cl_activeAction;
extern cvar_t *cl_autorecord;

extern cvar_t *cl_allowDownload;
extern cvar_t *cl_conXOffset;
extern cvar_t *cl_inGameVideo;

extern cvar_t *cl_altTab;

// -NERVE - SMF

extern cvar_t *cl_consoleKeys;
extern cvar_t *cl_consoleFont;
extern cvar_t *cl_consoleFontSize;
extern cvar_t *cl_consoleFontKerning;
extern cvar_t *cl_consoleCommand;

extern cvar_t *cl_logs;

extern cvar_t *cl_cgameSyscallStats;

/**
 * Independently of the gamelogic, we can assume the game to have "teams" with an id,
 * as long as we don't assume any semantics on that
 * we can assume however that "0" is some form of "neutral" or "non" team,
 * most likly a non-playing client that e.g. observes the game or hasn't joined yet.
 * even in a deathmatch or singleplayer game, joining would start with team 1, even though there might not be another one
 * this allows several client logic (like team specific binds or configurations) to work no matter how the team is called or what its attributes are
 */
extern cvar_t *p_team;

extern cvar_t *con_scrollLock;

// XreaL BEGIN
extern cvar_t *cl_aviFrameRate;
extern cvar_t *cl_aviMotionJpeg;
// XreaL END

extern cvar_t *cl_allowPaste;

extern cvar_t *cl_useMumble;
extern cvar_t *cl_mumbleScale;

extern Log::Logger downloadLogger;

//=================================================

//
// cl_main
//

void        CL_Init();
void        CL_FlushMemory();
void        CL_ShutdownAll();
void        CL_AddReliableCommand( const char *cmd );

void        CL_RegisterButtonCommands( const char *cmdList );

void        CL_StartHunkUsers();

void        CL_Disconnect_f();
void        CL_GetChallengePacket();
void        CL_Vid_Restart_f();
void        CL_Snd_Restart_f();

void        CL_NextDemo();
void        CL_ReadDemoMessage();
void        CL_StartDemoLoop();

void        CL_InitDownloads();
void        CL_NextDownload();

void        CL_GetPing( int n, char *buf, int buflen, int *pingtime );
void        CL_ClearPing( int n );
int         CL_GetPingQueueCount();

void        CL_ShutdownRef();
bool    CL_InitRef();

int         CL_ServerStatus( const char *serverAddress, char *serverStatusString, int maxLen );

void CL_Record( const char *name );

//
// cl_keys (for input usage)
//
int             Key_GetKeyNumber();
unsigned int    Key_GetKeyTime();

//
// cl_input
//
struct kbutton_t
{
	int      down[ 2 ]; // key nums holding it down
	unsigned downtime; // msec timestamp
	unsigned msec; // msec down this frame if both a down and up happened
	bool active; // current state
	bool wasPressed; // set when down, not cleared when up
};

enum kbuttons_t
{
  KB_LEFT = 0,
  KB_RIGHT,
  KB_FORWARD,
  KB_BACK,
  KB_LOOKUP,
  KB_LOOKDOWN,
  KB_MOVELEFT,
  KB_MOVERIGHT,
  KB_STRAFE,
  KB_SPEED,
  KB_UP,
  KB_DOWN,

  KB_MLOOK,
  KB_VOIPRECORD,

  KB_BUTTONS, // must be second-last
  NUM_BUTTONS = KB_BUTTONS + USERCMD_BUTTONS
};

void CL_ClearKeys();
void CL_ClearCmdButtons();

void CL_InitInput();
void CL_SendCmd();
void CL_ClearState();
void CL_ReadPackets();

void CL_WritePacket();

void IN_Notebook();
void IN_Help();

void IN_PrepareKeyUp();

//----(SA)

float    CL_KeyState( kbutton_t *key );
int      Key_StringToKeynum( const char *str );
const char *Key_KeynumToString( int keynum );

//cl_irc.c
void     CL_IRCSetup();
void     CL_InitIRC();
void     CL_IRCInitiateShutdown();
void     CL_IRCWaitShutdown();
void     CL_IRCSay();
bool CL_IRCIsConnected();
bool CL_IRCIsRunning();

//
// cl_parse.c
//
void CL_SystemInfoChanged();
void CL_ParseServerMessage( msg_t *msg );

//====================================================================

void     CL_ServerInfoPacket( netadr_t from, msg_t *msg );
void     CL_LocalServers_f();
void     CL_GlobalServers_f();
void     CL_FavoriteServers_f();
void     CL_Ping_f();
bool CL_UpdateVisiblePings_f( int source );

//
// console
//
#define     CONSOLE_FONT_VPADDING 0.3

struct consoleBoxWidth_t
{
	int top;
	int bottom;
	/* the sides of the console always get treated equally */
	int sides;
};

struct console_t
{
	bool initialized;

	std::vector<std::string> lines;

	int      lastReadLineIndex; // keep track fo the last read line, so we can show the user, what was added since he last opened the console
	int      scrollLineIndex; // bottom of console is supposed displays this line
	float    bottomDisplayedLine; // bottom of console displays this line, is trying to move towards:

	int      textWidthInChars; // characters across screen

	/**
	 * the amount of lines that fit onto the screen
	 */
	int      visibleAmountOfLines;

	/**
	 * the distances from the consoleborder to the screen in pixel
	 */
	consoleBoxWidth_t margin;

	/**
	 * the (optionally colored) gap between margin and padding
	 */
	consoleBoxWidth_t border;

	/**
	 * the distances from the consoletext to the border in pixel
	 */
	consoleBoxWidth_t padding;

	/**
	 * current console-content height in pixel from border to border (paddings are part of the content)
	 */
	int height;

	float    currentAnimationFraction; // changes between 0.0 and 1.0 at scr_conspeed
	bool isOpened;

	/**
	 * changes between 0.0 and 1.0 correlated with currentAnimationFraction
	 * if the animation of type fade is active
	 * it gets multiplied with the alpha of practically anything that gets rendered to allow fading in and out the
	 * console as a whole
	 */
	float    currentAlphaFactor;
};

extern console_t consoleState;

void             Con_DrawCharacter( int cx, int line, int num );

bool         Con_CheckResize();
void             Con_Init();
void             Con_Clear_f();
void             Con_ToggleConsole_f();
void             Con_OpenConsole_f();
void             Con_DrawRightFloatingTextLine( const int linePosition, const Color::Color& color, const char* text );
void             Con_DrawConsole();
void             Con_RunConsole();
void             Con_PageUp();
void             Con_PageDown();
void             Con_JumpUp();
void             Con_ScrollUp( int lines );
void             Con_ScrollDown( int lines );
void             Con_ScrollToMarkerLine();
void             Con_ScrollToTop();
void             Con_ScrollToBottom();
void             Con_Close();

void             CL_LoadConsoleHistory();
void             CL_SaveConsoleHistory();

//
// cl_scrn.c
//
void  SCR_Init();
void  SCR_UpdateScreen();

void  SCR_AdjustFrom640( float *x, float *y, float *w, float *h );
void  SCR_FillRect( float x, float y, float width, float height, const Color::Color& color );

void  SCR_DrawSmallStringExt( int x, int y, const char *string, const Color::Color& setColor, bool forceColor, bool noColorEscape );
void  SCR_DrawSmallUnichar( int x, int y, int ch );
void  SCR_DrawConsoleFontUnichar( float x, float y, int ch );
float SCR_ConsoleFontCharWidth( const char *s );
float SCR_ConsoleFontUnicharWidth( int ch );
float SCR_ConsoleFontCharHeight();
float SCR_ConsoleFontCharVPadding();
float SCR_ConsoleFontStringWidth( const char *s, int len );

//
// cl_cin.c
//

void     CL_PlayCinematic_f();
void     SCR_DrawCinematic();
void     SCR_RunCinematic();
void     SCR_StopCinematic();
int      CIN_PlayCinematic( const char *arg0, int xpos, int ypos, int width, int height, int bits );
e_status CIN_RunCinematic( int handle );
void     CIN_DrawCinematic( int handle );
void     CIN_UploadCinematic( int handle );
void     CIN_CloseAllVideos();

// yuv->rgb will be used for Theora(ogm)
void     ROQ_GenYUVTables();
void     Frame_yuv_to_rgb24( const unsigned char *y, const unsigned char *u, const unsigned char *v,
                             int width, int height, int y_stride, int uv_stride,
                             int yWShift, int uvWShift, int yHShift, int uvHShift, unsigned int *output );

//
// cin_ogm.c
//

int           Cin_OGM_Init( const char *filename );
int           Cin_OGM_Run( int time );
unsigned char *Cin_OGM_GetOutput( int *outWidth, int *outHeight );
void          Cin_OGM_Shutdown();

//
// cl_cgame.c
//
void     CL_InitCGame();
void     CL_ShutdownCGame();
void     CL_GameCommandHandler();
bool CL_GameConsoleText();
void     CL_CGameRendering();
void     CL_SetCGameTime();
void     CL_FirstSnapshot();
void     CL_ShaderStateChanged();
void     CL_CGameBinaryMessageReceived( const char *buf, int buflen, int serverTime );
void     CL_OnTeamChanged( int newTeam );

//
// cl_ui.c
//
void CL_ShutdownUI();
int  Key_GetCatcher();
void Key_SetCatcher( int catcher );
void UI_GameCommandHandler();

//
// cl_net_chan.c
//
void     CL_Netchan_Transmit( netchan_t *chan, msg_t *msg );  //int length, const byte *data );
void     CL_Netchan_TransmitNextFragment( netchan_t *chan );
bool CL_Netchan_Process( netchan_t *chan, msg_t *msg );

// XreaL BEGIN

//
// cl_avi.c
//
bool CL_OpenAVIForWriting( const char *filename );
void     CL_TakeVideoFrame();
void     CL_WriteAVIVideoFrame( const byte *imageBuffer, int size );
void     CL_WriteAVIAudioFrame( const byte *pcmBuffer, int size );
bool CL_CloseAVI();
bool CL_VideoRecording();

// XreaL END

//
// cl_main.c
//
void CL_WriteDemoMessage( msg_t *msg, int headerBytes );
void CL_RequestMotd();
void CL_GetClipboardData( char *, int );

//
// cl_input.c
//
MouseMode IN_GetMouseMode();
void IN_SetMouseMode(MouseMode mode);

#endif
