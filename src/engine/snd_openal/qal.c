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
 * qal.c
 * Dynamically loads OpenAL
 */

#ifndef OPENAL_STATIC

#include "snd_al_local.h"

#ifdef USE_SDL
#include <SDL_loadso.h>
#define OBJTYPE void *
#define OBJLOAD(x) SDL_LoadObject(x)
#define SYMLOAD(x,y) SDL_LoadFunction(x,y)
#define OBJFREE(x) SDL_UnloadObject(x)

#elif defined _WIN32
#include <windows.h>
#define OBJTYPE HMODULE
#define OBJLOAD(x) LoadLibrary(x)
#define SYMLOAD(x,y) GetProcAddress(x,y)
#define OBJFREE(x) FreeLibrary(x)

#elif defined __linux__ || defined __FreeBSD__ || defined MACOS_X
#include <unistd.h>
#include <sys/types.h>
#include <dlfcn.h>
#define OBJTYPE void *
#define OBJLOAD(x) dlopen(x, RTLD_LAZY | RTLD_GLOBAL)
#define SYMLOAD(x,y) dlsym(x,y)
#define OBJFREE(x) dlclose(x)
#else

#error No dynamic library loading code
#endif

LPALENABLE qalEnable;
LPALDISABLE qalDisable;
LPALISENABLED qalIsEnabled;
LPALGETSTRING qalGetString;
LPALGETBOOLEANV qalGetBooleanv;
LPALGETINTEGERV qalGetIntegerv;
LPALGETFLOATV qalGetFloatv;
LPALGETDOUBLEV qalGetDoublev;
LPALGETBOOLEAN qalGetBoolean;
LPALGETINTEGER qalGetInteger;
LPALGETFLOAT qalGetFloat;
LPALGETDOUBLE qalGetDouble;
LPALGETERROR qalGetError;
LPALISEXTENSIONPRESENT qalIsExtensionPresent;
LPALGETPROCADDRESS qalGetProcAddress;
LPALGETENUMVALUE qalGetEnumValue;
LPALLISTENERF qalListenerf;
LPALLISTENER3F qalListener3f;
LPALLISTENERFV qalListenerfv;
LPALLISTENERI qalListeneri;
LPALGETLISTENERF qalGetListenerf;
LPALGETLISTENER3F qalGetListener3f;
LPALGETLISTENERFV qalGetListenerfv;
LPALGETLISTENERI qalGetListeneri;
LPALGENSOURCES qalGenSources;
LPALDELETESOURCES qalDeleteSources;
LPALISSOURCE qalIsSource;
LPALSOURCEF qalSourcef;
LPALSOURCE3F qalSource3f;
LPALSOURCEFV qalSourcefv;
LPALSOURCEI qalSourcei;
LPALGETSOURCEF qalGetSourcef;
LPALGETSOURCE3F qalGetSource3f;
LPALGETSOURCEFV qalGetSourcefv;
LPALGETSOURCEI qalGetSourcei;
LPALSOURCEPLAYV qalSourcePlayv;
LPALSOURCESTOPV qalSourceStopv;
LPALSOURCEREWINDV qalSourceRewindv;
LPALSOURCEPAUSEV qalSourcePausev;
LPALSOURCEPLAY qalSourcePlay;
LPALSOURCESTOP qalSourceStop;
LPALSOURCEREWIND qalSourceRewind;
LPALSOURCEPAUSE qalSourcePause;
LPALSOURCEQUEUEBUFFERS qalSourceQueueBuffers;
LPALSOURCEUNQUEUEBUFFERS qalSourceUnqueueBuffers;
LPALGENBUFFERS qalGenBuffers;
LPALDELETEBUFFERS qalDeleteBuffers;
LPALISBUFFER qalIsBuffer;
LPALBUFFERDATA qalBufferData;
LPALGETBUFFERF qalGetBufferf;
LPALGETBUFFERI qalGetBufferi;
LPALDOPPLERFACTOR qalDopplerFactor;
LPALDOPPLERVELOCITY qalDopplerVelocity;
LPALDISTANCEMODEL qalDistanceModel;

LPALCCREATECONTEXT qalcCreateContext;
LPALCMAKECONTEXTCURRENT qalcMakeContextCurrent;
LPALCPROCESSCONTEXT qalcProcessContext;
LPALCSUSPENDCONTEXT qalcSuspendContext;
LPALCDESTROYCONTEXT qalcDestroyContext;
LPALCGETCURRENTCONTEXT qalcGetCurrentContext;
LPALCGETCONTEXTSDEVICE qalcGetContextsDevice;
LPALCOPENDEVICE qalcOpenDevice;
LPALCCLOSEDEVICE qalcCloseDevice;
LPALCGETERROR qalcGetError;
LPALCISEXTENSIONPRESENT qalcIsExtensionPresent;
LPALCGETPROCADDRESS qalcGetProcAddress;
LPALCGETENUMVALUE qalcGetEnumValue;
LPALCGETSTRING qalcGetString;
LPALCGETINTEGERV qalcGetIntegerv;
LPALCCAPTUREOPENDEVICE qalcCaptureOpenDevice;
LPALCCAPTURECLOSEDEVICE qalcCaptureCloseDevice;
LPALCCAPTURESTART qalcCaptureStart;
LPALCCAPTURESTOP qalcCaptureStop;
LPALCCAPTURESAMPLES qalcCaptureSamples;

static OBJTYPE OpenALLib = NULL;

static qboolean alinit_fail = qfalse;

static void *GPA(char *str)
{
	void *rv;

	rv = SYMLOAD(OpenALLib, str);
	if(!rv)
	{
		si.Printf(PRINT_ALL, " Can't load symbol %s\n", str);
		alinit_fail = qtrue;
		return NULL;
	}
	else
	{
		// Dushan : Print some developer information
		// Don't allow this for release
#if defined _DEBUG
		si.Printf(PRINT_ALL, " Loaded symbol %s (0x%08X)\n", str, rv);
#endif
        return rv;
	}
}

qboolean QAL_Init(const char *libname)
{
	if(OpenALLib)
		return qtrue;

	si.Printf(PRINT_ALL, "loading %s\n", libname);
	if( (OpenALLib = OBJLOAD(libname)) == 0 )
	{
#ifdef _WIN32
		si.Printf(PRINT_ALL, " Can't load %s\n", libname);
		return qfalse;
#else
		char fn[1024];
		getcwd(fn, sizeof(fn));
		strncat(fn, "/", sizeof(fn));
		strncat(fn, libname, sizeof(fn));

		if( (OpenALLib = OBJLOAD(fn)) == 0 )
		{
			si.Printf(PRINT_ALL, " Can't load %s\n", libname);
			return qfalse;
		}
#endif
	}

alinit_fail = qfalse;

	qalEnable = GPA("alEnable");
	qalDisable = GPA("alDisable");
	qalIsEnabled = GPA("alIsEnabled");
	qalGetString = GPA("alGetString");
	qalGetBooleanv = GPA("alGetBooleanv");
	qalGetIntegerv = GPA("alGetIntegerv");
	qalGetFloatv = GPA("alGetFloatv");
	qalGetDoublev = GPA("alGetDoublev");
	qalGetBoolean = GPA("alGetBoolean");
	qalGetInteger = GPA("alGetInteger");
	qalGetFloat = GPA("alGetFloat");
	qalGetDouble = GPA("alGetDouble");
	qalGetError = GPA("alGetError");
	qalIsExtensionPresent = GPA("alIsExtensionPresent");
	qalGetProcAddress = GPA("alGetProcAddress");
	qalGetEnumValue = GPA("alGetEnumValue");
	qalListenerf = GPA("alListenerf");
	qalListener3f = GPA("alListener3f");
	qalListenerfv = GPA("alListenerfv");
	qalListeneri = GPA("alListeneri");
	qalGetListenerf = GPA("alGetListenerf");
	qalGetListener3f = GPA("alGetListener3f");
	qalGetListenerfv = GPA("alGetListenerfv");
	qalGetListeneri = GPA("alGetListeneri");
	qalGenSources = GPA("alGenSources");
	qalDeleteSources = GPA("alDeleteSources");
	qalIsSource = GPA("alIsSource");
	qalSourcef = GPA("alSourcef");
	qalSource3f = GPA("alSource3f");
	qalSourcefv = GPA("alSourcefv");
	qalSourcei = GPA("alSourcei");
	qalGetSourcef = GPA("alGetSourcef");
	qalGetSource3f = GPA("alGetSource3f");
	qalGetSourcefv = GPA("alGetSourcefv");
	qalGetSourcei = GPA("alGetSourcei");
	qalSourcePlayv = GPA("alSourcePlayv");
	qalSourceStopv = GPA("alSourceStopv");
	qalSourceRewindv = GPA("alSourceRewindv");
	qalSourcePausev = GPA("alSourcePausev");
	qalSourcePlay = GPA("alSourcePlay");
	qalSourceStop = GPA("alSourceStop");
	qalSourceRewind = GPA("alSourceRewind");
	qalSourcePause = GPA("alSourcePause");
	qalSourceQueueBuffers = GPA("alSourceQueueBuffers");
	qalSourceUnqueueBuffers = GPA("alSourceUnqueueBuffers");
	qalGenBuffers = GPA("alGenBuffers");
	qalDeleteBuffers = GPA("alDeleteBuffers");
	qalIsBuffer = GPA("alIsBuffer");
	qalBufferData = GPA("alBufferData");
	qalGetBufferf = GPA("alGetBufferf");
	qalGetBufferi = GPA("alGetBufferi");
	qalDopplerFactor = GPA("alDopplerFactor");
	qalDopplerVelocity = GPA("alDopplerVelocity");
	qalDistanceModel = GPA("alDistanceModel");

	qalcCreateContext = GPA("alcCreateContext");
	qalcMakeContextCurrent = GPA("alcMakeContextCurrent");
	qalcProcessContext = GPA("alcProcessContext");
	qalcSuspendContext = GPA("alcSuspendContext");
	qalcDestroyContext = GPA("alcDestroyContext");
	qalcGetCurrentContext = GPA("alcGetCurrentContext");
	qalcGetContextsDevice = GPA("alcGetContextsDevice");
	qalcOpenDevice = GPA("alcOpenDevice");
	qalcCloseDevice = GPA("alcCloseDevice");
	qalcGetError = GPA("alcGetError");
	qalcIsExtensionPresent = GPA("alcIsExtensionPresent");
	qalcGetProcAddress = GPA("alcGetProcAddress");
	qalcGetEnumValue = GPA("alcGetEnumValue");
	qalcGetString = GPA("alcGetString");
	qalcGetIntegerv = GPA("alcGetIntegerv");
	qalcCaptureOpenDevice = GPA("alcCaptureOpenDevice");
	qalcCaptureCloseDevice = GPA("alcCaptureCloseDevice");
	qalcCaptureStart = GPA("alcCaptureStart");
	qalcCaptureStop = GPA("alcCaptureStop");
	qalcCaptureSamples = GPA("alcCaptureSamples");

	if(alinit_fail)
	{
		QAL_Shutdown();
		si.Printf( PRINT_ALL, " One or more symbols not found\n");
		return qfalse;
	}

	return qtrue;
}
void QAL_Shutdown()
{
	if(OpenALLib)
	{
		OBJFREE(OpenALLib);
		OpenALLib = NULL;
	}


	qalEnable = NULL;
	qalDisable = NULL;
	qalIsEnabled = NULL;
	qalGetString = NULL;
	qalGetBooleanv = NULL;
	qalGetIntegerv = NULL;
	qalGetFloatv = NULL;
	qalGetDoublev = NULL;
	qalGetBoolean = NULL;
	qalGetInteger = NULL;
	qalGetFloat = NULL;
	qalGetDouble = NULL;
	qalGetError = NULL;
	qalIsExtensionPresent = NULL;
	qalGetProcAddress = NULL;
	qalGetEnumValue = NULL;
	qalListenerf = NULL;
	qalListener3f = NULL;
	qalListenerfv = NULL;
	qalListeneri = NULL;
	qalGetListenerf = NULL;
	qalGetListener3f = NULL;
	qalGetListenerfv = NULL;
	qalGetListeneri = NULL;
	qalGenSources = NULL;
	qalDeleteSources = NULL;
	qalIsSource = NULL;
	qalSourcef = NULL;
	qalSource3f = NULL;
	qalSourcefv = NULL;
	qalSourcei = NULL;
	qalGetSourcef = NULL;
	qalGetSource3f = NULL;
	qalGetSourcefv = NULL;
	qalGetSourcei = NULL;
	qalSourcePlayv = NULL;
	qalSourceStopv = NULL;
	qalSourceRewindv = NULL;
	qalSourcePausev = NULL;
	qalSourcePlay = NULL;
	qalSourceStop = NULL;
	qalSourceRewind = NULL;
	qalSourcePause = NULL;
	qalSourceQueueBuffers = NULL;
	qalSourceUnqueueBuffers = NULL;
	qalGenBuffers = NULL;
	qalDeleteBuffers = NULL;
	qalIsBuffer = NULL;
	qalBufferData = NULL;
	qalGetBufferf = NULL;
	qalGetBufferi = NULL;
	qalDopplerFactor = NULL;
	qalDopplerVelocity = NULL;
	qalDistanceModel = NULL;

	qalcCreateContext = NULL;
	qalcMakeContextCurrent = NULL;
	qalcProcessContext = NULL;
	qalcSuspendContext = NULL;
	qalcDestroyContext = NULL;
	qalcGetCurrentContext = NULL;
	qalcGetContextsDevice = NULL;
	qalcOpenDevice = NULL;
	qalcCloseDevice = NULL;
	qalcGetError = NULL;
	qalcIsExtensionPresent = NULL;
	qalcGetProcAddress = NULL;
	qalcGetEnumValue = NULL;
	qalcGetString = NULL;
	qalcGetIntegerv = NULL;
	qalcCaptureOpenDevice = NULL;
	qalcCaptureCloseDevice = NULL;
	qalcCaptureStart = NULL;
	qalcCaptureStop = NULL;
	qalcCaptureSamples = NULL;
}

#endif // !OPENAL_STATIC
