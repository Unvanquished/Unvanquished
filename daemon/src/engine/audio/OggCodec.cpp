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
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/vorbisfile.h>

/*
 *The following assumptions made:
 *-Each byte consists of 8bits
 *-sizeof(char) == 1
 *-sizeof(short) == 2
 *-sizeof(int) == 4
 */

namespace Audio{
/*
 *Used by OggCallbackRead
 *audioFile contains the whole .ogg file
 *position tracks the current position while reading the file
 */
struct OggDataSource {
	std::string* audioFile;
	size_t position;
};

/*
 *Replacement for the read_func
 *ptr: Pointer to a block of memory with a size of at least (size*count) bytes, converted to a void*.
 *size: Size, in bytes, of each element to be read.
 *count: Number of elements, each one of size bytes.
 *datasource: Pointer to an object (here a OggDataSource) which contains the data to be read into ptr, converted to a *void. 
 *Returns the number of successfully read elements.
 */
size_t OggCallbackRead(void* ptr, size_t size, size_t count, void* datasource)
{

	OggDataSource* data = static_cast<OggDataSource*>(datasource);

	// check if input is valid
	if ( !ptr )
	{
		errno = EFAULT;
		return 0;
	}

	if ( !( size && count ) )
	{
		// It's not an error, caller just wants zero bytes!
		errno = 0;
		return 0;
	}

	if (data == nullptr || data->audioFile == nullptr || data->audioFile->size() < data->position) {
		errno = EBADF;
		return 0;
	}

	std::string* audioFile = data->audioFile;
	size_t position = data->position;
	size_t bytesRemaining = audioFile->size() - position;
	size_t bytesToRead = size * count;
	if (bytesToRead > bytesRemaining) {
		bytesToRead = bytesRemaining;
	}

	std::copy_n(audioFile->cbegin() + position, bytesToRead, static_cast<char*>(ptr));
	data->position += bytesToRead;

	size_t elementsRead = bytesToRead / size;

    //An element was partially read
	if (bytesToRead % size) {
		++ elementsRead;
    }

    return elementsRead;
}


const ov_callbacks Ogg_Callbacks = {&OggCallbackRead, nullptr, nullptr, nullptr};

AudioData LoadOggCodec(std::string filename)
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
	OggDataSource dataSource = {&audioFile, 0};
	std::unique_ptr<OggVorbis_File> vorbisFile(new OggVorbis_File);

	if (ov_open_callbacks(&dataSource, vorbisFile.get(), nullptr, 0, Ogg_Callbacks) != 0) {
        audioLogs.Warn("Error while reading %s", filename);
		ov_clear(vorbisFile.get());
		return AudioData();
	}

	if (ov_streams(vorbisFile.get()) != 1) {
		audioLogs.Warn("Unsupported number of streams in %s.", filename);
		ov_clear(vorbisFile.get());
		return AudioData();
	}

	vorbis_info* oggInfo = ov_info(vorbisFile.get(), 0);

	if (!oggInfo) {
        audioLogs.Warn("Could not read vorbis_info in %s.", filename);
		ov_clear(vorbisFile.get());
		return AudioData();
	}

	const int sampleWidth = 2;

	int sampleRate = oggInfo->rate;
	int numberOfChannels = oggInfo->channels;

	char buffer[4096];
	int bytesRead = 0;
	int bitStream = 0;

	std::vector<char> samples;

	while ((bytesRead = ov_read(vorbisFile.get(), buffer, 4096, 0, sampleWidth, 1, &bitStream)) > 0) {
		std::copy_n(buffer, bytesRead, std::back_inserter(samples));
	}
	ov_clear(vorbisFile.get());

	char* rawSamples = new char[samples.size()];
	std::copy_n(samples.data(), samples.size(), rawSamples);
	return AudioData(sampleRate, sampleWidth, numberOfChannels, samples.size(), rawSamples);
}

} //namespace Audio
