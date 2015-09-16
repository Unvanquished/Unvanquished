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

/*
=================================================================================

ctrl_relay

=================================================================================
*/

void target_relay_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	if (!self->enabled)
		return;

	if ( ( self->spawnflags & 1 ) && activator && activator->client &&
	     activator->client->pers.team != TEAM_HUMANS )
	{
		return;
	}

	if ( ( self->spawnflags & 2 ) && activator && activator->client &&
	     activator->client->pers.team != TEAM_ALIENS )
	{
		return;
	}

	if ( self->spawnflags & 4 )
	{
		G_FireEntityRandomly( self, activator );
		return;
	}

	if ( !self->config.wait.time )
	{
		G_FireEntity( self, activator );
	}
	else
	{
		self->nextthink = VariatedLevelTime( self->config.wait );
		self->think = think_fireDelayed;
		self->activator = activator;
	}
}

void ctrl_relay_reset( gentity_t *self )
{
	self->enabled = !(self->spawnflags & SPF_SPAWN_DISABLED);
}

void ctrl_relay_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	if (!self->enabled)
		return;

	if ( !self->config.wait.time )
	{
		G_EventFireEntity( self, activator, ON_ACT );
	}
	else
	{
		self->nextthink = VariatedLevelTime( self->config.wait );
		self->think = think_fireOnActDelayed;
		self->activator = activator;
	}
}

void SP_ctrl_relay( gentity_t *self )
{
	if( Q_stricmp(self->classname, S_CTRL_RELAY ) ) //if anything but ctrl_relay
	{
		if ( !self->config.wait.time ) {
			// check delay for backwards compatibility
			G_SpawnFloat( "delay", "0", &self->config.wait.time );

			//target delay had previously a default of 1 instead of 0
			if ( !self->config.wait.time && !Q_stricmp(self->classname, "target_delay") )
			{
				self->config.wait.time = 1;
			}
		}
		SP_WaitFields(self, 0, 0 );

		self->act = target_relay_act;
		return;
	}

	SP_WaitFields(self, 0, 0 );
	self->act = ctrl_relay_act;
	self->reset = ctrl_relay_reset;
}

/*
=================================================================================

ctrl_limited

=================================================================================
*/

void ctrl_limited_act(gentity_t *self, gentity_t*, gentity_t *activator)
{
	if (!self->enabled)
		return;

	G_FireEntity( self, activator );
	if ( self->count <= 1 )
	{
		G_FreeEntity( self );
		return;
	}
	self->count--;
}

void ctrl_limited_reset( gentity_t *self )
{
	self->enabled = !(self->spawnflags & SPF_SPAWN_DISABLED);

	G_ResetIntField(&self->count, true, self->config.amount, self->eclass->config.amount, 1);
}

void SP_ctrl_limited( gentity_t *self )
{
	self->act = ctrl_limited_act;
	self->reset = ctrl_limited_reset;
}
