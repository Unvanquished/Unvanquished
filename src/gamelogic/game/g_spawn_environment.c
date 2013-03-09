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

/*
=================================================================================

env_speaker

=================================================================================
*/

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
	ent->s.frame = ent->config.wait.time * 10;
	ent->s.clientNum = ent->config.wait.variance * 10;

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
=================================================================================

env_rumble

=================================================================================
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
		ent->client->ps.velocity[ 2 ] = self->speed.current;
	}

	if ( level.time < self->timestamp )
	{
		self->nextthink = level.time + FRAMETIME;
	}
}

void env_rumble_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	self->timestamp = level.time + ( self->customNumber * FRAMETIME );
	self->nextthink = level.time + FRAMETIME;
	self->activator = activator;
	self->last_move_time = 0;
}

void SP_env_rumble( gentity_t *self )
{
	if ( !self->names[ 0 ] )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: untargeted %s at %s\n", self->classname,
		          vtos( self->s.origin ) );
	}

	if ( !self->customNumber )
	{
		self->customNumber = 10;
	}

	if ( !self->speed.current )
	{
		self->speed.current = 100;
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

void env_particle_system_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	env_particle_system_toggle( self );

	if ( self->config.wait.time > 0.0f )
	{
		self->think = env_particle_system_toggle;
		self->nextthink = level.time + ( int )( self->config.wait.time * 1000 );
	}
}

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
=================================================================================

env_lens_flare

=================================================================================
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

void SP_env_lens_flare( gentity_t *self )
{
	self->s.eType = ET_LIGHTFLARE;
	self->s.modelindex = G_ShaderIndex( self->targetShaderName );
	VectorCopy( self->activatedPosition, self->s.origin2 );

	//try to find a spot near to the flare which is empty. This
	//is used to facilitate visibility testing
	findEmptySpot( self->s.origin, 8.0f, self->s.angles2 );

	self->use = env_lens_flare_toggle;

	if( !self->speed.current )
		self->speed.current = 200;

	self->s.time = self->speed.current;

	G_SpawnInt( "mindist", "0", &self->s.generic1 );

	if ( self->spawnflags & 1 )
	{
		self->s.eFlags |= EF_NODRAW;
	}

	trap_LinkEntity( self );
}

/*
=================================================================================

env_portal_*

=================================================================================
*/
void env_portal_locateCamera( gentity_t *ent )
{
	vec3_t    dir;
	gentity_t *target;
	gentity_t *owner;

	owner = G_PickRandomTargetFor( ent );

	if ( !owner )
	{
		G_FreeEntity( ent );
		return;
	}

	ent->r.ownerNum = owner->s.number;

	// frame holds the rotate speed
	if ( owner->spawnflags & 1 )
	{
		ent->s.frame = 25;
	}
	else if ( owner->spawnflags & 2 )
	{
		ent->s.frame = 75;
	}

	// swing camera ?
	if ( owner->spawnflags & 4 )
	{
		// set to 0 for no rotation at all
		ent->s.misc = 0;
	}
	else
	{
		ent->s.misc = 1;
	}

	// clientNum holds the rotate offset
	ent->s.clientNum = owner->s.clientNum;

	VectorCopy( owner->s.origin, ent->s.origin2 );

	// see if the portal_camera has a target
	target = G_PickRandomTargetFor( owner );

	if ( target )
	{
		VectorSubtract( target->s.origin, owner->s.origin, dir );
		VectorNormalize( dir );
	}
	else
	{
		G_SetMovedir( owner->s.angles, dir );
	}

	ent->s.eventParm = DirToByte( dir );
}

void SP_env_portal_surface( gentity_t *ent )
{
	VectorClear( ent->r.mins );
	VectorClear( ent->r.maxs );
	trap_LinkEntity( ent );

	ent->r.svFlags = SVF_PORTAL;
	ent->s.eType = ET_PORTAL;

	if ( !ent->targets[ 0 ].name )
	{
		VectorCopy( ent->s.origin, ent->s.origin2 );
	}
	else
	{
		ent->think = env_portal_locateCamera;
		ent->nextthink = level.time + 100;
	}
}

void SP_env_portal_camera( gentity_t *ent )
{
	float roll;

	VectorClear( ent->r.mins );
	VectorClear( ent->r.maxs );
	trap_LinkEntity( ent );

	G_SpawnFloat( "roll", "0", &roll );

	ent->s.clientNum = roll / 360.0f * 256;
}

/*
=================================================================================

env_animated_model

=================================================================================
*/
void env_animated_model( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->spawnflags & 1 )
	{
		//if spawnflag 1 is set
		//toggle EF_NODRAW
		if ( self->s.eFlags & EF_NODRAW )
		{
			self->s.eFlags &= ~EF_NODRAW;
		}
		else
		{
			self->s.eFlags |= EF_NODRAW;
		}
	}
	else
	{
		//if the animation loops then toggle the animation
		//toggle EF_MOVER_STOP
		if ( self->s.eFlags & EF_MOVER_STOP )
		{
			self->s.eFlags &= ~EF_MOVER_STOP;
		}
		else
		{
			self->s.eFlags |= EF_MOVER_STOP;
		}
	}
}

void SP_env_animated_model( gentity_t *self )
{
	self->s.misc = ( int ) self->animation[ 0 ];
	self->s.weapon = ( int ) self->animation[ 1 ];
	self->s.torsoAnim = ( int ) self->animation[ 2 ];
	self->s.legsAnim = ( int ) self->animation[ 3 ];

	self->s.angles2[ 0 ] = self->activatedPosition[ 0 ];

	//add the model to the client precache list
	self->s.modelindex = G_ModelIndex( self->model );

	self->use = env_animated_model;

	self->s.eType = ET_ANIMMAPOBJ;

	// spawn with animation stopped
	if ( self->spawnflags & 2 )
	{
		self->s.eFlags |= EF_MOVER_STOP;
	}

	trap_LinkEntity( self );
}

