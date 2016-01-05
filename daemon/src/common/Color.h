/*
===========================================================================
Daemon BSD Source Code
Copyright (c) 2013-2016, Daemon Developers
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

#ifndef COMMON_COLOR_H_
#define COMMON_COLOR_H_

namespace Color {

/*
 * Template class to get information on color components
 */
template<class T>
	struct ColorComponentTraits
{
	// Type used to represent the components
	using component_type = T;
	// Maxiumum value for a component
	static CONSTEXPR component_type component_max = std::numeric_limits<component_type>::max();
	// Size of a component value in bytes
	static CONSTEXPR std::size_t component_size = sizeof(component_type);
};
/*
 * Specialization for normalized floats
 */
template<>
	struct ColorComponentTraits<float>
{
	using component_type = float;
	static CONSTEXPR int            component_max = 1;
	static CONSTEXPR std::size_t    component_size = sizeof(component_type);
};

/*
 * ColorAdaptor to convert different representations of RGB(A) colors to BasicColor
 *
 * Specializations must provide the following members:
 * 	static (AdaptedType) Adapt( TemplateType ); // Creates an object matching the Color concept
 *
 * Requirements for AdaptedType:
 * 	static bool is_color = true;
 * 	typedef (unspecified) component_type; // See ColorComponentTraits
 * 	static component_type component_max = (unspecified); // See ColorComponentTraits
 * 	component_type Red()   const; // Red component
 * 	component_type Green() const; // Green component
 * 	component_type Blue()  const; // Blue component
 * 	component_type Alpha() const; // Alpha component
 *
 */
template<class T>
class ColorAdaptor;

/*
 * Specializations for arrays
 * Assumes it has 4 Component
 */
template<class Component>
class ColorAdaptor<Component*>
{
public:
	static CONSTEXPR bool is_color = true;
	using component_type = Component;
	static CONSTEXPR auto component_max = ColorComponentTraits<Component>::component_max;

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

/*
 * Creates an adaptor for the given value
 * T must have a proper specialization of ColorAdaptor
 */
template<class T>
auto Adapt( const T& object ) -> decltype( ColorAdaptor<T>::Adapt( object ) )
{
	return ColorAdaptor<T>::Adapt( object );
}

template<class Component, class Traits> class BasicColor;
namespace detail {
const BasicColor<float,ColorComponentTraits<float>>& Indexed(int index);
} // namespace detail

// A color with RGBA components
template<class Component, class Traits = ColorComponentTraits<Component>>
class BasicColor
{
public:
	static CONSTEXPR bool is_color = true;
	using color_traits = Traits;
	using component_type = typename color_traits::component_type;
	static CONSTEXPR auto component_max = color_traits::component_max;

	// Returns the value of an indexed color
	static BasicColor Indexed( int i )
    {
        return detail::Indexed( i );
    }

	// Initialize from the components
	CONSTEXPR_FUNCTION BasicColor( component_type r, component_type g, component_type b,
	       component_type a = component_max ) NOEXCEPT
		: red( r ), green( g ), blue( b ), alpha( a )
	{}

#ifdef HAS_EXPLICIT_DEFAULT
    // Default constructor, all components set to zero
    CONSTEXPR_FUNCTION BasicColor() NOEXCEPT = default;

	CONSTEXPR_FUNCTION BasicColor( const BasicColor& ) NOEXCEPT = default;
	CONSTEXPR_FUNCTION BasicColor( BasicColor&& ) NOEXCEPT = default;
    BasicColor& operator=( const BasicColor& ) NOEXCEPT = default;
    BasicColor& operator=( BasicColor&& ) NOEXCEPT = default;
#else
    BasicColor()
        : red( 0 ), green( 0 ), blue( 0 ), alpha( 0 )
    {}
#endif

	template<class T, class = std::enable_if<T::is_color>>
		BasicColor( const T& adaptor ) :
			red  ( ConvertComponent<T>( adaptor.Red()   ) ),
			green( ConvertComponent<T>( adaptor.Green() ) ),
			blue ( ConvertComponent<T>( adaptor.Blue()  ) ),
			alpha( ConvertComponent<T>( adaptor.Alpha() ) )
		{}


	template<class T, class = std::enable_if<T::is_color>>
		BasicColor& operator=( const T& adaptor )
		{
			red   = ConvertComponent<T>( adaptor.Red()   );
			green = ConvertComponent<T>( adaptor.Green() );
			blue  = ConvertComponent<T>( adaptor.Blue()  );
			alpha = ConvertComponent<T>( adaptor.Alpha() );

			return *this;
		}

	// Converts to an array
	CONSTEXPR_FUNCTION const component_type* ToArray() const NOEXCEPT
	{
		return &red;
	}

	CONSTEXPR_FUNCTION_RELAXED component_type* ToArray() NOEXCEPT
	{
		return &red;
	}

	void ToArray( component_type* output ) const
	{
		memcpy( output, ToArray(), ArrayBytes() );
	}

	// Size of the memory location returned by ToArray() in bytes
	CONSTEXPR_FUNCTION std::size_t ArrayBytes() const NOEXCEPT
	{
		return 4 * Traits::component_size;
	}

	CONSTEXPR_FUNCTION component_type Red() const NOEXCEPT
	{
		return red;
	}

	CONSTEXPR_FUNCTION component_type Green() const NOEXCEPT
	{
		return green;
	}

	CONSTEXPR_FUNCTION component_type Blue() const NOEXCEPT
	{
		return blue;
	}

	CONSTEXPR_FUNCTION component_type Alpha() const NOEXCEPT
	{
		return alpha;
	}

	CONSTEXPR_FUNCTION_RELAXED void SetRed( component_type v ) NOEXCEPT
	{
		red = v;
	}

	CONSTEXPR_FUNCTION_RELAXED void SetGreen( component_type v ) NOEXCEPT
	{
		green = v;
	}

	CONSTEXPR_FUNCTION_RELAXED void SetBlue( component_type v ) NOEXCEPT
	{
		blue = v;
	}

	CONSTEXPR_FUNCTION_RELAXED void SetAlpha( component_type v ) NOEXCEPT
	{
		alpha = v;
	}

	CONSTEXPR_FUNCTION_RELAXED BasicColor& operator*=( float factor ) NOEXCEPT
	{
		*this = *this * factor;
		return *this;
	}

	CONSTEXPR_FUNCTION BasicColor operator*( float factor ) const NOEXCEPT
	{
		return BasicColor( red * factor, green * factor, blue * factor, alpha * factor );
	}

	// Fits the component values from 0 to component_max
	CONSTEXPR_FUNCTION_RELAXED void Clamp()
	{
		red = Math::Clamp<component_type>( red, component_type(), component_max );
		green = Math::Clamp<component_type>( green, component_type(), component_max );
		blue = Math::Clamp<component_type>( blue, component_type(), component_max );
		alpha = Math::Clamp<component_type>( alpha, component_type(), component_max );
	}

private:
	// Converts a component, used by conversions from classes implementing the Color concepts
	template<class T>
	static CONSTEXPR_FUNCTION
		typename std::enable_if<component_max != T::component_max, component_type>::type
			ConvertComponent( typename T::component_type from ) NOEXCEPT
	{
		using work_type = typename std::common_type<
			component_type,
			typename T::component_type
		>::type;

		return work_type( from )  / work_type( T::component_max ) * work_type( component_max );
	}
	// Specialization for when the value shouldn't change
	template<class T>
	static CONSTEXPR_FUNCTION
		typename std::enable_if<component_max == T::component_max, component_type>::type
			ConvertComponent( typename T::component_type from ) NOEXCEPT
	{
		return from;
	}

	component_type red = 0, green = 0, blue = 0, alpha = 0;

};

typedef BasicColor<float>         Color;
typedef BasicColor<uint8_t>       Color32Bit;

/*
 * Blend two colors.
 * If factor is 0, the first color will be shown, it it's 1 the second one will
 */
template<class ComponentType, class Traits = ColorComponentTraits<ComponentType>>
CONSTEXPR_FUNCTION BasicColor<ComponentType, Traits> Blend(
	const BasicColor<ComponentType, Traits>& a,
	const BasicColor<ComponentType, Traits>& b,
	float factor ) NOEXCEPT
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
}; // enum
} // namespace Constants

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
extern Color LtOrange;

/*
 * Token for parsing colored strings
 */
class Token
{
public:
	enum TokenType {
		INVALID,       // Invalid/empty token
		CHARACTER,     // A character
		ESCAPE,        // Color escape
		COLOR,         // Color code
	};

	/*
	 * Constructs an invalid token
	 */
#ifdef HAS_EXPLICIT_DEFAULT
	Token() = default;
#else
    Token()
        : begin( nullptr ),
          end( nullptr ),
          type( INVALID )
    {}
#endif


	/*
	 * Constructs a token with the given type and range
	 */
	Token( const char* begin, const char* end, TokenType type )
		: begin( begin ),
		  end( end ),
		  type( type )
	{}

	/*
	 * Constructs a token representing a color
	 */
	Token( const char* begin, const char* end, const ::Color::Color& color )
		: begin( begin ),
		  end( end ),
		  type( COLOR ),
		  color( color )
	{}

	/*
	 * Pointer to the first character of this token in the input sequence
	 */
	const char* Begin() const
	{
		return begin;
	}

	/*
	 * Pointer to the last character of this token in the input sequence
	 */
	const char* End() const
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

	const char*   begin = nullptr;
	const char*   end   = nullptr;
	TokenType      type  = INVALID;
	::Color::Color color;

};

class Parser;

/**
 * Class to parse C-style strings into tokens,
 * implements the InputIterator concept
 */
class TokenIterator
{
public:
	using value_type = Token;
	using reference = const value_type&;
	using pointer = const value_type*;
	using iterator_category = std::input_iterator_tag;
	using difference_type = int;

	TokenIterator() = default;

	explicit TokenIterator( const char* input, const Parser* parent )
        : parent( parent )
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

	TokenIterator& operator++()
	{
		token = NextToken( token.End() );
		return *this;
	}

	TokenIterator operator++(int)
	{
		auto copy = *this;
		token = NextToken( token.End() );
		return copy;
	}

	bool operator==( const TokenIterator& rhs ) const
	{
		return token.Begin() == rhs.token.Begin();
	}

	bool operator!=( const TokenIterator& rhs ) const
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
	value_type NextToken(const char* input);

	value_type token;
    const Parser* parent = nullptr;
};

/**
 * Class to parse C-style utf-8 strings into tokens
 */
class Parser
{
public:
    using value_type = Token;
    using reference = const value_type&;
    using pointer = const value_type*;
    using iterator = TokenIterator;

    explicit Parser(const char* input, const Color& default_color = White)
        : input(input), default_color(default_color)
    {}

    iterator begin() const
    {
        return iterator(input, this);
    }

    iterator end() const
    {
        return iterator();
    }

    Color DefaultColor() const
    {
        return default_color;
    }

private:
    const char* input;
    Color default_color;
};


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
