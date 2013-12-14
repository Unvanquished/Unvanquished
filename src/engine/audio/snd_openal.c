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

#ifndef BUILD_TTY_CLIENT

#define ALC_ALL_DEVICES_SPECIFIER 0x1013

#include <al.h>
#include <alc.h>

// Console variables specific to OpenAL
cvar_t *s_alPrecache;
cvar_t *s_alGain;
cvar_t *s_alSources;
cvar_t *s_alDopplerFactor;
cvar_t *s_alDopplerSpeed;
cvar_t *s_alMinDistance;
cvar_t *s_alMaxDistance;
cvar_t *s_alRolloff;
cvar_t *s_alGraceDistance;
cvar_t *s_alDevice;
cvar_t *s_alInputDevice;
cvar_t *s_alAvailableDevices;
cvar_t *s_alAvailableInputDevices;

/*
=================
S_AL_ErrorMsg
=================
*/
static const char *S_AL_ErrorMsg( ALenum error )
{
	switch ( error )
	{
		case AL_NO_ERROR:
			return "No error";

		case AL_INVALID_NAME:
			return "Invalid name";

		case AL_INVALID_ENUM:
			return "Invalid enumerator";

		case AL_INVALID_VALUE:
			return "Invalid value";

		case AL_INVALID_OPERATION:
			return "Invalid operation";

		case AL_OUT_OF_MEMORY:
			return "Out of memory";

		default:
			return "Unknown error";
	}
}

/*
=================
S_AL_ClearError
=================
*/
static void S_AL_ClearError( qboolean quiet )
{
	int error = alGetError();

	if ( quiet )
		return;

	if ( error != AL_NO_ERROR )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: unhandled AL error: %s\n",
		            S_AL_ErrorMsg( error ) );
	}
}


//===========================================================================


sfxHandle_t S_AL_RegisterSound(const char* sample, qboolean) {
    return Audio::RegisterSFX(sample);
}

static
void S_AL_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
    Audio::UpdateEntityPosition(entityNum, origin);
}

static
void S_AL_UpdateEntityVelocity( int entityNum, const vec3_t velocity )
{
    Audio::UpdateEntityVelocity(entityNum, velocity);
}

static
void S_AL_UpdateEntityOcclusion( int entityNum, qboolean occluded, float ratio )
{
}

static
void S_AL_StartLocalSound( sfxHandle_t sfx, int channel )
{
    Audio::StartLocalSound(sfx);
}

static void S_AL_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
    Audio::StartSound(entnum, origin, sfx);
}

static
void S_AL_ClearLoopingSounds( qboolean killall )
{
    Audio::ClearAllLoopingSounds();
}

static void S_AL_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
    if (origin) {
        Audio::UpdateEntityPosition(entityNum, origin);
    }

    if (velocity) {
        Audio::UpdateEntityVelocity(entityNum, velocity);
    }

    Audio::AddEntityLoopingSound(entityNum, sfx);
}

static void S_AL_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
    if (origin) {
        Audio::UpdateEntityPosition(entityNum, origin);
    }

    if (velocity) {
        Audio::UpdateEntityVelocity(entityNum, velocity);
    }

    Audio::AddEntityLoopingSound(entityNum, sfx);
}

static
void S_AL_StopLoopingSound( int entityNum )
{
    Audio::ClearLoopingSoundsForEntity(entityNum);
}

static void S_AL_AllocateStreamChannel( int stream, int entityNum )
{
}

static void S_AL_FreeStreamChannel( int stream )
{
}

static
void S_AL_RawSamples( int stream, int samples, int rate, int width, int channels, const byte *data, float volume, int entityNum )
{
}

static
void S_AL_StreamUpdate( int stream )
{
}

static
void S_AL_StreamDie( int stream )
{
}

static
void S_AL_StopBackgroundTrack( void )
{
}
static
void S_AL_StartBackgroundTrack( const char *intro, const char *loop )
{
}

static ALCdevice *alDevice;
static ALCcontext *alContext;

static
void S_AL_StopAllSounds( void )
{
}

/*
=================
S_AL_Respatialize
=================
*/
static
void S_AL_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater )
{
	float		orientation[6];
	vec3_t	sorigin;

	VectorCopy( origin, sorigin );

	orientation[0] = axis[0][0];
	orientation[1] = axis[0][1];
	orientation[2] = axis[0][2];
	orientation[3] = axis[2][0];
	orientation[4] = axis[2][1];
	orientation[5] = axis[2][2];

	// Set OpenAL listener paramaters
	alListenerfv( AL_POSITION, ( ALfloat * )sorigin );
	alListenerfv( AL_VELOCITY, vec3_origin );
	alListenerfv( AL_ORIENTATION, orientation );
}

/*
=================
S_AL_Update
=================
*/
static
void S_AL_Update( void )
{
    Audio::Update();
}

/*
=================
S_AL_DisableSounds
=================
*/
static
void S_AL_DisableSounds( void )
{
	S_AL_StopAllSounds();
}

/*
=================
S_AL_BeginRegistration
=================
*/
static
void S_AL_BeginRegistration( void )
{
}

/*
=================
S_AL_ClearSoundBuffer
=================
*/
static
void S_AL_ClearSoundBuffer( void )
{
}

/*
=================
S_AL_SoundList
=================
*/
static
void S_AL_SoundList( void )
{
}

/*
=================
S_AL_SoundInfo
=================
*/
static void S_AL_SoundInfo( void )
{
	Com_Printf( "OpenAL info:\n" );
	Com_Printf( "  Vendor:         %s\n", alGetString( AL_VENDOR ) );
	Com_Printf( "  Version:        %s\n", alGetString( AL_VERSION ) );
	Com_Printf( "  Renderer:       %s\n", alGetString( AL_RENDERER ) );
	Com_Printf( "  AL Extensions:  %s\n", alGetString( AL_EXTENSIONS ) );
	Com_Printf( "  ALC Extensions: %s\n", alcGetString( alDevice, ALC_EXTENSIONS ) );
	Com_Printf( "  Device:         %s\n", alcGetString( alDevice, ALC_ALL_DEVICES_SPECIFIER ) );
	Com_Printf( "  Available Devices:\n%s", s_alAvailableDevices->string );
}


/*
=================
S_AL_Shutdown
=================
*/
static
void S_AL_Shutdown( void )
{
    Audio::Shutdown();

	alcDestroyContext( alContext );
	alcCloseDevice( alDevice );
}
#endif

/*
=================
S_AL_Init
=================
*/
qboolean S_AL_Init( soundInterface_t *si )
{
#ifndef BUILD_TTY_CLIENT
	const char* device = NULL;
	const char* inputdevice = NULL;
	int i;

	if ( !si )
	{
		return qfalse;
	}

	// New console variables
	s_alPrecache = Cvar_Get( "s_alPrecache", "1", CVAR_ARCHIVE );
	s_alGain = Cvar_Get( "s_alGain", "1.0", CVAR_ARCHIVE );
	s_alSources = Cvar_Get( "s_alSources", "96", CVAR_ARCHIVE );
	s_alDopplerFactor = Cvar_Get( "s_alDopplerFactor", "1.0", CVAR_ARCHIVE );
	s_alDopplerSpeed = Cvar_Get( "s_alDopplerSpeed", "2200", CVAR_ARCHIVE );
	s_alMinDistance = Cvar_Get( "s_alMinDistance", "120", CVAR_CHEAT );
	s_alMaxDistance = Cvar_Get( "s_alMaxDistance", "1024", CVAR_CHEAT );
	s_alRolloff = Cvar_Get( "s_alRolloff", "2", CVAR_CHEAT );
	s_alGraceDistance = Cvar_Get( "s_alGraceDistance", "512", CVAR_CHEAT );

	s_alInputDevice = Cvar_Get( "s_alInputDevice", "", CVAR_ARCHIVE | CVAR_LATCH );
	s_alDevice = Cvar_Get( "s_alDevice", "", CVAR_ARCHIVE | CVAR_LATCH );

	// Load al
	device = s_alDevice->string;

	if ( device && !*device )
		device = NULL;

	inputdevice = s_alInputDevice->string;

	if ( inputdevice && !*inputdevice )
		inputdevice = NULL;


	// Device enumeration support
	char devicenames[16384] = "";
	const char *devicelist;
	int curlen;

	// get all available devices + the default device name.
	devicelist = alcGetString( NULL, ALC_ALL_DEVICES_SPECIFIER );

	// dump a list of available devices to a cvar for the user to see.

	if ( devicelist )
	{
		while ( ( curlen = strlen( devicelist ) ) )
		{
			Q_strcat( devicenames, sizeof( devicenames ), devicelist );
			Q_strcat( devicenames, sizeof( devicenames ), "\n" );

			devicelist += curlen + 1;
		}
	}

	s_alAvailableDevices = Cvar_Get( "s_alAvailableDevices", devicenames, CVAR_ROM | CVAR_NORESTART );

	alDevice = alcOpenDevice( device );

	if ( !alDevice && device )
	{
		Com_Printf( "Failed to open OpenAL device '%s', trying default.\n", device );
		alDevice = alcOpenDevice( NULL );
	}

	if ( !alDevice )
	{
		Com_Printf( "Failed to open OpenAL device.\n" );
		return qfalse;
	}

	// Create OpenAL context
	alContext = alcCreateContext( alDevice, NULL );

	if ( !alContext )
	{
		alcCloseDevice( alDevice );
		Com_Printf( "Failed to create OpenAL context.\n" );
		return qfalse;
	}

	alcMakeContextCurrent( alContext );

	// Initialize sources, buffers, music
    Audio::Init();

	// Set up OpenAL parameters (doppler, etc)
	alDistanceModel( AL_INVERSE_DISTANCE_CLAMPED );
	alDopplerFactor( s_alDopplerFactor->value );
	alDopplerVelocity( s_alDopplerSpeed->value );

	si->Shutdown = S_AL_Shutdown;
	si->StartSound = S_AL_StartSound;
	si->StartLocalSound = S_AL_StartLocalSound;
	si->StartBackgroundTrack = S_AL_StartBackgroundTrack;
	si->StopBackgroundTrack = S_AL_StopBackgroundTrack;
	si->RawSamples = S_AL_RawSamples;
	si->StopAllSounds = S_AL_StopAllSounds;
	si->ClearLoopingSounds = S_AL_ClearLoopingSounds;
	si->AddLoopingSound = S_AL_AddLoopingSound;
	si->AddRealLoopingSound = S_AL_AddRealLoopingSound;
	si->StopLoopingSound = S_AL_StopLoopingSound;
	si->Respatialize = S_AL_Respatialize;
	si->UpdateEntityPosition = S_AL_UpdateEntityPosition;
	si->UpdateEntityVelocity = S_AL_UpdateEntityVelocity;
	si->UpdateEntityOcclusion = S_AL_UpdateEntityOcclusion;
	si->Update = S_AL_Update;
	si->DisableSounds = S_AL_DisableSounds;
	si->BeginRegistration = S_AL_BeginRegistration;
	si->RegisterSound = S_AL_RegisterSound;
	si->ClearSoundBuffer = S_AL_ClearSoundBuffer;
	si->SoundInfo = S_AL_SoundInfo;
	si->SoundList = S_AL_SoundList;

	return qtrue;
#else
	return qfalse;
#endif
}

