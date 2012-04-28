/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// g_utils.c -- misc utility functions for game module

#include "g_local.h"

typedef struct
{
	char  oldShader[ MAX_QPATH ];
	char  newShader[ MAX_QPATH ];
	float timeOffset;
} shaderRemap_t;

#define MAX_SHADER_REMAPS 128

int           remapCount = 0;
shaderRemap_t remappedShaders[ MAX_SHADER_REMAPS ];

void AddRemap( const char *oldShader, const char *newShader, float timeOffset )
{
	int i;

	for ( i = 0; i < remapCount; i++ )
	{
		if ( Q_stricmp( oldShader, remappedShaders[ i ].oldShader ) == 0 )
		{
			// found it, just update this one
			strcpy( remappedShaders[ i ].newShader, newShader );
			remappedShaders[ i ].timeOffset = timeOffset;
			return;
		}
	}

	if ( remapCount < MAX_SHADER_REMAPS )
	{
		strcpy( remappedShaders[ remapCount ].newShader, newShader );
		strcpy( remappedShaders[ remapCount ].oldShader, oldShader );
		remappedShaders[ remapCount ].timeOffset = timeOffset;
		remapCount++;
	}
}

const char *BuildShaderStateConfig( void )
{
	static char buff[ MAX_STRING_CHARS * 4 ];
	char        out[( MAX_QPATH * 2 ) + 5 ];
	int         i;

	memset( buff, 0, MAX_STRING_CHARS );

	for ( i = 0; i < remapCount; i++ )
	{
		Com_sprintf( out, ( MAX_QPATH * 2 ) + 5, "%s=%s:%5.2f@", remappedShaders[ i ].oldShader,
		             remappedShaders[ i ].newShader, remappedShaders[ i ].timeOffset );
		Q_strcat( buff, sizeof( buff ), out );
	}

	return buff;
}

/*
=========================================================================

model / sound configstring indexes

=========================================================================
*/

/*
================
G_FindConfigstringIndex

================
*/
static int G_FindConfigstringIndex( const char *name, int start, int max, qboolean create )
{
	int  i;
	char s[ MAX_STRING_CHARS ];

	if ( !name || !name[ 0 ] )
	{
		return 0;
	}

	for ( i = 1; i < max; i++ )
	{
		trap_GetConfigstring( start + i, s, sizeof( s ) );

		if ( !s[ 0 ] )
		{
			break;
		}

		if ( !strcmp( s, name ) )
		{
			return i;
		}
	}

	if ( !create )
	{
		return 0;
	}

	if ( i == max )
	{
		G_Error( "G_FindConfigstringIndex: overflow" );
	}

	trap_SetConfigstring( start + i, name );

	return i;
}

int G_ParticleSystemIndex( const char *name )
{
	return G_FindConfigstringIndex( name, CS_PARTICLE_SYSTEMS, MAX_GAME_PARTICLE_SYSTEMS, qtrue );
}

int G_ShaderIndex( const char *name )
{
	return G_FindConfigstringIndex( name, CS_SHADERS, MAX_GAME_SHADERS, qtrue );
}

int G_ModelIndex( const char *name )
{
	return G_FindConfigstringIndex( name, CS_MODELS, MAX_MODELS, qtrue );
}

int G_SoundIndex( const char *name )
{
	return G_FindConfigstringIndex( name, CS_SOUNDS, MAX_SOUNDS, qtrue );
}

//=====================================================================

/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the entity after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
gentity_t *G_Find( gentity_t *from, int fieldofs, const char *match )
{
	char *s;

	if ( !from )
	{
		from = g_entities;
	}
	else
	{
		from++;
	}

	for ( ; from < &g_entities[ level.num_entities ]; from++ )
	{
		if ( !from->inuse )
		{
			continue;
		}

		s = * ( char ** )( ( byte * ) from + fieldofs );

		if ( !s )
		{
			continue;
		}

		if ( !Q_stricmp( s, match ) )
		{
			return from;
		}
	}

	return NULL;
}

/*
=============
G_PickTarget

Selects a random entity from among the targets
=============
*/
#define MAXCHOICES 32

gentity_t *G_PickTarget( const char *targetname )
{
	gentity_t *ent = NULL;
	int       num_choices = 0;
	gentity_t *choice[ MAXCHOICES ];

	if ( !targetname )
	{
		G_Printf( "G_PickTarget called with NULL targetname\n" );
		return NULL;
	}

	while ( 1 )
	{
		ent = G_Find( ent, FOFS( targetname ), targetname );

		if ( !ent )
		{
			break;
		}

		choice[ num_choices++ ] = ent;

		if ( num_choices == MAXCHOICES )
		{
			break;
		}
	}

	if ( !num_choices )
	{
		G_Printf( "G_PickTarget: target %s not found\n", targetname );
		return NULL;
	}

	return choice[ rand() / ( RAND_MAX / num_choices + 1 ) ];
}

/*
==============================
G_UseTargets

"activator" should be set to the entity that initiated the firing.

Search for (string)targetname in all entities that
match (string)self.target and call their .use function

==============================
*/
void G_UseTargets( gentity_t *ent, gentity_t *activator )
{
	gentity_t *t;

	if ( !ent )
	{
		return;
	}

	if ( ent->targetShaderName && ent->targetShaderNewName )
	{
		float f = level.time * 0.001;
		AddRemap( ent->targetShaderName, ent->targetShaderNewName, f );
		trap_SetConfigstring( CS_SHADERSTATE, BuildShaderStateConfig() );
	}

	if ( !ent->target )
	{
		return;
	}

	t = NULL;

	while ( ( t = G_Find( t, FOFS( targetname ), ent->target ) ) != NULL )
	{
		if ( t == ent )
		{
			G_Printf( "WARNING: Entity used itself.\n" );
		}
		else
		{
			if ( t->use )
			{
				t->use( t, ent, activator );
			}
		}

		if ( !ent->inuse )
		{
			G_Printf( "entity was removed while using targets\n" );
			return;
		}
	}
}

/*
=============
VectorToString

This is just a convenience function
for printing vectors
=============
*/
char *vtos( const vec3_t v )
{
	static  int  index;
	static  char str[ 8 ][ 32 ];
	char         *s;

	// use an array so that multiple vtos won't collide
	s = str[ index ];
	index = ( index + 1 ) & 7;

	Com_sprintf( s, 32, "(%i %i %i)", ( int ) v[ 0 ], ( int ) v[ 1 ], ( int ) v[ 2 ] );

	return s;
}

/*
===============
G_SetMovedir

The editor only specifies a single value for angles (yaw),
but we have special constants to generate an up or down direction.
Angles will be cleared, because it is being used to represent a direction
instead of an orientation.
===============
*/
void G_SetMovedir( vec3_t angles, vec3_t movedir )
{
	static vec3_t VEC_UP = { 0, -1, 0 };
	static vec3_t MOVEDIR_UP = { 0, 0, 1 };
	static vec3_t VEC_DOWN = { 0, -2, 0 };
	static vec3_t MOVEDIR_DOWN = { 0, 0, -1 };

	if ( VectorCompare( angles, VEC_UP ) )
	{
		VectorCopy( MOVEDIR_UP, movedir );
	}
	else if ( VectorCompare( angles, VEC_DOWN ) )
	{
		VectorCopy( MOVEDIR_DOWN, movedir );
	}
	else
	{
		AngleVectors( angles, movedir, NULL, NULL );
	}

	VectorClear( angles );
}

void G_InitGentity( gentity_t *e )
{
	e->inuse = qtrue;
	e->classname = "noclass";
	e->s.number = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;
}

/*
=================
G_Spawn

Either finds a free entity, or allocates a new one.

  The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
never be used by anything else.

Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
gentity_t *G_Spawn( void )
{
	int       i, force;
	gentity_t *e;

	e = NULL; // shut up warning
	i = 0; // shut up warning

	for ( force = 0; force < 2; force++ )
	{
		// if we go through all entities and can't find one to free,
		// override the normal minimum times before use
		e = &g_entities[ MAX_CLIENTS ];

		for ( i = MAX_CLIENTS; i < level.num_entities; i++, e++ )
		{
			if ( e->inuse )
			{
				continue;
			}

			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if ( !force && e->freetime > level.startTime + 2000 && level.time - e->freetime < 1000 )
			{
				continue;
			}

			// reuse this slot
			G_InitGentity( e );
			return e;
		}

		if ( i != MAX_GENTITIES )
		{
			break;
		}
	}

	if ( i == ENTITYNUM_MAX_NORMAL )
	{
		for ( i = 0; i < MAX_GENTITIES; i++ )
		{
			G_Printf( "%4i: %s\n", i, g_entities[ i ].classname );
		}

		G_Error( "G_Spawn: no free entities" );
	}

	// open up a new slot
	level.num_entities++;

	// let the server system know that there are more entities
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
	                     &level.clients[ 0 ].ps, sizeof( level.clients[ 0 ] ) );

	G_InitGentity( e );
	return e;
}

/*
=================
G_EntitiesFree
=================
*/
qboolean G_EntitiesFree( void )
{
	int       i;
	gentity_t *e;

	e = &g_entities[ MAX_CLIENTS ];

	for ( i = MAX_CLIENTS; i < level.num_entities; i++, e++ )
	{
		if ( e->inuse )
		{
			continue;
		}

		// slot available
		return qtrue;
	}

	return qfalse;
}

/*
=================
G_FreeEntity

Marks the entity as free
=================
*/
void G_FreeEntity( gentity_t *ent )
{
	trap_UnlinkEntity( ent );  // unlink from world

	if ( ent->neverFree )
	{
		return;
	}

	memset( ent, 0, sizeof( *ent ) );
	ent->classname = "freent";
	ent->freetime = level.time;
	ent->inuse = qfalse;
}

/*
=================
G_TempEntity

Spawns an event entity that will be auto-removed
The origin will be snapped to save net bandwidth, so care
must be taken if the origin is right on a surface (snap towards start vector first)
=================
*/
gentity_t *G_TempEntity( vec3_t origin, int event )
{
	gentity_t *e;
	vec3_t    snapped;

	e = G_Spawn();
	e->s.eType = ET_EVENTS + event;

	e->classname = "tempEntity";
	e->eventTime = level.time;
	e->freeAfterEvent = qtrue;

	VectorCopy( origin, snapped );
	SnapVector( snapped );  // save network bandwidth
	G_SetOrigin( e, snapped );

	// find cluster for PVS
	trap_LinkEntity( e );

	return e;
}

/*
==============================================================================

Kill box

==============================================================================
*/

/*
=================
G_KillBox

Kills all entities that would touch the proposed new positioning
of ent.  Ent should be unlinked before calling this!
=================
*/
void G_KillBox( gentity_t *ent )
{
	int       i, num;
	int       touch[ MAX_GENTITIES ];
	gentity_t *hit;
	vec3_t    mins, maxs;

	VectorAdd( ent->r.currentOrigin, ent->r.mins, mins );
	VectorAdd( ent->r.currentOrigin, ent->r.maxs, maxs );
	num = trap_EntitiesInBox( mins, maxs, touch, MAX_GENTITIES );

	for ( i = 0; i < num; i++ )
	{
		hit = &g_entities[ touch[ i ] ];

		if ( !hit->client )
		{
			continue;
		}

		// impossible to telefrag self
		if ( ent == hit )
		{
			continue;
		}

		// nail it
		G_Damage( hit, ent, ent, NULL, NULL,
		          100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
	}
}

//==============================================================================

/*
===============
G_AddPredictableEvent

Use for non-pmove events that would also be predicted on the
client side: jumppads and item pickups
Adds an event+parm and twiddles the event counter
===============
*/
void G_AddPredictableEvent( gentity_t *ent, int event, int eventParm )
{
	if ( !ent->client )
	{
		return;
	}

	BG_AddPredictableEventToPlayerstate( event, eventParm, &ent->client->ps );
}

/*
===============
G_AddEvent

Adds an event+parm and twiddles the event counter
===============
*/
void G_AddEvent( gentity_t *ent, int event, int eventParm )
{
	int bits;

	if ( !event )
	{
		G_Printf( "G_AddEvent: zero event added for entity %i\n", ent->s.number );
		return;
	}

	// eventParm is converted to uint8_t (0 - 255) in msg.c
	if ( eventParm & ~0xFF )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: G_AddEvent( %s ) has eventParm %d, "
		          "which will overflow\n", BG_EventName( event ), eventParm );
	}

	// clients need to add the event in playerState_t instead of entityState_t
	if ( ent->client )
	{
		ent->client->ps.events[ ent->client->ps.eventSequence & ( MAX_EVENTS - 1 ) ] = event;
		ent->client->ps.eventParms[ ent->client->ps.eventSequence & ( MAX_EVENTS - 1 ) ] = eventParm;
		ent->client->ps.eventSequence++;
	}
	else
	{
		bits = ent->s.event & EV_EVENT_BITS;
		bits = ( bits + EV_EVENT_BIT1 ) & EV_EVENT_BITS;
		ent->s.event = event | bits;
		ent->s.eventParm = eventParm;
	}

	ent->eventTime = level.time;
}

/*
===============
G_BroadcastEvent

Sends an event to every client
===============
*/
void G_BroadcastEvent( int event, int eventParm )
{
	gentity_t *ent;

	ent = G_TempEntity( vec3_origin, event );
	ent->s.eventParm = eventParm;
	ent->r.svFlags = SVF_BROADCAST; // send to everyone
}

/*
=============
G_Sound
=============
*/
void G_Sound( gentity_t *ent, int channel, int soundIndex )
{
	gentity_t *te;

	te = G_TempEntity( ent->r.currentOrigin, EV_GENERAL_SOUND );
	te->s.eventParm = soundIndex;
}

/*
=============
G_ClientIsLagging
=============
*/
qboolean G_ClientIsLagging( gclient_t *client )
{
	if ( client )
	{
		if ( client->ps.ping >= 999 )
		{
			return qtrue;
		}
		else
		{
			return qfalse;
		}
	}

	return qfalse; //is a non-existant client lagging? woooo zen
}

//==============================================================================

/*
================
G_SetOrigin

Sets the pos trajectory for a fixed position
================
*/
void G_SetOrigin( gentity_t *ent, vec3_t origin )
{
	VectorCopy( origin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.pos.trTime = 0;
	ent->s.pos.trDuration = 0;
	VectorClear( ent->s.pos.trDelta );

	VectorCopy( origin, ent->r.currentOrigin );
	VectorCopy( origin, ent->s.origin );
}

// from quakestyle.telefragged.com
// (NOBODY): Code helper function
//
gentity_t *G_FindRadius( gentity_t *from, vec3_t org, float rad )
{
	vec3_t eorg;
	int    j;

	if ( !from )
	{
		from = g_entities;
	}
	else
	{
		from++;
	}

	for ( ; from < &g_entities[ level.num_entities ]; from++ )
	{
		if ( !from->inuse )
		{
			continue;
		}

		for ( j = 0; j < 3; j++ )
		{
			eorg[ j ] = org[ j ] - ( from->r.currentOrigin[ j ] + ( from->r.mins[ j ] + from->r.maxs[ j ] ) * 0.5 );
		}

		if ( VectorLength( eorg ) > rad )
		{
			continue;
		}

		return from;
	}

	return NULL;
}

/*
===============
G_Visible

Test for a LOS between two entities
===============
*/
qboolean G_Visible( gentity_t *ent1, gentity_t *ent2, int contents )
{
	trace_t trace;

	trap_Trace( &trace, ent1->s.pos.trBase, NULL, NULL, ent2->s.pos.trBase,
	            ent1->s.number, contents );

	return trace.fraction >= 1.0f || trace.entityNum == ent2 - g_entities;
}

/*
===============
G_ClosestEnt

Test a list of entities for the closest to a particular point
===============
*/
gentity_t *G_ClosestEnt( vec3_t origin, gentity_t **entities, int numEntities )
{
	int       i;
	float     nd, d;
	gentity_t *closestEnt;

	if ( numEntities <= 0 )
	{
		return NULL;
	}

	closestEnt = entities[ 0 ];
	d = DistanceSquared( origin, closestEnt->s.origin );

	for ( i = 1; i < numEntities; i++ )
	{
		gentity_t *ent = entities[ i ];

		nd = DistanceSquared( origin, ent->s.origin );

		if ( nd < d )
		{
			d = nd;
			closestEnt = ent;
		}
	}

	return closestEnt;
}

/*
===============
G_TriggerMenu

Trigger a menu on some client
===============
*/
void G_TriggerMenu( int clientNum, dynMenu_t menu )
{
	char buffer[ 32 ];

	Com_sprintf( buffer, sizeof( buffer ), "servermenu %d", menu );
	trap_SendServerCommand( clientNum, buffer );
}

/*
===============
G_TriggerMenuArgs

Trigger a menu on some client and passes an argument
===============
*/
void G_TriggerMenuArgs( int clientNum, dynMenu_t menu, int arg )
{
	char buffer[ 64 ];

	Com_sprintf( buffer, sizeof( buffer ), "servermenu %d %d", menu, arg );
	trap_SendServerCommand( clientNum, buffer );
}

/*
===============
G_CloseMenus

Close all open menus on some client
===============
*/
void G_CloseMenus( int clientNum )
{
	char buffer[ 32 ];

	Com_sprintf( buffer, 32, "serverclosemenus" );
	trap_SendServerCommand( clientNum, buffer );
}

/*
===============
G_AddressParse

Make an IP address more usable
===============
*/
static const char *addr4parse( const char *str, addr_t *addr )
{
	int i;
	int octet = 0;
	int num = 0;
	memset( addr, 0, sizeof( addr_t ) );
	addr->type = IPv4;

	for ( i = 0; octet < 4; i++ )
	{
		if ( isdigit( str[ i ] ) )
		{
			num = num * 10 + str[ i ] - '0';
		}
		else
		{
			if ( num < 0 || num > 255 )
			{
				return NULL;
			}

			addr->addr[ octet ] = ( byte ) num;
			octet++;

			if ( str[ i ] != '.' || str[ i + 1 ] == '.' )
			{
				break;
			}

			num = 0;
		}
	}

	if ( octet < 1 )
	{
		return NULL;
	}

	return str + i;
}

static const char *addr6parse( const char *str, addr_t *addr )
{
	int      i;
	qboolean seen = qfalse;

	/* keep track of the parts before and after the ::
	   it's either this or even uglier hacks */
	byte   a[ ADDRLEN ], b[ ADDRLEN ];
	size_t before = 0, after = 0;
	int    num = 0;

	/* 8 hexadectets unless :: is present */
	for ( i = 0; before + after <= 8; i++ )
	{
		//num = num << 4 | str[ i ] - '0';
		if ( isdigit( str[ i ] ) )
		{
			num = num * 16 + str[ i ] - '0';
		}
		else if ( str[ i ] >= 'A' && str[ i ] <= 'F' )
		{
			num = num * 16 + 10 + str[ i ] - 'A';
		}
		else if ( str[ i ] >= 'a' && str[ i ] <= 'f' )
		{
			num = num * 16 + 10 + str[ i ] - 'a';
		}
		else
		{
			if ( num < 0 || num > 65535 )
			{
				return NULL;
			}

			if ( i == 0 )
			{
				//
			}
			else if ( seen ) // :: has been seen already
			{
				b[ after * 2 ] = num >> 8;
				b[ after * 2 + 1 ] = num & 0xff;
				after++;
			}
			else
			{
				a[ before * 2 ] = num >> 8;
				a[ before * 2 + 1 ] = num & 0xff;
				before++;
			}

			if ( !str[ i ] )
			{
				break;
			}

			if ( str[ i ] != ':' || before + after == 8 )
			{
				break;
			}

			if ( str[ i + 1 ] == ':' )
			{
				// ::: or multiple ::
				if ( seen || str[ i + 2 ] == ':' )
				{
					break;
				}

				seen = qtrue;
				i++;
			}
			else if ( i == 0 ) // starts with : but not ::
			{
				return NULL;
			}

			num = 0;
		}
	}

	if ( seen )
	{
		// there have to be fewer than 8 hexadectets when :: is present
		if ( before + after == 8 )
		{
			return NULL;
		}
	}
	else if ( before + after < 8 ) // require exactly 8 hexadectets
	{
		return NULL;
	}

	memset( addr, 0, sizeof( addr_t ) );
	addr->type = IPv6;

	if ( before )
	{
		memcpy( addr->addr, a, before * 2 );
	}

	if ( after )
	{
		memcpy( addr->addr + ADDRLEN - 2 * after, b, after * 2 );
	}

	return str + i;
}

qboolean G_AddressParse( const char *str, addr_t *addr )
{
	const char *p;
	int        max;

	if ( strchr( str, ':' ) )
	{
		p = addr6parse( str, addr );
		max = 128;
	}
	else if ( strchr( str, '.' ) )
	{
		p = addr4parse( str, addr );
		max = 32;
	}
	else
	{
		return qfalse;
	}

	Q_strncpyz( addr->str, str, sizeof( addr->str ) );

	if ( !p )
	{
		return qfalse;
	}

	if ( *p == '/' )
	{
		addr->mask = atoi( p + 1 );

		if ( addr->mask < 1 || addr->mask > max )
		{
			addr->mask = max;
		}
	}
	else
	{
		if ( *p )
		{
			return qfalse;
		}

		addr->mask = max;
	}

	return qtrue;
}

/*
===============
G_AddressCompare

Based largely on NET_CompareBaseAdrMask from ioq3 revision 1557
===============
*/
qboolean G_AddressCompare( const addr_t *a, const addr_t *b )
{
	int i, netmask;

	if ( a->type != b->type )
	{
		return qfalse;
	}

	netmask = a->mask;

	if ( a->type == IPv4 )
	{
		if ( netmask < 1 || netmask > 32 )
		{
			netmask = 32;
		}
	}
	else if ( a->type == IPv6 )
	{
		if ( netmask < 1 || netmask > 128 )
		{
			netmask = 128;
		}
	}

	for ( i = 0; netmask > 7; i++, netmask -= 8 )
	{
		if ( a->addr[ i ] != b->addr[ i ] )
		{
			return qfalse;
		}
	}

	if ( netmask )
	{
		netmask = ( ( 1 << netmask ) - 1 ) << ( 8 - netmask );
		return ( a->addr[ i ] & netmask ) == ( b->addr[ i ] & netmask );
	}

	return qtrue;
}
