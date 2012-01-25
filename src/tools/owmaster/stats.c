/*
stats.c

Statistics for tremmaster

Copyright (C) 2009 Darklegion Development

This program is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#ifndef _WIN32

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <tdb.h>

#include "common.h"

#define MAX_DATA_SIZE 1024
#define CS_FILENAME   "clientStats.tdb"

/*
====================
RecordClientStat
====================
*/
void RecordClientStat( const char *address, const char *version, const char *renderer )
{
  TDB_CONTEXT *tctx = NULL;
  TDB_DATA    key, data;
  char        ipText[ 22 ];
  char        dataText[ MAX_DATA_SIZE ] = { 0 };
  char        *p;
  int         i;

  tctx = tdb_open( CS_FILENAME, 0, 0, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR );

  if( !tctx )
  {
		MsgPrint( MSG_DEBUG, "Couldn't open %s\n", CS_FILENAME );
    return;
  }

  strncpy( ipText, address, 22 );
  if( ( p = strrchr( ipText, ':' ) ) ) // Remove port
    *p = '\0';

  key.dptr = ipText;
  key.dsize = strlen( ipText );

  strncat( dataText, "\"", MAX_DATA_SIZE );
  strncat( dataText, version, MAX_DATA_SIZE );

  // Remove last three tokens (the date)
  for( i = 0; i < 3; i++ )
  {
    if( ( p = strrchr( dataText, ' ' ) ) )
      *p = '\0';
  }
  strncat( dataText, "\"", MAX_DATA_SIZE );

  strncat( dataText, " \"", MAX_DATA_SIZE );
  strncat( dataText, renderer, MAX_DATA_SIZE );
  strncat( dataText, "\"", MAX_DATA_SIZE );

  data.dptr = dataText;
  data.dsize = strlen( dataText );

  if( tdb_store( tctx, key, data, 0 ) < 0 )
		MsgPrint( MSG_DEBUG, "tdb_store failed\n" );

  tdb_close( tctx );
	MsgPrint( MSG_DEBUG, "Recorded client stat for %s\n", address );
}

#define GS_FILENAME   "gameStats.tdb"

/*
====================
RecordGameStat
====================
*/
void RecordGameStat( const char *address, const char *dataText )
{
  TDB_CONTEXT *tctx = NULL;
  TDB_DATA    key, data;
  char        keyText[ MAX_DATA_SIZE ] = { 0 };
  char        *p;
  time_t      tm = time( NULL );

  tctx = tdb_open( GS_FILENAME, 0, 0, O_RDWR|O_CREAT, S_IRUSR|S_IWUSR );

  if( !tctx )
  {
		MsgPrint( MSG_DEBUG, "Couldn't open %s\n", GS_FILENAME );
    return;
  }

  strncpy( keyText, address, 22 );
  if( ( p = strrchr( keyText, ':' ) ) ) // Remove port
    *p = '\0';

  strncat( keyText, " ", MAX_DATA_SIZE );
  strncat( keyText, asctime( gmtime( &tm ) ), MAX_DATA_SIZE );

  key.dptr = keyText;
  key.dsize = strlen( keyText );

  data.dptr = (char *)dataText;
  data.dsize = strlen( dataText );

  if( tdb_store( tctx, key, data, 0 ) < 0 )
		MsgPrint( MSG_DEBUG, "tdb_store failed\n" );

  tdb_close( tctx );
	MsgPrint( MSG_NORMAL, "Recorded game stat from %s\n", address );
}

#endif
