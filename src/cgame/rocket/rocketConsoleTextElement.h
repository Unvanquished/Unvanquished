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
#include <Rocket/Core/FontFaceHandle.h>
#include "../cg_local.h"
#include "rocket.h"

struct ConsoleLine
{
	Rocket::Core::String text;
	int time;
	ConsoleLine( Rocket::Core::String _i ) : text( _i )
	{
		time = trap_Milliseconds();
	}
};

class RocketConsoleTextElement : public Rocket::Core::Element
{
public:
	RocketConsoleTextElement( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ), numLines( 0 ), maxLines( 0 ), lastTime( -1 ),
	dirty_height( true )
	{
	}

	void OnUpdate()
	{
		// Clean up old elements
		int latency = trap_Cvar_VariableIntegerValue( "cg_consoleLatency" );
		int time = trap_Milliseconds();

		while ( !lines.empty() && lines.back().time + latency < time )
		{
			lines.pop_back();
		}

		while ( HasChildNodes() && atoi( GetFirstChild()->GetId().CString() ) + latency < time )
		{
			RemoveChild( GetFirstChild() );
			numLines--;
		}

		if ( !lines.empty() && lines.front().time > lastTime )
		{
			int line = 0;

			lastTime = lines[ line ].time;

			// Find out how many lines
			while ( line < (int) lines.size() && lines[ line ].time >= lastTime )
			{
				line++;
			}

			// Each line gets its own span element
			for (line = line - 1; line >= 0; --line, numLines++ )
			{
				Rocket::Core::Element *child = Rocket::Core::Factory::InstanceElement( this, "#text", "span", Rocket::Core::XMLAttributes() );
				Rocket::Core::Factory::InstanceElementText( child, Rocket_QuakeToRML( lines[ line ].text.CString(), RP_EMOTICONS ));
				child->SetId( va( "%d", lines[ line ].time ) );
				AppendChild( child );
				child->RemoveReference();
			}
		}

		// Calculate max lines when we have a child element with a fontface
		if ( dirty_height && HasChildNodes() )
		{
			const Rocket::Core::FontFaceHandle *font = GetFirstChild()->GetFontFaceHandle();
			if ( font )
			{
				maxLines = floor( GetProperty( "height" )->value.Get<float>() / ( font->GetBaseline() + font->GetLineHeight() ) );

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


	static std::deque<ConsoleLine> lines;
private:
	int numLines;
	int maxLines;
	int lastTime;
	bool dirty_height;
};
#endif


