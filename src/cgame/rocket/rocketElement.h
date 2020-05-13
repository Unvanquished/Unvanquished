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

#ifndef ROCKETELEMENT_H
#define ROCKETELEMENT_H

#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementInstancer.h>
#include <RmlUi/Core/Geometry.h>
#include <RmlUi/Core/GeometryUtilities.h>
#include <RmlUi/Core/Texture.h>

#include <queue>
#include "../cg_local.h"
#include "rocket.h"

Rml::Core::Element *activeElement = nullptr;

class RocketElement : public Rml::Core::Element
{
public:
	RocketElement( const Rml::Core::String &tag ) : Rml::Core::Element( tag ) { }
	~RocketElement() { }

	bool GetIntrinsicDimensions( Rml::Core::Vector2f &dimension ) override
	{
		const Rml::Core::Property *property;
		property = GetProperty( "width" );

		// Absolute unit. We can use it as is
		if ( property->unit & Rml::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.x = property->value.Get<float>();
		}
		else
		{
			Rml::Core::Element *parent = GetParentNode();
			if ( parent != nullptr )
			{
				dimensions.x = ResolveNumericProperty( "width" );
			}
		}

		property = GetProperty( "height" );
		if ( property->unit & Rml::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.y = property->value.Get<float>();
		}
		else
		{
			Rml::Core::Element *parent = GetParentNode();
			if ( parent != nullptr )
			{
				dimensions.y = ResolveNumericProperty( "height" );
			}
		}

		// Return the calculated dimensions. If this changes the size of the element, it will result in
		// a 'resize' event which is caught below and will regenerate the geometry.

		dimension = dimensions;
		return true;
	}

	void ProcessDefaultAction( Rml::Core::Event &event ) override
	{
		extern std::queue< RocketEvent_t* > eventQueue;

		// Class base class's Event processor
		Rml::Core::Element::ProcessDefaultAction( event );


		// Let this be picked up in the event loop if it is meant for us
		// HACK: Ignore mouse and resize events
		if ( event.GetTargetElement() == this && !Q_stristr( event.GetType().c_str(), "mouse" ) && !Q_stristr( event.GetType().c_str(), "resize" ) )
		{
			eventQueue.push( new RocketEvent_t( event, event.GetType() ) );
		}
	}

	void SetDimensions( float x, float y )
	{
		dimensions.x = x;
		dimensions.y = y;
	}

	void OnRender() override
	{
		activeElement = this;

		CG_Rocket_RenderElement( GetTagName().c_str() );

		// Render text on top
		Rml::Core::Element::OnRender();
	}


	Rml::Core::Vector2f dimensions;
};

#endif
