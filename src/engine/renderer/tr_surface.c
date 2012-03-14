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

// tr_surf.c
#include "tr_local.h"

/*

  THIS ENTIRE FILE IS BACK END

backEnd.currentEntity will be valid.

Tess_Begin has already been called for the surface's shader.

The modelview matrix will be set.

It is safe to actually issue drawing commands here if you don't want to
use the shader system.
*/

//============================================================================

/*
==============
RB_CheckOverflow
==============
*/
void RB_CheckOverflow( int verts, int indexes )
{
	if ( tess.numVertexes + verts < tess.maxShaderVerts && tess.numIndexes + indexes < tess.maxShaderIndicies )
	{
		return;
	}

	RB_EndSurface();

	if ( verts >= tess.maxShaderVerts )
	{
		ri.Error( ERR_DROP, "RB_CheckOverflow: verts > MAX (%d > %d)", verts, tess.maxShaderVerts );
	}

	if ( indexes >= tess.maxShaderIndicies )
	{
		ri.Error( ERR_DROP, "RB_CheckOverflow: indices > MAX (%d > %d)", indexes, tess.maxShaderIndicies );
	}

	RB_BeginSurface( tess.shader, tess.fogNum );
}

/*
==============
RB_AddQuadStampFadingCornersExt

  Creates a sprite with the center at colors[3] alpha, and the corners all 0 alpha
==============
*/
void RB_AddQuadStampFadingCornersExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 )
{
	vec3_t normal;
	int    ndx;
	byte   lColor[ 4 ];

	RB_CHECKOVERFLOW( 5, 12 );

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes + 0 ] = ndx + 0;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 4;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 2;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 4;

	tess.indexes[ tess.numIndexes + 6 ] = ndx + 2;
	tess.indexes[ tess.numIndexes + 7 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 8 ] = ndx + 4;

	tess.indexes[ tess.numIndexes + 9 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 10 ] = ndx + 0;
	tess.indexes[ tess.numIndexes + 11 ] = ndx + 4;

	tess.xyz[ ndx ].v[ 0 ] = origin[ 0 ] + left[ 0 ] + up[ 0 ];
	tess.xyz[ ndx ].v[ 1 ] = origin[ 1 ] + left[ 1 ] + up[ 1 ];
	tess.xyz[ ndx ].v[ 2 ] = origin[ 2 ] + left[ 2 ] + up[ 2 ];

	tess.xyz[ ndx + 1 ].v[ 0 ] = origin[ 0 ] - left[ 0 ] + up[ 0 ];
	tess.xyz[ ndx + 1 ].v[ 1 ] = origin[ 1 ] - left[ 1 ] + up[ 1 ];
	tess.xyz[ ndx + 1 ].v[ 2 ] = origin[ 2 ] - left[ 2 ] + up[ 2 ];

	tess.xyz[ ndx + 2 ].v[ 0 ] = origin[ 0 ] - left[ 0 ] - up[ 0 ];
	tess.xyz[ ndx + 2 ].v[ 1 ] = origin[ 1 ] - left[ 1 ] - up[ 1 ];
	tess.xyz[ ndx + 2 ].v[ 2 ] = origin[ 2 ] - left[ 2 ] - up[ 2 ];

	tess.xyz[ ndx + 3 ].v[ 0 ] = origin[ 0 ] + left[ 0 ] - up[ 0 ];
	tess.xyz[ ndx + 3 ].v[ 1 ] = origin[ 1 ] + left[ 1 ] - up[ 1 ];
	tess.xyz[ ndx + 3 ].v[ 2 ] = origin[ 2 ] + left[ 2 ] - up[ 2 ];

	tess.xyz[ ndx + 4 ].v[ 0 ] = origin[ 0 ];
	tess.xyz[ ndx + 4 ].v[ 1 ] = origin[ 1 ];
	tess.xyz[ ndx + 4 ].v[ 2 ] = origin[ 2 ];

	// constant normal all the way around
	VectorSubtract( vec3_origin, backEnd.viewParms.orientation.axis[ 0 ], normal );

	tess.normal[ ndx ].v[ 0 ] = tess.normal[ ndx + 1 ].v[ 0 ] = tess.normal[ ndx + 2 ].v[ 0 ] = tess.normal[ ndx + 3 ].v[ 0 ] =
	                              tess.normal[ ndx + 4 ].v[ 0 ] = normal[ 0 ];
	tess.normal[ ndx ].v[ 1 ] = tess.normal[ ndx + 1 ].v[ 1 ] = tess.normal[ ndx + 2 ].v[ 1 ] = tess.normal[ ndx + 3 ].v[ 1 ] =
	                              tess.normal[ ndx + 4 ].v[ 1 ] = normal[ 1 ];
	tess.normal[ ndx ].v[ 2 ] = tess.normal[ ndx + 1 ].v[ 2 ] = tess.normal[ ndx + 2 ].v[ 2 ] = tess.normal[ ndx + 3 ].v[ 2 ] =
	                              tess.normal[ ndx + 4 ].v[ 2 ] = normal[ 2 ];

	// standard square texture coordinates
	tess.texCoords0[ ndx ].v[ 0 ] = tess.texCoords1[ ndx ].v[ 0 ] = s1;
	tess.texCoords0[ ndx ].v[ 1 ] = tess.texCoords1[ ndx ].v[ 1 ] = t1;

	tess.texCoords0[ ndx + 1 ].v[ 0 ] = tess.texCoords1[ ndx + 1 ].v[ 0 ] = s2;
	tess.texCoords0[ ndx + 1 ].v[ 1 ] = tess.texCoords1[ ndx + 1 ].v[ 1 ] = t1;

	tess.texCoords0[ ndx + 2 ].v[ 0 ] = tess.texCoords1[ ndx + 2 ].v[ 0 ] = s2;
	tess.texCoords0[ ndx + 2 ].v[ 1 ] = tess.texCoords1[ ndx + 2 ].v[ 1 ] = t2;

	tess.texCoords0[ ndx + 3 ].v[ 0 ] = tess.texCoords1[ ndx + 3 ].v[ 0 ] = s1;
	tess.texCoords0[ ndx + 3 ].v[ 1 ] = tess.texCoords1[ ndx + 3 ].v[ 1 ] = t2;

	tess.texCoords0[ ndx + 4 ].v[ 0 ] = tess.texCoords1[ ndx + 4 ].v[ 0 ] = ( s1 + s2 ) / 2.0;
	tess.texCoords0[ ndx + 4 ].v[ 1 ] = tess.texCoords1[ ndx + 4 ].v[ 1 ] = ( t1 + t2 ) / 2.0;

	// center uses full alpha
	* ( unsigned int * ) &tess.vertexColors[ ndx + 4 ].v = * ( unsigned int * ) color;

	// fade around edges
	memcpy( lColor, color, sizeof( byte ) * 4 );
	lColor[ 3 ] = 0;
	* ( unsigned int * ) &tess.vertexColors[ ndx ].v =
	  * ( unsigned int * ) &tess.vertexColors[ ndx + 1 ].v =
	    * ( unsigned int * ) &tess.vertexColors[ ndx + 2 ].v = * ( unsigned int * ) &tess.vertexColors[ ndx + 3 ].v = * ( unsigned int * ) lColor;

	tess.numVertexes += 5;
	tess.numIndexes += 12;
}

/*
==============
RB_AddQuadStampExt
==============
*/
void RB_AddQuadStampExt( vec3_t origin, vec3_t left, vec3_t up, byte *color, float s1, float t1, float s2, float t2 )
{
	vec3_t normal;
	int    ndx;

	RB_CHECKOVERFLOW( 4, 6 );

	ndx = tess.numVertexes;

	// triangle indexes for a simple quad
	tess.indexes[ tess.numIndexes ] = ndx;
	tess.indexes[ tess.numIndexes + 1 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 2 ] = ndx + 3;

	tess.indexes[ tess.numIndexes + 3 ] = ndx + 3;
	tess.indexes[ tess.numIndexes + 4 ] = ndx + 1;
	tess.indexes[ tess.numIndexes + 5 ] = ndx + 2;

	tess.xyz[ ndx ].v[ 0 ] = origin[ 0 ] + left[ 0 ] + up[ 0 ];
	tess.xyz[ ndx ].v[ 1 ] = origin[ 1 ] + left[ 1 ] + up[ 1 ];
	tess.xyz[ ndx ].v[ 2 ] = origin[ 2 ] + left[ 2 ] + up[ 2 ];

	tess.xyz[ ndx + 1 ].v[ 0 ] = origin[ 0 ] - left[ 0 ] + up[ 0 ];
	tess.xyz[ ndx + 1 ].v[ 1 ] = origin[ 1 ] - left[ 1 ] + up[ 1 ];
	tess.xyz[ ndx + 1 ].v[ 2 ] = origin[ 2 ] - left[ 2 ] + up[ 2 ];

	tess.xyz[ ndx + 2 ].v[ 0 ] = origin[ 0 ] - left[ 0 ] - up[ 0 ];
	tess.xyz[ ndx + 2 ].v[ 1 ] = origin[ 1 ] - left[ 1 ] - up[ 1 ];
	tess.xyz[ ndx + 2 ].v[ 2 ] = origin[ 2 ] - left[ 2 ] - up[ 2 ];

	tess.xyz[ ndx + 3 ].v[ 0 ] = origin[ 0 ] + left[ 0 ] - up[ 0 ];
	tess.xyz[ ndx + 3 ].v[ 1 ] = origin[ 1 ] + left[ 1 ] - up[ 1 ];
	tess.xyz[ ndx + 3 ].v[ 2 ] = origin[ 2 ] + left[ 2 ] - up[ 2 ];

	// constant normal all the way around
	VectorSubtract( vec3_origin, backEnd.viewParms.orientation.axis[ 0 ], normal );

	tess.normal[ ndx ].v[ 0 ] = tess.normal[ ndx + 1 ].v[ 0 ] = tess.normal[ ndx + 2 ].v[ 0 ] = tess.normal[ ndx + 3 ].v[ 0 ] = normal[ 0 ];
	tess.normal[ ndx ].v[ 1 ] = tess.normal[ ndx + 1 ].v[ 1 ] = tess.normal[ ndx + 2 ].v[ 1 ] = tess.normal[ ndx + 3 ].v[ 1 ] = normal[ 1 ];
	tess.normal[ ndx ].v[ 2 ] = tess.normal[ ndx + 1 ].v[ 2 ] = tess.normal[ ndx + 2 ].v[ 2 ] = tess.normal[ ndx + 3 ].v[ 2 ] = normal[ 2 ];

	// standard square texture coordinates
	tess.texCoords0[ ndx ].v[ 0 ] = tess.texCoords1[ ndx ].v[ 0 ] = s1;
	tess.texCoords0[ ndx ].v[ 1 ] = tess.texCoords1[ ndx ].v[ 1 ] = t1;

	tess.texCoords0[ ndx + 1 ].v[ 0 ] = tess.texCoords1[ ndx + 1 ].v[ 0 ] = s2;
	tess.texCoords0[ ndx + 1 ].v[ 1 ] = tess.texCoords1[ ndx + 1 ].v[ 1 ] = t1;

	tess.texCoords0[ ndx + 2 ].v[ 0 ] = tess.texCoords1[ ndx + 2 ].v[ 0 ] = s2;
	tess.texCoords0[ ndx + 2 ].v[ 1 ] = tess.texCoords1[ ndx + 2 ].v[ 1 ] = t2;

	tess.texCoords0[ ndx + 3 ].v[ 0 ] = tess.texCoords1[ ndx + 3 ].v[ 0 ] = s1;
	tess.texCoords0[ ndx + 3 ].v[ 1 ] = tess.texCoords1[ ndx + 3 ].v[ 1 ] = t2;

	// constant color all the way around
	// should this be identity and let the shader specify from entity?
	* ( unsigned int * ) &tess.vertexColors[ ndx ].v =
	  * ( unsigned int * ) &tess.vertexColors[ ndx + 1 ].v =
	    * ( unsigned int * ) &tess.vertexColors[ ndx + 2 ].v = * ( unsigned int * ) &tess.vertexColors[ ndx + 3 ].v = * ( unsigned int * ) color;

	tess.numVertexes += 4;
	tess.numIndexes += 6;
}

/*
==============
RB_AddQuadStamp
==============
*/
void RB_AddQuadStamp( vec3_t origin, vec3_t left, vec3_t up, byte *color )
{
	RB_AddQuadStampExt( origin, left, up, color, 0, 0, 1, 1 );
}

/*
==============
RB_SurfaceSplash
==============
*/
static void RB_SurfaceSplash( void )
{
	vec3_t left, up;
	float  radius;

	// calculate the xyz locations for the four corners
	radius = backEnd.currentEntity->e.radius;

	VectorSet( left, -radius, 0, 0 );
	VectorSet( up, 0, radius, 0 );

	if ( backEnd.viewParms.isMirror )
	{
		VectorSubtract( vec3_origin, left, left );
	}

	RB_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA );
}

/*
==============
RB_SurfaceSprite
==============
*/
static void RB_SurfaceSprite( void )
{
	vec3_t left, up;
	float  radius;

	// calculate the xyz locations for the four corners
	radius = backEnd.currentEntity->e.radius;

	if ( backEnd.currentEntity->e.rotation == 0 )
	{
		VectorScale( backEnd.viewParms.orientation.axis[ 1 ], radius, left );
		VectorScale( backEnd.viewParms.orientation.axis[ 2 ], radius, up );
	}
	else
	{
		float s, c;
		float ang;

		ang = M_PI * backEnd.currentEntity->e.rotation / 180;
		s = sin( ang );
		c = cos( ang );

		VectorScale( backEnd.viewParms.orientation.axis[ 1 ], c * radius, left );
		VectorMA( left, -s * radius, backEnd.viewParms.orientation.axis[ 2 ], left );

		VectorScale( backEnd.viewParms.orientation.axis[ 2 ], c * radius, up );
		VectorMA( up, s * radius, backEnd.viewParms.orientation.axis[ 1 ], up );
	}

	if ( backEnd.viewParms.isMirror )
	{
		VectorSubtract( vec3_origin, left, left );
	}

	RB_AddQuadStamp( backEnd.currentEntity->e.origin, left, up, backEnd.currentEntity->e.shaderRGBA );
}

/*
=============
RB_SurfacePolychain
=============
*/
void RB_SurfacePolychain( srfPoly_t *p )
{
	int i;
	int numv;

	RB_CHECKOVERFLOW( p->numVerts, 3 * ( p->numVerts - 2 ) );

	// fan triangles into the tess array
	numv = tess.numVertexes;

	for ( i = 0; i < p->numVerts; i++ )
	{
		VectorCopy( p->verts[ i ].xyz, tess.xyz[ numv ].v );
		tess.texCoords0[ numv ].v[ 0 ] = p->verts[ i ].st[ 0 ];
		tess.texCoords0[ numv ].v[ 1 ] = p->verts[ i ].st[ 1 ];
		* ( int * ) &tess.vertexColors[ numv ].v = * ( int * ) p->verts[ i ].modulate;

		numv++;
	}

	// generate fan indexes into the tess array
	for ( i = 0; i < p->numVerts - 2; i++ )
	{
		tess.indexes[ tess.numIndexes + 0 ] = tess.numVertexes;
		tess.indexes[ tess.numIndexes + 1 ] = tess.numVertexes + i + 1;
		tess.indexes[ tess.numIndexes + 2 ] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.numVertexes = numv;
}

/*
=============
RB_SurfaceTriangles
=============
*/
#if 0
void RB_SurfaceTriangles( srfTriangles2_t *srf )
{
	vec4hack_t     *oldXYZ;
	vec4hack_t     *oldNormal;
	vec2hack_t     *oldST;
	vec2hack_t     *oldLightmap;
	glIndex_t      *oldIndicies;
	color4ubhack_t *oldColor;

	int            oldMaxVerts;
	int            oldMaxIndicies;

	RB_EndSurface();

	RB_BeginSurface( tess.shader, tess.fogNum );

	oldXYZ = tess.xyz;
	oldST = tess.texCoords0;
	oldLightmap = tess.texCoords1;
	oldIndicies = tess.indexes;
	oldNormal = tess.normal;
	oldColor = tess.vertexColors;

	oldMaxVerts = tess.maxShaderVerts;
	oldMaxIndicies = tess.maxShaderIndicies;

	// ===================================================
	tess.numIndexes = srf->numIndexes;
	tess.numVertexes = srf->numVerts;

	tess.xyz = srf->xyz;
	tess.texCoords0 = srf->st;
	tess.texCoords1 = srf->lightmap;
	tess.indexes = srf->indexes;
	tess.normal = srf->normal;
	tess.vertexColors = srf->color;

	tess.maxShaderIndicies = srf->numIndexes + 1;
	tess.maxShaderVerts = srf->numVerts + 1;
	// ===================================================

	RB_EndSurface();

	tess.xyz = oldXYZ;
	tess.texCoords0 = oldST;
	tess.texCoords1 = oldLightmap;
	tess.indexes = oldIndicies;
	tess.normal = oldNormal;
	tess.vertexColors = oldColor;

	tess.maxShaderVerts = oldMaxVerts;
	tess.maxShaderIndicies = oldMaxIndicies;
}

#else
void RB_SurfaceTriangles( srfTriangles_t *srf )
{
	int        i;
	drawVert_t *dv;
	float      *xyz, *normal, *texCoords0, *texCoords1;
	byte       *color;
	int        dlightBits;
	qboolean   needsNormal;

	// ydnar: moved before overflow so dlights work properly
	RB_CHECKOVERFLOW( srf->numVerts, srf->numIndexes );

	dlightBits = srf->dlightBits[ backEnd.smpFrame ];
	tess.dlightBits |= dlightBits;

	for ( i = 0; i < srf->numIndexes; i += 3 )
	{
		tess.indexes[ tess.numIndexes + i + 0 ] = tess.numVertexes + srf->indexes[ i + 0 ];
		tess.indexes[ tess.numIndexes + i + 1 ] = tess.numVertexes + srf->indexes[ i + 1 ];
		tess.indexes[ tess.numIndexes + i + 2 ] = tess.numVertexes + srf->indexes[ i + 2 ];
	}

	tess.numIndexes += srf->numIndexes;

	dv = srf->verts;
	xyz = tess.xyz[ tess.numVertexes ].v;
	normal = tess.normal[ tess.numVertexes ].v;
	texCoords0 = tess.texCoords0[ tess.numVertexes ].v;
	texCoords1 = tess.texCoords1[ tess.numVertexes ].v;
	color = tess.vertexColors[ tess.numVertexes ].v;
	needsNormal = tess.shader->needsNormal;

	if ( needsNormal )
	{
		for ( i = 0; i < srf->numVerts; i++, dv++, xyz += 4, normal += 4, texCoords0 += 2, texCoords1 += 2, color += 4 )
		{
			xyz[ 0 ] = dv->xyz[ 0 ];
			xyz[ 1 ] = dv->xyz[ 1 ];
			xyz[ 2 ] = dv->xyz[ 2 ];

			normal[ 0 ] = dv->normal[ 0 ];
			normal[ 1 ] = dv->normal[ 1 ];
			normal[ 2 ] = dv->normal[ 2 ];

			texCoords0[ 0 ] = dv->st[ 0 ];
			texCoords0[ 1 ] = dv->st[ 1 ];

			texCoords1[ 0 ] = dv->lightmap[ 0 ];
			texCoords1[ 1 ] = dv->lightmap[ 1 ];

			* ( int * ) color = * ( int * ) dv->color;
		}
	}
	else
	{
		for ( i = 0; i < srf->numVerts; i++, dv++, xyz += 4, normal += 4, texCoords0 += 2, texCoords1 += 2, color += 4 )
		{
			xyz[ 0 ] = dv->xyz[ 0 ];
			xyz[ 1 ] = dv->xyz[ 1 ];
			xyz[ 2 ] = dv->xyz[ 2 ];

			texCoords0[ 0 ] = dv->st[ 0 ];
			texCoords0[ 1 ] = dv->st[ 1 ];

			texCoords1[ 0 ] = dv->lightmap[ 0 ];
			texCoords1[ 1 ] = dv->lightmap[ 1 ];

			* ( int * ) color = * ( int * ) dv->color;
		}
	}

	tess.numVertexes += srf->numVerts;
}

#endif // 1

/*
=============
RB_SurfaceFoliage - ydnar
=============
*/

void RB_SurfaceFoliage( srfFoliage_t *srf )
{
	int               o, i, a, numVerts, numIndexes;
	vec4_t            distanceCull, distanceVector;
	float             alpha, z, dist, fovScale;
	vec3_t            viewOrigin, local;
	vec_t             *xyz;
	int               srcColor, *color;
	int               dlightBits;
	foliageInstance_t *instance;

	// basic setup
	numVerts = srf->numVerts;
	numIndexes = srf->numIndexes;
	VectorCopy( backEnd.orientation.viewOrigin, viewOrigin );

	// set fov scale
	fovScale = backEnd.viewParms.fovX * ( 1.0 / 90.0 );

	// calculate distance vector
	VectorSubtract( backEnd.orientation.origin, backEnd.viewParms.orientation.origin, local );
	distanceVector[ 0 ] = -backEnd.orientation.modelMatrix[ 2 ];
	distanceVector[ 1 ] = -backEnd.orientation.modelMatrix[ 6 ];
	distanceVector[ 2 ] = -backEnd.orientation.modelMatrix[ 10 ];
	distanceVector[ 3 ] = DotProduct( local, backEnd.viewParms.orientation.axis[ 0 ] );

	// attempt distance cull
	VectorCopy( tess.shader->distanceCull, distanceCull );
	distanceCull[ 3 ] = tess.shader->distanceCull[ 3 ];

	if ( distanceCull[ 1 ] > 0 )
	{
		//VectorSubtract( srf->localOrigin, viewOrigin, delta );
		//alpha = (distanceCull[ 1 ] - VectorLength( delta ) + srf->radius) * distanceCull[ 3 ];
		z = fovScale * ( DotProduct( srf->origin, distanceVector ) + distanceVector[ 3 ] - srf->radius );
		alpha = ( distanceCull[ 1 ] - z ) * distanceCull[ 3 ];

		if ( alpha < distanceCull[ 2 ] )
		{
			return;
		}
	}

	// set dlight bits
	dlightBits = srf->dlightBits[ backEnd.smpFrame ];
	tess.dlightBits |= dlightBits;

	// iterate through origin list
	instance = srf->instances;

	for ( o = 0; o < srf->numInstances; o++, instance++ )
	{
		// fade alpha based on distance between inner and outer radii
		if ( distanceCull[ 1 ] > 0.0f )
		{
			// calculate z distance
			z = fovScale * ( DotProduct( instance->origin, distanceVector ) + distanceVector[ 3 ] );

			if ( z < -64.0f )
			{
				// epsilon so close-by foliage doesn't pop in and out
				continue;
			}

			// check against frustum planes
			for ( i = 0; i < 5; i++ )
			{
				dist = DotProduct( instance->origin, backEnd.viewParms.frustum[ i ].normal ) - backEnd.viewParms.frustum[ i ].dist;

				if ( dist < -64.0 )
				{
					break;
				}
			}

			if ( i != 5 )
			{
				continue;
			}

			// radix
			if ( o & 1 )
			{
				z *= 1.25;

				if ( o & 2 )
				{
					z *= 1.25;
				}
			}

			// calculate alpha
			alpha = ( distanceCull[ 1 ] - z ) * distanceCull[ 3 ];

			if ( alpha < distanceCull[ 2 ] )
			{
				continue;
			}

			// set color
			a = alpha > 1.0f ? 255 : alpha * 255;
#if __MACOS__ // LBO 3/15/05. Byte-swap fix for Mac - alpha is in the LSB.
			srcColor = ( * ( ( int * ) instance->color ) & 0xFFFFFF00 ) | ( a & 0xff );
#else
			srcColor = ( * ( ( int * ) instance->color ) & 0xFFFFFF ) | ( a << 24 );
#endif
		}
		else
		{
			srcColor = * ( ( int * ) instance->color );
		}

		// Com_Printf( "Color: %d %d %d %d\n", srf->colors[ o ][ 0 ], srf->colors[ o ][ 1 ], srf->colors[ o ][ 2 ], alpha );

		RB_CHECKOVERFLOW( numVerts, numIndexes );

		// ydnar: set after overflow check so dlights work properly
		tess.dlightBits |= dlightBits;

		// copy indexes
		memcpy( &tess.indexes[ tess.numIndexes ], srf->indexes, numIndexes * sizeof( srf->indexes[ 0 ] ) );

		for ( i = 0; i < numIndexes; i++ )
		{
			tess.indexes[ tess.numIndexes + i ] += tess.numVertexes;
		}

		// copy xyz, normal and st
		xyz = tess.xyz[ tess.numVertexes ].v;
		memcpy( xyz, srf->xyz, numVerts * sizeof( srf->xyz[ 0 ] ) );

		if ( tess.shader->needsNormal )
		{
			memcpy( &tess.normal[ tess.numVertexes ].v, srf->normal, numVerts * sizeof( srf->xyz[ 0 ] ) );
		}

		memcpy( &tess.texCoords0[ tess.numVertexes ], srf->texCoords, numVerts * sizeof( srf->texCoords[ 0 ] ) );
		memcpy( &tess.texCoords1[ tess.numVertexes ], srf->lmTexCoords, numVerts * sizeof( srf->lmTexCoords[ 0 ] ) );

		// offset xyz
		for ( i = 0; i < numVerts; i++, xyz += 4 )
		{
			VectorAdd( xyz, instance->origin, xyz );
		}

		// copy color
		color = ( int * ) tess.vertexColors[ tess.numVertexes ].v;

		for ( i = 0; i < numVerts; i++ )
		{
			color[ i ] = srcColor;
		}

		// increment
		tess.numIndexes += numIndexes;
		tess.numVertexes += numVerts;
	}

	// RB_DrawBounds( srf->bounds[ 0 ], srf->bounds[ 1 ] );
}

/*
==============
RB_SurfaceBeam
==============
*/
void RB_SurfaceBeam( void )
{
#define NUM_BEAM_SEGS 6
	refEntity_t *e;
	int         i;
	vec3_t      perpvec;
	vec3_t      direction, normalized_direction;
	vec3_t      start_points[ NUM_BEAM_SEGS ], end_points[ NUM_BEAM_SEGS ];
	vec3_t      oldorigin, origin;

	e = &backEnd.currentEntity->e;

	oldorigin[ 0 ] = e->oldorigin[ 0 ];
	oldorigin[ 1 ] = e->oldorigin[ 1 ];
	oldorigin[ 2 ] = e->oldorigin[ 2 ];

	origin[ 0 ] = e->origin[ 0 ];
	origin[ 1 ] = e->origin[ 1 ];
	origin[ 2 ] = e->origin[ 2 ];

	normalized_direction[ 0 ] = direction[ 0 ] = oldorigin[ 0 ] - origin[ 0 ];
	normalized_direction[ 1 ] = direction[ 1 ] = oldorigin[ 1 ] - origin[ 1 ];
	normalized_direction[ 2 ] = direction[ 2 ] = oldorigin[ 2 ] - origin[ 2 ];

	if ( VectorNormalize( normalized_direction ) == 0 )
	{
		return;
	}

	PerpendicularVector( perpvec, normalized_direction );

	VectorScale( perpvec, 4, perpvec );

	for ( i = 0; i < NUM_BEAM_SEGS; i++ )
	{
		RotatePointAroundVector( start_points[ i ], normalized_direction, perpvec, ( 360.0 / NUM_BEAM_SEGS ) * i );
//      VectorAdd( start_points[i], origin, start_points[i] );
		VectorAdd( start_points[ i ], direction, end_points[ i ] );
	}

	GL_Bind( tr.whiteImage );

	GL_State( GLS_SRCBLEND_ONE | GLS_DSTBLEND_ONE );

	glColor3f( 1, 0, 0 );

	glBegin( GL_TRIANGLE_STRIP );

	for ( i = 0; i <= NUM_BEAM_SEGS; i++ )
	{
		glVertex3fv( start_points[ i % NUM_BEAM_SEGS ] );
		glVertex3fv( end_points[ i % NUM_BEAM_SEGS ] );
	}

	glEnd();
}

//================================================================================

static void DoRailCore( const vec3_t start, const vec3_t end, const vec3_t up, float len, float spanWidth )
{
	float spanWidth2;
	int   vbase;
	float t; // = len / 256.0f;

	vbase = tess.numVertexes;

	// Gordon: configurable tile
	if ( backEnd.currentEntity->e.radius > 0 )
	{
		t = len / backEnd.currentEntity->e.radius;
	}
	else
	{
		t = len / 256.f;
	}

	spanWidth2 = -spanWidth;

	// FIXME: use quad stamp?
	VectorMA( start, spanWidth, up, tess.xyz[ tess.numVertexes ].v );
	tess.texCoords0[ tess.numVertexes ].v[ 0 ] = 0;
	tess.texCoords0[ tess.numVertexes ].v[ 1 ] = 0;
	tess.vertexColors[ tess.numVertexes ].v[ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ];
	tess.vertexColors[ tess.numVertexes ].v[ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ];
	tess.vertexColors[ tess.numVertexes ].v[ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ];
	tess.vertexColors[ tess.numVertexes ].v[ 3 ] = backEnd.currentEntity->e.shaderRGBA[ 3 ];
	tess.numVertexes++;

	VectorMA( start, spanWidth2, up, tess.xyz[ tess.numVertexes ].v );
	tess.texCoords0[ tess.numVertexes ].v[ 0 ] = 0;
	tess.texCoords0[ tess.numVertexes ].v[ 1 ] = 1;
	tess.vertexColors[ tess.numVertexes ].v[ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ];
	tess.vertexColors[ tess.numVertexes ].v[ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ];
	tess.vertexColors[ tess.numVertexes ].v[ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ];
	tess.vertexColors[ tess.numVertexes ].v[ 3 ] = backEnd.currentEntity->e.shaderRGBA[ 3 ];
	tess.numVertexes++;

	VectorMA( end, spanWidth, up, tess.xyz[ tess.numVertexes ].v );

	tess.texCoords0[ tess.numVertexes ].v[ 0 ] = t;
	tess.texCoords0[ tess.numVertexes ].v[ 1 ] = 0;
	tess.vertexColors[ tess.numVertexes ].v[ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ];
	tess.vertexColors[ tess.numVertexes ].v[ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ];
	tess.vertexColors[ tess.numVertexes ].v[ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ];
	tess.vertexColors[ tess.numVertexes ].v[ 3 ] = backEnd.currentEntity->e.shaderRGBA[ 3 ];
	tess.numVertexes++;

	VectorMA( end, spanWidth2, up, tess.xyz[ tess.numVertexes ].v );
	tess.texCoords0[ tess.numVertexes ].v[ 0 ] = t;
	tess.texCoords0[ tess.numVertexes ].v[ 1 ] = 1;
	tess.vertexColors[ tess.numVertexes ].v[ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ];
	tess.vertexColors[ tess.numVertexes ].v[ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ];
	tess.vertexColors[ tess.numVertexes ].v[ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ];
	tess.vertexColors[ tess.numVertexes ].v[ 3 ] = backEnd.currentEntity->e.shaderRGBA[ 3 ];
	tess.numVertexes++;

	tess.indexes[ tess.numIndexes++ ] = vbase;
	tess.indexes[ tess.numIndexes++ ] = vbase + 1;
	tess.indexes[ tess.numIndexes++ ] = vbase + 2;

	tess.indexes[ tess.numIndexes++ ] = vbase + 2;
	tess.indexes[ tess.numIndexes++ ] = vbase + 1;
	tess.indexes[ tess.numIndexes++ ] = vbase + 3;
}

static void DoRailDiscs( int numSegs, const vec3_t start, const vec3_t dir, const vec3_t right, const vec3_t up )
{
	int    i;
	vec3_t pos[ 4 ];
	vec3_t v;
	int    spanWidth = r_railWidth->integer;
	float  c, s;
	float  scale;

	if ( numSegs > 1 )
	{
		numSegs--;
	}

	if ( !numSegs )
	{
		return;
	}

	scale = 0.25;

	for ( i = 0; i < 4; i++ )
	{
		c = cos( DEG2RAD( 45 + i * 90 ) );
		s = sin( DEG2RAD( 45 + i * 90 ) );
		v[ 0 ] = ( right[ 0 ] * c + up[ 0 ] * s ) * scale * spanWidth;
		v[ 1 ] = ( right[ 1 ] * c + up[ 1 ] * s ) * scale * spanWidth;
		v[ 2 ] = ( right[ 2 ] * c + up[ 2 ] * s ) * scale * spanWidth;
		VectorAdd( start, v, pos[ i ] );

		if ( numSegs > 1 )
		{
			// offset by 1 segment if we're doing a long distance shot
			VectorAdd( pos[ i ], dir, pos[ i ] );
		}
	}

	for ( i = 0; i < numSegs; i++ )
	{
		int j;

		RB_CHECKOVERFLOW( 4, 6 );

		for ( j = 0; j < 4; j++ )
		{
			VectorCopy( pos[ j ], tess.xyz[ tess.numVertexes ].v );
			tess.texCoords0[ tess.numVertexes ].v[ 0 ] = ( j < 2 );
			tess.texCoords0[ tess.numVertexes ].v[ 1 ] = ( j && j != 3 );
			tess.vertexColors[ tess.numVertexes ].v[ 0 ] = backEnd.currentEntity->e.shaderRGBA[ 0 ];
			tess.vertexColors[ tess.numVertexes ].v[ 1 ] = backEnd.currentEntity->e.shaderRGBA[ 1 ];
			tess.vertexColors[ tess.numVertexes ].v[ 2 ] = backEnd.currentEntity->e.shaderRGBA[ 2 ];
			tess.numVertexes++;

			VectorAdd( pos[ j ], dir, pos[ j ] );
		}

		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 0;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 1;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 3;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 3;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 1;
		tess.indexes[ tess.numIndexes++ ] = tess.numVertexes - 4 + 2;
	}
}

/*
** RB_SurfaceRailRinges
*/
void RB_SurfaceRailRings( void )
{
	refEntity_t *e;
	int         numSegs;
	int         len;
	vec3_t      vec;
	vec3_t      right, up;
	vec3_t      start, end;

	e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, start );
	VectorCopy( e->origin, end );

	// compute variables
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );
	MakeNormalVectors( vec, right, up );
	numSegs = ( len ) / r_railSegmentLength->value;

	if ( numSegs <= 0 )
	{
		numSegs = 1;
	}

	VectorScale( vec, r_railSegmentLength->value, vec );

	DoRailDiscs( numSegs, start, vec, right, up );
}

/*
** RB_SurfaceRailCore
*/
void RB_SurfaceRailCore( void )
{
	refEntity_t *e;
	int         len;
	vec3_t      right;
	vec3_t      vec;
	vec3_t      start, end;
	vec3_t      v1, v2;

	e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, start );
	VectorCopy( e->origin, end );

	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	// compute side vector
	VectorSubtract( start, backEnd.viewParms.orientation.origin, v1 );
	VectorNormalize( v1 );
	VectorSubtract( end, backEnd.viewParms.orientation.origin, v2 );
	VectorNormalize( v2 );
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	DoRailCore( start, end, right, len, e->frame > 0 ? e->frame : 1 );
}

/*
** RB_SurfaceLightningBolt
*/
void RB_SurfaceLightningBolt( void )
{
	refEntity_t *e;
	int         len;
	vec3_t      right;
	vec3_t      vec;
	vec3_t      start, end;
	vec3_t      v1, v2;
	int         i;

	e = &backEnd.currentEntity->e;

	VectorCopy( e->oldorigin, end );
	VectorCopy( e->origin, start );

	// compute variables
	VectorSubtract( end, start, vec );
	len = VectorNormalize( vec );

	// compute side vector
	VectorSubtract( start, backEnd.viewParms.orientation.origin, v1 );
	VectorNormalize( v1 );
	VectorSubtract( end, backEnd.viewParms.orientation.origin, v2 );
	VectorNormalize( v2 );
	CrossProduct( v1, v2, right );
	VectorNormalize( right );

	for ( i = 0; i < 4; i++ )
	{
		vec3_t temp;

		DoRailCore( start, end, right, len, 8 );
		RotatePointAroundVector( temp, vec, right, 45 );
		VectorCopy( temp, right );
	}
}

/*
** LerpMeshVertexes
*/
static void LerpMeshVertexes( md3Surface_t *surf, float backlerp )
{
	short    *oldXyz, *newXyz, *oldNormals, *newNormals;
	float    *outXyz, *outNormal;
	float    oldXyzScale, newXyzScale;
	float    oldNormalScale, newNormalScale;
	int      vertNum;
	unsigned lat, lng;
	int      numVerts;

	outXyz = tess.xyz[ tess.numVertexes ].v;
	outNormal = tess.normal[ tess.numVertexes ].v;

	newXyz = ( short * )( ( byte * ) surf + surf->ofsXyzNormals ) + ( backEnd.currentEntity->e.frame * surf->numVerts * 4 );
	newNormals = newXyz + 3;

	newXyzScale = MD3_XYZ_SCALE * ( 1.0 - backlerp );
	newNormalScale = 1.0 - backlerp;

	numVerts = surf->numVerts;

	if ( backlerp == 0 )
	{
		//
		// just copy the vertexes
		//
		for ( vertNum = 0; vertNum < numVerts; vertNum++, newXyz += 4, newNormals += 4, outXyz += 4, outNormal += 4 )
		{
			outXyz[ 0 ] = newXyz[ 0 ] * newXyzScale;
			outXyz[ 1 ] = newXyz[ 1 ] * newXyzScale;
			outXyz[ 2 ] = newXyz[ 2 ] * newXyzScale;

			lat = ( newNormals[ 0 ] >> 8 ) & 0xff;
			lng = ( newNormals[ 0 ] & 0xff );
			lat *= ( FUNCTABLE_SIZE / 256 );
			lng *= ( FUNCTABLE_SIZE / 256 );

			// decode X as cos( lat ) * sin( long )
			// decode Y as sin( lat ) * sin( long )
			// decode Z as cos( long )

			outNormal[ 0 ] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
			outNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
			outNormal[ 2 ] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];
		}
	}
	else
	{
		//
		// interpolate and copy the vertex and normal
		//
		oldXyz = ( short * )( ( byte * ) surf + surf->ofsXyzNormals ) + ( backEnd.currentEntity->e.oldframe * surf->numVerts * 4 );
		oldNormals = oldXyz + 3;

		oldXyzScale = MD3_XYZ_SCALE * backlerp;
		oldNormalScale = backlerp;

		for ( vertNum = 0; vertNum < numVerts; vertNum++,
		      oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4, outXyz += 4, outNormal += 4 )
		{
			//% vec3_t uncompressedOldNormal, uncompressedNewNormal;

			// interpolate the xyz
			outXyz[ 0 ] = oldXyz[ 0 ] * oldXyzScale + newXyz[ 0 ] * newXyzScale;
			outXyz[ 1 ] = oldXyz[ 1 ] * oldXyzScale + newXyz[ 1 ] * newXyzScale;
			outXyz[ 2 ] = oldXyz[ 2 ] * oldXyzScale + newXyz[ 2 ] * newXyzScale;

			// FIXME: interpolate lat/long instead?
			// ydnar: ok :)
#if 0
			lat = ( newNormals[ 0 ] >> 8 ) & 0xff;
			lng = ( newNormals[ 0 ] & 0xff );
			lat *= ( FUNCTABLE_SIZE / 256 );
			lng *= ( FUNCTABLE_SIZE / 256 );
			uncompressedNewNormal[ 0 ] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
			uncompressedNewNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
			uncompressedNewNormal[ 2 ] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];

			lat = ( oldNormals[ 0 ] >> 8 ) & 0xff;
			lng = ( oldNormals[ 0 ] & 0xff );
			lat *= ( FUNCTABLE_SIZE / 256 );
			lng *= ( FUNCTABLE_SIZE / 256 );

			uncompressedOldNormal[ 0 ] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
			uncompressedOldNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
			uncompressedOldNormal[ 2 ] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];

			outNormal[ 0 ] = uncompressedOldNormal[ 0 ] * oldNormalScale + uncompressedNewNormal[ 0 ] * newNormalScale;
			outNormal[ 1 ] = uncompressedOldNormal[ 1 ] * oldNormalScale + uncompressedNewNormal[ 1 ] * newNormalScale;
			outNormal[ 2 ] = uncompressedOldNormal[ 2 ] * oldNormalScale + uncompressedNewNormal[ 2 ] * newNormalScale;
#else
			lat = ri.ftol( ( ( ( oldNormals[ 0 ] >> 8 ) & 0xFF ) * ( FUNCTABLE_SIZE / 256 ) * newNormalScale ) +
			               ( ( ( oldNormals[ 0 ] >> 8 ) & 0xFF ) * ( FUNCTABLE_SIZE / 256 ) * oldNormalScale ) );
			lng = ri.ftol( ( ( oldNormals[ 0 ] & 0xFF ) * ( FUNCTABLE_SIZE / 256 ) * newNormalScale ) +
			               ( ( oldNormals[ 0 ] & 0xFF ) * ( FUNCTABLE_SIZE / 256 ) * oldNormalScale ) );

			outNormal[ 0 ] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
			outNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
			outNormal[ 2 ] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];
#endif

//          VectorNormalize (outNormal);
		}

		// ydnar: unecessary because of lat/lng lerping
		//% VectorArrayNormalize((vec4_t *)tess.normal[tess.numVertexes].v, numVerts);
	}
}

/*
=============
RB_SurfaceMesh
=============
*/
void RB_SurfaceMesh( md3Surface_t *surface )
{
	int   j;
	float backlerp;
	int   *triangles;
	float *texCoords;
	int   indexes;
	int   Bob, Doug;
	int   numVerts;

	// RF, check for REFLAG_HANDONLY
	if ( backEnd.currentEntity->e.reFlags & REFLAG_ONLYHAND )
	{
		if ( !strstr( surface->name, "hand" ) )
		{
			return;
		}
	}

	if ( backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame )
	{
		backlerp = 0;
	}
	else
	{
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW( surface->numVerts, surface->numTriangles * 3 );

	LerpMeshVertexes( surface, backlerp );

	triangles = ( int * )( ( byte * ) surface + surface->ofsTriangles );
	indexes = surface->numTriangles * 3;
	Bob = tess.numIndexes;
	Doug = tess.numVertexes;

	for ( j = 0; j < indexes; j++ )
	{
		tess.indexes[ Bob + j ] = Doug + triangles[ j ];
	}

	tess.numIndexes += indexes;

	texCoords = ( float * )( ( byte * ) surface + surface->ofsSt );

	numVerts = surface->numVerts;

	for ( j = 0; j < numVerts; j++ )
	{
		tess.texCoords0[ Doug + j ].v[ 0 ] = texCoords[ j * 2 + 0 ];
		tess.texCoords0[ Doug + j ].v[ 1 ] = texCoords[ j * 2 + 1 ];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;
}

/*
** R_LatLongToNormal
*/
void R_LatLongToNormal( vec3_t outNormal, short latLong )
{
	unsigned lat, lng;

	lat = ( latLong >> 8 ) & 0xff;
	lng = ( latLong & 0xff );
	lat *= ( FUNCTABLE_SIZE / 256 );
	lng *= ( FUNCTABLE_SIZE / 256 );

	// decode X as cos( lat ) * sin( long )
	// decode Y as sin( lat ) * sin( long )
	// decode Z as cos( long )

	outNormal[ 0 ] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
	outNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
	outNormal[ 2 ] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];
}

// Ridah

/*
** LerpCMeshVertexes
*/
static void LerpCMeshVertexes( mdcSurface_t *surf, float backlerp )
{
	short              *oldXyz, *newXyz, *oldNormals, *newNormals;
	float              *outXyz, *outNormal;
	float              oldXyzScale, newXyzScale;
	float              oldNormalScale, newNormalScale;
	int                vertNum;
	unsigned           lat, lng;
	int                numVerts;

	int                oldBase, newBase;
	short              *oldComp = NULL, *newComp = NULL; // TTimo: init
	mdcXyzCompressed_t *oldXyzComp = NULL, *newXyzComp = NULL; // TTimo: init
	vec3_t             oldOfsVec, newOfsVec;

	qboolean           hasComp;

	outXyz = tess.xyz[ tess.numVertexes ].v;
	outNormal = tess.normal[ tess.numVertexes ].v;

	newBase = ( int ) * ( ( short * )( ( byte * ) surf + surf->ofsFrameBaseFrames ) + backEnd.currentEntity->e.frame );
	newXyz = ( short * )( ( byte * ) surf + surf->ofsXyzNormals ) + ( newBase * surf->numVerts * 4 );
	newNormals = newXyz + 3;

	hasComp = ( surf->numCompFrames > 0 );

	if ( hasComp )
	{
		newComp = ( ( short * )( ( byte * ) surf + surf->ofsFrameCompFrames ) + backEnd.currentEntity->e.frame );

		if ( *newComp >= 0 )
		{
			newXyzComp = ( mdcXyzCompressed_t * )( ( byte * ) surf + surf->ofsXyzCompressed ) + ( *newComp * surf->numVerts );
		}
	}

	newXyzScale = MD3_XYZ_SCALE * ( 1.0 - backlerp );
	newNormalScale = 1.0 - backlerp;

	numVerts = surf->numVerts;

	if ( backlerp == 0 )
	{
		//
		// just copy the vertexes
		//
		for ( vertNum = 0; vertNum < numVerts; vertNum++, newXyz += 4, newNormals += 4, outXyz += 4, outNormal += 4 )
		{
			outXyz[ 0 ] = newXyz[ 0 ] * newXyzScale;
			outXyz[ 1 ] = newXyz[ 1 ] * newXyzScale;
			outXyz[ 2 ] = newXyz[ 2 ] * newXyzScale;

			// add the compressed ofsVec
			if ( hasComp && *newComp >= 0 )
			{
				R_MDC_DecodeXyzCompressed( newXyzComp->ofsVec, newOfsVec, outNormal );
				newXyzComp++;
				VectorAdd( outXyz, newOfsVec, outXyz );
			}
			else
			{
				lat = ( newNormals[ 0 ] >> 8 ) & 0xff;
				lng = ( newNormals[ 0 ] & 0xff );
				lat *= ( FUNCTABLE_SIZE / 256 ); // was 4 :sigh:
				lng *= ( FUNCTABLE_SIZE / 256 );

				outNormal[ 0 ] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
				outNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
				outNormal[ 2 ] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];

				// ydnar: testing anorms table
				//% VectorCopy( r_anormals[ (lng & (0xF << 4)) | ((lat >> 4) & 0xF) ], outNormal );
			}
		}
	}
	else
	{
		//
		// interpolate and copy the vertex and normal
		//
		oldBase = ( int ) * ( ( short * )( ( byte * ) surf + surf->ofsFrameBaseFrames ) + backEnd.currentEntity->e.oldframe );
		oldXyz = ( short * )( ( byte * ) surf + surf->ofsXyzNormals ) + ( oldBase * surf->numVerts * 4 );
		oldNormals = oldXyz + 3;

		if ( hasComp )
		{
			oldComp = ( ( short * )( ( byte * ) surf + surf->ofsFrameCompFrames ) + backEnd.currentEntity->e.oldframe );

			if ( *oldComp >= 0 )
			{
				oldXyzComp = ( mdcXyzCompressed_t * )( ( byte * ) surf + surf->ofsXyzCompressed ) + ( *oldComp * surf->numVerts );
			}
		}

		oldXyzScale = MD3_XYZ_SCALE * backlerp;
		oldNormalScale = backlerp;

		for ( vertNum = 0; vertNum < numVerts; vertNum++,
		      oldXyz += 4, newXyz += 4, oldNormals += 4, newNormals += 4, outXyz += 4, outNormal += 4 )
		{
			vec3_t uncompressedOldNormal, uncompressedNewNormal;

			// interpolate the xyz
			outXyz[ 0 ] = oldXyz[ 0 ] * oldXyzScale + newXyz[ 0 ] * newXyzScale;
			outXyz[ 1 ] = oldXyz[ 1 ] * oldXyzScale + newXyz[ 1 ] * newXyzScale;
			outXyz[ 2 ] = oldXyz[ 2 ] * oldXyzScale + newXyz[ 2 ] * newXyzScale;

			// add the compressed ofsVec
			if ( hasComp && *newComp >= 0 )
			{
				R_MDC_DecodeXyzCompressed( newXyzComp->ofsVec, newOfsVec, uncompressedNewNormal );
				newXyzComp++;
				VectorMA( outXyz, 1.0 - backlerp, newOfsVec, outXyz );
			}
			else
			{
				lat = ( newNormals[ 0 ] >> 8 ) & 0xff;
				lng = ( newNormals[ 0 ] & 0xff );
				lat *= ( FUNCTABLE_SIZE / 256 ); // was 4 :sigh:
				lng *= ( FUNCTABLE_SIZE / 256 );

				uncompressedNewNormal[ 0 ] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
				uncompressedNewNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
				uncompressedNewNormal[ 2 ] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];
			}

			if ( hasComp && *oldComp >= 0 )
			{
				R_MDC_DecodeXyzCompressed( oldXyzComp->ofsVec, oldOfsVec, uncompressedOldNormal );
				oldXyzComp++;
				VectorMA( outXyz, backlerp, oldOfsVec, outXyz );
			}
			else
			{
				lat = ( oldNormals[ 0 ] >> 8 ) & 0xff;
				lng = ( oldNormals[ 0 ] & 0xff );
				lat *= ( FUNCTABLE_SIZE / 256 ); // was 4 :sigh:
				lng *= ( FUNCTABLE_SIZE / 256 );

				uncompressedOldNormal[ 0 ] = tr.sinTable[( lat + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ] * tr.sinTable[ lng ];
				uncompressedOldNormal[ 1 ] = tr.sinTable[ lat ] * tr.sinTable[ lng ];
				uncompressedOldNormal[ 2 ] = tr.sinTable[( lng + ( FUNCTABLE_SIZE / 4 ) ) & FUNCTABLE_MASK ];
			}

			outNormal[ 0 ] = uncompressedOldNormal[ 0 ] * oldNormalScale + uncompressedNewNormal[ 0 ] * newNormalScale;
			outNormal[ 1 ] = uncompressedOldNormal[ 1 ] * oldNormalScale + uncompressedNewNormal[ 1 ] * newNormalScale;
			outNormal[ 2 ] = uncompressedOldNormal[ 2 ] * oldNormalScale + uncompressedNewNormal[ 2 ] * newNormalScale;

			// ydnar: wee bit faster (fixme: use lat/lng lerping)
			//% VectorNormalize (outNormal);
			VectorNormalizeFast( outNormal );
		}
	}
}

/*
=============
RB_SurfaceCMesh
=============
*/
void RB_SurfaceCMesh( mdcSurface_t *surface )
{
	int   j;
	float backlerp;
	int   *triangles;
	float *texCoords;
	int   indexes;
	int   Bob, Doug;
	int   numVerts;

	// RF, check for REFLAG_HANDONLY
	if ( backEnd.currentEntity->e.reFlags & REFLAG_ONLYHAND )
	{
		if ( !strstr( surface->name, "hand" ) )
		{
			return;
		}
	}

	if ( backEnd.currentEntity->e.oldframe == backEnd.currentEntity->e.frame )
	{
		backlerp = 0;
	}
	else
	{
		backlerp = backEnd.currentEntity->e.backlerp;
	}

	RB_CHECKOVERFLOW( surface->numVerts, surface->numTriangles * 3 );

	LerpCMeshVertexes( surface, backlerp );

	triangles = ( int * )( ( byte * ) surface + surface->ofsTriangles );
	indexes = surface->numTriangles * 3;
	Bob = tess.numIndexes;
	Doug = tess.numVertexes;

	for ( j = 0; j < indexes; j++ )
	{
		tess.indexes[ Bob + j ] = Doug + triangles[ j ];
	}

	tess.numIndexes += indexes;

	texCoords = ( float * )( ( byte * ) surface + surface->ofsSt );

	numVerts = surface->numVerts;

	for ( j = 0; j < numVerts; j++ )
	{
		tess.texCoords0[ Doug + j ].v[ 0 ] = texCoords[ j * 2 + 0 ];
		tess.texCoords0[ Doug + j ].v[ 1 ] = texCoords[ j * 2 + 1 ];
		// FIXME: fill in lightmapST for completeness?
	}

	tess.numVertexes += surface->numVerts;
}

// done.

/*
==============
RB_SurfaceFace
==============
*/
void RB_SurfaceFace( srfSurfaceFace_t *surf )
{
	int      i;
	unsigned *indices, *tessIndexes;
	float    *v;
	float    *normal;
	int      ndx;
	int      Bob;
	int      numPoints;
	int      dlightBits;

	RB_CHECKOVERFLOW( surf->numPoints, surf->numIndices );

	dlightBits = surf->dlightBits[ backEnd.smpFrame ];
	tess.dlightBits |= dlightBits;

	indices = ( unsigned * )( ( ( char * ) surf ) + surf->ofsIndices );

	Bob = tess.numVertexes;
	tessIndexes = tess.indexes + tess.numIndexes;

	for ( i = surf->numIndices - 1; i >= 0; i-- )
	{
		tessIndexes[ i ] = indices[ i ] + Bob;
	}

	tess.numIndexes += surf->numIndices;

	v = surf->points[ 0 ];

	ndx = tess.numVertexes;

	numPoints = surf->numPoints;

	if ( tess.shader->needsNormal )
	{
		normal = surf->plane.normal;

		for ( i = 0, ndx = tess.numVertexes; i < numPoints; i++, ndx++ )
		{
			VectorCopy( normal, tess.normal[ ndx ].v );
		}
	}

	for ( i = 0, v = surf->points[ 0 ], ndx = tess.numVertexes; i < numPoints; i++, v += VERTEXSIZE, ndx++ )
	{
		VectorCopy( v, tess.xyz[ ndx ].v );
		tess.texCoords0[ ndx ].v[ 0 ] = v[ 3 ];
		tess.texCoords0[ ndx ].v[ 1 ] = v[ 4 ];
		tess.texCoords1[ ndx ].v[ 0 ] = v[ 5 ];
		tess.texCoords1[ ndx ].v[ 1 ] = v[ 6 ];
		* ( unsigned int * ) &tess.vertexColors[ ndx ].v = * ( unsigned int * ) &v[ 7 ];
	}

	tess.numVertexes += surf->numPoints;
}

static float LodErrorForVolume( vec3_t local, float radius )
{
	vec3_t world;
	float  d;

	// never let it go lower than 1
	if ( r_lodCurveError->value < 1 )
	{
		return 1;
	}

	world[ 0 ] = local[ 0 ] * backEnd.orientation.axis[ 0 ][ 0 ] + local[ 1 ] * backEnd.orientation.axis[ 1 ][ 0 ] +
	             local[ 2 ] * backEnd.orientation.axis[ 2 ][ 0 ] + backEnd.orientation.origin[ 0 ];
	world[ 1 ] = local[ 0 ] * backEnd.orientation.axis[ 0 ][ 1 ] + local[ 1 ] * backEnd.orientation.axis[ 1 ][ 1 ] +
	             local[ 2 ] * backEnd.orientation.axis[ 2 ][ 1 ] + backEnd.orientation.origin[ 1 ];
	world[ 2 ] = local[ 0 ] * backEnd.orientation.axis[ 0 ][ 2 ] + local[ 1 ] * backEnd.orientation.axis[ 1 ][ 2 ] +
	             local[ 2 ] * backEnd.orientation.axis[ 2 ][ 2 ] + backEnd.orientation.origin[ 2 ];

	VectorSubtract( world, backEnd.viewParms.orientation.origin, world );
	d = DotProduct( world, backEnd.viewParms.orientation.axis[ 0 ] );

	if ( d < 0 )
	{
		d = -d;
	}

	d -= radius;

	if ( d < 1 )
	{
		d = 1;
	}

	return r_lodCurveError->value / d;
}

/*
=============
RB_SurfaceGrid

Just copy the grid of points and triangulate
=============
*/
void RB_SurfaceGrid( srfGridMesh_t *cv )
{
	int           i, j;
	float         *xyz;
	float         *texCoords0, *texCoords1;
	float         *normal;
	unsigned char *color;
	drawVert_t    *dv;
	int           rows, irows, vrows;
	int           used;
	int           widthTable[ MAX_GRID_SIZE ];
	int           heightTable[ MAX_GRID_SIZE ];
	float         lodError;
	int           lodWidth, lodHeight;
	int           numVertexes;
	int           dlightBits;
	qboolean      needsNormal;

	dlightBits = cv->dlightBits[ backEnd.smpFrame ];
	tess.dlightBits |= dlightBits;

	// determine the allowable discrepance
	lodError = LodErrorForVolume( cv->lodOrigin, cv->lodRadius );

	// determine which rows and columns of the subdivision
	// we are actually going to use
	widthTable[ 0 ] = 0;
	lodWidth = 1;

	for ( i = 1; i < cv->width - 1; i++ )
	{
		if ( cv->widthLodError[ i ] <= lodError )
		{
			widthTable[ lodWidth ] = i;
			lodWidth++;
		}
	}

	widthTable[ lodWidth ] = cv->width - 1;
	lodWidth++;

	heightTable[ 0 ] = 0;
	lodHeight = 1;

	for ( i = 1; i < cv->height - 1; i++ )
	{
		if ( cv->heightLodError[ i ] <= lodError )
		{
			heightTable[ lodHeight ] = i;
			lodHeight++;
		}
	}

	heightTable[ lodHeight ] = cv->height - 1;
	lodHeight++;

	// very large grids may have more points or indexes than can be fit
	// in the tess structure, so we may have to issue it in multiple passes

	used = 0;
	rows = 0;

	while ( used < lodHeight - 1 )
	{
		// see how many rows of both verts and indexes we can add without overflowing
		do
		{
			vrows = ( tess.maxShaderVerts - tess.numVertexes ) / lodWidth;
			irows = ( tess.maxShaderIndicies - tess.numIndexes ) / ( lodWidth * 6 );

			// if we don't have enough space for at least one strip, flush the buffer
			if ( vrows < 2 || irows < 1 )
			{
				RB_EndSurface();
				RB_BeginSurface( tess.shader, tess.fogNum );
				tess.dlightBits |= dlightBits; // ydnar: for proper dlighting
			}
			else
			{
				break;
			}
		}
		while ( 1 );

		rows = irows;

		if ( vrows < irows + 1 )
		{
			rows = vrows - 1;
		}

		if ( used + rows > lodHeight )
		{
			rows = lodHeight - used;
		}

		numVertexes = tess.numVertexes;

		xyz = tess.xyz[ numVertexes ].v;
		normal = tess.normal[ numVertexes ].v;
		texCoords0 = tess.texCoords0[ numVertexes ].v;
		texCoords1 = tess.texCoords1[ numVertexes ].v;
		color = ( unsigned char * ) &tess.vertexColors[ numVertexes ].v;
		needsNormal = tess.shader->needsNormal;

		for ( i = 0; i < rows; i++ )
		{
			for ( j = 0; j < lodWidth; j++ )
			{
				dv = cv->verts + heightTable[ used + i ] * cv->width + widthTable[ j ];

				xyz[ 0 ] = dv->xyz[ 0 ];
				xyz[ 1 ] = dv->xyz[ 1 ];
				xyz[ 2 ] = dv->xyz[ 2 ];
				texCoords0[ 0 ] = dv->st[ 0 ];
				texCoords0[ 1 ] = dv->st[ 1 ];
				texCoords1[ 0 ] = dv->lightmap[ 0 ];
				texCoords1[ 1 ] = dv->lightmap[ 1 ];

				if ( needsNormal )
				{
					normal[ 0 ] = dv->normal[ 0 ];
					normal[ 1 ] = dv->normal[ 1 ];
					normal[ 2 ] = dv->normal[ 2 ];
				}

				* ( unsigned int * ) color = * ( unsigned int * ) dv->color;
				xyz += 4;
				normal += 4;
				texCoords0 += 2;
				texCoords1 += 2;
				color += 4;
			}
		}

		// add the indexes
		{
			int numIndexes;
			int w, h;

			h = rows - 1;
			w = lodWidth - 1;
			numIndexes = tess.numIndexes;

			for ( i = 0; i < h; i++ )
			{
				for ( j = 0; j < w; j++ )
				{
					int v1, v2, v3, v4;

					// vertex order to be reckognized as tristrips
					v1 = numVertexes + i * lodWidth + j + 1;
					v2 = v1 - 1;
					v3 = v2 + lodWidth;
					v4 = v3 + 1;

					tess.indexes[ numIndexes ] = v2;
					tess.indexes[ numIndexes + 1 ] = v3;
					tess.indexes[ numIndexes + 2 ] = v1;

					tess.indexes[ numIndexes + 3 ] = v1;
					tess.indexes[ numIndexes + 4 ] = v3;
					tess.indexes[ numIndexes + 5 ] = v4;
					numIndexes += 6;
				}
			}

			tess.numIndexes = numIndexes;
		}

		tess.numVertexes += rows * lodWidth;

		used += rows - 1;
	}
}

/*
==============
RB_SurfaceMD5
==============
*/
static void RB_SurfaceMD5( md5Surface_t *srf )
{
	int             i, j, k;
	int             numIndexes = 0;
	int             numVertexes;
	md5Model_t      *model;
	md5Vertex_t     *v;
	md5Bone_t       *bone;
	md5Triangle_t   *tri;
	vec3_t          lightOrigin;
	float           *xyzw, *xyzw2;
	static matrix_t boneMatrices[ MAX_BONES ];
	vec3_t          tmpVert;
	vec3_t          tmpPosition;
	md5Weight_t     *w;

	GLimp_LogComment( "--- Tess_SurfaceMD5 ---\n" );

	RB_CHECKOVERFLOW( srf->numVerts, srf->numTriangles * 3 );

	model = srf->model;

	numIndexes = srf->numTriangles * 3;

	for ( i = 0, tri = srf->triangles; i < srf->numTriangles; i++, tri++ )
	{
		tess.indexes[ tess.numIndexes + i * 3 + 0 ] = tess.numVertexes + tri->indexes[ 0 ];
		tess.indexes[ tess.numIndexes + i * 3 + 1 ] = tess.numVertexes + tri->indexes[ 1 ];
		tess.indexes[ tess.numIndexes + i * 3 + 2 ] = tess.numVertexes + tri->indexes[ 2 ];
	}

	// convert bones back to matrices
	for ( i = 0; i < model->numBones; i++ )
	{
		matrix_t m, m2;

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )

		if ( backEnd.currentEntity->e.skeleton.type == SK_ABSOLUTE )
		{
			MatrixSetupScale( m,
			                  backEnd.currentEntity->e.skeleton.scale[ 0 ],
			                  backEnd.currentEntity->e.skeleton.scale[ 1 ], backEnd.currentEntity->e.skeleton.scale[ 2 ] );

			MatrixSetupTransformFromQuat( m2, backEnd.currentEntity->e.skeleton.bones[ i ].rotation,
			                              backEnd.currentEntity->e.skeleton.bones[ i ].origin );
			MatrixMultiply( m2, m, boneMatrices[ i ] );
		}
		else
#endif
		{
			MatrixSetupTransformFromQuat( boneMatrices[ i ], model->bones[ i ].rotation, model->bones[ i ].origin );
		}
	}

	// deform the vertices by the lerped bones
	numVertexes = srf->numVerts;

	for ( j = 0, v = srf->verts; j < numVertexes; j++, v++ )
	{
		VectorClear( tmpPosition );

		for ( k = 0, w = v->weights[ 0 ]; k < v->numWeights; k++, w++ )
		{
			bone = &model->bones[ w->boneIndex ];

			MatrixTransformPoint( boneMatrices[ w->boneIndex ], w->offset, tmpVert );
			VectorMA( tmpPosition, w->boneWeight, tmpVert, tmpPosition );
		}

		tess.xyz[ tess.numVertexes + j ].v[ 0 ] = tmpPosition[ 0 ];
		tess.xyz[ tess.numVertexes + j ].v[ 1 ] = tmpPosition[ 1 ];
		tess.xyz[ tess.numVertexes + j ].v[ 2 ] = tmpPosition[ 2 ];
		tess.xyz[ tess.numVertexes + j ].v[ 3 ] = 1;

		tess.texCoords0[ tess.numVertexes + j ].v[ 0 ] = v->texCoords[ 0 ];
		tess.texCoords0[ tess.numVertexes + j ].v[ 1 ] = v->texCoords[ 1 ];
	}

	tess.numIndexes += numIndexes;
	tess.numVertexes += numVertexes;
}

/*
===========================================================================

NULL MODEL

===========================================================================
*/

/*
===================
RB_SurfaceAxis

Draws x/y/z lines from the origin for orientation debugging
===================
*/
void RB_SurfaceAxis( void )
{
	GL_Bind( tr.whiteImage );
	glLineWidth( 3 );
	glBegin( GL_LINES );
	glColor3f( 1, 0, 0 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 16, 0, 0 );
	glColor3f( 0, 1, 0 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 0, 16, 0 );
	glColor3f( 0, 0, 1 );
	glVertex3f( 0, 0, 0 );
	glVertex3f( 0, 0, 16 );
	glEnd();
	glLineWidth( 1 );
}

//===========================================================================

/*
====================
RB_SurfaceEntity

Entities that have a single procedurally generated surface
====================
*/
void RB_SurfaceEntity( surfaceType_t *surfType )
{
	switch ( backEnd.currentEntity->e.reType )
	{
		case RT_SPLASH:
			RB_SurfaceSplash();
			break;

		case RT_SPRITE:
			RB_SurfaceSprite();
			break;

		case RT_BEAM:
			RB_SurfaceBeam();
			break;

		case RT_RAIL_CORE:
			RB_SurfaceRailCore();
			break;

		case RT_RAIL_RINGS:
			RB_SurfaceRailRings();
			break;

		case RT_LIGHTNING:
			RB_SurfaceLightningBolt();
			break;

		default:
			RB_SurfaceAxis();
			break;
	}

	return;
}

void RB_SurfaceBad( surfaceType_t *surfType )
{
	ri.Printf( PRINT_ALL, "Bad surface tesselated.\n" );
}

#if 0

void RB_SurfaceFlare( srfFlare_t *surf )
{
	vec3_t left, up;
	float  radius;
	byte   color[ 4 ];
	vec3_t dir;
	vec3_t origin;
	float  d;

	// calculate the xyz locations for the four corners
	radius = 30;
	VectorScale( backEnd.viewParms.orientation.axis[ 1 ], radius, left );
	VectorScale( backEnd.viewParms.orientation.axis[ 2 ], radius, up );

	if ( backEnd.viewParms.isMirror )
	{
		VectorSubtract( vec3_origin, left, left );
	}

	color[ 0 ] = color[ 1 ] = color[ 2 ] = color[ 3 ] = 255;

	VectorMA( surf->origin, 3, surf->normal, origin );
	VectorSubtract( origin, backEnd.viewParms.orientation.origin, dir );
	VectorNormalize( dir );
	VectorMA( origin, r_ignore->value, dir, origin );

	d = -DotProduct( dir, surf->normal );

	if ( d < 0 )
	{
		return;
	}

#if 0
	color[ 0 ] *= d;
	color[ 1 ] *= d;
	color[ 2 ] *= d;
#endif

	RB_AddQuadStamp( origin, left, up, color );
}

#else

void RB_SurfaceFlare( srfFlare_t *surf )
{
#if 0
	vec3_t left, up;
	byte   color[ 4 ];

	color[ 0 ] = surf->color[ 0 ] * 255;
	color[ 1 ] = surf->color[ 1 ] * 255;
	color[ 2 ] = surf->color[ 2 ] * 255;
	color[ 3 ] = 255;

	VectorClear( left );
	VectorClear( up );

	left[ 0 ] = r_ignore->value;

	up[ 1 ] = r_ignore->value;

	RB_AddQuadStampExt( surf->origin, left, up, color, 0, 0, 1, 1 );
#endif
}

#endif

void RB_SurfaceDisplayList( srfDisplayList_t *surf )
{
	// all apropriate state must be set in RB_BeginSurface
	// this isn't implemented yet...
	glCallList( surf->listNum );
}

void RB_SurfacePolyBuffer( srfPolyBuffer_t *surf )
{
	vec4hack_t     *oldXYZ;
	vec2hack_t     *oldST;
	glIndex_t      *oldIndicies;
	color4ubhack_t *oldColor;
	int            oldMaxVerts;
	int            oldMaxIndicies;

	RB_EndSurface();

	RB_BeginSurface( tess.shader, tess.fogNum );

	oldXYZ = tess.xyz;
	oldST = tess.texCoords0;
	oldIndicies = tess.indexes;
	oldMaxVerts = tess.maxShaderVerts;
	oldMaxIndicies = tess.maxShaderIndicies;
	oldColor = tess.vertexColors;

	// ===================================================
	tess.numIndexes = surf->pPolyBuffer->numIndicies;
	tess.numVertexes = surf->pPolyBuffer->numVerts;

	tess.xyz = ( vec4hack_t * ) surf->pPolyBuffer->xyz;
	tess.texCoords0 = ( vec2hack_t * ) surf->pPolyBuffer->st;
	tess.indexes = surf->pPolyBuffer->indicies;
	tess.vertexColors = ( color4ubhack_t * ) surf->pPolyBuffer->color;

	tess.maxShaderIndicies = MAX_PB_INDICIES;
	tess.maxShaderVerts = MAX_PB_VERTS;
	// ===================================================

	RB_EndSurface();

	tess.xyz = oldXYZ;
	tess.texCoords0 = oldST;
	tess.indexes = oldIndicies;
	tess.maxShaderVerts = oldMaxVerts;
	tess.maxShaderIndicies = oldMaxIndicies;
	tess.vertexColors = oldColor;
}

// ydnar: decal surfaces
void RB_SurfaceDecal( srfDecal_t *srf )
{
	int i;
	int numv;

	RB_CHECKOVERFLOW( srf->numVerts, 3 * ( srf->numVerts - 2 ) );

	// fan triangles into the tess array
	numv = tess.numVertexes;

	for ( i = 0; i < srf->numVerts; i++ )
	{
		VectorCopy( srf->verts[ i ].xyz, tess.xyz[ numv ].v );
		tess.texCoords0[ numv ].v[ 0 ] = srf->verts[ i ].st[ 0 ];
		tess.texCoords0[ numv ].v[ 1 ] = srf->verts[ i ].st[ 1 ];
		* ( int * ) &tess.vertexColors[ numv ].v = * ( int * ) srf->verts[ i ].modulate;
		numv++;
	}

	/* generate fan indexes into the tess array */
	for ( i = 0; i < srf->numVerts - 2; i++ )
	{
		tess.indexes[ tess.numIndexes + 0 ] = tess.numVertexes;
		tess.indexes[ tess.numIndexes + 1 ] = tess.numVertexes + i + 1;
		tess.indexes[ tess.numIndexes + 2 ] = tess.numVertexes + i + 2;
		tess.numIndexes += 3;
	}

	tess.numVertexes = numv;
}

void RB_SurfaceSkip( void *surf )
{
	return;
}

void ( *rb_surfaceTable[ SF_NUM_SURFACE_TYPES ] )( void * ) =
{
	( void ( * )( void * ) ) RB_SurfaceBad,  // SF_BAD,
	( void ( * )( void * ) ) RB_SurfaceSkip,  // SF_SKIP,
	( void ( * )( void * ) ) RB_SurfaceFace,  // SF_FACE,
	( void ( * )( void * ) ) RB_SurfaceGrid,  // SF_GRID,
	( void ( * )( void * ) ) RB_SurfaceTriangles,  // SF_TRIANGLES,
	( void ( * )( void * ) ) RB_SurfaceFoliage,  // SF_FOLIAGE,
	( void ( * )( void * ) ) RB_SurfacePolychain,  // SF_POLY,
	( void ( * )( void * ) ) RB_SurfaceMesh,  // SF_MD3,
	( void ( * )( void * ) ) RB_SurfaceCMesh,  // SF_MDC,
	( void ( * )( void * ) ) RB_SurfaceAnim,  // SF_MDS,
	( void ( * )( void * ) ) RB_MDM_SurfaceAnim,  // SF_MDM,
	( void ( * )( void * ) ) RB_SurfaceMD5,  // SF_MD5,
	( void ( * )( void * ) ) RB_SurfaceFlare,  // SF_FLARE,
	( void ( * )( void * ) ) RB_SurfaceEntity,  // SF_ENTITY
	( void ( * )( void * ) ) RB_SurfaceDisplayList,  // SF_DISPLAY_LIST
	( void ( * )( void * ) ) RB_SurfacePolyBuffer,  // SF_POLYBUFFER
	( void ( * )( void * ) ) RB_SurfaceDecal,  // SF_DECAL
};
