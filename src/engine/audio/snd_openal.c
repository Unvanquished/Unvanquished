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

#define ALC_ALL_DEVICES_SPECIFIER 0x1013

#include <al.h>
#include <alc.h>

// Console variables specific to OpenAL
cvar_t *s_alDevice;
cvar_t *s_alInputDevice;
cvar_t *s_alAvailableDevices;
cvar_t *s_alAvailableInputDevices;


static ALCdevice *alDevice;
static ALCcontext *alContext;

/*
=================
S_AL_Shutdown
=================
*/
void S_AL_Shutdown( void )
{
    Audio::Shutdown();

	alcDestroyContext( alContext );
	alcCloseDevice( alDevice );
}

/*
=================
S_AL_Init
=================
*/
qboolean S_AL_Init()
{
	const char* device = NULL;
	const char* inputdevice = NULL;

	// New console variables
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
	alDopplerFactor( 1.0f );
	alDopplerVelocity( 2200 );

	return qtrue;
}

