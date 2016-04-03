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
#include "FileSystem.h"
#include <fstream>


namespace Profiler{

    /*
         * Function execution times are saved in the samples vector, every element
         * contains the name of the function, the type of sample (function start,
         * function end, and if there's a new frame), and a timestamp.
         */
    std::vector <Profiler::Point>                   samples;
    Sys::SteadyClock::time_point                    startTime;
    bool                                            enabled    = false; ///TODO those bools are silly
    bool                                            nextFrame  = false;
    bool                                            shouldStop = false;


    /// Called at the end of every frame
    void Update(){

        if(nextFrame){
            enabled=true;
            shouldStop=true;
            nextFrame=false;
            startTime   = Sys::SteadyClock::now();
            return;
        }

        if(shouldStop){
            enabled=false;
            shouldStop=false;
            Stop();
            return;
        }

        if(!enabled)
            return;
        samples.push_back(Point(FRAME,""));
    }


    /// Point saves the time it's constructed
    Point::Point(Type type, const char *label=""): type(type), label(label){
        auto timeElapsed = Sys::SteadyClock::now() - startTime;
        time = std::chrono::duration_cast < std::chrono::microseconds > (timeElapsed).count();
    }


    /// At the Beginning and End of every function

    ProfilerGuard::ProfilerGuard( const char * label ): label( label ){
        if(!enabled)
            return;
        samples.push_back( Point( START, label ) );
    }

    ProfilerGuard::~ProfilerGuard(){
        if(!enabled)
            return;
        samples.push_back( Point( END, label ) );
    }


    void Stop(){
        enabled=false;

        auto file = FS::HomePath::OpenWrite("profile.log"); ///TODO add timestamp to file


        std::string line;
        std::string type;

        for (auto& i : samples) {

            switch(i.type){
            case START:
                type="B";
                break;
            case END:
                type="E";
                break;
            case FRAME:
                type="F";
                break;
            }

            line=type + ";" + i.label + ";" + std::to_string(i.time) + "$\n";

            file.Write(line.c_str(), line.length());
        }

        file.Close();
        samples.clear();
    }


    class ProfilerStartCmd: public Cmd::StaticCmd {
    public:
        ProfilerStartCmd(): Cmd::StaticCmd("profiler.start", Cmd::BASE, "starts performance profiling") {
        }

        void Run(const Cmd::Args& args) const OVERRIDE {
            (void)args;

            if(enabled){
                Print("Profiling is already active!");
                return;
            }

            enabled = true;
            startTime = Sys::SteadyClock::now();
        }
    };

    static ProfilerStartCmd ProfilerStartCmdRegistration;


    class ProfilerStopCmd: public Cmd::StaticCmd {
    public:
        ProfilerStopCmd(): Cmd::StaticCmd("profiler.stop", Cmd::BASE, "stop performance profiling") {
        }

        void Run(const Cmd::Args& args) const OVERRIDE {
            (void)args;

            Stop();
        }
    };

    static ProfilerStopCmd ProfilerStopCmdRegistration;


    class ProfilerFrameCmd: public Cmd::StaticCmd {
    public:
        ProfilerFrameCmd(): Cmd::StaticCmd("profiler.frame", Cmd::BASE, "profile the next rendered frame") {
        }

        void Run(const Cmd::Args& args) const OVERRIDE {
            (void)args;

            nextFrame = true;

        }
    };

    static ProfilerFrameCmd ProfilerFrameCmdRegistration;

}
