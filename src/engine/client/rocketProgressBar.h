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
#include <Rocket/Core/ContainerWrapper.h>
#include <Rocket/Core/GeometryUtilities.h>

extern "C"
{
#include "client.h"
}

enum
{
	START,
	PROGRESS,
	END,
	DECORATION,
	NUM_GEOMETRIES
};

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
	RocketProgressBar( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ), dirty_geometry( true ), orientation( LEFT ), value( 0.0f ), renderFilter( 0 ), loadFilter( 0 )
	{
		for ( int i = START; i < NUM_GEOMETRIES; ++i )
		{
			geometry[ i ].SetHostElement( this );
			size[ i ] = Rocket::Core::Vector2f( 0, 0 );
		}
	}

	~RocketProgressBar() {}

	// Get current value
	float GetValue( void ) const
	{
		return GetAttribute<float>( "value", 0.0f );
	}

	void SetValue( const float value )
	{
		SetAttribute( "value", value );
	}


	void OnUpdate( void )
	{
		float newValue;
		const char *str = GetAttribute<Rocket::Core::String>( "src", "" ).CString();

		if ( *str )
		{
			Cmd_TokenizeString( str );
			newValue = _vmf( VM_Call( cgvm, CG_ROCKET_PROGRESSBARVALUE ) );

			if ( newValue != value )
			{
				SetValue( newValue );
			}
		}
	}

	void OnRender( void )
	{
		OnUpdate(); // When loading maps, Render() is called multiple times without updating.

		if ( dirty_geometry )
		{
			GenerateGeometry();
		}

		for ( int i; i < NUM_GEOMETRIES; ++i )
		{
			if ( renderFilter & 1 << i )
			{
				geometry[ i ].Render( GetAbsoluteOffset( Rocket::Core::Box::CONTENT ) );
			}
		}
	}

	void OnPropertyChange( const Rocket::Core::PropertyNameList &changed_properties )
	{
		Element::OnPropertyChange( changed_properties );

		if ( changed_properties.find( "start-image" ) != changed_properties.end() )
		{
			LoadTexture( START );
		}

		if ( changed_properties.find( "progress-image" ) != changed_properties.end() )
		{
			LoadTexture( PROGRESS );
		}

		if ( changed_properties.find( "decoration-image" ) != changed_properties.end() )
		{
			LoadTexture( DECORATION );
		}

		if ( changed_properties.find( "end-image" ) != changed_properties.end() )
		{
			LoadTexture( END );
		}

		if ( changed_properties.find( "orientation" ) != changed_properties.end() )
		{
			Rocket::Core::String  orientation_string = GetProperty<Rocket::Core::String>( "orientation" );

			if ( orientation_string == "right" )
			{
				orientation = RIGHT;
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
				orientation = LEFT;
			}

			dirty_geometry = true;
		}
	}

	void OnAttributeChange( const Rocket::Core::AttributeNameList &changed_attributes )
	{
		Rocket::Core::Element::OnAttributeChange( changed_attributes );

		if ( changed_attributes.find( "value" ) != changed_attributes.end() )
		{
			value = Com_Clamp( 0.0f, 1.0f, GetAttribute<float>( "value", 0.0f ) );

			dirty_geometry = true;
		}
	}

	bool GetIntrinsicDimensions( Rocket::Core::Vector2f &dimensions )
	{
		// Calculate the x dimension.
		if ( ( ( dimensions.x = GetProperty< float >( "width" ) ) == 0 ) )
		{
			dimensions.x = ( float ) GetParentNode()->GetBox( Rocket::Core::Box::CONTENT ).GetSize().x;
		}

		// Calculate the y dimension.
		if ( ( ( dimensions.y = GetProperty< float >( "height" ) ) == 0 ) )
		{
			dimensions.y = ( float ) GetParentNode()->GetBox( Rocket::Core::Box::CONTENT ).GetSize().y;
		}

		// Return the calculated dimensions. If this changes the size of the element, it will result in
		// a 'resize' event which is caught below and will regenerate the geometry.
		return true;
	}

private:
	bool LoadTexture( Rocket::Core::URL &source_url, int index, const char *property_name, Rocket::Core::Geometry &geometry, Rocket::Core::Vector2f &size )
	{
		Rocket::Core::RenderInterface *render_interface = GetRenderInterface();
		Rocket::Core::StringList words;
		Rocket::Core::String source = GetProperty< Rocket::Core::String >( property_name );
		Rocket::Core::StringUtilities::ExpandString( words, source, ' ' );
		bool it_uses_tex_coords = false;
		Rocket::Core::Texture &texture = textures[index];

		if ( words.size() == 5 )
		{
			it_uses_tex_coords = true;
			source = words[0];
			Rocket::Core::TypeConverter< Rocket::Core::String, float >::Convert( words[1], texcoords[index][0].x );
			Rocket::Core::TypeConverter< Rocket::Core::String, float >::Convert( words[2], texcoords[index][0].y );
			Rocket::Core::TypeConverter< Rocket::Core::String, float >::Convert( words[3], texcoords[index][1].x );
			Rocket::Core::TypeConverter< Rocket::Core::String, float >::Convert( words[4], texcoords[index][1].y );
		}

		if ( !texture.Load( source, source_url.GetPath() ) )
		{
			geometry.SetTexture( NULL );
			return false;
		}

		if ( it_uses_tex_coords )
		{
			Rocket::Core::Vector2i texture_dimensions = texture.GetDimensions( render_interface );

			size.x = texcoords[index][1].x - texcoords[index][0].x;
			size.y = texcoords[index][1].y - texcoords[index][0].y;

			for ( int i = 0; i < 2; i++ )
			{
				texcoords[index][i].x /= texture_dimensions.x;
				texcoords[index][i].y /= texture_dimensions.y;
			}
		}
		else
		{
			texcoords[index][0].x = 0.0f;
			texcoords[index][0].y = 0.0f;
			texcoords[index][1].x = 1.0f;
			texcoords[index][1].y = 1.0f;
			size.x = float( texture.GetDimensions( render_interface ).x );
			size.y = float( texture.GetDimensions( render_interface ).y );
		}

		geometry.SetTexture( &texture );
		return true;
	}

	void LoadTexture( int in )
	{
		Rocket::Core::ElementDocument *document = GetOwnerDocument();
		Rocket::Core::URL source_url( document == NULL ? "" : document->GetSourceURL() );
		Rocket::Core::String property_name;

		switch ( in )
		{
			case START:
				property_name = "start-image";
				break;

			case PROGRESS:
				property_name = "progress-image";
				break;

			case DECORATION:
				property_name = "decoration-image";
				break;

			case END:
				property_name = "end-image";
				break;
		}

		if ( LoadTexture( source_url, in, property_name.CString(), geometry[ in ], size[ in ] ) )
		{
			renderFilter |= 1 << in;
			loadFilter |= 1 << in;
		}
		else
		{
			renderFilter &= ~( 1 << in );
			loadFilter &= ~( 1 << in );
		}
	}

	void GenerateGeometry( void )
	{
		Rocket::Core::Vector2f geometryPosition[ NUM_GEOMETRIES ] = { Rocket::Core::Vector2f( 0, 0 ) };
		Rocket::Core::Vector2f geometrySize[ NUM_GEOMETRIES ] = size;

		Rocket::Core::Vector2f elementSize = GetBox().GetSize( Rocket::Core::Box::CONTENT );

		Rocket::Core::Vector2f totalSize( 0, 0 );

		float progress_size;

		switch ( orientation )
		{
			case LEFT:
				progress_size = elementSize.x * value;
				geometrySize[ PROGRESS ].x = ( elementSize.x - geometrySize[ START ].x - geometrySize[ END ].x ) * value;

				for ( int i = START; i < NUM_GEOMETRIES; ++i )
				{
					if ( i == DECORATION || !( loadFilter & 1<<i ) )
					{
						continue;
					}

					if ( i == START )
					{
						geometryPosition[0].x = geometryPosition[0].y = 0;
					}
					else
					{
						geometryPosition[i].x = geometryPosition[i-1].x + geometrySize[i-1].x;
					}

					if ( progress_size > geometrySize[i].x )
					{
						progress_size -= geometrySize[i].x;
						texcoords[i][0] = Rocket::Core::Vector2f( 0, 0 );
						texcoords[i][1] = Rocket::Core::Vector2f( 1, 1 );

						renderFilter |= 1 << i;
					}
					else
					{
						texcoords[i][0] = Rocket::Core::Vector2f( 0, 0 );
						texcoords[i][1] = Rocket::Core::Vector2f( progress_size / geometrySize[i].x, 1 );

						renderFilter |= 1 << i;

						for ( int j = i + 1; j <= END ; ++j )
						{
							renderFilter &= ~( 1 << j );
						}

						break;
					}
				}

				break;

			case RIGHT:
				progress_size = elementSize.x * value;
				geometrySize[ PROGRESS ].x = ( elementSize.x - geometrySize[ START ].x - geometrySize[ END ].x ) * value;

				for ( int i = START; i < NUM_GEOMETRIES; ++i )
				{
					if ( i == DECORATION || !( loadFilter & 1<<i ) )
					{
						continue;
					}

					if ( i == START )
					{
						geometryPosition[0].x = elementSize.x + geometrySize[i].x;
						geometryPosition[0].y = 0;
					}
					else
					{
						geometryPosition[i].x = geometryPosition[i-1].x - geometrySize[i].x;
					}

					if ( progress_size > geometrySize[i].x )
					{
						progress_size -= geometrySize[i].x;
						texcoords[i][0] = Rocket::Core::Vector2f( 0, 0 );
						texcoords[i][1] = Rocket::Core::Vector2f( 1, 1 );

						renderFilter |= 1 << i;
					}
					else
					{
						texcoords[i][0] = Rocket::Core::Vector2f( 1 - ( progress_size / geometrySize[i].x ), 0 );
						texcoords[i][1] = Rocket::Core::Vector2f( 1, 1 );

						renderFilter |= 1 << i;

						for ( int j = i + 1; j <= END ; ++j )
						{
							renderFilter &= ~( 1 << j );
						}

						break;
					}
				}
				break;

			case UP:
				break;

			case DOWN:
				break;
		}

		for ( int i = START; i < NUM_GEOMETRIES; ++i )
		{
			if ( renderFilter & ( 1 << i ) )
			{
				Rocket::Core::Container::vector< Rocket::Core::Vertex >::Type &verticies = geometry[ i ].GetVertices();
				Rocket::Core::Container::vector< int >::Type &indicies = geometry[ i ].GetIndices();

				verticies.resize( 4 );
				indicies.resize( 6 );


				Rocket::Core::GeometryUtilities::GenerateQuad( &verticies[0],
											 &indicies[0],
											 geometryPosition[i],
											 geometrySize[i],
											 Rocket::Core::Colourb( 255, 255, 255, 255 ),
											 texcoords[i][0],
											 texcoords[i][1]
										   );
			}
		}

		dirty_geometry = false;
	}

	Rocket::Core::Geometry geometry[ NUM_GEOMETRIES ]; // 1: start image, 2: straight section, 3: straight section decoration, 4: end image

	Rocket::Core::Vector2f texcoords[ NUM_GEOMETRIES ][ 2 ]; // texture coords
	Rocket::Core::Vector2f size[ NUM_GEOMETRIES ]; // size of images
	Rocket::Core::Vector2f dimensions; // dimensions of this element

	Rocket::Core::Texture textures[ NUM_GEOMETRIES ];


	int renderFilter; // Filter components to draw
	int loadFilter; // Images loaded

	bool dirty_geometry; // Rebuild geometry

	float value; // current value

	progressBarOrientation_t orientation; // Direction progressbar grows
};

#endif
