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

// tr_skin.c

#include "tr_local.h"

/*
============================================================================

SKINS

============================================================================
*/

/*
==================
CommaParse

This is unfortunate, but the skin files aren't
compatable with our normal parsing rules.
==================
*/
static char    *CommaParse ( char **data_p )
{
	int         c = 0, len;
	char        *data;
	static char com_token[ MAX_TOKEN_CHARS ];

	data = *data_p;
	len = 0;
	com_token[ 0 ] = 0;

	// make sure incoming data is valid
	if ( !data )
	{
		*data_p = NULL;
		return com_token;
	}

	while ( 1 )
	{
		// skip whitespace
		while ( ( c = *data ) <= ' ' )
		{
			if ( !c )
			{
				break;
			}

			data++;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[ 1 ] == '/' )
		{
			while ( *data && *data != '\n' )
			{
				data++;
			}
		}
		// skip /* */ comments
		else if ( c == '/' && data[ 1 ] == '*' )
		{
			while ( *data && ( *data != '*' || data[ 1 ] != '/' ) )
			{
				data++;
			}

			if ( *data )
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	if ( c == 0 )
	{
		return "";
	}

	// handle quoted strings
	if ( c == '\"' )
	{
		data++;

		while ( 1 )
		{
			c = *data++;

			if ( c == '\"' || !c )
			{
				com_token[ len ] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}

			if ( len < MAX_TOKEN_CHARS )
			{
				com_token[ len ] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if ( len < MAX_TOKEN_CHARS )
		{
			com_token[ len ] = c;
			len++;
		}

		data++;
		c = *data;
	}
	while ( c > 32 && c != ',' );

	if ( len == MAX_TOKEN_CHARS )
	{
//      Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}

	com_token[ len ] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

/*
==============
RE_GetSkinModel
==============
*/
qboolean RE_GetSkinModel ( qhandle_t skinid, const char *type, char *name )
{
	int    i;
	int    hash;
	skin_t *skin;

	skin = tr.skins[ skinid ];
	hash = Com_HashKey ( ( char * ) type, strlen ( type ) );

	for ( i = 0; i < skin->numModels; i++ )
	{
		if ( hash != skin->models[ i ]->hash )
		{
			continue;
		}

		if ( !Q_stricmp ( skin->models[ i ]->type, type ) )
		{
			// (SA) whoops, should've been this way
			Q_strncpyz ( name, skin->models[ i ]->model, sizeof ( skin->models[ i ]->model ) );
			return qtrue;
		}
	}

	return qfalse;
}

/*
==============
RE_GetShaderFromModel
        return a shader index for a given model's surface
        'withlightmap' set to '0' will create a new shader that is a copy of the one found
        on the model, without the lighmap stage, if the shader has a lightmap stage

        NOTE: only works for bmodels right now.  Could modify for other models (md3's etc.)
==============
*/
qhandle_t RE_GetShaderFromModel ( qhandle_t modelid, int surfnum, int withlightmap )
{
	model_t    *model;
	bmodel_t   *bmodel;
	msurface_t *surf;
	shader_t   *shd;

	if ( surfnum < 0 )
	{
		surfnum = 0;
	}

	model = R_GetModelByHandle ( modelid ); // (SA) should be correct now

	if ( model )
	{
		bmodel = model->model.bmodel;

		if ( bmodel && bmodel->firstSurface )
		{
			if ( surfnum >= bmodel->numSurfaces )
			{
				// if it's out of range, return the first surface
				surfnum = 0;
			}

			surf = bmodel->firstSurface + surfnum;

			// RF, check for null shader (can happen on func_explosive's with botclips attached)
			if ( !surf->shader )
			{
				return 0;
			}

//          if(surf->shader->lightmapIndex != LIGHTMAP_NONE) {
			if ( surf->shader->lightmapIndex > LIGHTMAP_NONE )
			{
				image_t  *image;
				long     hash;
				qboolean mip = qtrue; // mip generation on by default

				// get mipmap info for original texture
				hash = GenerateImageHashValue ( surf->shader->name );

				for ( image = r_imageHashTable[ hash ]; image; image = image->next )
				{
					if ( !strcmp ( surf->shader->name, image->imgName ) )
					{
						mip = image->mipmap;
						break;
					}
				}

				shd = R_FindShader ( surf->shader->name, LIGHTMAP_NONE, mip );
				shd->stages[ 0 ]->rgbGen = CGEN_LIGHTING_DIFFUSE; // (SA) new
			}
			else
			{
				shd = surf->shader;
			}

			return shd->index;
		}
	}

	return 0;
}

//----(SA) end

/*
===============
RE_RegisterSkin

===============
*/
qhandle_t RE_RegisterSkin ( const char *name )
{
	qhandle_t     hSkin;
	skin_t        *skin;
	skinSurface_t *surf;
	skinModel_t   *model; //----(SA) added
	char          *text, *text_p;
	char          *token;
	char          surfName[ MAX_QPATH ];

	if ( !name || !name[ 0 ] )
	{
		Com_Printf ( "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( strlen ( name ) >= MAX_QPATH )
	{
		Com_Printf ( "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}

	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins; hSkin++ )
	{
		skin = tr.skins[ hSkin ];

		if ( !Q_stricmp ( skin->name, name ) )
		{
			if ( skin->numSurfaces == 0 )
			{
				return 0; // default skin
			}

			return hSkin;
		}
	}

	// allocate a new skin
	if ( tr.numSkins == MAX_SKINS )
	{
		ri.Printf ( PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}

//----(SA)  moved things around slightly to fix the problem where you restart
//          a map that has ai characters who had invalid skin names entered
//          in thier "skin" or "head" field

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// load and parse the skin file
	ri.FS_ReadFile ( name, ( void ** ) &text );

	if ( !text )
	{
		return 0;
	}

	tr.numSkins++;
	skin = ri.Hunk_Alloc ( sizeof ( skin_t ), h_low );
	tr.skins[ hSkin ] = skin;
	Q_strncpyz ( skin->name, name, sizeof ( skin->name ) );
	skin->numSurfaces = 0;
	skin->numModels = 0; //----(SA) added

//----(SA)  end

	text_p = text;

	while ( text_p && *text_p )
	{
		// get surface name
		token = CommaParse ( &text_p );
		Q_strncpyz ( surfName, token, sizeof ( surfName ) );

		if ( !token[ 0 ] )
		{
			break;
		}

		// lowercase the surface name so skin compares are faster
		Q_strlwr ( surfName );

		if ( *text_p == ',' )
		{
			text_p++;
		}

		if ( !Q_stricmpn ( token, "tag_", 4 ) )
		{
			continue;
		}

		if ( !Q_stricmpn ( token, "md3_", 4 ) )
		{
			// this is specifying a model
			model = skin->models[ skin->numModels ] = ri.Hunk_Alloc ( sizeof ( *skin->models[ 0 ] ), h_low );
			Q_strncpyz ( model->type, token, sizeof ( model->type ) );
			model->hash = Com_HashKey ( model->type, sizeof ( model->type ) );

			// get the model name
			token = CommaParse ( &text_p );

			Q_strncpyz ( model->model, token, sizeof ( model->model ) );

			skin->numModels++;
			continue;
		}

		// parse the shader name
		token = CommaParse ( &text_p );

		surf = skin->surfaces[ skin->numSurfaces ] = ri.Hunk_Alloc ( sizeof ( *skin->surfaces[ 0 ] ), h_low );
		Q_strncpyz ( surf->name, surfName, sizeof ( surf->name ) );
		surf->hash = Com_HashKey ( surf->name, sizeof ( surf->name ) );
		surf->shader = R_FindShader ( token, LIGHTMAP_NONE, qtrue );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile ( text );

	// never let a skin have 0 shaders
	if ( skin->numSurfaces == 0 )
	{
		return 0; // use default skin
	}

	return hSkin;
}

/*
===============
R_InitSkins
===============
*/
void R_InitSkins ( void )
{
	skin_t *skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[ 0 ] = ri.Hunk_Alloc ( sizeof ( skin_t ), h_low );
	Q_strncpyz ( skin->name, "<default skin>", sizeof ( skin->name ) );
	skin->numSurfaces = 1;
	skin->surfaces[ 0 ] = ri.Hunk_Alloc ( sizeof ( *skin->surfaces ), h_low );
	skin->surfaces[ 0 ]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t         *R_GetSkinByHandle ( qhandle_t hSkin )
{
	if ( hSkin < 1 || hSkin >= tr.numSkins )
	{
		return tr.skins[ 0 ];
	}

	return tr.skins[ hSkin ];
}

/*
===============
R_SkinList_f
===============
*/
void R_SkinList_f ( void )
{
	int    i, j;
	skin_t *skin;

	ri.Printf ( PRINT_ALL, "------------------\n" );

	for ( i = 0; i < tr.numSkins; i++ )
	{
		skin = tr.skins[ i ];

		ri.Printf ( PRINT_ALL, "%3i:%s\n", i, skin->name );

		for ( j = 0; j < skin->numSurfaces; j++ )
		{
			ri.Printf ( PRINT_ALL, "       %s = %s\n", skin->surfaces[ j ]->name, skin->surfaces[ j ]->shader->name );
		}
	}

	ri.Printf ( PRINT_ALL, "------------------\n" );
}
