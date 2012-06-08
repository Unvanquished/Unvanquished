/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

extern "C"
{
#include "q_shared.h"
#include "qcommon.h"
#include "../../libs/findlocale/findlocale.h"
}

#include "../../libs/tinygettext/tinygettext.hpp"
#include <sstream>

using namespace tinygettext;

DictionaryManager trans_manager;
DictionaryManager trans_managergame;
Dictionary        trans_dict;
Dictionary        trans_dictgame;

cvar_t            *language;
bool              enabled = false;

#define _(x) Trans_Gettext(x)

extern "C" void Trans_Init( void )
{
	char      **poFiles;
	int       numPoFiles, i;
	FL_Locale *locale;
	
	language = Cvar_Get( "language", "", CVAR_ARCHIVE );
	
	FL_FindLocale( &locale, FL_MESSAGES );
	
	// Invalid or not found. Just use builtin language.
	if( !locale->lang || !locale->lang[0] || !locale->country || !locale->country[0] ) 
	{
		Cvar_Set( "language", "en_US" );
	}
		
	poFiles = FS_ListFiles( "translation/client", ".po", &numPoFiles );
	
	// This assumes that the names in both folders are the same
	for( i = 0; i < numPoFiles; i++ )
	{
		int ret;
		Dictionary *dict1;
		Dictionary *dict2;
		char *buffer, language[ 6 ];
		
		if( FS_ReadFile( va( "translation/client/%s", poFiles[ i ] ), ( void ** ) &buffer ) > 0 )
		{
			dict1 = new Dictionary();
			COM_StripExtension2( poFiles[ i ], language, sizeof( language ) );
			std::stringstream ss( buffer );
			trans_manager.add_po( poFiles[ i ], ss, Language::from_env( std::string( language ) ) );
			FS_FreeFile( buffer );
		}
		else
		{
			Com_Printf( "^1ERROR: Could not open client translation: %s\n", poFiles[ i ] );
		}
		
		if( FS_ReadFile( va( "translation/game/%s", poFiles[ i ] ), ( void ** ) &buffer ) > 0 )
		{
			dict2 = new Dictionary();
			COM_StripExtension2( poFiles[ i ], language, sizeof( language ) );
			std::stringstream ss( buffer );
			trans_managergame.add_po( poFiles[ i ], ss, Language::from_env( std::string( language ) ) );
			FS_FreeFile( buffer );
		}
		else
		{
			Com_Printf( "^1ERROR: Could not open game translation: %s\n", poFiles[ i ] );
		}
	}
	FS_FreeFileList( poFiles );
	trans_dict = trans_manager.get_dictionary( Language::from_env( std::string( language->string ) ) );
	trans_dictgame = trans_managergame.get_dictionary( Language::from_env( std::string( language->string ) ) );
	enabled = true;
	Com_Printf( "Loaded %lu language(s)\n", trans_manager.get_languages().size() );
}

extern "C" const char* Trans_Gettext( const char *msgid )
{
	if( !enabled ) { return msgid; }
	return trans_dict.translate( std::string( msgid ) ).c_str();
}

extern "C" const char* Trans_GettextGame( const char *msgid )
{
	if( !enabled ) { return msgid; }
	return trans_dictgame.translate( std::string( msgid ) ).c_str();
}

extern "C" const char* Trans_GettextPlural( const char *msgid, const char *msgid_plural, int num )
{
	if( !enabled ) { return num == 1 ? msgid : msgid_plural; }
	return trans_dict.translate_plural( std::string( msgid ), std::string( msgid_plural ), num ).c_str();
}

extern "C" const char* Trans_GettextGamePlural( const char *msgid, const char *msgid_plural, int num )
{
	if( !enabled ) { return num == 1 ? msgid : msgid_plural; }
	return trans_dictgame.translate_plural( std::string( msgid ), std::string( msgid_plural ), num ).c_str();
}
