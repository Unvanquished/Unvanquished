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

#include "snd_local.h"
#include "snd_codec.h"
#include "../client/client.h"
#include "ALObjects.h"
#include "Emitter.h"
#include "Sample.h"
#include "Sound.h"

using namespace Audio;

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
S_AL_Format
=================
*/
static
ALuint S_AL_Format( int width, int channels )
{
	ALuint format = AL_FORMAT_MONO16;

	// Work out format
	if ( width == 1 )
	{
		if ( channels == 1 )
			format = AL_FORMAT_MONO8;

		else if ( channels == 2 )
			format = AL_FORMAT_STEREO8;
	}

	else if ( width == 2 )
	{
		if ( channels == 1 )
			format = AL_FORMAT_MONO16;

		else if ( channels == 2 )
			format = AL_FORMAT_STEREO16;
	}

	return format;
}

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


typedef struct src_s
{
	ALuint		alSource;		// OpenAL source object
	Sample*	sfx;			// Sound effect in use

	int		lastUsedTime;		// Last time used
	alSrcPriority_t	priority;		// Priority
	int		entity;			// Owning entity (-1 if none)
	int		channel;		// Associated channel (-1 if none)

	qboolean	isActive;		// Is this source currently in use?
	qboolean	isPlaying;		// Is this source currently playing, or stopped?
	qboolean	isLocked;		// This is locked (un-allocatable)
	qboolean	isLooping;		// Is this a looping effect (attached to an entity)
	qboolean	isTracking;		// Is this object tracking its owner
	qboolean	isStream;		// Is this source a stream

	float		curGain;		// gain employed if source is within maxdistance.
	float		scaleGain;		// Last gain value for this source. 0 if muted.

	float		lastTimePos;		// On stopped loops, the last position in the buffer
	int		lastSampleTime;		// Time when this was stopped
	vec3_t		loopSpeakerPos;		// Origin of the loop speaker

	qboolean	local;			// Is this local (relative to the cam)
} src_t;

#define MAX_SRC 128
static src_t srcList[MAX_SRC];
static int srcCount = 0;
static int srcActiveCnt = 0;
static qboolean alSourcesInitialised = qfalse;
static int lastListenerNumber = -1;
static vec3_t lastListenerOrigin = { 0.0f, 0.0f, 0.0f };

typedef struct sentity_s
{
	vec3_t					origin;

	qboolean						srcAllocated; // If a src_t has been allocated to this entity
	int							srcIndex;

	qboolean				loopAddedThisFrame;
	alSrcPriority_t	loopPriority;
	Sample*			loopSfx;
	qboolean				startLoopingSound;
} sentity_t;

static sentity_t entityList[MAX_GENTITIES];

sfxHandle_t S_AL_RegisterSound(const char* sample, qboolean) {
    return RegisterSample(sample)->GetHandle();
}

/*
=================
S_AL_SanitiseVector
=================
*/
#define S_AL_SanitiseVector(v) _S_AL_SanitiseVector(v,__LINE__)
static void _S_AL_SanitiseVector( vec3_t v, int line )
{
	if ( Q_isnan( v[ 0 ] ) || Q_isnan( v[ 1 ] ) || Q_isnan( v[ 2 ] ) )
	{
		Com_DPrintf( S_COLOR_YELLOW "WARNING: vector with one or more NaN components "
		             "being passed to OpenAL at %s:%d -- zeroing\n", __FILE__, line );
		VectorClear( v );
	}
}

/*
=================
S_AL_Gain
Set gain to 0 if muted, otherwise set it to given value.
=================
*/

static void S_AL_Gain( ALuint source, float gainval )
{
	if ( s_muted->integer )
		alSourcef( source, AL_GAIN, 0.0f );

	else
		alSourcef( source, AL_GAIN, gainval );
}

/*
=================
S_AL_ScaleGain
Adapt the gain if necessary to get a quicker fadeout when the source is too far away.
=================
*/

static void S_AL_ScaleGain( src_t *chksrc, vec3_t origin )
{
	float distance = chksrc->local ? 0 : Distance( origin, lastListenerOrigin );

	// If we exceed a certain distance, scale the gain linearly until the sound
	// vanishes into nothingness.
	if ( !chksrc->local && ( distance -= s_alMaxDistance->value ) > 0 )
	{
		float scaleFactor;

		if ( distance >= s_alGraceDistance->value )
			scaleFactor = 0.0f;

		else
			scaleFactor = 1.0f - distance / s_alGraceDistance->value;

		scaleFactor *= chksrc->curGain;

		if ( chksrc->scaleGain != scaleFactor )
		{
			chksrc->scaleGain = scaleFactor;
			S_AL_Gain( chksrc->alSource, chksrc->scaleGain );
		}
	}
	else if ( chksrc->scaleGain != chksrc->curGain )
	{
		chksrc->scaleGain = chksrc->curGain;
		S_AL_Gain( chksrc->alSource, chksrc->scaleGain );
	}
}

/*
=================
S_AL_HearingThroughEntity

Also see S_Base_HearingThroughEntity
=================
*/
static qboolean S_AL_HearingThroughEntity( int entityNum )
{
	float	distanceSq;

	if ( lastListenerNumber == entityNum )
	{
		// This is an outrageous hack to detect
		// whether or not the player is rendering in third person or not. We can't
		// ask the renderer because the renderer has no notion of entities and we
		// can't ask cgame since that would involve changing the API and hence mod
		// compatibility. I don't think there is any way around this, but I'll leave
		// the FIXME just in case anyone has a bright idea.
		distanceSq = DistanceSquared(
		                 entityList[ entityNum ].origin,
		                 lastListenerOrigin );

		if ( distanceSq > THIRD_PERSON_THRESHOLD_SQ )
			return qfalse; //we're the player, but third person

		else
			return qtrue;  //we're the player
	}

	else
		return qfalse; //not the player
}

/*
=================
S_AL_UpdateEntityPosition
=================
*/
static
void S_AL_UpdateEntityPosition( int entityNum, const vec3_t origin )
{
	vec3_t sanOrigin;

	VectorCopy( origin, sanOrigin );
	S_AL_SanitiseVector( sanOrigin );

	if ( entityNum < 0 || entityNum >= MAX_GENTITIES )
		Com_Error( ERR_DROP, "S_UpdateEntityPosition: bad entitynum %i", entityNum );

	VectorCopy( sanOrigin, entityList[entityNum].origin );

    UpdateEntityPosition(entityNum, origin);
}

/*
=================
S_AL_UpdateEntityVelocity
=================
*/
static
void S_AL_UpdateEntityVelocity( int entityNum, const vec3_t velocity )
{
	vec3_t sanVelocity;

	VectorCopy( velocity, sanVelocity );
	S_AL_SanitiseVector( sanVelocity );

	if ( entityNum < 0 || entityNum >= MAX_GENTITIES )
		Com_Error( ERR_DROP, "S_UpdateEntityVelocity: bad entitynum %i", entityNum );

    UpdateEntityVelocity(entityNum, velocity);
	//VectorCopy( sanOrigin, entityList[entityNum].origin );
}

/*
=================
S_AL_UpdateEntityOcclusion
=================
*/
static
void S_AL_UpdateEntityOcclusion( int entityNum, qboolean occluded, float ratio )
{
	if ( entityNum < 0 || entityNum >= MAX_GENTITIES )
		Com_Error( ERR_DROP, "S_UpdateEntityOcclusion: bad entitynum %i", entityNum );
}

/*
=================
S_AL_CheckInput
Check whether input values from mods are out of range.
Necessary for i.g. Western Quake3 mod which is buggy.
=================
*/
static qboolean S_AL_CheckInput( int entityNum, sfxHandle_t sfx )
{
	if ( entityNum < 0 || entityNum >= MAX_GENTITIES )
		Com_Error( ERR_DROP, "ERROR: S_AL_CheckInput: bad entitynum %i", entityNum );

	if ( not Sample::IsValidHandle(sfx) )
	{
		Com_Printf( S_COLOR_RED "ERROR: S_AL_CheckInput: handle %i out of range\n", sfx );
		return qtrue;
	}

	return qfalse;
}

/*
=================
S_AL_StartLocalSound

Play a local (non-spatialized) sound effect
=================
*/
static
void S_AL_StartLocalSound( sfxHandle_t sfx, int channel )
{
	if ( S_AL_CheckInput( 0, sfx ) )
		return;

    StartLocalSound(Sample::FromHandle(sfx));
}

/*
=================
S_AL_StartSound

Play a one-shot sound effect
=================
*/
static void S_AL_StartSound( vec3_t origin, int entnum, int entchannel, sfxHandle_t sfx )
{
	if ( S_AL_CheckInput( entnum, sfx ) )
		return;

    StartSound(entnum, origin, Sample::FromHandle(sfx));
}

/*
=================
S_AL_ClearLoopingSounds
=================
*/
static
void S_AL_ClearLoopingSounds( qboolean killall )
{
}

/*
=================
S_AL_SrcLoop
=================
*/
static void S_AL_SrcLoop( alSrcPriority_t priority, sfxHandle_t sfx,
                          const vec3_t origin, const vec3_t velocity, int entityNum )
{
}

/*
=================
S_AL_AddLoopingSound
=================
*/
static void S_AL_AddLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
}

/*
=================
S_AL_AddRealLoopingSound
=================
*/
static void S_AL_AddRealLoopingSound( int entityNum, const vec3_t origin, const vec3_t velocity, sfxHandle_t sfx )
{
}

/*
=================
S_AL_StopLoopingSound
=================
*/
static
void S_AL_StopLoopingSound( int entityNum )
{
}

//===========================================================================

static srcHandle_t streamSourceHandles[MAX_RAW_STREAMS];
static qboolean streamPlaying[MAX_RAW_STREAMS];
static ALuint streamSources[MAX_RAW_STREAMS];

/*
=================
S_AL_AllocateStreamChannel
=================
*/
static void S_AL_AllocateStreamChannel( int stream, int entityNum )
{/*
	srcHandle_t cursrc;
	ALuint alsrc;

	if ( ( stream < 0 ) || ( stream >= MAX_RAW_STREAMS ) )
		return;

	if ( entityNum >= 0 )
	{
		// This is a stream that tracks an entity
		// Allocate a streamSource at normal priority
		cursrc = S_AL_SrcAlloc( SRCPRI_ENTITY, entityNum, 0 );

		if ( cursrc < 0 )
			return;

		S_AL_SrcSetup( cursrc, -1, SRCPRI_ENTITY, entityNum, 0, qfalse );
		alsrc = S_AL_SrcGet( cursrc );
		srcList[cursrc].isTracking = qtrue;
		srcList[cursrc].isStream = qtrue;
	}

	else
	{
		// Unspatialized stream source

		// Allocate a streamSource at high priority
		cursrc = S_AL_SrcAlloc( SRCPRI_STREAM, -2, 0 );

		if ( cursrc < 0 )
			return;

		alsrc = S_AL_SrcGet( cursrc );

		// Lock the streamSource so nobody else can use it, and get the raw streamSource
		S_AL_SrcLock( cursrc );

		// make sure that after unmuting the S_AL_Gain in S_Update() does not turn
		// volume up prematurely for this source
		srcList[cursrc].scaleGain = 0.0f;

		// Set some streamSource parameters
		alSourcei( alsrc, AL_BUFFER,          0 );
		alSourcei( alsrc, AL_LOOPING,         AL_FALSE );
		alSource3f( alsrc, AL_POSITION,        0.0, 0.0, 0.0 );
		alSource3f( alsrc, AL_VELOCITY,        0.0, 0.0, 0.0 );
		alSource3f( alsrc, AL_DIRECTION,       0.0, 0.0, 0.0 );
		alSourcef( alsrc, AL_ROLLOFF_FACTOR,  0.0 );
		alSourcei( alsrc, AL_SOURCE_RELATIVE, AL_TRUE );
	}

	streamSourceHandles[stream] = cursrc;
	streamSources[stream] = alsrc;
*/}

/*
=================
S_AL_FreeStreamChannel
=================
*/
static void S_AL_FreeStreamChannel( int stream )
{/*
	if ( ( stream < 0 ) || ( stream >= MAX_RAW_STREAMS ) )
		return;

	// Release the output streamSource
	S_AL_SrcUnlock( streamSourceHandles[stream] );
	S_AL_SrcKill( streamSourceHandles[stream] );
	streamSources[stream] = 0;
	streamSourceHandles[stream] = -1;
*/}

/*
=================
S_AL_RawSamples
=================
*/
static
void S_AL_RawSamples( int stream, int samples, int rate, int width, int channels, const byte *data, float volume, int entityNum )
{/*
	ALuint buffer;
	ALuint format;

	if ( ( stream < 0 ) || ( stream >= MAX_RAW_STREAMS ) )
		return;

	format = S_AL_Format( width, channels );

	// Create the streamSource if necessary
	if ( streamSourceHandles[stream] == -1 )
	{
		S_AL_AllocateStreamChannel( stream, entityNum );

		// Failed?
		if ( streamSourceHandles[stream] == -1 )
		{
			Com_Printf( S_COLOR_RED "ERROR: Can't allocate streaming streamSource\n" );
			return;
		}
	}

	// Create a buffer, and stuff the data into it
	alGenBuffers( 1, &buffer );
	alBufferData( buffer, format, ( ALvoid * )data, ( samples * width * channels ), rate );

	// Shove the data onto the streamSource
	alSourceQueueBuffers( streamSources[stream], 1, &buffer );

	if ( entityNum < 0 )
	{
		// Volume
		S_AL_Gain( streamSources[stream], volume * s_volume->value * s_alGain->value );
	}
*/}

/*
=================
S_AL_StreamUpdate
=================
*/
static
void S_AL_StreamUpdate( int stream )
{/*
	int		numBuffers;
	ALint	state;

	if ( ( stream < 0 ) || ( stream >= MAX_RAW_STREAMS ) )
		return;

	if ( streamSourceHandles[stream] == -1 )
		return;

	// Un-queue any buffers, and delete them
	alGetSourcei( streamSources[stream], AL_BUFFERS_PROCESSED, &numBuffers );

	while ( numBuffers-- )
	{
		ALuint buffer;
		alSourceUnqueueBuffers( streamSources[stream], 1, &buffer );
		alDeleteBuffers( 1, &buffer );
	}

	// Start the streamSource playing if necessary
	alGetSourcei( streamSources[stream], AL_BUFFERS_QUEUED, &numBuffers );

	alGetSourcei( streamSources[stream], AL_SOURCE_STATE, &state );

	if ( state == AL_STOPPED )
	{
		streamPlaying[stream] = qfalse;

		// If there are no buffers queued up, release the streamSource
		if ( !numBuffers )
			S_AL_FreeStreamChannel( stream );
	}

	if ( !streamPlaying[stream] && numBuffers )
	{
		alSourcePlay( streamSources[stream] );
		streamPlaying[stream] = qtrue;
	}
*/}

/*
=================
S_AL_StreamDie
=================
*/
static
void S_AL_StreamDie( int stream )
{/*
	int		numBuffers;

	if ( ( stream < 0 ) || ( stream >= MAX_RAW_STREAMS ) )
		return;

	if ( streamSourceHandles[stream] == -1 )
		return;

	streamPlaying[stream] = qfalse;
	alSourceStop( streamSources[stream] );

	// Un-queue any buffers, and delete them
	alGetSourcei( streamSources[stream], AL_BUFFERS_PROCESSED, &numBuffers );

	while ( numBuffers-- )
	{
		ALuint buffer;
		alSourceUnqueueBuffers( streamSources[stream], 1, &buffer );
		alDeleteBuffers( 1, &buffer );
	}

	S_AL_FreeStreamChannel( stream );
*/}


//===========================================================================


#define NUM_MUSIC_BUFFERS	4
#define	MUSIC_BUFFER_SIZE 4096

static qboolean musicPlaying = qfalse;
static srcHandle_t musicSourceHandle = -1;
static ALuint musicSource;
static ALuint musicBuffers[NUM_MUSIC_BUFFERS];

static snd_stream_t *mus_stream;
static snd_stream_t *intro_stream;
static char s_backgroundLoop[MAX_QPATH];

static byte decode_buffer[MUSIC_BUFFER_SIZE];

/*
=================
S_AL_MusicSourceGet
=================
*/
static void S_AL_MusicSourceGet( void )
{/*
	// Allocate a musicSource at high priority
	musicSourceHandle = S_AL_SrcAlloc( SRCPRI_STREAM, -2, 0 );

	if ( musicSourceHandle == -1 )
		return;

	// Lock the musicSource so nobody else can use it, and get the raw musicSource
	S_AL_SrcLock( musicSourceHandle );
	musicSource = S_AL_SrcGet( musicSourceHandle );

	// make sure that after unmuting the S_AL_Gain in S_Update() does not turn
	// volume up prematurely for this source
	srcList[musicSourceHandle].scaleGain = 0.0f;

	// Set some musicSource parameters
	alSource3f( musicSource, AL_POSITION,        0.0, 0.0, 0.0 );
	alSource3f( musicSource, AL_VELOCITY,        0.0, 0.0, 0.0 );
	alSource3f( musicSource, AL_DIRECTION,       0.0, 0.0, 0.0 );
	alSourcef( musicSource, AL_ROLLOFF_FACTOR,  0.0 );
	alSourcei( musicSource, AL_SOURCE_RELATIVE, AL_TRUE );
*/}

/*
=================
S_AL_MusicSourceFree
=================
*/
static void S_AL_MusicSourceFree( void )
{/*
	// Release the output musicSource
	S_AL_SrcUnlock( musicSourceHandle );
	musicSource = 0;
	musicSourceHandle = -1;
*/}

/*
=================
S_AL_CloseMusicFiles
=================
*/
static void S_AL_CloseMusicFiles( void )
{/*
	if ( intro_stream )
	{
		S_CodecCloseStream( intro_stream );
		intro_stream = NULL;
	}

	if ( mus_stream )
	{
		S_CodecCloseStream( mus_stream );
		mus_stream = NULL;
	}
*/}

/*
=================
S_AL_StopBackgroundTrack
=================
*/
static
void S_AL_StopBackgroundTrack( void )
{/*
	if ( !musicPlaying )
		return;

	// Stop playing
	alSourceStop( musicSource );

	// De-queue the musicBuffers
	alSourceUnqueueBuffers( musicSource, NUM_MUSIC_BUFFERS, musicBuffers );

	// Destroy the musicBuffers
	alDeleteBuffers( NUM_MUSIC_BUFFERS, musicBuffers );

	// Free the musicSource
	S_AL_MusicSourceFree();

	// Unload the stream
	S_AL_CloseMusicFiles();

	musicPlaying = qfalse;
*/}

/*
=================
S_AL_MusicProcess
=================
*/
static
void S_AL_MusicProcess( ALuint b )
{/*
	ALenum error;
	int l;
	ALuint format;
	snd_stream_t *curstream;

	S_AL_ClearError( qfalse );

	if ( intro_stream )
		curstream = intro_stream;

	else
		curstream = mus_stream;

	if ( !curstream )
		return;

	l = S_CodecReadStream( curstream, MUSIC_BUFFER_SIZE, decode_buffer );

	// Run out data to read, start at the beginning again
	if ( l == 0 )
	{
		S_CodecCloseStream( curstream );

		// the intro stream just finished playing so we don't need to reopen
		// the music stream.
		if ( intro_stream )
			intro_stream = NULL;

		else
			mus_stream = S_CodecOpenStream( s_backgroundLoop );

		curstream = mus_stream;

		if ( !curstream )
		{
			S_AL_StopBackgroundTrack();
			return;
		}

		l = S_CodecReadStream( curstream, MUSIC_BUFFER_SIZE, decode_buffer );
	}

	format = S_AL_Format( curstream->info.width, curstream->info.channels );

	if ( l == 0 )
	{
		// We have no data to buffer, so buffer silence
		byte dummyData[ 2 ] = { 0 };

		alBufferData( b, AL_FORMAT_MONO16, ( void * )dummyData, 2, 22050 );
	}

	else
		alBufferData( b, format, decode_buffer, l, curstream->info.rate );

	if ( ( error = alGetError( ) ) != AL_NO_ERROR )
	{
		S_AL_StopBackgroundTrack( );
		Com_Printf( S_COLOR_RED "ERROR: while buffering data for music stream - %s\n",
		            S_AL_ErrorMsg( error ) );
		return;
	}
*/}

/*
=================
S_AL_StartBackgroundTrack
=================
*/
static
void S_AL_StartBackgroundTrack( const char *intro, const char *loop )
{/*
	int i;
	qboolean issame;

	// Stop any existing music that might be playing
	S_AL_StopBackgroundTrack();

	if ( ( !intro || !*intro ) && ( !loop || !*loop ) )
		return;

	// Allocate a musicSource
	S_AL_MusicSourceGet();

	if ( musicSourceHandle == -1 )
		return;

	if ( !loop || !*loop )
	{
		loop = intro;
		issame = qtrue;
	}

	else if ( intro && *intro && !strcmp( intro, loop ) )
		issame = qtrue;

	else
		issame = qfalse;

	// Copy the loop over
	strncpy( s_backgroundLoop, loop, sizeof( s_backgroundLoop ) );

	if ( !issame )
	{
		// Open the intro and don't mind whether it succeeds.
		// The important part is the loop.
		intro_stream = S_CodecOpenStream( intro );
	}

	else
		intro_stream = NULL;

	mus_stream = S_CodecOpenStream( s_backgroundLoop );

	if ( !mus_stream )
	{
		S_AL_CloseMusicFiles();
		S_AL_MusicSourceFree();
		return;
	}

	// Generate the musicBuffers
	alGenBuffers( NUM_MUSIC_BUFFERS, musicBuffers );

	// Queue the musicBuffers up
	for ( i = 0; i < NUM_MUSIC_BUFFERS; i++ )
	{
		S_AL_MusicProcess( musicBuffers[i] );
	}

	alSourceQueueBuffers( musicSource, NUM_MUSIC_BUFFERS, musicBuffers );

	// Set the initial gain property
	S_AL_Gain( musicSource, s_alGain->value * s_musicVolume->value );

	// Start playing
	alSourcePlay( musicSource );

	musicPlaying = qtrue;
*/}

/*
=================
S_AL_MusicUpdate
=================
*/
static
void S_AL_MusicUpdate( void )
{/*
	int		numBuffers;
	ALint	state;

	if ( !musicPlaying )
		return;

	alGetSourcei( musicSource, AL_BUFFERS_PROCESSED, &numBuffers );

	while ( numBuffers-- )
	{
		ALuint b;
		alSourceUnqueueBuffers( musicSource, 1, &b );
		S_AL_MusicProcess( b );
		alSourceQueueBuffers( musicSource, 1, &b );
	}

	// Hitches can cause OpenAL to be starved of buffers when streaming.
	// If this happens, it will stop playback. This restarts the source if
	// it is no longer playing, and if there are buffers available
	alGetSourcei( musicSource, AL_SOURCE_STATE, &state );
	alGetSourcei( musicSource, AL_BUFFERS_QUEUED, &numBuffers );

	if ( state == AL_STOPPED && numBuffers )
	{
		Com_DPrintf( S_COLOR_YELLOW "Restarted OpenAL music\n" );
		alSourcePlay( musicSource );
	}

	// Set the gain property
	S_AL_Gain( musicSource, s_alGain->value * s_musicVolume->value );
*/}


//===========================================================================


// Local state variables
static ALCdevice *alDevice;
static ALCcontext *alContext;

/*
=================
S_AL_StopAllSounds
=================
*/
static
void S_AL_StopAllSounds( void )
{
	int i;
	//S_AL_SrcShutup();
	S_AL_StopBackgroundTrack();

	for ( i = 0; i < MAX_RAW_STREAMS; i++ )
		S_AL_StreamDie( i );
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
	S_AL_SanitiseVector( sorigin );

	S_AL_SanitiseVector( axis[ 0 ] );
	S_AL_SanitiseVector( axis[ 1 ] );
	S_AL_SanitiseVector( axis[ 2 ] );

	orientation[0] = axis[0][0];
	orientation[1] = axis[0][1];
	orientation[2] = axis[0][2];
	orientation[3] = axis[2][0];
	orientation[4] = axis[2][1];
	orientation[5] = axis[2][2];

	lastListenerNumber = entityNum;
	VectorCopy( sorigin, lastListenerOrigin );

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
	int i;

	if ( s_muted->modified )
	{
		// muted state changed. Let S_AL_Gain turn up all sources again.
		for ( i = 0; i < srcCount; i++ )
		{
			if ( srcList[i].isActive )
				S_AL_Gain( srcList[i].alSource, srcList[i].scaleGain );
		}

		s_muted->modified = qfalse;
	}

    UpdateEverything();
	// Update SFX channels
	//S_AL_SrcUpdate();

	// Update streams
	for ( i = 0; i < MAX_RAW_STREAMS; i++ )
		S_AL_StreamUpdate( i );

	S_AL_MusicUpdate();

	// Doppler
	if ( s_doppler->modified )
	{
		s_alDopplerFactor->modified = qtrue;
		s_doppler->modified = qfalse;
	}

	// Doppler parameters
	if ( s_alDopplerFactor->modified )
	{
		if ( s_doppler->integer )
			alDopplerFactor( s_alDopplerFactor->value );

		else
			alDopplerFactor( 0.0f );

		s_alDopplerFactor->modified = qfalse;
	}

	if ( s_alDopplerSpeed->modified )
	{
		alDopplerVelocity( s_alDopplerSpeed->value );
		s_alDopplerSpeed->modified = qfalse;
	}

	// Clear the modified flags on the other cvars
	s_alGain->modified = qfalse;
	s_volume->modified = qfalse;
	s_musicVolume->modified = qfalse;
	s_alMinDistance->modified = qfalse;
	s_alRolloff->modified = qfalse;
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
	//if ( !numSfx )
	//	S_AL_BufferInit();
    InitSamples();
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
	// Shut down everything
	int i;

	for ( i = 0; i < MAX_RAW_STREAMS; i++ )
		S_AL_StreamDie( i );

	S_AL_StopBackgroundTrack( );
	//S_AL_SrcShutdown( );
    ShutdownSamples();

	alcDestroyContext( alContext );
	alcCloseDevice( alDevice );

	for ( i = 0; i < MAX_RAW_STREAMS; i++ )
	{
		streamSourceHandles[i] = -1;
		streamPlaying[i] = qfalse;
		streamSources[i] = 0;
	}
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

	for ( i = 0; i < MAX_RAW_STREAMS; i++ )
	{
		streamSourceHandles[i] = -1;
		streamPlaying[i] = qfalse;
		streamSources[i] = 0;
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
    InitSamples();
    InitEmitters();
    InitSounds();
	//S_AL_SrcInit( );

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

