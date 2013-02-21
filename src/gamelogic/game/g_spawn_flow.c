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

flow_relay

=================================================================================
*/

void flow_relay_think_ifDelayed( gentity_t *ent )
{
	G_UseTargets( ent, ent->activator );
}

void flow_relay_use( gentity_t *self, gentity_t *other, gentity_t *activator )
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
		gentity_t *ent;

		ent = G_PickTargetFor( self );

		if ( ent && ent->use )
		{
			ent->use( ent, self, activator );
		}

		return;
	}

	if ( !self->wait && !self->waitVariance )
	{
		G_UseTargets( self, activator );
	}
	else
	{
		entity_SetNextthink( self );
		self->think = flow_relay_think_ifDelayed;
		self->activator = activator;
	}
}

void SP_flow_relay( gentity_t *self )
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

	self->use = flow_relay_use;
}

/*
=================================================================================

flow_limited

=================================================================================
*/

void flow_limited_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	G_UseTargets( self, activator );

	if ( self->count <= 1 )
	{
		G_FreeEntity( self );
	}
	else
	{
		self->count--;
	}
}

void SP_flow_limited( gentity_t *self )
{
	if ( self->count < 1 )
		self->count = 1;

	self->use = flow_limited_use;
}
