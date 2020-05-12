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

class RocketChatField : public Rml::Core::Element, Rml::Core::EventListener
{
public:
	RocketChatField( const Rml::Core::String &tag ) :
			Rml::Core::Element( tag ),
			font_engine_interface( Rml::Core::GetFontEngineInterface() ),
			focus( false ),
			cursor_character_index( 0 ),
			text_element( nullptr )
	{
		// Spawn text container and add it to this element
		text_element = AppendChild( Rml::Core::Factory::InstanceElement( this, "div", "*", Rml::Core::XMLAttributes() ) );
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

	virtual void OnAttributeChange( const Rml::Core::ElementAttributes& changed_attributes )
	{
		if (changed_attributes.find( "exec" ) != changed_attributes.end() )
		{
			cmd = GetAttribute<Rml::Core::String>( "exec", "" );
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

	void ProcessEvent( Rml::Core::Event &event )
	{
		Rml::Core::EventListener::ProcessEvent( event );

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
				Rml::Core::Input::KeyIdentifier key_identifier = ( Rml::Core::Input::KeyIdentifier ) event.GetParameter<int>( "key_identifier", 0 );

				switch ( key_identifier )
				{
					case Rml::Core::Input::KI_BACK:
						text.erase( cursor_character_index - 1, 1 );
						UpdateText();
						MoveCursor( -1 );
						break;

					case Rml::Core::Input::KI_DELETE:
						if ( cursor_character_index < (int) text.size() )
						{
							text.erase( cursor_character_index, 1 );
							UpdateText();
						}

						break;

					case Rml::Core::Input::KI_LEFT:
						MoveCursor( -1 );
						break;

					case Rml::Core::Input::KI_RIGHT:
						MoveCursor( 1 );
						break;

					case Rml::Core::Input::KI_RETURN:
					case Rml::Core::Input::KI_NUMPADENTER:
					{
						if ( text.empty() )
						{
							GetOwnerDocument()->Hide();
							return;
						}
						else if ( cmd == "/" )
						{
							trap_SendConsoleCommand( va( "%s\n", text.c_str() ) );
							text.clear();
							UpdateText();
							GetOwnerDocument()->Hide();
							return;
						}

						if ( cmd.empty() )
						{
							cmd = cg_sayCommand.string;
						}

						if ( !cmd.empty() && !text.empty() )
						{
							trap_SendConsoleCommand( va( "%s %s", cmd.c_str(), Cmd::Escape( text.c_str() ).c_str() ) );
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
					const Rml::Core::String& character = event.GetParameter< Rml::Core::String >( "text", "" );

					if ( (int) text.size() == cursor_character_index )
					{
						text.append( character );
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

	bool GetIntrinsicDimensions( Rml::Core::Vector2f &dimension )
	{
		Rml::Core::FontFaceHandle font = text_element->GetFontFaceHandle();
		if ( font == 0 )
		{
			return false;
		}

		const Rml::Core::Property *property;
		property = GetProperty( "width" );

		// Absolute unit. We can use it as is
		if ( property->unit & Rml::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.x = property->value.Get<float>();
		}

		else
		{
			float base_size = 0;
			Rml::Core::Element *parent = this;
			std::stack<Rml::Core::Element*> stack;
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
		Rml::Core::FontFaceHandle font = text_element->GetFontFaceHandle();
		if ( font == 0) return;
		// Generates the cursor.
		cursor_geometry.Release();

		std::vector< Rml::Core::Vertex > &vertices = cursor_geometry.GetVertices();
		vertices.resize( 4 );

		std::vector< int > &indices = cursor_geometry.GetIndices();
		indices.resize( 6 );

		cursor_size.x = 1;
		cursor_size.y = ( float ) font_engine_interface->GetLineHeight( font ) + 2;
		Rml::Core::GeometryUtilities::GenerateQuad( &vertices[0], &indices[0], Rml::Core::Vector2f( 0, 0 ), cursor_size, GetProperty< Rml::Core::Colourb >( "color" ) );
	}

	void MoveCursor( int amt )
	{
		cursor_character_index += amt;

		cursor_character_index = Rml::Core::Math::Clamp<int>( cursor_character_index, 0, text.size() );
	}

	void UpdateCursorPosition()
	{
		if ( text_element->GetFontFaceHandle() == 0 )
		{
			return;
		}

		cursor_position = GetAbsoluteOffset();

		cursor_position.x += ( float ) Rml::Core::ElementUtilities::GetStringWidth( text_element, text.substr( 0, cursor_character_index ) );
	}

	void UpdateText()
	{
		RemoveChild( text_element );
		text_element = AppendChild( Rml::Core::Factory::InstanceElement( this, "div", "*", Rml::Core::XMLAttributes() ) );
		if ( !text.empty() )
		{
			q2rml( text, text_element );
		}
	}

	// Special q -> rml conversion function that preserves carets and codes
	void q2rml( Str::StringRef in, Rml::Core::Element *parent )
	{
		Rml::Core::String out;
		Rml::Core::ElementPtr child;
		bool span = false;

		if ( in.empty() )
		{
			return;
		}

		for ( const auto& token : Color::Parser( in.c_str(), Color::White ) )
		{
			if ( token.Type() == Color::Token::TokenType::COLOR )
			{
				Rml::Core::XMLAttributes xml;

				// Child element initialized
				if ( span && child )
				{
					span = false;
					static_cast<Rml::Core::ElementText *>( child.get() )->SetText( out );
					parent->AppendChild( std::move( child ) );
					out.clear();
				}
				// If not intialized, probably the first one, and should be white.
				else if ( !out.empty() )
				{
					Rml::Core::XMLAttributes xml;
					child = Rml::Core::Factory::InstanceElement( parent, "#text", "span", xml );
					child->SetProperty( "color", "#FFFFFF" );
					static_cast<Rml::Core::ElementText *>( child.get() )->SetText( out );
					parent->AppendChild( std::move( child ) );
					out.clear();
				}


				child = Rml::Core::Factory::InstanceElement( parent, "#text", "span", xml );
				Color::Color32Bit color32 = token.Color();
				child->SetProperty( "color", va( "#%02X%02X%02X", (int) color32.Red(), (int) color32.Green(), (int) color32.Blue() ) );
				out.append( token.Begin(), token.Size() );
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
					out.append( "&lt;" );
				}
				else if ( c == '>' )
				{
					out.append( "&gt;" );
				}
				else if ( c == '&' )
				{
					out.append( "&amp;" );
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
						Rml::Core::XMLAttributes xml;
						child = Rml::Core::Factory::InstanceElement( parent, "#text", "span", xml );
						child->SetProperty( "color", "#FFFFFF" );
					}

					static_cast<Rml::Core::ElementText *>( child.get() )->SetText( out );
					parent->AppendChild( std::move( child ) );
					parent->AppendChild( Rml::Core::Factory::InstanceElement( parent, "*", "br", Rml::Core::XMLAttributes() ) );
					out.clear();
				}
				else
				{
					out.append( token.Begin(), token.Size() );
				}
			}
		}

		if ( span && child && !out.empty() )
		{
			static_cast<Rml::Core::ElementText *>( child.get() )->SetText( out );
			parent->AppendChild( std::move( child ) );
			span = false;
		}

		else if ( !span && !child && !out.empty() )
		{
			child = Rml::Core::Factory::InstanceElement( parent, "#text", "span", Rml::Core::XMLAttributes() );
			static_cast<Rml::Core::ElementText *>( child.get() )->SetText( out );
			parent->AppendChild( std::move( child ) );
		}
	}

private:
	Rml::Core::FontEngineInterface* const font_engine_interface;
	Rml::Core::Vector2f cursor_position;
	bool focus;
	int cursor_character_index;
	Rml::Core::Element *text_element;
	Rml::Core::Context *context;

	Rml::Core::Geometry cursor_geometry;
	Rml::Core::Vector2f cursor_size;
	Rml::Core::Vector2f dimensions;
	Rml::Core::String text;
	Rml::Core::String cmd;

};
#endif


