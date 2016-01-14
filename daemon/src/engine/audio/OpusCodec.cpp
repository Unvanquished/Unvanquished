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
#include <common/FileSystem.h>

#include <errno.h>
#include <opusfile.h>

/*
 *The following assumptions made:
 *-Each byte consists of 8bits
 *-sizeof(char) == 1
 *-sizeof(short) == 2
 *-sizeof(int) == 4
 */

namespace Audio{

struct OpusDataSource {
	std::string* audioFile;
	size_t position;
};

/*
 *Replacement for the op_read_func
 *datasource: Pointer to an object (here a OggDataSource) which contains the data to be read into ptr, converted to a *void. 
 *ptr: Pointer to a block of memory with a size of at least nbytes.
 *nBytes: the number of bytes to read. 
 *Returns the number of successfully read elements.
 */
int OpusCallbackRead(void* dataSource, unsigned char* ptr, int nBytes)
{

	OpusDataSource* data = static_cast<OpusDataSource*>(dataSource);

	// check if input is valid
	if ( !ptr )
	{
		errno = EFAULT;
		return 0;
	}

	if (!nBytes) {
		// It's not an error, caller just wants zero bytes!
		errno = 0;
		return 0;
	}

	if (data == nullptr || data->audioFile == nullptr || data->audioFile->size() < data->position ||
	    nBytes < 0) {
		errno = EBADF;
		return 0;
	}

	std::string* audioFile = data->audioFile;
	size_t position = data->position;
	size_t bytesRemaining = audioFile->size() - position;
	size_t bytesToRead = nBytes;

	if (bytesToRead > bytesRemaining) {
		bytesToRead = bytesRemaining;
	}

	// char to unsigned char????
	std::copy_n(reinterpret_cast<const unsigned char*>(audioFile->data()) + position, bytesToRead, ptr);
	data->position += bytesToRead;

    return bytesToRead;
}

const OpusFileCallbacks Opus_Callbacks = {&OpusCallbackRead, nullptr, nullptr, nullptr};

AudioData LoadOpusCodec(std::string filename)
{
	std::string audioFile;
	try
	{
		audioFile = FS::PakPath::ReadFile(filename);
	}
	catch (std::system_error& err)
	{
		audioLogs.Warn("Failed to open %s: %s", filename, err.what());
		return AudioData();
	}

	OpusDataSource dataSource = {&audioFile, 0};
	OggOpusFile* opusFile = op_open_callbacks(&dataSource, &Opus_Callbacks, nullptr, 0, nullptr);

	if (!opusFile) {
		audioLogs.Warn("Error while reading %s", filename);
		return AudioData();
	}

	const OpusHead* opusInfo = op_head(opusFile, -1);

	if (!opusInfo) {
		op_free(opusFile);
		audioLogs.Warn("Could not read OpusHead in %s", filename);
		return AudioData();
	}

	if (opusInfo->stream_count != 1) {
		op_free(opusFile);
		audioLogs.Warn("Only one stream is supported in Opus files: %s", filename);
		return AudioData();
	}

	if (opusInfo->channel_count != 1 && opusInfo->channel_count != 2) {
		op_free(opusFile);
		audioLogs.Warn("Only mono and stereo Opus files are supported: %s", filename);
		return AudioData();
	}

	const int sampleWidth = 2;

	int sampleRate = 48000;
	int numberOfChannels = opusInfo->channel_count;

	// The buffer is big enough to hold 120ms worth of samples per channel
	opus_int16* buffer = new opus_int16[numberOfChannels * 5760];
	int samplesPerChannelRead = 0;

	std::vector<opus_int16> samples;

	while ((samplesPerChannelRead =
	            op_read(opusFile, buffer, sampleWidth * numberOfChannels * 5760, nullptr)) > 0) {
		std::copy_n(buffer, samplesPerChannelRead * numberOfChannels, std::back_inserter(samples));
	}

	op_free(opusFile);

	char* rawSamples = new char[sampleWidth * samples.size()];
	std::copy_n(reinterpret_cast<char*>(samples.data()), sampleWidth * samples.size(), rawSamples);

	return AudioData(sampleRate, sampleWidth, numberOfChannels, samples.size() * sampleWidth,
	                 rawSamples);
}

} //namespace Audio
