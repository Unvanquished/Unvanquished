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

#ifndef ROCKET_H
#define ROCKET_H
#ifdef DotProduct
// Ugly hack to fix the DotProduct conflict
#undef DotProduct
#endif

#include "common/Compiler.h"
#include "common/Color.h"
#include "common/String.h"

#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
#include <Rocket/Core/Core.h>
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

extern Rocket::Core::Context *menuContext;
extern Rocket::Core::Context *hudContext;

class RocketEvent_t
{
public:
	RocketEvent_t( Rocket::Core::Event &event, const Rocket::Core::String &cmds ) : cmd( cmds )
	{
		targetElement = event.GetTargetElement();
		Parameters = *(event.GetParameters());
	}
	RocketEvent_t( const Rocket::Core::String &cmds ) : RocketEvent_t( nullptr, cmds )
	{
	}
	RocketEvent_t( Rocket::Core::Element *e, const Rocket::Core::String &cmds ) : targetElement( e ), cmd( cmds )
	{
	}
	~RocketEvent_t() { }
	Rocket::Core::Element *targetElement;
	Rocket::Core::Dictionary Parameters;
	Rocket::Core::String cmd;
};

// HTML-escape a string that will be used as text. Text meaning that it will be not be
// somewhere weird like inside a tag, which may require a different form of escaping.
std::string CG_EscapeHTMLText( Str::StringRef text );

Rocket::Core::String Rocket_QuakeToRML( const char *in, int parseFlags );
std::string CG_KeyBinding( const char *bind, int team );

void Rocket_AddEvent( RocketEvent_t *event );

namespace Color {

template<class ColourType, int AlphaDefault>
class ColorAdaptor<Rocket::Core::Colour<ColourType,AlphaDefault>>
{
public:
	static constexpr bool is_color = true;
	using color_type = Rocket::Core::Colour<ColourType,AlphaDefault>;
	using component_type = ColourType;
	static constexpr int component_max = AlphaDefault;

	static ColorAdaptor Adapt( const color_type& object )
	{
		return ColorAdaptor( object );
	}

	ColorAdaptor( const color_type& object ) : object( object ) {}

	component_type Red() const { return object.red; }
	component_type Green() const { return object.green; }
	component_type Blue() const { return object.blue; }
	component_type Alpha() const { return object.alpha; }

private:
	color_type object;
};
} // namespace Color

#endif
