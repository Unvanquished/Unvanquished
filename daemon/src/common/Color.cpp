/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2015, Daemon Developers
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:
    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the Daemon developers nor the
      names of its contributors may be used to endorse or promote products
      derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL DAEMON DEVELOPERS BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
