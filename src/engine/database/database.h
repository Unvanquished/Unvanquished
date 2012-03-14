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

#ifndef DATABASE_H
#define DATABASE_H

#include "../qcommon/q_shared.h"
#include "../../engine/qcommon/qcommon.h"

#ifdef ET_MYSQL

//cvars
extern cvar_t *db_backend;
extern cvar_t *db_statusmaster;
extern cvar_t *db_statusslave;

extern cvar_t *db_addressMaster;
extern cvar_t *db_portMaster;
extern cvar_t *db_usernameMaster;
extern cvar_t *db_passwordMaster;
extern cvar_t *db_databaseMaster;

extern cvar_t *db_addressSlave;
extern cvar_t *db_portSlave;
extern cvar_t *db_usernameSlave;
extern cvar_t *db_passwordSlave;
extern cvar_t *db_databaseSlave;

//databse interface object
typedef struct
{
	//Connection related functins
	void ( *DBConnectMaster )( void );
	void ( *DBConnectSlave )( void );
	void ( *DBStatus )( void );
	void ( *DBDisconnect )( void );

	void ( *DBCreateTable )( void );

	//query related functions
	int ( *RunQuery )( const char *query );
	void ( *FinishQuery )( int queryid );

	//query result row manipulation
	qboolean( *NextRow )( int queryid );
	int ( *RowCount )( int queryid );

	//query result data manipulation
	void ( *GetFieldByID )( int queryid, int fieldid, char *buffer, int len );
	void ( *GetFieldByName )( int queryid, const char *name, char *buffer, int len );
	int ( *GetFieldByID_int )( int queryid, int fieldid );
	int ( *GetFieldByName_int )( int queryid, const char *name );
	int ( *FieldCount )( int queryid );

	//string cleaning
	void ( *CleanString )( const char *in, char *out, int len );
} dbinterface_t;

//database system functions
void     D_Init( void );
void     D_Shutdown( void );

void     D_Connect( void );
void     D_Status( void );
void     D_Disconnect( void );

int      D_RunQuery( const char *query );
void     D_FinishQuery( int queryid );

qboolean D_NextRow( int queryid );
int      D_RowCount( int queryid );

void     D_GetFieldByID( int queryid, int fieldid, char *buffer, int len );
void     D_GetFieldByName( int queryid, const char *name, char *buffer, int len );
int      D_GetFieldByID_int( int queryid, int fieldid );
int      D_GetFieldByName_int( int queryid, const char *name );
int      D_FieldCount( int queryid );

void     D_CleanString( const char *in, char *out, int len );

//
// MySQL functions
//
qboolean D_MySQL_Init( dbinterface_t *dbi );

//
// MYSQL Connecting related functions
//

void D_MySQL_ConnectMaster( void );
void D_MySQL_ConnectSlave( void );
void D_MySQL_DBStatus( void );
void D_MySQL_Disconnect( void );

//
// MYSQL Query related functions
//

int  D_MySQL_RunQuery( const char *query );
void D_MySQL_FinishQuery( int queryid );

//
// MYSQL ROW related functions
//

qboolean D_MySQL_NextRow( int queryid );
int      D_MySQL_RowCount( int queryid );

//
// MYSQL Field related functions
//

void D_MySQL_GetFieldByID( int queryid, int fieldid, char *buffer, int len );
void D_MySQL_GetFieldByName( int queryid, const char *name, char *buffer, int len );
int  D_MySQL_GetFieldByID_int( int queryid, int fieldid );
int  D_MySQL_GetFieldByName_int( int queryid, const char *name );
int  D_MySQL_FieldCount( int queryid );

void D_MySQL_CleanString( const char *in, char *out, int len );

//
// MYSQL Create Table
//

void D_MySQL_CreateTable( void );

#endif

#endif //ET_MYSQL
