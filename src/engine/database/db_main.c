/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2009 SlackerLinux85 <SlackerLinux85@gmail.com>
Copyright (C) 2011 Dusan Jocic <dusanjocic@msn.com>

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "database.h"

#ifdef ET_MYSQL

//cvars
cvar_t *db_enable;
cvar_t *db_backend;
cvar_t *db_statusmaster;
cvar_t *db_statusslave;

// MySQL Master Server related
cvar_t *db_addressMaster;
cvar_t *db_portMaster;
cvar_t *db_usernameMaster;
cvar_t *db_passwordMaster;
cvar_t *db_databaseMaster;

// MySQL Slave Server Related
cvar_t *db_addressSlave;
cvar_t *db_portSlave;
cvar_t *db_usernameSlave;
cvar_t *db_passwordSlave;
cvar_t *db_databaseSlave;

#ifdef USE_HUB_SERVER
// TODO
#endif

static dbinterface_t dbi;

qboolean DB_ValidateInterface ( dbinterface_t *dbi )
{
	if ( !dbi->DBConnectMaster ) { return qfalse; }

	if ( !dbi->DBConnectSlave ) { return qfalse; }

	if ( !dbi->DBStatus ) { return qfalse; }

	if ( !dbi->DBDisconnect ) { return qfalse; }

	if ( !dbi->RunQuery ) { return qfalse; }

	if ( !dbi->FinishQuery ) { return qfalse; }

	if ( !dbi->NextRow ) { return qfalse; }

	if ( !dbi->RowCount ) { return qfalse; }

	if ( !dbi->GetFieldByID ) { return qfalse; }

	if ( !dbi->GetFieldByName ) { return qfalse; }

	if ( !dbi->GetFieldByID_int ) { return qfalse; }

	if ( !dbi->GetFieldByName_int ) { return qfalse; }

	if ( !dbi->FieldCount ) { return qfalse; }

	if ( !dbi->CleanString ) { return qfalse; }

	return qtrue;
}

void D_Init ( void )
{
	qboolean started = qfalse;

	Com_Printf ( "------ Initializing Database ------\n" );

	db_enable = Cvar_Get ( "db_enable", "1", CVAR_SERVERINFO | CVAR_ARCHIVE );
	db_backend = Cvar_Get ( "db_backend", "MySQL", CVAR_ARCHIVE );
	db_statusmaster = Cvar_Get ( "db_statusmaster", "0", CVAR_ARCHIVE );
	db_statusslave = Cvar_Get ( "db_statusslave", "0", CVAR_ARCHIVE );

	// MySQL Master Server
	db_addressMaster = Cvar_Get ( "db_addressmaster", "localhost", CVAR_ARCHIVE );
	db_portMaster = Cvar_Get ( "db_portmaster", "0", CVAR_ARCHIVE );
	db_usernameMaster = Cvar_Get ( "db_usernamemaster", "root", CVAR_ARCHIVE );
	db_passwordMaster = Cvar_Get ( "db_passwordmaster", "", CVAR_ARCHIVE );
	db_databaseMaster = Cvar_Get ( "db_databasemaster", "testdb", CVAR_ARCHIVE );

	// MySQL Slave Server
	db_addressSlave = Cvar_Get ( "db_addressslave", "localhost", CVAR_ARCHIVE );
	db_portSlave = Cvar_Get ( "db_portslave", "0", CVAR_ARCHIVE );
	db_usernameSlave = Cvar_Get ( "db_usernameslave", "root", CVAR_ARCHIVE );
	db_passwordSlave = Cvar_Get ( "db_passwordslave", "", CVAR_ARCHIVE );
	db_databaseSlave = Cvar_Get ( "db_databaseslave", "test", CVAR_ARCHIVE );

	if ( db_enable->integer == 0 )
	{
		Com_Printf ( "Database Disabled.\n" );
	}
	else
	{
		if ( strstr ( db_backend->string, "MySQL" ) )
		{
			started = D_MySQL_Init ( &dbi );
		}
		else
		{
			Cvar_Set ( "db_enable", "0" );
			Com_Printf ( "Database was set enabled but no valid backend specified.\n" );
		}

		if ( started )
		{
			if ( !DB_ValidateInterface ( &dbi ) )
			{
				Com_Error ( ERR_FATAL, "Database interface invalid." );
			}
		}
		else
		{
			Com_Printf ( "Database Initilisation Failed.\n" );
		}
	}

	if ( dbi.DBConnectMaster )
	{
		dbi.DBConnectMaster();
		Cvar_Set ( "db_statusmaster", "1" );
	}

	if ( dbi.DBConnectSlave )
	{
		dbi.DBConnectSlave();
		Cvar_Set ( "db_statusslave", "1" );
	}

	if ( db_enable->integer == 1 )
	{
		Com_DPrintf ( "Master MySQL Database connected.\n" );
		Com_DPrintf ( "Slave MySQL Database connected.\n" );
	}

	Com_Printf ( "-----------------------------------\n" );
}

void D_Shutdown ( void )
{
	if ( dbi.DBDisconnect )
	{
		dbi.DBDisconnect();
		Cvar_Set ( "db_statusmaster", "0" );
		Cvar_Set ( "db_statusslave", "0" );
	}
}

void D_Connect ( void )
{
	// MySQL Master Server
	if ( dbi.DBConnectMaster )
	{
		dbi.DBConnectMaster();
		Cvar_Set ( "db_statusmaster", "1" );
	}

	// MySQL Slave Server
	if ( dbi.DBConnectSlave )
	{
		dbi.DBConnectSlave();
		Cvar_Set ( "db_statusslave", "1" );
	}
}

void D_Status ( void )
{
	if ( dbi.DBStatus )
	{
		dbi.DBStatus();
	}
}

void D_Disconnect ( void )
{
	if ( dbi.DBDisconnect )
	{
		dbi.DBDisconnect();
		Cvar_Set ( "db_statusmaster", "0" );
		Cvar_Set ( "db_statusslave", "0" );
	}
}

int D_RunQuery ( const char *query )
{
	if ( dbi.RunQuery )
	{
		return dbi.RunQuery ( query );
	}

	return -1;
}

void D_FinishQuery ( int queryid )
{
	if ( dbi.FinishQuery )
	{
		dbi.FinishQuery ( queryid );
	}
}

qboolean D_NextRow ( int queryid )
{
	if ( dbi.NextRow )
	{
		return dbi.NextRow ( queryid );
	}

	return qfalse;
}

int D_RowCount ( int queryid )
{
	if ( dbi.RowCount )
	{
		return dbi.RowCount ( queryid );
	}

	return 0;
}

void D_GetFieldByID ( int queryid, int fieldid, char *buffer, int len )
{
	if ( dbi.GetFieldByID )
	{
		dbi.GetFieldByID ( queryid, fieldid, buffer, len );
	}
}

void D_GetFieldByName ( int queryid, const char *name, char *buffer, int len )
{
	if ( dbi.GetFieldByName )
	{
		dbi.GetFieldByName ( queryid, name, buffer, len );
	}
}

int D_GetFieldByID_int ( int queryid, int fieldid )
{
	if ( dbi.GetFieldByID_int )
	{
		return dbi.GetFieldByID_int ( queryid, fieldid );
	}

	return 0;
}

int D_GetFieldByName_int ( int queryid, const char *name )
{
	if ( dbi.GetFieldByName_int )
	{
		return dbi.GetFieldByName_int ( queryid, name );
	}

	return 0;
}

int D_FieldCount ( int queryid )
{
	if ( dbi.FieldCount )
	{
		return dbi.FieldCount ( queryid );
	}

	return 0;
}

void D_CleanString ( const char *in, char *out, int len )
{
	if ( dbi.CleanString )
	{
		dbi.CleanString ( in, out, len );
	}
}

#endif //ET_MYSQL
