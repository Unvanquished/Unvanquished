/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

#ifndef ROCKETCONDITIONALELEMENT_H
#define ROCKETCONDITIONALELEMENT_H
#include "../framework/CvarSystem.h"
#include <Rocket/Core/Core.h>

class RocketConditionalElement : public Rocket::Core::Element
{
public:
	RocketConditionalElement( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ), condition( NOT_EQUAL ) {}

	virtual void OnAttributeChange( const Rocket::Core::AttributeNameList &changed_attributes )
	{
		if ( changed_attributes.find( "cvar" ) != changed_attributes.end() )
		{
			cvar = GetAttribute< Rocket::Core::String >( "cvar",  "" );
			cvar_value = Cvar_VariableString( cvar.CString() );
		}

		if ( changed_attributes.find( "condition" ) != changed_attributes.end() )
		{
			ParseCondition( GetAttribute< Rocket::Core::String >( "condition", "" ) );
		}

		if ( changed_attributes.find( "value" ) != changed_attributes.end() )
		{
			Rocket::Core::String tmp = GetAttribute< Rocket::Core::String >( "value", "" );
			char *end = nullptr;
			// Check if float
			float isFloat = strtof( tmp.CString(), &end );

			// Is either an integer or float
			if ( end )
			{
				// is integer
				if ( static_cast< int >( isFloat ) == isFloat )
				{
					value.Set( static_cast< int >( isFloat ) );
				}
				else
				{
					value.Set( isFloat );
				}
			}

			// Is a string
			else
			{
				value.Set( tmp );
			}
		}
	}

	virtual void OnUpdate( void )
	{
		if ( !cvar.Empty() && cvar_value != Cvar_VariableString( cvar.CString() ) )
		{
			if ( IsConditionValid() )
			{
				if ( !IsVisible() )
				{
					SetProperty( "display", "block" );
				}
			}
			else
			{
				if ( IsVisible() )
				{
					SetProperty( "display", "none" );
				}
			}

			cvar_value = Cvar_VariableString( cvar.CString() );
		}
	}

private:
	enum Condition
	{
		EQUALS,
		LESS,
		GREATER,
		LESS_EQUAL,
		GREATER_EQUAL,
		NOT_EQUAL
	};

	void ParseCondition( const Rocket::Core::String& str )
	{
		if ( str == "==" )
		{
			condition = EQUALS;
		}
		else if ( str == "<" )
		{
			condition = LESS;
		}
		else if ( str == ">" )
		{
			condition = GREATER;
		}
		else if ( str == "<=" )
		{
			condition = LESS_EQUAL;
		}
		else if ( str == ">=" )
		{
			condition = GREATER_EQUAL;
		}
		else
		{
			condition = NOT_EQUAL;
		}
	}

#define Compare( one, two ) switch ( condition ) { \
			case EQUALS: return one == two; \
			case LESS: return one < two; \
			case GREATER: return one > two; \
			case LESS_EQUAL: return one <= two; \
			case GREATER_EQUAL: return one >= two; \
			case NOT_EQUAL: return one != two; }

	bool IsConditionValid( void )
	{
		switch ( value.GetType() )
		{
			case Rocket::Core::Variant::INT:
				Compare( Cvar_VariableIntegerValue( cvar.CString() ), value.Get<int>() );
			case Rocket::Core::Variant::FLOAT:
				Compare( Cvar_VariableValue( cvar.CString() ), value.Get<float>() );
			default:
				switch ( condition )
				{
					case LESS:
					case LESS_EQUAL:
						return Q_stricmp( Cvar_VariableString( cvar.CString() ), value.Get< Rocket::Core::String >().CString() ) < 0;
					case GREATER:
					case GREATER_EQUAL:
						return Q_stricmp( Cvar_VariableString( cvar.CString() ), value.Get< Rocket::Core::String >().CString() ) > 0;
					case EQUALS:
						return !Q_stricmp( Cvar_VariableString( cvar.CString() ), value.Get< Rocket::Core::String >().CString() );
					case NOT_EQUAL:
						return !!Q_stricmp( Cvar_VariableString( cvar.CString() ), value.Get< Rocket::Core::String >().CString() );
				}
		}

		// Should never reach
		return false;
	}

	Rocket::Core::String cvar;
	Rocket::Core::String cvar_value;
	Condition condition;
	Rocket::Core::Variant value;

};
#endif
