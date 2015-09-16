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

#ifndef ROCKETCHATFIELD_H
#define ROCKETCHATFIELD_H

#include <Rocket/Core.h>
#include <Rocket/Core/Element.h>
#include <Rocket/Core/ElementUtilities.h>
#include <Rocket/Core/GeometryUtilities.h>
#include "../cg_local.h"
#include "rocket.h"

class RocketChatField : public Rocket::Core::Element, Rocket::Core::EventListener
{
public:
	RocketChatField( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ), focus( false ), cursor_character_index( 0 ), text_element( nullptr )
	{
		// Spawn text container
		text_element = Rocket::Core::Factory::InstanceElement( this, "div", "*", Rocket::Core::XMLAttributes() );

		// Add it to this element
		AppendChild( text_element );
		text_element->RemoveReference();
	}

	virtual void OnChildRemove( Element *child )
	{
		Element::OnChildRemove(child);
		if ( child == this && context )
		{
			context->GetRootElement()->RemoveEventListener( "show", this );
			context->GetRootElement()->RemoveEventListener( "hide", this );
			context->GetRootElement()->RemoveEventListener( "blur", this );
			context->GetRootElement()->RemoveEventListener( "mousemove", this );

			context = nullptr;
		}
	}

	void OnRender()
	{
		UpdateCursorPosition();
		cursor_geometry.Render( cursor_position );
	}

	virtual void OnChildAdd( Element *child )
	{
		Element::OnChildAdd( child );
		if ( child == this )
		{
			// Cache context so we can remove the event listeners later
			context = GetContext();

			context->GetRootElement()->AddEventListener( "show", this );
			context->GetRootElement()->AddEventListener( "hide", this );
			context->GetRootElement()->AddEventListener( "blur", this );
			context->GetRootElement()->AddEventListener( "mousemove", this );
		}
	}

	virtual void OnAttributeChange( const Rocket::Core::AttributeNameList& changed_attributes )
	{
		if (changed_attributes.find( "exec" ) != changed_attributes.end() )
		{
			cmd = GetAttribute<Rocket::Core::String>( "exec", "" );
		}
	}

	void OnUpdate()
	{
		// Ensure mouse follow cursor
		if ( focus )
		{
			GetContext()->ProcessMouseMove( cursor_position.x, cursor_position.y, 0 );

			// Make sure this element is in focus if visible
			if ( GetContext()->GetFocusElement() != this )
			{
				this->Click();
				this->Focus();
			}
		}
	}

	void ProcessEvent( Rocket::Core::Event &event )
	{
		Element::ProcessEvent( event );

		// Cannot move mouse while this element is in view
		if ( focus && event == "mousemove" )
		{
			event.StopPropagation();
			return;
		}

		// We are in focus, let the element know
		if ( event.GetTargetElement() == GetOwnerDocument() )
		{
			if ( event == "show" )
			{
				focus = true;
				GetContext()->ShowMouseCursor( false );
			}

			else if ( event == "blur" || event == "hide" )
			{
				focus =  false;
				text.Clear();
				UpdateText();
			}
		}

		if ( focus )
		{
			if ( event == "resize" )
			{
				GetContext()->ShowMouseCursor( false );
				focus = true;
				GenerateCursor();
			}

			// Handle key presses
			else if ( event == "keydown" )
			{
				Rocket::Core::Input::KeyIdentifier key_identifier = ( Rocket::Core::Input::KeyIdentifier ) event.GetParameter<int>( "key_identifier", 0 );

				switch ( key_identifier )
				{
					case Rocket::Core::Input::KI_BACK:
						text.Erase( cursor_character_index - 1, 1 );
						UpdateText();
						MoveCursor( -1 );
						break;

					case Rocket::Core::Input::KI_DELETE:
						if ( cursor_character_index < (int) text.Length() )
						{
							text.Erase( cursor_character_index, 1 );
							UpdateText();
						}

						break;

					case Rocket::Core::Input::KI_LEFT:
						MoveCursor( -1 );
						break;

					case Rocket::Core::Input::KI_RIGHT:
						MoveCursor( 1 );
						break;

					case Rocket::Core::Input::KI_RETURN:
					case Rocket::Core::Input::KI_NUMPADENTER:
					{
						if ( text.Empty() )
						{
							GetOwnerDocument()->Hide();
							return;
						}
						else if ( cmd == "/" )
						{
							Rocket::Core::String utf8String;
							Rocket::Core::WString( text ).ToUTF8( utf8String );
							trap_SendConsoleCommand( va( "%s\n", utf8String.CString() ) );
							text.Clear();
							UpdateText();
							GetOwnerDocument()->Hide();
							return;
						}

						if ( cmd.Empty() )
						{
							cmd = cg_sayCommand.string;
						}

						if ( !cmd.Empty() && !text.Empty() )
						{
							Rocket::Core::String utf8String;
							Rocket::Core::WString( text ).ToUTF8( utf8String );
							trap_SendConsoleCommand( va( "%s %s", cmd.CString(), Cmd::Escape( utf8String.CString() ).c_str() ) );
							text.Clear();
							UpdateText();
							GetOwnerDocument()->Hide();
						}
					}
						break;

					default:
						break;
				}
			}

			else if ( event == "textinput" )
			{
				if ( event.GetParameter< int >( "ctrl_key", 0 ) == 0 &&
				        event.GetParameter< int >( "alt_key", 0 ) == 0 &&
				        event.GetParameter< int >( "meta_key", 0 ) == 0 )
				{
					Rocket::Core::word character = event.GetParameter< Rocket::Core::word >( "data", 0 );

					if ( (int) text.Length() == cursor_character_index )
					{
						text.Append( character );
					}

					else
					{
						text.Insert( cursor_character_index, character );
					}

					UpdateText();
					MoveCursor( 1 );
				}
			}
		}
	}

	bool GetIntrinsicDimensions( Rocket::Core::Vector2f &dimension )
	{
		const Rocket::Core::Property *property;
		property = GetProperty( "width" );

		// Absolute unit. We can use it as is
		if ( property->unit & Rocket::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.x = property->value.Get<float>();
		}

		else
		{
			float base_size = 0;
			Rocket::Core::Element *parent = this;
			std::stack<Rocket::Core::Element*> stack;
			stack.push( this );

			while ( ( parent = parent->GetParentNode() ) )
			{
				if ( ( base_size = parent->GetOffsetWidth() ) != 0 )
				{
					dimensions.x = base_size;
					while ( !stack.empty() )
					{
						dimensions.x = stack.top()->ResolveProperty( "width", dimensions.x );

						stack.pop();
					}
					break;
				}

				stack.push( parent );
			}
		}

		dimensions.y = Rocket::Core::ElementUtilities::GetLineHeight( this );

		dimension = dimensions;

		return true;
	}


protected:
	void GenerateCursor()
	{
		// Generates the cursor.
		cursor_geometry.Release();

		std::vector< Rocket::Core::Vertex > &vertices = cursor_geometry.GetVertices();
		vertices.resize( 4 );

		std::vector< int > &indices = cursor_geometry.GetIndices();
		indices.resize( 6 );

		cursor_size.x = 1;
		cursor_size.y = ( float ) Rocket::Core::ElementUtilities::GetLineHeight( text_element ) + 2;
		Rocket::Core::GeometryUtilities::GenerateQuad( &vertices[0], &indices[0], Rocket::Core::Vector2f( 0, 0 ), cursor_size, GetProperty< Rocket::Core::Colourb >( "color" ) );
	}

	void MoveCursor( int amt )
	{
		cursor_character_index += amt;

		cursor_character_index = Rocket::Core::Math::Clamp<int>( cursor_character_index, 0, text.Length() );
	}

	void UpdateCursorPosition()
	{
		if ( text_element->GetFontFaceHandle() == nullptr )
		{
			return;
		}

		cursor_position = GetAbsoluteOffset();

		cursor_position.x += ( float ) Rocket::Core::ElementUtilities::GetStringWidth( text_element, text.Substring( 0, cursor_character_index ) );
	}

	void UpdateText()
	{
		RemoveChild( text_element );
		text_element = Rocket::Core::Factory::InstanceElement( this, "div", "*", Rocket::Core::XMLAttributes() );
		AppendChild( text_element );
		text_element->RemoveReference();
		if ( !text.Empty() )
		{
			q2rml( text, text_element );
		}
	}

	bool IsColorString( Rocket::Core::WString str, size_t position )
	{
		if ( position + 1 < str.Length() )
		{
			return ( str[position] == Q_COLOR_ESCAPE &&
			( str[position + 1] == COLOR_NULL || ( str[position + 1] >= '0' && str[position + 1] != Q_COLOR_ESCAPE && str[position + 1] < 'p' ) )
			) ? true : false;
		}
		else
		{
			return false;
		}
	}

	// Special q -> rml conversion function that preserves carets and codes
	void q2rml( Rocket::Core::WString in, Rocket::Core::Element *parent )
	{
		Rocket::Core::WString out;
		Rocket::Core::Element *child = nullptr;
		bool span = false;

		if ( in.Empty() )
		{
			return;
		}

		for ( size_t i = 0; i < in.Length(); ++i )
		{
			if ( in[i] == '<' )
			{
				out.Append( Rocket::Core::WString( "&lt;" ) );
			}

			else if ( in[i] == '>' )
			{
				out.Append( Rocket::Core::WString( "&gt;" ) );
			}

			else if ( in[i] == '&' )
			{
				out.Append( Rocket::Core::WString( "&amp;" ) );
			}

			else if ( in[i] == '\n' )
			{
				// Child element initialized.
				if ( span )
				{
					span = false;
				}

				// If not intialized, probably the first one, and should be white.
				else
				{
					Rocket::Core::XMLAttributes xml;
					child = Rocket::Core::Factory::InstanceElement( parent, "#text", "span", xml );
					child->SetProperty( "color", "#FFFFFF" );
				}

				static_cast<Rocket::Core::ElementText *>( child )->SetText( out );
				parent->AppendChild( child );
				child->RemoveReference();
				parent->AppendChild( ( child = Rocket::Core::Factory::InstanceElement( parent, "*", "br", Rocket::Core::XMLAttributes() ) ) );
				child->RemoveReference();
				out.Clear();
			}
			// Convert ^^ to ^
			else if ( in[i] == '^' && i < in.Length() - 1 && in[i + 1] == '^' )
			{
				i++;
				out.Append( Rocket::Core::WString( "^" ) );
			}
			else if ( IsColorString( in, i ) )
			{
				Rocket::Core::XMLAttributes xml;
				int code = ColorIndex( in[++i] );
				auto c = in[i];

				// Child element initialized
				if ( span )
				{
					span = false;
					static_cast<Rocket::Core::ElementText *>( child )->SetText( out );
					parent->AppendChild( child );
					child->RemoveReference();
					out.Clear();
				}

				// If not intialized, probably the first one, and should be white.
				else if ( !out.Empty() )
				{
					Rocket::Core::XMLAttributes xml;
					child = Rocket::Core::Factory::InstanceElement( parent, "#text", "span", xml );
					child->SetProperty( "color", "#FFFFFF" );
					static_cast<Rocket::Core::ElementText *>( child )->SetText( out );
					parent->AppendChild( child );
					child->RemoveReference();
					out.Clear();
				}


				child = Rocket::Core::Factory::InstanceElement( parent, "#text", "span", xml );
				child->SetProperty( "color", va( "#%02X%02X%02X",
				                                 ( int )( g_color_table[ code ][ 0 ] * 255 ),
				                                 ( int )( g_color_table[ code ][ 1 ] * 255 ),
				                                 ( int )( g_color_table[ code ][ 2 ] * 255 ) ) );
				out.Append( Rocket::Core::WString( va( "^%c", c ) ) );
				span = true;
			}
			else
			{
				out.Append( in[i] );
			}
		}

		if ( span && child && !out.Empty() )
		{
			static_cast<Rocket::Core::ElementText *>( child )->SetText( out );
			parent->AppendChild( child );
			child->RemoveReference();
			span = false;
		}

		else if ( !span && !child && !out.Empty() )
		{
			child = Rocket::Core::Factory::InstanceElement( parent, "#text", "span", Rocket::Core::XMLAttributes() );
			static_cast<Rocket::Core::ElementText *>( child )->SetText( out );
			parent->AppendChild( child );
			child->RemoveReference();
		}
	}

private:
	Rocket::Core::Vector2f cursor_position;
	bool focus;
	int cursor_character_index;
	Rocket::Core::Element *text_element;
	Rocket::Core::Context *context;

	Rocket::Core::Geometry cursor_geometry;
	Rocket::Core::Vector2f cursor_size;
	Rocket::Core::Vector2f dimensions;
	Rocket::Core::WString text;
	Rocket::Core::String cmd;

};
#endif


