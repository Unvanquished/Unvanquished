/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "sg_local.h"
#include "sg_spawn.h"

#include <glm/geometric.hpp>
/*
======================================================================

  Particle System

======================================================================
*/

static void gfx_particle_system_toggle( gentity_t *self )
{
	//toggle EF_NODRAW
	self->s.eFlags ^= EF_NODRAW;

	self->nextthink = 0;
}

static void gfx_particle_system_act( gentity_t *self, gentity_t*, gentity_t* )
{
	gfx_particle_system_toggle( self );

	if ( self->mapEntity.config.wait.time > 0.0f )
	{
		self->think = gfx_particle_system_toggle;
		self->nextthink = level.time + ( int )( self->mapEntity.config.wait.time * 1000 );
	}
}

void SP_gfx_particle_system( gentity_t *self )
{
	char *s;

	G_SetOrigin( self, VEC2GLM( self->s.origin ) );
	VectorCopy( self->s.angles, self->s.apos.trBase );

	G_SpawnString( "psName", "", &s );

	//add the particle system to the client precache list
	self->s.modelindex = G_ParticleSystemIndex( s );

	if ( self->mapEntity.spawnflags & SPF_SPAWN_DISABLED )
	{
		self->s.eFlags |= EF_NODRAW;
	}

	self->act = gfx_particle_system_act;
	self->s.eType = entityType_t::ET_PARTICLE_SYSTEM;
	trap_LinkEntity( self );
}

/*
=================================================================================

Light Flare

=================================================================================
*/
static void gfx_light_flare_toggle( gentity_t *self, gentity_t*, gentity_t* )
{
	self->s.eFlags ^= EF_NODRAW;
}

/*
===============
findEmptySpot

Finds an empty spot radius units from origin
==============
*/
static void findEmptySpot( glm::vec3 const& origin, float radius, glm::vec3& spot )
{
	glm::vec3 total = {};
	//54(!) traces to test for empty spots
	for ( int i = -1; i <= 1; i++ )
	{
		for ( int j = -1; j <= 1; j++ )
		{
			for ( int k = -1; k <= 1; k++ )
			{
				glm::vec3 delta( i, j, k );
				delta *= radius;

				glm::vec3 test = origin + delta;

				trace_t trace;
				trap_Trace( &trace, &test[0], nullptr, nullptr, &test[0], ENTITYNUM_NONE, MASK_SOLID, 0 );

				if ( !trace.allsolid )
				{
					trap_Trace( &trace, &test[0], nullptr, nullptr, &origin[0], ENTITYNUM_NONE, MASK_SOLID, 0 );
					delta *= trace.fraction;
					total += delta;
				}
			}
		}
	}

	spot = origin + glm::normalize( total ) * radius;
}

void SP_gfx_light_flare( gentity_t *self )
{
	if ( !self->mapEntity.shaderKey )
	{
		Log::Warn( "Light flare entity %d at (%d, %d, %d) has no shader", self->num(), self->s.origin[0], self->s.origin[1], self->s.origin[2] );
		return;
	}

	self->s.eType = entityType_t::ET_LIGHTFLARE;
	self->s.modelindex = G_ShaderIndex( self->mapEntity.shaderKey );
	VectorCopy( self->mapEntity.activatedPosition, self->s.origin2 );

	//try to find a spot near to the flare which is empty. This
	//is used to facilitate visibility testing
	glm::vec3 angles2 = VEC2GLM( self->s.angles2 );
	findEmptySpot( VEC2GLM( self->s.origin ), 8.0f, angles2 );
	VectorCopy( &angles2[0], self->s.angles2 );

	self->act = gfx_light_flare_toggle;

	if( !self->mapEntity.config.speed )
		self->mapEntity.config.speed = 200;

	self->s.time = self->mapEntity.config.speed;

	G_SpawnInt( "mindist", "0", &self->s.generic1 );

	if ( self->mapEntity.spawnflags & SPF_SPAWN_DISABLED )
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
static void gfx_portal_locateCamera( gentity_t *self )
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

	self->r.ownerNum = owner->num();

	// frame holds the rotate speed
	if ( owner->mapEntity.spawnflags & 1 )
	{
		self->s.frame = 25;
	}
	else if ( owner->mapEntity.spawnflags & 2 )
	{
		self->s.frame = 75;
	}

	// swing camera ?
	if ( owner->mapEntity.spawnflags & 4 )
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
		glm::vec3 angles = VEC2GLM( owner->s.angles );
		glm::vec3 dir_ = VEC2GLM( dir );
		G_SetMovedir( angles, dir_ );
		VectorCopy( &angles[0], owner->s.angles );
		VectorCopy( &dir_[0], dir );
	}

	self->s.eventParm = DirToByte( dir );
}

void SP_gfx_portal_surface( gentity_t *self )
{
	VectorClear( self->r.mins );
	VectorClear( self->r.maxs );
	trap_LinkEntity( self );

	self->r.svFlags = SVF_PORTAL;
	self->s.eType = entityType_t::ET_PORTAL;

	if ( !self->mapEntity.targetCount )
	{
		VectorCopy( self->s.origin, self->s.origin2 );
	}
	else
	{
		self->think = gfx_portal_locateCamera;
		self->nextthink = level.time + 100;
	}
}

void SP_gfx_portal_camera( gentity_t *self )
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
static void gfx_animated_model_act( gentity_t *self, gentity_t*, gentity_t* )
{
	if ( self->mapEntity.spawnflags & 1 )
	{
		//if spawnflag 1 is set
		//toggle EF_NODRAW
		self->s.eFlags ^= EF_NODRAW;
	}
	else
	{
		//if the animation loops then toggle the animation
		//toggle EF_MOVER_STOP
		self->s.eFlags ^= EF_MOVER_STOP;
	}
}

void SP_gfx_animated_model( gentity_t *self )
{
	self->s.misc = ( int ) self->mapEntity.animation[ 0 ];
	self->s.weapon = ( int ) self->mapEntity.animation[ 1 ];
	self->s.torsoAnim = ( int ) self->mapEntity.animation[ 2 ];
	self->s.legsAnim = ( int ) self->mapEntity.animation[ 3 ];

	self->s.angles2[ 0 ] = self->mapEntity.activatedPosition[ 0 ];

	//add the model to the client precache list
	self->s.modelindex = G_ModelIndex( self->mapEntity.model );

	self->act = gfx_animated_model_act;

	self->s.eType = entityType_t::ET_ANIMMAPOBJ;

	// spawn with animation stopped
	if ( self->mapEntity.spawnflags & 2 )
	{
		self->s.eFlags |= EF_MOVER_STOP;
	}

	trap_LinkEntity( self );
}

/*
=================================================================================

gfx_shader_mod

=================================================================================
*/

static void gfx_shader_mod_act( gentity_t *self, gentity_t*, gentity_t* )
{
	if ( !self->mapEntity.shaderKey || !self->mapEntity.shaderReplacement || !self->enabled )
	{
		return;
	}

	G_SetShaderRemap( self->mapEntity.shaderKey, self->mapEntity.shaderReplacement, level.time * 0.001 );
	trap_SetConfigstring( CS_SHADERSTATE, BuildShaderStateConfig() );

	self->shaderActive = true;
}

static void gfx_shader_mod_reset( gentity_t *self )
{
	if ( !self->mapEntity.shaderKey || !self->mapEntity.shaderReplacement )
	{
		return;
	}

	if( !self->shaderActive ) // initial reset doesn't need a remap
	{
		return;
	}

	G_SetShaderRemap( self->mapEntity.shaderKey, self->mapEntity.shaderKey, level.time * 0.001 );
	trap_SetConfigstring( CS_SHADERSTATE, BuildShaderStateConfig() );

	self->shaderActive = false;
}

void SP_gfx_shader_mod( gentity_t *self )
{
	self->act = gfx_shader_mod_act;
	self->reset = gfx_shader_mod_reset;
}
