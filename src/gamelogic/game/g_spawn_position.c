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

void SP_pos_player_spawn( gentity_t *self )
{
	int i;

	G_SpawnInt( "nobots", "0", &i );

	if ( i )
	{
		self->flags |= FL_NO_BOTS;
	}

	G_SpawnInt( "nohumans", "0", &i );

	if ( i )
	{
		self->flags |= FL_NO_HUMANS;
	}
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

void SP_pos_location( gentity_t *self )
{
	char       *message;
	self->s.eType = ET_LOCATION;
	self->r.svFlags = SVF_BROADCAST;
	trap_LinkEntity( self );  // make the server send them to the clients

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

	self->nextPathSegment = level.locationHead;
	self->s.generic1 = G_LocationIndex(message);
	level.locationHead = self;

	G_SetOrigin( self, self->s.origin );
}
