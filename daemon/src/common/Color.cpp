/*
===========================================================================

Daemon GPL Source Code

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

#include "Color.h"

#include <algorithm>

namespace Color {

namespace Named {
Color Black    = { 0.00, 0.00, 0.00, 1.00 };
Color Red      = { 1.00, 0.00, 0.00, 1.00 };
Color Green    = { 0.00, 1.00, 0.00, 1.00 };
Color Blue     = { 0.00, 0.00, 1.00, 1.00 };
Color Yellow   = { 1.00, 1.00, 0.00, 1.00 };
Color Orange   = { 1.00, 0.50, 0.00, 1.00 };
Color Magenta  = { 1.00, 0.00, 1.00, 1.00 };
Color Cyan     = { 0.00, 1.00, 1.00, 1.00 };
Color White    = { 1.00, 1.00, 1.00, 1.00 };
Color LtGrey   = { 0.75, 0.75, 0.75, 1.00 };
Color MdGrey   = { 0.50, 0.50, 0.50, 1.00 };
Color DkGrey   = { 0.25, 0.25, 0.25, 1.00 };
Color MdRed    = { 0.50, 0.00, 0.00, 1.00 };
Color MdGreen  = { 0.00, 0.50, 0.00, 1.00 };
Color DkGreen  = { 0.00, 0.20, 0.00, 1.00 };
Color MdCyan   = { 0.00, 0.50, 0.50, 1.00 };
Color MdYellow = { 0.50, 0.50, 0.00, 1.00 };
Color MdOrange = { 0.50, 0.25, 0.00, 1.00 };
Color LtOrange = { 0.50, 0.25, 0.00, 1.00 };
Color MdBlue   = { 0.00, 0.00, 0.50, 1.00 };
} // namespace Named

OptionalColor DefaultColor = {};

static Color g_color_table[ 32 ] =
{
	{ 0.20f, 0.20f, 0.20f, 1.00f }, // 0 - black    0
	{ 1.00f, 0.00f, 0.00f, 1.00f }, // 1 - red      1
	{ 0.00f, 1.00f, 0.00f, 1.00f }, // 2 - green    2
	{ 1.00f, 1.00f, 0.00f, 1.00f }, // 3 - yellow   3
	{ 0.00f, 0.00f, 1.00f, 1.00f }, // 4 - blue     4
	{ 0.00f, 1.00f, 1.00f, 1.00f }, // 5 - cyan     5
	{ 1.00f, 0.00f, 1.00f, 1.00f }, // 6 - purple   6
	{ 1.00f, 1.00f, 1.00f, 1.00f }, // 7 - white    7
	{ 1.00f, 0.50f, 0.00f, 1.00f }, // 8 - orange   8
	{ 0.50f, 0.50f, 0.50f, 1.00f }, // 9 - md.grey    9
	{ 0.75f, 0.75f, 0.75f, 1.00f }, // : - lt.grey    10    // lt grey for names
	{ 0.75f, 0.75f, 0.75f, 1.00f }, // ; - lt.grey    11
	{ 0.00f, 0.50f, 0.00f, 1.00f }, // < - md.green   12
	{ 0.50f, 0.50f, 0.00f, 1.00f }, // = - md.yellow  13
	{ 0.00f, 0.00f, 0.50f, 1.00f }, // > - md.blue    14
	{ 0.50f, 0.00f, 0.00f, 1.00f }, // ? - md.red   15
	{ 0.50f, 0.25f, 0.00f, 1.00f }, // @ - md.orange  16
	{ 1.00f, 0.60f, 0.10f, 1.00f }, // A - lt.orange  17
	{ 0.00f, 0.50f, 0.50f, 1.00f }, // B - md.cyan    18
	{ 0.50f, 0.00f, 0.50f, 1.00f }, // C - md.purple  19
	{ 0.00f, 0.50f, 1.00f, 1.00f }, // D        20
	{ 0.50f, 0.00f, 1.00f, 1.00f }, // E        21
	{ 0.20f, 0.60f, 0.80f, 1.00f }, // F        22
	{ 0.80f, 1.00f, 0.80f, 1.00f }, // G        23
	{ 0.00f, 0.40f, 0.20f, 1.00f }, // H        24
	{ 1.00f, 0.00f, 0.20f, 1.00f }, // I        25
	{ 0.70f, 0.10f, 0.10f, 1.00f }, // J        26
	{ 0.60f, 0.20f, 0.00f, 1.00f }, // K        27
	{ 0.80f, 0.60f, 0.20f, 1.00f }, // L        28
	{ 0.60f, 0.60f, 0.20f, 1.00f }, // M        29
	{ 1.00f, 1.00f, 0.75f, 1.00f }, // N        30
	{ 1.00f, 1.00f, 0.50f, 1.00f }, // O        31
};

template<>
const Color& Color::Indexed( int index )
{
	if ( index < 0 )
		index *= -1;
	return g_color_table[index%32];
}

template<>
int Color::To4bit() const noexcept
{
    int color = 0;

    float cmax = std::max({Red(),Green(),Blue()});
    float cmin = std::min({Red(),Green(),Blue()});
    float delta = cmax-cmin;

    if ( delta > 0 )
    {
        float hue = 0;
        if ( Red() == cmax )
            hue = (Green()-Blue())/delta;
        else if ( Green() == cmax )
            hue = (Blue()-Red())/delta + 2;
        else if ( Blue() == cmax )
            hue = (Red()-Green())/delta + 4;

        float sat = delta / cmax;
        if ( sat >= 0.3 )
        {
            if ( hue < 0 )
                hue += 6;

            if ( hue <= 0.5 )      color = 0b001; // red
            else if ( hue <= 1.5 ) color = 0b011; // yellow
            else if ( hue <= 2.5 ) color = 0b010; // green
            else if ( hue <= 3.5 ) color = 0b110; // cyan
            else if ( hue <= 4.5 ) color = 0b100; // blue
            else if ( hue <= 5.5 ) color = 0b101; // magenta
            else                   color = 0b001; // red
        }
        else if ( cmax >= 0.5 )
            color = 7;

        if ( ( cmax + cmin ) / 2 >= 0.64 )
            color |= 0b1000; // bright
    }
    else
    {
        if ( cmax > 0.8 )
            color = 0b1111; // white
        else if ( cmax > 0.53 )
            color = 0b0111; // silver
        else if ( cmax > 0.27 )
            color = 0b1000; // gray
        else
            color = 0b0000; // black
    }

    return color;
}

int StrlenNocolor( const char *string )
{
	int len = 0;
	for ( TokenIterator i ( string ); *i; ++i )
	{
		if ( i->Type() == Token::CHARACTER || i->Type() == Token::ESCAPE )
		{
			len++;
		}
	}

	return len;
}

char *StripColors( char *string )
{
	std::string output;

	for ( TokenIterator i ( string ); *i; ++i )
	{
		if ( i->Type() == Token::CHARACTER )
		{
			output.append( i->Begin(), i->Size() );
		}
		else if ( i->Type() == Token::ESCAPE )
		{
			output.push_back(Constants::ESCAPE);
		}
	}

	strcpy( string, output.c_str() );

	return string;
}

void StripColors( const char *in, char *out, int len )
{
	--len;
	bool decolor = true;
	for ( TokenIterator i ( in ); *i; ++i )
	{
		if ( !decolor || i->Type() == Token::CHARACTER )
		{
			if ( len < i->Size() )
			{
				break;
			}
			if ( *i->Begin() == Constants::DECOLOR_OFF || *i->Begin() == Constants::DECOLOR_ON )
			{
				decolor = ( *i->Begin() == Constants::DECOLOR_ON );
				continue;
			}
			strncpy( out, i->Begin(), i->Size() );
			out += i->Size();
			len -= i->Size();
		}
		else if ( i->Type() == Token::ESCAPE )
		{
			if ( len < 1 )
			{
				break;
			}
			*out++ = Constants::ESCAPE;
		}
	}

	*out = '\0';
}

std::string StripColors( const std::string& input )
{
	std::string output;

	for ( TokenIterator i ( input.c_str() ); *i; ++i )
	{
		if ( i->Type() == Token::CHARACTER )
		{
			output.append( i->Begin(), i->Size() );
		}
		else if ( i->Type() == Token::ESCAPE )
		{
			output.push_back(Constants::ESCAPE);
		}
	}

	return output;
}

namespace detail {

template<class Int>
constexpr bool Has8Bits( Int v )
{
	return ( v / 0x11 * 0x11 ) != v;
}

/*
 * This is to be used when colors are needed to be printed in C-like code
 * Uses multiple character arrays to allow multiple calls
 * within the same sequence point
 */
const char* CString ( const Color32Bit& color )
{
	static const int length = 9;
	static const int nstrings = 4;
	static char text[nstrings][length];
	static int i = nstrings-1;

	i = ( i + 1 ) % nstrings;

	Color32Bit intcolor ( color );

	if ( Has8Bits( intcolor.Red() ) || Has8Bits( intcolor.Green() ) || Has8Bits( intcolor.Blue() ) )
	{
		sprintf(text[i], "^#%02x%02x%02x",
			(int)intcolor.Red(),
			(int)intcolor.Green(),
			(int)intcolor.Blue()
		);
	}
	else
	{
		sprintf(text[i], "^x%x%x%x",
			(int) ( intcolor.Red() & 0xf ) ,
			(int) ( intcolor.Green() & 0xf ),
			(int) ( intcolor.Blue() & 0xf )
		);
	}

	return text[i];

}

} // namespace detail

} // namespace Color
