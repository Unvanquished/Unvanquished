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

extern "C"
{
#include "client.h"
}

Rocket::Core::Element *activeElement = NULL;

class RocketElement : public Rocket::Core::Element
{
public:
	RocketElement( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ) { }
	~RocketElement() { }

	bool GetIntrinsicDimensions( Rocket::Core::Vector2f &dimensions )
	{
		activeElement = this;
		VM_Call( cgvm, CG_ROCKET_GETELEMENTDIMENSIONS );
		dimensions = this->dimensions;
		return true;
	}

	void ProcessEvent( Rocket::Core::Event &event )
	{
		extern std::queue< RocketEvent_t* > eventQueue;

		// Class base class's Event processor
		Rocket::Core::Element::ProcessEvent( event );

		// Let this be picked up in the event loop
		eventQueue.push( new RocketEvent_t( event, event.GetType() ) );
	}

	void SetDimensions( float x, float y )
	{
		dimensions.x = x;
		dimensions.y = y;
	}

	void OnRender( void )
	{
		activeElement = this;

		VM_Call( cgvm, CG_ROCKET_RENDERELEMENT );

		for ( int i = 0; i < geometryList.size(); ++i )
		{
			geometryList[i].Render( GetAbsoluteOffset( Rocket::Core::Box::CONTENT ) );
		}

		// Render text on top
		Rocket::Core::Element::OnRender();
	}

	void OnAttributeChange( const Rocket::Core::AttributeNameList &changed_attributes )
	{
		Rocket::Core::Element::OnAttributeChange( changed_attributes );
	}

	void OnPropertyChange( const Rocket::Core::PropertyNameList &changed_properties )
	{
		Rocket::Core::Element::OnPropertyChange( changed_properties );
	}

	void DrawPic( float x, float y, float w, float h, float t1, float s1, float t2, float s2, const char *src )
	{
		geometryList.push_back( Rocket::Core::Geometry( this ) );

		std::vector< Rocket::Core::Vertex> &vertices = geometryList.back().GetVertices();
		std::vector< int > &indices = geometryList.back().GetIndices();

		vertices.resize( 4 );
		indices.resize( 6 );

		Rocket::Core::GeometryUtilities::GenerateQuad( &vertices[0], &indices[0],

													   Rocket::Core::Vector2f( x, y ),
													   Rocket::Core::Vector2f( w, h ),
													   Rocket::Core::Colourb( 255, 255, 255, 255 ),
													   Rocket::Core::Vector2f( t1, s1 ),
													   Rocket::Core::Vector2f( t2, s2 ) );

		textureList.push_back( Rocket::Core::Texture() );
		textureList.back().Load( src );

		geometryList.back().SetTexture( &textureList.back() );
	}

	void ClearGeometry( void )
	{
		for ( int i = 0; i < geometryList.size(); ++i )
		{
			geometryList[ i ].Release();
		}

		geometryList.clear();
		textureList.clear();
	}

	Rocket::Core::Vector2f dimensions;
private:
	bool dirty_geometry;
	std::vector<Rocket::Core::Geometry> geometryList;
	std::vector<Rocket::Core::Texture> textureList;
};

class RocketElementInstancer : public Rocket::Core::ElementInstancer
{
public:
	RocketElementInstancer() { }
	~RocketElementInstancer() { }
	Rocket::Core::Element *InstanceElement( Rocket::Core::Element *parent,
											const Rocket::Core::String &tag,
											const Rocket::Core::XMLAttributes &attributes )
	{
		return new RocketElement( tag );
	}

	void ReleaseElement( Rocket::Core::Element *element )
	{
		delete element;
	}

	void Release( void )
	{
		delete this;
	}
};
#endif
