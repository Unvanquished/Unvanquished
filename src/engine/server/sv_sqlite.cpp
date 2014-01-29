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

// Interface for using SQL commands by the VM
#ifdef USE_SQLITE

#include "server.h"
#include <sqlite3.h>

std::vector<sqlite3*> databases;

/**
 * SV_SQL_Open
 *
 * Returns opens database and returns handle to a database
 * */
int SV_SQL_Open( const char *dbName )
{
	sqlite3 *db;

	// HACK: Make sure we don't try to leave the home dir
	// FIXME: Will probably need to be updated with filesystem update
	if ( Q_stristr( dbName, ".." ) )
	{
		return -1;
	}

	const char *o = Cvar_VariableString( "fs_game");
	const char *p = va( "%s/%s/%s", Cvar_VariableString( "fs_homepath" ), *o ? o : "main" , dbName );

	int ret = sqlite3_open_v2( p, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE, NULL);

	if ( ret )
	{
		Com_Printf( "SV_SQL_Open: Error openning database %s: %s\n", dbName, sqlite3_errmsg( db ) );
		return -1;
	}

	// Prevents gamelogic from attaching further databases from arbitrary pathes
	sqlite3_limit( db, SQLITE_LIMIT_ATTACHED, 0 );

	databases.push_back( db );

	return databases.size() - 1;
}
/**
 * SV_SQL_Exec
 *
 * Execute a given sql statement on db. if len is zero, do not return output
 */
int SV_SQL_Exec( int dbHandle, const char *sql, char *out, int len )
{
	sqlite3 *db = databases[ dbHandle ];
	sqlite3_stmt *statement;
	int numItems = 0;

	if ( !sql )
	{
		return -1;
	}

	int ret = sqlite3_prepare_v2( db, sql, strlen( sql ), &statement, NULL );

	if ( ret != SQLITE_OK )
	{
		Com_Printf( "SV_SQL_Exec: Error preparing statement \"%s\": %s\n", sql, sqlite3_errmsg( db ) );
		return -1;
	}

	ret = sqlite3_step( statement );

	if ( ret != SQLITE_ROW && ret != SQLITE_DONE )
	{
		Com_Printf( "SV_SQL_Exec: Error executing statement \"%s\": %s\n", sql, sqlite3_errmsg( db ) );
		sqlite3_finalize( statement );
		return -1;
	}

	// We want some output back
	if ( len > 0 )
	{
		char *p = out;
		int length = len;
		int numColumns = sqlite3_column_count( statement );

		while ( 1 )
		{
			if ( ret == SQLITE_DONE )
			{
				break;
			}

			if ( ret != SQLITE_ROW )
			{
				Com_Printf( "SV_SQL_Exec: Error executing statement \"%s\": %s\n", sql, sqlite3_errmsg( db ) );
				sqlite3_finalize( statement );
				return -1;
			}

			// Info string not needed
			if ( numColumns == 1 )
			{
				int newlen;
				Q_strncpyz( p, ( const char * )sqlite3_column_text( statement, 0 ), length );
				newlen = strlen( p );
				length -= newlen;
				p += strlen( p );
				*p++ = '\n';
			}

			else
			{
				for ( int i = 0; i < numColumns; ++i )
				{
					const char *c = va( "\\\\%s\\\\%s", sqlite3_column_name( statement, i ), sqlite3_column_text( statement, i ) );
					int newlen;
					Q_strcat( p, length, c );
					newlen = strlen( c );
					length -= newlen;
					p += newlen;
				}
				*p++ = '\n';
			}

			numItems++;
			ret = sqlite3_step( statement );
		}
	}

	sqlite3_finalize( statement );
	return numItems;
}

void SV_SQL_Close( int dbHandle )
{
	sqlite3_close( databases[ dbHandle ] );
}
#endif
