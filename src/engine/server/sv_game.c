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

#include "../botlib/botlib.h"

botlib_export_t *botlib_export;

void CMod_PhysicsAddEntity(sharedEntity_t * gEnt);
void CMod_PhysicsAddStatic(const sharedEntity_t * gEnt);

void SV_GameError(const char *string)
{
	Com_Error(ERR_DROP, "%s", string);
}

void SV_GamePrint(const char *string)
{
	Com_Printf("%s", string);
}

// these functions must be used instead of pointer arithmetic, because
// the game allocates gentities with private information after the server shared part
int SV_NumForGentity(sharedEntity_t * ent)
{
	int             num;

	num = ((byte *) ent - (byte *) sv.gentities) / sv.gentitySize;

	return num;
}

sharedEntity_t *SV_GentityNum(int num)
{
	sharedEntity_t *ent;

	ent = (sharedEntity_t *) ((byte *) sv.gentities + sv.gentitySize * (num));

	return ent;
}

playerState_t  *SV_GameClientNum(int num)
{
	playerState_t  *ps;

	ps = (playerState_t *) ((byte *) sv.gameClients + sv.gameClientSize * (num));

	return ps;
}

svEntity_t     *SV_SvEntityForGentity(sharedEntity_t * gEnt)
{
	if(!gEnt || gEnt->s.number < 0 || gEnt->s.number >= MAX_GENTITIES)
	{
		Com_Error(ERR_DROP, "SV_SvEntityForGentity: bad gEnt");
	}
	return &sv.svEntities[gEnt->s.number];
}

sharedEntity_t *SV_GEntityForSvEntity(svEntity_t * svEnt)
{
	int             num;

	num = svEnt - sv.svEntities;
	return SV_GentityNum(num);
}

/*
===============
SV_GameSendServerCommand

Sends a command string to a client
===============
*/
void SV_GameSendServerCommand(int clientNum, const char *text)
{
	if(clientNum == -1)
	{
		SV_SendServerCommand(NULL, "%s", text);
	}
	else
	{
		if(clientNum < 0 || clientNum >= sv_maxclients->integer)
		{
			return;
		}
		SV_SendServerCommand(svs.clients + clientNum, "%s", text);
	}
}


/*
===============
SV_GameDropClient

Disconnects the client with a message
===============
*/
void SV_GameDropClient(int clientNum, const char *reason, int length)
{
	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		return;
	}
	SV_DropClient(svs.clients + clientNum, reason);
	if(length)
	{
		SV_TempBanNetAddress(svs.clients[clientNum].netchan.remoteAddress, length);
	}
}

/*
===============
SV_RSAGenMsg

Generate an encrypted RSA message
===============
*/
int SV_RSAGenMsg( const char *pubkey, char *cleartext, char *encrypted ) {
#ifdef USE_CRYPTO
	struct rsa_public_key public_key;
	mpz_t message;
	unsigned char buffer[ RSA_KEY_LENGTH / 8 - 11 ];
	int retval;
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
void SV_SetBrushModel(sharedEntity_t * ent, const char *name)
{
	clipHandle_t    h;
	vec3_t          mins, maxs;

	if(!name)
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: NULL");
	}

	if(name[0] != '*')
	{
		Com_Error(ERR_DROP, "SV_SetBrushModel: %s isn't a brush model", name);
	}


	ent->s.modelindex = atoi(name + 1);

	h = CM_InlineModel(ent->s.modelindex);
	CM_ModelBounds(h, mins, maxs);
	VectorCopy(mins, ent->r.mins);
	VectorCopy(maxs, ent->r.maxs);
	ent->r.bmodel = qtrue;

	ent->r.contents = -1;		// we don't know exactly what is in the brushes

	SV_LinkEntity(ent);			// FIXME: remove
}



/*
=================
SV_inPVS

Also checks portalareas so that doors block sight
=================
*/
qboolean SV_inPVS(const vec3_t p1, const vec3_t p2)
{
	int             leafnum;
	int             cluster;
	int             area1, area2;
	byte           *mask;

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
	area1 = CM_LeafArea(leafnum);
	mask = CM_ClusterPVS(cluster);

	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
	area2 = CM_LeafArea(leafnum);
	if(mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
	{
		return qfalse;
	}
	if(!CM_AreasConnected(area1, area2))
	{
		return qfalse;			// a door blocks sight
	}
	return qtrue;
}


/*
=================
SV_inPVSIgnorePortals

Does NOT check portalareas
=================
*/
qboolean SV_inPVSIgnorePortals(const vec3_t p1, const vec3_t p2)
{
	int             leafnum;
	int             cluster;
//	int             area1, area2; //unused
	byte           *mask;

	leafnum = CM_PointLeafnum(p1);
	cluster = CM_LeafCluster(leafnum);
//	area1 = CM_LeafArea(leafnum); //Doesn't modify anything.

	mask = CM_ClusterPVS(cluster);
	leafnum = CM_PointLeafnum(p2);
	cluster = CM_LeafCluster(leafnum);
//	area2 = CM_LeafArea(leafnum); //Doesn't modify anything.

	if(mask && (!(mask[cluster >> 3] & (1 << (cluster & 7)))))
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
void SV_AdjustAreaPortalState(sharedEntity_t * ent, qboolean open)
{
	svEntity_t     *svEnt;

	svEnt = SV_SvEntityForGentity(ent);
	if(svEnt->areanum2 == -1)
	{
		return;
	}
	CM_AdjustAreaPortalState(svEnt->areanum, svEnt->areanum2, open);
}


/*
==================
SV_GameAreaEntities
==================
*/
qboolean SV_EntityContact(vec3_t mins, vec3_t maxs, const sharedEntity_t * gEnt, traceType_t type)
{
	const float    *origin, *angles;
	clipHandle_t    ch;
	trace_t         trace;

	// check for exact collision
	origin = gEnt->r.currentOrigin;
	angles = gEnt->r.currentAngles;

	ch = SV_ClipHandleForEntity(gEnt);
	CM_TransformedBoxTrace(&trace, vec3_origin, vec3_origin, mins, maxs, ch, -1, origin, angles, type);

	return trace.startsolid;
}


/*
===============
SV_GetServerinfo

===============
*/
void SV_GetServerinfo(char *buffer, int bufferSize)
{
	if(bufferSize < 1)
	{
		Com_Error(ERR_DROP, "SV_GetServerinfo: bufferSize == %i", bufferSize);
	}
	Q_strncpyz(buffer, Cvar_InfoString(CVAR_SERVERINFO | CVAR_SERVERINFO_NOUPDATE), bufferSize);
}

/*
===============
SV_LocateGameData

===============
*/
void SV_LocateGameData(sharedEntity_t * gEnts, int numGEntities, int sizeofGEntity_t,
					   playerState_t * clients, int sizeofGameClient)
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
void SV_GetUsercmd(int clientNum, usercmd_t * cmd)
{
	if(clientNum < 0 || clientNum >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_GetUsercmd: bad clientNum:%i", clientNum);
	}
	*cmd = svs.clients[clientNum].lastUsercmd;
}

/*
====================
SV_SendBinaryMessage
====================
*/
static void SV_SendBinaryMessage(int cno, char *buf, int buflen)
{
	if(cno < 0 || cno >= sv_maxclients->integer)
	{
		Com_Error(ERR_DROP, "SV_SendBinaryMessage: bad client %i", cno);
		return;
	}

	if(buflen < 0 || buflen > MAX_BINARY_MESSAGE)
	{
		Com_Error(ERR_DROP, "SV_SendBinaryMessage: bad length %i", buflen);
		svs.clients[cno].binaryMessageLength = 0;
		return;
	}

	svs.clients[cno].binaryMessageLength = buflen;
	memcpy(svs.clients[cno].binaryMessage, buf, buflen);
}

/*
====================
SV_BinaryMessageStatus
====================
*/
static int SV_BinaryMessageStatus(int cno)
{
	if(cno < 0 || cno >= sv_maxclients->integer)
	{
		return qfalse;
	}

	if(svs.clients[cno].binaryMessageLength == 0)
	{
		return MESSAGE_EMPTY;
	}

	if(svs.clients[cno].binaryMessageOverflowed)
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
void SV_GameBinaryMessageReceived(int cno, const char *buf, int buflen, int commandTime)
{
	VM_Call(gvm, GAME_MESSAGERECEIVED, cno, buf, buflen, commandTime);
}

// taken from cl_main.c
#define MAX_RCON_MESSAGE 1024

/*
===============
SV_UpdateSharedConfig
===============
*/

void SV_UpdateSharedConfig( unsigned int port, const char *rconpass ) {
	char 		message[MAX_RCON_MESSAGE];
	netadr_t	to;

	message[0] = -1;
	message[1] = -1;
	message[2] = -1;
	message[3] = -1;
	message[4] = 0;

	Q_strcat (message, MAX_RCON_MESSAGE, "rcon ");

	Q_strcat (message, MAX_RCON_MESSAGE, rconpass);
	Q_strcat (message, MAX_RCON_MESSAGE, " !readconfig");
	NET_StringToAdr ("127.0.0.1", &to, NA_UNSPEC);
	to.port = BigShort (port);

	NET_SendPacket (NS_SERVER, strlen(message)+1, message, to);
}


//==============================================

static int FloatAsInt(float f)
{
	floatint_t      fi;

	fi.f = f;
	return fi.i;
}

extern int S_RegisterSound( const char *name, qboolean compressed );
extern int S_GetSoundLength( sfxHandle_t sfxHandle );

/*
====================
SV_GameSystemCalls

The module is making a system call
====================
*/
intptr_t SV_GameSystemCalls(intptr_t * args) {
	switch (args[0]) {
		case G_PRINT:
			Com_Printf("%s", (char *)VMA(1));
			return 0;
		case G_ERROR:
			Com_Error(ERR_DROP, "%s", (char *)VMA(1));
			return 0;
		case G_MILLISECONDS:
			return Sys_Milliseconds();
		case G_CVAR_REGISTER:
			Cvar_Register(VMA(1), VMA(2), VMA(3), args[4]);
			return 0;
		case G_CVAR_UPDATE:
			Cvar_Update(VMA(1));
			return 0;
		case G_CVAR_SET:
			Cvar_Set((const char *)VMA(1), (const char *)VMA(2));
			return 0;
		case G_CVAR_VARIABLE_INTEGER_VALUE:
			return Cvar_VariableIntegerValue((const char *)VMA(1));
		case G_CVAR_VARIABLE_STRING_BUFFER:
			Cvar_VariableStringBuffer(VMA(1), VMA(2), args[3]);
			return 0;
		case G_CVAR_LATCHEDVARIABLESTRINGBUFFER:
			Cvar_LatchedVariableStringBuffer(VMA(1), VMA(2), args[3]);
			return 0;
		case G_ARGC:
			return Cmd_Argc();
		case G_ARGV:
			Cmd_ArgvBuffer(args[1], VMA(2), args[3]);
			return 0;
		case G_SEND_CONSOLE_COMMAND:
			Cbuf_ExecuteText(args[1], VMA(2));
			return 0;

		case G_FS_FOPEN_FILE:
			return FS_FOpenFileByMode(VMA(1), VMA(2), args[3]);
		case G_FS_READ:
			FS_Read2(VMA(1), args[2], args[3]);
			return 0;
		case G_FS_WRITE:
			return FS_Write(VMA(1), args[2], args[3]);
		case G_FS_RENAME:
			FS_Rename(VMA(1), VMA(2));
			return 0;
		case G_FS_FCLOSE_FILE:
			FS_FCloseFile(args[1]);
			return 0;
		case G_FS_GETFILELIST:
			return FS_GetFileList(VMA(1), VMA(2), VMA(3), args[4]);

		case G_LOCATE_GAME_DATA:
			SV_LocateGameData(VMA(1), args[2], args[3], VMA(4), args[5]);
			return 0;
		case G_DROP_CLIENT:
			SV_GameDropClient(args[1], VMA(2), args[3]);
			return 0;
		case G_SEND_SERVER_COMMAND:
			SV_GameSendServerCommand(args[1], VMA(2));
			return 0;
		case G_LINKENTITY:
			SV_LinkEntity(VMA(1));
			return 0;
		case G_UNLINKENTITY:
			SV_UnlinkEntity(VMA(1));
			return 0;
		case G_ENTITIES_IN_BOX:
			return SV_AreaEntities(VMA(1), VMA(2), VMA(3), args[4]);
		case G_ENTITY_CONTACT:
			return SV_EntityContact(VMA(1), VMA(2), VMA(3), TT_AABB);
		case G_ENTITY_CONTACTCAPSULE:
			return SV_EntityContact(VMA(1), VMA(2), VMA(3), TT_CAPSULE);
		case G_TRACE:
			SV_Trace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], TT_AABB);
			return 0;
		case G_TRACECAPSULE:
			SV_Trace(VMA(1), VMA(2), VMA(3), VMA(4), VMA(5), args[6], args[7], TT_CAPSULE);
			return 0;
		case G_POINT_CONTENTS:
			return SV_PointContents(VMA(1), args[2]);
		case G_SET_BRUSH_MODEL:
			SV_SetBrushModel(VMA(1), VMA(2));
			return 0;
		case G_IN_PVS:
			return SV_inPVS(VMA(1), VMA(2));
		case G_IN_PVS_IGNORE_PORTALS:
			return SV_inPVSIgnorePortals(VMA(1), VMA(2));

		case G_SET_CONFIGSTRING:
			SV_SetConfigstring(args[1], VMA(2));
			return 0;
		case G_GET_CONFIGSTRING:
			SV_GetConfigstring(args[1], VMA(2), args[3]);
			return 0;
		// Dushan - Tremulous
		case G_SET_CONFIGSTRING_RESTRICTIONS:
			// Dushan: FIX ME
			//SV_SetConfigstringRestrictions( args[1], VMA(2) );
			return 0;
		case G_SET_USERINFO:
			SV_SetUserinfo(args[1], VMA(2));
			return 0;
		case G_GET_USERINFO:
			SV_GetUserinfo(args[1], VMA(2), args[3]);
			return 0;
		case G_GET_SERVERINFO:
			SV_GetServerinfo(VMA(1), args[2]);
			return 0;
		case G_ADJUST_AREA_PORTAL_STATE:
			SV_AdjustAreaPortalState(VMA(1), args[2]);
			return 0;
		case G_AREAS_CONNECTED:
			return CM_AreasConnected(args[1], args[2]);

		case G_UPDATE_SHARED_CONFIG:
			SV_UpdateSharedConfig( args[1], VMA(2) );
			return 0;

		case G_BOT_ALLOCATE_CLIENT:
			return SV_BotAllocateClient(args[1]);
		case G_BOT_FREE_CLIENT:
			SV_BotFreeClient(args[1]);
			return 0;

		case G_GET_USERCMD:
			SV_GetUsercmd(args[1], VMA(2));
			return 0;
		case G_GET_ENTITY_TOKEN:
		{
			const char     *s;

			s = COM_Parse(&sv.entityParsePoint);
			Q_strncpyz(VMA(1), s, args[2]);
			if(!sv.entityParsePoint && !s[0])
			{
				return qfalse;
			}
			else
			{
				return qtrue;
			}
		}

		case G_DEBUG_POLYGON_CREATE:
			return BotImport_DebugPolygonCreate(args[1], args[2], VMA(3));
		case G_DEBUG_POLYGON_DELETE:
			BotImport_DebugPolygonDelete(args[1]);
			return 0;
		case G_REAL_TIME:
			return Com_RealTime(VMA(1));
		case G_SNAPVECTOR:
			Q_SnapVector(VMA(1));
			return 0;

		// Dushan - Tremulous
		case G_SEND_GAMESTAT:
			SV_MasterGameStat( VMA(1) );
			return 0;

		case G_ADDCOMMAND:
			Cmd_AddCommand( VMA(1), NULL );
			return 0;
		case G_REMOVECOMMAND:
			Cmd_RemoveCommand( VMA(1) );
			return 0;

		case G_GETTAG:
			return SV_GetTag(args[1], args[2], VMA(3), VMA(4));

		case G_REGISTERTAG:
			return SV_LoadTag(VMA(1));

			// START    xkan, 10/28/2002
		case G_REGISTERSOUND:
#ifdef DOOMSOUND				///// (SA) DOOMSOUND
			return S_RegisterSound(VMA(1));
#else
			return S_RegisterSound(VMA(1), args[2]);
#endif							///// (SA) DOOMSOUND
		case G_GET_SOUND_LENGTH:
			return S_GetSoundLength(args[1]);

		// Dushan - Tremulous
		case G_PARSE_ADD_GLOBAL_DEFINE:
			return Parse_AddGlobalDefine( VMA(1) );
		case G_PARSE_LOAD_SOURCE:
			return Parse_LoadSourceHandle( VMA(1) );
		case G_PARSE_FREE_SOURCE:
			return Parse_FreeSourceHandle( args[1] );
		case G_PARSE_READ_TOKEN:
			return Parse_ReadTokenHandle( args[1], VMA(2) );
		case G_PARSE_SOURCE_FILE_AND_LINE:
			return Parse_SourceFileAndLine( args[1], VMA(2), VMA(3) );

			//====================================

		case BOTLIB_SETUP:
			return SV_BotLibSetup();
		case BOTLIB_SHUTDOWN:
			return SV_BotLibShutdown();
		case BOTLIB_LIBVAR_SET:
			return botlib_export->BotLibVarSet(VMA(1), VMA(2));
		case BOTLIB_LIBVAR_GET:
			return botlib_export->BotLibVarGet(VMA(1), VMA(2), args[3]);

		case BOTLIB_PC_ADD_GLOBAL_DEFINE:
			return botlib_export->PC_AddGlobalDefine(VMA(1));
		case BOTLIB_PC_LOAD_SOURCE:
			return botlib_export->PC_LoadSourceHandle(VMA(1));
		case BOTLIB_PC_FREE_SOURCE:
			return botlib_export->PC_FreeSourceHandle(args[1]);
		case BOTLIB_PC_READ_TOKEN:
			return botlib_export->PC_ReadTokenHandle(args[1], VMA(2));
		case BOTLIB_PC_SOURCE_FILE_AND_LINE:
			return botlib_export->PC_SourceFileAndLine(args[1], VMA(2), VMA(3));
		case BOTLIB_PC_UNREAD_TOKEN:
			botlib_export->PC_UnreadLastTokenHandle(args[1]);
			return 0;

		case BOTLIB_START_FRAME:
			return botlib_export->BotLibStartFrame(VMF(1));
		case BOTLIB_LOAD_MAP:
			return botlib_export->BotLibLoadMap(VMA(1));
		case BOTLIB_UPDATENTITY:
			return botlib_export->BotLibUpdateEntity(args[1], VMA(2));
		case BOTLIB_TEST:
			return botlib_export->Test(args[1], VMA(2), VMA(3), VMA(4));

		case BOTLIB_GET_SNAPSHOT_ENTITY:
			return SV_BotGetSnapshotEntity(args[1], args[2]);
		case BOTLIB_GET_CONSOLE_MESSAGE:
			return SV_BotGetConsoleMessage(args[1], VMA(2), args[3]);
		case BOTLIB_USER_COMMAND:
			SV_ClientThink(&svs.clients[args[1]], VMA(2));
			return 0;

		case BOTLIB_AAS_ENTITY_INFO:
			botlib_export->aas.AAS_EntityInfo(args[1], VMA(2));
			return 0;

		case BOTLIB_AAS_INITIALIZED:
			return botlib_export->aas.AAS_Initialized();
		case BOTLIB_AAS_PRESENCE_TYPE_BOUNDING_BOX:
			botlib_export->aas.AAS_PresenceTypeBoundingBox(args[1], VMA(2), VMA(3));
			return 0;
		case BOTLIB_AAS_TIME:
			return FloatAsInt(botlib_export->aas.AAS_Time());

			// Ridah
		case BOTLIB_AAS_SETCURRENTWORLD:
			botlib_export->aas.AAS_SetCurrentWorld(args[1]);
			return 0;
			// done.

		case BOTLIB_AAS_POINT_AREA_NUM:
			return botlib_export->aas.AAS_PointAreaNum(VMA(1));
		case BOTLIB_AAS_TRACE_AREAS:
			return botlib_export->aas.AAS_TraceAreas(VMA(1), VMA(2), VMA(3), VMA(4), args[5]);
		case BOTLIB_AAS_BBOX_AREAS:
			return botlib_export->aas.AAS_BBoxAreas(VMA(1), VMA(2), VMA(3), args[4]);
		case BOTLIB_AAS_AREA_CENTER:
			botlib_export->aas.AAS_AreaCenter(args[1], VMA(2));
			return 0;
		case BOTLIB_AAS_AREA_WAYPOINT:
			return botlib_export->aas.AAS_AreaWaypoint(args[1], VMA(2));

		case BOTLIB_AAS_POINT_CONTENTS:
			return botlib_export->aas.AAS_PointContents(VMA(1));
		case BOTLIB_AAS_NEXT_BSP_ENTITY:
			return botlib_export->aas.AAS_NextBSPEntity(args[1]);
		case BOTLIB_AAS_VALUE_FOR_BSP_EPAIR_KEY:
			return botlib_export->aas.AAS_ValueForBSPEpairKey(args[1], VMA(2), VMA(3), args[4]);
		case BOTLIB_AAS_VECTOR_FOR_BSP_EPAIR_KEY:
			return botlib_export->aas.AAS_VectorForBSPEpairKey(args[1], VMA(2), VMA(3));
		case BOTLIB_AAS_FLOAT_FOR_BSP_EPAIR_KEY:
			return botlib_export->aas.AAS_FloatForBSPEpairKey(args[1], VMA(2), VMA(3));
		case BOTLIB_AAS_INT_FOR_BSP_EPAIR_KEY:
			return botlib_export->aas.AAS_IntForBSPEpairKey(args[1], VMA(2), VMA(3));

		case BOTLIB_AAS_AREA_REACHABILITY:
			return botlib_export->aas.AAS_AreaReachability(args[1]);
		case BOTLIB_AAS_AREA_LADDER:
			return botlib_export->aas.AAS_AreaLadder(args[1]);

		case BOTLIB_AAS_AREA_TRAVEL_TIME_TO_GOAL_AREA:
			return botlib_export->aas.AAS_AreaTravelTimeToGoalArea(args[1], VMA(2), args[3], args[4]);

		case BOTLIB_AAS_SWIMMING:
			return botlib_export->aas.AAS_Swimming(VMA(1));
		case BOTLIB_AAS_PREDICT_CLIENT_MOVEMENT:
			return botlib_export->aas.AAS_PredictClientMovement(VMA(1), args[2], VMA(3), args[4], args[5],
																VMA(6), VMA(7), args[8], args[9], VMF(10), args[11], args[12],
																args[13]);

			// Ridah, route-tables
		case BOTLIB_AAS_RT_SHOWROUTE:
			botlib_export->aas.AAS_RT_ShowRoute(VMA(1), args[2], args[3]);
			return 0;

			//case BOTLIB_AAS_RT_GETHIDEPOS:
			//  return botlib_export->aas.AAS_RT_GetHidePos( VMA(1), args[2], args[3], VMA(4), args[5], args[6], VMA(7) );

			//case BOTLIB_AAS_FINDATTACKSPOTWITHINRANGE:
			//  return botlib_export->aas.AAS_FindAttackSpotWithinRange( args[1], args[2], args[3], VMF(4), args[5], VMA(6) );

		case BOTLIB_AAS_NEARESTHIDEAREA:
			return botlib_export->aas.AAS_NearestHideArea(args[1], VMA(2), args[3], args[4], VMA(5), args[6], args[7], VMF(8),
														  VMA(9));

		case BOTLIB_AAS_LISTAREASINRANGE:
			return botlib_export->aas.AAS_ListAreasInRange(VMA(1), args[2], VMF(3), args[4], VMA(5), args[6]);

		case BOTLIB_AAS_AVOIDDANGERAREA:
			return botlib_export->aas.AAS_AvoidDangerArea(VMA(1), args[2], VMA(3), args[4], VMF(5), args[6]);

		case BOTLIB_AAS_RETREAT:
			return botlib_export->aas.AAS_Retreat(VMA(1), args[2], VMA(3), args[4], VMA(5), args[6], VMF(7), VMF(8), args[9]);

		case BOTLIB_AAS_ALTROUTEGOALS:
			return botlib_export->aas.AAS_AlternativeRouteGoals(VMA(1), VMA(2), args[3], VMA(4), args[5], args[6]);

		case BOTLIB_AAS_SETAASBLOCKINGENTITY:
			botlib_export->aas.AAS_SetAASBlockingEntity(VMA(1), VMA(2), args[3]);
			return 0;

		case BOTLIB_AAS_RECORDTEAMDEATHAREA:
			botlib_export->aas.AAS_RecordTeamDeathArea(VMA(1), args[2], args[3], args[4], args[5]);
			return 0;
			// done.

		case BOTLIB_EA_SAY:
			botlib_export->ea.EA_Say(args[1], VMA(2));
			return 0;
		case BOTLIB_EA_SAY_TEAM:
			botlib_export->ea.EA_SayTeam(args[1], VMA(2));
			return 0;
		case BOTLIB_EA_USE_ITEM:
			botlib_export->ea.EA_UseItem(args[1], VMA(2));
			return 0;
		case BOTLIB_EA_DROP_ITEM:
			botlib_export->ea.EA_DropItem(args[1], VMA(2));
			return 0;
		case BOTLIB_EA_USE_INV:
			botlib_export->ea.EA_UseInv(args[1], VMA(2));
			return 0;
		case BOTLIB_EA_DROP_INV:
			botlib_export->ea.EA_DropInv(args[1], VMA(2));
			return 0;
		case BOTLIB_EA_GESTURE:
			botlib_export->ea.EA_Gesture(args[1]);
			return 0;
		case BOTLIB_EA_COMMAND:
			botlib_export->ea.EA_Command(args[1], VMA(2));
			return 0;

		case BOTLIB_EA_SELECT_WEAPON:
			botlib_export->ea.EA_SelectWeapon(args[1], args[2]);
			return 0;
		case BOTLIB_EA_TALK:
			botlib_export->ea.EA_Talk(args[1]);
			return 0;
		case BOTLIB_EA_ATTACK:
			botlib_export->ea.EA_Attack(args[1]);
			return 0;
		case BOTLIB_EA_RELOAD:
			botlib_export->ea.EA_Reload(args[1]);
			return 0;
		case BOTLIB_EA_USE:
			botlib_export->ea.EA_Use(args[1]);
			return 0;
		case BOTLIB_EA_RESPAWN:
			botlib_export->ea.EA_Respawn(args[1]);
			return 0;
		case BOTLIB_EA_JUMP:
			botlib_export->ea.EA_Jump(args[1]);
			return 0;
		case BOTLIB_EA_DELAYED_JUMP:
			botlib_export->ea.EA_DelayedJump(args[1]);
			return 0;
		case BOTLIB_EA_CROUCH:
			botlib_export->ea.EA_Crouch(args[1]);
			return 0;
		case BOTLIB_EA_WALK:
			botlib_export->ea.EA_Walk(args[1]);
			return 0;
		case BOTLIB_EA_MOVE_UP:
			botlib_export->ea.EA_MoveUp(args[1]);
			return 0;
		case BOTLIB_EA_MOVE_DOWN:
			botlib_export->ea.EA_MoveDown(args[1]);
			return 0;
		case BOTLIB_EA_MOVE_FORWARD:
			botlib_export->ea.EA_MoveForward(args[1]);
			return 0;
		case BOTLIB_EA_MOVE_BACK:
			botlib_export->ea.EA_MoveBack(args[1]);
			return 0;
		case BOTLIB_EA_MOVE_LEFT:
			botlib_export->ea.EA_MoveLeft(args[1]);
			return 0;
		case BOTLIB_EA_MOVE_RIGHT:
			botlib_export->ea.EA_MoveRight(args[1]);
			return 0;
		case BOTLIB_EA_MOVE:
			botlib_export->ea.EA_Move(args[1], VMA(2), VMF(3));
			return 0;
		case BOTLIB_EA_VIEW:
			botlib_export->ea.EA_View(args[1], VMA(2));
			return 0;
		case BOTLIB_EA_PRONE:
			botlib_export->ea.EA_Prone(args[1]);
			return 0;

		case BOTLIB_EA_END_REGULAR:
			botlib_export->ea.EA_EndRegular(args[1], VMF(2));
			return 0;
		case BOTLIB_EA_GET_INPUT:
			botlib_export->ea.EA_GetInput(args[1], VMF(2), VMA(3));
			return 0;
		case BOTLIB_EA_RESET_INPUT:
			botlib_export->ea.EA_ResetInput(args[1], VMA(2));
			return 0;

		case BOTLIB_AI_LOAD_CHARACTER:
			return botlib_export->ai.BotLoadCharacter(VMA(1), args[2]);
		case BOTLIB_AI_FREE_CHARACTER:
			botlib_export->ai.BotFreeCharacter(args[1]);
			return 0;
		case BOTLIB_AI_CHARACTERISTIC_FLOAT:
			return FloatAsInt(botlib_export->ai.Characteristic_Float(args[1], args[2]));
		case BOTLIB_AI_CHARACTERISTIC_BFLOAT:
			return FloatAsInt(botlib_export->ai.Characteristic_BFloat(args[1], args[2], VMF(3), VMF(4)));
		case BOTLIB_AI_CHARACTERISTIC_INTEGER:
			return botlib_export->ai.Characteristic_Integer(args[1], args[2]);
		case BOTLIB_AI_CHARACTERISTIC_BINTEGER:
			return botlib_export->ai.Characteristic_BInteger(args[1], args[2], args[3], args[4]);
		case BOTLIB_AI_CHARACTERISTIC_STRING:
			botlib_export->ai.Characteristic_String(args[1], args[2], VMA(3), args[4]);
			return 0;

		case BOTLIB_AI_ALLOC_CHAT_STATE:
			return botlib_export->ai.BotAllocChatState();
		case BOTLIB_AI_FREE_CHAT_STATE:
			botlib_export->ai.BotFreeChatState(args[1]);
			return 0;
		case BOTLIB_AI_QUEUE_CONSOLE_MESSAGE:
			botlib_export->ai.BotQueueConsoleMessage(args[1], args[2], VMA(3));
			return 0;
		case BOTLIB_AI_REMOVE_CONSOLE_MESSAGE:
			botlib_export->ai.BotRemoveConsoleMessage(args[1], args[2]);
			return 0;
		case BOTLIB_AI_NEXT_CONSOLE_MESSAGE:
			return botlib_export->ai.BotNextConsoleMessage(args[1], VMA(2));
		case BOTLIB_AI_NUM_CONSOLE_MESSAGE:
			return botlib_export->ai.BotNumConsoleMessages(args[1]);
		case BOTLIB_AI_INITIAL_CHAT:
			botlib_export->ai.BotInitialChat(args[1], VMA(2), args[3], VMA(4), VMA(5), VMA(6), VMA(7), VMA(8), VMA(9), VMA(10),
											 VMA(11));
			return 0;
		case BOTLIB_AI_NUM_INITIAL_CHATS:
			return botlib_export->ai.BotNumInitialChats(args[1], VMA(2));
		case BOTLIB_AI_REPLY_CHAT:
			return botlib_export->ai.BotReplyChat(args[1], VMA(2), args[3], args[4], VMA(5), VMA(6), VMA(7), VMA(8), VMA(9),
												  VMA(10), VMA(11), VMA(12));
		case BOTLIB_AI_CHAT_LENGTH:
			return botlib_export->ai.BotChatLength(args[1]);
		case BOTLIB_AI_ENTER_CHAT:
			botlib_export->ai.BotEnterChat(args[1], args[2], args[3]);
			return 0;
		case BOTLIB_AI_GET_CHAT_MESSAGE:
			botlib_export->ai.BotGetChatMessage(args[1], VMA(2), args[3]);
			return 0;
		case BOTLIB_AI_STRING_CONTAINS:
			return botlib_export->ai.StringContains(VMA(1), VMA(2), args[3]);
		case BOTLIB_AI_FIND_MATCH:
			return botlib_export->ai.BotFindMatch(VMA(1), VMA(2), args[3]);
		case BOTLIB_AI_MATCH_VARIABLE:
			botlib_export->ai.BotMatchVariable(VMA(1), args[2], VMA(3), args[4]);
			return 0;
		case BOTLIB_AI_UNIFY_WHITE_SPACES:
			botlib_export->ai.UnifyWhiteSpaces(VMA(1));
			return 0;
		case BOTLIB_AI_REPLACE_SYNONYMS:
			botlib_export->ai.BotReplaceSynonyms(VMA(1), args[2]);
			return 0;
		case BOTLIB_AI_LOAD_CHAT_FILE:
			return botlib_export->ai.BotLoadChatFile(args[1], VMA(2), VMA(3));
		case BOTLIB_AI_SET_CHAT_GENDER:
			botlib_export->ai.BotSetChatGender(args[1], args[2]);
			return 0;
		case BOTLIB_AI_SET_CHAT_NAME:
			botlib_export->ai.BotSetChatName(args[1], VMA(2));
			return 0;

		case BOTLIB_AI_RESET_GOAL_STATE:
			botlib_export->ai.BotResetGoalState(args[1]);
			return 0;
		case BOTLIB_AI_RESET_AVOID_GOALS:
			botlib_export->ai.BotResetAvoidGoals(args[1]);
			return 0;
		case BOTLIB_AI_REMOVE_FROM_AVOID_GOALS:
			botlib_export->ai.BotRemoveFromAvoidGoals(args[1], args[2]);
			return 0;
		case BOTLIB_AI_PUSH_GOAL:
			botlib_export->ai.BotPushGoal(args[1], VMA(2));
			return 0;
		case BOTLIB_AI_POP_GOAL:
			botlib_export->ai.BotPopGoal(args[1]);
			return 0;
		case BOTLIB_AI_EMPTY_GOAL_STACK:
			botlib_export->ai.BotEmptyGoalStack(args[1]);
			return 0;
		case BOTLIB_AI_DUMP_AVOID_GOALS:
			botlib_export->ai.BotDumpAvoidGoals(args[1]);
			return 0;
		case BOTLIB_AI_DUMP_GOAL_STACK:
			botlib_export->ai.BotDumpGoalStack(args[1]);
			return 0;
		case BOTLIB_AI_GOAL_NAME:
			botlib_export->ai.BotGoalName(args[1], VMA(2), args[3]);
			return 0;
		case BOTLIB_AI_GET_TOP_GOAL:
			return botlib_export->ai.BotGetTopGoal(args[1], VMA(2));
		case BOTLIB_AI_GET_SECOND_GOAL:
			return botlib_export->ai.BotGetSecondGoal(args[1], VMA(2));
		case BOTLIB_AI_CHOOSE_LTG_ITEM:
			return botlib_export->ai.BotChooseLTGItem(args[1], VMA(2), VMA(3), args[4]);
		case BOTLIB_AI_CHOOSE_NBG_ITEM:
			return botlib_export->ai.BotChooseNBGItem(args[1], VMA(2), VMA(3), args[4], VMA(5), VMF(6));
		case BOTLIB_AI_TOUCHING_GOAL:
			return botlib_export->ai.BotTouchingGoal(VMA(1), VMA(2));
		case BOTLIB_AI_ITEM_GOAL_IN_VIS_BUT_NOT_VISIBLE:
			return botlib_export->ai.BotItemGoalInVisButNotVisible(args[1], VMA(2), VMA(3), VMA(4));
		case BOTLIB_AI_GET_LEVEL_ITEM_GOAL:
			return botlib_export->ai.BotGetLevelItemGoal(args[1], VMA(2), VMA(3));
		case BOTLIB_AI_GET_NEXT_CAMP_SPOT_GOAL:
			return botlib_export->ai.BotGetNextCampSpotGoal(args[1], VMA(2));
		case BOTLIB_AI_GET_MAP_LOCATION_GOAL:
			return botlib_export->ai.BotGetMapLocationGoal(VMA(1), VMA(2));
		case BOTLIB_AI_AVOID_GOAL_TIME:
			return FloatAsInt(botlib_export->ai.BotAvoidGoalTime(args[1], args[2]));
		case BOTLIB_AI_INIT_LEVEL_ITEMS:
			botlib_export->ai.BotInitLevelItems();
			return 0;
		case BOTLIB_AI_UPDATE_ENTITY_ITEMS:
			botlib_export->ai.BotUpdateEntityItems();
			return 0;
		case BOTLIB_AI_LOAD_ITEM_WEIGHTS:
			return botlib_export->ai.BotLoadItemWeights(args[1], VMA(2));
		case BOTLIB_AI_FREE_ITEM_WEIGHTS:
			botlib_export->ai.BotFreeItemWeights(args[1]);
			return 0;
		case BOTLIB_AI_INTERBREED_GOAL_FUZZY_LOGIC:
			botlib_export->ai.BotInterbreedGoalFuzzyLogic(args[1], args[2], args[3]);
			return 0;
		case BOTLIB_AI_SAVE_GOAL_FUZZY_LOGIC:
			botlib_export->ai.BotSaveGoalFuzzyLogic(args[1], VMA(2));
			return 0;
		case BOTLIB_AI_MUTATE_GOAL_FUZZY_LOGIC:
			botlib_export->ai.BotMutateGoalFuzzyLogic(args[1], VMF(2));
			return 0;
		case BOTLIB_AI_ALLOC_GOAL_STATE:
			return botlib_export->ai.BotAllocGoalState(args[1]);
		case BOTLIB_AI_FREE_GOAL_STATE:
			botlib_export->ai.BotFreeGoalState(args[1]);
			return 0;

		case BOTLIB_AI_RESET_MOVE_STATE:
			botlib_export->ai.BotResetMoveState(args[1]);
			return 0;
		case BOTLIB_AI_MOVE_TO_GOAL:
			botlib_export->ai.BotMoveToGoal(VMA(1), args[2], VMA(3), args[4]);
			return 0;
		case BOTLIB_AI_MOVE_IN_DIRECTION:
			return botlib_export->ai.BotMoveInDirection(args[1], VMA(2), VMF(3), args[4]);
		case BOTLIB_AI_RESET_AVOID_REACH:
			botlib_export->ai.BotResetAvoidReach(args[1]);
			return 0;
		case BOTLIB_AI_RESET_LAST_AVOID_REACH:
			botlib_export->ai.BotResetLastAvoidReach(args[1]);
			return 0;
		case BOTLIB_AI_REACHABILITY_AREA:
			return botlib_export->ai.BotReachabilityArea(VMA(1), args[2]);
		case BOTLIB_AI_MOVEMENT_VIEW_TARGET:
			return botlib_export->ai.BotMovementViewTarget(args[1], VMA(2), args[3], VMF(4), VMA(5));
		case BOTLIB_AI_PREDICT_VISIBLE_POSITION:
			return botlib_export->ai.BotPredictVisiblePosition(VMA(1), args[2], VMA(3), args[4], VMA(5));
		case BOTLIB_AI_ALLOC_MOVE_STATE:
			return botlib_export->ai.BotAllocMoveState();
		case BOTLIB_AI_FREE_MOVE_STATE:
			botlib_export->ai.BotFreeMoveState(args[1]);
			return 0;
		case BOTLIB_AI_INIT_MOVE_STATE:
			botlib_export->ai.BotInitMoveState(args[1], VMA(2));
			return 0;
			// Ridah
		case BOTLIB_AI_INIT_AVOID_REACH:
			botlib_export->ai.BotInitAvoidReach(args[1]);
			return 0;
			// done.

		case BOTLIB_AI_CHOOSE_BEST_FIGHT_WEAPON:
			return botlib_export->ai.BotChooseBestFightWeapon(args[1], VMA(2));
		case BOTLIB_AI_GET_WEAPON_INFO:
			botlib_export->ai.BotGetWeaponInfo(args[1], args[2], VMA(3));
			return 0;
		case BOTLIB_AI_LOAD_WEAPON_WEIGHTS:
			return botlib_export->ai.BotLoadWeaponWeights(args[1], VMA(2));
		case BOTLIB_AI_ALLOC_WEAPON_STATE:
			return botlib_export->ai.BotAllocWeaponState();
		case BOTLIB_AI_FREE_WEAPON_STATE:
			botlib_export->ai.BotFreeWeaponState(args[1]);
			return 0;
		case BOTLIB_AI_RESET_WEAPON_STATE:
			botlib_export->ai.BotResetWeaponState(args[1]);
			return 0;

		case BOTLIB_AI_GENETIC_PARENTS_AND_CHILD_SELECTION:
			return botlib_export->ai.GeneticParentsAndChildSelection(args[1], VMA(2), VMA(3), VMA(4), VMA(5));

		case G_ADD_PHYSICS_ENTITY:
#ifdef USE_PHYSICS
			CMod_PhysicsAddEntity(VMA(1));
#endif
			return 0;

		case G_ADD_PHYSICS_STATIC:
#ifdef USE_PHYSICS
			CMod_PhysicsAddStatic(VMA(1));
#endif
			return 0;

		case TRAP_MEMSET:
			memset(VMA(1), args[2], args[3]);
			return 0;

		case TRAP_MEMCPY:
			memcpy(VMA(1), VMA(2), args[3]);
			return 0;

		case TRAP_STRNCPY:
			return (intptr_t)strncpy( VMA( 1 ), VMA( 2 ), args[3] );

		case TRAP_SIN:
			return FloatAsInt(sin(VMF(1)));

		case TRAP_COS:
			return FloatAsInt(cos(VMF(1)));

		case TRAP_ATAN2:
			return FloatAsInt(atan2(VMF(1), VMF(2)));

		case TRAP_SQRT:
			return FloatAsInt(sqrt(VMF(1)));

		case TRAP_MATRIXMULTIPLY:
			AxisMultiply(VMA(1), VMA(2), VMA(3));
			return 0;

		case TRAP_ANGLEVECTORS:
			AngleVectors(VMA(1), VMA(2), VMA(3), VMA(4));
			return 0;

		case TRAP_PERPENDICULARVECTOR:
			PerpendicularVector(VMA(1), VMA(2));
			return 0;

		case TRAP_FLOOR:
			return FloatAsInt(floor(VMF(1)));

		case TRAP_CEIL:
			return FloatAsInt(ceil(VMF(1)));

		case G_SENDMESSAGE:
			SV_SendBinaryMessage(args[1], VMA(2), args[3]);
			return 0;
		case G_MESSAGESTATUS:
			return SV_BinaryMessageStatus(args[1]);

#if defined(ET_MYSQL)
		//Dushan
        case G_SQL_RUNQUERY:
                return OW_RunQuery( VMA(1) );

        case G_SQL_FINISHQUERY:
                OW_FinishQuery( args[1] );
                return 0;

        case G_SQL_NEXTROW:
                return OW_NextRow( args[1] );

        case G_SQL_ROWCOUNT:
                return OW_RowCount( args[1] );

        case G_SQL_GETFIELDBYID:
                OW_GetFieldByID( args[1], args[2], VMA(3), args[4]  );
                return 0;

        case G_SQL_GETFIELDBYNAME:
                OW_GetFieldByName( args[1], VMA(2), VMA(3), args[4] );
                return 0;

        case G_SQL_GETFIELDBYID_INT:
                return OW_GetFieldByID_int( args[1], args[2] );

        case G_SQL_GETFIELDBYNAME_INT:
                return OW_GetFieldByName_int( args[1], VMA(2) );

        case G_SQL_FIELDCOUNT:
                return OW_FieldCount( args[1] );

        case G_SQL_CLEANSTRING:
                OW_CleanString( VMA(1), VMA(2), args[3] );
                return 0;
#endif
		case G_RSA_GENMSG:
			return SV_RSAGenMsg( VMA(1), VMA(2), VMA(3) );
		default:
			Com_Error( ERR_DROP, "Bad game system trap: %ld", (long int) args[0] );
	}
	return -1;
}

/*
===============
SV_ShutdownGameProgs

Called every time a map changes
===============
*/
void SV_ShutdownGameProgs(void) {
	if(!gvm) {
		return;
	}

	VM_Call(gvm, GAME_SHUTDOWN, qfalse);
	VM_Free(gvm);
	gvm = NULL;
	if ( sv_newGameShlib->string[0] ) {
		FS_Rename( sv_newGameShlib->string, "game" DLL_EXT);
		Cvar_Set( "sv_newGameShlib", "" );
	}
}

/*
==================
SV_InitGameVM

Called for both a full init and a restart
==================
*/
static void SV_InitGameVM(qboolean restart)
{
	int             i;

	// start the entity parsing at the beginning
	sv.entityParsePoint = CM_EntityString();

	// clear all gentity pointers that might still be set from
	// a previous level
	for(i = 0; i < sv_maxclients->integer; i++)
	{
		svs.clients[i].gentity = NULL;
	}

	// use the current msec count for a random seed
	// init for this gamestate
	VM_Call (gvm, GAME_INIT, svs.time, Com_Milliseconds(), restart);
}



/*
===================
SV_RestartGameProgs

Called on a map_restart, but not on a normal map change
===================
*/
void SV_RestartGameProgs(void)
{
	if(!gvm)
	{
		return;
	}
	VM_Call(gvm, GAME_SHUTDOWN, qtrue);

#ifdef USE_PHYSICS
	CMod_PhysicsClearBodies();
#endif

	// do a restart instead of a free
	gvm = VM_Restart(gvm);
	if(!gvm)
	{							// bk001212 - as done below
		Com_Error(ERR_FATAL, "VM_Restart on game failed");
	}

	SV_InitGameVM(qtrue);
}


/*
===============
SV_InitGameProgs

Called on a normal map change, not on a map_restart
===============
*/
void SV_InitGameProgs(void) {
	sv.num_tagheaders = 0;
	sv.num_tags = 0;

	// load the dll or bytecode
	gvm = VM_Create( "qagame", SV_GameSystemCalls, Cvar_VariableValue( "vm_game" ) );
	if(!gvm)
	{
		Com_Error(ERR_FATAL, "VM_Create on game failed");
	}

	SV_InitGameVM(qfalse);
}


/*
====================
SV_GameCommand

See if the current console command is claimed by the game
====================
*/
qboolean SV_GameCommand(void)
{
	if(sv.state != SS_GAME)
	{
		return qfalse;
	}

	return VM_Call(gvm, GAME_CONSOLE_COMMAND);
}

/*
====================
SV_GameIsSinglePlayer
====================
*/
qboolean SV_GameIsSinglePlayer(void)
{
	return (com_gameInfo.spGameTypes & (1 << g_gameType->integer));
}

/*
====================
SV_GameIsCoop

	This is a modified SinglePlayer, no savegame capability for example
====================
*/
qboolean SV_GameIsCoop(void)
{
	return (com_gameInfo.coopGameTypes & (1 << g_gameType->integer));
}

/*
====================
SV_GetTag

  return qfalse if unable to retrieve tag information for this client
====================
*/
extern qboolean CL_GetTag(int clientNum, char *tagname, orientation_t * or);

qboolean SV_GetTag(int clientNum, int tagFileNumber, char *tagname, orientation_t * or)
{
	int             i;

	if(tagFileNumber > 0 && tagFileNumber <= sv.num_tagheaders)
	{
		for(i = sv.tagHeadersExt[tagFileNumber - 1].start;
			i < sv.tagHeadersExt[tagFileNumber - 1].start + sv.tagHeadersExt[tagFileNumber - 1].count; i++)
		{
			if(!Q_stricmp(sv.tags[i].name, tagname))
			{
				VectorCopy(sv.tags[i].origin, or->origin);
				VectorCopy(sv.tags[i].axis[0], or->axis[0]);
				VectorCopy(sv.tags[i].axis[1], or->axis[1]);
				VectorCopy(sv.tags[i].axis[2], or->axis[2]);
				return qtrue;
			}
		}
	}

	// Gordon: lets try and remove the inconsitancy between ded/non-ded servers...
	// Gordon: bleh, some code in clientthink_real really relies on this working on player models...
#ifndef DEDICATED				// TTimo: dedicated only binary defines DEDICATED
	if(com_dedicated->integer)
	{
		return qfalse;
	}

	return CL_GetTag(clientNum, tagname, or);
#else
	return qfalse;
#endif
}
