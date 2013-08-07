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

#include "../../shared/LineEditData.h"
#include "ConsoleHistory.h"

#ifndef FRAMEWORK_CONSOLE_FIELD_H_
#define FRAMEWORK_CONSOLE_FIELD_H_

namespace Console {

    class Field : public Util::LineEditData {
        public:
            Field();
            ~Field();

            void HistoryPrev();
            void HistoryNext();

            void RunCommand(const std::string& defaultCommand = "");
            void AutoComplete();

        private:
            char* oldBuffer = nullptr;
            int oldScroll;
            int oldCursor;
            bool inHistory = false;
            HistoryHandle hist;
    };

}

#endif // FRAMEWORK_CONSOLE_FIELD_H_
