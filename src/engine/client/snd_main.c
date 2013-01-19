/*
===========================================================================
This file is part of Daemon.

OpenTrem is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenTrem is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*
 * snd_load.c
 * Loads a sound module, and redirects calls to it
 */

#include "client.h"
#include "snd_codec.h"
#include "snd_local.h"
#include "snd_public.h"

static soundInterface_t si;

cvar_t                *s_volume;
cvar_t                *s_testsound;
cvar_t                *s_show;
cvar_t                *s_mixahead;
cvar_t                *s_mixPreStep;
cvar_t                *s_musicVolume;
cvar_t                *s_separation;
cvar_t                *s_doppler;

/*
 * 0: unmuted
 * 1: mute all
 * 2: mute only local sounds and music
 * 3: mute only gamesounds and music
 */
cvar_t                *s_mute;
cvar_t                *s_muteWhenMinimized;
cvar_t                *s_muteWhenUnfocused;


void S_Shutdown( void )
{
	if( si.Shutdown )
	{
		si.Shutdown( );
	}

	codec_shutdown();

	Cmd_RemoveCommand( "play" );
	Cmd_RemoveCommand( "music" );
	Cmd_RemoveCommand( "stopsound" );
	Cmd_RemoveCommand( "soundlist" );
	Cmd_RemoveCommand( "soundinfo" );
}

/**
 * the mute mode for the current client state (focus, unfocused, minimized)
 *
 * @returns 0 if not muted,
 *			1 if everything is muted,
 *	 	 	2 for having all but non-local gamesounds muted (to strip most of what is optional to the gameplay),
 * 			3 for having all but local sounds muted (great for focusing on chatnotifications while minimized)
 */
int S_IsMuted( void )
{
	if ( com_minimized->integer )
		return s_muteWhenMinimized->integer;

	if ( com_unfocused->integer )
		return s_muteWhenUnfocused->integer;

	return s_mute->integer;
}

/**
 * used for most ingame sounds like steps, damage taken or shooting
 * (excluding e.g. the saw, flamer or barbs flying through the air)
 */
void S_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
	if( S_IsMuted() && S_IsMuted() != 2 )
		return;

	if( si.StartSound )
	{
		si.StartSound( origin, entnum, entchannel, sfx );
	}
}

void S_StartSoundEx( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
	if( si.StartSoundEx )
	{
		si.StartSoundEx( origin, entnum, entchannel, sfx );
	}
}

/**
 * start a local (to the player) sound, that is not placed anywhere
 * particular in the world.
 *
 * This includes chat notifications or menu selection-sounds,
 * but is also used for the vocal notifications about the overmind being attacked etc.
 */
void S_StartLocalSound( sfxHandle_t sfx, int channelNum )
{
	if( S_IsMuted() && S_IsMuted() != 3 )
		return;

	if( si.StartLocalSound )
	{
		si.StartLocalSound( sfx, channelNum );
	}
}

void S_StartBackgroundTrack( const char *intro, const char *loop )
{
	if( si.StartBackgroundTrack )
	{
		si.StartBackgroundTrack( intro, loop );
	}
}

void S_StopBackgroundTrack( void )
{
	if( si.StopBackgroundTrack )
	{
		si.StopBackgroundTrack( );
	}
}

void S_RawSamples( int stream, int samples, int rate, int width, int channels,
                   const byte *data, float volume, int entityNum )
{
	if( si.RawSamples )
	{
		si.RawSamples( stream, samples, rate, width, channels, data, volume, entityNum );
	}
}

void S_ClearLoopingSounds( qboolean killall )
{
	if( si.ClearLoopingSounds )
	{
		si.ClearLoopingSounds( killall );
	}
}

/**
 * add a looping sound
 * used for many temporary environment sounds that loop while being active,
 * like the idle sounds of base-buildings, saws,
 * flame-throwers and the sound of a building being assembled or growing
 */
void S_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
	if( S_IsMuted() && S_IsMuted() != 2 )
		return;

	if( si.AddLoopingSound )
	{
		si.AddLoopingSound( entityNum,  origin, velocity, sfx );
	}
}

/**
 * add a looping sound that will remain active
 * used for many environment sounds like those defined by maps, that will continue during the game
 */
void S_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
	if( S_IsMuted() )
		return;

	if( si.AddRealLoopingSound )
	{
		si.AddRealLoopingSound( entityNum, origin, velocity, sfx );
	}
}

void S_StopLoopingSound( int entityNum )
{
	if( si.StopLoopingSound )
	{
		si.StopLoopingSound( entityNum );
	}
}

void S_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[ 3 ], int inwater )
{
	if( si.Respatialize )
	{
		si.Respatialize( entityNum, origin, axis, inwater );
	}
}

void S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	if( si.UpdateEntityPosition )
	{
		si.UpdateEntityPosition( entityNum, origin );
	}
}

void S_Update( void )
{
	/*
	 * explicit stop as fix for the DMA backend continuing looping without regard to s_mute-cvars
	 *
	 * since this is mostly limited to environment sounds and a restart is prevented either way
	 * it should not produce any stutter as former solutions did via S_Update
	 *
	 */
	if (S_IsMuted())
	{
		S_ClearLoopingSounds(qtrue);
	}

	if( si.Update )
	{
		si.Update( );
	}
}

void S_DisableSounds( void )
{
	if( si.DisableSounds )
	{
		si.DisableSounds( );
	}
}

void S_BeginRegistration( void )
{
	if( si.BeginRegistration )
	{
		si.BeginRegistration( );
	}
}

sfxHandle_t S_RegisterSound( const char *sample, qboolean compressed )
{
	if( si.RegisterSound )
	{
		return si.RegisterSound( sample, compressed );
	}
	else
	{
		return 0;
	}
}

void S_ClearSoundBuffer( void )
{
	if( si.ClearSoundBuffer )
	{
		si.ClearSoundBuffer( );
	}
}

int S_SoundDuration( sfxHandle_t handle )
{
	if( si.SoundDuration )
	{
		return si.SoundDuration( handle );
	}
	else
	{
		return 0;
	}
}

int S_GetVoiceAmplitude( int entnum )
{
	if ( si.GetVoiceAmplitude )
	{
		return si.GetVoiceAmplitude( entnum );
	}
	else
	{
		return 0;
	}
}
int S_GetSoundLength( sfxHandle_t sfxHandle )
{
	if( si.GetSoundLength )
	{
		return si.GetSoundLength( sfxHandle );
	}
	else
	{
		return 0;
	}
}

#ifdef USE_VOIP
void S_StartCapture( void )
{
	if( si.StartCapture )
	{
		si.StartCapture( );
	}
}

int S_AvailableCaptureSamples( void )
{
	if ( si.AvailableCaptureSamples )
	{
		return si.AvailableCaptureSamples( );
	}
	else
	{
		return 0;
	}
}

void S_Capture( int samples, byte *data )
{
	if ( si.Capture )
	{
		si.Capture( samples, data );
	}
}

void S_StopCapture( void )
{
	if( si.StopCapture )
	{
		si.StopCapture( );
	}
}

void S_MasterGain( float gain )
{
	if( si.MasterGain )
	{
		si.MasterGain( gain );
	}
}

#endif

int S_GetCurrentSoundTime( void )
{
	if( si.GetCurrentSoundTime )
	{
		return si.GetCurrentSoundTime( );
	}
	else
	{
		return 0;
	}
}

void S_SoundInfo_f( void )
{
	if ( si.SoundInfo_f )
	{
		si.SoundInfo_f( );
	}
}
void  S_SoundList_f( void )
{
	if ( si.SoundList_f )
	{
		si.SoundList_f( );
	}
}
void S_StopAllSounds( void )
{
	if ( si.StopAllSounds )
	{
		si.StopAllSounds( );
	}
}

void S_Play_f( void )
{
	int         i;
	sfxHandle_t h;
	char        name[ 256 ];

	i = 1;

	while ( i < Cmd_Argc() )
	{
		if ( !Q_strrchr( Cmd_Argv( i ), '.' ) )
		{
			Com_sprintf( name, sizeof( name ), "%s.wav", Cmd_Argv( 1 ) );
		}
		else
		{
			Q_strncpyz( name, Cmd_Argv( i ), sizeof( name ) );
		}

		h = S_RegisterSound( name, qfalse );

		if ( h )
		{
			S_StartLocalSound( h, CHAN_LOCAL_SOUND );
		}

		i++;
	}
}

void S_Music_f( void )
{
	int c;

	c = Cmd_Argc();

	if ( c == 2 )
	{
		S_StartBackgroundTrack( Cmd_Argv( 1 ), NULL );
	}
	else if ( c == 3 )
	{
		S_StartBackgroundTrack( Cmd_Argv( 1 ), Cmd_Argv( 2 ) );
	}
	else
	{
		Com_Printf( "music <musicfile> [loopfile]\n" );
		return;
	}
}


void S_Init( void )
{
	cvar_t *cv;
	qboolean  started = qfalse;

	Com_Printf("%s", _( "------ Initializing Sound -----\n" ));

	cv = Cvar_Get( "s_initsound", "1", 0 );
	if ( !cv->integer )
	{
		Com_Printf( "Sound Disabled\n" );
		return;
	}
	else
	{
		codec_init();

		s_volume = Cvar_Get( "s_volume", "0.8", CVAR_ARCHIVE );
		s_musicVolume = Cvar_Get( "s_musicvolume", "0.25", CVAR_ARCHIVE );
		s_separation = Cvar_Get( "s_separation", "0.5", CVAR_ARCHIVE );
		s_doppler = Cvar_Get( "s_doppler", "1", CVAR_ARCHIVE );
		s_mixahead = Cvar_Get( "s_mixahead", "0.2", CVAR_ARCHIVE );

		s_mute = Cvar_Get( "s_mute", "0", CVAR_TEMP );
		s_muteWhenMinimized = Cvar_Get( "s_muteWhenMinimized", "1", CVAR_ARCHIVE );
		s_muteWhenUnfocused = Cvar_Get( "s_muteWhenUnfocused", "0", CVAR_ARCHIVE );

		s_mixPreStep = Cvar_Get( "s_mixPreStep", "0.05", CVAR_ARCHIVE );
		s_show = Cvar_Get( "s_show", "0", CVAR_CHEAT );
		s_testsound = Cvar_Get( "s_testsound", "0", CVAR_CHEAT );

		cv = Cvar_Get( "s_useOpenAL", "1", CVAR_ARCHIVE );
		if( cv->integer )
		{
			started = S_AL_Init( &si );
		}

		if( !started )
		{
			Com_Printf( "Using builtin sound backend\n" );
			S_Base_Init( &si );
		}
		else
		{
			Com_Printf( "Using OpenAL sound backend\n" );
		}

		Cmd_AddCommand( "play", S_Play_f );
		Cmd_AddCommand( "music", S_Music_f );
		Cmd_AddCommand( "s_list", S_SoundList_f );
		Cmd_AddCommand( "s_info", S_SoundInfo_f );
		Cmd_AddCommand( "s_stop", S_StopAllSounds );
	}
	Com_Printf( "------------------------------------\n" );
}
