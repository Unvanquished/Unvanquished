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

/*
 *The following assumptions made:
 *-Each byte consists of 8bits
 *-sizeof(char) == 1
 *-sizeof(short) == 2
 *-sizeof(int) == 4
 */

namespace Audio {

inline int PackChars(const std::string& input, int startingPosition, int numberOfCharsToPack)
{
	int packed = 0;
	int charsLeftToPack = numberOfCharsToPack;
	while (charsLeftToPack > 0) {
		int position = numberOfCharsToPack - charsLeftToPack;
        //the number must be converted to unsinged char first
        //else if it's >127, 1s will be added to the higher-order bits
		packed |= static_cast<unsigned char>(input[startingPosition + position]) << position * 8;
		--charsLeftToPack;
	}
	return packed;
}

AudioData LoadWavCodec(std::string filename)
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

	std::string format = audioFile.substr(8, 4);

	if (format != "WAVE") {
		audioLogs.Warn("The format label in %s is not \"WAVE\".", filename);
		return AudioData();
	}

	std::string chunk1ID = audioFile.substr(12, 4);

	if (chunk1ID != "fmt ") {
		audioLogs.Warn("The Chunk1ID in %s is not \"fmt\".", filename);
		return AudioData();
	}

	int numChannels = PackChars(audioFile, 22, 2);

	if (numChannels != 1 && numChannels != 2) {
		audioLogs.Warn("%s has an unsupported number of channels.", filename);
		return AudioData();
	}

	int sampleRate = PackChars(audioFile, 24, 4);
	int byteDepth = PackChars(audioFile, 34, 2) / 8;

	if (byteDepth != 1 && byteDepth != 2) {
		audioLogs.Warn("%s has an unsupported bytedepth.", filename);
		return AudioData();
	}

    //TODO  find the position of "data"
    std::size_t dataOffset{audioFile.find("data", 36)};
	if (dataOffset == std::string::npos) {
		audioLogs.Warn("Could not find the data chunk in %s", filename);
		return AudioData();
	}
	std::string chunk2ID = audioFile.substr(dataOffset, 4);

	int size = PackChars(audioFile, dataOffset + 4, 4);

	if (size <= 0 || sampleRate  <=0 ){
		audioLogs.Warn("Error in reading %s.", filename);
		return AudioData();
	}

	char* data = new char[size];

	std::copy_n(audioFile.data() + dataOffset + 8, size, data);

	return AudioData(sampleRate, byteDepth, numChannels, size, data);
}

} // namespace Audio
