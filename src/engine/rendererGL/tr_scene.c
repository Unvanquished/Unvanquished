/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2008 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_scene.c
#include "tr_local.h"

static int r_firstSceneDrawSurf;
static int r_firstSceneInteraction;

static int r_numLights;
static int r_firstSceneLight;

static int r_numEntities;
static int r_firstSceneEntity;

static int r_numPolys;
static int r_firstScenePoly;

int        r_numPolyVerts;

int        r_firstScenePolybuffer;
int        r_numPolybuffers;

// ydnar: decals
int        r_firstSceneDecalProjector;
int        r_numDecalProjectors;
int        r_firstSceneDecal;
int        r_numDecals;

int r_numVisTests;
int r_firstSceneVisTest;

/*
====================
R_ToggleSmpFrame
====================
*/
void R_ToggleSmpFrame( void )
{
	if ( r_smp->integer )
	{
		// use the other buffers next frame, because another CPU
		// may still be rendering into the current ones
		tr.smpFrame ^= 1;
	}
	else
	{
		tr.smpFrame = 0;
	}

	backEndData[ tr.smpFrame ]->commands.used = 0;

	r_firstSceneDrawSurf = 0;
	r_firstSceneInteraction = 0;

	r_numLights = 0;
	r_firstSceneLight = 0;

	r_numEntities = 0;
	r_firstSceneEntity = 0;

	r_numPolys = 0;
	r_firstScenePoly = 0;

	r_numPolyVerts = 0;

	r_numPolybuffers = 0;
	r_firstScenePolybuffer = 0;

	// ydnar: decals
	r_numDecalProjectors = 0;
	r_firstSceneDecalProjector = 0;
	r_numDecals = 0;
	r_firstSceneDecal = 0;

	r_numVisTests = 0;
	r_firstSceneVisTest = 0;
}

/*
====================
RE_ClearScene
====================
*/
void RE_ClearScene( void )
{
	r_firstSceneLight = r_numLights;
	r_firstSceneEntity = r_numEntities;
	r_firstScenePoly = r_numPolys;
	r_firstSceneVisTest = r_numVisTests;
}

/*
===========================================================================

DISCRETE POLYS

===========================================================================
*/

/*
=====================
R_AddPolygonSurfaces

Adds all the scene's polys into this view's drawsurf list
=====================
*/
void R_AddPolygonSurfaces( void )
{
	int       i;
	shader_t  *sh;
	srfPoly_t *poly;

	if ( !r_drawpolies->integer )
	{
		return;
	}

	tr.currentEntity = &tr.worldEntity;

	for ( i = 0, poly = tr.refdef.polys; i < tr.refdef.numPolys; i++, poly++ )
	{
		sh = R_GetShaderByHandle( poly->hShader );
		R_AddDrawSurf( ( void * ) poly, sh, -1, poly->fogIndex );
	}
}

/*
=====================
R_AddPolygonSurfaces

Adds all the scene's polys into this view's drawsurf list
=====================
*/
void R_AddPolygonBufferSurfaces( void )
{
	int             i;
	shader_t        *sh;
	srfPolyBuffer_t *polybuffer;

	tr.currentEntity = &tr.worldEntity;

	for ( i = 0, polybuffer = tr.refdef.polybuffers; i < tr.refdef.numPolybuffers; i++, polybuffer++ )
	{
		sh = R_GetShaderByHandle( polybuffer->pPolyBuffer->shader );

		//R_AddDrawSurf((void *)polybuffer, sh, polybuffer->fogIndex, 0, 0);
		R_AddDrawSurf( ( void * ) polybuffer, sh, -1, polybuffer->fogIndex );
	}
}

/*
=====================
R_AddPolysToScene
=====================
*/
static void R_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys )
{
	srfPoly_t *poly;
	int       i, j;
	int       fogIndex;
	fog_t     *fog;
	vec3_t    bounds[ 2 ];

	if ( !tr.registered )
	{
		return;
	}

	if ( !r_drawpolies->integer )
	{
		return;
	}

	if ( !hShader )
	{
		ri.Printf( PRINT_DEVELOPER, "WARNING: RE_AddPolyToScene: NULL poly shader\n" );
		return;
	}

	for ( j = 0; j < numPolys; j++ )
	{
		if ( r_numPolyVerts + numVerts >= r_maxPolyVerts->integer || r_numPolys >= r_maxPolys->integer )
		{
			/*
			   NOTE TTimo this was initially a PRINT_WARNING
			   but it happens a lot with high fighting scenes and particles
			   since we don't plan on changing the const and making for room for those effects
			   simply cut this message to developer only
			 */
			ri.Printf( PRINT_DEVELOPER, "WARNING: RE_AddPolyToScene: r_max_polys or r_max_polyverts reached\n" );
			return;
		}

		poly = &backEndData[ tr.smpFrame ]->polys[ r_numPolys ];
		poly->surfaceType = SF_POLY;
		poly->hShader = hShader;
		poly->numVerts = numVerts;
		poly->verts = &backEndData[ tr.smpFrame ]->polyVerts[ r_numPolyVerts ];

		Com_Memcpy( poly->verts, &verts[ numVerts * j ], numVerts * sizeof( *verts ) );

		// done.
		r_numPolys++;
		r_numPolyVerts += numVerts;

		// if no world is loaded
		if ( tr.world == NULL )
		{
			fogIndex = 0;
		}
		// see if it is in a fog volume
		else if ( tr.world->numFogs == 1 )
		{
			fogIndex = 0;
		}
		else
		{
			// find which fog volume the poly is in
			VectorCopy( poly->verts[ 0 ].xyz, bounds[ 0 ] );
			VectorCopy( poly->verts[ 0 ].xyz, bounds[ 1 ] );

			for ( i = 1; i < poly->numVerts; i++ )
			{
				AddPointToBounds( poly->verts[ i ].xyz, bounds[ 0 ], bounds[ 1 ] );
			}

			for ( fogIndex = 1; fogIndex < tr.world->numFogs; fogIndex++ )
			{
				fog = &tr.world->fogs[ fogIndex ];

				if ( BoundsIntersect( bounds[ 0 ], bounds[ 1 ], fog->bounds[ 0 ], fog->bounds[ 1 ] ) )
				{
					break;
				}
			}

			if ( fogIndex == tr.world->numFogs )
			{
				fogIndex = 0;
			}
		}

		poly->fogIndex = fogIndex;
	}
}

/*
=====================
RE_AddPolyToScene
=====================
*/
void RE_AddPolyToSceneQ3A( qhandle_t hShader, int numVerts, const polyVert_t *verts, int num )
{
	R_AddPolysToScene( hShader, numVerts, verts, num );
}

void RE_AddPolyToSceneET( qhandle_t hShader, int numVerts, const polyVert_t *verts )
{
	R_AddPolysToScene( hShader, numVerts, verts, 1 );
}

/*
=====================
RE_AddPolysToScene
=====================
*/
void RE_AddPolysToScene( qhandle_t hShader, int numVerts, const polyVert_t *verts, int numPolys )
{
	R_AddPolysToScene( hShader, numVerts, verts, numPolys );
}

/*
=====================
RE_AddPolyBufferToScene
=====================
*/
void RE_AddPolyBufferToScene( polyBuffer_t *pPolyBuffer )
{
	srfPolyBuffer_t *pPolySurf;
	int             fogIndex;
	fog_t           *fog;
	vec3_t          bounds[ 2 ];
	int             i;

	if ( !r_drawpolies->integer )
	{
		return;
	}

	if ( r_numPolybuffers >= r_maxPolyVerts->integer )
	{
		return;
	}

	pPolySurf = &backEndData[ tr.smpFrame ]->polybuffers[ r_numPolybuffers ];
	r_numPolybuffers++;

	pPolySurf->surfaceType = SF_POLYBUFFER;
	pPolySurf->pPolyBuffer = pPolyBuffer;

	VectorCopy( pPolyBuffer->xyz[ 0 ], bounds[ 0 ] );
	VectorCopy( pPolyBuffer->xyz[ 0 ], bounds[ 1 ] );

	for ( i = 1; i < pPolyBuffer->numVerts; i++ )
	{
		AddPointToBounds( pPolyBuffer->xyz[ i ], bounds[ 0 ], bounds[ 1 ] );
	}

	for ( fogIndex = 1; fogIndex < tr.world->numFogs; fogIndex++ )
	{
		fog = &tr.world->fogs[ fogIndex ];

		if ( BoundsIntersect( bounds[ 0 ], bounds[ 1 ], fog->bounds[ 0 ], fog->bounds[ 1 ] ) )
		{
			break;
		}
	}

	if ( fogIndex == tr.world->numFogs )
	{
		fogIndex = 0;
	}

	pPolySurf->fogIndex = fogIndex;
}

//=================================================================================

/*
=====================
RE_AddRefEntityToScene
=====================
*/
void RE_AddRefEntityToScene( const refEntity_t *ent )
{
	if ( !tr.registered )
	{
		return;
	}

	// Tr3B: fixed was ENTITYNUM_WORLD
	if ( r_numEntities >= MAX_REF_ENTITIES )
	{
		return;
	}

	if ( ( unsigned ) ent->reType >= RT_MAX_REF_ENTITY_TYPE )
	{
		ri.Error( ERR_DROP, "RE_AddRefEntityToScene: bad reType %i", ent->reType );
	}

	Com_Memcpy( &backEndData[ tr.smpFrame ]->entities[ r_numEntities ].e, ent, sizeof( refEntity_t ) );
	//backEndData[tr.smpFrame]->entities[r_numentities].e = *ent;
	backEndData[ tr.smpFrame ]->entities[ r_numEntities ].lightingCalculated = qfalse;

	r_numEntities++;
}

/*
=====================
RE_AddRefLightToScene
=====================
*/
void RE_AddRefLightToScene( const refLight_t *l )
{
	trRefLight_t *light;

	if ( !tr.registered )
	{
		return;
	}

	if ( r_numLights >= MAX_REF_LIGHTS )
	{
		return;
	}

	if ( l->radius[ 0 ] <= 0 && !VectorLength( l->radius ) && !VectorLength( l->projTarget ) )
	{
		return;
	}

	if ( ( unsigned ) l->rlType >= RL_MAX_REF_LIGHT_TYPE )
	{
		ri.Error( ERR_DROP, "RE_AddRefLightToScene: bad rlType %i", l->rlType );
	}

	light = &backEndData[ tr.smpFrame ]->lights[ r_numLights++ ];
	Com_Memcpy( &light->l, l, sizeof( light->l ) );

	light->isStatic = qfalse;
	light->additive = qtrue;

	if ( light->l.scale <= 0 )
	{
		light->l.scale = r_lightScale->value;
	}

	if ( !HDR_ENABLED() )
	{
		if ( light->l.scale >= r_lightScale->value )
		{
			light->l.scale = r_lightScale->value;
		}
	}

	if ( !r_dynamicLightCastShadows->integer && !light->l.inverseShadows )
	{
		light->l.noShadows = qtrue;
	}
}

/*
=====================
R_AddWorldLightsToScene
=====================
*/
static void R_AddWorldLightsToScene( void )
{
	int          i;
	trRefLight_t *light;

	if ( !tr.registered )
	{
		return;
	}

	if ( tr.refdef.rdflags & RDF_NOWORLDMODEL )
	{
		return;
	}

	for ( i = 0; i < tr.world->numLights; i++ )
	{
		light = tr.currentLight = &tr.world->lights[ i ];

		if ( r_numLights >= MAX_REF_LIGHTS )
		{
			return;
		}

		/*
		   if(light->radius[0] <= 0 && !VectorLength(light->radius) && light->distance <= 0)
		   {
		   continue;
		   }
		 */

		if ( !light->firstInteractionCache )
		{
			// this light has no interactions precached
			continue;
		}

		Com_Memcpy( &backEndData[ tr.smpFrame ]->lights[ r_numLights ], light, sizeof( trRefLight_t ) );
		r_numLights++;
	}
}

/*
=====================
RE_AddDynamicLightToScene

ydnar: modified dlight system to support separate radius and intensity
=====================
*/
void RE_AddDynamicLightToSceneET( const vec3_t org, float radius, float intensity, float r, float g, float b, qhandle_t hShader, int flags )
{
	trRefLight_t *light;

	if ( !tr.registered )
	{
		return;
	}

	if ( r_numLights >= MAX_REF_LIGHTS )
	{
		return;
	}

	if ( intensity <= 0 || radius <= 0 )
	{
		return;
	}

	light = &backEndData[ tr.smpFrame ]->lights[ r_numLights++ ];

	light->l.rlType = RL_OMNI;
//  light->l.lightfx = 0;
	VectorCopy( org, light->l.origin );

	QuatClear( light->l.rotation );
	VectorClear( light->l.center );

	// HACK: this will tell the renderer backend to use tr.defaultLightShader
#if 0
	dl->shader = R_GetShaderByHandle( hShader );

	if ( dl->shader == tr.defaultShader )
	{
		dl->shader = NULL;
	}

#endif
	light->l.attenuationShader = 0;

	light->l.radius[ 0 ] = radius;
	light->l.radius[ 1 ] = radius;
	light->l.radius[ 2 ] = radius;

	light->l.color[ 0 ] = r;
	light->l.color[ 1 ] = g;
	light->l.color[ 2 ] = b;

	light->l.noShadows = r_dynamicLightCastShadows->integer ? qfalse : qtrue;
	light->l.inverseShadows = qfalse;

	light->isStatic = qfalse;
	light->additive = qtrue;

	light->l.scale = intensity;
#if 0

	if ( light->l.scale <= r_lightScale->value )
	{
		light->l.scale = r_lightScale->value;
	}

#endif
}

void RE_AddDynamicLightToSceneQ3A( const vec3_t org, float radius, float r, float g, float b )
{
	RE_AddDynamicLightToSceneET( org, radius, r_lightScale->value, r, g, b, 0, 0 );
}

/*
==============
RE_AddCoronaToScene

RB: TODO
==============
*/
void RE_AddCoronaToScene( const vec3_t org, float r, float g, float b, float scale, int id, qboolean visible )
{
#if 0
	corona_t *cor;

	if ( !tr.registered )
	{
		return;
	}

	if ( r_numcoronas >= MAX_CORONAS )
	{
		return;
	}

	cor = &backEndData[ tr.smpFrame ]->coronas[ r_numcoronas++ ];
	VectorCopy( org, cor->origin );
	cor->color[ 0 ] = r;
	cor->color[ 1 ] = g;
	cor->color[ 2 ] = b;
	cor->scale = scale;
	cor->id = id;
	cor->visible = visible;
#endif
}

/*
@@@@@@@@@@@@@@@@@@@@@
RE_RenderScene

Draw a 3D view into a part of the window, then return
to 2D drawing.

Rendering a scene may require multiple views to be rendered
to handle mirrors,
@@@@@@@@@@@@@@@@@@@@@
*/
void RE_RenderScene( const refdef_t *fd )
{
	viewParms_t parms;
	int         startTime;

	if ( !tr.registered )
	{
		return;
	}

	GLimp_LogComment( "====== RE_RenderScene =====\n" );

	if ( r_norefresh->integer )
	{
		return;
	}

	startTime = ri.Milliseconds();

	if ( !tr.world && !( fd->rdflags & RDF_NOWORLDMODEL ) )
	{
		ri.Error( ERR_DROP, "R_RenderScene: NULL worldmodel" );
	}

	Com_Memcpy( tr.refdef.text, fd->text, sizeof( tr.refdef.text ) );

	tr.refdef.x = fd->x;
	tr.refdef.y = fd->y;
	tr.refdef.width = fd->width;
	tr.refdef.height = fd->height;
	tr.refdef.fov_x = fd->fov_x;
	tr.refdef.fov_y = fd->fov_y;

	VectorCopy( fd->vieworg, tr.refdef.vieworg );
	VectorCopy( fd->viewaxis[ 0 ], tr.refdef.viewaxis[ 0 ] );
	VectorCopy( fd->viewaxis[ 1 ], tr.refdef.viewaxis[ 1 ] );
	VectorCopy( fd->viewaxis[ 2 ], tr.refdef.viewaxis[ 2 ] );
	VectorCopy( fd->blurVec, tr.refdef.blurVec );

	tr.refdef.time = fd->time;
	tr.refdef.rdflags = fd->rdflags;

	/*
	if(fd->rdflags & RDF_SKYBOXPORTAL)
	{
	        ri.Printf(PRINT_ALL, "skyboxportal = 1\n");
	}
	*/

	// copy the areamask data over and note if it has changed, which
	// will force a reset of the visible leafs even if the view hasn't moved
	tr.refdef.areamaskModified = qfalse;

	if ( !( tr.refdef.rdflags & RDF_NOWORLDMODEL ) && !( ( tr.refdef.rdflags & RDF_SKYBOXPORTAL ) && tr.world->numSkyNodes > 0 ) )
	{
		int areaDiff;
		int i;

		// compare the area bits
		areaDiff = 0;

		for ( i = 0; i < MAX_MAP_AREA_BYTES / 4; i++ )
		{
			areaDiff |= ( ( int * ) tr.refdef.areamask ) [ i ] ^ ( ( int * ) fd->areamask ) [ i ];
			( ( int * ) tr.refdef.areamask ) [ i ] = ( ( int * ) fd->areamask ) [ i ];
		}

		if ( areaDiff )
		{
			// a door just opened or something
			tr.refdef.areamaskModified = qtrue;
		}
	}

	R_AddWorldLightsToScene();

	// derived info
	tr.refdef.floatTime = tr.refdef.time * 0.001f;

	tr.refdef.numDrawSurfs = r_firstSceneDrawSurf;
	tr.refdef.drawSurfs = backEndData[ tr.smpFrame ]->drawSurfs;

	tr.refdef.numInteractions = r_firstSceneInteraction;
	tr.refdef.interactions = backEndData[ tr.smpFrame ]->interactions;

	tr.refdef.numEntities = r_numEntities - r_firstSceneEntity;
	tr.refdef.entities = &backEndData[ tr.smpFrame ]->entities[ r_firstSceneEntity ];

	tr.refdef.numLights = r_numLights - r_firstSceneLight;
	tr.refdef.lights = &backEndData[ tr.smpFrame ]->lights[ r_firstSceneLight ];

	tr.refdef.numPolys = r_numPolys - r_firstScenePoly;
	tr.refdef.polys = &backEndData[ tr.smpFrame ]->polys[ r_firstScenePoly ];

	tr.refdef.numPolybuffers = r_numPolybuffers - r_firstScenePolybuffer;
	tr.refdef.polybuffers = &backEndData[ tr.smpFrame ]->polybuffers[ r_firstScenePolybuffer ];

	tr.refdef.numDecalProjectors = r_numDecalProjectors - r_firstSceneDecalProjector;
	tr.refdef.decalProjectors = &backEndData[ tr.smpFrame ]->decalProjectors[ r_firstSceneDecalProjector ];

	tr.refdef.numDecals = r_numDecals - r_firstSceneDecal;
	tr.refdef.decals = &backEndData[ tr.smpFrame ]->decals[ r_firstSceneDecal ];

	tr.refdef.numVisTests = r_numVisTests - r_firstSceneVisTest;
	tr.refdef.visTests = &backEndData[ tr.smpFrame ]->visTests[ r_firstSceneVisTest ];

	// a single frame may have multiple scenes draw inside it --
	// a 3D game view, 3D status bar renderings, 3D menus, etc.
	// They need to be distinguished by the light flare code, because
	// the visibility state for a given surface may be different in
	// each scene / view.
	tr.frameSceneNum++;
	tr.sceneCount++;

	// Tr3B: a scene can have multiple views caused by mirrors or portals
	// the number of views is restricted so we can use hardware occlusion queries
	// and put them into the BSP nodes for each view
	tr.viewCount = -1;

	// setup view parms for the initial view
	//
	// set up viewport
	// The refdef takes 0-at-the-top y coordinates, so
	// convert to GL's 0-at-the-bottom space
	//
	Com_Memset( &parms, 0, sizeof( parms ) );

#if 1

	if ( tr.refdef.pixelTarget == NULL )
	{
		parms.viewportX = tr.refdef.x;
		parms.viewportY = glConfig.vidHeight - ( tr.refdef.y + tr.refdef.height );
	}
	else
	{
		//Driver bug, if we try and do pixel target work along the top edge of a window
		//we can end up capturing part of the status bar. (see screenshot corruption..)
		//Soooo.. use the middle.
		parms.viewportX = glConfig.vidWidth / 2;
		parms.viewportY = glConfig.vidHeight / 2;
	}

#else
	parms.viewportX = tr.refdef.x;
	parms.viewportY = glConfig.vidHeight - ( tr.refdef.y + tr.refdef.height );
#endif

	parms.viewportWidth = tr.refdef.width;
	parms.viewportHeight = tr.refdef.height;

	Vector4Set( parms.viewportVerts[ 0 ], parms.viewportX, parms.viewportY, 0, 1 );
	Vector4Set( parms.viewportVerts[ 1 ], parms.viewportX + parms.viewportWidth, parms.viewportY, 0, 1 );
	Vector4Set( parms.viewportVerts[ 2 ], parms.viewportX + parms.viewportWidth, parms.viewportY + parms.viewportHeight, 0, 1 );
	Vector4Set( parms.viewportVerts[ 3 ], parms.viewportX, parms.viewportY + parms.viewportHeight, 0, 1 );

	parms.isPortal = qfalse;

	parms.fovX = tr.refdef.fov_x;
	parms.fovY = tr.refdef.fov_y;

	parms.stereoFrame = tr.refdef.stereoFrame;

	VectorCopy( fd->vieworg, parms.orientation.origin );
	VectorCopy( fd->viewaxis[ 0 ], parms.orientation.axis[ 0 ] );
	VectorCopy( fd->viewaxis[ 1 ], parms.orientation.axis[ 1 ] );
	VectorCopy( fd->viewaxis[ 2 ], parms.orientation.axis[ 2 ] );

	VectorCopy( fd->vieworg, parms.pvsOrigin );
	Vector4Copy( fd->gradingWeights, parms.gradingWeights );

	R_RenderView( &parms );

	// the next scene rendered in this frame will tack on after this one
	r_firstSceneDrawSurf = tr.refdef.numDrawSurfs;
	r_firstSceneInteraction = tr.refdef.numInteractions;
	r_firstSceneEntity = r_numEntities;
	r_firstSceneLight = r_numLights;
	r_firstScenePoly = r_numPolys;
	r_firstScenePolybuffer = r_numPolybuffers;

	tr.frontEndMsec += ri.Milliseconds() - startTime;
}

// Temp storage for saving view paramters.  Drawing the animated head in the corner
// was creaming important view info.
static viewParms_t g_oldViewParms;

/*
================
RE_SaveViewParms

Save out the old render info to a temp place so we don't kill the LOD system
when we do a second render.
================
*/
void RE_SaveViewParms( void )
{
	// save old viewParms so we can return to it after the mirror view
	g_oldViewParms = tr.viewParms;
}

/*
================
RE_RestoreViewParms

Restore the old render info so we don't kill the LOD system
when we do a second render.
================
*/
void RE_RestoreViewParms( void )
{
	// This was killing the LOD computation
	tr.viewParms = g_oldViewParms;
}

/*
================
RE_RegisterVisTest

Reserves a VisTest handle. This can be used to query the visibility
of a 3D-point in the scene. The engine will try not to stall the GPU,
so the result may be available delayed.
================
*/
qhandle_t RE_RegisterVisTest( void )
{
	int hTest;
	visTest_t *test;

	for( hTest = 0; hTest < tr.numVisTests; hTest++ ) {
		test = tr.visTests[ hTest ];
		if( !test->registered )
			break;
	}

	if( hTest >= tr.numVisTests ) {
		if( tr.numVisTests == MAX_VISTESTS ) {
			ri.Printf( PRINT_WARNING, "WARNING: RE_RegisterVisTest - MAX_VISTESTS hit\n" );
			return 0;
		}

		hTest = tr.numVisTests++;
		test = (visTest_t *)ri.Hunk_Alloc( sizeof( visTest_t ), h_low );
		tr.visTests[ hTest ] = test;
	}

	memset( test, 0, sizeof( visTest_t ) );
	glGenQueries( 1, &test->hQuery );
	glGenQueries( 1, &test->hQueryRef );
	test->registered = qtrue;

	return hTest + 1;
}

/*
================
RE_AddVisTestToScene

Add a VisTest to the current scene. If the VisTest is still
running from a prior scene, just noop.
================
*/
void RE_AddVisTestToScene( qhandle_t hTest, vec3_t pos, float depthAdjust,
			   float area )
{
	visTest_t *test;

	if( r_numVisTests == MAX_VISTESTS ) {
		ri.Printf( PRINT_WARNING, "WARNING: RE_AddVisTestToScene - MAX_VISTESTS hit\n" );
		return;
	}

	if( hTest <= 0 || hTest > MAX_VISTESTS ||
	    !(test = tr.visTests[ hTest - 1 ] ) )
		return;

	VectorCopy( pos, test->position );
	test->depthAdjust = depthAdjust;
	test->area = area;

	backEndData[ tr.smpFrame ]->visTests[ r_numVisTests++ ] = test;
}

/*
================
RE_CheckVisibility

Query the last available result of a VisTest.
================
*/
float RE_CheckVisibility( qhandle_t hTest )
{
	if( hTest <= 0 || hTest > MAX_VISTESTS || !tr.visTests[ hTest - 1 ] )
		return 0.0f;

	return tr.visTests[ hTest - 1 ]->lastResult;
}

void RE_UnregisterVisTest( qhandle_t hTest )
{
	if( hTest <= 0 || hTest > tr.numVisTests || !tr.visTests[ hTest - 1 ] )
		return;

	glDeleteQueries( 1, &tr.visTests[ hTest - 1 ]->hQuery );
	glDeleteQueries( 1, &tr.visTests[ hTest - 1 ]->hQueryRef );
	tr.visTests[ hTest - 1 ]->registered = qfalse;
}

void R_ShutdownVisTests( void )
{
	int hTest;

	for( hTest = 0; hTest < tr.numVisTests; hTest++ ) {
		if( !tr.visTests[ hTest ]->registered )
			continue;

		glDeleteQueries( 1, &tr.visTests[ hTest ]->hQuery );
		glDeleteQueries( 1, &tr.visTests[ hTest ]->hQueryRef );
		tr.visTests[ hTest ]->registered = qfalse;
	}
}
