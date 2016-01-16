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

#include "server.h"

/*
=============================================================================

Delta encode a client frame onto the network channel

A server packet will look something like:

4 sequence number (high bit set if an oversize fragment)
<optional reliable commands>
1 svc_snapshot
4 last client reliable command
4 serverTime
1 lastframe for delta compression
1 snapFlags
1 areaBytes
<areabytes>
<playerstate>
<packetentities>

=============================================================================
*/

/*
=============
SV_EmitPacketEntities

Writes a delta update of an entityState_t list to the message.
=============
*/
static void SV_EmitPacketEntities( const clientSnapshot_t *from, clientSnapshot_t *to, msg_t *msg )
{
	entityState_t *oldent, *newent;
	int           oldindex, newindex;
	int           oldnum, newnum;
	int           from_num_entities;

    MSG_WriteShort(msg, to->num_entities);

	// generate the delta update
	if ( !from )
	{
		static const clientSnapshot_t nullfrom{};

		from = &nullfrom;
		from_num_entities = 0;
	}
	else
	{
		from_num_entities = from->num_entities;
	}

	newent = nullptr;
	oldent = nullptr;
	newindex = 0;
	oldindex = 0;

	while ( newindex < to->num_entities || oldindex < from_num_entities )
	{
		if ( newindex >= to->num_entities )
		{
			newnum = MAX_GENTITIES;
		}
		else
		{
			newent = &svs.snapshotEntities[( to->first_entity + newindex ) % svs.numSnapshotEntities ];
			newnum = newent->number;
		}

		if ( oldindex >= from_num_entities )
		{
			oldnum = MAX_GENTITIES;
		}
		else
		{
			oldent = &svs.snapshotEntities[( from->first_entity + oldindex ) % svs.numSnapshotEntities ];
			oldnum = oldent->number;
		}

		if ( newnum == oldnum )
		{
			// delta update from old position
			// because the force parm is false, this will not result
			// in any bytes being emited if the entity has not changed at all
			MSG_WriteDeltaEntity( msg, oldent, newent, false );
			oldindex++;
			newindex++;
			continue;
		}

		if ( newnum < oldnum )
		{
			// this is a new entity, send it from the baseline
			MSG_WriteDeltaEntity( msg, &sv.svEntities[ newnum ].baseline, newent, true );
			newindex++;
			continue;
		}

		if ( newnum > oldnum )
		{
			// the old entity isn't present in the new message
			MSG_WriteDeltaEntity( msg, oldent, nullptr, true );
			oldindex++;
			continue;
		}
	}

	MSG_WriteBits( msg, ( MAX_GENTITIES - 1 ), GENTITYNUM_BITS );  // end of packetentities
}

/*
==================
SV_WriteSnapshotToClient
==================
*/
static void SV_WriteSnapshotToClient( client_t *client, msg_t *msg )
{
	clientSnapshot_t *frame, *oldframe;
	int              lastframe;
	int              i;
	int              snapFlags;

	// this is the snapshot we are creating
	frame = &client->frames[ client->netchan.outgoingSequence & PACKET_MASK ];

	// try to use a previous frame as the source for delta compressing the snapshot
	if ( client->deltaMessage <= 0 || client->state != clientState_t::CS_ACTIVE )
	{
		// client is asking for a retransmit
		oldframe = nullptr;
		lastframe = 0;
	}
	else if ( client->netchan.outgoingSequence - client->deltaMessage >= ( PACKET_BACKUP - 3 ) )
	{
		// client hasn't gotten a good message through in a long time
		Com_DPrintf( "%s^7: Delta request from out of date packet.\n", client->name );
		oldframe = nullptr;
		lastframe = 0;
	}
	else
	{
		// we have a valid snapshot to delta from
		oldframe = &client->frames[ client->deltaMessage & PACKET_MASK ];
		lastframe = client->netchan.outgoingSequence - client->deltaMessage;

		// the snapshot's entities may still have rolled off the buffer, though
		if ( oldframe->first_entity <= svs.nextSnapshotEntities - svs.numSnapshotEntities )
		{
			Com_DPrintf( "%s^7: Delta request from out of date entities.\n", client->name );
			oldframe = nullptr;
			lastframe = 0;
		}
	}

	MSG_WriteByte( msg, svc_snapshot );

	// NOTE, MRE: now sent at the start of every message from server to client
	// let the client know which reliable clientCommands we have received
	//MSG_WriteLong( msg, client->lastClientCommand );

	// send over the current server time so the client can drift
	// its view of time to try to match
	MSG_WriteLong( msg, sv.time );

	// what we are delta'ing from
	MSG_WriteByte( msg, lastframe );

	snapFlags = svs.snapFlagServerBit;

	if ( client->rateDelayed )
	{
		snapFlags |= SNAPFLAG_RATE_DELAYED;
	}

	if ( client->state != clientState_t::CS_ACTIVE )
	{
		snapFlags |= SNAPFLAG_NOT_ACTIVE;
	}

	MSG_WriteByte( msg, snapFlags );

	// send over the areabits
	MSG_WriteByte( msg, frame->areabytes );
	MSG_WriteData( msg, frame->areabits, frame->areabytes );

	{
		// delta encode the playerstate
		if ( oldframe )
		{
			MSG_WriteDeltaPlayerstate( msg, &oldframe->ps, &frame->ps );
		}
		else
		{
			MSG_WriteDeltaPlayerstate( msg, nullptr, &frame->ps );
		}
	}

	// delta encode the entities
	SV_EmitPacketEntities( oldframe, frame, msg );

	// padding for rate debugging
	if ( sv_padPackets->integer )
	{
		for ( i = 0; i < sv_padPackets->integer; i++ )
		{
			MSG_WriteByte( msg, svc_nop );
		}
	}
}

/*
==================
SV_UpdateServerCommandsToClient

(re)send all server commands the client hasn't acknowledged yet
==================
*/
void SV_UpdateServerCommandsToClient( client_t *client, msg_t *msg )
{
	int i;

	// write any unacknowledged serverCommands
	for ( i = client->reliableAcknowledge + 1; i <= client->reliableSequence; i++ )
	{
		MSG_WriteByte( msg, svc_serverCommand );
		MSG_WriteLong( msg, i );
		MSG_WriteString( msg, client->reliableCommands[ i & ( MAX_RELIABLE_COMMANDS - 1 ) ] );
	}

	client->reliableSent = client->reliableSequence;
}

/*
=============================================================================

Build a client snapshot structure

=============================================================================
*/

//#define   MAX_SNAPSHOT_ENTITIES   1024
static const int MAX_SNAPSHOT_ENTITIES = 2048;

struct snapshotEntityNumbers_t
{
	int numSnapshotEntities;
	int snapshotEntities[ MAX_SNAPSHOT_ENTITIES ];
};

/*
=======================
SV_QsortEntityNumbers
=======================
*/
static int QDECL SV_QsortEntityNumbers( const void *a, const void *b )
{
	int *ea, *eb;

	ea = ( int * ) a;
	eb = ( int * ) b;

	if ( *ea == *eb )
	{
		Com_Error( errorParm_t::ERR_DROP, "SV_QsortEntityStates: duplicated entity" );
	}

	if ( *ea < *eb )
	{
		return -1;
	}

	return 1;
}

/*
===============
SV_AddEntToSnapshot
===============
*/
static void SV_AddEntToSnapshot( sharedEntity_t *clientEnt, svEntity_t *svEnt, sharedEntity_t *gEnt,
                                 snapshotEntityNumbers_t *eNums )
{
	// if we have already added this entity to this snapshot, don't add again
	if ( svEnt->snapshotCounter == sv.snapshotCounter )
	{
		return;
	}

	svEnt->snapshotCounter = sv.snapshotCounter;

	// if we are full, silently discard entities
	if ( eNums->numSnapshotEntities == MAX_SNAPSHOT_ENTITIES )
	{
		return;
	}

	if ( gEnt->r.snapshotCallback )
	{
		if ( !gvm.GameSnapshotCallback( gEnt->s.number, clientEnt->s.number ) )
		{
			return;
		}
	}

	eNums->snapshotEntities[ eNums->numSnapshotEntities ] = gEnt->s.number;
	eNums->numSnapshotEntities++;
}

/*
===============
SV_AddEntitiesVisibleFromPoint
===============
*/
static void SV_AddEntitiesVisibleFromPoint( vec3_t origin, clientSnapshot_t *frame,
//                                  snapshotEntityNumbers_t *eNums, bool portal, clientSnapshot_t *oldframe, bool localClient ) {
//                                  snapshotEntityNumbers_t *eNums, bool portal ) {
    snapshotEntityNumbers_t *eNums /*, bool portal, bool localClient */ )
{
	int            e, i;
	sharedEntity_t *ent, *playerEnt;
	svEntity_t     *svEnt;
	int            l;
	int            clientarea, clientcluster;
	int            leafnum;
//	int             c_fullsend;
	byte           *clientpvs;
	byte           *bitvector;

	// during an error shutdown message we may need to transmit
	// the shutdown message after the server has shutdown, so
	// specfically check for it
	if (sv.state == serverState_t::SS_DEAD)
	{
		return;
	}

	leafnum = CM_PointLeafnum( origin );
	clientarea = CM_LeafArea( leafnum );
	clientcluster = CM_LeafCluster( leafnum );

	// calculate the visible areas
	frame->areabytes = CM_WriteAreaBits( frame->areabits, clientarea );

	clientpvs = CM_ClusterPVS( clientcluster );

//	c_fullsend = 0;

	playerEnt = SV_GentityNum( frame->ps.clientNum );

	if ( playerEnt->r.svFlags & SVF_SELF_PORTAL )
	{
		SV_AddEntitiesVisibleFromPoint( playerEnt->s.origin2, frame, eNums );
	}

	for ( e = 0; e < sv.num_entities; e++ )
	{
		ent = SV_GentityNum( e );

		// never send entities that aren't linked in
		if ( !ent->r.linked )
		{
			continue;
		}

		if ( ent->s.number != e )
		{
			Com_DPrintf( "FIXING ENT->S.NUMBER!!!\n" );
			ent->s.number = e;
		}

		// entities can be flagged to explicitly not be sent to the client
		if ( ent->r.svFlags & SVF_NOCLIENT )
		{
			continue;
		}

		// entities can be flagged to be sent to only one client
		if ( ent->r.svFlags & SVF_SINGLECLIENT )
		{
			if ( ent->r.singleClient != frame->ps.clientNum )
			{
				continue;
			}
		}

		// entities can be flagged to be sent to everyone but one client
		if ( ent->r.svFlags & SVF_NOTSINGLECLIENT )
		{
			if ( ent->r.singleClient == frame->ps.clientNum )
			{
				continue;
			}
		}

		// entities can be flagged to be sent to only a given mask of clients
		if ( ent->r.svFlags & SVF_CLIENTMASK )
		{
			if ( frame->ps.clientNum >= 32 )
			{
				if ( ~ent->r.hiMask & ( 1 << ( frame->ps.clientNum - 32 ) ) )
				{
					continue;
				}
			}
			else
			{
				if ( ~ent->r.loMask & ( 1 << frame->ps.clientNum ) )
				{
					continue;
				}
			}
		}

		svEnt = SV_SvEntityForGentity( ent );

		// don't double add an entity through portals
		if ( svEnt->snapshotCounter == sv.snapshotCounter )
		{
			continue;
		}

		// broadcast entities are always sent
		if ( ent->r.svFlags & SVF_BROADCAST )
		{
			SV_AddEntToSnapshot( playerEnt, svEnt, ent, eNums );
			continue;
		}

		// send entity if the client is in range
		if ( (ent->r.svFlags & SVF_CLIENTS_IN_RANGE) &&
		     Distance( ent->s.origin, playerEnt->s.origin ) <= ent->r.clientRadius )
		{
			SV_AddEntToSnapshot( playerEnt, svEnt, ent, eNums );
			continue;
		}

		bitvector = clientpvs;

		// Gordon: just check origin for being in pvs, ignore bmodel extents
		if ( ent->r.svFlags & SVF_IGNOREBMODELEXTENTS )
		{
			if ( bitvector[ ent->r.originCluster >> 3 ] & ( 1 << ( ent->r.originCluster & 7 ) ) )
			{
				SV_AddEntToSnapshot( playerEnt, svEnt, ent, eNums );
			}

			continue;
		}

		// ignore if not touching a PV leaf
		// check area
		if ( !CM_AreasConnected( clientarea, ent->r.areanum ) )
		{
			// doors can legally straddle two areas, so
			// we may need to check another one
			if ( !CM_AreasConnected( clientarea, ent->r.areanum2 ) )
			{
				continue;
			}
		}

		// check individual leafs
		if ( !ent->r.numClusters )
		{
			continue;
		}

		l = 0;

		for ( i = 0; i < std::min(std::max(0, ent->r.numClusters), MAX_ENT_CLUSTERS); i++ )
		{
			l = ent->r.clusternums[ i ];

			if ( bitvector[ l >> 3 ] & ( 1 << ( l & 7 ) ) )
			{
				break;
			}
		}

		// if we haven't found it to be visible,
		// check the overflow clusters that couldn't be stored
		if ( i == ent->r.numClusters )
		{
			if ( ent->r.lastCluster )
			{
				for ( ; l <= ent->r.lastCluster; l++ )
				{
					if ( bitvector[ l >> 3 ] & ( 1 << ( l & 7 ) ) )
					{
						break;
					}
				}

				if ( l == ent->r.lastCluster )
				{
					continue;
				}
			}
			else
			{
				continue;
			}
		}

		//----(SA) added "visibility dummies"
		if ( ent->r.svFlags & SVF_VISDUMMY )
		{
			sharedEntity_t *ment = 0;

			//find master;
			ment = SV_GentityNum( ent->s.otherEntityNum );

			if ( ment )
			{
				svEntity_t *master = 0;

				master = SV_SvEntityForGentity( ment );

				if ( master->snapshotCounter == sv.snapshotCounter || !ment->r.linked )
				{
					continue;
				}

				SV_AddEntToSnapshot( playerEnt, master, ment, eNums );
			}

			continue; // master needs to be added, but not this dummy ent
		}
		//----(SA) end
		else if ( ent->r.svFlags & SVF_VISDUMMY_MULTIPLE )
		{
			{
				int            h;
				sharedEntity_t *ment = 0;
				svEntity_t     *master = 0;

				for ( h = 0; h < sv.num_entities; h++ )
				{
					ment = SV_GentityNum( h );

					if ( ment == ent )
					{
						continue;
					}

					if ( ment )
					{
						master = SV_SvEntityForGentity( ment );
					}
					else
					{
						continue;
					}

					if ( !( ment->r.linked ) )
					{
						continue;
					}

					if ( ment->s.number != h )
					{
						Com_DPrintf( "FIXING vis dummy multiple ment->S.NUMBER!!!\n" );
						ment->s.number = h;
					}

					if ( ment->r.svFlags & SVF_NOCLIENT )
					{
						continue;
					}

					if ( master->snapshotCounter == sv.snapshotCounter )
					{
						continue;
					}

					if ( ment->s.otherEntityNum == ent->s.number )
					{
						SV_AddEntToSnapshot( playerEnt, master, ment, eNums );
					}
				}

				continue;
			}
		}

		// add it
		SV_AddEntToSnapshot( playerEnt, svEnt, ent, eNums );

		// if it's a portal entity, add everything visible from its camera position
		if ( ent->r.svFlags & SVF_PORTAL )
		{
			if ( ent->s.generic1 )
			{
				vec3_t dir;
				VectorSubtract( ent->s.origin, origin, dir );

				if ( VectorLengthSquared( dir ) > ( float ) ent->s.generic1 * ent->s.generic1 )
				{
					continue;
				}
			}

//          SV_AddEntitiesVisibleFromPoint( ent->s.origin2, frame, eNums, true, oldframe, localClient );
			SV_AddEntitiesVisibleFromPoint( ent->s.origin2, frame, eNums /*, true, localClient */ );
		}

		continue;
	}
}

/*
=============
SV_BuildClientSnapshot

Decides which entities are going to be visible to the client, and
copies off the playerstate and areabits.

This properly handles multiple recursive portals, but the render
currently doesn't.

For viewing through other player's eyes, clent can be something other than client->gentity
=============
*/
static void SV_BuildClientSnapshot( client_t *client )
{
	vec3_t                  org;
	clientSnapshot_t        *frame;
	snapshotEntityNumbers_t entityNumbers;
	int                     i;
	sharedEntity_t          *ent;
	entityState_t           *state;
	svEntity_t              *svEnt;
	sharedEntity_t          *clent;
	int                     clientNum;
	playerState_t           *ps;

	// bump the counter used to prevent double adding
	sv.snapshotCounter++;

	// this is the frame we are creating
	frame = &client->frames[ client->netchan.outgoingSequence & PACKET_MASK ];

	// clear everything in this snapshot
	entityNumbers.numSnapshotEntities = 0;
	Com_Memset( frame->areabits, 0, sizeof( frame->areabits ) );

	// show_bug.cgi?id=62
	frame->num_entities = 0;

	clent = client->gentity;

	if ( !clent || client->state == clientState_t::CS_ZOMBIE )
	{
		return;
	}

	// grab the current playerState_t
	ps = SV_GameClientNum( client - svs.clients );
	frame->ps = *ps;

	// never send client's own entity, because it can
	// be regenerated from the playerstate
	clientNum = frame->ps.clientNum;

	if ( clientNum < 0 || clientNum >= MAX_GENTITIES )
	{
		Com_Error( errorParm_t::ERR_DROP, "SV_SvEntityForGentity: bad gEnt" );
	}

	svEnt = &sv.svEntities[ clientNum ];

	svEnt->snapshotCounter = sv.snapshotCounter;

	if ( clent->r.svFlags & SVF_SELF_PORTAL_EXCLUSIVE )
	{
		// find the client's viewpoint
		VectorCopy( clent->s.origin2, org );
	}
	else
	{
		VectorCopy( ps->origin, org );
	}

	org[ 2 ] += ps->viewheight;

	// add all the entities directly visible to the eye, which
	// may include portal entities that merge other viewpoints
	SV_AddEntitiesVisibleFromPoint( org, frame, &entityNumbers /*, false, client->netchan.remoteAddress.type == NA_LOOPBACK */ );

	// if there were portals visible, there may be out of order entities
	// in the list which will need to be resorted for the delta compression
	// to work correctly.  This also catches the error condition
	// of an entity being included twice.
	qsort( entityNumbers.snapshotEntities, entityNumbers.numSnapshotEntities,
	       sizeof( entityNumbers.snapshotEntities[ 0 ] ), SV_QsortEntityNumbers );

	// now that all viewpoint's areabits have been OR'd together, invert
	// all of them to make it a mask vector, which is what the renderer wants
	for ( i = 0; i < MAX_MAP_AREA_BYTES / 4; i++ )
	{
		( ( int * ) frame->areabits ) [ i ] = ( ( int * ) frame->areabits ) [ i ] ^ -1;
	}

	// copy the entity states out
	frame->num_entities = 0;
	frame->first_entity = svs.nextSnapshotEntities;

	for ( i = 0; i < entityNumbers.numSnapshotEntities; i++ )
	{
		ent = SV_GentityNum( entityNumbers.snapshotEntities[ i ] );
		state = &svs.snapshotEntities[ svs.nextSnapshotEntities % svs.numSnapshotEntities ];
		*state = ent->s;
		svs.nextSnapshotEntities++;

		// this should never hit, map should always be restarted first in SV_Frame
		if ( svs.nextSnapshotEntities >= 0x7FFFFFFE )
		{
			Com_Error( errorParm_t::ERR_FATAL, "svs.nextSnapshotEntities wrapped" );
		}

		frame->num_entities++;
	}
}

/*
====================
SV_RateMsec

Return the number of msec a given size message is supposed
to take to clear, based on the current rate
TTimo - use sv_maxRate or sv_dl_maxRate depending on regular or downloading client
====================
*/
static const int HEADER_RATE_BYTES = 48; // include our header, IP header, and some overhead
static int SV_RateMsec( client_t *client, int messageSize )
{
	int rate;
	int rateMsec;
	int maxRate;

	// individual messages will never be larger than fragment size
	if ( messageSize > 1500 )
	{
		messageSize = 1500;
	}

	// low watermark for sv_maxRate, never 0 < sv_maxRate < 1000 (0 is no limitation)
	if ( sv_maxRate->integer && sv_maxRate->integer < 1000 )
	{
		Cvar_Set( "sv_MaxRate", "1000" );
	}

	rate = client->rate;

	// work on the appropriate max rate (client or download)
	if ( !*client->downloadName )
	{
		maxRate = sv_maxRate->integer;
	}
	else
	{
		maxRate = sv_dl_maxRate->integer;
	}

	if ( maxRate )
	{
		if ( maxRate < rate )
		{
			rate = maxRate;
		}
	}

	rateMsec = ( messageSize + HEADER_RATE_BYTES ) * 1000 / rate;

	return rateMsec;
}

/*
=======================
SV_SendMessageToClient

Called by SV_SendClientSnapshot and SV_SendClientGameState
=======================
*/
void SV_SendMessageToClient( msg_t *msg, client_t *client )
{
	int rateMsec;

	// record information about the message
	client->frames[ client->netchan.outgoingSequence & PACKET_MASK ].messageSize = msg->cursize;
	client->frames[ client->netchan.outgoingSequence & PACKET_MASK ].messageSent = svs.time;
	client->frames[ client->netchan.outgoingSequence & PACKET_MASK ].messageAcked = -1;

	// send the datagram
	SV_Netchan_Transmit( client, msg );

	// set nextSnapshotTime based on rate and requested number of updates

	// local clients get snapshots every frame
	// TTimo - show_bug.cgi?id=491
	// added sv_lanForceRate check
	if ( client->netchan.remoteAddress.type == netadrtype_t::NA_LOOPBACK ||
	     ( sv_lanForceRate->integer && Sys_IsLANAddress( client->netchan.remoteAddress ) ) )
	{
		client->nextSnapshotTime = svs.time - 1;
		return;
	}

	rateMsec = SV_RateMsec( client, msg->cursize );

	// TTimo - during a download, ignore the snapshotMsec
	// the update server on steroids, with this disabled and sv_fps 60, the download can reach 30 kb/s
	// on a regular server, we will still top at 20 kb/s because of sv_fps 20
	if ( !*client->downloadName && rateMsec < client->snapshotMsec )
	{
		// never send more packets than this, no matter what the rate is at
		rateMsec = client->snapshotMsec;
		client->rateDelayed = false;
	}
	else
	{
		client->rateDelayed = true;
	}

	client->nextSnapshotTime = svs.time + rateMsec;

	// don't pile up empty snapshots while connecting
	if ( client->state != clientState_t::CS_ACTIVE )
	{
		// a gigantic connection message may have already put the nextSnapshotTime
		// more than a second away, so don't shorten it
		// do shorten if client is downloading
		if ( !*client->downloadName && client->nextSnapshotTime < svs.time + 1000 )
		{
			client->nextSnapshotTime = svs.time + 1000;
		}
	}
}

//bani

/*
=======================
SV_SendClientIdle

There is no need to send full snapshots to clients who are loading a map.
So we send them "idle" packets with the bare minimum required to keep them on the server.

=======================
*/
void SV_SendClientIdle( client_t *client )
{
	byte  msg_buf[ MAX_MSGLEN ];
	msg_t msg;

	MSG_Init( &msg, msg_buf, sizeof( msg_buf ) );
	msg.allowoverflow = true;

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong( &msg, client->lastClientCommand );

	// (re)send any reliable server commands
	SV_UpdateServerCommandsToClient( client, &msg );

	// send over all the relevant entityState_t
	// and the playerState_t
//  SV_WriteSnapshotToClient( client, &msg );

	// Add any download data if the client is downloading
	SV_WriteDownloadToClient( client, &msg );

	// check for overflow
	if ( msg.overflowed )
	{
		Com_Printf( "WARNING: msg overflowed for %s\n", client->name );
		MSG_Clear( &msg );

		SV_DropClient( client, "Msg overflowed" );
		return;
	}

	SV_SendMessageToClient( &msg, client );

	sv.bpsTotalBytes += msg.cursize; // NERVE - SMF - net debugging
	sv.ubpsTotalBytes += msg.uncompsize / 8; // NERVE - SMF - net debugging
}

/*
=======================
SV_SendClientSnapshot

Also called by SV_FinalCommand

=======================
*/
void SV_SendClientSnapshot( client_t *client )
{
	byte  msg_buf[ MAX_MSGLEN ];
	msg_t msg;

	//bani
	if ( client->state < clientState_t::CS_ACTIVE )
	{
		// bani - #760 - zombie clients need full snaps so they can still process reliable commands
		// (eg so they can pick up the disconnect reason)
		if ( client->state != clientState_t::CS_ZOMBIE )
		{
			SV_SendClientIdle( client );
			return;
		}
	}

	// build the snapshot
	SV_BuildClientSnapshot( client );

	// bots need to have their snapshots built, but
	// those are queried directly without needing to be sent
	if ( SV_IsBot(client) )
	{
		return;
	}

	MSG_Init( &msg, msg_buf, sizeof( msg_buf ) );
	msg.allowoverflow = true;

	// NOTE, MRE: all server->client messages now acknowledge
	// let the client know which reliable clientCommands we have received
	MSG_WriteLong( &msg, client->lastClientCommand );

	// (re)send any reliable server commands
	SV_UpdateServerCommandsToClient( client, &msg );

	// send over all the relevant entityState_t
	// and the playerState_t
	SV_WriteSnapshotToClient( client, &msg );

	// Add any download data if the client is downloading
	SV_WriteDownloadToClient( client, &msg );

	// check for overflow
	if ( msg.overflowed )
	{
		Com_Logf(log_level_t::LOG_WARN, "msg overflowed for %s", client->name );
		MSG_Clear( &msg );

		SV_DropClient( client, "Msg overflowed" );
		return;
	}

	SV_SendMessageToClient( &msg, client );

	sv.bpsTotalBytes += msg.cursize; // NERVE - SMF - net debugging
	sv.ubpsTotalBytes += msg.uncompsize / 8; // NERVE - SMF - net debugging
}

/*
=======================
SV_SendClientMessages
=======================
*/

void SV_SendClientMessages()
{
	int      i;
	client_t *c;
	int      numclients = 0; // NERVE - SMF - net debugging

	sv.bpsTotalBytes = 0; // NERVE - SMF - net debugging
	sv.ubpsTotalBytes = 0; // NERVE - SMF - net debugging

	// Gordon: update any changed configstrings from this frame
	SV_UpdateConfigStrings();

	// send a message to each connected client
	for ( i = 0; i < sv_maxclients->integer; i++ )
	{
		c = &svs.clients[ i ];

		// rain - changed <= CS_ZOMBIE to < CS_ZOMBIE so that the
		// disconnect reason is properly sent in the network stream
		if ( c->state < clientState_t::CS_ZOMBIE )
		{
			continue; // not connected
		}

		// RF, needed to insert this otherwise bots would cause error drops in sv_net_chan.c:
		// --> "netchan queue is not properly initialized in SV_Netchan_TransmitNextFragment\n"
		if ( SV_IsBot(c) )
		{
			continue;
		}

		if ( svs.time < c->nextSnapshotTime )
		{
			continue; // not time yet
		}

		numclients++; // NERVE - SMF - net debugging

		// send additional message fragments if the last message
		// was too large to send at once
		if ( c->netchan.unsentFragments )
		{
			c->nextSnapshotTime = svs.time + SV_RateMsec( c, c->netchan.unsentLength - c->netchan.unsentFragmentStart );
			SV_Netchan_TransmitNextFragment( c );
			continue;
		}

		// generate and send a new message
		SV_SendClientSnapshot( c );
	}

	// NERVE - SMF - net debugging
	if ( sv_showAverageBPS->integer && numclients > 0 )
	{
		float ave = 0, uave = 0;

		for ( i = 0; i < MAX_BPS_WINDOW - 1; i++ )
		{
			sv.bpsWindow[ i ] = sv.bpsWindow[ i + 1 ];
			ave += sv.bpsWindow[ i ];

			sv.ubpsWindow[ i ] = sv.ubpsWindow[ i + 1 ];
			uave += sv.ubpsWindow[ i ];
		}

		sv.bpsWindow[ MAX_BPS_WINDOW - 1 ] = sv.bpsTotalBytes;
		ave += sv.bpsTotalBytes;

		sv.ubpsWindow[ MAX_BPS_WINDOW - 1 ] = sv.ubpsTotalBytes;
		uave += sv.ubpsTotalBytes;

		if ( sv.bpsTotalBytes >= sv.bpsMaxBytes )
		{
			sv.bpsMaxBytes = sv.bpsTotalBytes;
		}

		if ( sv.ubpsTotalBytes >= sv.ubpsMaxBytes )
		{
			sv.ubpsMaxBytes = sv.ubpsTotalBytes;
		}

		sv.bpsWindowSteps++;

		if ( sv.bpsWindowSteps >= MAX_BPS_WINDOW )
		{
			float comp_ratio;

			sv.bpsWindowSteps = 0;

			ave = ( ave / ( float ) MAX_BPS_WINDOW );
			uave = ( uave / ( float ) MAX_BPS_WINDOW );

			comp_ratio = ( 1 - ave / uave ) * 100.f;
			sv.ucompAve += comp_ratio;
			sv.ucompNum++;

			Com_DPrintf( "bpspc(%2.0f) bps(%2.0f) pk(%i) ubps(%2.0f) upk(%i) cr(%2.2f) acr(%2.2f)\n",
			             ave / ( float ) numclients, ave, sv.bpsMaxBytes, uave, sv.ubpsMaxBytes, comp_ratio,
			             sv.ucompAve / sv.ucompNum );
		}
	}

	// -NERVE - SMF
}
