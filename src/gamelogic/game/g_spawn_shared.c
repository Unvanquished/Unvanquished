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

shared entity think functions

=================================================================================
*/

void think_fireDelayed( gentity_t *ent )
{
	G_FireAllTargetsOf( ent, ent->activator );
}

/*
=================
AimAtTarget

Calculate origin2 so the target apogee will be hit
=================
*/
void think_aimAtTarget( gentity_t *self )
{
	gentity_t *ent;
	vec3_t    origin;
	float     height, gravity, time, forward;
	float     dist;

	VectorAdd( self->r.absmin, self->r.absmax, origin );
	VectorScale( origin, 0.5, origin );

	ent = G_PickRandomTargetFor( self );

	if ( !ent )
	{
		G_FreeEntity( self );
		return;
	}

	height = ent->s.origin[ 2 ] - origin[ 2 ];
	gravity = g_gravity.value;
	time = sqrt( height / ( 0.5 * gravity ) );

	if ( !time )
	{
		G_FreeEntity( self );
		return;
	}

	// set s.origin2 to the push velocity
	VectorSubtract( ent->s.origin, origin, self->s.origin2 );
	self->s.origin2[ 2 ] = 0;
	dist = VectorNormalize( self->s.origin2 );

	forward = dist / time;
	VectorScale( self->s.origin2, forward, self->s.origin2 );

	self->s.origin2[ 2 ] = time * gravity;
}

/*
=================================================================================

shared reset functions

=================================================================================
*/

void reset_intField( int* target, int instanceField, int classField, int fallback )
{
	if(instanceField)
	{
		*target = instanceField;
	}
	else if (classField)
	{
		*target = classField;
	}
	else
	{
		*target = fallback;
	}
}

void reset_floatField( float* target, float instanceField, float classField, float fallback )
{
	if(instanceField)
	{
		*target = instanceField;
	}
	else if (classField)
	{
		*target = classField;
	}
	else
	{
		*target = fallback;
	}
}

/*
=================================================================================

shared class spawn functions

=================================================================================
*/

void SP_Nothing( gentity_t *self ) {
}

void SP_RemoveSelf( gentity_t *self ) {
	G_FreeEntity( self );
}

/*
=================================================================================

shared field spawn functions

=================================================================================
*/

void SP_ConditionFields( gentity_t *self ) {
	char *buffer;

	if ( G_SpawnString( "buildables", "", &buffer ) )
		BG_ParseCSVBuildableList( buffer, self->conditions.buildables, BA_NUM_BUILDABLES );

	if ( G_SpawnString( "classes", "", &buffer ) )
		BG_ParseCSVClassList( buffer, self->conditions.classes, PCL_NUM_CLASSES );

	if ( G_SpawnString( "equipment", "", &buffer ) )
		BG_ParseCSVEquipmentList( buffer, self->conditions.weapons, WP_NUM_WEAPONS,
	                          self->conditions.upgrades, UP_NUM_UPGRADES );

}

void SP_WaitFields( gentity_t *self, float defaultWait, float defaultWaitVariance ) {
	if (!self->config.wait.time)
		self->config.wait.time = defaultWait;

	if (!self->config.wait.variance)
		self->config.wait.variance = defaultWaitVariance;

	if ( self->config.wait.variance >= self->config.wait.time )
	{
		self->config.wait.variance = self->config.wait.time - FRAMETIME;

		if( g_debugEntities.integer > -1)
		{
			G_Printf( "^3WARNING: ^7Entity ^5#%i ^7of type ^5%s^7 has waitWariance >= wait\n", self->s.number, self->classname );
		}
	}
}



