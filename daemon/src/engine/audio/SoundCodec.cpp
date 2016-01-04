/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
===========================================================================
*/
#include "SoundCodec.h"
#include "AudioPrivate.h"

namespace Audio {

typedef struct
{
	const char *ext;
	AudioData (*SoundLoader) (std::string);
} soundExtToLoaderMap_t;

// Note that the ordering indicates the order of preference used
// when there are multiple sound files of different formats available
static const soundExtToLoaderMap_t soundLoaders[] =
{
	{ ".wav",	LoadWavCodec },
	{ ".opus",	LoadOpusCodec  },
	{ ".ogg",	LoadOggCodec  },
};

static int numSoundLoaders = ARRAY_LEN(soundLoaders);

AudioData LoadSoundCodec(std::string filename)
{

	std::string ext = FS::Path::Extension(filename);

	// if filename has extension, try to load it
	if (ext != "") {
		// look for the correct loader and use it
		for (int i = 0; i < numSoundLoaders; i++) {
			if (ext == soundLoaders[i].ext) {
				// if file exists, load it
				if (FS::PakPath::FileExists(filename)) {
					return soundLoaders[i].SoundLoader(filename);
				}
			}
		}
	}

	// if filename does not have extension or there is no file with such extension
	// or if there is no codec available for this file format,
	// try and find a suitable match using all the sound file formats supported
	// prioritize with the pak priority
	int bestLoader = -1;
	const FS::PakInfo* bestPak = nullptr;
	std::string strippedname = FS::Path::StripExtension(filename);
	
	for (int i = 0; i < numSoundLoaders; i++)
	{
		std::string altName = Str::Format("%s%s", strippedname, soundLoaders[i].ext);
		const FS::PakInfo* pak = FS::PakPath::LocateFile(altName);

		// We found a file and its pak is better than the best pak we have
		// this relies on how the filesystem works internally and should be moved
		// to a more explicit interface once there is one. (FIXME)
		if (pak != nullptr && (bestPak == nullptr || pak < bestPak ))
		{
			bestPak = pak;
			bestLoader = i;
		}
	}

	if (bestLoader >= 0)
	{
		std::string altName = Str::Format("%s%s", strippedname, soundLoaders[bestLoader].ext );
		return soundLoaders[bestLoader].SoundLoader(altName);
	}

	if (FS::PakPath::FileExists(filename)) {
		audioLogs.Warn("No codec available for opening %s.", filename);
		return AudioData();
	}

	audioLogs.Warn("Sound file %s not found.", filename);
	return AudioData();

}
} // namespace Audio
