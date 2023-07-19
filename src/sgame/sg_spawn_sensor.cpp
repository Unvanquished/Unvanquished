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

//the same as InitTrigger
static void InitBrushSensor( gentity_t *self )
{
	if ( !VectorCompare( self->s.angles, vec3_origin ) )
	{
		glm::vec3 angles = VEC2GLM( self->s.angles );
		glm::vec3 movedir = VEC2GLM( self->mapEntity.movedir );
		G_SetMovedir( angles, movedir );
		VectorCopy( &angles[0], self->s.angles );
		VectorCopy( &movedir[0], self->mapEntity.movedir );
	}

	trap_SetBrushModel( self, self->mapEntity.model );
	self->r.contents = CONTENTS_TRIGGER; // replaces the -1 from trap_SetBrushModel
	self->r.svFlags = SVF_NOCLIENT;
	trap_LinkEntity( self );
}

static void sensor_act(gentity_t *self, gentity_t*, gentity_t*)
{
	// if we wanted to tell the cgame about our deactivation, this would be the way to do it
	// self->s.eFlags ^= EF_NODRAW;
	// but unless we have to, we rather not share the information, so "patched" clients cannot do anything with it either
	self->enabled = !self->enabled;
}

static void sensor_reset( gentity_t *self )
{
	// SPAWN_DISABLED?
	self->enabled = !(self->mapEntity.spawnflags & SPF_SPAWN_DISABLED);

	// NEGATE?
	self->mapEntity.conditions.negated = !!( self->mapEntity.spawnflags & 2 );
}

//some old sensors/triggers used to propagate use-events, this is deprecated behavior
static void trigger_compat_propagation_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	G_FireEntity( self, self );

	if ( g_debugEntities.Get() >= -1 ) //dont't warn about anything with -1 or lower
	{
		Log::Warn( "It appears as if %s is targeted by %s to enforce firing, which is undefined behavior — stop doing that! This WILL break in future releases and toggle the sensor instead.", etos( self ), etos( activator ) );
	}
}

// the wait time has passed, so set back up for another activation
static void sensor_checkWaitForReactivation_think( gentity_t *self )
{
	self->nextthink = 0;
}

static void trigger_checkWaitForReactivation( gentity_t *self )
{
	if ( self->mapEntity.config.wait.time > 0 )
	{
		self->think = sensor_checkWaitForReactivation_think;
		self->nextthink = VariatedLevelTime( self->mapEntity.config.wait );
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
static void trigger_multiple_act( gentity_t *self, gentity_t*,
		gentity_t *activator )
{
	self->activator = activator;

	if ( self->nextthink )
		return; // can't retrigger until the wait is over

	if ( activator && activator->client && self->mapEntity.conditions.team &&
	   ( activator->client->pers.team != self->mapEntity.conditions.team ) )
		return;

	G_FireEntity( self, self->activator );
	trigger_checkWaitForReactivation( self );
}

static void trigger_multiple_touch( gentity_t *self, gentity_t *other, trace_t* )
{
	trigger_multiple_act( self, other, other );
}

static void trigger_multiple_compat_reset( gentity_t *self )
{
	if (!!( self->mapEntity.spawnflags & 1 ) != !!( self->mapEntity.spawnflags & 2 )) //if both are set or none are set we assume TEAM_ALL
	{
		if ( self->mapEntity.spawnflags & 1 )
			self->mapEntity.conditions.team = TEAM_HUMANS;
		else if ( self->mapEntity.spawnflags & 2 )
			self->mapEntity.conditions.team = TEAM_ALIENS;
	}

	if ( self->mapEntity.spawnflags && g_debugEntities.Get() >= -1 ) //dont't warn about anything with -1 or lower
	{
		Log::Warn( "It appears as if %s has set spawnflags that were not defined behavior of the entities.def; this is likely to break in the future", etos( self ));
	}
}


/*
==============================================================================

sensor_start

==============================================================================
*/

static void sensor_start_fireAndForget( gentity_t *self )
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

	if( g_debugEntities.Get() >= 2 )
		Log::Debug( "Notification of match start.");

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

static void sensor_timer_think( gentity_t *self )
{
	G_FireEntity( self, self->activator );
	// set time before next firing
	self->nextthink = VariatedLevelTime( self->mapEntity.config.wait );
}

static void sensor_timer_act( gentity_t *self, gentity_t*, gentity_t *activator )
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

	if ( self->mapEntity.spawnflags & 1 )
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

	if( g_debugEntities.Get() >= 2 )
		Log::Debug( "Notification of team %i changing stage from %i to %i (0-2).", team, previousStage, newStage );

	if(newStage <= previousStage) //not supporting stage down yet, also no need to fire if stage didn't change at all
		return;

	while ((entities = G_IterateEntitiesOfClass(entities, S_SENSOR_STAGE)) != nullptr )
	{
		if (((!entities->mapEntity.conditions.stage || newStage == entities->mapEntity.conditions.stage)
				&& (!entities->mapEntity.conditions.team || team == entities->mapEntity.conditions.team))
				== !entities->mapEntity.conditions.negated)
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

	if( g_debugEntities.Get() >= 2 )
		Log::Debug( "Notification of game end. Winning team %i.", winningTeam );

	while ((entity = G_IterateEntitiesOfClass(entity, S_SENSOR_END)) != nullptr )
	{
		if ((winningTeam == entity->mapEntity.conditions.team) == !entity->mapEntity.conditions.negated)
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

static bool sensor_buildable_match( gentity_t *self, gentity_t *activator )
{
	if ( !activator )
	{
		return false;
	}

	//if there is no buildable list every buildable triggers
	if ( self->mapEntity.conditions.buildables.empty() )
	{
		return true;
	}
	else
	{
		//otherwise check against the list
		for ( buildable_t b : self->mapEntity.conditions.buildables )
		{
			if ( activator->s.modelindex == b )
			{
				return true;
			}
		}
	}

	return false;
}

static void sensor_buildable_touch( gentity_t *self, gentity_t *activator, trace_t* )
{
	//sanity check
	if ( !activator || activator->s.eType != entityType_t::ET_BUILDABLE )
	{
		return;
	}

	self->activator = activator;

	if ( self->nextthink )
	{
		return; // can't retrigger until the wait is over
	}

	if( sensor_buildable_match( self, activator ) == !self->mapEntity.conditions.negated )
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
static bool sensor_class_match( gentity_t *self, gentity_t *activator )
{
	if ( !activator )
	{
		return false;
	}

	//if there is no class list every class triggers (stupid case)
	if ( self->mapEntity.conditions.classes.empty() )
	{
		return true;
	}
	else
	{
		//otherwise check against the list
		for ( class_t c : self->mapEntity.conditions.classes )
		{
			if ( activator->client->ps.stats[ STAT_CLASS ] == c )
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
static bool sensor_equipment_match( gentity_t *self, gentity_t *activator )
{
	if ( !activator )
	{
		return false;
	}

	if ( self->mapEntity.conditions.weapons.empty() && self->mapEntity.conditions.upgrades.empty() )
	{
		//if there is no equipment list all equipment triggers for the old behavior of target_equipment, but not the new or different one
		return true;
	}
	else
	{
		//otherwise check against the lists
		for ( weapon_t w : self->mapEntity.conditions.weapons )
		{
			if ( BG_InventoryContainsWeapon( w, activator->client->ps.stats ) )
			{
				return true;
			}
		}

		for ( upgrade_t u : self->mapEntity.conditions.upgrades )
		{
			if ( BG_InventoryContainsUpgrade( u, activator->client->ps.stats ) )
			{
				return true;
			}
		}
	}

	return false;
}

static void sensor_player_touch( gentity_t *self, gentity_t *activator, trace_t* )
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

	if ( self->mapEntity.conditions.team && ( activator->client->pers.team != self->mapEntity.conditions.team ) )
		return;

	// handle legacy `trigger_class` and `trigger_equip` entities
	// by only checking for class on aliens, and equipment on humans
	if ( self->mapEntity.conditions.isClassSensor && G_Team( activator ) != TEAM_ALIENS )
	{
		return;
	}
	if ( self->mapEntity.conditions.isEquipSensor && G_Team( activator ) != TEAM_HUMANS )
	{
		return;
	}

	if ( ( !self->mapEntity.conditions.upgrades.empty() || !self->mapEntity.conditions.weapons.empty() ) && activator->client->pers.team == TEAM_HUMANS )
	{
		shouldFire = sensor_equipment_match( self, activator );
	}
	else if ( !self->mapEntity.conditions.classes.empty() && activator->client->pers.team == TEAM_ALIENS )
	{
		shouldFire = sensor_class_match( self, activator );
	}
	else
	{
		shouldFire = true;
	}

	if( shouldFire == !self->mapEntity.conditions.negated )
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

	if(!Q_stricmp(self->classname, "trigger_class"))
	{
		self->mapEntity.conditions.isClassSensor = true;
	}
	if(!Q_stricmp(self->classname, "trigger_equipment"))
	{
		self->mapEntity.conditions.isEquipSensor = true;
	}
	InitBrushSensor( self );
}

/*
=================================================================================

sensor_support

=================================================================================
*/

static void sensor_support_think( gentity_t *self )
{
	if(!self->enabled)
	{
		self->nextthink = level.time + SENSOR_POLL_PERIOD * 5;
		return;
	}

	bool powered;

	switch (self->mapEntity.conditions.team) {
		case TEAM_HUMANS:
			powered = (G_ActiveReactor() != nullptr);
			break;

		case TEAM_ALIENS:
			powered = (G_ActiveOvermind() != nullptr);
			break;

		case TEAM_ALL:
			powered = (G_ActiveReactor() != nullptr && G_ActiveOvermind() != nullptr);
			break;

		default:
			Log::Warn("missing team field for %s", etos( self ));
			G_FreeEntity( self );
			return;
	}

	if(powered)
		G_FireEntity( self, nullptr );

	self->nextthink = level.time + SENSOR_POLL_PERIOD;
}

static void sensor_support_reset( gentity_t *self )
{
	self->enabled = !(self->mapEntity.spawnflags & SPF_SPAWN_DISABLED);
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


static void sensor_power_think( gentity_t *self )
{
	if(!self->enabled)
	{
		self->nextthink = level.time + SENSOR_POLL_PERIOD * 5;
		return;
	}

	bool powered = (G_ActiveReactor() != nullptr);

	if (powered)
		G_FireEntity( self, nullptr );

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


static void sensor_creep_think( gentity_t *self )
{
	if(!self->enabled)
	{
		self->nextthink = level.time + SENSOR_POLL_PERIOD * 5;
		return;
	}

	bool powered = (G_ActiveOvermind() != nullptr);

	if(powered)
		G_FireEntity( self, nullptr );

	self->nextthink = level.time + SENSOR_POLL_PERIOD;
}

void SP_sensor_creep( gentity_t *self )
{
	self->think = sensor_creep_think;
	self->reset = sensor_support_reset;
}
