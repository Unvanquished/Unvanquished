/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2011 Cervesato Andrea ("koochi")
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

#ifndef ROCKETDATASOURCESINGLE_H
#define ROCKETDATASOURCESINGLE_H

extern "C"
{
#include "client.h"
}

#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include <Rocket/Controls/DataSource.h>

class RocketDataSourceSingle : public Rocket::Core::Element, public Rocket::Controls::DataSourceListener, public Rocket::Core::EventListener
{
public:
	RocketDataSourceSingle( const Rocket::Core::String &tag ) : Rocket::Core::Element( tag ), formatter( NULL ), data_source( NULL ), selection( -1 ),
	targetElement( NULL ), dirty_query( false ), dirty_listener( false ) { }

	void OnAttributeChange( const Rocket::Core::AttributeNameList &changed_attributes )
	{
		if ( changed_attributes.find( "source" ) != changed_attributes.end() )
		{
			ParseDataSource( data_source, data_table, GetAttribute( "source")->Get<Rocket::Core::String>() );
			dirty_query = true;
		}
		if ( changed_attributes.find( "fields" ) != changed_attributes.end() )
		{
			csvFields = GetAttribute( "fields" )->Get<Rocket::Core::String>();
			Rocket::Core::StringUtilities::ExpandString( fields, csvFields );
			dirty_query = true;
		}
		if ( changed_attributes.find( "formatter" ) != changed_attributes.end() )
		{
			formatter = Rocket::Controls::DataFormatter::GetDataFormatter( GetAttribute( "formatter" )->Get<Rocket::Core::String>() );
			dirty_query = true;
		}
		if ( changed_attributes.find( "targetid" ) != changed_attributes.end() || changed_attributes.find( "targetdoc" ) != changed_attributes.end() )
		{
			dirty_listener = true;
		}
	}

	void ProcessEvent( Rocket::Core::Event &evt )
	{
		Rocket::Core::Element::ProcessEvent( evt );

		if ( evt == "rowselect" )
		{
			const Rocket::Core::Dictionary *parameters = evt.GetParameters();

			if ( parameters->Get<Rocket::Core::String>( "datasource", "" ) == data_source->GetDataSourceName() && parameters->Get<Rocket::Core::String>( "table", "" ) == data_table )
			{
				selection = parameters->Get<int>( "index", -1 );
				dirty_query = true;
			}
		}

	}

	void OnUpdate( void )
	{
		if ( dirty_listener )
		{
			Rocket::Core::ElementDocument *document;
			Rocket::Core::String td;

			if (  ( td = GetAttribute<Rocket::Core::String>( "targetdoc", "" ) ).Empty() )
			{
				document = GetOwnerDocument();
			}
			else
			{
				document = GetContext()->GetDocument( td );
			}

			if ( document )
			{
				Rocket::Core::Element *element;

				if ( ( element = document->GetElementById( GetAttribute<Rocket::Core::String>( "targetid", "" ) ) ) )
				{
					if ( element != targetElement )
					{
						if ( targetElement )
						{
							targetElement->RemoveEventListener( "rowselect", this );
						}

						targetElement = element;
						targetElement->AddEventListener( "rowselect", this );
					}
				}
			}

			dirty_listener = false;
		}
		if ( dirty_query && selection >= 0 )
		{
			Rocket::Controls::DataQuery query( data_source, data_table, csvFields, selection, 1 );
			Rocket::Core::StringList raw_data;
			Rocket::Core::String out_data;

			query.NextRow();

			for ( size_t i = 0; i < fields.size(); ++i )
			{
				raw_data.push_back( query.Get<Rocket::Core::String>( fields[ i ], "" ) );
			}

			if ( formatter )
			{
				formatter->FormatData( out_data, raw_data );
			}
			else
			{
				for ( size_t i = 0; i < raw_data.size(); ++i )
				{
					if ( i > 0 )
					{
						out_data.Append( "," );
					}

					out_data.Append( raw_data[ i ] );
				}
			}

			SetInnerRML( out_data );

			dirty_query = false;
		}
	}





private:
	Rocket::Controls::DataFormatter *formatter;
	Rocket::Controls::DataSource *data_source;
	int selection;
	Rocket::Core::String data_table;
	Rocket::Core::String csvFields;
	Rocket::Core::StringList fields;
	Rocket::Core::Element *targetElement;
	bool dirty_query;
	bool dirty_listener;
};

#endif
