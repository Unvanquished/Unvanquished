/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#ifndef ROCKETFOCUSMANAGER_H
#define ROCKETFOCUSMANAGER_H

#include "../cg_local.h"
#include "rocket.h"
#include <RmlUi/Core.h>

// input capturing is based on this
enum class VisibleMenusState
{
	// menus ignore all input
	NONE,
	// menus capture mouse (if cg_circleMenusCaptureMouse set), and any keystrokes consumed
	// by the menu, while other keystrokes are passed through to binds
	ONLY_PASSTHROUGH,
	// menus capture everything
	NON_PASSTHROUGH,
};

class RocketFocusManager : public Rml::EventListener
{
	VisibleMenusState state_ = VisibleMenusState::NON_PASSTHROUGH;

public:
	RocketFocusManager() { }

	VisibleMenusState GetState() { return state_; }

	void ProcessEvent( Rml::Event& )
	{
		VisibleMenusState catchLevel;

		if ( rocketInfo.cstate.connState < connstate_t::CA_PRIMED )
		{
			catchLevel = VisibleMenusState::NON_PASSTHROUGH;
		}
		else
		{
			catchLevel = VisibleMenusState::NONE;

			for ( const rocketMenu_t &menu : rocketInfo.menu )
			{
				if ( !menu.id )
				{
					continue;
				}

				auto *document = menuContext->GetDocument( menu.id );
				if ( document && document->IsVisible() )
				{
					if ( !menu.passthrough )
					{
						catchLevel = VisibleMenusState::NON_PASSTHROUGH;
						break;
					}
					else
					{
						catchLevel = VisibleMenusState::ONLY_PASSTHROUGH;
					}
				}
			}
		}

		state_ = catchLevel;

		if ( catchLevel == VisibleMenusState::ONLY_PASSTHROUGH && !cg_circleMenusCaptureMouse.Get() )
		{
			catchLevel = VisibleMenusState::NONE;
		}

		switch ( catchLevel )
		{
		case VisibleMenusState::NONE:
			CG_SetKeyCatcher( rocketInfo.keyCatcher & ~KEYCATCH_UI_MOUSE & ~KEYCATCH_UI_KEY );
			break;

		case VisibleMenusState::ONLY_PASSTHROUGH:
			CG_SetKeyCatcher( ( rocketInfo.keyCatcher | KEYCATCH_UI_MOUSE ) & ~KEYCATCH_UI_KEY );
			break;

		case VisibleMenusState::NON_PASSTHROUGH:
			if ( !( rocketInfo.keyCatcher & KEYCATCH_UI_KEY ) )
			{
				trap_Key_ClearCmdButtons();
				trap_Key_ClearStates();
			}

			CG_SetKeyCatcher( rocketInfo.keyCatcher | KEYCATCH_UI_MOUSE | KEYCATCH_UI_KEY );
			break;

		}
	}
};
#endif
