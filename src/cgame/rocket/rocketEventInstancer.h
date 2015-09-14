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

#ifndef ROCKETEVENTINSTANCER_H
#define ROCKETEVENTINSTANCER_H
#include <Rocket/Core/EventListener.h>
#include <Rocket/Core/EventListenerInstancer.h>
#include "rocket.h"

void Rocket_ProcessEvent( Rocket::Core::Event&, Rocket::Core::String& );

class RocketEvent : public Rocket::Core::EventListener
{
public:
	RocketEvent(const Rocket::Core::String& value) : value( value ) { }
	~RocketEvent() { }

	/// Sends the event value through to the CGame's event processing system.
	void ProcessEvent(Rocket::Core::Event& event)
	{
		Rocket_ProcessEvent( event, value );
	}

	/// Destroys the event.
	void OnDetach(Rocket::Core::Element*)
	{
		delete this;
	}

private:
	Rocket::Core::String value;
};

class EventInstancer : public Rocket::Core::EventListenerInstancer
{
public:
	EventInstancer() { }
	~EventInstancer() { }

	/// Instances a new event handle for the CGame
	Rocket::Core::EventListener* InstanceEventListener(const Rocket::Core::String& value, Rocket::Core::Element*)
	{
		return new RocketEvent( value );
	}

	/// Destroys the instancer.
	void Release() { delete this; }
};
#endif
