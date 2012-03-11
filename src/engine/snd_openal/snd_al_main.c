/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2005 Stuart Dalton (badcdev@gmail.com)
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

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
along with Tremulous; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

/*
 * snd_al_main.c
 * Main section of OpenAL audio system
 *
 * 2005-08-22
 *  Started logging
 *  Split buffer management out from snd_al.c
 *
 * 2005-08-23
 *  Reworked to use sfxHandles instead of re-cast pointers. What was I thinking?
 *  New initialization code using alc instead of alut
 *  Renamed to snd_al_main.c
 *  Split source management out from snd_al_main.c, reworked it a bit
 *  More error checking
 *  Moved music stuff to snd_al_music.c
 *
 * 2005-08-27
 *  Switched back to ALUT for init and shutdown
 *
 * 2005-08-28
 *  Made things shut up properly
 *  Added stream system
 *  Added support for changing doppler paramaters without restarting
 *  Added checks for official Linux OpenAL
 *  Added workaround for Linux hang on shutdown bug
 *  Wiped out s_khz cvar
 *
 * 2005-08-30
 *  Added s_alDriver
 *  Added QAL hooks
 *
 * 2005-08-31
 *  Reverted to ALC for init and shutdown
 *  Remove Play and Music commands on shutdown
 *  Totally wiped out ALUT
 *
 * 2005-09-01
 *  Properly flagged AL inited
 *
 * 2005-09-09
 *  DLL interface
 *
 * 2005-09-11
 *  Fixed on Windows
 */

#include "snd_al_local.h"

sndexport_t se;
sndimport_t si;

void S_Play_f( void );
void S_Music_f( void );

#ifdef _MSC_VER
#define snprintf _snprintf
#endif

/*
 * Util
 */
ALuint al_format(int width, int channels)
{
	ALuint format = AL_FORMAT_MONO16;

	// Work out format
	if(width == 1)
	{
		if(channels == 1)
			format = AL_FORMAT_MONO8;
		else if(channels == 2)
			format = AL_FORMAT_STEREO8;
	}
	else if(width == 2)
	{
		if(channels == 1)
			format = AL_FORMAT_MONO16;
		else if(channels == 2)
			format = AL_FORMAT_STEREO16;
	}

	return format;
}

/*
 * OpenAL error messages
 */
char *al_errormsg(ALenum error)
{
	switch(error)
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
 * Local state variables
 */
static qboolean snd_shutdown_bug = qfalse;
static ALCdevice *alDevice;
static ALCcontext *alContext;
#ifdef USE_VOIP 
static ALCdevice *alCaptureDevice;
static cvar_t *s_alCapture;
#endif

/*
 * Console variables
 */
cvar_t *s_volume;
cvar_t *s_musicVolume;
cvar_t *s_doppler;
cvar_t *s_precache;
cvar_t *s_gain;
cvar_t *s_sources;
cvar_t *s_dopplerFactor;
cvar_t *s_dopplerSpeed;
cvar_t *s_minDistance;
cvar_t *s_rolloff;
cvar_t *s_alDevice;
cvar_t *s_alAvailableDevices;
cvar_t *s_alAvailableInputDevices;

#ifdef USE_VOIP
static qboolean capture_ext = qfalse;
#endif

#ifdef USE_OPENAL_DLOPEN
cvar_t *s_alDriver;
#if defined( _WIN32 )
#	define ALDRIVER_DEFAULT "OpenAL32"
#elif defined( MACOS_X )
#	define ALDRIVER_DEFAULT "/System/Library/Frameworks/OpenAL.framework/OpenAL"
#else
#	define ALDRIVER_DEFAULT "libopenal.so.1"
#endif
#endif // USE_OPENAL_DLOPEN

qboolean SndAl_Init(void)
{
	const char* device = NULL;
	// Original console variables
	s_volume = si.Cvar_Get ("s_volume", "0.8", CVAR_ARCHIVE);
	s_musicVolume = si.Cvar_Get ("s_musicvolume", "0.25", CVAR_ARCHIVE);
	s_doppler = si.Cvar_Get ("s_doppler", "1", CVAR_ARCHIVE);

	// New console variables
	s_precache = si.Cvar_Get ("al_precache", "1", CVAR_ARCHIVE);
	s_gain = si.Cvar_Get ("al_gain", "0.4", CVAR_ARCHIVE);
	s_sources = si.Cvar_Get ("al_sources", "64", CVAR_ARCHIVE);
	s_dopplerFactor = si.Cvar_Get ("al_dopplerfactor", "1.0", CVAR_ARCHIVE);
	s_dopplerSpeed = si.Cvar_Get ("al_dopplerspeed", "2200", CVAR_ARCHIVE);
	s_minDistance = si.Cvar_Get ("al_mindistance", "80", CVAR_ARCHIVE);
	s_rolloff = si.Cvar_Get ("al_rolloff", "0.25", CVAR_ARCHIVE);
	s_alDevice = si.Cvar_Get( "al_device", "", CVAR_ARCHIVE | CVAR_LATCH );
#ifdef USE_OPENAL_DLOPEN
	s_alDriver = si.Cvar_Get ("al_driver", ALDRIVER_DEFAULT, CVAR_ARCHIVE);
#endif // USE_OPENAL_DLOPEN

#ifdef USE_OPENAL_DLOPEN
	// Load QAL
	if(!QAL_Init(s_alDriver->string))
	{
		si.Printf(PRINT_ALL, "not initializing.\n");
		return qfalse;
	}
#endif // USE_OPENAL_DLOPEN
	// Open default device
	device = s_alDevice->string;
	if( device && !*device )
	  device = NULL;
	if(qalcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
	{
		char devicenames[1024] = "";
		const char *devicelist;
#ifdef _WIN32
		const char *defaultdevice;
#endif
		int curlen;
		
		// get all available devices + the default device name.
		devicelist = qalcGetString(NULL, ALC_DEVICE_SPECIFIER);
#ifdef _WIN32
		defaultdevice = qalcGetString(NULL, ALC_DEFAULT_DEVICE_SPECIFIER);


		// check whether the default device is generic hardware. If it is, change to
		// Generic Software as that one works more reliably with various sound systems.
		// If it's not, use OpenAL's default selection as we don't want to ignore
		// native hardware acceleration.
		if(!device && !strcmp(defaultdevice, "Generic Hardware"))
			device = "Generic Software";
#endif

		// dump a list of available devices to a cvar for the user to see.
#ifndef MACOS_X
		while((curlen = strlen(devicelist)))
		{
			strcat(devicenames, devicelist);
			strcat(devicenames, "\n");

			devicelist += curlen + 1;
		}
#endif
		s_alAvailableDevices = si.Cvar_Get("al_AvailableDevices", devicenames, CVAR_ROM | CVAR_NORESTART);
	}

	alDevice = qalcOpenDevice(device);
	if( !alDevice && device )
	{
		si.Printf( PRINT_ALL,  "Failed to open OpenAL device '%s', trying default.\n", device );
		alDevice = qalcOpenDevice(NULL);
	}

	if( !alDevice )
	{
#ifdef USE_OPENAL_DLOPEN
		QAL_Shutdown();
#endif
		si.Printf( PRINT_ALL,  "Failed to open OpenAL device.\n" );
		return qfalse;
	}


	// Create OpenAL context
	alContext = qalcCreateContext(alDevice, NULL);
	if(!alContext)
	{
#ifdef USE_OPENAL_DLOPEN
		QAL_Shutdown();
#endif
		qalcCloseDevice(alDevice);
		si.Printf(PRINT_ALL, "Failed to create context\n");
		return qfalse;
	}
	qalcMakeContextCurrent(alContext);
	qalcProcessContext( alContext );

	// Print OpenAL information
	si.Printf(PRINT_ALL, "OpenAL initialised\n");
	si.Printf(PRINT_ALL, "  Vendor:     %s\n", qalGetString(AL_VENDOR));
	si.Printf(PRINT_ALL, "  Version:    %s\n", qalGetString(AL_VERSION));
	si.Printf(PRINT_ALL, "  Renderer:   %s\n", qalGetString(AL_RENDERER));
	si.Printf(PRINT_ALL, "  AL Extensions: %s\n", qalGetString(AL_EXTENSIONS));
	si.Printf( PRINT_ALL,  "  ALC Extensions: %s\n", qalcGetString( alDevice, ALC_EXTENSIONS ) );
	if(qalcIsExtensionPresent(NULL, "ALC_ENUMERATION_EXT"))
	{
		si.Printf( PRINT_ALL, "  Device:     %s\n", qalcGetString(alDevice, ALC_DEVICE_SPECIFIER));
		si.Printf( PRINT_ALL, "Available Devices:\n%s", s_alAvailableDevices->string);
	}
	

	// Check for Linux shutdown race condition
	if(!strcmp(qalGetString(AL_VENDOR), "J. Valenzuela"))
		snd_shutdown_bug = qtrue;

	// Initialize sources, buffers, music
	al_buf_init();
	al_src_init();

	// Set up OpenAL parameters (doppler, etc)
	qalDistanceModel(AL_INVERSE_DISTANCE_CLAMPED);
	qalDopplerFactor( s_dopplerFactor->value );
	qalDopplerVelocity( s_dopplerSpeed->value );;

	// Add commands
	si.Cmd_AddCommand("play", S_Play_f);
	si.Cmd_AddCommand("music", S_Music_f);
#ifdef USE_VOIP
 	SndAl_InitCapture( qtrue );
#endif


	// Init successful
	si.Printf(PRINT_ALL, "initialization successful\n");
	return qtrue;
}

void SndAl_Shutdown(void)
{
	// Remove commands
	si.Cmd_RemoveCommand("music");
	si.Cmd_RemoveCommand("play");

	// Shut down everything
	al_stream_die();
	SndAl_StopBackgroundTrack();
	al_src_shutdown();
	al_buf_shutdown();

	if(!snd_shutdown_bug)
		qalcMakeContextCurrent(NULL);
	qalcDestroyContext(alContext);
	qalcCloseDevice(alDevice);
	
#ifdef USE_VOIP
	if (alCaptureDevice != NULL) {
		qalcCaptureStop(alCaptureDevice);
		qalcCaptureCloseDevice(alCaptureDevice);
		alCaptureDevice = NULL;
		si.Printf( PRINT_ALL, "OpenAL capture device closed.\n" );
	}
#endif	

#ifdef USE_OPENAL_DLOPEN
	QAL_Shutdown();
#endif
}

void SndAl_StopAllSounds( void )
{
	al_src_shutup();
	SndAl_StopBackgroundTrack();
}

void SndAl_Respatialize( int entityNum, const vec3_t origin, vec3_t axis[3], int inwater )
{
	// Axis[0] = Forward
	// Axis[2] = Up
	float velocity[] = {0.0f, 0.0f, 0.0f};
	float orientation[] = {axis[0][0], axis[0][1], axis[0][2],
		axis[2][0], axis[2][1], axis[2][2]};
		vec3_t sorigin;

	// Set OpenAL listener paramaters
	VectorScale(origin, POSITION_SCALE, sorigin);
	qalListenerfv(AL_POSITION, sorigin);
	qalListenerfv(AL_VELOCITY, velocity);
	qalListenerfv(AL_ORIENTATION, orientation);
}

void SndAl_Update( void )
{
	// Update SFX channels
	al_src_update();

	// Update streams
	al_stream_update();
	al_mus_update();

	// Doppler
	if(s_doppler->modified)
	{
		s_dopplerFactor->modified = qtrue;
		s_doppler->modified = qfalse;
	}

	// Doppler parameters
	if(s_dopplerFactor->modified)
	{
		if(s_doppler->integer)
			qalDopplerFactor(s_dopplerFactor->value);
		else
			qalDopplerFactor(0.0f);
		s_dopplerFactor->modified = qfalse;
	}
	if(s_dopplerSpeed->modified)
	{
		qalDopplerVelocity(s_dopplerSpeed->value);
		s_dopplerSpeed->modified = qfalse;
	}

	// Clear the modified flags on the other cvars
	s_gain->modified = qfalse;
	s_volume->modified = qfalse;
	s_musicVolume->modified = qfalse;
	s_minDistance->modified = qfalse;
	s_rolloff->modified = qfalse;
}

void SndAl_DisableSounds( void )
{
	SndAl_StopAllSounds();
}

void SndAl_BeginRegistration( void )
{
}

void SndAl_ClearSoundBuffer( void )
{
}

DLLEXPORT sndexport_t *GetSndAPI( int apiVersion, sndimport_t *simp )
{
	si = *simp;

	memset( &se, 0, sizeof( se ) );

	if ( apiVersion != SND_API_VERSION ) {
		si.Printf(PRINT_ALL, "Mismatched SND_API_VERSION: expected %i, got %i\n",
			  SND_API_VERSION, apiVersion );
		return NULL;
	}

	se.Init = SndAl_Init;
	se.Shutdown = SndAl_Shutdown;
	se.StartSound = SndAl_StartSound;
	se.StartLocalSound = SndAl_StartLocalSound;
	se.StartBackgroundTrack = SndAl_StartBackgroundTrack;
	se.StopBackgroundTrack = SndAl_StopBackgroundTrack;
	se.RawSamples = SndAl_RawSamples;
	se.StopAllSounds = SndAl_StopAllSounds;
	se.ClearLoopingSounds = SndAl_ClearLoopingSounds;
	se.AddLoopingSound = SndAl_AddLoopingSound;
	se.AddRealLoopingSound = SndAl_AddRealLoopingSound;
	se.StopLoopingSound = SndAl_StopLoopingSound;
	se.Respatialize = SndAl_Respatialize;
	se.UpdateEntityPosition = SndAl_UpdateEntityPosition;
	se.Update = SndAl_Update;
	se.DisableSounds = SndAl_DisableSounds;
	se.BeginRegistration = SndAl_BeginRegistration;
	se.RegisterSound = SndAl_RegisterSound;
	se.ClearSoundBuffer = SndAl_ClearSoundBuffer;
	se.SoundDuration = SndAl_SoundDuration;
#ifdef USE_VOIP
	se.StartCapture = SndAl_StartCapture;
	se.AvailableCaptureSamples = SndAl_AvailableCaptureSamples;
	se.Capture = SndAl_Capture;
	se.StopCapture = SndAl_StopCapture;
	se.MasterGain = SndAl_MasterGain;
#endif
	se.GetVoiceAmplitude = SndAl_GetVoiceAmplitude;
	se.GetSoundLength = SndAl_GetSoundLength;
	se.GetCurrentSoundTime = SndAl_GetCurrentSoundTime;
	return &se;
}

/**
 * Commands
 */
void S_Play_f( void ) {
	int 		i;
	sfxHandle_t	h;
	char		name[256];
	
	i = 1;
	while ( i<si.Cmd_Argc() ) {
		if ( !strrchr(si.Cmd_Argv(i), '.') ) {
			snprintf( name, sizeof(name), "%s.wav", si.Cmd_Argv(1) );
		} else {
			strncpy( name, si.Cmd_Argv(i), sizeof(name) );
		}

		h = SndAl_RegisterSound( name, qfalse );
		if( h ) {
			SndAl_StartLocalSound( h, CHAN_LOCAL_SOUND );
		}
		i++;
	}
}

void S_Music_f( void ) {
	int		c;

	c = si.Cmd_Argc();

	if ( c == 2 ) {
		SndAl_StartBackgroundTrack( si.Cmd_Argv(1), si.Cmd_Argv(1) );
	} else if ( c == 3 ) {
		SndAl_StartBackgroundTrack( si.Cmd_Argv(1), si.Cmd_Argv(2) );
	} else {
		si.Printf(PRINT_ALL, "music <musicfile> [loopfile]\n");
		return;
	}

}

#ifdef USE_VOIP
void SndAl_InitCapture( qboolean usingAL )
{
	const char* inputdevice = NULL;

#ifdef USE_OPENAL_DLOPEN
	// Load QAL if we are called from the base sound driver
	if( !usingAL )
	{
		s_alDriver = si.Cvar_Get( "s_alDriver", ALDRIVER_DEFAULT, CVAR_ARCHIVE | CVAR_LATCH );
		if( !QAL_Init( s_alDriver->string ) )
		{
			si.Printf( PRINT_ALL, "Failed to load library: \"%s\".\n", s_alDriver->string );
			return;
		}
	}
#endif

	// !!! FIXME: some of these alcCaptureOpenDevice() values should be cvars.
	// !!! FIXME: add support for capture device enumeration.
	// !!! FIXME: add some better error reporting.
	s_alCapture = si.Cvar_Get( "s_alCapture", "1", CVAR_ARCHIVE | CVAR_LATCH );
	if (!s_alCapture->integer)
	{
		si.Printf( PRINT_ALL, "OpenAL capture support disabled by user ('+set s_alCapture 1' to enable)\n");
	}
	else
	{
		if (!qalcIsExtensionPresent(NULL, "ALC_EXT_capture"))
		{
			si.Printf( PRINT_ALL, "No ALC_EXT_CAPTURE support, can't record audio.\n");
		}
		else
		{
			char inputdevicenames[16384] = "";
			const char *inputdevicelist;
			const char *defaultinputdevice;
			int curlen;

			capture_ext = qtrue;

			// get all available input devices + the default input device name.
			inputdevicelist = qalcGetString(NULL, ALC_CAPTURE_DEVICE_SPECIFIER);
			defaultinputdevice = qalcGetString(NULL, ALC_CAPTURE_DEFAULT_DEVICE_SPECIFIER);

			// dump a list of available devices to a cvar for the user to see.
#ifndef MACOS_X
			while((curlen = strlen(inputdevicelist)))
			{
				si.strcat(inputdevicenames, sizeof(inputdevicenames), inputdevicelist);
				si.strcat(inputdevicenames, sizeof(inputdevicenames), "\n");
				inputdevicelist += curlen + 1;
			}
#endif
			s_alAvailableInputDevices = si.Cvar_Get("s_alAvailableInputDevices", inputdevicenames, CVAR_ROM | CVAR_NORESTART);

			// !!! FIXME: 8000Hz is what Speex narrowband mode needs, but we
			// !!! FIXME:  should probably open the capture device after
			// !!! FIXME:  initializing Speex so we can change to wideband
			// !!! FIXME:  if we like.
			si.Printf( PRINT_ALL, "OpenAL default capture device is '%s'\n", defaultinputdevice);
			alCaptureDevice = qalcCaptureOpenDevice(inputdevice, 8000, AL_FORMAT_MONO16, 4096);
			if( !alCaptureDevice && inputdevice )
			{
				si.Printf( PRINT_ALL, "Failed to open OpenAL Input device '%s', trying default.\n", inputdevice );
				alCaptureDevice = qalcCaptureOpenDevice(NULL, 8000, AL_FORMAT_MONO16, 4096);
			}
			si.Printf( PRINT_ALL,  "OpenAL capture device %s.\n", (alCaptureDevice == NULL) ? "failed to open" : "opened");
		}
	}
}


void SndAl_StartCapture( void )
{
	if (alCaptureDevice != NULL) {
#ifndef _WIN32
	alContext = qalcCreateContext(alDevice, NULL);
#endif
	qalcCaptureStart(alCaptureDevice);
	}
}

int SndAl_AvailableCaptureSamples( void )
{
	int retval = 0;
	if (alCaptureDevice != NULL)
	{
		ALint samples = 0;
		qalcGetIntegerv(alCaptureDevice, ALC_CAPTURE_SAMPLES, sizeof (samples), &samples);
		retval = (int) samples;
	}
	return retval;
}

void SndAl_Capture( int samples, byte *data )
{
	if (alCaptureDevice != NULL)
		qalcCaptureSamples(alCaptureDevice, data, samples);
}

void SndAl_StopCapture( void )
{
	if (alCaptureDevice != NULL)
		qalcCaptureStop(alCaptureDevice);
}

void SndAl_MasterGain( float gain )
{
	qalListenerf(AL_GAIN, gain);
}

#endif
