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

void G_SetShaderRemap( const char *oldShader, const char *newShader, float timeOffset )
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
	char        out[ MAX_QPATH * 2 + 5 ];
	int         i;

	memset( buff, 0, MAX_STRING_CHARS );

	for ( i = 0; i < remapCount; i++ )
	{
		Com_sprintf( out, sizeof( out ), "%s=%s:%5.2f@", remappedShaders[ i ].oldShader,
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

/**
 * searches for a the grading texture with the given name among the configstrings and returns the index
 * if it wasn't found it will add the texture to the configstrings, send these to the client and return the new index
 *
 * the first one at CS_GRADING_TEXTURES is always the global one, so we start searching from CS_GRADING_TEXTURES+1
 */
int G_GradingTextureIndex( const char *name )
{
	return G_FindConfigstringIndex( name, CS_GRADING_TEXTURES+1, MAX_GRADING_TEXTURES-1, qtrue );
}

int G_LocationIndex( const char *name )
{
	return G_FindConfigstringIndex( name, CS_LOCATIONS, MAX_LOCATIONS, qtrue );
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
=============
G_TeleportPlayer
teleports the player to another location
=============
*/
void G_TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles, float speed )
{
	// unlink to make sure it can't possibly interfere with G_KillBox
	trap_UnlinkEntity( player );

	VectorCopy( origin, player->client->ps.origin );
	player->client->ps.groundEntityNum = ENTITYNUM_NONE;
	player->client->ps.stats[ STAT_STATE ] &= ~SS_GRABBED;

	AngleVectors( angles, player->client->ps.velocity, NULL, NULL );
	VectorScale( player->client->ps.velocity, speed, player->client->ps.velocity );
	player->client->ps.pm_time = 0.4f * abs( speed ); // duration of loss of control
	if ( player->client->ps.pm_time > 160 )
		player->client->ps.pm_time = 160;
	if ( player->client->ps.pm_time != 0 )
		player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;
	G_UnlaggedClear( player );

	// cut all relevant zap beams
	G_ClearPlayerZapEffects( player );

	// set angles
	G_SetClientViewAngle( player, angles );

	// save results of pmove
	BG_PlayerStateToEntityState( &player->client->ps, &player->s, qtrue );

	// use the precise origin for linking
	VectorCopy( player->client->ps.origin, player->r.currentOrigin );

	if ( player->client->sess.spectatorState == SPECTATOR_NOT )
	{
		// kill anything at the destination
		G_KillBox( player );

		trap_LinkEntity( player );
	}
}

/*
==============
G_CopyString
==============
*/
char *G_CopyString( const char *str )
{
	size_t size = strlen( str ) + 1;
	char *cp = BG_Alloc( size );
	memcpy( cp, str, size );
	return cp;
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

/*
====================
G_KillBrushModel
====================
*/
void G_KillBrushModel( gentity_t *ent, gentity_t *activator )
{
  gentity_t *e;
  vec3_t mins, maxs;
  trace_t tr;

  for( e = &g_entities[ 0 ]; e < &g_entities[ level.num_entities ]; ++e )
  {
    if( !e->takedamage || !e->r.linked || !e->clipmask || ( e->client && e->client->noclip ) )
      continue;

    VectorAdd( e->r.currentOrigin, e->r.mins, mins );
    VectorAdd( e->r.currentOrigin, e->r.maxs, maxs );

    if( !trap_EntityContact( mins, maxs, ent ) )
      continue;

    trap_Trace( &tr, e->r.currentOrigin, e->r.mins, e->r.maxs,
                e->r.currentOrigin, e->s.number, e->clipmask );

    if( tr.entityNum != ENTITYNUM_NONE )
      G_Damage( e, ent, activator, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_CRUSH );
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
		G_Printf( S_WARNING "G_AddEvent( %s ) has eventParm %d, "
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

	ent = G_NewTempEntity( vec3_origin, event );
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

	te = G_NewTempEntity( ent->r.currentOrigin, EV_GENERAL_SOUND );
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

void G_AddConfidence( team_t team, confidence_t type, confidence_reason_t reason,
                      confidence_qualifier_t qualifier, float amount, gentity_t *source )
{
	float     *confidence;
	gclient_t *client = NULL;
	gentity_t *event;

	switch ( team )
	{
		case TEAM_ALIENS:
			confidence = level.alienConfidence;
			break;
		case TEAM_HUMANS:
			confidence = level.humanConfidence;
			break;
		default:
			return;
	}

	if ( type <= CONFIDENCE_SUM || type >= NUM_CONFIDENCE_TYPES )
	{
		return;
	}

	confidence[ type ] += amount;

	// If source is a client who is still on the team, notify
	if ( source )
	{
		client = source->client;

		if ( client && client->pers.teamSelection == team )
		{
			event = G_NewTempEntity( client->ps.origin, EV_CONFIDENCE );
			event->s.eventParm = reason;
			event->s.otherEntityNum = qualifier;
			event->s.otherEntityNum2 = ( int )( amount * 10.0f );
			event->r.svFlags = SVF_SINGLECLIENT;
			event->r.singleClient = client->ps.clientNum;
		}
	}
}
