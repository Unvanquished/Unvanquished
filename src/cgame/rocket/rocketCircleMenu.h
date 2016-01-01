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

#ifndef ROCKETCIRCLEMENU_H
#define ROCKETCIRCLEMENU_H

#include <Rocket/Core.h>
#include <Rocket/Core/Element.h>
#include <Rocket/Controls.h>
#include <Rocket/Controls/DataSourceListener.h>
#include <Rocket/Controls/DataSource.h>
#include "../cg_local.h"
#include "rocket.h"

class RocketCircleMenu : public Rocket::Core::Element, public Rocket::Controls::DataSourceListener, Rocket::Core::EventListener
{
public:
	RocketCircleMenu( const Rocket::Core::String &tag ) :
		Rocket::Core::Element( tag ),
		dirty_query( false ),
		dirty_layout( false ),
		init( false ),
		startAngle( 0 ),
		endAngle( 360 ),
		formatter( nullptr ),
		data_source( nullptr )
	{
	}

	virtual ~RocketCircleMenu()
	{
		if ( data_source )
		{
			data_source->DetachListener( this );
		}
	}

	void SetDataSource( const Rocket::Core::String &dsn )
	{
		ParseDataSource( data_source, data_table, dsn );
		data_source->AttachListener( this );
		dirty_query = true;
	}

	bool GetIntrinsicDimensions( Rocket::Core::Vector2f &dimensions )
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
			Rocket::Core::Element *parent = GetParentNode();
			if ( parent != nullptr )
			{
				dimensions.x = ResolveProperty( "width", parent->GetBox().GetSize().x );
			}
		}

		property = GetProperty( "height" );
		if ( property->unit & Rocket::Core::Property::ABSOLUTE_UNIT )
		{
			dimensions.y = property->value.Get<float>();
		}
		else
		{
			Rocket::Core::Element *parent = GetParentNode();
			if ( parent != nullptr )
			{
				dimensions.y = ResolveProperty( "height", parent->GetBox().GetSize().y );
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

	void OnAttributeChange( const Rocket::Core::AttributeNameList &changed_attributes )
	{
		Rocket::Core::Element::OnAttributeChange( changed_attributes );
		if ( changed_attributes.find( "source" ) != changed_attributes.end() )
		{
			SetDataSource( GetAttribute<Rocket::Core::String>( "source", "" ) );
		}

		if ( changed_attributes.find( "fields" ) != changed_attributes.end() )
		{
			csvFields = GetAttribute<Rocket::Core::String>( "fields", "" );
			Rocket::Core::StringUtilities::ExpandString( fields, csvFields );
			dirty_query = true;
		}

		if ( changed_attributes.find( "formatter" ) != changed_attributes.end() )
		{
			formatter = Rocket::Controls::DataFormatter::GetDataFormatter( GetAttribute( "formatter" )->Get<Rocket::Core::String>() );
			dirty_query = true;
		}
	}

	void OnPropertyChange( const Rocket::Core::PropertyNameList &changed_properties )
	{
		Rocket::Core::Element::OnPropertyChange( changed_properties );
		if ( changed_properties.find( "width" ) != changed_properties.end() || changed_properties.find( "height" ) != changed_properties.end() )
		{
			dirty_layout = true;
		}

		if ( changed_properties.find( "start-angle" ) != changed_properties.end() )
		{
			startAngle = GetProperty<int>( "start-angle" );
			dirty_layout = true;
		}

		if ( changed_properties.find ( "end-angle" ) != changed_properties.end() )
		{
			endAngle = GetProperty<int>( "end-angle" );
			dirty_layout = true;
		}
	}

	void OnRowAdd( Rocket::Controls::DataSource*, const Rocket::Core::String&, int, int )
	{
		dirty_query = true;
	}

	void OnRowChange( Rocket::Controls::DataSource*, const Rocket::Core::String&, int, int )
	{
		dirty_query = true;
	}

	void OnRowChange( Rocket::Controls::DataSource*, const Rocket::Core::String& )
	{
		dirty_query = true;
	}

	void OnRowRemove( Rocket::Controls::DataSource*, const Rocket::Core::String&, int, int )
	{
		dirty_query = true;
	}


	// Checks if parents are visible as well
	bool IsTreeVisible()
	{
		if ( IsVisible() )
		{
			Rocket::Core::Element *parent = this;

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

			Rocket::Controls::DataQuery query( data_source, data_table, csvFields, 0, -1 );
			int index = 0;

			while ( query.NextRow() )
			{
				Rocket::Core::StringList raw_data;
				Rocket::Core::String out;

				for ( size_t i = 0; i < fields.size(); ++i )
				{
					raw_data.push_back( query.Get<Rocket::Core::String>( fields[ i ], "" ) );
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
							out.Append( "," );
						}

						out.Append( raw_data[ i ] );
					}
				}

				Rocket::Core::Factory::InstanceElementText( this, out );
			}

			LayoutChildren();
		}
	}

	virtual void ProcessEvent( Rocket::Core::Event &event )
	{
		Element::ProcessEvent( event );
		if ( event == "mouseover" )
		{
			Rocket::Core::Element *parent = event.GetTargetElement();
			Rocket::Core::Element *button = parent->GetTagName() == "button" ? parent : nullptr;
			while ( ( parent = parent->GetParentNode() ) )
			{
				if ( !button && parent->GetTagName() == "button" )
				{
					button = parent;
					continue;
				}

				if ( parent == this )
				{
					Rocket::Core::Dictionary parameters;
					int i = 0;

					for ( i = 1; i < GetNumChildren(); ++i )
					{
						if ( GetChild( i ) == button )
						{
							parameters.Set( "index", va( "%d", i - 1 ) );
							parameters.Set( "datasource", data_source->GetDataSourceName() );
							parameters.Set( "table", data_table );

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
				DispatchEvent( "cancel", Rocket::Core::Dictionary() );
			}
		}
	}

protected:
	void LayoutChildren()
	{
		dirty_layout = false;

		int numChildren = 0;
		float width, height;
		Rocket::Core::Element *child;
		Rocket::Core::Vector2f offset = GetRelativeOffset();
		float radiusX = dimensions.x / 2.0f;
		float radiusY = dimensions.y / 2.0f;

		// First child is the cancel button. It should go in the center.
		child = GetFirstChild();
		Rocket::Core::Vector2f childSize = child->GetBox().GetSize();
		width = childSize.x;
		height = childSize.y;
		child->SetProperty( "position", "absolute" );
		child->SetProperty( "top", va( "%fpx", offset.y + ( dimensions.y / 2 ) - ( height / 2 ) ) );
		child->SetProperty( "left", va( "%fpx", offset.x + ( dimensions.x / 2 ) - ( width / 2 ) ) );

		// No other children
		if ( ( numChildren = GetNumChildren() ) <= 1 )
		{
			return;
		}

		float angle = ( endAngle - startAngle ) / ( numChildren - 1 );

		// Rest are the circular buttons
		for ( int i = 1; i < numChildren; ++i )
		{
			child = GetChild( i );
			childSize = child->GetBox().GetSize();
			width = childSize.x;
			height = childSize.y;
			float y = sin( ( ( angle * ( i - 1 ) ) + startAngle ) * ( M_PI / 180.0f ) ) * ( radiusY - ( height / 2.0f ) );
			float x = cos( ( ( angle * ( i - 1 ) ) + startAngle ) * ( M_PI / 180.0f ) ) * ( radiusX - ( width / 2.0f ) );

			child->SetProperty( "position", "absolute" );
			child->SetProperty( "left", va( "%fpx", ( dimensions.x / 2 ) - ( width / 2 ) + offset.x + x ) );
			child->SetProperty( "top", va( "%fpx", ( dimensions.y / 2 ) - ( height / 2 ) + offset.y + y ) );
		}
	}

private:

	void AddCancelbutton()
	{
		init = true;
		Rocket::Core::Factory::InstanceElementText( this, "<button>Cancel</button>" );
		GetFirstChild()->SetClass( "cancelButton", true );
		GetFirstChild()->AddEventListener( "click", this );
	}

	bool dirty_query;
	bool dirty_layout;
	bool init;
	int startAngle;
	int endAngle;
	Rocket::Controls::DataFormatter *formatter;
	Rocket::Controls::DataSource *data_source;

	Rocket::Core::String data_table;
	Rocket::Core::String csvFields;
	Rocket::Core::StringList fields;
	Rocket::Core::Vector2f dimensions;
};

#endif
