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

ctrl_relay

=================================================================================
*/

static void target_relay_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	if (!self->enabled)
		return;

	if ( ( self->mapEntity.spawnflags & 1 ) && activator && activator->client &&
	     activator->client->pers.team != TEAM_HUMANS )
	{
		return;
	}

	if ( ( self->mapEntity.spawnflags & 2 ) && activator && activator->client &&
	     activator->client->pers.team != TEAM_ALIENS )
	{
		return;
	}

	if ( self->mapEntity.spawnflags & 4 )
	{
		G_FireEntityRandomly( self, activator );
		return;
	}

	if ( !self->mapEntity.config.wait.time )
	{
		G_FireEntity( self, activator );
	}
	else
	{
		self->nextthink = VariatedLevelTime( self->mapEntity.config.wait );
		self->think = think_fireDelayed;
		self->activator = activator;
	}
}

static void ctrl_relay_reset( gentity_t *self )
{
	self->enabled = !(self->mapEntity.spawnflags & SPF_SPAWN_DISABLED);
}

static void ctrl_relay_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	if (!self->enabled)
		return;

	if ( !self->mapEntity.config.wait.time )
	{
		G_EventFireEntity( self, activator, ON_ACT );
	}
	else
	{
		self->nextthink = VariatedLevelTime( self->mapEntity.config.wait );
		self->think = think_fireOnActDelayed;
		self->activator = activator;
	}
}

void SP_ctrl_relay( gentity_t *self )
{
	if( Q_stricmp(self->classname, S_CTRL_RELAY ) ) //if anything but ctrl_relay
	{
		if ( !self->mapEntity.config.wait.time ) {
			// check delay for backwards compatibility
			G_SpawnFloat( "delay", "0", &self->mapEntity.config.wait.time );

			//target delay had previously a default of 1 instead of 0
			if ( !self->mapEntity.config.wait.time && !Q_stricmp(self->classname, "target_delay") )
			{
				self->mapEntity.config.wait.time = 1;
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

static void ctrl_limited_act(gentity_t *self, gentity_t*, gentity_t *activator)
{
	if (!self->enabled)
		return;

	G_FireEntity( self, activator );
	if ( self->mapEntity.count <= 1 )
	{
		G_FreeEntity( self );
		return;
	}
	self->mapEntity.count--;
}

static void ctrl_limited_reset( gentity_t *self )
{
	self->enabled = !(self->mapEntity.spawnflags & SPF_SPAWN_DISABLED);

	G_ResetIntField(&self->mapEntity.count, true, self->mapEntity.config.amount, self->mapEntity.eclass->config.amount, 1);
}

void SP_ctrl_limited( gentity_t *self )
{
	self->act = ctrl_limited_act;
	self->reset = ctrl_limited_reset;
}
