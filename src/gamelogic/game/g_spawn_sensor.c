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

//the same as InitTrigger
void InitBrushSensor( gentity_t *self )
{
	if ( !VectorCompare( self->s.angles, vec3_origin ) )
	{
		G_SetMovedir( self->s.angles, self->movedir );
	}

	trap_SetBrushModel( self, self->model );
	self->r.contents = CONTENTS_TRIGGER; // replaces the -1 from trap_SetBrushModel
	self->r.svFlags = SVF_NOCLIENT;
}

void sensor_act(target_t* target, gentity_t *self, gentity_t *other, gentity_t *activator)
{
	// if we wanted to tell the cgame about our deactivation, this would be the way to do it
	// self->s.eFlags ^= EF_NODRAW;
	// but unless we have to, we rather not share the information, so "patched" clients cannot do anything with it either
	self->enabled = !self->enabled;
}

void sensor_reset( gentity_t *self )
{
	// SPAWN_DISABLED?
	self->enabled = !(self->spawnflags & 1);

	// NEGATE?
	self->conditions.negated = !!( self->spawnflags & 2 );
}

//some old sensors/triggers used to propagate use-events, this is deprecated behavior
void trigger_compat_propagation_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	G_FireAllTargetsOf( self, self );

	if ( g_debugEntities.integer >= -1 ) //dont't warn about anything with -1 or lower
	{
		G_Printf( "^3ERROR: ^7It appears as if ^5%s^7 is targeted by ^5%s^7 to enforce firing, which is undefined behavior — stop doing that! This WILL break in future releases and toggle the sensor instead.\n", self->classname, activator->classname );
	}
}


// the wait time has passed, so set back up for another activation
void sensor_checkWaitForReactivation_think( gentity_t *ent )
{
	ent->nextthink = 0;
}

void trigger_checkWaitForReactivation( gentity_t *self )
{
	if ( self->config.wait.time > 0 )
	{
		self->think = sensor_checkWaitForReactivation_think;
		entity_SetNextthink( self );
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
 * old trigger_multiple functions for backward compatbility
 */
// the trigger was just activated
// ent->activator should be set to the activator so it can be held through a delay
// so wait for the delay time before firing
void trigger_multiple_trigger( gentity_t *self, gentity_t *activator )
{
	self->activator = activator;

	if ( self->nextthink )
		return; // can't retrigger until the wait is over

	if ( activator && activator->client && self->conditions.team &&
	   ( activator->client->ps.stats[ STAT_TEAM ] != self->conditions.team ) )
		return;

	G_FireAllTargetsOf( self, self->activator );
	trigger_checkWaitForReactivation( self );
}

void trigger_multiple_propagation_compat_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	trigger_multiple_trigger( self, self );
}

void trigger_multiple_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	if ( !other->client && other->s.eType != ET_BUILDABLE )
	{
		return;
	}

	trigger_multiple_trigger( self, other );
}

void trigger_multiple_compat_reset( gentity_t *self )
{
	if (( self->spawnflags & 1 ) != ( self->spawnflags & 2 )) //if both are set or none are set we assume TEAM_ALL
	{
		if ( self->spawnflags & 1 )
			self->conditions.team = TEAM_HUMANS;
		else if ( self->spawnflags & 2 )
			self->conditions.team = TEAM_ALIENS;
	}

	if ( self->spawnflags && g_debugEntities.integer >= -1 ) //dont't warn about anything with -1 or lower
	{
		G_Printf( "^3ERROR: ^7It appears as if ^5%s^7 has set spawnflags that were not defined behavior of the entities.def; this is likly to break in the future\n", self->classname);
	}
}

void SP_trigger_multiple( gentity_t *ent )
{
	SP_WaitFields(ent, 0.5f, 0);

	ent->touch = trigger_multiple_touch;
	ent->use = trigger_multiple_propagation_compat_use;
	ent->reset = trigger_multiple_compat_reset;

	InitBrushSensor( ent );
	trap_LinkEntity( ent );
}

/*
==============================================================================

sensor_start

==============================================================================
*/

void sensor_start_think( gentity_t *ent )
{
	G_FireAllTargetsOf( ent, ent );
	G_FreeEntity( ent );
}

void SP_sensor_start( gentity_t *ent )
{
	// we must have some delay to make sure our use targets are present
	ent->nextthink = level.time + 300;
	ent->think = sensor_start_think;
}

/*
==============================================================================

timer

==============================================================================
*/

void sensor_timer_think( gentity_t *self )
{
	G_FireAllTargetsOf( self, self->activator );
	// set time before next firing
	entity_SetNextthink( self );
}

void sensor_timer_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	self->activator = activator;

	// if on, turn it off
	if ( self->nextthink )
	{
		self->nextthink = 0;
		return;
	}

	// turn it on
	sensor_timer_think( self );
}

void SP_sensor_timer( gentity_t *self )
{
	SP_WaitFields(self, 1.0f, (self->classname[0] == 'f') ? 1.0f : 0.0f); //wait variance default only for func_timer

	self->use = sensor_timer_use;
	self->think = sensor_timer_think;

	if ( self->spawnflags & 1 )
	{
		self->nextthink = level.time + FRAMETIME;
		self->activator = self;
	}

	self->r.svFlags = SVF_NOCLIENT;
}

/*
=================================================================================

sensor_stage

=================================================================================
*/

/*
===============
G_notify_sensor_stage

Called when stages change
===============
*/
void G_notify_sensor_stage( team_t team, stage_t stage )
{
	gentity_t *ent;

	ent = &g_entities[MAX_CLIENTS - 1]; //start after the reserved player slots
	while ((ent = G_FindNextEntity(ent, FOFS( classname ), "sensor_stage")) != NULL )
	{
		if (((!ent->conditions.stage || stage == ent->conditions.stage)
				&& (!ent->conditions.team || team == ent->conditions.team))
				== !ent->conditions.negated)
		{
			G_FireAllTargetsOf(ent, ent);
		}
	}
}

void SP_sensor_stage( gentity_t *self )
{
	G_SpawnInt( "team", "0", ( int * ) &self->conditions.team );

	if(self->classname[0] == 't')
		self->use = trigger_compat_propagation_use;
	else
		self->act = sensor_act;

	self->reset = sensor_reset;

	self->r.svFlags = SVF_NOCLIENT;
}

/*
=================================================================================

sensor_end

=================================================================================
*/

void G_notify_sensor_end( team_t winningTeam )
{
	gentity_t *ent;

	ent = &g_entities[MAX_CLIENTS - 1]; //start after the reserved player slots
	while ((ent = G_FindNextEntity(ent, FOFS( classname ), "sensor_stage")) != NULL )
	{
		if ((winningTeam == ent->conditions.team) == !ent->conditions.negated)
			G_FireAllTargetsOf(ent, ent);
	}
}


void SP_sensor_end( gentity_t *self )
{
	G_SpawnInt( "team", "0", ( int * ) &self->conditions.team );

	if(self->classname[0] == 't')
		self->use = trigger_compat_propagation_use;
	else
		self->act = sensor_act;

	self->reset = sensor_reset;

	self->r.svFlags = SVF_NOCLIENT;
}

/*
=================================================================================

sensor_touch

=================================================================================
*/

qboolean sensor_buildable_match( gentity_t *self, gentity_t *activator )
{
	int i = 0;

	if ( !activator )
	{
		return qfalse;
	}

	//if there is no buildable list every buildable triggers
	if ( self->conditions.buildables[ i ] == BA_NONE )
	{
		return qtrue;
	}
	else
	{
		//otherwise check against the list
		for ( i = 0; self->conditions.buildables[ i ] != BA_NONE; i++ )
		{
			if ( activator->s.modelindex == self->conditions.buildables[ i ] )
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
===============
sensor_class_match
===============
*/
qboolean sensor_class_match( gentity_t *self, gentity_t *activator )
{
	int i = 0;

	if ( !activator )
	{
		return qfalse;
	}

	//if there is no class list every class triggers (stupid case)
	if ( self->conditions.classes[ i ] == PCL_NONE )
	{
		return qtrue;
	}
	else
	{
		//otherwise check against the list
		for ( i = 0; self->conditions.classes[ i ] != PCL_NONE; i++ )
		{
			if ( activator->client->ps.stats[ STAT_CLASS ] == self->conditions.classes[ i ] )
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
===============
sensor_equipment_match
===============
*/
qboolean sensor_equipment_match( gentity_t *self, gentity_t *activator )
{
	int i = 0;

	if ( !activator )
	{
		return qfalse;
	}

	if ( self->conditions.weapons[ i ] == WP_NONE && self->conditions.upgrades[ i ] == UP_NONE )
	{
		//if there is no equipment list all equipment triggers for the old behavior of target_equipment, but not the new or different one
		return qtrue;
	}
	else
	{
		//otherwise check against the lists
		for ( i = 0; self->conditions.weapons[ i ] != WP_NONE; i++ )
		{
			if ( BG_InventoryContainsWeapon( self->conditions.weapons[ i ], activator->client->ps.stats ) )
			{
				return qtrue;
			}
		}

		for ( i = 0; self->conditions.upgrades[ i ] != UP_NONE; i++ )
		{
			if ( BG_InventoryContainsUpgrade( self->conditions.upgrades[ i ], activator->client->ps.stats ) )
			{
				return qtrue;
			}
		}
	}

	return qfalse;
}

/*
===============
trigger_buildable_trigger
===============
*/
void sensor_buildable_trigger( gentity_t *self, gentity_t *activator )
{
	if( sensor_buildable_match( self, activator ) == !self->conditions.negated )
	{
		G_FireAllTargetsOf( self, activator );
		trigger_checkWaitForReactivation( self );
	}
}

/*
===============
trigger_equipment_trigger
===============
*/
void sensor_client_trigger( gentity_t *self, gentity_t *activator )
{
	qboolean shouldFire;

	if ( ( self->conditions.upgrades[0] || self->conditions.weapons[0] ) && activator->client->ps.stats[ STAT_TEAM ] == TEAM_HUMANS )
	{
		shouldFire = sensor_equipment_match( self, activator );
	}
	else if ( self->conditions.classes[0] && activator->client->ps.stats[ STAT_TEAM ] == TEAM_ALIENS )
	{
		shouldFire = sensor_class_match( self, activator );
	}
	else
	{
		shouldFire = qfalse;
	}

	if( shouldFire == !self->conditions.negated )
	{
		G_FireAllTargetsOf( self, activator );
		trigger_checkWaitForReactivation( self );
	}
}

void sensor_player_touch( gentity_t *self, gentity_t *activator, trace_t *trace )
{
	//sanity check
	if ( !activator || !self->enabled )
	{
		return;
	}

	self->activator = activator;

	if ( self->nextthink )
	{
		return; // can't retrigger until the wait is over
	}

	if ( self->conditions.buildables[0] && activator->s.eType == ET_BUILDABLE )
	{
		sensor_buildable_trigger( self, activator );
	}
	else if ( activator->client )
	{
		sensor_client_trigger( self, activator );
	}
}

//for compatibility
void SP_sensor_touch_compat( gentity_t *entity )
{
	SP_WaitFields(entity, 0.5f, 0);
	SP_sensor_touch( entity );
}

void SP_sensor_touch( gentity_t *self )
{
	SP_ConditionFields( self );

	self->touch = sensor_player_touch;
	self->act = sensor_act;
	self->reset = sensor_reset;

	InitBrushSensor( self );
	trap_LinkEntity( self );
}
