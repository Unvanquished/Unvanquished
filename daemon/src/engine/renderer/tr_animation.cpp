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
// tr_animation.c
#include "tr_local.h"

/*
===========================================================================
All bones should be an identity orientation to display the mesh exactly
as it is specified.

For all other frames, the bones represent the transformation from the
orientation of the bone in the base frame to the orientation in this
frame.
===========================================================================
*/

static skelAnimation_t *R_AllocAnimation()
{
	skelAnimation_t *anim;

	if ( tr.numAnimations == MAX_ANIMATIONFILES )
	{
		return nullptr;
	}

	anim = (skelAnimation_t*) ri.Hunk_Alloc( sizeof( *anim ), ha_pref::h_low );
	anim->index = tr.numAnimations;
	tr.animations[ tr.numAnimations ] = anim;
	tr.numAnimations++;

	return anim;
}

/*
===============
R_InitAnimations
===============
*/
void R_InitAnimations()
{
	skelAnimation_t *anim;

	// leave a space for nullptr animation
	tr.numAnimations = 0;

	anim = R_AllocAnimation();
	anim->type = animType_t::AT_BAD;
	strcpy( anim->name, "<default animation>" );
}

static bool R_LoadMD5Anim( skelAnimation_t *skelAnim, void *buffer, const char *name )
{
	int            i, j;
	md5Animation_t *anim;
	md5Frame_t     *frame;
	md5Channel_t   *channel;
	char           *token;
	int            version;
	const char     *buf_p;

	buf_p = (char*) buffer;

	skelAnim->type = animType_t::AT_MD5;
	skelAnim->md5 = anim = (md5Animation_t*) ri.Hunk_Alloc( sizeof( *anim ), ha_pref::h_low );

	// skip MD5Version indent string
	COM_ParseExt2( &buf_p, false );

	// check version
	token = COM_ParseExt2( &buf_p, false );
	version = atoi( token );

	if ( version != MD5_VERSION )
	{
		Log::Warn("RE_RegisterAnimation: '%s' has wrong version (%i should be %i)", name, version, MD5_VERSION );
		return false;
	}

	// skip commandline <arguments string>
	token = COM_ParseExt2( &buf_p, true );
	token = COM_ParseExt2( &buf_p, true );

	// parse numFrames <number>
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "numFrames" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected 'numFrames' found '%s' in model '%s'", token, name );
		return false;
	}

	token = COM_ParseExt2( &buf_p, false );
	anim->numFrames = atoi( token );

	// parse numJoints <number>
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "numJoints" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected 'numJoints' found '%s' in model '%s'", token, name );
		return false;
	}

	token = COM_ParseExt2( &buf_p, false );
	anim->numChannels = atoi( token );

	// parse frameRate <number>
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "frameRate" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected 'frameRate' found '%s' in model '%s'", token, name );
		return false;
	}

	token = COM_ParseExt2( &buf_p, false );
	anim->frameRate = atoi( token );

	// parse numAnimatedComponents <number>
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "numAnimatedComponents" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected 'numAnimatedComponents' found '%s' in model '%s'", token,
		           name );
		return false;
	}

	token = COM_ParseExt2( &buf_p, false );
	anim->numAnimatedComponents = atoi( token );

	// parse hierarchy {
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "hierarchy" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected 'hierarchy' found '%s' in model '%s'", token, name );
		return false;
	}

	token = COM_ParseExt2( &buf_p, false );

	if ( Q_stricmp( token, "{" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected '{' found '%s' in model '%s'", token, name );
		return false;
	}

	// parse all the channels
	anim->channels = (md5Channel_t*) ri.Hunk_Alloc( sizeof( md5Channel_t ) * anim->numChannels, ha_pref::h_low );

	for ( i = 0, channel = anim->channels; i < anim->numChannels; i++, channel++ )
	{
		token = COM_ParseExt2( &buf_p, true );
		Q_strncpyz( channel->name, token, sizeof( channel->name ) );

		//Log::Notice("RE_RegisterAnimation: '%s' has channel '%s'", name, channel->name);

		token = COM_ParseExt2( &buf_p, false );
		channel->parentIndex = atoi( token );

		if ( channel->parentIndex >= anim->numChannels )
		{
			ri.Error( errorParm_t::ERR_DROP, "RE_RegisterAnimation: '%s' has channel '%s' with bad parent index %i while numBones is %i",
			          name, channel->name, channel->parentIndex, anim->numChannels );
		}

		token = COM_ParseExt2( &buf_p, false );
		channel->componentsBits = atoi( token );

		token = COM_ParseExt2( &buf_p, false );
		channel->componentsOffset = atoi( token );
	}

	// parse }
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "}" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected '}' found '%s' in model '%s'", token, name );
		return false;
	}

	// parse bounds {
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "bounds" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected 'bounds' found '%s' in model '%s'", token, name );
		return false;
	}

	token = COM_ParseExt2( &buf_p, false );

	if ( Q_stricmp( token, "{" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected '{' found '%s' in model '%s'", token, name );
		return false;
	}

	anim->frames = (md5Frame_t*) ri.Hunk_Alloc( sizeof( md5Frame_t ) * anim->numFrames, ha_pref::h_low );

	for ( i = 0, frame = anim->frames; i < anim->numFrames; i++, frame++ )
	{
		// skip (
		token = COM_ParseExt2( &buf_p, true );

		if ( Q_stricmp( token, "(" ) )
		{
			Log::Warn("RE_RegisterAnimation: expected '(' found '%s' in model '%s'", token, name );
			return false;
		}

		for ( j = 0; j < 3; j++ )
		{
			token = COM_ParseExt2( &buf_p, false );
			frame->bounds[ 0 ][ j ] = atof( token );
		}

		// skip )
		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, ")" ) )
		{
			Log::Warn("RE_RegisterAnimation: expected ')' found '%s' in model '%s'", token, name );
			return false;
		}

		// skip (
		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, "(" ) )
		{
			Log::Warn("RE_RegisterAnimation: expected '(' found '%s' in model '%s'", token, name );
			return false;
		}

		for ( j = 0; j < 3; j++ )
		{
			token = COM_ParseExt2( &buf_p, false );
			frame->bounds[ 1 ][ j ] = atof( token );
		}

		// skip )
		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, ")" ) )
		{
			Log::Warn("RE_RegisterAnimation: expected ')' found '%s' in model '%s'", token, name );
			return false;
		}
	}

	// parse }
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "}" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected '}' found '%s' in model '%s'", token, name );
		return false;
	}

	// parse baseframe {
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "baseframe" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected 'baseframe' found '%s' in model '%s'", token, name );
		return false;
	}

	token = COM_ParseExt2( &buf_p, false );

	if ( Q_stricmp( token, "{" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected '{' found '%s' in model '%s'", token, name );
		return false;
	}

	for ( i = 0, channel = anim->channels; i < anim->numChannels; i++, channel++ )
	{
		// skip (
		token = COM_ParseExt2( &buf_p, true );

		if ( Q_stricmp( token, "(" ) )
		{
			Log::Warn("RE_RegisterAnimation: expected '(' found '%s' in model '%s'", token, name );
			return false;
		}

		for ( j = 0; j < 3; j++ )
		{
			token = COM_ParseExt2( &buf_p, false );
			channel->baseOrigin[ j ] = atof( token );
		}

		// skip )
		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, ")" ) )
		{
			Log::Warn("RE_RegisterAnimation: expected ')' found '%s' in model '%s'", token, name );
			return false;
		}

		// skip (
		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, "(" ) )
		{
			Log::Warn("RE_RegisterAnimation: expected '(' found '%s' in model '%s'", token, name );
			return false;
		}

		for ( j = 0; j < 3; j++ )
		{
			token = COM_ParseExt2( &buf_p, false );
			channel->baseQuat[ j ] = atof( token );
		}

		QuatCalcW( channel->baseQuat );

		// skip )
		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, ")" ) )
		{
			Log::Warn("RE_RegisterAnimation: expected ')' found '%s' in model '%s'", token, name );
			return false;
		}
	}

	// parse }
	token = COM_ParseExt2( &buf_p, true );

	if ( Q_stricmp( token, "}" ) )
	{
		Log::Warn("RE_RegisterAnimation: expected '}' found '%s' in model '%s'", token, name );
		return false;
	}

	for ( i = 0, frame = anim->frames; i < anim->numFrames; i++, frame++ )
	{
		// parse frame <number> {
		token = COM_ParseExt2( &buf_p, true );

		if ( Q_stricmp( token, "frame" ) )
		{
			Log::Warn("RE_RegisterAnimation: expected 'baseframe' found '%s' in model '%s'", token, name );
			return false;
		}

		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, va( "%i", i ) ) )
		{
			Log::Warn("RE_RegisterAnimation: expected '%i' found '%s' in model '%s'", i, token, name );
			return false;
		}

		token = COM_ParseExt2( &buf_p, false );

		if ( Q_stricmp( token, "{" ) )
		{
			Log::Warn("RE_RegisterAnimation: expected '{' found '%s' in model '%s'", token, name );
			return false;
		}

		frame->components = (float*) ri.Hunk_Alloc( sizeof( float ) * anim->numAnimatedComponents, ha_pref::h_low );

		for (unsigned j = 0; j < anim->numAnimatedComponents; j++ )
		{
			token = COM_ParseExt2( &buf_p, true );
			frame->components[ j ] = atof( token );
		}

		// parse }
		token = COM_ParseExt2( &buf_p, true );

		if ( Q_stricmp( token, "}" ) )
		{
			Log::Warn("RE_RegisterAnimation: expected '}' found '%s' in model '%s'", token, name );
			return false;
		}
	}

	// everything went ok
	return true;
}

/*
===============
RE_RegisterAnimationIQM

Animation data has already been loaded
===============
*/
qhandle_t RE_RegisterAnimationIQM( const char *name, IQAnim_t *data )
{
	qhandle_t       hAnim;
	skelAnimation_t *anim;

	if ( !name || !name[ 0 ] )
	{
		Log::Warn("Empty name passed to RE_RegisterAnimationIQM" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		Log::Warn("Animation name exceeds MAX_QPATH" );
			return 0;
	}

	// search the currently loaded animations
	for ( hAnim = 1; hAnim < tr.numAnimations; hAnim++ )
	{
		anim = tr.animations[ hAnim ];

		if ( !Q_stricmp( anim->name, name ) )
		{
			if ( anim->type == animType_t::AT_BAD )
			{
				return 0;
			}

			return hAnim;
		}
	}

	// allocate a new model_t
	if ( ( anim = R_AllocAnimation() ) == nullptr )
	{
		Log::Warn("RE_RegisterAnimationIQM: R_AllocAnimation() failed for '%s'", name );
		return 0;
	}

	// only set the name after the animation has been successfully allocated
	Q_strncpyz( anim->name, name, sizeof( anim->name ) );
	anim->type = animType_t::AT_IQM;
	anim->iqm = data;

	return anim->index;
}

/*
===============
RE_RegisterAnimation
===============
*/
qhandle_t RE_RegisterAnimation( const char *name )
{
	qhandle_t       hAnim;
	skelAnimation_t *anim;
	char            *buffer;
	bool        loaded = false;

	if ( !name || !name[ 0 ] )
	{
		Log::Warn("Empty name passed to RE_RegisterAnimation" );
		return 0;
	}

	//Log::Notice("RE_RegisterAnimation(%s)", name);

	if ( strlen( name ) >= MAX_QPATH )
	{
		Log::Warn("Animation name exceeds MAX_QPATH" );
		return 0;
	}

	// search the currently loaded animations
	for ( hAnim = 1; hAnim < tr.numAnimations; hAnim++ )
	{
		anim = tr.animations[ hAnim ];

		if ( !Q_stricmp( anim->name, name ) )
		{
			if ( anim->type == animType_t::AT_BAD )
			{
				return 0;
			}

			return hAnim;
		}
	}

	// allocate a new model_t
	if ( ( anim = R_AllocAnimation() ) == nullptr )
	{
		Log::Warn("RE_RegisterAnimation: R_AllocAnimation() failed for '%s'", name );
		return 0;
	}

	// only set the name after the animation has been successfully allocated
	Q_strncpyz( anim->name, name, sizeof( anim->name ) );

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// load and parse the .md5anim file
	int bufferLen = ri.FS_ReadFile( name, ( void ** ) &buffer );

	if ( !buffer )
	{
		return 0;
	}

	if ( bufferLen >= 10 && !Q_strnicmp( ( const char * ) buffer, "MD5Version", 10 ) )
	{
		loaded = R_LoadMD5Anim( anim, buffer, name );
	}
	else
	{
		Log::Warn("RE_RegisterAnimation: unknown fileid for '%s'", name );
	}

	ri.FS_FreeFile( buffer );

	if ( !loaded )
	{
		Log::Warn("couldn't load '%s'", name );

		// we still keep the model_t around, so if the model name is asked for
		// again, we won't bother scanning the filesystem
		anim->type = animType_t::AT_BAD;
	}

	return anim->index;
}

/*
================
R_GetAnimationByHandle
================
*/
skelAnimation_t *R_GetAnimationByHandle( qhandle_t index )
{
	skelAnimation_t *anim;

	// out of range gets the default animation
	if ( index < 1 || index >= tr.numAnimations )
	{
		return tr.animations[ 0 ];
	}

	anim = tr.animations[ index ];

	return anim;
}

/*
================
R_AnimationList_f
================
*/
void R_AnimationList_f()
{
	int             i;
	skelAnimation_t *anim;

	for ( i = 0; i < tr.numAnimations; i++ )
	{
		anim = tr.animations[ i ];

		Log::Notice("'%s'", anim->name );
	}

	Log::Notice("%8i : Total animations", tr.numAnimations );
}

/*
=============
R_CullMD5
=============
*/
static void R_CullMD5( trRefEntity_t *ent )
{
	int        i;

	if ( ent->e.skeleton.type == refSkeletonType_t::SK_INVALID )
	{
		// no properly set skeleton so use the bounding box by the model instead by the animations
		md5Model_t *model = tr.currentModel->md5;

		VectorCopy( model->bounds[ 0 ], ent->localBounds[ 0 ] );
		VectorCopy( model->bounds[ 1 ], ent->localBounds[ 1 ] );
	}
	else
	{
		// copy a bounding box in the current coordinate system provided by skeleton
		for ( i = 0; i < 3; i++ )
		{
			ent->localBounds[ 0 ][ i ] = ent->e.skeleton.bounds[ 0 ][ i ] * ent->e.skeleton.scale;
			ent->localBounds[ 1 ][ i ] = ent->e.skeleton.bounds[ 1 ][ i ] * ent->e.skeleton.scale;
		}
	}

	switch ( R_CullLocalBox( ent->localBounds ) )
	{
		case cullResult_t::CULL_IN:
			tr.pc.c_box_cull_md5_in++;
			ent->cull = cullResult_t::CULL_IN;
			return;

		case cullResult_t::CULL_CLIP:
			tr.pc.c_box_cull_md5_clip++;
			ent->cull = cullResult_t::CULL_CLIP;
			return;

		case cullResult_t::CULL_OUT:
		default:
			tr.pc.c_box_cull_md5_out++;
			ent->cull = cullResult_t::CULL_OUT;
			return;
	}
}

/*
==============
R_AddMD5Surfaces
==============
*/
void R_AddMD5Surfaces( trRefEntity_t *ent )
{
	md5Model_t   *model;
	md5Surface_t *surface;
	shader_t     *shader;
	int          i;
	bool     personalModel;
	int          fogNum;

	model = tr.currentModel->md5;

	// don't add third_person objects if not in a portal
	personalModel = ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal;

	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum
	R_CullMD5( ent );

	if ( ent->cull == cullResult_t::CULL_OUT )
	{
		return;
	}

	// set up world bounds for light intersection tests
	R_SetupEntityWorldBounds( ent );

	// set up lighting now that we know we aren't culled
	if ( !personalModel || r_shadows->integer > Util::ordinal(shadowingMode_t::SHADOWING_BLOB))
	{
		R_SetupEntityLighting( &tr.refdef, ent, nullptr );
	}

	// see if we are in a fog volume
	fogNum = R_FogWorldBox( ent->worldBounds );

	if ( !r_vboModels->integer || !model->numVBOSurfaces ||
	     ( !glConfig2.vboVertexSkinningAvailable && ent->e.skeleton.type == refSkeletonType_t::SK_ABSOLUTE ) )
	{
		// finally add surfaces
		for ( i = 0, surface = model->surfaces; i < model->numSurfaces; i++, surface++ )
		{
			if ( ent->e.customShader )
			{
				shader = R_GetShaderByHandle( ent->e.customShader );
			}
			else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins )
			{
				skin_t *skin;

				skin = R_GetSkinByHandle( ent->e.customSkin );

				// match the surface name to something in the skin file
				shader = tr.defaultShader;

				// FIXME: replace MD3_MAX_SURFACES for skin_t::surfaces
				if ( i >= 0 && i < skin->numSurfaces && skin->surfaces[ i ] )
				{
					shader = skin->surfaces[ i ]->shader;
				}

				if ( shader == tr.defaultShader )
				{
					Log::Warn("no shader for surface %i in skin %s", i, skin->name );
				}
				else if ( shader->defaultShader )
				{
					Log::Warn("shader %s in skin %s not found", shader->name, skin->name );
				}
			}
			else
			{
				shader = R_GetShaderByHandle( surface->shaderIndex );

				if ( ent->e.altShaderIndex > 0 && ent->e.altShaderIndex < MAX_ALTSHADERS &&
				     shader->altShader[ ent->e.altShaderIndex ].index )
				{
					shader = R_GetShaderByHandle( shader->altShader[ ent->e.altShaderIndex ].index );
				}
			}

			// we will add shadows even if the main object isn't visible in the view

			// don't add third_person objects if not viewing through a portal
			if ( !personalModel )
			{
				R_AddDrawSurf( (surfaceType_t*) surface, shader, -1, fogNum );
			}
		}
	}
	else
	{
		int             i;
		srfVBOMD5Mesh_t *vboSurface;
		shader_t        *shader;

		for ( i = 0; i < model->numVBOSurfaces; i++ )
		{
			vboSurface = model->vboSurfaces[ i ];

			if ( ent->e.customShader )
			{
				shader = R_GetShaderByHandle( ent->e.customShader );
			}
			else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins )
			{
				skin_t *skin;

				skin = R_GetSkinByHandle( ent->e.customSkin );

				// match the surface name to something in the skin file
				shader = tr.defaultShader;

				// FIXME: replace MD3_MAX_SURFACES for skin_t::surfaces
				if ( vboSurface->skinIndex >= 0 && vboSurface->skinIndex < skin->numSurfaces && skin->surfaces[ vboSurface->skinIndex ] )
				{
					shader = skin->surfaces[ vboSurface->skinIndex ]->shader;
				}

				if ( shader == tr.defaultShader )
				{
					Log::Warn("no shader for surface %i in skin %s", i, skin->name );
				}
				else if ( shader->defaultShader )
				{
					Log::Warn("shader %s in skin %s not found", shader->name, skin->name );
				}
			}
			else
			{
				shader = vboSurface->shader;
			}

			// don't add third_person objects if not viewing through a portal
			if ( !personalModel )
			{
				R_AddDrawSurf( (surfaceType_t*) vboSurface, shader, -1, fogNum );
			}
		}
	}
}

/*
=================
R_AddIQMInteractions
=================
*/
void R_AddIQMInteractions( trRefEntity_t *ent, trRefLight_t *light, interactionType_t iaType )
{
	int               i;
	IQModel_t         *model;
	srfIQModel_t      *surface;
	shader_t          *shader = 0;
	bool          personalModel;
	byte              cubeSideBits = CUBESIDE_CLIPALL;

	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum and we don't care about proper shadowing
	if ( ent->cull == cullResult_t::CULL_OUT )
	{
		iaType = (interactionType_t) (iaType & ~IA_LIGHT);

		if( !iaType ) {
			return;
		}
	}

	// avoid drawing of certain objects
#if defined( USE_REFENTITY_NOSHADOWID )

	if ( light->l.inverseShadows )
	{
		if ( (iaType & IA_SHADOW) && ( light->l.noShadowID && ( light->l.noShadowID != ent->e.noShadowID ) ) )
		{
			return;
		}
	}
	else
	{
		if ( (iaType & IA_SHADOW) && ( light->l.noShadowID && ( light->l.noShadowID == ent->e.noShadowID ) ) )
		{
			return;
		}
	}

#endif

	// don't add third_person objects if not in a portal
	personalModel = ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal;

	model = tr.currentModel->iqm;

	// do a quick AABB cull
	if ( !BoundsIntersect( light->worldBounds[ 0 ], light->worldBounds[ 1 ], ent->worldBounds[ 0 ], ent->worldBounds[ 1 ] ) )
	{
		tr.pc.c_dlightSurfacesCulled += model->num_surfaces;
		return;
	}

	// do a more expensive and precise light frustum cull
	if ( !r_noLightFrustums->integer )
	{
		if ( R_CullLightWorldBounds( light, ent->worldBounds ) == cullResult_t::CULL_OUT )
		{
			tr.pc.c_dlightSurfacesCulled += model->num_surfaces;
			return;
		}
	}

	cubeSideBits = R_CalcLightCubeSideBits( light, ent->worldBounds );

		// generate interactions with all surfaces
		for ( i = 0, surface = model->surfaces; i < model->num_surfaces; i++, surface++ )
		{
			if ( ent->e.customShader )
			{
				shader = R_GetShaderByHandle( ent->e.customShader );
			}
			else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins )
			{
				skin_t *skin;

				skin = R_GetSkinByHandle( ent->e.customSkin );

				// match the surface name to something in the skin file
				shader = tr.defaultShader;

				// FIXME: replace MD3_MAX_SURFACES for skin_t::surfaces
				if ( i >= 0 && i < skin->numSurfaces && skin->surfaces[ i ] )
				{
					shader = skin->surfaces[ i ]->shader;
				}

				if ( shader == tr.defaultShader )
				{
					Log::Warn("no shader for surface %i in skin %s", i, skin->name );
				}
				else if ( shader->defaultShader )
				{
					Log::Warn("shader %s in skin %s not found", shader->name, skin->name );
				}
			}
			else
			{
				shader = R_GetShaderByHandle( surface->shader->index );
			}

			// skip all surfaces that don't matter for lighting only pass
			if ( shader->isSky || ( !shader->interactLight && shader->noShadows ) )
			{
				continue;
			}

			// we will add shadows even if the main object isn't visible in the view

			// don't add third_person objects if not viewing through a portal
			if ( !personalModel )
			{
				R_AddLightInteraction( light, ( surfaceType_t * ) surface, shader, cubeSideBits, iaType );
				tr.pc.c_dlightSurfaces++;
			}
		}
}

/*
=================
R_AddMD5Interactions
=================
*/
void R_AddMD5Interactions( trRefEntity_t *ent, trRefLight_t *light, interactionType_t iaType )
{
	int               i;
	md5Model_t        *model;
	md5Surface_t      *surface;
	shader_t          *shader = 0;
	bool          personalModel;
	byte              cubeSideBits = CUBESIDE_CLIPALL;

	// cull the entire model if merged bounding box of both frames
	// is outside the view frustum and we don't care about proper shadowing
	if ( ent->cull == cullResult_t::CULL_OUT )
	{
		iaType = Util::enum_cast<interactionType_t>(iaType & ~IA_LIGHT);
	}

	if( !iaType )
	{
		return;
	}

	// avoid drawing of certain objects
#if defined( USE_REFENTITY_NOSHADOWID )

	if ( light->l.inverseShadows )
	{
		if ( (iaType & IA_SHADOW) && ( light->l.noShadowID && ( light->l.noShadowID != ent->e.noShadowID ) ) )
		{
			return;
		}
	}
	else
	{
		if ( (iaType & IA_SHADOW) && ( light->l.noShadowID && ( light->l.noShadowID == ent->e.noShadowID ) ) )
		{
			return;
		}
	}

#endif

	// don't add third_person objects if not in a portal
	personalModel = ( ent->e.renderfx & RF_THIRD_PERSON ) && !tr.viewParms.isPortal;

	model = tr.currentModel->md5;

	// do a quick AABB cull
	if ( !BoundsIntersect( light->worldBounds[ 0 ], light->worldBounds[ 1 ], ent->worldBounds[ 0 ], ent->worldBounds[ 1 ] ) )
	{
		tr.pc.c_dlightSurfacesCulled += model->numSurfaces;
		return;
	}

	// do a more expensive and precise light frustum cull
	if ( !r_noLightFrustums->integer )
	{
		if ( R_CullLightWorldBounds( light, ent->worldBounds ) == cullResult_t::CULL_OUT )
		{
			tr.pc.c_dlightSurfacesCulled += model->numSurfaces;
			return;
		}
	}

	cubeSideBits = R_CalcLightCubeSideBits( light, ent->worldBounds );

	if ( !r_vboModels->integer || !model->numVBOSurfaces ||
	     ( !glConfig2.vboVertexSkinningAvailable && ent->e.skeleton.type == refSkeletonType_t::SK_ABSOLUTE ) )
	{
		// generate interactions with all surfaces
		for ( i = 0, surface = model->surfaces; i < model->numSurfaces; i++, surface++ )
		{
			if ( ent->e.customShader )
			{
				shader = R_GetShaderByHandle( ent->e.customShader );
			}
			else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins )
			{
				skin_t *skin;

				skin = R_GetSkinByHandle( ent->e.customSkin );

				// match the surface name to something in the skin file
				shader = tr.defaultShader;

				// FIXME: replace MD3_MAX_SURFACES for skin_t::surfaces
				if ( i >= 0 && i < skin->numSurfaces && skin->surfaces[ i ] )
				{
					shader = skin->surfaces[ i ]->shader;
				}

				if ( shader == tr.defaultShader )
				{
					Log::Warn("no shader for surface %i in skin %s", i, skin->name );
				}
				else if ( shader->defaultShader )
				{
					Log::Warn("shader %s in skin %s not found", shader->name, skin->name );
				}
			}
			else
			{
				shader = R_GetShaderByHandle( surface->shaderIndex );
			}

			// skip all surfaces that don't matter for lighting only pass
			if ( shader->isSky || ( !shader->interactLight && shader->noShadows ) )
			{
				continue;
			}

			// we will add shadows even if the main object isn't visible in the view

			// don't add third_person objects if not viewing through a portal
			if ( !personalModel )
			{
				R_AddLightInteraction( light, (surfaceType_t*) surface, shader, cubeSideBits, iaType );
				tr.pc.c_dlightSurfaces++;
			}
		}
	}
	else
	{
		int             i;
		srfVBOMD5Mesh_t *vboSurface;
		shader_t        *shader;

		for ( i = 0; i < model->numVBOSurfaces; i++ )
		{
			vboSurface = model->vboSurfaces[ i ];

			if ( ent->e.customShader )
			{
				shader = R_GetShaderByHandle( ent->e.customShader );
			}
			else if ( ent->e.customSkin > 0 && ent->e.customSkin < tr.numSkins )
			{
				skin_t *skin;

				skin = R_GetSkinByHandle( ent->e.customSkin );

				// match the surface name to something in the skin file
				shader = tr.defaultShader;

				// FIXME: replace MD3_MAX_SURFACES for skin_t::surfaces
				if ( i >= 0 && i < skin->numSurfaces && skin->surfaces[ i ] )
				{
					shader = skin->surfaces[ i ]->shader;
				}

				if ( shader == tr.defaultShader )
				{
					Log::Warn("no shader for surface %i in skin %s", i, skin->name );
				}
				else if ( shader->defaultShader )
				{
					Log::Warn("shader %s in skin %s not found", shader->name, skin->name );
				}
			}
			else
			{
				shader = vboSurface->shader;
			}

			// skip all surfaces that don't matter for lighting only pass
			if ( shader->isSky || ( !shader->interactLight && shader->noShadows ) )
			{
				continue;
			}

			// don't add third_person objects if not viewing through a portal
			if ( !personalModel )
			{
				R_AddLightInteraction( light, (surfaceType_t*) vboSurface, shader, cubeSideBits, iaType );
				tr.pc.c_dlightSurfaces++;
			}
		}
	}
}

/*
==============
IQMCheckSkeleton

check if the skeleton bones are the same in the model and animation
and copy the parentIndex entries into the refSkeleton_t
==============
*/
static bool IQMCheckSkeleton( refSkeleton_t *skel, model_t *model,
				  skelAnimation_t *anim )
{
	int        i;
	IQModel_t *IQModel = model->iqm;

	if ( IQModel->num_joints < 1 )
	{
		Log::Warn("R_IQMCheckSkeleton: '%s' has no bones", model->name );
		return false;
	}

	if ( IQModel->num_joints > MAX_BONES )
	{
		Log::Warn("RE_CheckSkeleton: '%s' has more than %i bones (%i)", model->name, MAX_BONES, IQModel->num_joints );
		return false;
	}

	if ( anim->type == animType_t::AT_IQM && anim->iqm )
	{
		IQAnim_t *IQAnim = anim->iqm;
		char     *modelNames;
		char     *animNames;

		if ( IQModel->jointNames == IQAnim->jointNames ) {
			// loaded from same IQM file, must match
			for ( i = 0; i < IQModel->num_joints; i++ )
			{
				skel->bones[ i ].parentIndex = IQModel->jointParents[ i ];
			}
			return true;
		}

		if ( IQModel->num_joints != IQAnim->num_joints )
		{
			Log::Warn("R_IQMCheckSkeleton: model '%s' has different number of bones than animation '%s': %d != %d", model->name, IQAnim->name, IQModel->num_joints, IQAnim->num_joints );
			return false;
		}

		// check bone names
		modelNames = IQModel->jointNames;
		animNames = IQAnim->jointNames;
		for ( i = 0; i < IQModel->num_joints; i++ )
		{
			if ( Q_stricmp( modelNames, animNames ) )
			{
				return false;
			}
			modelNames += strlen( modelNames ) + 1;
			animNames += strlen( animNames ) + 1;

			skel->bones[ i ].parentIndex = IQModel->jointParents[ i ];
		}

		return true;
	}

	Log::Warn("R_IQMCheckSkeleton: bad animation" );

	return false;
}

/*
==============
RE_CheckSkeleton

Tr3B: check if the skeleton bones are the same in the model and animation
and copy the parentIndex entries into the refSkeleton_t
==============
*/
int RE_CheckSkeleton( refSkeleton_t *skel, qhandle_t hModel, qhandle_t hAnim )
{
	int             i;
	model_t         *model;
	md5Model_t      *md5Model;
	skelAnimation_t *skelAnim;

	model = R_GetModelByHandle( hModel );
	skelAnim = R_GetAnimationByHandle( hAnim );

	if( model->type == modtype_t::MOD_IQM && model->iqm ) {
		return IQMCheckSkeleton( skel, model, skelAnim );
	}
	else if ( model->type != modtype_t::MOD_MD5 || !model->md5 )
	{
		Log::Warn("RE_CheckSkeleton: '%s' is not a skeletal model", model->name );
		return false;
	}

	md5Model = model->md5;

	if ( md5Model->numBones < 1 )
	{
		Log::Warn("RE_CheckSkeleton: '%s' has no bones", model->name );
		return false;
	}

	if ( md5Model->numBones > MAX_BONES )
	{
		Log::Warn("RE_CheckSkeleton: '%s' has more than %i bones (%i)", model->name, MAX_BONES, md5Model->numBones );
		return false;
	}

	if ( skelAnim->type == animType_t::AT_MD5 && skelAnim->md5 )
	{
		md5Animation_t *md5Animation;
		md5Bone_t      *md5Bone;
		md5Channel_t   *md5Channel;

		md5Animation = skelAnim->md5;

		if ( md5Model->numBones != md5Animation->numChannels )
		{
			Log::Warn("RE_CheckSkeleton: model '%s' has different number of bones than animation '%s': %d != %d", model->name, skelAnim->name, md5Model->numBones, md5Animation->numChannels );
			return false;
		}

		// check bone names
		for ( i = 0, md5Bone = md5Model->bones, md5Channel = md5Animation->channels; i < md5Model->numBones; i++, md5Bone++, md5Channel++ )
		{
			if ( Q_stricmp( md5Bone->name, md5Channel->name ) )
			{
				return false;
			}

			skel->bones[ i ].parentIndex = md5Bone->parentIndex;
		}

		return true;
	}

	Log::Warn("RE_BuildSkeleton: bad animation '%s' with handle %i", skelAnim->name, hAnim );

	return false;
}

/*
==============
R_IQMBuildSkeleton
==============
*/
static int IQMBuildSkeleton( refSkeleton_t *skel, skelAnimation_t *skelAnim,
			     int startFrame, int endFrame, float frac )
{
	int            i;
	IQAnim_t       *anim;
	transform_t    *newPose, *oldPose;
	vec3_t         mins, maxs;

	anim = skelAnim->iqm;

	// Validate the frames so there is no chance of a crash.
	// This will write directly into the entity structure, so
	// when the surfaces are rendered, they don't need to be
	// range checked again.
	if( anim->flags & IQM_LOOP ) {
		startFrame %= anim->num_frames;
		endFrame %= anim->num_frames;
	} else {
		Q_clamp( startFrame, 0, anim->num_frames - 1 );
		Q_clamp( endFrame, 0, anim->num_frames - 1 );
	}

	// compute frame pointers
	oldPose = &anim->poses[ startFrame * anim->num_joints ];
	newPose = &anim->poses[ endFrame * anim->num_joints ];

	// calculate a bounding box in the current coordinate system
	if( anim->bounds ) {
		float *bounds = &anim->bounds[ 6 * startFrame ];
		VectorCopy( bounds, mins );
		VectorCopy( bounds + 3, maxs );

		bounds = &anim->bounds[ 6 * endFrame ];
		BoundsAdd( mins, maxs, bounds, bounds + 3 );
	}

	for ( i = 0; i < anim->num_joints; i++ )
	{
		TransStartLerp( &skel->bones[ i ].t );
		TransAddWeight( 1.0f - frac, &oldPose[ i ], &skel->bones[ i ].t );
		TransAddWeight( frac, &newPose[ i ], &skel->bones[ i ].t );
		TransEndLerp( &skel->bones[ i ].t );

#if defined( REFBONE_NAMES )
		Q_strncpyz( skel->bones[ i ].name, anim->name, sizeof( skel->bones[ i ].name ) );
#endif

		skel->bones[ i ].parentIndex = anim->jointParents[ i ];
	}

	skel->numBones = anim->num_joints;
	skel->type = refSkeletonType_t::SK_RELATIVE;
	VectorCopy( mins, skel->bounds[ 0 ] );
	VectorCopy( maxs, skel->bounds[ 1 ] );
	return true;
}

/*
==============
RE_BuildSkeleton
==============
*/
int RE_BuildSkeleton( refSkeleton_t *skel, qhandle_t hAnim, int startFrame, int endFrame, float frac, bool clearOrigin )
{
	skelAnimation_t *skelAnim;

	skelAnim = R_GetAnimationByHandle( hAnim );

	if ( skelAnim->type == animType_t::AT_IQM && skelAnim->iqm ) {
		return IQMBuildSkeleton( skel, skelAnim, startFrame, endFrame, frac );
	}
	else if ( skelAnim->type == animType_t::AT_MD5 && skelAnim->md5 )
	{
		int            i;
		md5Animation_t *anim;
		md5Channel_t   *channel;
		md5Frame_t     *newFrame, *oldFrame;
		vec3_t         newOrigin, oldOrigin, lerpedOrigin;
		quat_t         newQuat, oldQuat, lerpedQuat;
		int            componentsApplied;

		anim = skelAnim->md5;

		// Validate the frames so there is no chance of a crash.
		// This will write directly into the entity structure, so
		// when the surfaces are rendered, they don't need to be
		// range checked again.

		/*
		   if((startFrame >= anim->numFrames) || (startFrame < 0) || (endFrame >= anim->numFrames) || (endFrame < 0))
		   {
		   Log::Debug("RE_BuildSkeleton: no such frame %d to %d for '%s'\n", startFrame, endFrame, anim->name);
		   //startFrame = 0;
		   //endFrame = 0;
		   }
		 */

		startFrame = Math::Clamp( startFrame, 0, anim->numFrames - 1 );
		endFrame = Math::Clamp( endFrame, 0, anim->numFrames - 1 );

		// compute frame pointers
		oldFrame = &anim->frames[ startFrame ];
		newFrame = &anim->frames[ endFrame ];

		// calculate a bounding box in the current coordinate system
		for ( i = 0; i < 3; i++ )
		{
			skel->bounds[ 0 ][ i ] =
			  oldFrame->bounds[ 0 ][ i ] < newFrame->bounds[ 0 ][ i ] ? oldFrame->bounds[ 0 ][ i ] : newFrame->bounds[ 0 ][ i ];
			skel->bounds[ 1 ][ i ] =
			  oldFrame->bounds[ 1 ][ i ] > newFrame->bounds[ 1 ][ i ] ? oldFrame->bounds[ 1 ][ i ] : newFrame->bounds[ 1 ][ i ];
		}

		for ( i = 0, channel = anim->channels; i < anim->numChannels; i++, channel++ )
		{
			// set baseframe values
			VectorCopy( channel->baseOrigin, newOrigin );
			VectorCopy( channel->baseOrigin, oldOrigin );

			QuatCopy( channel->baseQuat, newQuat );
			QuatCopy( channel->baseQuat, oldQuat );

			componentsApplied = 0;

			// update tranlation bits
			if ( channel->componentsBits & COMPONENT_BIT_TX )
			{
				oldOrigin[ 0 ] = oldFrame->components[ channel->componentsOffset + componentsApplied ];
				newOrigin[ 0 ] = newFrame->components[ channel->componentsOffset + componentsApplied ];
				componentsApplied++;
			}

			if ( channel->componentsBits & COMPONENT_BIT_TY )
			{
				oldOrigin[ 1 ] = oldFrame->components[ channel->componentsOffset + componentsApplied ];
				newOrigin[ 1 ] = newFrame->components[ channel->componentsOffset + componentsApplied ];
				componentsApplied++;
			}

			if ( channel->componentsBits & COMPONENT_BIT_TZ )
			{
				oldOrigin[ 2 ] = oldFrame->components[ channel->componentsOffset + componentsApplied ];
				newOrigin[ 2 ] = newFrame->components[ channel->componentsOffset + componentsApplied ];
				componentsApplied++;
			}

			// update quaternion rotation bits
			if ( channel->componentsBits & COMPONENT_BIT_QX )
			{
				( ( vec_t * ) oldQuat ) [ 0 ] = oldFrame->components[ channel->componentsOffset + componentsApplied ];
				( ( vec_t * ) newQuat ) [ 0 ] = newFrame->components[ channel->componentsOffset + componentsApplied ];
				componentsApplied++;
			}

			if ( channel->componentsBits & COMPONENT_BIT_QY )
			{
				( ( vec_t * ) oldQuat ) [ 1 ] = oldFrame->components[ channel->componentsOffset + componentsApplied ];
				( ( vec_t * ) newQuat ) [ 1 ] = newFrame->components[ channel->componentsOffset + componentsApplied ];
				componentsApplied++;
			}

			if ( channel->componentsBits & COMPONENT_BIT_QZ )
			{
				( ( vec_t * ) oldQuat ) [ 2 ] = oldFrame->components[ channel->componentsOffset + componentsApplied ];
				( ( vec_t * ) newQuat ) [ 2 ] = newFrame->components[ channel->componentsOffset + componentsApplied ];
			}

			QuatCalcW( oldQuat );
			QuatNormalize( oldQuat );

			QuatCalcW( newQuat );
			QuatNormalize( newQuat );

#if 1
			VectorLerp( oldOrigin, newOrigin, frac, lerpedOrigin );
			QuatSlerp( oldQuat, newQuat, frac, lerpedQuat );
#else
			VectorCopy( newOrigin, lerpedOrigin );
			QuatCopy( newQuat, lerpedQuat );
#endif

			// copy lerped information to the bone + extra data
			skel->bones[ i ].parentIndex = channel->parentIndex;

			if ( channel->parentIndex < 0 && clearOrigin )
			{
				VectorClear( skel->bones[ i ].t.trans );
				QuatClear( skel->bones[ i ].t.rot );

				// move bounding box back
				VectorSubtract( skel->bounds[ 0 ], lerpedOrigin, skel->bounds[ 0 ] );
				VectorSubtract( skel->bounds[ 1 ], lerpedOrigin, skel->bounds[ 1 ] );
			}
			else
			{
				VectorCopy( lerpedOrigin, skel->bones[ i ].t.trans );
			}

			QuatCopy( lerpedQuat, skel->bones[ i ].t.rot );
			skel->bones[ i ].t.scale = 1.0f;

#if defined( REFBONE_NAMES )
			Q_strncpyz( skel->bones[ i ].name, channel->name, sizeof( skel->bones[ i ].name ) );
#endif
		}

		skel->numBones = anim->numChannels;
		skel->type = refSkeletonType_t::SK_RELATIVE;
		return true;
	}

	// FIXME: clear existing bones and bounds?
	return false;
}

/*
==============
RE_BlendSkeleton
==============
*/
int RE_BlendSkeleton( refSkeleton_t *skel, const refSkeleton_t *blend, float frac )
{
	int    i;
	vec3_t bounds[ 2 ];

	if ( skel->numBones != blend->numBones )
	{
		Log::Warn("RE_BlendSkeleton: different number of bones %d != %d", skel->numBones, blend->numBones );
		return false;
	}

	// lerp between the 2 bone poses
	for ( i = 0; i < skel->numBones; i++ )
	{
		transform_t trans;

		TransStartLerp( &trans );
		TransAddWeight( 1.0f - frac, &skel->bones[ i ].t, &trans );
		TransAddWeight( frac, &blend->bones[ i ].t, &trans );
		TransEndLerp( &trans );

		TransCopy( &trans, &skel->bones[ i ].t );
	}

	// calculate a bounding box in the current coordinate system
	for ( i = 0; i < 3; i++ )
	{
		bounds[ 0 ][ i ] = skel->bounds[ 0 ][ i ] < blend->bounds[ 0 ][ i ] ? skel->bounds[ 0 ][ i ] : blend->bounds[ 0 ][ i ];
		bounds[ 1 ][ i ] = skel->bounds[ 1 ][ i ] > blend->bounds[ 1 ][ i ] ? skel->bounds[ 1 ][ i ] : blend->bounds[ 1 ][ i ];
	}

	VectorCopy( bounds[ 0 ], skel->bounds[ 0 ] );
	VectorCopy( bounds[ 1 ], skel->bounds[ 1 ] );

	return true;
}

/*
==============
RE_AnimNumFrames
==============
*/
int RE_AnimNumFrames( qhandle_t hAnim )
{
	skelAnimation_t *anim;

	anim = R_GetAnimationByHandle( hAnim );

	if( anim->type == animType_t::AT_IQM && anim->iqm ) {
		return anim->iqm->num_frames;
	}
	else if ( anim->type == animType_t::AT_MD5 && anim->md5 )
	{
		return anim->md5->numFrames;
	}

	return 0;
}

/*
==============
RE_AnimFrameRate
==============
*/
int RE_AnimFrameRate( qhandle_t hAnim )
{
	skelAnimation_t *anim;

	anim = R_GetAnimationByHandle( hAnim );

	if( anim->type == animType_t::AT_IQM && anim->iqm ) {
		return anim->iqm->framerate;
	}
	else if ( anim->type == animType_t::AT_MD5 && anim->md5 )
	{
		return anim->md5->frameRate;
	}

	return 0;
}
