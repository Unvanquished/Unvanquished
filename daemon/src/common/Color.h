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

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <string>
#include <type_traits>

#include "../engine/qcommon/q_unicode.h"
#include "Math.h"

namespace Color {

template<class T>
	struct ColorComponentTraits
{
	typedef T component_type;
	static constexpr const component_type component_max = std::numeric_limits<component_type>::max();
	static constexpr const std::size_t component_size = sizeof(component_type);
};
template<>
	struct ColorComponentTraits<float>
{
	typedef float component_type;
	static constexpr const component_type component_max = 1.0f;
	static constexpr const std::size_t    component_size = sizeof(component_type);
};

template<class T>
class ColorAdaptor;

// Assume it has 4 Component
template<class Component>
class ColorAdaptor<Component*>
{
public:
	static constexpr const bool is_color = true;
	typedef Component component_type;
	static constexpr const component_type component_max = ColorComponentTraits<Component>::component_max;

	static ColorAdaptor Adapt( const Component* array )
	{
		return ColorAdaptor( array );
	}

	ColorAdaptor( const Component* array ) : array( array ) {}

	Component Red() const { return array[0]; }
	Component Green() const { return array[1]; }
	Component Blue() const { return array[2]; }
	Component Alpha() const { return array[3]; }

private:
	const Component* array;
};

template<class Component>
class ColorAdaptor<Component[4]> : public ColorAdaptor<Component*>
{};

template<class Component>
class ColorAdaptor<Component[3]> : public ColorAdaptor<Component*>
{
public:
	static ColorAdaptor Adapt( const Component array[3] )
	{
		return ColorAdaptor( array );
	}

	ColorAdaptor( const Component array[3] ) : ColorAdaptor<Component*>( array ) {}

	Component Alpha() const { return ColorAdaptor<Component*>::component_max; }
};

template<class T>
auto Adapt( const T& object ) -> decltype( ColorAdaptor<T>::Adapt( object ) )
{
	return ColorAdaptor<T>::Adapt( object );
}

// A color with RGBA components
template<class Component, class Traits = ColorComponentTraits<Component>>
class BasicColor
{
public:
	static constexpr const bool is_color = true;
	typedef Traits color_traits;
	typedef typename color_traits::component_type component_type;
	static constexpr const component_type component_max = color_traits::component_max;

	// Returns the value of an indexed color
	static const BasicColor& Indexed( int i );

	// Default constructor, all components set to zero
	constexpr BasicColor() noexcept = default;


	// Initialize from the components
	constexpr BasicColor( component_type r, component_type g, component_type b,
	       component_type a = component_max ) noexcept
		: red( r ), green( g ), blue( b ), alpha( a )
	{}

	constexpr BasicColor( const BasicColor& ) noexcept = default;

	constexpr BasicColor( BasicColor&& ) noexcept = default;

	template<class T, class = std::enable_if<T::is_color>>
		BasicColor( const T& adaptor ) :
			red  ( ConvertComponent<T>( adaptor.Red()   ) ),
			green( ConvertComponent<T>( adaptor.Green() ) ),
			blue ( ConvertComponent<T>( adaptor.Blue()  ) ),
			alpha( ConvertComponent<T>( adaptor.Alpha() ) )
		{}

	BasicColor& operator=( const BasicColor& ) noexcept = default;

	BasicColor& operator=( BasicColor&& ) noexcept = default;

	template<class T, class = std::enable_if<T::is_color>>
		BasicColor& operator=( const T& adaptor )
		{
			red   = ConvertComponent<T>( adaptor.Red()   );
			green = ConvertComponent<T>( adaptor.Green() );
			blue  = ConvertComponent<T>( adaptor.Blue()  );
			alpha = ConvertComponent<T>( adaptor.Alpha() );

			return *this;
		}

	constexpr const component_type* ToArray() const noexcept
	{
		return &red;
	}

	component_type* ToArray() noexcept
	{
		return &red;
	}

	void ToArray( component_type* output ) const
	{
		memcpy( output, ToArray(), ArrayBytes() );
	}

	// Size of the memory location returned by ToArray() in bytes
	constexpr std::size_t ArrayBytes() const noexcept
	{
		return 4 * Traits::component_size;
	}

	constexpr component_type Red() const noexcept
	{
		return red;
	}

	constexpr component_type Green() const noexcept
	{
		return green;
	}

	constexpr component_type Blue() const noexcept
	{
		return blue;
	}

	constexpr component_type Alpha() const noexcept
	{
		return alpha;
	}

	void SetRed( component_type v ) noexcept
	{
		red = v;
	}

	void SetGreen( component_type v ) noexcept
	{
		green = v;
	}

	void SetBlue( component_type v ) noexcept
	{
		blue = v;
	}

	void SetAlpha( component_type v ) noexcept
	{
		alpha = v;
	}

	/*
	 * Returns a 4 bit integer with the bits following this pattern:
	 * 	1 red
	 * 	2 green
	 * 	4 blue
	 * 	8 bright
	 */
	int To4bit() const noexcept;

	BasicColor& operator*=( float factor ) noexcept
	{
		*this = *this * factor;
		return *this;
	}

	constexpr BasicColor operator*( float factor ) const noexcept
	{
		return BasicColor( red * factor, green * factor, blue * factor, alpha * factor );
	}

	void Clamp()
	{
		red = Math::Clamp( red, component_type(), component_max );
		green = Math::Clamp( green, component_type(), component_max );
		blue = Math::Clamp( blue, component_type(), component_max );
		alpha = Math::Clamp( alpha, component_type(), component_max );
	}

private:
	// Converts a component, used by the explicit constructor converting between
	// colors with different template arguments
	template<class T>
	static constexpr
		typename std::enable_if<component_max != T::component_max, component_type>::type
			ConvertComponent( typename T::component_type from ) noexcept
	{
		using work_type = typename std::common_type<
			component_type,
			typename T::component_type
		>::type;

		return work_type( from )  / work_type( T::component_max ) * work_type( component_max );
	}
	template<class T>
	static constexpr
		typename std::enable_if<component_max == T::component_max, component_type>::type
			ConvertComponent( typename T::component_type from ) noexcept
	{
		return from;
	}

	component_type red = 0, green = 0, blue = 0, alpha = 0;

};

template<class Component, class Traits = ColorComponentTraits<Component>>
class BasicOptionalColor
{
public:
	typedef BasicColor<Component, Traits> color_type;

	constexpr BasicOptionalColor() noexcept = default;
	constexpr BasicOptionalColor( const color_type& color ) noexcept
		: color( color ), has_color( true ) {}

	constexpr operator const color_type&() const noexcept
	{
		return color;
	}

	constexpr explicit operator bool() const noexcept
	{
		return has_color;
	}

	constexpr const color_type& Color() const noexcept
	{
		return color;
	}

	constexpr color_type Color( const color_type& default_color ) const noexcept
	{
		return has_color ? color : default_color;
	}

	color_type* operator->() noexcept
	{
		return &color;
	}

	constexpr const color_type* operator->() const noexcept
	{
		return &color;
	}

	color_type& operator*() noexcept
	{
		return color;
	}

	constexpr const color_type& operator*() const noexcept
	{
		return color;
	}

private:
	color_type color { color_type::component_max, color_type::component_max,
	                   color_type::component_max, color_type::component_max };
	bool  has_color = false;
};

typedef BasicColor<float>         Color;
typedef BasicOptionalColor<float> OptionalColor;
typedef BasicColor<uint8_t>       Color32Bit;

extern OptionalColor DefaultColor;

/*
 * Blend two colors.
 * If factor is 0, the first color will be shown, it it's 1 the second one will
 */
template<class ComponentType, class Traits = ColorComponentTraits<ComponentType>>
constexpr BasicColor<ComponentType, Traits> Blend(
	const BasicColor<ComponentType, Traits>& a,
	const BasicColor<ComponentType, Traits>& b,
	float factor ) noexcept
{
	return BasicColor<ComponentType, Traits> {
		ComponentType ( a.Red()   * ( 1 - factor ) + b.Red()   * factor ),
		ComponentType ( a.Green() * ( 1 - factor ) + b.Green() * factor ),
		ComponentType ( a.Blue()  * ( 1 - factor ) + b.Blue()  * factor ),
		ComponentType ( a.Alpha() * ( 1 - factor ) + b.Alpha() * factor ),
	};
}

namespace detail {

const char* CString( const Color32Bit& color );

} // namespace detail

/**
 * Returns a C string for the given color, suitable for printf-like functions
 */
template<class Component, class Traits = ColorComponentTraits<Component>>
const char* CString( const BasicColor<Component, Traits>& color )
{
	return detail::CString( color );
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
extern Color LtOrange;
extern Color MdBlue;
} // namespace Named

/*
 * Generic token for parsing colored strings
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
	 * Constructs an invalid token
	 */
	BasicToken() = default;


	/*
	 * Constructs a token with the given type and range
	 */
	BasicToken( const charT* begin, const charT* end, TokenType type )
		: begin( begin ),
		  end( end ),
		  type( type )
	{}

	/*
	 * Constructs a token representing a color
	 */
	BasicToken( const charT* begin, const charT* end, const ::Color::Color& color )
		: begin( begin ),
		  end( end ),
		  type( COLOR ),
		  color( color )
	{}

	/*
	 * Pointer to the first character of this token in the input sequence
	 */
	const charT* Begin() const
	{
		return begin;
	}

	/*
	 * Pointer to the last character of this token in the input sequence
	 */
	const charT* End() const
	{
		return end;
	}

	/*
	 * Distance berween Begin and End
	 */
	std::size_t Size() const
	{
		return end - begin;
	}

	// Token Type
	TokenType Type() const
	{
		return type;
	}

	/*
	 * Parsed color
	 * Pre: Type() == COLOR
	 */
	::Color::Color Color() const
	{
		return color;
	}

	/*
	 * Converts to bool if the token is valid (and not empty)
	 */
	explicit operator bool() const
	{
		return type != INVALID && begin && begin < end;
	}

private:

	const charT*  begin = nullptr;
	const charT*  end   = nullptr;
	TokenType     type  = INVALID;
	::Color::Color   color;

};

/*
 * Policy for BasicTokenIterator, advances by 1 input element
 */
class TokenAdvanceOne
{
public:
	template<class CharT>
		constexpr int operator()(const CharT*) const { return 1; }
};

/*
 * Policy for BasicTokenIterator<char>, advances to the next Utf-8 code point
 */
class TokenAdvanceUtf8
{
public:
	int operator()(const char* c) const
	{
		return Q_UTF8_Width(c);
	}
};

/**
 * Generic class to parse C-style strings into tokens,
 * implements the InputIterator concept
 *
 * CharT is the type for the input
 * TokenAdvanceT is the advancement policy used to define characters
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

	// Skips the current token by "count" number of input elements
	void Skip( difference_type count )
	{
		if ( count != 0 )
		{
			token = NextToken( token.Begin() + count );
		}
	}

private:
	// Returns the token corresponding to the given input string
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
			else if ( std::toupper( input[1] ) >= '0' && std::toupper( input[1] ) < 'P' )
			{
				return value_type( input, input+2, Color::Indexed( input[1] - '0' ) );
			}
			else if ( std::tolower( input[1] ) == 'x' && ishex( input[2] ) && ishex( input[3] ) && ishex( input[4] ) )
			{
				return value_type( input, input+5, Color(
					gethex( input[2] ) / 15.f,
					gethex( input[3] ) / 15.f,
					gethex( input[4] ) / 15.f,
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
						( (gethex( input[2] ) << 4) | gethex( input[3] ) ) / 255.f,
						( (gethex( input[4] ) << 4) | gethex( input[5] ) ) / 255.f,
						( (gethex( input[6] ) << 4) | gethex( input[7] ) ) / 255.f,
						1
					) );
				}
			}
		}

		return value_type( input, input + TokenAdvanceT()( input ), value_type::CHARACTER );
	}

	/*
	 * Converts a hexadecimal character to the value of the digit it represents.
	 * Pre: ishex(ch)
	 */
	inline static constexpr int gethex( CharT ch )
	{
		return ch > '9' ?
			( ch >= 'a' ? ch - 'a' + 10 : ch - 'A' + 10 )
			: ch - '0'
		;
	}

	// Whether a character is a hexadecimal digit
	inline static constexpr bool ishex( CharT ch )
	{
		return ( ch >= '0' && ch <= '9' ) ||
			( ch >= 'A' && ch <= 'F' ) ||
			( ch >= 'a' && ch <= 'f' );
	}

	value_type token;
};

// Default token type for Utf-8 C-strings
using Token = BasicToken<char>;
// Default token iterator for Utf-8 C-strings
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
