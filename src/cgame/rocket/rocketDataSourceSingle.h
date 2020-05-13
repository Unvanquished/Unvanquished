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
#include <RmlUi/Controls.h>
#include <RmlUi/Controls/DataSource.h>

class RocketDataSourceSingle : public Rml::Core::Element, public Rml::Controls::DataSourceListener, public Rml::Core::EventListener
{
public:
	RocketDataSourceSingle( const Rml::Core::String &tag ) : Rml::Core::Element( tag ), formatter( nullptr ), data_source( nullptr ), selection( -1 ),
	targetElement( nullptr ), dirty_query( false ), dirty_listener( false ) { }

	void OnAttributeChange( const Rml::Core::ElementAttributes &changed_attributes )
	{
		Rml::Core::Element::OnAttributeChange( changed_attributes );
		auto it = changed_attributes.find( "source" );
		if ( it != changed_attributes.end() )
		{
			ParseDataSource( data_source, data_table, it->second.Get<Rml::Core::String>() );
			dirty_query = true;
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
		if ( changed_attributes.count( "targetid" ) || changed_attributes.count( "targetdoc" ) )
		{
			dirty_listener = true;
		}
	}

	void ProcessDefaultAction( Rml::Core::Event& event ) override
	{
		Element::ProcessDefaultAction( event );
	}

	void ProcessEvent( Rml::Core::Event &evt ) override
	{
		// Make sure it is meant for the element we are listening to
		if ( evt == "rowselect" && targetElement == evt.GetTargetElement() )
		{
			const Rml::Core::Dictionary& parameters = evt.GetParameters();
			selection = parameters.at("index").Get<int>( -1 );
			dirty_query = true;
		}

	}

	void OnUpdate()
	{
		if ( dirty_listener )
		{
			Rml::Core::ElementDocument *document;
			Rml::Core::String td;

			if (  ( td = GetAttribute<Rml::Core::String>( "targetdoc", "" ) ).empty() )
			{
				document = GetOwnerDocument();
			}
			else
			{
				document = GetContext()->GetDocument( td );
			}

			if ( document )
			{
				Rml::Core::Element *element;

				if ( ( element = document->GetElementById( GetAttribute<Rml::Core::String>( "targetid", "" ) ) ) )
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
			Rml::Controls::DataQuery query( data_source, data_table, csvFields, selection, 1 );
			Rml::Core::StringList raw_data;
			Rml::Core::String out_data;

			query.NextRow();

			for ( size_t i = 0; i < fields.size(); ++i )
			{
				raw_data.push_back( query.Get<Rml::Core::String>( fields[ i ], "" ) );
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
	Rml::Controls::DataFormatter *formatter;
	Rml::Controls::DataSource *data_source;
	int selection;
	Rml::Core::String data_table;
	Rml::Core::String csvFields;
	Rml::Core::StringList fields;
	Rml::Core::Element *targetElement;
	bool dirty_query;
	bool dirty_listener;
};

#endif
