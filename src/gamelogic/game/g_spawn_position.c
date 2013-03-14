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

void SP_pos_player_deathmatch( gentity_t *ent )
{
	int i;

	G_SpawnInt( "nobots", "0", &i );

	if ( i )
	{
		ent->flags |= FL_NO_BOTS;
	}

	G_SpawnInt( "nohumans", "0", &i );

	if ( i )
	{
		ent->flags |= FL_NO_HUMANS;
	}
}

void SP_pos_player_start( gentity_t *ent )
{
	ent->classname = "info_player_deathmatch";
	SP_pos_player_deathmatch( ent );
}

/*
=================================================================================

pos_target

=================================================================================
*/
void SP_pos_target( gentity_t *self )
{
	G_SetOrigin( self, self->s.origin );
}

/*
=================================================================================

pos_location

=================================================================================
*/
static int pos_location_counter = 1;

void SP_pos_location( gentity_t *self )
{
	char       *message;
	self->s.eType = ET_LOCATION;
	self->r.svFlags = SVF_BROADCAST;
	trap_LinkEntity( self );  // make the server send them to the clients

	if ( pos_location_counter == MAX_LOCATIONS )
	{
		if(g_debugEntities.integer > -1)
			G_Printf( "^3WARNING: ^7too many locations set\n" );
		return;
	}

	if ( G_SpawnInt( "count", "", &self->customNumber) )
	{
		if ( self->customNumber < 0 )
		{
			self->customNumber = 0;
		}

		if ( self->customNumber > 7 )
		{
			self->customNumber = 7;
		}

		message = va( "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE, self->customNumber + '0',
		              self->message );
	}
	else
	{
		message = self->message;
	}

	trap_SetConfigstring( CS_LOCATIONS + pos_location_counter, message );
	self->nextPathSegment = level.locationHead;
	self->s.generic1 = pos_location_counter; // use for location marking
	level.locationHead = self;
	pos_location_counter++;

	G_SetOrigin( self, self->s.origin );
}

/*
=================================================================================

init

=================================================================================
*/

void SP_position_init( void )
{
	pos_location_counter = 1;
}
