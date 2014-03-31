/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2014, Theodoros Theodoridis <theodoridisgr@gmail.com>
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
#include "audio_file_factory.h"
#include "wav_file.h"
#include "ogg_file.h"
#include "opus_file.h"

/*
 *TODO
 *create a make_unique method
 */



unique_ptr<audio_file> Audio::audio_file_factory::get_audio_file(string filename){

  size_t position_of_last_dot{filename.find_last_of('.')};

  if (position_of_last_dot == string::npos)
    ;  // error no extension

  string ext{filename.substr(position_of_last_dot)};

  if( ext == "wav")
      return unique_ptr<audio_file>(new Audio::wav_file(filename));
  if( ext == "ogg")
      return unique_ptr<audio_file>(new Audio::ogg_file(filename));
  if( ext == "opus")
      return unique_ptr<audio_file>(new Audio::opus_file(filename));

  return nullptr;
  //error unknown extension
  //should cause an error?
}
