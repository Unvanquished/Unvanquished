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

// Code for generating custom events for libRocket

#include <queue>
#include "rocket.h"
#include <RmlUi/Core.h>
#include "../cg_local.h"

std::queue< RocketEvent_t* > eventQueue;
extern Rml::Element *activeElement;

void Rocket_ProcessEvent( Rml::Event& event, Rml::String& value );

void Rocket_ProcessEvent( Rml::Event& event, Rml::String& value )
{
	Rml::StringList list;

	Rml::StringUtilities::ExpandString( list, value, ';' );
	for ( size_t i = 0; i < list.size(); ++i )
	{
		eventQueue.push( new RocketEvent_t( event, list[ i ] ) );
	}
}

bool Rocket_GetEvent(std::string& cmdText)
{
	if ( !eventQueue.empty() )
	{
		cmdText = eventQueue.front()->cmd.c_str();
		activeElement = eventQueue.front()->targetElement;
		return true;
	}

	return false;
}

void Rocket_DeleteEvent()
{
	RocketEvent_t *event = eventQueue.front();
	eventQueue.pop();
	activeElement = nullptr;
	delete event;
}

void Rocket_GetEventParameters( char *params, int /*length*/ )
{
	RocketEvent_t *event = eventQueue.front();
	*params = '\0';
	if ( !eventQueue.empty() )
	{
		for ( const auto& it : event->Parameters )
		{
			Info_SetValueForKeyRocket( params, it.first.c_str(), it.second.Get<Rml::String>().c_str(), true );
		}
	}
}

void Rocket_AddEvent( RocketEvent_t *event )
{
	eventQueue.push( event );
}
