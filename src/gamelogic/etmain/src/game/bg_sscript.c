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
#include "bg_public.h"

#define MAX_SCRIPTSPEAKERS 256

static bg_speaker_t scriptSpeakers[ MAX_SCRIPTSPEAKERS ];
static int          numScriptSpeakers;

void BG_ClearScriptSpeakerPool( void )
{
	numScriptSpeakers = 0;
}

int BG_NumScriptSpeakers( void )
{
	return numScriptSpeakers;
}

int BG_GetIndexForSpeaker( bg_speaker_t *speaker )
{
	return speaker - scriptSpeakers;
}

bg_speaker_t   *BG_GetScriptSpeaker( int index )
{
	if( index < 0 || index >= numScriptSpeakers )
	{
		return NULL;
	}

	return &scriptSpeakers[ index ];
}

qboolean BG_SS_DeleteSpeaker( int index )
{
	if( index < 0 || index >= numScriptSpeakers )
	{
		return qfalse;
	}

	memcpy( &scriptSpeakers[ index ], &scriptSpeakers[ index + 1 ], sizeof( bg_speaker_t ) * ( numScriptSpeakers - index - 1 ) );

	numScriptSpeakers--;

	return qtrue;
}

qboolean BG_SS_StoreSpeaker( bg_speaker_t *speaker )
{
	if( numScriptSpeakers >= MAX_SCRIPTSPEAKERS )
	{
		return qfalse;
	}

	memcpy( &scriptSpeakers[ numScriptSpeakers++ ], speaker, sizeof( bg_speaker_t ) );

	return qtrue;
}

static __attribute__( ( format( printf, 2, 3 ) ) ) qboolean BG_SS_ParseError( int handle, char *format, ... )
{
	int         line;
	char        filename[ 128 ];
	va_list     argptr;
	static char string[ 4096 ];

	va_start( argptr, format );
	Q_vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );

	filename[ 0 ] = '\0';
	line = 0;
	trap_PC_SourceFileAndLine( handle, filename, &line );

	Com_Printf( S_COLOR_RED "ERROR: %s, line %d: %s\n", filename, line, string );

	trap_PC_FreeSource( handle );

	return qfalse;
}

static qboolean BG_SS_ParseSpeaker( int handle )
{
	pc_token_t   token;
	bg_speaker_t speaker;

	memset( &speaker, 0, sizeof( speaker ) );

	speaker.volume = 127;
	speaker.range = 1250;

	if( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) )
	{
		return BG_SS_ParseError( handle, "expected '{'" );
	}

	while( 1 )
	{
		if( !trap_PC_ReadToken( handle, &token ) )
		{
			break;
		}

		if( token.string[ 0 ] == '}' )
		{
			break;
		}

		if( !Q_stricmp( token.string, "noise" ) )
		{
			if( !PC_String_ParseNoAlloc( handle, speaker.filename, sizeof( speaker.filename ) ) )
			{
				return BG_SS_ParseError( handle, "expected sound filename" );
			}
		}
		else if( !Q_stricmp( token.string, "origin" ) )
		{
			if( !PC_Vec_Parse( handle, &speaker.origin ) )
			{
				return BG_SS_ParseError( handle, "expected origin vector" );
			}
		}
		else if( !Q_stricmp( token.string, "targetname" ) )
		{
			if( !PC_String_ParseNoAlloc( handle, speaker.targetname, sizeof( speaker.targetname ) ) )
			{
				return BG_SS_ParseError( handle, "expected targetname string" );
			}
			else
			{
				speaker.targetnamehash = BG_StringHashValue( speaker.targetname );
			}
		}
		else if( !Q_stricmp( token.string, "looped" ) )
		{
			if( !trap_PC_ReadToken( handle, &token ) )
			{
				return BG_SS_ParseError( handle, "expected loop value" );
			}
			else
			{
				if( !Q_stricmp( token.string, "no" ) )
				{
					speaker.loop = S_LT_NOT_LOOPED;
				}
				else if( !Q_stricmp( token.string, "on" ) )
				{
					speaker.loop = S_LT_LOOPED_ON;
					speaker.activated = qtrue;
				}
				else if( !Q_stricmp( token.string, "off" ) )
				{
					speaker.loop = S_LT_LOOPED_OFF;
				}
				else
				{
					return BG_SS_ParseError( handle, "unknown loop value '%s'", token.string );
				}
			}
		}
		else if( !Q_stricmp( token.string, "broadcast" ) )
		{
			if( !trap_PC_ReadToken( handle, &token ) )
			{
				return BG_SS_ParseError( handle, "expected broadcast value" );
			}
			else
			{
				if( !Q_stricmp( token.string, "no" ) )
				{
					speaker.broadcast = S_BT_LOCAL;
				}
				else if( !Q_stricmp( token.string, "global" ) )
				{
					speaker.broadcast = S_BT_GLOBAL;
				}
				else if( !Q_stricmp( token.string, "nopvs" ) )
				{
					speaker.broadcast = S_BT_NOPVS;
				}
				else
				{
					return BG_SS_ParseError( handle, "unknown broadcast value '%s'", token.string );
				}
			}
		}
		else if( !Q_stricmp( token.string, "wait" ) )
		{
			if( !PC_Int_Parse( handle, &speaker.wait ) )
			{
				return BG_SS_ParseError( handle, "expected wait value" );
			}
			else if( speaker.wait < 0 )
			{
				return BG_SS_ParseError( handle, "wait value %i is invalid", speaker.wait );
			}
		}
		else if( !Q_stricmp( token.string, "random" ) )
		{
			if( !PC_Int_Parse( handle, &speaker.random ) )
			{
				return BG_SS_ParseError( handle, "expected random value" );
			}
			else if( speaker.random < 0 )
			{
				return BG_SS_ParseError( handle, "random value %i is invalid", speaker.random );
			}
		}
		else if( !Q_stricmp( token.string, "volume" ) )
		{
			if( !PC_Int_Parse( handle, &speaker.volume ) )
			{
				return BG_SS_ParseError( handle, "expected volume value" );
			}
			else if( speaker.volume < 0 || speaker.volume > 65535 )
			{
				return BG_SS_ParseError( handle, "volume value %i is invalid", speaker.volume );
			}
		}
		else if( !Q_stricmp( token.string, "range" ) )
		{
			if( !PC_Int_Parse( handle, &speaker.range ) )
			{
				return BG_SS_ParseError( handle, "expected range value" );
			}
			else if( speaker.range < 0 )
			{
				return BG_SS_ParseError( handle, "range value %i is invalid", speaker.range );
			}
		}
		else
		{
			return BG_SS_ParseError( handle, "unknown token '%s'", token.string );
		}
	}

	if( !BG_SS_StoreSpeaker( &speaker ) )
	{
		return BG_SS_ParseError( handle, "Failed to store speaker" );
	}

	return qtrue;
}

qboolean BG_LoadSpeakerScript( const char *filename )
{
	pc_token_t token;
	int        handle;

	handle = trap_PC_LoadSource( filename );

	if( !handle )
	{
		return qfalse;
	}

	if( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "speakerScript" ) )
	{
		return BG_SS_ParseError( handle, "expected 'soundScript'" );
	}

	if( !trap_PC_ReadToken( handle, &token ) || Q_stricmp( token.string, "{" ) )
	{
		return BG_SS_ParseError( handle, "expected '{'" );
	}

	while( 1 )
	{
		if( !trap_PC_ReadToken( handle, &token ) )
		{
			break;
		}

		if( token.string[ 0 ] == '}' )
		{
			break;
		}

		if( !Q_stricmp( token.string, "speakerDef" ) )
		{
			if( !BG_SS_ParseSpeaker( handle ) )
			{
				return qfalse;
			}
		}
		else
		{
			return BG_SS_ParseError( handle, "unknown token '%s'", token.string );
		}
	}

	trap_PC_FreeSource( handle );

	return qtrue;
}
