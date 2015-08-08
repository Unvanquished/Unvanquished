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

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <string>
#include <type_traits>

#include "../engine/qcommon/q_unicode.h"

namespace Color {

/*
================
gethex

Converts a hexadecimal character to the value of the digit it represents.
Pre: ishex(ch)
================
*/
inline constexpr int gethex( char ch )
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
inline constexpr bool ishex( char ch )
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
inline constexpr char hex_to_char( int i )
{
	return i < 10 ? i + '0' : i + 'a' - 10;
}

// Description of the components used by a color class
enum class Components
{
	RGBA
};

// SFINAE trait to determine if a type is a color
template<class T, class = void>
struct IsColor : public std::false_type {};

template<class T>
struct IsColor<T,
	typename std::enable_if<
		std::is_same<
			typename std::remove_const<decltype(T::components)>::type,
			Components
		>::value &&
		std::is_same<
			typename std::remove_const<decltype(T::component_max)>::type,
			typename T::component_type
		>::value
	>::type
> : std::true_type {};


/*
================
Color

Value class to represent colors
================
*/
class Color
{
public:
	typedef uint8_t component_type;
	static constexpr const component_type component_max
		= std::numeric_limits<component_type>::max();
	static constexpr const Components components = Components::RGBA;

	/*
	================
	Color::Color

	Default constructor, all components set to zero
	================
	*/
	constexpr Color() = default;

	// Initialize from the components
	constexpr Color( component_type r, component_type g, component_type b,
					  component_type a = component_max )
		: r(r), g(g), b(b), a(a)
	{
	}

	/*
	================
	Color::Color

	Initialize from a color index
	================
	*/
	explicit Color( int index );

	/*
	================
	Color::Color

	Initialize from a color index character
	================
	*/
	explicit Color( char index )
		: Color( int( index - '0') )
	{
	}

	constexpr component_type Red() const
	{
		return r;
	}

	constexpr component_type Green() const
	{
		return g;
	}

	constexpr component_type Blue() const
	{
		return b;
	}

	constexpr component_type Alpha() const
	{
		return a;
	}

	void SetRed( component_type v )
	{
		r = v;
	}

	void SetGreen( component_type v )
	{
		g = v;
	}

	void SetBlue( component_type v )
	{
		b = v;
	}

	void SetAlpha( component_type v )
	{
		a = v;
	}

	bool operator==( const Color& other ) const
	{
		return To32bit() == other.To32bit();
	}

	bool operator!=( const Color& other ) const
	{
		return To32bit() != other.To32bit();
	}

	/*
	================
	Color::toString

	Returns a string representing the color
	================
	*/
	std::string ToString() const
	{
		return std::string("^x")+hex_to_char(r/17)+hex_to_char(g/17)+hex_to_char(b/17);
	}

	/*
	================
	Color::to4bit

	Returns a 4 bit integer with the bits following this pattern:
		1 red
		2 green
		4 blue
		8 bright
	================
	*/
	int To4bit() const;

private:

	/*
	================
	Color::To32bit

	Returns a 32bit integer representing the color,
	no guarantees are made with respect to endianness
	================
	*/
	uint32_t To32bit() const
	{
		return *reinterpret_cast<const uint32_t*>(this);
	}


	component_type r = 0, g = 0, b = 0, a = 0;
};

// A color with normalized floats RGBA components
class ColorFloat
{
public:
	typedef float component_type;
	static constexpr const component_type component_max = 1;
	static constexpr const Components components = Components::RGBA;


	// Default constructor, all components set to zero
	constexpr ColorFloat() : array{ 0.f, 0.f, 0.f, 0.f } {}


	// Initialize from the components
	constexpr ColorFloat( component_type r, component_type g, component_type b,
					  component_type a = component_max )
		: array{ r, g, b, a }
	{
	}

	// Copy from array
	constexpr ColorFloat( const float array[4] ) :
		array{ array[0], array[1], array[2], array[3] }
	{

	}

	ColorFloat( const std::nullptr_t& ) = delete;

	// Convert to array
	constexpr operator const float*() const
	{
		return array;
	}

	operator float*()
	{
		return array;
	}

	constexpr const float* toArray() const
	{
		return array;
	}

	float* toArray()
	{
		return array;
	}

	void toArray( float* output ) const
	{
		memcpy( output, array, sizeof(float)*4 );
	}


	constexpr component_type Red() const
	{
		return array[0];
	}

	constexpr component_type Green() const
	{
		return array[1];
	}

	constexpr component_type Blue() const
	{
		return array[2];
	}

	constexpr component_type Alpha() const
	{
		return array[3];
	}

	void SetRed( component_type v )
	{
		array[0] = v;
	}

	void SetGreen( component_type v )
	{
		array[1] = v;
	}

	void SetBlue( component_type v )
	{
		array[2] = v;
	}

	void SetAlpha( component_type v )
	{
		array[3] = v;
	}

	ColorFloat& operator*= ( component_type factor )
	{
		for ( component_type& c : array )
		{
			c *= factor;
		}
		return *this;
	}

private:
	// Note: if the internal layout, the way it's serialized should be changed
	// (see SerializeTraits<Color::ColorFloat> in cg_msgdef.h)
	component_type array[4];
};

/*
 * Blend two colors.
 * If factor is 0, the first color will be shown, it it's 1 the second one will
 */
template<class ColorType>
constexpr ColorType Blend(const ColorType& a, const ColorType& b, float factor)
{
	return ColorType {
		typename ColorType::component_type ( a.Red()   * ( 1 - factor ) + b.Red()   * factor ),
		typename ColorType::component_type ( a.Green() * ( 1 - factor ) + b.Green() * factor ),
		typename ColorType::component_type ( a.Blue()  * ( 1 - factor ) + b.Blue()  * factor ),
		typename ColorType::component_type ( a.Alpha() * ( 1 - factor ) + b.Alpha() * factor ),
	};
}

// Converts a component, used by Cast()
template<class ColorDest, class ColorSource>
constexpr typename ColorDest::component_type ConvertComponent( typename ColorSource::component_type from )
{
	using Component = typename std::common_type<
		typename ColorSource::component_type,
		typename ColorDest::component_type
	>::type;

	return Component( from )   / Component( ColorSource::component_max ) * Component( ColorDest::component_max );
}

// Converts two types implementing the Color concept
template<class ColorDest, class ColorSource>
constexpr typename std::enable_if<
		ColorDest::components == Components::RGBA &&
		ColorSource::components == Components::RGBA &&
		!std::is_same<ColorDest, ColorSource>::value, ColorDest>::type
	Cast( const ColorSource& from )
{

	return {
		ConvertComponent<ColorDest, ColorSource>( from.Red() ),
		ConvertComponent<ColorDest, ColorSource>( from.Green() ),
		ConvertComponent<ColorDest, ColorSource>( from.Blue() ),
		ConvertComponent<ColorDest, ColorSource>( from.Alpha() ),
	};
}

// Converts to the same type
template<class ColorSource>
	ColorSource Cast( const ColorSource& from )
{
	return from;
}


namespace Constants {
// Namespace enum to have these constants scoped but allowing implicit conversions
enum {
	ESCAPE = '^',
	NULL_COLOR = '*',
	DECOLOR_OFF = '\16', // these two are used by the second overload of StripColors (and a couple other places)
	DECOLOR_ON  = '\17', // will have to be checked whether they are really useful
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
extern ColorFloat Black;
extern ColorFloat Red;
extern ColorFloat Green;
extern ColorFloat Blue;
extern ColorFloat Yellow;
extern ColorFloat Orange;
extern ColorFloat Magenta;
extern ColorFloat Cyan;
extern ColorFloat White;
extern ColorFloat LtGrey;
extern ColorFloat MdGrey;
extern ColorFloat DkGrey;
extern ColorFloat MdRed;
extern ColorFloat MdGreen;
extern ColorFloat DkGreen;
extern ColorFloat MdCyan;
extern ColorFloat MdYellow;
extern ColorFloat MdOrange;
extern ColorFloat MdBlue;
} // namespace NamedFloat

/*
================
BasicToken

Generic token for parsing colored strings
================
*/
template<class charT>
	class BasicToken
{
public:
	enum TokenType {
		INVALID,       // Invalid/empty token
		CHARACTER,     // A character
		ESCAPE,        // Color escape
		COLOR,         // Color code
		DEFAULT_COLOR, // Color code to reset to the default
	};

	/*
	================
	BasicToken::BasicToken

	Constructs an invalid token
	================
	*/
	BasicToken() = default;


	/*
	================
	BasicToken::BasicToken

	Constructs a token with the given type and range
	================
	*/
	BasicToken( const charT* begin, const charT* end, TokenType type )
		: begin( begin ),
		  end( end ),
		  type( type )
	{}

	/*
	================
	BasicToken::BasicToken

	Constructs a token representing a color
	================
	*/
	BasicToken( const charT* begin, const charT* end, const ::Color::Color& color )
		: begin( begin ),
		  end( end ),
		  type( COLOR ),
		  color( color )
	{}

	/*
	================
	BasicToken::Begin

	Pointer to the first character of this token in the input sequence
	================
	*/
	const charT* Begin() const
	{
		return begin;
	}

	/*
	================
	BasicToken::Begin

	Pointer to the last character of this token in the input sequence
	================
	*/
	const charT* End() const
	{
		return end;
	}

	/*
	================
	BasicToken::Size

	Distance berween Begin and End
	================
	*/
	std::size_t Size() const
	{
		return end - begin;
	}

	/*
	================
	BasicToken::Type

	Token Type
	================
	*/
	TokenType Type() const
	{
		return type;
	}

	/*
	================
	BasicToken::Color

	Parsed color
	Pre: Type() == COLOR
	================
	*/
	::Color::Color Color() const
	{
		return color;
	}

	/*
	================
	BasicToken::operator bool

	Converts to bool if the token is valid (and not empty)
	================
	*/
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

/*
================
TokenAdvanceOne

Policy for BasicTokenIterator, advances by 1 input element
================
*/
class TokenAdvanceOne
{
public:
	template<class CharT>
		constexpr int operator()(const CharT*) const { return 1; }
};

/*
================
TokenAdvanceUtf8

Policy for BasicTokenIterator<char>, advances to the next Utf-8 code point
================
*/
class TokenAdvanceUtf8
{
public:
	int operator()(const char* c) const
	{
		return Q_UTF8_Width(c);
	}
};

/*
================
BasicTokenIterator

Generic class to parse C-style strings into tokens,
implements the InputIterator concept

CharT is the type for the input
TokenAdvanceT is the advancement policy used to define characters
================
*/
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

	/*
	================
	BasicTokenIterator::Skip

	Skips the current token by "count" number of input elements
	================
	*/
	void Skip( difference_type count )
	{
		if ( count != 0 )
		{
			token = NextToken( token.Begin() + count );
		}
	}

private:
	/*
	================
	BasicTokenIterator::NextToken

	Returns the token corresponding to the given input string
	================
	*/
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
			else if ( input[1] == '#' )
			{
				bool long_hex = true;
				for ( int i = 0; i < 6; i++ )
				{
					if ( !ishex( input[i+2] ) )
					{
						long_hex = false;
						break;
					}
				}
				if ( long_hex )
				{
					return value_type( input, input+8, Color(
						(gethex( input[2] ) << 4) | gethex( input[3] ),
						(gethex( input[4] ) << 4) | gethex( input[5] ),
						(gethex( input[6] ) << 4) | gethex( input[7] ),
						1
					) );
				}
			}
		}

		return value_type( input, input + TokenAdvanceT()( input ), value_type::CHARACTER );
	}

	value_type token;
};

/*
================
Token

Default token type for Utf-8 C-strings
================
*/
using Token = BasicToken<char>;
/*
================
TokenIterator

Default token iterator for Utf-8 C-strings
================
*/
using TokenIterator = BasicTokenIterator<char, TokenAdvanceUtf8>;


// Returns the number of characters in a string discarding color codes
// UTF-8 sequences are counted as a single character
int StrlenNocolor( const char* string );

// Removes the color codes from string (in place)
char* StripColors( char* string );

// Removes color codes from in, writing to out
// Pre: in NUL terminated and out can contain at least len characters
void StripColors( const char *in, char *out, int len );

// Overload for C++ strings
std::string StripColors( const std::string& input );



} // namespace Color

#endif // COMMON_COLOR_H_
