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

//the same as InitTrigger
void InitBrushSensor( gentity_t *self )
{
	if ( !VectorCompare( self->s.angles, vec3_origin ) )
	{
		G_SetMovedir( self->s.angles, self->movedir );
	}

	trap_SetBrushModel( self, self->model );
	self->r.contents = CONTENTS_SENSOR; // replaces the -1 from trap_SetBrushModel
	self->r.svFlags = SVF_NOCLIENT;
	trap_LinkEntity( self );
}

void sensor_act(gentity_t *self, gentity_t*, gentity_t*)
{
	// if we wanted to tell the cgame about our deactivation, this would be the way to do it
	// self->s.eFlags ^= EF_NODRAW;
	// but unless we have to, we rather not share the information, so "patched" clients cannot do anything with it either
	self->enabled = !self->enabled;
}

void sensor_reset( gentity_t *self )
{
	// SPAWN_DISABLED?
	self->enabled = !(self->spawnflags & SPF_SPAWN_DISABLED);

	// NEGATE?
	self->conditions.negated = !!( self->spawnflags & 2 );
}

//some old sensors/triggers used to propagate use-events, this is deprecated behavior
void trigger_compat_propagation_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	G_FireEntity( self, self );

	if ( g_debugEntities.integer >= -1 ) //dont't warn about anything with -1 or lower
	{
		G_Printf( S_ERROR "It appears as if %s is targeted by %s to enforce firing, which is undefined behavior — stop doing that! This WILL break in future releases and toggle the sensor instead.\n", etos( self ), etos( activator ) );
	}
}

// the wait time has passed, so set back up for another activation
void sensor_checkWaitForReactivation_think( gentity_t *self )
{
	self->nextthink = 0;
}

void trigger_checkWaitForReactivation( gentity_t *self )
{
	if ( self->config.wait.time > 0 )
	{
		self->think = sensor_checkWaitForReactivation_think;
		self->nextthink = VariatedLevelTime( self->config.wait );
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
void trigger_multiple_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	self->activator = activator;

	if ( self->nextthink )
		return; // can't retrigger until the wait is over

	if ( activator && activator->client && self->conditions.team &&
	   ( activator->client->pers.team != self->conditions.team ) )
		return;

	G_FireEntity( self, self->activator );
	trigger_checkWaitForReactivation( self );
}

void trigger_multiple_touch( gentity_t *self, gentity_t *other, trace_t* )
{
	trigger_multiple_act( self, other, other );
}

void trigger_multiple_compat_reset( gentity_t *self )
{
	if (!!( self->spawnflags & 1 ) != !!( self->spawnflags & 2 )) //if both are set or none are set we assume TEAM_ALL
	{
		if ( self->spawnflags & 1 )
			self->conditions.team = TEAM_HUMANS;
		else if ( self->spawnflags & 2 )
			self->conditions.team = TEAM_ALIENS;
	}

	if ( self->spawnflags && g_debugEntities.integer >= -1 ) //dont't warn about anything with -1 or lower
	{
		G_Printf( S_ERROR "It appears as if %s has set spawnflags that were not defined behavior of the entities.def; this is likely to break in the future\n", etos( self ));
	}
}


/*
==============================================================================

sensor_start

==============================================================================
*/

void sensor_start_fireAndForget( gentity_t *self )
{
	G_FireEntity(self, self);
	G_FreeEntity( self );
}

void SP_sensor_start( gentity_t* )
{
	//self->think = sensor_start_fireAndForget; //gonna reuse that later, when we make sensor_start delayable again (configurable though)
}

void G_notify_sensor_start()
{
	gentity_t *sensor = nullptr;

	if( g_debugEntities.integer >= 2 )
		G_Printf( S_DEBUG "Notification of match start.\n");

	while ((sensor = G_IterateEntitiesOfClass(sensor, S_SENSOR_START)) != nullptr )
	{
		sensor_start_fireAndForget(sensor);
	}
}

/*
==============================================================================

timer

==============================================================================
*/

void sensor_timer_think( gentity_t *self )
{
	G_FireEntity( self, self->activator );
	// set time before next firing
	self->nextthink = VariatedLevelTime( self->config.wait );
}

void sensor_timer_act( gentity_t *self, gentity_t*, gentity_t *activator )
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

	self->act = sensor_timer_act;
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
void G_notify_sensor_stage( team_t team, int previousStage, int newStage )
{
	gentity_t *entities = nullptr;

	if( g_debugEntities.integer >= 2 )
		G_Printf( S_DEBUG "Notification of team %i changing stage from %i to %i (0-2).\n", team, previousStage, newStage );

	if(newStage <= previousStage) //not supporting stage down yet, also no need to fire if stage didn't change at all
		return;

	while ((entities = G_IterateEntitiesOfClass(entities, S_SENSOR_STAGE)) != nullptr )
	{
		if (((!entities->conditions.stage || newStage == entities->conditions.stage)
				&& (!entities->conditions.team || team == entities->conditions.team))
				== !entities->conditions.negated)
		{
			G_FireEntity(entities, entities);
		}
	}
}

void SP_sensor_stage( gentity_t *self )
{
	if(self->classname[0] == 't')
		self->act = trigger_compat_propagation_act;
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
	gentity_t *entity = nullptr;

	if( g_debugEntities.integer >= 2 )
		G_Printf( S_DEBUG "Notification of game end. Winning team %i.\n", winningTeam );

	while ((entity = G_IterateEntitiesOfClass(entity, S_SENSOR_END)) != nullptr )
	{
		if ((winningTeam == entity->conditions.team) == !entity->conditions.negated)
			G_FireEntity(entity, entity);
	}
}


void SP_sensor_end( gentity_t *self )
{
	if(self->classname[0] == 't')
		self->act = trigger_compat_propagation_act;
	else
		self->act = sensor_act;

	self->reset = sensor_reset;

	self->r.svFlags = SVF_NOCLIENT;
}

/*
=================================================================================

sensor_buildable

=================================================================================
*/

bool sensor_buildable_match( gentity_t *self, gentity_t *activator )
{
	int i = 0;

	if ( !activator )
	{
		return false;
	}

	//if there is no buildable list every buildable triggers
	if ( self->conditions.buildables[ i ] == BA_NONE )
	{
		return true;
	}
	else
	{
		//otherwise check against the list
		for ( i = 0; self->conditions.buildables[ i ] != BA_NONE; i++ )
		{
			if ( activator->s.modelindex == self->conditions.buildables[ i ] )
			{
				return true;
			}
		}
	}

	return false;
}

void sensor_buildable_touch( gentity_t *self, gentity_t *activator, trace_t* )
{
	//sanity check
	if ( !activator || !(activator->s.eType == ET_BUILDABLE) )
	{
		return;
	}

	self->activator = activator;

	if ( self->nextthink )
	{
		return; // can't retrigger until the wait is over
	}

	if( sensor_buildable_match( self, activator ) == !self->conditions.negated )
	{
		G_FireEntity( self, activator );
		trigger_checkWaitForReactivation( self );
	}
}

void SP_sensor_buildable( gentity_t *self )
{
	SP_WaitFields(self, 0.5f, 0);
	SP_ConditionFields( self );

	self->touch = sensor_buildable_touch;
	self->act = sensor_act;
	self->reset = sensor_reset;

	InitBrushSensor( self );
}

/*
=================================================================================

sensor_player

=================================================================================
*/

/*
===============
sensor_class_match
===============
*/
bool sensor_class_match( gentity_t *self, gentity_t *activator )
{
	int i = 0;

	if ( !activator )
	{
		return false;
	}

	//if there is no class list every class triggers (stupid case)
	if ( self->conditions.classes[ i ] == PCL_NONE )
	{
		return true;
	}
	else
	{
		//otherwise check against the list
		for ( i = 0; self->conditions.classes[ i ] != PCL_NONE; i++ )
		{
			if ( activator->client->ps.stats[ STAT_CLASS ] == self->conditions.classes[ i ] )
			{
				return true;
			}
		}
	}

	return false;
}

/*
===============
sensor_equipment_match
===============
*/
bool sensor_equipment_match( gentity_t *self, gentity_t *activator )
{
	int i = 0;

	if ( !activator )
	{
		return false;
	}

	if ( self->conditions.weapons[ i ] == WP_NONE && self->conditions.upgrades[ i ] == UP_NONE )
	{
		//if there is no equipment list all equipment triggers for the old behavior of target_equipment, but not the new or different one
		return true;
	}
	else
	{
		//otherwise check against the lists
		for ( i = 0; self->conditions.weapons[ i ] != WP_NONE; i++ )
		{
			if ( BG_InventoryContainsWeapon( self->conditions.weapons[ i ], activator->client->ps.stats ) )
			{
				return true;
			}
		}

		for ( i = 0; self->conditions.upgrades[ i ] != UP_NONE; i++ )
		{
			if ( BG_InventoryContainsUpgrade( self->conditions.upgrades[ i ], activator->client->ps.stats ) )
			{
				return true;
			}
		}
	}

	return false;
}

void sensor_player_touch( gentity_t *self, gentity_t *activator, trace_t* )
{
	bool shouldFire;

	//sanity check
	if ( !activator || !activator->client )
	{
		return;
	}

	self->activator = activator;

	if ( self->nextthink )
	{
		return; // can't retrigger until the wait is over
	}

	if ( self->conditions.team && ( activator->client->pers.team != self->conditions.team ) )
		return;

	if ( ( self->conditions.upgrades[0] || self->conditions.weapons[0] ) && activator->client->pers.team == TEAM_HUMANS )
	{
		shouldFire = sensor_equipment_match( self, activator );
	}
	else if ( self->conditions.classes[0] && activator->client->pers.team == TEAM_ALIENS )
	{
		shouldFire = sensor_class_match( self, activator );
	}
	else
	{
		shouldFire = true;
	}

	if( shouldFire == !self->conditions.negated )
	{
		G_FireEntity( self, activator );
		trigger_checkWaitForReactivation( self );
	}
}

void SP_sensor_player( gentity_t *self )
{
	SP_WaitFields(self, 0.5f, 0);
	SP_ConditionFields( self );

	if(!Q_stricmp(self->classname, "trigger_multiple"))
	{
		self->touch = trigger_multiple_touch;
		self->act = trigger_multiple_act;
		self->reset = trigger_multiple_compat_reset;
	} else {
		self->touch = sensor_player_touch;
		self->act = sensor_act;
		self->reset = sensor_reset;
	}

	InitBrushSensor( self );
}

/*
=================================================================================

sensor_support

=================================================================================
*/

void sensor_support_think( gentity_t *self )
{
	if(!self->enabled)
	{
		self->nextthink = level.time + SENSOR_POLL_PERIOD * 5;
		return;
	}

	//TODO check the difference between G_FindCreep and G_FindPower
	switch (self->conditions.team) {
		case TEAM_HUMANS:
			self->powered = false;
			break;
		case TEAM_ALIENS:
			self->powered = G_FindCreep( self );
			break;
		case TEAM_ALL:
			self->powered = G_FindCreep( self );
			break;
		default:
			G_Printf(S_ERROR "missing team field for %s\n", etos( self ));
			G_FreeEntity( self );
			break;
	}

	if(self->powered)
		G_FireEntity( self, self->powerSource );

	self->nextthink = level.time + SENSOR_POLL_PERIOD;
}

void sensor_support_reset( gentity_t *self )
{
	self->enabled = !(self->spawnflags & SPF_SPAWN_DISABLED);
	//if(self->enabled)
	self->nextthink = level.time + SENSOR_POLL_PERIOD;
}

void SP_sensor_support( gentity_t *self )
{
	self->think = sensor_support_think;
	self->reset = sensor_support_reset;
}

/*
=================================================================================

sensor_power

=================================================================================
*/


void sensor_power_think( gentity_t *self )
{
	if(!self->enabled)
	{
		self->nextthink = level.time + SENSOR_POLL_PERIOD * 5;
		return;
	}

	self->powered = false; //TODO: Reuse or remove this sensor

	if(self->powered)
		G_FireEntity( self, self->powerSource );

	self->nextthink = level.time + SENSOR_POLL_PERIOD;
}

void SP_sensor_power( gentity_t *self )
{
	self->think = sensor_power_think;
	self->reset = sensor_support_reset;
}

/*
=================================================================================

sensor_creep

=================================================================================
*/


void sensor_creep_think( gentity_t *self )
{
	if(!self->enabled)
	{
		self->nextthink = level.time + SENSOR_POLL_PERIOD * 5;
		return;
	}

	self->powered = G_FindCreep( self );

	if(self->powered)
		G_FireEntity( self, self->powerSource );

	self->nextthink = level.time + SENSOR_POLL_PERIOD;
}

void SP_sensor_creep( gentity_t *self )
{
	self->think = sensor_creep_think;
	self->reset = sensor_support_reset;
}
