/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2013 Unvanquished Developers

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "ConsoleField.h"
#include "CommandSystem.h"
#include "../../common/String.h"
#include "../qcommon/q_shared.h"
#include <locale>
#include <algorithm>

namespace Console {

    Field::Field(int size): LineEditData(size), hist(HISTORY_END) {
    }

    void Field::HistoryPrev() {
        std::string current = Str::UTF32To8(GetText());
        PrevLine(hist, current);
        SetText(Str::UTF8To32(current));
    }

    void Field::HistoryNext() {
        std::string current = Str::UTF32To8(GetText());
        NextLine(hist, current);
        SetText(Str::UTF8To32(current));
    }

    void Field::RunCommand(const std::string& defaultCommand) {
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
        AddToHistory(hist, std::move(current));

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
        if (!args.Argc() || GetText()[GetCursorPos() - 1] == ' ') {
            argNum++;
        } else {
            prefix = args.Argv(argNum);
        }

        Cmd::CompletionResult candidates = Cmd::CompleteArgument(args, argNum);
        if (candidates.empty()) {
            return;
        }
        std::sort(candidates.begin(), candidates.end());

        //Compute the longest common prefix of all the results
        int prefixSize = candidates[0].first.size();
        size_t maxCandidateLength = 0;
        for (auto& candidate : candidates) {
            prefixSize = std::min(prefixSize, Str::LongestIPrefixSize(candidate.first, candidates[0].first));
            maxCandidateLength = std::max(maxCandidateLength, candidate.first.length());
        }

        std::string completedArg(candidates[0].first, 0, prefixSize);

        //Help the user bash the TAB key
        if (candidates.size() == 1 && GetText()[GetCursorPos()] != ' ') {
            completedArg += " ";
        }

        //Insert the completed arg
        std::u32string toInsert = Str::UTF8To32(completedArg);
        DeletePrev(prefix.size());
        GetText().insert(GetCursorPos(), toInsert);
        SetCursor(GetCursorPos() + toInsert.size());

        //Print the matches if it is ambiguous
        if (candidates.size() >= 2) {
            Com_Printf(S_COLOR_YELLOW "-> " S_COLOR_WHITE "%s\n", Str::UTF32To8(GetText()).c_str());
            for (auto& candidate : candidates) {
                std::string filler(maxCandidateLength - candidate.first.length(), ' ');
                Com_Printf("   %s%s %s\n", candidate.first.c_str(), filler.c_str(), candidate.second.c_str());
            }
        }
    }

}
