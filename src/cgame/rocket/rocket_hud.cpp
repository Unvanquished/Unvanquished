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

#include "rocket.h"
#include "../cg_local.h"


struct HudUnit
{
	HudUnit( Rocket::Core::ElementDocument *doc ) : unit( doc )
	{
		load = true;
	}

	Rocket::Core::ElementDocument *unit; // Element document that holds the unit
	bool load; // Whether to load or not
};

typedef std::list<HudUnit> RocketHud;
RocketHud *activeHud = nullptr;
std::vector<RocketHud> huds;


// Initialized to WP_NUM_WEAPONS, which may be different based on different mods
void Rocket_InitializeHuds( int size )
{
	huds.clear();

	for ( int i = 0; i < size; ++i )
	{
		huds.push_back( RocketHud() );
	}

	activeHud = nullptr;
}

void Rocket_LoadUnit( const char *path )
{
	Rocket::Core::ElementDocument *document = hudContext->LoadDocument( path );

	Rocket::Core::ElementDocument* other;

	if ( document )
	{
		document->RemoveReference();
		hudContext->PullDocumentToFront( document );

		// Close any other documents which may have the same ID
		other = hudContext->GetDocument( document->GetId() );
		if ( other && other != document )
		{
			other->Close();
		}

		document->Hide();
	}
}

void Rocket_AddUnitToHud( int weapon, const char *id )
{
	if ( id && *id )
	{
		Rocket::Core::ElementDocument *doc = hudContext->GetDocument( id );
		if ( doc )
		{
			huds[ weapon ].push_back( HudUnit( doc ) );
		}
	}
}

// See if unit exists in a hud
static HudUnit *findUnit( RocketHud *in, HudUnit &find )
{
	for ( RocketHud::iterator it = in->begin(); it != in->end(); ++it )
	{
		if ( it->unit == find.unit )
		{
			return &*it;
		}
	}

	return nullptr;
}

void Rocket_ShowHud( int weapon )
{
	RocketHud *currentHud = &huds[ weapon ];

	if ( activeHud )
	{
		for ( RocketHud::iterator it = activeHud->begin(); it != activeHud->end(); ++it )
		{
			HudUnit *unit;
			// Check if the new HUD needs this unit
			if ( ( unit = findUnit( currentHud, *it ) ) )
			{
				unit->load = false;

				continue;
			}

			// Nope. Doesn't need it
			it->unit->Hide();
		}
	}

	for ( RocketHud::iterator it = currentHud->begin(); it != currentHud->end(); ++it )
	{
		if ( it->load )
		{
			it->unit->Show();
		}

		// No matter what, reset load status
		it->load = true;
	}

	activeHud = currentHud;
}

void Rocket_ClearHud( unsigned weapon )
{
	if ( weapon < huds.size() )
	{
		huds[ weapon ].clear();
	}
}

void Rocket_ShowScoreboard( const char *name, bool show )
{
	Rocket::Core::ElementDocument* doc = hudContext->GetDocument( name );

	if ( doc )
	{
		if ( show )
		{
			doc->Show();
		}
		else
		{
			doc->Hide();
		}
	}
}
