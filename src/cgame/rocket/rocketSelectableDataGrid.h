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

#ifndef ROCKETSELECTABLEDATAGRID_H
#define ROCKETSELECTABLEDATAGRID_H
#include "../cg_local.h"
#include "rocket.h"
#include <RmlUi/Core.h>

class SelectableDataGrid : public Rml::ElementDataGrid, public Rml::EventListener
{
public:
	SelectableDataGrid( const Rml::String& tag ) :
		Rml::ElementDataGrid(tag), lastSelectedRow( nullptr ), lastSelectedRowIndex( -1 )
	{
		SetAttribute("selected-row", "-1");
	}

	~SelectableDataGrid()
	{
	}

	void OnChildAdd( Element *child ) override
	{
		ElementDataGrid::OnChildAdd( child );
		if ( child == this )
		{
			this->AddEventListener( Rml::EventId::Rowremove, this );
			this->AddEventListener( Rml::EventId::Rowadd, this );
		}
	}

	/// Called for every event sent to this element or one of its descendants.
	/// @param[in] event The event to process.
	void ProcessDefaultAction( Rml::Event& evt ) override
	{
		extern std::queue< RocketEvent_t* > eventQueue;
		Rml::String dataSource = GetAttribute<Rml::String>( "source", "" );

		ElementDataGrid::ProcessDefaultAction( evt );

		if( evt == Rml::EventId::Click || evt == Rml::EventId::Dblclick )
		{
			Element* elem;
			int column = -1;
			Rml::String dsName = dataSource.substr( 0, dataSource.find( "." ) );
			Rml::String tableName =  dataSource.substr( dataSource.find( "." ) + 1, dataSource.size() );


			// get the column index
			elem = evt.GetTargetElement();
			while( elem && elem->GetTagName() != "datagridcell" && elem->GetTagName() != "datagridcolumn" ) {
				elem = elem->GetParentNode();
			}
			if( elem ) {
				// "datagridcolumn" points to just Element, so figure out the index by iterating
				// FIXME: We could be little smarter with this and get the column definition here too
				// and use colselect or colactivate events
				if( elem->GetTagName() == "datagridcolumn" ) {
					Rml::Element* child = elem->GetParentNode()->GetFirstChild();
					column = 0;
					while( child && child != elem ) {
						child = child->GetNextSibling();
						column++;
					}
				}
				else {
					column = static_cast<Rml::ElementDataGridCell *>( elem )->GetColumn();
				}
			}

			// get the row element
			elem = evt.GetTargetElement();
			while( elem && elem->GetTagName() != "datagridrow" && elem->GetTagName() != "datagridheader" ) {
				elem = elem->GetParentNode();
			}

			if( elem )
			{
				Rml::ElementDataGridRow *row = dynamic_cast<Rml::ElementDataGridRow*>( elem );
				int index = row->GetTableRelativeIndex();
				Rml::String indexStr( va( "%d", index ) );

				// this should never happen
				if( index >= this->GetNumRows() )
					return;
				if( index >= 0 )
				{
					// deselect last selected row
					if( lastSelectedRow != row )
					{
						if( lastSelectedRow ) {
							lastSelectedRow->SetPseudoClass( "selected", false );
						}
					}

					// select clicked row
					lastSelectedRow = row;
					lastSelectedRowIndex = index;
					SetAttribute( "selected-row", std::to_string( lastSelectedRowIndex ) );

					row->SetPseudoClass( "selected", true );
					eventQueue.push( new RocketEvent_t( Rml::String( va ( "setDS %s %s %d", dsName.c_str(),tableName.c_str(), index ) ) ) );
				}

				Rml::Dictionary parameters;
				parameters[ "index" ] = indexStr;
				parameters[ "column_index" ] = column;
				parameters[ "datasource" ] = dsName;
				parameters[ "table" ] = tableName;

				if( evt == Rml::EventId::Click )
					DispatchEvent( "rowselect", parameters );
				else
					DispatchEvent( "rowactivate", parameters );
			}

			if( evt == Rml::EventId::Dblclick )
			{
				eventQueue.push( new RocketEvent_t( Rml::String( va ( "execDS %s %s", dataSource.substr( 0, dataSource.find( "." ) ).c_str(), tableName.c_str() ) ) ) );
			}
		}
	}

	void ProcessEvent( Rml::Event &evt ) override
	{
		if ( evt == Rml::EventId::Rowremove )
		{
			int numRowsRemoved = evt.GetParameter< int >("num_rows_removed", 0);
			if( !numRowsRemoved ) {
				return;
			}

			int firstRowRemoved = evt.GetParameter< int >("first_row_removed", 0);
			if( lastSelectedRowIndex >= firstRowRemoved && lastSelectedRowIndex < firstRowRemoved + numRowsRemoved ) {
				lastSelectedRow = nullptr;

				lastSelectedRowIndex = -1;
				SetAttribute( "selected-row", "-1" );
			}
			else if ( lastSelectedRowIndex >= firstRowRemoved )
			{
				lastSelectedRowIndex -= numRowsRemoved;
				SetAttribute( "selected-row", std::to_string( lastSelectedRowIndex ) );
			}
		}
		else if ( evt == Rml::EventId::Rowadd )
		{
			if ( !lastSelectedRow ) return;

			int numRowsAdded = evt.GetParameter<int>( "num_rows_added", 0 );
			int firstRowAdded = evt.GetParameter<int>( "first_row_added", 0 );
			if ( firstRowAdded < lastSelectedRowIndex )
			{
				lastSelectedRowIndex += numRowsAdded;
				SetAttribute( "selected-row", std::to_string( lastSelectedRowIndex ) );
			}
		}
	}

private:
	Rml::Element *lastSelectedRow;
	int lastSelectedRowIndex;
};
#endif
