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
}

#include "../../libs/tinygettext/tinygettext.hpp"
#include <sstream>

using namespace tinygettext;

DictionaryManager trans_manager;
Dictionary        trans_dict;
Language          trans_lang;
cvar_t            *language;
bool              enabled = false;

#define _(x) Trans_Gettext(x)

extern "C" void Trans_Init( void )
{
	char **poFiles;
	int  numPoFiles, i;
	
	language = Cvar_Get( "language", "en_us", CVAR_ARCHIVE ); // FIXME:Should probably detect based on users locale
	poFiles = FS_ListFiles( "translation/client", ".po", &numPoFiles );
	
	for( i = 0; i < numPoFiles; i++ )
	{
		Dictionary *dict = new Dictionary();
		char *buffer, language[ 6 ];
		
		FS_ReadFile( va( "translation/client/%s", poFiles[ i ] ), ( void ** ) &buffer );
		//TODO: Error checking
		COM_StripExtension2( poFiles[ i ], language, sizeof( language ) );
		std::stringstream ss( buffer );
		trans_manager.add_po( poFiles[ i ], ss, Language::from_env( std::string( language ) ) );
		Hunk_FreeTempMemory( buffer );
	}
	FS_FreeFileList( poFiles );
	trans_dict = trans_manager.get_dictionary( Language::from_env( std::string( language->string ) ) );
	enabled = true;
	Com_Printf( "Loaded %d language(s)\n", numPoFiles );
}

extern "C" const char* Trans_Gettext( const char *msgid )
{
	if( !enabled ) { return msgid; }
	return trans_dict.translate( std::string( msgid ) ).c_str();
}

extern "C" void Test_Translation( void )
{
	Com_Printf( "%s", _("Hello\n") );
// 	Com_Printf( _("Hello") );
// 	Com_Printf( _("Bye") );
// 	Com_Printf( _("Okay" ) );
}
