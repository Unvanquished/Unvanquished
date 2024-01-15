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

class RocketFocusManager : public Rml::EventListener
{
public:
	RocketFocusManager() { }
	void ProcessEvent( Rml::Event &evt )
	{
		// 0 = ignore all input
		// 1 = capture mouse; optionally consume keys while passing on to binds if not consumed
		// 2 = capture everything
		int catchLevel;

		if ( rocketInfo.cstate.connState < connstate_t::CA_PRIMED )
		{
			catchLevel = 2;
		}
		else
		{
			catchLevel = 0;

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
						catchLevel = 2;
						break;
					}

					catchLevel = 1;
				}
			}
		}

		switch ( catchLevel )
		{
		case 0:
			CG_SetKeyCatcher( rocketInfo.keyCatcher & ~KEYCATCH_UI_MOUSE & ~KEYCATCH_UI_KEY );
			break;

		case 1:
			CG_SetKeyCatcher( ( rocketInfo.keyCatcher | KEYCATCH_UI_MOUSE ) & ~KEYCATCH_UI_KEY );
			break;

		case 2:
			if ( !( rocketInfo.keyCatcher & KEYCATCH_UI_KEY ) )
			{
				trap_Key_ClearCmdButtons();
				trap_Key_ClearStates();
			}

			CG_SetKeyCatcher( rocketInfo.keyCatcher | KEYCATCH_UI_MOUSE | KEYCATCH_UI_KEY );
			break;

		}
	}

private:
	// Checks if parents are visible as well
	bool IsTreeVisible( Rml::Element *element )
	{
		if ( element && element->IsVisible() )
		{
			Rml::Element *parent = element;

			while ( ( parent = parent->GetParentNode() ) )
			{
				if ( !parent->IsVisible() )
				{
					return false;
				}
			}

			return true;
		}

		else
		{
			return false;
		}
	}
};
#endif
