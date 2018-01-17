/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definitely
// be a valid snapshot this frame

#include "cg_local.h"
#include "shared/CommonProxies.h"

/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores()
{
	int i;

	cg.numScores = ( trap_Argc() - 3 ) / 6;

	if ( cg.numScores > MAX_CLIENTS )
	{
		cg.numScores = MAX_CLIENTS;
	}

	cg.teamScores[ 0 ] = atoi( CG_Argv( 1 ) );
	cg.teamScores[ 1 ] = atoi( CG_Argv( 2 ) );

	memset( cg.scores, 0, sizeof( cg.scores ) );

	if ( cg_debugRandom.integer )
	{
		Log::Debug( "cg.numScores: %d", cg.numScores );
	}

	for ( i = 0; i < cg.numScores; i++ )
	{
		//
		cg.scores[ i ].client = atoi( CG_Argv( i * 6 + 3 ) );
		cg.scores[ i ].score = atoi( CG_Argv( i * 6 + 4 ) );
		cg.scores[ i ].ping = atoi( CG_Argv( i * 6 + 5 ) );
		cg.scores[ i ].time = atoi( CG_Argv( i * 6 + 6 ) );
		cg.scores[ i ].weapon = (weapon_t) atoi( CG_Argv( i * 6 + 7 ) );
		cg.scores[ i ].upgrade = (upgrade_t) atoi( CG_Argv( i * 6 + 8 ) );

		if ( cg.scores[ i ].client < 0 || cg.scores[ i ].client >= MAX_CLIENTS )
		{
			cg.scores[ i ].client = 0;
		}

		cgs.clientinfo[ cg.scores[ i ].client ].score = cg.scores[ i ].score;

		cg.scores[ i ].team = cgs.clientinfo[ cg.scores[ i ].client ].team;
	}

	cg.scoreInvalidated = true;
}

/*
=================
CG_ParseTeamInfo

=================
*/
static void CG_ParseTeamInfo()
{
	int i;
	int count;
	int client;

	count = trap_Argc();

	for ( i = 1; i < count; ++i ) // i is also incremented when writing into cgs.clientinfo
	{
		client = atoi( CG_Argv( i ) );

		if ( client < 0 || client >= MAX_CLIENTS )
		{
			Log::Warn( S_SKIPNOTIFY "CG_ParseTeamInfo: bad client number: %d", client );
			return;
		}

		// wrong team? skip to the next one
		if ( cgs.clientinfo[ client ].team != cg.snap->ps.persistant[ PERS_TEAM ] )
		{
			return;
		}

		cgs.clientinfo[ client ].location       = atoi( CG_Argv( ++i ) );
		cgs.clientinfo[ client ].health         = atoi( CG_Argv( ++i ) );
		cgs.clientinfo[ client ].curWeaponClass = atoi( CG_Argv( ++i ) );
		cgs.clientinfo[ client ].credit         = atoi( CG_Argv( ++i ) );

		if( cg.snap->ps.persistant[ PERS_TEAM ] != TEAM_ALIENS )
		{
			cgs.clientinfo[ client ].upgrade = atoi( CG_Argv( ++i ) );
		}
	}

	cgs.teamInfoReceived = true;
}

/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo()
{
	const char *info;

	info = CG_ConfigString( CS_SERVERINFO );

	cgs.timelimit          = atoi( Info_ValueForKey( info, "timelimit" ) );
	cgs.maxclients         = atoi( Info_ValueForKey( info, "sv_maxclients" ) );

	// TODO: Remove those two.
	cgs.powerReactorRange  = atoi( Info_ValueForKey( info, "g_powerReactorRange" ) );

	cgs.momentumHalfLife  = atof( Info_ValueForKey( info, "g_momentumHalfLife" ) );
	cgs.unlockableMinTime = atof( Info_ValueForKey( info, "g_unlockableMinTime" ) );

	cgs.buildPointBudgetPerMiner       = atof( Info_ValueForKey( info, "g_BPBudgetPerMiner" ) );
	cgs.buildPointRecoveryInitialRate  = atof( Info_ValueForKey( info, "g_BPRecoveryInitialRate" ) );
	cgs.buildPointRecoveryRateHalfLife = atof( Info_ValueForKey( info, "g_BPRecoveryRateHalfLife" ) );

	Q_strncpyz( cgs.mapname, Info_ValueForKey( info, "mapname" ), sizeof(cgs.mapname) );

	// pass some of these to UI
	trap_Cvar_Set( "ui_momentumHalfLife", va( "%f", cgs.momentumHalfLife ) );
	trap_Cvar_Set( "ui_unlockableMinTime",  va( "%f", cgs.unlockableMinTime ) );
}

/*
==================
CG_ParseWarmup
==================
*/
static void CG_ParseWarmup()
{
	const char *info;
	int        warmup;

	info = CG_ConfigString( CS_WARMUP );

	warmup = atoi( info );
	cg.warmupTime = warmup;
}

/*
================
CG_SetConfigValues

Called on load to set the initial values from configure strings
================
*/
void CG_SetConfigValues()
{
	cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );
	cg.warmupTime = atoi( CG_ConfigString( CS_WARMUP ) );
}

/*
=====================
CG_ShaderStateChanged
=====================
*/
void CG_ShaderStateChanged()
{
	char       originalShader[ MAX_QPATH ];
	char       newShader[ MAX_QPATH ];
	char       timeOffset[ 16 ];
	const char *o;
	const char *n, *t;

	o = CG_ConfigString( CS_SHADERSTATE );

	while ( o && *o )
	{
		n = strstr( o, "=" );

		if ( n && *n )
		{
			memcpy( originalShader, o, n - o );
			originalShader[ n - o ] = 0;
			n++;
			t = strstr( n, ":" );

			if ( t && *t )
			{
				memcpy( newShader, n, t - n );
				newShader[ t - n ] = 0;
			}
			else
			{
				break;
			}

			t++;
			o = strstr( t, "@" );

			if ( o )
			{
				memcpy( timeOffset, t, o - t );
				timeOffset[ o - t ] = 0;
				o++;
				trap_R_RemapShader( originalShader, newShader, timeOffset );
			}
		}
		else
		{
			break;
		}
	}
}

/*
================
CG_ConfigStringModified

================
*/
static void CG_ConfigStringModified()
{
	const char *str;
	int        num;

	// update the config string state with the new data, keeping in sync
	// with the config strings in the engine.
	// The engine already checked the inputs
	num = atoi( CG_Argv( 1 ) );
	cgs.gameState[num] = CG_Argv(2);

	// look up the individual string that was modified
	str = CG_ConfigString( num );

	//Log::Debug("configstring modification %i: %s", num, str);

	// do something with it if necessary
	if ( num == CS_MUSIC )
	{
		CG_StartMusic();
	}
	else if ( num == CS_SERVERINFO )
	{
		CG_ParseServerinfo();
	}
	else if ( num == CS_WARMUP )
	{
		CG_ParseWarmup();
	}
	else if ( num == CS_LEVEL_START_TIME )
	{
		cgs.levelStartTime = atoi( str );
	}
	else if ( num >= CS_VOTE_TIME && num < CS_VOTE_TIME + NUM_TEAMS )
	{
		cgs.voteTime[ num - CS_VOTE_TIME ] = atoi( str );
		cgs.voteModified[ num - CS_VOTE_TIME ] = true;

		if ( num - CS_VOTE_TIME == TEAM_NONE )
		{
			trap_Cvar_Set( "ui_voteActive", cgs.voteTime[ TEAM_NONE ] ? "1" : "0" );
		}
		else if ( num - CS_VOTE_TIME == TEAM_ALIENS )
		{
			trap_Cvar_Set( "ui_alienTeamVoteActive",
			               cgs.voteTime[ TEAM_ALIENS ] ? "1" : "0" );
		}
		else if ( num - CS_VOTE_TIME == TEAM_HUMANS )
		{
			trap_Cvar_Set( "ui_humanTeamVoteActive",
			               cgs.voteTime[ TEAM_HUMANS ] ? "1" : "0" );
		}
	}
	else if ( num >= CS_VOTE_YES && num < CS_VOTE_YES + NUM_TEAMS )
	{
		cgs.voteYes[ num - CS_VOTE_YES ] = atoi( str );
		cgs.voteModified[ num - CS_VOTE_YES ] = true;
	}
	else if ( num >= CS_VOTE_NO && num < CS_VOTE_NO + NUM_TEAMS )
	{
		cgs.voteNo[ num - CS_VOTE_NO ] = atoi( str );
		cgs.voteModified[ num - CS_VOTE_NO ] = true;
	}
	else if ( num >= CS_VOTE_STRING && num < CS_VOTE_STRING + NUM_TEAMS )
	{
		Q_strncpyz( cgs.voteString[ num - CS_VOTE_STRING ], str,
		            sizeof( cgs.voteString[ num - CS_VOTE_STRING ] ) );
	}
	else if ( num >= CS_VOTE_CALLER && num < CS_VOTE_CALLER + NUM_TEAMS )
	{
		Q_strncpyz( cgs.voteCaller[ num - CS_VOTE_CALLER ], str,
		            sizeof( cgs.voteCaller[ num - CS_VOTE_CALLER ] ) );
	}
	else if ( num == CS_INTERMISSION )
	{
		cg.intermissionStarted = atoi( str );
		if ( cg.intermissionStarted )
		{
			Rocket_ShowScoreboard( "scoreboard", true );
		}
	}
	else if ( num >= CS_MODELS && num < CS_MODELS + MAX_MODELS )
	{
		cgs.gameModels[ num - CS_MODELS ] = trap_R_RegisterModel( str );
	}
	else if ( num >= CS_SHADERS && num < CS_SHADERS + MAX_GAME_SHADERS )
	{
		cgs.gameShaders[ num - CS_SHADERS ] = trap_R_RegisterShader(str,
									    RSF_DEFAULT);
	}
	else if ( num >= CS_GRADING_TEXTURES && num < CS_GRADING_TEXTURES + MAX_GRADING_TEXTURES )
	{
		CG_RegisterGrading( num - CS_GRADING_TEXTURES, str );
	}
	else if ( num >= CS_PARTICLE_SYSTEMS && num < CS_PARTICLE_SYSTEMS + MAX_GAME_PARTICLE_SYSTEMS )
	{
		cgs.gameParticleSystems[ num - CS_PARTICLE_SYSTEMS ] = CG_RegisterParticleSystem( ( char * ) str );
	}
	else if ( num >= CS_SOUNDS && num < CS_SOUNDS + MAX_SOUNDS )
	{
		if ( str[ 0 ] != '*' )
		{
			// player specific sounds don't register here
			cgs.gameSounds[ num - CS_SOUNDS ] = trap_S_RegisterSound( str, false );
		}
	}
	else if ( num >= CS_PLAYERS && num < CS_PLAYERS + MAX_CLIENTS )
	{
		CG_NewClientInfo( num - CS_PLAYERS );
		CG_BuildSpectatorString();
	}
	else if ( num == CS_WINNER )
	{
		trap_Cvar_Set( "ui_winner", str );
	}
	else if ( num == CS_SHADERSTATE )
	{
		CG_ShaderStateChanged();
	}
}

/*
===============
CG_MapRestart

The server has issued a map_restart, so the next snapshot
is completely new and should not be interpolated to.

A tournement restart will clear everything, but doesn't
require a reload of all the media
===============
*/
static void CG_MapRestart()
{
	if ( cg_showmiss.integer )
	{
		Log::Debug( "CG_MapRestart" );
	}

	CG_InitMarkPolys();

	cg.intermissionStarted = false;

	cgs.voteTime[ TEAM_NONE ] = 0;

	cg.mapRestart = true;

	CG_StartMusic();

	trap_S_ClearLoopingSounds( true );

	// we really should clear more parts of cg here and stop sounds

	trap_Cvar_Set( "cg_thirdPerson", "0" );

	CG_OnMapRestart();
}

/*
==============
CG_Menu
==============
*/
void CG_Menu( int menuType, int arg )
{
	int          menu = -1; // Menu to open
	const char   *longMsg = nullptr; // command parameter
	const char   *shortMsg = nullptr; // non-modal version of message

	switch ( menuType )
	{
	        case MN_WELCOME:
	                break;

		case MN_TEAM:
			menu = ROCKETMENU_TEAMSELECT;
			break;

		case MN_A_CLASS:
			menu = ROCKETMENU_ALIENSPAWN;
			break;

		case MN_H_SPAWN:
			menu = ROCKETMENU_HUMANSPAWN;
			break;

		case MN_A_BUILD:
			menu = ROCKETMENU_ALIENBUILD;
			break;

		case MN_H_BUILD:
			menu = ROCKETMENU_HUMANBUILD;
			break;

		case MN_H_ARMOURY:
			menu = ROCKETMENU_ARMOURYBUY;
			break;

		case MN_H_UNKNOWNITEM:
			shortMsg = "Unknown item";
			break;

		case MN_A_TEAMFULL:
			longMsg = _("The alien team has too many players. Please wait until slots "
			          "become available or join the human team.");
			shortMsg = _("The alien team has too many players");
			break;

		case MN_H_TEAMFULL:
			longMsg = _("The human team has too many players. Please wait until slots "
			          "become available or join the alien team.");
			shortMsg = _("The human team has too many players");
			break;

		case MN_A_TEAMLOCKED:
			longMsg = _("The alien team is locked. You cannot join the aliens "
			          "at this time.");
			shortMsg = _("The alien team is locked");
			break;

		case MN_H_TEAMLOCKED:
			longMsg = _("The human team is locked. You cannot join the humans "
			          "at this time.");
			shortMsg = _("The human team is locked");
			break;

		case MN_PLAYERLIMIT:
			longMsg = _("The maximum number of playing clients has been reached. "
			          "Please wait until slots become available.");
			shortMsg = _("No free player slots");
			break;

		case MN_WARMUP:
			longMsg = _("You must wait until the warmup time is finished "
			          "before joining a team. ");
			shortMsg = _("You cannot join a team during warmup.");
			break;

			//===============================

		case MN_CMD_CHEAT:
			//longMsg   = "This action is considered cheating. It can only be used "
			//            "in cheat mode, which is not enabled on this server.";
			shortMsg = _("Cheats are not enabled on this server");
			break;

		case MN_CMD_CHEAT_TEAM:
			shortMsg = _("Cheats are not enabled on this server, so "
			           "you may not use this command while on a team");
			break;

		case MN_CMD_TEAM:
			//longMsg   = "You must be on a team to perform this action. Join the alien"
			//            "or human team and try again.";
			shortMsg = _("Join a team first");
			break;

		case MN_CMD_SPEC:
			//longMsg   = "You may not perform this action while on a team. Become a "
			//            "spectator before trying again.";
			shortMsg = _("You can only use this command when spectating");
			break;

		case MN_CMD_ALIEN:
			//longMsg   = "You must be on the alien team to perform this action.";
			shortMsg = _("Must be alien to use this command");
			break;

		case MN_CMD_HUMAN:
			//longMsg   = "You must be on the human team to perform this action.";
			shortMsg = _("Must be human to use this command");
			break;

		case MN_CMD_ALIVE:
			//longMsg   = "You must be alive to perform this action.";
			shortMsg = _("Must be alive to use this command");
			break;

			//===============================

		case MN_B_NOROOM:
			longMsg = _("There is no room to build here. Move until the structure turns "
			          "translucent green, indicating a valid build location.");
			shortMsg = _("There is no room to build here");
			break;

		case MN_B_NORMAL:
			longMsg = _("Cannot build on this surface. The surface is too steep or "
			          "unsuitable for building. Please choose another site for this "
			          "structure.");
			shortMsg = _("Cannot build on this surface");
			break;

		case MN_B_CANNOT:
			longMsg = nullptr;
			shortMsg = _("You cannot build that structure");
			break;

		case MN_B_LASTSPAWN:
			longMsg = _("This action would remove your team's last spawn point, "
			          "which often quickly results in a loss. Try building more "
			          "spawns.");
			shortMsg = _("You may not deconstruct the last spawn");
			break;

		case MN_B_MAINSTRUCTURE:
			longMsg = _("The main structure is protected against instant removal. "
			            "When it is marked, you can move it to another place by "
			            "building it there.");
			shortMsg = _("You may not deconstruct this structure");
			break;

		case MN_B_DISABLED:
			longMsg = _("Building has been disabled on the server for your team.");
			shortMsg = _("Building has been disabled for your team");
			break;

		case MN_B_REVOKED:
			longMsg = _("Your teammates have lost faith in your ability to build "
			          "for the team. You will not be allowed to build until your "
			          "team votes to reinstate your building rights.");
			shortMsg = _("Your building rights have been revoked");
			break;

		case MN_B_SURRENDER:
			longMsg = _("Your team has decided to admit defeat and concede the game: "
			            "There's no point in building anything anymore.");
			shortMsg = _("Cannot build after admitting defeat");
			break;

		case MN_H_NOBP:
			longMsg = _("There are no resources remaining. Free up resources by "
			            "marking existing buildables for deconstruction.");
			shortMsg = _("There are no resources remaining");
			break;

		case MN_H_NOTPOWERED:
			longMsg = _("This buildable is not powered.");
			shortMsg = _("This buildable is not powered");
			break;

		case MN_H_NOREACTOR:
			longMsg = _("There is no reactor. A reactor must be built before any other "
			            "structure can be placed.");
			shortMsg = _("There is no reactor");
			break;

		case MN_H_ONEREACTOR:
			longMsg = _("There can only be one Reactor. Mark the existing one if you "
			            "wish to move it.");
			shortMsg = _("There can only be one Reactor");
			break;

		case MN_H_NOSLOTS:
			longMsg = _("You have no room to carry this. Please sell any conflicting "
			          "upgrades before purchasing this item.");
			shortMsg = _("You have no room to carry this");
			break;

		case MN_H_NOFUNDS:
			longMsg = _("Insufficient funds. You do not have enough credits to perform "
			          "this action.");
			shortMsg = _("Insufficient funds");
			break;

		case MN_H_ITEMHELD:
			longMsg = _("You already hold this item. It is not possible to carry multiple "
			          "items of the same type.");
			shortMsg = _("You already hold this item");
			break;

		case MN_H_NOARMOURYHERE:
			longMsg = _("You must be near a powered Armoury in order to purchase "
			          "weapons, upgrades or ammunition.");
			shortMsg = _("You must be near a powered Armoury");
			break;

		case MN_H_NOENERGYAMMOHERE:
			longMsg = _("You must be near a Reactor or a powered Armoury "
			          "in order to purchase energy ammunition.");
			shortMsg = _("You must be near a Reactor or a powered Armoury");
			break;

		case MN_H_NOROOMARMOURCHANGE:
			longMsg = _("There is not enough room here to change armour.");
			shortMsg = _("Not enough room here to change armour.");
			break;

		case MN_H_ARMOURYBUILDTIMER:
			longMsg = _("You are not allowed to buy or sell weapons until your "
			          "build timer has expired.");
			shortMsg = _("You can not buy or sell weapons until your build timer "
			           "expires");
			break;

		case MN_H_DEADTOCLASS:
			shortMsg = _("You must be dead to use the class command");
			break;

		case MN_H_UNKNOWNSPAWNITEM:
			shortMsg = _("Unknown starting item");
			break;

			//===============================

		case MN_A_NOOVMND:
			longMsg = _("There is no Overmind. An Overmind must be built before any other "
			            "structure can be placed.");
			shortMsg = _("There is no Overmind");
			break;

		case MN_A_ONEOVERMIND:
			longMsg = _("There can only be one Overmind. Deconstruct the existing one if you "
			          "wish to move it.");
			shortMsg = _("There can only be one Overmind");
			break;

		case MN_A_NOBP:
			longMsg = _("The Overmind cannot control any more structures. Deconstruct existing "
			          "structures to build more.");
			shortMsg = _("The Overmind cannot control any more structures");
			break;

		case MN_A_NOEROOM:
			longMsg = _("There is no room to evolve here. Move away from walls or other "
			          "nearby objects and try again.");
			shortMsg = _("There is no room to evolve here");
			break;

		case MN_A_TOOCLOSE:
			longMsg = _("This location is too close to the enemy to evolve. Move away "
			          "from the enemy's presence and try again.");
			shortMsg = _("This location is too close to the enemy to evolve");
			break;

		case MN_A_NOOVMND_EVOLVE:
			longMsg = _("There is no Overmind. An Overmind must be built to allow "
			          "you to upgrade.");
			shortMsg = _("There is no Overmind");
			break;

		case MN_A_EVOLVEBUILDTIMER:
			longMsg = _("You cannot evolve until your build timer has expired.");
			shortMsg = _("You cannot evolve until your build timer expires");
			break;

		case MN_A_INFEST:
			trap_Cvar_Set( "ui_currentClass",
			               va( "%d %d", cg.snap->ps.stats[ STAT_CLASS ],
			                   cg.snap->ps.persistant[ PERS_CREDIT ] ) );

			menu = ROCKETMENU_ALIENEVOLVE;
			break;

		case MN_A_CANTEVOLVE:
			shortMsg = va( _("You cannot evolve into a %s"),
			               _( BG_ClassModelConfig( arg )->humanName ) );
			break;

		case MN_A_EVOLVEWALLWALK:
			shortMsg = _("You cannot evolve while wallwalking");
			break;

		case MN_A_UNKNOWNCLASS:
			shortMsg = _("Unknown class");
			break;

		case MN_A_CLASSNOTSPAWN:
			shortMsg = va( _("You cannot spawn as a %s"),
			               _( BG_ClassModelConfig( arg )->humanName ) );
			break;

		case MN_A_CLASSNOTALLOWED:
			shortMsg = va( _("The %s is not allowed"),
			               _( BG_ClassModelConfig( arg )->humanName ) );
			break;

		case MN_A_CLASSLOCKED:
			shortMsg = va( _("The %s has not been unlocked yet"),
			               _( BG_ClassModelConfig( arg )->humanName ) );
			break;

		default:
			Log::Notice(_( "cgame: debug: no such menu %d"), menu );
	}

	if ( menu > 0 )
	{
		Rocket_DocumentAction( rocketInfo.menu[ menu ].id, "show" );
	}
	else if ( longMsg && cg_disableWarningDialogs.integer == 0 )
	{
		CG_CenterPrint( longMsg, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );

		if ( shortMsg && cg_disableWarningDialogs.integer < 2 )
		{
			Log::Notice( "%s", shortMsg );
		}
	}
	else if ( shortMsg && cg_disableWarningDialogs.integer < 2 )
	{
		Log::Notice( "%s", shortMsg );
	}
}

/*
=================
CG_Say
=================
*/
static void CG_Say( const char *name, int clientNum, saymode_t mode, const char *text )
{
	char prefix[ 21 ] = "";
	const char *ignore = "";
	const char *location = "";
	team_t team = TEAM_NONE;

	if ( clientNum >= 0 && clientNum < MAX_CLIENTS )
	{
		clientInfo_t *ci = &cgs.clientinfo[ clientNum ];
		Color::Color tcolor = Color::White;

		name = ci->name;
		team = ci->team;
		if ( ci->team == TEAM_ALIENS )
		{
			tcolor = Color::Red;
		}
		else if ( ci->team == TEAM_HUMANS )
		{
			tcolor = Color::Cyan;
		}

		if ( cg_chatTeamPrefix.integer )
		{
			Com_sprintf( prefix, sizeof( prefix ), "[%s%c^*] ",
			             Color::ToString( tcolor ).c_str(),
			             Str::ctoupper( * ( BG_TeamName( ci->team ) ) ) );
		}

		if ( Com_ClientListContains( &cgs.ignoreList, clientNum ) )
		{
			ignore = S_SKIPNOTIFY;
		}

		if ( ( mode == SAY_TEAM || mode == SAY_AREA ) &&
		     cg.snap->ps.pm_type != PM_INTERMISSION )
		{
			int locationNum;

			if ( clientNum == cg.snap->ps.clientNum )
			{
				centity_t *locent;

				locent = CG_GetPlayerLocation();

				if ( locent )
				{
					locationNum = locent->currentState.generic1;
				}
				else
				{
					locationNum = 0;
				}
			}
			else
			{
				locationNum = ci->location;
			}

			if ( locationNum > 0 && locationNum < MAX_LOCATIONS )
			{
				const char *s = CG_ConfigString( CS_LOCATIONS + locationNum );

				if ( *s )
				{
					location = va( " (%s^*)", s );
				}
			}
		}
	}
	else if ( name )
	{
		Q_strcat( prefix, sizeof( prefix ), "[ADMIN]" );
	}
	else
	{
		name = "console";
	}

	// IRC-like /me parsing
	if ( mode != SAY_RAW && Q_strnicmp( text, "/me ", 4 ) == 0 )
	{
		text += 4;
		Q_strcat( prefix, sizeof( prefix ), "* " );
	}

	auto color = Color::ToString( UI_GetChatColour( mode, team ) );

	switch ( mode )
	{
		case SAY_ALL:
			// might already be ignored but in that case no harm is done
			if ( cg_teamChatsOnly.integer )
			{
				ignore = S_SKIPNOTIFY;
			}
			DAEMON_FALLTHROUGH;

		case SAY_ALL_ADMIN:
			Log::Notice(  "%s%s%s^7: %s%s",
			           ignore, prefix, name, color, text );
			break;

		case SAY_TEAM:
			Log::Notice( "%s%s(%s^7)%s: %s%s",
			           ignore, prefix, name, location, color, text );
			break;

		case SAY_ADMINS:
		case SAY_ADMINS_PUBLIC:
			Log::Notice( "%s%s%s%s^7: %s%s",
			           ignore, prefix,
			           ( mode == SAY_ADMINS ) ? "[ADMIN]" : "[PLAYER]",
			           name, color, text );
			break;

		case SAY_AREA:
		case SAY_AREA_TEAM:
			Log::Notice( "%s%s<%s^7>%s: %s%s",
			           ignore, prefix, name, location, color, text );
			break;

		case SAY_PRIVMSG:
		case SAY_TPRIVMSG:
			Log::Notice( "%s%s[%s^7 → %s^7]: %s%s",
			           ignore, prefix, name, cgs.clientinfo[ cg.clientNum ].name,
			           color, text );

			if ( !ignore[ 0 ] )
			{
				CG_CenterPrint( va( _("%sPrivate message from: ^7%s"),
				                    color.c_str(), name ), 200, GIANTCHAR_WIDTH * 4 );

				if ( clientNum < 0 || clientNum >= MAX_CLIENTS )
				{
					clientNum = cg.clientNum;
				}

				Log::Notice(_( ">> to reply, say: /m %d [your message] <<"), clientNum );
			}

			break;

		case SAY_ALL_ME:
			Log::Notice(  "%s* %s%s^7 %s%s",
			           ignore, prefix, name, color, text );
			break;

		case SAY_TEAM_ME:
			Log::Notice( "%s* %s(%s^7)%s %s%s",
			           ignore, prefix, name, location, color, text );
			break;

		case SAY_RAW:
			Log::Notice( "%s", text );
			break;

		case SAY_DEFAULT:
		default:
			break;
	}

	switch ( mode )
	{
		case SAY_TEAM:
		case SAY_AREA:
		case SAY_TPRIVMSG:
			if ( cgs.clientinfo[ clientNum ].team == TEAM_ALIENS )
			{
				trap_S_StartLocalSound( cgs.media.alienTalkSound, soundChannel_t::CHAN_LOCAL_SOUND );
				break;
			}
			else if ( cgs.clientinfo[ clientNum ].team == TEAM_HUMANS )
			{
				trap_S_StartLocalSound( cgs.media.humanTalkSound, soundChannel_t::CHAN_LOCAL_SOUND );
				break;
			}
			DAEMON_FALLTHROUGH;

		default:
			trap_S_StartLocalSound( cgs.media.talkSound, soundChannel_t::CHAN_LOCAL_SOUND );
	}
}

/*
=================
CG_VoiceTrack

return the voice indexed voice track or print errors quietly to console
in case someone is on an unpure server and wants to know which voice pak
is missing or incomplete
=================
*/
static voiceTrack_t *CG_VoiceTrack( char *voice, int cmd, int track )
{
	voice_t      *v;
	voiceCmd_t   *c;
	voiceTrack_t *t;

	v = BG_VoiceByName( cgs.voices, voice );

	if ( !v )
	{
		Log::Warn( S_SKIPNOTIFY "could not find voice \"%s\"", voice );
		return nullptr;
	}

	c = BG_VoiceCmdByNum( v->cmds, cmd );

	if ( !c )
	{
		Log::Warn( S_SKIPNOTIFY "could not find command %d "
		           "in voice \"%s\"", cmd, voice );
		return nullptr;
	}

	t = BG_VoiceTrackByNum( c->tracks, track );

	if ( !t )
	{
		Log::Warn( S_SKIPNOTIFY "could not find track %d for command %d in "
		           "voice \"%s\"", track, cmd, voice );
		return nullptr;
	}

	return t;
}

/*
=================
CG_ParseVoice

voice clientNum vChan cmdNum trackNum [sayText]
=================
*/
static void CG_ParseVoice()
{
	int            clientNum;
	voiceChannel_t vChan;
	char           sayText[ MAX_SAY_TEXT ] = { "" };
	voiceTrack_t   *track;
	clientInfo_t   *ci;

	if ( trap_Argc() < 5 || trap_Argc() > 6 )
	{
		return;
	}

	if ( trap_Argc() == 6 )
	{
		Q_strncpyz( sayText, CG_Argv( 5 ), sizeof( sayText ) );
	}

	clientNum = atoi( CG_Argv( 1 ) );

	if ( clientNum < 0 || clientNum >= MAX_CLIENTS )
	{
		return;
	}

	vChan = (voiceChannel_t) atoi( CG_Argv( 2 ) );

	if ( ( unsigned ) vChan >= VOICE_CHAN_NUM_CHANS )
	{
		return;
	}

	if ( cg_teamChatsOnly.integer && vChan != VOICE_CHAN_TEAM )
	{
		return;
	}

	ci = &cgs.clientinfo[ clientNum ];

	// this joker is still talking
	if ( ci->voiceTime > cg.time )
	{
		return;
	}

	track = CG_VoiceTrack( ci->voice, atoi( CG_Argv( 3 ) ), atoi( CG_Argv( 4 ) ) );

	// keep track of how long the player will be speaking
	// assume it takes 3s to say "*unintelligible gibberish*"
	if ( track )
	{
		ci->voiceTime = cg.time + track->duration;
	}
	else
	{
		ci->voiceTime = cg.time + 3000;
	}

	if ( !sayText[ 0 ] )
	{
		if ( track )
		{
			Q_strncpyz( sayText, track->text, sizeof( sayText ) );
		}
		else
		{
			Q_strncpyz( sayText, "*unintelligible gibberish*", sizeof( sayText ) );
		}
	}

	if ( !cg_noVoiceText.integer )
	{
		switch ( vChan )
		{
			case VOICE_CHAN_ALL:
				CG_Say( nullptr, clientNum, SAY_ALL, sayText );
				break;

			case VOICE_CHAN_TEAM:
				CG_Say( nullptr, clientNum, SAY_TEAM, sayText );
				break;

			case VOICE_CHAN_LOCAL:
				CG_Say( nullptr, clientNum, SAY_AREA_TEAM, sayText );
				break;

			default:
				break;
		}
	}

	// playing voice audio tracks disabled
	if ( cg_noVoiceChats.integer )
	{
		return;
	}

	// no audio track to play
	if ( !track )
	{
		return;
	}

	// don't play audio track for lamers
	if ( Com_ClientListContains( &cgs.ignoreList, clientNum ) )
	{
		return;
	}

	switch ( vChan )
	{
		case VOICE_CHAN_ALL:
			trap_S_StartLocalSound( track->track, soundChannel_t::CHAN_VOICE );
			break;

		case VOICE_CHAN_TEAM:
			trap_S_StartLocalSound( track->track, soundChannel_t::CHAN_VOICE );
			break;

		case VOICE_CHAN_LOCAL:
			trap_S_StartSound( nullptr, clientNum, soundChannel_t::CHAN_VOICE, track->track );
			break;

		default:
			break;
	}
}

#define TRANSLATE_FUNC        _
#define PLURAL_TRANSLATE_FUNC P_
#define Cmd_Argv CG_Argv
#define Cmd_Argc trap_Argc
#include "engine/qcommon/print_translated.h"

/*
=================
CG_CenterPrint_f
=================
*/
void CG_CenterPrint_f()
{
	CG_CenterPrint( CG_Argv( 1 ), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
}

/*
=================
CG_CenterPrint_Delay_f
=================
*/
void CG_CenterPrint_Delay_f()
{
	char cmd[ MAX_STRING_CHARS ];

	Com_sprintf( cmd, sizeof( cmd ), "delay %s lcp %s", Quote( CG_Argv( 1 ) ), Quote( CG_Argv( 2 ) ) );
	trap_SendConsoleCommand( cmd );
}

/*
=================
CG_CenterPrintTR_f
=================
*/
void CG_CenterPrintTR_f()
{
	CG_CenterPrint( TranslateText_Internal( false, 1 ), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
}

/*
=================
CG_CenterPrintTR_Delay_f
=================
*/
void CG_CenterPrintTR_Delay_f()
{
	char cmd[ MAX_STRING_CHARS ];

	Com_sprintf( cmd, sizeof( cmd ), "delay %s lcp %s", Quote( CG_Argv( 1 ) ), Quote( TranslateText_Internal( false, 2 ) ) );
	trap_SendConsoleCommand( cmd );
}

/*
=================
CG_Print_f
=================
*/
static void CG_Print_f()
{
	Log::Notice( "%s", CG_Argv( 1 ) );
}

/*
=================
CG_PrintTR_f
=================
*/
static void CG_PrintTR_f()
{
	Log::Notice( "%s", TranslateText_Internal( false, 1 ) );
}

static void CG_PrintTR_plural_f()
{
	Log::Notice("%s", TranslateText_Internal( true, 1 ) );
}

/*
=================
CG_Chat_f
=================
*/
static void CG_Chat_f()
{
	char id[ 3 ];
	char mode[ 3 ];

	trap_Argv( 1, id, sizeof( id ) );
	trap_Argv( 2, mode, sizeof( mode ) );

	CG_Say( nullptr, atoi( id ), (saymode_t) atoi( mode ), CG_Argv( 3 ) );
}

/*
=================
CG_AdminChat_f
=================
*/
static void CG_AdminChat_f()
{
	char name[ MAX_NAME_LENGTH ];
	char mode[ 3 ];

	trap_Argv( 1, name, sizeof( name ) );
	trap_Argv( 2, mode, sizeof( mode ) );

	CG_Say( name, -1, (saymode_t) atoi( mode ), CG_Argv( 3 ) );
}

/*
=================
CG_ServerMenu_f
=================
*/
static void CG_ServerMenu_f()
{
	if ( !cg.demoPlayback )
	{
		if ( trap_Argc() == 2 )
		{
			CG_Menu( atoi( CG_Argv( 1 ) ), 0 );
		}
		else if ( trap_Argc() == 3 )
		{
			CG_Menu( atoi( CG_Argv( 1 ) ), atoi( CG_Argv( 2 ) ) );
		}
	}
}

/*
=================
CG_ServerCloseMenus_f
=================
*/
static void CG_ServerCloseMenus_f()
{
	trap_SendConsoleCommand( "closemenus\n" );
}

/*
=================
CG_VCommand

The server has asked us to execute a string from some variable
=================
*/
static void CG_VCommand()
{
	static int recurse = 0;
	char       cmd[ 32 ];

	if ( recurse || trap_Argc() != 2 )
	{
		return;
	}

	recurse = 1;

	trap_Argv( 1, cmd, sizeof( cmd ) );

	if ( !Q_stricmp( cmd, "grenade" ) )
	{
		trap_SendClientCommand( cg_cmdGrenadeThrown.string );
	}
	else if ( !Q_stricmp( cmd, "needhealth" ) )
	{
		trap_SendClientCommand( cg_cmdNeedHealth.string );
	}

	recurse = 0;
}

static void CG_GameCmds_f()
{
	int i;
	int c = trap_Argc();

	/*
	There is no corresponding trap_RemoveCommand because a server could send
	something like
	  cmds quit
	which would result in trap_RemoveCommand( "quit" ), which would be really bad
	*/
	for ( i = 1; i < c; i++ )
	{
		trap_AddCommand( CG_Argv( i ) );
	}
}

static void CG_PmoveParams_f() {
	char arg1[64], arg2[64], arg3[64], arg4[64];

	if (trap_Argc() != 5) {
		return;
	}

	trap_Argv(1, arg1, sizeof(arg1));
	trap_Argv(2, arg2, sizeof(arg2));
	trap_Argv(3, arg3, sizeof(arg3));
	trap_Argv(4, arg4, sizeof(arg4));

	cg.pmoveParams.synchronous = atoi(arg1);
	cg.pmoveParams.fixed = atoi(arg2);
	cg.pmoveParams.msec = atoi(arg3);
	cg.pmoveParams.accurate = atoi(arg4);

	if (cg.pmoveParams.msec < 8) {
		cg.pmoveParams.msec = 8;
	} else if (cg.pmoveParams.msec > 33) {
		cg.pmoveParams.msec = 33;
	}
}

static const consoleCommand_t svcommands[] =
{	// sorting: use 'sort -f'
	{ "achat",            CG_AdminChat_f          },
	{ "chat",             CG_Chat_f               },
	{ "cmds",             CG_GameCmds_f           },
	{ "cp",               CG_CenterPrint_f        },
	{ "cpd",              CG_CenterPrint_Delay_f  },
	{ "cpd_tr",           CG_CenterPrintTR_Delay_f},
	{ "cp_tr",            CG_CenterPrintTR_f      },
	{ "cs",               CG_ConfigStringModified },
	{ "map_restart",      CG_MapRestart           },
	{ "pmove_params",     CG_PmoveParams_f        },
	{ "print",            CG_Print_f              },
	{ "print_tr",         CG_PrintTR_f            },
	{ "print_tr_p",       CG_PrintTR_plural_f     },
	{ "scores",           CG_ParseScores          },
	{ "serverclosemenus", CG_ServerCloseMenus_f   },
	{ "servermenu",       CG_ServerMenu_f         },
	{ "tinfo",            CG_ParseTeamInfo        },
	{ "vcommand",         CG_VCommand             },
	{ "voice",            CG_ParseVoice           }
};

/*
=================
CG_ServerCommand

The string has been tokenized and can be retrieved with
CG_Argc() / CG_Argv()
=================
*/
static void CG_ServerCommand()
{
	const char       *cmd;
	consoleCommand_t *command;

	cmd = CG_Argv( 0 );

	if ( !cmd[ 0 ] )
	{
		return;
	}

	command = (consoleCommand_t*) bsearch( cmd, svcommands, ARRAY_LEN( svcommands ),
	                   sizeof( svcommands[ 0 ] ), cmdcmp );

	if ( command )
	{
		command->function();
		return;
	}

	Log::Warn(_( "Unknown client game command: %s"), cmd );
}

/*
====================
CG_ExecuteServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteServerCommands(snapshot_t* snap) {
	for (const auto& command: snap->serverCommands) {
		Cmd::PushArgs(command);
		CG_ServerCommand();
		Cmd::PopArgs();
	}
}
