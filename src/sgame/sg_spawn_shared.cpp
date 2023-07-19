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

	bool hasTarget = false;
	for ( char* target : self->mapEntity.targets )
	{
		if ( target && target[0] )
		{
			hasTarget = true;
			break;
		}
	}

	if ( !hasTarget )
	{
		return;
	}
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
		self->mapEntity.conditions.buildables = BG_ParseBuildableList( buffer );
	}

	if ( G_SpawnString( "classes", "", &buffer ) ) {
		self->mapEntity.conditions.classes = BG_ParseClassList( buffer );
	}

	if ( G_SpawnString( "equipment", "", &buffer ) ) {
		auto equipments = BG_ParseEquipmentList( buffer );
		self->mapEntity.conditions.weapons = std::move(equipments.first);
		self->mapEntity.conditions.upgrades = std::move(equipments.second);
	}
}

void SP_WaitFields( gentity_t *self, float defaultWait, float defaultWaitVariance ) {
	if (!self->mapEntity.config.wait.time)
		self->mapEntity.config.wait.time = defaultWait;

	if (!self->mapEntity.config.wait.variance)
		self->mapEntity.config.wait.variance = defaultWaitVariance;

	if ( self->mapEntity.config.wait.variance >= self->mapEntity.config.wait.time && self->mapEntity.config.wait.variance > 0)
	{
		self->mapEntity.config.wait.variance = self->mapEntity.config.wait.time - FRAMETIME;

		if( g_debugEntities.Get() > -1)
		{
			Log::Warn( "Entity %s has wait.variance >= wait.time", etos( self ) );
		}
	}
}


