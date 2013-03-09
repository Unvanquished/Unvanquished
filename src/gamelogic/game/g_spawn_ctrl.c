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

ctrl_relay

=================================================================================
*/

void ctrl_relay_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( ( self->spawnflags & 1 ) && activator && activator->client &&
	     activator->client->ps.stats[ STAT_TEAM ] != TEAM_HUMANS )
	{
		return;
	}

	if ( ( self->spawnflags & 2 ) && activator && activator->client &&
	     activator->client->ps.stats[ STAT_TEAM ] != TEAM_ALIENS )
	{
		return;
	}

	if ( self->spawnflags & 4 )
	{
		G_FireRandomTargetOf( self, activator );
		return;
	}

	if ( !self->wait.time )
	{
		G_FireAllTargetsOf( self, activator );
	}
	else
	{
		entity_SetNextthink( self );
		self->think = think_fireDelayed;
		self->activator = activator;
	}
}

void SP_ctrl_relay( gentity_t *self )
{
	if ( !self->wait.time ) {
		// check delay for backwards compatibility
		G_SpawnFloat( "delay", "0", &self->wait.time );

		//target delay had previously a default of 1 instead of 0
		if ( !self->wait.time && !Q_stricmp(self->classname, "target_delay") )
		{
			self->wait.time = 1;
		}
	}

	SP_WaitFields(self, 0, 0 );

	self->use = ctrl_relay_use;
}

/*
=================================================================================

ctrl_limited

=================================================================================
*/

void ctrl_limited_act(target_t* target, gentity_t *self, gentity_t *other, gentity_t *activator)
{
	if (!self->enabled)
		return;

	G_FireAllTargetsOf( self, activator );
	if ( self->count.current <= 1 )
	{
		G_FreeEntity( self );
		return;
	}
	self->count.current--;
}

void ctrl_limited_reset( gentity_t *self )
{
	self->count.current = self->count.previous;
}

void SP_ctrl_limited( gentity_t *self )
{
	if ( self->count.previous < 1 )
		self->count.previous = 1;

	self->act = ctrl_limited_act;
	self->reset = ctrl_limited_reset;
}
