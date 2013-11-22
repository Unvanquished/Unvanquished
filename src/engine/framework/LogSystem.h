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

#include "../../common/Log.h"
#include <vector>

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

    class Target {
        public:
            Target();

            // Should process all the logs in the batch given or none at all
            // return true iff the logs were processed (on false the log system
            // retains them for later).
            // Can be called by any thread.
            virtual bool Process(std::vector<Log::Event>& events) = 0;

        protected:
            // Register itself as the target with this id
            void Register(TargetId id);
    };

    // Internal

    void RegisterTarget(TargetId id, Target* target);
}

#endif //FRAMEWORK_LOG_SYSTEM_H_
