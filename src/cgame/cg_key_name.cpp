/*
===========================================================================

Copyright 2018 Unvanquished Developers

This file is part of Unvanquished.

Unvanquished is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished. If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "cg_key_name.h"

#include "common/Log.h"
#include "common/String.h"
#include "engine/client/cg_api.h"

using Keyboard::Key;

std::string CG_KeyDisplayName(Key key) {
    switch (key.kind()) {
    case Key::Kind::SCANCODE:
    {
        for (auto& functionKey : Keyboard::leftRightFunctionKeys) {
            if (functionKey.scancode == key.AsScancode()) {
                return functionKey.name;
            }
        }
        int character = trap_Key_GetCharForScancode(key.AsScancode());
        if (character) {
            return Keyboard::CharToString(character);
        } else {
            return Str::Format("0x%02x", key.AsScancode());
        }
    }
    case Key::Kind::KEYNUM:
        return Keyboard::KeynumToString(key.AsKeynum());
    case Key::Kind::UNICODE_CHAR:
        return "[Character]" + Keyboard::CharToString(key.AsCharacter());
    default:
        return "???";
    }
}

static int bindTeam;

void CG_SetBindTeam(team_t team) {
    switch (team) {
    case TEAM_NONE:
        bindTeam = 3; // BIND_TEAM_SPECTATORS
        break;
    case TEAM_HUMANS:
    case TEAM_ALIENS:
        bindTeam = static_cast<int>(team);
        break;
    default:
        Log::Warn("Invalid team %d", team);
    }
}

// Get the current team according to the Keyboard::BindTeam type in the engine
int CG_CurrentBindTeam() {
    return bindTeam;
}
