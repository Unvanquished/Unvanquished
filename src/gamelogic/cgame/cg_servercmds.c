/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

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

// cg_servercmds.c -- reliably sequenced text commands sent by the server
// these are processed at snapshot transition time, so there will definitely
// be a valid snapshot this frame

#include "cg_local.h"

/*
=================
CG_ParseScores

=================
*/
static void CG_ParseScores( void )
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
		CG_Printf( "cg.numScores: %d\n", cg.numScores );
	}

	for ( i = 0; i < cg.numScores; i++ )
	{
		//
		cg.scores[ i ].client = atoi( CG_Argv( i * 6 + 3 ) );
		cg.scores[ i ].score = atoi( CG_Argv( i * 6 + 4 ) );
		cg.scores[ i ].ping = atoi( CG_Argv( i * 6 + 5 ) );
		cg.scores[ i ].time = atoi( CG_Argv( i * 6 + 6 ) );
		cg.scores[ i ].weapon = atoi( CG_Argv( i * 6 + 7 ) );
		cg.scores[ i ].upgrade = atoi( CG_Argv( i * 6 + 8 ) );

		if ( cg.scores[ i ].client < 0 || cg.scores[ i ].client >= MAX_CLIENTS )
		{
			cg.scores[ i ].client = 0;
		}

		cgs.clientinfo[ cg.scores[ i ].client ].score = cg.scores[ i ].score;

		cg.scores[ i ].team = cgs.clientinfo[ cg.scores[ i ].client ].team;
	}
}

/*
=================
CG_ParseTeamInfo

=================
*/
static void CG_ParseTeamInfo( void )
{
	int i;
	int count;
	int client;

	count = trap_Argc();

	for ( i = 1; i < count; ++i ) // i is also incremented when writing into cgs.clientinfo
	{
		client = atoi( CG_Argv( i ) );

		// wrong team? skip to the next one
		if ( cgs.clientinfo[ client ].team != cg.snap->ps.stats[ STAT_TEAM ] )
		{
			return;
		}

		if ( client < 0 || client >= MAX_CLIENTS )
		{
			CG_Printf( "[skipnotify]CG_ParseTeamInfo: bad client number: %d\n", client );
			return;
		}

		cgs.clientinfo[ client ].location       = atoi( CG_Argv( ++i ) );
		cgs.clientinfo[ client ].health         = atoi( CG_Argv( ++i ) );
		cgs.clientinfo[ client ].curWeaponClass = atoi( CG_Argv( ++i ) );
		cgs.clientinfo[ client ].credit         = atoi( CG_Argv( ++i ) );

		if( cg.snap->ps.stats[ STAT_TEAM ] != TEAM_ALIENS )
		{
			cgs.clientinfo[ client ].upgrade = atoi( CG_Argv( ++i ) );
		}
	}

	cgs.teamInfoReceived = qtrue;
}

/*
================
CG_ParseServerinfo

This is called explicitly when the gamestate is first received,
and whenever the server updates any serverinfo flagged cvars
================
*/
void CG_ParseServerinfo( void )
{
	const char *info;

	info = CG_ConfigString( CS_SERVERINFO );
	cgs.timelimit = atoi( Info_ValueForKey( info, "timelimit" ) );
	cgs.maxclients = atoi( Info_ValueForKey( info, "sv_maxclients" ) );
	cgs.markDeconstruct = atoi( Info_ValueForKey( info, "g_markDeconstruct" ) );
	Q_strncpyz( cgs.mapname, Info_ValueForKey( info, "mapname" ), sizeof(cgs.mapname) );
}

/*
==================
CG_ParseWarmup
==================
*/
static void CG_ParseWarmup( void )
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
void CG_SetConfigValues( void )
{
	const char *alienStage = CG_ConfigString( CS_ALIEN_STAGE );
	const char *humanStage = CG_ConfigString( CS_HUMAN_STAGE );

	if ( alienStage[ 0 ] )
	{
		sscanf( alienStage, "%d", &cgs.alienStage );
	}
	else
	{
		cgs.alienStage = -1;
	}

	if ( humanStage[ 0 ] )
	{
		sscanf( humanStage, "%d", &cgs.humanStage );
	}
	else
	{
		cgs.humanStage = -1;
	}

	cgs.levelStartTime = atoi( CG_ConfigString( CS_LEVEL_START_TIME ) );
	cg.warmupTime = atoi( CG_ConfigString( CS_WARMUP ) );
}

/*
=====================
CG_ShaderStateChanged
=====================
*/
void CG_ShaderStateChanged( void )
{
	char       originalShader[ MAX_QPATH ];
	char       newShader[ MAX_QPATH ];
	char       timeOffset[ 16 ];
	const char *o;
	char       *n, *t;

	o = CG_ConfigString( CS_SHADERSTATE );

	while ( o && *o )
	{
		n = strstr( o, "=" );

		if ( n && *n )
		{
			strncpy( originalShader, o, n - o );
			originalShader[ n - o ] = 0;
			n++;
			t = strstr( n, ":" );

			if ( t && *t )
			{
				strncpy( newShader, n, t - n );
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
				strncpy( timeOffset, t, o - t );
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
CG_AnnounceAlienStageTransition
================
*/
static void CG_AnnounceAlienStageTransition( stage_t from, stage_t to )
{
	if ( cg.predictedPlayerState.stats[ STAT_TEAM ] != TEAM_ALIENS )
	{
		return;
	}

	if ( to > from )
	{
		trap_S_StartLocalSound( cgs.media.alienStageTransition, CHAN_ANNOUNCER );
		CG_CenterPrint( _("We have evolved!"), 200, GIANTCHAR_WIDTH * 4 );
	}
	else if ( to < from )
	{
		trap_S_StartLocalSound( cgs.media.alienStageTransition, CHAN_ANNOUNCER ); // TODO: Add alien stage down sound
		CG_CenterPrint( _("^1We have devolved!"), 200, GIANTCHAR_WIDTH * 4 );
	}
}

/*
================
CG_AnnounceHumanStageTransition
================
*/
static void CG_AnnounceHumanStageTransition( stage_t from, stage_t to )
{
	if ( cg.predictedPlayerState.stats[ STAT_TEAM ] != TEAM_HUMANS )
	{
		return;
	}

	if ( to > from )
	{
		trap_S_StartLocalSound( cgs.media.humanStageTransition, CHAN_ANNOUNCER );
		CG_CenterPrint( _("Reinforcements have arrived!"), 200, GIANTCHAR_WIDTH * 4 );
	}
	else if ( to < from )
	{
		trap_S_StartLocalSound( cgs.media.humanStageTransition, CHAN_ANNOUNCER ); // TODO: Add human stage down sound
		CG_CenterPrint( _("^1Reinforcements are lost!"), 200, GIANTCHAR_WIDTH * 4 );
	}
}

/*
================
CG_ConfigStringModified

================
*/
static void CG_ConfigStringModified( void )
{
	const char *str;
	int        num;

	num = atoi( CG_Argv( 1 ) );

	// get the gamestate from the client system, which will have the
	// new configstring already integrated
	trap_GetGameState( &cgs.gameState );

	// look up the individual string that was modified
	str = CG_ConfigString( num );

	//CG_Printf("configstring modification %i: %s\n", num, str);

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
	else if ( num == CS_ALIEN_STAGE )
	{
		stage_t oldAlienStage = cgs.alienStage;

		if ( str[ 0 ] )
		{
			sscanf( str, "%d", &cgs.alienStage );

			if ( cgs.alienStage != oldAlienStage )
			{
				CG_AnnounceAlienStageTransition( oldAlienStage, cgs.alienStage );
			}
		}
		else
		{
			cgs.alienStage = -1;
		}
	}
	else if ( num == CS_HUMAN_STAGE )
	{
		stage_t oldHumanStage = cgs.humanStage;

		if ( str[ 0 ] )
		{
			sscanf( str, "%d", &cgs.humanStage );

			if ( cgs.humanStage != oldHumanStage )
			{
				CG_AnnounceHumanStageTransition( oldHumanStage, cgs.humanStage );
			}
		}
		else
		{
			cgs.humanStage = 0;
		}
	}
	else if ( num == CS_LEVEL_START_TIME )
	{
		cgs.levelStartTime = atoi( str );
	}
	else if ( num >= CS_VOTE_TIME && num < CS_VOTE_TIME + NUM_TEAMS )
	{
		cgs.voteTime[ num - CS_VOTE_TIME ] = atoi( str );
		cgs.voteModified[ num - CS_VOTE_TIME ] = qtrue;

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
		cgs.voteModified[ num - CS_VOTE_YES ] = qtrue;
	}
	else if ( num >= CS_VOTE_NO && num < CS_VOTE_NO + NUM_TEAMS )
	{
		cgs.voteNo[ num - CS_VOTE_NO ] = atoi( str );
		cgs.voteModified[ num - CS_VOTE_NO ] = qtrue;
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
			cgs.gameSounds[ num - CS_SOUNDS ] = trap_S_RegisterSound( str, qfalse );
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
static void CG_MapRestart( void )
{
	if ( cg_showmiss.integer )
	{
		CG_Printf( "CG_MapRestart\n" );
	}

	CG_InitMarkPolys();

	cg.intermissionStarted = qfalse;

	cgs.voteTime[ TEAM_NONE ] = 0;

	cg.mapRestart = qtrue;

	CG_StartMusic();

	trap_S_ClearLoopingSounds( qtrue );

	// we really should clear more parts of cg here and stop sounds

	trap_Cvar_Set( "cg_thirdPerson", "0" );
}

/*
==============
CG_Menu
==============
*/
void CG_Menu( int menu, int arg )
{
	const char   *cmd; // command to send
	const char   *longMsg = NULL; // command parameter
	const char   *shortMsg = NULL; // non-modal version of message
	const char   *dialog;
	dialogType_t type = 0; // controls which cg_disable var will switch it off

	switch ( cg.snap->ps.stats[ STAT_TEAM ] )
	{
		case TEAM_ALIENS:
			dialog = "menu tremulous_alien_dialog\n";
			break;

		case TEAM_HUMANS:
			dialog = "menu tremulous_human_dialog\n";
			break;

		default:
			dialog = "menu tremulous_default_dialog\n";
	}

	cmd = dialog;

	switch ( menu )
	{
	        case MN_WELCOME:
	                cmd = "menu ingame_help\n";
	                type = DT_INTERACTIVE;
	                break;

		case MN_TEAM:
			cmd = "menu tremulous_teamselect\n";
			type = DT_INTERACTIVE;
			break;

		case MN_A_CLASS:
			cmd = "menu tremulous_alienclass\n";
			type = DT_INTERACTIVE;
			break;

		case MN_H_SPAWN:
			cmd = "menu tremulous_humanitem\n";
			type = DT_INTERACTIVE;
			break;

		case MN_A_BUILD:
			cmd = "menu tremulous_alienbuild\n";
			type = DT_INTERACTIVE;
			break;

		case MN_H_BUILD:
			cmd = "menu tremulous_humanbuild\n";
			type = DT_INTERACTIVE;
			break;

		case MN_H_ARMOURY:
			cmd = "menu tremulous_humanarmoury\n";
			type = DT_INTERACTIVE;
			break;

		case MN_H_UNKNOWNITEM:
			shortMsg = "Unknown item";
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_A_TEAMFULL:
			longMsg = _("The alien team has too many players. Please wait until slots "
			          "become available or join the human team.");
			shortMsg = _("The alien team has too many players");
			type = DT_COMMAND;
			break;

		case MN_H_TEAMFULL:
			longMsg = _("The human team has too many players. Please wait until slots "
			          "become available or join the alien team.");
			shortMsg = _("The human team has too many players");
			type = DT_COMMAND;
			break;

		case MN_A_TEAMLOCKED:
			longMsg = _("The alien team is locked. You cannot join the aliens "
			          "at this time.");
			shortMsg = _("The alien team is locked");
			type = DT_COMMAND;
			break;

		case MN_H_TEAMLOCKED:
			longMsg = _("The human team is locked. You cannot join the humans "
			          "at this time.");
			shortMsg = _("The human team is locked");
			type = DT_COMMAND;
			break;

		case MN_PLAYERLIMIT:
			longMsg = _("The maximum number of playing clients has been reached. "
			          "Please wait until slots become available.");
			shortMsg = _("No free player slots");
			type = DT_COMMAND;
			break;

		case MN_WARMUP:
			longMsg = _("You must wait until the warmup time is finished "
			          "before joining a team. ");
			shortMsg = _("You cannot join a team during warmup.");
			type = DT_COMMAND;
			break;

			//===============================

			// Since cheating commands have no default binds, they will often be done
			// via console. In light of this, perhaps opening a menu is
			// counterintuitive
		case MN_CMD_CHEAT:
			//longMsg   = "This action is considered cheating. It can only be used "
			//            "in cheat mode, which is not enabled on this server.";
			shortMsg = _("Cheats are not enabled on this server");
			type = DT_COMMAND;
			break;

		case MN_CMD_CHEAT_TEAM:
			shortMsg = _("Cheats are not enabled on this server, so "
			           "you may not use this command while on a team");
			type = DT_COMMAND;
			break;

		case MN_CMD_TEAM:
			//longMsg   = "You must be on a team to perform this action. Join the alien"
			//            "or human team and try again.";
			shortMsg = _("Join a team first");
			type = DT_COMMAND;
			break;

		case MN_CMD_SPEC:
			//longMsg   = "You may not perform this action while on a team. Become a "
			//            "spectator before trying again.";
			shortMsg = _("You can only use this command when spectating");
			type = DT_COMMAND;
			break;

		case MN_CMD_ALIEN:
			//longMsg   = "You must be on the alien team to perform this action.";
			shortMsg = _("Must be alien to use this command");
			type = DT_COMMAND;
			break;

		case MN_CMD_HUMAN:
			//longMsg   = "You must be on the human team to perform this action.";
			shortMsg = _("Must be human to use this command");
			type = DT_COMMAND;
			break;

		case MN_CMD_ALIVE:
			//longMsg   = "You must be alive to perform this action.";
			shortMsg = _("Must be alive to use this command");
			type = DT_COMMAND;
			break;

			//===============================

		case MN_B_NOROOM:
			longMsg = _("There is no room to build here. Move until the structure turns "
			          "translucent green, indicating a valid build location.");
			shortMsg = _("There is no room to build here");
			type = DT_BUILD;
			break;

		case MN_B_NORMAL:
			longMsg = _("Cannot build on this surface. The surface is too steep or "
			          "unsuitable for building. Please choose another site for this "
			          "structure.");
			shortMsg = _("Cannot build on this surface");
			type = DT_BUILD;
			break;

		case MN_B_CANNOT:
			longMsg = NULL;
			shortMsg = _("You cannot build that structure");
			type = DT_BUILD;
			break;

			// FIXME: MN_H_ and MN_A_?
		case MN_B_LASTSPAWN:
			longMsg = _("This action would remove your team's last spawn point, "
			          "which often quickly results in a loss. Try building more "
			          "spawns.");
			shortMsg = _("You may not deconstruct the last spawn");
			type = DT_MISC_CP;
			break;

		case MN_B_SUDDENDEATH:
			longMsg = _("Neither team has prevailed after a certain time and the "
			          "game has entered Sudden Death. During Sudden Death "
			          "building is not allowed.");
			shortMsg = _("Cannot build during Sudden Death");
			type = DT_BUILD;
			break;

		case MN_B_REVOKED:
			longMsg = _("Your teammates have lost faith in your ability to build "
			          "for the team. You will not be allowed to build until your "
			          "team votes to reinstate your building rights.");
			shortMsg = _("Your building rights have been revoked");
			type = DT_BUILD;
			break;

		case MN_B_SURRENDER:
			longMsg = _("Your team has decided to admit defeat and concede the game:"
			          "traitors and cowards are not allowed to build.");
			// too harsh?
			shortMsg = _("Building is denied to traitorous cowards");
			type = DT_MISC_CP;
			break;

			//===============================

		case MN_H_NOBP:
			if ( cgs.markDeconstruct )
			{
				longMsg = _("There is no power remaining. Free up power by marking "
				          "existing buildable objects.");
			}
			else
			{
				longMsg = _("There is no power remaining. Free up power by deconstructing "
				          "existing buildable objects.");
			}

			shortMsg = _("There is no power remaining");
			type = DT_BUILD;
			break;

		case MN_H_NOTPOWERED:
			longMsg = _("This buildable is not powered. Build a Reactor and/or Repeater "
			          "in order to power it.");
			shortMsg = _("This buildable is not powered");
			type = DT_BUILD;
			break;

		case MN_H_ONEREACTOR:
			longMsg = _("There can only be one Reactor. Deconstruct the existing one if you "
			          "wish to move it.");
			shortMsg = _("There can only be one Reactor");
			type = DT_BUILD;
			break;

		case MN_H_NOPOWERHERE:
			longMsg = _("There is no power here. If available, a Repeater may be used to "
			          "transmit power to this location.");
			shortMsg = _("There is no power here");
			type = DT_BUILD;
			break;

		case MN_H_NODCC:
			longMsg = _("There is no Defense Computer. A Defense Computer is needed to "
			          "build this.");
			shortMsg = _("There is no Defense Computer");
			type = DT_BUILD;
			break;

		case MN_H_RPTPOWERHERE:
			longMsg = _("This area already has power. A Repeater is not required here.");
			shortMsg = _("This area already has power");
			type = DT_BUILD;
			break;

		case MN_H_NOSLOTS:
			longMsg = _("You have no room to carry this. Please sell any conflicting "
			          "upgrades before purchasing this item.");
			shortMsg = _("You have no room to carry this");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_H_NOFUNDS:
			longMsg = _("Insufficient funds. You do not have enough credits to perform "
			          "this action.");
			shortMsg = _("Insufficient funds");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_H_ITEMHELD:
			longMsg = _("You already hold this item. It is not possible to carry multiple "
			          "items of the same type.");
			shortMsg = _("You already hold this item");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_H_NOARMOURYHERE:
			longMsg = _("You must be near a powered Armoury in order to purchase "
			          "weapons, upgrades or ammunition.");
			shortMsg = _("You must be near a powered Armoury");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_H_NOENERGYAMMOHERE:
			longMsg = _("You must be near a Reactor or a powered Armoury or Repeater "
			          "in order to purchase energy ammunition.");
			shortMsg = _("You must be near a Reactor or a powered Armoury or Repeater");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_H_NOROOMBSUITON:
			longMsg = _("There is not enough room here to put on a Battle Suit. "
			          "Make sure you have enough head room to climb in.");
			shortMsg = _("Not enough room here to put on a Battle Suit");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_H_NOROOMBSUITOFF:
			longMsg = _("There is not enough room here to take off your Battle Suit. "
			          "Make sure you have enough head room to climb out.");
			shortMsg = _("Not enough room here to take off your Battle Suit");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_H_ARMOURYBUILDTIMER:
			longMsg = _("You are not allowed to buy or sell weapons until your "
			          "build timer has expired.");
			shortMsg = _("You can not buy or sell weapons until your build timer "
			           "expires");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_H_DEADTOCLASS:
			shortMsg = _("You must be dead to use the class command");
			type = DT_COMMAND;
			break;

		case MN_H_UNKNOWNSPAWNITEM:
			shortMsg = _("Unknown starting item");
			type = DT_COMMAND;
			break;

			//===============================

		case MN_A_NOCREEP:
			longMsg = _("There is no creep here. You must build near existing Eggs or "
			          "the Overmind. Alien structures will not support themselves.");
			shortMsg = _("There is no creep here");
			type = DT_BUILD;
			break;

		case MN_A_NOOVMND:
			longMsg = _("There is no Overmind. An Overmind must be built to control "
			          "the structure you tried to place.");
			shortMsg = _("There is no Overmind");
			type = DT_BUILD;
			break;

		case MN_A_ONEOVERMIND:
			longMsg = _("There can only be one Overmind. Deconstruct the existing one if you "
			          "wish to move it.");
			shortMsg = _("There can only be one Overmind");
			type = DT_BUILD;
			break;

		case MN_A_NOBP:
			longMsg = _("The Overmind cannot control any more structures. Deconstruct existing "
			          "structures to build more.");
			shortMsg = _("The Overmind cannot control any more structures");
			type = DT_BUILD;
			break;

		case MN_A_NOEROOM:
			longMsg = _("There is no room to evolve here. Move away from walls or other "
			          "nearby objects and try again.");
			shortMsg = _("There is no room to evolve here");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_A_TOOCLOSE:
			longMsg = _("This location is too close to the enemy to evolve. Move away "
			          "from the enemy's presence and try again.");
			shortMsg = _("This location is too close to the enemy to evolve");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_A_NOOVMND_EVOLVE:
			longMsg = _("There is no Overmind. An Overmind must be built to allow "
			          "you to upgrade.");
			shortMsg = _("There is no Overmind");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_A_EVOLVEBUILDTIMER:
			longMsg = _("You cannot evolve until your build timer has expired.");
			shortMsg = _("You cannot evolve until your build timer expires");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_A_INFEST:
			trap_Cvar_Set( "ui_currentClass",
			               va( "%d %d", cg.snap->ps.stats[ STAT_CLASS ],
			                   cg.snap->ps.persistant[ PERS_CREDIT ] ) );

			cmd = "menu tremulous_alienupgrade\n";
			type = DT_INTERACTIVE;
			break;

		case MN_A_CANTEVOLVE:
			shortMsg = va( _("You cannot evolve into a %s"),
			               _( BG_ClassModelConfig( arg )->humanName ) );
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_A_EVOLVEWALLWALK:
			shortMsg = _("You cannot evolve while wallwalking");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_A_UNKNOWNCLASS:
			shortMsg = _("Unknown class");
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_A_CLASSNOTSPAWN:
			shortMsg = va( _("You cannot spawn as a %s"),
			               _( BG_ClassModelConfig( arg )->humanName ) );
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_A_CLASSNOTALLOWED:
			shortMsg = va( _("The %s is not allowed"),
			               _( BG_ClassModelConfig( arg )->humanName ) );
			type = DT_ARMOURYEVOLVE;
			break;

		case MN_A_CLASSNOTATSTAGE:
			shortMsg = va( _("The %s is not allowed at Stage %d"),
			               _( BG_ClassModelConfig( arg )->humanName ),
			               cgs.alienStage + 1 );
			type = DT_ARMOURYEVOLVE;
			break;

		default:
			Com_Printf(_( "cgame: debug: no such menu %d\n"), menu );
	}

	switch ( type )
	{
		case DT_BUILD:
		case DT_ARMOURYEVOLVE:
		case DT_MISC_CP:
			// menu open? we need to use the modal dbox
			// menu closed? we want to centre print
			if ( !trap_Cvar_VariableIntegerValue( "ui_menuIsOpen" ) )
			{
				cmd = NULL;
				// only a short message? make the long message the same
				longMsg = longMsg ? longMsg : shortMsg;
                        }
		default:
			break;
	}

	if ( type == DT_ARMOURYEVOLVE && cg_disableUpgradeDialogs.integer )
	{
		return;
	}

	if ( type == DT_BUILD && cg_disableBuildDialogs.integer )
	{
		return;
	}

	if ( type == DT_COMMAND && cg_disableCommandDialogs.integer )
	{
		return;
	}

	if ( cmd && cmd != dialog )
	{
		trap_SendConsoleCommand( cmd );
	}
	else if ( longMsg && cg_disableWarningDialogs.integer == 0 )
	{
		if ( cmd )
		{
			trap_Cvar_Set( "ui_dialog", longMsg );
			trap_SendConsoleCommand( cmd );
		}
		else
		{
			CG_CenterPrint( longMsg, SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );

			if ( shortMsg && cg_disableWarningDialogs.integer < 2 )
			{
				CG_Printf( "%s\n", shortMsg );
			}
		}
	}
	else if ( shortMsg && cg_disableWarningDialogs.integer < 2 )
	{
		CG_Printf( "%s\n", shortMsg );
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
	char *ignore = "";
	char *location = "";
	char color;
	char *maybeColon;

	if ( clientNum >= 0 && clientNum < MAX_CLIENTS )
	{
		clientInfo_t *ci = &cgs.clientinfo[ clientNum ];
		char         *tcolor = S_COLOR_WHITE;

		name = ci->name;

		if ( ci->team == TEAM_ALIENS )
		{
			tcolor = S_COLOR_RED;
		}
		else if ( ci->team == TEAM_HUMANS )
		{
			tcolor = S_COLOR_CYAN;
		}

		if ( cg_chatTeamPrefix.integer )
		{
			Com_sprintf( prefix, sizeof( prefix ), "[%s%c" S_COLOR_WHITE "] ",
			             tcolor, toupper( * ( BG_TeamName( ci->team ) ) ) );
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
					location = va( " (%s" S_COLOR_WHITE ")", s );
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
		maybeColon = "";
	}
	else
	{
		maybeColon = ":";
	}

	color = '0' + UI_GetChatColour( mode, cgs.clientinfo[ clientNum ].team );

	switch ( mode )
	{
		case SAY_ALL:
			// might already be ignored but in that case no harm is done
			if ( cg_teamChatsOnly.integer )
			{
				ignore = S_SKIPNOTIFY;
			}

		case SAY_ALL_ADMIN:
			CG_Printf(  "%s%s%s^7%s ^%c%s\n",
			           ignore, prefix, name, maybeColon, color, text );
			break;

		case SAY_TEAM:
			CG_Printf( "%s%s(%s^7)%s%s ^%c%s\n",
			           ignore, prefix, name, location, maybeColon, color, text );
			break;

		case SAY_ADMINS:
		case SAY_ADMINS_PUBLIC:
			CG_Printf( "%s%s%s%s^7%s ^%c%s\n",
			           ignore, prefix,
			           ( mode == SAY_ADMINS ) ? "[ADMIN]" : "[PLAYER]",
			           name, maybeColon, color, text );
			break;

		case SAY_AREA:
		case SAY_AREA_TEAM:
			CG_Printf( "%s%s<%s^7>%s%s ^%c%s\n",
			           ignore, prefix, name, location, maybeColon, color, text );
			break;

		case SAY_PRIVMSG:
		case SAY_TPRIVMSG:
			CG_Printf( "%s%s[%s^7 → %s^7]%s ^%c%s\n",
			           ignore, prefix, name, cgs.clientinfo[ cg.clientNum ].name,
			           maybeColon, color, text );

			if ( !ignore[ 0 ] )
			{
				CG_CenterPrint( va( "^%cPrivate message from: " S_COLOR_WHITE "%s",
				                    color, name ), 200, GIANTCHAR_WIDTH * 4 );

				if ( clientNum < 0 || clientNum >= MAX_CLIENTS )
				{
					clientNum = cg.clientNum;
				}

				CG_Printf(_( ">> to reply, say: /m %d [your message] <<\n"), clientNum );
			}

			break;

		case SAY_RAW:
			CG_Printf( "%s\n", text );
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
			if ( cg.snap->ps.stats[ STAT_TEAM ] == TEAM_ALIENS )
			{
				trap_S_StartLocalSound( cgs.media.alienTalkSound, CHAN_LOCAL_SOUND );
				break;
			}
			else if ( cg.snap->ps.stats[ STAT_TEAM ] == TEAM_HUMANS )
			{
				trap_S_StartLocalSound( cgs.media.humanTalkSound, CHAN_LOCAL_SOUND );
				break;
			}

		default:
			trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
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
		CG_Printf( "[skipnotify]WARNING: could not find voice \"%s\"\n", voice );
		return NULL;
	}

	c = BG_VoiceCmdByNum( v->cmds, cmd );

	if ( !c )
	{
		CG_Printf( "[skipnotify]WARNING: could not find command %d "
		           "in voice \"%s\"\n", cmd, voice );
		return NULL;
	}

	t = BG_VoiceTrackByNum( c->tracks, track );

	if ( !t )
	{
		CG_Printf( "[skipnotify]WARNING: could not find track %d for command %d in "
		           "voice \"%s\"\n", track, cmd, voice );
		return NULL;
	}

	return t;
}

/*
=================
CG_ParseVoice

voice clientNum vChan cmdNum trackNum [sayText]
=================
*/
static void CG_ParseVoice( void )
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

	vChan = atoi( CG_Argv( 2 ) );

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
				CG_Say( NULL, clientNum, SAY_ALL, sayText );
				break;

			case VOICE_CHAN_TEAM:
				CG_Say( NULL, clientNum, SAY_TEAM, sayText );
				break;

			case VOICE_CHAN_LOCAL:
				CG_Say( NULL, clientNum, SAY_AREA_TEAM, sayText );
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
			trap_S_StartLocalSound( track->track, CHAN_VOICE );
			break;

		case VOICE_CHAN_TEAM:
			trap_S_StartLocalSound( track->track, CHAN_VOICE );
			break;

		case VOICE_CHAN_LOCAL:
			trap_S_StartSound( NULL, clientNum, CHAN_VOICE, track->track );
			break;

		default:
			break;
	}
}

/*
=================
CG_CenterPrint_f
=================
*/
void CG_CenterPrint_f( void )
{
	CG_CenterPrint( CG_Argv( 1 ), SCREEN_HEIGHT * 0.30, BIGCHAR_WIDTH );
}

/*
=================
CG_CenterPrint_Delay_f
=================
*/
void CG_CenterPrint_Delay_f( void )
{
	char cmd[ MAX_STRING_CHARS ];

	Com_sprintf( cmd, sizeof( cmd ), "delay %s lcp %s", Quote( CG_Argv( 1 ) ), Quote( CG_Argv( 2 ) ) );
	trap_SendConsoleCommand( cmd );
}

/*
=================
CG_Print_f
=================
*/
static void CG_Print_f( void )
{
	CG_Printf( "%s", CG_Argv( 1 ) );
}

/*
=================
CG_PrintTR_f
=================
*/
#define TRANSLATE_FUNC        _
#define PLURAL_TRANSLATE_FUNC P_
#define Cmd_Argv CG_Argv
#define Cmd_Argc trap_Argc
#include "../../engine/qcommon/print_translated.h"

static void CG_PrintTR_f( void )
{
	PrintTranslatedText_Internal( qfalse );
}

static void CG_PrintTR_plural_f( void )
{
	PrintTranslatedText_Internal( qtrue );
}

/*
=================
CG_Chat_f
=================
*/
static void CG_Chat_f( void )
{
	char id[ 3 ];
	char mode[ 3 ];

	trap_Argv( 1, id, sizeof( id ) );
	trap_Argv( 2, mode, sizeof( mode ) );

	CG_Say( NULL, atoi( id ), atoi( mode ), CG_Argv( 3 ) );
}

/*
=================
CG_AdminChat_f
=================
*/
static void CG_AdminChat_f( void )
{
	char name[ MAX_NAME_LENGTH ];
	char mode[ 3 ];

	trap_Argv( 1, name, sizeof( name ) );
	trap_Argv( 2, mode, sizeof( mode ) );

	CG_Say( name, -1, atoi( mode ), CG_Argv( 3 ) );
}

/*
=================
CG_ServerMenu_f
=================
*/
static void CG_ServerMenu_f( void )
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
static void CG_ServerCloseMenus_f( void )
{
	trap_SendConsoleCommand( "closemenus\n" );
}

/*
=================
CG_PoisonCloud_f
=================
*/
static void CG_PoisonCloud_f( void )
{
	cg.poisonedTime = cg.time;

	if ( CG_IsParticleSystemValid( &cg.poisonCloudPS ) )
	{
		cg.poisonCloudPS = CG_SpawnNewParticleSystem( cgs.media.poisonCloudPS );
		CG_SetAttachmentCent( &cg.poisonCloudPS->attachment, &cg.predictedPlayerEntity );
		CG_AttachToCent( &cg.poisonCloudPS->attachment );
	}
}

/*
=================
CG_VCommand

The server has asked us to execute a string from some variable
=================
*/
static void CG_VCommand( void )
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

static void CG_GameCmds_f( void )
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

static const consoleCommand_t svcommands[] =
{
	{ "achat",            CG_AdminChat_f          },
	{ "chat",             CG_Chat_f               },
	{ "cmds",             CG_GameCmds_f           },
	{ "cp",               CG_CenterPrint_f        },
	{ "cpd",              CG_CenterPrint_Delay_f  },
	{ "cs",               CG_ConfigStringModified },
	{ "map_restart",      CG_MapRestart           },
	{ "poisoncloud",      CG_PoisonCloud_f        },
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
static void CG_ServerCommand( void )
{
	const char       *cmd;
	consoleCommand_t *command;

	cmd = CG_Argv( 0 );

	if ( !cmd[ 0 ] )
	{
		return;
	}

	command = bsearch( cmd, svcommands, ARRAY_LEN( svcommands ),
	                   sizeof( svcommands[ 0 ] ), cmdcmp );

	if ( command )
	{
		command->function();
		return;
	}

	CG_Printf(_( "Unknown client game command: %s\n"), cmd );
}

/*
====================
CG_ExecuteNewServerCommands

Execute all of the server commands that were received along
with this this snapshot.
====================
*/
void CG_ExecuteNewServerCommands( int latestSequence )
{
	while ( cgs.serverCommandSequence < latestSequence )
	{
		if ( trap_GetServerCommand( ++cgs.serverCommandSequence ) )
		{
			CG_ServerCommand();
		}
	}
}
