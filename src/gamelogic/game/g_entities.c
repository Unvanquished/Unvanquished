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

#include "g_local.h"

/*
=================================================================================

basic gentity lifecycle handling

=================================================================================
*/

void G_InitGentity( gentity_t *e )
{
	e->inuse = qtrue;
	e->classname = "noclass";
	e->s.number = e - g_entities;
	e->r.ownerNum = ENTITYNUM_NONE;
}

/*
=================
G_Spawn

Either finds a free entity, or allocates a new one.

  The slots from 0 to MAX_CLIENTS-1 are always reserved for clients, and will
never be used by anything else.

Try to avoid reusing an entity that was recently freed, because it
can cause the client to think the entity morphed into something else
instead of being removed and recreated, which can cause interpolated
angles and bad trails.
=================
*/
gentity_t *G_Spawn( void )
{
	int       i, force;
	gentity_t *e;

	e = NULL; // shut up warning
	i = 0; // shut up warning

	for ( force = 0; force < 2; force++ )
	{
		// if we go through all entities first and can't find a free one,
		// then try again a second time, this time ignoring times
		e = &g_entities[ MAX_CLIENTS ];

		for ( i = MAX_CLIENTS; i < level.num_entities; i++, e++ )
		{
			if ( e->inuse )
			{
				continue;
			}

			// the first couple seconds of server time can involve a lot of
			// freeing and allocating, so relax the replacement policy
			if ( !force && e->freetime > level.startTime + 2000 && level.time - e->freetime < 1000 )
			{
				continue;
			}

			// reuse this slot
			G_InitGentity( e );
			return e;
		}

		if ( i != MAX_GENTITIES )
		{
			break;
		}
	}

	if ( i == ENTITYNUM_MAX_NORMAL )
	{
		for ( i = 0; i < MAX_GENTITIES; i++ )
		{
			G_Printf( "%4i: %s\n", i, g_entities[ i ].classname );
		}

		G_Error( "G_Spawn: no free entities" );
	}

	// open up a new slot
	level.num_entities++;

	// let the server system know that there are more entities
	trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
	                     &level.clients[ 0 ].ps, sizeof( level.clients[ 0 ] ) );

	G_InitGentity( e );
	return e;
}

/*
=================
G_FreeEntity

Marks the entity as free
=================
*/
void G_FreeEntity( gentity_t *ent )
{
	trap_UnlinkEntity( ent );  // unlink from world

	if ( ent->neverFree )
	{
		return;
	}

	if ( g_debugEntities.integer > 2 )
		G_Printf("Debug: Freeing Entity ^5#%i^7 of type ^5%s\n", ent->s.number, ent->classname);

	if( ent->eclass && ent->eclass->instanceCounter > 0)
		ent->eclass->instanceCounter--;

	memset( ent, 0, sizeof( *ent ) );
	ent->classname = "freent";
	ent->freetime = level.time;
	ent->inuse = qfalse;
}


/*
=================
G_TempEntity

Spawns an event entity that will be auto-removed
The origin will be snapped to save net bandwidth, so care
must be taken if the origin is right on a surface (snap towards start vector first)
=================
*/
gentity_t *G_TempEntity( const vec3_t origin, int event )
{
	gentity_t *e;
	vec3_t    snapped;

	e = G_Spawn();
	e->s.eType = ET_EVENTS + event;

	e->classname = "tempEntity";
	e->eventTime = level.time;
	e->freeAfterEvent = qtrue;

	VectorCopy( origin, snapped );
	SnapVector( snapped );  // save network bandwidth
	G_SetOrigin( e, snapped );

	// find cluster for PVS
	trap_LinkEntity( e );

	return e;
}

/*
=================================================================================

gentity debuging

=================================================================================
*/

void G_DebugPrintEntitiy(gentity_t *entity)
{
	if(!entity)
	{
		G_Printf("<NULL>");
		return;
	}

	if(entity->names[0])
		G_Printf("%s ", entity->names[0]);

	G_Printf("^7(^5%s^7|^5#%i^7)", entity->classname, entity->s.number);
}

/*
=================================================================================

gentity list handling and searching

=================================================================================
*/

/*
=============
G_Find

Searches all active entities for the next one that holds
the matching string at fieldofs (use the FOFS() macro) in the structure.

Searches beginning at the entity after from, or the beginning if NULL
NULL will be returned if the end of the list is reached.

=============
*/
gentity_t *G_FindNextEntity( gentity_t *from, size_t fieldofs, const char *match )
{
	char *s;

	if ( !from )
	{
		from = g_entities;
	}
	else
	{
		from++;
	}

	for ( ; from < &g_entities[ level.num_entities ]; from++ )
	{
		if ( !from->inuse || !from->enabled )
		{
			continue;
		}

		s = * ( char ** )( ( byte * ) from + fieldofs );

		if ( !s )
		{
			continue;
		}

		if ( !Q_stricmp( s, match ) )
		{
			return from;
		}
	}

	return NULL;
}

gentity_t *G_PickRandomEntity( size_t fieldofs, const char *match  )
{
	gentity_t *foundEntity;
	int       totalChoiceCount = 0;
	gentity_t *choices[ MAX_GENTITIES - 2 - MAX_CLIENTS ];

	/*
	 * we either want to pick a random player or non-player
	 * if we actually want a player, we need another dedicated function for it anyway
	 * so lets skip the playerslots
	 */
	foundEntity = &g_entities[ MAX_CLIENTS - 1 ];

	//collects the targets
	while( ( foundEntity = G_FindNextEntity( foundEntity, fieldofs, match ) ) != NULL )
		choices[ totalChoiceCount++ ] = foundEntity;

	if ( !totalChoiceCount )
	{

		if ( g_debugEntities.integer > -1 )
			G_Printf( "^3WARNING: ^7Could not find any entity matching \"^5%s^7\"\n", match );

		return NULL;
	}

	//return a random one from among the choices
	return choices[ rand() / ( RAND_MAX / totalChoiceCount + 1 ) ];
}

// from quakestyle.telefragged.com
// (NOBODY): Code helper function
//
gentity_t *G_FindRadius( gentity_t *from, vec3_t org, float rad )
{
	vec3_t eorg;
	int    j;

	if ( !from )
	{
		from = g_entities;
	}
	else
	{
		from++;
	}

	for ( ; from < &g_entities[ level.num_entities ]; from++ )
	{
		if ( !from->inuse )
		{
			continue;
		}

		for ( j = 0; j < 3; j++ )
		{
			eorg[ j ] = org[ j ] - ( from->r.currentOrigin[ j ] + ( from->r.mins[ j ] + from->r.maxs[ j ] ) * 0.5 );
		}

		if ( VectorLength( eorg ) > rad )
		{
			continue;
		}

		return from;
	}

	return NULL;
}

/*
===============
G_ClosestEnt

Test a list of entities for the closest to a particular point
===============
*/
gentity_t *G_ClosestEnt( vec3_t origin, gentity_t **entities, int numEntities )
{
	int       i;
	float     nd, d;
	gentity_t *closestEnt;

	if ( numEntities <= 0 )
	{
		return NULL;
	}

	closestEnt = entities[ 0 ];
	d = DistanceSquared( origin, closestEnt->s.origin );

	for ( i = 1; i < numEntities; i++ )
	{
		gentity_t *ent = entities[ i ];

		nd = DistanceSquared( origin, ent->s.origin );

		if ( nd < d )
		{
			d = nd;
			closestEnt = ent;
		}
	}

	return closestEnt;
}

/*
=================================================================================

gentity chain handling

=================================================================================
*/

gentity_t *G_FindNextTarget(gentity_t *currentTarget, int *targetIndex, int *nameIndex, gentity_t *self)
{
	if (currentTarget)
		goto cont;

	for (*targetIndex = 0; self->targets[*targetIndex].name; ++(*targetIndex))
	{
		for( currentTarget = &g_entities[ 0 ]; currentTarget < &g_entities[ level.num_entities ]; ++currentTarget )
		{
			for (*nameIndex = 0; currentTarget->names[*nameIndex]; ++(*nameIndex))
			{
				if (!Q_stricmp(self->targets[*targetIndex].name, currentTarget->names[*nameIndex]))
					return currentTarget;
				cont: ;
			}
		}
	}
	return NULL;
}

/**
 * G_PickRandomTargetFor
 * Selects a random entity from among the targets
 */
gentity_t *G_PickRandomTargetFor( gentity_t *self )
{
	int       targetIndex, nameIndex;
	gentity_t *foundTarget = NULL;
	int       totalChoiceCount = 0;
	gentity_t *choices[ MAX_GENTITIES ];

	//collects the targets
	while( ( foundTarget = G_FindNextTarget( foundTarget, &targetIndex, &nameIndex, self ) ) != NULL )
		choices[ totalChoiceCount++ ] = foundTarget;

	if ( !totalChoiceCount )
	{
		if ( g_debugEntities.integer > -1 )
		{
			G_Printf( "^3WARNING: ^7none of the following targets could be resolved for Entity ^5#%i^7:", self->s.number );

			for( targetIndex = 0; self->targets[ targetIndex ].name; ++targetIndex )
			  G_Printf( "%s%s", ( targetIndex == 0 ? "" : ", " ), self->targets[ targetIndex ].name );
			G_Printf( "\n" );
		}
		return NULL;
	}

	//return a random one from among the choices
	return choices[ rand() / ( RAND_MAX / totalChoiceCount + 1 ) ];
}

typedef struct
{
	gentityCallDefinition_t *target;
	gentity_t *recipient;
} gentityTargetChoice_t;

void G_FireRandomTargetOf( gentity_t *entity, gentity_t *activator )
{
	int       targetIndex, nameIndex;
	gentity_t *possbileTarget = NULL;
	int       totalChoiceCount = 0;
	gentityTargetChoice_t choices[ MAX_GENTITIES ];
	gentityTargetChoice_t *selectedChoice;

	//collects the targets
	while( ( possbileTarget = G_FindNextTarget( possbileTarget, &targetIndex, &nameIndex, entity ) ) != NULL )
	{
		choices[ totalChoiceCount ].recipient = possbileTarget;
		choices[ totalChoiceCount ].target = &entity->targets[targetIndex];
		totalChoiceCount++;
	}

	//return a random one from among the choices
	selectedChoice = &choices[ rand() / ( RAND_MAX / totalChoiceCount + 1 ) ];
	if (!selectedChoice)
		return;

	G_FireTarget( selectedChoice->target, selectedChoice->recipient, entity, activator );
}

/*
==============================
G_FireAllTargetsOf

"activator" should be set to the entity that initiated the firing.

For all t in the entities, where t.targetnames[i] matches
ent.targets[j] for any (i,j) pairs, call the t.use function.
==============================
*/
void G_FireAllTargetsOf( gentity_t *self, gentity_t *activator )
{
	gentity_t *currentTarget = NULL;
	int targetIndex, nameIndex;

	if ( self->targetShaderName && self->targetShaderNewName )
	{
		float f = level.time * 0.001;
		AddRemap( self->targetShaderName, self->targetShaderNewName, f );
		trap_SetConfigstring( CS_SHADERSTATE, BuildShaderStateConfig() );
	}

	while( ( currentTarget = G_FindNextTarget( currentTarget, &targetIndex, &nameIndex, self ) ) != NULL )
	{
		G_FireTarget( &self->targets[ targetIndex ], currentTarget, self, activator );

		if ( !self->inuse )
		{
			G_Printf( "entity was removed while using targets\n" );
			return;
		}
	}
}

void G_FireTarget(gentityCallDefinition_t *target, gentity_t *targetedEntity, gentity_t *caller, gentity_t *activator)
{
	if ( g_debugEntities.integer > 1 )
	{
		G_Printf("Debug: [");
		G_DebugPrintEntitiy(activator);
		G_Printf("] ");
		G_DebugPrintEntitiy(caller);
		G_Printf(" → ");
		G_DebugPrintEntitiy(targetedEntity);
		G_Printf(":%s\n", target && target->action ? target->action : "default");
	}

	if(!targetedEntity->handleCall || !targetedEntity->handleCall(targetedEntity, target, caller, activator))
	{
		switch (target->actionType)
		{
		case ECA_CUSTOM:
			if ( g_debugEntities.integer > -1 )
			{
				G_Printf("^3Warning:^7 Unknown action \"%s\" for ", target->action) ;
				G_DebugPrintEntitiy(targetedEntity);
				G_Printf("\n");
			}
			return;

		case ECA_FREE:
			G_FreeEntity(targetedEntity);
			return; //we have to handle notification differently in the free-case

		case ECA_PROPAGATE:
			G_FireAllTargetsOf( targetedEntity, activator);
			break;

		case ECA_ENABLE:
			targetedEntity->enabled = qtrue;
			break;
		case ECA_DISABLE:
			targetedEntity->enabled = qfalse;
			break;
		case ECA_TOGGLE:
			targetedEntity->enabled = !targetedEntity->enabled;
			break;

		case ECA_USE:
			if (targetedEntity->use)
				targetedEntity->use(targetedEntity, caller, activator);
			break;
		case ECA_RESET:
			if (targetedEntity->reset)
				targetedEntity->reset(targetedEntity);
			break;
		case ECA_ACT:
			if (targetedEntity->act)
				targetedEntity->act(target, targetedEntity, caller, activator);
			break;

		default:
			//by default call act, or fall back to use as a means of backward compatibility, until everything we need has a proper act function
			if (targetedEntity->act)
				targetedEntity->act(target, targetedEntity, caller, activator);
			else if (targetedEntity->use)
				targetedEntity->use(targetedEntity, caller, activator);
			break;
		}
	}
	if(targetedEntity->notify)
		targetedEntity->notify( targetedEntity, target );
}

/*
=================================================================================

gentity testing/querying

=================================================================================
*/

/*
=================
G_EntitiesFree
=================
*/
qboolean G_EntitiesFree( void )
{
	int       i;
	gentity_t *e;

	e = &g_entities[ MAX_CLIENTS ];

	for ( i = MAX_CLIENTS; i < level.num_entities; i++, e++ )
	{
		if ( e->inuse )
		{
			continue;
		}

		// slot available
		return qtrue;
	}

	return qfalse;
}

/*
===============
G_Visible

Test for a LOS between two entities
===============
*/
qboolean G_Visible( gentity_t *ent1, gentity_t *ent2, int contents )
{
	trace_t trace;

	trap_Trace( &trace, ent1->s.pos.trBase, NULL, NULL, ent2->s.pos.trBase,
	            ent1->s.number, contents );

	return trace.fraction >= 1.0f || trace.entityNum == ent2 - g_entities;
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
void G_SetMovedir( vec3_t angles, vec3_t movedir )
{
	static vec3_t VEC_UP = { 0, -1, 0 };
	static vec3_t MOVEDIR_UP = { 0, 0, 1 };
	static vec3_t VEC_DOWN = { 0, -2, 0 };
	static vec3_t MOVEDIR_DOWN = { 0, 0, -1 };

	if ( VectorCompare( angles, VEC_UP ) )
	{
		VectorCopy( MOVEDIR_UP, movedir );
	}
	else if ( VectorCompare( angles, VEC_DOWN ) )
	{
		VectorCopy( MOVEDIR_DOWN, movedir );
	}
	else
	{
		AngleVectors( angles, movedir, NULL, NULL );
	}

	VectorClear( angles );
}

/*
================
G_SetOrigin

Sets the pos trajectory for a fixed position
================
*/
void G_SetOrigin( gentity_t *ent, const vec3_t origin )
{
	VectorCopy( origin, ent->s.pos.trBase );
	ent->s.pos.trType = TR_STATIONARY;
	ent->s.pos.trTime = 0;
	ent->s.pos.trDuration = 0;
	VectorClear( ent->s.pos.trDelta );

	VectorCopy( origin, ent->r.currentOrigin );
	VectorCopy( origin, ent->s.origin );
}

/**
 * predefined field interpretations
 */
void G_SetNextthink( gentity_t *self ) {
	self->nextthink = level.time + ( self->config.wait.time + self->config.wait.variance * crandom() ) * 1000;
}
