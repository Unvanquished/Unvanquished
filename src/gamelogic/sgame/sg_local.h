/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

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

#ifndef G_LOCAL_H_
#define G_LOCAL_H_

// engine headers
#include "../../engine/qcommon/q_shared.h"
#include "../../engine/server/g_api.h"
#include "../../engine/botlib/bot_types.h"

// shared gamelogic (bg) headers
#include "../shared/bg_public.h"

// macros and common constants
#include "g_definitions.h"

// type definitions
#include "g_typedef.h"

// topic function headers and definitions
#include "g_admin.h"
#include "g_bot.h"
#include "g_entities.h"

// struct definitions
#include "g_struct.h"

// function headers
#include "g_public.h"

// trapcall headers
#include "g_trapcalls.h"

// externalized fields
#include "g_extern.h"

// future imports
#ifndef Q3_VM
#include "../../common/Maths.h"
#endif

#endif // G_LOCAL_H_
