/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/
#ifndef ROCKET_H
#define ROCKET_H
#ifdef DotProduct
// Ugly hack to fix the DotProduct conflict
#undef DotProduct
#endif

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-pedantic"
#include <Rocket/Core/Core.h>
#pragma GCC diagnostic pop

extern Rocket::Core::Context *menuContext;
extern Rocket::Core::Context *hudContext;

class RocketEvent_t
{
public:
	RocketEvent_t( Rocket::Core::Event &event, const Rocket::Core::String &cmds ) : cmd( cmds )
	{
		targetElement = event.GetTargetElement();
		Parameters = *(event.GetParameters());
	}
	RocketEvent_t( const Rocket::Core::String &cmds ) : cmd( cmds )
	{
	}
	RocketEvent_t( Rocket::Core::Element *e, const Rocket::Core::String &cmds ) : targetElement( e ), cmd( cmds )
	{
	}
	~RocketEvent_t() { }
	Rocket::Core::Element *targetElement;
	Rocket::Core::Dictionary Parameters;
	Rocket::Core::String cmd;
};

Rocket::Core::String Rocket_QuakeToRML( const char *in, int parseFlags );
Rocket::Core::String CG_KeyBinding( const char *bind, int team );
void Rocket_AddEvent( RocketEvent_t *event );
#endif
