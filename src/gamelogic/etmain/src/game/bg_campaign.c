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

#include "../../../../engine/qcommon/q_shared.h"
//#include "bg_public.h"

#ifdef GAMEDLL
#include "g_local.h"
#else
#ifdef CGAMEDLL
#include "../cgame/cg_local.h"
#else
#include "../ui/ui_local.h"
#endif
#endif

// Campaign File Handling

// Saves
// FIXME: need byteswapping for macs

qboolean BG_LoadCampaignSave ( const char *filename, cpsFile_t *file, const char *profile )
{
	fileHandle_t f;
	long         hash;
	char         *ch;
	int          i, j;

	memset ( file, 0, sizeof ( cpsFile_t ) );

	// open the file
	if ( trap_FS_FOpenFile ( filename, &f, FS_READ ) < 0 )
	{
		return ( qfalse );
	}

	// read the header
	trap_FS_Read ( &file->header.ident, sizeof ( int ), f );

	if ( file->header.ident != CPS_IDENT )
	{
		trap_FS_FCloseFile ( f );
		Com_Printf ( "^1ERROR: BG_LoadCampaignSave: not a campaignsave\n" );
		return ( qfalse );
	}

	trap_FS_Read ( &file->header.version, 1, f );
	trap_FS_Read ( &file->header.numCampaigns, sizeof ( int ), f );
	trap_FS_Read ( &file->header.profileHash, sizeof ( int ), f );

	// generate hash for profile
	for ( hash = 0, ch = ( char * ) profile; *ch != '\0'; ch++ )
	{
		hash += ( long ) ( tolower ( *ch ) ) * ( ( ch - profile ) + 119 );
	}

	if ( file->header.profileHash != hash )
	{
		trap_FS_FCloseFile ( f );
		Com_Printf ( "^1WARNING: BG_LoadCampaignSave: campaignsave is for another profile\n" );
		return ( qfalse );
	}

	// read the campaigns and maps
	for ( i = 0; i < file->header.numCampaigns; i++ )
	{
		trap_FS_Read ( &file->campaigns[ i ].shortnameHash, sizeof ( int ), f );
		trap_FS_Read ( &file->campaigns[ i ].progress, sizeof ( int ), f );

		// all completed maps
		for ( j = 0; j < file->campaigns[ i ].progress; j++ )
		{
			trap_FS_Read ( &file->campaigns[ i ].maps[ j ].mapnameHash, sizeof ( int ), f );
		}
	}

	// done
	trap_FS_FCloseFile ( f );

	return ( qtrue );
}

qboolean BG_StoreCampaignSave ( const char *filename, cpsFile_t *file, const char *profile )
{
	fileHandle_t f;
	long         hash;
	char         *ch;
	int          i, j;

	// open the file
	if ( trap_FS_FOpenFile ( filename, &f, FS_WRITE ) < 0 )
	{
		return ( qfalse );
	}

	// write the header
	file->header.ident = CPS_IDENT;
	file->header.version = CPS_VERSION;

	trap_FS_Write ( &file->header.ident, sizeof ( int ), f );
	trap_FS_Write ( &file->header.version, 1, f );
	trap_FS_Write ( &file->header.numCampaigns, sizeof ( int ), f );

	// generate hash for profile
	for ( hash = 0, ch = ( char * ) profile; *ch != '\0'; ch++ )
	{
		hash += ( long ) ( tolower ( *ch ) ) * ( ( ch - profile ) + 119 );
	}

	file->header.profileHash = ( int ) hash;

	trap_FS_Write ( &file->header.profileHash, sizeof ( int ), f );

	// write the campaigns and maps
	for ( i = 0; i < file->header.numCampaigns; i++ )
	{
		trap_FS_Write ( &file->campaigns[ i ].shortnameHash, sizeof ( int ), f );
		trap_FS_Write ( &file->campaigns[ i ].progress, sizeof ( int ), f );

		// all completed maps
		for ( j = 0; j < file->campaigns[ i ].progress; j++ )
		{
			trap_FS_Write ( &file->campaigns[ i ].maps[ j ].mapnameHash, sizeof ( int ), f );
		}
	}

	// done
	trap_FS_FCloseFile ( f );

	return ( qtrue );
}
