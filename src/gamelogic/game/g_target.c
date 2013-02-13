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

//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void Use_Target_Score( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if ( !activator )
	{
		return;
	}

	AddScore( activator, ent->count );
}

void SP_target_score( gentity_t *ent )
{
	if ( !ent->count )
	{
		ent->count = 1;
	}

	ent->use = Use_Target_Score;
}

//==========================================================

/*QUAKED target_print (1 0 0) (-8 -8 -8) (8 8 8) humanteam alienteam private
"message" text to print
If "private", only the activator gets the message.  If no checks, all clients get the message.
*/
void Use_Target_Print( gentity_t *ent, gentity_t *other, gentity_t *activator )
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
	ent->use = Use_Target_Print;
}

//==========================================================

void Use_Target_Speaker( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if ( ent->spawnflags & 3 )
	{
		// looping sound toggles
		if ( ent->s.loopSound )
		{
			ent->s.loopSound = 0; // turn it off
		}
		else
		{
			ent->s.loopSound = ent->noise_index; // start it
		}
	}
	else
	{
		// one-time sound
		if ( (ent->spawnflags & 8) && activator )
		{
			G_AddEvent( activator, EV_GENERAL_SOUND, ent->noise_index );
		}
		else if ( ent->spawnflags & 4 )
		{
			G_AddEvent( ent, EV_GLOBAL_SOUND, ent->noise_index );
		}
		else
		{
			G_AddEvent( ent, EV_GENERAL_SOUND, ent->noise_index );
		}
	}
}

/**
 * Warning: The following comment contains information, that might be parsed and used by radiant based mapeditors.
 */
/*QUAKED target_speaker (0 .7 .7) (-8 -8 -8) (8 8 8) LOOPED_ON LOOPED_OFF GLOBAL ACTIVATOR
Sound generating entity that plays .wav files. Normal non-looping sounds play each time the target_speaker is triggered. Looping sounds can be set to play by themselves (no activating trigger) or be toggled on/off by a trigger.
-------- KEYS --------
noise: path/name of .wav file to play (eg. sound/world/growl1.wav - see Notes).
wait: delay in seconds between each time the sound is played ("random" key must be set - see Notes).
random: random time variance in seconds added or subtracted from "wait" delay ("wait" key must be set - see Notes).
targetname, targetname2, targetname3, targetname3: the activating button or trigger points to one of these.
-------- SPAWNFLAGS --------
LOOPED_ON: sound will loop and initially start on in level (will toggle on/off when triggered).
LOOPED_OFF: sound will loop and initially start off in level (will toggle on/off when triggered).
GLOBAL: sound will play full volume throughout the level.
ACTIVATOR: sound will play only for the player that activated the target.
-------- NOTES --------
The path portion value of the "noise" key can be replaced by the implicit folder character "*" for triggered sounds that belong to a particular player model. For example, if you want to create a "bottomless pit" in which the player screams and dies when he falls into, you would place a trigger_multiple over the floor of the pit and target a target_speaker with it. Then, you would set the "noise" key to "*falling1.wav". The * character means the current player model's sound folder. So if your current player model is Visor, * = sound/player/visor, if your current player model is Sarge, * = sound/player/sarge, etc. This cool feature provides an excellent way to create "player-specific" triggered sounds in your levels.
The combination of the "wait" and "random" keys can be used to play non-looping sounds without requiring an activating trigger but both keys must be used together. The value of the "random" key is used to calculate a minimum and a maximum delay. The final time delay will be a random value anywhere between the minimum and maximum values: (min delay = wait - random) (max delay = wait + random).
*/
void SP_target_speaker( gentity_t *ent )
{
	char buffer[ MAX_QPATH ];
	char *s;

	G_SpawnFloat( "wait", "0", &ent->wait );
	G_SpawnFloat( "random", "0", &ent->random );

	if ( !G_SpawnString( "noise", "NOSOUND", &s ) )
	{
		G_Error( "target_speaker without a noise key at %s", vtos( ent->s.origin ) );
	}

	// force all client-relative sounds to be "activator" speakers that
	// play on the entity that activates the speaker
	if ( s[ 0 ] == '*' )
	{
		ent->spawnflags |= 8;
	}

	if ( !strstr( s, ".wav" ) )
	{
		Com_sprintf( buffer, sizeof( buffer ), "%s.wav", s );
	}
	else
	{
		Q_strncpyz( buffer, s, sizeof( buffer ) );
	}

	ent->noise_index = G_SoundIndex( buffer );

	// a repeating speaker can be done completely client side
	ent->s.eType = ET_SPEAKER;
	ent->s.eventParm = ent->noise_index;
	ent->s.frame = ent->wait * 10;
	ent->s.clientNum = ent->random * 10;

	// check for prestarted looping sound
	if ( ent->spawnflags & 1 )
	{
		ent->s.loopSound = ent->noise_index;
	}

	ent->use = Use_Target_Speaker;

	if ( ent->spawnflags & 4 )
	{
		ent->r.svFlags |= SVF_BROADCAST;
	}

	VectorCopy( ent->s.origin, ent->s.pos.trBase );

	// must link the entity so we get areas and clusters so
	// the server can determine who to send updates to
	trap_LinkEntity( ent );
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

	G_SpawnFloat( "speed", "400", &self->speed );

	self->use = target_teleporter_use;
}

//==========================================================

void target_relay_think_ifDelayed( gentity_t *ent )
{
	G_UseTargets( ent, ent->activator );
}

void target_relay_use( gentity_t *self, gentity_t *other, gentity_t *activator )
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

	if ( !self->wait && !self->random )
	{
		G_UseTargets( self, activator );
	}
	else
	{
		self->nextthink = level.time + ( self->wait + self->random * crandom() ) * 1000;
		self->think = target_relay_think_ifDelayed;
		self->activator = activator;
	}
}

/**
 * Warning: The following comment contains information, that might be parsed and used by radiant based mapeditors.
 */
/*QUAKED target_relay (0 .7 .7) (-8 -8 -8) (8 8 8) HUMAN_ONLY ALIEN_ONLY RANDOM
This can only be activated by other triggers which will cause it in turn to activate its own targets.
-------- KEYS --------
targetname, targetname2, targetname3, targetname3: activating trigger points to one of these.
target, target2, target3, target4: this points to entities to activate when this entity is triggered. RANDOM chooses whether all gets executed or one gets selected Randomly.
-------- SPAWNFLAGS --------
HUMAN_ONLY: only human team players can activate trigger.
ALIEN_ONLY: only alien team players can activate trigger.
RANDOM: one one of the targeted entities will be triggered at random.
*/
void SP_target_relay( gentity_t *self )
{
	// check delay for backwards compatibility
	if ( !G_SpawnFloat( "delay", "0", &self->wait ) )
	{
		G_SpawnFloat( "wait", "0", &self->wait );
	}

	if ( !Q_stricmp(self->classname, "target_delay") )
	{

		if ( !self->wait )
		{
			self->wait = 1;
		}

		G_Entitiy_Deprecation_Alias(self, "target_relay");
	}

	self->use = target_relay_use;
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
	if ( !Q_stricmp(self->classname, "info_notnull") )
	{
		G_Entitiy_Deprecation_Alias(self, "info_notnull");
	}
	G_SetOrigin( self, self->s.origin );
}

/*QUAKED target_location (0 0.5 0) (-8 -8 -8) (8 8 8)
Set "message" to the name of this location.
Set "count" to 0-7 for color.
0:white 1:red 2:green 3:yellow 4:blue 5:cyan 6:magenta 7:white

Closest target_location in sight used for the location, if none
in site, closest in distance
*/
static int SP_target_counter = 1;

void SP_target_location( gentity_t *self )
{
	char       *message;
	self->s.eType = ET_LOCATION;
	self->r.svFlags = SVF_BROADCAST;
	trap_LinkEntity( self );  // make the server send them to the clients

	if ( SP_target_counter == MAX_LOCATIONS )
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

	trap_SetConfigstring( CS_LOCATIONS + SP_target_counter, message );
	self->nextTrain = level.locationHead;
	self->s.generic1 = SP_target_counter; // use for location marking
	level.locationHead = self;
	SP_target_counter++;

	G_SetOrigin( self, self->s.origin );
}

/*
===============
target_rumble_think
===============
*/
void target_rumble_think( gentity_t *self )
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
		ent->client->ps.velocity[ 2 ] = self->speed;
	}

	if ( level.time < self->timestamp )
	{
		self->nextthink = level.time + FRAMETIME;
	}
}

/*
===============
target_rumble_use
===============
*/
void target_rumble_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	self->timestamp = level.time + ( self->count * FRAMETIME );
	self->nextthink = level.time + FRAMETIME;
	self->activator = activator;
	self->last_move_time = 0;
}

/*
===============
SP_target_rumble
===============
*/
void SP_target_rumble( gentity_t *self )
{
	if ( !self->targetnames[ 0 ] )
	{
		G_Printf( S_COLOR_YELLOW "WARNING: untargeted %s at %s\n", self->classname,
		          vtos( self->s.origin ) );
	}

	if ( !self->count )
	{
		self->count = 10;
	}

	if ( !self->speed )
	{
		self->speed = 100;
	}

	self->think = target_rumble_think;
	self->use = target_rumble_use;
}

/*
===============
target_alien_win_use
===============
*/
void target_alien_win_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !level.uncondHumanWin )
	{
		level.uncondAlienWin = qtrue;
	}
}

/*
===============
SP_target_alien_win
===============
*/
void SP_target_alien_win( gentity_t *self )
{
	self->use = target_alien_win_use;
}

/*
===============
target_human_win_use
===============
*/
void target_human_win_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !level.uncondAlienWin )
	{
		level.uncondHumanWin = qtrue;
	}
}

/*
===============
SP_target_human_win
===============
*/
void SP_target_human_win( gentity_t *self )
{
	self->use = target_human_win_use;
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
	SP_target_counter = 1;
}
