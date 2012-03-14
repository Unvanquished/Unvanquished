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
// tr_models.c -- model loading and caching
#include "tr_local.h"

#define LL(x) x = LittleLong(x)
#define LF(x) x = LittleFloat(x)

qboolean        R_LoadMD3( model_t *mod, int lod, void *buffer, int bufferSize, const char *name );

#if defined( COMPAT_ET )
qboolean        R_LoadMDC( model_t *mod, int lod, void *buffer, int bufferSize, const char *name );
qboolean        R_LoadMDM( model_t *mod, void *buffer, const char *name );
static qboolean R_LoadMDX( model_t *mod, void *buffer, const char *name );

#endif

qboolean R_LoadMD5( model_t *mod, void *buffer, int bufferSize, const char *name );
qboolean R_LoadPSK( model_t *mod, void *buffer, int bufferSize, const char *name );

model_t  *loadmodel;

/*
** R_GetModelByHandle
*/
model_t        *R_GetModelByHandle( qhandle_t index )
{
	model_t *mod;

	// out of range gets the defualt model
	if ( index < 1 || index >= tr.numModels )
	{
		return tr.models[ 0 ];
	}

	mod = tr.models[ index ];

	return mod;
}

//===============================================================================

/*
** R_AllocModel
*/
model_t        *R_AllocModel( void )
{
	model_t *mod;

	if ( tr.numModels == MAX_MOD_KNOWN )
	{
		return NULL;
	}

	mod = ri.Hunk_Alloc( sizeof( *tr.models[ tr.numModels ] ), h_low );
	mod->index = tr.numModels;
	tr.models[ tr.numModels ] = mod;
	tr.numModels++;

	return mod;
}

/*
====================
RE_RegisterModel

Loads in a model for the given name

Zero will be returned if the model fails to load.
An entry will be retained for failed models as an
optimization to prevent disk rescanning if they are
asked for again.
====================
*/
qhandle_t RE_RegisterModel( const char *name )
{
	model_t   *mod;
	unsigned  *buffer;
	int       bufferLen = 0;
	int       lod;
	int       ident;
	qboolean  loaded;
	qhandle_t hModel;
	int       numLoaded;

	if ( !name || !name[ 0 ] )
	{
		ri.Printf( PRINT_ALL, "RE_RegisterModel: NULL name\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		Com_Printf( "Model name exceeds MAX_QPATH\n" );
		return 0;
	}

	// search the currently loaded models
	for ( hModel = 1; hModel < tr.numModels; hModel++ )
	{
		mod = tr.models[ hModel ];

		if ( !strcmp( mod->name, name ) )
		{
			if ( mod->type == MOD_BAD )
			{
				return 0;
			}

			return hModel;
		}
	}

	// allocate a new model_t
	if ( ( mod = R_AllocModel() ) == NULL )
	{
		ri.Printf( PRINT_WARNING, "RE_RegisterModel: R_AllocModel() failed for '%s'\n", name );
		return 0;
	}

	// only set the name after the model has been successfully loaded
	Q_strncpyz( mod->name, name, sizeof( mod->name ) );

	// make sure the render thread is stopped
	R_SyncRenderThread();

	mod->numLods = 0;

	// load the files
	numLoaded = 0;

#if defined( COMPAT_ET )

	if ( strstr( name, ".mds" ) || strstr( name, ".mdm" ) || strstr( name, ".mdx" ) || strstr( name, ".md5mesh" ) || strstr( name, ".psk" ) )
#else
	if ( strstr( name, ".md5mesh" ) || strstr( name, ".psk" ) )
#endif
	{
		// try loading skeletal file

		loaded = qfalse;
		bufferLen = ri.FS_ReadFile( name, ( void ** ) &buffer );

		if ( buffer )
		{
			loadmodel = mod;

			ident = LittleLong( * ( unsigned * ) buffer );
#if defined( COMPAT_ET )
#if 0

			if ( ident == MDS_IDENT )
			{
				loaded = R_LoadMDS( mod, buffer, name );
			}
			else
#endif
				if ( ident == MDM_IDENT )
				{
					loaded = R_LoadMDM( mod, buffer, name );
				}
				else if ( ident == MDX_IDENT )
				{
					loaded = R_LoadMDX( mod, buffer, name );
				}

#endif

#if defined( USE_REFENTITY_ANIMATIONSYSTEM )

			if ( !Q_stricmpn( ( const char * ) buffer, "MD5Version", 10 ) )
			{
				loaded = R_LoadMD5( mod, buffer, bufferLen, name );
			}
			else if ( !Q_stricmpn( ( const char * ) buffer, PSK_IDENTSTRING, PSK_IDENTLEN ) )
			{
				loaded = R_LoadPSK( mod, buffer, bufferLen, name );
			}

#endif
			ri.FS_FreeFile( buffer );
		}

		if ( loaded )
		{
			// make sure the VBO glState entries are save
			R_BindNullVBO();
			R_BindNullIBO();

			return mod->index;
		}
	}

	for ( lod = MD3_MAX_LODS - 1; lod >= 0; lod-- )
	{
		char filename[ 1024 ];

		strcpy( filename, name );

		if ( lod != 0 )
		{
			char namebuf[ 80 ];

			if ( strrchr( filename, '.' ) )
			{
				*strrchr( filename, '.' ) = 0;
			}

			sprintf( namebuf, "_%d.md3", lod );
			strcat( filename, namebuf );
		}

		filename[ strlen( filename ) - 1 ] = 'c';  // try MDC first
		ri.FS_ReadFile( filename, ( void ** ) &buffer );

		if ( !buffer )
		{
			filename[ strlen( filename ) - 1 ] = '3';  // try MD3 second
			ri.FS_ReadFile( filename, ( void ** ) &buffer );

			if ( !buffer )
			{
				continue;
			}
		}

		loadmodel = mod;

		ident = LittleLong( * ( unsigned * ) buffer );

		if ( ident == MD3_IDENT )
		{
			loaded = R_LoadMD3( mod, lod, buffer, bufferLen, name );
			ri.FS_FreeFile( buffer );
		}

#if defined( COMPAT_ET )
		else if ( ident == MDC_IDENT )
		{
			loaded = R_LoadMDC( mod, lod, buffer, bufferLen, name );
			ri.FS_FreeFile( buffer );
		}

#endif
		else
		{
			ri.FS_FreeFile( buffer );

			ri.Printf( PRINT_WARNING, "RE_RegisterModel: unknown fileid for %s\n", name );
			goto fail;
		}

		if ( !loaded )
		{
			if ( lod == 0 )
			{
				goto fail;
			}
			else
			{
				break;
			}
		}
		else
		{
			// make sure the VBO glState entries are save
			R_BindNullVBO();
			R_BindNullIBO();

			mod->numLods++;
			numLoaded++;
			// if we have a valid model and are biased
			// so that we won't see any higher detail ones,
			// stop loading them
//          if ( lod <= r_lodbias->integer ) {
//              break;
//          }
		}
	}

	// make sure the VBO glState entries are save
	R_BindNullVBO();
	R_BindNullIBO();

	if ( numLoaded )
	{
		// duplicate into higher lod spots that weren't
		// loaded, in case the user changes r_lodbias on the fly
		for ( lod--; lod >= 0; lod-- )
		{
			mod->numLods++;
			mod->mdv[ lod ] = mod->mdv[ lod + 1 ];
		}

		return mod->index;
	}

#ifdef _DEBUG
	else
	{
		ri.Printf( PRINT_WARNING, "couldn't load '%s'\n", name );
	}

#endif

fail:
	// we still keep the model_t around, so if the model name is asked for
	// again, we won't bother scanning the filesystem
	mod->type = MOD_BAD;

	// make sure the VBO glState entries are save
	R_BindNullVBO();
	R_BindNullIBO();

	return 0;
}

/*
=================
R_LoadMDX
=================
*/
#if defined( COMPAT_ET )
static qboolean R_LoadMDX( model_t *mod, void *buffer, const char *mod_name )
{
	int           i, j;
	mdxHeader_t   *pinmodel, *mdx;
	mdxFrame_t    *frame;
	short         *bframe;
	mdxBoneInfo_t *bi;
	int           version;
	int           size;
	int           frameSize;

	pinmodel = ( mdxHeader_t * ) buffer;

	version = LittleLong( pinmodel->version );

	if ( version != MDX_VERSION )
	{
		ri.Printf( PRINT_WARNING, "R_LoadMDX: %s has wrong version (%i should be %i)\n", mod_name, version, MDX_VERSION );
		return qfalse;
	}

	mod->type = MOD_MDX;
	size = LittleLong( pinmodel->ofsEnd );
	mod->dataSize += size;
	mdx = mod->mdx = ri.Hunk_Alloc( size, h_low );

	memcpy( mdx, buffer, LittleLong( pinmodel->ofsEnd ) );

	LL( mdx->ident );
	LL( mdx->version );
	LL( mdx->numFrames );
	LL( mdx->numBones );
	LL( mdx->ofsFrames );
	LL( mdx->ofsBones );
	LL( mdx->ofsEnd );
	LL( mdx->torsoParent );

	if ( LittleLong( 1 ) != 1 )
	{
		// swap all the frames
		frameSize = ( int )( sizeof( mdxBoneFrameCompressed_t ) ) * mdx->numBones;

		for ( i = 0; i < mdx->numFrames; i++ )
		{
			frame = ( mdxFrame_t * )( ( byte * ) mdx + mdx->ofsFrames + i * frameSize + i * sizeof( mdxFrame_t ) );
			frame->radius = LittleFloat( frame->radius );

			for ( j = 0; j < 3; j++ )
			{
				frame->bounds[ 0 ][ j ] = LittleFloat( frame->bounds[ 0 ][ j ] );
				frame->bounds[ 1 ][ j ] = LittleFloat( frame->bounds[ 1 ][ j ] );
				frame->localOrigin[ j ] = LittleFloat( frame->localOrigin[ j ] );
				frame->parentOffset[ j ] = LittleFloat( frame->parentOffset[ j ] );
			}

			bframe = ( short * )( ( byte * ) mdx + mdx->ofsFrames + i * frameSize + ( ( i + 1 ) * sizeof( mdxFrame_t ) ) );

			for ( j = 0; j < mdx->numBones * sizeof( mdxBoneFrameCompressed_t ) / sizeof( short ); j++ )
			{
				( ( short * ) bframe ) [ j ] = LittleShort( ( ( short * ) bframe ) [ j ] );
			}
		}

		// swap all the bones
		for ( i = 0; i < mdx->numBones; i++ )
		{
			bi = ( mdxBoneInfo_t * )( ( byte * ) mdx + mdx->ofsBones + i * sizeof( mdxBoneInfo_t ) );
			LL( bi->parent );
			bi->torsoWeight = LittleFloat( bi->torsoWeight );
			bi->parentDist = LittleFloat( bi->parentDist );
			LL( bi->flags );
		}
	}

	return qtrue;
}

#endif // #if defined(COMPAT_ET)

//=============================================================================

/*
=================
R_XMLError
=================
*/
void __attribute__( ( format( printf, 2, 3 ) ) ) R_XMLError( void *ctx, const char *fmt, ... )
{
	va_list     argptr;
	static char msg[ 4096 ];

	va_start( argptr, fmt );
	Q_vsnprintf( msg, sizeof( msg ), fmt, argptr );
	va_end( argptr );

	ri.Printf( PRINT_WARNING, "%s", msg );
}

/*
=================
R_LoadDAE
=================
*/

/*
static qboolean R_LoadDAE(model_t * mod, void *buffer, int bufferLen, const char *modName)
{
        xmlDocPtr       doc;
        xmlNodePtr      node;

        // setup error function handler
        xmlInitParser();
        xmlSetGenericErrorFunc(NULL, R_XMLError);

        ri.Printf(PRINT_ALL, "...loading DAE '%s'\n", modName);

        doc = xmlParseMemory(buffer, bufferLen);
        if(doc == NULL)
        {
                ri.Printf(PRINT_WARNING, "R_LoadDAE: '%s' xmlParseMemory returned NULL\n", modName);
                return qfalse;
        }
        node = xmlDocGetRootElement(doc);

        if(node == NULL)
        {
                ri.Printf(PRINT_WARNING, "R_LoadDAE: '%s' empty document\n", modName);
                xmlFreeDoc(doc);
                return qfalse;
        }

        if(xmlStrcmp(node->name, (const xmlChar *) "COLLADA"))
        {
                ri.Printf(PRINT_WARNING, "R_LoadDAE: '%s' document of the wrong type, root node != COLLADA", modName);
                xmlFreeDoc(doc);
                return qfalse;
        }

        //TODO

        xmlFreeDoc(doc);

        ri.Printf(PRINT_ALL, "...finished DAE '%s'\n", modName);

        return qfalse;
}
*/

//=============================================================================

/*
** RE_BeginRegistration
*/
qboolean RE_BeginRegistration( glconfig_t *glconfigOut, glconfig2_t *glconfig2Out )
{
	if ( !R_Init() )
	{
		return qfalse;
	}

	*glconfigOut = glConfig;
	*glconfig2Out = glConfig2;

	R_SyncRenderThread();

	tr.visIndex = 0;
	memset( tr.visClusters, -2, sizeof( tr.visClusters ) );   // force markleafs to regenerate

#if defined( USE_D3D10 )
	// TODO
#else
	R_ClearFlares();
#endif

	RE_ClearScene();

	// HACK: give world entity white color for "colored" shader keyword
	tr.worldEntity.e.shaderRGBA[ 0 ] = 255;
	tr.worldEntity.e.shaderRGBA[ 1 ] = 255;
	tr.worldEntity.e.shaderRGBA[ 2 ] = 255;
	tr.worldEntity.e.shaderRGBA[ 3 ] = 255;

	tr.worldEntity.e.nonNormalizedAxes = qfalse;

	// RB: world will be never ignored by occusion query test
	tr.worldEntity.occlusionQuerySamples = 1;

	tr.registered = qtrue;

	// NOTE: this sucks, for some reason the first stretch pic is never drawn
	// without this we'd see a white flash on a level load because the very
	// first time the level shot would not be drawn
	RE_StretchPic( 0, 0, 0, 0, 0, 0, 1, 1, 0 );

	return qtrue;
}

//=============================================================================

/*
===============
R_ModelInit
===============
*/
void R_ModelInit( void )
{
	model_t *mod;

	// leave a space for NULL model
	tr.numModels = 0;

	mod = R_AllocModel();
	mod->type = MOD_BAD;
}

/*
================
R_Modellist_f
================
*/
void R_Modellist_f( void )
{
	int      i, j, k;
	model_t  *mod;
	int      total;
	int      totalDataSize;
	qboolean showFrames;

	if ( !strcmp( ri.Cmd_Argv( 1 ), "frames" ) )
	{
		showFrames = qtrue;
	}
	else
	{
		showFrames = qfalse;
	}

	total = 0;
	totalDataSize = 0;

	for ( i = 1; i < tr.numModels; i++ )
	{
		mod = tr.models[ i ];

		if ( mod->type == MOD_MESH )
		{
			for ( j = 0; j < MD3_MAX_LODS; j++ )
			{
				if ( mod->mdv[ j ] && mod->mdv[ j ] != mod->mdv[ j - 1 ] )
				{
					mdvModel_t   *mdvModel;
					mdvSurface_t *mdvSurface;
					mdvTagName_t *mdvTagName;

					mdvModel = mod->mdv[ j ];

					total++;
					ri.Printf( PRINT_ALL, "%d.%02d MB '%s' LOD = %i\n",      mod->dataSize / ( 1024 * 1024 ),
					           ( mod->dataSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ),
					           mod->name, j );

					if ( showFrames && mdvModel->numFrames > 1 )
					{
						ri.Printf( PRINT_ALL, "\tnumSurfaces = %i\n", mdvModel->numSurfaces );
						ri.Printf( PRINT_ALL, "\tnumFrames = %i\n", mdvModel->numFrames );

						for ( k = 0, mdvSurface = mdvModel->surfaces; k < mdvModel->numSurfaces; k++, mdvSurface++ )
						{
							ri.Printf( PRINT_ALL, "\t\tmesh = '%s'\n", mdvSurface->name );
							ri.Printf( PRINT_ALL, "\t\t\tnumVertexes = %i\n", mdvSurface->numVerts );
							ri.Printf( PRINT_ALL, "\t\t\tnumTriangles = %i\n", mdvSurface->numTriangles );
						}
					}

					ri.Printf( PRINT_ALL, "\t\tnumTags = %i\n", mdvModel->numTags );

					for ( k = 0, mdvTagName = mdvModel->tagNames; k < mdvModel->numTags; k++, mdvTagName++ )
					{
						ri.Printf( PRINT_ALL, "\t\t\ttagName = '%s'\n", mdvTagName->name );
					}
				}
			}
		}
		else
		{
			ri.Printf( PRINT_ALL, "%d.%02d MB '%s'\n",       mod->dataSize / ( 1024 * 1024 ),
			           ( mod->dataSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ),
			           mod->name );

			total++;
		}

		totalDataSize += mod->dataSize;
	}

	ri.Printf( PRINT_ALL, " %d.%02d MB total model memory\n", totalDataSize / ( 1024 * 1024 ),
	           ( totalDataSize % ( 1024 * 1024 ) ) * 100 / ( 1024 * 1024 ) );
	ri.Printf( PRINT_ALL, " %i total models\n\n", total );

#if     0 // not working right with new hunk

	if ( tr.world )
	{
		ri.Printf( PRINT_ALL, "\n%8i : %s\n", tr.world->dataSize, tr.world->name );
	}

#endif
}

//=============================================================================

/*
================
R_GetTag
================
*/
static int R_GetTag( mdvModel_t *model, int frame, const char *_tagName, int startTagIndex, mdvTag_t **outTag )
{
	int          i;
	mdvTag_t     *tag;
	mdvTagName_t *tagName;

	// it is possible to have a bad frame while changing models, so don't error
	frame = Q_bound( 0, frame, model->numFrames - 1 );

	if ( startTagIndex > model->numTags )
	{
		*outTag = NULL;
		return -1;
	}

#if 1
	tag = model->tags + frame * model->numTags;
	tagName = model->tagNames;

	for ( i = 0; i < model->numTags; i++, tag++, tagName++ )
	{
		if ( ( i >= startTagIndex ) && !strcmp( tagName->name, _tagName ) )
		{
			*outTag = tag;
			return i;
		}
	}

#endif

	*outTag = NULL;
	return -1;
}

/*
================
RE_LerpTagQ3A
================
*/
int RE_LerpTagQ3A( orientation_t *tag, qhandle_t handle, int startFrame, int endFrame, float frac, const char *tagNameIn )
{
	mdvTag_t *start, *end;
	int      i;
	float    frontLerp, backLerp;
	model_t  *model;
	char     tagName[ MAX_QPATH ];
	int      retval;

	Q_strncpyz( tagName, tagNameIn, MAX_QPATH );

	model = R_GetModelByHandle( handle );

	if ( !model->mdv[ 0 ] )
	{
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return -1;
	}

	start = end = NULL;

	retval = R_GetTag( model->mdv[ 0 ], startFrame, tagName, 0, &start );
	retval = R_GetTag( model->mdv[ 0 ], endFrame, tagName, 0, &end );

	if ( !start || !end )
	{
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return -1;
	}

	frontLerp = frac;
	backLerp = 1.0f - frac;

	for ( i = 0; i < 3; i++ )
	{
		tag->origin[ i ] = start->origin[ i ] * backLerp + end->origin[ i ] * frontLerp;
		tag->axis[ 0 ][ i ] = start->axis[ 0 ][ i ] * backLerp + end->axis[ 0 ][ i ] * frontLerp;
		tag->axis[ 1 ][ i ] = start->axis[ 1 ][ i ] * backLerp + end->axis[ 1 ][ i ] * frontLerp;
		tag->axis[ 2 ][ i ] = start->axis[ 2 ][ i ] * backLerp + end->axis[ 2 ][ i ] * frontLerp;
	}

	VectorNormalize( tag->axis[ 0 ] );
	VectorNormalize( tag->axis[ 1 ] );
	VectorNormalize( tag->axis[ 2 ] );
	return retval;
}

/*
================
RE_LerpTag
================
*/
#if defined( COMPAT_ET )
int RE_LerpTagET( orientation_t *tag, const refEntity_t *refent, const char *tagNameIn, int startIndex )
{
	mdvTag_t  *start, *end;
	int       i;
	float     frontLerp, backLerp;
	model_t   *model;
	char      tagName[ MAX_QPATH ]; //, *ch;
	int       retval;
	qhandle_t handle;
	int       startFrame, endFrame;
	float     frac;

	handle = refent->hModel;
	startFrame = refent->oldframe;
	endFrame = refent->frame;
	frac = 1.0 - refent->backlerp;

	Q_strncpyz( tagName, tagNameIn, MAX_QPATH );

	/*
	        // if the tagName has a space in it, then it is passing through the starting tag number
	        if (ch = strrchr(tagName, ' ')) {
	                *ch = 0;
	                ch++;
	                startIndex = atoi(ch);
	        }
	*/
	model = R_GetModelByHandle( handle );

	/*
	if(!model->mdv[0]) //if(!model->model.md3[0] && !model->model.mdc[0] && !model->model.mds)
	{
	        AxisClear(tag->axis);
	        VectorClear(tag->origin);
	        return -1;
	}
	*/

	frontLerp = frac;
	backLerp = 1.0 - frac;

	start = end = NULL;

	if ( model->type == MOD_MESH )
	{
		// old MD3 style
		retval = R_GetTag( model->mdv[ 0 ], startFrame, tagName, startIndex, &start );
		retval = R_GetTag( model->mdv[ 0 ], endFrame, tagName, startIndex, &end );
	}

	/*
	        else if(model->type == MOD_MDS)
	        {
	                // use bone lerping
	                retval = R_GetBoneTag(tag, model->model.mds, startIndex, refent, tagNameIn);

	                if(retval >= 0)
	                {
	                        return retval;
	                }

	                // failed
	                return -1;

	        }
	        */
	else if ( model->type == MOD_MDM )
	{
		// use bone lerping
		retval = R_MDM_GetBoneTag( tag, model->mdm, startIndex, refent, tagNameIn );

		if ( retval >= 0 )
		{
			return retval;
		}

		// failed
		return -1;
	}
	else if ( model->type == MOD_MD5 )
	{
		vec3_t tmp;

		retval = RE_BoneIndex( handle, tagName );

		if ( retval <= 0 )
		{
			return -1;
		}

		VectorCopy( refent->skeleton.bones[ retval ].origin, tag->origin );
		QuatToAxis( refent->skeleton.bones[ retval ].rotation, tag->axis );
		VectorCopy( tag->axis[ 2 ], tmp );
		VectorCopy( tag->axis[ 1 ], tag->axis[ 2 ] );
		VectorCopy( tag->axis[ 0 ], tag->axis[ 1 ] );
		VectorCopy( tmp, tag->axis[ 0 ] );
		return retval;
	}

	/*
	else
	{
	        // psuedo-compressed MDC tags
	        mdcTag_t       *cstart, *cend;

	        retval = R_GetMDCTag((byte *) model->model.mdc[0], startFrame, tagName, startIndex, &cstart);
	        retval = R_GetMDCTag((byte *) model->model.mdc[0], endFrame, tagName, startIndex, &cend);

	        // uncompress the MDC tags into MD3 style tags
	        if(cstart && cend)
	        {
	                for(i = 0; i < 3; i++)
	                {
	                        ustart.origin[i] = (float)cstart->xyz[i] * MD3_XYZ_SCALE;
	                        uend.origin[i] = (float)cend->xyz[i] * MD3_XYZ_SCALE;
	                        sangles[i] = (float)cstart->angles[i] * MDC_TAG_ANGLE_SCALE;
	                        eangles[i] = (float)cend->angles[i] * MDC_TAG_ANGLE_SCALE;
	                }

	                AnglesToAxis(sangles, ustart.axis);
	                AnglesToAxis(eangles, uend.axis);

	                start = &ustart;
	                end = &uend;
	        }
	        else
	        {
	                start = NULL;
	                end = NULL;
	        }
	}
	*/

	if ( !start || !end )
	{
		AxisClear( tag->axis );
		VectorClear( tag->origin );
		return -1;
	}

	for ( i = 0; i < 3; i++ )
	{
		tag->origin[ i ] = start->origin[ i ] * backLerp + end->origin[ i ] * frontLerp;
		tag->axis[ 0 ][ i ] = start->axis[ 0 ][ i ] * backLerp + end->axis[ 0 ][ i ] * frontLerp;
		tag->axis[ 1 ][ i ] = start->axis[ 1 ][ i ] * backLerp + end->axis[ 1 ][ i ] * frontLerp;
		tag->axis[ 2 ][ i ] = start->axis[ 2 ][ i ] * backLerp + end->axis[ 2 ][ i ] * frontLerp;
	}

	VectorNormalize( tag->axis[ 0 ] );
	VectorNormalize( tag->axis[ 1 ] );
	VectorNormalize( tag->axis[ 2 ] );

	return retval;
}

#endif

/*
================
RE_BoneIndex
================
*/
int RE_BoneIndex( qhandle_t hModel, const char *boneName )
{
	int        i;
	md5Bone_t  *bone;
	md5Model_t *md5;
	model_t    *model;

	model = R_GetModelByHandle( hModel );

	if ( !model->md5 )
	{
		return -1;
	}
	else
	{
		md5 = model->md5;
	}

	for ( i = 0, bone = md5->bones; i < md5->numBones; i++, bone++ )
	{
		if ( !Q_stricmp( bone->name, boneName ) )
		{
			return i;
		}
	}

	return -1;
}

/*
====================
R_ModelBounds
====================
*/
void R_ModelBounds( qhandle_t handle, vec3_t mins, vec3_t maxs )
{
	model_t    *model;
	mdvModel_t *header;
	mdvFrame_t *frame;

	model = R_GetModelByHandle( handle );

	if ( model->bsp )
	{
		VectorCopy( model->bsp->bounds[ 0 ], mins );
		VectorCopy( model->bsp->bounds[ 1 ], maxs );
	}
	else if ( model->mdv[ 0 ] )
	{
		header = model->mdv[ 0 ];

		frame = header->frames;

		VectorCopy( frame->bounds[ 0 ], mins );
		VectorCopy( frame->bounds[ 1 ], maxs );
	}
	else if ( model->md5 )
	{
		VectorCopy( model->md5->bounds[ 0 ], mins );
		VectorCopy( model->md5->bounds[ 1 ], maxs );
	}
	else
	{
		VectorClear( mins );
		VectorClear( maxs );
	}
}
