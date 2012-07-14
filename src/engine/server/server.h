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
// server.h

#include "../qcommon/q_shared.h"
#include "../qcommon/qcommon.h"
#include "../server/g_api.h"

//=============================================================================

#define PERS_SCORE        0 // !!! MUST NOT CHANGE, SERVER AND
// GAME BOTH REFERENCE !!!

#define MAX_ENT_CLUSTERS  16

#ifdef USE_VOIP
#define VOIP_QUEUE_LENGTH 64

typedef struct voipServerPacket_s
{
	int  generation;
	int  sequence;
	int  frames;
	int  len;
	int  sender;
	int  flags;
	byte data[ 1024 ];
} voipServerPacket_t;
#endif

#define MAX_BPS_WINDOW 20 // NERVE - SMF - net debugging

typedef struct svEntity_s
{
	struct worldSector_s *worldSector;

	struct svEntity_s    *nextEntityInWorldSector;

	entityState_t        baseline; // for delta compression of initial sighting
	int                  numClusters; // if -1, use headnode instead
	int                  clusternums[ MAX_ENT_CLUSTERS ];
	int                  lastCluster; // if all the clusters don't fit in clusternums
	int                  areanum, areanum2;
	int                  snapshotCounter; // used to prevent double adding from portal views
	int                  originCluster; // Gordon: calced upon linking, for origin only bmodel vis checks
} svEntity_t;

typedef enum
{
  SS_DEAD, // no map loaded
  SS_LOADING, // spawning level entities
  SS_GAME // actively running
} serverState_t;

typedef struct configString_s
{
	char         *s;

	qboolean     restricted; // if true, don't send to clientList
	clientList_t clientList;
} configString_t;

typedef struct
{
	serverState_t state;
	qboolean      restarting; // if true, send configstring changes during SS_LOADING
	int           serverId; // changes each server start
	int           restartedServerId; // serverId before a map_restart
	int           checksumFeed; // the feed key that we use to compute the pure checksum strings
	// show_bug.cgi?id=475
	// the serverId associated with the current checksumFeed (always <= serverId)
	int             checksumFeedServerId;
	int             snapshotCounter; // incremented for each snapshot built
	int             timeResidual; // <= 1000 / sv_frame->value
	int             nextFrameTime; // when time > nextFrameTime, process world
	struct cmodel_s *models[ MAX_MODELS ];

	char            *configstrings[ MAX_CONFIGSTRINGS ];
	qboolean        configstringsmodified[ MAX_CONFIGSTRINGS ];
	svEntity_t      svEntities[ MAX_GENTITIES ];

	char            *entityParsePoint; // used during game VM init

	// the game virtual machine will update these on init and changes
	sharedEntity_t *gentities;
	int            gentitySize;
	int            num_entities; // current number, <= MAX_GENTITIES

	playerState_t  *gameClients;
	int            gameClientSize; // will be > sizeof(playerState_t) due to game private data

	int            restartTime;

	// NERVE - SMF - net debugging
	int   bpsWindow[ MAX_BPS_WINDOW ];
	int   bpsWindowSteps;
	int   bpsTotalBytes;
	int   bpsMaxBytes;

	int   ubpsWindow[ MAX_BPS_WINDOW ];
	int   ubpsTotalBytes;
	int   ubpsMaxBytes;

	float ucompAve;
	int   ucompNum;
	// -NERVE - SMF

	md3Tag_t       tags[ MAX_SERVER_TAGS ];
	tagHeaderExt_t tagHeadersExt[ MAX_TAG_FILES ];

	int            num_tagheaders;
	int            num_tags;
} server_t;

typedef struct
{
	int           areabytes;
	byte          areabits[ MAX_MAP_AREA_BYTES ]; // portalarea visibility bits
	playerState_t ps;
	int           num_entities;
	int           first_entity; // into the circular sv_packet_entities[]
	// the entities MUST be in increasing state number
	// order, otherwise the delta compression will fail
	int messageSent; // time the message was transmitted
	int messageAcked; // time the message was acked
	int messageSize; // used to rate drop packets
} clientSnapshot_t;

typedef enum
{
  CS_FREE, // can be reused for a new connection
  CS_ZOMBIE, // client has been disconnected, but don't reuse connection for a couple seconds
  CS_CONNECTED, // has been assigned to a client_t, but no gamestate yet
  CS_PRIMED, // gamestate has been sent, but client hasn't sent a usercmd
  CS_ACTIVE // client is fully in game
} clientState_t;

typedef struct netchan_buffer_s
{
	msg_t                   msg;
	byte                    msgBuffer[ MAX_MSGLEN ];
	char                    lastClientCommandString[ MAX_STRING_CHARS ];
	struct netchan_buffer_s *next;
} netchan_buffer_t;

typedef struct client_s
{
	clientState_t  state;
	char           userinfo[ MAX_INFO_STRING ]; // name, etc

	char           reliableCommands[ MAX_RELIABLE_COMMANDS ][ MAX_STRING_CHARS ];
	int            reliableSequence; // last added reliable message, not necessarily sent or acknowledged yet
	int            reliableAcknowledge; // last acknowledged reliable message
	int            reliableSent; // last sent reliable message, not necessarily acknowledged yet
	int            messageAcknowledge;

	int            binaryMessageLength;
	char           binaryMessage[ MAX_BINARY_MESSAGE ];
	qboolean       binaryMessageOverflowed;

	int            gamestateMessageNum; // netchan->outgoingSequence of gamestate
	int            challenge;

	usercmd_t      lastUsercmd;
	int            lastMessageNum; // for delta compression
	int            lastClientCommand; // reliable client message sequence
	char           lastClientCommandString[ MAX_STRING_CHARS ];
	sharedEntity_t *gentity; // SV_GentityNum(clientnum)
	char           name[ MAX_NAME_LENGTH ]; // extracted from userinfo, high bits masked

	// downloading
	char          downloadName[ MAX_QPATH ]; // if not empty string, we are downloading
	fileHandle_t  download; // file being downloaded
	int           downloadSize; // total bytes (can't use EOF because of paks)
	int           downloadCount; // bytes sent
	int           downloadClientBlock; // last block we sent to the client, awaiting ack
	int           downloadCurrentBlock; // current block number
	int           downloadXmitBlock; // last block we xmited
	unsigned char *downloadBlocks[ MAX_DOWNLOAD_WINDOW ]; // the buffers for the download blocks
	int           downloadBlockSize[ MAX_DOWNLOAD_WINDOW ];
	qboolean      downloadEOF; // We have sent the EOF block
	int           downloadSendTime; // time we last got an ack from the client

	// www downloading
	qboolean bDlOK; // passed from cl_wwwDownload CVAR_USERINFO, whether this client supports www dl
	char     downloadURL[ MAX_OSPATH ]; // the URL we redirected the client to
	qboolean bWWWDl; // we have a www download going
	qboolean bWWWing; // the client is doing an ftp/http download
	qboolean bFallback; // last www download attempt failed, fallback to regular download
	// note: this is one-shot, multiple downloads would cause a www download to be attempted again

	int              deltaMessage; // frame last client usercmd message
	int              nextReliableTime; // svs.time when another reliable command will be allowed
	int              lastPacketTime; // svs.time when packet was last received
	int              lastConnectTime; // svs.time when connection started
	int              nextSnapshotTime; // send another snapshot when svs.time >= nextSnapshotTime
	qboolean         rateDelayed; // true if nextSnapshotTime was set based on rate instead of snapshotMsec
	int              timeoutCount; // must timeout a few frames in a row so debugging doesn't break
	clientSnapshot_t frames[ PACKET_BACKUP ]; // updates can be delta'd from here
	int              ping;
	int              rate; // bytes / second
	int              snapshotMsec; // requests a snapshot every snapshotMsec unless rate choked
	int              pureAuthentic;
	qboolean         gotCP; // TTimo - additional flag to distinguish between a bad pure checksum, and no cp command at all
	netchan_t        netchan;
	// TTimo
	// queuing outgoing fragmented messages to send them properly, without udp packet bursts
	// in case large fragmented messages are stacking up
	// buffer them into this queue, and hand them out to netchan as needed
	netchan_buffer_t *netchan_start_queue;
	//% netchan_buffer_t **netchan_end_queue;
	netchan_buffer_t *netchan_end_queue;

#ifdef USE_VOIP
	qboolean           hasVoip;
	qboolean           muteAllVoip;
	qboolean           ignoreVoipFromClient[ MAX_CLIENTS ];
	voipServerPacket_t *voipPacket[ VOIP_QUEUE_LENGTH ];
	int                queuedVoipPackets;
	int                queuedVoipIndex;
#endif

	//bani
	int downloadnotify;
} client_t;

//=============================================================================

#define STATFRAMES 100
typedef struct
{
	double active;
	double idle;
	int    count;
	int    packets;

	double latched_active;
	double latched_idle;
	int    latched_packets;
} svstats_t;

// MAX_CHALLENGES is made large to prevent a denial
// of service attack that could cycle all of them
// out before legitimate users connected
#define MAX_CHALLENGES    1024

#define AUTHORIZE_TIMEOUT 5000

typedef struct
{
	netadr_t adr;
	int      challenge;
	int      time; // time the last packet was sent to the autherize server
	int      pingTime; // time the challenge response was sent to client
	int      firstTime; // time the adr was first used, for authorize timeout checks
	int      firstPing; // Used for min and max ping checks
	qboolean connected;
} challenge_t;

typedef struct
{
	netadr_t adr;
	int      time;
} receipt_t;

// MAX_INFO_RECEIPTS is the maximum number of getstatus+getinfo responses that we send
// in a two second time period.
#define MAX_INFO_RECEIPTS 48

typedef struct tempBan_s
{
	netadr_t adr;
	int      endtime;
} tempBan_t;

#define MAX_MASTERS                       8 // max recipients for heartbeat packets
#define MAX_TEMPBAN_ADDRESSES             MAX_CLIENTS

#define SERVER_PERFORMANCECOUNTER_FRAMES  600
#define SERVER_PERFORMANCECOUNTER_SAMPLES 6

// this structure will be cleared only when the game dll changes
typedef struct
{
	qboolean      initialized; // sv_init has completed

	int           time; // will be strictly increasing across level changes

	int           snapFlagServerBit; // ^= SNAPFLAG_SERVERCOUNT every SV_SpawnServer()

	client_t      *clients; // [sv_maxclients->integer];
	int           numSnapshotEntities; // sv_maxclients->integer*PACKET_BACKUP*MAX_PACKET_ENTITIES
	int           nextSnapshotEntities; // next snapshotEntities to use
	entityState_t *snapshotEntities; // [numSnapshotEntities]
	int           nextHeartbeatTime;
	challenge_t   challenges[ MAX_CHALLENGES ]; // to prevent invalid IPs from connecting
	receipt_t     infoReceipts[ MAX_INFO_RECEIPTS ];
	netadr_t      redirectAddress; // for rcon return messages
	tempBan_t     tempBanAddresses[ MAX_TEMPBAN_ADDRESSES ];

#ifdef AUTHORIZE_SUPPORT
	netadr_t authorizeAddress;
#endif // AUTHORIZE_SUPPORT

	int       sampleTimes[ SERVER_PERFORMANCECOUNTER_SAMPLES ];
	int       currentSampleIndex;
	int       totalFrameTime;
	int       currentFrameIndex;
	int       serverLoad;
#ifdef USE_HUB_SERVER
	netadr_t  owHubAddress; // address of hub we're using
#endif
	svstats_t stats;
} serverStatic_t;

//=============================================================================

extern serverStatic_t svs; // persistent server info across maps
extern server_t       sv; // cleared each map
extern vm_t           *gvm; // game virtual machine

extern cvar_t         *sv_fps;
extern cvar_t         *sv_timeout;
extern cvar_t         *sv_zombietime;
extern cvar_t         *sv_rconPassword;
extern cvar_t         *sv_privatePassword;
extern cvar_t         *sv_allowDownload;
extern cvar_t         *sv_friendlyFire; // NERVE - SMF
extern cvar_t         *sv_maxlives; // NERVE - SMF
extern cvar_t         *sv_maxclients;
extern cvar_t         *sv_needpass;

extern cvar_t         *sv_privateClients;
extern cvar_t         *sv_hostname;
extern cvar_t         *sv_master[ MAX_MASTER_SERVERS ];
extern cvar_t         *sv_reconnectlimit;
extern cvar_t         *sv_tempbanmessage;
extern cvar_t         *sv_showloss;
extern cvar_t         *sv_padPackets;
extern cvar_t         *sv_killserver;
extern cvar_t         *sv_mapname;
extern cvar_t         *sv_mapChecksum;
extern cvar_t         *sv_serverid;
extern cvar_t         *sv_maxRate;
extern cvar_t         *sv_minPing;
extern cvar_t         *sv_maxPing;

//extern    cvar_t  *sv_gametype;

extern cvar_t *sv_newGameShlib;

extern cvar_t *sv_pure;
extern cvar_t *sv_floodProtect;
extern cvar_t *sv_allowAnonymous;
extern cvar_t *sv_lanForceRate;
extern cvar_t *sv_onlyVisibleClients;

extern cvar_t *sv_showAverageBPS; // NERVE - SMF - net debugging

extern cvar_t *sv_requireValidGuid;

#ifdef USE_HUB_SERVER
extern cvar_t *sv_owHubHost;
extern cvar_t *sv_owHubKey;
#endif

extern cvar_t *sv_ircchannel;

extern cvar_t *g_gameType;

// Rafael gameskill
//extern    cvar_t  *sv_gameskill;
// done

extern cvar_t *sv_reloading;

// TTimo - autodl
extern cvar_t *sv_dl_maxRate;

// TTimo
extern cvar_t *sv_wwwDownload; // general flag to enable/disable www download redirects
extern cvar_t *sv_wwwBaseURL; // the base URL of all the files

// tell clients to perform their downloads while disconnected from the server
// this gets you a better throughput, but you lose the ability to control the download usage
extern cvar_t *sv_wwwDlDisconnected;
extern cvar_t *sv_wwwFallbackURL;

//bani
extern cvar_t *sv_cheats;
extern cvar_t *sv_packetloss;
extern cvar_t *sv_packetdelay;

//fretn
extern cvar_t *sv_fullmsg;

#ifdef USE_VOIP
extern cvar_t *sv_voip;
#endif

//===========================================================

//
// sv_main.c
//
void       SV_FinalCommand( char *cmd, qboolean disconnect );  // ydnar: added disconnect flag so map changes can use this function as well
void QDECL SV_SendServerCommand( client_t *cl, const char *fmt, ... ) PRINTF_LIKE(2);
void       SV_PrintTranslatedText( const char *text, qboolean broadcast );

void       SV_AddOperatorCommands( void );
void       SV_RemoveOperatorCommands( void );

void       SV_MasterHeartbeat( const char *hbname );
void       SV_MasterShutdown( void );
void       SV_MasterGameStat( const char *data );

void       SV_MasterGameCompleteStatus(); // NERVE - SMF

//bani - bugtraq 12534
qboolean   SV_VerifyChallenge( char *challenge );

//
// sv_init.c
//
void SV_SetConfigstringNoUpdate( int index, const char *val );
void SV_UpdateConfigStrings( void );
void SV_SetConfigstring( int index, const char *val );
void SV_UpdateConfigStrings( void );
void SV_GetConfigstring( int index, char *buffer, int bufferSize );
void SV_SetConfigstringRestrictions( int index, const clientList_t *clientList );

void SV_SetUserinfo( int index, const char *val );
void SV_GetUserinfo( int index, char *buffer, int bufferSize );

void SV_CreateBaseline( void );

void SV_ChangeMaxClients( void );
void SV_SpawnServer( char *server, qboolean killBots );

//
// sv_client.c
//
void SV_GetChallenge( netadr_t from );

void SV_DirectConnect( netadr_t from );

void SV_AuthorizeIpPacket( netadr_t from );

void SV_ExecuteClientMessage( client_t *cl, msg_t *msg );
void SV_UserinfoChanged( client_t *cl );

void SV_ClientEnterWorld( client_t *client, usercmd_t *cmd );
void SV_FreeClient( client_t *client );
void SV_DropClient( client_t *drop, const char *reason );

void SV_ExecuteClientCommand( client_t *cl, const char *s, qboolean clientOK, qboolean premaprestart );
void SV_ClientThink( client_t *cl, usercmd_t *cmd );

void SV_WriteDownloadToClient( client_t *cl, msg_t *msg );

#ifdef USE_VOIP
void SV_WriteVoipToClient( client_t *cl, msg_t *msg );

#endif

//
// sv_ccmds.c
//
void     SV_Heartbeat_f( void );

qboolean SV_TempBanIsBanned( netadr_t address );
void     SV_TempBanNetAddress( netadr_t address, int length );

//
// sv_snapshot.c
//
void SV_AddServerCommand( client_t *client, const char *cmd );
void SV_UpdateServerCommandsToClient( client_t *client, msg_t *msg );
void SV_WriteFrameToClient( client_t *client, msg_t *msg );
void SV_SendMessageToClient( msg_t *msg, client_t *client );
void SV_SendClientMessages( void );
void SV_SendClientSnapshot( client_t *client );

//bani
void SV_SendClientIdle( client_t *client );

//
// sv_game.c
//
int SV_NumForGentity( sharedEntity_t *ent );

//#define SV_GentityNum( num ) ((sharedEntity_t *)((byte *)sv.gentities + sv.gentitySize*(num)))
//#define SV_GameClientNum( num ) ((playerState_t *)((byte *)sv.gameClients + sv.gameClientSize*(num)))

sharedEntity_t *SV_GentityNum( int num );
playerState_t  *SV_GameClientNum( int num );

svEntity_t     *SV_SvEntityForGentity( sharedEntity_t *gEnt );
sharedEntity_t *SV_GEntityForSvEntity( svEntity_t *svEnt );
void           SV_InitGameProgs( void );
void           SV_ShutdownGameProgs( void );
void           SV_RestartGameProgs( void );
qboolean       SV_inPVS( const vec3_t p1, const vec3_t p2 );
qboolean       SV_GetTag( int clientNum, int tagFileNumber, const char *tagname, orientation_t *ort );
int            SV_LoadTag( const char *mod_name );
qboolean       SV_GameIsSinglePlayer( void );
qboolean       SV_GameIsCoop( void );
void           SV_GameBinaryMessageReceived( int cno, const char *buf, int buflen, int commandTime );

//
// sv_bot.c
//
void SV_BotFrame( int time );
int  SV_BotAllocateClient( int clientNum );
void SV_BotFreeClient( int clientNum );

void SV_BotInitCvars( void );
int  SV_BotLibSetup( void );
int  SV_BotLibShutdown( void );
int  SV_BotGetSnapshotEntity( int client, int ent );
int  SV_BotGetConsoleMessage( int client, char *buf, int size );

int  BotImport_DebugPolygonCreate( int color, int numPoints, vec3_t *points );
void BotImport_DebugPolygonDelete( int id );

//============================================================
//
// high level object sorting to reduce interaction tests
//

void SV_ClearWorld( void );

// called after the world model has been loaded, before linking any entities

void SV_UnlinkEntity( sharedEntity_t *ent );

// call before removing an entity, and before trying to move one,
// so it doesn't clip against itself

void SV_LinkEntity( sharedEntity_t *ent );

// Needs to be called any time an entity changes origin, mins, maxs,
// or solid.  Automatically unlinks if needed.
// sets ent->r.absmin and ent->r.absmax
// sets ent->leafnums[] for pvs determination even if the entity
// is not solid

clipHandle_t SV_ClipHandleForEntity( const sharedEntity_t *ent );

void         SV_SectorList_f( void );

int          SV_AreaEntities( const vec3_t mins, const vec3_t maxs, int *entityList, int maxcount );

// fills in a table of entity numbers with entities that have bounding boxes
// that intersect the given area.  It is possible for a non-axial bmodel
// to be returned that doesn't actually intersect the area on an exact
// test.
// returns the number of pointers filled in
// The world entity is never returned in this list.

int SV_PointContents( const vec3_t p, int passEntityNum );

// returns the CONTENTS_* value from the world and all entities at the given point.

void SV_Trace( trace_t *results, const vec3_t start, vec3_t mins, vec3_t maxs, const vec3_t end, int passEntityNum,
               int contentmask, traceType_t type );

// mins and maxs are relative

// if the entire move stays in a solid volume, trace.allsolid will be set,
// trace.startsolid will be set, and trace.fraction will be 0

// if the starting point is in a solid, it will be allowed to move out
// to an open area

// passEntityNum, if isn't ENTITYNUM_NONE, will be explicitly excluded from clipping checks

void SV_ClipToEntity( trace_t *trace, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int entityNum, int contentmask, traceType_t type );

// clip to a specific entity

//
// sv_net_chan.c
//
void     SV_Netchan_Transmit( client_t *client, msg_t *msg );
void     SV_Netchan_TransmitNextFragment( client_t *client );
qboolean SV_Netchan_Process( client_t *client, msg_t *msg );
void     SV_Netchan_FreeQueue( client_t *client );

//bani - cl->downloadnotify
#define DLNOTIFY_REDIRECT 0x00000001 // "Redirecting client ..."
#define DLNOTIFY_BEGIN    0x00000002 // "clientDownload: 4 : beginning ..."
#define DLNOTIFY_ALL      ( DLNOTIFY_REDIRECT | DLNOTIFY_BEGIN )
