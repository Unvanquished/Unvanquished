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
#include "../../shared/String.h"
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
            Cmd::BufferCommandText(current.c_str() + 1, Cmd::END, true);
        } else if (defaultCommand.empty()) {
            Cmd::BufferCommandText(current, Cmd::END, true);
        } else {
            Cmd::BufferCommandText(defaultCommand + " " + current, Cmd::END, true);
        }
        AddToHistory(hist, std::move(current));

        Clear();
    }

    void Field::AutoComplete() {
        //We want to complete in the middle of a command text with potentially multiple commands

        std::string current = Str::UTF32To8(GetText());
        int slashOffset = 0;
        char slashChar = current[0];
        if (slashChar == '/' or slashChar == '\\') {
            slashOffset = 1;
        }
        std::string commandText(current.c_str() + slashOffset);

        //Split the command text and find the command to complete
        std::vector<int> commandStarts = Cmd::StartsOfCommands(commandText);
        if (commandStarts.back() < current.size()) {
            commandStarts.push_back(current.size() + 1);
        }

        int index = 0;
        for (index = commandStarts.size(); index --> 0;) {
            if (commandStarts[index] < GetCursorPos()) {
                break;
            }
        }
        int commandStart = commandStarts[index];

        //Skips whitespaces, like command parsing does
        //TODO: factor it?
        while(commandText[commandStart] == ' ') {
            commandStart ++;
        }
        std::string command(commandText, commandStart, commandStarts[index + 1] - commandStart - 1);

        //Split the command and find the arg to complete
        Cmd::Args args(command);
        int cursorPos = GetCursorPos() - commandStart;
        int argNum = args.PosToArg(cursorPos);
        int argStartPos = args.ArgStartPos(argNum);

        std::vector<std::string> candidates = Cmd::CompleteArgument(command, cursorPos);
        if (candidates.empty()) {
            return;
        }

        //Compute the longest common prefix of all the results
        int prefixSize = candidates[0].size();
        for (auto& candidate : candidates) {
            prefixSize = std::min(prefixSize, Str::LongestPrefixSize(candidate, candidates[0]));
        }

        std::string completedArg(candidates[0], 0, prefixSize);

        //Help the user bash the TAB key
        if (candidates.size() == 1) {
            completedArg += " ";
        }

        //Insert the completed arg
        commandText.replace(commandStart + argStartPos, cursorPos - argStartPos - 1, completedArg);

        //Print the matches if it is ambiguous
        //TODO: multi column nice print?
        if (candidates.size() >= 2) {
            for (auto& candidate : candidates) {
                Com_Printf("  %s\n", candidate.c_str());
            }
        }

        SetText(Str::UTF8To32(slashChar + commandText));
        SetCursor(argStartPos + completedArg.size() + slashOffset + commandStart);
    }

}
