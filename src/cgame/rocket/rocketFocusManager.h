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
		bool anyVisible = false;
		Rml::Context* context = evt.GetTargetElement() ? evt.GetTargetElement()->GetContext() : nullptr;

		if ( context )
		{
			for ( int i = 0; i < context->GetNumDocuments(); ++i )
			{
				if ( context->GetDocument( i )->IsVisible() )
				{
					anyVisible = true;
					break;
				}
			}
		}

		if ( anyVisible && ! ( rocketInfo.keyCatcher & KEYCATCH_UI ) )
		{
			trap_Key_ClearCmdButtons();
			trap_Key_ClearStates();
			CG_SetKeyCatcher( rocketInfo.keyCatcher | KEYCATCH_UI );
		}
		else if ( !anyVisible && rocketInfo.keyCatcher && rocketInfo.cstate.connState >= connstate_t::CA_PRIMED )
		{
			CG_SetKeyCatcher( rocketInfo.keyCatcher & ~KEYCATCH_UI );
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
