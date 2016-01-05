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

#ifndef FRAMEWORK_LOG_SYSTEM_H_
#define FRAMEWORK_LOG_SYSTEM_H_

/*
 * The log system takes log events from different sources and forwards them
 * to a number of targets. The event and the targets are decoupled so that
 * we can choose precisely where to send each event. Targets include
 * the TTY and graphical console and the HUD.
 *
 * A full list of the targets and "printing" facilities can be found in
 * common/Log
 */

namespace Log {

    // Dispatches the event to all the targets specified by targetControl (flags)
    // Can be called by any thread.
    void Dispatch(Log::Event event, int targetControl);

    // Open the log file and start writing to it
    void OpenLogFile();

    class Target {
        public:
            Target();

            // Should process all the logs in the batch given or none at all
            // return true iff the logs were processed (on false the log system
            // retains them for later).
            // Can be called by any thread.
            virtual bool Process(const std::vector<Log::Event>& events) = 0;

        protected:
            // Register itself as the target with this id
            void Register(TargetId id);
    };

    // Internal

    void RegisterTarget(TargetId id, Target* target);
}

#endif //FRAMEWORK_LOG_SYSTEM_H_
