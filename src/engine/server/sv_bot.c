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

// sv_bot.c

#include "server.h"

int                    bot_enable;

/*
==================
SV_BotAllocateClient
==================
*/
int SV_BotAllocateClient( int clientNum )
{
	int      i;
	int      firstSlot = MAX( 1, sv_privateClients->integer );
	client_t *cl;

	// Arnout: added possibility to request a clientnum
	if ( clientNum > 0 )
	{
		if ( clientNum >= sv_maxclients->integer )
		{
			return -1;
		}

		cl = &svs.clients[ clientNum ];

		if ( cl->state != CS_FREE )
		{
			return -1;
		}
		else
		{
			i = clientNum;
		}
	}
	else
	{
		// find a client slot
		for ( i = 0, cl = svs.clients; i < sv_maxclients->integer; i++, cl++ )
		{
			// Wolfenstein, never use the first slot, otherwise if a bot connects before the first client on a listen server, game won't start
			// Also, never use reserved slots since that messes up the reported player count
			if ( i < firstSlot )
			{
				continue;
			}

			// done.
			if ( cl->state == CS_FREE )
			{
				break;
			}
		}
	}

	if ( i == sv_maxclients->integer )
	{
		return -1;
	}

	cl->gentity = SV_GentityNum( i );
	cl->gentity->s.number = i;
	cl->state = CS_ACTIVE;
	cl->lastPacketTime = svs.time;
	cl->netchan.remoteAddress.type = NA_BOT;
	cl->rate = 16384;

	return i;
}

/*
==================
SV_BotFreeClient
==================
*/
void SV_BotFreeClient( int clientNum )
{
	client_t *cl;

	if ( clientNum < 0 || clientNum >= sv_maxclients->integer )
	{
		Com_Error( ERR_DROP, "SV_BotFreeClient: bad clientNum: %i", clientNum );
	}

	cl = &svs.clients[ clientNum ];
	cl->state = CS_FREE;
	cl->name[ 0 ] = 0;

	if ( cl->gentity )
	{
		cl->gentity->r.svFlags &= ~SVF_BOT;
	}
}

/*
==================
BotImport_PointContents
==================
*/
int BotImport_PointContents( vec3_t point )
{
	return SV_PointContents( point, -1 );
}

/*
==================
BotImport_inPVS
==================
*/
int BotImport_inPVS( vec3_t p1, vec3_t p2 )
{
	return SV_inPVS( p1, p2 );
}

/*
==================
BotImport_BSPEntityData
==================
*/
char           *BotImport_BSPEntityData( void )
{
	return CM_EntityString();
}

/*
==================
BotImport_BSPModelMinsMaxsOrigin
==================
*/
void BotImport_BSPModelMinsMaxsOrigin( int modelnum, vec3_t angles, vec3_t outmins, vec3_t outmaxs, vec3_t origin )
{
	clipHandle_t h;
	vec3_t       mins, maxs;
	float        max;
	int          i;

	h = CM_InlineModel( modelnum );
	CM_ModelBounds( h, mins, maxs );

	//if the model is rotated
	if ( ( angles[ 0 ] || angles[ 1 ] || angles[ 2 ] ) )
	{
		// expand for rotation

		max = RadiusFromBounds( mins, maxs );

		for ( i = 0; i < 3; i++ )
		{
			mins[ i ] = -max;
			maxs[ i ] = max;
		}
	}

	if ( outmins )
	{
		VectorCopy( mins, outmins );
	}

	if ( outmaxs )
	{
		VectorCopy( maxs, outmaxs );
	}

	if ( origin )
	{
		VectorClear( origin );
	}
}

/*
==================
BotImport_GetMemory
==================
*/
void           *BotImport_GetMemory( int size )
{
	void *ptr;

	ptr = Z_TagMalloc( size, TAG_BOTLIB );
	return ptr;
}

/*
==================
BotImport_FreeMemory
==================
*/
void BotImport_FreeMemory( void *ptr )
{
	Z_Free( ptr );
}

/*
==================
BotImport_FreeZoneMemory
==================
*/
void BotImport_FreeZoneMemory( void )
{
	Z_FreeTags( TAG_BOTLIB );
}

/*
=================
BotImport_HunkAlloc
=================
*/
void           *BotImport_HunkAlloc( int size )
{
	if ( Hunk_CheckMark() )
	{
		Com_Error( ERR_DROP, "SV_Bot_HunkAlloc: Alloc with marks already set" );
	}

	return Hunk_Alloc( size, h_high );
}

/*
==================
SV_BotClientCommand
==================
*/
void BotClientCommand( int client, char *command )
{
	SV_ExecuteClientCommand( &svs.clients[ client ], command, qtrue, qfalse );
}

/*
==================
SV_BotFrame
==================
*/
void SV_BotFrame( int time )
{
	if ( !bot_enable )
	{
		return;
	}

	//NOTE: maybe the game is already shutdown
	if ( !gvm )
	{
		return;
	}

	VM_Call( gvm, BOTAI_START_FRAME, time );
}

//
//  * * * BOT AI CODE IS BELOW THIS POINT * * *
//

/*
==================
SV_BotGetConsoleMessage
==================
*/
int SV_BotGetConsoleMessage( int client, char *buf, int size )
{
	client_t *cl;
	int      index;

	cl = &svs.clients[ client ];
	cl->lastPacketTime = svs.time;

	if ( cl->reliableAcknowledge == cl->reliableSequence )
	{
		return qfalse;
	}

	cl->reliableAcknowledge++;
	index = cl->reliableAcknowledge & ( MAX_RELIABLE_COMMANDS - 1 );

	if ( !cl->reliableCommands[ index ][ 0 ] )
	{
		return qfalse;
	}

	//Q_strncpyz( buf, cl->reliableCommands[index], size );
	return qtrue;
}

#if 0

/*
==================
EntityInPVS
==================
*/
int EntityInPVS( int client, int entityNum )
{
	client_t         *cl;
	clientSnapshot_t *frame;
	int              i;

	cl = &svs.clients[ client ];
	frame = &cl->frames[ cl->netchan.outgoingSequence & PACKET_MASK ];

	for ( i = 0; i < frame->num_entities; i++ )
	{
		if ( svs.snapshotEntities[( frame->first_entity + i ) % svs.numSnapshotEntities ].number == entityNum )
		{
			return qtrue;
		}
	}

	return qfalse;
}

#endif

/*
==================
SV_BotGetSnapshotEntity
==================
*/
int SV_BotGetSnapshotEntity( int client, int sequence )
{
	client_t         *cl;
	clientSnapshot_t *frame;

	cl = &svs.clients[ client ];
	frame = &cl->frames[ cl->netchan.outgoingSequence & PACKET_MASK ];

	if ( sequence < 0 || sequence >= frame->num_entities )
	{
		return -1;
	}

	return svs.snapshotEntities[( frame->first_entity + sequence ) % svs.numSnapshotEntities ].number;
}
