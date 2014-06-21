/*
===========================================================================
Copyright (C) 2010 Darklegion Development

This file is part of Daemon.

Daemon is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Daemon is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

void G_namelog_cleanup( void )
{
	namelog_t *namelog, *n;

	for ( namelog = level.namelogs; namelog; namelog = n )
	{
		n = namelog->next;
		BG_Free( namelog );
	}
}

void G_namelog_connect( gclient_t *client )
{
	namelog_t *n, *p = NULL;
	int       i;
	char      *newname;

	for ( n = level.namelogs; n; p = n, n = n->next )
	{
		if ( n->slot != -1 )
		{
			continue;
		}

		if ( !Q_stricmp( client->pers.guid, n->guid ) )
		{
			break;
		}
	}

	if ( !n )
	{
		n = (namelog_t*) BG_Alloc( sizeof( namelog_t ) );
		strcpy( n->guid, client->pers.guid );

		if ( p )
		{
			p->next = n;
			n->id = p->id + 1;
		}
		else
		{
			level.namelogs = n;
			n->id = MAX_CLIENTS;
		}
	}

	client->pers.namelog = n;
	n->slot = client - level.clients;
	n->banned = qfalse;

	newname = n->name[ n->nameOffset ];

	// If they're muted, copy in their last known name - this will stop people
	// reconnecting to get around the name change protection.
	if ( n->muted && G_admin_name_check( &g_entities[ n->slot ],
	                                     newname, NULL, 0 ) )
	{
		Q_strncpyz( client->pers.netname, newname, MAX_NAME_LENGTH );
	}

	for ( i = 0; i < MAX_NAMELOG_ADDRS && n->ip[ i ].str[ 0 ]; i++ )
	{
		if ( !strcmp( n->ip[ i ].str, client->pers.ip.str ) )
		{
			return;
		}
	}

	if ( i == MAX_NAMELOG_ADDRS )
	{
		i--;
	}

	memcpy( &n->ip[ i ], &client->pers.ip, sizeof( n->ip[ i ] ) );
}

void G_namelog_disconnect( gclient_t *client )
{
	if ( client->pers.namelog == NULL )
	{
		return;
	}

	client->pers.namelog->slot = -1;
	client->pers.namelog = NULL;
}

void G_namelog_update_score( gclient_t *client )
{
	namelog_t *n = client->pers.namelog;

	if ( n == NULL )
	{
		return;
	}

	n->team = (team_t) client->pers.team;
	n->score = client->ps.persistant[ PERS_SCORE ];
	n->credits = client->pers.credit;
}

void G_namelog_update_name( gclient_t *client )
{
	char      n1[ MAX_NAME_LENGTH ], n2[ MAX_NAME_LENGTH ];
	namelog_t *n = client->pers.namelog;

	if ( n->name[ n->nameOffset ][ 0 ] )
	{
		G_SanitiseString( client->pers.netname, n1, sizeof( n1 ) );
		G_SanitiseString( n->name[ n->nameOffset ],
		                  n2, sizeof( n2 ) );

		if ( strcmp( n1, n2 ) != 0 )
		{
			n->nameOffset = ( n->nameOffset + 1 ) % MAX_NAMELOG_NAMES;
		}
	}

	strcpy( n->name[ n->nameOffset ], client->pers.netname );
}

void G_namelog_restore( gclient_t *client )
{
	namelog_t *n = client->pers.namelog;

	G_ChangeTeam( g_entities + n->slot, n->team );
	client->ps.persistant[ PERS_SCORE ] = n->score;
	client->ps.persistant[ PERS_CREDIT ] = 0;
	G_AddCreditToClient( client, n->credits, qfalse );
}
