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

#ifndef ROCKETCHATFIELD_H
#define ROCKETCHATFIELD_H

#include <RmlUi/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Core/ElementUtilities.h>
#include <RmlUi/Core/GeometryUtilities.h>
#include "../cg_local.h"
#include "rocket.h"

class RocketChatField : public Rocket::Core::Element, Rocket::Core::EventListener
{
public:
	RocketChatField( const Rocket::Core::String &tag ) :
			Rocket::Core::Element( tag ),
			font_engine_interface( Rocket::Core::GetFontEngineInterface() ),
			focus( false ),
			cursor_character_index( 0 ),
			text_element( nullptr )
	{
		// Spawn text container and add it to this element
		text_element = AppendChild( Rocket::Core::Factory::InstanceElement( this, "div", "*", Rocket::Core::XMLAttributes() ) );
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

	virtual void OnAttributeChange( const Rocket::Core::ElementAttributes& changed_attributes )
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
				GetContext()->EnableMouseCursor( false );
			}

			else if ( event == "blur" || event == "hide" )
			{
				focus =  false;
				text.clear();
				UpdateText();
			}
		}

		if ( focus )
		{
			if ( event == "resize" )
			{
				GetContext()->EnableMouseCursor( false );
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
						text.erase( cursor_character_index - 1, 1 );
						UpdateText();
						MoveCursor( -1 );
						break;

					case Rocket::Core::Input::KI_DELETE:
						if ( cursor_character_index < (int) text.Length() )
						{
							text.erase( cursor_character_index, 1 );
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
							trap_SendConsoleCommand( va( "%s\n", text.CString() ) );
							text.clear();
							UpdateText();
							GetOwnerDocument()->Hide();
							return;
						}

						if ( cmd.Empty() )
						{
							cmd = cg_sayCommand.Get().c_str();
						}

						if ( !cmd.Empty() && !text.Empty() )
						{
							trap_SendConsoleCommand( va( "%s %s", cmd.CString(), Cmd::Escape( text.CString() ).c_str() ) );
							text.clear();
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
					const Rocket::Core::String& character = event.GetParameter< Rocket::Core::String >( "text", "" );

					if ( (int) text.Length() == cursor_character_index )
					{
						text.Append( character );
					}

					else
					{
						text.insert( cursor_character_index, character );
					}

					UpdateText();
					MoveCursor( 1 );
				}
			}
		}
	}

	bool GetIntrinsicDimensions( Rocket::Core::Vector2f &dimension )
	{
		Rocket::Core::FontFaceHandle font = text_element->GetFontFaceHandle();
		if ( font == 0 )
		{
			return false;
		}

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
						dimensions.x = stack.top()->ResolveNumericProperty( property, dimensions.x );

						stack.pop();
					}
					break;
				}

				stack.push( parent );
			}
		}

		dimensions.y = font_engine_interface->GetLineHeight( font );

		dimension = dimensions;

		return true;
	}


protected:
	void GenerateCursor()
	{
		Rocket::Core::FontFaceHandle font = text_element->GetFontFaceHandle();
		if ( font == 0 )
		{
			return;
		}
		// Generates the cursor.
		cursor_geometry.Release();

		std::vector< Rocket::Core::Vertex > &vertices = cursor_geometry.GetVertices();
		vertices.resize( 4 );

		std::vector< int > &indices = cursor_geometry.GetIndices();
		indices.resize( 6 );

		cursor_size.x = 1;
		cursor_size.y = ( float ) font_engine_interface->GetLineHeight( font ) + 2;
		Rocket::Core::GeometryUtilities::GenerateQuad( &vertices[0], &indices[0], Rocket::Core::Vector2f( 0, 0 ), cursor_size, GetProperty< Rocket::Core::Colourb >( "color" ) );
	}

	void MoveCursor( int amt )
	{
		cursor_character_index += amt;

		cursor_character_index = Rocket::Core::Math::Clamp<int>( cursor_character_index, 0, text.Length() );
	}

	void UpdateCursorPosition()
	{
		if ( text_element->GetFontFaceHandle() == 0 )
		{
			return;
		}

		cursor_position = GetAbsoluteOffset();

		cursor_position.x += ( float ) Rocket::Core::ElementUtilities::GetStringWidth( text_element, text.substr( 0, cursor_character_index ) );
	}

	void UpdateText()
	{
		RemoveChild( text_element );
		text_element = AppendChild( Rocket::Core::Factory::InstanceElement( this, "div", "*", Rocket::Core::XMLAttributes() ) );
		if ( !text.Empty() )
		{
			q2rml( text, text_element );
		}
	}

	// Special q -> rml conversion function that preserves carets and codes
	void q2rml( Str::StringRef in, Rocket::Core::Element *parent )
	{
		Rocket::Core::String out;
		Rocket::Core::ElementPtr child;
		bool span = false;

		if ( in.Empty() )
		{
			return;
		}

		for ( const auto& token : Color::Parser( in.CString(), Color::White ) )
		{
			if ( token.Type() == Color::Token::TokenType::COLOR )
			{
				Rocket::Core::XMLAttributes xml;

				// Child element initialized
				if ( span && child )
				{
					span = false;
					static_cast<Rocket::Core::ElementText *>( child.get() )->SetText( out );
					parent->AppendChild( std::move( child ) );
					out.clear();
				}
				// If not intialized, probably the first one, and should be white.
				else if ( !out.Empty() )
				{
					Rocket::Core::XMLAttributes xml;
					child = Rocket::Core::Factory::InstanceElement( parent, "#text", "span", xml );
					child->SetProperty( "color", "#FFFFFF" );
					static_cast<Rocket::Core::ElementText *>( child.get() )->SetText( out );
					parent->AppendChild( std::move( child ) );
					out.clear();
				}


				child = Rocket::Core::Factory::InstanceElement( parent, "#text", "span", xml );
				Color::Color32Bit color32 = token.Color();
				child->SetProperty( "color", va( "#%02X%02X%02X", (int) color32.Red(), (int) color32.Green(), (int) color32.Blue() ) );
				out.Append( token.Begin(), token.Size() );
				span = true;
			}
			else if ( token.Type() == Color::Token::TokenType::ESCAPE )
			{
				out.push_back( Color::Constants::ESCAPE );
			}
			else if ( token.Type() == Color::Token::TokenType::CHARACTER )
			{
				auto c = *token.Begin();

				if ( c == '<' )
				{
					out.Append( "&lt;" );
				}
				else if ( c == '>' )
				{
					out.Append( "&gt;" );
				}
				else if ( c == '&' )
				{
					out.Append( "&amp;" );
				}
				else if ( c == '\n' )
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

					static_cast<Rocket::Core::ElementText *>( child.get() )->SetText( out );
					parent->AppendChild( std::move( child ) );
					parent->AppendChild( Rocket::Core::Factory::InstanceElement( parent, "*", "br", Rocket::Core::XMLAttributes() ) );
					out.clear();
				}
				else
				{
					out.Append( token.Begin(), token.Size() );
				}
			}
		}

		if ( span && child && !out.Empty() )
		{
			static_cast<Rocket::Core::ElementText *>( child.get() )->SetText( out );
			parent->AppendChild( std::move( child ) );
			span = false;
		}

		else if ( !span && !child && !out.Empty() )
		{
			child = Rocket::Core::Factory::InstanceElement( parent, "#text", "span", Rocket::Core::XMLAttributes() );
			static_cast<Rocket::Core::ElementText *>( child.get() )->SetText( out );
			parent->AppendChild( std::move( child ) );
		}
	}

private:
	Rocket::Core::FontEngineInterface* const font_engine_interface;
	Rocket::Core::Vector2f cursor_position;
	bool focus;
	int cursor_character_index;
	Rocket::Core::Element *text_element;
	Rocket::Core::Context *context;

	Rocket::Core::Geometry cursor_geometry;
	Rocket::Core::Vector2f cursor_size;
	Rocket::Core::Vector2f dimensions;
	Rocket::Core::String text;
	Rocket::Core::String cmd;

};
#endif


