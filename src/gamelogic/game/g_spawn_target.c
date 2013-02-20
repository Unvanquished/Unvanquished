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

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) humanteam alienteam private
"message" text to print
If "private", only the activator gets the message.  If no checks, all clients get the message.
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

//==========================================================

void target_teleporter_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	gentity_t *dest;

	if ( !activator || !activator->client )
	{
		return;
	}

	dest = G_PickTargetFor( self );

	if ( !dest )
	{
		G_Printf( "Couldn't find teleporter destination\n" );
		return;
	}

	TeleportPlayer( activator, dest->s.origin, dest->s.angles, self->speed );
}

/*QUAKED target_teleporter (1 0 0) (-8 -8 -8) (8 8 8)
The activator will be teleported away.
*/
void SP_target_teleporter( gentity_t *self )
{
	if ( !self->targetnames[ 0 ] )
	{
		G_Printf( "untargeted %s at %s\n", self->classname, vtos( self->s.origin ) );
	}

	if( !self->speed )
		self->speed = 400;

	self->use = target_teleporter_use;
}

//==========================================================

/*QUAKED target_kill (.5 .5 .5) (-8 -8 -8) (8 8 8)
Kills the activator.
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

/**
 * Warning: The following comment contains information, that might be parsed and used by radiant based mapeditors.
 */
/*QUAKED target_position (0 .5 0) (-8 -8 -8) (8 8 8)
Used as a positional target for in-game calculation.
Other entities like light, misc_portal_camera and trigger_push (jump pads)
might use it for aiming.

targetname, targetname2, targetname3, targetname3: the names under which this position can be referenced

-------- NOTES --------
To make a jump pad, place this entity at the highest point of the jump and target it with a trigger_push entity.
*/
void SP_target_position( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
static int target_location_counter = 1;

void SP_target_location( gentity_t *self )
{
	char       *message;
	self->s.eType = ET_LOCATION;
	self->r.svFlags = SVF_BROADCAST;
	trap_LinkEntity( self );  // make the server send them to the clients

	if ( target_location_counter == MAX_LOCATIONS )
	{
		G_Printf( S_COLOR_YELLOW "too many target_locations\n" );
		return;
	}

	if ( self->count )
	{
		if ( self->count < 0 )
		{
			self->count = 0;
		}

		if ( self->count > 7 )
		{
			self->count = 7;
		}

		message = va( "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE, self->count + '0',
		              self->message );
	}
	else
	{
		message = self->message;
	}

	trap_SetConfigstring( CS_LOCATIONS + target_location_counter, message );
	self->nextPathSegment = level.locationHead;
	self->s.generic1 = target_location_counter; // use for location marking
	level.locationHead = self;
	target_location_counter++;

	G_SetOrigin( self, self->s.origin );
}

/*
===============
target_hurt_use
===============
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

/*
===============
SP_target_hurt
===============
*/
void SP_target_hurt( gentity_t *self )
{
	if ( !self->targetnames[ 0 ] )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: untargeted %s at %s\n", self->classname,
		          vtos( self->s.origin ) );
	}

	if ( !self->damage )
	{
		self->damage = 5;
	}

	self->use = target_hurt_use;
}

/* Init */
void SP_target_init( void )
{
	target_location_counter = 1;
}
