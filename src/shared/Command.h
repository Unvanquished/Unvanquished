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

#include "../engine/qcommon/q_shared.h"
#include <string>
#include <vector>

#ifndef SHARED_COMMAND_H_
#define SHARED_COMMAND_H_

namespace Cmd {

    enum {
        BASE             = BIT(0),
        ALIAS            = BIT(1),
        SYSTEM           = BIT(2),
        RENDERER         = BIT(3),
        SOUND            = BIT(4),
        GAME             = BIT(5),
        CGAME            = BIT(6),
        UI               = BIT(6),
        PROXY_FOR_OLD    = BIT(31)
    };


    std::string Escape(const std::string& text, bool quote = false);
    void Tokenize(const std::string& text, std::vector<std::string>& tokens, std::vector<int>& tokenStarts);
    std::vector<int> StartsOfCommands(const std::string& text);
    std::vector<std::string> SplitCommandText(const std::string& commands);
    std::string SubstituteCvars(const std::string& text);

    class Args {
        public:
            Args();
            Args(std::vector<std::string> args);
            Args(std::string command);

            int Argc() const;
            const std::string& Argv(int argNum) const;
            std::string EscapedArgs(int start = 1, int end = -1) const;
            const std::string& RawCmd() const;
            std::string RawArgsFrom(int start = 1) const;

            int PosToArg(int pos);
            int ArgStartPos(int argNum);

            const std::vector<std::string>& ArgVector() const;

            int size() const; // same as Argc()
            const std::string& operator[] (int argNum) const; // same as Argv(int)

        private:
            std::vector<std::string> args;
            std::vector<int> argsStarts;
            std::string cmd;
    };


    class CmdBase {
        public:
            virtual void Run(const Args& args) const = 0;
            virtual std::vector<std::string> Complete(int argNum, const Args& args) const;

            int GetFlags() const;

            static void PrintUsage(const Args& args, const std::string& syntax, const std::string& description = "");

        protected:
            CmdBase(int flags);

        private:
            int flags;
    };

    class StaticCmd : public CmdBase {
        protected:
            StaticCmd(std::string name, int flags, std::string description);
    };
}

#endif // SHARED_COMMAND_H_
