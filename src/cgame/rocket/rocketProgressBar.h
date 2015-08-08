/*
 = =*=========================================================================

 Daemon GPL Source Code
 Copyright (C) 2013 Unvanquished Developers

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

#ifndef ROCKETPROGRESSBAR_H
#define ROCKETPROGRESSBAR_H

#include <Rocket/Core.h>
#include <Rocket/Core/Element.h>
#include <Rocket/Core/GeometryUtilities.h>
#include "../cg_local.h"
#include "rocket.h"

typedef enum
{
	LEFT,
	RIGHT,
	UP,
	DOWN
} progressBarOrientation_t;

class RocketProgressBar : public Rocket::Core::Element
{
public:
	RocketProgressBar( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ), orientation( LEFT ), value( 0.0f ), shader( 0 )
	{
		// Default to white
		for (unsigned i = 0; i < ARRAY_LEN( color ); ++i )
		{
			color[ i ] = 1.0f;
		}
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

		if ( !source.Empty() )
		{
			newValue = CG_Rocket_ProgressBarValue(source.CString());

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
		Rocket::Core::Vector2f position = GetAbsoluteOffset();

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
				trap_R_SetColor( nullptr );
			}
			else
			{
				trap_R_SetColor( color );
				trap_R_DrawStretchPic( position.x, position.y - height + dimensions.y, dimensions.x,
						       height, 0.0f, 1.0f - value, 1.0f, 1.0f, shader );
				trap_R_SetColor( nullptr );
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
				trap_R_SetColor( nullptr );
			}
			else
			{
				trap_R_SetColor( color );
				trap_R_DrawStretchPic( position.x - width + dimensions.x, position.y, width,
						       dimensions.y, 1.0f - value, 0.0f, 1.0f, 1.0f, shader );
				trap_R_SetColor( nullptr );
			}
		}
	}

	void OnPropertyChange( const Rocket::Core::PropertyNameList &changed_properties )
	{
		Element::OnPropertyChange( changed_properties );

		if ( changed_properties.find( "color" ) != changed_properties.end() )
		{
			Rocket::Core::Colourb tempColor = GetProperty( "color" )->Get<Rocket::Core::Colourb>();

			color[ 0 ] = tempColor.red / 255.0f;
			color[ 1 ] = tempColor.blue / 255.0f;
			color[ 2 ] = tempColor.green / 255.0f;
			color[ 3 ] = tempColor.alpha / 255.0f;
		}

		if ( changed_properties.find( "image" ) != changed_properties.end() )
		{
			Rocket::Core::String image = GetProperty<Rocket::Core::String>( "image" );

			// skip the leading slash
			if ( !image.Empty() && image[0] == '/' )
			{
				image = image.Substring( 1 );
			}

			shader = trap_R_RegisterShader( image.CString(), RSF_NOMIP );
		}

		if ( changed_properties.find( "orientation" ) != changed_properties.end() )
		{
			Rocket::Core::String  orientation_string = GetProperty<Rocket::Core::String>( "orientation" );

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

	void OnAttributeChange( const Rocket::Core::AttributeNameList &changed_attributes )
	{
		Rocket::Core::Element::OnAttributeChange( changed_attributes );

		if ( changed_attributes.find( "value" ) != changed_attributes.end() )
		{
			value = Com_Clamp( 0.0f, 1.0f, GetAttribute<float>( "value", 0.0f ) );
		}

		if ( changed_attributes.find( "src" ) != changed_attributes.end() )
		{
			source = GetAttribute<Rocket::Core::String>( "src", "" );
		}
	}

	bool GetIntrinsicDimensions( Rocket::Core::Vector2f &dimension )
	{
		const Rocket::Core::Property *property;
		property = GetProperty( "width" );
		bool auto_width, auto_height;

		auto_width = auto_height = false;

		// Keyword means its auto
		if ( property->unit == Rocket::Core::Property::KEYWORD )
		{
			auto_width = true;
		}
		// Absolute unit. We can use it as is
		else if ( property->unit & Rocket::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.x = property->value.Get<float>();
		}
		else
		{
			float base_size = 0;
			Rocket::Core::Element *parent = this;
			std::stack<Rocket::Core::Element*> stack;
			stack.push( this );

			while ( ( parent = parent->GetParentNode() ) )
			{
				if ( ( base_size = parent->GetOffsetWidth() ) != 0 )
				{
					dimensions.x = base_size;
					while ( !stack.empty() )
					{
						dimensions.x = stack.top()->ResolveProperty( "width", dimensions.x );

						stack.pop();
					}
					break;
				}

				stack.push( parent );
			}
		}

		property = GetProperty( "height" );
		if ( property->unit == Rocket::Core::Property::KEYWORD )
		{
			auto_height = true;
		}
		else if ( property->unit & Rocket::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.y = property->value.Get<float>();
		}
		else
		{
			float base_size = 0;
			Rocket::Core::Element *parent = this;
			std::stack<Rocket::Core::Element*> stack;
			stack.push( this );

			while ( ( parent = parent->GetParentNode() ) )
			{
				if ( ( base_size = parent->GetOffsetHeight() ) != 0 )
				{
					dimensions.y = base_size;
					while ( !stack.empty() )
					{
						dimensions.y = stack.top()->ResolveProperty( "height", dimensions.y );

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
	vec4_t color;
	Rocket::Core::Vector2f dimensions;
	Rocket::Core::String source;
};
#endif
