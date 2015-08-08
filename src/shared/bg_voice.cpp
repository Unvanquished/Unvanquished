/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development
Copyright (C) 2008      Tony J. White

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

// bg_voice.c -- both games voice functions
#include "engine/qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

int         trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
int         trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
int         trap_Parse_LoadSource( const char *filename );
int         trap_Parse_FreeSource( int handle );
bool        trap_Parse_ReadToken( int handle, pc_token_t *pc_token );
int         trap_Parse_SourceFileAndLine( int handle, char *filename, int *line );

#ifdef BUILD_CGAME
sfxHandle_t trap_S_RegisterSound( const char *sample, bool compressed );

#endif

/*
============
BG_VoiceParseError
============
*/
static void NORETURN BG_VoiceParseError( fileHandle_t handle, const char *err )
{
	int  line;
	char filename[ MAX_QPATH ];

	trap_Parse_SourceFileAndLine( handle, filename, &line );
	trap_Parse_FreeSource( handle );
	Com_Error( ERR_FATAL, "%s on line %d of %s", err, line, filename );
}

/*
============
BG_VoiceList
============
*/
static voice_t *BG_VoiceList()
{
	char    fileList[ MAX_VOICES * ( MAX_VOICE_NAME_LEN + 6 ) ] = { "" };
	int     numFiles, i, fileLen = 0;
	int     count = 0;
	char    *filePtr;
	voice_t *voices = nullptr;
	voice_t *top = nullptr;

	numFiles = trap_FS_GetFileList( "voice", ".voice", fileList,
	                                sizeof( fileList ) );

	if ( numFiles < 1 )
	{
		return nullptr;
	}

	// special case for default.voice.  this file is REQUIRED and will
	// always be loaded first in the event of overflow of voice definitions
	if ( !trap_FS_FOpenFile( "voice/default.voice", nullptr, FS_READ ) )
	{
		Com_Printf( "voice/default.voice missing, voice system disabled.\n" );
		return nullptr;
	}

	voices = ( voice_t * ) BG_Alloc( sizeof( voice_t ) );
	Q_strncpyz( voices->name, "default", sizeof( voices->name ) );
	voices->cmds = nullptr;
	voices->next = nullptr;
	count = 1;

	top = voices;

	filePtr = fileList;

	for ( i = 0; i < numFiles; i++, filePtr += fileLen + 1 )
	{
		fileLen = strlen( filePtr );

		// accounted for above
		if ( !Q_stricmp( filePtr, "default.voice" ) )
		{
			continue;
		}

		if ( fileLen >= MAX_VOICE_NAME_LEN + 6 )
		{
			Com_Printf( S_WARNING "MAX_VOICE_NAME_LEN is %d. "
			            "skipping \"%s\", filename too long\n", MAX_VOICE_NAME_LEN, filePtr );
			continue;
		}

		// trap_FS_GetFileList() buffer has overflowed
		if ( !trap_FS_FOpenFile( va( "voice/%s", filePtr ), nullptr, FS_READ ) )
		{
			Com_Printf( S_WARNING "BG_VoiceList(): detected "
			            "an invalid .voice file \"%s\" in directory listing.  You have "
			            "probably named one or more .voice files with outrageously long "
			            "names.\n", filePtr );
			break;
		}

		if ( count >= MAX_VOICES )
		{
			Com_Printf( S_WARNING ".voice file overflow.  "
			            "%d of %d .voice files loaded.  MAX_VOICES is %d\n",
			            count, numFiles, MAX_VOICES );
			break;
		}

		voices->next = ( voice_t * ) BG_Alloc( sizeof( voice_t ) );
		voices = voices->next;

		Q_strncpyz( voices->name, filePtr, sizeof( voices->name ) );
		// strip extension
		voices->name[ fileLen - 6 ] = '\0';
		voices->cmds = nullptr;
		voices->next = nullptr;
		count++;
	}

	return top;
}

/*
============
BG_VoiceParseTrack
============
*/
static bool BG_VoiceParseTrack( int handle, voiceTrack_t *voiceTrack )
{
	pc_token_t token;
	bool   found = false;
	bool   foundText = false;
	bool   foundToken = trap_Parse_ReadToken( handle, &token );

	while ( foundToken )
	{
		if ( token.string[ 0 ] == '}' )
		{
			if ( foundText )
			{
				return true;
			}
			else
			{
				BG_VoiceParseError( handle, "BG_VoiceParseTrack(): "
				                    "missing text attribute for track" );
			}
		}
		else if ( !Q_stricmp( token.string, "team" ) )
		{
			foundToken = trap_Parse_ReadToken( handle, &token );
			found = false;

			while ( foundToken )
			{
				if ( voiceTrack->team < 0 )
				{
					voiceTrack->team = 0;
				}

				if ( !Q_stricmp( token.string, "humans" ) )
				{
					voiceTrack->team |= 1 << TEAM_HUMANS;
				}
				else if ( !Q_stricmp( token.string, "aliens" ) )
				{
					voiceTrack->team |= 1 << TEAM_ALIENS;
				}
				else
				{
					break;
				}

				found = true;
				foundToken = trap_Parse_ReadToken( handle, &token );
			}

			if ( !found )
			{
				BG_VoiceParseError( handle, "BG_VoiceParseTrack(): missing \"team\" name" );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "class" ) )
		{
			bool negate = false;

			foundToken = trap_Parse_ReadToken( handle, &token );
			found = false;

			while ( foundToken )
			{
				classModelConfig_t *model;
				int                modelno = -1;

				if ( voiceTrack->pClass < 0 )
				{
					voiceTrack->pClass = 0;
				}

				if ( !Q_stricmp( token.string, "all" ) )
				{
					modelno = PCL_ALL_CLASSES;
				}
				else if ( !Q_stricmp( token.string, "humans" ) )
				{
					modelno = PCL_HUMAN_CLASSES;
				}
				else if ( !Q_stricmp( token.string, "aliens" ) )
				{
					modelno = PCL_ALIEN_CLASSES;
				}
				else if ( !Q_stricmp( token.string, "-" ) ) // this must be outside quotation marks
				{
					negate = true;
					modelno = 0;
				}
				else
				{
					model = BG_ClassModelConfigByName( token.string );

					if ( model != BG_ClassModelConfigByName( nullptr ) )
					{
						modelno = 1 << ( model - BG_ClassModelConfig( 0 ) );

						if ( modelno <= 1)
						{
							modelno = -1; // match failure
						}
					}

				}

				if ( modelno > 0 )
				{
					if ( negate )
					{
						negate = false;
						voiceTrack->pClass &= ~modelno;
					}
					else
					{
						voiceTrack->pClass |= modelno;
					}
				}
				else if ( modelno < 0 )
				{
				        break; // possibly the next keyword
				}

				found = true;
				foundToken = trap_Parse_ReadToken( handle, &token );
			}

			if ( !found )
			{
				BG_VoiceParseError( handle, "BG_VoiceParseTrack(): missing \"class\" name" );
			}

			continue;
		}
		else if ( !Q_stricmp( token.string, "text" ) )
		{
			if ( foundText )
			{
				BG_VoiceParseError( handle, "BG_VoiceParseTrack(): "
				                    "duplicate \"text\" definition for track" );
			}

			foundToken = trap_Parse_ReadToken( handle, &token );

			if ( !foundToken )
			{
				BG_VoiceParseError( handle, "BG_VoiceParseTrack(): "
				                    "missing \"text\" value" );
			}

			foundText = true;

			if ( strlen( token.string ) >= MAX_SAY_TEXT )
			{
				BG_VoiceParseError( handle, va( "BG_VoiceParseTrack(): "
				                                "\"text\" value " "\"%s\" exceeds MAX_SAY_TEXT length",
				                                token.string ) );
			}

			voiceTrack->text = ( char * ) BG_Alloc( strlen( token.string ) + 1 );
			Q_strncpyz( voiceTrack->text, token.string, strlen( token.string ) + 1 );
			foundToken = trap_Parse_ReadToken( handle, &token );
			continue;
		}
		else if ( !Q_stricmp( token.string, "enthusiasm" ) )
		{
			foundToken = trap_Parse_ReadToken( handle, &token );

			if ( token.type == TT_NUMBER )
			{
				voiceTrack->enthusiasm = token.intvalue;
			}
			else
			{
				BG_VoiceParseError( handle, "BG_VoiceParseTrack(): "
				                    "missing \"enthusiasm\" value" );
			}

			foundToken = trap_Parse_ReadToken( handle, &token );
			continue;
		}
		else
		{
			BG_VoiceParseError( handle, va( "BG_VoiceParseTrack():"
			                                " unknown token \"%s\"", token.string ) );
		}
	}

	return false;
}

/*
============
BG_VoiceParseCommand
============
*/
static voiceTrack_t *BG_VoiceParseCommand( int handle )
{
	pc_token_t   token;
	bool     parsingTrack = false;
	voiceTrack_t *voiceTracks = nullptr;
	voiceTrack_t *top = nullptr;

	while ( trap_Parse_ReadToken( handle, &token ) )
	{
		if ( !parsingTrack && token.string[ 0 ] == '}' )
		{
			return top;
		}

		if ( parsingTrack )
		{
			if ( token.string[ 0 ] == '{' )
			{
				BG_VoiceParseTrack( handle, voiceTracks );
				parsingTrack = false;
				continue;
			}
			else
			{
				BG_VoiceParseError( handle, va( "BG_VoiceParseCommand(): "
				                                "parse error at \"%s\"", token.string ) );
			}
		}

		if ( top == nullptr )
		{
			voiceTracks = (voiceTrack_t*) BG_Alloc( sizeof( voiceTrack_t ) );
			top = voiceTracks;
		}
		else
		{
			voiceTracks->next = (voiceTrack_t*) BG_Alloc( sizeof( voiceTrack_t ) );
			voiceTracks = voiceTracks->next;
		}

		if ( !trap_FS_FOpenFile( token.string, nullptr, FS_READ ) )
		{
			int  line;
			char filename[ MAX_QPATH ];

			trap_Parse_SourceFileAndLine( handle, filename, &line );
			Com_Printf( S_WARNING "BG_VoiceParseCommand(): "
			            "track \"%s\" referenced on line %d of %s does not exist\n",
			            token.string, line, filename );
		}
		else
		{
#ifdef BUILD_CGAME
			voiceTracks->track = trap_S_RegisterSound( token.string, false );
			voiceTracks->duration = 0; // FIXME: Was always zero...
#endif
		}

		voiceTracks->team = -1;
		voiceTracks->pClass = -1;
		voiceTracks->weapon = -1;
		voiceTracks->enthusiasm = 0;
		voiceTracks->text = nullptr;
		voiceTracks->next = nullptr;
		parsingTrack = true;
	}

	return nullptr;
}

/*
============
BG_VoiceParse
============
*/
static voiceCmd_t *BG_VoiceParse( const char *name )
{
	voiceCmd_t *voiceCmds = nullptr;
	voiceCmd_t *top = nullptr;
	pc_token_t token;
	bool   parsingCmd = false;
	int        handle;

	handle = trap_Parse_LoadSource( va( "voice/%s.voice", name ) );

	if ( !handle )
	{
		return nullptr;
	}

	while ( trap_Parse_ReadToken( handle, &token ) )
	{
		if ( parsingCmd )
		{
			if ( token.string[ 0 ] == '{' )
			{
				voiceCmds->tracks = BG_VoiceParseCommand( handle );
				parsingCmd = false;
				continue;
			}
			else
			{
				int  line;
				char filename[ MAX_QPATH ];

				trap_Parse_SourceFileAndLine( handle, filename, &line );
				Com_Error( ERR_FATAL, "BG_VoiceParse(): "
				           "parse error on line %d of %s", line, filename );
			}
		}

		if ( strlen( token.string ) >= MAX_VOICE_CMD_LEN )
		{
			int  line;
			char filename[ MAX_QPATH ];

			trap_Parse_SourceFileAndLine( handle, filename, &line );
			Com_Error( ERR_FATAL, "BG_VoiceParse(): "
			           "command \"%s\" exceeds MAX_VOICE_CMD_LEN (%d) on line %d of %s",
			           token.string, MAX_VOICE_CMD_LEN, line, filename );
		}

		if ( top == nullptr )
		{
			voiceCmds = (voiceCmd_t*) BG_Alloc( sizeof( voiceCmd_t ) );
			top = voiceCmds;
		}
		else
		{
			voiceCmds->next = (voiceCmd_t*) BG_Alloc( sizeof( voiceCmd_t ) );
			voiceCmds = voiceCmds->next;
		}

		Q_strncpyz( voiceCmds->cmd, token.string, sizeof( voiceCmds->cmd ) );
		voiceCmds->next = nullptr;
		parsingCmd = true;
	}

	trap_Parse_FreeSource( handle );

	return top;
}

/*
============
BG_VoiceInit
============
*/
voice_t *BG_VoiceInit()
{
	voice_t *voices;
	voice_t *voice;

	voices = BG_VoiceList();

	voice = voices;

	while ( voice )
	{
		voice->cmds = BG_VoiceParse( voice->name );
		voice = voice->next;
	}

	return voices;
}

/*
============
BG_PrintVoices
============
*/
void BG_PrintVoices( voice_t *voices, int debugLevel )
{
	voice_t      *voice = voices;
	voiceCmd_t   *voiceCmd;
	voiceTrack_t *voiceTrack;

	int          cmdCount;
	int          trackCount;

	if ( voice == nullptr )
	{
		Com_Printf( "voice list is empty\n" );
		return;
	}

	while ( voice != nullptr )
	{
		if ( debugLevel > 0 )
		{
			Com_Printf( "voice %s\n", Quote( voice->name ) );
		}

		voiceCmd = voice->cmds;
		cmdCount = 0;
		trackCount = 0;

		while ( voiceCmd != nullptr )
		{
			if ( debugLevel > 0 )
			{
				Com_Printf( "  %s\n", voiceCmd->cmd );
			}

			voiceTrack = voiceCmd->tracks;
			cmdCount++;

			while ( voiceTrack != nullptr )
			{
				if ( debugLevel > 1 )
				{
					Com_Printf( "    text -> %s\n", voiceTrack->text );
				}

				if ( debugLevel > 2 )
				{
					Com_Printf( "    team -> %d\n", voiceTrack->team );
					Com_Printf( "    class -> %d\n", voiceTrack->pClass );
					Com_Printf( "    weapon -> %d\n", voiceTrack->weapon );
					Com_Printf( "    enthusiasm -> %d\n", voiceTrack->enthusiasm );
#ifdef BUILD_CGAME
					Com_Printf( "    duration -> %d\n", voiceTrack->duration );
#endif
				}

				if ( debugLevel > 1 )
				{
					Com_Printf( "\n" );
				}

				trackCount++;
				voiceTrack = voiceTrack->next;
			}

			voiceCmd = voiceCmd->next;
		}

		if ( debugLevel )
		{
			Com_Printf( "voice \"%s\": %d commands, %d tracks\n",
			            voice->name, cmdCount, trackCount );
		}

		voice = voice->next;
	}
}

/*
============
BG_VoiceByName
============
*/
voice_t *BG_VoiceByName( voice_t *head, const char *name )
{
	voice_t *v = head;

	while ( v )
	{
		if ( !Q_stricmp( v->name, name ) )
		{
			return v;
		}

		v = v->next;
	}

	return nullptr;
}

/*
============
BG_VoiceCmdFind
============
*/
voiceCmd_t *BG_VoiceCmdFind( voiceCmd_t *head, const char *name, int *cmdNum )
{
	voiceCmd_t *vc = head;
	int        i = 0;

	while ( vc )
	{
		i++;

		if ( !Q_stricmp( vc->cmd, name ) )
		{
			*cmdNum = i;
			return vc;
		}

		vc = vc->next;
	}

	return nullptr;
}

/*
============
BG_VoiceCmdByNum
============
*/
voiceCmd_t *BG_VoiceCmdByNum( voiceCmd_t *head, int num )
{
	voiceCmd_t *vc = head;
	int        i = 0;

	while ( vc )
	{
		i++;

		if ( i == num )
		{
			return vc;
		}

		vc = vc->next;
	}

	return nullptr;
}

/*
============
BG_VoiceTrackByNum
============
*/
voiceTrack_t *BG_VoiceTrackByNum( voiceTrack_t *head, int num )
{
	voiceTrack_t *vt = head;
	int          i = 0;

	while ( vt )
	{
		i++;

		if ( i == num )
		{
			return vt;
		}

		vt = vt->next;
	}

	return nullptr;
}

/*
============
BG_VoiceTrackFind
============
*/
voiceTrack_t *BG_VoiceTrackFind( voiceTrack_t *head, int team,
                                 int class_, int,
                                 int enthusiasm, int *trackNum )
{
	voiceTrack_t *vt = head;
	int          highestMatch = 0;
	int          matchCount = 0;
	int          selectedMatch = 0;
	int          i = 0;
	int          j = 0;

	// find highest enthusiasm without going over
	while ( vt )
	{
		if ( ( vt->team >= 0 && !( vt->team  & ( 1 << team ) ) ) ||
		     ( vt->pClass >= 0 && !( vt->pClass & ( 1 << class_ ) ) ) ||
		     vt->enthusiasm > enthusiasm )
		{
			vt = vt->next;
			continue;
		}

		if ( vt->enthusiasm > highestMatch )
		{
			matchCount = 0;
			highestMatch = vt->enthusiasm;
		}

		if ( vt->enthusiasm == highestMatch )
		{
			matchCount++;
		}

		vt = vt->next;
	}

	if ( !matchCount )
	{
		return nullptr;
	}

	// return randomly selected match
	selectedMatch = rand() / ( RAND_MAX / matchCount + 1 );
	vt = head;
	i = 0;
	j = 0;

	while ( vt )
	{
		j++;

		if ( ( vt->team >= 0 && !( vt->team  & ( 1 << team ) ) ) ||
		     ( vt->pClass >= 0 && !( vt->pClass & ( 1 << class_ ) ) ) ||
		     vt->enthusiasm != highestMatch )
		{
			vt = vt->next;
			continue;
		}

		if ( i == selectedMatch )
		{
			*trackNum = j;
			return vt;
		}

		i++;
		vt = vt->next;
	}

	return nullptr;
}
