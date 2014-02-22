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

// sv_game.c -- interface to the game module

#include "server.h"
#include "../../common/Cvar.h"
#include "../qcommon/crypto.h"
#include "../framework/CommonVMServices.h"

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int SV_NumForGentity( sharedEntity_t *ent )
{
	int num;

	num = ( ( byte * ) ent - ( byte * ) sv.gentities ) / sv.gentitySize;

	return num;
}

sharedEntity_t *SV_GentityNum( int num )
{
	sharedEntity_t *ent;

	if ( num < 0 || num >= MAX_GENTITIES )
	{
		Com_Error( ERR_DROP, "SV_GentityNum: bad num %d", num );
	}

	ent = ( sharedEntity_t * )( ( byte * ) sv.gentities + sv.gentitySize * ( num ) );

	return ent;
}

playerState_t  *SV_GameClientNum( int num )
{
	playerState_t *ps;

	if ( num >= sv_maxclients->integer )
	{
		Com_Error( ERR_DROP, "SV_GameClientNum: bad num" );
	}

	ps = ( playerState_t * )( ( byte * ) sv.gameClients + sv.gameClientSize * ( num ) );

	return ps;
}

svEntity_t     *SV_SvEntityForGentity( sharedEntity_t *gEnt )
{
	if ( !gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES )
	{
		Com_Error( ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
	}

	return &sv.svEntities[ gEnt->s.number ];
}

sharedEntity_t *SV_GEntityForSvEntity( svEntity_t *svEnt )
{
	int num;

	num = svEnt - sv.svEntities;
	return SV_GentityNum( num );
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand( int clientNum, const char *text )
{
	if ( clientNum == -1 )
	{
		SV_SendServerCommand( NULL, "%s", text );
	}
	else if ( clientNum == -2 )
	{
		SV_PrintTranslatedText( text, qfalse, qfalse );
	}
	else
	{
		if ( clientNum < 0 || clientNum >= sv_maxclients->integer )
		{
			return;
		}

		SV_SendServerCommand( svs.clients + clientNum, "%s", text );
	}
}

/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient( int clientNum, const char *reason )
{
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer )
	{
		return;
	}

	SV_DropClient( svs.clients + clientNum, reason );
}

/*
===============
SV_RSAGenMsg

Generate an encrypted RSA message
===============
*/
int SV_RSAGenMsg( const char *pubkey, char *cleartext, char *encrypted )
{
	struct rsa_public_key public_key;

	mpz_t                 message;
	unsigned char         buffer[ RSA_KEY_LENGTH / 8 - 11 ];
	int                   retval;
	Com_RandomBytes( buffer, RSA_KEY_LENGTH / 8 - 11 );
	nettle_mpz_init_set_str_256_u( message, RSA_KEY_LENGTH / 8 - 11, buffer );
	mpz_get_str( cleartext, 16, message );
	rsa_public_key_init( &public_key );
	mpz_set_ui( public_key.e, RSA_PUBLIC_EXPONENT );
	retval = mpz_set_str( public_key.n, pubkey, 16 ) + 1;

	if ( retval )
	{
		rsa_public_key_prepare( &public_key );
		retval = rsa_encrypt( &public_key, NULL, qnettle_random, RSA_KEY_LENGTH / 8 - 11, buffer, message );
	}

	rsa_public_key_clear( &public_key );
	mpz_get_str( encrypted, 16, message );
	mpz_clear( message );
	return retval;
}

/*
=================
SV_SetBrushModel

sets mins and maxs for inline bmodels
=================
*/
void SV_SetBrushModel( sharedEntity_t *ent, const char *name )
{
	clipHandle_t h;
	vec3_t       mins, maxs;

	if ( !name )
	{
		Com_Error( ERR_DROP, "SV_SetBrushModel: NULL for #%i", ent->s.number );
	}

	if ( name[ 0 ] != '*' )
	{
		Com_Error( ERR_DROP, "SV_SetBrushModel: %s of #%i isn't a brush model", name, ent->s.number );
	}

	ent->s.modelindex = atoi( name + 1 );

	h = CM_InlineModel( ent->s.modelindex );
	CM_ModelBounds( h, mins, maxs );
	VectorCopy( mins, ent->r.mins );
	VectorCopy( maxs, ent->r.maxs );
	ent->r.bmodel = qtrue;

	ent->r.contents = -1; // we don't know exactly what is in the brushes
}

/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean SV_inPVS( const vec3_t p1, const vec3_t p2 )
{
	int  leafnum;
	int  cluster;
	int  area1, area2;
	byte *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
	area1 = CM_LeafArea( leafnum );
	mask = CM_ClusterPVS( cluster );

	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
	area2 = CM_LeafArea( leafnum );

	if ( mask && ( !( mask[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) ) )
	{
		return qfalse;
	}

	if ( !CM_AreasConnected( area1, area2 ) )
	{
		return qfalse; // a door blocks sight
	}

	return qtrue;
}

/*
=================
SV_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
qboolean SV_inPVSIgnorePortals( const vec3_t p1, const vec3_t p2 )
{
	int  leafnum;
	int  cluster;
//	int             area1, area2; //unused
	byte *mask;

	leafnum = CM_PointLeafnum( p1 );
	cluster = CM_LeafCluster( leafnum );
//	area1 = CM_LeafArea(leafnum); //Doesn't modify anything.

	mask = CM_ClusterPVS( cluster );
	leafnum = CM_PointLeafnum( p2 );
	cluster = CM_LeafCluster( leafnum );
//	area2 = CM_LeafArea(leafnum); //Doesn't modify anything.

	if ( mask && ( !( mask[ cluster >> 3 ] & ( 1 << ( cluster & 7 ) ) ) ) )
	{
		return qfalse;
	}

	return qtrue;
}

/*
========================
SV_AdjustAreaPortalState
========================
*/
void SV_AdjustAreaPortalState( sharedEntity_t *ent, qboolean open )
{
	svEntity_t *svEnt;

	svEnt = SV_SvEntityForGentity( ent );

	if ( svEnt->areanum2 == -1 )
	{
		return;
	}

	CM_AdjustAreaPortalState( svEnt->areanum, svEnt->areanum2, open );
}

/*
==================
SV_EntityContact
==================
*/
qboolean SV_EntityContact( vec3_t mins, vec3_t maxs, const sharedEntity_t *gEnt, traceType_t type )
{
	const float  *origin, *angles;
	clipHandle_t ch;
	trace_t      trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = SV_ClipHandleForEntity( gEnt );
	CM_TransformedBoxTrace( &trace, vec3_origin, vec3_origin, mins, maxs, ch, -1, origin, angles, type );

	return trace.startsolid;
}

/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo( char *buffer, int bufferSize )
{
	if ( bufferSize < 1 )
	{
		Com_Error( ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize );
	}

	Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO, qfalse ), bufferSize );
}

/*
===============
SV_LocateGameData

===============
*/

void SV_LocateGameData( const IPC::SharedMemory& shmRegion, int numGEntities, int sizeofGEntity_t,
                        int sizeofGameClient )
{
	if ( numGEntities < 0 || sizeofGEntity_t < 0 || sizeofGameClient < 0 )
		Com_Error( ERR_DROP, "SV_LocateGameData: Invalid game data parameters" );
	if ( shmRegion.GetSize() < numGEntities * sizeofGEntity_t + sv_maxclients->integer * sizeofGameClient )
		Com_Error( ERR_DROP, "SV_LocateGameData: Shared memory region too small" );

	char* base = static_cast<char*>(shmRegion.GetBase());
	sv.gentities = reinterpret_cast<sharedEntity_t*>(base);
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = reinterpret_cast<playerState_t*>(base + MAX_GENTITIES * sizeofGEntity_t);
	sv.gameClientSize = sizeofGameClient;
}

/*
===============
SV_GetUsercmd

===============
*/
void SV_GetUsercmd( int clientNum, usercmd_t *cmd )
{
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer )
	{
		Com_Error( ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum );
	}

	*cmd = svs.clients[ clientNum ].lastUsercmd;
}

/*
====================
SV_SendBinaryMessage
====================
*/
static void SV_SendBinaryMessage( int cno, const char *buf, int buflen )
{
	if ( cno < 0 || cno >= sv_maxclients->integer )
	{
		Com_Error( ERR_DROP, "SV_SendBinaryMessage: bad client %i", cno );
	}

	if ( buflen < 0 || buflen > MAX_BINARY_MESSAGE )
	{
		Com_Error( ERR_DROP, "SV_SendBinaryMessage: bad length %i", buflen );
	}

	svs.clients[ cno ].binaryMessageLength = buflen;
	memcpy( svs.clients[ cno ].binaryMessage, buf, buflen );
}

/*
====================
SV_BinaryMessageStatus
====================
*/
static int SV_BinaryMessageStatus( int cno )
{
	if ( cno < 0 || cno >= sv_maxclients->integer )
	{
		return qfalse;
	}

	if ( svs.clients[ cno ].binaryMessageLength == 0 )
	{
		return MESSAGE_EMPTY;
	}

	if ( svs.clients[ cno ].binaryMessageOverflowed )
	{
		return MESSAGE_WAITING_OVERFLOW;
	}

	return MESSAGE_WAITING;
}

/*
====================
SV_GameBinaryMessageReceived
====================
*/
void SV_GameBinaryMessageReceived( int cno, const char *buf, int buflen, int commandTime )
{
	gvm->GameMessageRecieved( cno, buf, buflen, commandTime );
}

//==============================================

/*
====================
SV_GetTimeString

Returns 0 if we have a representable time
Truncation is ignored
====================
*/
static void SV_GetTimeString( char *buffer, int length, const char *format, const qtime_t *tm )
{
	if ( tm )
	{
		struct tm t;

		t.tm_sec   = tm->tm_sec;
		t.tm_min   = tm->tm_min;
		t.tm_hour  = tm->tm_hour;
		t.tm_mday  = tm->tm_mday;
		t.tm_mon   = tm->tm_mon;
		t.tm_year  = tm->tm_year;
		t.tm_wday  = tm->tm_wday;
		t.tm_yday  = tm->tm_yday;
		t.tm_isdst = tm->tm_isdst;

		strftime ( buffer, length, format, &t );
	}
	else
	{
		strftime( buffer, length, format, gmtime( NULL ) );
	}
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs( void )
{
	if ( !gvm )
	{
		return;
	}

	gvm->GameShutdown( qfalse );
	delete gvm;
    gvm = nullptr;

	if ( sv_newGameShlib->string[ 0 ] )
	{
		FS_Rename( sv_newGameShlib->string, "game" DLL_EXT );
		Cvar_Set( "sv_newGameShlib", "" );
	}
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM( qboolean restart )
{
	int i;

	// start the entity parsing at the beginning
	sv.entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	for ( i = 0; i < sv_maxclients->integer; i++ )
	{
		svs.clients[ i ].gentity = NULL;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	gvm->GameInit( svs.time, Com_Milliseconds(), restart );
}

/*
===================
SV_CreateGameVM

Load a QVM vm or fails and try to load a NaCl vm
===================
*/
GameVM* SV_CreateGameVM( void )
{
    GameVM* vm = new GameVM();

    if (vm->Start()) {
        return vm;
    }
    delete vm;

	Com_Error(ERR_DROP, "Couldn't load the game VM");

	return nullptr;
}

/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a map change
===================
*/
void SV_RestartGameProgs( void )
{
	if ( !gvm )
	{
		return;
	}

	gvm->GameShutdown( qtrue );

	delete gvm;
	gvm = nullptr;

	gvm = SV_CreateGameVM();

	SV_InitGameVM( qtrue );
}

/*
===============
SV_InitGameProgs

Called on a map change, not on a map_restart
===============
*/
void SV_InitGameProgs( void )
{
	sv.num_tagheaders = 0;
	sv.num_tags = 0;

	// load the game module
	gvm = SV_CreateGameVM();

	SV_InitGameVM( qfalse );
}

/*
====================
SV_GetTag

  return qfalse if unable to retrieve tag information for this client
====================
*/
extern qboolean CL_GetTag( int clientNum, const char *tagname, orientation_t * org );

qboolean SV_GetTag( int clientNum, int tagFileNumber, const char *tagname, orientation_t * org )
{
	int i;

	if ( tagFileNumber > 0 && tagFileNumber <= sv.num_tagheaders )
	{
		for ( i = sv.tagHeadersExt[ tagFileNumber - 1 ].start;
		      i < sv.tagHeadersExt[ tagFileNumber - 1 ].start + sv.tagHeadersExt[ tagFileNumber - 1 ].count; i++ )
		{
			if ( !Q_stricmp( sv.tags[ i ].name, tagname ) )
			{
				VectorCopy( sv.tags[ i ].origin, org ->origin );
				VectorCopy( sv.tags[ i ].axis[ 0 ], org ->axis[ 0 ] );
				VectorCopy( sv.tags[ i ].axis[ 1 ], org ->axis[ 1 ] );
				VectorCopy( sv.tags[ i ].axis[ 2 ], org ->axis[ 2 ] );
				return qtrue;
			}
		}
	}

	// Gordon: let's try and remove the inconsistency between ded/non-ded servers...
	// Gordon: bleh, some code in clientthink_real really relies on this working on player models...
#ifndef DEDICATED // TTimo: dedicated only binary defines DEDICATED

	if ( com_dedicated->integer )
	{
		return qfalse;
	}

	return CL_GetTag( clientNum, tagname, org );
#else
	return qfalse;
#endif
}

GameVM::GameVM(): services(new VM::CommonVMServices(*this, "Game", Cmd::GAME))
{
}

bool GameVM::Start()
{
    int version = this->Create( "game", ( VM::vmType_t ) vm_game->integer );

    if (version < 0)
    {
        return false;
    }

	if ( version != GAME_API_VERSION ) {
		Com_Error( ERR_DROP, "Game ABI mismatch, expected %d, got %d", GAME_API_VERSION, version );
    }


    return true;
}

GameVM::~GameVM()
{
    this->Free();
}

void GameVM::GameInit(int levelTime, int randomSeed, qboolean restart)
{
	this->SendMsg<GameInitMsg>(levelTime, randomSeed, restart);
}

void GameVM::GameShutdown(qboolean restart)
{
	//TODO ignore errors
	this->SendMsg<GameShutdownMsg>(restart);

	// Release the shared memory region
	this->shmRegion.Close();
}

qboolean GameVM::GameClientConnect(char* reason, size_t size, int clientNum, qboolean firstTime, qboolean isBot)
{
	bool denied;
	std::string sentReason;
	this->SendMsg<GameClientConnectMsg>(clientNum, firstTime, isBot, denied, sentReason);

	if (denied) {
		Q_strncpyz(reason, sentReason.c_str(), size);
	}
	return denied;
}

void GameVM::GameClientBegin(int clientNum)
{
	this->SendMsg<GameClientBeginMsg>(clientNum);
}

void GameVM::GameClientUserInfoChanged(int clientNum)
{
	this->SendMsg<GameClientUserinfoChangedMsg>(clientNum);
}

void GameVM::GameClientDisconnect(int clientNum)
{
	this->SendMsg<GameClientDisconnectMsg>(clientNum);
}

void GameVM::GameClientCommand(int clientNum, const char* command)
{
	this->SendMsg<GameClientCommandMsg>(clientNum, command);
}

void GameVM::GameClientThink(int clientNum)
{
	this->SendMsg<GameClientThinkMsg>(clientNum);
}

void GameVM::GameRunFrame(int levelTime)
{
	this->SendMsg<GameRunFrameMsg>(levelTime);
}

qboolean GameVM::GameSnapshotCallback(int entityNum, int clientNum)
{
	Com_Error(ERR_DROP, "GameVM::GameSnapshotCallback not implemented");
}

void GameVM::BotAIStartFrame(int levelTime)
{
	Com_Error(ERR_DROP, "GameVM::BotAIStartFrame not implemented");
}

void GameVM::GameMessageRecieved(int clientNum, const char *buffer, int bufferSize, int commandTime)
{
	//Com_Error(ERR_DROP, "GameVM::GameMessageRecieved not implemented");
}

void GameVM::Syscall(uint32_t id, IPC::Reader reader, const IPC::Socket& socket) const
{
	int major = id >> 16;
	int minor = id & 0xffff;
	if (major == VM::QVM) {
		this->QVMSyscall(minor, reader, socket);

    } else if (major < VM::LAST_COMMON_SYSCALL) {
        services->Syscall(major, minor, std::move(reader), socket);

    } else {
		Com_Error(ERR_DROP, "Bad major game syscall number: %d", major);
	}
}

void GameVM::QVMSyscall(int index, IPC::Reader& reader, const IPC::Socket& socket) const
{
	switch (index) {
	case G_PRINT:
		IPC::HandleMsg<PrintMsg>(socket, std::move(reader), [this](Str::StringRef text) {
			Com_Printf("%s", text.c_str());
		});
		break;

	case G_ERROR:
		IPC::HandleMsg<ErrorMsg>(socket, std::move(reader), [this](Str::StringRef text) {
			Com_Error(ERR_DROP, "%s", text.c_str());
		});
		break;

	case G_LOG:
		Com_Error(ERR_DROP, "trap_Log not implemented");

	case G_MILLISECONDS:
		IPC::HandleMsg<MillisecondsMsg>(socket, std::move(reader), [this](int& time) {
			time = Sys_Milliseconds();
		});
		break;

	case G_SEND_CONSOLE_COMMAND:
		IPC::HandleMsg<SendConsoleCommandMsg>(socket, std::move(reader), [this](int when, Str::StringRef text) {
			Cbuf_ExecuteText(when, text.c_str());
		});
		break;

	case G_FS_FOPEN_FILE:
		IPC::HandleMsg<FSFOpenFileMsg>(socket, std::move(reader), [this](Str::StringRef filename, bool open, int fsMode, int& success, int& handle) {
			fsMode_t mode = static_cast<fsMode_t>(fsMode);
			success = FS_Game_FOpenFileByMode(filename.c_str(), open ? &handle : NULL, mode);
		});
		break;

	case G_FS_READ:
		IPC::HandleMsg<FSReadMsg>(socket, std::move(reader), [this](int handle, int len, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			FS_Read(buffer.get(), len, handle);
			res.assign(buffer.get(), len);
		});
		break;

	case G_FS_WRITE:
		IPC::HandleMsg<FSWriteMsg>(socket, std::move(reader), [this](int handle, Str::StringRef text, int& res) {
			res = FS_Write(text.c_str(), text.size(), handle);
		});
		break;

	case G_FS_RENAME:
		IPC::HandleMsg<FSRenameMsg>(socket, std::move(reader), [this](Str::StringRef from, Str::StringRef to) {
			FS_Rename(from.c_str(), to.c_str());
		});
		break;

	case G_FS_FCLOSE_FILE:
		IPC::HandleMsg<FSFCloseFileMsg>(socket, std::move(reader), [this](int handle) {
			FS_FCloseFile(handle);
		});
		break;

	case G_FS_GET_FILE_LIST:
		IPC::HandleMsg<FSGetFileListMsg>(socket, std::move(reader), [this](Str::StringRef path, Str::StringRef extension, int len, int& intRes, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			intRes = FS_GetFileList(path.c_str(), extension.c_str(), buffer.get(), len);
			res.assign(buffer.get(), len);
		});
		break;

	case G_FS_FIND_PAK:
		IPC::HandleMsg<FSFindPakMsg>(socket, std::move(reader), [this](Str::StringRef pakName, bool& found) {
			found = FS::FindPak(pakName);
		});
		break;

	case G_LOCATE_GAME_DATA1:
		IPC::HandleMsg<LocateGameDataMsg1>(socket, std::move(reader), [this](IPC::SharedMemory shm, int numEntities, int entitySize, int playerSize) {
			shmRegion = std::move(shm);
			SV_LocateGameData(shmRegion, numEntities, entitySize, playerSize);
		});
		break;

	case G_LOCATE_GAME_DATA2:
		IPC::HandleMsg<LocateGameDataMsg2>(socket, std::move(reader), [this](int numEntities, int entitySize, int playerSize) {
			SV_LocateGameData(shmRegion, numEntities, entitySize, playerSize);
		});
		break;

	case G_LINK_ENTITY:
		IPC::HandleMsg<LinkEntityMsg>(socket, std::move(reader), [this](int entityNum) {
			SV_LinkEntity(SV_GentityNum(entityNum));
		});
		break;

	case G_UNLINK_ENTITY:
		IPC::HandleMsg<UnlinkEntityMsg>(socket, std::move(reader), [this](int entityNum) {
			SV_UnlinkEntity(SV_GentityNum(entityNum));
		});
		break;

	case G_ENTITIES_IN_BOX:
		IPC::HandleMsg<EntitiesInBoxMsg>(socket, std::move(reader), [this](std::array<float, 3> mins, std::array<float, 3> maxs, int len, std::vector<int>& res) {
			res.resize(len); //reserve doesn't guarantee that data() will return an array long enough
			len = SV_AreaEntities(mins.data(), maxs.data(), res.data(), len);
		});
		break;

	case G_ENTITY_CONTACT:
		IPC::HandleMsg<EntityContactMsg>(socket, std::move(reader), [this](std::array<float, 3> mins, std::array<float, 3> maxs, int entityNum, int& res) {
			const sharedEntity_t* ent = SV_GentityNum(entityNum);
			res = SV_EntityContact(mins.data(), maxs.data(), ent, TT_AABB);
		});

	case G_TRACE:
		IPC::HandleMsg<TraceMsg>(socket, std::move(reader), [this](std::array<float, 3> start, std::array<float, 3> mins, std::array<float, 3> maxs, std::array<float, 3> end, int passEntityNum, int contentMask, trace_t& res) {
			SV_Trace(&res, start.data(), mins.data(), maxs.data(), end.data(), passEntityNum, contentMask, TT_AABB);
		});

	case G_POINT_CONTENTS:
		IPC::HandleMsg<PointContentsMsg>(socket, std::move(reader), [this](std::array<float, 3> p, int passEntityNum, int& res) {
			res = SV_PointContents(p.data(), passEntityNum);
		});
		break;

	case G_SET_BRUSH_MODEL:
		IPC::HandleMsg<SetBrushModelMsg>(socket, std::move(reader), [this](int entityNum, Str::StringRef name) {
			sharedEntity_t* ent = SV_GentityNum(entityNum);
			SV_SetBrushModel(ent, name.c_str());
		});
		break;

	case G_IN_PVS:
		IPC::HandleMsg<InPVSMsg>(socket, std::move(reader), [this](std::array<float, 3> p1, std::array<float, 3> p2, bool& res) {
			res = SV_inPVS(p1.data(), p2.data());
		});
		break;

	case G_IN_PVS_IGNORE_PORTALS:
		IPC::HandleMsg<InPVSIgnorePortalsMsg>(socket, std::move(reader), [this](std::array<float, 3> p1, std::array<float, 3> p2, bool& res) {
			res = SV_inPVSIgnorePortals(p1.data(), p2.data());
		});
		break;

	case G_ADJUST_AREA_PORTAL_STATE:
		IPC::HandleMsg<AdjustAreaPortalStateMsg>(socket, std::move(reader), [this](int entityNum, bool open) {
			sharedEntity_t* ent = SV_GentityNum(entityNum);
			SV_AdjustAreaPortalState(ent, open);
        });
		break;

	case G_AREAS_CONNECTED:
		IPC::HandleMsg<AreasConnectedMsg>(socket, std::move(reader), [this](int area1, int area2, bool& res) {
			res = CM_AreasConnected(area1, area2);
		});
		break;


	case G_DROP_CLIENT:
		IPC::HandleMsg<DropClientMsg>(socket, std::move(reader), [this](int clientNum, Str::StringRef reason) {
			SV_GameDropClient(clientNum, reason.c_str());
		});
		break;

	case G_SEND_SERVER_COMMAND:
		IPC::HandleMsg<SendServerCommandMsg>(socket, std::move(reader), [this](int clientNum, Str::StringRef text) {
			SV_GameSendServerCommand(clientNum, text.c_str());
		});
		break;

	case G_SET_CONFIGSTRING:
		IPC::HandleMsg<SetConfigStringMsg>(socket, std::move(reader), [this](int index, Str::StringRef val) {
			SV_SetConfigstring(index, val.c_str());
		});
		break;

	case G_GET_CONFIGSTRING:
		IPC::HandleMsg<GetConfigStringMsg>(socket, std::move(reader), [this](int index, int len, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			SV_GetConfigstring(index, buffer.get(), len);
			res.assign(buffer.get(), len);
		});
		break;

	case G_SET_CONFIGSTRING_RESTRICTIONS:
		IPC::HandleMsg<SetConfigStringRestrictionsMsg>(socket, std::move(reader), [this]() {
			Com_Printf("SV_SetConfigstringRestrictions not implemented\n");
		});
		break;

	case G_SET_USERINFO:
		IPC::HandleMsg<SetUserinfoMsg>(socket, std::move(reader), [this](int index, Str::StringRef val) {
			SV_SetUserinfo(index, val.c_str());
		});
		break;

	case G_GET_USERINFO:
		IPC::HandleMsg<GetUserinfoMsg>(socket, std::move(reader), [this](int index, int len, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			SV_GetUserinfo(index, buffer.get(), len);
			res.assign(buffer.get(), len);
		});
		break;

	case G_GET_SERVERINFO:
		IPC::HandleMsg<GetServerinfoMsg>(socket, std::move(reader), [this](int len, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			SV_GetServerinfo(buffer.get(), len);
			res.assign(buffer.get(), len);
		});
		break;

	case G_GET_USERCMD:
		IPC::HandleMsg<GetUsercmdMsg>(socket, std::move(reader), [this](int index, usercmd_t& cmd) {
			SV_GetUsercmd(index, &cmd);
		});
		break;

	case G_GET_ENTITY_TOKEN:
		IPC::HandleMsg<GetEntityTokenMsg>(socket, std::move(reader), [this](bool& boolRes, std::string& res) {
			res = COM_Parse(&sv.entityParsePoint);
			boolRes = sv.entityParsePoint or res.size() > 0;
		});
		break;

	case G_SEND_GAME_STAT:
		IPC::HandleMsg<SendGameStatMsg>(socket, std::move(reader), [this](Str::StringRef text) {
			SV_MasterGameStat(text.c_str());
		});
		break;

	case G_GET_TAG:
		IPC::HandleMsg<GetTagMsg>(socket, std::move(reader), [this](int clientNum, int tagFileNumber, Str::StringRef tagName, int& res, orientation_t& orientation) {
			res = SV_GetTag(clientNum, tagFileNumber, tagName.c_str(), &orientation);
		});
		break;

	case G_REGISTER_TAG:
		IPC::HandleMsg<RegisterTagMsg>(socket, std::move(reader), [this](Str::StringRef tagFileName, int& res) {
			res = SV_LoadTag(tagFileName.c_str());
		});
		break;

	case G_SEND_MESSAGE:
		IPC::HandleMsg<SendMessageMsg>(socket, std::move(reader), [this](int clientNum, int len, std::vector<char> message) {
			SV_SendBinaryMessage(clientNum, message.data(), len);
		});
		break;

	case G_MESSAGE_STATUS:
		IPC::HandleMsg<MessageStatusMsg>(socket, std::move(reader), [this](int index, int& status) {
			status = SV_BinaryMessageStatus(index);
		});
		break;

	case G_RSA_GENMSG:
		IPC::HandleMsg<RSAGenMsgMsg>(socket, std::move(reader), [this](Str::StringRef pubkey, int& res, std::string& cleartext, std::string& encrypted) {
			char cleartextBuffer[RSA_STRING_LENGTH];
			char encryptedBuffer[RSA_STRING_LENGTH];
			res = SV_RSAGenMsg(pubkey.c_str(), cleartextBuffer, encryptedBuffer);
			cleartext = cleartextBuffer;
			encrypted = encryptedBuffer;
		});
		break;

	case G_GEN_FINGERPRINT:
		IPC::HandleMsg<GenFingerprintMsg>(socket, std::move(reader), [this](int keylen, const std::vector<char>& key, int len, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			Com_MD5Buffer(key.data(), keylen, buffer.get(), len);
			res.assign(buffer.get(), len);
		});
		break;

	case G_GET_PLAYER_PUBKEY:
		IPC::HandleMsg<GetPlayerPubkeyMsg>(socket, std::move(reader), [this](int clientNum, int len, std::string& pubkey) {
			std::unique_ptr<char[]> buffer(new char[len]);
			SV_GetPlayerPubkey(clientNum, buffer.get(), len);
			pubkey.assign(buffer.get());
		});
		break;

	case G_GM_TIME:
		IPC::HandleMsg<GMTimeMsg>(socket, std::move(reader), [this](int& res, qtime_t& time) {
			res = Com_GMTime(&time);
		});
		break;

	case G_GET_TIME_STRING:
		IPC::HandleMsg<GetTimeStringMsg>(socket, std::move(reader), [this](int len, Str::StringRef format, const qtime_t& time, std::string& res) {
			std::unique_ptr<char[]> buffer(new char[len]);
			SV_GetTimeString(buffer.get(), len, format.c_str(), &time);
			res.assign(buffer.get(), len);
		});
		break;

	case G_PARSE_ADD_GLOBAL_DEFINE:
		IPC::HandleMsg<ParseAddGlobalDefineMsg>(socket, std::move(reader), [this](Str::StringRef define, int& res) {
			res = Parse_AddGlobalDefine(define.c_str());
		});
		break;

	case G_PARSE_LOAD_SOURCE:
		IPC::HandleMsg<ParseLoadSourceMsg>(socket, std::move(reader), [this](Str::StringRef name, int& res) {
			res = Parse_LoadSourceHandle(name.c_str());
		});
		break;

	case G_PARSE_FREE_SOURCE:
		IPC::HandleMsg<ParseFreeSourceMsg>(socket, std::move(reader), [this](int source, int& res) {
			res = Parse_FreeSourceHandle(source);
		});
		break;

	case G_PARSE_READ_TOKEN:
		IPC::HandleMsg<ParseReadTokenMsg>(socket, std::move(reader), [this](int source, int& res, pc_token_t& token) {
			res = Parse_ReadTokenHandle(source, &token);
		});
		break;

	case G_PARSE_SOURCE_FILE_AND_LINE:
		IPC::HandleMsg<ParseSourceFileAndLineMsg>(socket, std::move(reader), [this](int source, int& res, std::string& file, int& line) {
			char buffer[128] = {0};
			res = Parse_SourceFileAndLine(source, buffer, &line);
			file = buffer;
		});
		break;
/*
	case G_BOT_ALLOCATE_CLIENT:
		outputs.WriteInt(SV_BotAllocateClient(inputs.ReadInt()));
		break;

	case G_BOT_FREE_CLIENT:
		SV_BotFreeClient(inputs.ReadInt());
		break;

	case BOT_GET_CONSOLE_MESSAGE:
	{
		int client = inputs.ReadInt();
		int len = inputs.ReadInt();
		std::unique_ptr<char[]> buffer(new char[len]);
		outputs.WriteInt(SV_BotGetConsoleMessage(client, buffer.get(), len));
		outputs.WriteString(buffer.get());
		break;
	}

	case BOT_NAV_SETUP:
	{
		botClass_t botClass;
		qhandle_t handle;
		inputs.Read(&botClass, sizeof(botClass_t));
		outputs.WriteInt(BotSetupNav(&botClass, &handle));
		outputs.WriteInt(handle);
		break;
	}

	case BOT_NAV_SHUTDOWN:
		BotShutdownNav();
		break;

	case BOT_SET_NAVMESH:
	{
		int botClientNum = inputs.ReadInt();
		qhandle_t navHandle = inputs.ReadInt();
		BotSetNavMesh(botClientNum, navHandle);
		break;
	}

	case BOT_FIND_ROUTE:
	{
		int botClientNum = inputs.ReadInt();
		botRouteTarget_t target;
		inputs.Read(&target, sizeof(botRouteTarget_t));
		qboolean allowPartial = inputs.ReadInt();
		outputs.WriteInt(BotFindRouteExt(botClientNum, &target, allowPartial));
		break;
	}

	case BOT_UPDATE_PATH:
	{
		int botClientNum = inputs.ReadInt();
		botRouteTarget_t target;
		inputs.Read(&target, sizeof(botRouteTarget_t));
		botNavCmd_t cmd;
		BotUpdateCorridor(botClientNum, &target, &cmd);
		outputs.Write(&cmd, sizeof(botNavCmd_t));
		break;
	}

	case BOT_NAV_RAYCAST:
	{
		int botClientNum = inputs.ReadInt();
		botTrace_t botTrace;
		vec3_t start;
		inputs.Read(start, sizeof(vec3_t));
		vec3_t end;
		inputs.Read(end, sizeof(vec3_t));
		outputs.WriteInt(BotNavTrace(botClientNum, &botTrace, start, end));
		outputs.Write(&botTrace, sizeof(botTrace_t));
		break;
	}

	case BOT_NAV_RANDOMPOINT:
	{
		int botClientNum = inputs.ReadInt();
		vec3_t point;
		BotFindRandomPoint(botClientNum, point);
		outputs.Write(point, sizeof(vec3_t));
		break;
	}

	case BOT_NAV_RANDOMPOINTRADIUS:
	{
		int botClientNum = inputs.ReadInt();
		vec3_t origin;
		inputs.Read(origin, sizeof(vec3_t));
		vec3_t point;
		float radius = inputs.ReadFloat();
		BotFindRandomPointInRadius(botClientNum, origin, point, radius);
		outputs.Write(point, sizeof(vec3_t));
		break;
	}

	case BOT_ENABLE_AREA:
	{
		vec3_t origin;
		inputs.Read(origin, sizeof(vec3_t));
		vec3_t mins;
		inputs.Read(mins, sizeof(vec3_t));
		vec3_t maxs;
		inputs.Read(maxs, sizeof(vec3_t));
		BotEnableArea(origin, mins, maxs);
		break;
	}

	case BOT_DISABLE_AREA:
	{
		vec3_t origin;
		inputs.Read(origin, sizeof(vec3_t));
		vec3_t mins;
		inputs.Read(mins, sizeof(vec3_t));
		vec3_t maxs;
		inputs.Read(maxs, sizeof(vec3_t));
		BotDisableArea(origin, mins, maxs);
		break;
	}

	case BOT_ADD_OBSTACLE:
	{
		vec3_t mins;
		inputs.Read(mins, sizeof(vec3_t));
		vec3_t maxs;
		inputs.Read(maxs, sizeof(vec3_t));
		qhandle_t handle;
		BotAddObstacle(mins, maxs, &handle);
		outputs.WriteInt(handle);
		break;
	}

	case BOT_REMOVE_OBSTACLE:
		BotRemoveObstacle(inputs.ReadInt());
		break;

	case BOT_UPDATE_OBSTACLES:
		BotUpdateObstacles();
		break;
*/
	default:
		Com_Error(ERR_DROP, "Bad game system trap: %d", index);
	}
}
