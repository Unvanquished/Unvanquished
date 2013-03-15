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

target_print

=================================================================================
*/
void target_print_use( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if ( ent->spawnflags & 4 )
	{
		if ( activator && activator->client )
		{
			trap_SendServerCommand( activator - g_entities, va( "cp %s", Quote( ent->message ) ) );
		}

		return;
	}

	if ( ent->spawnflags & 3 )
	{
		if ( ent->spawnflags & 1 )
		{
			G_TeamCommand( TEAM_HUMANS, va( "cp %s", Quote( ent->message ) ) );
		}

		if ( ent->spawnflags & 2 )
		{
			G_TeamCommand( TEAM_ALIENS, va( "cp %s", Quote( ent->message ) ) );
		}

		return;
	}

	trap_SendServerCommand( -1, va( "cp %s", Quote( ent->message ) ) );
}

void SP_target_print( gentity_t *ent )
{
	ent->use = target_print_use;
}

/*
=================================================================================

target_push

=================================================================================
*/

void target_push_use( gentity_t *self, gentity_t *other, gentity_t *activator )
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

	if ( self )
	{
		VectorCopy( self->s.origin, self->r.absmin );
		VectorCopy( self->s.origin, self->r.absmax );
		self->think = think_aimAtTarget;
		self->nextthink = level.time + FRAMETIME;
	}

	self->use = target_push_use;
}

/*
=================================================================================

target_teleporter

=================================================================================
*/
void target_teleporter_use( gentity_t *self, gentity_t *other, gentity_t *activator )
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

	self->use = target_teleporter_use;
}

/*
=================================================================================

target_kill

=================================================================================
*/
void target_kill_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !activator )
	{
		return;
	}

	G_Damage( activator, NULL, NULL, NULL, NULL, 100000, DAMAGE_NO_PROTECTION, MOD_TELEFRAG );
}

void SP_target_kill( gentity_t *self )
{
	self->use = target_kill_use;
}

/*
=================================================================================

target_hurt

=================================================================================
*/
void target_hurt_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	// hurt the activator
	if ( !activator || !activator->takedamage )
	{
		return;
	}

	G_Damage( activator, self, self, NULL, NULL, self->damage, 0, MOD_TRIGGER_HURT );
}

void SP_target_hurt( gentity_t *self )
{
	reset_intField(&self->damage, self->config.damage, self->eclass->config.damage, 5);
	self->use = target_hurt_use;
}

