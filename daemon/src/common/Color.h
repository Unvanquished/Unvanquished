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
#ifndef COMMON_COLOR_H_
#define COMMON_COLOR_H_

#include <cstdint>
#include <iterator>
#include <limits>
#include <string>

#include "../engine/qcommon/q_unicode.h"

namespace Color {

/*
================
ColorIndex

Converts a character into an index to be used with g_color_table
================
*/
inline int ColorIndex( char c )
{
	return (c - '0') & 31;
}

extern struct color_s g_color_table[ 32 ];

/*
================
gethex

Converts a hexadecimal character to the value of the digit it represents.
Pre: ishex(ch)
================
*/
inline int gethex( char ch )
{
	return ch > '9' ?
		( ch >= 'a' ? ch - 'a' + 10 : ch - 'A' + 10 )
		: ch - '0'
	;
}

/*
================
ishex

Whether a character is a hexadecimal digit
================
*/
inline bool ishex( char ch )
{
	return ( ch >= '0' && ch <= '9' ) ||
		   ( ch >= 'A' && ch <= 'F' ) ||
		   ( ch >= 'a' && ch <= 'f' );
}

/*
================
hex_to_char

Returns a character representing a hexadecimal digit
Pre: 0 <= i && i < 16
================
*/
inline char hex_to_char( int i )
{
	return i < 10 ? i + '0' : i + 'a' - 10;
}


/*
================
color_s

Value class to represent colors
================
*/
struct color_s
{
	typedef unsigned char component_type;
	typedef std::numeric_limits<component_type> limits_type;

	component_type r = 0, g = 0, b = 0, a = 0;

	/*
	================
	color_s::color_s

	Initialize from a color index
	================
	*/
	explicit color_s(int index)
	{
		if ( index >= 0 && index < 32 )
			*this = g_color_table[index];
	}

	/*
	================
	color_s::color_s

	Initialize from a color index character
	================
	*/
	explicit color_s(char index)
		: color_s( int( ColorIndex(index) ) )
	{
	}

	/*
	================
	color_s::color_s

	Initialize from a float array
	================
	*/
	color_s(const float array[4])
	{
		assign_float_array(array);
	}

	/*
	================
	color_s::color_s

	Default constructor, all components set to zero
	================
	*/
	constexpr color_s() = default;

	/*
	================
	color_s::color_s

	Default constructor, all components set to zero
	================
	*/
	constexpr color_s(component_type r, component_type g, component_type b,
					  component_type a = limits_type::max())
		: r(r), g(g), b(b), a(a)
	{
	}

	bool operator==( const color_s& other ) const
	{
		return integer_32bit() == other.integer_32bit();
	}

	bool operator!=( const color_s& other ) const
	{
		return integer_32bit() != other.integer_32bit();
	}

	void to_float_array(float output[4]) const
	{
		output[0] = r / float(limits_type::max());
		output[1] = g / float(limits_type::max());
		output[2] = b / float(limits_type::max());
		output[3] = a / float(limits_type::max());
	}

	/*
	================
	color_s::to_string

	Returns a string representing the color
	================
	*/
	std::string to_string() const
	{
		return std::string("^x")+hex_to_char(r/17)+hex_to_char(g/17)+hex_to_char(b/17);
	}

	/*
	================
	color_s::to_4bit

	Returns a 4 bit integer with the bits following this pattern:
		1 red
		2 green
		4 blue
		8 bright
	================
	*/
	int to_4bit() const;

private:
	void assign_float_array(const float* col)
	{
		if ( !col )
		{
			// replicate behaviour from refexport_t::SetColor
			r = g = b = a = limits_type::max();
			return;
		}

		r = col[0] * limits_type::max();
		g = col[1] * limits_type::max();
		b = col[2] * limits_type::max();
		a = col[3] * limits_type::max();
	}

	/*
	================
	color_s::integer_32bit

	Returns a 32bit integer representing the color,
	no guarantees are made with respect to endianness
	================
	*/
	uint32_t integer_32bit() const
	{
		return *reinterpret_cast<const uint32_t*>(this);
	}
};

typedef float vec4_t[4];
extern vec4_t colorBlack;
extern vec4_t colorRed;
extern vec4_t colorGreen;
extern vec4_t colorBlue;
extern vec4_t colorYellow;
extern vec4_t colorOrange;
extern vec4_t colorMagenta;
extern vec4_t colorCyan;
extern vec4_t colorWhite;
extern vec4_t colorLtGrey;
extern vec4_t colorMdGrey;
extern vec4_t colorDkGrey;
extern vec4_t colorMdRed;
extern vec4_t colorMdGreen;
extern vec4_t colorDkGreen;
extern vec4_t colorMdCyan;
extern vec4_t colorMdYellow;
extern vec4_t colorMdOrange;
extern vec4_t colorMdBlue;

#define Q_COLOR_ESCAPE   '^'

#define COLOR_BLACK      '0'
#define COLOR_RED        '1'
#define COLOR_GREEN      '2'
#define COLOR_YELLOW     '3'
#define COLOR_BLUE       '4'
#define COLOR_CYAN       '5'
#define COLOR_MAGENTA    '6'
#define COLOR_WHITE      '7'
#define COLOR_ORANGE     '8'
#define COLOR_MDGREY     '9'
#define COLOR_LTGREY     ':'
//#define COLOR_LTGREY  ';'
#define COLOR_MDGREEN    '<'
#define COLOR_MDYELLOW   '='
#define COLOR_MDBLUE     '>'
#define COLOR_MDRED      '?'
#define COLOR_LTORANGE   'A'
#define COLOR_MDCYAN     'B'
#define COLOR_MDPURPLE   'C'
#define COLOR_NULL       '*'


#define S_COLOR_BLACK    "^0"
#define S_COLOR_RED      "^1"
#define S_COLOR_GREEN    "^2"
#define S_COLOR_YELLOW   "^3"
#define S_COLOR_BLUE     "^4"
#define S_COLOR_CYAN     "^5"
#define S_COLOR_MAGENTA  "^6"
#define S_COLOR_WHITE    "^7"
#define S_COLOR_ORANGE   "^8"
#define S_COLOR_MDGREY   "^9"
#define S_COLOR_LTGREY   "^:"
//#define S_COLOR_LTGREY    "^;"
#define S_COLOR_MDGREEN  "^<"
#define S_COLOR_MDYELLOW "^="
#define S_COLOR_MDBLUE   "^>"
#define S_COLOR_MDRED    "^?"
#define S_COLOR_LTORANGE "^A"
#define S_COLOR_MDCYAN   "^B"
#define S_COLOR_MDPURPLE "^C"
#define S_COLOR_NULL     "^*"

template<class String>
	inline bool Q_IsColorString( String&& p )
{
	return p[0] == Q_COLOR_ESCAPE &&
	         ( p[1] == COLOR_NULL || ( p[1] >= '0' && p[1] != Q_COLOR_ESCAPE && p[1] < 'p' ) );
}

/*
================
Q_IsHexColorString

Checks that a string is in the form ^xHHH (where H is a hex digit)
================
*/
template<class String>
	inline bool Q_IsHexColorString( String&& p )
{
	return p[0] == Q_COLOR_ESCAPE && p[1] == 'x'
		&& ishex(p[2]) && ishex(p[3]) && ishex(p[4]);
}

/*
================
Q_SkipColorString

If Q_IsHexColorString(p) or Q_IsColorString(p),
increases i by the right amount of bytes

Returns whether that has been done
================
*/
template<class Incrementable>
	bool Q_SkipColorString( const char* p, Incrementable& i )
{
	if ( Q_IsColorString( p ) )
	{
		i += 2;
		return true;
	}
	else if ( Q_IsHexColorString( p ) )
	{
		i += 5;
		return true;
	}
	return false;
}

template<class CharT>
	bool Q_SkipColorString( CharT *& p )
{
	return Q_SkipColorString(p, p);
}

template<class charT>
	class BasicToken
{
public:
	enum TokenType {
		INVALID,
		CHARACTER,
		ESCAPE,
		COLOR,
		DEFAULT_COLOR,
	};

	BasicToken() = default;

	BasicToken( const charT* begin, const charT* end, TokenType type )
		: begin( begin ),
		  end( end ),
		  type( type )
	{}

	BasicToken( const charT* begin, const charT* end, const color_s& color )
		: begin( begin ),
		  end( end ),
		  type( COLOR ),
		  color( color )
	{}

	const charT* Begin() const
	{
		return begin;
	}

	const charT* End() const
	{
		return end;
	}

	std::size_t Size() const
	{
		return end - begin;
	}

	TokenType Type() const
	{
		return type;
	}

	color_s Color() const
	{
		return color;
	}

	explicit operator bool() const
	{
		return type != INVALID && begin && begin < end;
	}

private:

	const charT*   begin = nullptr;
	const charT*   end   = nullptr;
	TokenType     type  = INVALID;
	color_s       color;

};

class TokenAdvanceOne
{
public:
	template<class CharT>
		constexpr int operator()(const CharT*) const { return 1; }
};

class TokenAdvanceUtf8
{
public:
	int operator()(const char* c) const
	{
		return Q_UTF8_Width(c);
	}
};

template<class CharT, class TokenAdvanceT = TokenAdvanceOne>
	class BasicTokenIterator
{
public:
	using value_type = BasicToken<CharT>;
	using reference = const value_type&;
	using pointer = const value_type*;
	using iterator_category = std::input_iterator_tag;
	using difference_type = int;

	BasicTokenIterator() = default;

	explicit BasicTokenIterator( const CharT* input )
	{
		token = NextToken( input );
	}

	reference operator*() const
	{
		return token;
	}

	pointer operator->() const
	{
		return &token;
	}

	BasicTokenIterator& operator++()
	{
		token = NextToken( token.End() );
		return *this;
	}

	BasicTokenIterator operator++(int)
	{
		auto copy = *this;
		token = NextToken( token.End() );
		return copy;
	}

	bool operator==( const BasicTokenIterator& rhs ) const
	{
		return token.Begin() == rhs.token.Begin();
	}

	bool operator!=( const BasicTokenIterator& rhs ) const
	{
		return token.Begin() != rhs.token.Begin();
	}

	void Skip( difference_type bytes )
	{
		if ( bytes != 0 )
		{
			token = NextToken( token.Begin() + bytes );
		}
	}

private:
	value_type NextToken(const CharT* input)
	{
		if ( !input || *input == '\0' )
		{
			return value_type();
		}

		if ( input[0] == '^' )
		{
			if ( input[1] == '^' )
			{
				return value_type( input, input+2, value_type::ESCAPE );
			}
			else if ( input[1] == COLOR_NULL )
			{
				return value_type( input, input+2, value_type::DEFAULT_COLOR );
			}
			else if ( input[1] >= '0' && input[1] < 'p' )
			{
				return value_type( input, input+2, color_s( char( input[1] ) ) );
			}
			else if ( input[1] == 'x' && ishex( input[2] ) && ishex( input[3] ) && ishex( input[4] ) )
			{
				return value_type( input, input+5, color_s(
					gethex( input[2] ) * 17,
					gethex( input[3] ) * 17,
					gethex( input[4] ) * 17,
					1
				) );
			}
		}

		return value_type( input, input + TokenAdvanceT()( input ), value_type::CHARACTER );
	}

	value_type token;
};

using Token = BasicToken<char>;
using TokenIterator = BasicTokenIterator<char, TokenAdvanceUtf8>;

} // namespace Color

#endif // COMMON_COLOR_H_
