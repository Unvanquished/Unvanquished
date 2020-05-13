/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2013 Unvanquished Developers

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

#ifndef ROCKETPROGRESSBAR_H
#define ROCKETPROGRESSBAR_H

#include <RmlUi/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/GeometryUtilities.h>
#include "../cg_local.h"
#include "rocket.h"

enum progressBarOrientation_t
{
	LEFT,
	RIGHT,
	UP,
	DOWN
};

class RocketProgressBar : public Rml::Core::Element
{
public:
	RocketProgressBar( const Rml::Core::String &tag ) :
		Rml::Core::Element( tag ), orientation( LEFT ), value( 0.0f ), shader( 0 ), color( Color::White )
	{
	}

	~RocketProgressBar() {}

	// Get current value
	float GetValue() const
	{
		return value;
	}

	void SetValue( const float value )
	{
		SetAttribute( "value", value );
	}


	void Update()
	{
		float newValue;

		if ( !source.empty() )
		{
			newValue = CG_Rocket_ProgressBarValue(source.c_str());

			if ( newValue != value )
			{
				SetValue( newValue );
			}
		}
	}

	void OnRender()
	{

		if ( !shader )
		{
			return;
		}

		Update();
		Rml::Core::Vector2f position = GetAbsoluteOffset();

		// Vertical meter
		if( orientation >= UP )
		{
			float height;
			height = dimensions.y * value;

			// Meter decreases down
			if ( orientation == DOWN )
			{
				trap_R_SetColor( color );
				trap_R_DrawStretchPic( position.x, position.y, dimensions.x, height,
						       0.0f, 0.0f, 1.0f, value, shader );
				trap_R_ClearColor();
			}
			else
			{
				trap_R_SetColor( color );
				trap_R_DrawStretchPic( position.x, position.y - height + dimensions.y, dimensions.x,
						       height, 0.0f, 1.0f - value, 1.0f, 1.0f, shader );
				trap_R_ClearColor();
			}
		}

		// Horizontal meter
		else
		{
			float width;

			width = dimensions.x * value;

			// Meter decreases to the left
			if ( orientation == LEFT )
			{
				trap_R_SetColor( color );
				trap_R_DrawStretchPic( position.x, position.y, width,
						       dimensions.y, 0.0f, 0.0f, value, 1.0f, shader );
				trap_R_ClearColor();
			}
			else
			{
				trap_R_SetColor( color );
				trap_R_DrawStretchPic( position.x - width + dimensions.x, position.y, width,
						       dimensions.y, 1.0f - value, 0.0f, 1.0f, 1.0f, shader );
				trap_R_ClearColor();
			}
		}
	}

	void OnPropertyChange( const Rml::Core::PropertyIdSet &changed_properties )
	{
		Element::OnPropertyChange( changed_properties );

		if ( changed_properties.Contains( Rml::Core::PropertyId::Color ) )
		{
			color = Color::Adapt( GetProperty( Rml::Core::PropertyId::Color )->Get<Rml::Core::Colourb>() );
		}

		if ( changed_properties.Contains( UnvPropertyId::Orientation ) )
		{
			Rml::Core::String  orientation_string = GetProperty( UnvPropertyId::Orientation )->Get<Rml::Core::String>();

			if ( orientation_string == "left" )
			{
				orientation = LEFT;
			}
			else if ( orientation_string == "up" )
			{
				orientation = UP;
			}
			else if ( orientation_string == "down" )
			{
				orientation = DOWN;
			}
			else
			{
				orientation = RIGHT;
			}
		}
	}

	void OnAttributeChange( const Rml::Core::ElementAttributes &changed_attributes )
	{
		Rml::Core::Element::OnAttributeChange( changed_attributes );

		auto it = changed_attributes.find( "value" );
		if ( it != changed_attributes.end() )
		{
			value = Com_Clamp( 0.0f, 1.0f, it->second.Get<float>() );
		}

		it = changed_attributes.find( "src" );
		if ( it != changed_attributes.end() )
		{
			source = it->second.Get<Rml::Core::String>();
		}

		it = changed_attributes.find( "image" );
		if ( it != changed_attributes.end() )
		{
			Rml::Core::String image = it->second.Get<Rml::Core::String>();

			// skip the leading slash
			if ( !image.empty() && image[0] == '/' )
			{
				image = image.substr( 1 );
			}

			shader = trap_R_RegisterShader( image.c_str(), RSF_NOMIP );
		}

	}

	bool GetIntrinsicDimensions( Rml::Core::Vector2f &dimension )
	{
		const Rml::Core::Property *property;
		property = GetProperty( "width" );
		bool auto_width, auto_height;

		auto_width = auto_height = false;

		// Keyword means its auto
		if ( property->unit == Rml::Core::Property::KEYWORD )
		{
			auto_width = true;
		}
		// Absolute unit. We can use it as is
		else if ( property->unit & Rml::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.x = property->value.Get<float>();
		}
		else
		{
			float base_size = 0;
			Rml::Core::Element *parent = this;
			std::stack<Rml::Core::Element*> stack;
			stack.push( this );

			while ( ( parent = parent->GetParentNode() ) )
			{
				if ( ( base_size = parent->GetOffsetWidth() ) != 0 )
				{
					dimensions.x = base_size;
					while ( !stack.empty() )
					{
						dimensions.x = stack.top()->ResolveNumericProperty( "width" );

						stack.pop();
					}
					break;
				}

				stack.push( parent );
			}
		}

		property = GetProperty( "height" );
		if ( property->unit == Rml::Core::Property::KEYWORD )
		{
			auto_height = true;
		}
		else if ( property->unit & Rml::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.y = property->value.Get<float>();
		}
		else
		{
			float base_size = 0;
			Rml::Core::Element *parent = this;
			std::stack<Rml::Core::Element*> stack;
			stack.push( this );

			while ( ( parent = parent->GetParentNode() ) )
			{
				if ( ( base_size = parent->GetOffsetHeight() ) != 0 )
				{
					dimensions.y = base_size;
					while ( !stack.empty() )
					{
						dimensions.y = stack.top()->ResolveNumericProperty( "height" );

						stack.pop();
					}
					break;
				}

				stack.push( parent );
			}
		}

		if ( shader && ( auto_height || auto_width ) )
		{
			int x, y;
			trap_R_GetTextureSize( shader, &x, &y );

			if ( auto_height && !auto_width )
			{
				dimensions.y = ( dimensions.x / x ) * y;
			}
			else if ( !auto_height && auto_width )
			{
				dimensions.x = ( dimensions.y / y ) * x;
			}
			else
			{
				dimensions.x = x;
				dimensions.y = y;
			}
		}
		// Return the calculated dimensions. If this changes the size of the element, it will result in
		// a 'resize' event which is caught below and will regenerate the geometry.

		dimension = dimensions;

		return true;
	}

private:
	progressBarOrientation_t orientation; // Direction progressbar grows
	float value; // current value
	qhandle_t shader;
	Color::Color color;
	Rml::Core::Vector2f dimensions;
	Rml::Core::String source;
};
#endif
