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

#ifndef ROCKETELEMENTDOCUMENT_H
#define ROCKETELEMENTDOCUMENT_H

#include <Rocket/Core.h>
#include <Rocket/Core/Element.h>
#include <Rocket/Core/ElementDocument.h>


class RocketElementDocument : public Rocket::Core::ElementDocument
{
public:
	RocketElementDocument( const Rocket::Core::String &tag ) : Rocket::Core::ElementDocument( tag ) { }

	void ProcessEvent( Rocket::Core::Event &event )
	{
		Rocket::Core::ElementDocument::ProcessEvent( event );

		if ( event == "keydown" )
		{
			Rocket::Core::Input::KeyIdentifier key = (Rocket::Core::Input::KeyIdentifier) event.GetParameter<int>( "key_identifier", 0 );

			if ( key == Rocket::Core::Input::KI_ESCAPE && !HasAttribute( "nohide" ) )
			{
				this->Hide();
			}
		}
	}
};

#endif
