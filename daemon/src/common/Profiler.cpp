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

#include "Profiler.h"
#include "../engine/framework/LogSystem.h"

#include "../engine/framework/CommandSystem.h"
#include "../engine/framework/BaseCommands.h"



namespace Profiler{


std::vector <Profiler::Point>                   times;
std::chrono::high_resolution_clock::time_point  start;
bool                                            enabled = false;


std::chrono::microseconds::rep TimeElapsed( std::chrono::high_resolution_clock::time_point since )
{
    auto tmp = std::chrono::high_resolution_clock::now() - since;

    return std::chrono::duration_cast < std::chrono::microseconds > (tmp).count();
}

void Update(){
    if( !enabled )
        return;

    times.push_back(Point(FRAME));
}


//void PrintRaw(){
//    for (auto& i : times) {
//        std::string type;
//        switch(i.type){
//        case START:
//            type="start";
//            break;
//        case END:
//            type="end";
//            break;
//        case FRAME:
//            type="frame";
//            break;
//        }
//        std::cout << type << ", "<< i.label << ", " << i.time <<"\n";
//    }
//    times.clear();
//}




Profile::Profile( std::string label ): label( label ){
    if( !enabled )
        return;

    times.push_back( Point(START, label, TimeElapsed(start) ) );
}

Profile::~Profile(){
    if( !enabled )
        return;

    times.push_back( Point(END, label, TimeElapsed(start) ) );
}




class ProfilerStartCmd: public Cmd::StaticCmd {
public:
    ProfilerStartCmd(): Cmd::StaticCmd("profiler.start", Cmd::RENDERER, "prints to the console") {
    }

    void Run(const Cmd::Args& args) const OVERRIDE {

        enabled=true;
        start=std::chrono::high_resolution_clock::now();

    }
};

static ProfilerStartCmd ProfilerStartCmdRegistration;



class ProfilerStopCmd: public Cmd::StaticCmd {
public:
    ProfilerStopCmd(): Cmd::StaticCmd("profiler.stop", Cmd::RENDERER, "prints to the console") {
    }

    void Run(const Cmd::Args& args) const OVERRIDE {

        enabled=false;

        uint padding=0;
        std::map <uint, std::chrono::microseconds::rep> levels;

        for (auto& i : times) {

            if(i.type==FRAME){
                Print("\n");
                continue;
            }

            if(i.type==START){
                padding++;
                levels[padding]=i.time;
            }else if (i.type==END){
                std::string padstr;
                for(char j=0;j<padding;j++)
                    padstr+= "    ";

                Print("%s %s %d",padstr, i.label, i.time - levels[padding] );
                padstr="";

                padding--;
            }
        }

        times.clear();
    }
};
static ProfilerStopCmd ProfilerStopCmdRegistration;

}
