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

#include "g_local.h"
#include "g_spawn.h"

void env_speaker_use( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if ( ent->spawnflags & 3 )
	{
		// looping sound toggles
		if ( ent->s.loopSound )
		{
			ent->s.loopSound = 0; // turn it off
		}
		else
		{
			ent->s.loopSound = ent->soundIndex; // start it
		}
	}
	else
	{
		// one-time sound
		if ( (ent->spawnflags & 8) && activator )
		{
			G_AddEvent( activator, EV_GENERAL_SOUND, ent->soundIndex );
		}
		else if ( ent->spawnflags & 4 )
		{
			G_AddEvent( ent, EV_GLOBAL_SOUND, ent->soundIndex );
		}
		else
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->soundIndex );
		}
	}
}

/**
 * Warning: The following comment contains information, that might be parsed and used by radiant based mapeditors.
 */
/*QUAKED target_speaker (0 .7 .7) (-8 -8 -8) (8 8 8) LOOPED_ON LOOPED_OFF GLOBAL ACTIVATOR
Sound generating entity that plays .wav files. Normal non-looping sounds play each time the target_speaker is triggered. Looping sounds can be set to play by themselves (no activating trigger) or be toggled on/off by a trigger.
-------- KEYS --------
noise: path/name of .wav file to play (eg. sound/world/growl1.wav - see Notes).
wait: delay in seconds between each time the sound is played ("random" key must be set - see Notes).
random: random time variance in seconds added or subtracted from "wait" delay ("wait" key must be set - see Notes).
targetname, targetname2, targetname3, targetname3: the activating button or trigger points to one of these.
-------- SPAWNFLAGS --------
LOOPED_ON: sound will loop and initially start on in level (will toggle on/off when triggered).
LOOPED_OFF: sound will loop and initially start off in level (will toggle on/off when triggered).
GLOBAL: sound will play full volume throughout the level.
ACTIVATOR: sound will play only for the player that activated the target.
-------- NOTES --------
The path portion value of the "noise" key can be replaced by the implicit folder character "*" for triggered sounds that belong to a particular player model. For example, if you want to create a "bottomless pit" in which the player screams and dies when he falls into, you would place a trigger_multiple over the floor of the pit and target a target_speaker with it. Then, you would set the "noise" key to "*falling1.wav". The * character means the current player model's sound folder. So if your current player model is Visor, * = sound/player/visor, if your current player model is Sarge, * = sound/player/sarge, etc. This cool feature provides an excellent way to create "player-specific" triggered sounds in your levels.
The combination of the "wait" and "random" keys can be used to play non-looping sounds without requiring an activating trigger but both keys must be used together. The value of the "random" key is used to calculate a minimum and a maximum delay. The final time delay will be a random value anywhere between the minimum and maximum values: (min delay = wait - random) (max delay = wait + random).
*/
void SP_env_speaker( gentity_t *ent )
{
	char buffer[ MAX_QPATH ];
	char *s;

	if ( !G_SpawnString( "noise", "NOSOUND", &s ) )
	{
		G_Error( "target_speaker without a noise key at %s", vtos( ent->s.origin ) );
	}

	// force all client-relative sounds to be "activator" speakers that
	// play on the entity that activates the speaker
	if ( s[ 0 ] == '*' )
	{
		ent->spawnflags |= 8;
	}

	if ( !strstr( s, ".wav" ) )
	{
		Com_sprintf( buffer, sizeof( buffer ), "%s.wav", s );
	}
	else
	{
		Q_strncpyz( buffer, s, sizeof( buffer ) );
	}

	ent->soundIndex = G_SoundIndex( buffer );

	// a repeating speaker can be done completely client side
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->soundIndex;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->waitVariance * 10;

	// check for prestarted looping sound
	if ( ent->spawnflags & 1 )
	{
		ent->s.loopSound = ent->soundIndex;
	}

	ent->use = env_speaker_use;

	if ( ent->spawnflags & 4 )
	{
		ent->r.svFlags |= SVF_BROADCAST;
	}

	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity( ent );
}

/*
===============
target_rumble_think
===============
*/
void env_rumble_think( gentity_t *self )
{
	int       i;
	gentity_t *ent;

	if ( self->last_move_time < level.time )
	{
		self->last_move_time = level.time + 0.5;
	}

	for ( i = 0, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( !ent->inuse )
		{
			continue;
		}

		if ( !ent->client )
		{
			continue;
		}

		if ( ent->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{
			continue;
		}

		ent->client->ps.groundEntityNum = ENTITYNUM_NONE;
		ent->client->ps.velocity[ 0 ] += crandom() * 150;
		ent->client->ps.velocity[ 1 ] += crandom() * 150;
		ent->client->ps.velocity[ 2 ] = self->speed;
	}

	if ( level.time < self->timestamp )
	{
		self->nextthink = level.time + FRAMETIME;
	}
}

/*
===============
target_rumble_use
===============
*/
void env_rumble_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	self->timestamp = level.time + ( self->count * FRAMETIME );
	self->nextthink = level.time + FRAMETIME;
	self->activator = activator;
	self->last_move_time = 0;
}

/*
===============
SP_target_rumble
===============
*/
void SP_env_rumble( gentity_t *self )
{
	if ( !self->targetnames[ 0 ] )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: untargeted %s at %s\n", self->classname,
		          vtos( self->s.origin ) );
	}

	if ( !self->count )
	{
		self->count = 10;
	}

	if ( !self->speed )
	{
		self->speed = 100;
	}

	self->think = env_rumble_think;
	self->use = env_rumble_use;
}

/*
======================================================================

  Particle System

======================================================================
*/

void env_particle_system_toggle( gentity_t *self )
{
	//toggle EF_NODRAW
	self->s.eFlags ^= EF_NODRAW;

	self->nextthink = 0;
}

/*
===============
SP_use_particle_system

Use function for particle_system
===============
*/
void env_particle_system_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	env_particle_system_toggle( self );

	if ( self->wait > 0.0f )
	{
		self->think = env_particle_system_toggle;
		self->nextthink = level.time + ( int )( self->wait * 1000 );
	}
}

/*
===============
SP_spawn_particle_system

Spawn function for particle system
===============
*/
void SP_env_particle_system( gentity_t *self )
{
	char *s;

	G_SetOrigin( self, self->s.origin );
	VectorCopy( self->s.angles, self->s.apos.trBase );

	G_SpawnString( "psName", "", &s );

	//add the particle system to the client precache list
	self->s.modelindex = G_ParticleSystemIndex( s );

	if ( self->spawnflags & 1 )
	{
		self->s.eFlags |= EF_NODRAW;
	}

	self->use = env_particle_system_use;
	self->s.eType = ET_PARTICLE_SYSTEM;
	trap_LinkEntity( self );
}

/*
===============
SP_use_light_flare

Use function for light flare
===============
*/
void env_lens_flare_toggle( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	self->s.eFlags ^= EF_NODRAW;
}

/*
===============
findEmptySpot

Finds an empty spot radius units from origin
==============
*/
static void findEmptySpot( vec3_t origin, float radius, vec3_t spot )
{
	int     i, j, k;
	vec3_t  delta, test, total;
	trace_t tr;

	VectorClear( total );

	//54(!) traces to test for empty spots
	for ( i = -1; i <= 1; i++ )
	{
		for ( j = -1; j <= 1; j++ )
		{
			for ( k = -1; k <= 1; k++ )
			{
				VectorSet( delta, ( i * radius ),
				           ( j * radius ),
				           ( k * radius ) );

				VectorAdd( origin, delta, test );

				trap_Trace( &tr, test, NULL, NULL, test, -1, MASK_SOLID );

				if ( !tr.allsolid )
				{
					trap_Trace( &tr, test, NULL, NULL, origin, -1, MASK_SOLID );
					VectorScale( delta, tr.fraction, delta );
					VectorAdd( total, delta, total );
				}
			}
		}
	}

	VectorNormalize( total );
	VectorScale( total, radius, total );
	VectorAdd( origin, total, spot );
}

/*
===============
SP_misc_light_flare

Spawn function for light flare
===============
*/
void SP_env_lens_flare( gentity_t *self )
{
	self->s.eType = ET_LIGHTFLARE;
	self->s.modelindex = G_ShaderIndex( self->targetShaderName );
	VectorCopy( self->pos2, self->s.origin2 );

	//try to find a spot near to the flare which is empty. This
	//is used to facilitate visibility testing
	findEmptySpot( self->s.origin, 8.0f, self->s.angles2 );

	self->use = env_lens_flare_toggle;

	if( !self->speed )
		self->speed = 200;

	self->s.time = self->speed;

	G_SpawnInt( "mindist", "0", &self->s.generic1 );

	if ( self->spawnflags & 1 )
	{
		self->s.eFlags |= EF_NODRAW;
	}

	trap_LinkEntity( self );
}
