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

#include "sg_local.h"
#include "sg_spawn.h"
#include "CBSE.h"

/*
=================================================================================

target_print

=================================================================================
*/
void target_print_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	if ( self->spawnflags & 4 )
	{
		if ( activator && activator->client )
		{
			trap_SendServerCommand( activator - g_entities, va( "cp %s", Quote( self->message ) ) );
		}

		return;
	}

	if ( self->spawnflags & 3 )
	{
		if ( self->spawnflags & 1 )
		{
			G_TeamCommand( TEAM_HUMANS, va( "cp %s", Quote( self->message ) ) );
		}

		if ( self->spawnflags & 2 )
		{
			G_TeamCommand( TEAM_ALIENS, va( "cp %s", Quote( self->message ) ) );
		}

		return;
	}

	trap_SendServerCommand( -1, va( "cp %s", Quote( self->message ) ) );
}

void SP_target_print( gentity_t *self )
{
	self->act = target_print_act;
}

/*
=================================================================================

target_push

=================================================================================
*/

void target_push_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	if ( !activator || !activator->client )
	{
		return;
	}

	if ( activator->client->ps.pm_type != PM_NORMAL )
	{
		return;
	}

	VectorCopy( self->s.origin2, activator->client->ps.velocity );
}

void SP_target_push( gentity_t *self )
{
	if ( !self->config.speed)
	{
		self->config.speed = 1000;
	}

	G_SetMovedir( self->s.angles, self->s.origin2 );
	VectorScale( self->s.origin2, self->config.speed, self->s.origin2 );
	VectorCopy( self->s.origin, self->r.absmin );
	VectorCopy( self->s.origin, self->r.absmax );
	self->think = think_aimAtTarget;
	self->nextthink = level.time + FRAMETIME;
	self->act = target_push_act;
}

/*
=================================================================================

target_teleporter

=================================================================================
*/
void target_teleporter_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	gentity_t *dest;

	if ( !activator || !activator->client )
	{
		return;
	}

	dest = G_PickRandomTargetFor( self );

	if ( !dest )
		return;

	G_TeleportPlayer( activator, dest->s.origin, dest->s.angles, self->config.speed );
}

void SP_target_teleporter( gentity_t *self )
{
	if( !self->config.speed )
		self->config.speed = 400;

	self->act = target_teleporter_act;
}

/*
=================================================================================

target_hurt

=================================================================================
*/
void target_hurt_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	// hurt the activator
	if ( !activator )
	{
		return;
	}

	activator->entity->Damage((float)self->damage, self, Util::nullopt, Util::nullopt, 0,
	                          MOD_TRIGGER_HURT);
}

void SP_target_hurt( gentity_t *self )
{
	G_ResetIntField(&self->damage, true, self->config.damage, self->eclass->config.damage, 5);
	self->act = target_hurt_act;
}

