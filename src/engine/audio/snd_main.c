/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)

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

#include "AudioPrivate.h"

cvar_t *s_volume;
cvar_t *s_muted;
cvar_t *s_musicVolume;
cvar_t *s_backend;
cvar_t *s_muteWhenMinimized;
cvar_t *s_muteWhenUnfocused;

/*
=================
S_StartSound
=================
*/
void S_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
    Audio::StartSound(entnum, origin, sfx);
}

/*
=================
S_StartLocalSound
=================
*/
void S_StartLocalSound( sfxHandle_t sfx, int channelNum )
{
    Audio::StartLocalSound(sfx);
}

/*
=================
S_StartBackgroundTrack
=================
*/
void S_StartBackgroundTrack( const char *intro, const char *loop )
{
    Audio::StartMusic(intro, loop);
}

/*
=================
S_StopBackgroundTrack
=================
*/
void S_StopBackgroundTrack( void )
{
    Audio::StopMusic();
}

/*
=================
S_RawSamples
=================
*/
void S_RawSamples( int stream, int samples, int rate, int width, int channels,
                   const byte *data, float volume, int entityNum )
{
    Audio::StreamData(stream, data, samples, rate, width, volume, entityNum);
}

/*
=================
S_StopAllSounds
=================
*/
void S_StopAllSounds( void )
{
    Audio::StopMusic();
}

/*
=================
S_ClearLoopingSounds
=================
*/
void S_ClearLoopingSounds( qboolean killall )
{
    Audio::ClearAllLoopingSounds();
}

/*
=================
S_AddLoopingSound
=================
*/
void S_AddLoopingSound( int entityNum, const vec3_t origin,
                        const vec3_t velocity, sfxHandle_t sfx )
{
    if (origin) {
        Audio::UpdateEntityPosition(entityNum, origin);
    }

    if (velocity) {
        Audio::UpdateEntityVelocity(entityNum, velocity);
    }

    Audio::AddEntityLoopingSound(entityNum, sfx);
}

/*
=================
S_AddRealLoopingSound
=================
*/
void S_AddRealLoopingSound( int entityNum, const vec3_t origin,
                            const vec3_t velocity, sfxHandle_t sfx )
{
    if (origin) {
        Audio::UpdateEntityPosition(entityNum, origin);
    }

    if (velocity) {
        Audio::UpdateEntityVelocity(entityNum, velocity);
    }

    Audio::AddEntityLoopingSound(entityNum, sfx);
}

/*
=================
S_StopLoopingSound
=================
*/
void S_StopLoopingSound( int entityNum )
{
    Audio::ClearLoopingSoundsForEntity(entityNum);
}

/*
=================
S_Respatialize
=================
*/
void S_Respatialize( int entityNum, const vec3_t origin,
                     vec3_t axis[3], int inwater )
{
    if (entityNum >= 0 and entityNum < MAX_GENTITIES) {
        Audio::UpdateEntityPosition(entityNum, origin);
    }
	Audio::UpdateListener(entityNum, axis);
}

/*
=================
S_UpdateEntityPosition
=================
*/
void S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
    Audio::UpdateEntityPosition(entityNum, origin);
}

/*
=================
S_UpdateEntityVelocity
=================
*/
void S_UpdateEntityVelocity( int entityNum, const vec3_t velocity )
{
    Audio::UpdateEntityVelocity(entityNum, velocity);
}

/*
=================
S_UpdateEntityPosition
=================
*/
void S_UpdateEntityOcclusion( int entityNum, qboolean occluded, float ratio )
{
}

/*
=================
S_Update
=================
*/
void S_Update( void )
{
	if ( s_muted->integer )
	{
		if ( !( s_muteWhenMinimized->integer && com_minimized->integer ) &&
		        !( s_muteWhenUnfocused->integer && com_unfocused->integer ) )
		{
			s_muted->integer = qfalse;
			s_muted->modified = qtrue;
		}
	}

	else
	{
		if ( ( s_muteWhenMinimized->integer && com_minimized->integer ) ||
		        ( s_muteWhenUnfocused->integer && com_unfocused->integer ) )
		{
			s_muted->integer = qtrue;
			s_muted->modified = qtrue;
		}
	}

    Audio::Update();
}

/*
=================
S_DisableSounds
=================
*/
void S_DisableSounds( void )
{
    S_StopAllSounds();
}

/*
=================
S_BeginRegistration
=================
*/
void S_BeginRegistration( void )
{
}

/*
=================
S_RegisterSound
=================
*/
sfxHandle_t	S_RegisterSound( const char *sample, qboolean compressed )
{
    return Audio::RegisterSFX(sample);
}

/*
=================
S_ClearSoundBuffer
=================
*/
void S_ClearSoundBuffer( void )
{
}

/*
=================
S_SoundInfo
=================
*/
void S_SoundInfo( void )
{
}

/*
=================
S_SoundList
=================
*/
void S_SoundList( void )
{
}


#ifdef USE_VOIP
/*
=================
S_StartCapture
=================
*/
void S_StartCapture( void )
{
    Audio::StartCapture();
}

/*
=================
S_AvailableCaptureSamples
=================
*/
int S_AvailableCaptureSamples( void )
{
    return Audio::AvailableCaptureSamples();
}

/*
=================
S_Capture
=================
*/
void S_Capture( int samples, byte *data )
{
    Audio::GetCapturedDate(samples, data);
}

/*
=================
S_StopCapture
=================
*/
void S_StopCapture( void )
{
    Audio::StopCapture();
}

/*
=================
S_MasterGain
=================
*/
void S_MasterGain( float gain )
{
}
#endif

//=============================================================================

/*
=================
S_Init
=================
*/
void S_Init( void )
{
	cvar_t		*cv;
	qboolean	started = qfalse;

	Com_Printf( "------ Initializing Sound ------\n" );

	s_volume = Cvar_Get( "s_volume", "0.8", CVAR_ARCHIVE );
	s_musicVolume = Cvar_Get( "s_musicvolume", "0.25", CVAR_ARCHIVE );
	s_muted = Cvar_Get( "s_muted", "0", CVAR_ROM );
	s_backend = Cvar_Get( "s_backend", "", CVAR_ROM );
	s_muteWhenMinimized = Cvar_Get( "s_muteWhenMinimized", "0", CVAR_ARCHIVE );
	s_muteWhenUnfocused = Cvar_Get( "s_muteWhenUnfocused", "0", CVAR_ARCHIVE );

	cv = Cvar_Get( "s_initsound", "1", 0 );

	if ( !cv->integer )
	{
		Com_Printf( "Sound disabled.\n" );
	}

	else
	{

		S_CodecInit( );

        //OpenAL
        started = Audio::Init();

		if ( started )
		{
			S_SoundInfo( );
			Com_Printf( "Sound initialization successful.\n" );
		}

		else
		{
            //TODO should be an error?
			Com_Printf( "Sound initialization failed.\n" );
		}
	}

	Com_Printf( "--------------------------------\n" );
}

/*
=================
S_Shutdown
=================
*/
void S_Shutdown( void )
{
    Audio::Shutdown();
	S_CodecShutdown( );
}

