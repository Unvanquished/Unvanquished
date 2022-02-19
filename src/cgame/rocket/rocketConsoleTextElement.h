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

#ifndef ROCKETCONSOLETEXTELEMENT_H
#define ROCKETCONSOLETEXTELEMENT_H

#include <RmlUi/Core.h>
#include <RmlUi/Core/Element.h>
#include "../cg_local.h"
#include "rocket.h"

struct ConsoleLine
{
	Rml::Core::String text;
	int time;
	ConsoleLine( Rml::Core::String _i ) : text( _i )
	{
		time = trap_Milliseconds();
	}
};

class RocketConsoleTextElement : public Rml::Core::Element
{
public:
	RocketConsoleTextElement( const Rml::Core::String &tag ) : Rml::Core::Element( tag ), numLines( 0 ), maxLines( 0 ), lastTime( -1 ),
	dirty_height( true ), font_engine_interface( Rml::Core::GetFontEngineInterface() )
	{
	}

	void OnUpdate()
	{
		// Clean up old elements
		int latency = cg_consoleLatency.Get();
		int time = trap_Milliseconds();

		while ( !lines.empty() && lines.back().time + latency < time )
		{
			lines.pop_back();
		}

		while ( HasChildNodes() && atoi( GetFirstChild()->GetId().c_str() ) + latency < time )
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
				Rml::Core::ElementPtr childPtr = Rml::Core::Factory::InstanceElement( this, "#text", "span", Rml::Core::XMLAttributes() );
				Rml::Core::Factory::InstanceElementText( childPtr.get(), Rocket_QuakeToRML( lines[ line ].text.c_str(), RP_EMOTICONS ));
				childPtr->SetId( va( "%d", lines[ line ].time ) );
				AppendChild( std::move( childPtr ) );
			}
		}

		// Calculate max lines when we have a child element with a fontface
		if ( dirty_height && HasChildNodes() )
		{
			const Rml::Core::FontFaceHandle font = GetFirstChild()->GetFontFaceHandle();
			if ( font )
			{
				maxLines = floor( GetProperty( "height" )->value.Get<float>() / ( font_engine_interface->GetBaseline( font ) + font_engine_interface->GetLineHeight( font ) ) );

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

		Rml::Core::Element::OnUpdate();
	}

	void OnPropertyChange( const Rml::Core::PropertyIdSet &changed_properties )
	{
		if ( changed_properties.Contains( Rml::Core::PropertyId::Height ) )
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
	Rml::Core::FontEngineInterface* const font_engine_interface;
};
#endif


