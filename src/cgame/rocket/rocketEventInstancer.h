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

#ifndef ROCKETEVENTINSTANCER_H
#define ROCKETEVENTINSTANCER_H

#include <RmlUi/Core.h>
#include "rocket.h"

void Rocket_ProcessEvent( Rml::Event&, Rml::String& );

class RocketEvent : public Rml::EventListener
{
public:
	RocketEvent(const Rml::String& value) : value( value ) { }
	~RocketEvent() { }

	/// Sends the event value through to the CGame's event processing system.
	void ProcessEvent(Rml::Event& event)
	{
		Rocket_ProcessEvent( event, value );
	}

	/// Destroys the event.
	void OnDetach(Rml::Element*)
	{
		delete this;
	}

private:
	Rml::String value;
};

class EventInstancer : public Rml::EventListenerInstancer
{
public:
	EventInstancer() { }
	~EventInstancer() { }

	/// Instances a new event handle for the CGame
	Rml::EventListener* InstanceEventListener(const Rml::String& value, Rml::Element*)
	{
		return new RocketEvent( value );
	}

	/// Destroys the instancer.
	void Release() { delete this; }
};
#endif
