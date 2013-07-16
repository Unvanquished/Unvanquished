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

namespace Cmd {

    static CmdBase* firstCommand = nullptr;

    std::string Escape(const std::string& text, bool quote) {
        std::string res;

        if (quote) {
            res = "\"";
        }

        for (int i = 0; i < text.size(); i ++) {
            char c = text[i];

            bool commentStart = (i < text.size() + 1) and (c == '/') and (text[i + 1] == '/' or text[i + 1] == '*');
            bool escapeNotQuote = (c <= ' ' and c > 0) or (c == ';') or commentStart;
            bool escape = (c == '$') or (c == '"') or (c == '\\');

            if ((not quote and escapeNotQuote) or escape) {
                res.push_back('\\');
            }
            res.push_back(c);
        }

        if (quote) {
            res += "\"";
        }

        return res;
    }

    void Tokenize(const std::string& text, std::vector<std::string>& tokens, std::vector<int>& tokenStarts) {
        const char* raw_text = text.c_str();
        std::string token;
        int tokenStart = 0;

        int pos = 0;
        bool inToken = false;

        while (pos < text.size()) {
            char c = text[pos ++];

            //Avoid whitespaces
            if (c <= ' ') {
                continue;
            }

            //Check for comments
            if (c == '/' and pos < text.size()) {
                char nextC = text[pos];

                // a // finishes the text and the token
                if (nextC == '/') {
                    break;

                // if it is a /* jump to the end of it
                } else if (nextC == '*'){
                    pos ++; //avoid /*/
                    while (pos < text.size() - 1 and not (text[pos] == '*' and text[pos + 1] == '/') ) {
                        pos ++;
                    }
                    pos += 2;

                    //The comment doesn't end
                    if (pos >= text.size()) {
                        break;
                    }
                    continue;
                }
            }

            //We have something that is not whitespace nor comments so it must be a token

            if (c == '"' and pos < text.size()) {
                //This is a quoted token
                bool escaped = false;

                c = text[pos ++]; //skips the "

                //Add all the characters (except the escape \) until the last unescaped "
                while (pos < text.size() and (escaped or c !='"')) {
                    if (escaped or c != '\\') {
                        token.push_back(c);
                        escaped = false;
                    } else if (not escaped) { // c = \ here
                        escaped = true;
                    }
                    c = text[pos ++];
                }

                tokens.push_back(token);
                tokenStarts.push_back(tokenStart);
                token = "";
                tokenStart = pos;

            } else {
                //An unquoted string, until the next " or comment start
                bool escaped = false;
                bool finished;
                bool startsSomethingElse;

                do {
                    if (escaped or c != '\\') {
                        token.push_back(c);
                        escaped = false;
                    } else if (not escaped) { // c = \ here
                        escaped = true;
                    }
                    c = text[pos ++];

                    startsSomethingElse = (
                        (pos < text.size() and c == '"') or //Start of a quote or start of a comment
                        (pos < text.size() and c == '/' and (text[pos] == '/' or text[pos] == '*'))
                    );

                    finished = not escaped and (c <= ' ' or startsSomethingElse);
                }while (pos < text.size() and not finished);

                //If we did not finish early (hitting another something)
                if(not escaped and not finished){
                    token.push_back(c);
                }

                tokens.push_back(token);
                tokenStarts.push_back(tokenStart);
                token = "";

                //Get back to the beginning of the seomthing
                if(startsSomethingElse){
                    pos--;
                }
                tokenStart = pos;
            }
        }

        //Handle the last token
        if (token != ""){
            tokens.push_back(token);
            tokenStarts.push_back(tokenStart);
        }
    }

    std::vector<int> StartsOfCommands(const std::string& text) {
        std::vector<int> res = {0};

        bool inQuotes = false;
        bool escaped = false;
        bool inComment = false;
        bool inInlineComment = false;

        for (int i = 0; i < text.size(); i++) {
            if (escaped) {
                escaped = false;
                continue;
            }

            char c = text[i];

            if (not inInlineComment) {
                // /* */ comments have precedence over everything
                if (c == '\n') {
                    inComment = false;
                    res.push_back(i + 1);
                    continue;
                }

                if (not inComment) {
                    if (c == '\\') {
                        escaped = true;
                        continue;
                    }

                    if (c == '"') {
                        inQuotes = not inQuotes;
                        continue;
                    }

                    if (c == ';' and not inQuotes) {
                        res.push_back(i + 1);
                        continue;
                    }
                }
            }

            if (not inQuotes and i < text.size() - 1) {
                //Search for 2 char delimiters
                char c2 = text[i + 1];

                if (inInlineComment and c == '*' and c2 == '/') {
                    inInlineComment = false;
                    i++;
                    continue;
                }

                if (not inInlineComment and c == '/' and c2 == '/') {
                    inComment = true;
                    continue;
                }

                if (not inComment and c == '/' and c2 == '*') {
                    inInlineComment = true;
                    continue;
                }
            }
        }

        return res;
    }


    std::list<std::string> SplitCommandText(const std::string& commands) {
        std::list<std::string> res;
        const char *data = commands.data();

        std::vector<int> start = StartsOfCommands(commands);
        if (start.back() < commands.size()) {
            start.push_back(commands.size() + 1);
        }

        for(int i = 0; i < start.size() - 1; i++) {
            //Get the command, except the command delimiter character (if there is one)
            int p = start[i];
            //Strip leading white space
            while (p < start[i + 1] && data[p] >= 0 && data[p] <= ' ')
                ++p;
            //If the result is not "", add it to the list
            if (p < start[i + 1] && data[p] != ';')
                res.push_back(std::string(commands.c_str() + p, start[i + 1] - p - 1));
        }

        return res;
    }

    std::string SubstituteCvars(const std::string& text) {
        const char* raw_text = text.c_str();
        std::string result;

        bool isEscaped = false;
        bool inCvarName = false;
        int lastBlockStart = 0;

        //Cvar are delimited by $ so we parse a bloc at a time
        for(int i = 0; i < text.size(); i++){

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
                    //For now we use the cvar C api to get the cvar value but it should be replaced
                    //by Cvar::get(cvarName)->getString() or something
                    char cvarValue[ MAX_CVAR_VALUE_STRING ];
                    Cvar_VariableStringBuffer( block.c_str(), cvarValue, sizeof( cvarValue ) );
                    result += Escape(std::string(cvarValue));

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

    Args::Args(std::string command) {
        cmd = std::move(command);
        Tokenize(cmd, args, argsStarts);
    }

    int Args::Argc() const {
        return args.size();
    }

    const std::string& Args::Argv(int argNum) const {
        return args[argNum];
    }

    std::string Args::QuotedArgs(int start, int end) const {
        std::string res;

        if (end < 0) {
            end = args.size() - 1;
        }

        for (int i = start; i < end + 1; i++) {
            if (i != start) {
                res += " ";
            }
            res += Escape(args[i], true);
        }

        return res;
    }

    const std::string& Args::RawArgs() const {
        return cmd;
    }

    const std::string& Args::RawArgsFrom(int start) const {
        if (start < argsStarts.size()) {
            return cmd.c_str() + argsStarts[start];
        } else {
            return "";
        }
    }

    int Args::PosToArg(int pos) {
        for (int i = argsStarts.size(); i-->0;) {
            if (argsStarts[i] <= pos) {
                return i;
            }
        }

        return -1;
    }

    int Args::ArgStartPos(int argNum) {
        return argsStarts[argNum];
    }

    /*
    ===============================================================================

    Cmd::CmdBase

    ===============================================================================
    */

    CmdBase::CmdBase(const cmdFlags_t flags): flags(flags) {
    }

    std::vector<std::string> CmdBase::Complete(int argNum, const Args& args) const {
        return {};
    }

    void CmdBase::PrintUsage(const Args& args, const std::string& syntax, const std::string& description) {
        if(description.empty()) {
            Com_Printf("%s: %s %s\n", _("usage"), args.Argv(0).c_str(), syntax.c_str());
        } else {
            Com_Printf("%s: %s %s — %s\n", _("usage"), args.Argv(0).c_str(), syntax.c_str(), description.c_str());
        }
    }

    cmdFlags_t CmdBase::GetFlags() const {
        return flags;
    }

    StaticCmd::StaticCmd(std::string name, const cmdFlags_t flags, std::string description)
    :CmdBase(flags){
        //Register this command statically
        AddCommand(std::move(name), *this, std::move(description));
    }


}
