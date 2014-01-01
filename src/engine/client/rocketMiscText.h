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


#ifndef ROCKETMISCTEXT_H
#define ROCKETMISCTEXT_H

#include <Rocket/Core.h>
#include "client.h"

struct TextAndRect
{
	TextAndRect( const char *_text, const char *_class, float _x, float _y ) : text( _text ), text_class( _class ), x( _x ), y( _y ) { }
	Rocket::Core::String text;
	Rocket::Core::String text_class;
	float x;
	float y;
};

// A libRocket element to hold other misc text on the screen. It is assumed that it will change every frame

class RocketMiscText : public Rocket::Core::Element
{
public:
	RocketMiscText( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag )
	{
	}

	static void ClearText( void )
	{
		strings.clear();
	}

	static void AddText( const char *text, const char *text_class, float x, float y )
	{
		strings.push_back( TextAndRect( text, text_class, x, y ) );
	}

	void OnUpdate( void )
	{
		while ( HasChildNodes() )
		{

			RemoveChild( GetFirstChild() );
		}

		for ( size_t i = 0; i < strings.size(); ++i )
		{
// 			Rocket::Core::ElementText* element;
// 			element = GetOwnerDocument()->CreateTextNode( strings[ i ].text );
// 			element->SetProperty( "position", "fixed" );
// 			element->SetProperty( "left", va( "%.2f%%", strings[ i ].x * 100.0f ) );
// 			element->SetProperty( "top", va( "%.2f%%", strings[ i ].y * 100.0f ) );
// 			element->SetClassNames( strings [ i ].text_class );
//
// 			AppendChild( element );
// 			element->RemoveReference();
			Rocket::Core::Factory::InstanceElementText( this, va( "<div style='position: fixed; left: %.2f%%; top: %.2f%%;' class='%s'>%s</div>", strings[ i ].x * 100.0f, strings[ i ].y * 100.0f, strings[ i ].text_class.CString(), strings[ i ].text.CString() ) );
		}
	}

private:
	static std::vector<TextAndRect> strings;
	Rocket::Core::Element *base_element;
};
#endif
