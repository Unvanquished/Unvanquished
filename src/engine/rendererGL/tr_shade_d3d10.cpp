/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2006-2009 Robert Beckebans <trebor_7@users.sourceforge.net>

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
// tr_shade.c
#include "tr_local.h"

/*
=================================================================================
THIS ENTIRE FILE IS BACK END!

This file deals with applying shaders to surface data in the tess struct.
=================================================================================
*/

/*
==================
Tess_DrawElements
==================
*/
void Tess_DrawElements()
{
	if( tess.numIndexes == 0 || tess.numVertexes == 0 )
	{
		return;
	}

	/*
	// move tess data through the GPU, finally
	if(glState.currentVBO && glState.currentIBO)
	{
	        //qglDrawRangeElementsEXT(GL_TRIANGLES, 0, tessmesh->vertexes.size(), mesh->indexes.size(), GL_UNSIGNED_INT, VBO_BUFFER_OFFSET(mesh->vbo_indexes_ofs));

	        //qglDrawElements(GL_TRIANGLES, tess.numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET(glState.currentIBO->ofsIndexes));
	        qglDrawElements(GL_TRIANGLES, tess.numIndexes, GL_INDEX_TYPE, BUFFER_OFFSET(0));

	        backEnd.pc.c_vboVertexes += tess.numVertexes;
	        backEnd.pc.c_vboIndexes += tess.numIndexes;
	}
	else
	{
	        qglDrawElements(GL_TRIANGLES, tess.numIndexes, GL_INDEX_TYPE, tess.indexes);
	}
	*/

	// update performance counters
	backEnd.pc.c_drawElements++;
	backEnd.pc.c_indexes += tess.numIndexes;
	backEnd.pc.c_vertexes += tess.numVertexes;
}

/*
=============================================================

SURFACE SHADERS

=============================================================
*/

shaderCommands_t tess;

/*
===============
Tess_ComputeColor
===============
*/
void Tess_ComputeColor( shaderStage_t *pStage )
{
	float rgb;
	float red;
	float green;
	float blue;
	float alpha;

	GLimp_LogComment( "--- Tess_ComputeColor ---\n" );

	// rgbGen
	switch( pStage->rgbGen )
	{
		case CGEN_IDENTITY:
			{
				tess.svars.color[ 0 ] = 1.0;
				tess.svars.color[ 1 ] = 1.0;
				tess.svars.color[ 2 ] = 1.0;
				tess.svars.color[ 3 ] = 1.0;
				break;
			}

		default:
		case CGEN_IDENTITY_LIGHTING:
			{
				tess.svars.color[ 0 ] = tr.identityLight;
				tess.svars.color[ 1 ] = tr.identityLight;
				tess.svars.color[ 2 ] = tr.identityLight;
				tess.svars.color[ 3 ] = tr.identityLight;
				break;
			}

		case CGEN_CONST:
			{
				tess.svars.color[ 0 ] = pStage->constantColor[ 0 ] * ( 1.0 / 255.0 );
				tess.svars.color[ 1 ] = pStage->constantColor[ 1 ] * ( 1.0 / 255.0 );
				tess.svars.color[ 2 ] = pStage->constantColor[ 2 ] * ( 1.0 / 255.0 );
				tess.svars.color[ 3 ] = pStage->constantColor[ 3 ] * ( 1.0 / 255.0 );
				break;
			}

		case CGEN_ENTITY:
			{
				if( backEnd.currentLight )
				{
					tess.svars.color[ 0 ] = Q_bound( 0.0, backEnd.currentLight->l.color[ 0 ], 1.0 );
					tess.svars.color[ 1 ] = Q_bound( 0.0, backEnd.currentLight->l.color[ 1 ], 1.0 );
					tess.svars.color[ 2 ] = Q_bound( 0.0, backEnd.currentLight->l.color[ 2 ], 1.0 );
					tess.svars.color[ 3 ] = 1.0;
				}
				else if( backEnd.currentEntity )
				{
					tess.svars.color[ 0 ] = Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 0 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 1 ] = Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 1 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 2 ] = Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 2 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 3 ] = Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( 1.0 / 255.0 ), 1.0 );
				}
				else
				{
					tess.svars.color[ 0 ] = 1.0;
					tess.svars.color[ 1 ] = 1.0;
					tess.svars.color[ 2 ] = 1.0;
					tess.svars.color[ 3 ] = 1.0;
				}

				break;
			}

		case CGEN_ONE_MINUS_ENTITY:
			{
				if( backEnd.currentLight )
				{
					tess.svars.color[ 0 ] = 1.0 - Q_bound( 0.0, backEnd.currentLight->l.color[ 0 ], 1.0 );
					tess.svars.color[ 1 ] = 1.0 - Q_bound( 0.0, backEnd.currentLight->l.color[ 1 ], 1.0 );
					tess.svars.color[ 2 ] = 1.0 - Q_bound( 0.0, backEnd.currentLight->l.color[ 2 ], 1.0 );
					tess.svars.color[ 3 ] = 0.0; // FIXME
				}
				else if( backEnd.currentEntity )
				{
					tess.svars.color[ 0 ] = 1.0 - Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 0 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 1 ] = 1.0 - Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 1 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 2 ] = 1.0 - Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 2 ] * ( 1.0 / 255.0 ), 1.0 );
					tess.svars.color[ 3 ] = 1.0 - Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( 1.0 / 255.0 ), 1.0 );
				}
				else
				{
					tess.svars.color[ 0 ] = 0.0;
					tess.svars.color[ 1 ] = 0.0;
					tess.svars.color[ 2 ] = 0.0;
					tess.svars.color[ 3 ] = 0.0;
				}

				break;
			}

		case CGEN_WAVEFORM:
			{
				float      glow;
				waveForm_t *wf;

				wf = &pStage->rgbWave;

				if( wf->func == GF_NOISE )
				{
					glow = wf->base + R_NoiseGet4f( 0, 0, 0, ( backEnd.refdef.floatTime + wf->phase ) * wf->frequency ) * wf->amplitude;
				}
				else
				{
					glow = RB_EvalWaveForm( wf ) * tr.identityLight;
				}

				if( glow < 0 )
				{
					glow = 0;
				}
				else if( glow > 1 )
				{
					glow = 1;
				}

				tess.svars.color[ 0 ] = glow;
				tess.svars.color[ 1 ] = glow;
				tess.svars.color[ 2 ] = glow;
				tess.svars.color[ 3 ] = 1.0;
				break;
			}

		case CGEN_CUSTOM_RGB:
			{
				rgb = Q_bound( 0.0, RB_EvalExpression( &pStage->rgbExp, 1.0 ), 1.0 );

				tess.svars.color[ 0 ] = rgb;
				tess.svars.color[ 1 ] = rgb;
				tess.svars.color[ 2 ] = rgb;
				break;
			}

		case CGEN_CUSTOM_RGBs:
			{
				if( backEnd.currentLight )
				{
					red = Q_bound( 0.0, RB_EvalExpression( &pStage->redExp, backEnd.currentLight->l.color[ 0 ] ), 1.0 );
					green = Q_bound( 0.0, RB_EvalExpression( &pStage->greenExp, backEnd.currentLight->l.color[ 1 ] ), 1.0 );
					blue = Q_bound( 0.0, RB_EvalExpression( &pStage->blueExp, backEnd.currentLight->l.color[ 2 ] ), 1.0 );
				}
				else if( backEnd.currentEntity )
				{
					red =
					  Q_bound( 0.0, RB_EvalExpression( &pStage->redExp, backEnd.currentEntity->e.shaderRGBA[ 0 ] * ( 1.0 / 255.0 ) ), 1.0 );
					green =
					  Q_bound( 0.0, RB_EvalExpression( &pStage->greenExp, backEnd.currentEntity->e.shaderRGBA[ 1 ] * ( 1.0 / 255.0 ) ),
					           1.0 );
					blue =
					  Q_bound( 0.0, RB_EvalExpression( &pStage->blueExp, backEnd.currentEntity->e.shaderRGBA[ 2 ] * ( 1.0 / 255.0 ) ),
					           1.0 );
				}
				else
				{
					red = Q_bound( 0.0, RB_EvalExpression( &pStage->redExp, 1.0 ), 1.0 );
					green = Q_bound( 0.0, RB_EvalExpression( &pStage->greenExp, 1.0 ), 1.0 );
					blue = Q_bound( 0.0, RB_EvalExpression( &pStage->blueExp, 1.0 ), 1.0 );
				}

				tess.svars.color[ 0 ] = red;
				tess.svars.color[ 1 ] = green;
				tess.svars.color[ 2 ] = blue;
				break;
			}
	}

	// alphaGen
	switch( pStage->alphaGen )
	{
		default:
		case AGEN_IDENTITY:
			{
				if( pStage->rgbGen != CGEN_IDENTITY )
				{
					tess.svars.color[ 3 ] = 1.0;
				}

				break;
			}

		case AGEN_CONST:
			{
				if( pStage->rgbGen != CGEN_CONST )
				{
					tess.svars.color[ 3 ] = pStage->constantColor[ 3 ] * ( 1.0 / 255.0 );
				}

				break;
			}

		case AGEN_ENTITY:
			{
				if( backEnd.currentLight )
				{
					tess.svars.color[ 3 ] = 1.0; // FIXME ?
				}
				else if( backEnd.currentEntity )
				{
					tess.svars.color[ 3 ] = Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( 1.0 / 255.0 ), 1.0 );
				}
				else
				{
					tess.svars.color[ 3 ] = 1.0;
				}

				break;
			}

		case AGEN_ONE_MINUS_ENTITY:
			{
				if( backEnd.currentLight )
				{
					tess.svars.color[ 3 ] = 0.0; // FIXME ?
				}
				else if( backEnd.currentEntity )
				{
					tess.svars.color[ 3 ] = 1.0 - Q_bound( 0.0, backEnd.currentEntity->e.shaderRGBA[ 3 ] * ( 1.0 / 255.0 ), 1.0 );
				}
				else
				{
					tess.svars.color[ 3 ] = 0.0;
				}

				break;
			}

		case AGEN_WAVEFORM:
			{
				float      glow;
				waveForm_t *wf;

				wf = &pStage->alphaWave;

				glow = RB_EvalWaveFormClamped( wf );

				tess.svars.color[ 3 ] = glow;
				break;
			}

		case AGEN_CUSTOM:
			{
				alpha = Q_bound( 0.0, RB_EvalExpression( &pStage->alphaExp, 1.0 ), 1.0 );

				tess.svars.color[ 3 ] = alpha;
				break;
			}
	}
}

/*
===============
Tess_ComputeTexMatrices
===============
*/
static void Tess_ComputeTexMatrices( shaderStage_t *pStage )
{
	int   i;
	vec_t *matrix;

	GLimp_LogComment( "--- Tess_ComputeTexMatrices ---\n" );

	for( i = 0; i < MAX_TEXTURE_BUNDLES; i++ )
	{
		matrix = tess.svars.texMatrices[ i ];

		RB_CalcTexMatrix( &pStage->bundle[ i ], matrix );
	}
}

void Tess_StageIteratorGeneric()
{
	int stage;

	// log this call
	if( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va
		                  ( "--- Tess_StageIteratorGeneric( %s, %i vertices, %i triangles ) ---\n", tess.surfaceShader->name,
		                    tess.numVertexes, tess.numIndexes / 3 ) );
	}

	//GL_CheckErrors();

	Tess_DeformGeometry();

	//if(!glState.currentVBO || !glState.currentIBO || glState.currentVBO == tess.vbo || glState.currentIBO == tess.ibo)
	{
		// Tr3B: FIXME analyze required vertex attribs by the current material
		Tess_UpdateVBOs( 0 );
	}

	/*
	if(tess.surfaceShader->fogVolume)
	{
	        Render_volumetricFog();
	        return;
	}
	*/

	// set face culling appropriately
	//GL_Cull(tess.surfaceShader->cullType);

	// set polygon offset if necessary

	/*
	if(tess.surfaceShader->polygonOffset)
	{
	        qglEnable(GL_POLYGON_OFFSET_FILL);
	        qglPolygonOffset(r_offsetFactor->value, r_offsetUnits->value);
	}
	*/

	// call shader function
	for( stage = 0; stage < MAX_SHADER_STAGES; stage++ )
	{
		shaderStage_t *pStage = tess.surfaceStages[ stage ];

		if( !pStage )
		{
			break;
		}

		if( !RB_EvalExpression( &pStage->ifExp, 1.0 ) )
		{
			continue;
		}

		Tess_ComputeColor( pStage );
		Tess_ComputeTexMatrices( pStage );

		switch( pStage->type )
		{
			case ST_COLORMAP:
				{
					//Render_genericSingle(stage);
					break;
				}

#if defined( COMPAT_Q3A )

			case ST_LIGHTMAP:
				{
					//Render_lightMapping(stage, qtrue);
					break;
				}

#endif

				/*
				case ST_DIFFUSEMAP:
				case ST_COLLAPSE_lighting_DB:
				case ST_COLLAPSE_lighting_DBS:
				{
				        //if(tess.surfaceShader->sort <= SS_OPAQUE)
				        {
				                if(r_precomputedLighting->integer || r_vertexLighting->integer)
				                {
				                        if(!r_vertexLighting->integer && tess.lightmapNum >= 0 && (tess.lightmapNum / 2) < tr.lightmaps.currentElements)
				                        {
				                                if(tr.worldDeluxeMapping && r_normalMapping->integer)
				                                {
				                                        Render_deluxeMapping(stage);
				                                }
				                                else
				                                {
				                                        Render_lightMapping(stage, qfalse);
				                                }
				                        }
				                        else if(backEnd.currentEntity != &tr.worldEntity)
				                        {
				                                Render_vertexLighting_DBS_entity(stage);
				                        }
				                        else
				                        {
				                                Render_vertexLighting_DBS_world(stage);
				                        }
				                }
				                else
				                {
				                        Render_depthFill(stage, qfalse);
				                }
				        }
				        break;
				}

				case ST_COLLAPSE_reflection_CB:
				{
				        Render_reflection_CB(stage);
				        break;
				}

				case ST_REFLECTIONMAP:
				{
				        Render_reflection_C(stage);
				        break;
				}

				case ST_REFRACTIONMAP:
				{
				        Render_refraction_C(stage);
				        break;
				}

				case ST_DISPERSIONMAP:
				{
				        Render_dispersion_C(stage);
				        break;
				}

				case ST_SKYBOXMAP:
				{
				        Render_skybox(stage);
				        break;
				}

				case ST_SCREENMAP:
				{
				        Render_screen(stage);
				        break;
				}

				case ST_PORTALMAP:
				{
				        Render_portal(stage);
				        break;
				}

				case ST_HEATHAZEMAP:
				{
				        Render_heatHaze(stage);
				        break;
				}

				case ST_LIQUIDMAP:
				{
				        Render_liquid(stage);
				        break;
				}
				*/

			default:
				break;
		}
	}

	// reset polygon offset

	/*
	if(tess.surfaceShader->polygonOffset)
	{
	        qglDisable(GL_POLYGON_OFFSET_FILL);
	}
	*/
}

/*
==============
Tess_Begin

We must set some things up before beginning any tesselation,
because a surface may be forced to perform a Tess_End due
to overflow.
==============
*/
// *INDENT-OFF*
void            Tess_Begin( void ( *stageIteratorFunc )(),
                            void ( *stageIteratorFunc2 )(),
                            shader_t *surfaceShader, shader_t *lightShader,
                            qboolean skipTangentSpaces,
                            qboolean skipVBO,
                            int lightmapNum,
                            int     fogNum )
{
	shader_t *state;

	tess.numIndexes = 0;
	tess.numVertexes = 0;

	tess.multiDrawPrimitives = 0;

	// materials are optional
	if( surfaceShader != NULL )
	{
		state = ( surfaceShader->remappedShader ) ? surfaceShader->remappedShader : surfaceShader;

		tess.surfaceShader = state;
		tess.surfaceStages = state->stages;
		tess.numSurfaceStages = state->numStages;
	}
	else
	{
		state = NULL;

		tess.numSurfaceStages = 0;
		tess.surfaceShader = NULL;
		tess.surfaceStages = NULL;
	}

	bool isSky = ( state != NULL && state->isSky != qfalse );

	tess.lightShader = lightShader;

	tess.stageIteratorFunc = stageIteratorFunc;
	tess.stageIteratorFunc2 = stageIteratorFunc2;

	if( !tess.stageIteratorFunc )
	{
		//tess.stageIteratorFunc = &Tess_StageIteratorGeneric;
		ri.Error( ERR_FATAL, "tess.stageIteratorFunc == NULL" );
	}

	if( tess.stageIteratorFunc == &Tess_StageIteratorGeneric )
	{
		if( isSky )
		{
			tess.stageIteratorFunc = &Tess_StageIteratorSky;
			tess.stageIteratorFunc2 = &Tess_StageIteratorGeneric;
		}
	}

#if 0
	else if( tess.stageIteratorFunc == &Tess_StageIteratorDepthFill )
	{
		if( isSky )
		{
			tess.stageIteratorFunc = &Tess_StageIteratorSky;
			tess.stageIteratorFunc2 = &Tess_StageIteratorDepthFill;
		}
	}
	else if( tess.stageIteratorFunc == Tess_StageIteratorGBuffer )
	{
		if( isSky )
		{
			tess.stageIteratorFunc = &Tess_StageIteratorSky;
			tess.stageIteratorFunc2 = &Tess_StageIteratorGBuffer;
		}
	}

#endif

	tess.skipTangentSpaces = skipTangentSpaces;
	//tess.shadowVolume = shadowVolume;
	tess.lightmapNum = lightmapNum;

	if( r_logFile->integer )
	{
		// don't just call LogComment, or we will get
		// a call to va() every frame!
		GLimp_LogComment( va( "--- Tess_Begin( surfaceShader = %s, lightShader = %s, skipTangentSpaces = %i, shadowVolume = %i, lightmap = %i ) ---\n", tess.surfaceShader->name, tess.lightShader ? tess.lightShader->name : NULL, tess.skipTangentSpaces, tess.shadowVolume, tess.lightmapNum ) );
	}
}

/*
=================
Tess_End

Render tesselated data
=================
*/
void Tess_End()
{
	if( tess.numIndexes == 0 || tess.numVertexes == 0 )
	{
		return;
	}

	if( tess.indexes[ SHADER_MAX_INDEXES - 1 ] != 0 )
	{
		ri.Error( ERR_DROP, "Tess_End() - SHADER_MAX_INDEXES hit" );
	}

	if( tess.xyz[ SHADER_MAX_VERTEXES - 1 ][ 0 ] != 0 )
	{
		ri.Error( ERR_DROP, "Tess_End() - SHADER_MAX_VERTEXES hit" );
	}

	// for debugging of sort order issues, stop rendering after a given sort value
	if( r_debugSort->integer && r_debugSort->integer < tess.surfaceShader->sort )
	{
		return;
	}

	// update performance counter
	backEnd.pc.c_batches++;

	//GL_CheckErrors();

	// call off to shader specific tess end function
	tess.stageIteratorFunc();

	/*
	if(!tess.shadowVolume)
	{
	        // draw debugging stuff
	        if(r_showTris->integer || r_showBatches->integer ||
	           (r_showLightBatches->integer && (tess.stageIteratorFunc == Tess_StageIteratorLighting)))
	        {
	                DrawTris();
	        }
	}
	*/

	tess.vboVertexSkinning = qfalse;

	// clear shader so we can tell we don't have any unclosed surfaces
	tess.numIndexes = 0;
	tess.numVertexes = 0;

	GLimp_LogComment( "--- Tess_End ---\n" );

	//GL_CheckErrors();
}

void GLimp_LogComment( char *comment )
{
	// TODO
}
