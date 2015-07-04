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

#include "rocket.h"
#include <Rocket/Controls.h>
#include "rocketDataGrid.h"
#include <string>
#include <map>

// Code to interface with libRocket's datagrids for displaying
// tabulated information nicely

typedef std::map<std::string, RocketDataGrid*> StringGridMap_t;
StringGridMap_t dataSourceMap;

void Rocket_RegisterDataSource( const char *name )
{
	dataSourceMap[ name ] = new RocketDataGrid( name );
}

static RocketDataGrid *FindDataSource( const char *name )
{
	StringGridMap_t::iterator it = dataSourceMap.find( name );

	if ( it == dataSourceMap.end() )
	{
		return nullptr;
	}

	return it->second;
}

void Rocket_DSAddRow( const char *name, const char *table, const char *data )
{
	RocketDataGrid *ds = FindDataSource( name );

	if ( !ds )
	{
		Com_Printf( "^1ERROR: ^7Rocket_DSAddRow: data source %s does not exist.\n", name );
		return;
	}

	ds->AddRow( table, data );
}

void Rocket_DSChangeRow( const char *name, const char *table, const int row, const char *data )
{
	RocketDataGrid *ds = FindDataSource( name );

	if ( !ds )
	{
		Com_Printf( "^1ERROR: ^7Rocket_DSChangeRow: data source %s does not exist.\n", name );
		return;
	}

	ds->ChangeRow( table, row, data );
}

void Rocket_DSRemoveRow( const char *name, const char *table, const int row )
{
	RocketDataGrid *ds = FindDataSource( name );

	if ( !ds )
	{
		Com_Printf( "^1ERROR: ^7Rocket_DSRemoveRow: data source %s does not exist.\n", name );
		return;
	}

	ds->RemoveRow( table, row );
}

void Rocket_DSClearTable( const char *name, const char *table )
{
	RocketDataGrid *ds = FindDataSource( name );

	if ( !ds )
	{
		Com_Printf( "^1ERROR: ^7Rocket_DSClearTable: data source %s does not exist.\n", name );
		return;
	}

	ds->ClearTable( table );
}
