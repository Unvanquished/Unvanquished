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

env_speaker

=================================================================================
*/

void target_speaker_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	if ( self->spawnflags & 3 )
	{
		// looping sound toggles
		if ( self->s.loopSound )
		{
			self->s.loopSound = 0; // turn it off
		}
		else
		{
			self->s.loopSound = self->soundIndex; // start it
		}
	}
	else
	{
		// one-time sound
		if ( (self->spawnflags & 8) && activator )
		{
			G_AddEvent( activator, EV_GENERAL_SOUND, self->soundIndex );
		}
		else if ( self->spawnflags & 4 )
		{
			G_AddEvent( self, EV_GLOBAL_SOUND, self->soundIndex );
		}
		else
		{
			G_AddEvent( self, EV_GENERAL_SOUND, self->soundIndex );
		}
	}
}

void SP_sfx_speaker( gentity_t *self )
{
	char *tmpString;

	if ( !G_SpawnString( "noise", "NOSOUND", &tmpString ) )
	{
		G_Error( "speaker %s without a noise key", etos( self ) );
	}

	// force all client-relative sounds to be "activator" speakers that
	// play on the entity that activates the speaker
	if ( tmpString[ 0 ] == '*' )
	{
		self->spawnflags |= 8;
	}
	self->soundIndex = G_SoundIndex( tmpString );

	// a repeating speaker can be done completely client side
	self->s.eType = ET_SPEAKER;
	self->s.eventParm = self->soundIndex;
	self->s.frame = self->config.wait.time * 10;
	self->s.clientNum = self->config.wait.variance * 10;

	// check for prestarted looping sound
	if ( self->spawnflags & 1 )
	{
		self->s.loopSound = self->soundIndex;
	}

	self->act = target_speaker_act;

	if ( self->spawnflags & 4 )
	{
		self->r.svFlags |= SVF_BROADCAST;
	}

	VectorCopy( self->s.origin, self->s.pos.trBase );

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity( self );
}

/*
=================================================================================

env_rumble

=================================================================================
*/
void fx_rumble_think( gentity_t *self )
{
	int       i;
	gentity_t *ent;

	if ( self->last_move_time < level.time )
	{
		self->last_move_time = level.time + 0.5;
	}

	for ( i = 0, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( !ent->inuse )
		{
			continue;
		}

		if ( !ent->client )
		{
			continue;
		}

		if ( ent->client->ps.groundEntityNum == ENTITYNUM_NONE )
		{
			continue;
		}

		ent->client->ps.groundEntityNum = ENTITYNUM_NONE;
		ent->client->ps.velocity[ 0 ] += crandom() * 150;
		ent->client->ps.velocity[ 1 ] += crandom() * 150;
		ent->client->ps.velocity[ 2 ] = self->config.speed;
	}

	if ( level.time < self->timestamp )
	{
		self->nextthink = level.time + FRAMETIME;
	}
}

void fx_rumble_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	self->timestamp = level.time + ( self->config.amount * FRAMETIME );
	self->nextthink = level.time + FRAMETIME;
	self->activator = activator;
	self->last_move_time = 0;
}

void SP_fx_rumble( gentity_t *self )
{
	if ( !self->config.amount )
	{
		if( G_SpawnInt( "count", "0", &self->config.amount) )
		{
			G_WarnAboutDeprecatedEntityField( self, "amount", "count", ENT_V_RENAMED );
		}
		else
		{
			self->customNumber = 10;
		}
	}

	if ( !self->config.speed )
	{
		self->config.speed = 100;
	}

	self->think = fx_rumble_think;
	self->act = fx_rumble_act;
}

