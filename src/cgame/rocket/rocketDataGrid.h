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

#ifndef ROCKETDATAGRID_H
#define ROCKETDATAGRID_H

#include "../cg_local.h"
#include "rocket.h"
#include <Rocket/Controls/DataSource.h>
#include <Rocket/Core/Types.h>

class RocketDataGrid : public Rocket::Controls::DataSource
{
public:
	RocketDataGrid( const char *name ) : Rocket::Controls::DataSource( name ) { }
	~RocketDataGrid() { }
	void GetRow( Rocket::Core::StringList& row, const Rocket::Core::String& table, int row_index, const Rocket::Core::StringList& columns )
	{
		if ( data.find( table ) == data.end() || data[table].size() <= (unsigned) row_index )
		{
			return;
		}

		for ( size_t i = 0; i < columns.size(); ++i )
		{
			row.push_back( Rocket_QuakeToRML( Info_ValueForKey( data[ table ][ row_index ].CString(), columns[ i ].CString() ), RP_EMOTICONS ) );
		}
	}

	int GetNumRows( const Rocket::Core::String& table )
	{
		return data[ table ].size();
	}

	void AddRow( const char *table, const char *dataIn )
	{
		data[ table ].push_back( dataIn );
		NotifyRowAdd( table, data[ table ].size() - 1, 1 );
	}

	void ChangeRow( const char *table, const int row, const char *dataIn )
	{
		data[ table ][ row ] = dataIn;
		NotifyRowChange( table, row, 1 );
	}

	void RemoveRow( const char *table, const int row )
	{
		data[ table ].erase( data[ table ].begin() + row - 1 );
		NotifyRowRemove( table, row, 1 );
	}

	void ClearTable( const char *table )
	{
		data.erase( table );
		NotifyRowChange( table );
	}


private:
	std::map<Rocket::Core::String, std::vector<Rocket::Core::String> > data;
};

#endif
