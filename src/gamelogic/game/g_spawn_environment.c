/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

#include "g_local.h"
#include "g_spawn.h"

/*
=================================================================================

env_speaker

=================================================================================
*/

void env_speaker_act( gentity_t *self, gentity_t *caller, gentity_t *activator )
{
	if ( self->spawnflags & 3 )
	{
		// looping sound toggles
		if ( self->s.loopSound )
		{
			self->s.loopSound = 0; // turn it off
		}
		else
		{
			self->s.loopSound = self->soundIndex; // start it
		}
	}
	else
	{
		// one-time sound
		if ( (self->spawnflags & 8) && activator )
		{
			G_AddEvent( activator, EV_GENERAL_SOUND, self->soundIndex );
		}
		else if ( self->spawnflags & 4 )
		{
			G_AddEvent( self, EV_GLOBAL_SOUND, self->soundIndex );
		}
		else
		{
			G_AddEvent( self, EV_GENERAL_SOUND, self->soundIndex );
		}
	}
}

void SP_env_speaker( gentity_t *self )
{
	char buffer[ MAX_QPATH ];
	char *tmpString;

	if ( !G_SpawnString( "noise", "NOSOUND", &tmpString ) )
	{
		G_Error( "target_speaker without a noise key at %s", vtos( self->s.origin ) );
	}

	// force all client-relative sounds to be "activator" speakers that
	// play on the entity that activates the speaker
	if ( tmpString[ 0 ] == '*' )
	{
		self->spawnflags |= 8;
	}

	if ( !strstr( tmpString, ".wav" ) )
	{
		Com_sprintf( buffer, sizeof( buffer ), "%s.wav", tmpString );
	}
	else
	{
		Q_strncpyz( buffer, tmpString, sizeof( buffer ) );
	}

	self->soundIndex = G_SoundIndex( buffer );

	// a repeating speaker can be done completely client side
	self->s.eType = ET_SPEAKER;
	self->s.eventParm = self->soundIndex;
	self->s.frame = self->config.wait.time * 10;
	self->s.clientNum = self->config.wait.variance * 10;

	// check for prestarted looping sound
	if ( self->spawnflags & 1 )
	{
		self->s.loopSound = self->soundIndex;
	}

	self->act = env_speaker_act;

	if ( self->spawnflags & 4 )
	{
		self->r.svFlags |= SVF_BROADCAST;
	}

	VectorCopy( self->s.origin, self->s.pos.trBase );

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity( self );
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
		ent->client->ps.velocity[ 2 ] = self->config.speed;
	}

	if ( level.time < self->timestamp )
	{
		self->nextthink = level.time + FRAMETIME;
	}
}

void env_rumble_act( gentity_t *self, gentity_t *caller, gentity_t *activator )
{
	self->timestamp = level.time + ( self->config.amount * FRAMETIME );
	self->nextthink = level.time + FRAMETIME;
	self->activator = activator;
	self->last_move_time = 0;
}

void SP_env_rumble( gentity_t *self )
{
	if ( !self->config.amount )
	{
		if( G_SpawnInt( "count", "0", &self->config.amount) )
		{
			G_WarnAboutDeprecatedEntityField( self, "amount", "count", ENT_V_RENAMED );
		}
		else
		{
			self->customNumber = 10;
		}
	}

	if ( !self->config.speed )
	{
		self->config.speed = 100;
	}

	self->think = env_rumble_think;
	self->act = env_rumble_act;
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

void env_particle_system_act( gentity_t *self, gentity_t *caller, gentity_t *activator )
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

	self->act = env_particle_system_act;
	self->s.eType = ET_PARTICLE_SYSTEM;
	trap_LinkEntity( self );
}

/*
=================================================================================

env_lens_flare

=================================================================================
*/
void env_lens_flare_toggle( gentity_t *self, gentity_t *caller, gentity_t *activator )
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
	trace_t trace;

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

				trap_Trace( &trace, test, NULL, NULL, test, -1, MASK_SOLID );

				if ( !trace.allsolid )
				{
					trap_Trace( &trace, test, NULL, NULL, origin, -1, MASK_SOLID );
					VectorScale( delta, trace.fraction, delta );
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

	self->act = env_lens_flare_toggle;

	if( !self->config.speed )
		self->config.speed = 200;

	self->s.time = self->config.speed;

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
void env_portal_locateCamera( gentity_t *self )
{
	vec3_t    dir;
	gentity_t *target;
	gentity_t *owner;

	owner = G_PickRandomTargetFor( self );

	if ( !owner )
	{
		G_FreeEntity( self );
		return;
	}

	self->r.ownerNum = owner->s.number;

	// frame holds the rotate speed
	if ( owner->spawnflags & 1 )
	{
		self->s.frame = 25;
	}
	else if ( owner->spawnflags & 2 )
	{
		self->s.frame = 75;
	}

	// swing camera ?
	if ( owner->spawnflags & 4 )
	{
		// set to 0 for no rotation at all
		self->s.misc = 0;
	}
	else
	{
		self->s.misc = 1;
	}

	// clientNum holds the rotate offset
	self->s.clientNum = owner->s.clientNum;

	VectorCopy( owner->s.origin, self->s.origin2 );

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

	self->s.eventParm = DirToByte( dir );
}

void SP_env_portal_surface( gentity_t *self )
{
	VectorClear( self->r.mins );
	VectorClear( self->r.maxs );
	trap_LinkEntity( self );

	self->r.svFlags = SVF_PORTAL;
	self->s.eType = ET_PORTAL;

	if ( !self->targetCount )
	{
		VectorCopy( self->s.origin, self->s.origin2 );
	}
	else
	{
		self->think = env_portal_locateCamera;
		self->nextthink = level.time + 100;
	}
}

void SP_env_portal_camera( gentity_t *self )
{
	float roll;

	VectorClear( self->r.mins );
	VectorClear( self->r.maxs );
	trap_LinkEntity( self );

	G_SpawnFloat( "roll", "0", &roll );

	self->s.clientNum = roll / 360.0f * 256;
}

/*
=================================================================================

env_animated_model

=================================================================================
*/
void env_animated_model_act( gentity_t *self, gentity_t *other, gentity_t *activator )
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

	self->act = env_animated_model_act;

	self->s.eType = ET_ANIMMAPOBJ;

	// spawn with animation stopped
	if ( self->spawnflags & 2 )
	{
		self->s.eFlags |= EF_MOVER_STOP;
	}

	trap_LinkEntity( self );
}

