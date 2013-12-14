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

#ifndef FRAMEWORK_CONSOLE_HISTORY_H_
#define FRAMEWORK_CONSOLE_HISTORY_H_

namespace Console {

    typedef unsigned HistoryHandle;
    static const HistoryHandle HISTORY_END = -1;

    void SaveHistory();
    void LoadHistory();

    void AddToHistory(HistoryHandle& handle, std::string current);
    void PrevLine(HistoryHandle& handle, std::string& current);
    void NextLine(HistoryHandle& handle, std::string& current);

}

#endif // FRAMEWORK_CONSOLE_HISTORY_H_
