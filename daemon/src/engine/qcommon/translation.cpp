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

#include "q_shared.h"
#include "qcommon.h"

extern "C" {
    #include "findlocale/findlocale.h"
}

#include "tinygettext/log.hpp"
#include "tinygettext/tinygettext.hpp"
#include "tinygettext/file_system.hpp"

using namespace tinygettext;

// Ugly char buffer
static std::string gettextbuffer[ 4 ];
static int num = -1;

DictionaryManager trans_manager;
DictionaryManager trans_managergame;

cvar_t            *language;
cvar_t            *trans_debug;
cvar_t            *trans_encodings;
cvar_t            *trans_languages;

#ifndef BUILD_SERVER
extern cvar_t *cl_consoleKeys; // should really #include client.h
#endif

/*
====================
DaemonInputbuf

Streambuf based class that uses the engine's File I/O functions for input
====================
*/

class DaemonInputbuf : public std::streambuf
{
private:
	static const size_t BUFFER_SIZE = 8192;
	fileHandle_t fileHandle;
	char  buffer[ BUFFER_SIZE ];
	size_t  putBack;
public:
	DaemonInputbuf( const std::string& filename ) : putBack( 1 )
	{
		char *end;

		end = buffer + BUFFER_SIZE - putBack;
		setg( end, end, end );

		FS_FOpenFileRead( filename.c_str(), &fileHandle, false );
	}

	~DaemonInputbuf()
	{
		if( fileHandle )
		{
			FS_FCloseFile( fileHandle );
		}
	}
};

/*
====================
DaemonIstream

Simple istream based class that takes ownership of the streambuf
====================
*/

class DaemonIstream : public std::istream
{
public:
	DaemonIstream( const std::string& filename ) : std::istream( new DaemonInputbuf( filename ) ) {}
	~DaemonIstream()
	{
		delete rdbuf();
	}
};

/*
====================
DaemonFileSystem

Class used by tinygettext to read files and directorys
Uses the engine's File I/O functions for this purpose
====================
*/

class DaemonFileSystem : public FileSystem
{
public:
	DaemonFileSystem() {}

	std::vector<std::string> open_directory( const std::string& pathname )
	{
		int numFiles;
		char **files;
		std::vector<std::string> ret;

		files = FS_ListFiles( pathname.c_str(), nullptr, &numFiles );

		for( int i = 0; i < numFiles; i++ )
		{
			ret.push_back( std::string( files[ i ] ) );
		}

		FS_FreeFileList( files );
		return ret;
	}

	std::unique_ptr<std::istream> open_file( const std::string& filename )
	{
		return std::unique_ptr<std::istream>( new DaemonIstream( filename ) );
	}
};

/*
====================
Logging functions used by tinygettext
====================
*/

void Trans_Error( const std::string& str )
{
	Com_Printf( "^1%s^7", str.c_str() );
}

void Trans_Warning( const std::string& str )
{
	if( trans_debug->integer != 0 )
	{
		Com_Printf( "^3%s^7", str.c_str() );
	}
}

void Trans_Info( const std::string& str )
{
	if( trans_debug->integer != 0 )
	{
		Com_Printf( "%s", str.c_str() );
	}
}

/*
====================
Trans_SetLanguage

Sets a loaded language. If desired language is not found, set closest match.
If no languages are close, force English.
====================
*/

void Trans_SetLanguage( const char* lang )
{
	Language requestLang = Language::from_env( std::string( lang ) );

	// default to english
	Language bestLang = Language::from_env( "en" );
	int bestScore = Language::match( requestLang, bestLang );

	std::set<Language> langs = trans_manager.get_languages();

	for( std::set<Language>::iterator i = langs.begin(); i != langs.end(); i++ )
	{
		int score = Language::match( requestLang, *i );

		if( score > bestScore )
		{
			bestScore = score;
			bestLang = *i;
		}
	}

	// language not found, display warning
	if( bestScore == 0 )
	{
		Com_Printf( S_WARNING "Language \"%s\" (%s) not found. Default to \"English\" (en)\n",
					requestLang.get_name().empty() ? "Unknown Language" : requestLang.get_name().c_str(),
					lang );
	}

	trans_manager.set_language( bestLang );
	trans_managergame.set_language( bestLang );

	Cvar_Set( "language", bestLang.str().c_str() );

	Com_Printf( "Set language to %s" , bestLang.get_name().c_str() );
}

void Trans_UpdateLanguage_f()
{
	Trans_SetLanguage( language->string );

#ifndef BUILD_SERVER
	// update the default console keys string
	Z_Free( cl_consoleKeys->resetString );
	cl_consoleKeys->resetString = CopyString( _("~ ` 0x7e 0x60") );
#endif
}

/*
============
Trans_Init
============
*/
void Trans_Init()
{
	char langList[ MAX_TOKEN_CHARS ] = "";
	char encList[ MAX_TOKEN_CHARS ] = "";
	std::set<Language>  langs;
	Language            lang;

	Cmd_AddCommand( "updatelanguage", Trans_UpdateLanguage_f );

	language = Cvar_Get( "language", "", CVAR_ARCHIVE );
	trans_debug = Cvar_Get( "trans_debug", "0", 0 );
	trans_languages = Cvar_Get( "trans_languages", "", CVAR_ROM );
	trans_encodings = Cvar_Get( "trans_encodings", "", CVAR_ROM );

	// set tinygettext log callbacks
	tinygettext::Log::set_log_error_callback( &Trans_Error );
	tinygettext::Log::set_log_warning_callback( &Trans_Warning );
	tinygettext::Log::set_log_info_callback( &Trans_Info );

	trans_manager.set_filesystem( std::unique_ptr<FileSystem>( new DaemonFileSystem ) );
	trans_managergame.set_filesystem( std::unique_ptr<FileSystem>( new DaemonFileSystem ) );

	trans_manager.add_directory( "translation/client" );
	trans_managergame.add_directory( "translation/game" );

	langs = trans_manager.get_languages();

	for( std::set<Language>::iterator p = langs.begin(); p != langs.end(); p++ )
	{
		Q_strcat( langList, sizeof( langList ), va( "\"%s\" ", p->get_name().c_str() ) );
		Q_strcat( encList, sizeof( encList ), va( "\"%s\" ", p->str().c_str() ) );
	}

	Cvar_Set( "trans_languages", langList );
	Cvar_Set( "trans_encodings", encList );

	Com_Printf( "Loaded %lu language%s\n", ( unsigned long ) langs.size(), ( langs.size() == 1 ? "" : "s" ) );
}

void Trans_LoadDefaultLanguage()
{
	FL_Locale           *locale;

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

		FL_FreeLocale( &locale );
	}

	Trans_SetLanguage( language->string );
}

const char* Trans_Gettext_Internal( const char *msgid, DictionaryManager& manager )
{
	if ( !msgid )
	{
		return msgid;
	}

	num = ( num + 1 ) & 3;
	gettextbuffer[ num ] = manager.get_dictionary().translate( msgid );
	return gettextbuffer[ num ].c_str();
}

const char* Trans_Pgettext_Internal( const char *ctxt, const char *msgid, DictionaryManager& manager )
{
	if ( !ctxt || !msgid )
	{
		return msgid;
	}

	num = ( num + 1 ) & 3;
	gettextbuffer[ num ] = manager.get_dictionary().translate_ctxt( ctxt, msgid );
	return gettextbuffer[ num ].c_str();
}

const char* Trans_GettextPlural_Internal( const char *msgid, const char *msgid_plural, int number, DictionaryManager& manager )
{
	if ( !msgid || !msgid_plural )
	{
		if ( msgid )
		{
			return msgid;
		}

		if ( msgid_plural )
		{
			return msgid_plural;
		}

		return nullptr;
	}

	num = ( num + 1 ) & 3;
	gettextbuffer[ num ] = manager.get_dictionary().translate_plural( msgid, msgid_plural, number );
	return gettextbuffer[ num ].c_str();
}

const char* Trans_Gettext( const char *msgid )
{
	return Trans_Gettext_Internal( msgid, trans_manager );
}

const char* Trans_Pgettext( const char *ctxt, const char *msgid )
{
	return Trans_Pgettext_Internal( ctxt, msgid, trans_manager );
}
const char* Trans_GettextPlural( const char *msgid, const char *msgid_plural, int num )
{
	return Trans_GettextPlural_Internal( msgid, msgid_plural, num, trans_manager );
}

const char* Trans_GettextGame( const char *msgid )
{
	return Trans_Gettext_Internal( msgid, trans_managergame );
}

const char* Trans_PgettextGame( const char *ctxt, const char *msgid )
{
	return Trans_Pgettext_Internal( ctxt, msgid, trans_managergame );
}

const char* Trans_GettextGamePlural( const char *msgid, const char *msgid_plural, int num )
{
	return Trans_GettextPlural_Internal( msgid, msgid_plural, num, trans_managergame );
}
