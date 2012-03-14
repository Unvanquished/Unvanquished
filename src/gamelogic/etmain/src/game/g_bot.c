/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

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

// g_bot.c

#include "../game/g_local.h"
#include "../../../src/shared/q_shared.h"
#include "../game/botlib.h" //bot lib interface
#include "../game/be_aas.h"
#include "../game/be_ea.h"
#include "../game/be_ai_gen.h"
#include "../game/be_ai_goal.h"
#include "../game/be_ai_move.h"
#include "../botai/botai.h" //bot ai interface
#include "../botai/ai_main.h"
#include "../botai/chars.h"
#include "../botai/ai_team.h"
#include "../botai/ai_dmq3.h"
#include "../game/be_ai_chat.h"

static int  g_numBots;
static char g_botInfos[ MAX_BOTS ][ MAX_INFO_STRING ];

int         g_numArenas;
static char g_arenaInfos[ MAX_ARENAS ][ MAX_INFO_STRING ];

#define BOT_BEGIN_DELAY_BASE      2000
#define BOT_BEGIN_DELAY_INCREMENT 1500

#define BOT_SPAWN_QUEUE_DEPTH     16

typedef struct
{
	int clientNum;
	int spawnTime;
} botSpawnQueue_t;

static int             botBeginDelay;
static botSpawnQueue_t botSpawnQueue[ BOT_SPAWN_QUEUE_DEPTH ];

vmCvar_t               bot_minplayers;
vmCvar_t               bot_eventeams;
vmCvar_t               bot_defaultskill;

// Mad Doc - TDF
// for bot debugging purposes only.
vmCvar_t         bot_debug; // if set, draw "thought bubbles" for crosshair-selected bot
vmCvar_t         bot_debug_curAINode; // the text of the current ainode for the bot begin debugged
vmCvar_t         bot_debug_alertState; // alert state of the bot being debugged
vmCvar_t         bot_debug_pos; // coords of the bot being debugged
vmCvar_t         bot_debug_weaponAutonomy; // weapon autonomy of the bot being debugged
vmCvar_t         bot_debug_movementAutonomy; // movement autonomy of the bot being debugged
vmCvar_t         bot_debug_cover_spot; // What cover spot are we going to?
vmCvar_t         bot_debug_anim; // what animation is the bot playing?

extern gentity_t *podium1;
extern gentity_t *podium2;
extern gentity_t *podium3;

// TTimo gcc: defined but not used
#if 0

/*
===============
G_LoadArenas
===============
*/
static void G_LoadArenas( void )
{
#ifdef QUAKESTUFF
	int          len;
	char         *filename;
	vmCvar_t     arenasFile;
	fileHandle_t f;
	int          n;
	char         buf[ MAX_ARENAS_TEXT ];

	trap_Cvar_Register( &arenasFile, "g_arenasFile", "", CVAR_INIT | CVAR_ROM );

	if( *arenasFile.string )
	{
		filename = arenasFile.string;
	}
	else
	{
		filename = "scripts/arenas.txt";
	}

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if( !f )
	{
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}

	if( len >= MAX_ARENAS_TEXT )
	{
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_ARENAS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[ len ] = 0;
	trap_FS_FCloseFile( f );

	g_numArenas = Com_ParseInfos( buf, MAX_ARENAS, g_arenaInfos );
	trap_Print( va( "%i arenas parsed\n", g_numArenas ) );

	for( n = 0; n < g_numArenas; n++ )
	{
		Info_SetValueForKey( g_arenaInfos[ n ], "num", va( "%i", n ) );
	}

#endif
}

#endif

/*
===============
G_GetArenaInfoByNumber
===============
*/
const char     *G_GetArenaInfoByMap( const char *map )
{
	int n;

	for( n = 0; n < g_numArenas; n++ )
	{
		if( Q_stricmp( Info_ValueForKey( g_arenaInfos[ n ], "map" ), map ) == 0 )
		{
			return g_arenaInfos[ n ];
		}
	}

	return NULL;
}

// TTimo: defined but not used
#if 0

/*
=================
PlayerIntroSound
=================
*/
static void PlayerIntroSound( const char *modelAndSkin )
{
	char model[ MAX_QPATH ];
	char *skin;

	Q_strncpyz( model, modelAndSkin, sizeof( model ) );
	skin = Q_strrchr( model, '/' );

	if( skin )
	{
		*skin++ = '\0';
	}
	else
	{
		skin = model;
	}

	if( Q_stricmp( skin, "default" ) == 0 )
	{
		skin = model;
	}

	trap_SendConsoleCommand( EXEC_APPEND, va( "play sound/player/announce/%s.wav\n", skin ) );
}

#endif

/*
===============
G_AddRandomBot
===============
*/
void G_AddRandomBot( int team )
{
	char *teamstr;
	int  skill;

	skill = trap_Cvar_VariableIntegerValue( "bot_defaultskill" );

	if( team == TEAM_AXIS )
	{
		teamstr = "red";
	}
	else if( team == TEAM_ALLIES )
	{
		teamstr = "blue";
	}
	else
	{
		teamstr = "";
	}

	trap_SendConsoleCommand( EXEC_INSERT, va( "addbot %i %s %i\n", skill, teamstr, 0 ) );

	/*
	        int   i, n, num, skill;
	        char  *value, netname[36], *teamstr;
	        gclient_t *cl;

	        num = 0;
	        for ( n = 0; n < g_numBots ; n++ ) {
	                value = Info_ValueForKey( g_botInfos[n], "name" );
	                //
	                for ( i=0 ; i< g_maxclients.integer ; i++ ) {
	                        cl = level.clients + i;
	                        if ( cl->pers.connected != CON_CONNECTED ) {
	                                continue;
	                        }
	                        if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
	                                continue;
	                        }
	                        if ( team >= 0 && cl->sess.sessionTeam != team ) {
	                                continue;
	                        }
	                        if ( !Q_stricmp( value, cl->pers.netname ) ) {
	                                break;
	                        }
	                }
	                if (i >= g_maxclients.integer) {
	                        num++;
	                }
	        }
	        num = random() * num;
	        for ( n = 0; n < g_numBots ; n++ ) {
	                value = Info_ValueForKey( g_botInfos[n], "name" );
	                //
	                for ( i=0 ; i< g_maxclients.integer ; i++ ) {
	                        cl = level.clients + i;
	                        if ( cl->pers.connected != CON_CONNECTED ) {
	                                continue;
	                        }
	                        if ( !(g_entities[cl->ps.clientNum].r.svFlags & SVF_BOT) ) {
	                                continue;
	                        }
	                        if ( team >= 0 && cl->sess.sessionTeam != team ) {
	                                continue;
	                        }
	                        if ( !Q_stricmp( value, cl->pers.netname ) ) {
	                                break;
	                        }
	                }
	                if (i >= g_maxclients.integer) {
	                        num--;
	                        if (num <= 0) {
	                                skill = trap_Cvar_VariableIntegerValue( "g_spSkill" );
	                                if (team == TEAM_AXIS) teamstr = "red";
	                                else if (team == TEAM_ALLIES) teamstr = "blue";
	                                else teamstr = "";
	                                strncpy(netname, value, sizeof(netname)-1);
	                                netname[sizeof(netname)-1] = '\0';
	                                Q_CleanStr(netname);
	                                trap_SendConsoleCommand( EXEC_INSERT, va("addbot %i %s %i\n", skill, teamstr, 0) );
	                                return;
	                        }
	                }
	        }
	*/
}

/*
===============
G_RemoveRandomBot
===============
*/
int G_RemoveRandomBot( int team )
{
	int       i;
	char      netname[ 36 ];
	gclient_t *cl;

	for( i = 0; i < g_maxclients.integer; i++ )
	{
		cl = level.clients + i;

		if( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if( !( g_entities[ cl->ps.clientNum ].r.svFlags & SVF_BOT ) )
		{
			continue;
		}

		if( team >= 0 && cl->sess.sessionTeam != team )
		{
			continue;
		}

		strcpy( netname, cl->pers.netname );
		Q_CleanStr( netname );
		trap_SendConsoleCommand( EXEC_INSERT, va( "kick \"%s\" 0\n", netname ) );
		return qtrue;
	}

	return qfalse;
}

/*
===============
G_CountHumanPlayers
===============
*/
int G_CountHumanPlayers( int team )
{
	int       i, num;
	gclient_t *cl;

	num = 0;

	for( i = 0; i < g_maxclients.integer; i++ )
	{
		cl = level.clients + i;

		if( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if( g_entities[ cl->ps.clientNum ].r.svFlags & SVF_BOT )
		{
			continue;
		}

		if( team >= 0 && cl->sess.sessionTeam != team )
		{
			continue;
		}

		num++;
	}

	return num;
}

/*
===============
G_CountBotPlayers
===============
*/
int G_CountBotPlayers( int team )
{
	int       i, n, num;
	gclient_t *cl;

	num = 0;

	for( i = 0; i < g_maxclients.integer; i++ )
	{
		cl = level.clients + i;

		if( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if( !( g_entities[ cl->ps.clientNum ].r.svFlags & SVF_BOT ) )
		{
			continue;
		}

		if( team >= 0 && cl->sess.sessionTeam != team )
		{
			continue;
		}

		num++;
	}

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ )
	{
		if( !botSpawnQueue[ n ].spawnTime )
		{
			continue;
		}

		if( botSpawnQueue[ n ].spawnTime > level.time )
		{
			continue;
		}

		num++;
	}

	return num;
}

/*
===============
G_CheckMinimumPlayers
===============
*/
void G_CheckMinimumPlayers( void )
{
	int        minplayers /*, weakestTeam, strongestTeam */;
	int        humanplayers[ TEAM_NUM_TEAMS ], botplayers[ TEAM_NUM_TEAMS ], players[ TEAM_NUM_TEAMS ];
	static int checkminimumplayers_time = 0;

	// wait until the system is ready
	if( !level.initStaticEnts )
	{
		return;
	}

	//only check once each second
	if( checkminimumplayers_time < level.time && checkminimumplayers_time > level.time - 1000 )
	{
		return;
	}

	// More safety on initial load for MP
	if( !G_IsSinglePlayerGame() && level.time - level.startTime < 7500 )
	{
		return;
	}

	humanplayers[ TEAM_AXIS ] = G_CountHumanPlayers( TEAM_AXIS );
	botplayers[ TEAM_AXIS ] = G_CountBotPlayers( TEAM_AXIS );
	players[ TEAM_AXIS ] = humanplayers[ TEAM_AXIS ] + botplayers[ TEAM_AXIS ];

	//
	humanplayers[ TEAM_ALLIES ] = G_CountHumanPlayers( TEAM_ALLIES );
	botplayers[ TEAM_ALLIES ] = G_CountBotPlayers( TEAM_ALLIES );
	players[ TEAM_ALLIES ] = humanplayers[ TEAM_ALLIES ] + botplayers[ TEAM_ALLIES ];

	checkminimumplayers_time = level.time;
	trap_Cvar_Update( &bot_minplayers );
	minplayers = bot_minplayers.integer;

	if( minplayers >= g_maxclients.integer / 2 )
	{
		minplayers = ( g_maxclients.integer / 2 ) - 1;
	}

	//

	/*  if (players[TEAM_AXIS] < minplayers) {
	                G_AddRandomBot( TEAM_AXIS );
	                return;
	        }



	        //
	        if (players[TEAM_ALLIES] < minplayers) {
	                G_AddRandomBot( TEAM_ALLIES );
	                return;
	        }

	        //
	        trap_Cvar_Update(&bot_eventeams);
	        if (bot_eventeams.integer) {
	                if (players[TEAM_AXIS] < players[TEAM_ALLIES]) {
	                        weakestTeam = TEAM_AXIS;
	                        strongestTeam = TEAM_ALLIES;
	                } else if (players[TEAM_AXIS] > players[TEAM_ALLIES]) {
	                        weakestTeam = TEAM_ALLIES;
	                        strongestTeam = TEAM_AXIS;
	                } else {
	                        return; // no changes required
	                }
	                //
	                // even up the teams
	                //
	                if (!botplayers[strongestTeam] || (minplayers && players[weakestTeam] <= minplayers)) {
	                        // we have to add players to the weakestTeam
	                        G_AddRandomBot( weakestTeam );
	                        return;
	                } else {
	                        // remove players from the strongestTeam
	                        G_RemoveRandomBot( strongestTeam );
	                        return;
	                }
	        } else if (minplayers) {
	                // remove bots from teams with too many players
	                if (players[TEAM_AXIS] > minplayers && botplayers[TEAM_AXIS]) {
	                        G_RemoveRandomBot( TEAM_AXIS );
	                        return;
	                }
	                if (players[TEAM_ALLIES] > minplayers && botplayers[TEAM_ALLIES]) {
	                        G_RemoveRandomBot( TEAM_ALLIES );
	                        return;
	                }
	        }*/
}

/*
===============
G_CheckBotSpawn
===============
*/
void G_CheckBotSpawn( void )
{
	int n;

//  char    userinfo[MAX_INFO_VALUE];

	G_CheckMinimumPlayers();

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ )
	{
		if( !botSpawnQueue[ n ].spawnTime )
		{
			continue;
		}

		if( botSpawnQueue[ n ].spawnTime > level.time )
		{
			continue;
		}

		ClientBegin( botSpawnQueue[ n ].clientNum );
		botSpawnQueue[ n ].spawnTime = 0;
	}
}

/*
===============
AddBotToSpawnQueue
===============
*/
static void AddBotToSpawnQueue( int clientNum, int delay )
{
	int n;

	for( n = 0; n < BOT_SPAWN_QUEUE_DEPTH; n++ )
	{
		if( !botSpawnQueue[ n ].spawnTime )
		{
			botSpawnQueue[ n ].spawnTime = level.time + delay;
			botSpawnQueue[ n ].clientNum = clientNum;
			return;
		}
	}

	G_Printf( S_COLOR_YELLOW "Unable to delay spawn\n" );
	ClientBegin( clientNum );
}

/*
===============
G_QueueBotBegin
===============
*/
void G_QueueBotBegin( int clientNum )
{
	AddBotToSpawnQueue( clientNum, botBeginDelay );
	botBeginDelay += BOT_BEGIN_DELAY_INCREMENT;
}

/*
===============
G_BotConnect
===============
*/
qboolean G_BotConnect( int clientNum, qboolean restart )
{
	bot_settings_t settings;
	char           userinfo[ MAX_INFO_STRING ];

	trap_GetUserinfo( clientNum, userinfo, sizeof( userinfo ) );

	Q_strncpyz( settings.characterfile, Info_ValueForKey( userinfo, "characterfile" ), sizeof( settings.characterfile ) );
	settings.skill = atoi( Info_ValueForKey( userinfo, "skill" ) );
	Q_strncpyz( settings.team, Info_ValueForKey( userinfo, "team" ), sizeof( settings.team ) );

	if( !BotAISetupClient( clientNum, &settings ) )
	{
		trap_DropClient( clientNum, "BotAISetupClient failed", 0 );
		return qfalse;
	}

	return qtrue;
}

/*
===============
Bot_GetWeaponForClassAndTeam
===============
*/
// returns the wp_ number for weapon "weaponName", or -1 if weapon not allowed for player/class/team combination
int Bot_GetWeaponForClassAndTeam( int classNum, int teamNum, const char *weaponName )
{
	weapon_t weapon = -1; // start by assuming the weapon is invalid

	if( !Q_stricmp( weaponName, "MP40" ) )
	{
		weapon = WP_MP40;
	}
	else if( !Q_stricmp( weaponName, "THOMPSON" ) )
	{
		weapon = WP_THOMPSON;
	}
	else if( !Q_stricmp( weaponName, "KAR98" ) )
	{
		weapon = WP_KAR98;
	}
	else if( !Q_stricmp( weaponName, "CARBINE" ) )
	{
		weapon = WP_CARBINE;
	}
	// Gordon: 25/10/02: adding ability to set pliers
	else if( !Q_stricmp( weaponName, "PLIERS" ) )
	{
		weapon = WP_PLIERS;
	}
	else if( !Q_stricmp( weaponName, "DYNAMITE" ) )
	{
		weapon = WP_DYNAMITE;
	}
	else if( !Q_stricmp( weaponName, "LANDMINE" ) )
	{
		weapon = WP_LANDMINE;
	}
	else if( !Q_stricmp( weaponName, "STEN" ) )
	{
		weapon = WP_STEN;
	}
	else if( !Q_stricmp( weaponName, "PANZERFAUST" ) )
	{
		weapon = WP_PANZERFAUST;
	}
	else if( !Q_stricmp( weaponName, "MORTAR" ) )
	{
		weapon = WP_MORTAR;
	}
	else if( !Q_stricmp( weaponName, "MORTAR_DEPLOYED" ) )
	{
		weapon = WP_MORTAR_SET;
	}
	else if( !Q_stricmp( weaponName, "FLAMETHROWER" ) )
	{
		weapon = WP_FLAMETHROWER;
	}
	else if( !Q_stricmp( weaponName, "FG42" ) )
	{
		weapon = WP_FG42;
	}
	else if( !Q_stricmp( weaponName, "MOBILE_MG42" ) )
	{
		weapon = WP_MOBILE_MG42;
	}
	else if( !Q_stricmp( weaponName, "SYRINGE" ) )
	{
		weapon = WP_MEDIC_SYRINGE;
	}
	else if( !Q_stricmp( weaponName, "MEDKIT" ) )
	{
		weapon = WP_MEDKIT;
	}
	else if( !Q_stricmp( weaponName, "K43" ) )
	{
		weapon = WP_K43;
	}
	else if( !Q_stricmp( weaponName, "GARAND" ) )
	{
		weapon = WP_GARAND;
	}
	else if( !Q_stricmp( weaponName, "SMOKEBOMB" ) )
	{
		weapon = WP_SMOKE_BOMB;
	}
	else if( !Q_stricmp( weaponName, "SATCHEL" ) )
	{
		weapon = WP_SATCHEL;
	}
	else if( !Q_stricmp( weaponName, "AMMOKIT" ) )
	{
		weapon = WP_AMMO;
	}
	else if( !Q_stricmp( weaponName, "NONE" ) )
	{
		weapon = WP_NONE;
	}
	else if( !Q_stricmp( weaponName, "KNIFE" ) )
	{
		weapon = WP_KNIFE;
	}
	else if( !Q_stricmp( weaponName, "LUGER" ) )
	{
		weapon = WP_LUGER;
	}
	else if( !Q_stricmp( weaponName, "COLT" ) )
	{
		weapon = WP_COLT;
	}
	else
	{
		return -1;
	}

	if( BG_CanUseWeapon( classNum, teamNum, weapon ) )
	{
		return weapon;
	}
	else
	{
		return -1;
	}
}

/*
===============
G_AddBot
===============
*/

#define MAX_SLOT_NUMBER 6

/*
=================================
Find the next free slot number (for allies bot)
=================================
*/
#if 0 // rain - unused
static void G_AssignBotSlot( gentity_t *bot )
{
	int       i;
	gentity_t *ent;
	int       slotUsed[ MAX_SLOT_NUMBER ];

	for( i = 0; i < MAX_SLOT_NUMBER; i++ )
	{
		slotUsed[ i ] = qfalse;
	}

	for( i = 0; i < MAX_CLIENTS; i++ )
	{
		ent = g_entities + i;

		if( ent->client && ent != bot && ent->r.svFlags & SVF_BOT && ent->client->sess.sessionTeam == TEAM_ALLIES )
		{
			if( ent->client->botSlotNumber > 0 )
			{
				slotUsed[ ent->client->botSlotNumber - 1 ] = qtrue;
			}
			else
			{
				G_Error( "Buddy %s is not assigned to any number", ent->client->pers.netname );
			}
		}
	}

	for( i = 0; i < MAX_SLOT_NUMBER; i++ )
	{
		if( !slotUsed[ i ] )
		{
			bot->client->botSlotNumber = i + 1; // slot number is non-0
			return;
		}
	}

	G_Error( "Maximum number of allies buddies exceeded" );
}

#endif

static void G_AddBot( const char *name, int skill, const char *team, const char *spawnPoint, int playerClass, int playerWeapon,
                      int characerIndex, const char *respawn, const char *scriptName, int rank, int skills[], qboolean pow )
{
#define MAX_BOTNAMES 1024
	int       clientNum;
	char      *botinfo;
	gentity_t *bot;
	char      *key;
	char      *s;
	char      *botname;

//  char            *model;
	char      userinfo[ MAX_INFO_STRING ];

	// get the botinfo from bots.txt
	botinfo = G_GetBotInfoByName( "WolfBot" );

	if( !botinfo )
	{
		G_Printf( S_COLOR_RED "Error: Bot '%s' not defined\n", name );
		return;
	}

	// create the bot's userinfo
	userinfo[ 0 ] = '\0';

	botname = Info_ValueForKey( botinfo, "funname" );

	if( !botname[ 0 ] )
	{
		botname = Info_ValueForKey( botinfo, "name" );
	}

	Info_SetValueForKey( userinfo, "name", botname );
	Info_SetValueForKey( userinfo, "rate", "25000" );
	Info_SetValueForKey( userinfo, "snaps", "20" );
	Info_SetValueForKey( userinfo, "skill", va( "%i", skill ) );

	s = Info_ValueForKey( botinfo, "aifile" );

	if( !*s )
	{
		trap_Print( S_COLOR_RED "Error: bot has no aifile specified\n" );
		return;
	}

	// have the server allocate a client slot
	clientNum = trap_BotAllocateClient( 0 );  // Arnout: 0 means no prefered clientslot

	if( clientNum == -1 )
	{
		G_Printf( S_COLOR_RED "Unable to add bot.  All player slots are in use.\n" );
		G_Printf( S_COLOR_RED "Start server with more 'open' slots (or check setting of sv_maxclients cvar).\n" );
		return;
	}

	// initialize the bot settings
	if( !team || !*team )
	{
		if( PickTeam( clientNum ) == TEAM_AXIS )
		{
			team = "red";
		}
		else
		{
			team = "blue";
		}
	}

	Info_SetValueForKey( userinfo, "characterfile", Info_ValueForKey( botinfo, "aifile" ) );
	//Info_SetValueForKey( userinfo, "skill", va( "%i", skill ) );
	Info_SetValueForKey( userinfo, "team", team );

	if( spawnPoint && spawnPoint[ 0 ] )
	{
		Info_SetValueForKey( userinfo, "spawnPoint", spawnPoint );
	}

	if( scriptName && scriptName[ 0 ] )
	{
		Info_SetValueForKey( userinfo, "scriptName", scriptName );
	}

	/*  if (playerClass > 0) {
	                Info_SetValueForKey( userinfo, "pClass", va("%i", playerClass) );
	        }

	        if (playerWeapon) {
	                Info_SetValueForKey( userinfo, "pWeapon", va("%i", playerWeapon) );
	        }*/
	// END Mad Doc - TDF

	key = "wolfbot";

	if( !Q_stricmp( ( char * ) name, key ) )
	{
		// read the botnames file, and pick a name that doesnt exist
		fileHandle_t f;
		int          len, i, j, k;
		qboolean     setname = qfalse;
		char         botnames[ 8192 ], *pbotnames, *listbotnames[ MAX_BOTNAMES ], *token, *oldpbotnames;
		int          lengthbotnames[ MAX_BOTNAMES ];

		len = trap_FS_FOpenFile( "botfiles/botnames.txt", &f, FS_READ );

		if( len >= 0 )
		{
			if( len > sizeof( botnames ) )
			{
				G_Error( "botfiles/botnames.txt is too big (max = %i)", ( int ) sizeof( botnames ) );
			}

			memset( botnames, 0, sizeof( botnames ) );
			trap_FS_Read( botnames, len, f );
			pbotnames = botnames;
			// read them in
			i = 0;
			oldpbotnames = pbotnames;

			while( ( token = COM_Parse( &pbotnames ) ) )
			{
				if( !token[ 0 ] )
				{
					break;
				}

				listbotnames[ i ] = strstr( oldpbotnames, token );
				lengthbotnames[ i ] = strlen( token );
				listbotnames[ i ][ lengthbotnames[ i ] ] = 0;
				oldpbotnames = pbotnames;

				if( ++i == MAX_BOTNAMES )
				{
					break;
				}
			}

			//
			if( i > 2 )
			{
				j = rand() % ( i - 1 ); // start at a random spot inthe list

				for( k = j + 1; k != j; k++ )
				{
					if( k == i )
					{
						k = -1; // gets increased on next loop
						continue;
					}

					if( ClientFromName( listbotnames[ k ] ) == -1 )
					{
						// found an unused name
						Info_SetValueForKey( userinfo, "name", listbotnames[ k ] );
						setname = qtrue;
						break;
					}
				}
			}

			//
			trap_FS_FCloseFile( f );
		}

		if( !setname )
		{
			Info_SetValueForKey( userinfo, "name", va( "wolfbot_%i", clientNum + 1 ) );
		}
	}
	else
	{
		Info_SetValueForKey( userinfo, "name", name );
	}

	// if a character was specified, put the index of that character filename in the CS_CHARACTERS table in the userinfo
	if( characerIndex != -1 )
	{
		Info_SetValueForKey( userinfo, "ch", va( "%i", characerIndex ) );
	}

	// if a rank was specified, use that

	/*  if (rank != -1) {
	                Info_SetValueForKey(userinfo, "rank", va("%i", rank));
	        }*/

	// END Mad Doc - TDF

	bot = &g_entities[ clientNum ];
	bot->r.svFlags |= SVF_BOT;

	if( pow )
	{
		bot->r.svFlags |= SVF_POW;
	}

	bot->inuse = qtrue;
	bot->aiName = bot->client->pers.netname;

	// register the userinfo
	trap_SetUserinfo( clientNum, userinfo );

	// have it connect to the game as a normal client
	if( ( s = ClientConnect( clientNum, qtrue, qtrue ) ) )
	{
		G_Printf( S_COLOR_RED "Unable to add bot: %s\n", s );
		return;
	}

	SetTeam( bot, ( char * ) team, qtrue, -1, -1, qfalse );

	/*  if( skills ) {
	                int i;

	                for( i = 0; i < SK_NUM_SKILLS; i++ ) {
	                        bot->client->sess.skill[i] = skills[i];
	                }
	        }*/
	return;
}

/*
===============
Svcmd_AddBot_f
===============
*/
void Svcmd_AddBot_f( void )
{
	int  skill;
	int  delay;
	char name[ MAX_TOKEN_CHARS ];
	char string[ MAX_TOKEN_CHARS ];
	char team[ MAX_TOKEN_CHARS ];

	// are bots enabled?
	if( !bot_enable.integer )
	{
		return;
	}

	// name
	trap_Argv( 1, name, sizeof( name ) );   /// read it just so we can check if it's a name (old method)

	if( name[ 0 ] && !Q_stricmp( name, "?" ) )
	{
		trap_Print( "Usage: Addbot [skill 1-4] [team (RED/BLUE)] [msec delay]\n" );
		return;
	}

	Q_strncpyz( name, "WolfBot", sizeof( name ) );   // RF, hard code the bots for wolf

	if( !name[ 0 ] )
	{
		trap_Print( "Usage: Addbot [skill 1-4] [team (RED/BLUE)] [msec delay]\n" );
		return;
	}

	// skill
	trap_Argv( 1, string, sizeof( string ) );

	if( !string[ 0 ] )
	{
		trap_Cvar_Update( &bot_defaultskill );
		skill = bot_defaultskill.integer;
	}
	else
	{
		skill = atoi( string );
	}

	// team
	trap_Argv( 2, team, sizeof( team ) );

	// delay
	trap_Argv( 3, string, sizeof( string ) );

	if( !string[ 0 ] )
	{
		delay = 0;
	}
	else
	{
		delay = atoi( string );
	}

	G_AddBot( name, skill, team, NULL, 0, 0, -1, NULL, NULL, -1, NULL, qfalse );

	// if this was issued during gameplay and we are playing locally,
	// go ahead and load the bot's media immediately
	if( level.time - level.startTime > 1000 && trap_Cvar_VariableIntegerValue( "cl_running" ) )
	{
	}
}

typedef struct
{
	char     *cmd;
	char     *string;
	qboolean appendParams;
	char     *help;
	int      count;
} spawnBotCommand_t;

int G_ClassForString( char *string )
{
	if( !Q_stricmp( string, "ANY" ) )
	{
		return -1;
	}
	else if( !Q_stricmp( string, "soldier" ) )
	{
		return PC_SOLDIER;
	}
	else if( !Q_stricmp( string, "medic" ) )
	{
		return PC_MEDIC;
	}
	else if( !Q_stricmp( string, "engineer" ) )
	{
		return PC_ENGINEER;
	}
	else if( !Q_stricmp( string, "lieutenant" ) )
	{
		// FIXME: remove from missionpack
		return PC_FIELDOPS;
	}
	else if( !Q_stricmp( string, "fieldops" ) )
	{
		return PC_FIELDOPS;
	}
	else if( !Q_stricmp( string, "covertops" ) )
	{
		return PC_COVERTOPS;
	}
	else
	{
		G_Error( "unknown player class: %s", string );
		return -1; //shutup compiler
	}
}

void G_BotParseCharacterParms( char *characterFile, int *characterInt )
{
	if( strlen( characterFile ) )
	{
		int characterIndex = G_CharacterIndex( characterFile );

		if( characterIndex )
		{
			*characterInt = characterIndex;
		}
		else
		{
			*characterInt = -1;
		}
	}
	else
	{
		*characterInt = -1;
	}
}

/*
==================
G_SpawnBot
==================
*/
void G_SpawnBot( const char *text )
{
	// bot parameters
	char name[ MAX_TOKEN_CHARS ] = "WolfBot";

	//GS  prevent bot health from counting down to 70 (i.e. don't set STAT_MAX_HEALTH = 70)
	char skill[ MAX_TOKEN_CHARS ] = "4";
	char team[ MAX_TOKEN_CHARS ] = "";
	char pClass[ MAX_TOKEN_CHARS ] = "";
	char pWeapon[ MAX_TOKEN_CHARS ] = "0";
	char spawnPoint[ MAX_TOKEN_CHARS ] = "";
	char respawn[ MAX_TOKEN_CHARS ] = "";
	char scriptName[ MAX_TOKEN_CHARS ] = "WolfBot";
	char characterFile[ MAX_TOKEN_CHARS ] = "";

	// START - Mad Doc - TDF
	char rank[ MAX_TOKEN_CHARS ] = "";
	char botSkills[ MAX_TOKEN_CHARS ] = "";

	// END Mad Doc - TDF

	char              pow[ MAX_TOKEN_CHARS ] = "no";

	// This is the selection meny index used in SetWolfSpawnWeapons
	int               weaponSpawnNumber = -1;

	// parsing vars
	char              *token, *pStr, *old_pStr;
	char              cmd[ MAX_TOKEN_CHARS ], last_cmd[ MAX_TOKEN_CHARS ];
	char              cmd_var[ MAX_TOKEN_CHARS ];
	char              string[ MAX_TOKEN_CHARS ];
	int               j, pClassInt;
	int               characterInt, rankNum;
	int               skills[ SK_NUM_SKILLS ];
	qboolean          prisonerOfWar;
	int               teamNum;

	// parameters
	spawnBotCommand_t params[] =
	{
		{ "/name",       name,          qfalse, "[name]"                                                 },
		{ "/skill",      skill,         qfalse, "[0-4]"                                                  },
		{ "/team",       team,          qfalse, "[AXIS/ALLIES]"                                          },
		{ "/class",      pClass,        qfalse, "[SOLDIER/MEDIC/LIEUTENANT/ENGINEER/COVERTOPS/FIELDOPS]" }, // FIXME: remove LIEUTENANT from missionpack
		{ "/weapon",     pWeapon,       qfalse, "[weaponValue]"                                          },
		{ "/spawnpoint", spawnPoint,    qtrue,  "[targetname]"                                           },
		{ "/respawn",    respawn,       qfalse, "[ON/OFF]"                                               },
		{ "/scriptName", scriptName,    qfalse, "[scriptName]"                                           },
		{ "/character",  characterFile, qfalse, "[character]"                                            },
		// START Mad Doc - TDF
		{ "/rank",       rank,          qfalse, "[rank]"                                                 },
		{ "/skills",     botSkills,     qfalse, "[botskills]"                                            }, // not really to be exposed to script
		// END Mad Doc - TDF
		{ "/pow",        pow,           qfalse, "[yes/no]"                                               },

		{ NULL }
	};
	// special tables
	typedef struct
	{
		char *weapon;
		int  index;
	} spawnBotWeapons_t;

	// TAT 1/16/2003 - uninit'ed data here - getting crazy data for the skills
	memset( &skills, 0, sizeof( skills ) );

	//
	// parse the vars
	pStr = ( char * ) text;
	token = COM_Parse( &pStr );
	Q_strncpyz( cmd, token, sizeof( cmd ) );

	// if this is a question mark, show help info
	if( !Q_stricmp( cmd, "?" ) || !Q_stricmp( cmd, "/?" ) )
	{
		G_Printf
		( "Spawns a bot into the game, with the given parameters.\n\nSPAWNBOT [/param [value]] [/param [value]] ...\n\n  where [/param [value]] may consist of:\n\n" );

		for( j = 0; params[ j ].cmd; j++ )
		{
			G_Printf( "  %s %s\n", params[ j ].cmd, params[ j ].help );
		}

		return;
	}

	//
	// intitializations
	for( j = 0; params[ j ].cmd; j++ )
	{
		params[ j ].count = 0;
	}

	memset( last_cmd, 0, sizeof( last_cmd ) );
	pStr = ( char * ) text;

	//
	// parse each command
	while( cmd[ 0 ] )
	{
		//
		// build the string up to the next parameter change
		token = COM_Parse( &pStr );
		Q_strncpyz( cmd, token, sizeof( cmd ) );

		if( !cmd[ 0 ] )
		{
			break;
		}

		// if we find an "or", then use the last command
		if( !Q_stricmp( cmd, "or" ) )
		{
			// use the last command
			Q_strncpyz( cmd, last_cmd, sizeof( cmd ) );
		}

		//
		// read the parameters
		memset( string, 0, sizeof( string ) );
		token = COM_Parse( &pStr );
		Q_strncpyz( cmd_var, token, sizeof( cmd_var ) );

		if( !cmd_var[ 0 ] )
		{
			break;
		}

		while( qtrue )
		{
			Q_strcat( string, sizeof( string ), cmd_var );
			old_pStr = pStr;
			token = COM_Parse( &pStr );
			Q_strncpyz( cmd_var, token, sizeof( cmd_var ) );

			if( cmd_var[ 0 ] && ( cmd_var[ 0 ] == '/' || !Q_stricmp( cmd_var, "or" ) ) )
			{
				pStr = old_pStr;
				break;
			}

			if( !cmd_var[ 0 ] )
			{
				break;
			}

			Q_strcat( string, sizeof( string ), " " );
		}

		//
		// see if this command exists in the parameters table
		for( j = 0; params[ j ].cmd; j++ )
		{
			if( !Q_stricmp( params[ j ].cmd, cmd ) )
			{
				// found a match, if this field already has an entry, then randomly override it
				if( !params[ j ].count || ( !params[ j ].appendParams && ( ( rand() % ( ++params[ j ].count ) ) == 0 ) ) )
				{
					Q_strncpyz( params[ j ].string, string, sizeof( string ) );
				}
				else if( params[ j ].appendParams )
				{
					// append this token
					Q_strcat( params[ j ].string, sizeof( string ), va( " %s", string ) );
				}

				params[ j ].count++;
				break;
			}
		}

		if( !params[ j ].cmd )
		{
			G_Printf( "G_SpawnBot: unknown parameter: %s\nFor usage info, use \"spawnbot /?\"\n", cmd );
			return;
		}

		//
		Q_strncpyz( last_cmd, cmd, sizeof( last_cmd ) );
		Q_strncpyz( cmd, cmd_var, sizeof( cmd ) );
	}

	//
	if( strlen( pClass ) )
	{
		pClassInt = 1 + G_ClassForString( pClass );
	}
	else
	{
		pClassInt = 0;
	}

	if( !Q_stricmp( team, "red" ) || !Q_stricmp( team, "r" ) || !Q_stricmp( team, "axis" ) )
	{
		teamNum = TEAM_AXIS;
	}
	else if( !Q_stricmp( team, "blue" ) || !Q_stricmp( team, "b" ) || !Q_stricmp( team, "allies" ) )
	{
		teamNum = TEAM_ALLIES;
	}
	else
	{
		// pick the team with the least number of players
		teamNum = PickTeam( -1 );
	}

	G_BotParseCharacterParms( characterFile, &characterInt );

	// Gordon: 27/11/02
	if( *pow && !Q_stricmp( pow, "yes" ) )
	{
		prisonerOfWar = qtrue;
	}
	else
	{
		prisonerOfWar = qfalse;
	}

	// START Mad Doc - TDF
	// special case: if "NONE" is specified, treat this differently
	if( !Q_stricmp( pWeapon, "NONE" ) )
	{
		weaponSpawnNumber = -1;
	}
	// END Mad Doc - TDF
	// START    Mad Doctor I changes, 8/17/2002.
	// If we have a weapon specified, and we have a class specified
	else if( isdigit( pWeapon[ 0 ] ) )
	{
		// Just convert the string to a number
		weaponSpawnNumber = atoi( pWeapon );
	} // if (isdigit(pWeapon[0]))...
	// If we have a weapon specified as a string, and we have a class specified
	else if( pClassInt > 0 )
	{
		// Translate the weapon name into a proper weapon index
		// Get the index for the weapon
		weaponSpawnNumber = Bot_GetWeaponForClassAndTeam( pClassInt - 1, teamNum, pWeapon );

		// Get default weapon
		if( weaponSpawnNumber == -1 )
		{
			weaponSpawnNumber = BG_GetPlayerClassInfo( teamNum, pClassInt - 1 )->classWeapons[ 0 ];
		}
	} // if (Q_stricmp(pWeapon[MAX_TOKEN_CHARS], "0")...
	// Otherwise, no weapon is selected
	else
	{
		// Just use the default
		weaponSpawnNumber = BG_GetPlayerClassInfo( teamNum, pClassInt - 1 )->classWeapons[ 0 ];
	} // else...

	// START Mad Doc - TDF
	rankNum = atoi( rank );

	if( rankNum )
	{
		rankNum--; // people like to start with 1
		// Gordon: coders are people too :(
	}

	if( botSkills[ 0 ] )
	{
		// parse the skills out
		int  i;
		char *pString, *token;

		pString = botSkills;

		for( i = 0; i < SK_NUM_SKILLS; i++ )
		{
			token = COM_ParseExt( &pString, qfalse );
			skills[ i ] = atoi( token );
		}
	}

	//  {"/botskills",  botSkills,      qfalse,     "[botskills]"}, // not really to be exposed to script

// END Mad Doc - TDF

	G_AddBot( name, atoi( skill ), team, spawnPoint, pClassInt, weaponSpawnNumber, characterInt, respawn, scriptName, rankNum,
	          skills, prisonerOfWar );

// END      Mad Doctor I changes, 8/17/2002.
}

/*
==============
Svcmd_SpawnBot
==============
*/
void Svcmd_SpawnBot()
{
	char text[ 1024 ];
	char cmd[ MAX_TOKEN_CHARS ];
	int  i;

	// build the string
	memset( text, 0, sizeof( text ) );

	for( i = 1; i < trap_Argc(); i++ )
	{
		trap_Argv( i, cmd, sizeof( cmd ) );

		//
		if( i > 1 )
		{
			Q_strcat( text, sizeof( text ), " " );
		}

		//
		if( strchr( cmd, ' ' ) )
		{
			Q_strcat( text, sizeof( text ), "\"" );
		}

		//
		Q_strcat( text, sizeof( text ), cmd );

		//
		if( strchr( cmd, ' ' ) )
		{
			Q_strcat( text, sizeof( text ), "\"" );
		}
	}

	G_SpawnBot( text );
}

// TDF - Mad Doc

/*
==============
G_RemoveNamedBot - removes the bot named "name"
==============
*/
int G_RemoveNamedBot( char *name )
{
	int       i;
	char      netname[ 36 ];
	gclient_t *cl;

	for( i = 0; i < g_maxclients.integer; i++ )
	{
		cl = level.clients + i;

		if( cl->pers.connected != CON_CONNECTED )
		{
			continue;
		}

		if( !( g_entities[ cl->ps.clientNum ].r.svFlags & SVF_BOT ) )
		{
			continue;
		}

		// use scriptname, not netname
		if( !Q_stricmp( name, cl->pers.botScriptName ) )
		{
			// found the bot. boot him
			strcpy( netname, cl->pers.netname );
			Q_CleanStr( netname );
			trap_SendConsoleCommand( EXEC_INSERT, va( "kick \"%s\" 0\n", netname ) );
			return qtrue;
		}
	}

	return qfalse;
}

/*
TDF - Mad Doc
===============
Svcmd_RemoveBot_f
===============
*/
void Svcmd_RemoveBot_f( void )
{
	char name[ MAX_TOKEN_CHARS ];

	// are bots enabled?
	if( !bot_enable.integer )
	{
		return;
	}

	// name
	trap_Argv( 1, name, sizeof( name ) );   /// read it just so we can check if it's a name (old method)

	if( !name[ 0 ] )
	{
		trap_Print( "Usage: Removebot name\n" );
		return;
	}

	G_RemoveNamedBot( name );
}

/*
===============
G_LoadBots
===============
*/
static void G_LoadBots( void )
{
	int          len;
	char         *filename;
	vmCvar_t     botsFile;
	fileHandle_t f;
	char         buf[ MAX_BOTS_TEXT ];

	if( !bot_enable.integer )
	{
		return;
	}

	trap_Cvar_Register( &botsFile, "g_botsFile", "", CVAR_INIT | CVAR_ROM );

	if( *botsFile.string )
	{
		filename = botsFile.string;
	}
	else
	{
		filename = "scripts/bots.txt";
	}

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if( !f )
	{
		trap_Print( va( S_COLOR_RED "file not found: %s\n", filename ) );
		return;
	}

	if( len >= MAX_BOTS_TEXT )
	{
		trap_Print( va( S_COLOR_RED "file too large: %s is %i, max allowed is %i", filename, len, MAX_BOTS_TEXT ) );
		trap_FS_FCloseFile( f );
		return;
	}

	trap_FS_Read( buf, len, f );
	buf[ len ] = 0;
	trap_FS_FCloseFile( f );

	g_numBots = Com_ParseInfos( buf, MAX_BOTS, g_botInfos );
	trap_Print( va( "%i bots parsed\n", g_numBots ) );

	// load bot script
	Bot_ScriptLoad();
}

/*
===============
G_GetBotInfoByNumber
===============
*/
char           *G_GetBotInfoByNumber( int num )
{
	if( num < 0 || num >= g_numBots )
	{
		trap_Print( va( S_COLOR_RED "Invalid bot number: %i\n", num ) );
		return NULL;
	}

	return g_botInfos[ num ];
}

/*
===============
G_GetBotInfoByName
===============
*/
char           *G_GetBotInfoByName( const char *name )
{
	int  n;
	char *value;

	for( n = 0; n < g_numBots; n++ )
	{
		value = Info_ValueForKey( g_botInfos[ n ], "name" );

		if( !Q_stricmp( value, name ) )
		{
			return g_botInfos[ n ];
		}
	}

	return NULL;
}

void BotBuildStaticEntityCache( void );
void BotCalculateMg42Spots( void );

/*
=================
G_BotDelayedInit
=================
*/
void G_BotDelayedInit( gentity_t *ent )
{
	G_FreeEntity( ent );

	BotBuildStaticEntityCache();

	// are there explosiveTargets?
	level.explosiveTargets[ 0 ] = GetTargetExplosives( TEAM_AXIS, qtrue );
	level.explosiveTargets[ 1 ] = GetTargetExplosives( TEAM_ALLIES, qtrue );

	// calculate mg42 spots
	BotCalculateMg42Spots();
}

/*
===============
G_InitBots
===============
*/
void G_InitBots( qboolean restart )
{
	G_LoadBots();

	trap_Cvar_Register( &bot_debug, "bot_debug", "0", 0 );

	// the static entity cache is not ready yet
	level.initStaticEnts = qfalse;

	// determine which team is attacking
	level.captureFlagMode = qfalse;

	return;
}

//============================================================================================
extern vec3_t playerMins, playerMaxs;

/*
===============
BotDropToFloor
===============
*/
void BotDropToFloor( gentity_t *ent )
{
	vec3_t  dest;
	trace_t tr;
	vec3_t  checkMins, checkMaxs;

	//----(SA)  move the bounding box for the check in 1 unit on each side so they can butt up against a wall and not startsolid
	VectorCopy( playerMins, checkMins );
	checkMins[ 0 ] += 1;
	checkMins[ 1 ] += 1;
	VectorCopy( playerMaxs, checkMaxs );
	checkMaxs[ 0 ] -= 1;
	checkMaxs[ 1 ] -= 1;

	// use low max Z in case this is a crouch spot
	checkMaxs[ 2 ] = 0;

	// drop to floor
	ent->r.currentOrigin[ 2 ] += 1.0; // fixes QErad -> engine bug?
	VectorSet( dest, ent->r.currentOrigin[ 0 ], ent->r.currentOrigin[ 1 ], ent->r.currentOrigin[ 2 ] - 4096 );
	trap_Trace( &tr, ent->r.currentOrigin, checkMins, checkMaxs, dest, ent->s.number, MASK_PLAYERSOLID );

	if( tr.startsolid )
	{
		// try raising us up some
		if( fabs( ent->r.currentOrigin[ 2 ] - ent->s.origin[ 2 ] ) < 48 )
		{
			ent->r.currentOrigin[ 2 ] += 4;
			BotDropToFloor( ent );
			return;
		}

		G_Printf( "WARNING: %s (%s) in solid at %s\n", ent->classname, ent->targetname, vtos( ent->r.currentOrigin ) );
		return;
	}

	G_SetOrigin( ent, tr.endpos );
	VectorCopy( ent->r.currentOrigin, ent->s.origin );
}

// Equivalent of BotDropToFloor for server entities
void ServerEntityDropToFloor( g_serverEntity_t *ent )
{
	vec3_t  dest;
	trace_t tr;
	vec3_t  checkMins, checkMaxs;

	//----(SA)  move the bounding box for the check in 1 unit on each side so they can butt up against a wall and not startsolid
	VectorCopy( playerMins, checkMins );
	checkMins[ 0 ] += 1;
	checkMins[ 1 ] += 1;
	VectorCopy( playerMaxs, checkMaxs );
	checkMaxs[ 0 ] -= 1;
	checkMaxs[ 1 ] -= 1;

	// use low max Z in case this is a crouch spot
	checkMaxs[ 2 ] = 0;

	// drop to floor
	ent->origin[ 2 ] += 1.0; // fixes QErad -> engine bug?
	VectorSet( dest, ent->origin[ 0 ], ent->origin[ 1 ], ent->origin[ 2 ] - 4096 );
	trap_Trace( &tr, ent->origin, checkMins, checkMaxs, dest, -1, MASK_PLAYERSOLID );

	if( tr.startsolid )
	{
		// try raising us up some
		if( fabs( ent->origin[ 2 ] - ent->origin[ 2 ] ) < 48 )
		{
			ent->origin[ 2 ] += 4;
			ServerEntityDropToFloor( ent );
			return;
		}

		G_Printf( "WARNING: %s (%s) in solid at %s\n", ent->classname, ent->name, vtos( ent->origin ) );
		return;
	}

	VectorCopy( tr.endpos, ent->origin );
}

/*QUAKED ai_marker (1 0.5 0) (-18 -18 -24) (18 18 48) NODROP CROUCHING
AI marker

NODROP means dont drop it to the ground

"targetname" : identifier for this marker
*/

// TAT 11/18/2002 - Use this function as the setup func for ai_markers that are NOT generated from q3map, but are parsed from the external file
void SP_AIMarker_Setup( g_serverEntity_t *ent )
{
	// since we didn't spawn, we haven't done this yet
	if( !( ent->spawnflags & 1 ) )
	{
		ServerEntityDropToFloor( ent );
	}
}

void SP_ai_marker( gentity_t *ent )
{
	if( !( ent->spawnflags & 1 ) )
	{
		BotDropToFloor( ent );
	}

	// TAT 11/13/2002 - use the server entities for this
	CreateServerEntity( ent );

	// free this entity - we should now have a server entity for it
	G_FreeEntity( ent );
}

/*QUAKED bot_sniper_spot (1 0.2 0) (-18 -18 -24) (18 18 48) NODROP CROUCHING
Bots will use this spot to snipe from

NODROP means dont drop it to the ground

  "aiTeam" team to use this spot, if 0, any team can use it 1 = AXIS, 2 = ALLIES
*/
void SP_bot_sniper_spot( gentity_t *ent )
{
	if( !( ent->spawnflags & 1 ) )
	{
		BotDropToFloor( ent );
	}

	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eType = ET_SNIPER_HINT;
}

/*QUAKED bot_landminespot_spot (1 0.2 0) (-18 -18 -24) (18 18 48) NODROP CROUCHING
Bots will use this spot to search for landmines from, target at a info_null

NODROP means dont drop it to the ground

  "aiTeam" team to use this spot, if 0, any team can use it 1 = AXIS, 2 = ALLIES
*/

void bot_landminespot_setup( gentity_t *self )
{
	gentity_t *target = G_FindByTargetname( NULL, self->target );

	if( !target )
	{
		G_FreeEntity( self );
		return;
	}

	VectorCopy( target->s.origin, self->s.origin2 );
}

void SP_bot_landminespot_spot( gentity_t *ent )
{
	// MUST have a target
	if( !ent->target || !*ent->target )
	{
		G_FreeEntity( ent );
		return;
	}

	if( !( ent->spawnflags & 1 ) )
	{
		BotDropToFloor( ent );
	}

	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eType = ET_LANDMINESPOT_HINT;

	ent->think = bot_landminespot_setup;
	ent->nextthink = level.time + FRAMETIME;
}

/*QUAKED bot_attractor (1 0.2 0) (-18 -18 -24) (18 18 48) NODROP
Bots will defend this general area when enabled as a goal

NODROP means dont drop it to the ground
*/
void SP_bot_attractor( gentity_t *ent )
{
	if( !( ent->spawnflags & 1 ) )
	{
		BotDropToFloor( ent );
	}

	ent->r.svFlags |= SVF_NOCLIENT;
	ent->s.eType = ET_ATTRACTOR_HINT;
}

/*QUAKED bot_jump_source (0.5 0.8 0) (-18 -18 -24) (18 18 40) NODROP
Bots will use this spot to jump from

Must recompile .AAS file after adding/changing these entities

NOTE: MUST HAVE A MATCHING BOT_JUMP_DEST

NODROP means dont drop it to the ground
*/
void SP_bot_jump_source( gentity_t *ent )
{
	// only used for bspc process
	G_FreeEntity( ent );
}

/*QUAKED bot_jump_dest (0.5 0.8 0) (-18 -18 -24) (18 18 40) NODROP
Bots will use this spot to jump to

Must recompile .AAS file after adding/changing these entities

NOTE: MUST HAVE A MATCHING BOT_JUMP_SOURCE

NODROP means dont drop it to the ground
*/
void SP_bot_jump_dest( gentity_t *ent )
{
	// only used for bspc process
	G_FreeEntity( ent );
}

/*QUAKED bot_seek_cover_spot (0 0.6 0) (-18 -18 -24) (18 18 48) NODROP CROUCHING CROUCH_TOWARDS PRONING PRONE_TOWARDS EXPOSED NEVER_PRONE
Allies bots will use this spot to seek cover while in follow formation

Join these together in sequence of player progression for best results

!!NOTE!! : each seek_cover_spot can only have ONE parent. A parent can have
multiple children, but you cannot point two spots to the same destination.

NODROP means dont drop it to the ground
CROUCHING means crouch when bot gets there
CROUCH_TOWARD means crouch all the way to the spot (within a resonable distance)
PRONING means prone when bot gets there
PRONE_TOWARD means prone all the way to the spot (within a reasonable distance)
EXPOSED means this spot is in the open, so flee from it easier
NEVER_PRONE means bots should never go prone at this spot

  "angle" is the direction the bot should face when they reach the spot
  "target" next marker(s) in sequence
  "targetname" our identifier for sequencing
*/

void bot_seek_cover_spot_think( g_serverEntity_t *ent )
{
	g_serverEntity_t *trav, *lastTrav;

	//
	if( ent->name )
	{
		// find our parent
		trav = NULL;

		while( ( trav = FindServerEntity( trav, SE_FOFS( target ), ent->name ) ) )
		{
			if( !Q_stricmp( trav->classname, ent->classname ) )
			{
				ent->parent = trav;
				break;
			}
		}
	}

	// Don't bother looking for a target if none is specified.  Mad Doctor I
	if( ent->target && ( ent->target[ 0 ] != 0 ) )
	{
		//
		// now find our ->target_ent, if we have multiple targets, then use ->chain
		trav = NULL;
		lastTrav = NULL;

		while( ( trav = FindServerEntity( trav, SE_FOFS( name ), ent->target ) ) )
		{
			if( Q_stricmp( trav->classname, ent->classname ) )
			{
				G_Error( "bot_seek_cover_spot at %s is targetting a %s", vtos( ent->origin ), trav->classname );
			}

			if( !ent->target_ent )
			{
				ent->target_ent = trav;
				//VectorSubtract( trav->s.origin, ent->s.origin, ent->movedir );
				//VectorNormalize( ent->movedir );
			}

			if( lastTrav )
			{
				lastTrav->chain = trav;
			}

			lastTrav = trav;
		}
	} // if (ent->target[0] != 0)...
}

// TAT 11/18/2002 - Use this function as the setup func for cover spots that are NOT generated from q3map, but are parsed from the external file
void SP_SeekCover_Setup( g_serverEntity_t *ent )
{
	// since we didn't spawn, we haven't done this yet
	if( !( ent->spawnflags & 1 ) )
	{
		ServerEntityDropToFloor( ent );
	}

	// now do the normal thing
	bot_seek_cover_spot_think( ent );
}

void SP_Seek_Cover_Spawn( gentity_t *ent, int team )
{
	g_serverEntity_t *svEnt;

	if( !( ent->spawnflags & 1 ) )
	{
		BotDropToFloor( ent );
	}

	ent->aiTeam = team;

	if( ent->targetname )
	{
		// TAT 11/13/2002 - seek cover spots are special server only entities
		//      so let's make one with our data
		svEnt = CreateServerEntity( ent );
		//      set the setup func
		svEnt->setup = bot_seek_cover_spot_think;
	}

	// free this entity - we should now have a server entity for it
	G_FreeEntity( ent );
}

void SP_bot_seek_cover_spot( gentity_t *ent )
{
	// for allied bots only
	SP_Seek_Cover_Spawn( ent, 2 );
}

/*QUAKED bot_axis_seek_cover_spot (0.6 0.6 0.8) (-18 -18 -24) (18 18 48) NODROP CROUCHING CROUCH_TOWARDS PRONING PRONE_TOWARDS EXPOSED NEVER_PRONE
Axis bots will use this spot to seek cover while fighting

!!NOTE!! : each axis_seek_cover_spot can only have ONE parent. A parent can have
multiple children, but you cannot point two spots to the same destination.

NODROP means dont drop it to the ground
CROUCHING means crouch when bot gets there
CROUCH_TOWARD means crouch all the way to the spot (within a resonable distance)
PRONING means prone when bot gets there
PRONE_TOWARD means prone all the way to the spot (within a reasonable distance)
EXPOSED means this spot is in the open, so flee from it easier
NEVER_PRONE means bots should never go prone at this spot

  "angle" is the direction the bot should face when they reach the spot
  "target" next marker(s) in sequence
  "targetname" our identifier for sequencing
*/
void SP_bot_axis_seek_cover_spot( gentity_t *ent )
{
	// for axis bots only
	SP_Seek_Cover_Spawn( ent, 1 );
}

/*QUAKED bot_seek_cover_sequence (1 0.2 0) ?
Point this to the bot_seek_cover_spot that should be used for following or bot_axis_seek_cover_spot for axis bots to use

  target : targetname of the spot to start the sequence. can point to multiple spots.
*/
void bot_seek_cover_sequence_touch( gentity_t *self, gentity_t *other, trace_t *trace )
{
	// if we have a seek cover spot, then set the guy who stepped on me to be using it
	if( self->serverEntity )
	{
		other->teamchain = self;
	}
	else
	{
		// stop using seek_cover_spots
		other->teamchain = NULL;
	}
}

void bot_seek_cover_sequence_init( gentity_t *ent )
{
	g_serverEntity_t *trav, *lastTrav;

	//
	if( !ent->target || !ent->target[ 0 ] )
	{
		return;
	}

	// find our list of spots
	trav = NULL;

	// the first one is different, since we're getting the sequence (a gentity_t) to point at the first spot (a g_serverEntity_t)
	trav = FindServerEntity( trav, SE_FOFS( name ), ent->target );
	ent->serverEntity = trav;

	// now we just want serverentities pointing at each other
	lastTrav = trav;

	while( ( trav = FindServerEntity( trav, SE_FOFS( name ), ent->target ) ) )
	{
		lastTrav->nextServerEntity = trav;
		lastTrav = trav;
	}

	//
	// we didn't find anything
	if( ent->serverEntity == NULL )
	{
		G_Error( "bot_seek_cover_sequence has no matching spots (\"target\" = \"%s\"", ent->target );
	}

	//
	lastTrav->nextServerEntity = NULL;
}

extern void InitTrigger( gentity_t *self );  // Arnout: this was missing

void SP_bot_seek_cover_sequence( gentity_t *ent )
{
	ent->touch = bot_seek_cover_sequence_touch;

	ent->think = bot_seek_cover_sequence_init;
	ent->nextthink = level.time + 500;

	InitTrigger( ent );
	trap_LinkEntity( ent );
}

/*QUAKED bot_landmine_area (0.5 0.7 0.3) ? AXIS_ONLY ALLIED_ONLY
Bots will attempt to place landmines in this area.

  "count" - number of landmines to place in this area. Defaults to 2.
  "targetname" - used only to set bot goal-state from within scripting
*/
void SP_bot_landmine_area( gentity_t *ent )
{
	char *spawnString;

	G_SpawnString( "count", "2", &spawnString );
	ent->count = atoi( spawnString );

	trap_SetBrushModel( ent, ent->model );

	ent->r.contents = 0; // replaces the -1 from trap_SetBrushModel
	ent->r.svFlags = SVF_NOCLIENT;
	ent->s.eType = ET_LANDMINE_HINT;

	trap_LinkEntity( ent );
}
