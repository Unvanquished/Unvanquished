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

#ifndef ROCKETDATAGRID_H
#define ROCKETDATAGRID_H

#include "../cg_local.h"
#include "rocket.h"
#include <RmlUi/Core.h>
#include <RmlUi/Core/Elements/DataSource.h>

class RocketDataGrid : public Rml::DataSource
{
public:
	RocketDataGrid( const char *name ) : Rml::DataSource( name ) { }
	~RocketDataGrid() { }
	void GetRow( Rml::StringList& row, const Rml::String& table, int row_index, const Rml::StringList& columns )
	{
		if ( data.find( table ) == data.end() || data[table].size() <= (unsigned) row_index )
		{
			return;
		}

		for ( auto &&column : columns )
		{
			row.emplace_back( Rocket_QuakeToRML( Info_ValueForKey( data[ table ][ row_index ].c_str(), column.c_str() ), RP_EMOTICONS ) );
		}
	}

	int GetNumRows( const Rml::String& table )
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
	std::map<Rml::String, std::vector<Rml::String> > data;
};

#endif
