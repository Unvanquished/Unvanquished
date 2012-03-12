/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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

level_locals_t  level;

typedef struct
{
  vmCvar_t  *vmCvar;
  char    *cvarName;
  char    *defaultString;
  int     cvarFlags;
  int     modificationCount;  // for tracking changes
  qboolean  trackChange;  // track this variable, and announce if changed
  qboolean teamShader;        // track and if changed, update shader state
} cvarTable_t;

gentity_t   g_entities[ MAX_GENTITIES ];
gclient_t   g_clients[ MAX_CLIENTS ];

vmCvar_t  g_fraglimit;
vmCvar_t  g_timelimit;
vmCvar_t  g_suddenDeathTime;
vmCvar_t  g_capturelimit;
vmCvar_t  g_friendlyFire;
vmCvar_t  g_password;
vmCvar_t  g_needpass;
vmCvar_t  g_maxclients;
vmCvar_t  g_maxGameClients;
vmCvar_t  g_dedicated;
vmCvar_t  g_speed;
vmCvar_t  g_gravity;
vmCvar_t  g_cheats;
vmCvar_t  g_knockback;
vmCvar_t  g_quadfactor;
vmCvar_t  g_forcerespawn;
vmCvar_t  g_inactivity;
vmCvar_t  g_debugMove;
vmCvar_t  g_debugDamage;
vmCvar_t  g_debugAlloc;
vmCvar_t  g_weaponRespawn;
vmCvar_t  g_weaponTeamRespawn;
vmCvar_t  g_motd;
vmCvar_t  g_synchronousClients;
vmCvar_t  g_warmup;
vmCvar_t  g_doWarmup;
vmCvar_t  g_restarted;
vmCvar_t  g_logFile;
vmCvar_t  g_logFileSync;
vmCvar_t  g_blood;
vmCvar_t  g_podiumDist;
vmCvar_t  g_podiumDrop;
vmCvar_t  g_allowVote;
vmCvar_t  g_teamAutoJoin;
vmCvar_t  g_teamForceBalance;
vmCvar_t  g_banIPs;
vmCvar_t  g_filterBan;
vmCvar_t  g_smoothClients;
vmCvar_t  pmove_fixed;
vmCvar_t  pmove_msec;
vmCvar_t  g_rankings;
vmCvar_t  g_listEntity;
vmCvar_t  g_minCommandPeriod;

//TA
vmCvar_t  g_humanBuildPoints;
vmCvar_t  g_alienBuildPoints;
vmCvar_t  g_humanStage;
vmCvar_t  g_humanKills;
vmCvar_t  g_humanMaxStage;
vmCvar_t  g_humanStage2Threshold;
vmCvar_t  g_humanStage3Threshold;
vmCvar_t  g_alienStage;
vmCvar_t  g_alienKills;
vmCvar_t  g_alienMaxStage;
vmCvar_t  g_alienStage2Threshold;
vmCvar_t  g_alienStage3Threshold;

vmCvar_t  g_disabledEquipment;
vmCvar_t  g_disabledClasses;
vmCvar_t  g_disabledBuildables;

vmCvar_t  g_debugMapRotation;
vmCvar_t  g_currentMapRotation;
vmCvar_t  g_currentMap;
vmCvar_t  g_initialMapRotation;

static cvarTable_t   gameCvarTable[ ] =
{
  // don't override the cheat state set by the system
  { &g_cheats, "sv_cheats", "", 0, 0, qfalse },

  // noset vars
  { NULL, "gamename", GAME_VERSION , CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },
  { NULL, "gamedate", __DATE__ , CVAR_ROM, 0, qfalse  },
  { &g_restarted, "g_restarted", "0", CVAR_ROM, 0, qfalse  },
  { NULL, "sv_mapname", "", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse  },

  // latched vars

  { &g_maxclients, "sv_maxclients", "8", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },
  { &g_maxGameClients, "g_maxGameClients", "0", CVAR_SERVERINFO | CVAR_LATCH | CVAR_ARCHIVE, 0, qfalse  },

  // change anytime vars
  { &g_timelimit, "timelimit", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },
  { &g_suddenDeathTime, "g_suddenDeathTime", "0", CVAR_SERVERINFO | CVAR_ARCHIVE | CVAR_NORESTART, 0, qtrue },

  { &g_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO, 0, qfalse  },

  { &g_friendlyFire, "g_friendlyFire", "0", CVAR_ARCHIVE, 0, qtrue  },

  { &g_teamAutoJoin, "g_teamAutoJoin", "0", CVAR_ARCHIVE  },
  { &g_teamForceBalance, "g_teamForceBalance", "0", CVAR_ARCHIVE  },

  { &g_warmup, "g_warmup", "20", CVAR_ARCHIVE, 0, qtrue  },
  { &g_doWarmup, "g_doWarmup", "0", 0, 0, qtrue  },
  { &g_logFile, "g_logFile", "games.log", CVAR_ARCHIVE, 0, qfalse  },
  { &g_logFileSync, "g_logFileSync", "0", CVAR_ARCHIVE, 0, qfalse  },

  { &g_password, "g_password", "", CVAR_USERINFO, 0, qfalse  },

  { &g_banIPs, "g_banIPs", "", CVAR_ARCHIVE, 0, qfalse  },
  { &g_filterBan, "g_filterBan", "1", CVAR_ARCHIVE, 0, qfalse  },

  { &g_needpass, "g_needpass", "0", CVAR_SERVERINFO | CVAR_ROM, 0, qfalse },

  { &g_dedicated, "dedicated", "0", 0, 0, qfalse  },

  { &g_speed, "g_speed", "320", 0, 0, qtrue  },
  { &g_gravity, "g_gravity", "800", 0, 0, qtrue  },
  { &g_knockback, "g_knockback", "1000", 0, 0, qtrue  },
  { &g_quadfactor, "g_quadfactor", "3", 0, 0, qtrue  },
  { &g_weaponRespawn, "g_weaponrespawn", "5", 0, 0, qtrue  },
  { &g_weaponTeamRespawn, "g_weaponTeamRespawn", "30", 0, 0, qtrue },
  { &g_forcerespawn, "g_forcerespawn", "20", 0, 0, qtrue },
  { &g_inactivity, "g_inactivity", "0", 0, 0, qtrue },
  { &g_debugMove, "g_debugMove", "0", 0, 0, qfalse },
  { &g_debugDamage, "g_debugDamage", "0", 0, 0, qfalse },
  { &g_debugAlloc, "g_debugAlloc", "0", 0, 0, qfalse },
  { &g_motd, "g_motd", "", 0, 0, qfalse },
  { &g_blood, "com_blood", "1", 0, 0, qfalse },

  { &g_podiumDist, "g_podiumDist", "80", 0, 0, qfalse },
  { &g_podiumDrop, "g_podiumDrop", "70", 0, 0, qfalse },

  { &g_allowVote, "g_allowVote", "1", CVAR_ARCHIVE, 0, qfalse },
  { &g_listEntity, "g_listEntity", "0", 0, 0, qfalse },
  { &g_minCommandPeriod, "g_minCommandPeriod", "500", 0, 0, qfalse},

  { &g_smoothClients, "g_smoothClients", "1", 0, 0, qfalse},
  { &pmove_fixed, "pmove_fixed", "0", CVAR_SYSTEMINFO, 0, qfalse},
  { &pmove_msec, "pmove_msec", "8", CVAR_SYSTEMINFO, 0, qfalse},

  { &g_humanBuildPoints, "g_humanBuildPoints", "100", 0, 0, qfalse  },
  { &g_alienBuildPoints, "g_alienBuildPoints", "100", 0, 0, qfalse  },
  { &g_humanStage, "g_humanStage", "0", 0, 0, qfalse  },
  { &g_humanKills, "g_humanKills", "0", 0, 0, qfalse  },
  { &g_humanMaxStage, "g_humanMaxStage", "2", 0, 0, qfalse  },
  { &g_humanStage2Threshold, "g_humanStage2Threshold", "20", 0, 0, qfalse  },
  { &g_humanStage3Threshold, "g_humanStage3Threshold", "40", 0, 0, qfalse  },
  { &g_alienStage, "g_alienStage", "0", 0, 0, qfalse  },
  { &g_alienKills, "g_alienKills", "0", 0, 0, qfalse  },
  { &g_alienMaxStage, "g_alienMaxStage", "2", 0, 0, qfalse  },
  { &g_alienStage2Threshold, "g_alienStage2Threshold", "20", 0, 0, qfalse  },
  { &g_alienStage3Threshold, "g_alienStage3Threshold", "40", 0, 0, qfalse  },

  { &g_disabledEquipment, "g_disabledEquipment", "", CVAR_ROM, 0, qfalse  },
  { &g_disabledClasses, "g_disabledClasses", "", CVAR_ROM, 0, qfalse  },
  { &g_disabledBuildables, "g_disabledBuildables", "", CVAR_ROM, 0, qfalse  },

  { &g_debugMapRotation, "g_debugMapRotation", "0", 0, 0, qfalse  },
  { &g_currentMapRotation, "g_currentMapRotation", "-1", 0, 0, qfalse  }, // -1 = NOT_ROTATING
  { &g_currentMap, "g_currentMap", "0", 0, 0, qfalse  },
  { &g_initialMapRotation, "g_initialMapRotation", "", CVAR_ARCHIVE, 0, qfalse  },

  { &g_rankings, "g_rankings", "0", 0, 0, qfalse}
};

static int gameCvarTableSize = sizeof( gameCvarTable ) / sizeof( gameCvarTable[ 0 ] );


void G_InitGame( int levelTime, int randomSeed, int restart );
void G_RunFrame( int levelTime );
void G_ShutdownGame( int restart );
void CheckExitRules( void );

void G_CountSpawns( void );
void G_CalculateBuildPoints( void );

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
intptr_t vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  ) {
  switch( command )
  {
    case GAME_INIT:
      G_InitGame( arg0, arg1, arg2 );
      return 0;

    case GAME_SHUTDOWN:
      G_ShutdownGame( arg0 );
      return 0;

    case GAME_CLIENT_CONNECT:
      return (intptr_t)ClientConnect( arg0, arg1, arg2 );

    case GAME_CLIENT_THINK:
      ClientThink( arg0 );
      return 0;

    case GAME_CLIENT_USERINFO_CHANGED:
      ClientUserinfoChanged( arg0 );
      return 0;

    case GAME_CLIENT_DISCONNECT:
      ClientDisconnect( arg0 );
      return 0;

    case GAME_CLIENT_BEGIN:
      ClientBegin( arg0 );
      return 0;

    case GAME_CLIENT_COMMAND:
      ClientCommand( arg0 );
      return 0;

    case GAME_RUN_FRAME:
      G_RunFrame( arg0 );
      return 0;

    case GAME_CONSOLE_COMMAND:
      return ConsoleCommand( );
  }

  return -1;
}


void QDECL G_Printf( const char *fmt, ... )
{
  va_list argptr;
  char    text[ 1024 ];

  va_start( argptr, fmt );
  vsprintf( text, fmt, argptr );
  va_end( argptr );

  trap_Print( text );
}

void QDECL G_Error( const char *fmt, ... )
{
  va_list argptr;
  char    text[ 1024 ];

  va_start( argptr, fmt );
  vsprintf( text, fmt, argptr );
  va_end( argptr );

  trap_Error( text );
}

/*
================
G_FindTeams

Chain together all entities with a matching team field.
Entity teams are used for item groups and multi-entity mover groups.

All but the first will have the FL_TEAMSLAVE flag set and teammaster field set
All but the last will have the teamchain field set to the next one
================
*/
void G_FindTeams( void )
{
  gentity_t *e, *e2;
  int       i, j;
  int       c, c2;

  c = 0;
  c2 = 0;

  for( i = 1, e = g_entities+i; i < level.num_entities; i++, e++ )
  {
    if( !e->inuse )
      continue;

    if( !e->team )
      continue;

    if( e->flags & FL_TEAMSLAVE )
      continue;

    e->teammaster = e;
    c++;
    c2++;

    for( j = i + 1, e2 = e + 1; j < level.num_entities; j++, e2++ )
    {
      if( !e2->inuse )
        continue;

      if( !e2->team )
        continue;

      if( e2->flags & FL_TEAMSLAVE )
        continue;

      if( !strcmp( e->team, e2->team ) )
      {
        c2++;
        e2->teamchain = e->teamchain;
        e->teamchain = e2;
        e2->teammaster = e;
        e2->flags |= FL_TEAMSLAVE;

        // make sure that targets only point at the master
        if( e2->targetname )
        {
          e->targetname = e2->targetname;
          e2->targetname = NULL;
        }
      }
    }
  }

  G_Printf( "%i teams with %i entities\n", c, c2 );
}

void G_RemapTeamShaders( void )
{
}


/*
=================
G_RegisterCvars
=================
*/
void G_RegisterCvars( void )
{
  int         i;
  cvarTable_t *cv;
  qboolean    remapped = qfalse;

  for( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ )
  {
    trap_Cvar_Register( cv->vmCvar, cv->cvarName,
      cv->defaultString, cv->cvarFlags );

    if( cv->vmCvar )
      cv->modificationCount = cv->vmCvar->modificationCount;

    if( cv->teamShader )
      remapped = qtrue;
  }

  if( remapped )
    G_RemapTeamShaders( );

  // check some things
  level.warmupModificationCount = g_warmup.modificationCount;
}

/*
=================
G_UpdateCvars
=================
*/
void G_UpdateCvars( void )
{
  int         i;
  cvarTable_t *cv;
  qboolean    remapped = qfalse;

  for( i = 0, cv = gameCvarTable; i < gameCvarTableSize; i++, cv++ )
  {
    if( cv->vmCvar )
    {
      trap_Cvar_Update( cv->vmCvar );

      if( cv->modificationCount != cv->vmCvar->modificationCount )
      {
        cv->modificationCount = cv->vmCvar->modificationCount;

        if( cv->trackChange )
          G_SendCommandFromServer( -1, va( "print \"Server: %s changed to %s\n\"",
            cv->cvarName, cv->vmCvar->string ) );

        if( cv->teamShader )
          remapped = qtrue;
      }
    }
  }

  if( remapped )
    G_RemapTeamShaders( );
}

/*
============
G_InitGame

============
*/
void G_InitGame( int levelTime, int randomSeed, int restart )
{
  int i;

  srand( randomSeed );

  G_RegisterCvars( );

  G_Printf( "------- Game Initialization -------\n" );
  G_Printf( "gamename: %s\n", GAME_VERSION );
  G_Printf( "gamedate: %s\n", __DATE__ );

  G_ProcessIPBans( );

  G_InitMemory( );

  // set some level globals
  memset( &level, 0, sizeof( level ) );
  level.time = levelTime;
  level.startTime = levelTime;
  level.alienStage2Time = level.alienStage3Time =
    level.humanStage2Time = level.humanStage3Time = level.startTime;

  level.snd_fry = G_SoundIndex( "sound/misc/fry.wav" ); // FIXME standing in lava / slime

  if( g_logFile.string[ 0 ] )
  {
    if( g_logFileSync.integer )
      trap_FS_FOpenFile( g_logFile.string, &level.logFile, FS_APPEND_SYNC );
    else
      trap_FS_FOpenFile( g_logFile.string, &level.logFile, FS_APPEND );

    if( !level.logFile )
      G_Printf( "WARNING: Couldn't open logfile: %s\n", g_logFile.string );
    else
    {
      char serverinfo[ MAX_INFO_STRING ];

      trap_GetServerinfo( serverinfo, sizeof( serverinfo ) );

      G_LogPrintf( "------------------------------------------------------------\n" );
      G_LogPrintf( "InitGame: %s\n", serverinfo );
    }
  }
  else
    G_Printf( "Not logging to disk\n" );

  // initialize all entities for this game
  memset( g_entities, 0, MAX_GENTITIES * sizeof( g_entities[ 0 ] ) );
  level.gentities = g_entities;

  // initialize all clients for this game
  level.maxclients = g_maxclients.integer;
  memset( g_clients, 0, MAX_CLIENTS * sizeof( g_clients[ 0 ] ) );
  level.clients = g_clients;

  // set client fields on player ents
  for( i = 0; i < level.maxclients; i++ )
    g_entities[ i ].client = level.clients + i;

  // always leave room for the max number of clients,
  // even if they aren't all used, so numbers inside that
  // range are NEVER anything but clients
  level.num_entities = MAX_CLIENTS;

  // let the server system know where the entites are
  trap_LocateGameData( level.gentities, level.num_entities, sizeof( gentity_t ),
    &level.clients[ 0 ].ps, sizeof( level.clients[ 0 ] ) );

  trap_SetConfigstring( CS_INTERMISSION, "0" );

  // parse the key/value pairs and spawn gentities
  G_SpawnEntitiesFromString( );

  // the map might disable some things
  BG_InitAllowedGameElements( );

  // general initialization
  G_FindTeams( );

  //TA:
  BG_InitClassOverrides( );
  BG_InitBuildableOverrides( );
  G_InitDamageLocations( );
  G_InitMapRotations( );
  G_InitSpawnQueue( &level.alienSpawnQueue );
  G_InitSpawnQueue( &level.humanSpawnQueue );

  if( g_debugMapRotation.integer )
    G_PrintRotations( );

  //reset stages
  trap_Cvar_Set( "g_alienStage", va( "%d", S1 ) );
  trap_Cvar_Set( "g_humanStage", va( "%d", S1 ) );
  trap_Cvar_Set( "g_alienKills", 0 );
  trap_Cvar_Set( "g_humanKills", 0 );

  G_Printf( "-----------------------------------\n" );

  G_RemapTeamShaders( );

  //TA: so the server counts the spawns without a client attached
  G_CountSpawns( );

  G_ResetPTRConnections( );
}



/*
=================
G_ShutdownGame
=================
*/
void G_ShutdownGame( int restart )
{
  G_Printf( "==== ShutdownGame ====\n" );

  if( level.logFile )
  {
    G_LogPrintf( "ShutdownGame:\n" );
    G_LogPrintf( "------------------------------------------------------------\n" );
    trap_FS_FCloseFile( level.logFile );
  }

  // write all the client session data so we can get it back
  G_WriteSessionData( );
}



//===================================================================

void QDECL Com_Error( int level, const char *error, ... )
{
  va_list argptr;
  char    text[ 1024 ];

  va_start( argptr, error );
  vsprintf( text, error, argptr );
  va_end( argptr );

  trap_Error( text );
}

void QDECL Com_Printf( const char *msg, ... )
{
  va_list argptr;
  char    text[ 1024 ];

  va_start( argptr, msg );
  vsprintf( text, msg, argptr );
  va_end( argptr );

  trap_Print( text );
}

/*
========================================================================

PLAYER COUNTING / SCORE SORTING

========================================================================
*/


/*
=============
SortRanks

=============
*/
int QDECL SortRanks( const void *a, const void *b )
{
  gclient_t *ca, *cb;

  ca = &level.clients[ *(int *)a ];
  cb = &level.clients[ *(int *)b ];

  // then sort by score
  if( ca->ps.persistant[ PERS_SCORE ] > cb->ps.persistant[ PERS_SCORE ] )
    return -1;
  else if( ca->ps.persistant[ PERS_SCORE ] < cb->ps.persistant[ PERS_SCORE ] )
    return 1;
  else
    return 0;
}

/*
============
G_InitSpawnQueue

Initialise a spawn queue
============
*/
void G_InitSpawnQueue( spawnQueue_t *sq )
{
  int i;

  sq->back = sq->front = 0;
  sq->back = QUEUE_MINUS1( sq->back );

  //0 is a valid clientNum, so use something else
  for( i = 0; i < MAX_CLIENTS; i++ )
    sq->clients[ i ] = -1;
}

/*
============
G_GetSpawnQueueLength

Return tha length of a spawn queue
============
*/
int G_GetSpawnQueueLength( spawnQueue_t *sq )
{
  int length = sq->back - sq->front + 1;

  while( length < 0 )
    length += MAX_CLIENTS;

  while( length >= MAX_CLIENTS )
    length -= MAX_CLIENTS;

  return length;
}

/*
============
G_PopSpawnQueue

Remove from front element from a spawn queue
============
*/
int G_PopSpawnQueue( spawnQueue_t *sq )
{
  int clientNum = sq->clients[ sq->front ];

  if( G_GetSpawnQueueLength( sq ) > 0 )
  {
    sq->clients[ sq->front ] = -1;
    sq->front = QUEUE_PLUS1( sq->front );
    g_entities[ clientNum ].client->ps.pm_flags &= ~PMF_QUEUED;

    return clientNum;
  }
  else
    return -1;
}

/*
============
G_PeekSpawnQueue

Look at front element from a spawn queue
============
*/
int G_PeekSpawnQueue( spawnQueue_t *sq )
{
  return sq->clients[ sq->front ];
}

/*
============
G_PushSpawnQueue

Add an element to the back of the spawn queue
============
*/
void G_PushSpawnQueue( spawnQueue_t *sq, int clientNum )
{
  sq->back = QUEUE_PLUS1( sq->back );
  sq->clients[ sq->back ] = clientNum;

  g_entities[ clientNum ].client->ps.pm_flags |= PMF_QUEUED;
}

/*
============
G_RemoveFromSpawnQueue

remove a specific client from a spawn queue
============
*/
qboolean G_RemoveFromSpawnQueue( spawnQueue_t *sq, int clientNum )
{
  int i = sq->front;

  if( G_GetSpawnQueueLength( sq ) )
  {
    do
    {
      if( sq->clients[ i ] == clientNum )
      {
        //and this kids is why it would have
        //been better to use an LL for internal
        //representation
        do
        {
          sq->clients[ i ] = sq->clients[ QUEUE_PLUS1( i ) ];

          i = QUEUE_PLUS1( i );
        } while( i != QUEUE_PLUS1( sq->back ) );

        sq->back = QUEUE_MINUS1( sq->back );
        g_entities[ clientNum ].client->ps.pm_flags &= ~PMF_QUEUED;

        return qtrue;
      }

      i = QUEUE_PLUS1( i );
    } while( i != QUEUE_PLUS1( sq->back ) );
  }

  return qfalse;
}

/*
============
G_GetPosInSpawnQueue

Get the position of a client in a spawn queue
============
*/
int G_GetPosInSpawnQueue( spawnQueue_t *sq, int clientNum )
{
  int i = sq->front;

  if( G_GetSpawnQueueLength( sq ) )
  {
    do
    {
      if( sq->clients[ i ] == clientNum )
      {
        if( i < sq->front )
          return i + MAX_CLIENTS - sq->front;
        else
          return i - sq->front;
      }

      i = QUEUE_PLUS1( i );
    } while( i != QUEUE_PLUS1( sq->back ) );
  }

  return -1;
}

/*
============
G_PrintSpawnQueue

Print the contents of a spawn queue
============
*/
void G_PrintSpawnQueue( spawnQueue_t *sq )
{
  int i = sq->front;
  int length = G_GetSpawnQueueLength( sq );

  G_Printf( "l:%d f:%d b:%d    :", length, sq->front, sq->back );

  if( length > 0 )
  {
    do
    {
      if( sq->clients[ i ] == -1 )
        G_Printf( "*:" );
      else
        G_Printf( "%d:", sq->clients[ i ] );

      i = QUEUE_PLUS1( i );
    } while( i != QUEUE_PLUS1( sq->back ) );
  }

  G_Printf( "\n" );
}

/*
============
G_SpawnClients

Spawn queued clients
============
*/
void G_SpawnClients( pTeam_t team )
{
  int           clientNum;
  gentity_t     *ent, *spawn;
  vec3_t        spawn_origin, spawn_angles;
  spawnQueue_t  *sq = NULL;
  int           numSpawns = 0;

  if( team == PTE_ALIENS )
  {
    sq = &level.alienSpawnQueue;
    numSpawns = level.numAlienSpawns;
  }
  else if( team == PTE_HUMANS )
  {
    sq = &level.humanSpawnQueue;
    numSpawns = level.numHumanSpawns;
  }

  if( G_GetSpawnQueueLength( sq ) > 0 && numSpawns > 0 )
  {
    clientNum = G_PeekSpawnQueue( sq );
    ent = &g_entities[ clientNum ];

    if( ( spawn = SelectTremulousSpawnPoint( team,
            ent->client->pers.lastDeathLocation,
            spawn_origin, spawn_angles ) ) )
    {
      clientNum = G_PopSpawnQueue( sq );

      if( clientNum < 0 )
        return;

      ent = &g_entities[ clientNum ];

      ent->client->sess.sessionTeam = TEAM_FREE;
      ClientUserinfoChanged( clientNum );
      ClientSpawn( ent, spawn, spawn_origin, spawn_angles );
    }
  }
}

/*
============
G_CountSpawns

Counts the number of spawns for each team
============
*/
void G_CountSpawns( void )
{
  int i;
  gentity_t *ent;

  level.numAlienSpawns = 0;
  level.numHumanSpawns = 0;

  for( i = 1, ent = g_entities + i ; i < level.num_entities ; i++, ent++ )
  {
    if( !ent->inuse )
      continue;

    if( ent->s.modelindex == BA_A_SPAWN && ent->health > 0 )
      level.numAlienSpawns++;

    if( ent->s.modelindex == BA_H_SPAWN && ent->health > 0 )
      level.numHumanSpawns++;
  }

  //let the client know how many spawns there are
  trap_SetConfigstring( CS_SPAWNS, va( "%d %d",
        level.numAlienSpawns, level.numHumanSpawns ) );
}


#define PLAYER_COUNT_MOD 5.0f

/*
============
G_CalculateBuildPoints

Recalculate the quantity of building points available to the teams
============
*/
void G_CalculateBuildPoints( void )
{
  int         i;
  buildable_t buildable;
  gentity_t   *ent;
  int         localHTP = g_humanBuildPoints.integer,
              localATP = g_alienBuildPoints.integer;

  if( g_suddenDeathTime.integer && !level.warmupTime &&
    ( level.time - level.startTime >= g_suddenDeathTime.integer * 60000 ) )
  {
    localHTP = 0;
    localATP = 0;
  }
  else
  {
    localHTP = g_humanBuildPoints.integer;
    localATP = g_alienBuildPoints.integer;
  }

  level.humanBuildPoints = level.humanBuildPointsPowered = localHTP;
  level.alienBuildPoints = localATP;

  level.reactorPresent = qfalse;
  level.overmindPresent = qfalse;

  for( i = 1, ent = g_entities + i ; i < level.num_entities ; i++, ent++ )
  {
    if( !ent->inuse )
      continue;

    if( ent->s.eType != ET_BUILDABLE )
      continue;

    buildable = ent->s.modelindex;

    if( buildable != BA_NONE )
    {
      if( buildable == BA_H_REACTOR && ent->spawned && ent->health > 0 )
        level.reactorPresent = qtrue;

      if( buildable == BA_A_OVERMIND && ent->spawned && ent->health > 0 )
        level.overmindPresent = qtrue;

      if( BG_FindTeamForBuildable( buildable ) == BIT_HUMANS )
      {
        level.humanBuildPoints -= BG_FindBuildPointsForBuildable( buildable );

        if( ent->powered )
          level.humanBuildPointsPowered -= BG_FindBuildPointsForBuildable( buildable );
      }
      else
      {
        level.alienBuildPoints -= BG_FindBuildPointsForBuildable( buildable );
      }
    }
  }

  if( level.humanBuildPoints < 0 )
  {
    localHTP -= level.humanBuildPoints;
    level.humanBuildPointsPowered -= level.humanBuildPoints;
    level.humanBuildPoints = 0;
  }

  if( level.alienBuildPoints < 0 )
  {
    localATP -= level.alienBuildPoints;
    level.alienBuildPoints = 0;
  }

  trap_SetConfigstring( CS_BUILDPOINTS,
                        va( "%d %d %d %d %d", level.alienBuildPoints,
                                              localATP,
                                              level.humanBuildPoints,
                                              localHTP,
                                              level.humanBuildPointsPowered ) );

  //may as well pump the stages here too
  {
    float alienPlayerCountMod = level.averageNumAlienClients / PLAYER_COUNT_MOD;
    float humanPlayerCountMod = level.averageNumHumanClients / PLAYER_COUNT_MOD;
    int   alienNextStageThreshold, humanNextStageThreshold;

    if( alienPlayerCountMod < 0.1f )
      alienPlayerCountMod = 0.1f;

    if( humanPlayerCountMod < 0.1f )
      humanPlayerCountMod = 0.1f;

    if( g_alienStage.integer == S1 && g_alienMaxStage.integer > S1 )
      alienNextStageThreshold = (int)( ceil( (float)g_alienStage2Threshold.integer * alienPlayerCountMod ) );
    else if( g_alienStage.integer == S2 && g_alienMaxStage.integer > S2 )
      alienNextStageThreshold = (int)( ceil( (float)g_alienStage3Threshold.integer * alienPlayerCountMod ) );
    else
      alienNextStageThreshold = -1;

    if( g_humanStage.integer == S1 && g_humanMaxStage.integer > S1 )
      humanNextStageThreshold = (int)( ceil( (float)g_humanStage2Threshold.integer * humanPlayerCountMod ) );
    else if( g_humanStage.integer == S2 && g_humanMaxStage.integer > S2 )
      humanNextStageThreshold = (int)( ceil( (float)g_humanStage3Threshold.integer * humanPlayerCountMod ) );
    else
      humanNextStageThreshold = -1;

    trap_SetConfigstring( CS_STAGES, va( "%d %d %d %d %d %d",
          g_alienStage.integer, g_humanStage.integer,
          g_alienKills.integer, g_humanKills.integer,
          alienNextStageThreshold, humanNextStageThreshold ) );
  }
}

/*
============
G_CalculateStages
============
*/
void G_CalculateStages( void )
{
  float alienPlayerCountMod = level.averageNumAlienClients / PLAYER_COUNT_MOD;
  float humanPlayerCountMod = level.averageNumHumanClients / PLAYER_COUNT_MOD;

  if( alienPlayerCountMod < 0.1f )
    alienPlayerCountMod = 0.1f;

  if( humanPlayerCountMod < 0.1f )
    humanPlayerCountMod = 0.1f;

  if( g_alienKills.integer >=
      (int)( ceil( (float)g_alienStage2Threshold.integer * alienPlayerCountMod ) ) &&
      g_alienStage.integer == S1 && g_alienMaxStage.integer > S1 )
  {
    G_Checktrigger_stages( PTE_ALIENS, S2 );
    trap_Cvar_Set( "g_alienStage", va( "%d", S2 ) );
    level.alienStage2Time = level.time;
  }

  if( g_alienKills.integer >=
      (int)( ceil( (float)g_alienStage3Threshold.integer * alienPlayerCountMod ) ) &&
      g_alienStage.integer == S2 && g_alienMaxStage.integer > S2 )
  {
    G_Checktrigger_stages( PTE_ALIENS, S3 );
    trap_Cvar_Set( "g_alienStage", va( "%d", S3 ) );
    level.alienStage3Time = level.time;
  }

  if( g_humanKills.integer >=
      (int)( ceil( (float)g_humanStage2Threshold.integer * humanPlayerCountMod ) ) &&
      g_humanStage.integer == S1 && g_humanMaxStage.integer > S1 )
  {
    G_Checktrigger_stages( PTE_HUMANS, S2 );
    trap_Cvar_Set( "g_humanStage", va( "%d", S2 ) );
    level.humanStage2Time = level.time;
  }

  if( g_humanKills.integer >=
      (int)( ceil( (float)g_humanStage3Threshold.integer * humanPlayerCountMod ) ) &&
      g_humanStage.integer == S2 && g_humanMaxStage.integer > S2 )
  {
    G_Checktrigger_stages( PTE_HUMANS, S3 );
    trap_Cvar_Set( "g_humanStage", va( "%d", S3 ) );
    level.humanStage3Time = level.time;
  }
}

/*
============
CalculateAvgPlayers

Calculates the average number of players playing this game
============
*/
void G_CalculateAvgPlayers( void )
{
  //there are no clients or only spectators connected, so
  //reset the number of samples in order to avoid the situation
  //where the average tends to 0
  if( !level.numAlienClients )
  {
    level.numAlienSamples = 0;
    trap_Cvar_Set( "g_alienKills", "0" );
  }

  if( !level.numHumanClients )
  {
    level.numHumanSamples = 0;
    trap_Cvar_Set( "g_humanKills", "0" );
  }

  //calculate average number of clients for stats
  level.averageNumAlienClients =
    ( ( level.averageNumAlienClients * level.numAlienSamples )
      + level.numAlienClients ) /
    (float)( level.numAlienSamples + 1 );
  level.numAlienSamples++;

  level.averageNumHumanClients =
    ( ( level.averageNumHumanClients * level.numHumanSamples )
      + level.numHumanClients ) /
    (float)( level.numHumanSamples + 1 );
  level.numHumanSamples++;
}

/*
============
CalculateRanks

Recalculates the score ranks of all players
This will be called on every client connect, begin, disconnect, death,
and team change.
============
*/
void CalculateRanks( void )
{
  int       i;
  int       rank;
  int       score;
  int       newScore;
  gclient_t *cl;

  level.follow1 = -1;
  level.follow2 = -1;
  level.numConnectedClients = 0;
  level.numNonSpectatorClients = 0;
  level.numPlayingClients = 0;
  level.numVotingClients = 0;   // don't count bots
  level.numAlienClients = 0;
  level.numHumanClients = 0;
  level.numLiveAlienClients = 0;
  level.numLiveHumanClients = 0;

  for( i = 0; i < TEAM_NUM_TEAMS; i++ )
    level.numteamVotingClients[ i ] = 0;

  for( i = 0; i < level.maxclients; i++ )
  {
    if ( level.clients[ i ].pers.connected != CON_DISCONNECTED )
    {
      level.sortedClients[ level.numConnectedClients ] = i;
      level.numConnectedClients++;

      if( !( level.clients[ i ].ps.pm_flags & PMF_FOLLOW ) )
      {
        //so we know when the game ends and for team leveling
        if( level.clients[ i ].ps.stats[ STAT_PTEAM ] == PTE_ALIENS )
        {
          level.numAlienClients++;
          if( level.clients[ i ].sess.sessionTeam != TEAM_SPECTATOR )
            level.numLiveAlienClients++;
        }

        if( level.clients[ i ].ps.stats[ STAT_PTEAM ] == PTE_HUMANS )
        {
          level.numHumanClients++;
          if( level.clients[ i ].sess.sessionTeam != TEAM_SPECTATOR )
            level.numLiveHumanClients++;
        }
      }

      if( level.clients[ i ].sess.sessionTeam != TEAM_SPECTATOR )
      {
        level.numNonSpectatorClients++;

        // decide if this should be auto-followed
        if( level.clients[ i ].pers.connected == CON_CONNECTED )
        {
          level.numPlayingClients++;
          if( !(g_entities[ i ].r.svFlags & SVF_BOT) )
            level.numVotingClients++;

          if( level.clients[ i ].ps.stats[ STAT_PTEAM ] == PTE_HUMANS )
            level.numteamVotingClients[ 0 ]++;
          else if( level.clients[ i ].ps.stats[ STAT_PTEAM ] == PTE_ALIENS )
            level.numteamVotingClients[ 1 ]++;

          if( level.follow1 == -1 )
            level.follow1 = i;
          else if( level.follow2 == -1 )
            level.follow2 = i;
        }

      }
    }
  }

  qsort( level.sortedClients, level.numConnectedClients,
    sizeof( level.sortedClients[ 0 ] ), SortRanks );

  // set the rank value for all clients that are connected and not spectators
  rank = -1;
  score = 0;
  for( i = 0;  i < level.numPlayingClients; i++ )
  {
    cl = &level.clients[ level.sortedClients[ i ] ];
    newScore = cl->ps.persistant[ PERS_SCORE ];

    if( i == 0 || newScore != score )
    {
      rank = i;
      // assume we aren't tied until the next client is checked
      level.clients[ level.sortedClients[ i ] ].ps.persistant[ PERS_RANK ] = rank;
    }
    else
    {
      // we are tied with the previous client
      level.clients[ level.sortedClients[ i - 1 ] ].ps.persistant[ PERS_RANK ] = rank;
      level.clients[ level.sortedClients[ i ] ].ps.persistant[ PERS_RANK ] = rank;
    }

    score = newScore;
  }

  // set the CS_SCORES1/2 configstrings, which will be visible to everyone
  if( level.numConnectedClients == 0 )
  {
    trap_SetConfigstring( CS_SCORES1, va( "%i", SCORE_NOT_PRESENT ) );
    trap_SetConfigstring( CS_SCORES2, va( "%i", SCORE_NOT_PRESENT ) );
  }
  else if( level.numConnectedClients == 1 )
  {
    trap_SetConfigstring( CS_SCORES1, va( "%i",
          level.clients[ level.sortedClients[ 0 ] ].ps.persistant[ PERS_SCORE ] ) );
    trap_SetConfigstring( CS_SCORES2, va( "%i", SCORE_NOT_PRESENT ) );
  }
  else
  {
    trap_SetConfigstring( CS_SCORES1, va( "%i",
          level.clients[ level.sortedClients[ 0 ] ].ps.persistant[ PERS_SCORE ] ) );
    trap_SetConfigstring( CS_SCORES2, va( "%i",
          level.clients[ level.sortedClients[ 1 ] ].ps.persistant[ PERS_SCORE ] ) );
  }

  // see if it is time to end the level
  CheckExitRules( );

  // if we are at the intermission, send the new info to everyone
  if( level.intermissiontime )
    SendScoreboardMessageToAllClients( );
}


/*
========================================================================

MAP CHANGING

========================================================================
*/

/*
========================
SendScoreboardMessageToAllClients

Do this at BeginIntermission time and whenever ranks are recalculated
due to enters/exits/forced team changes
========================
*/
void SendScoreboardMessageToAllClients( void )
{
  int   i;

  for( i = 0; i < level.maxclients; i++ )
  {
    if( level.clients[ i ].pers.connected == CON_CONNECTED )
      ScoreboardMessage( g_entities + i );
  }
}

/*
========================
MoveClientToIntermission

When the intermission starts, this will be called for all players.
If a new client connects, this will be called after the spawn function.
========================
*/
void MoveClientToIntermission( gentity_t *ent )
{
  // take out of follow mode if needed
  if( ent->client->sess.spectatorState == SPECTATOR_FOLLOW )
    G_StopFollowing( ent );

  // move to the spot
  VectorCopy( level.intermission_origin, ent->s.origin );
  VectorCopy( level.intermission_origin, ent->client->ps.origin );
  VectorCopy( level.intermission_angle, ent->client->ps.viewangles );
  ent->client->ps.pm_type = PM_INTERMISSION;

  // clean up powerup info
  memset( ent->client->ps.powerups, 0, sizeof( ent->client->ps.powerups ) );

  ent->client->ps.eFlags = 0;
  ent->s.eFlags = 0;
  ent->s.eType = ET_GENERAL;
  ent->s.modelindex = 0;
  ent->s.loopSound = 0;
  ent->s.event = 0;
  ent->r.contents = 0;
}

/*
==================
FindIntermissionPoint

This is also used for spectator spawns
==================
*/
void FindIntermissionPoint( void )
{
  gentity_t *ent, *target;
  vec3_t    dir;

  // find the intermission spot
  ent = G_Find( NULL, FOFS( classname ), "info_player_intermission" );

  if( !ent )
  { // the map creator forgot to put in an intermission point...
    SelectSpawnPoint( vec3_origin, level.intermission_origin, level.intermission_angle );
  }
  else
  {
    VectorCopy( ent->s.origin, level.intermission_origin );
    VectorCopy( ent->s.angles, level.intermission_angle );
    // if it has a target, look towards it
    if( ent->target )
    {
      target = G_PickTarget( ent->target );

      if( target )
      {
        VectorSubtract( target->s.origin, level.intermission_origin, dir );
        vectoangles( dir, level.intermission_angle );
      }
    }
  }

}

/*
==================
BeginIntermission
==================
*/
void BeginIntermission( void )
{
  int     i;
  gentity_t *client;

  if( level.intermissiontime )
    return;   // already active

  level.intermissiontime = level.time;
  FindIntermissionPoint( );

  // move all clients to the intermission point
  for( i = 0; i < level.maxclients; i++ )
  {
    client = g_entities + i;

    if( !client->inuse )
      continue;

    // respawn if dead
    if( client->health <= 0 )
      respawn(client);

    MoveClientToIntermission( client );
  }

  // send the current scoring to all clients
  SendScoreboardMessageToAllClients( );
}


/*
=============
ExitLevel

When the intermission has been exited, the server is either killed
or moved to a new level based on the "nextmap" cvar

=============
*/
void ExitLevel( void )
{
  int       i;
  gclient_t *cl;

  if( G_MapRotationActive( ) )
    G_AdvanceMapRotation( );
  else
    trap_SendConsoleCommand( EXEC_APPEND, "vstr nextmap\n" );

  level.changemap = NULL;
  level.intermissiontime = 0;

  // reset all the scores so we don't enter the intermission again
  for( i = 0; i < g_maxclients.integer; i++ )
  {
    cl = level.clients + i;
    if( cl->pers.connected != CON_CONNECTED )
      continue;

    cl->ps.persistant[ PERS_SCORE ] = 0;
  }

  // we need to do this here before chaning to CON_CONNECTING
  G_WriteSessionData( );

  // change all client states to connecting, so the early players into the
  // next level will know the others aren't done reconnecting
  for( i = 0; i < g_maxclients.integer; i++ )
  {
    if( level.clients[ i ].pers.connected == CON_CONNECTED )
      level.clients[ i ].pers.connected = CON_CONNECTING;
  }

}

/*
=================
G_LogPrintf

Print to the logfile with a time stamp if it is open
=================
*/
void QDECL G_LogPrintf( const char *fmt, ... )
{
  va_list argptr;
  char    string[ 1024 ];
  int     min, tens, sec;

  sec = level.time / 1000;

  min = sec / 60;
  sec -= min * 60;
  tens = sec / 10;
  sec -= tens * 10;

  Com_sprintf( string, sizeof( string ), "%3i:%i%i ", min, tens, sec );

  va_start( argptr, fmt );
  vsprintf( string +7 , fmt,argptr );
  va_end( argptr );

  if( g_dedicated.integer )
    G_Printf( "%s", string + 7 );

  if( !level.logFile )
    return;

  trap_FS_Write( string, strlen( string ), level.logFile );
}

/*
=================
G_SendGameStat
=================
*/
void G_SendGameStat( pTeam_t team )
{
  char      map[ MAX_STRING_CHARS ];
  char      teamChar;
  char      data[ BIG_INFO_STRING ];
  char      entry[ MAX_STRING_CHARS ];
  int       i, dataLength, entryLength;
  gclient_t *cl;

  trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );

  switch( team )
  {
    case PTE_ALIENS:  teamChar = 'A'; break;
    case PTE_HUMANS:  teamChar = 'H'; break;
    case PTE_NONE:    teamChar = 'L'; break;
    default: return;
  }

  Com_sprintf( data, BIG_INFO_STRING,
      "%s T:%c A:%f H:%f M:%s D:%d AS:%d AS2T:%d AS3T:%d HS:%d HS2T:%d HS3T:%d CL:%d",
      Q3_VERSION,
      teamChar,
      level.averageNumAlienClients,
      level.averageNumHumanClients,
      map,
      level.time - level.startTime,
      g_alienStage.integer,
      level.alienStage2Time - level.startTime,
      level.alienStage3Time - level.startTime,
      g_humanStage.integer,
      level.humanStage2Time - level.startTime,
      level.humanStage3Time - level.startTime,
      level.numConnectedClients );

  dataLength = strlen( data );

  for( i = 0; i < level.numConnectedClients; i++ )
  {
    int ping;

    cl = &level.clients[ level.sortedClients[ i ] ];

    if( cl->pers.connected == CON_CONNECTING )
      ping = -1;
    else
      ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

    switch( cl->ps.stats[ STAT_PTEAM ] )
    {
      case PTE_ALIENS:  teamChar = 'A'; break;
      case PTE_HUMANS:  teamChar = 'H'; break;
      case PTE_NONE:    teamChar = 'S'; break;
      default: return;
    }

    Com_sprintf( entry, MAX_STRING_CHARS,
      " %s %c %d %d %d",
      cl->pers.netname,
      teamChar,
      cl->ps.persistant[ PERS_SCORE ],
      ping,
      ( level.time - cl->pers.enterTime ) / 60000 );

    entryLength = strlen( entry );

    if( dataLength + entryLength > MAX_STRING_CHARS )
      break;

    Q_strncpyz( data + dataLength, entry, BIG_INFO_STRING );
    dataLength += entryLength;
  }

  trap_SendGameStat( data );
}

/*
================
LogExit

Append information about this game to the log file
================
*/
void LogExit( const char *string )
{
  int         i, numSorted;
  gclient_t   *cl;
  gentity_t   *ent;

  G_LogPrintf( "Exit: %s\n", string );

  level.intermissionQueued = level.time;

  // this will keep the clients from playing any voice sounds
  // that will get cut off when the queued intermission starts
  trap_SetConfigstring( CS_INTERMISSION, "1" );

  // don't send more than 32 scores (FIXME?)
  numSorted = level.numConnectedClients;
  if( numSorted > 32 )
    numSorted = 32;

  for( i = 0; i < numSorted; i++ )
  {
    int   ping;

    cl = &level.clients[ level.sortedClients[ i ] ];

    if( cl->ps.stats[ STAT_PTEAM ] == PTE_NONE )
      continue;

    if( cl->pers.connected == CON_CONNECTING )
      continue;

    ping = cl->ps.ping < 999 ? cl->ps.ping : 999;

    G_LogPrintf( "score: %i  ping: %i  client: %i %s\n",
      cl->ps.persistant[ PERS_SCORE ], ping, level.sortedClients[ i ],
      cl->pers.netname );

  }

  for( i = 1, ent = g_entities + i ; i < level.num_entities ; i++, ent++ )
  {
    if( !ent->inuse )
      continue;

    if( !Q_stricmp( ent->classname, "trigger_win" ) )
    {
      if( level.lastWin == ent->stageTeam )
        ent->use( ent, ent, ent );
    }
  }

  G_SendGameStat( level.lastWin );
}


/*
=================
CheckIntermissionExit

The level will stay at the intermission for a minimum of 5 seconds
If all players wish to continue, the level will then exit.
If one or more players have not acknowledged the continue, the game will
wait 10 seconds before going on.
=================
*/
void CheckIntermissionExit( void )
{
  int       ready, notReady, numPlayers;
  int       i;
  gclient_t *cl;
  int       readyMask;

  //if no clients are connected, just exit
  if( !level.numConnectedClients )
  {
    ExitLevel( );
    return;
  }

  // see which players are ready
  ready = 0;
  notReady = 0;
  readyMask = 0;
  numPlayers = 0;
  for( i = 0; i < g_maxclients.integer; i++ )
  {
    cl = level.clients + i;
    if( cl->pers.connected != CON_CONNECTED )
      continue;

    if( cl->ps.stats[ STAT_PTEAM ] == PTE_NONE )
      continue;

    if( g_entities[ cl->ps.clientNum ].r.svFlags & SVF_BOT )
      continue;

    if( cl->readyToExit )
    {
      ready++;
      if( i < 16 )
        readyMask |= 1 << i;
    }
    else
      notReady++;

    numPlayers++;
  }

  trap_SetConfigstring( CS_CLIENTS_READY, va( "%d", readyMask ) );

  // never exit in less than five seconds
  if( level.time < level.intermissiontime + 5000 )
    return;

  // if nobody wants to go, clear timer
  if( !ready && numPlayers )
  {
    level.readyToExit = qfalse;
    return;
  }

  // if everyone wants to go, go now
  if( !notReady )
  {
    ExitLevel( );
    return;
  }

  // the first person to ready starts the thirty second timeout
  if( !level.readyToExit )
  {
    level.readyToExit = qtrue;
    level.exitTime = level.time;
  }

  // if we have waited thirty seconds since at least one player
  // wanted to exit, go ahead
  if( level.time < level.exitTime + 30000 )
    return;

  ExitLevel( );
}

/*
=============
ScoreIsTied
=============
*/
qboolean ScoreIsTied( void )
{
  int   a, b;

  if( level.numPlayingClients < 2 )
    return qfalse;

  a = level.clients[ level.sortedClients[ 0 ] ].ps.persistant[ PERS_SCORE ];
  b = level.clients[ level.sortedClients[ 1 ] ].ps.persistant[ PERS_SCORE ];

  return a == b;
}

/*
=================
CheckExitRules

There will be a delay between the time the exit is qualified for
and the time everyone is moved to the intermission spot, so you
can see the last frag.
=================
*/
void CheckExitRules( void )
{
  // if at the intermission, wait for all non-bots to
  // signal ready, then go to next level
  if( level.intermissiontime )
  {
    CheckIntermissionExit( );
    return;
  }

  if( level.intermissionQueued )
  {
    if( level.time - level.intermissionQueued >= INTERMISSION_DELAY_TIME )
    {
      level.intermissionQueued = 0;
      BeginIntermission( );
    }

    return;
  }

  if( g_timelimit.integer && !level.warmupTime )
  {
    if( level.time - level.startTime >= g_timelimit.integer * 60000 )
    {
      level.lastWin = PTE_NONE;
      G_SendCommandFromServer( -1, "print \"Timelimit hit\n\"" );
      LogExit( "Timelimit hit." );
      return;
    }
  }

  if( level.uncondHumanWin ||
      ( ( level.time > level.startTime + 1000 ) &&
        ( level.numAlienSpawns == 0 ) &&
        ( level.numLiveAlienClients == 0 ) ) )
  {
    //humans win
    level.lastWin = PTE_HUMANS;
    G_SendCommandFromServer( -1, "print \"Humans win\n\"");
    LogExit( "Humans win." );
  }
  else if( level.uncondAlienWin ||
           ( ( level.time > level.startTime + 1000 ) &&
             ( level.numHumanSpawns == 0 ) &&
             ( level.numLiveHumanClients == 0 ) ) )
  {
    //aliens win
    level.lastWin = PTE_ALIENS;
    G_SendCommandFromServer( -1, "print \"Aliens win\n\"");
    LogExit( "Aliens win." );
  }
}



/*
========================================================================

FUNCTIONS CALLED EVERY FRAME

========================================================================
*/


/*
==================
CheckVote
==================
*/
void CheckVote( void )
{
  if( level.voteExecuteTime && level.voteExecuteTime < level.time )
  {
    level.voteExecuteTime = 0;

    //SUPAR HAK
    if( !Q_stricmp( level.voteString, "vstr nextmap" ) )
    {
      level.lastWin = PTE_NONE;
      LogExit( "Vote for next map." );
    }
    else
      trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.voteString ) );
  }

  if( !level.voteTime )
    return;

  if( level.time - level.voteTime >= VOTE_TIME )
  {
    if( level.voteYes > level.voteNo )
    {
      // execute the command, then remove the vote
      G_SendCommandFromServer( -1, "print \"Vote passed\n\"" );
      level.voteExecuteTime = level.time + 3000;
    }
    else
    {
      // same behavior as a timeout
      G_SendCommandFromServer( -1, "print \"Vote failed\n\"" );
    }
  }
  else
  {
    if( level.voteYes > level.numConnectedClients / 2 )
    {
      // execute the command, then remove the vote
      G_SendCommandFromServer( -1, "print \"Vote passed\n\"" );
      level.voteExecuteTime = level.time + 3000;
    }
    else if( level.voteNo >= level.numConnectedClients / 2 )
    {
      // same behavior as a timeout
      G_SendCommandFromServer( -1, "print \"Vote failed\n\"" );
    }
    else
    {
      // still waiting for a majority
      return;
    }
  }

  level.voteTime = 0;
  trap_SetConfigstring( CS_VOTE_TIME, "" );
}


/*
==================
CheckTeamVote
==================
*/
void CheckTeamVote( int team )
{
  int cs_offset;

  if ( team == PTE_HUMANS )
    cs_offset = 0;
  else if ( team == PTE_ALIENS )
    cs_offset = 1;
  else
    return;

  if( !level.teamVoteTime[ cs_offset ] )
    return;

  if( level.time - level.teamVoteTime[ cs_offset ] >= VOTE_TIME )
  {
    G_SendCommandFromServer( -1, "print \"Team vote failed\n\"" );
  }
  else
  {
    if( level.teamVoteYes[ cs_offset ] > level.numteamVotingClients[ cs_offset ] / 2 )
    {
      // execute the command, then remove the vote
      G_SendCommandFromServer( -1, "print \"Team vote passed\n\"" );
      //
      trap_SendConsoleCommand( EXEC_APPEND, va( "%s\n", level.teamVoteString[ cs_offset ] ) );
    }
    else if( level.teamVoteNo[ cs_offset ] >= level.numteamVotingClients[ cs_offset ] / 2 )
    {
      // same behavior as a timeout
      G_SendCommandFromServer( -1, "print \"Team vote failed\n\"" );
    }
    else
    {
      // still waiting for a majority
      return;
    }
  }

  level.teamVoteTime[ cs_offset ] = 0;
  trap_SetConfigstring( CS_TEAMVOTE_TIME + cs_offset, "" );
}


/*
==================
CheckCvars
==================
*/
void CheckCvars( void )
{
  static int lastMod = -1;

  if( g_password.modificationCount != lastMod )
  {
    lastMod = g_password.modificationCount;

    if( *g_password.string && Q_stricmp( g_password.string, "none" ) )
      trap_Cvar_Set( "g_needpass", "1" );
    else
      trap_Cvar_Set( "g_needpass", "0" );
  }
}

/*
=============
G_RunThink

Runs thinking code for this frame if necessary
=============
*/
void G_RunThink( gentity_t *ent )
{
  float thinktime;

  thinktime = ent->nextthink;
  if( thinktime <= 0 )
    return;

  if( thinktime > level.time )
    return;

  ent->nextthink = 0;
  if( !ent->think )
    G_Error( "NULL ent->think" );

  ent->think( ent );
}

/*
=============
G_EvaluateAcceleration

Calculates the acceleration for an entity
=============
*/
void G_EvaluateAcceleration( gentity_t *ent, int msec )
{
  vec3_t  deltaVelocity;
  vec3_t  deltaAccel;

  VectorSubtract( ent->s.pos.trDelta, ent->oldVelocity, deltaVelocity );
  VectorScale( deltaVelocity, 1.0f / (float)msec, ent->acceleration );

  VectorSubtract( ent->acceleration, ent->oldAccel, deltaAccel );
  VectorScale( deltaAccel, 1.0f / (float)msec, ent->jerk );

  VectorCopy( ent->s.pos.trDelta, ent->oldVelocity );
  VectorCopy( ent->acceleration, ent->oldAccel );
}

/*
================
G_RunFrame

Advances the non-player objects in the world
================
*/
void G_RunFrame( int levelTime )
{
  int       i;
  gentity_t *ent;
  int       msec;
  int       start, end;

  // if we are waiting for the level to restart, do nothing
  if( level.restarted )
    return;

  level.framenum++;
  level.previousTime = level.time;
  level.time = levelTime;
  msec = level.time - level.previousTime;

  //TA: seed the rng
  srand( level.framenum );

  // get any cvar changes
  G_UpdateCvars( );

  //
  // go through all allocated objects
  //
  start = trap_Milliseconds( );
  ent = &g_entities[ 0 ];

  for( i = 0; i < level.num_entities; i++, ent++ )
  {
    if( !ent->inuse )
      continue;

    // clear events that are too old
    if( level.time - ent->eventTime > EVENT_VALID_MSEC )
    {
      if( ent->s.event )
      {
        ent->s.event = 0; // &= EV_EVENT_BITS;
        if ( ent->client )
        {
          ent->client->ps.externalEvent = 0;
          //ent->client->ps.events[0] = 0;
          //ent->client->ps.events[1] = 0;
        }
      }

      if( ent->freeAfterEvent )
      {
        // tempEntities or dropped items completely go away after their event
        G_FreeEntity( ent );
        continue;
      }
      else if( ent->unlinkAfterEvent )
      {
        // items that will respawn will hide themselves after their pickup event
        ent->unlinkAfterEvent = qfalse;
        trap_UnlinkEntity( ent );
      }
    }

    // temporary entities don't think
    if( ent->freeAfterEvent )
      continue;

    //TA: calculate the acceleration of this entity
    if( ent->evaluateAcceleration )
      G_EvaluateAcceleration( ent, msec );

    if( !ent->r.linked && ent->neverFree )
      continue;

    if( ent->s.eType == ET_MISSILE )
    {
      G_RunMissile( ent );
      continue;
    }

    if( ent->s.eType == ET_BUILDABLE )
    {
      G_BuildableThink( ent, msec );
      continue;
    }

    if( ent->s.eType == ET_CORPSE || ent->physicsObject )
    {
      G_Physics( ent, msec );
      continue;
    }

    if( ent->s.eType == ET_MOVER )
    {
      G_RunMover( ent );
      continue;
    }

    if( i < MAX_CLIENTS )
    {
      G_RunClient( ent );
      continue;
    }

    G_RunThink( ent );
  }
  end = trap_Milliseconds();

  start = trap_Milliseconds();

  // perform final fixups on the players
  ent = &g_entities[ 0 ];

  for( i = 0; i < level.maxclients; i++, ent++ )
  {
    if( ent->inuse )
      ClientEndFrame( ent );
  }

  end = trap_Milliseconds();

  //TA:
  G_CountSpawns( );
  G_CalculateBuildPoints( );
  G_CalculateStages( );
  G_SpawnClients( PTE_ALIENS );
  G_SpawnClients( PTE_HUMANS );
  G_CalculateAvgPlayers( );
  G_UpdateZaps( msec );

  //send any pending commands
  G_ProcessCommandQueues( );

  // see if it is time to end the level
  CheckExitRules( );

  // update to team status?
  CheckTeamStatus( );

  // cancel vote if timed out
  CheckVote( );

  // check team votes
  CheckTeamVote( PTE_HUMANS );
  CheckTeamVote( PTE_ALIENS );

  // for tracking changes
  CheckCvars( );

  if( g_listEntity.integer )
  {
    for( i = 0; i < MAX_GENTITIES; i++ )
      G_Printf( "%4i: %s\n", i, g_entities[ i ].classname );

    trap_Cvar_Set( "g_listEntity", "0" );
  }
}

