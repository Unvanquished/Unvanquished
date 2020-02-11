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
#include <RmlUi/Controls.h>

class SelectableDataGrid : public Rml::Controls::ElementDataGrid
{
public:
	SelectableDataGrid( const Rml::Core::String& tag ) :
		Rml::Controls::ElementDataGrid(tag), lastSelectedRow( nullptr ), lastSelectedRowIndex( -1 )
	{
		SetProperty( "selected-row", "-1" );
	}

	~SelectableDataGrid()
	{
		if( lastSelectedRow != nullptr ) {
			lastSelectedRow->RemoveReference();
		}
	}

	/// Called for every event sent to this element or one of its descendants.
	/// @param[in] event The event to process.
	void ProcessEvent( Rml::Core::Event& evt )
	{
		extern std::queue< RocketEvent_t* > eventQueue;
		Rml::Core::String dataSource = GetAttribute<Rml::Core::String>( "source", "" );

		ElementDataGrid::ProcessEvent( evt );

		if( evt == "click" || evt == "dblclick" )
		{
			Element* elem;
			int column = -1;
			Rml::Core::String dsName = dataSource.Substring( 0, dataSource.Find( "." ) );
			Rml::Core::String tableName =  dataSource.Substring( dataSource.Find( "." ) + 1, dataSource.Length() );


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
					Rml::Core::Element* child = elem->GetParentNode()->GetFirstChild();
					column = 0;
					while( child && child != elem ) {
						child = child->GetNextSibling();
						column++;
					}
				}
				else {
					column = static_cast<Rml::Controls::ElementDataGridCell *>( elem )->GetColumn();
				}
			}

			// get the row element
			elem = evt.GetTargetElement();
			while( elem && elem->GetTagName() != "datagridrow" && elem->GetTagName() != "datagridheader" ) {
				elem = elem->GetParentNode();
			}

			if( elem )
			{
				Rml::Controls::ElementDataGridRow *row = dynamic_cast<Rml::Controls::ElementDataGridRow*>( elem );
				int index = row->GetTableRelativeIndex();
				Rml::Core::String indexStr( va( "%d", index ) );

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
							lastSelectedRow->RemoveReference();
						}
					}

					// select clicked row
					lastSelectedRow = row;
					lastSelectedRowIndex = index;

					this->SetProperty( "selected-row", indexStr );

					row->SetPseudoClass( "selected", true );
					row->AddReference();



					eventQueue.push( new RocketEvent_t( Rml::Core::String( va ( "setDS %s %s %d", dsName.c_str(),tableName.c_str(), index ) ) ) );
				}

				Rml::Core::Dictionary parameters;
				parameters.Set( "index", indexStr );
				parameters.Set( "column_index", column );
				parameters.Set( "datasource", dsName );
				parameters.Set( "table", tableName );

				if( evt == "click" )
					DispatchEvent( "rowselect", parameters );
				else
					DispatchEvent( "rowactivate", parameters );
			}

			if( evt == "dblclick" )
			{
				eventQueue.push( new RocketEvent_t( Rml::Core::String( va ( "execDS %s %s", dataSource.Substring( 0, dataSource.Find( "." ) ).c_str(), tableName.c_str() ) ) ) );
			}
		}
		else if( evt == "rowremove" )
		{
			int numRowsRemoved = evt.GetParameter< int >("num_rows_removed", 0);
			if( !numRowsRemoved ) {
				return;
			}

			int firstRowRemoved = evt.GetParameter< int >("first_row_removed", 0);
			if( lastSelectedRowIndex >= firstRowRemoved && lastSelectedRowIndex < firstRowRemoved + numRowsRemoved ) {
				lastSelectedRow->RemoveReference();
				lastSelectedRow = nullptr;

				lastSelectedRowIndex = -1;
				this->SetProperty( "selected-row", "-1" );
			}
		}

	}

private:
	Rml::Core::Element *lastSelectedRow;
	int lastSelectedRowIndex;
};
#endif
