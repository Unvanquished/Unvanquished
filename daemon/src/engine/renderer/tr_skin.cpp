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
compatible with our engine's main script/source parsing rules.
==================
*/
static const char *CommaParse( char **data_p )
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
		*data_p = nullptr;
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
				*data_p = data;
				return com_token;
			}

			if ( len < MAX_TOKEN_CHARS - 1 )
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
		len = 0;
	}

	com_token[ len ] = 0;

	*data_p = data;
	return com_token;
}

/*
===============
RE_RegisterSkin

===============
*/
qhandle_t RE_RegisterSkin( const char *name )
{
	qhandle_t     hSkin;
	skin_t        *skin;
	skinSurface_t *surf;
	skinModel_t   *model; //----(SA) added
	char          *text, *text_p;
	const char    *token;
	char          surfName[ MAX_QPATH ];

	if ( !name || !name[ 0 ] )
	{
		Com_Printf( "Empty name passed to RE_RegisterSkin\n" );
		return 0;
	}

	if ( strlen( name ) >= MAX_QPATH )
	{
		Com_Printf( "Skin name exceeds MAX_QPATH\n" );
		return 0;
	}

	// see if the skin is already loaded
	for ( hSkin = 1; hSkin < tr.numSkins; hSkin++ )
	{
		skin = tr.skins[ hSkin ];

		if ( !Q_stricmp( skin->name, name ) )
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
		ri.Printf(printParm_t::PRINT_WARNING, "WARNING: RE_RegisterSkin( '%s' ) MAX_SKINS hit\n", name );
		return 0;
	}

//----(SA)  moved things around slightly to fix the problem where you restart
//          a map that has ai characters who had invalid skin names entered
//          in thier "skin" or "head" field

	// make sure the render thread is stopped
	R_SyncRenderThread();

	// load and parse the skin file
	ri.FS_ReadFile( name, ( void ** ) &text );

	if ( !text )
	{
		return 0;
	}

	tr.numSkins++;
	skin = (skin_t*) ri.Hunk_Alloc( sizeof( skin_t ), ha_pref::h_low );
	tr.skins[ hSkin ] = skin;
	Q_strncpyz( skin->name, name, sizeof( skin->name ) );
	skin->numSurfaces = 0;
	skin->numModels = 0; //----(SA) added

//----(SA)  end

	text_p = text;

	while ( text_p && *text_p )
	{
		// get surface name
		token = CommaParse( &text_p );
		Q_strncpyz( surfName, token, sizeof( surfName ) );

		if ( !token[ 0 ] )
		{
			break;
		}

		// lowercase the surface name so skin compares are faster
		Q_strlwr( surfName );

		if ( *text_p == ',' )
		{
			text_p++;
		}

		if ( !Q_strnicmp( token, "tag_", 4 ) )
		{
			continue;
		}

		if ( !Q_strnicmp( token, "md3_", 4 ) )
		{
			// this is specifying a model
			model = skin->models[ skin->numModels ] = (skinModel_t*) ri.Hunk_Alloc( sizeof( *skin->models[ 0 ] ), ha_pref::h_low );
			Q_strncpyz( model->type, token, sizeof( model->type ) );
			model->hash = Com_HashKey( model->type, sizeof( model->type ) );

			// get the model name
			token = CommaParse( &text_p );

			Q_strncpyz( model->model, token, sizeof( model->model ) );

			skin->numModels++;
			continue;
		}

		// parse the shader name
		token = CommaParse( &text_p );

		surf = skin->surfaces[ skin->numSurfaces ] = (skinSurface_t*) ri.Hunk_Alloc( sizeof( *skin->surfaces[ 0 ] ), ha_pref::h_low );
		Q_strncpyz( surf->name, surfName, sizeof( surf->name ) );

		// RB: bspSurface_t does not have ::hash yet
		surf->shader = R_FindShader( token, shaderType_t::SHADER_3D_DYNAMIC, RSF_DEFAULT );
		skin->numSurfaces++;
	}

	ri.FS_FreeFile( text );

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
void R_InitSkins()
{
	skin_t *skin;

	tr.numSkins = 1;

	// make the default skin have all default shaders
	skin = tr.skins[ 0 ] = (skin_t*) ri.Hunk_Alloc( sizeof( skin_t ), ha_pref::h_low );
	Q_strncpyz( skin->name, "<default skin>", sizeof( skin->name ) );
	skin->numSurfaces = 1;
	skin->surfaces[ 0 ] = (skinSurface_t*) ri.Hunk_Alloc( sizeof( *skin->surfaces[ 0 ] ), ha_pref::h_low );
	skin->surfaces[ 0 ]->shader = tr.defaultShader;
}

/*
===============
R_GetSkinByHandle
===============
*/
skin_t         *R_GetSkinByHandle( qhandle_t hSkin )
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
void R_SkinList_f()
{
	int    i, j;
	skin_t *skin;

	ri.Printf(printParm_t::PRINT_ALL, "------------------\n" );

	for ( i = 0; i < tr.numSkins; i++ )
	{
		skin = tr.skins[ i ];

		ri.Printf(printParm_t::PRINT_ALL, "%3i:%s\n", i, skin->name );

		for ( j = 0; j < skin->numSurfaces; j++ )
		{
			ri.Printf(printParm_t::PRINT_ALL, "       %s = %s\n", skin->surfaces[ j ]->name, skin->surfaces[ j ]->shader->name );
		}
	}

	ri.Printf(printParm_t::PRINT_ALL, "------------------\n" );
}
