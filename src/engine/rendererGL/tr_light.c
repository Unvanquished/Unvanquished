/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2011 Robert Beckebans <trebor_7@users.sourceforge.net>

This file is part of Daemon source code.

Daemon source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon source code; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/
// tr_light.c
#include "tr_local.h"

/*
=============
R_AddBrushModelInteractions

Determine which dynamic lights may effect this bmodel
=============
*/
void R_AddBrushModelInteractions( trRefEntity_t *ent, trRefLight_t *light )
{
	int               i;
	bspSurface_t      *surf;
	bspModel_t        *bspModel = NULL;
	model_t           *pModel = NULL;
	byte              cubeSideBits;
	interactionType_t iaType = IA_DEFAULT;

	// cull the entire model if it is outside the view frustum
	// and we don't care about proper shadowing
	if( ent->cull == CULL_OUT )
	{
		if( r_shadows->integer <= SHADOWING_BLOB || light->l.noShadows )
		{
			return;
		}
		else
		{
			iaType = IA_SHADOWONLY;
		}
	}

	// avoid drawing of certain objects
#if defined( USE_REFENTITY_NOSHADOWID )

	if( light->l.inverseShadows )
	{
		if( iaType != IA_LIGHTONLY && ( light->l.noShadowID && ( light->l.noShadowID != ent->e.noShadowID ) ) )
		{
			return;
		}
	}
	else
	{
		if( iaType != IA_LIGHTONLY && ( light->l.noShadowID && ( light->l.noShadowID == ent->e.noShadowID ) ) )
		{
			return;
		}
	}

#endif

	pModel = R_GetModelByHandle( ent->e.hModel );
	bspModel = pModel->bsp;

	// do a quick AABB cull
	if( !BoundsIntersect( light->worldBounds[ 0 ], light->worldBounds[ 1 ], ent->worldBounds[ 0 ], ent->worldBounds[ 1 ] ) )
	{
		tr.pc.c_dlightSurfacesCulled += bspModel->numSurfaces;
		return;
	}

	// do a more expensive and precise light frustum cull
	if( !r_noLightFrustums->integer )
	{
		if( R_CullLightWorldBounds( light, ent->worldBounds ) == CULL_OUT )
		{
			tr.pc.c_dlightSurfacesCulled += bspModel->numSurfaces;
			return;
		}
	}

	cubeSideBits = R_CalcLightCubeSideBits( light, ent->worldBounds );

	if( r_vboModels->integer && bspModel->numVBOSurfaces )
	{
		srfVBOMesh_t *vboSurface;
		shader_t     *shader;

		// static VBOs are fine for lighting and shadow mapping
		for( i = 0; i < bspModel->numVBOSurfaces; i++ )
		{
			vboSurface = bspModel->vboSurfaces[ i ];
			shader = vboSurface->shader;

			// skip all surfaces that don't matter for lighting only pass
			if( shader->isSky || ( !shader->interactLight && shader->noShadows ) )
			{
				continue;
			}

			R_AddLightInteraction( light, ( void * ) vboSurface, shader, cubeSideBits, iaType );
			tr.pc.c_dlightSurfaces++;
		}
	}
	else
	{
		// set the light bits in all the surfaces
		for( i = 0; i < bspModel->numSurfaces; i++ )
		{
			surf = bspModel->firstSurface + i;

			// FIXME: do more culling?

			/*
			   if(*surf->data == SF_FACE)
			   {
			   ((srfSurfaceFace_t *) surf->data)->dlightBits[tr.smpFrame] = mask;
			   }
			   else if(*surf->data == SF_GRID)
			   {
			   ((srfGridMesh_t *) surf->data)->dlightBits[tr.smpFrame] = mask;
			   }
			   else if(*surf->data == SF_TRIANGLES)
			   {
			   ((srfTriangles_t *) surf->data)->dlightBits[tr.smpFrame] = mask;
			   }
			 */

			// skip all surfaces that don't matter for lighting only pass
			if( surf->shader->isSky || ( !surf->shader->interactLight && surf->shader->noShadows ) )
			{
				continue;
			}

			R_AddLightInteraction( light, surf->data, surf->shader, cubeSideBits, iaType );
			tr.pc.c_dlightSurfaces++;
		}
	}
}

/*
=============================================================================

LIGHT SAMPLING

=============================================================================
*/

/*
=================
R_SetupEntityLightingGrid
=================
*/
static void R_SetupEntityLightingGrid( trRefEntity_t *ent, vec3_t forcedOrigin )
{
	vec3_t         lightOrigin;
	int            pos[ 3 ];
	int            i, j;
	bspGridPoint_t *gridPoint;
	bspGridPoint_t *gridPoint2;
	float          frac[ 3 ];
	int            gridStep[ 3 ];
	vec3_t         direction;
	float          totalFactor;

	if( forcedOrigin )
	{
		VectorCopy( forcedOrigin, lightOrigin );
	}
	else
	{
		if( ent->e.renderfx & RF_LIGHTING_ORIGIN )
		{
			// seperate lightOrigins are needed so an object that is
			// sinking into the ground can still be lit, and so
			// multi-part models can be lit identically
			VectorCopy( ent->e.lightingOrigin, lightOrigin );
		}
		else
		{
			VectorCopy( ent->e.origin, lightOrigin );
		}
	}

	VectorSubtract( lightOrigin, tr.world->lightGridOrigin, lightOrigin );

	for( i = 0; i < 3; i++ )
	{
		float v;

		v = lightOrigin[ i ] * tr.world->lightGridInverseSize[ i ];
		pos[ i ] = floor( v );
		frac[ i ] = v - pos[ i ];

		if( pos[ i ] < 0 )
		{
			pos[ i ] = 0;
		}
		else if( pos[ i ] >= tr.world->lightGridBounds[ i ] - 1 )
		{
			pos[ i ] = tr.world->lightGridBounds[ i ] - 1;
		}
	}

	VectorClear( ent->ambientLight );
	VectorClear( ent->directedLight );
	VectorClear( direction );

	// trilerp the light value
	gridStep[ 0 ] = 1; //sizeof(bspGridPoint_t);
	gridStep[ 1 ] = tr.world->lightGridBounds[ 0 ]; // * sizeof(bspGridPoint_t);
	gridStep[ 2 ] = tr.world->lightGridBounds[ 0 ] * tr.world->lightGridBounds[ 1 ]; // * sizeof(bspGridPoint_t);
	gridPoint = tr.world->lightGridData + pos[ 0 ] * gridStep[ 0 ] + pos[ 1 ] * gridStep[ 1 ] + pos[ 2 ] * gridStep[ 2 ];

	totalFactor = 0;

	for( i = 0; i < 8; i++ )
	{
		float factor;

		factor = 1.0;
		gridPoint2 = gridPoint;

		for( j = 0; j < 3; j++ )
		{
			if( i & ( 1 << j ) )
			{
				factor *= frac[ j ];
				gridPoint2 += gridStep[ j ];
			}
			else
			{
				factor *= ( 1.0f - frac[ j ] );
			}
		}

		if( !( gridPoint2->ambientColor[ 0 ] + gridPoint2->ambientColor[ 1 ] + gridPoint2->ambientColor[ 2 ] ) )
		{
			continue; // ignore samples in walls
		}

		totalFactor += factor;

		ent->ambientLight[ 0 ] += factor * gridPoint2->ambientColor[ 0 ];
		ent->ambientLight[ 1 ] += factor * gridPoint2->ambientColor[ 1 ];
		ent->ambientLight[ 2 ] += factor * gridPoint2->ambientColor[ 2 ];

		ent->directedLight[ 0 ] += factor * gridPoint2->directedColor[ 0 ];
		ent->directedLight[ 1 ] += factor * gridPoint2->directedColor[ 1 ];
		ent->directedLight[ 2 ] += factor * gridPoint2->directedColor[ 2 ];

		VectorMA( direction, factor, gridPoint2->direction, direction );
	}

#if 1

	if( totalFactor > 0 && totalFactor < 0.99 )
	{
		totalFactor = 1.0f / totalFactor;
		VectorScale( ent->ambientLight, totalFactor, ent->ambientLight );
		VectorScale( ent->directedLight, totalFactor, ent->directedLight );
	}

#endif

	VectorNormalize2( direction, ent->lightDir );

	if( VectorLength( ent->ambientLight ) < r_forceAmbient->value )
	{
		ent->ambientLight[ 0 ] = r_forceAmbient->value;
		ent->ambientLight[ 1 ] = r_forceAmbient->value;
		ent->ambientLight[ 2 ] = r_forceAmbient->value;
	}

//----(SA)  added
	// cheats?  check for single player?
	if( tr.lightGridMulDirected )
	{
		VectorScale( ent->directedLight, tr.lightGridMulDirected, ent->directedLight );
	}

	if( tr.lightGridMulAmbient )
	{
		VectorScale( ent->ambientLight, tr.lightGridMulAmbient, ent->ambientLight );
	}

//----(SA)  end
}

/*
===============
LogLight
===============
*/
#if 0
static void LogLight( trRefEntity_t *ent )
{
	int max1, max2;

	if( !( ent->e.renderfx & RF_FIRST_PERSON ) )
	{
		return;
	}

	max1 = ent->ambientLight[ 0 ];

	if( ent->ambientLight[ 1 ] > max1 )
	{
		max1 = ent->ambientLight[ 1 ];
	}
	else if( ent->ambientLight[ 2 ] > max1 )
	{
		max1 = ent->ambientLight[ 2 ];
	}

	max2 = ent->directedLight[ 0 ];

	if( ent->directedLight[ 1 ] > max2 )
	{
		max2 = ent->directedLight[ 1 ];
	}
	else if( ent->directedLight[ 2 ] > max2 )
	{
		max2 = ent->directedLight[ 2 ];
	}

	ri.Printf( PRINT_ALL, "amb:%i  dir:%i\n", max1, max2 );
}

#endif

/*
=================
R_SetupEntityLighting

Calculates all the lighting values that will be used
by the Calc_* functions
=================
*/
void R_SetupEntityLighting( const trRefdef_t *refdef, trRefEntity_t *ent, vec3_t forcedOrigin )
{
	//vec3_t          lightDir;
	//vec3_t          lightOrigin;
	//float           d;

	// lighting calculations
	if( ent->lightingCalculated )
	{
		return;
	}

	ent->lightingCalculated = qtrue;

	/*
	if(forcedOrigin)
	{
	        VectorCopy(forcedOrigin, lightOrigin);
	}
	else
	{
	        // trace a sample point down to find ambient light
	        if(ent->e.renderfx & RF_LIGHTING_ORIGIN)
	        {
	                // seperate lightOrigins are needed so an object that is
	                // sinking into the ground can still be lit, and so
	                // multi-part models can be lit identically
	                VectorCopy(ent->e.lightingOrigin, lightOrigin);
	        }
	        else
	        {
	                VectorCopy(ent->e.origin, lightOrigin);
	        }
	}
	*/

	// if NOWORLDMODEL, only use dynamic lights (menu system, etc)
	if( !( refdef->rdflags & RDF_NOWORLDMODEL ) && tr.world && tr.world->lightGridData )
	{
		R_SetupEntityLightingGrid( ent, forcedOrigin );
	}
	else
	{
#if 0

		if( !( refdef->rdflags & RDF_NOWORLDMODEL ) )
		{
			ent->ambientLight[ 0 ] = tr.worldEntity.ambientLight[ 0 ];
			ent->ambientLight[ 1 ] = tr.worldEntity.ambientLight[ 1 ];
			ent->ambientLight[ 2 ] = tr.worldEntity.ambientLight[ 2 ];
		}
		else
		{
			ent->ambientLight[ 0 ] = r_forceAmbient->value;
			ent->ambientLight[ 1 ] = r_forceAmbient->value;
			ent->ambientLight[ 2 ] = r_forceAmbient->value;
		}

		ent->directedLight[ 0 ] = ent->directedLight[ 1 ] = ent->directedLight[ 2 ] = tr.identityLight * ( 150.0f / 255.0f );

		if( ent->e.renderfx & RF_LIGHTING_ORIGIN )
		{
			VectorSubtract( ent->e.lightingOrigin, ent->e.origin, ent->lightDir );
			VectorNormalize( ent->lightDir );
		}
		else
		{
			VectorCopy( tr.sunDirection, ent->lightDir );
		}

#else
		//% ent->ambientLight[0] = ent->ambientLight[1] = ent->ambientLight[2] = tr.identityLight * 150;
		//% ent->directedLight[0] = ent->directedLight[1] = ent->directedLight[2] = tr.identityLight * 150;
		//% VectorCopy( tr.sunDirection, ent->lightDir );
		ent->ambientLight[ 0 ] = tr.identityLight * ( 64.0f / 255.0f );
		ent->ambientLight[ 1 ] = tr.identityLight * ( 64.0f / 255.0f );
		ent->ambientLight[ 2 ] = tr.identityLight * ( 96.0f / 255.0f );

		ent->directedLight[ 0 ] = tr.identityLight * ( 255.0f / 255.0f );
		ent->directedLight[ 1 ] = tr.identityLight * ( 232.0f / 255.0f );
		ent->directedLight[ 2 ] = tr.identityLight * ( 224.0f / 255.0f );

		VectorSet( ent->lightDir, -1, 1, 1.25 );
		VectorNormalize( ent->lightDir );
#endif
	}

#if 1

	if( ent->e.hilightIntensity )
	{
		// level of intensity was set because the item was looked at
		ent->ambientLight[ 0 ] += tr.identityLight * 0.5f * ent->e.hilightIntensity;
		ent->ambientLight[ 1 ] += tr.identityLight * 0.5f * ent->e.hilightIntensity;
		ent->ambientLight[ 2 ] += tr.identityLight * 0.5f * ent->e.hilightIntensity;
	}
	else if( ( ent->e.renderfx & RF_MINLIGHT ) )  // && VectorLength(ent->ambientLight) <= 0)
	{
		// give everything a minimum light add
		ent->ambientLight[ 0 ] += tr.identityLight * 0.125f;
		ent->ambientLight[ 1 ] += tr.identityLight * 0.125f;
		ent->ambientLight[ 2 ] += tr.identityLight * 0.125f;
	}

#endif

#if 0

	// clamp ambient
	for( i = 0; i < 3; i++ )
	{
		if( ent->ambientLight[ i ] > tr.identityLight )
		{
			ent->ambientLight[ i ] = tr.identityLight;
		}
	}

#endif

#if defined( COMPAT_ET )

	if( ent->e.entityNum < MAX_CLIENTS && ( refdef->rdflags & RDF_SNOOPERVIEW ) )
	{
		VectorSet( ent->ambientLight, 0.96f, 0.96f, 0.96f );  // allow a little room for flicker from directed light
	}

#endif

	// Tr3B: keep it in world space

	// transform the direction to local space
	//% d = VectorLength(ent->directedLight);
	//% VectorScale(ent->lightDir, d, lightDir);
	//% VectorNormalize(lightDir);

	//% ent->lightDir[0] = DotProduct(lightDir, ent->e.axis[0]);
	//% ent->lightDir[1] = DotProduct(lightDir, ent->e.axis[1]);
	//% ent->lightDir[2] = DotProduct(lightDir, ent->e.axis[2]);
}

/*
=================
R_LightForPoint
=================
*/
int R_LightForPoint( vec3_t point, vec3_t ambientLight, vec3_t directedLight, vec3_t lightDir )
{
	trRefEntity_t ent;

	// bk010103 - this segfaults with -nolight maps
	if( tr.world->lightGridData == NULL )
	{
		return qfalse;
	}

	Com_Memset( &ent, 0, sizeof( ent ) );
	VectorCopy( point, ent.e.origin );
	R_SetupEntityLightingGrid( &ent, NULL );
	VectorCopy( ent.ambientLight, ambientLight );
	VectorCopy( ent.directedLight, directedLight );
	VectorCopy( ent.lightDir, lightDir );

	return qtrue;
}

/*
=================
R_SetupLightOrigin
Tr3B - needs finished transformMatrix
=================
*/
void R_SetupLightOrigin( trRefLight_t *light )
{
	vec3_t transformed;

	if( light->l.rlType == RL_DIRECTIONAL )
	{
#if 1

		if( !VectorCompare( light->l.center, vec3_origin ) )
		{
			MatrixTransformPoint( light->transformMatrix, light->l.center, transformed );
			VectorSubtract( transformed, light->l.origin, light->direction );
			VectorNormalize( light->direction );

			VectorMA( light->l.origin, 10000, light->direction, light->origin );
		}
		else
#endif
		{
			vec3_t down = { 0, 0, 1 };

			MatrixTransformPoint( light->transformMatrix, down, transformed );
			VectorSubtract( transformed, light->l.origin, light->direction );
			VectorNormalize( light->direction );

			VectorMA( light->l.origin, 10000, light->direction, light->origin );

			VectorCopy( light->l.origin, light->origin );
		}
	}
	else
	{
		MatrixTransformPoint( light->transformMatrix, light->l.center, light->origin );
	}
}

/*
=================
R_SetupLightLocalBounds
=================
*/
void R_SetupLightLocalBounds( trRefLight_t *light )
{
	switch( light->l.rlType )
	{
		case RL_OMNI:
		case RL_DIRECTIONAL:
			{
				light->localBounds[ 0 ][ 0 ] = -light->l.radius[ 0 ];
				light->localBounds[ 0 ][ 1 ] = -light->l.radius[ 1 ];
				light->localBounds[ 0 ][ 2 ] = -light->l.radius[ 2 ];
				light->localBounds[ 1 ][ 0 ] = light->l.radius[ 0 ];
				light->localBounds[ 1 ][ 1 ] = light->l.radius[ 1 ];
				light->localBounds[ 1 ][ 2 ] = light->l.radius[ 2 ];
				break;
			}

		case RL_PROJ:
			{
				int    j;
				vec3_t farCorners[ 4 ];
				//vec4_t      frustum[6];
				vec4_t *frustum = light->localFrustum;

				ClearBounds( light->localBounds[ 0 ], light->localBounds[ 1 ] );

				// transform frustum from world space to local space

				/*
				for(j = 0; j < 6; j++)
				{
				        VectorCopy(light->frustum[j].normal, frustum[j]);
				        frustum[j][3] = light->frustum[j].dist;

				        MatrixTransformPlane2(light->viewMatrix, frustum[j]);
				}
				*/

				PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 0 ] );
				PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 1 ] );
				PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 2 ] );
				PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 3 ] );

				if( !VectorCompare( light->l.projStart, vec3_origin ) )
				{
					vec3_t nearCorners[ 4 ];

					// calculate the vertices defining the top area
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 0 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 1 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 2 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 3 ] );

					for( j = 0; j < 4; j++ )
					{
						AddPointToBounds( farCorners[ j ], light->localBounds[ 0 ], light->localBounds[ 1 ] );
						AddPointToBounds( nearCorners[ j ], light->localBounds[ 0 ], light->localBounds[ 1 ] );
					}
				}
				else
				{
					vec3_t top;

					PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], top );
					AddPointToBounds( top, light->localBounds[ 0 ], light->localBounds[ 1 ] );

					for( j = 0; j < 4; j++ )
					{
						AddPointToBounds( farCorners[ j ], light->localBounds[ 0 ], light->localBounds[ 1 ] );
					}
				}

				break;
			}

		default:
			break;
	}

	light->sphereRadius = RadiusFromBounds( light->localBounds[ 0 ], light->localBounds[ 1 ] );
}

/*
=================
R_SetupLightWorldBounds
Tr3B - needs finished transformMatrix
=================
*/
void R_SetupLightWorldBounds( trRefLight_t *light )
{
	int    j;
	vec3_t v, transformed;

	ClearBounds( light->worldBounds[ 0 ], light->worldBounds[ 1 ] );

	for( j = 0; j < 8; j++ )
	{
		v[ 0 ] = light->localBounds[ j & 1 ][ 0 ];
		v[ 1 ] = light->localBounds[( j >> 1 ) & 1 ][ 1 ];
		v[ 2 ] = light->localBounds[( j >> 2 ) & 1 ][ 2 ];

		// transform local bounds vertices into world space
		MatrixTransformPoint( light->transformMatrix, v, transformed );

		AddPointToBounds( transformed, light->worldBounds[ 0 ], light->worldBounds[ 1 ] );
	}
}

/*
=================
R_SetupLightView
=================
*/
void R_SetupLightView( trRefLight_t *light )
{
	switch( light->l.rlType )
	{
		case RL_OMNI:
		case RL_PROJ:
		case RL_DIRECTIONAL:
			{
				MatrixAffineInverse( light->transformMatrix, light->viewMatrix );
				break;
			}

			/*
			case RL_PROJ:
			{
			        matrix_t        viewMatrix;

			        MatrixAffineInverse(light->transformMatrix, viewMatrix);

			        // convert from our coordinate system (looking down X)
			        // to OpenGL's coordinate system (looking down -Z)
			        MatrixMultiply(quakeToOpenGLMatrix, viewMatrix, light->viewMatrix);
			        break;
			}
			*/

		default:
			ri.Error( ERR_DROP, "R_SetupLightView: Bad rlType" );
	}
}

/*
=================
R_SetupLightFrustum
=================
*/
void R_SetupLightFrustum( trRefLight_t *light )
{
	switch( light->l.rlType )
	{
		case RL_OMNI:
		case RL_DIRECTIONAL:
			{
				int    i;
				vec3_t planeNormal;
				vec3_t planeOrigin;
				axis_t axis;

				QuatToAxis( light->l.rotation, axis );

				for( i = 0; i < 3; i++ )
				{
					VectorMA( light->l.origin, light->l.radius[ i ], axis[ i ], planeOrigin );
					VectorNegate( axis[ i ], planeNormal );
					VectorNormalize( planeNormal );

					VectorCopy( planeNormal, light->frustum[ i ].normal );
					light->frustum[ i ].dist = DotProduct( planeOrigin, planeNormal );
				}

				for( i = 0; i < 3; i++ )
				{
					VectorMA( light->l.origin, -light->l.radius[ i ], axis[ i ], planeOrigin );
					VectorCopy( axis[ i ], planeNormal );
					VectorNormalize( planeNormal );

					VectorCopy( planeNormal, light->frustum[ i + 3 ].normal );
					light->frustum[ i + 3 ].dist = DotProduct( planeOrigin, planeNormal );
				}

				for( i = 0; i < 6; i++ )
				{
					vec_t length, ilength;

					light->frustum[ i ].type = PLANE_NON_AXIAL;

					// normalize
					length = VectorLength( light->frustum[ i ].normal );

					if( length )
					{
						ilength = 1.0 / length;
						light->frustum[ i ].normal[ 0 ] *= ilength;
						light->frustum[ i ].normal[ 1 ] *= ilength;
						light->frustum[ i ].normal[ 2 ] *= ilength;
						light->frustum[ i ].dist *= ilength;
					}

					SetPlaneSignbits( &light->frustum[ i ] );
				}

				break;
			}

		case RL_PROJ:
			{
				int    i;
				vec4_t worldFrustum[ 6 ];

				// transform local frustum to world space
				for( i = 0; i < 6; i++ )
				{
					MatrixTransformPlane( light->transformMatrix, light->localFrustum[ i ], worldFrustum[ i ] );
				}

				// normalize all frustum planes
				for( i = 0; i < 6; i++ )
				{
					PlaneNormalize( worldFrustum[ i ] );

					VectorCopy( worldFrustum[ i ], light->frustum[ i ].normal );
					light->frustum[ i ].dist = worldFrustum[ i ][ 3 ];

					light->frustum[ i ].type = PLANE_NON_AXIAL;

					SetPlaneSignbits( &light->frustum[ i ] );
				}

				break;
			}

		default:
			break;
	}

	if( light->isStatic )
	{
		int           i, j;
		vec4_t        quadVerts[ 4 ];
		srfVert_t     *verts;
		srfTriangle_t *triangles;

		if( glConfig.smpActive )
		{
			ri.Error( ERR_FATAL, "R_SetupLightFrustum: FIXME SMP" );
		}

		tess.multiDrawPrimitives = 0;
		tess.numIndexes = 0;
		tess.numVertexes = 0;

		switch( light->l.rlType )
		{
			case RL_OMNI:
			case RL_DIRECTIONAL:
				{
					vec3_t worldBounds[ 2 ];

					MatrixTransformPoint( light->transformMatrix, light->localBounds[ 0 ], worldBounds[ 0 ] );
					MatrixTransformPoint( light->transformMatrix, light->localBounds[ 1 ], worldBounds[ 1 ] );

					Tess_AddCube( vec3_origin, worldBounds[ 0 ], worldBounds[ 1 ], colorWhite );

					verts = ri.Hunk_AllocateTempMemory( tess.numVertexes * sizeof( srfVert_t ) );
					triangles = ri.Hunk_AllocateTempMemory( ( tess.numIndexes / 3 ) * sizeof( srfTriangle_t ) );

					for( i = 0; i < tess.numVertexes; i++ )
					{
						VectorCopy( tess.xyz[ i ], verts[ i ].xyz );
					}

					for( i = 0; i < ( tess.numIndexes / 3 ); i++ )
					{
						triangles[ i ].indexes[ 0 ] = tess.indexes[ i * 3 + 0 ];
						triangles[ i ].indexes[ 1 ] = tess.indexes[ i * 3 + 1 ];
						triangles[ i ].indexes[ 2 ] = tess.indexes[ i * 3 + 2 ];
					}

					light->frustumVBO = R_CreateVBO2( "staticLightFrustum_VBO", tess.numVertexes, verts, ATTR_POSITION, VBO_USAGE_STATIC );
					light->frustumIBO = R_CreateIBO2( "staticLightFrustum_IBO", tess.numIndexes / 3, triangles, VBO_USAGE_STATIC );

					ri.Hunk_FreeTempMemory( triangles );
					ri.Hunk_FreeTempMemory( verts );

					light->frustumVerts = tess.numVertexes;
					light->frustumIndexes = tess.numIndexes;
					break;
				}

			case RL_PROJ:
				{
					vec3_t farCorners[ 4 ];
					vec4_t frustum[ 6 ];

					// transform local frustum to world space
					for( i = 0; i < 6; i++ )
					{
						MatrixTransformPlane( light->transformMatrix, light->localFrustum[ i ], frustum[ i ] );
					}

					PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 0 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 1 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 2 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 3 ] );

					if( !VectorCompare( light->l.projStart, vec3_origin ) )
					{
						vec3_t nearCorners[ 4 ];

						// calculate the vertices defining the top area
						PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 0 ] );
						PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 1 ] );
						PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 2 ] );
						PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 3 ] );

						// draw outer surfaces
						for( j = 0; j < 4; j++ )
						{
							Vector4Set( quadVerts[ 3 ], nearCorners[ j ][ 0 ], nearCorners[ j ][ 1 ], nearCorners[ j ][ 2 ], 1 );
							Vector4Set( quadVerts[ 2 ], farCorners[ j ][ 0 ], farCorners[ j ][ 1 ], farCorners[ j ][ 2 ], 1 );
							Vector4Set( quadVerts[ 1 ], farCorners[( j + 1 ) % 4 ][ 0 ], farCorners[( j + 1 ) % 4 ][ 1 ], farCorners[( j + 1 ) % 4 ][ 2 ], 1 );
							Vector4Set( quadVerts[ 0 ], nearCorners[( j + 1 ) % 4 ][ 0 ], nearCorners[( j + 1 ) % 4 ][ 1 ], nearCorners[( j + 1 ) % 4 ][ 2 ], 1 );
							Tess_AddQuadStamp2( quadVerts, colorCyan );
						}

						// draw far cap
						Vector4Set( quadVerts[ 0 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 1 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 2 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 3 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
						Tess_AddQuadStamp2( quadVerts, colorRed );

						// draw near cap
						Vector4Set( quadVerts[ 3 ], nearCorners[ 0 ][ 0 ], nearCorners[ 0 ][ 1 ], nearCorners[ 0 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 2 ], nearCorners[ 1 ][ 0 ], nearCorners[ 1 ][ 1 ], nearCorners[ 1 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 1 ], nearCorners[ 2 ][ 0 ], nearCorners[ 2 ][ 1 ], nearCorners[ 2 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 0 ], nearCorners[ 3 ][ 0 ], nearCorners[ 3 ][ 1 ], nearCorners[ 3 ][ 2 ], 1 );
						Tess_AddQuadStamp2( quadVerts, colorGreen );
					}
					else
					{
						vec3_t top;

						// no light_start, just use the top vertex (doesn't need to be mirrored)
						PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], top );

						// draw pyramid
						for( j = 0; j < 4; j++ )
						{
							VectorCopy( top, tess.xyz[ tess.numVertexes ] );
							Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
							tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
							tess.numVertexes++;

							VectorCopy( farCorners[( j + 1 ) % 4 ], tess.xyz[ tess.numVertexes ] );
							Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
							tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
							tess.numVertexes++;

							VectorCopy( farCorners[ j ], tess.xyz[ tess.numVertexes ] );
							Vector4Copy( colorCyan, tess.colors[ tess.numVertexes ] );
							tess.indexes[ tess.numIndexes++ ] = tess.numVertexes;
							tess.numVertexes++;
						}

						Vector4Set( quadVerts[ 0 ], farCorners[ 0 ][ 0 ], farCorners[ 0 ][ 1 ], farCorners[ 0 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 1 ], farCorners[ 1 ][ 0 ], farCorners[ 1 ][ 1 ], farCorners[ 1 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 2 ], farCorners[ 2 ][ 0 ], farCorners[ 2 ][ 1 ], farCorners[ 2 ][ 2 ], 1 );
						Vector4Set( quadVerts[ 3 ], farCorners[ 3 ][ 0 ], farCorners[ 3 ][ 1 ], farCorners[ 3 ][ 2 ], 1 );
						Tess_AddQuadStamp2( quadVerts, colorRed );
					}

					verts = ri.Hunk_AllocateTempMemory( tess.numVertexes * sizeof( srfVert_t ) );
					triangles = ri.Hunk_AllocateTempMemory( ( tess.numIndexes / 3 ) * sizeof( srfTriangle_t ) );

					for( i = 0; i < tess.numVertexes; i++ )
					{
						VectorCopy( tess.xyz[ i ], verts[ i ].xyz );
					}

					for( i = 0; i < ( tess.numIndexes / 3 ); i++ )
					{
						triangles[ i ].indexes[ 0 ] = tess.indexes[ i * 3 + 0 ];
						triangles[ i ].indexes[ 1 ] = tess.indexes[ i * 3 + 1 ];
						triangles[ i ].indexes[ 2 ] = tess.indexes[ i * 3 + 2 ];
					}

					light->frustumVBO = R_CreateVBO2( "staticLightFrustum_VBO", tess.numVertexes, verts, ATTR_POSITION, VBO_USAGE_STATIC );
					light->frustumIBO = R_CreateIBO2( "staticLightFrustum_IBO", tess.numIndexes / 3, triangles, VBO_USAGE_STATIC );

					ri.Hunk_FreeTempMemory( triangles );
					ri.Hunk_FreeTempMemory( verts );

					light->frustumVerts = tess.numVertexes;
					light->frustumIndexes = tess.numIndexes;
					break;
				}

			default:
				break;
		}

		tess.multiDrawPrimitives = 0;
		tess.numIndexes = 0;
		tess.numVertexes = 0;
	}
}

/*
=================
R_SetupLightProjection
=================
*/
// *INDENT-OFF*
void R_SetupLightProjection( trRefLight_t *light )
{
	switch( light->l.rlType )
	{
		case RL_OMNI:
		case RL_DIRECTIONAL:
			{
				MatrixSetupScale( light->projectionMatrix, 1.0 / light->l.radius[ 0 ], 1.0 / light->l.radius[ 1 ], 1.0 / light->l.radius[ 2 ] );
				break;
			}

		case RL_PROJ:
			{
				int    i;
				float  *proj = light->projectionMatrix;
				vec4_t *frustum = light->localFrustum;
				vec4_t lightProject[ 4 ];
				vec3_t right, up, normal;
				vec3_t start, stop;
				vec3_t falloff;
				float  falloffLen;
				float  rLen;
				float  uLen;
				float  a, b, ofs, dist;
				vec4_t targetGlobal;

				// This transformation remaps the X,Y coordinates from [-1..1] to [0..1],
				// presumably needed because the up/right vectors extend symmetrically
				// either side of the target point.
				//MatrixSetupTranslation(proj, 0.5f, 0.5f, 0);
				//MatrixMultiplyScale(proj, 0.5f, 0.5f, 1);

				rLen = VectorNormalize2( light->l.projRight, right );
				uLen = VectorNormalize2( light->l.projUp, up );

				CrossProduct( up, right, normal );
				VectorNormalize( normal );

				dist = DotProduct( light->l.projTarget, normal );

				if( dist < 0 )
				{
					dist = -dist;
					VectorInverse( normal );
				}

				VectorScale( right, ( 0.5f * dist ) / rLen, right );
				VectorScale( up, - ( 0.5f * dist ) / uLen, up );

				Vector4Set( lightProject[ 0 ], right[ 0 ], right[ 1 ], right[ 2 ], 0 );
				Vector4Set( lightProject[ 1 ], up[ 0 ], up[ 1 ], up[ 2 ], 0 );
				Vector4Set( lightProject[ 2 ], normal[ 0 ], normal[ 1 ], normal[ 2 ], 0 );

				// now offset to center
				VectorCopy( light->l.projTarget, targetGlobal );
				targetGlobal[ 3 ] = 1;
				{
					a = DotProduct4( targetGlobal, lightProject[ 0 ] );
					b = DotProduct4( targetGlobal, lightProject[ 2 ] );
					ofs = 0.5 - a / b;

					Vector4MA( lightProject[ 0 ], ofs, lightProject[ 2 ], lightProject[ 0 ] );
				}
				{
					a = DotProduct4( targetGlobal, lightProject[ 1 ] );
					b = DotProduct4( targetGlobal, lightProject[ 2 ] );
					ofs = 0.5 - a / b;

					Vector4MA( lightProject[ 1 ], ofs, lightProject[ 2 ], lightProject[ 1 ] );
				}

				if( !VectorCompare( light->l.projStart, vec3_origin ) )
				{
					VectorCopy( light->l.projStart, start );
				}
				else
				{
					VectorClear( start );
				}

				if( !VectorCompare( light->l.projEnd, vec3_origin ) )
				{
					VectorCopy( light->l.projEnd, stop );
				}
				else
				{
					VectorCopy( light->l.projTarget, stop );
				}

				// Calculate the falloff vector
				VectorSubtract( stop, start, falloff );
				light->falloffLength = falloffLen = VectorNormalize( falloff );

				if( falloffLen <= 0 )
				{
					falloffLen = 1;
				}

				//FIXME ?
				VectorScale( falloff, 1.0f / falloffLen, falloff );

				//light->falloffLength = 1;

				Vector4Set( lightProject[ 3 ], falloff[ 0 ], falloff[ 1 ], falloff[ 2 ], -DotProduct( start, falloff ) );

				// we want the planes of s=0, s=q, t=0, and t=q
				Vector4Copy( lightProject[ 0 ], frustum[ FRUSTUM_LEFT ] );
				Vector4Copy( lightProject[ 1 ], frustum[ FRUSTUM_BOTTOM ] );

				VectorSubtract( lightProject[ 2 ], lightProject[ 0 ], frustum[ FRUSTUM_RIGHT ] );
				frustum[ FRUSTUM_RIGHT ][ 3 ] = lightProject[ 2 ][ 3 ] - lightProject[ 0 ][ 3 ];

				VectorSubtract( lightProject[ 2 ], lightProject[ 1 ], frustum[ FRUSTUM_TOP ] );
				frustum[ FRUSTUM_TOP ][ 3 ] = lightProject[ 2 ][ 3 ] - lightProject[ 1 ][ 3 ];

				// we want the planes of s=0 and s=1 for front and rear clipping planes
				VectorCopy( lightProject[ 3 ], frustum[ FRUSTUM_NEAR ] );
				frustum[ FRUSTUM_NEAR ][ 3 ] = lightProject[ 3 ][ 3 ];

				VectorNegate( lightProject[ 3 ], frustum[ FRUSTUM_FAR ] );
				frustum[ FRUSTUM_FAR ][ 3 ] = -lightProject[ 3 ][ 3 ] - 1.0f;

#if 0
				ri.Printf( PRINT_ALL, "light_target: (%5.3f, %5.3f, %5.3f)\n", light->l.projTarget[ 0 ], light->l.projTarget[ 1 ], light->l.projTarget[ 2 ] );
				ri.Printf( PRINT_ALL, "light_right: (%5.3f, %5.3f, %5.3f)\n", light->l.projRight[ 0 ], light->l.projRight[ 1 ], light->l.projRight[ 2 ] );
				ri.Printf( PRINT_ALL, "light_up: (%5.3f, %5.3f, %5.3f)\n", light->l.projUp[ 0 ], light->l.projUp[ 1 ], light->l.projUp[ 2 ] );
				ri.Printf( PRINT_ALL, "light_start: (%5.3f, %5.3f, %5.3f)\n", light->l.projStart[ 0 ], light->l.projStart[ 1 ], light->l.projStart[ 2 ] );
				ri.Printf( PRINT_ALL, "light_end: (%5.3f, %5.3f, %5.3f)\n", light->l.projEnd[ 0 ], light->l.projEnd[ 1 ], light->l.projEnd[ 2 ] );

				ri.Printf( PRINT_ALL, "unnormalized frustum:\n" );

				for( i = 0; i < 6; i++ )
				{
					ri.Printf( PRINT_ALL, "(%5.6f, %5.6f, %5.6f, %5.6f)\n", frustum[ i ][ 0 ], frustum[ i ][ 1 ], frustum[ i ][ 2 ], frustum[ i ][ 3 ] );
				}

#endif

				// calculate the new projection matrix from the frustum planes
				MatrixFromPlanes( proj, frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], frustum[ FRUSTUM_FAR ] );

				//MatrixMultiply2(proj, newProjection);

				// scale the falloff texture coordinate so that 0.5 is at the apex and 0.0
				// as at the base of the pyramid.
				// TODO: I don't like hacking the matrix like this, but all attempts to use
				// a transformation seemed to affect too many other things.
				//proj[10] *= 0.5f;

				// normalise all frustum planes
				for( i = 0; i < 6; i++ )
				{
					PlaneNormalize( frustum[ i ] );
				}

#if 0
				ri.Printf( PRINT_ALL, "normalized frustum:\n" );

				for( i = 0; i < 6; i++ )
				{
					ri.Printf( PRINT_ALL, "(%5.3f, %5.3f, %5.3f, %5.3f)\n", light->frustum[ i ].normal[ 0 ], frustum[ i ][ 1 ], frustum[ i ][ 2 ], frustum[ i ][ 3 ] );
				}

#endif
				break;
			}

		default:
			ri.Error( ERR_DROP, "R_SetupLightProjection: Bad rlType" );
	}
}

// *INDENT-ON*

/*
=================
R_AddLightInteraction
=================
*/
qboolean R_AddLightInteraction( trRefLight_t *light, surfaceType_t *surface, shader_t *surfaceShader, byte cubeSideBits,
                                interactionType_t iaType )
{
	int           iaIndex;
	interaction_t *ia;

	// skip all surfaces that don't matter for lighting only pass
	if( surfaceShader )
	{
		if( surfaceShader->isSky || ( !surfaceShader->interactLight && surfaceShader->noShadows ) )
		{
			return qfalse;
		}
	}

	// instead of checking for overflow, we just mask the index
	// so it wraps around
	iaIndex = tr.refdef.numInteractions & INTERACTION_MASK;
	ia = &tr.refdef.interactions[ iaIndex ];
	tr.refdef.numInteractions++;

	light->noSort = iaIndex == 0;

	// connect to interaction grid
	if( !light->firstInteraction )
	{
		light->firstInteraction = ia;
	}

	if( light->lastInteraction )
	{
		light->lastInteraction->next = ia;
	}

	light->lastInteraction = ia;

	// update counters
	light->numInteractions++;

	switch( iaType )
	{
		case IA_SHADOWONLY:
			light->numShadowOnlyInteractions++;
			break;

		case IA_LIGHTONLY:
			light->numLightOnlyInteractions++;
			break;

		default:
			break;
	}

	ia->next = NULL;

	ia->type = iaType;

	ia->light = light;
	ia->entity = tr.currentEntity;
	ia->surface = surface;
	ia->surfaceShader = surfaceShader;

	ia->cubeSideBits = cubeSideBits;

	ia->scissorX = light->scissor.coords[ 0 ];
	ia->scissorY = light->scissor.coords[ 1 ];
	ia->scissorWidth = light->scissor.coords[ 2 ] - light->scissor.coords[ 0 ];
	ia->scissorHeight = light->scissor.coords[ 3 ] - light->scissor.coords[ 1 ];

	/*
	if(r_shadows->integer == SHADOWING_STENCIL && glDepthBoundsEXT)
	{
	        ia->depthNear = light->depthNear;
	        ia->depthFar = light->depthFar;
	        ia->noDepthBoundsTest = light->noDepthBoundsTest;
	}
	*/

	if( glConfig2.occlusionQueryAvailable )
	{
		ia->noOcclusionQueries = light->noOcclusionQueries;
	}

	if( light->isStatic )
	{
		tr.pc.c_slightInteractions++;
	}
	else
	{
		tr.pc.c_dlightInteractions++;
	}

	return qtrue;
}

/*
=================
InteractionCompare
compare function for qsort()
=================
*/
static int InteractionCompare( const void *a, const void *b )
{
#if 1

	// shader first
	if( ( ( interaction_t * ) a )->surfaceShader < ( ( interaction_t * ) b )->surfaceShader )
	{
		return -1;
	}

	else if( ( ( interaction_t * ) a )->surfaceShader > ( ( interaction_t * ) b )->surfaceShader )
	{
		return 1;
	}

#endif

#if 1

	// then entity
	if( ( ( interaction_t * ) a )->entity == &tr.worldEntity && ( ( interaction_t * ) b )->entity != &tr.worldEntity )
	{
		return -1;
	}

	else if( ( ( interaction_t * ) a )->entity != &tr.worldEntity && ( ( interaction_t * ) b )->entity == &tr.worldEntity )
	{
		return 1;
	}

	else if( ( ( interaction_t * ) a )->entity < ( ( interaction_t * ) b )->entity )
	{
		return -1;
	}

	else if( ( ( interaction_t * ) a )->entity > ( ( interaction_t * ) b )->entity )
	{
		return 1;
	}

#endif

	return 0;
}

/*
=================
R_SortInteractions
=================
*/
void R_SortInteractions( trRefLight_t *light )
{
	int           i;
	int           iaFirstIndex;
	interaction_t *iaFirst;
	interaction_t *ia;
	interaction_t *iaLast;

	if( r_noInteractionSort->integer )
	{
		return;
	}

	if( !light->numInteractions || light->noSort )
	{
		return;
	}

	iaFirst = light->firstInteraction;
	iaFirstIndex = light->firstInteraction - tr.refdef.interactions;

	// sort by material etc. for geometry batching in the renderer backend
	qsort( iaFirst, light->numInteractions, sizeof( interaction_t ), InteractionCompare );

	// fix linked list
	iaLast = NULL;

	for( i = 0; i < light->numInteractions; i++ )
	{
		ia = &tr.refdef.interactions[ iaFirstIndex + i ];

		if( iaLast )
		{
			iaLast->next = ia;
		}

		ia->next = NULL;

		iaLast = ia;
	}
}

/*
=================
R_IntersectRayPlane
=================
*/
static void R_IntersectRayPlane( const vec3_t v1, const vec3_t v2, cplane_t *plane, vec3_t res )
{
	vec3_t v;
	float  sect;

	VectorSubtract( v1, v2, v );
	sect = - ( DotProduct( plane->normal, v1 ) - plane->dist ) / DotProduct( plane->normal, v );
	VectorScale( v, sect, v );
	VectorAdd( v1, v, res );
}

/*
=================
R_AddPointToLightScissor
=================
*/
static void R_AddPointToLightScissor( trRefLight_t *light, const vec3_t world )
{
	vec4_t eye, clip, normalized, window;

	R_TransformWorldToClip( world, tr.viewParms.world.viewMatrix, tr.viewParms.projectionMatrix, eye, clip );
	R_TransformClipToWindow( clip, &tr.viewParms, normalized, window );

	if( window[ 0 ] > light->scissor.coords[ 2 ] )
	{
		light->scissor.coords[ 2 ] = ( int ) window[ 0 ];
	}

	if( window[ 0 ] < light->scissor.coords[ 0 ] )
	{
		light->scissor.coords[ 0 ] = ( int ) window[ 0 ];
	}

	if( window[ 1 ] > light->scissor.coords[ 3 ] )
	{
		light->scissor.coords[ 3 ] = ( int ) window[ 1 ];
	}

	if( window[ 1 ] < light->scissor.coords[ 1 ] )
	{
		light->scissor.coords[ 1 ] = ( int ) window[ 1 ];
	}
}

/*
=================
R_AddEdgeToLightScissor
=================
*/
static void R_AddEdgeToLightScissor( trRefLight_t *light, vec3_t local1, vec3_t local2 )
{
	int      i;
	vec3_t   intersect = { 0 };
	vec3_t   world1, world2;
	qboolean side1, side2;
	cplane_t *frust;

	for( i = 0; i < FRUSTUM_PLANES; i++ )
	{
		R_LocalPointToWorld( local1, world1 );
		R_LocalPointToWorld( local2, world2 );

		frust = &tr.viewParms.frustums[ 0 ][ i ];

		// check edge to frustrum plane
		side1 = ( ( DotProduct( frust->normal, world1 ) - frust->dist ) >= 0.0 );
		side2 = ( ( DotProduct( frust->normal, world2 ) - frust->dist ) >= 0.0 );

		if( glConfig2.occlusionQueryAvailable && i == FRUSTUM_NEAR )
		{
			if( !side1 || !side2 )
			{
				light->noOcclusionQueries = qtrue;
			}
		}

		if( !side1 && !side2 )
		{
			continue; // edge behind plane
		}

		if( !side1 || !side2 )
		{
			R_IntersectRayPlane( world1, world2, frust, intersect );
		}

		if( !side1 )
		{
			VectorCopy( intersect, world1 );
		}
		else if( !side2 )
		{
			VectorCopy( intersect, world2 );
		}

		R_AddPointToLightScissor( light, world1 );
		R_AddPointToLightScissor( light, world2 );
	}
}

/*
=================
R_SetLightScissor
Recturns the screen space rectangle taken by the box.
        (Clips the box to the near plane to have correct results even if the box intersects the near plane)
Tr3B - recoded from Tenebrae2
=================
*/
void R_SetupLightScissor( trRefLight_t *light )
{
	vec3_t v1, v2;

	light->scissor.coords[ 0 ] = tr.viewParms.viewportX;
	light->scissor.coords[ 1 ] = tr.viewParms.viewportY;
	light->scissor.coords[ 2 ] = tr.viewParms.viewportX + tr.viewParms.viewportWidth;
	light->scissor.coords[ 3 ] = tr.viewParms.viewportY + tr.viewParms.viewportHeight;

	light->clipsNearPlane = ( BoxOnPlaneSide( light->worldBounds[ 0 ], light->worldBounds[ 1 ], &tr.viewParms.frustums[ 0 ][ FRUSTUM_NEAR ] ) == 3 );

	if( glConfig2.occlusionQueryAvailable )
	{
		light->noOcclusionQueries = qfalse;
	}

	// check if the light volume clips agains the near plane
	if( r_noLightScissors->integer || light->clipsNearPlane )
	{
		if( glConfig2.occlusionQueryAvailable )
		{
			light->noOcclusionQueries = qtrue;
		}

		return;
	}

	if( !r_dynamicBspOcclusionCulling->integer )
	{
		// don't calculate the light scissors because there are up to 500 realtime lights in the view frustum
		// that were not killed by the PVS
		return;
	}

	// transform local light corners to world space -> eye space -> clip space -> window space
	// and extend the light scissor's mins maxs by resulting window coords
	light->scissor.coords[ 0 ] = 100000000;
	light->scissor.coords[ 1 ] = 100000000;
	light->scissor.coords[ 2 ] = -100000000;
	light->scissor.coords[ 3 ] = -100000000;

	switch( light->l.rlType )
	{
		case RL_OMNI:
			{
				// top plane
				VectorSet( v1, light->localBounds[ 1 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 0 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );

				VectorSet( v1, light->localBounds[ 1 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 1 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );

				VectorSet( v1, light->localBounds[ 0 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 0 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );

				VectorSet( v1, light->localBounds[ 0 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 1 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );

				// bottom plane
				VectorSet( v1, light->localBounds[ 1 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 0 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );

				VectorSet( v1, light->localBounds[ 1 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 1 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );

				VectorSet( v1, light->localBounds[ 0 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 0 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );

				VectorSet( v1, light->localBounds[ 0 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 1 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );

				// sides
				VectorSet( v1, light->localBounds[ 0 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 0 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );

				VectorSet( v1, light->localBounds[ 1 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 1 ][ 0 ], light->localBounds[ 1 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );

				VectorSet( v1, light->localBounds[ 0 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 0 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );

				VectorSet( v1, light->localBounds[ 1 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 0 ][ 2 ] );
				VectorSet( v2, light->localBounds[ 1 ][ 0 ], light->localBounds[ 0 ][ 1 ], light->localBounds[ 1 ][ 2 ] );
				R_AddEdgeToLightScissor( light, v1, v2 );
				break;
			}

		case RL_PROJ:
			{
				int    j;
				vec3_t farCorners[ 4 ];
				vec4_t *frustum = light->localFrustum;

				PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 0 ] );
				PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_FAR ], farCorners[ 1 ] );
				PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 2 ] );
				PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_FAR ], farCorners[ 3 ] );
#if 1

				if( !VectorCompare( light->l.projStart, vec3_origin ) )
				{
					vec3_t nearCorners[ 4 ];

					// calculate the vertices defining the top area
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 0 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], frustum[ FRUSTUM_NEAR ], nearCorners[ 1 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 2 ] );
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_BOTTOM ], frustum[ FRUSTUM_NEAR ], nearCorners[ 3 ] );

					for( j = 0; j < 4; j++ )
					{
						// outer quad
						R_AddEdgeToLightScissor( light, nearCorners[ j ], farCorners[ j ] );
						R_AddEdgeToLightScissor( light, farCorners[ j ], farCorners[( j + 1 ) % 4 ] );
						R_AddEdgeToLightScissor( light, farCorners[( j + 1 ) % 4 ], nearCorners[( j + 1 ) % 4 ] );
						R_AddEdgeToLightScissor( light, nearCorners[( j + 1 ) % 4 ],  nearCorners[ j ] );

						// far cap
						R_AddEdgeToLightScissor( light, farCorners[ j ], farCorners[( j + 1 ) % 4 ] );

						// near cap
						R_AddEdgeToLightScissor( light, nearCorners[ j ], nearCorners[( j + 1 ) % 4 ] );
					}
				}
				else
#endif
				{
					vec3_t top;

					// no light_start, just use the top vertex (doesn't need to be mirrored)
					PlanesGetIntersectionPoint( frustum[ FRUSTUM_LEFT ], frustum[ FRUSTUM_RIGHT ], frustum[ FRUSTUM_TOP ], top );

					for( j = 0; j < 4; j++ )
					{
						R_AddEdgeToLightScissor( light, farCorners[ j ], farCorners[( j + 1 ) % 4 ] );
						R_AddEdgeToLightScissor( light, top, farCorners[ j ] );
					}
				}

				break;
			}

		default:
			break;
	}

	Q_clamp( light->scissor.coords[ 0 ], tr.viewParms.viewportX, tr.viewParms.viewportX + tr.viewParms.viewportWidth );
	Q_clamp( light->scissor.coords[ 2 ], tr.viewParms.viewportX, tr.viewParms.viewportX + tr.viewParms.viewportWidth );

	Q_clamp( light->scissor.coords[ 1 ], tr.viewParms.viewportY, tr.viewParms.viewportY + tr.viewParms.viewportHeight );
	Q_clamp( light->scissor.coords[ 3 ], tr.viewParms.viewportY, tr.viewParms.viewportY + tr.viewParms.viewportHeight );
}

/*
=================
R_SetupLightDepthBounds
=================
*/
void R_SetupLightDepthBounds( trRefLight_t *light )
{
#if 0
	int    i, j;
	vec3_t v, world;
	vec4_t eye, clip, normalized, window;
	float  depthMin, depthMax;

	if( r_shadows->integer == SHADOWING_STENCIL && glDepthBoundsEXT )
	{
		tr.pc.c_depthBoundsTestsRejected++;

		depthMin = 1.0;
		depthMax = 0.0;

		for( j = 0; j < 8; j++ )
		{
			v[ 0 ] = light->localBounds[ j & 1 ][ 0 ];
			v[ 1 ] = light->localBounds[( j >> 1 ) & 1 ][ 1 ];
			v[ 2 ] = light->localBounds[( j >> 2 ) & 1 ][ 2 ];

			// transform local bounds vertices into world space
			MatrixTransformPoint( light->transformMatrix, v, world );
			R_TransformWorldToClip( world, tr.viewParms.world.viewMatrix, tr.viewParms.projectionMatrix, eye, clip );

			//R_TransformModelToClip(v, tr.or.modelViewMatrix, tr.viewParms.projectionMatrix, eye, clip);

			// check to see if the point is completely off screen
			for( i = 0; i < 3; i++ )
			{
				if( clip[ i ] >= clip[ 3 ] || clip[ i ] <= -clip[ 3 ] )
				{
					light->noDepthBoundsTest = qtrue;
					return;
				}
			}

			R_TransformClipToWindow( clip, &tr.viewParms, normalized, window );

			if( window[ 0 ] < 0 || window[ 0 ] >= tr.viewParms.viewportWidth
			    || window[ 1 ] < 0 || window[ 1 ] >= tr.viewParms.viewportHeight )
			{
				// shouldn't happen, since we check the clip[] above, except for FP rounding
				light->noDepthBoundsTest = qtrue;
				return;
			}

			depthMin = min( normalized[ 2 ], depthMin );
			depthMax = max( normalized[ 2 ], depthMax );
		}

		if( depthMin > depthMax )
		{
			// light behind near plane or clipped
			light->noDepthBoundsTest = qtrue;
		}
		else
		{
			light->noDepthBoundsTest = qfalse;
			light->depthNear = depthMin;
			light->depthFar = depthMax;

			tr.pc.c_depthBoundsTestsRejected--;
			tr.pc.c_depthBoundsTests++;
		}
	}

#endif
}

/*
=============
R_CalcLightCubeSideBits
=============
*/
// *INDENT-OFF*
byte R_CalcLightCubeSideBits( trRefLight_t *light, vec3_t worldBounds[ 2 ] )
{
	int        i;
	int        cubeSide;
	byte       cubeSideBits;
	float      xMin, xMax, yMin, yMax;
	float      width, height, depth;
	float      zNear, zFar;
	float      fovX, fovY;
	float      *proj;
	vec3_t     angles;
	matrix_t   tmpMatrix, rotationMatrix, transformMatrix, viewMatrix, projectionMatrix, viewProjectionMatrix;
	frustum_t  frustum;
	cplane_t   *clipPlane;
	int        r;
	qboolean   anyClip;
	qboolean   culled;

#if 0
	static int count = 0;
	cubeSideBits = 0;

	for( cubeSide = 0; cubeSide < 6; cubeSide++ )
	{
		if( count % 2 == 0 )
		{
			cubeSideBits |= ( 1 << cubeSide );
		}
	}

	return cubeSideBits;
#endif

	if( light->l.rlType != RL_OMNI || r_shadows->integer < SHADOWING_ESM16 || r_noShadowPyramids->integer )
	{
		return CUBESIDE_CLIPALL;
	}

	cubeSideBits = 0;

	for( cubeSide = 0; cubeSide < 6; cubeSide++ )
	{
		switch( cubeSide )
		{
			case 0:
				{
					// view parameters
					VectorSet( angles, 0, 0, 0 );
					break;
				}

			case 1:
				{
					VectorSet( angles, 0, 180, 0 );
					break;
				}

			case 2:
				{
					VectorSet( angles, 0, 90, 0 );
					break;
				}

			case 3:
				{
					VectorSet( angles, 0, 270, 0 );
					break;
				}

			case 4:
				{
					VectorSet( angles, -90, 0, 0 );
					break;
				}

			case 5:
				{
					VectorSet( angles, 90, 0, 0 );
					break;
				}

			default:
				{
					// shut up compiler
					VectorSet( angles, 0, 0, 0 );
					break;
				}
		}

		// Quake -> OpenGL view matrix from light perspective
		MatrixFromAngles( rotationMatrix, angles[ PITCH ], angles[ YAW ], angles[ ROLL ] );
		MatrixSetupTransformFromRotation( transformMatrix, rotationMatrix, light->origin );
		MatrixAffineInverse( transformMatrix, tmpMatrix );

		// convert from our coordinate system (looking down X)
		// to OpenGL's coordinate system (looking down -Z)
		MatrixMultiply( quakeToOpenGLMatrix, tmpMatrix, viewMatrix );

		// OpenGL projection matrix
		fovX = 90;
		fovY = 90; //R_CalcFov(fovX, shadowMapResolutions[light->shadowLOD], shadowMapResolutions[light->shadowLOD]);

		zNear = 1.0;
		zFar = light->sphereRadius;

		xMax = zNear * tan( fovX * M_PI / 360.0f );
		xMin = -xMax;

		yMax = zNear * tan( fovY * M_PI / 360.0f );
		yMin = -yMax;

		width = xMax - xMin;
		height = yMax - yMin;
		depth = zFar - zNear;

		proj = projectionMatrix;
		proj[ 0 ] = ( 2 * zNear ) / width;
		proj[ 4 ] = 0;
		proj[ 8 ] = ( xMax + xMin ) / width;
		proj[ 12 ] = 0;
		proj[ 1 ] = 0;
		proj[ 5 ] = ( 2 * zNear ) / height;
		proj[ 9 ] = ( yMax + yMin ) / height;
		proj[ 13 ] = 0;
		proj[ 2 ] = 0;
		proj[ 6 ] = 0;
		proj[ 10 ] = - ( zFar + zNear ) / depth;
		proj[ 14 ] = - ( 2 * zFar * zNear ) / depth;
		proj[ 3 ] = 0;
		proj[ 7 ] = 0;
		proj[ 11 ] = -1;
		proj[ 15 ] = 0;

		// calculate frustum planes using the modelview projection matrix
		MatrixMultiply( projectionMatrix, viewMatrix, viewProjectionMatrix );
		R_SetupFrustum2( frustum, viewProjectionMatrix );

		// use the frustum planes to cut off shadowmaps beyond the light volume
		anyClip = qfalse;
		culled = qfalse;

		for( i = 0; i < 5; i++ )
		{
			clipPlane = &frustum[ i ];

			r = BoxOnPlaneSide( worldBounds[ 0 ], worldBounds[ 1 ], clipPlane );

			if( r == 2 )
			{
				culled = qtrue;
				break;
			}

			if( r == 3 )
			{
				anyClip = qtrue;
			}
		}

		if( !culled )
		{
			if( !anyClip )
			{
				// completely inside frustum
				tr.pc.c_pyramid_cull_ent_in++;
			}
			else
			{
				// partially clipped
				tr.pc.c_pyramid_cull_ent_clip++;
			}

			cubeSideBits |= ( 1 << cubeSide );
		}
		else
		{
			// completely outside frustum
			tr.pc.c_pyramid_cull_ent_out++;
		}
	}

	tr.pc.c_pyramidTests++;

	return cubeSideBits;
}

// *INDENT-ON*

/*
=================
R_SetupLightLOD
=================
*/
void R_SetupLightLOD( trRefLight_t *light )
{
	float radius;
	float flod, lodscale;
	float projectedRadius;
	int   lod;
	int   numLods;

	if( light->l.noShadows )
	{
		light->shadowLOD = -1;
		return;
	}

	numLods = 5;

	// compute projected bounding sphere
	// and use that as a criteria for selecting LOD
	radius = light->sphereRadius;

	if( ( projectedRadius = R_ProjectRadius( radius, light->l.origin ) ) != 0 )
	{
		lodscale = r_shadowLodScale->value;

		if( lodscale > 20 )
		{
			lodscale = 20;
		}

		flod = 1.0f - projectedRadius * lodscale;
	}
	else
	{
		// object intersects near view plane, e.g. view weapon
		flod = 0;
	}

	flod *= numLods;
	lod = XreaL_Q_ftol( flod );

	if( lod < 0 )
	{
		lod = 0;
	}
	else if( lod >= numLods )
	{
		//lod = numLods - 1;
	}

	lod += r_shadowLodBias->integer;

	if( lod < 0 )
	{
		lod = 0;
	}

	if( lod >= numLods )
	{
		// don't draw any shadow
		lod = -1;

		//lod = numLods - 1;
	}

	// never give ultra quality for point lights
	if( lod == 0 && light->l.rlType == RL_OMNI )
	{
		lod = 1;
	}

	light->shadowLOD = lod;
}

/*
=================
R_SetupLightShader
=================
*/
void R_SetupLightShader( trRefLight_t *light )
{
	if( !light->l.attenuationShader )
	{
		if( light->isStatic )
		{
			switch( light->l.rlType )
			{
				default:
				case RL_OMNI:
					light->shader = tr.defaultPointLightShader;
					break;

				case RL_PROJ:
					light->shader = tr.defaultProjectedLightShader;
					break;
			}
		}
		else
		{
			switch( light->l.rlType )
			{
				default:
				case RL_OMNI:
					light->shader = tr.defaultDynamicLightShader;
					break;

				case RL_PROJ:
					light->shader = tr.defaultProjectedLightShader;
					break;
			}
		}
	}
	else
	{
		light->shader = R_GetShaderByHandle( light->l.attenuationShader );
	}
}

/*
===============
R_ComputeFinalAttenuation
===============
*/
void R_ComputeFinalAttenuation( shaderStage_t *pStage, trRefLight_t *light )
{
	matrix_t matrix;

	GLimp_LogComment( "--- R_ComputeFinalAttenuation ---\n" );

	RB_CalcTexMatrix( &pStage->bundle[ TB_COLORMAP ], matrix );

	MatrixMultiply( matrix, light->attenuationMatrix, light->attenuationMatrix2 );
}

/*
=================
R_CullLightPoint

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
int R_CullLightPoint( trRefLight_t *light, const vec3_t p )
{
	int      i;
	cplane_t *frust;
	float    dist;

	// check against frustum planes
	for( i = 0; i < 6; i++ )
	{
		frust = &light->frustum[ i ];

		dist = DotProduct( p, frust->normal ) - frust->dist;

		if( dist < 0 )
		{
			// completely outside frustum
			return CULL_OUT;
		}
	}

	// completely inside frustum
	return CULL_IN;
}

/*
=================
R_CullLightTriangle

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
int R_CullLightTriangle( trRefLight_t *light, vec3_t verts[ 3 ] )
{
	int    i;
	vec3_t worldBounds[ 2 ];

	if( r_nocull->integer )
	{
		return CULL_CLIP;
	}

	// calc AABB of the triangle
	ClearBounds( worldBounds[ 0 ], worldBounds[ 1 ] );

	for( i = 0; i < 3; i++ )
	{
		AddPointToBounds( verts[ i ], worldBounds[ 0 ], worldBounds[ 1 ] );
	}

	return R_CullLightWorldBounds( light, worldBounds );
}

/*
=================
R_CullLightTriangle

Returns CULL_IN, CULL_CLIP, or CULL_OUT
=================
*/
int R_CullLightWorldBounds( trRefLight_t *light, vec3_t worldBounds[ 2 ] )
{
	int      i;
	cplane_t *frust;
	qboolean anyClip;
	int      r;

	if( r_nocull->integer )
	{
		return CULL_CLIP;
	}

	// check against frustum planes
	anyClip = qfalse;

	for( i = 0; i < 6; i++ )
	{
		frust = &light->frustum[ i ];

		r = BoxOnPlaneSide( worldBounds[ 0 ], worldBounds[ 1 ], frust );

		if( r == 2 )
		{
			// completely outside frustum
			return CULL_OUT;
		}

		if( r == 3 )
		{
			anyClip = qtrue;
		}
	}

	if( !anyClip )
	{
		// completely inside frustum
		return CULL_IN;
	}

	// partially clipped
	return CULL_CLIP;
}
