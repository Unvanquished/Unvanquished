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

#include "Command.h"

#include "../engine/qcommon/qcommon.h"
#include "../engine/framework/CommandSystem.h"
#include "../engine/framework/CvarSystem.h"

namespace Cmd {

    std::string Escape(const std::string& text) {
        std::string res = "\"";

        for (size_t i = 0; i < text.size(); i ++) {
            if (text[i] == '\"') {
                res.push_back('\\');
            }
            res.push_back(text[i]);
        }

        res += "\"";
        return res;
    }

    static bool SkipSpaces(std::string::const_iterator& in, std::string::const_iterator end)
    {
        bool foundSpace = false;
        while (in != end) {
            // Handle spaces
            if ((in[0] & 0xff) <= ' ') {
                foundSpace = true;
                in++;
            } else if (in + 1 != end) {
                // Handle // comments
                if (in[0] == '/' && in[1] == '/') {
                    in = end;
                    return true;
                }
                // Handle /* */ comments
                if (in[0] == '/' && in[1] == '*') {
                    foundSpace = true;
                    in += 2;
                    while (in != end) {
                        if (in + 1 != end && in[0] == '*' && in[1] == '/') {
                            in += 2;
                            break;
                        } else {
                            in++;
                        }
                    }
                } else {
                    // Non-space character
                    return foundSpace;
                }
            } else {
                // Non-space character
                return foundSpace;
            }
        }
        return true;
    }

    std::string::const_iterator SplitCommand(const std::string& text, std::string::const_iterator start) {
        bool inQuote = false;
        bool inComment = false;
        bool inInlineComment = false;

        while (start != text.end()) {
            //End of comment
            if (inInlineComment && start + 1 != text.end() && start[0] == '*' && start[1] == '/') {
                inInlineComment = false;
                start += 2;
                continue;
            }

            //Ignore everything else in an inline comment
            if (inInlineComment) {
                start++;
                continue;
            }

            //Start of comment
            if (not inQuote && not inComment && start + 1 != text.end() && start[0] == '/' && start[1] == '*') {
                inInlineComment = true;
                start += 2;
                continue;
            }
            if (not inQuote && start + 1 != text.end() && start[0] == '/' && start[1] == '/') {
                inComment = true;
                start += 2;
                continue;
            }

            //Escaped quote
            if (inQuote && start + 1 != text.end() && start[0] == '\\' && start[1] == '"') {
                start += 2;
                continue;
            }

            //Quote
            if (start[0] == '"') {
                inQuote = !inQuote;
                start++;
                continue;
            }

            //Semicolon
            if (start[0] == ';' && !inQuote && !inComment) {
                return start + 1;
            }

            //Newline
            if (start[0] == '\n') {
                return start + 1;
            }

            //Normal character
            start++;
        }

        return text.end();
    }

    std::string SubstituteCvars(const std::string& text) {
        const char* raw_text = text.c_str();
        std::string result;

        bool isEscaped = false;
        bool inCvarName = false;
        int lastBlockStart = 0;

        //Cvar are delimited by $ so we parse a bloc at a time
        for(size_t i = 0; i < text.size(); i++){

            // a \ escapes the next letter so we don't use it for block delimitation
            if (isEscaped) {
                isEscaped = false;
                continue;
            }

            if (text[i] == '\\') {
                isEscaped = true;

            } else if (text[i] == '$') {
                //Found a block, every second block is a cvar name block
                std::string block(raw_text + lastBlockStart, i - lastBlockStart);

                if (inCvarName) {
                    result += Escape(Cvar::GetValue(block));
                    inCvarName = false;

                } else {
                    result += block;
                    inCvarName = true;
                }

                lastBlockStart = i + 1;
            }
        }

        //Handle the last block
        if (inCvarName) {
            Com_Printf("Warning: last CVar substitution block not closed in %s\n", raw_text);
        } else {
            std::string block(raw_text + lastBlockStart, text.size() - lastBlockStart);
            result += block;
        }

        return result;
    }

    /*
    ===============================================================================

    Cmd::CmdArgs

    ===============================================================================
    */

    Args::Args() {
    }

    Args::Args(std::vector<std::string> args_) {
        args = std::move(args_);

        for (size_t i = 0; i < args.size(); i++) {
            argsStarts.push_back(cmd.size());
            cmd += Escape(args[i]);
            if (i != args.size() - 1) {
                cmd += " ";
            }
        }
    }

    Args::Args(std::string command) {
        cmd = std::move(command);

        std::string::const_iterator in = cmd.begin();

        //Skip leading whitespace
        SkipSpaces(in, cmd.end());

        //Return if the cmd is empty
        if (in == cmd.end())
            return;

        //Build arg tokens
        std::string currentToken;
        int currentTokenStart = in - cmd.begin();
        while (true) {
            //Check for end of current token
            if (SkipSpaces(in, cmd.end())) {
                 args.push_back(std::move(currentToken));
                 argsStarts.push_back(currentTokenStart);
                 if (in == cmd.end())
                    return;
                 currentToken.clear();
                 currentTokenStart = in - cmd.begin();
            }

            //Handle quoted strings
            if (in[0] == '\"') {
                in++;
                //Read all characters until quote end
                while (in != cmd.end()) {
                    if (in[0] == '\"') {
                        in++;
                        break;
                    } else if (in + 1 !=  cmd.end() && in[0] == '\\' && in[1] == '\"') {
                        currentToken.push_back('\"');
                        in++;
                    } else {
                        currentToken.push_back(in[0]);
                        in++;
                    }
                }
            } else {
                //Handle normal character
                currentToken.push_back(in[0]);
                in++;
            }
        }
    }

    int Args::Argc() const {
        return args.size();
    }

    const std::string& Args::Argv(int argNum) const {
        return args[argNum];
    }

    std::string Args::EscapedArgs(int start, int end) const {
        std::string res;

        if (end < 0) {
            end = args.size() - 1;
        }

        for (int i = start; i < end + 1; i++) {
            if (i != start) {
                res += " ";
            }
            res += Escape(args[i]);
        }

        return res;
    }

    const std::string& Args::RawCmd() const {
        return cmd;
    }

    std::string Args::RawArgsFrom(unsigned start) const {
        if (start < argsStarts.size()) {
            return std::string(cmd.begin() + argsStarts[start], cmd.end());
        } else {
            return "";
        }
    }

    int Args::PosToArg(int pos) const {
        for (int i = argsStarts.size(); i-->0;) {
            if (argsStarts[i] <= pos) {
                return i;
            }
        }

        return -1;
    }

    int Args::ArgStartPos(unsigned argNum) const {
        if (argNum > argsStarts.size()) {
            return cmd.size();
        }
        return argsStarts[argNum];
    }

    std::string Args::ArgPrefix(int pos) const {
        int argNum = PosToArg(pos);
        return args[argNum].substr(0, pos);
    }

    const std::vector<std::string>& Args::ArgVector() const {
        return args;
    }

    int Args::size() const {
        return Argc();
    }

    const std::string& Args::operator[] (int argNum) const {
        return Argv(argNum);
    }

    /*
    ===============================================================================

    Cmd::CmdBase

    ===============================================================================
    */

    CmdBase::CmdBase(const int flags): flags(flags) {
    }

    CompletionResult CmdBase::Complete(int argNum, const Args& args) const {
        Q_UNUSED(argNum); //TODO
        Q_UNUSED(args); //TODO
        return {};
    }

    void CmdBase::PrintUsage(const Args& args, const std::string& syntax, const std::string& description) {
        if(description.empty()) {
            Com_Printf("%s: %s %s\n", _("usage"), args.Argv(0).c_str(), syntax.c_str());
        } else {
            Com_Printf("%s: %s %s — %s\n", _("usage"), args.Argv(0).c_str(), syntax.c_str(), description.c_str());
        }
    }

    int CmdBase::GetFlags() const {
        return flags;
    }

    StaticCmd::StaticCmd(std::string name, const int flags, std::string description)
    :CmdBase(flags){
        //Register this command statically
        AddCommand(std::move(name), *this, std::move(description));
    }


}
