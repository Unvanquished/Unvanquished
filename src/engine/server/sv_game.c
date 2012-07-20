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

// sv_game.c -- interface to the game dll

#include "server.h"

#ifdef USE_CRYPTO
#include "../qcommon/crypto.h"
#endif

void            CMod_PhysicsAddEntity( sharedEntity_t *gEnt );
void            CMod_PhysicsAddStatic( const sharedEntity_t *gEnt );

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

	ent = ( sharedEntity_t * )( ( byte * ) sv.gentities + sv.gentitySize * ( num ) );

	return ent;
}

playerState_t  *SV_GameClientNum( int num )
{
	playerState_t *ps;

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
		SV_PrintTranslatedText( text, qfalse );
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
void SV_GameDropClient( int clientNum, const char *reason, int length )
{
	if ( clientNum < 0 || clientNum >= sv_maxclients->integer )
	{
		return;
	}

	SV_DropClient( svs.clients + clientNum, reason );

	if ( length )
	{
		SV_TempBanNetAddress( svs.clients[ clientNum ].netchan.remoteAddress, length );
	}
}

/*
===============
SV_RSAGenMsg

Generate an encrypted RSA message
===============
*/
int SV_RSAGenMsg( const char *pubkey, char *cleartext, char *encrypted )
{
#ifdef USE_CRYPTO
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
#else
	return 0;
#endif
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
		Com_Error( ERR_DROP, "SV_SetBrushModel: NULL" );
	}

	if ( name[ 0 ] != '*' )
	{
		Com_Error( ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name );
	}

	ent->s.modelindex = atoi( name + 1 );

	h = CM_InlineModel( ent->s.modelindex );
	CM_ModelBounds( h, mins, maxs );
	VectorCopy( mins, ent->r.mins );
	VectorCopy( maxs, ent->r.maxs );
	ent->r.bmodel = qtrue;

	ent->r.contents = -1; // we don't know exactly what is in the brushes

	SV_LinkEntity( ent );  // FIXME: remove
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
SV_GameAreaEntities
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

	Q_strncpyz( buffer, Cvar_InfoString( CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE ), bufferSize );
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData( sharedEntity_t *gEnts, int numGEntities, int sizeofGEntity_t,
                        playerState_t *clients, int sizeofGameClient )
{
	sv.gentities = gEnts;
	sv.gentitySize = sizeofGEntity_t;
	sv.num_entities = numGEntities;

	sv.gameClients = clients;
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
static void SV_SendBinaryMessage( int cno, char *buf, int buflen )
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
	VM_Call( gvm, GAME_MESSAGERECEIVED, cno, buf, buflen, commandTime );
}

// taken from cl_main.c
#define MAX_RCON_MESSAGE 1024

/*
===============
SV_UpdateSharedConfig
===============
*/

void SV_UpdateSharedConfig( unsigned int port, const char *rconpass )
{
	char     message[ MAX_RCON_MESSAGE ];
	netadr_t to;

	message[ 0 ] = -1;
	message[ 1 ] = -1;
	message[ 2 ] = -1;
	message[ 3 ] = -1;
	message[ 4 ] = 0;

	Q_strcat( message, MAX_RCON_MESSAGE, "rcon " );

	Q_strcat( message, MAX_RCON_MESSAGE, rconpass );
	Q_strcat( message, MAX_RCON_MESSAGE, " !readconfig" );
	NET_StringToAdr( "127.0.0.1", &to, NA_UNSPEC );
	to.port = BigShort( port );

	NET_SendPacket( NS_SERVER, strlen( message ) + 1, message, to );
}

//==============================================

extern int S_RegisterSound( const char *name, qboolean compressed );
extern int S_GetSoundLength( sfxHandle_t sfxHandle );

/*
====================
SV_GameSystemCalls

The module is making a system call
====================
*/
intptr_t SV_GameSystemCalls( intptr_t *args )
{
	switch ( args[ 0 ] )
	{
		case G_PRINT:
			Com_Printf( "%s", ( char * ) VMA( 1 ) );
			return 0;

		case G_ERROR:
			Com_Error( ERR_DROP, "%s", ( char * ) VMA( 1 ) );

		case G_MILLISECONDS:
			return Sys_Milliseconds();

		case G_CVAR_REGISTER:
			Cvar_Register( VMA( 1 ), VMA( 2 ), VMA( 3 ), args[ 4 ] );
			return 0;

		case G_CVAR_UPDATE:
			Cvar_Update( VMA( 1 ) );
			return 0;

		case G_CVAR_SET:
			Cvar_Set( ( const char * ) VMA( 1 ), ( const char * ) VMA( 2 ) );
			return 0;

		case G_CVAR_VARIABLE_INTEGER_VALUE:
			return Cvar_VariableIntegerValue( ( const char * ) VMA( 1 ) );

		case G_CVAR_VARIABLE_STRING_BUFFER:
		        VM_CheckBlock( args[2], args[3], "CVARVSB" );
			Cvar_VariableStringBuffer( VMA( 1 ), VMA( 2 ), args[ 3 ] );
			return 0;

		case G_CVAR_LATCHEDVARIABLESTRINGBUFFER:
		        VM_CheckBlock( args[2], args[3], "CVARLVSB" );
			Cvar_LatchedVariableStringBuffer( VMA( 1 ), VMA( 2 ), args[ 3 ] );
			return 0;

		case G_ARGC:
			return Cmd_Argc();

		case G_ARGV:
		        VM_CheckBlock( args[2], args[3], "ARGV" );
			Cmd_ArgvBuffer( args[ 1 ], VMA( 2 ), args[ 3 ] );
			return 0;

		case G_SEND_CONSOLE_COMMAND:
			Cbuf_ExecuteText( args[ 1 ], VMA( 2 ) );
			return 0;

		case G_FS_FOPEN_FILE:
			return FS_FOpenFileByMode( VMA( 1 ), VMA( 2 ), args[ 3 ] );

		case G_FS_READ:
		        VM_CheckBlock( args[1], args[2], "FSREAD" );
			FS_Read2( VMA( 1 ), args[ 2 ], args[ 3 ] );
			return 0;

		case G_FS_WRITE:
		        VM_CheckBlock( args[1], args[2], "FSWRITE" );
			return FS_Write( VMA( 1 ), args[ 2 ], args[ 3 ] );

		case G_FS_RENAME:
			FS_Rename( VMA( 1 ), VMA( 2 ) );
			return 0;

		case G_FS_FCLOSE_FILE:
			FS_FCloseFile( args[ 1 ] );
			return 0;

		case G_FS_GETFILELIST:
		        VM_CheckBlock( args[3], args[4], "FSGFL" );
			return FS_GetFileList( VMA( 1 ), VMA( 2 ), VMA( 3 ), args[ 4 ] );

		case G_LOCATE_GAME_DATA:
			SV_LocateGameData( VMA( 1 ), args[ 2 ], args[ 3 ], VMA( 4 ), args[ 5 ] );
			return 0;

		case G_DROP_CLIENT:
			SV_GameDropClient( args[ 1 ], VMA( 2 ), args[ 3 ] );
			return 0;

		case G_SEND_SERVER_COMMAND:
			SV_GameSendServerCommand( args[ 1 ], VMA( 2 ) );
			return 0;

		case G_LINKENTITY:
			SV_LinkEntity( VMA( 1 ) );
			return 0;

		case G_UNLINKENTITY:
			SV_UnlinkEntity( VMA( 1 ) );
			return 0;

		case G_ENTITIES_IN_BOX:
		        VM_CheckBlock( args[3], args[4] * sizeof( int ), "ENTIB" );
			return SV_AreaEntities( VMA( 1 ), VMA( 2 ), VMA( 3 ), args[ 4 ] );

		case G_ENTITY_CONTACT:
			return SV_EntityContact( VMA( 1 ), VMA( 2 ), VMA( 3 ), TT_AABB );

		case G_ENTITY_CONTACTCAPSULE:
			return SV_EntityContact( VMA( 1 ), VMA( 2 ), VMA( 3 ), TT_CAPSULE );

		case G_TRACE:
			SV_Trace( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ), VMA( 5 ), args[ 6 ], args[ 7 ], TT_AABB );
			return 0;

		case G_TRACECAPSULE:
			SV_Trace( VMA( 1 ), VMA( 2 ), VMA( 3 ), VMA( 4 ), VMA( 5 ), args[ 6 ], args[ 7 ], TT_CAPSULE );
			return 0;

		case G_POINT_CONTENTS:
			return SV_PointContents( VMA( 1 ), args[ 2 ] );

		case G_SET_BRUSH_MODEL:
			SV_SetBrushModel( VMA( 1 ), VMA( 2 ) );
			return 0;

		case G_IN_PVS:
			return SV_inPVS( VMA( 1 ), VMA( 2 ) );

		case G_IN_PVS_IGNORE_PORTALS:
			return SV_inPVSIgnorePortals( VMA( 1 ), VMA( 2 ) );

		case G_SET_CONFIGSTRING:
			SV_SetConfigstring( args[ 1 ], VMA( 2 ) );
			return 0;

		case G_GET_CONFIGSTRING:
		        VM_CheckBlock( args[2], args[3], "GETCS" );
			SV_GetConfigstring( args[ 1 ], VMA( 2 ), args[ 3 ] );
			return 0;

		case G_SET_CONFIGSTRING_RESTRICTIONS:
			// FIXME
			//SV_SetConfigstringRestrictions( args[1], VMA(2) );
			return 0;

		case G_SET_USERINFO:
			SV_SetUserinfo( args[ 1 ], VMA( 2 ) );
			return 0;

		case G_GET_USERINFO:
		        VM_CheckBlock( args[2], args[3], "GETUI" );
			SV_GetUserinfo( args[ 1 ], VMA( 2 ), args[ 3 ] );
			return 0;

		case G_GET_SERVERINFO:
		        VM_CheckBlock( args[2], args[3], "GETSI" );
			SV_GetServerinfo( VMA( 1 ), args[ 2 ] );
			return 0;

		case G_ADJUST_AREA_PORTAL_STATE:
			SV_AdjustAreaPortalState( VMA( 1 ), args[ 2 ] );
			return 0;

		case G_AREAS_CONNECTED:
			return CM_AreasConnected( args[ 1 ], args[ 2 ] );

		case G_UPDATE_SHARED_CONFIG:
			SV_UpdateSharedConfig( args[ 1 ], VMA( 2 ) );
			return 0;

		case G_BOT_ALLOCATE_CLIENT:
			return SV_BotAllocateClient( args[ 1 ] );

		case G_BOT_FREE_CLIENT:
			SV_BotFreeClient( args[ 1 ] );
			return 0;

		case BOT_GET_CONSOLE_MESSAGE:
		        VM_CheckBlock( args[2], args[3], "BOTGCM" );
			return SV_BotGetConsoleMessage( args[ 1 ], VMA( 2 ), args[ 3 ] );

		case G_GET_USERCMD:
			SV_GetUsercmd( args[ 1 ], VMA( 2 ) );
			return 0;

		case G_GET_ENTITY_TOKEN:
		        VM_CheckBlock( args[1], args[2], "GETET" );
			{
				const char *s;

				s = COM_Parse( &sv.entityParsePoint );
				Q_strncpyz( VMA( 1 ), s, args[ 2 ] );

				if ( !sv.entityParsePoint && !s[ 0 ] )
				{
					return qfalse;
				}
				else
				{
					return qtrue;
				}
			}

		case G_REAL_TIME:
			return Com_RealTime( VMA( 1 ) );

		case G_SNAPVECTOR:
			Q_SnapVector( VMA( 1 ) );
			return 0;

		case G_SEND_GAMESTAT:
			SV_MasterGameStat( VMA( 1 ) );
			return 0;

		case G_ADDCOMMAND:
			Cmd_AddCommand( VMA( 1 ), NULL );
			return 0;

		case G_REMOVECOMMAND:
			Cmd_RemoveCommand( VMA( 1 ) );
			return 0;

		case G_GETTAG:
			return SV_GetTag( args[ 1 ], args[ 2 ], VMA( 3 ), VMA( 4 ) );

		case G_REGISTERTAG:
			return SV_LoadTag( VMA( 1 ) );

			// START    xkan, 10/28/2002
		case G_REGISTERSOUND:
#ifdef DOOMSOUND ///// (SA) DOOMSOUND
			return S_RegisterSound( VMA( 1 ) );
#else
			return S_RegisterSound( VMA( 1 ), args[ 2 ] );
#endif ///// (SA) DOOMSOUND

		case G_GET_SOUND_LENGTH:
			return S_GetSoundLength( args[ 1 ] );

		case G_PARSE_ADD_GLOBAL_DEFINE:
			return Parse_AddGlobalDefine( VMA( 1 ) );

		case G_PARSE_LOAD_SOURCE:
			return Parse_LoadSourceHandle( VMA( 1 ) );

		case G_PARSE_FREE_SOURCE:
			return Parse_FreeSourceHandle( args[ 1 ] );

		case G_PARSE_READ_TOKEN:
			return Parse_ReadTokenHandle( args[ 1 ], VMA( 2 ) );

		case G_PARSE_SOURCE_FILE_AND_LINE:
			return Parse_SourceFileAndLine( args[ 1 ], VMA( 2 ), VMA( 3 ) );

// ===

		case G_ADD_PHYSICS_ENTITY:
#ifdef USE_PHYSICS
			CMod_PhysicsAddEntity( VMA( 1 ) );
#endif
			return 0;

		case G_ADD_PHYSICS_STATIC:
#ifdef USE_PHYSICS
			CMod_PhysicsAddStatic( VMA( 1 ) );
#endif
			return 0;

		case G_SENDMESSAGE:
		        VM_CheckBlock( args[2], args[3], "SENDM" );
			SV_SendBinaryMessage( args[ 1 ], VMA( 2 ), args[ 3 ] );
			return 0;

		case G_MESSAGESTATUS:
			return SV_BinaryMessageStatus( args[ 1 ] );

		case G_RSA_GENMSG:
			return SV_RSAGenMsg( VMA( 1 ), VMA( 2 ), VMA( 3 ) );

                case G_QUOTESTRING:
			Cmd_QuoteStringBuffer( VMA( 1 ), VMA( 2 ), args[ 3 ] );
			return 0;

		default:
			Com_Error( ERR_DROP, "Bad game system trap: %ld", ( long int ) args[ 0 ] );
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

	VM_Call( gvm, GAME_SHUTDOWN, qfalse );
	VM_Free( gvm );
	gvm = NULL;

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
	VM_Call( gvm, GAME_INIT, svs.time, Com_Milliseconds(), restart );
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

	VM_Call( gvm, GAME_SHUTDOWN, qtrue );

#ifdef USE_PHYSICS
	CMod_PhysicsClearBodies();
#endif

	// do a restart instead of a free
	gvm = VM_Restart( gvm );

	if ( !gvm )
	{
		// bk001212 - as done below
		Com_Error( ERR_FATAL, "VM_Restart on game failed" );
	}

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

	// load the dll or bytecode
	gvm = VM_Create( "qagame", SV_GameSystemCalls, Cvar_VariableValue( "vm_game" ) );

	if ( !gvm )
	{
		Com_Error( ERR_FATAL, "VM_Create on game failed" );
	}

	SV_InitGameVM( qfalse );
}

/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand( void )
{
	if ( sv.state != SS_GAME )
	{
		return qfalse;
	}

	return VM_Call( gvm, GAME_CONSOLE_COMMAND );
}

/*
====================
SV_GameIsSinglePlayer
====================
*/
qboolean SV_GameIsSinglePlayer( void )
{
	return ( com_gameInfo.spGameTypes & ( 1 << g_gameType->integer ) );
}

/*
====================
SV_GameIsCoop

        This is a modified SinglePlayer, no savegame capability for example
====================
*/
qboolean SV_GameIsCoop( void )
{
	return ( com_gameInfo.coopGameTypes & ( 1 << g_gameType->integer ) );
}

/*
====================
SV_GetTag

  return qfalse if unable to retrieve tag information for this client
====================
*/
extern qboolean CL_GetTag( int clientNum, const char *tagname, orientation_t * or );

qboolean SV_GetTag( int clientNum, int tagFileNumber, const char *tagname, orientation_t * or )
{
	int i;

	if ( tagFileNumber > 0 && tagFileNumber <= sv.num_tagheaders )
	{
		for ( i = sv.tagHeadersExt[ tagFileNumber - 1 ].start;
		      i < sv.tagHeadersExt[ tagFileNumber - 1 ].start + sv.tagHeadersExt[ tagFileNumber - 1 ].count; i++ )
		{
			if ( !Q_stricmp( sv.tags[ i ].name, tagname ) )
			{
				VectorCopy( sv.tags[ i ].origin, or ->origin );
				VectorCopy( sv.tags[ i ].axis[ 0 ], or ->axis[ 0 ] );
				VectorCopy( sv.tags[ i ].axis[ 1 ], or ->axis[ 1 ] );
				VectorCopy( sv.tags[ i ].axis[ 2 ], or ->axis[ 2 ] );
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

	return CL_GetTag( clientNum, tagname, or );
#else
	return qfalse;
#endif
}
