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

#include "common/Common.h"
#include "sg_local.h"
#include "sg_entities.h"
#include "CBSE.h"

#include <glm/geometric.hpp>
#include <glm/gtx/norm.hpp>

/*
=================================================================================

basic gentity lifecycle handling

=================================================================================
*/

// init the CBSE entity for an entity without any CBSE logic
void G_InitGentityMinimal( gentity_t *entity )
{
	EmptyEntity::Params params;
	params.oldEnt = entity;
	entity->entity = new EmptyEntity( params );
}

void G_InitGentity( gentity_t *entity )
{
	ASSERT( !entity->inuse );
	ASSERT_EQ( entity->entity, nullptr );
	++entity->generation;
	entity->inuse = true;
	entity->enabled = true;
	entity->classname = "noclass";
	entity->s.number = entity->num();
	entity->r.ownerNum = ENTITYNUM_NONE;
	entity->creationTime = level.time;

	if ( g_debugEntities.Get() > 2 )
	{
		Log::Debug("Initing Entity %i", entity->num() );
	}
}

/*
=================
FindEntitySlot

Either finds a free entity, or allocates a new one.

  The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
never be used by anything else.

Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
static gentity_t *FindEntitySlot()
{
	// we iterate through all the entities and look for a free one that was allocated enough time ago,
	// as well as one that died recently in case the first kind is not available
	gentity_t *newEntity = &g_entities[ MAX_CLIENTS ];
	gentity_t *forcedEnt = nullptr;
	int i;

	for ( i = MAX_CLIENTS; i < level.num_entities; i++, newEntity++ )
	{
		if ( newEntity->inuse )
		{
			continue;
		}

		// the first couple seconds of server time can involve a lot of
		// freeing and allocating, so relax the replacement policy
		if ( newEntity->freetime > level.startTime + 2000 && level.time - newEntity->freetime < 1000 )
		{
			if( ! forcedEnt || newEntity->freetime < forcedEnt->freetime ) {
				forcedEnt = newEntity;
			}
			continue;
		}

		// reuse this slot
		return newEntity;
	}

	if ( i == ENTITYNUM_MAX_NORMAL )
	{
		// no more entities available! let's force-reuse one if possible, or die
		if ( forcedEnt )
		{
			if ( g_debugEntities.Get() ) {
				Log::Verbose( "Reusing Entity %i, freed at %i (%ims ago)",
				              forcedEnt->num(), forcedEnt->freetime, level.time - forcedEnt->freetime );
			}
			// reuse this slot
			return forcedEnt;
		}

		for ( i = 0; i < MAX_GENTITIES; i++ )
		{
			Log::Warn( "%4i: %s", i, g_entities[ i ].classname );
		}

		Sys::Drop( "FindEntitySlot: no free entities" );
	}

	// open up a new slot
	level.num_entities++;

	// let the server system know that there are more entities
	trap_LocateGameData( level.num_entities, sizeof( gentity_t ),
	                     sizeof( level.clients[ 0 ] ) );

	return newEntity;
}

gentity_t *G_NewEntity( initEntityStyle_t style )
{
	gentity_t *ent = FindEntitySlot();
	G_InitGentity( ent );
	if ( style == NO_CBSE )
	{
		G_InitGentityMinimal( ent );
	}
	return ent;
}

/*
=================
G_FreeEntity

Marks the entity as free
=================
*/
void G_FreeEntity( gentity_t *entity )
{
	trap_UnlinkEntity( entity );  // unlink from world

	if ( g_debugEntities.Get() > 2 )
	{
		Log::Debug("Freeing Entity %s", etos(entity));
	}

	G_BotRemoveObstacle( entity->num() );

	entity->mapEntity.deinstantiate();

	if ( entity->s.eType == entityType_t::ET_BEACON && entity->s.modelindex == BCT_TAG )
	{
		// It's possible that this happened before, but we need to be sure.
		BaseClustering::Remove(entity);
	}

	if ( entity->id != nullptr )
	{
		G_ForgetEntityId( entity->id );
	}

	delete entity->entity;

	unsigned generation = entity->generation;

	if ( entity->id )
	{
		BG_Free( entity->id );
	}

	entity->~gentity_t();
	new(entity) gentity_t{};

	entity->generation = generation + 1;
	entity->entity = nullptr;
	entity->classname = "freent";
	entity->freetime = level.time;
	entity->inuse = false;
}


/*
=================
G_NewTempEntity

Spawns an event entity that will be auto-removed
The origin will be snapped to save net bandwidth, so care
must be taken if the origin is right on a surface (snap towards start vector first)
=================
*/
gentity_t *G_NewTempEntity( glm::vec3 origin, int event )
{
	gentity_t *newEntity;

	newEntity = G_NewEntity( NO_CBSE );
	newEntity->s.eType = Util::enum_cast<entityType_t>( Util::ordinal(entityType_t::ET_EVENTS) + event );

	newEntity->classname = "tempEntity";
	newEntity->eventTime = level.time;
	newEntity->freeAfterEvent = true;

	SnapVector( origin );  // save network bandwidth
	G_SetOrigin( newEntity, origin );

	// find cluster for PVS
	trap_LinkEntity( newEntity );

	return newEntity;
}

/*
=================================================================================

gentity debugging

=================================================================================
*/

/*
=============
EntityToString

Convenience function for printing entities
=============
*/
//assuming MAX_GENTITIES to be 5 digits or less
#define MAX_ETOS_LENGTH (MAX_NAME_LENGTH + 5 * 2 + 4 + 1 + 5)
static bool matchesName( mapEntity_t const& ent, std::string const& name )
{
	for ( char const* n : ent.names )
	{
		if ( !Q_stricmp( name.c_str(), n ) )
		{
			return true;
		}
	}
	return false;
}

static char const* name0( mapEntity_t const& ent )
{
	static std::string buffer;
	if ( ent.names[0] == nullptr )
	{
		return "";
	}
	buffer = ent.names[0];
	buffer += ' ';
	return buffer.c_str();
}

const char *etos( const gentity_t *entity )
{
	static  int  index;
	static  char str[ 4 ][ MAX_ETOS_LENGTH ];
	char         *resultString;

	if(!entity)
		return "<NULL>";

	// use an array so that multiple etos have smaller chance of colliding
	resultString = str[ index ];
	index = ( index + 1 ) & 3;

	Com_sprintf( resultString, MAX_ETOS_LENGTH,
			"%s^7(^5%s^*|^5#%i^*)",
			name0( entity->mapEntity ), entity->classname, entity->num()
			);

	return resultString;
}

void G_PrintEntityNameList(gentity_t *entity)
{
	if(!entity)
	{
		Log::Notice("<NULL>");
		return;
	}

	char const* names = entity->mapEntity.nameList();
	Log::Notice("{ %s }", names);
}

/*
=================================================================================

gentity list handling and searching

=================================================================================
*/

/*
=============
G_IterateEntities

Iterates through all active entities optionally filtered by classname
and a fieldoffset (set via FOFS() macro) of the callers choosing.

Iteration will continue to return the gentity following the "previous" parameter that fullfill these conditions
or nullptr if there are no further matching gentities.

Set nullptr as previous gentity to start the iteration from the beginning
=============
*/
gentity_t *G_IterateEntities( gentity_t *entity, const char *classname, bool skipdisabled, size_t fieldofs, const char *match )
{
	char *fieldString;

	if ( !entity )
	{
		entity = g_entities;
		//start after the reserved player slots, if we are not searching for a player
		if ( classname && !strcmp(classname, S_PLAYER_CLASSNAME) )
			entity += MAX_CLIENTS;
	}
	else
	{
		entity++;
	}

	for ( ; entity < &g_entities[ level.num_entities ]; entity++ )
	{
		if ( !entity->inuse )
			continue;

		if( skipdisabled && !entity->enabled)
			continue;


		if ( classname && Q_stricmp( entity->classname, classname ) )
			continue;

		if ( fieldofs && match )
		{
			fieldString = * ( char ** )( ( byte * ) entity + fieldofs );
			if ( Q_stricmp( fieldString, match ) )
				continue;
		}

		return entity;
	}

	return nullptr;
}

gentity_t *G_IterateEntities( gentity_t *entity )
{
	return G_IterateEntities( entity, nullptr, true, 0, nullptr );
}

gentity_t *G_IterateEntitiesOfClass( gentity_t *entity, const char *classname )
{
	return G_IterateEntities( entity, classname, true, 0, nullptr );
}

/*
=============
Manage a mapping from entity IDs to entity numbers for quick lookup
=============
*/

static std::unordered_map<std::string, int> idToEntityNumMap;

int G_IdToEntityNum( Str::StringRef id )
{
	auto it = idToEntityNumMap.find( id );
	if ( it != idToEntityNumMap.end() )
	{
		return it->second;
	}
	else
	{
		return -1;
	}
}

void G_RegisterEntityId( int entityNum, Str::StringRef id )
{
	auto it = idToEntityNumMap.find( id );
	if ( it != idToEntityNumMap.end() )
	{
		Log::Warn( "G_RegisterEntityId: overwriting existing id `%s`", id );
		idToEntityNumMap.erase( it );
	}
	bool ok;
	std::tie( it, ok ) = idToEntityNumMap.insert( { id, entityNum } );
	if ( !ok )
	{
		Log::Warn( "G_RegisterEntityId: inserting id `%s` failed", id );
	}
}

void G_ForgetEntityId( Str::StringRef id )
{
	auto it = idToEntityNumMap.find( id );
	if ( it != idToEntityNumMap.end() )
	{
		idToEntityNumMap.erase( it );
	}
	else
	{
		Log::Warn( "G_ForgetEntityId: id `%s` does not exist", id );
	}
}

/*
=============
G_IterateEntitiesWithField

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the entity after from, or the beginning if nullptr
nullptr will be returned if the end of the list is reached.

if we are not searching for player entities it is recommended to start searching from gentities[MAX_CLIENTS - 1]

=============
*/
gentity_t *G_IterateEntitiesWithField( gentity_t *entity, size_t fieldofs, const char *match )
{
	return G_IterateEntities( entity, nullptr, true, fieldofs, match );
}

// from quakestyle.telefragged.com
// (NOBODY): Code helper function
//
gentity_t *G_IterateEntitiesWithinRadius( gentity_t *entity, const glm::vec3& origin, float radius )
{
	if ( !entity )
	{
		entity = g_entities;
	}
	else
	{
		entity++;
	}

	for ( ; entity < &g_entities[ level.num_entities ]; entity++ )
	{
		if ( !entity->inuse )
		{
			continue;
		}

		//TODO: (glm) remove temp copies when things will be vec3_t
		//  will only remains in bad memories
		glm::vec3 currentOrigin = VEC2GLM( entity->r.currentOrigin );
		glm::vec3 mins = VEC2GLM( entity->r.mins );
		glm::vec3 maxs = VEC2GLM( entity->r.maxs );
		glm::vec3 eorg = origin - ( currentOrigin + ( mins + maxs ) * 0.5f );

		if ( glm::length( eorg ) > radius )
		{
			continue;
		}

		return entity;
	}

	return nullptr;
}

gentity_t *G_PickRandomEntity( const char *classname, size_t fieldofs, const char *match )
{
	gentity_t *foundEntity = nullptr;
	int       totalChoiceCount = 0;
	gentity_t *choices[ MAX_GENTITIES - 2 - MAX_CLIENTS ];

	//collects the targets
	while( ( foundEntity = G_IterateEntities( foundEntity, classname, true, fieldofs, match ) ) != nullptr )
		choices[ totalChoiceCount++ ] = foundEntity;

	if ( !totalChoiceCount )
	{

		if ( g_debugEntities.Get() > -1 )
			Log::Warn( "Could not find any entity matching \"^5%s%s%s^*\"",
					classname ? classname : "",
					classname && match ? "^7 and ^5" :  "",
					match ? match : ""
					);

		return nullptr;
	}

	//return a random one from among the choices
	return choices[ rand() % totalChoiceCount ];
}

gentity_t *G_PickRandomEntityOfClass( const char *classname )
{
	return G_PickRandomEntity(classname, 0, nullptr);
}

gentity_t *G_PickRandomEntityWithField( size_t fieldofs, const char *match )
{
	return G_PickRandomEntity(nullptr, fieldofs, match);
}

/*
=================================================================================

gentity chain handling

=================================================================================
*/

/**
 * a non made call
 */
#define NULL_CALL gentityCall_t{ nullptr, &g_entities[ ENTITYNUM_NONE ], &g_entities[ ENTITYNUM_NONE ] }

struct entityCallEventDescription_t
{
	const char *key;
	gentityCallEvent_t eventType;
};

static const entityCallEventDescription_t gentityEventDescriptions[] =
{
		{ "onAct",       ON_ACT       },
		{ "onDie",       ON_DIE       },
		{ "onDisable",   ON_DISABLE   },
		{ "onEnable",    ON_ENABLE    },
		{ "onFree",      ON_FREE      },
		{ "onReach",     ON_REACH     },
		{ "onReset",     ON_RESET     },
		{ "onTouch",     ON_TOUCH     },
		{ "onUse",       ON_USE       },
		{ "target",      ON_DEFAULT   },
};

gentityCallEvent_t G_GetCallEventTypeFor( const char* event )
{
	entityCallEventDescription_t *foundDescription;

	if(!event)
		return ON_DEFAULT;

	foundDescription = (entityCallEventDescription_t*) bsearch(event, gentityEventDescriptions, ARRAY_LEN( gentityEventDescriptions ),
		             sizeof( entityCallEventDescription_t ), cmdcmp );

	if(foundDescription && foundDescription->key)
		return foundDescription->eventType;

	return ON_CUSTOM;
}

struct entityActionDescription_t
{
	const char *alias;
	gentityCallActionType_t action;
};

static const entityActionDescription_t actionDescriptions[] =
{
		{ "act",       ECA_ACT       },
		{ "disable",   ECA_DISABLE   },
		{ "enable",    ECA_ENABLE    },
		{ "free",      ECA_FREE      },
		{ "nop",       ECA_NOP       },
		{ "propagate", ECA_PROPAGATE },
		{ "reset",     ECA_RESET     },
		{ "toggle",    ECA_TOGGLE    },
		{ "use",       ECA_USE       },
};

gentityCallActionType_t G_GetCallActionTypeFor( const char* action )
{
	entityActionDescription_t *foundDescription;

	if(!action)
		return ECA_DEFAULT;

	foundDescription = (entityActionDescription_t*) bsearch(action, actionDescriptions, ARRAY_LEN( actionDescriptions ),
		             sizeof( entityActionDescription_t ), cmdcmp );

	if(foundDescription && foundDescription->alias)
		return foundDescription->action;

	return ECA_CUSTOM;
}

gentity_t *G_ResolveEntityKeyword( gentity_t *self, char *keyword )
{
	gentity_t *resolution = nullptr;

	if (!Q_stricmp(keyword, "$activator"))
		resolution = self->activator;
	else if (!Q_stricmp(keyword, "$self"))
		resolution = self;
	else if (!Q_stricmp(keyword, "$parent"))
		resolution = self->parent;
	else if (!Q_stricmp(keyword, "$target"))
		resolution = self->target ? self->target.entity : nullptr;
	//TODO $tracker for entities, that currently target, track or aim for this entity, is the reverse to "target"

	if(!resolution || !resolution->inuse)
		return nullptr;

	return resolution;
}

gentity_t *G_IterateTargets(gentity_t *entity, int *targetIndex, gentity_t *self)
{
	gentity_t *possibleTarget = nullptr;

	if (entity)
		goto cont;

	for (*targetIndex = 0; self->mapEntity.targets[*targetIndex]; ++(*targetIndex))
	{
		if(self->mapEntity.targets[*targetIndex][0] == '$')
		{
			possibleTarget = G_ResolveEntityKeyword( self, self->mapEntity.targets[*targetIndex] );
			if(possibleTarget && possibleTarget->enabled)
				return possibleTarget;
			return nullptr;
		}

		for( entity = &g_entities[ MAX_CLIENTS ]; entity < &g_entities[ level.num_entities ]; entity++ )
		{
			if ( !entity->inuse || !entity->enabled)
				continue;

			if ( matchesName( entity->mapEntity, self->mapEntity.targets[*targetIndex] ) )
			{
				return entity;
			}

			cont: ;
		}
	}
	return nullptr;
}

gentity_t *G_IterateCallEndpoints(gentity_t *entity, int *calltargetIndex, gentity_t *self)
{
	if (entity)
		goto cont;

	for (*calltargetIndex = 0; self->mapEntity.calltargets[*calltargetIndex].name; ++(*calltargetIndex))
	{
		if(self->mapEntity.calltargets[*calltargetIndex].name[0] == '$')
			return G_ResolveEntityKeyword( self, self->mapEntity.calltargets[*calltargetIndex].name );

		for( entity = &g_entities[ MAX_CLIENTS ]; entity < &g_entities[ level.num_entities ]; entity++ )
		{
			if ( !entity->inuse )
				continue;

			if ( matchesName( entity->mapEntity, self->mapEntity.calltargets[*calltargetIndex].name ) )
			{
				return entity;
			}

			cont: ;
		}
	}
	return nullptr;
}

/**
 * G_PickRandomTargetFor
 * Selects a random entity from among the targets
 */
gentity_t *G_PickRandomTargetFor( gentity_t *self )
{
	int       targetIndex;
	gentity_t *foundTarget = nullptr;
	int       totalChoiceCount = 0;
	gentity_t *choices[ MAX_GENTITIES ];

	//collects the targets
	while( ( foundTarget = G_IterateTargets( foundTarget, &targetIndex, self ) ) != nullptr )
		choices[ totalChoiceCount++ ] = foundTarget;

	if ( !totalChoiceCount )
	{
		if ( g_debugEntities.Get() > -1 )
		{
			Log::Warn( "none of the following targets could be resolved for Entity %s:", etos(self));
			G_PrintEntityNameList( self );
		}
		return nullptr;
	}

	//return a random one from among the choices
	return choices[ rand() / ( RAND_MAX / totalChoiceCount + 1 ) ];
}

struct gentityTargetChoice_t
{
	gentityCallDefinition_t *callDefinition;
	gentity_t *recipient;
};

void G_FireEntityRandomly( gentity_t *entity, gentity_t *activator )
{
	int       targetIndex;
	gentity_t *possibleTarget = nullptr;
	int       totalChoiceCount = 0;
	gentityCall_t call;
	gentityTargetChoice_t choices[ MAX_GENTITIES ];
	gentityTargetChoice_t *selectedChoice;

	//collects the targets
	while( ( possibleTarget = G_IterateCallEndpoints( possibleTarget, &targetIndex, entity ) ) != nullptr )
	{
		choices[ totalChoiceCount ].recipient = possibleTarget;
		choices[ totalChoiceCount ].callDefinition = &entity->mapEntity.calltargets[targetIndex];
		totalChoiceCount++;
	}

	if ( totalChoiceCount == 0 )
		return;

	//return a random one from among the choices
	selectedChoice = &choices[ rand() / ( RAND_MAX / totalChoiceCount + 1 ) ];

	call.definition = selectedChoice->callDefinition;
	call.caller = entity;
	call.activator = activator;

	G_CallEntity( selectedChoice->recipient, &call );
}

/*
==============================
G_EventFireEntity

"activator" should be set to the entity that initiated the firing.

For all t in the entities, where t.targetnames[i] matches
ent.targets[j] for any (i,j) pairs, call the t.use function.
==============================
*/
void G_EventFireEntity( gentity_t *self, gentity_t *activator, gentityCallEvent_t eventType )
{
	gentity_t *currentTarget = nullptr;
	int targetIndex;
	gentityCall_t call;
	call.activator = activator;

	// Shader replacement. Example usage: map "habitat" elevator activation, red to green light replacement
	if ( self->mapEntity.shaderKey && self->mapEntity.shaderReplacement )
	{
		G_SetShaderRemap( self->mapEntity.shaderKey, self->mapEntity.shaderReplacement, level.time * 0.001 );
		trap_SetConfigstring( CS_SHADERSTATE, BuildShaderStateConfig() );
	}

	while( ( currentTarget = G_IterateCallEndpoints( currentTarget, &targetIndex, self ) ) != nullptr )
	{
		if( eventType && self->mapEntity.calltargets[ targetIndex ].eventType != eventType )
		{
			continue;
		}

		call.caller = self; //reset the caller in case there have been nested calls
		call.definition = &self->mapEntity.calltargets[ targetIndex ];

		G_CallEntity(currentTarget, &call);

		if ( !self->inuse )
		{
			Log::Warn( "entity was removed while using targets" );
			return;
		}
	}
}

void G_FireEntity( gentity_t *self, gentity_t *activator )
{
	G_EventFireEntity( self, activator, ON_DEFAULT );
}

/**
 * executes the entities act function
 * This is basically nothing but a wrapper around act() ensuring a correct call,
 * neither parameter may be nullptr, and the entity is required to have an act function to execute
 * or this function will fail
 */
void G_ExecuteAct( gentity_t *entity, gentityCall_t *call )
{
	ASSERT(entity->act != nullptr);
	ASSERT(call != nullptr);

	//ASSERT(entity->callIn->activator != nullptr);

	entity->nextAct = 0;
	/*
	 * for now we use the callIn activator if its set or fallback to the old solution, but we should
	 * //TODO remove the old solution of activator setting from this
	 */
	entity->act(entity, call->caller, call->caller->activator ? call->caller->activator : entity->activator );
}

/**
 * check delayed variable and either call an entity act() directly or delay its execution
 */
static void G_ResetTimeField( variatingTime_t *result, variatingTime_t instanceField, variatingTime_t classField, variatingTime_t fallback )
{
	if( instanceField.time && instanceField.time > 0 )
	{
		*result = instanceField;
	}
	else if (classField.time && classField.time > 0 )
	{
		*result = classField;
	}
	else
	{
		*result = fallback;
	}

	if ( result->variance < 0 )
	{
		result->variance = 0;

		if( g_debugEntities.Get() >= 0 )
		{
			Log::Warn( "negative variance (%f); resetting to 0", result->variance );
		}
	}
	else if ( result->variance >= result->time && result->variance > 0)
	{
		result->variance = result->time - FRAMETIME;

		if( g_debugEntities.Get() > 0 )
		{
			Log::Warn( "limiting variance (%f) to be smaller than time (%f)", result->variance, result->time );
		}
	}
}

void G_HandleActCall( gentity_t *entity, gentityCall_t *call )
{
	variatingTime_t delay = {0, 0};

	ASSERT(call != nullptr);
	entity->callIn = *call;

	G_ResetTimeField(&delay, entity->mapEntity.config.delay, entity->mapEntity.eclass->config.delay, delay );

	if(delay.time)
	{
		entity->nextAct = VariatedLevelTime( delay );
	}
	else /* no time and variance set means, we can call it directly instead of waiting for the next frame */
	{
		G_ExecuteAct( entity, call );
	}
}

void G_CallEntity(gentity_t *targetedEntity, gentityCall_t *call)
{
	if ( g_debugEntities.Get() > 1 )
	{
		Log::Debug("[%s] %s calling %s %s:%s",
				etos( call->activator ),
				etos( call->caller ),
				call->definition ? call->definition->event : "onUnknown",
				etos( targetedEntity ),
				call->definition && call->definition->action ? call->definition->action : "default");
	}

	targetedEntity->callIn = *call;

	if(call->definition)
	{
		switch (call->definition->actionType)
		{
		case ECA_NOP:
			break;

		case ECA_CUSTOM:
			if ( g_debugEntities.Get() > -1 )
			{
				Log::Warn("Unknown action \"%s\" for %s",
						call->definition->action, etos(targetedEntity));
			}
			break;

		case ECA_FREE:
			G_FreeEntity(targetedEntity);
			return; //we have to handle notification differently in the free-case

		case ECA_PROPAGATE:
			G_FireEntity( targetedEntity, call->activator);
			break;

		case ECA_ENABLE:
			if(!targetedEntity->enabled) //only fire an event if we weren't already enabled
			{
				targetedEntity->enabled = true;
				G_EventFireEntity( targetedEntity, call->activator, ON_ENABLE );
			}
			break;
		case ECA_DISABLE:
			if(targetedEntity->enabled) //only fire an event if we weren't already disabled
			{
				targetedEntity->enabled = false;
				G_EventFireEntity( targetedEntity, call->activator, ON_DISABLE );
			}
			break;
		case ECA_TOGGLE:
			targetedEntity->enabled = !targetedEntity->enabled;
			G_EventFireEntity( targetedEntity, call->activator, targetedEntity->enabled ? ON_ENABLE : ON_DISABLE );
			break;

		case ECA_USE:
			if (!targetedEntity->use)
			{
				if(g_debugEntities.Get() >= 0)
					Log::Warn("calling :use on %s, which has no use function!", etos(targetedEntity));
				break;
			}
			if(!call->activator || !call->activator->client)
			{
				if(g_debugEntities.Get() >= 0)
					Log::Warn("calling %s:use, without a client as activator.", etos(targetedEntity));
				break;
			}
			targetedEntity->use(targetedEntity, call->caller, call->activator);
			break;
		case ECA_RESET:
			if (targetedEntity->reset)
			{
				targetedEntity->reset(targetedEntity);
				G_EventFireEntity( targetedEntity, call->activator, ON_RESET );
			}
			break;
		case ECA_ACT:
			G_HandleActCall( targetedEntity, call );
			break;

		default:
			if (targetedEntity->act)
				targetedEntity->act(targetedEntity, call->caller, call->activator);
			break;
		}
	}

	targetedEntity->callIn = NULL_CALL; /**< not called anymore */
}

/*
===============
G_IsVisible

Test for a LOS between two entities
===============
*/
bool G_IsVisible( gentity_t *start, gentity_t *end, int contents )
{
	trace_t trace;

	trap_Trace( &trace, start->s.pos.trBase, nullptr, nullptr, end->s.pos.trBase,
	            start->num(), contents, 0 );

	return trace.fraction >= 1.0f || trace.entityNum == end->num();
}

/*
=================================================================================

gentity configuration

=================================================================================
*/

/*
===============
G_SetMovedir

The editor only specifies a single value for angles (yaw),
but we have special constants to generate an up or down direction.
Angles will be cleared, because it is being used to represent a direction
instead of an orientation.
===============
*/
void G_SetMovedir( glm::vec3& angles, glm::vec3& movedir )
{
	static glm::vec3 VEC_UP = { 0, -1, 0 };
	static glm::vec3 MOVEDIR_UP = { 0, 0, 1 };
	static glm::vec3 VEC_DOWN = { 0, -2, 0 };
	static glm::vec3 MOVEDIR_DOWN = { 0, 0, -1 };

	if ( angles == VEC_UP )
	{
		movedir = MOVEDIR_UP;
	}
	else if ( angles == VEC_DOWN )
	{
		movedir = MOVEDIR_DOWN;
	}
	else
	{
		AngleVectors( angles, &movedir, nullptr, nullptr );
	}

	angles = glm::vec3();
}

/*
================
G_SetOrigin

Sets the pos trajectory for a fixed position
================
*/
void G_SetOrigin( gentity_t *self, const glm::vec3& origin )
{
	VectorCopy( origin, self->s.pos.trBase );
	self->s.pos.trType = trType_t::TR_STATIONARY;
	self->s.pos.trTime = 0;
	self->s.pos.trDuration = 0;
	VectorClear( self->s.pos.trDelta );

	VectorCopy( origin, self->r.currentOrigin );
	VectorCopy( origin, self->s.origin );
}
