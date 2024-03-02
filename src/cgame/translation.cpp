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

#include "engine/qcommon/q_shared.h"
#include "engine/qcommon/qcommon.h"
#include "engine/client/cg_api.h"

#include "tinygettext/log.hpp"
#include "tinygettext/tinygettext.hpp"
#include "tinygettext/file_system.hpp"

#include "cg_local.h"

static Log::Logger LOG(VM_STRING_PREFIX "translation");
using namespace tinygettext;

// Ugly char buffer
static std::string gettextbuffer[ 4 ];
static int num = -1;

// Should be ROM but that doesn't work in gamelogic
static Cvar::Cvar<std::string> trans_encodings("trans_encodings", "Supported values for 'language' cvar", Cvar::NONE, "");
static Cvar::Cvar<std::string> trans_languages("trans_languages", "Supported languages (human-readable)", Cvar::NONE, "");

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

		trap_FS_OpenPakFile( filename.c_str(), fileHandle );
	}

	~DaemonInputbuf()
	{
		if( fileHandle )
		{
			trap_FS_FCloseFile( fileHandle );
		}
	}

	int underflow() override
	{
		if( gptr() < egptr() ) // buffer not exhausted
		{
			return traits_type::to_int_type( *gptr() );
		}

		if( !fileHandle )
		{
			return traits_type::eof();
		}

		char *base = buffer;
		char *start = base;

		if( eback() == base )
		{
			// Make arrangements for putback characters
			memmove( base, egptr() - putBack, putBack );
			start += putBack;
		}

		size_t n = trap_FS_Read( start, BUFFER_SIZE - ( start - base ), fileHandle );

		if( n == 0 )
		{
			return traits_type::eof();
		}

		// Set buffer pointers
		setg( base, start, start + n );

		return traits_type::to_int_type( *gptr() );
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

Class used by tinygettext to read files and directories
Uses the engine's File I/O functions for this purpose
====================
*/

class DaemonFileSystem : public tinygettext::FileSystem
{
public:
	DaemonFileSystem() = default;

	std::vector<std::string> open_directory( const std::string& pathname )
	{
		int numFiles;
		std::vector<std::string> ret;
		char listbuf[15000];
		numFiles = trap_FS_GetFileList( pathname.c_str(), "", listbuf, sizeof( listbuf ) );
		const char* next = listbuf;
		for( int i = 0; i < numFiles; i++ )
		{
			ret.emplace_back( next );
			next += ret.back().size() + 1;
		}

		return ret;
	}

	std::unique_ptr<std::istream> open_file( const std::string& filename )
	{
		return std::unique_ptr<std::istream>( new DaemonIstream( filename ) );
	}
};

static tinygettext::DictionaryManager trans_manager{ "UTF-8", Util::make_unique<DaemonFileSystem>() };

/*
====================
Logging functions used by tinygettext
====================
*/

static std::string Trimmed( const std::string& str )
{
	std::string trimmed = str;
	while ( !trimmed.empty() && ( trimmed.back() == '\r' || trimmed.back() == '\n' ) )
	{
		trimmed.pop_back();
	}
	return trimmed;
}

static void Trans_Error( const std::string& str )
{
	LOG.DoWarnCode([&] { LOG.Warn(Trimmed(str)); });
}

static void Trans_Warning( const std::string& str )
{
	LOG.DoNoticeCode([&] { LOG.Notice(Trimmed(str)); });
}

static void Trans_Info( const std::string& str )
{
	LOG.DoVerboseCode([&] { LOG.Verbose(Trimmed(str)); });
}

/*
====================
Trans_SetLanguage

Sets a loaded language. If desired language is not found, set closest match.
If no languages are close, force English.
====================
*/

static void Trans_SetLanguage( const char* lang )
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
	if ( bestScore == 0 && !Str::IsIEqual( lang, "C" ) )
	{
		LOG.Warn( "Language \"%s\" (%s) not found. Default to \"English\" (en)",
					requestLang.get_name().empty() ? "Unknown Language" : requestLang.get_name().c_str(),
					lang );
	}

	trans_manager.set_language( bestLang );

	LOG.Notice( "Set language to %s_%s (%s)" , bestLang.get_language().c_str() , bestLang.get_country().c_str() , bestLang.get_name().c_str() );
}

// TODO: update automatically on modification instead of having this stupid command
void Trans_UpdateLanguage_f()
{
	Trans_SetLanguage( Cvar::GetValue( "language" ).c_str() );
	Rocket_UpdateLanguage();
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

	// set tinygettext log callbacks
	tinygettext::Log::set_log_error_callback( &Trans_Error );
	tinygettext::Log::set_log_warning_callback( &Trans_Warning );
	tinygettext::Log::set_log_info_callback( &Trans_Info );

	trans_manager.set_filesystem( Util::make_unique<DaemonFileSystem>() );

	trans_manager.add_directory( "translation/game" );

	langs = trans_manager.get_languages();

	for( std::set<Language>::iterator p = langs.begin(); p != langs.end(); p++ )
	{
		Q_strcat( langList, sizeof( langList ), va( "\"%s\" ", p->get_name().c_str() ) );
		Q_strcat( encList, sizeof( encList ), va( "\"%s\" ", p->str().c_str() ) );
	}

	trans_languages.Set( langList );
	trans_encodings.Set( encList );

	LOG.Notice( "Loaded %u language%s", langs.size(), ( langs.size() == 1 ? "" : "s" ) );

	Trans_SetLanguage( Cvar::GetValue( "language" ).c_str() );
}

const char* Trans_Gettext( const char *msgid )
{
	LOG.Debug( "translate[_]: %s", msgid );
	if ( !msgid )
	{
		return msgid;
	}

	num = ( num + 1 ) & 3;
	gettextbuffer[ num ] = trans_manager.get_dictionary().translate( msgid );
	return gettextbuffer[ num ].c_str();
}

const char* Trans_Pgettext( const char *ctxt, const char *msgid )
{
	LOG.Debug( "translate[_G]: %s", msgid );
	if ( !ctxt || !msgid )
	{
		return msgid;
	}

	num = ( num + 1 ) & 3;
	gettextbuffer[ num ] = trans_manager.get_dictionary().translate_ctxt( ctxt, msgid );
	return gettextbuffer[ num ].c_str();
}

const char* Trans_GettextPlural( const char *msgid, const char *msgid_plural, int number )
{
	LOG.Debug( "translate[_P]: %s", msgid );
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
	gettextbuffer[ num ] = trans_manager.get_dictionary().translate_plural( msgid, msgid_plural, number );
	return gettextbuffer[ num ].c_str();
}
