/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

This file is part of OpenWolf.

OpenWolf is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

OpenWolf is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "g_local.h"

// NULL for everyone
void QDECL PrintMsg( gentity_t *ent, const char *fmt, ... )
{
  char    msg[ 1024 ];
  va_list argptr;
  char    *p;

  va_start( argptr,fmt );

  if( vsprintf( msg, fmt, argptr ) > sizeof( msg ) )
    G_Error ( "PrintMsg overrun" );

  va_end( argptr );

  // double quotes are bad
  while( ( p = strchr( msg, '"' ) ) != NULL )
    *p = '\'';

  G_SendCommandFromServer( ( ( ent == NULL ) ? -1 : ent-g_entities ), va( "print \"%s\"", msg ) );
}


/*
==============
OnSameTeam
==============
*/
qboolean OnSameTeam( gentity_t *ent1, gentity_t *ent2 )
{
  if( !ent1->client || !ent2->client )
    return qfalse;

  if( ent1->client->pers.teamSelection == ent2->client->pers.teamSelection )
    return qtrue;

  return qfalse;
}

/*
===========
Team_GetLocation

Report a location for the player. Uses placed nearby target_location entities
============
*/
gentity_t *Team_GetLocation( gentity_t *ent )
{
  gentity_t   *eloc, *best;
  float       bestlen, len;
  vec3_t      origin;

  best = NULL;
  bestlen = 3.0f * 8192.0f * 8192.0f;

  VectorCopy( ent->r.currentOrigin, origin );

  for( eloc = level.locationHead; eloc; eloc = eloc->nextTrain )
  {
    len = ( origin[ 0 ] - eloc->r.currentOrigin[ 0 ] ) * ( origin[ 0 ] - eloc->r.currentOrigin[ 0 ] )
        + ( origin[ 1 ] - eloc->r.currentOrigin[ 1 ] ) * ( origin[ 1 ] - eloc->r.currentOrigin[ 1 ] )
        + ( origin[ 2 ] - eloc->r.currentOrigin[ 2 ] ) * ( origin[ 2 ] - eloc->r.currentOrigin[ 2 ] );

    if( len > bestlen )
      continue;

    if( !trap_InPVS( origin, eloc->r.currentOrigin ) )
      continue;

    bestlen = len;
    best = eloc;
  }

  return best;
}


/*
===========
Team_GetLocationMsg

Report a location message for the player. Uses placed nearby target_location entities
============
*/
qboolean Team_GetLocationMsg( gentity_t *ent, char *loc, int loclen )
{
  gentity_t *best;

  best = Team_GetLocation( ent );

  if( !best )
    return qfalse;

  if( best->count )
  {
    if( best->count < 0 )
      best->count = 0;

    if( best->count > 7 )
      best->count = 7;

    Com_sprintf( loc, loclen, "%c%c%s" S_COLOR_WHITE, Q_COLOR_ESCAPE, best->count + '0', best->message );
  }
  else
    Com_sprintf( loc, loclen, "%s", best->message );

  return qtrue;
}


/*---------------------------------------------------------------------------*/

static int QDECL SortClients( const void *a, const void *b )
{
  return *(int *)a - *(int *)b;
}


/*
==================
TeamplayLocationsMessage

Format:
  clientNum location health armor weapon powerups

==================
*/
void TeamplayInfoMessage( gentity_t *ent )
{
  char      entry[ 1024 ];
  char      string[ 8192 ];
  int       stringlength;
  int       i, j;
  gentity_t *player;
  int       cnt;
  int       h, a = 0;
  int       clients[ TEAM_MAXOVERLAY ];

  if( ! ent->client->pers.teamInfo )
    return;

  // figure out what client should be on the display
  // we are limited to 8, but we want to use the top eight players
  // but in client order (so they don't keep changing position on the overlay)
  for( i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY; i++ )
  {
    player = g_entities + level.sortedClients[ i ];

    if( player->inuse && player->client->sess.sessionTeam ==
        ent->client->sess.sessionTeam )
      clients[ cnt++ ] = level.sortedClients[ i ];
  }

  // We have the top eight players, sort them by clientNum
  qsort( clients, cnt, sizeof( clients[ 0 ] ), SortClients );

  // send the latest information on all clients
  string[ 0 ] = 0;
  stringlength = 0;

  for( i = 0, cnt = 0; i < g_maxclients.integer && cnt < TEAM_MAXOVERLAY; i++)
  {
    player = g_entities + i;

    if( player->inuse && player->client->sess.sessionTeam ==
        ent->client->sess.sessionTeam )
    {
      h = player->client->ps.stats[ STAT_HEALTH ];

      if( h < 0 )
        h = 0;

      Com_sprintf( entry, sizeof( entry ),
        " %i %i %i %i %i %i",
//        level.sortedClients[i], player->client->pers.teamState.location, h, a,
        i, player->client->pers.teamState.location, h, a,
        player->client->ps.weapon, player->s.powerups );

      j = strlen( entry );

      if( stringlength + j > sizeof( string ) )
        break;

      strcpy( string + stringlength, entry );
      stringlength += j;
      cnt++;
    }
  }

  G_SendCommandFromServer( ent - g_entities, va( "tinfo %i %s", cnt, string ) );
}

void CheckTeamStatus( void )
{
  int i;
  gentity_t *loc, *ent;

  if( level.time - level.lastTeamLocationTime > TEAM_LOCATION_UPDATE_TIME )
  {
    level.lastTeamLocationTime = level.time;

    for( i = 0; i < g_maxclients.integer; i++ )
    {
      ent = g_entities + i;
      if( ent->client->pers.connected != CON_CONNECTED )
        continue;

      if( ent->inuse && ( ent->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS ||
                          ent->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS ) )
      {

        loc = Team_GetLocation( ent );

        if( loc )
          ent->client->pers.teamState.location = loc->health;
        else
          ent->client->pers.teamState.location = 0;
      }
    }

    for( i = 0; i < g_maxclients.integer; i++ )
    {
      ent = g_entities + i;
      if( ent->client->pers.connected != CON_CONNECTED )
        continue;

      if( ent->inuse && ( ent->client->ps.stats[ STAT_PTEAM ] == PTE_HUMANS ||
                          ent->client->ps.stats[ STAT_PTEAM ] == PTE_ALIENS ) )
        TeamplayInfoMessage( ent );
    }
  }
}
