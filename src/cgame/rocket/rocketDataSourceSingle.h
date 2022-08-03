/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2011 Cervesato Andrea ("koochi")
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

#ifndef ROCKETDATASOURCESINGLE_H
#define ROCKETDATASOURCESINGLE_H

#include "../cg_local.h"

#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/DataSource.h>

class RocketDataSourceSingle : public Rml::Element, public Rml::DataSourceListener, public Rml::EventListener
{
public:
	RocketDataSourceSingle( const Rml::String &tag ) : Rml::Element( tag ), formatter( nullptr ), data_source( nullptr ), selection( -1 ),
	targetElement( nullptr ), dirty_query( false ), dirty_listener( false ) { }

	void OnAttributeChange( const Rml::ElementAttributes &changed_attributes ) override
	{
		Rml::Element::OnAttributeChange( changed_attributes );
		auto it = changed_attributes.find( "source" );
		if ( it != changed_attributes.end() )
		{
			ParseDataSource( data_source, data_table, it->second.Get<Rml::String>() );
			dirty_query = true;
		}

		it = changed_attributes.find( "fields" );
		if ( it != changed_attributes.end() )
		{
			csvFields = it->second.Get<Rml::String>();
			Rml::StringUtilities::ExpandString( fields, csvFields );
			dirty_query = true;
		}

		it = changed_attributes.find( "formatter" );
		if ( it != changed_attributes.end() )
		{
			formatter = Rml::DataFormatter::GetDataFormatter( it->second.Get<Rml::String>() );
			dirty_query = true;
		}
		if ( changed_attributes.count( "targetid" ) || changed_attributes.count( "targetdoc" ) )
		{
			dirty_listener = true;
		}
	}

	void ProcessDefaultAction( Rml::Event& event ) override
	{
		Element::ProcessDefaultAction( event );
	}

	void ProcessEvent( Rml::Event &evt ) override
	{
		// Make sure it is meant for the element we are listening to
		if ( evt == "rowselect" && targetElement == evt.GetTargetElement() )
		{
			const Rml::Dictionary& parameters = evt.GetParameters();
			selection = parameters.at("index").Get<int>( -1 );
			dirty_query = true;
		}

	}

	void OnUpdate() override
	{
		if ( dirty_listener )
		{
			Rml::ElementDocument *document;
			Rml::String td;

			if (  ( td = GetAttribute<Rml::String>( "targetdoc", "" ) ).empty() )
			{
				document = GetOwnerDocument();
			}
			else
			{
				document = GetContext()->GetDocument( td );
			}

			if ( document )
			{
				Rml::Element *element;

				if ( ( element = document->GetElementById( GetAttribute<Rml::String>( "targetid", "" ) ) ) )
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
			Rml::DataQuery query( data_source, data_table, csvFields, selection, 1 );
			Rml::StringList raw_data;
			Rml::String out_data;

			query.NextRow();

			for ( auto &&field : fields )
			{
				raw_data.emplace_back( query.Get<Rml::String>( field, "" ) );
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
						out_data.append( "," );
					}

					out_data.append( raw_data[ i ] );
				}
			}

			SetInnerRML( out_data );

			dirty_query = false;
		}
	}





private:
	Rml::DataFormatter *formatter;
	Rml::DataSource *data_source;
	int selection;
	Rml::String data_table;
	Rml::String csvFields;
	Rml::StringList fields;
	Rml::Element *targetElement;
	bool dirty_query;
	bool dirty_listener;
};

#endif
