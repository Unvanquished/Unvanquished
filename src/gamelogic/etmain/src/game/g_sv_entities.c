/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

/*
 * name:    g_sv_entities.c
 *
 * desc:    Server sided only map entities
*/

#include "g_local.h"

// TAT 11/13/2002
//      The SP uses entities that have no physical manifestation in the game, are are used simply
//      as locational indicators - seek cover spots, and ai markers, for example
//      This is an alternate system so those don't clutter up the real game entities

// how many of these entities?
#define MAX_SERVER_ENTITIES 4096

// for now, statically allocate them
g_serverEntity_t g_serverEntities[ MAX_SERVER_ENTITIES ];
int              numServerEntities;

// clear out all the sp entities
void InitServerEntities( void )
{
	memset( g_serverEntities, 0, sizeof( g_serverEntities ) );
	numServerEntities = 0;
}

// get the server entity with the passed in number
g_serverEntity_t *GetServerEntity( int num )
{
	// if it's an invalid number, return null
	if( ( num < MAX_GENTITIES ) || ( num >= MAX_GENTITIES + numServerEntities ) )
	{
		return NULL;
	}

	return &g_serverEntities[ num - MAX_GENTITIES ];
}

g_serverEntity_t *GetFreeServerEntity()
{
	// NOTE:  this is simplistic because we can't currently free these entities
	//      if we change this, then we need to be more careful when allocating the entities
	if( numServerEntities >= MAX_SERVER_ENTITIES )
	{
		G_Error( "GetFreeServerEntity: Cannot allocate server entity" );
		return NULL;
	}

	g_serverEntities[ numServerEntities ].number = MAX_GENTITIES + numServerEntities;
	g_serverEntities[ numServerEntities ].inuse = qtrue;
	return &g_serverEntities[ numServerEntities++ ];
}

// Give a gentity_t, create a sp entity, copy all pertinent data, and return it
g_serverEntity_t *CreateServerEntity( gentity_t *ent )
{
	// get the entity out of our pool
	g_serverEntity_t *newEnt = GetFreeServerEntity();

	// if we managed to get one, copy over data
	if( newEnt )
	{
		// G_NewString crashes if you pass in NULL, so let's check...
		if( ent->classname )
		{
			newEnt->classname = G_NewString( ent->classname );
		}

		if( ent->targetname )
		{
			newEnt->name = G_NewString( ent->targetname );
		}

		if( ent->target )
		{
			newEnt->target = G_NewString( ent->target );
		}

		newEnt->spawnflags = ent->spawnflags;
		newEnt->team = ent->aiTeam;
		VectorCopy( ent->s.origin, newEnt->origin );
		VectorCopy( ent->s.angles, newEnt->angles );
		// DON'T set the number - that should have been set when it was spawned

		// set the areanum to -1, which means we haven't calculated it yet
		//      these don't move, so we should only have to calc it once, the first
		//      time someone asks for it
		newEnt->areaNum = -1;
	}

	return newEnt;
}

// TAT - create the server entities for the current map
void CreateMapServerEntities();

// These server entities don't get to update every frame, but some of them have to set themselves up
//      after they've all been created
//      So we want to give each entity the chance to set itself up after it has been created
void InitialServerEntitySetup()
{
	int              i;
	g_serverEntity_t *ent;

	// TAT - create the server entities for the current map
	//      these are read from an additional file
	CreateMapServerEntities();

	for( i = 0; i < numServerEntities; i++ )
	{
		ent = &g_serverEntities[ i ];

		// if this entity is in use and has a setup function
		if( ent->inuse && ent->setup )
		{
			// call it
			ent->setup( ent );
		}
	}
}

// Like G_Find, but for server entities
g_serverEntity_t *FindServerEntity( g_serverEntity_t *from, int fieldofs, char *match )
{
	char             *s;
	g_serverEntity_t *max = &g_serverEntities[ numServerEntities ];

	if( !from )
	{
		from = g_serverEntities;
	}
	else
	{
		from++;
	}

	for( ; from < max; from++ )
	{
		if( !from->inuse )
		{
			continue;
		}

		s = * ( char ** )( ( byte * ) from + fieldofs );

		if( !s )
		{
			continue;
		}

		if( !Q_stricmp( s, match ) )
		{
			return from;
		}
	}

	return NULL;
}

// TAT 11/18/2002
//      For the (possibly temporary) system of loading a separate file of these server entities
extern void SP_SeekCover_Setup( g_serverEntity_t *ent );
extern void SP_AIMarker_Setup( g_serverEntity_t *ent );

// We have to hardcode the setup functions for these
void InitServerEntitySetupFunc( g_serverEntity_t *ent )
{
	if( strcmp( ent->classname, "ai_marker" ) == 0 )
	{
		ent->setup = SP_AIMarker_Setup;
	}
	else if( strcmp( ent->classname, "bot_seek_cover_spot" ) == 0 )
	{
		// set the team to allies
		ent->team = TEAM_ALLIES;
		// set the setup func
		ent->setup = SP_SeekCover_Setup;
	}
	else if( strcmp( ent->classname, "bot_axis_seek_cover_spot" ) == 0 )
	{
		// set the team to axis
		ent->team = TEAM_AXIS;
		// set the setup func
		ent->setup = SP_SeekCover_Setup;
	}
}

// Create a server entity from some basic data
void CreateServerEntityFromData( char *classname, char *targetname, char *target, vec3_t origin, int spawnflags, vec3_t angle )
{
	// get the entity out of our pool
	g_serverEntity_t *newEnt = GetFreeServerEntity();

	// if we managed to get one, copy over data
	if( newEnt )
	{
		// G_NewString crashes if you pass in NULL, so let's check...
		if( classname )
		{
			newEnt->classname = G_NewString( classname );
		}

		if( targetname )
		{
			newEnt->name = G_NewString( targetname );
		}

		if( target )
		{
			newEnt->target = G_NewString( target );
		}

		newEnt->spawnflags = spawnflags;
		//newEnt->team = ent->aiTeam;
		VectorCopy( origin, newEnt->origin );
		VectorCopy( angle, newEnt->angles );
		// DON'T set the number - that should have been set when it was spawned

		// set the areanum to -1, which means we haven't calculated it yet
		//      these don't move, so we should only have to calc it once, the first
		//      time someone asks for it
		newEnt->areaNum = -1;

		// and do class specific stuff
		InitServerEntitySetupFunc( newEnt );
	}
}

// TAT - create the server entities for the current map
void CreateMapServerEntities()
{
	char info[ 1024 ];
	char mapname[ 128 ];

	trap_GetServerinfo( info, sizeof( info ) );

	Q_strncpyz( mapname, Info_ValueForKey( info, "mapname" ), sizeof( mapname ) );
}
