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

#include "rocket.h"
#include "rocketElement.h"
#include <Rocket/Core/Factory.h>
#include <Rocket/Core/ElementInstancer.h>
#include <Rocket/Core/ElementInstancerGeneric.h>
#include <Rocket/Controls/ElementFormControlDataSelect.h>
#include "rocketConsoleTextElement.h"
#include "../cg_local.h"

extern Rocket::Core::Element *activeElement;

Rocket::Core::String Rocket_QuakeToRML( const char *in );

// File contains support code for custom rocket elements

void Rocket_GetElementTag( char *tag, int length )
{
	if ( activeElement )
	{
		Q_strncpyz( tag, activeElement->GetTagName().CString(), length );
	}
}

void Rocket_SetElementDimensions( float x, float y )
{
	if ( activeElement )
	{
		static_cast<RocketElement *>( activeElement )->SetDimensions( x, y );
	}
}

void Rocket_RegisterElement( const char *tag )
{
	Rocket::Core::Factory::RegisterElementInstancer( tag, new Rocket::Core::ElementInstancerGeneric< RocketElement >() )->RemoveReference();
}

// reduces an rml string to a common format so two rml strings can be compared
static Rocket::Core::String ReduceRML( const Rocket::Core::String &rml )
{
	Rocket::Core::String ret;
	Rocket::Core::String::size_type length = rml.Length();
	ret.Reserve( length );

	for ( unsigned i = 0; i < length; i++ )
	{
		if ( rml[ i ] == ' ' || rml[ i ] == '\n' )
		{
			continue;
		}

		if ( rml[ i ] == '"' )
		{
			ret += '\'';
		}

		else
		{
			ret += rml[ i ];
		}
	}

	return ret;
}

static inline void Rocket_SetInnerRMLGuarded( Rocket::Core::Element *e, const Rocket::Core::String &newRML )
{
	Rocket::Core::String newReducedRML = ReduceRML( newRML );
	Rocket::Core::String oldReducedRML = ReduceRML( e->GetInnerRML() );

	if ( newReducedRML != oldReducedRML )
	{
		e->SetInnerRML( newRML );
	}
}

// This always escapes HTML special characters.
// With additional flags parses colors or emoticons (see Rocket_QuakeToRML).
void Rocket_SetInnerRML( const char *text, int parseFlags )
{
	Rocket::Core::String newRML = Rocket_QuakeToRML( text, parseFlags );

	if ( activeElement )
	{
		Rocket_SetInnerRMLGuarded( activeElement, newRML );
	}

	else
	{
		// TODO warning?
	}
}

// Use this if your string contains RML markup.
void Rocket_SetInnerRMLRaw( const char* RML )
{
	if ( activeElement )
	{
		Rocket_SetInnerRMLGuarded( activeElement, RML );
	}

	else
	{
		// TODO warning?
	}
}

void Rocket_GetAttribute( const char *name, const char *id, const char *attribute, char *out, int length )
{
	if ( ( !*name || !*id ) && activeElement )
	{
		Q_strncpyz( out, activeElement->GetAttribute< Rocket::Core::String >( attribute, "" ).CString(), length );
	}

	else
	{
		Rocket::Core::ElementDocument *document = menuContext->GetDocument( name );

		if ( document )
		{
			Q_strncpyz( out, document->GetElementById( id )->GetAttribute< Rocket::Core::String >( attribute, "" ).CString(), length );
		}
	}
}

void Rocket_SetAttribute( const char *name, const char *id, const char *attribute, const char *value )
{
	if ( ( !*name && !*id ) && activeElement )
	{
		activeElement->SetAttribute( attribute, value );
	}

	else
	{
		Rocket::Core::ElementDocument *document = name[0] ? menuContext->GetDocument( name ) : menuContext->GetFocusElement()->GetOwnerDocument();

		if ( document )
		{
			Rocket::Core::Element *element = document->GetElementById( id );

			if ( element )
			{
				element->SetAttribute( attribute, value );
			}
		}
	}
}

void Rocket_GetElementAbsoluteOffset( float *x, float *y )
{
	if ( activeElement )
	{
		Rocket::Core::Vector2f position = activeElement->GetAbsoluteOffset();
		*x = position.x;
		*y = position.y;
	}

	else
	{
		*x = -1;
		*y = -1;
	}
}

void Rocket_GetElementDimensions( float *w, float *h )
{
	if ( activeElement )
	{
		Rocket::Core::Vector2f dimensions = activeElement->GetBox().GetSize();
		*w = dimensions.x;
		*h = dimensions.y;
	}
	else
	{
		*w = -1;
		*h = -1;
	}
}

void Rocket_GetProperty( const char *name, void *out, int len, rocketVarType_t type )
{
	if ( activeElement )
	{
		const Rocket::Core::Property *property = activeElement->GetProperty( name );

		if ( !property )
		{
			return;
		}

		switch ( type )
		{
			case rocketVarType_t::ROCKET_STRING:
			{
				char *string = ( char * ) out;

				Q_strncpyz( string, property->Get<Rocket::Core::String>().CString(), len );

				return;
			}

			case rocketVarType_t::ROCKET_FLOAT:
			{
				float *f = ( float * ) out;

				if ( len != sizeof( float ) )
				{
					return;
				}

				// HACK: special case for width and height specified in non absolute units

				if ( !Q_stricmp( "width", name ) && property->unit & Rocket::Core::Property::RELATIVE_UNIT )
				{
					float base_size = 0;
					Rocket::Core::Element *parent = activeElement;

					while ( ( parent = parent->GetParentNode() ) )
					{
						if ( ( base_size = parent->GetOffsetWidth() ) != 0 )
						{
							*f = activeElement->ResolveProperty( "width", base_size );
							return;
						}
					}
				}

				if ( !Q_stricmp( "height", name ) && property->unit & Rocket::Core::Property::RELATIVE_UNIT )
				{
					float base_size = 0;
					Rocket::Core::Element *parent = activeElement;

					while ( ( parent = parent->GetParentNode() ) )
					{
						if ( ( base_size = parent->GetOffsetHeight() ) != 0 )
						{
							*f = activeElement->ResolveProperty( "height", base_size );
							return;
						}
					}
				}

				*f = property->Get<float>();
				return;
			}

			case rocketVarType_t::ROCKET_INT:
			{
				int *i = ( int * ) out;

				if ( len != sizeof( int ) )
				{
					return;
				}

				*i = property->Get<int>();
				return;
			}

			case rocketVarType_t::ROCKET_COLOR:
			{
				if ( len == sizeof( Color::Color ) )
				{
					Color::Color* outColor = ( Color::Color* ) out;
					*outColor = Color::Adapt( property->Get<Rocket::Core::Colourb>() );
				}

				return;
			}
		}
	}
}

void Rocket_SetClass( const char *in, bool activate )
{
	bool isSet = activeElement->IsClassSet( in );
	if ( ( activate && !isSet ) || ( !activate && isSet ) )
	{
		activeElement->SetClass( in, static_cast<bool>( activate ) );
	}
}


void Rocket_SetPropertyById( const char *id, const char *property, const char *value )
{
	if ( *id )
	{
		Rocket::Core::ElementDocument *document = menuContext->GetFocusElement()->GetOwnerDocument();

		if ( document )
		{
			Rocket::Core::Element *element = document->GetElementById( id );

			if ( element )
			{
				element->SetProperty( property, value );
			}
		}
	}

	else if ( activeElement )
	{
		activeElement->SetProperty( property, value );
	}
}
std::deque<ConsoleLine> RocketConsoleTextElement::lines;
void Rocket_AddConsoleText(Str::StringRef text)
{
	// HACK: Ugly way to force pre-engine-upgrade behavior. TODO: Make it work without this hack
	static char buffer[ MAX_STRING_CHARS ];
	Q_strncpyz( buffer, text.c_str(), sizeof(buffer) );

	if ( !Q_stricmp( "\n", buffer ) )
	{
		return;
	}

	RocketConsoleTextElement::lines.emplace_front( Rocket::Core::String( va( "%s\n", buffer ) ) );
}

void Rocket_RegisterProperty( const char *name, const char *defaultValue, bool inherited, bool force_layout, const char *parseAs )
{
	Rocket::Core::StyleSheetSpecification::RegisterProperty( name, defaultValue, ( bool ) inherited, ( bool ) force_layout ).AddParser( parseAs );
}


void Rocket_SetDataSelectIndex( int index )
{
	if ( activeElement )
	{
		dynamic_cast< Rocket::Controls::ElementFormControlDataSelect* >( activeElement )->SetSelection( index );
	}
}
