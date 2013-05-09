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
#ifndef ROCKETSELECTABLEDATAGRID_H
#define ROCKETSELECTABLEDATAGRID_H
extern "C"
{
#include "client.h"
}
#include <Rocket/Core.h>
#include <Rocket/Controls.h>
#include <boost/concept_check.hpp>

class SelectableDataGrid : public Rocket::Controls::ElementDataGrid
{
public:
	SelectableDataGrid( const Rocket::Core::String& tag ) :
		Rocket::Controls::ElementDataGrid(tag), lastSelectedRow( NULL ), lastSelectedRowIndex( -1 )
	{
		SetProperty( "selected-row", "-1" );
	}

	~SelectableDataGrid()
	{
		if( lastSelectedRow != NULL ) {
			lastSelectedRow->SetPseudoClass( "selected", false );
			lastSelectedRow->RemoveReference();
		}
	}

	/// Called for every event sent to this element or one of its descendants.
	/// @param[in] event The event to process.
	void ProcessEvent( Rocket::Core::Event& evt )
	{
		ElementDataGrid::ProcessEvent( evt );

		if( evt == "click" || evt == "dblclick" )
		{
			Element* elem;
			int column = -1;

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
					Rocket::Core::Element* child = elem->GetParentNode()->GetFirstChild();
					column = 0;
					while( child && child != elem ) {
						child = child->GetNextSibling();
						column++;
					}
				}
				else {
					column = static_cast<Rocket::Controls::ElementDataGridCell *>( elem )->GetColumn();
				}
			}

			// get the row element
			elem = evt.GetTargetElement();
			while( elem && elem->GetTagName() != "datagridrow" && elem->GetTagName() != "datagridheader" ) {
				elem = elem->GetParentNode();
			}

			if( elem )
			{
				Rocket::Controls::ElementDataGridRow *row = dynamic_cast<Rocket::Controls::ElementDataGridRow*>( elem );
				int index = row->GetTableRelativeIndex();
				Rocket::Core::String indexStr( va( "%d", index ) );

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
				}

				Rocket::Core::Dictionary parameters;
				parameters.Set( "index", indexStr );
				parameters.Set( "column_index", column );
				if( evt == "click" )
					DispatchEvent( "rowselect", parameters );
				else
					DispatchEvent( "rowactivate", parameters );
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
				lastSelectedRow = NULL;

				lastSelectedRowIndex = -1;
				this->SetProperty( "selected-row", "-1" );
			}
		}
		else if( evt == "dblclick" )
		{
		}
	}

private:
	Rocket::Core::Element *lastSelectedRow;
	int lastSelectedRowIndex;
};
#endif
