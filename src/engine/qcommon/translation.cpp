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
#include <iostream>

using namespace tinygettext;
// Ugly char buffer
static char gettextbuffer[ MAX_STRING_CHARS ];

DictionaryManager trans_manager;
DictionaryManager trans_managergame;
Dictionary        trans_dict;
Dictionary        trans_dictgame;

cvar_t            *language;
cvar_t            *language_debug;
cvar_t            *trans_encodings;
cvar_t            *trans_languages;
bool              enabled = false;
int               modificationCount=0;

#define _(x) Trans_Gettext(x)
#define C_(x, y) Trans_Pgettext(x, y)

/*
====================
Trans_ReturnLanguage

Return a loaded language. If desired language is not found, return closest match. 
If no languages are close, force English.
====================
*/
Language Trans_ReturnLanguage( const char *lang )
{
	int bestScore = 0;
	Language bestLang, language = Language::from_env( std::string( lang ) );
	
	std::set<Language> langs = trans_manager.get_languages();
	for( std::set<Language>::iterator i = langs.begin(); i != langs.end(); i++ )
	{
		int score = Language::match( language, *i );
		
		if( score > bestScore )
		{
			bestScore = score;
			bestLang = *i;
		}
	}
	
	// Return "en" if language not found
	if( !bestLang )
	{
		Com_Printf( _("^3WARNING:^7 Language \"%s\" (%s) not found. Default to \"English\" (en)\n"),
					language.get_name().empty() ? _("Unknown Language") : language.get_name().c_str(),
					lang );
		bestLang = Language::from_env( "en" );
	}
	
	return bestLang;
}

extern "C" void Trans_UpdateLanguage_f( void )
{
	if( !enabled ) { return; }
	Language lang = Trans_ReturnLanguage( language->string );
	trans_dict = trans_manager.get_dictionary( lang );
	trans_dictgame = trans_managergame.get_dictionary( lang );
	Com_Printf(_( "Switched language to %s\n"), lang.get_name().c_str() );

#ifndef DEDICATED
	// update the default console keys string
	extern cvar_t *cl_consoleKeys; // should really #include client.h
	Z_Free( cl_consoleKeys->resetString );
	const char *default_consoleKeys = _("~ ` 0x7e 0x60");
	cl_consoleKeys->resetString = (char *) Z_Malloc( strlen( default_consoleKeys ) + 1 );
	strcpy( cl_consoleKeys->resetString, default_consoleKeys );
#endif
}

/*
============
Trans_Init
============
*/
extern "C" void Trans_Init( void )
{
	char                **poFiles, langList[ MAX_TOKEN_CHARS ] = "", encList[ MAX_TOKEN_CHARS ] = "";
	int                 numPoFiles, i;
	FL_Locale           *locale;
	std::set<Language>  langs;
	Language            lang;
	
	language = Cvar_Get( "language", "", CVAR_ARCHIVE );
	language_debug = Cvar_Get( "language_debug", "0", CVAR_ARCHIVE );
	trans_languages = Cvar_Get( "trans_languages", "", CVAR_ROM );
	trans_encodings = Cvar_Get( "trans_languages", "", CVAR_ROM );
	
	// Only detect locale if no previous language set.
	if( !language->string[0] )
	{
		FL_FindLocale( &locale, FL_MESSAGES );
		
		// Invalid or not found. Just use builtin language.
		if( !locale->lang || !locale->lang[0] || !locale->country || !locale->country[0] ) 
		{
			Cvar_Set( "language", "en" );
		}
		else
		{
			Cvar_Set( "language", va( "%s%s%s", locale->lang, 
									  locale->country[0] ? "_" : "",
									  locale->country ) );
		}
	}
	
	poFiles = FS_ListFiles( "translation/client", ".po", &numPoFiles );
	
	// This assumes that the filenames in both folders are the same
	for( i = 0; i < numPoFiles; i++ )
	{
		int ret;
		char *buffer, language[ 6 ];
		
		if( FS_ReadFile( va( "translation/client/%s", poFiles[ i ] ), ( void ** ) &buffer ) > 0 )
		{
			COM_StripExtension2( poFiles[ i ], language, sizeof( language ) );
			std::stringstream ss( buffer );
			trans_manager.add_po( poFiles[ i ], ss, Language::from_env( std::string( language ) ) );
			FS_FreeFile( buffer );
		}
		else
		{
			Com_Printf(_( "^1ERROR: Could not open client translation: %s\n"), poFiles[ i ] );
		}
		
		if( FS_ReadFile( va( "translation/game/%s", poFiles[ i ] ), ( void ** ) &buffer ) > 0 )
		{
			COM_StripExtension2( poFiles[ i ], language, sizeof( language ) );
			std::stringstream ss( buffer );
			trans_managergame.add_po( poFiles[ i ], ss, Language::from_env( std::string( language ) ) );
			FS_FreeFile( buffer );
		}
		else
		{
			Com_Printf(_( "^1ERROR: Could not open game translation: %s\n"), poFiles[ i ] );
		}
	}
	FS_FreeFileList( poFiles );
	langs = trans_manager.get_languages();
	for( std::set<Language>::iterator p = langs.begin(); p != langs.end(); p++ )
	{
		Q_strcat( langList, sizeof( langList ), va( "\"%s\" ", p->get_name().c_str() ) );
		Q_strcat( encList, sizeof( encList ), va( "\"%s%s%s\" ", p->get_language().c_str(), 
												  p->get_country().c_str()[0] ? "_" : "",
												  p->get_country().c_str() ) );
	}
	Cvar_Set( "trans_languages", langList );
	Cvar_Set( "trans_encodings", encList );
	Com_Printf(_( "Loaded %lu language(s)\n"), langs.size() );
	Cmd_AddCommand( "updatelanguage", Trans_UpdateLanguage_f );
	if( langs.size() )
	{
		lang = Trans_ReturnLanguage( language->string );
		trans_dict = trans_manager.get_dictionary( lang );
		trans_dictgame = trans_managergame.get_dictionary( lang );
		enabled = true;
	}
}

extern "C" const char* Trans_Gettext( const char *msgid )
{
	if( !enabled ) { return msgid; }
	Q_strncpyz( gettextbuffer, trans_dict.translate( msgid ).c_str(), sizeof( gettextbuffer ) );
	return gettextbuffer;
}

extern "C" const char* Trans_Pgettext( const char *msgctxt, const char *msgid )
{
	if ( !enabled ) { return msgid; }
	Q_strncpyz( gettextbuffer, trans_dict.translate_ctxt( msgctxt, msgid ).c_str(), sizeof( gettextbuffer ) );
	return gettextbuffer;
}

extern "C" const char* Trans_GettextGame( const char *msgid )
{
	if( !enabled ) { return msgid; }
	Q_strncpyz( gettextbuffer, trans_dictgame.translate( msgid ).c_str(), sizeof( gettextbuffer ) );
	return gettextbuffer;
}

extern "C" const char* Trans_GettextPlural( const char *msgid, const char *msgid_plural, int num )
{
	if( !enabled ) { return num == 1 ? msgid : msgid_plural; }
	Q_strncpyz( gettextbuffer, trans_dict.translate_plural( msgid, msgid_plural, num ).c_str(), sizeof( gettextbuffer ) );
	return gettextbuffer;
}

extern "C" const char* Trans_GettextGamePlural( const char *msgid, const char *msgid_plural, int num )
{
	if( !enabled ) { return num == 1 ? msgid : msgid_plural; }
	Q_strncpyz( gettextbuffer, trans_dictgame.translate_plural( msgid, msgid_plural, num ).c_str(), sizeof( gettextbuffer ) );
	return gettextbuffer;
}
