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

#ifndef ROCKETELEMENT_H
#define ROCKETELEMENT_H

#include <Rocket/Core/Element.h>
#include <Rocket/Core/ElementInstancer.h>
#include <Rocket/Core/Geometry.h>
#include <Rocket/Core/GeometryUtilities.h>
#include <Rocket/Core/Texture.h>

#include <queue>
#include "../cg_local.h"
#include "rocket.h"

Rocket::Core::Element *activeElement = nullptr;

class RocketElement : public Rocket::Core::Element
{
public:
	RocketElement( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ) { }
	~RocketElement() { }

	bool GetIntrinsicDimensions( Rocket::Core::Vector2f &dimension )
	{
		const Rocket::Core::Property *property;
		property = GetProperty( "width" );

		// Absolute unit. We can use it as is
		if ( property->unit & Rocket::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.x = property->value.Get<float>();
		}
		else
		{
			Rocket::Core::Element *parent = GetParentNode();
			if ( parent != nullptr )
			{
				dimensions.x = ResolveProperty( "width", parent->GetBox().GetSize().x );
			}
		}

		property = GetProperty( "height" );
		if ( property->unit & Rocket::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.y = property->value.Get<float>();
		}
		else
		{
			Rocket::Core::Element *parent = GetParentNode();
			if ( parent != nullptr )
			{
				dimensions.y = ResolveProperty( "height", parent->GetBox().GetSize().y );
			}
		}

		// Return the calculated dimensions. If this changes the size of the element, it will result in
		// a 'resize' event which is caught below and will regenerate the geometry.

		dimension = dimensions;
		return true;
	}

	void ProcessEvent( Rocket::Core::Event &event )
	{
		extern std::queue< RocketEvent_t* > eventQueue;

		// Class base class's Event processor
		Rocket::Core::Element::ProcessEvent( event );


		// Let this be picked up in the event loop if it is meant for us
		// HACK: Ignore mouse and resize events
		if ( event.GetTargetElement() == this && !Q_stristr( event.GetType().CString(), "mouse" ) && !Q_stristr( event.GetType().CString(), "resize" ) )
		{
			eventQueue.push( new RocketEvent_t( event, event.GetType() ) );
		}
	}

	void SetDimensions( float x, float y )
	{
		dimensions.x = x;
		dimensions.y = y;
	}

	void OnRender()
	{
		activeElement = this;

		CG_Rocket_RenderElement( GetTagName().CString() );

		// Render text on top
		Rocket::Core::Element::OnRender();
	}


	Rocket::Core::Vector2f dimensions;
};

class RocketElementInstancer : public Rocket::Core::ElementInstancer
{
public:
	RocketElementInstancer() { }
	~RocketElementInstancer() { }
	Rocket::Core::Element *InstanceElement( Rocket::Core::Element*,
											const Rocket::Core::String &tag,
											const Rocket::Core::XMLAttributes& )
	{
		return new RocketElement( tag );
	}

	void ReleaseElement( Rocket::Core::Element *element )
	{
		delete element;
	}

	void Release()
	{
		delete this;
	}
};
#endif
