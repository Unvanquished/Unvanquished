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

void S_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
    Audio::StartSound(entnum, origin, sfx);
}

void S_StartLocalSound( sfxHandle_t sfx, int channelNum )
{
    Audio::StartLocalSound(sfx);
}

void S_StartBackgroundTrack( const char *intro, const char *loop )
{
    Audio::StartMusic(intro, loop);
}

void S_StopBackgroundTrack( void )
{
    Audio::StopMusic();
}

void S_RawSamples( int stream, int samples, int rate, int width, int channels,
                   const byte *data, float volume, int entityNum )
{
    Audio::StreamData(stream, data, samples, rate, width, volume, entityNum);
}

void S_StopAllSounds( void )
{
    Audio::StopMusic();
}

void S_ClearLoopingSounds( qboolean killall )
{
    Audio::ClearAllLoopingSounds();
}

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

void S_StopLoopingSound( int entityNum )
{
    Audio::ClearLoopingSoundsForEntity(entityNum);
}

void S_Respatialize( int entityNum, const vec3_t origin,
                     vec3_t axis[3], int inwater )
{
    if (entityNum >= 0 and entityNum < MAX_GENTITIES) {
        Audio::UpdateEntityPosition(entityNum, origin);
    }
	Audio::UpdateListener(entityNum, axis);
}

void S_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
    Audio::UpdateEntityPosition(entityNum, origin);
}

void S_UpdateEntityVelocity( int entityNum, const vec3_t velocity )
{
    Audio::UpdateEntityVelocity(entityNum, velocity);
}

void S_SetReverb( int slotNum, const char* name, float ratio )
{
	Audio::SetReverb(slotNum, name, ratio);
}

void S_Update( void )
{
    Audio::Update();
}

void S_DisableSounds( void )
{
    S_StopAllSounds();
}

void S_BeginRegistration( void )
{
}

sfxHandle_t	S_RegisterSound( const char *sample, qboolean compressed )
{
    return Audio::RegisterSFX(sample);
}

void S_ClearSoundBuffer( void )
{
}

void S_StartCapture( void )
{
    Audio::StartCapture(8000);
}

int S_AvailableCaptureSamples( void )
{
    return Audio::AvailableCaptureSamples();
}

void S_Capture( int samples, byte *data )
{
    Audio::GetCapturedData(samples, data);
}

void S_StopCapture( void )
{
    Audio::StopCapture();
}

void S_MasterGain( float gain )
{
}

void S_Init( void )
{
	Com_Printf( "------ Initializing Sound ------\n" );

    S_CodecInit( );
    bool started = Audio::Init();

    if ( started )
    {
        Com_Printf( "Sound initialization successful.\n" );
    }

    else
    {
        Com_Printf( "Sound initialization failed.\n" );
    }

	Com_Printf( "--------------------------------\n" );
}

void S_Shutdown( void )
{
    Audio::Shutdown();
	S_CodecShutdown( );
}

