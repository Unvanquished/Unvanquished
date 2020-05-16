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

#ifndef CG_KEY_NAME_H_
#define CG_KEY_NAME_H_

#include "common/KeyIdentification.h"
#include "shared/bg_public.h"

std::string CG_KeyDisplayName(Keyboard::Key key);

void CG_SetBindTeam(team_t team);

int CG_CurrentBindTeam();

#endif // CG_KEY_NAME_H_
