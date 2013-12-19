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

#ifndef ROCKETCONSOLETEXTELEMENT_H
#define ROCKETCONSOLETEXTELEMENT_H

#include <Rocket/Core.h>
#include <Rocket/Core/Element.h>
#include "client.h"
#include "rocket.h"

struct ConsoleLine
{
	Rocket::Core::String text;
	int time;
	ConsoleLine( Rocket::Core::String _i ) : text( _i )
	{
		time = Sys_Milliseconds();
	}
};

class RocketConsoleTextElement : public Rocket::Core::Element
{
public:
	RocketConsoleTextElement( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ), numLines( 0 ), maxLines( 0 ), lastTime( -1 ),
	dirty_height( true )
	{
	}

	void OnUpdate( void )
	{
		// Clean up old elements
		int latency = Cvar_VariableIntegerValue( "cg_consoleLatency" );
		int time = Sys_Milliseconds();

		while ( !lines.empty() && lines.back().time + latency < time )
		{
			lines.pop_back();
		}

		while ( GetNumChildren() && atoi( GetFirstChild()->GetId().CString() ) + latency < time )
		{
			RemoveChild( GetFirstChild() );
			numLines--;
		}

		if ( !lines.empty() && lines.front().time > lastTime )
		{
			int line = 0;

			lastTime = lines[ line ].time;

			// Find out how many lines
			while ( lines[ line ].time > lastTime )
			{
				line++;
			}

			// Each line gets its own span element
			for (; line >= 0; --line, numLines++ )
			{
				Rocket::Core::Element *child = Rocket::Core::Factory::InstanceElement( this, "#text", "span", Rocket::Core::XMLAttributes() );
				q2rml( lines[ line ].text.CString(), child );
				child->SetId( va( "%d", lines[ line ].time ) );
				AppendChild( child );
			}
		}

		// Calculate max lines when we have a child element with a fontface
		if ( dirty_height && GetNumChildren() )
		{
			const Rocket::Core::FontFaceHandle *font = GetFirstChild()->GetFontFaceHandle();
			if ( font )
			{
				maxLines = floor( GetProperty( "height" )->value.Get<float>() / ( 10.0f ) );

				if ( maxLines <= 0 )
				{
					maxLines = 4; // conservatively low number
				}

				dirty_height = false;
			}
		}

		while ( maxLines < numLines )
		{
			RemoveChild( GetFirstChild() );
			numLines--;
		}

		Rocket::Core::Element::OnUpdate();
	}

	void OnPropertyChange( const Rocket::Core::PropertyNameList &changed_properties )
	{
		if ( changed_properties.find( "height" ) != changed_properties.end() )
		{
			int fontPt = GetProperty<int>( "font-size" );
			maxLines = GetProperty<int>("height") / ( fontPt > 0 ? fontPt : 1 );
		}
	}


	void q2rml( const char *in, Rocket::Core::Element *parent )
	{
		const char *p;
		Rocket::Core::String out;
		Rocket::Core::Element *child;
		qboolean span = qfalse;

		if ( !*in )
		{
			return;
		}

		for ( p = in; p && *p; ++p )
		{
			if ( *p == '<' )
			{
				out.Append( "&lt;" );
			}

			else if ( *p == '>' )
			{
				out.Append( "&gt;" );
			}

			else if ( *p == '&' )
			{
				out.Append( "&amp;" );
			}

			else if ( *p == '\n' )
			{
				// Child element initialized.
				if ( span )
				{
					span = qfalse;
				}
				// If not intialized, probably the first one, and should be white.
				else
				{
					Rocket::Core::XMLAttributes xml;
					child = Rocket::Core::Factory::InstanceElement( parent, "#text", "span", xml );
					child->SetProperty( "color", "#FFFFFF" );
				}
				static_cast<Rocket::Core::ElementText *>( child )->SetText( out );
				parent->AppendChild( child );
				parent->AppendChild( Rocket::Core::Factory::InstanceElement( parent, "*", "br", Rocket::Core::XMLAttributes() ) );
				out.Clear();
			}
			// Convert ^^ to ^
			else if ( *p == '^' && *( p + 1 ) == '^' )
			{
				p++;
				out.Append( "^" );
			}
			else if ( Q_IsColorString( p ) )
			{
				Rocket::Core::XMLAttributes xml;
				int code = ColorIndex( *++p );

				// Child element initialized
				if ( span )
				{
					span = qfalse;
					static_cast<Rocket::Core::ElementText *>( child )->SetText( out );
					parent->AppendChild( child );
					out.Clear();
				}
				// If not intialized, probably the first one, and should be white.
				else if ( !out.Empty() )
				{
					Rocket::Core::XMLAttributes xml;
					child = Rocket::Core::Factory::InstanceElement( parent, "#text", "span", xml );
					child->SetProperty( "color", "#FFFFFF" );
					static_cast<Rocket::Core::ElementText *>( child )->SetText( out );
					parent->AppendChild( child );
					out.Clear();
				}


				child = Rocket::Core::Factory::InstanceElement( parent, "#text", "span", xml );
				child->SetProperty( "color", va( "#%02X%02X%02X",
				                                 ( int )( g_color_table[ code ][ 0 ] * 255 ),
				                                 ( int )( g_color_table[ code ][ 1 ] * 255 ),
				                                 ( int )( g_color_table[ code ][ 2 ] * 255 ) ) );

				span = qtrue;
			}

			else
			{
				out.Append( *p );
			}
		}

		if ( span && child && !out.Empty() )
		{
			static_cast<Rocket::Core::ElementText *>( child )->SetText( out );
			parent->AppendChild( child );
			span = qfalse;
		}
	}

	static std::deque<ConsoleLine> lines;
private:
	int numLines;
	int maxLines;
	int lastTime;
	bool dirty_height;
};
#endif


