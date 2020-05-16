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

#ifndef ROCKETCIRCLEMENU_H
#define ROCKETCIRCLEMENU_H

#include <RmlUi/Core.h>
#include <RmlUi/Core/Element.h>
#include <RmlUi/Controls.h>
#include <RmlUi/Controls/DataSourceListener.h>
#include <RmlUi/Controls/DataSource.h>
#include "../cg_local.h"
#include "rocket.h"

class RocketCircleMenu : public Rml::Core::Element, public Rml::Controls::DataSourceListener, Rml::Core::EventListener
{
public:
	RocketCircleMenu( const Rml::Core::String &tag ) : Rml::Core::Element( tag ), dirty_query( false ), dirty_layout( false ), init( false ), radius( 10 ), formatter( nullptr ), data_source( nullptr )
	{
	}

	virtual ~RocketCircleMenu()
	{
		if ( data_source )
		{
			data_source->DetachListener( this );
		}
	}

	void SetDataSource( const Rml::Core::String &dsn )
	{
		ParseDataSource( data_source, data_table, dsn );
		data_source->AttachListener( this );
		dirty_query = true;
	}

	bool GetIntrinsicDimensions( Rml::Core::Vector2f &dimensions )
	{
		const Rml::Core::Property *property;

		dimensions.x = dimensions.y = -1;

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
						dimensions.x = stack.top()->ResolveNumericProperty( "width" );

						stack.pop();
					}
					break;
				}

				stack.push( parent );
			}
		}

		property = GetProperty( "height" );

		if ( property->unit & Rml::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.y = property->value.Get<float>();
		}

		else
		{
			float base_size = 0;
			Rml::Core::Element *parent = this;
			std::stack<Rml::Core::Element*> stack;
			stack.push( this );

			while ( ( parent = parent->GetParentNode() ) )
			{
				if ( ( base_size = parent->GetOffsetHeight() ) != 0 )
				{
					dimensions.y = base_size;
					while ( !stack.empty() )
					{
						dimensions.y = stack.top()->ResolveNumericProperty( "height" );

						stack.pop();
					}
					break;
				}

				stack.push( parent );
			}
		}

		// Return the calculated dimensions. If this changes the size of the element, it will result in
		// a 'resize' event which is caught below and will regenerate the geometry.

		if ( this->dimensions != dimensions )
		{
			this->dimensions = dimensions;
			dirty_layout = true;
		}

		return true;
	}

	void OnAttributeChange( const Rml::Core::ElementAttributes &changed_attributes )
	{
		Rml::Core::Element::OnAttributeChange( changed_attributes );
		auto it = changed_attributes.find( "source" );
		if ( it != changed_attributes.end() )
		{
			SetDataSource( it->second.Get<Rml::Core::String>( "source" ) );
		}

		it = changed_attributes.find( "fields" );
		if ( it != changed_attributes.end() )
		{
			csvFields = it->second.Get<Rml::Core::String>();
			Rml::Core::StringUtilities::ExpandString( fields, csvFields );
			dirty_query = true;
		}

		it = changed_attributes.find( "formatter" );
		if ( it != changed_attributes.end() )
		{
			formatter = Rml::Controls::DataFormatter::GetDataFormatter( it->second.Get<Rml::Core::String>() );
			dirty_query = true;
		}
	}

	void OnPropertyChange( const Rml::Core::PropertyIdSet &changed_properties )
	{
		Rml::Core::Element::OnPropertyChange( changed_properties );
		if ( changed_properties.Contains( Rml::Core::PropertyId::Width ) || changed_properties.Contains( Rml::Core::PropertyId::Height ) )
		{
			dirty_layout = true;
		}

		if ( changed_properties.Contains( UnvPropertyId::Radius ) )
		{
			radius = GetProperty( UnvPropertyId::Radius )->Get<float>();
		}

	}

	void OnRowAdd( Rml::Controls::DataSource*, const Rml::Core::String&, int, int )
	{
		dirty_query = true;
	}

	void OnRowChange( Rml::Controls::DataSource*, const Rml::Core::String&, int, int )
	{
		dirty_query = true;
	}

	void OnRowChange( Rml::Controls::DataSource*, const Rml::Core::String& )
	{
		dirty_query = true;
	}

	void OnRowRemove( Rml::Controls::DataSource*, const Rml::Core::String&, int, int )
	{
		dirty_query = true;
	}


	// Checks if parents are visible as well
	bool IsTreeVisible()
	{
		if ( IsVisible() )
		{
			Rml::Core::Element *parent = this;

			while ( ( parent = parent->GetParentNode() ) )
			{
				if ( !parent->IsVisible() )
				{
					return false;
				}
			}

			return true;
		}

		else
		{
			return false;
		}
	}



	void OnUpdate()
	{
		// Only do layout if element is visible
		// Positions calcs are not correct if element
		// is not visible.
		if ( dirty_query && IsTreeVisible() )
		{
			dirty_query = false;

			while ( HasChildNodes() )
			{

				RemoveChild( GetFirstChild() );
			}

			AddCancelbutton();

			Rml::Controls::DataQuery query( data_source, data_table, csvFields, 0, -1 );
			int index = 0;

			while ( query.NextRow() )
			{
				Rml::Core::StringList raw_data;
				Rml::Core::String out;

				for ( auto &&field : fields )
				{
					raw_data.emplace_back( query.Get<Rml::Core::String>( field, "" ) );
				}

				raw_data.push_back( va( "%d", index++ ) );

				if ( formatter )
				{
					formatter->FormatData( out, raw_data );
				}

				else
				{
					for ( size_t i = 0; i < raw_data.size(); ++i )
					{
						if ( i > 0 )
						{
							out.append( "," );
						}

						out.append( raw_data[ i ] );
					}
				}

				Rml::Core::Factory::InstanceElementText( this, out );
			}

			LayoutChildren();
		}
	}

	virtual void ProcessDefaultAction( Rml::Core::Event& event )
	{
		Element::ProcessDefaultAction( event );
		ProcessEvent( event );
	}

	virtual void ProcessEvent( Rml::Core::Event &event )
	{
		if ( event == "mouseover" )
		{
			Rml::Core::Element *parent = event.GetTargetElement();
			Rml::Core::Element *button = parent->GetTagName() == "button" ? parent : nullptr;
			while ( ( parent = parent->GetParentNode() ) )
			{
				if ( !button && parent->GetTagName() == "button" )
				{
					button = parent;
					continue;
				}

				if ( parent == this )
				{
					Rml::Core::Dictionary parameters;
					int i = 0;

					for ( i = 1; i < GetNumChildren(); ++i )
					{
						if ( GetChild( i ) == button )
						{
							parameters[ "index" ] = button->GetAttribute< int >( "alienclass", 0 );
							parameters[ "datasource" ] = data_source->GetDataSourceName();
							parameters[ "table" ] = data_table;

							DispatchEvent( "rowselect", parameters );
							break;
						}
					}

					break;
				}
			}
		}
		if ( event.GetTargetElement() == GetFirstChild() )
		{
			if ( event == "click" )
			{
				DispatchEvent( "cancel", Rml::Core::Dictionary() );
			}
		}
	}

protected:
	void LayoutChildren()
	{
		dirty_layout = false;

		int numChildren = 0;
		float width, height;
		Rml::Core::Element *child;
		Rml::Core::Vector2f offset = GetRelativeOffset();

		// First child is the cancel button. It should go in the center.
		child = GetFirstChild();
		width = child->GetOffsetWidth();
		height = child->GetOffsetHeight();
		child->SetProperty( "position", "absolute" );
		child->SetProperty( "top", va( "%fpx", offset.y + ( dimensions.y / 2 ) - ( height / 2 ) ) );
		child->SetProperty( "left", va( "%fpx", offset.x + ( dimensions.x / 2 ) - ( width / 2 ) ) );

		// No other children
		if ( ( numChildren = GetNumChildren() ) <= 1 )
		{
			return;
		}

		float angle = 360.0f / ( numChildren - 1 );

		// Rest are the circular buttons
		for ( int i = 1; i < numChildren; ++i )
		{
			child = GetChild( i );
			width = child->GetOffsetWidth();
			height = child->GetOffsetHeight();
			float y = sin( angle * ( i - 1 ) * ( M_PI / 180.0f ) ) * radius;
			float x = cos( angle * ( i - 1 ) * ( M_PI / 180.0f ) ) * radius;

			// This gets 12px on 1920×1080 screen, which is libRocket default for 1em
			int fontSize = std::min(cgs.glconfig.vidWidth, cgs.glconfig.vidHeight) / 90;

			child->SetProperty( "position", "absolute" );
			child->SetProperty( "left", va( "%fpx", ( dimensions.x / 2 ) - ( width / 2 ) + offset.x + x * fontSize ) );
			child->SetProperty( "top", va( "%fpx", ( dimensions.y / 2 ) - ( height / 2 ) + offset.y + y * fontSize ) );
		}
	}

private:

	void AddCancelbutton()
	{
		init = true;
		Rml::Core::Factory::InstanceElementText( this, "<button>Cancel</button>" );
		GetFirstChild()->SetClass( "cancelButton", true );
		GetFirstChild()->AddEventListener( "click", this );
	}

	bool dirty_query;
	bool dirty_layout;
	bool init;
	float radius;
	Rml::Controls::DataFormatter *formatter;
	Rml::Controls::DataSource *data_source;

	Rml::Core::String data_table;
	Rml::Core::String csvFields;
	Rml::Core::StringList fields;
	Rml::Core::Vector2f dimensions;
};

#endif
