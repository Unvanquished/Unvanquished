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
#include "../cg_local.h"
#include "rocket.h"

class RocketChatField : public Rml::Element, Rml::EventListener
{
public:
	RocketChatField( const Rml::String &tag ) :
			Rml::Element( tag ),
			font_engine_interface( Rml::GetFontEngineInterface() ),
			cursor_character_index( 0 ),
			text_element( nullptr )
	{
		// Spawn text container and add it to this element
		text_element = AppendChild( Rml::Factory::InstanceElement( this, "div", "*", Rml::XMLAttributes() ) );
	}

	void OnRender()
	{
		UpdateCursorPosition();
		if ( cursor_geometry.GetVertices().empty() )
		{
			GenerateCursor();
		}
		cursor_geometry.Render( cursor_position );
	}

	virtual void OnChildAdd( Element *child )
	{
		Element::OnChildAdd( child );
		if ( child == this )
		{
			this->AddEventListener( "focus", this );
			this->AddEventListener( "mousemove", this );
			this->AddEventListener( "textinput", this );
			this->AddEventListener( "keydown", this );
			this->AddEventListener( "resize", this );
		}
	}

	virtual void OnAttributeChange( const Rml::ElementAttributes& changed_attributes )
	{
		Element::OnAttributeChange( changed_attributes );
		if (changed_attributes.find( "exec" ) != changed_attributes.end() )
		{
			cmd = GetAttribute<Rml::String>( "exec", "" );
		}
	}

	void ProcessEvent( Rml::Event &event )
	{
		// Cannot move mouse while this element is in view
		// FIXME: Does not work? You can move the mouse out of the window.
		if ( event == "mousemove" )
		{
			event.StopPropagation();
			return;
		}

		if ( event == "focus" )
		{
			CG_Rocket_EnableCursor( false );
		}

		{
			if ( event == "resize" )
			{
				CG_Rocket_EnableCursor( false );
				GenerateCursor();
			}

			// Handle key presses
			else if ( event == "keydown" )
			{
				Rml::Input::KeyIdentifier key_identifier = ( Rml::Input::KeyIdentifier ) event.GetParameter<int>( "key_identifier", 0 );

				switch ( key_identifier )
				{
					case Rml::Input::KI_BACK:
						if ( cursor_character_index > 0 )
						{
							MoveCursorBackward();
							text.erase( cursor_character_index, Q_UTF8_Width( text.c_str() + cursor_character_index ) );
							UpdateText();
						}
						break;

					case Rml::Input::KI_DELETE:
						if ( cursor_character_index < text.size() )
						{
							text.erase( cursor_character_index, Q_UTF8_Width( text.c_str() + cursor_character_index ) );
							UpdateText();
						}

						break;

					case Rml::Input::KI_LEFT:
						MoveCursorBackward();
						break;

					case Rml::Input::KI_RIGHT:
						MoveCursorForward();
						break;

					case Rml::Input::KI_C:
						if ( event.GetParameter< int >( "ctrl_key", 0 ) == 1 )
						{
							text.clear();
							cursor_character_index = 0;
							UpdateText();
						}
						break;

					case Rml::Input::KI_RETURN:
					case Rml::Input::KI_NUMPADENTER:
						if ( text.empty() )
						{
							GetOwnerDocument()->Hide();
							return;
						}
						else if ( cmd == "/" )
						{
							trap_SendConsoleCommand( va( "%s\n", text.c_str() ) );
							text.clear();
							cursor_character_index = 0;
							UpdateText();
							GetOwnerDocument()->Hide();
							return;
						}

						if ( cmd.empty() )
						{
							cmd = cg_sayCommand.Get().c_str();
						}

						if ( !cmd.empty() && !text.empty() )
						{
							trap_SendConsoleCommand( va( "%s %s", cmd.c_str(), Cmd::Escape( text.c_str() ).c_str() ) );
							text.clear();
							cursor_character_index = 0;
							UpdateText();
							GetOwnerDocument()->Hide();
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
					const Rml::String& character = event.GetParameter< Rml::String >( "text", "" );

					if ( text.size() == cursor_character_index )
					{
						text.append( character );
					}

					else
					{
						text.insert( cursor_character_index, character );
					}

					UpdateText();
					MoveCursorForward();
				}
			}
		}
	}

	bool GetIntrinsicDimensions( Rml::Vector2f &dimension, float& /*ratio*/ )
	{
		Rml::FontFaceHandle font = text_element->GetFontFaceHandle();
		if ( font == 0 )
		{
			return false;
		}

		const Rml::Property *property;
		property = GetProperty( "width" );

		// Absolute unit. We can use it as is
		if ( property->unit & Rml::Property::ABSOLUTE_UNIT )
		{
			dimensions.x = property->value.Get<float>();
		}

		else
		{
			float base_size = 0;
			Rml::Element *parent = this;
			std::stack<Rml::Element*> stack;
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
		Rml::FontFaceHandle font = text_element->GetFontFaceHandle();
		if ( font == 0 )
		{
			return;
		}
		// Generates the cursor.
		cursor_geometry.Release();

		std::vector< Rml::Vertex > &vertices = cursor_geometry.GetVertices();
		vertices.resize( 4 );

		std::vector< int > &indices = cursor_geometry.GetIndices();
		indices.resize( 6 );

		cursor_size.x = 1;
		cursor_size.y = ( float ) font_engine_interface->GetLineHeight( font ) + 2;
		Rml::GeometryUtilities::GenerateQuad( &vertices[0], &indices[0], Rml::Vector2f( 0, 0 ), cursor_size, GetProperty< Rml::Colourb >( "color" ) );
	}

	void MoveCursorForward()
	{
		if (cursor_character_index >= text.size() )
		{
			return;
		}
		cursor_character_index += Q_UTF8_Width( text.c_str() + cursor_character_index );
	}

	void MoveCursorBackward()
	{
		while ( cursor_character_index > 0 )
		{
			--cursor_character_index;
			if ( !Q_UTF8_ContByte( text[ cursor_character_index ] ) ) break;
		}
	}

	void UpdateCursorPosition()
	{
		if ( text_element->GetFontFaceHandle() == 0 )
		{
			return;
		}

		cursor_position = GetAbsoluteOffset();

		cursor_position.x += ( float ) Rml::ElementUtilities::GetStringWidth( text_element, text.substr( 0, cursor_character_index ) );
	}

	void UpdateText()
	{
		RemoveChild( text_element );
		text_element = AppendChild( Rml::Factory::InstanceElement( this, "div", "*", Rml::XMLAttributes() ) );
		if ( !text.empty() )
		{
			q2rml( text, text_element );
		}
	}

	// Special q -> rml conversion function that preserves carets and codes
	void q2rml( Str::StringRef in, Rml::Element *parent )
	{
		Rml::String out;
		Rml::ElementPtr child;
		bool span = false;

		if ( in.empty() )
		{
			return;
		}

		for ( const auto& token : Color::Parser( in.c_str(), Color::White ) )
		{
			if ( token.Type() == Color::Token::TokenType::COLOR )
			{
				Rml::XMLAttributes xml;

				// Child element initialized
				if ( span && child )
				{
					span = false;
					static_cast<Rml::ElementText *>( child.get() )->SetText( out );
					parent->AppendChild( std::move( child ) );
					out.clear();
				}
				// If not intialized, probably the first one, and should be white.
				else if ( !out.empty() )
				{
					Rml::XMLAttributes xml;
					child = Rml::Factory::InstanceElement( parent, "#text", "span", xml );
					child->SetProperty( "color", "#FFFFFF" );
					static_cast<Rml::ElementText *>( child.get() )->SetText( out );
					parent->AppendChild( std::move( child ) );
					out.clear();
				}


				child = Rml::Factory::InstanceElement( parent, "#text", "span", xml );
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
						Rml::XMLAttributes xml;
						child = Rml::Factory::InstanceElement( parent, "#text", "span", xml );
						child->SetProperty( "color", "#FFFFFF" );
					}

					static_cast<Rml::ElementText *>( child.get() )->SetText( out );
					parent->AppendChild( std::move( child ) );
					parent->AppendChild( Rml::Factory::InstanceElement( parent, "*", "br", Rml::XMLAttributes() ) );
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
			static_cast<Rml::ElementText *>( child.get() )->SetText( out );
			parent->AppendChild( std::move( child ) );
			span = false;
		}

		else if ( !span && !child && !out.empty() )
		{
			child = Rml::Factory::InstanceElement( parent, "#text", "span", Rml::XMLAttributes() );
			static_cast<Rml::ElementText *>( child.get() )->SetText( out );
			parent->AppendChild( std::move( child ) );
		}
	}

private:
	Rml::FontEngineInterface* const font_engine_interface;
	Rml::Vector2f cursor_position;
	size_t cursor_character_index;
	Rml::Element *text_element;

	Rml::Geometry cursor_geometry;
	Rml::Vector2f cursor_size;
	Rml::Vector2f dimensions;
	Rml::String text;
	Rml::String cmd;

};
#endif
