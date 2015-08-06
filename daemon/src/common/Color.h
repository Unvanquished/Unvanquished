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
Color

Value class to represent colors
================
*/
struct Color
{
	typedef unsigned char component_type;
	typedef std::numeric_limits<component_type> limits_type;

	component_type r = 0, g = 0, b = 0, a = 0;

	/*
	================
	Color::Color

	Initialize from a color index
	================
	*/
	explicit Color(int index);

	/*
	================
	Color::Color

	Initialize from a color index character
	================
	*/
	explicit Color(char index)
		: Color( int( index - '0') )
	{
	}

	/*
	================
	Color::Color

	Initialize from a float array
	================
	*/
	Color(const float array[4])
	{
		assign_float_array(array);
	}

	/*
	================
	Color::Color

	Default constructor, all components set to zero
	================
	*/
	constexpr Color() = default;

	/*
	================
	Color::Color

	Default constructor, all components set to zero
	================
	*/
	constexpr Color(component_type r, component_type g, component_type b,
					  component_type a = limits_type::max())
		: r(r), g(g), b(b), a(a)
	{
	}

	bool operator==( const Color& other ) const
	{
		return integer_32bit() == other.integer_32bit();
	}

	bool operator!=( const Color& other ) const
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
	Color::to_string

	Returns a string representing the color
	================
	*/
	std::string to_string() const
	{
		return std::string("^x")+hex_to_char(r/17)+hex_to_char(g/17)+hex_to_char(b/17);
	}

	/*
	================
	Color::to_4bit

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
	Color::integer_32bit

	Returns a 32bit integer representing the color,
	no guarantees are made with respect to endianness
	================
	*/
	uint32_t integer_32bit() const
	{
		return *reinterpret_cast<const uint32_t*>(this);
	}
};


namespace Constants {
// Namespace enum to have these constants scoped but allowing implicit conversions
enum {
	ESCAPE = '^',
	NULL_COLOR = '*',
}; // enum
} // namespace Constants

namespace Named {
extern Color Black;
extern Color Red;
extern Color Green;
extern Color Blue;
extern Color Yellow;
extern Color Orange;
extern Color Magenta;
extern Color Cyan;
extern Color White;
extern Color LtGrey;
extern Color MdGrey;
extern Color DkGrey;
extern Color MdRed;
extern Color MdGreen;
extern Color DkGreen;
extern Color MdCyan;
extern Color MdYellow;
extern Color MdOrange;
extern Color MdBlue;
} // namespace Named

namespace NamedString {
extern const char* Black;
extern const char* Red;
extern const char* Green;
extern const char* Blue;
extern const char* Yellow;
extern const char* Orange;
extern const char* Magenta;
extern const char* Cyan;
extern const char* White;
extern const char* LtGrey;
extern const char* MdGrey;
extern const char* MdRed;
extern const char* MdGreen;
extern const char* MdCyan;
extern const char* MdYellow;
extern const char* LtOrange;
extern const char* MdBlue;
extern const char* Null;
} // namespace Named

namespace NamedFloat {
typedef float vec4_t[4];
extern vec4_t Black;
extern vec4_t Red;
extern vec4_t Green;
extern vec4_t Blue;
extern vec4_t Yellow;
extern vec4_t Orange;
extern vec4_t Magenta;
extern vec4_t Cyan;
extern vec4_t White;
extern vec4_t LtGrey;
extern vec4_t MdGrey;
extern vec4_t DkGrey;
extern vec4_t MdRed;
extern vec4_t MdGreen;
extern vec4_t DkGreen;
extern vec4_t MdCyan;
extern vec4_t MdYellow;
extern vec4_t MdOrange;
extern vec4_t MdBlue;
} // namespace NamedFloat

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

	BasicToken( const charT* begin, const charT* end, const ::Color::Color& color )
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

	::Color::Color Color() const
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
	::Color::Color       color;

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

		if ( input[0] == Constants::ESCAPE )
		{
			if ( input[1] == Constants::ESCAPE )
			{
				return value_type( input, input+2, value_type::ESCAPE );
			}
			else if ( input[1] == Constants::NULL_COLOR )
			{
				return value_type( input, input+2, value_type::DEFAULT_COLOR );
			}
			else if ( input[1] >= '0' && input[1] < 'p' )
			{
				return value_type( input, input+2, Color( char( input[1] ) ) );
			}
			else if ( input[1] == 'x' && ishex( input[2] ) && ishex( input[3] ) && ishex( input[4] ) )
			{
				return value_type( input, input+5, Color(
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
