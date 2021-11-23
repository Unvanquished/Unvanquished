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

/*
=================================================================================

shared entity think functions

=================================================================================
*/

void think_fireDelayed( gentity_t *self )
{
	G_FireEntity( self, self->activator );
}

void think_fireOnActDelayed( gentity_t *self )
{
	G_EventFireEntity( self, self->activator, ON_ACT );
}

/*
=================
AimAtTarget

Calculate origin2 so the target apogee will be hit
=================
*/
void think_aimAtTarget( gentity_t *self )
{
	gentity_t *pickedTarget;
	vec3_t    origin;
	float     distance;

	VectorAdd( self->r.absmin, self->r.absmax, origin );
	VectorScale( origin, 0.5f, origin );

	pickedTarget = G_PickRandomTargetFor( self );

	if ( !pickedTarget )
	{
		G_FreeEntity( self );
		return;
	}

	float height = pickedTarget->s.origin[ 2 ] - origin[ 2 ];
	float gravity = static_cast<float>(g_gravity.Get());
	float time = sqrtf( height / ( 0.5f * gravity ) );

	if ( !time )
	{
		G_FreeEntity( self );
		return;
	}

	// set s.origin2 to the push velocity
	VectorSubtract( pickedTarget->s.origin, origin, self->s.origin2 );
	self->s.origin2[ 2 ] = 0;
	distance = VectorNormalize( self->s.origin2 );

	float forward = distance / time;
	VectorScale( self->s.origin2, forward, self->s.origin2 );

	self->s.origin2[ 2 ] = time * gravity;
}

/*
=================================================================================

shared reset functions

=================================================================================
*/

void G_ResetIntField( int* result, bool fallbackIfNegative, int instanceField, int classField, int fallback )
{
	if(instanceField && (instanceField > 0 || !fallbackIfNegative))
	{
		*result = instanceField;
	}
	else if (classField && (classField > 0 || !fallbackIfNegative))
	{
		*result = classField;
	}
	else
	{
		*result = fallback;
	}
}

void G_ResetFloatField( float* result, bool fallbackIfNegative, float instanceField, float classField, float fallback )
{
	if(instanceField && (instanceField > 0 || !fallbackIfNegative))
	{
		*result = instanceField;
	}
	else if (classField && (classField > 0 || !fallbackIfNegative))
	{
		*result = classField;
	}
	else
	{
		*result = fallback;
	}
}

void G_ResetTimeField( variatingTime_t *result,
		variatingTime_t instanceField, variatingTime_t classField, variatingTime_t fallback )
{
	if( instanceField.time && instanceField.time > 0 )
	{
		*result = instanceField;
	}
	else if (classField.time && classField.time > 0 )
	{
		*result = classField;
	}
	else
	{
		*result = fallback;
	}

	if ( result->variance < 0 )
	{
		result->variance = 0;

		if( g_debugEntities.Get() >= 0 )
		{
			Log::Warn( "negative variance (%f); resetting to 0", result->variance );
		}
	}
	else if ( result->variance >= result->time && result->variance > 0)
	{
		result->variance = result->time - FRAMETIME;

		if( g_debugEntities.Get() > 0 )
		{
			Log::Warn( "limiting variance (%f) to be smaller than time (%f)", result->variance, result->time );
		}
	}
}

/*
=================================================================================

shared class spawn functions

=================================================================================
*/

void SP_Nothing( gentity_t* ) {
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

	if ( G_SpawnString( "buildables", "", &buffer ) ) {
		self->conditions.buildables = BG_ParseBuildableList( buffer );
	}

	if ( G_SpawnString( "classes", "", &buffer ) ) {
		self->conditions.classes = BG_ParseClassList( buffer );
	}

	if ( G_SpawnString( "equipment", "", &buffer ) ) {
		auto equipments = BG_ParseEquipmentList( buffer );
		self->conditions.weapons = std::move(equipments.first);
		self->conditions.upgrades = std::move(equipments.second);
	}
}

void SP_WaitFields( gentity_t *self, float defaultWait, float defaultWaitVariance ) {
	if (!self->config.wait.time)
		self->config.wait.time = defaultWait;

	if (!self->config.wait.variance)
		self->config.wait.variance = defaultWaitVariance;

	if ( self->config.wait.variance >= self->config.wait.time && self->config.wait.variance > 0)
	{
		self->config.wait.variance = self->config.wait.time - FRAMETIME;

		if( g_debugEntities.Get() > -1)
		{
			Log::Warn( "Entity %s has wait.variance >= wait.time", etos( self ) );
		}
	}
}


