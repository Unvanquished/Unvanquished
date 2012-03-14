/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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

/*QUAKED func_group (0 0 0) ?
Used to group brushes together just for editor convenience.  They are turned into normal brushes by the utilities.
*/

/*QUAKED info_null (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for calculations in the utilities (spotlights, etc), but removed during gameplay.
*/
void SP_info_null( gentity_t *self )
{
	G_FreeEntity( self );
}

/*QUAKED info_notnull (0 0.5 0) (-4 -4 -4) (4 4 4)
Used as a positional target for in-game calculation, like jumppad targets.
target_position does the same thing
*/
void SP_info_notnull( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
}

/*QUAKED light (0 1 0) (-8 -8 -8) (8 8 8) linear
Non-displayed light.
"light" overrides the default 300 intensity.
Linear checbox gives linear falloff instead of inverse square
Lights pointed at a target will be spotlights.
"radius" overrides the default 64 unit radius of a spotlight at the target point.
*/
void SP_light( gentity_t *self )
{
	G_FreeEntity( self );
}

/*
=================================================================================

TELEPORTERS

=================================================================================
*/

void TeleportPlayer( gentity_t *player, vec3_t origin, vec3_t angles )
{
	// unlink to make sure it can't possibly interfere with G_KillBox
	trap_UnlinkEntity( player );

	VectorCopy( origin, player->client->ps.origin );
	player->client->ps.origin[ 2 ] += 1;

	// spit the player out
	AngleVectors( angles, player->client->ps.velocity, NULL, NULL );
	VectorScale( player->client->ps.velocity, 400, player->client->ps.velocity );
	player->client->ps.pm_time = 160; // hold time
	player->client->ps.pm_flags |= PMF_TIME_KNOCKBACK;

	// toggle the teleport bit so the client knows to not lerp
	player->client->ps.eFlags ^= EF_TELEPORT_BIT;

	// set angles
	SetClientViewAngle( player, angles );

	// kill anything at the destination
	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR )
	{
		G_KillBox( player );
	}

	// save results of pmove
	BG_PlayerStateToEntityState( &player->client->ps, &player->s, qtrue );

	// use the precise origin for linking
	VectorCopy( player->client->ps.origin, player->r.currentOrigin );

	if ( player->client->sess.sessionTeam != TEAM_SPECTATOR )
	{
		trap_LinkEntity( player );
	}
}

/*QUAKED misc_teleporter_dest (1 0 0) (-32 -32 -24) (32 32 -16)
Point teleporters at these.
Now that we don't have teleport destination pads, this is just
an info_notnull
*/
void SP_misc_teleporter_dest( gentity_t *ent )
{
}

//===========================================================

/*QUAKED misc_model (1 0 0) (-16 -16 -16) (16 16 16)
"model"   arbitrary .md3 file to display
*/
void SP_misc_model( gentity_t *ent )
{
#if 0
	ent->s.modelindex = G_ModelIndex( ent->model );
	VectorSet( ent->mins, -16, -16, -16 );
	VectorSet( ent->maxs, 16, 16, 16 );
	trap_LinkEntity( ent );

	G_SetOrigin( ent, ent->s.origin );
	VectorCopy( ent->s.angles, ent->s.apos.trBase );
#else
	G_FreeEntity( ent );
#endif
}

//===========================================================

void locateCamera( gentity_t *ent )
{
	vec3_t    dir;
	gentity_t *target;
	gentity_t *owner;

	owner = G_PickTarget( ent->target );

	if ( !owner )
	{
		G_Printf( "Couldn't find target for misc_partal_surface\n" );
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
		ent->s.powerups = 0;
	}
	else
	{
		ent->s.powerups = 1;
	}

	// clientNum holds the rotate offset
	ent->s.clientNum = owner->s.clientNum;

	VectorCopy( owner->s.origin, ent->s.origin2 );

	// see if the portal_camera has a target
	target = G_PickTarget( owner->target );

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

/*QUAKED misc_portal_surface (0 0 1) (-8 -8 -8) (8 8 8)
The portal surface nearest this entity will show a view from the targeted misc_portal_camera, or a mirror view if untargeted.
This must be within 64 world units of the surface!
*/
void SP_misc_portal_surface( gentity_t *ent )
{
	VectorClear( ent->r.mins );
	VectorClear( ent->r.maxs );
	trap_LinkEntity( ent );

	ent->r.svFlags = SVF_PORTAL;
	ent->s.eType = ET_PORTAL;

	if ( !ent->target )
	{
		VectorCopy( ent->s.origin, ent->s.origin2 );
	}
	else
	{
		ent->think = locateCamera;
		ent->nextthink = level.time + 100;
	}
}

/*QUAKED misc_portal_camera (0 0 1) (-8 -8 -8) (8 8 8) slowrotate fastrotate noswing

The target for a misc_portal_director.  You can set either angles or target another entity to determine the direction of view.
"roll" an angle modifier to orient the camera around the target vector;
*/
void SP_misc_portal_camera( gentity_t *ent )
{
	float roll;

	VectorClear( ent->r.mins );
	VectorClear( ent->r.maxs );
	trap_LinkEntity( ent );

	G_SpawnFloat( "roll", "0", &roll );

	ent->s.clientNum = roll / 360.0f * 256;
}

/*
======================================================================

  NEAT EFFECTS AND STUFF FOR TREMULOUS

======================================================================
*/

/*
===============
SP_use_particle_system

Use function for particle_system
===============
*/
void SP_use_particle_system( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	//toggle EF_NODRAW
	self->s.eFlags ^= EF_NODRAW;
}

/*
===============
SP_spawn_particle_system

Spawn function for particle system
===============
*/
void SP_misc_particle_system( gentity_t *self )
{
	char *s;

	G_SetOrigin( self, self->s.origin );

	G_SpawnString( "psName", "", &s );

	//add the particle system to the client precache list
	self->s.modelindex = G_ParticleSystemIndex( s );

	if ( self->spawnflags & 1 )
	{
		self->s.eFlags |= EF_NODRAW;
	}

	self->use = SP_use_particle_system;
	self->s.eType = ET_PARTICLE_SYSTEM;
	trap_LinkEntity( self );
}

/*
===============
SP_use_anim_model

Use function for anim model
===============
*/
void SP_use_anim_model( gentity_t *self, gentity_t *other, gentity_t *activator )
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

/*
===============
SP_misc_anim_model

Spawn function for anim model
===============
*/
void SP_misc_anim_model( gentity_t *self )
{
	self->s.powerups = ( int ) self->animation[ 0 ];
	self->s.weapon = ( int ) self->animation[ 1 ];
	self->s.torsoAnim = ( int ) self->animation[ 2 ];
	self->s.legsAnim = ( int ) self->animation[ 3 ];

	self->s.angles2[ 0 ] = self->pos2[ 0 ];

	//add the model to the client precache list
	self->s.modelindex = G_ModelIndex( self->model );

	self->use = SP_use_anim_model;

	self->s.eType = ET_ANIMMAPOBJ;

	trap_LinkEntity( self );
}

/*
===============
SP_use_light_flare

Use function for light flare
===============
*/
void SP_use_light_flare( gentity_t *self, gentity_t *other, gentity_t *activator )
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
void SP_misc_light_flare( gentity_t *self )
{
	self->s.eType = ET_LIGHTFLARE;
	self->s.modelindex = G_ShaderIndex( self->targetShaderName );
	VectorCopy( self->pos2, self->s.origin2 );

	//try to find a spot near to the flare which is empty. This
	//is used to facilitate visibility testing
	findEmptySpot( self->s.origin, 8.0f, self->s.angles2 );

	self->use = SP_use_light_flare;

	G_SpawnFloat( "speed", "200", &self->speed );
	self->s.time = self->speed;

	G_SpawnInt( "mindist", "0", &self->s.generic1 );

	if ( self->spawnflags & 1 )
	{
		self->s.eFlags |= EF_NODRAW;
	}

	trap_LinkEntity( self );
}
