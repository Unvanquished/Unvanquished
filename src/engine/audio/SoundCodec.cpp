/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2014, Daemon Developers
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

AudioData LoadSoundCodec(Str::StringRef filename)
{
	size_t position_of_last_dot{filename.rfind('.')};

	if (position_of_last_dot == Str::StringRef::npos) {
		audioLogs.Warn("Could not find the extension in %s", filename);
		return AudioData();
	}

	std::string ext{filename.substr(position_of_last_dot + 1)};

	if (ext == "wav")
		return LoadWavCodec(filename);
	if (ext == "ogg")
		return LoadOggCodec(filename);
	if (ext == "opus")
		return LoadOpusCodec(filename);

	audioLogs.Warn("No codec available for opening %s.", filename);
	return AudioData();
}
} // namespace Audio
