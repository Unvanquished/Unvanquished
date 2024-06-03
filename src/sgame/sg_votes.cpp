/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2022 Unvanquished Developers

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

#include "common/Common.h"
#include "sg_votes.h"

#include "sg_local.h"

#include "shared/parse.h"

/*
Return true if arg is valid, and store the number in argnum.
Otherwise, return false and do not modify argnum.
*/
static bool botFillVoteParseArg( int& argnum, Str::StringRef arg )
{
	int num = -1;
	if ( Str::ParseInt( num, arg ) && num >= 0 && num <= g_maxVoteFillBots.Get() )
	{
		argnum = num;
		return true;
	}
	else
	{
		return false;
	}
}

static bool HandleKickVote( gentity_t* ent, team_t team, const std::string&, const std::string&,
                            std::string& reason, const std::string& name, int clientNum, int )
{
	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
	             "ban %s 1s%s %s^* called vote kick (%s^*)", level.clients[ clientNum ].pers.ip.str,
	             Cmd::Escape( g_adminTempBan.Get() ).c_str(),
	             Cmd::Escape( ent->client->pers.netname ).c_str(), Cmd::Escape( reason ).c_str() );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ), N_( "Kick player '%s'" ),
	             name.c_str() );
	return true;
}

static bool HandleSpectateVote( gentity_t*, team_t team, const std::string&, const std::string&,
                                std::string&, const std::string& name, int clientNum, int )
{
	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
	             "speclock %d 1s%s", clientNum, Cmd::Escape( g_adminTempBan.Get() ).c_str() );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ),
	             N_( "Move player '%s' to spectators" ), name.c_str() );
	return true;
}

static bool HandleMuteVote( gentity_t* ent, team_t team, const std::string& cmd, const std::string&,
                            std::string&, const std::string& name, int clientNum, int id )
{
	if ( level.clients[ clientNum ].pers.namelog->muted )
	{
		trap_SendServerCommand(
			ent->num(),
			va( "print_tr %s %s", QQ( N_( "$1$: player is already muted" ) ), cmd.c_str() ) );
		return false;
	}

	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ), "mute %d",
	             id );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ), N_( "Mute player '%s'" ),
	             name.c_str() );
	return true;
}

static bool HandleUnmuteVote( gentity_t* ent, team_t team, const std::string& cmd, const std::string&,
                              std::string&, const std::string& name, int clientNum, int id )
{
	if ( !level.clients[ clientNum ].pers.namelog->muted )
	{
		trap_SendServerCommand(
			ent->num(),
			va( "print_tr %s %s", QQ( N_( "$1$: player is not currently muted" ) ), cmd.c_str() ) );
		return false;
	}

	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
	             "unmute %d", id );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ), N_( "Unmute player '%s'" ),
	             name.c_str() );
	return true;
}

static bool HandleDenybuildVote( gentity_t* ent, team_t team, const std::string& cmd, const std::string&,
                                 std::string&, const std::string& name, int clientNum, int id )
{
	if ( level.clients[ clientNum ].pers.namelog->denyBuild )
	{
		trap_SendServerCommand(
			ent->num(), va( "print_tr %s %s",
		                    QQ( N_( "$1$: player already lost building rights" ) ), cmd.c_str() ) );
		return false;
	}

	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
	             "denybuild %d", id );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ),
	             "Take away building rights from '%s'", name.c_str() );
	return true;
}

static bool HandleAllowbuildVote( gentity_t* ent, team_t team, const std::string& cmd, const std::string&,
                                  std::string&, const std::string& name, int clientNum, int id )
{
	if ( !level.clients[ clientNum ].pers.namelog->denyBuild )
	{
		trap_SendServerCommand(
			ent->num(), va( "print_tr %s %s", QQ( N_( "$1$: player already has building rights" ) ),
		                    cmd.c_str() ) );
		return false;
	}

	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
	             "allowbuild %d", id );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ), "Allow '%s' to build",
	             name.c_str() );
	return true;
}

static bool HandleExtendVote( gentity_t*, team_t team, const std::string&, const std::string&,
                              std::string&, const std::string&, int, int )
{
	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ), "time %i",
	             level.timelimit + g_extendVotesTime.Get() );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ),
	             "Extend the timelimit by %d minutes", g_extendVotesTime.Get() );
	return true;
}

static bool HandleAdmitDefeatVote( gentity_t*, team_t team, const std::string&, const std::string&,
                                   std::string&, const std::string&, int, int )
{
	level.team[ team ].voteDelay = 3000;
	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
	             "admitdefeat %d", team );
	strcpy( level.team[ team ].voteDisplayString, "Admit Defeat" );
	return true;
}

static bool HandleDrawVote( gentity_t*, team_t team, const std::string&, const std::string&,
                            std::string&, const std::string&, int, int )
{
	level.team[ team ].voteDelay = 3000;
	strcpy( level.team[ team ].voteString, "evacuation" );
	strcpy( level.team[ team ].voteDisplayString, "End match in a draw" );
	return true;
}

static bool HandleMapRestart( gentity_t*, team_t team, const std::string&, const std::string&,
                              std::string&, const std::string&, int, int )
{
	strcpy( level.team[ team ].voteString, "map_restart" );
	strcpy( level.team[ team ].voteDisplayString, "Restart current map" );
	// map_restart comes with a default delay
	return true;
}

static bool HandleMapVote( gentity_t* ent, team_t team, const std::string& cmd, const std::string& arg,
                           std::string& reason, const std::string&, int, int )
{
	if ( arg.empty() )
	{
		trap_SendServerCommand(
			ent->num(), va( "print_tr %s %s",
		                    QQ( N_( "$1$: map name not specified" ) ),
		                    cmd.c_str() ) );
		return false;
	}
	if ( !G_MapExists( arg.c_str() ) )
	{
		trap_SendServerCommand(
			ent->num(), va( "print_tr %s %s %s",
		                    QQ( N_( "$1$: 'maps/$2$.bsp' could not be found on the server" ) ),
		                    cmd.c_str(), Quote( arg ) ) );
		return false;
	}

	level.team[ team ].voteDelay = 3000;

	if ( !reason.empty() )  // layout?
	{
		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "%s %s %s", G_NextMapCommand().c_str(), Cmd::Escape( arg ).c_str(),
		             Cmd::Escape( reason ).c_str() );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             "Change to map '%s' layout '%s'", arg.c_str(), reason.c_str() );
	}
	else
	{
		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "%s %s", G_NextMapCommand().c_str(), Cmd::Escape( arg ).c_str() );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ), "Change to map '%s'",
		             arg.c_str() );
	}

	reason.clear();  // nullify reason since we've used it here
	return true;
}

static bool HandleLayoutVote( gentity_t* ent, team_t team, const std::string&, const std::string& arg,
                              std::string&, const std::string&, int, int )
{
	if ( Q_stricmp( arg.c_str(), S_BUILTIN_LAYOUT ) &&
	     !G_LayoutExists( Cvar::GetValue( "mapname" ), arg ) )
	{
		trap_SendServerCommand( ent->num(),
		                        va( "print_tr %s %s",
		                            QQ( N_( "callvote: "
		                                    "layout '$1$' could not be found on the server" ) ),
		                            Quote( arg ) ) );
		return false;
	}

	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
	             "restart %s", Cmd::Escape( arg ).c_str() );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ), "Change to map layout '%s'",
	             arg.c_str() );
	return true;
}

static bool HandleNextMapVote( gentity_t* ent, team_t team, const std::string& cmd, const std::string& arg,
                               std::string& reason, const std::string&, int, int )
{
	if ( arg.empty() )
	{
		trap_SendServerCommand(
			ent->num(), va( "print_tr %s %s",
		                    QQ( N_( "$1$: map name not specified" ) ),
		                    cmd.c_str() ) );
		return false;
	}
	if ( G_MapExists( g_nextMap.Get().c_str() ) )
	{
		trap_SendServerCommand(
			ent->num(),
			va( "print_tr %s %s %s", QQ( N_( "$1$: the next map is already set to '$2$'" ) ),
		        cmd.c_str(), Quote( g_nextMap.Get().c_str() ) ) );
		return false;
	}

	if ( !G_MapExists( arg.c_str() ) )
	{
		trap_SendServerCommand(
			ent->num(), va( "print_tr %s %s %s",
		                    QQ( N_( "$1$: 'maps/$2$.bsp' could not be found on the server" ) ),
		                    cmd.c_str(), Quote( arg ) ) );
		return false;
	}

	if ( !reason.empty() )  // layout?
	{
		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "set g_nextMap %s; set g_nextMapLayouts %s", Cmd::Escape( arg ).c_str(),
		             Cmd::Escape( reason ).c_str() );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             "Set the next map to '%s' layout '%s'", arg.c_str(), reason.c_str() );
	}
	else
	{
		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "set g_nextMap %s; set g_nextMapLayouts \"\"", Cmd::Escape( arg ).c_str() );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ), "Set the next map to '%s'",
		             arg.c_str() );
	}

	reason.clear();  // nullify since we've used it here...
	return true;
}

static bool HandlePollVote( gentity_t*, team_t team, const std::string&, const std::string&,
                            std::string& reason, const std::string&, int, int )
{
	level.team[ team ].voteString[ 0 ] = '\0';
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ), "(poll) %s", reason.c_str() );
	reason.clear();  // nullify since we've used it here...
	return true;
}

static bool HandleKickbotsVote( gentity_t* ent, team_t team, const std::string& cmd, const std::string&,
                                std::string&, const std::string&, int, int )
{
	int numBots = 0;
	for ( const auto& t : level.team )
	{
		numBots += t.numBots;
	}

	if ( numBots == 0 )
	{
		trap_SendServerCommand(
			ent->num(),
			va( "print_tr %s %s", QQ( N_( "$1$: there are no active bots" ) ), cmd.c_str() ) );
		return false;
	}

	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
	             "bot del all" );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ), N_( "Remove all bots" ) );
	return true;
}

static bool HandleFillBotsVote( gentity_t* ent, team_t team, const std::string& cmd, const std::string& arg,
                                std::string&, const std::string&, int, int  )
{
	int num = 0;
	if ( !botFillVoteParseArg( num, arg ) )
	{
		trap_SendServerCommand(
			ent->num(), va( "print_tr %s %s %s",
		                    QQ( N_( "$1$: number must be non-negative and smaller than $2$" ) ),
		                    cmd.c_str(), std::to_string( g_maxVoteFillBots.Get() + 1 ).c_str() ) );
		return false;
	}

	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
	             "bot fill %d", num );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ),
	             N_( "Fill each team with bots to %d" ), num );
	return true;
}

static bool HandleFillBotsHumanVote( gentity_t* ent, team_t team, const std::string& cmd,
                                     const std::string& arg, std::string&, const std::string&,
                                     int, int )
{
	int num = 0;
	if ( !botFillVoteParseArg( num, arg ) )
	{
		trap_SendServerCommand(
			ent->num(), va( "print_tr %s %s %s",
		                    QQ( N_( "$1$: number must be non-negative and smaller than $2$" ) ),
		                    cmd.c_str(), std::to_string( g_maxVoteFillBots.Get() + 1 ).c_str() ) );
		return false;
	}

	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
	             "bot fill %d h", num );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ),
	             N_( "Fill the human team with bots to %d" ), num );
	return true;
}

static bool HandleFillBotsAliensVote( gentity_t* ent, team_t team, const std::string& cmd,
                                      const std::string& arg, std::string&, const std::string&,
                                      int, int )
{
	int num = 0;
	if ( !botFillVoteParseArg( num, arg ) )
	{
		trap_SendServerCommand(
			ent->num(), va( "print_tr %s %s %s",
		                    QQ( N_( "$1$: number must be non-negative and smaller than $2$" ) ),
		                    cmd.c_str(), std::to_string( g_maxVoteFillBots.Get() + 1 ).c_str() ) );
		return false;
	}

	Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
	             "bot fill %d a", num );
	Com_sprintf( level.team[ team ].voteDisplayString,
	             sizeof( level.team[ team ].voteDisplayString ),
	             N_( "Fill the alien team with bots to %d" ), num );
	return true;
}

// Basic vote information
// clang-format off
static std::unordered_map<std::string, VoteDefinition> voteInfo = {
//   Vote                   Stop?  Type      Target     Immune  Quorum    Reason          Vote percentage var         Extra                                                         Handler
	{"kick",              { false, V_ANY,    T_PLAYER,   true,   true,  qtrinary::qyes,   &g_kickVotesPercent,        VOTE_ALWAYS,  nullptr,             nullptr,                   &HandleKickVote } },
	{"spectate",          { true , V_ANY,    T_PLAYER,   true,   true,  qtrinary::qyes,   &g_kickVotesPercent,        VOTE_ALWAYS,  nullptr,             nullptr,                   &HandleSpectateVote } },
	{"mute",              { false, V_PUBLIC, T_PLAYER,   true,   true,  qtrinary::qyes,   &g_denyVotesPercent,        VOTE_ALWAYS,  nullptr,             nullptr,                   &HandleMuteVote } },
	{"unmute",            { false, V_PUBLIC, T_PLAYER,   false,  true,  qtrinary::qno,    &g_denyVotesPercent,        VOTE_ALWAYS,  nullptr,             nullptr,                   &HandleUnmuteVote } },
	{"denybuild",         { true,  V_TEAM,   T_PLAYER,   true,   true,  qtrinary::qyes,   &g_denyVotesPercent,        VOTE_ALWAYS,  nullptr,             nullptr,                   &HandleDenybuildVote } },
	{"allowbuild",        { true,  V_TEAM,   T_PLAYER,   false,  true,  qtrinary::qno,    &g_denyVotesPercent,        VOTE_ALWAYS,  nullptr,             nullptr,                   &HandleAllowbuildVote } },
	{"extend",            { true,  V_PUBLIC, T_OTHER,    false,  false, qtrinary::qno,    &g_extendVotesPercent,      VOTE_REMAIN,  &g_extendVotesTime,  nullptr,                   &HandleExtendVote } },
	{"admitdefeat",       { true,  V_TEAM,   T_NONE,     false,  true,  qtrinary::qno,    &g_admitDefeatVotesPercent, VOTE_ALWAYS,  nullptr,             nullptr,                   &HandleAdmitDefeatVote } },
	{"draw",              { true,  V_PUBLIC, T_NONE,     false,  true,  qtrinary::qyes,   &g_drawVotesPercent,        VOTE_AFTER,   &g_drawVotesAfter,   &g_drawVoteReasonRequired, &HandleDrawVote } },
	{"map_restart",       { true,  V_PUBLIC, T_NONE,     false,  true,  qtrinary::qno,    &g_mapVotesPercent,         VOTE_ALWAYS,  nullptr,             nullptr,                   &HandleMapRestart } },
	{"map",               { true,  V_PUBLIC, T_OTHER,    false,  true,  qtrinary::qmaybe, &g_mapVotesPercent,         VOTE_BEFORE,  &g_mapVotesBefore,   nullptr,                   &HandleMapVote } },
	{"layout",            { true,  V_PUBLIC, T_OTHER,    false,  true,  qtrinary::qno,    &g_mapVotesPercent,         VOTE_BEFORE,  &g_mapVotesBefore,   nullptr,                   &HandleLayoutVote } },
	{"nextmap",           { false, V_PUBLIC, T_OTHER,    false,  false, qtrinary::qmaybe, &g_nextMapVotesPercent,     VOTE_ALWAYS,  nullptr,             nullptr,                   &HandleNextMapVote } },
	{"poll",              { false, V_ANY,    T_NONE,     false,  false, qtrinary::qyes,   &g_pollVotesPercent,        VOTE_NO_AUTO, nullptr,             nullptr,                   &HandlePollVote } },
	{"kickbots",          { true,  V_PUBLIC, T_NONE,     false,  false, qtrinary::qno,    &g_kickVotesPercent,        VOTE_ALWAYS,  nullptr,             nullptr,                   &HandleKickbotsVote } },
	{"fillbots",          { true,  V_PUBLIC, T_OTHER,    false,  true,  qtrinary::qno,    &g_fillBotsVotesPercent,    VOTE_ALWAYS,  nullptr,             nullptr,                   &HandleFillBotsVote } },
	{"fillbots_humans",   { true,  V_PUBLIC, T_OTHER,    false,  true,  qtrinary::qno,    &g_fillBotsTeamVotesPercent, VOTE_ALWAYS, nullptr,             nullptr,                   &HandleFillBotsHumanVote } },
	{"fillbots_aliens",   { true,  V_PUBLIC, T_OTHER,    false,  true,  qtrinary::qno,    &g_fillBotsTeamVotesPercent, VOTE_ALWAYS, nullptr,             nullptr,                   &HandleFillBotsAliensVote } },
};

// clang-format on
/*
==================
G_CheckStopVote
==================
*/
bool G_CheckStopVote( team_t team )
{
	if ( level.team[ team ].voteType.empty() )
	{
		return false;
	}
	const auto& it = voteInfo.find( level.team[ team ].voteType );
	if ( it == voteInfo.end() )
	{
		return false;
	}
	return level.team[ team ].voteTime && it->second.stopOnIntermission;
}

/*
==================
isDisabledVoteType

Check for disabled vote types.
Does not distinguish between public and team votes.
==================
*/
static bool isDisabledVoteType( const char* vote )
{
	for ( Parse_WordListSplitter i( g_disabledVoteCalls.Get() ); *i; ++i )
	{
		if ( Q_stricmp( vote, *i ) == 0 ) return true;
	}
	return false;
}

void G_HandleVote( gentity_t* ent )
{
	const Cmd::Args& args = trap_Args();
	std::string cmd = Str::ToLower( args.Argv( 0 ) );
	team_t team = (team_t)( ( cmd == "callteamvote" ) ? ent->client->pers.team : TEAM_NONE );

	if ( !g_allowVote.Get() )
	{
		trap_SendServerCommand(
			ent->num(),
			va( "print_tr %s %s", QQ( N_( "$1$: voting not allowed here" ) ), cmd.c_str() ) );
		return;
	}

	if ( ( G_admin_permission( ent, ADMF_NO_GLOBALVOTE ) && team == TEAM_NONE ) ||
	     ( G_admin_permission( ent, ADMF_NO_TEAMVOTE ) && team != TEAM_NONE ) )
	{
		trap_SendServerCommand(
			ent->num(),
			va( "print_tr %s %s %s", QQ( N_( "$1$: you are not permitted to call $2$ votes" ) ),
		        cmd.c_str(), team != TEAM_NONE ? QQ( "team" ) : QQ( "global" ) ) );
		return;
	}

	if ( level.team[ team ].voteTime )
	{
		trap_SendServerCommand(
			ent->num(),
			va( "print_tr %s %s", QQ( N_( "$1$: a vote is already in progress" ) ), cmd.c_str() ) );
		return;
	}

	if ( level.team[ team ].voteExecuteTime )
	{
		G_ExecuteVote( team );
	}

	G_ResetVote( team );

	std::string vote = args.Argc() > 1 ? Str::ToLower( args.Argv( 1 ) ) : "";

	// look up the vote detail
	auto it = voteInfo.find( vote );
	if ( vote.empty() || it == voteInfo.end() ||
	     ( team == TEAM_NONE && it->second.type == V_TEAM ) ||
	     ( team != TEAM_NONE && it->second.type == V_PUBLIC ) )
	{
		bool added = false;

		trap_SendServerCommand( ent->num(), "print_tr \"" N_( "Invalid vote string" ) "\"" );
		trap_SendServerCommand(
			ent->num(),
			va( "print_tr %s", team == TEAM_NONE ? QQ( N_( "Valid vote commands are: " ) )
		                                         : QQ( N_( "Valid team-vote commands are: " ) ) ) );
		cmd.clear();
		cmd.reserve( MAX_STRING_CHARS );
		cmd += "print \"";

		for ( const auto& it2 : voteInfo )
		{
			const VoteDefinition& vi = it2.second;
			if ( ( team == TEAM_NONE && vi.type != V_TEAM ) ||
			     ( team != TEAM_NONE && vi.type != V_PUBLIC ) )
			{
				if ( !vi.percentage || vi.percentage->Get() > 0 )
				{
					cmd += va( "%s%s", added ? ", " : "", it2.first.c_str() );
					added = true;
				}
			}
		}

		cmd += "\"";
		trap_SendServerCommand( ent->num(), cmd.c_str() );
		return;
	}
	const VoteDefinition& vi = it->second;

	if ( g_voteLimit.Get() > 0 && ent->client->pers.namelog->voteCount >= g_voteLimit.Get() &&
	     !G_admin_permission( ent, ADMF_NO_VOTE_LIMIT ) )
	{
		trap_SendServerCommand(
			ent->num(),
			va( "print_tr %s %s %d",
		        QQ( N_( "$1$: you have already called the maximum number of votes ($2$)" ) ),
		        cmd.c_str(), g_voteLimit.Get() ) );
		return;
	}

	int voteThreshold = vi.percentage ? vi.percentage->Get() : 50;
	if ( voteThreshold <= 0 || isDisabledVoteType( vote.c_str() ) )
	{
		trap_SendServerCommand(
			ent->num(),
			va( "print_tr %s %s", QQ( N_( "'$1$' votes have been disabled" ) ), vote.c_str() ) );
		return;
	}
	if ( voteThreshold > 100 )
	{
		voteThreshold = 100;
	}

	level.team[ team ].quorum = vi.quorum;
	level.team[ team ].voteDelay = 0;
	level.team[ team ].voteThreshold = voteThreshold;

	switch ( vi.special )
	{
		case VOTE_BEFORE:
			if ( level.numConnectedPlayers > 1 &&
			     level.matchTime >= ( vi.specialCvar->Get() * 60000 ) )
			{
				trap_SendServerCommand(
					ent->num(),
					va( "print_tr %s %s %d",
				        QQ( N_( "'$1$' votes are not allowed once $2$ minutes have passed" ) ),
				        vote.c_str(), vi.specialCvar->Get() ) );
				return;
			}

			break;

		case VOTE_AFTER:
			if ( level.numConnectedPlayers > 1 &&
			     level.matchTime < ( vi.specialCvar->Get() * 60000 ) )
			{
				trap_SendServerCommand(
					ent->num(),
					va( "print_tr %s %s %d",
				        QQ( N_( "'$1$' votes are not allowed until $2$ minutes have passed" ) ),
				        vote.c_str(), vi.specialCvar->Get() ) );
				return;
			}

			break;

		case VOTE_REMAIN:
			if ( !level.timelimit ||
			     level.matchTime < ( level.timelimit - vi.specialCvar->Get() / 2 ) * 60000 )
			{
				trap_SendServerCommand(
					ent->num(),
					va( "print_tr %s %s %d",
				        QQ( N_(
							"'$1$' votes are only allowed with less than $2$ minutes remaining" ) ),
				        vote.c_str(), vi.specialCvar->Get() / 2 ) );
				return;
			}

			break;

		default:;
	}

	// Get argument and reason, if needed
	std::string arg;
	std::string reason;

	std::string name;
	int clientNum = -1;
	int id = -1;

	if ( vi.target != T_NONE )
	{
		arg = args.Argc() > 2 ? args.Argv( 2 ) : "";
	}

	if ( vi.reasonNeeded != qtrinary::qno )
	{
		if ( vi.target != T_NONE )
		{
			reason = args.Argc() > 3 ? args.ConcatArgs( 3 ) : "";
		}
		else
		{
			reason = args.Argc() > 2 ? args.ConcatArgs( 2 ) : "";
		}

		reason = Color::StripColors( reason );
	}

	if ( vi.target == T_PLAYER )
	{
		char err[ MAX_STRING_CHARS ];

		if ( !arg[ 0 ] )
		{
			trap_SendServerCommand(
				ent->num(), va( "print_tr %s %s", QQ( N_( "$1$: no target" ) ), cmd.c_str() ) );
			return;
		}

		// with a little extra work only players from the right team are considered
		clientNum = G_ClientNumberFromString( arg.c_str(), err, sizeof( err ) );

		if ( clientNum == -1 )
		{
			ADMP( va( "%s %s %s", QQ( "$1$: $2t$" ), cmd.c_str(), Quote( err ) ) );
			return;
		}

		if ( level.clients[ clientNum ].pers.isBot )
		{
			trap_SendServerCommand(
				ent->num(),
				va( "print_tr %s %s", QQ( N_( "$1$: player is a bot" ) ), cmd.c_str() ) );
			return;
		}

		name = level.clients[ clientNum ].pers.netname;
		name = Color::StripColors( name );
		id = level.clients[ clientNum ].pers.namelog->id;

		if ( vi.adminImmune && G_admin_permission( g_entities + clientNum, ADMF_IMMUNITY ) )
		{
			trap_SendServerCommand(
				ent->num(),
				va( "print_tr %s %s", QQ( N_( "$1$: admin is immune" ) ), cmd.c_str() ) );
			G_AdminMessage( nullptr, va( "^7%s^3 attempted %s %s"
			                             " on immune admin ^7%s"
			                             " ^3for: %s",
			                             ent->client->pers.netname, cmd.c_str(), vote.c_str(),
			                             g_entities[ clientNum ].client->pers.netname,
			                             !reason.empty() ? reason.c_str() : "no reason" ) );
			return;
		}

		if ( level.clients[ clientNum ].pers.localClient )
		{
			trap_SendServerCommand(
				ent->num(),
				va( "print_tr %s %s", QQ( N_( "$1$: admin is immune" ) ), cmd.c_str() ) );
			return;
		}

		if ( team != TEAM_NONE && ent->client->pers.team != level.clients[ clientNum ].pers.team )
		{
			trap_SendServerCommand(
				ent->num(), va( "print_tr %s %s", QQ( N_( "$1$: player is not on your team" ) ),
			                    cmd.c_str() ) );
			return;
		}
	}

	if ( vi.reasonNeeded == qtrinary::qyes && !reason[ 0 ] &&
	     !( vi.adminImmune && G_admin_permission( ent, ADMF_UNACCOUNTABLE ) ) &&
	     !( vi.reasonFlag && vi.reasonFlag->Get() ) )
	{
		trap_SendServerCommand(
			ent->num(),
			va( "print_tr %s %s", QQ( N_( "$1$: You must provide a reason" ) ), cmd.c_str() ) );
		return;
	}

	if ( !vi.handler( ent, team, cmd, arg, reason, name, clientNum, id ) )
	{
		return;
	}

	// Append the vote reason (if applicable)
	if ( !reason.empty() )
	{
		Q_strcat( level.team[ team ].voteDisplayString,
		          sizeof( level.team[ team ].voteDisplayString ),
		          va( " for '%s'", reason.c_str() ) );
	}

	G_LogPrintf( "%s: %d \"%s^*\": %s", team == TEAM_NONE ? "CallVote" : "CallTeamVote",
	             (int)( ent->num() ), ent->client->pers.netname, level.team[ team ].voteString );

	if ( team == TEAM_NONE )
	{
		trap_SendServerCommand( -1, va( "print_tr %s %s %s", QQ( N_( "$1$^* called a vote: $2$" ) ),
		                                Quote( ent->client->pers.netname ),
		                                Quote( level.team[ team ].voteDisplayString ) ) );

		trap_SendServerCommand( -1, va( "cp_tr %s %s %s", QQ( N_( "$1$^7 called a vote:\n^7$2$" ) ),
		                                Quote( ent->client->pers.netname ),
		                                Quote( level.team[ team ].voteDisplayString ) ) );
	}
	else
	{
		int i;

		for ( i = 0; i < level.maxclients; i++ )
		{
			if ( level.clients[ i ].pers.connected == CON_CONNECTED )
			{
				if ( level.clients[ i ].pers.team == team ||
				     ( level.clients[ i ].pers.team == TEAM_NONE &&
				       G_admin_permission( &g_entities[ i ], ADMF_SPEC_ALLCHAT ) ) )
				{
					trap_SendServerCommand(
					    i, va( "print_tr %s %s %s", QQ( N_( "$1$^* called a team vote: $2t$" ) ),
					           Quote( ent->client->pers.netname ),
					           Quote( level.team[ team ].voteDisplayString ) ) );

					trap_SendServerCommand(
					    i, va( "cp_tr %s %s %s", QQ( N_( "$1$^7 called a team vote:\n^7$2t$" ) ),
					           Quote( ent->client->pers.netname ),
					           Quote( level.team[ team ].voteDisplayString ) ) );
				}
				else if ( G_admin_permission( &g_entities[ i ], ADMF_ADMINCHAT ) )
				{
					trap_SendServerCommand(
					    i, va( "chat -1 %d ^3%s\"^3 called a team vote (%ss): \"%s", SAY_ADMINS,
					           Quote( ent->client->pers.netname ), BG_TeamName( team ),
					           Quote( level.team[ team ].voteDisplayString ) ) );
				}
			}
		}
	}

	level.team[ team ].voteTime = level.time;
	trap_SetConfigstring( CS_VOTE_TIME + team, va( "%d", level.team[ team ].voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING + team, level.team[ team ].voteDisplayString );
	trap_SetConfigstring( CS_VOTE_CALLER + team, ent->client->pers.netname );

	if ( vi.special != VOTE_NO_AUTO )
	{
		ent->client->pers.namelog->voteCount++;
		ent->client->pers.voteYes |= 1 << team;
		G_Vote( ent, team, true );
	}
}

void G_ExecuteVote( team_t team )
{
	level.team[ team ].voteExecuteTime = 0;

	trap_SendConsoleCommand( va( "%s", level.team[ team ].voteString ) );

	if ( !Q_stricmp( level.team[ team ].voteString, "map_restart" ) )
	{
		G_MapLog_Result( 'r' );
		level.restarted = true;
	}
	else if ( Str::IsIPrefix( "map ", level.team[ team ].voteString ) )
	{
		G_MapLog_Result( 'm' );
		// FIXME this is hacky; the map command could fail and this would probably freeze the game
		level.restarted = true;
	}
}

/*
==================
G_CheckVote
==================
*/
void G_CheckVote( team_t team )
{
	float votePassThreshold = (float)level.team[ team ].voteThreshold / 100.0f;
	bool pass = false;
	bool quorum = true;
	char* cmd;

	if ( level.team[ team ].voteExecuteTime /* > 0 ?? more readable imho */ &&
	     level.team[ team ].voteExecuteTime < level.time )
	{
		G_ExecuteVote( team );
	}

	if ( !level.team[ team ].voteTime )
	{
		return;
	}

	if ( ( level.time - level.team[ team ].voteTime >= VOTE_TIME ) ||
	     ( level.team[ team ].voteYes + level.team[ team ].voteNo ==
	       level.team[ team ].numPlayers ) )
	{
		pass = ( level.team[ team ].voteYes &&
		         (float)level.team[ team ].voteYes /
		                 ( (float)level.team[ team ].voteYes + (float)level.team[ team ].voteNo ) >
		             votePassThreshold );
	}
	else
	{
		if ( (float)level.team[ team ].voteYes >
		     (float)level.team[ team ].numPlayers * votePassThreshold )
		{
			pass = true;
		}
		else if ( (float)level.team[ team ].voteNo <=
		          (float)level.team[ team ].numPlayers * ( 1.0f - votePassThreshold ) )
		{
			return;
		}
	}

	// If quorum is required, check whether at least half of who could vote did
	if ( level.team[ team ].quorum &&
	     level.team[ team ].voted < floor( powf( level.team[ team ].numPlayers, 0.6 ) ) )
	{
		quorum = false;
	}

	if ( pass && quorum )
	{
		level.team[ team ].voteExecuteTime = level.time + level.team[ team ].voteDelay;
	}

	G_LogPrintf( "EndVote: %s %s %d %d %d %d", team == TEAM_NONE ? "global" : BG_TeamName( team ),
	             pass ? "pass" : "fail", level.team[ team ].voteYes, level.team[ team ].voteNo,
	             level.team[ team ].numPlayers, level.team[ team ].voted );

	if ( !quorum )
	{
		cmd = va(
			"print_tr %s %d %d",
			( team == TEAM_NONE ) ? QQ( N_( "Vote failed ($1$ of $2$; quorum not reached)" ) )
								  : QQ( N_( "Team vote failed ($1$ of $2$; quorum not reached)" ) ),
			level.team[ team ].voteYes + level.team[ team ].voteNo, level.team[ team ].numPlayers );
	}
	else if ( pass )
	{
		cmd = va( "print_tr %s %d %d",
		          ( team == TEAM_NONE ) ? QQ( N_( "Vote passed ($1$ — $2$)" ) )
		                                : QQ( N_( "Team vote passed ($1$ — $2$)" ) ),
		          level.team[ team ].voteYes, level.team[ team ].voteNo );
	}
	else
	{
		cmd = va( "print_tr %s %d %d %.0f",
		          ( team == TEAM_NONE ) ? QQ( N_( "Vote failed ($1$ — $2$; $3$% needed)" ) )
		                                : QQ( N_( "Team vote failed ($1$ — $2$; $3$% needed)" ) ),
		          level.team[ team ].voteYes, level.team[ team ].voteNo, votePassThreshold * 100 );
	}

	if ( team == TEAM_NONE )
	{
		trap_SendServerCommand( -1, cmd );
	}
	else
	{
		G_TeamCommand( team, cmd );
	}

	G_ResetVote( team );
}

void G_ResetVote( team_t team )
{
	int i;

	level.team[ team ].voteTime = 0;
	level.team[ team ].voteYes = 0;
	level.team[ team ].voteNo = 0;
	level.team[ team ].voted = 0;

	for ( i = 0; i < level.maxclients; i++ )
	{
		level.clients[ i ].pers.voted &= ~( 1 << team );
		level.clients[ i ].pers.voteYes &= ~( 1 << team );
		level.clients[ i ].pers.voteNo &= ~( 1 << team );
	}

	trap_SetConfigstring( CS_VOTE_TIME + team, "" );
	trap_SetConfigstring( CS_VOTE_STRING + team, "" );
	trap_SetConfigstring( CS_VOTE_YES + team, "0" );
	trap_SetConfigstring( CS_VOTE_NO + team, "0" );
}

/*
==================
G_Vote
==================
*/
/*
==================
G_Vote
==================
*/
void G_Vote( gentity_t* ent, team_t team, bool voting )
{
	if ( !level.team[ team ].voteTime )
	{
		return;
	}

	// TODO maybe refactor vote no/yes in one and only one variable and divide the NLOC by 2 ?
	if ( voting )
	{
		if ( ( ent->client->pers.voted & ( 1 << team ) ) )
		{
			return;
		}
		level.team[ team ].voted++;
		ent->client->pers.voted |= 1 << team;
		if ( ent->client->pers.voteYes & ( 1 << team ) )
		{
			level.team[ team ].voteYes++;
			trap_SetConfigstring( CS_VOTE_YES + team, va( "%d", level.team[ team ].voteYes ) );
		}

		if ( ent->client->pers.voteNo & ( 1 << team ) )
		{
			level.team[ team ].voteNo++;
			trap_SetConfigstring( CS_VOTE_NO + team, va( "%d", level.team[ team ].voteNo ) );
		}
		return;
	}

	if ( !( ent->client->pers.voted & ( 1 << team ) ) )
	{
		return;
	}

	ent->client->pers.voted &= ~( 1 << team );
	level.team[ team ].voted--;
	bool wasYes = ent->client->pers.voteYes & ( 1 << team );
	bool wasNo = ent->client->pers.voteNo & ( 1 << team );
	ent->client->pers.voteYes &= ~( 1 << team );
	ent->client->pers.voteNo &= ~( 1 << team );

	if ( wasYes )
	{
		level.team[ team ].voteYes--;
		trap_SetConfigstring( CS_VOTE_YES + team, va( "%d", level.team[ team ].voteYes ) );
	}
	if ( wasNo )
	{
		level.team[ team ].voteNo--;
		trap_SetConfigstring( CS_VOTE_NO + team, va( "%d", level.team[ team ].voteNo ) );
	}
}
static constexpr char MARKER = '$';

static bool IsEscapedMarker( Str::StringRef s, size_t pos )
{
	return pos + 1 < s.size() && s[ pos ] == MARKER && s[ pos + 1 ] == MARKER;
}

// Use a custom escape to avoid issues with Cmd::Escape unintentionally terminating quotes
// for strings based on user input. Note that this needs to work in both quoted and unquoted contexts.
static std::string VoteEscape( Str::StringRef s )
{
	if ( s.empty() )
	{
		return "\"\"";
	}
	std::string out;
	out.reserve( s.size() * 2 );
	for ( const auto& c : s )
	{
		out += '\\';
		out += c;
	}
	return out;
}

// We only want to escape for commands that are executed. For display only, there is no injection
// concern, so don't escape those.
static std::string G_HandleVoteTemplate( Str::StringRef str, gentity_t* ent, team_t team,
                                         const std::string&, const std::string& arg, std::string& reason,
                                         const std::string& name, int clientNum, int id, bool escape )
{
	std::unordered_map<std::string, std::string> params = {
		{ "team", BG_TeamNamePlural( team ) },
		{ "arg", arg },
		{ "reason", reason },
		{ "name", name },
		{ "slot", std::to_string( clientNum ) },
		{ "namelogId", std::to_string( id ) },
		{ "caller", ent->client->pers.netname },
	};
	std::string out;
	out.reserve( str.size() + arg.size() + reason.size() );
	size_t c = 0;
	size_t s = 0;
	size_t e = 0;
	while ( c < str.size() )
	{
		s = str.find( MARKER, c );
		if ( s != std::string::npos && !IsEscapedMarker( str, s ) )
		{
			e = str.find( MARKER, s + 1 );
			if ( e != std::string::npos )
			{
				out += str.substr( c, s - c );
				const auto& param = params[ str.substr( s + 1, e - s - 1 ) ];
				out += escape ? VoteEscape( param ) : param;
				c = e + 1;
			}
			else
			{
				Log::Warn( "Unterminated %c in str: %s", MARKER, str );
				return "";
			}
		}
		else
		{
			out += str.substr( c );
			c = str.size();
		}
	}
	return out;
}

bool G_AddCustomVote( std::string vote, VoteDefinition def, std::string voteTemplate,
                      std::string displayTemplate )
{
	if ( vote.empty() )
	{
		return false;
	}
	auto it = voteInfo.find( vote );
	if ( it != voteInfo.end() )
	{
		return false;
	}
	it = voteInfo.emplace( std::move( vote ), std::move( def ) ).first;
	auto handler = [ vt = std::move( voteTemplate ), dt = std::move( displayTemplate ) ](
					   gentity_t* ent, team_t team, const std::string& cmd, const std::string& arg,
					   std::string& reason, const std::string& name, int clientNum, int id )
	{
		std::string voteCmd =
			G_HandleVoteTemplate( vt, ent, team, cmd, arg, reason, name, clientNum, id, true );
		std::string display =
			G_HandleVoteTemplate( dt, ent, team, cmd, arg, reason, name, clientNum, id, false );

		if ( voteCmd.size() >= sizeof( level.team[ team ].voteString ) )
		{
			Log::Warn( "vote command string is too long: %s: %d > %d", voteCmd, voteCmd.size(),
			           sizeof( level.team[ team ].voteString ) );
			return false;
		}
		if ( display.size() >= sizeof( level.team[ team ].voteDisplayString ) )
		{
			Log::Warn( "vote display string is too long: %s: %d > %d", display, display.size(),
			           sizeof( level.team[ team ].voteDisplayString ) );
			return false;
		}

		Q_strncpyz( level.team[ team ].voteString, voteCmd.c_str(),
		            sizeof( level.team[ team ].voteString ) );

		Q_strncpyz( level.team[ team ].voteDisplayString, display.c_str(),
		            sizeof( level.team[ team ].voteDisplayString ) );
		return true;
	};
	it->second.handler = std::move( handler );
	return true;
}

bool G_ParseVoteType( Str::StringRef s, VoteType* type )
{
	static const std::unordered_map<std::string, VoteType, Str::IHash, Str::IEqual> map = {
		{ "V_ANY", V_ANY },
		{ "V_TEAM", V_TEAM },
		{ "V_PUBLIC", V_PUBLIC },
	};
	const auto& it = map.find( s );
	if ( it == map.end() )
	{
		return false;
	}
	*type = it->second;
	return true;
}

Str::StringRef G_VoteTypeString( VoteType type )
{
	switch ( type )
	{
		case V_TEAM:
			return "V_TEAM";
		case V_PUBLIC:
			return "V_PUBLIC";
		case V_ANY:
		default:
			return "V_ANY";
	}
}

bool G_ParseVoteTarget( Str::StringRef s, VoteTarget* type )
{
	static const std::unordered_map<std::string, VoteTarget, Str::IHash, Str::IEqual> map = {
		{ "T_NONE", T_NONE },
		{ "T_PLAYER", T_PLAYER },
		{ "T_OTHER", T_OTHER },
	};
	const auto& it = map.find( s );
	if ( it == map.end() )
	{
		return false;
	}
	*type = it->second;
	return true;
}

Str::StringRef G_VoteTargetString( VoteTarget type )
{
	switch ( type )
	{
		case T_PLAYER:
			return "T_PLAYER";
		case T_OTHER:
			return "T_OTHER";
		case T_NONE:
		default:
			return "T_NONE";
	}
}

bool G_ParseVoteOptions( Str::StringRef s, VoteOptions* type )
{
	static const std::unordered_map<std::string, VoteOptions, Str::IHash, Str::IEqual> map = {
		{ "VOTE_ALWAYS", VOTE_ALWAYS },
		{ "VOTE_BEFORE", VOTE_BEFORE },
		{ "VOTE_AFTER", VOTE_AFTER },
		{ "VOTE_REMAIN", VOTE_REMAIN },
		{ "VOTE_NO_AUTO", VOTE_NO_AUTO },
	};
	const auto& it = map.find( s );
	if ( it == map.end() )
	{
		return false;
	}
	*type = it->second;
	return true;
}

Str::StringRef G_VoteOptionsString( VoteOptions type )
{
	switch ( type )
	{
		case VOTE_BEFORE:
			return "VOTE_BEFORE";
		case VOTE_AFTER:
			return "VOTE_AFTER";
		case VOTE_REMAIN:
			return "VOTE_REMAIN";
		case VOTE_NO_AUTO:
			return "VOTE_NO_AUTO";
		case VOTE_ALWAYS:
		default:
			return "VOTE_ALWAYS";
	}
}

bool G_ParseReasonNeeded( Str::StringRef s, qtrinary* tri )
{
	static const std::unordered_map<std::string, qtrinary, Str::IHash, Str::IEqual> map = {
		{ "yes", qtrinary::qyes },
		{ "no", qtrinary::qno },
		{ "maybe", qtrinary::qmaybe },
	};
	const auto& it = map.find( s );
	if ( it == map.end() )
	{
		return false;
	}
	*tri = it->second;
	return true;
}

Str::StringRef G_ReasonNeededString( qtrinary tri )
{
	switch ( tri )
	{
		case qtrinary::qyes:
			return "yes";
		case qtrinary::qmaybe:
			return "maybe";
		case qtrinary::qno:
		default:
			return "no";
	}
}
