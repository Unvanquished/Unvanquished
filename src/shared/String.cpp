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

#include "String.h"
#include "../engine/qcommon/q_shared.h"

namespace Str {

    int ToInt(const std::string& text) {
        return atoi(text.c_str());
    }

    int ToInt(const std::string& text, bool& success) {
        try {
            int res = std::stoi(text);
            success = true;
            return res;
        } catch(...) {
            success = false;
        }
        return false;
    }

}
