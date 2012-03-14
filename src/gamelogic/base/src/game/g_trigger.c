/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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

void InitTrigger ( gentity_t *self )
{
	if ( !VectorCompare ( self->s.angles, vec3_origin ) )
	{
		G_SetMovedir ( self->s.angles, self->movedir );
	}

	trap_SetBrushModel ( self, self->model );
	self->r.contents = CONTENTS_TRIGGER; // replaces the -1 from trap_SetBrushModel
	self->r.svFlags = SVF_NOCLIENT;
}

// the wait time has passed, so set back up for another activation
void multi_wait ( gentity_t *ent )
{
	ent->nextthink = 0;
}

// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void multi_trigger ( gentity_t *ent, gentity_t *activator )
{
	ent->activator = activator;

	if ( ent->nextthink )
	{
		return; // can't retrigger until the wait is over
	}

	if ( activator->client )
	{
		if ( ( ent->spawnflags & 1 ) &&
		     activator->client->ps.stats[ STAT_PTEAM ] != PTE_HUMANS )
		{
			return;
		}

		if ( ( ent->spawnflags & 2 ) &&
		     activator->client->ps.stats[ STAT_PTEAM ] != PTE_ALIENS )
		{
			return;
		}
	}

	G_UseTargets ( ent, ent->activator );

	if ( ent->wait > 0 )
	{
		ent->think = multi_wait;
		ent->nextthink = level.time + ( ent->wait + ent->random * crandom() ) * 1000;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		ent->touch = 0;
		ent->nextthink = level.time + FRAMETIME;
		ent->think = G_FreeEntity;
	}
}

void Use_Multi ( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	multi_trigger ( ent, activator );
}

void Touch_Multi ( gentity_t *self, gentity_t *other, trace_t *trace )
{
	if ( !other->client && other->s.eType != ET_BUILDABLE )
	{
		return;
	}

	multi_trigger ( self, other );
}

/*QUAKED trigger_multiple (.5 .5 .5) ?
"wait" : Seconds between triggerings, 0.5 default, -1 = one time only.
"random"  wait variance, default is 0
Variable sized repeatable trigger.  Must be targeted at one or more entities.
so, the basic time between firing is a random time between
(wait - random) and (wait + random)
*/
void SP_trigger_multiple ( gentity_t *ent )
{
	G_SpawnFloat ( "wait", "0.5", &ent->wait );
	G_SpawnFloat ( "random", "0", &ent->random );

	if ( ent->random >= ent->wait && ent->wait >= 0 )
	{
		ent->random = ent->wait - FRAMETIME;
		G_Printf ( "trigger_multiple has random >= wait\n" );
	}

	ent->touch = Touch_Multi;
	ent->use = Use_Multi;

	InitTrigger ( ent );
	trap_LinkEntity ( ent );
}

/*
==============================================================================

trigger_always

==============================================================================
*/

void trigger_always_think ( gentity_t *ent )
{
	G_UseTargets ( ent, ent );
	G_FreeEntity ( ent );
}

/*QUAKED trigger_always (.5 .5 .5) (-8 -8 -8) (8 8 8)
This trigger will always fire.  It is activated by the world.
*/
void SP_trigger_always ( gentity_t *ent )
{
	// we must have some delay to make sure our use targets are present
	ent->nextthink = level.time + 300;
	ent->think = trigger_always_think;
}

/*
==============================================================================

trigger_push

==============================================================================
*/

void trigger_push_touch ( gentity_t *self, gentity_t *other, trace_t *trace )
{
	if ( !other->client )
	{
		return;
	}
}

/*
=================
AimAtTarget

Calculate origin2 so the target apogee will be hit
=================
*/
void AimAtTarget ( gentity_t *self )
{
	gentity_t *ent;
	vec3_t    origin;
	float     height, gravity, time, forward;
	float     dist;

	VectorAdd ( self->r.absmin, self->r.absmax, origin );
	VectorScale ( origin, 0.5, origin );

	ent = G_PickTarget ( self->target );

	if ( !ent )
	{
		G_FreeEntity ( self );
		return;
	}

	height = ent->s.origin[ 2 ] - origin[ 2 ];
	gravity = g_gravity.value;
	time = sqrt ( height / ( 0.5 * gravity ) );

	if ( !time )
	{
		G_FreeEntity ( self );
		return;
	}

	// set s.origin2 to the push velocity
	VectorSubtract ( ent->s.origin, origin, self->s.origin2 );
	self->s.origin2[ 2 ] = 0;
	dist = VectorNormalize ( self->s.origin2 );

	forward = dist / time;
	VectorScale ( self->s.origin2, forward, self->s.origin2 );

	self->s.origin2[ 2 ] = time * gravity;
}

/*QUAKED trigger_push (.5 .5 .5) ?
Must point at a target_position, which will be the apex of the leap.
This will be client side predicted, unlike target_push
*/
void SP_trigger_push ( gentity_t *self )
{
	InitTrigger ( self );

	// unlike other triggers, we need to send this one to the client
	self->r.svFlags &= ~SVF_NOCLIENT;

	// make sure the client precaches this sound
	G_SoundIndex ( "sound/world/jumppad.wav" );

	self->s.eType = ET_PUSH_TRIGGER;
	self->touch = trigger_push_touch;
	self->think = AimAtTarget;
	self->nextthink = level.time + FRAMETIME;
	trap_LinkEntity ( self );
}

void Use_target_push ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !activator->client )
	{
		return;
	}

	if ( activator->client->ps.pm_type != PM_NORMAL )
	{
		return;
	}

	VectorCopy ( self->s.origin2, activator->client->ps.velocity );

	// play fly sound every 1.5 seconds
	if ( activator->fly_sound_debounce_time < level.time )
	{
		activator->fly_sound_debounce_time = level.time + 1500;
		G_Sound ( activator, CHAN_AUTO, self->noise_index );
	}
}

/*QUAKED target_push (.5 .5 .5) (-8 -8 -8) (8 8 8) bouncepad
Pushes the activator in the direction.of angle, or towards a target apex.
"speed"   defaults to 1000
if "bouncepad", play bounce noise instead of windfly
*/
void SP_target_push ( gentity_t *self )
{
	if ( !self->speed )
	{
		self->speed = 1000;
	}

	G_SetMovedir ( self->s.angles, self->s.origin2 );
	VectorScale ( self->s.origin2, self->speed, self->s.origin2 );

	if ( self->spawnflags & 1 )
	{
		self->noise_index = G_SoundIndex ( "sound/world/jumppad.wav" );
	}
	else
	{
		self->noise_index = G_SoundIndex ( "sound/misc/windfly.wav" );
	}

	if ( self->target )
	{
		VectorCopy ( self->s.origin, self->r.absmin );
		VectorCopy ( self->s.origin, self->r.absmax );
		self->think = AimAtTarget;
		self->nextthink = level.time + FRAMETIME;
	}

	self->use = Use_target_push;
}

/*
==============================================================================

trigger_teleport

==============================================================================
*/

void trigger_teleporter_touch ( gentity_t *self, gentity_t *other, trace_t *trace )
{
	gentity_t *dest;

	if ( !other->client )
	{
		return;
	}

	if ( other->client->ps.pm_type == PM_DEAD )
	{
		return;
	}

	// Spectators only?
	if ( ( self->spawnflags & 1 ) &&
	     other->client->sess.sessionTeam != TEAM_SPECTATOR )
	{
		return;
	}

	dest = G_PickTarget ( self->target );

	if ( !dest )
	{
		G_Printf ( "Couldn't find teleporter destination\n" );
		return;
	}

	TeleportPlayer ( other, dest->s.origin, dest->s.angles );
}

/*QUAKED trigger_teleport (.5 .5 .5) ? SPECTATOR
Allows client side prediction of teleportation events.
Must point at a target_position, which will be the teleport destination.

If spectator is set, only spectators can use this teleport
Spectator teleporters are not normally placed in the editor, but are created
automatically near doors to allow spectators to move through them
*/
void SP_trigger_teleport ( gentity_t *self )
{
	InitTrigger ( self );

	// unlike other triggers, we need to send this one to the client
	// unless is a spectator trigger
	if ( self->spawnflags & 1 )
	{
		self->r.svFlags |= SVF_NOCLIENT;
	}
	else
	{
		self->r.svFlags &= ~SVF_NOCLIENT;
	}

	// make sure the client precaches this sound
	G_SoundIndex ( "sound/world/jumppad.wav" );

	self->s.eType = ET_TELEPORT_TRIGGER;
	self->touch = trigger_teleporter_touch;

	trap_LinkEntity ( self );
}

/*
==============================================================================

trigger_hurt

==============================================================================
*/

/*QUAKED trigger_hurt (.5 .5 .5) ? START_OFF - SILENT NO_PROTECTION SLOW
Any entity that touches this will be hurt.
It does dmg points of damage each server frame
Targeting the trigger will toggle its on / off state.

SILENT      supresses playing the sound
SLOW      changes the damage rate to once per second
NO_PROTECTION *nothing* stops the damage

"dmg"     default 5 (whole numbers only)

*/
void hurt_use ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->r.linked )
	{
		trap_UnlinkEntity ( self );
	}
	else
	{
		trap_LinkEntity ( self );
	}
}

void hurt_touch ( gentity_t *self, gentity_t *other, trace_t *trace )
{
	int dflags;

	if ( !other->takedamage )
	{
		return;
	}

	if ( self->timestamp > level.time )
	{
		return;
	}

	if ( self->spawnflags & 16 )
	{
		self->timestamp = level.time + 1000;
	}
	else
	{
		self->timestamp = level.time + FRAMETIME;
	}

	// play sound
	if ( ! ( self->spawnflags & 4 ) )
	{
		G_Sound ( other, CHAN_AUTO, self->noise_index );
	}

	if ( self->spawnflags & 8 )
	{
		dflags = DAMAGE_NO_PROTECTION;
	}
	else
	{
		dflags = 0;
	}

	G_Damage ( other, self, self, NULL, NULL, self->damage, dflags, MOD_TRIGGER_HURT );
}

void SP_trigger_hurt ( gentity_t *self )
{
	InitTrigger ( self );

	self->noise_index = G_SoundIndex ( "sound/misc/electro.wav" );
	self->touch = hurt_touch;

	if ( !self->damage )
	{
		self->damage = 5;
	}

	self->r.contents = CONTENTS_TRIGGER;

	if ( self->spawnflags & 2 )
	{
		self->use = hurt_use;
	}

	// link in to the world if starting active
	if ( ! ( self->spawnflags & 1 ) )
	{
		trap_LinkEntity ( self );
	}
}

/*
==============================================================================

timer

==============================================================================
*/

/*QUAKED func_timer (0.3 0.1 0.6) (-8 -8 -8) (8 8 8) START_ON
This should be renamed trigger_timer...
Repeatedly fires its targets.
Can be turned on or off by using.

"wait"      base time between triggering all targets, default is 1
"random"    wait variance, default is 0
so, the basic time between firing is a random time between
(wait - random) and (wait + random)

*/
void func_timer_think ( gentity_t *self )
{
	G_UseTargets ( self, self->activator );
	// set time before next firing
	self->nextthink = level.time + 1000 * ( self->wait + crandom() * self->random );
}

void func_timer_use ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	self->activator = activator;

	// if on, turn it off
	if ( self->nextthink )
	{
		self->nextthink = 0;
		return;
	}

	// turn it on
	func_timer_think ( self );
}

void SP_func_timer ( gentity_t *self )
{
	G_SpawnFloat ( "random", "1", &self->random );
	G_SpawnFloat ( "wait", "1", &self->wait );

	self->use = func_timer_use;
	self->think = func_timer_think;

	if ( self->random >= self->wait )
	{
		self->random = self->wait - FRAMETIME;
		G_Printf ( "func_timer at %s has random >= wait\n", vtos ( self->s.origin ) );
	}

	if ( self->spawnflags & 1 )
	{
		self->nextthink = level.time + FRAMETIME;
		self->activator = self;
	}

	self->r.svFlags = SVF_NOCLIENT;
}

/*
===============
G_Checktrigger_stages

Called when stages change
===============
*/
void G_Checktrigger_stages ( pTeam_t team, stage_t stage )
{
	int       i;
	gentity_t *ent;

	for ( i = 1, ent = g_entities + i; i < level.num_entities; i++, ent++ )
	{
		if ( !ent->inuse )
		{
			continue;
		}

		if ( !Q_stricmp ( ent->classname, "trigger_stage" ) )
		{
			if ( team == ent->stageTeam && stage == ent->stageStage )
			{
				ent->use ( ent, ent, ent );
			}
		}
	}
}

/*
===============
trigger_stage_use
===============
*/
void trigger_stage_use ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	G_UseTargets ( self, self );
}

void SP_trigger_stage ( gentity_t *self )
{
	G_SpawnInt ( "team", "0", ( int * ) &self->stageTeam );
	G_SpawnInt ( "stage", "0", ( int * ) &self->stageStage );

	self->use = trigger_stage_use;

	self->r.svFlags = SVF_NOCLIENT;
}

/*
===============
trigger_win
===============
*/
void trigger_win ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	G_UseTargets ( self, self );
}

void SP_trigger_win ( gentity_t *self )
{
	G_SpawnInt ( "team", "0", ( int * ) &self->stageTeam );

	self->use = trigger_win;

	self->r.svFlags = SVF_NOCLIENT;
}

/*
===============
trigger_buildable_trigger
===============
*/
void trigger_buildable_trigger ( gentity_t *self, gentity_t *activator )
{
	int i = 0;

	self->activator = activator;

	if ( self->nextthink )
	{
		return; // can't retrigger until the wait is over
	}

	//if there is no buildable list every buildable triggers
	if ( self->bTriggers[ i ] == BA_NONE )
	{
		G_UseTargets ( self, activator );
	}
	else
	{
		//otherwise check against the list
		for ( i = 0; self->bTriggers[ i ] != BA_NONE; i++ )
		{
			if ( activator->s.modelindex == self->bTriggers[ i ] )
			{
				G_UseTargets ( self, activator );
				return;
			}
		}
	}

	if ( self->wait > 0 )
	{
		self->think = multi_wait;
		self->nextthink = level.time + ( self->wait + self->random * crandom() ) * 1000;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		self->touch = 0;
		self->nextthink = level.time + FRAMETIME;
		self->think = G_FreeEntity;
	}
}

/*
===============
trigger_buildable_touch
===============
*/
void trigger_buildable_touch ( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	//only triggered by buildables
	if ( other->s.eType != ET_BUILDABLE )
	{
		return;
	}

	trigger_buildable_trigger ( ent, other );
}

/*
===============
trigger_buildable_use
===============
*/
void trigger_buildable_use ( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	trigger_buildable_trigger ( ent, activator );
}

/*
===============
SP_trigger_buildable
===============
*/
void SP_trigger_buildable ( gentity_t *self )
{
	char *buffer;

	G_SpawnFloat ( "wait", "0.5", &self->wait );
	G_SpawnFloat ( "random", "0", &self->random );

	if ( self->random >= self->wait && self->wait >= 0 )
	{
		self->random = self->wait - FRAMETIME;
		G_Printf ( S_COLOR_YELLOW "WARNING: trigger_buildable has random >= wait\n" );
	}

	G_SpawnString ( "buildables", "", &buffer );

	BG_ParseCSVBuildableList ( buffer, self->bTriggers, BA_NUM_BUILDABLES );

	self->touch = trigger_buildable_touch;
	self->use = trigger_buildable_use;

	InitTrigger ( self );
	trap_LinkEntity ( self );
}

/*
===============
trigger_class_trigger
===============
*/
void trigger_class_trigger ( gentity_t *self, gentity_t *activator )
{
	int i = 0;

	//sanity check
	if ( !activator->client )
	{
		return;
	}

	if ( activator->client->ps.stats[ STAT_PTEAM ] != PTE_ALIENS )
	{
		return;
	}

	self->activator = activator;

	if ( self->nextthink )
	{
		return; // can't retrigger until the wait is over
	}

	//if there is no class list every class triggers (stupid case)
	if ( self->cTriggers[ i ] == PCL_NONE )
	{
		G_UseTargets ( self, activator );
	}
	else
	{
		//otherwise check against the list
		for ( i = 0; self->cTriggers[ i ] != PCL_NONE; i++ )
		{
			if ( activator->client->ps.stats[ STAT_PCLASS ] == self->cTriggers[ i ] )
			{
				G_UseTargets ( self, activator );
				return;
			}
		}
	}

	if ( self->wait > 0 )
	{
		self->think = multi_wait;
		self->nextthink = level.time + ( self->wait + self->random * crandom() ) * 1000;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		self->touch = 0;
		self->nextthink = level.time + FRAMETIME;
		self->think = G_FreeEntity;
	}
}

/*
===============
trigger_class_touch
===============
*/
void trigger_class_touch ( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	//only triggered by clients
	if ( !other->client )
	{
		return;
	}

	trigger_class_trigger ( ent, other );
}

/*
===============
trigger_class_use
===============
*/
void trigger_class_use ( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	trigger_class_trigger ( ent, activator );
}

/*
===============
SP_trigger_class
===============
*/
void SP_trigger_class ( gentity_t *self )
{
	char *buffer;

	G_SpawnFloat ( "wait", "0.5", &self->wait );
	G_SpawnFloat ( "random", "0", &self->random );

	if ( self->random >= self->wait && self->wait >= 0 )
	{
		self->random = self->wait - FRAMETIME;
		G_Printf ( S_COLOR_YELLOW "WARNING: trigger_class has random >= wait\n" );
	}

	G_SpawnString ( "classes", "", &buffer );

	BG_ParseCSVClassList ( buffer, self->cTriggers, PCL_NUM_CLASSES );

	self->touch = trigger_class_touch;
	self->use = trigger_class_use;

	InitTrigger ( self );
	trap_LinkEntity ( self );
}

/*
===============
trigger_equipment_trigger
===============
*/
void trigger_equipment_trigger ( gentity_t *self, gentity_t *activator )
{
	int i = 0;

	//sanity check
	if ( !activator->client )
	{
		return;
	}

	if ( activator->client->ps.stats[ STAT_PTEAM ] != PTE_HUMANS )
	{
		return;
	}

	self->activator = activator;

	if ( self->nextthink )
	{
		return; // can't retrigger until the wait is over
	}

	//if there is no equipment list all equipment triggers (stupid case)
	if ( self->wTriggers[ i ] == WP_NONE && self->wTriggers[ i ] == UP_NONE )
	{
		G_UseTargets ( self, activator );
	}
	else
	{
		//otherwise check against the lists
		for ( i = 0; self->wTriggers[ i ] != WP_NONE; i++ )
		{
			if ( BG_InventoryContainsWeapon ( self->wTriggers[ i ], activator->client->ps.stats ) )
			{
				G_UseTargets ( self, activator );
				return;
			}
		}

		for ( i = 0; self->uTriggers[ i ] != UP_NONE; i++ )
		{
			if ( BG_InventoryContainsUpgrade ( self->uTriggers[ i ], activator->client->ps.stats ) )
			{
				G_UseTargets ( self, activator );
				return;
			}
		}
	}

	if ( self->wait > 0 )
	{
		self->think = multi_wait;
		self->nextthink = level.time + ( self->wait + self->random * crandom() ) * 1000;
	}
	else
	{
		// we can't just remove (self) here, because this is a touch function
		// called while looping through area links...
		self->touch = 0;
		self->nextthink = level.time + FRAMETIME;
		self->think = G_FreeEntity;
	}
}

/*
===============
trigger_equipment_touch
===============
*/
void trigger_equipment_touch ( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	//only triggered by clients
	if ( !other->client )
	{
		return;
	}

	trigger_equipment_trigger ( ent, other );
}

/*
===============
trigger_equipment_use
===============
*/
void trigger_equipment_use ( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	trigger_equipment_trigger ( ent, activator );
}

/*
===============
SP_trigger_equipment
===============
*/
void SP_trigger_equipment ( gentity_t *self )
{
	char *buffer;

	G_SpawnFloat ( "wait", "0.5", &self->wait );
	G_SpawnFloat ( "random", "0", &self->random );

	if ( self->random >= self->wait && self->wait >= 0 )
	{
		self->random = self->wait - FRAMETIME;
		G_Printf ( S_COLOR_YELLOW "WARNING: trigger_equipment has random >= wait\n" );
	}

	G_SpawnString ( "equipment", "", &buffer );

	BG_ParseCSVEquipmentList ( buffer, self->wTriggers, WP_NUM_WEAPONS,
	                           self->uTriggers, UP_NUM_UPGRADES );

	self->touch = trigger_equipment_touch;
	self->use = trigger_equipment_use;

	InitTrigger ( self );
	trap_LinkEntity ( self );
}

/*
===============
trigger_gravity_touch
===============
*/
void trigger_gravity_touch ( gentity_t *ent, gentity_t *other, trace_t *trace )
{
	//only triggered by clients
	if ( !other->client )
	{
		return;
	}

	other->client->ps.gravity = ent->triggerGravity;
}

/*
===============
trigger_gravity_use
===============
*/
void trigger_gravity_use ( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if ( ent->r.linked )
	{
		trap_UnlinkEntity ( ent );
	}
	else
	{
		trap_LinkEntity ( ent );
	}
}

/*
===============
SP_trigger_gravity
===============
*/
void SP_trigger_gravity ( gentity_t *self )
{
	G_SpawnInt ( "gravity", "800", &self->triggerGravity );

	self->touch = trigger_gravity_touch;
	self->use = trigger_gravity_use;

	InitTrigger ( self );
	trap_LinkEntity ( self );
}

/*
===============
trigger_heal_use
===============
*/
void trigger_heal_use ( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( self->r.linked )
	{
		trap_UnlinkEntity ( self );
	}
	else
	{
		trap_LinkEntity ( self );
	}
}

/*
===============
trigger_heal_touch
===============
*/
void trigger_heal_touch ( gentity_t *self, gentity_t *other, trace_t *trace )
{
	int max;

	if ( !other->client )
	{
		return;
	}

	if ( self->timestamp > level.time )
	{
		return;
	}

	if ( self->spawnflags & 2 )
	{
		self->timestamp = level.time + 1000;
	}
	else
	{
		self->timestamp = level.time + FRAMETIME;
	}

	max = other->client->ps.stats[ STAT_MAX_HEALTH ];

	other->health += self->damage;

	if ( other->health > max )
	{
		other->health = max;
	}

	other->client->ps.stats[ STAT_HEALTH ] = other->health;
}

/*
===============
SP_trigger_heal
===============
*/
void SP_trigger_heal ( gentity_t *self )
{
	G_SpawnInt ( "heal", "5", &self->damage );

	self->touch = trigger_heal_touch;
	self->use = trigger_heal_use;

	InitTrigger ( self );

	// link in to the world if starting active
	if ( ! ( self->spawnflags & 1 ) )
	{
		trap_LinkEntity ( self );
	}
}

/*
===============
trigger_ammo_touch
===============
*/
void trigger_ammo_touch ( gentity_t *self, gentity_t *other, trace_t *trace )
{
	int ammo, clips, maxClips, maxAmmo;

	if ( !other->client )
	{
		return;
	}

	if ( other->client->ps.stats[ STAT_PTEAM ] != PTE_HUMANS )
	{
		return;
	}

	if ( self->timestamp > level.time )
	{
		return;
	}

	if ( other->client->ps.weaponstate != WEAPON_READY )
	{
		return;
	}

	if ( BG_FindUsesEnergyForWeapon ( other->client->ps.weapon ) && self->spawnflags & 2 )
	{
		return;
	}

	if ( !BG_FindUsesEnergyForWeapon ( other->client->ps.weapon ) && self->spawnflags & 4 )
	{
		return;
	}

	if ( self->spawnflags & 1 )
	{
		self->timestamp = level.time + 1000;
	}
	else
	{
		self->timestamp = level.time + FRAMETIME;
	}

	BG_FindAmmoForWeapon ( other->client->ps.weapon, &maxAmmo, &maxClips );
	BG_UnpackAmmoArray ( other->client->ps.weapon, other->client->ps.ammo, other->client->ps.powerups,
	                     &ammo, &clips );

	if ( ( ammo + self->damage ) > maxAmmo )
	{
		if ( clips < maxClips )
		{
			clips++;
			ammo = 1;
		}
		else
		{
			ammo = maxAmmo;
		}
	}
	else
	{
		ammo += self->damage;
	}

	BG_PackAmmoArray ( other->client->ps.weapon, other->client->ps.ammo, other->client->ps.powerups,
	                   ammo, clips );
}

/*
===============
SP_trigger_ammo
===============
*/
void SP_trigger_ammo ( gentity_t *self )
{
	G_SpawnInt ( "ammo", "1", &self->damage );

	self->touch = trigger_ammo_touch;

	InitTrigger ( self );
	trap_LinkEntity ( self );
}
