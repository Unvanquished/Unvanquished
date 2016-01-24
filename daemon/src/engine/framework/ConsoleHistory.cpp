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

#include <common/FileSystem.h>
#include "framework/Application.h"
#include "ConsoleHistory.h"

namespace Console {
History::Container History::lines;
std::mutex History::lines_mutex;

std::string History::GetFilename()
{
	return "conhistory" + Application::GetTraits().uniqueHomepathSuffix;
}

void History::Save() {
	try {
		FS::File f = FS::HomePath::OpenWrite(GetFilename());

		auto lock = Lock();
		Container::size_type i = 0;
		if ( lines.size() > SAVED_HISTORY_LINES )
			i = lines.size() - SAVED_HISTORY_LINES;
		for (; i < lines.size(); i++)
		{
			f.Write(lines[i].data(), lines[i].size());
			f.Write("\n", 1);
		}
	} catch (const std::system_error& error) {
		Log::Warn("Couldn't write %s: %s", GetFilename(), error.what());
	}
}

void History::Load() {
	std::string buffer;

	try {
		FS::File f = FS::HomePath::OpenRead(GetFilename());
		buffer = f.ReadAll();
	} catch (const std::system_error& error) {
		Log::Warn("Couldn't read %s: %s", GetFilename(), error.what());
	}

	auto lock = Lock();
	lines.clear();
	std::size_t currentPos = 0;
	std::size_t nextPos = 0;
	while (true)
	{
		nextPos = buffer.find('\n', currentPos);
		if ( nextPos == std::string::npos )
			break;
		lines.emplace_back(buffer, currentPos, (nextPos - currentPos));
		currentPos = nextPos + 1;
	}
}

History::History()
	: current_line( std::numeric_limits<Container::size_type>::max() )
{}

void History::Add( const Line& text )
{
	auto lock = Lock();

	if ( lines.empty() || text != lines.back() )
	{
		lines.push_back(std::move( text ));
	}

	current_line = lines.size();
	unfinished.clear();

	lock.unlock();
	Save();
}

void History::PrevLine( Line& text )
{
	auto lock = Lock();

	if ( lines.empty() )
	{
		return;
	}

	if ( !text.empty() && ( current_line >= lines.size() || text != lines[current_line] ) )
	{
		unfinished = text;
	}

	if ( current_line >= lines.size() )
	{
		current_line = lines.size() - 1;
	}
	else if ( current_line > 0 )
	{
		current_line--;
	}

	text = lines[current_line];
}

void History::NextLine( Line& text )
{
	auto lock = Lock();

	if ( lines.empty() || current_line >= lines.size() - 1 )
	{
		text = unfinished;
		unfinished.clear();
		current_line = lines.size();
	}
	else
	{
		current_line++;
		text = lines[current_line];
	}
}

}
