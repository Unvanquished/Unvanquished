/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2009 Alex Lo <xycaleth@gmail.com>
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifdef USE_PHYSICS

#ifdef DEDICATED
#define _NEWTON_USE_LIB
#else
#define _NEWTON_BUILD_DLL
#endif

#include <vector>
#include <map>
#include "Newton.h"
#include "dVector.h"
#include "dMatrix.h"

extern "C"
{
#include "../server/server.h"
}

#define METRES_PER_UNIT 32.0f
#define UNITS_PER_METRE ( 1.0f / METRES_PER_UNIT )

static int            defaultMaterialGroup;

static NewtonWorld     *g_world = NULL;
static NewtonCollision *g_collision;
static NewtonBody      *g_rigidBody;

static void            *AllocMemory( int sizeInBytes );
static void           FreeMemory( void *ptr, int sizeInBytes );

struct bspCmodel
{
	bspCmodel() : index( 0 ), firstSurface( 0 ), numSurfaces( 0 ), rigidBody( NULL )
	{
	}

	int                    index;
	int                    firstSurface, numSurfaces;
	std::vector< dVector > vertices;
	NewtonBody             *rigidBody;
};

std::map< int, bspCmodel > bspModels;

extern "C" void CMod_PhysicsInit()
{
	// create the Newton World
	bspModels.clear();
	g_world = NewtonCreate( AllocMemory, FreeMemory );

	// use the standard x86 floating point model
	NewtonSetPlatformArchitecture( g_world, 0 );

	// Set up default material properties for newton
	defaultMaterialGroup = NewtonMaterialGetDefaultGroupID( g_world );
	NewtonMaterialSetDefaultFriction( g_world, defaultMaterialGroup, defaultMaterialGroup, 0.9f, 0.5f );
	NewtonMaterialSetDefaultElasticity( g_world, defaultMaterialGroup, defaultMaterialGroup, 0.4f );
	NewtonMaterialSetDefaultSoftness( g_world, defaultMaterialGroup, defaultMaterialGroup, 0.1f );
	NewtonMaterialSetCollisionCallback( g_world, defaultMaterialGroup, defaultMaterialGroup, NULL, NULL, NULL );
	NewtonMaterialSetDefaultCollidable( g_world, defaultMaterialGroup, defaultMaterialGroup, 1 );

	// configure the Newton world to use iterative solve mode 0
	// this is the most efficient but the less accurate mode
	NewtonSetSolverModel( g_world, 8 );
	NewtonSetFrictionModel( g_world, 0 );

	g_collision = NULL;
}

// this is the callback for allocating Newton memory
void *AllocMemory( int sizeInBytes )
{
	return malloc( sizeInBytes );
}

// this is the callback for freeing Newton Memory
void FreeMemory( void *ptr, int sizeInBytes )
{
	free( ptr );
}

extern "C" void CMod_PhysicsClearBodies()
{
	for ( std::map< int, bspCmodel >::iterator it = bspModels.begin();
	      it != bspModels.end();
	      ++it )
	{
		if ( it->second.rigidBody == NULL )
		{
			continue;
		}

		NewtonDestroyBody( g_world, it->second.rigidBody );
	}
}

extern "C" void CMod_PhysicsShutdown()
{
	if ( g_world != NULL )
	{
		NewtonDestroy( g_world );
		g_world = NULL;
	}
}

extern "C" void CMod_PhysicsAddBSPFace( vec3_t vec[ 3 ] )
{
	VectorScale( vec[ 0 ], UNITS_PER_METRE, vec[ 0 ] );
	VectorScale( vec[ 1 ], UNITS_PER_METRE, vec[ 1 ] );
	VectorScale( vec[ 2 ], UNITS_PER_METRE, vec[ 2 ] );

	NewtonTreeCollisionAddFace( g_collision, 3, &vec[ 0 ][ 0 ], sizeof( vec3_t ), NewtonMaterialGetDefaultGroupID( g_world ) );
}

extern "C" void CMod_PhysicsBeginBSPCollisionTree()
{
	g_collision = NewtonCreateTreeCollision( g_world );
	NewtonTreeCollisionBeginBuild( g_collision );
}

extern "C" void CMod_PhysicsEndBSPCollisionTree()
{
	dVector worldMin, worldMax;
	NewtonTreeCollisionEndBuild( g_collision, 1 );

	g_rigidBody = NewtonCreateBody( g_world, g_collision );
	NewtonReleaseCollision( g_world, g_collision );

	dMatrix worldMatrix( GetIdentityMatrix() );
	NewtonBodySetMatrix( g_rigidBody, &worldMatrix[ 0 ][ 0 ] );

	NewtonCollisionCalculateAABB( g_collision, &worldMatrix[ 0 ][ 0 ], &worldMin[ 0 ], &worldMax[ 0 ] );
	NewtonSetWorldSize( g_world, &worldMin[ 0 ], &worldMax[ 0 ] );
}

extern "C" void CMod_PhysicsUpdate()
{
	float timestep = 1.0f / sv_fps->value;
	NewtonUpdate( g_world, timestep );
}

static void PhysicsEntityDie( const NewtonBody *body )
{
	// Don't do anything.
}

static void PhysicsEntityThink( const NewtonBody *body, dFloat timestep, int threadIndex )
{
	dFloat  Ixx;
	dFloat  Iyy;
	dFloat  Izz;
	dFloat  mass;
	dMatrix matr;

	NewtonBodyGetMassMatrix( body, &mass, &Ixx, &Iyy, &Izz );
	dVector force( 0.0f, 0.0f, ( mass * /*-313.92f*/ -9.81f ) );
	NewtonBodySetForce( body, &force.m_x );
}

static char *vtos( const vec3_t v )
{
	static int  index;
	static char str[ 8 ][ 32 ];
	char        *s;

	// use an array so that multiple vtos won't collide
	s = str[ index ];
	index = ( index + 1 ) & 7;

	Com_sprintf( s, 32, "(%i %i %i)", ( int ) v[ 0 ], ( int ) v[ 1 ], ( int ) v[ 2 ] );

	return s;
}

void TransposeMatrix( vec3_t matrix[ 3 ], vec3_t transpose[ 3 ] )
{
	int i, j;

	for ( i = 0; i < 3; i++ )
	{
		for ( j = 0; j < 3; j++ )
		{
			transpose[ i ][ j ] = matrix[ j ][ i ];
		}
	}
}

static void PhysicsEntitySetTransform( const NewtonBody *body, const dFloat *matrix, int threadIndex )
{
	if ( com_sv_running->integer )
	{
		sharedEntity_t *ent = ( sharedEntity_t * ) NewtonBodyGetUserData( body );
		vec3_t         newPosition;
		vec3_t         angles;
		vec3_t         axis[ 3 ];

		SV_UnlinkEntity( ent );

		newPosition[ 0 ] = matrix[ 12 ];
		newPosition[ 1 ] = matrix[ 13 ];
		newPosition[ 2 ] = matrix[ 14 ];

		// update the origin.
		VectorScale( newPosition, METRES_PER_UNIT, newPosition );
		VectorCopy( newPosition, ent->s.pos.trBase );
		VectorCopy( newPosition, ent->r.currentOrigin );
		VectorCopy( newPosition, ent->s.origin );

		axis[ 0 ][ 0 ] = matrix[ 0 ];
		axis[ 0 ][ 1 ] = matrix[ 1 ];
		axis[ 0 ][ 2 ] = matrix[ 2 ];
		axis[ 1 ][ 0 ] = matrix[ 4 ];
		axis[ 1 ][ 1 ] = matrix[ 5 ];
		axis[ 1 ][ 2 ] = matrix[ 6 ];
		axis[ 2 ][ 0 ] = matrix[ 8 ];
		axis[ 2 ][ 1 ] = matrix[ 9 ];
		axis[ 2 ][ 2 ] = matrix[ 10 ];
		AxisToAngles( axis, angles );

		VectorCopy( angles, ent->s.apos.trBase );
		VectorCopy( angles, ent->r.currentAngles );
		VectorCopy( angles, ent->s.angles );

		SV_LinkEntity( ent );

		Com_Printf( "PHYSICS: updating %d new position %s\n", ent->s.number, vtos( newPosition ) );
	}
}

extern "C" void CMod_PhysicsAddStatic( const sharedEntity_t *gEnt )
{
}

extern "C" void CMod_PhysicsAddEntity( sharedEntity_t *gEnt )
{
	std::map< int, bspCmodel >::iterator it = bspModels.find( gEnt->s.modelindex );

	if ( it == bspModels.end() )
	{
		return;
	}

	vec3_t         inertia, com;
	dMatrix        matrix( GetIdentityMatrix() );
	bspCmodel       *bmodel = &it->second;

	NewtonCollision *collision = NewtonCreateConvexHull( g_world, bmodel->vertices.size(), &bmodel->vertices[ 0 ].m_x, sizeof( dVector ), 0.0f, NULL );
	NewtonBody      *body = NewtonCreateBody( g_world, collision );

	bmodel->rigidBody = body;

	NewtonBodySetMaterialGroupID( body, defaultMaterialGroup );
	NewtonBodySetUserData( body, ( void * ) gEnt );
	NewtonBodySetDestructorCallback( body, PhysicsEntityDie );
	NewtonBodySetContinuousCollisionMode( body, 0 );
	NewtonBodySetForceAndTorqueCallback( body, PhysicsEntityThink );
	NewtonBodySetTransformCallback( body, PhysicsEntitySetTransform );

	NewtonConvexCollisionCalculateInertialMatrix( collision, &inertia[ 0 ], &com[ 0 ] );
	NewtonReleaseCollision( g_world, collision );
	NewtonBodySetCentreOfMass( body, &com[ 0 ] );

	VectorScale( inertia, 10.0f, inertia );  // The inertia needs to be scaled by the mass.

	NewtonBodySetMassMatrix( body, 10.f, inertia[ 0 ], inertia[ 1 ], inertia[ 2 ] );

	matrix.m_posit.m_x = gEnt->s.origin[ 0 ] * UNITS_PER_METRE;
	matrix.m_posit.m_y = gEnt->s.origin[ 1 ] * UNITS_PER_METRE;
	matrix.m_posit.m_z = gEnt->s.origin[ 2 ] * UNITS_PER_METRE;
	NewtonBodySetMatrix( body, &matrix[ 0 ][ 0 ] );

	gEnt->s.pos.trType = TR_INTERPOLATE;
	VectorCopy( gEnt->s.origin, gEnt->s.pos.trBase );
	VectorCopy( gEnt->s.origin, gEnt->r.currentOrigin );

	gEnt->s.apos.trType = TR_INTERPOLATE;
	VectorCopy( gEnt->s.angles, gEnt->s.apos.trBase );
	VectorCopy( gEnt->s.angles, gEnt->r.currentAngles );
}

extern "C" void CMod_PhysicsAddBSPModel( int index, int firstSurface, int numSurfaces )
{
	bspCmodel newModel;
	newModel.index = index;
	newModel.firstSurface = firstSurface;
	newModel.numSurfaces = numSurfaces;

	bspModels[ index ] = newModel;
}

extern "C" int CMod_PhysicsBSPSurfaceIsModel( int surfaceID )
{
	for ( std::map< int, bspCmodel >::iterator it = bspModels.begin();
	      it != bspModels.end();
	      ++it )
	{
		if ( surfaceID >= it->second.firstSurface && surfaceID < ( it->second.firstSurface + it->second.numSurfaces ) )
		{
			return it->second.index;
		}
	}

	return 0;
}

extern "C" void CMod_PhysicsAddFaceToModel( int modelIndex, int surface, vec3_t vec[ 3 ] )
{
	std::map< int, bspCmodel >::iterator it = bspModels.find( modelIndex );

	if ( it == bspModels.end() )
	{
		return;
	}

	VectorScale( vec[ 0 ], UNITS_PER_METRE, vec[ 0 ] );
	VectorScale( vec[ 1 ], UNITS_PER_METRE, vec[ 1 ] );
	VectorScale( vec[ 2 ], UNITS_PER_METRE, vec[ 2 ] );

	dVector newVert[ 3 ];
	newVert[ 0 ].m_x = vec[ 0 ][ 0 ];
	newVert[ 0 ].m_y = vec[ 0 ][ 1 ];
	newVert[ 0 ].m_z = vec[ 0 ][ 2 ];
	newVert[ 1 ].m_x = vec[ 1 ][ 0 ];
	newVert[ 1 ].m_y = vec[ 1 ][ 1 ];
	newVert[ 1 ].m_z = vec[ 1 ][ 2 ];
	newVert[ 2 ].m_x = vec[ 2 ][ 0 ];
	newVert[ 2 ].m_y = vec[ 2 ][ 1 ];
	newVert[ 2 ].m_z = vec[ 2 ][ 2 ];

	it->second.vertices.push_back( newVert[ 0 ] );
	it->second.vertices.push_back( newVert[ 1 ] );
	it->second.vertices.push_back( newVert[ 2 ] );
}

#endif
