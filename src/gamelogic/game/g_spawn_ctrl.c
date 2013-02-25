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

void ctrl_relay_think_ifDelayed( gentity_t *ent )
{
	G_FireAllTargetsOf( ent, ent->activator );
}

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

	if ( !self->wait && !self->waitVariance )
	{
		G_FireAllTargetsOf( self, activator );
	}
	else
	{
		entity_SetNextthink( self );
		self->think = ctrl_relay_think_ifDelayed;
		self->activator = activator;
	}
}

void SP_ctrl_relay( gentity_t *self )
{
	if ( !self->wait ) {
		// check delay for backwards compatibility
		G_SpawnFloat( "delay", "0", &self->wait );

		//target delay had previously a default of 1 instead of 0
		if ( !self->wait && !Q_stricmp(self->classname, "target_delay") )
		{
			self->wait = 1;
		}
	}

	self->use = ctrl_relay_use;
}

/*
=================================================================================

ctrl_limited

=================================================================================
*/

void ctrl_limited_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	G_FireAllTargetsOf( self, activator );

	if ( self->customNumber <= 1 )
	{
		G_FreeEntity( self );
	}
	else
	{
		self->customNumber--;
	}
}

void SP_ctrl_limited( gentity_t *self )
{
	if ( self->customNumber < 1 )
		self->customNumber = 1;

	self->use = ctrl_limited_use;
}
