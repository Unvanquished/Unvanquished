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

#include "qcommon/q_shared.h"
#include "ConsoleField.h"
#include "CommandSystem.h"

namespace Console {

    Field::Field(int size): LineEditData(size) {
    }

    void Field::HistoryPrev() {
        std::string current = Str::UTF32To8(GetText());
        hist.PrevLine(current);
        SetText(Str::UTF8To32(current));
    }

    void Field::HistoryNext() {
        std::string current = Str::UTF32To8(GetText());
        hist.NextLine(current);
        SetText(Str::UTF8To32(current));
    }

    void Field::RunCommand(Str::StringRef defaultCommand) {
        if (GetText().empty()) {
            return;
        }

        std::string current = Str::UTF32To8(GetText());
        if (current[0] == '/' or current[0] == '\\') {
            Cmd::BufferCommandText(current.c_str() + 1, true);
        } else if (defaultCommand.empty()) {
            Cmd::BufferCommandText(current, true);
        } else {
            Cmd::BufferCommandText(defaultCommand + " " + Cmd::Escape(current), true);
        }
        hist.Add(std::move(current));

        Clear();
    }

    void Field::AutoComplete() {
        //We want to complete in the middle of a command text with potentially multiple commands

        //Add slash prefix and get command text up to cursor
        if (GetText().empty() || (GetText()[0] != '/' && GetText()[0] != '\\')) {
            GetText().insert(GetText().begin(), '/');
            SetCursor(GetCursorPos() + 1);
        }
        std::string commandText = Str::UTF32To8(GetText().substr(1, GetCursorPos() - 1));

        //Split the command text and find the command to complete
        const char* commandStart = commandText.data();
        const char* commandEnd = commandText.data() + commandText.size();
        while (true) {
            const char* next = Cmd::SplitCommand(commandStart, commandEnd);
            if (next != commandEnd)
                commandStart = next;
            else
                break;
        }

        //Parse the arguments and get the list of candidates
        Cmd::Args args(std::string(commandStart, commandEnd));
        int argNum = args.Argc() - 1;
        std::string prefix;
        if (!args.Argc() || Str::cisspace(GetText()[GetCursorPos() - 1])) {
            argNum++;
        } else {
            prefix = args.Argv(argNum);
        }

        Cmd::CompletionResult candidates = Cmd::CompleteArgument(args, argNum);
        if (candidates.empty()) {
            return;
        }
        std::sort(candidates.begin(), candidates.end());
        candidates.erase(std::unique(candidates.begin(), candidates.end()), candidates.end());

        //Compute the longest common prefix of all the results
        int prefixSize = candidates[0].first.size();
        size_t maxCandidateLength = 0;
        for (auto& candidate : candidates) {
            prefixSize = std::min(prefixSize, Str::LongestIPrefixSize(candidate.first, candidates[0].first));
            maxCandidateLength = std::max(maxCandidateLength, candidate.first.length());
        }

        std::string completedArg(candidates[0].first, 0, prefixSize);

        //Help the user bash the TAB key, but not when completing paths
        if (candidates.size() == 1 && !Str::cisspace(GetText()[GetCursorPos()]) && !Str::IsSuffix("/", completedArg)) {
            completedArg += " ";
        }

        //Insert the completed arg
        std::u32string toInsert = Str::UTF8To32(completedArg);
        DeletePrev(prefix.size());
        GetText().insert(GetCursorPos(), toInsert);
        SetCursor(GetCursorPos() + toInsert.size());

        //Print the matches if it is ambiguous
        if (candidates.size() >= 2) {
            Log::Notice("^3-> ^*%s", Str::UTF32To8(GetText()).c_str());
            for (const auto& candidate : candidates) {
                std::string filler(maxCandidateLength - candidate.first.length(), ' ');
                Log::Notice("   %s%s %s", candidate.first.c_str(), filler.c_str(), candidate.second.c_str());
            }
        }
    }

}
