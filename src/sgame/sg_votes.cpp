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

#include "sg_votes.h"

#include "sg_local.h"
#include "shared/parse.h"

// Basic vote information
// Entries must be in the same order as for voteType_t
static const std::unordered_map<std::string, VoteDefinition> voteInfo = {
	// Name                Stop?  Type      Target     Immune  Quorum    Reason            Vote percentage var  Extra
	{ "kick",            { false, V_ANY,    T_PLAYER,  true,   true,     qtrinary::qyes,   &g_kickVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "spectate",        { false, V_ANY,    T_PLAYER,  true,   true,     qtrinary::qyes,   &g_kickVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "mute",            { true,  V_PUBLIC, T_PLAYER,  true,   true,     qtrinary::qyes,   &g_denyVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "unmute",          { true,  V_PUBLIC, T_PLAYER,  false,  true,     qtrinary::qno,    &g_denyVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "denybuild",       { true,  V_TEAM,   T_PLAYER,  true,   true,     qtrinary::qyes,   &g_denyVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "allowbuild",      { true,  V_TEAM,   T_PLAYER,  false,  true,     qtrinary::qno,    &g_denyVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "extend",          { true,  V_PUBLIC, T_OTHER,   false,  false,    qtrinary::qno,    &g_extendVotesPercent,      VOTE_REMAIN, &g_extendVotesTime, nullptr } },
	{ "admitdefeat",     { true,  V_TEAM,   T_NONE,    false,  true,     qtrinary::qno,    &g_admitDefeatVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "draw",            { true,  V_PUBLIC, T_NONE,    true,   true,     qtrinary::qyes,   &g_drawVotesPercent,        VOTE_AFTER,  &g_drawVotesAfter,  &g_drawVoteReasonRequired } },
	{ "map_restart",     { true,  V_PUBLIC, T_NONE,    false,  true,     qtrinary::qno,    &g_mapVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "map",             { true,  V_PUBLIC, T_OTHER,   false,  true,     qtrinary::qmaybe, &g_mapVotesPercent,         VOTE_BEFORE, &g_mapVotesBefore, nullptr } },
	{ "layout",          { true,  V_PUBLIC, T_OTHER,   false,  true,     qtrinary::qno,    &g_mapVotesPercent,         VOTE_BEFORE, &g_mapVotesBefore, nullptr } },
	{ "nextmap",         { false, V_PUBLIC, T_OTHER,   false,  false,    qtrinary::qmaybe, &g_nextMapVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "poll",            { false, V_ANY,    T_NONE,    false,  false,    qtrinary::qyes,   &g_pollVotesPercent,        VOTE_NO_AUTO, nullptr, nullptr } },
	{ "kickbots",        { true,  V_PUBLIC, T_NONE,    false,  false,    qtrinary::qno,    &g_kickVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "fillbots",        { true,  V_PUBLIC, T_OTHER,   false,  true,     qtrinary::qno,    &g_fillBotsVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "fillbots_humans", { true,  V_PUBLIC, T_OTHER,   false,  true,     qtrinary::qno,    &g_fillBotsTeamVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	{ "fillbots_aliens", { true,  V_PUBLIC, T_OTHER,   false,  true,     qtrinary::qno,    &g_fillBotsTeamVotesPercent, VOTE_ALWAYS, nullptr, nullptr } },
	// note: map votes use the reason, if given, as the layout name
};

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
reason[0] = '\0'; // nullify since we've used it here...
Check for disabled vote types.
Does not distinguish between public and team votes.
==================
*/
static bool isDisabledVoteType(const char *vote)
{
	for (Parse_WordListSplitter i(g_disabledVoteCalls.Get()); *i; ++i)
	{
		if (Q_stricmp(vote, *i) == 0) return true;
	}
	return false;
}

/*reason[0] = '\0'; // nullify since we've used it here...
Return true if arg is valid, and store the number in argnum.
Otherwise, return false and do not modify argnum.
*/
static bool botFillVoteParseArg( int& argnum, char *arg )
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

void G_HandleVote( gentity_t *ent )
{
	char   cmd[ MAX_TOKEN_CHARS ],
	       voteStr[ MAX_TOKEN_CHARS ],
	       arg[ MAX_TOKEN_CHARS ];
	char   name[ MAX_NAME_LENGTH ] = "";
	char   caller[ MAX_NAME_LENGTH ] = "";
	char   reason[ MAX_TOKEN_CHARS ];
	int    clientNum = -1;
	int    id = -1;
	team_t team;
	int    i;

	trap_Argv( 0, cmd, sizeof( cmd ) );
	team = (team_t) ( ( !Q_stricmp( cmd, "callteamvote" ) ) ? ent->client->pers.team : TEAM_NONE );

	if ( !g_allowVote.Get() )
	{
		trap_SendServerCommand( ent - g_entities,
		                        va( "print_tr %s %s", QQ( N_("$1$: voting not allowed here") ), cmd ) );
		return;
	}

	if ( level.team[ team ].voteTime )
	{
		trap_SendServerCommand( ent - g_entities,
		                        va( "print_tr %s %s", QQ( N_("$1$: a vote is already in progress") ), cmd ) );
		return;
	}

	if ( level.team[ team ].voteExecuteTime )
	{
		G_ExecuteVote( team );
	}

	G_ResetVote( team );

	trap_Argv( 1, voteStr, sizeof( voteStr ) );
    std::string vote = Str::ToLower( voteStr );


	// look up the vote detail
    auto it = voteInfo.find( vote );
    if ( it == voteInfo.end()
        || ( team == TEAM_NONE && it->second.type == V_TEAM   )
        || ( team != TEAM_NONE && it->second.type == V_PUBLIC ) )
	{
		bool added = false;

		trap_SendServerCommand( ent - g_entities, "print_tr \"" N_("Invalid vote string") "\"" );
		trap_SendServerCommand( ent - g_entities, va( "print_tr %s", team == TEAM_NONE ? QQ( N_("Valid vote commands are: ") ) :
			QQ( N_("Valid team-vote commands are: ") ) ) );
		cmd[0] = '\0';

		Q_strcat( cmd, sizeof( cmd ), "print \"" );

		for ( const auto& it : voteInfo )
		{
            const VoteDefinition& vi = it.second;
			if ( ( team == TEAM_NONE && vi.type != V_TEAM   ) ||
			     ( team != TEAM_NONE && vi.type != V_PUBLIC ) )
			{
				if ( !vi.percentage || vi.percentage->Get() > 0 )
				{
					Q_strcat( cmd, sizeof( cmd ), va( "%s%s", added ? ", " : "", it.first.c_str() ) );
					added = true;
				}
			}
		}

		Q_strcat( cmd, sizeof( cmd ), "\"" );
		trap_SendServerCommand( ent - g_entities, cmd );

		return;
	}
    const VoteDefinition& vi = it->second;

	if ( g_voteLimit.Get() > 0 &&
	     ent->client->pers.namelog->voteCount >= g_voteLimit.Get() &&
	     !G_admin_permission( ent, ADMF_NO_VOTE_LIMIT ) )
	{
		trap_SendServerCommand( ent - g_entities, va(
		                          "print_tr %s %s %d", QQ( N_("$1$: you have already called the maximum number of votes ($2$)") ),
		                          cmd, g_voteLimit.Get() ) );
		return;
	}

	int voteThreshold = vi.percentage ?
		vi.percentage->Get() : 50;
	if ( voteThreshold <= 0 || isDisabledVoteType( vote.c_str() ) )
	{
		trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s", QQ( N_("'$1$' votes have been disabled") ), vote.c_str() ) );
		return;
	}
	if ( voteThreshold > 100)
	{
		voteThreshold = 100;
	}

	level.team[ team ].quorum = vi.quorum;
	level.team[ team ].voteDelay = 0;
	level.team[ team ].voteThreshold = voteThreshold;

	switch ( vi.special )
	{
	case VOTE_BEFORE:
		if ( level.numConnectedPlayers > 1 && level.matchTime >= ( vi.specialCvar->Get() * 60000 ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %d", QQ( N_("'$1$' votes are not allowed once $2$ minutes have passed") ), vote.c_str(), vi.specialCvar->Get() ) );
			return;
		}

		break;

	case VOTE_AFTER:
		if ( level.numConnectedPlayers > 1 && level.matchTime < ( vi.specialCvar->Get() * 60000 ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %d", QQ( N_("'$1$' votes are not allowed until $2$ minutes have passed") ), vote.c_str(), vi.specialCvar->Get() ) );
			return;
		}

		break;

	case VOTE_REMAIN:
		if ( !level.timelimit || level.matchTime < ( level.timelimit - vi.specialCvar->Get() / 2 ) * 60000 )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %d", QQ( N_("'$1$' votes are only allowed with less than $2$ minutes remaining") ),
			                            vote.c_str(), vi.specialCvar->Get() / 2 ) );
			return;
		}

		break;

	default:;
	}

	// Get argument and reason, if needed
	arg[0] = '\0';
	reason[0] = '\0';

	if( vi.target != T_NONE )
	{
		trap_Argv( 2, arg, sizeof( arg ) );
	}

	if( vi.reasonNeeded != qtrinary::qno )
	{
		char *creason = ConcatArgs( vi.target != T_NONE ? 3 : 2 );
		Color::StripColors( creason, reason, sizeof( reason ) );
	}

	if ( vi.target == T_PLAYER )
	{
		char err[ MAX_STRING_CHARS ];

		if ( !arg[ 0 ] )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: no target") ), cmd ) );
			return;
		}

		// with a little extra work only players from the right team are considered
		clientNum = G_ClientNumberFromString( arg, err, sizeof( err ) );

		if ( clientNum == -1 )
		{
			ADMP( va( "%s %s %s", QQ( "$1$: $2t$" ), cmd, Quote( err ) ) );
			return;
		}

		Color::StripColors( level.clients[ clientNum ].pers.netname, name, sizeof( name ) );
		id = level.clients[ clientNum ].pers.namelog->id;

		if ( g_entities[clientNum].r.svFlags & SVF_BOT )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player is a bot") ), cmd ) );
			return;
		}

		if ( vi.adminImmune && G_admin_permission( g_entities + clientNum, ADMF_IMMUNITY ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: admin is immune") ), cmd ) );
			G_AdminMessage( nullptr,
			                va( "^7%s^3 attempted %s %s"
			                    " on immune admin ^7%s"
			                    " ^3for: %s",
			                    ent->client->pers.netname, cmd, vote.c_str(),
			                    g_entities[ clientNum ].client->pers.netname,
			                    reason[ 0 ] ? reason : "no reason" ) );
			return;
		}

		if ( level.clients[ clientNum ].pers.localClient )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: admin is immune") ), cmd ) );
			return;
		}

		if ( team != TEAM_NONE &&
			 ent->client->pers.team != level.clients[ clientNum ].pers.team )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player is not on your team") ), cmd ) );
			return;
		}
	}

	if ( vi.reasonNeeded == qtrinary::qyes && !reason[ 0 ] &&
	     !( vi.adminImmune && G_admin_permission( ent, ADMF_UNACCOUNTABLE ) ) &&
	     !( vi.reasonFlag && vi.reasonFlag->Get() ) )
	{
		trap_SendServerCommand( ent - g_entities,
		                        va( "print_tr %s %s", QQ( N_("$1$: You must provide a reason") ), cmd ) );
		return;
	}



	if ( vote == "kick" )
    {
		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "ban %s 1s%s %s^* called vote kick (%s^*)", level.clients[ clientNum ].pers.ip.str,
		             Cmd::Escape( g_adminTempBan.Get() ).c_str(), Cmd::Escape( ent->client->pers.netname ).c_str(), Cmd::Escape( reason ).c_str() );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ), N_("Kick player '%s'"), name );
    }
    else if ( vote == "spectate" )
    {
		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "speclock %d 1s%s", clientNum, Cmd::Escape( g_adminTempBan.Get() ).c_str() );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             N_("Move player '%s' to spectators"), name );
    }
    else if ( vote == "kickbots" )
    {
		for ( i = 0; i < MAX_CLIENTS; ++i )
		{
			if ( g_entities[i].r.svFlags & SVF_BOT &&
			     g_entities[i].client->pers.team != TEAM_NONE )
			{
				break;
			}
		}

		if ( i == MAX_CLIENTS )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: there are no active bots") ), cmd ) );
			return;
		}

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ), "bot del all" );
		Com_sprintf( level.team[ team ].voteDisplayString, sizeof( level.team[ team ].voteDisplayString ), N_("Remove all bots") );
    }
    else if ( vote == "fillbots")
    {
        int num = 0;
        if ( !botFillVoteParseArg( num, arg ) )
        {
            trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s %s", QQ( N_("$1$: number must be non-negative and smaller than $2$") ), cmd, std::to_string( g_maxVoteFillBots.Get() + 1 ).c_str() ) );
            return;
        }

        Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ), "bot fill %d", num );
        Com_sprintf( level.team[ team ].voteDisplayString, sizeof( level.team[ team ].voteDisplayString ), N_("Fill each team with bots to %d"), num );

    }
    else if ( vote == "fillbots_humans" )
    {
        int num = 0;
        if ( !botFillVoteParseArg( num, arg ) )
        {
            trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s %s", QQ( N_("$1$: number must be non-negative and smaller than $2$") ), cmd, std::to_string( g_maxVoteFillBots.Get() + 1 ).c_str() ) );
            return;
        }

        Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ), "bot fill %d humans", num );
        Com_sprintf( level.team[ team ].voteDisplayString, sizeof( level.team[ team ].voteDisplayString ), N_("Fill only humans with bots to %d"), num );
    }
	else if ( vote == "fillbots_aliens" )
    {
        int num = 0;
        if ( !botFillVoteParseArg( num, arg ) )
        {
            trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s %s", QQ( N_("$1$: number must be non-negative and smaller than $2$") ), cmd, std::to_string( g_maxVoteFillBots.Get() + 1 ).c_str() ) );
            return;
        }

        Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ), "bot fill %d aliens", num );
        Com_sprintf( level.team[ team ].voteDisplayString, sizeof( level.team[ team ].voteDisplayString ), N_("Fill only aliens with bots to %d"), num );
    }
	else if ( vote == "mute" )
    {
		if ( level.clients[ clientNum ].pers.namelog->muted )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player is already muted") ), cmd ) );
			return;
		}

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "mute %d", id );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             N_("Mute player '%s'"), name );
    }
	else if ( vote == "mute" )
    {
		if ( !level.clients[ clientNum ].pers.namelog->muted )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player is not currently muted") ), cmd ) );
			return;
		}

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "unmute %d", id );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             N_("Unmute player '%s'"), name );
    }
	else if ( vote == "denybuild" )
    {
		if ( level.clients[ clientNum ].pers.namelog->denyBuild )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player already lost building rights") ), cmd ) );
			return;
		}

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "denybuild %d", id );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             "Take away building rights from '%s'", name );
    }
	else if ( vote == "allowbuild" )
    {
		if ( !level.clients[ clientNum ].pers.namelog->denyBuild )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s", QQ( N_("$1$: player already has building rights") ), cmd ) );
			return;
		}

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "allowbuild %d", id );
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             "Allow '%s' to build", name );
    }
	else if ( vote == "extend" )
    {
		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "time %i", level.timelimit + g_extendVotesTime.Get() );
		Com_sprintf( level.team[ team ].voteDisplayString, sizeof( level.team[ team ].voteDisplayString ),
		             "Extend the timelimit by %d minutes", g_extendVotesTime.Get() );
    }
	else if ( vote == "admitdefeat" )
    {
		level.team[ team ].voteDelay = 3000;

		Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
		             "admitdefeat %d", team );
		strcpy( level.team[ team ].voteDisplayString, "Admit Defeat" );
    }
	else if ( vote == "draw" )
    {
		level.team[ team ].voteDelay = 3000;
		strcpy( level.team[ team ].voteString, "evacuation" );
		strcpy( level.team[ team ].voteDisplayString, "End match in a draw" );
    }
	else if ( vote == "map_restart" )
    {
		strcpy( level.team[ team ].voteString, vote.c_str() );
		strcpy( level.team[ team ].voteDisplayString, "Restart current map" );
		// map_restart comes with a default delay
    }
	else if ( vote == "map" )
    {
		if ( !G_MapExists( arg ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %s", QQ( N_("$1$: 'maps/$2$.bsp' could not be found on the server") ),
			                            cmd, Quote( arg ) ) );
			return;
		}

		level.team[ team ].voteDelay = 3000;

		if ( *reason ) // layout?
		{
			Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
			             "%s %s %s", G_NextMapCommand().c_str(), Cmd::Escape(arg).c_str(), Cmd::Escape(reason).c_str());
			Com_sprintf( level.team[ team ].voteDisplayString,
			             sizeof( level.team[ team ].voteDisplayString ),
			             "Change to map '%s' layout '%s'", arg, reason );
		}
		else
		{
			Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
			             "%s %s", G_NextMapCommand().c_str(), Cmd::Escape(arg).c_str());
			Com_sprintf( level.team[ team ].voteDisplayString,
			             sizeof( level.team[ team ].voteDisplayString ),
			             "Change to map '%s'", arg );
		}

		reason[0] = '\0'; // nullify since we've used it here...
    }
    else if ( vote == "layout" )
    {
        char map[ 64 ];

        trap_Cvar_VariableStringBuffer( "mapname", map, sizeof( map ) );

        if ( Q_stricmp( arg, S_BUILTIN_LAYOUT ) &&
                !G_LayoutExists( map, arg ) )
        {
            trap_SendServerCommand( ent - g_entities, va( "print_tr %s %s", QQ( N_("callvote: "
                                    "layout '$1$' could not be found on the server") ), Quote( arg ) ) );
            return;
        }

        Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ), "restart %s", Cmd::Escape( arg ).c_str() );
        Com_sprintf( level.team[ team ].voteDisplayString,
                        sizeof( level.team[ team ].voteDisplayString ), "Change to map layout '%s'", arg );
    }
    else if ( vote == "nextmap" )
    {
		if ( G_MapExists( g_nextMap.Get().c_str() ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %s", QQ( N_("$1$: the next map is already set to '$2$'") ),
			                            cmd, Quote( g_nextMap.Get().c_str() ) ) );
			return;
		}

		if ( !G_MapExists( arg ) )
		{
			trap_SendServerCommand( ent - g_entities,
			                        va( "print_tr %s %s %s", QQ( N_("$1$: 'maps/$2$.bsp' could not be found on the server") ),
			                            cmd, Quote( arg ) ) );
			return;
		}

		if ( *reason ) // layout?
		{
			Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
			             "set g_nextMap %s; set g_nextMapLayouts %s", Cmd::Escape( arg ).c_str(), Cmd::Escape( reason ).c_str() );
			Com_sprintf( level.team[ team ].voteDisplayString,
			             sizeof( level.team[ team ].voteDisplayString ),
			             "Set the next map to '%s' layout '%s'", arg, reason );
		}
		else
		{
			Com_sprintf( level.team[ team ].voteString, sizeof( level.team[ team ].voteString ),
			             "set g_nextMap %s; set g_nextMapLayouts \"\"", Cmd::Escape( arg ).c_str() );
			Com_sprintf( level.team[ team ].voteDisplayString,
			             sizeof( level.team[ team ].voteDisplayString ),
			             "Set the next map to '%s'", arg );
		}

		reason[0] = '\0'; // nullify since we've used it here...
    }
	else if ( vote == "poll" )
    {
		level.team[ team ].voteString[0] = '\0';
		Com_sprintf( level.team[ team ].voteDisplayString,
		             sizeof( level.team[ team ].voteDisplayString ),
		             "(poll) %s", reason );
		reason[0] = '\0'; // nullify since we've used it here...
    }

	// Append the vote reason (if applicable)
	if ( reason[ 0 ] )
	{
		Q_strcat( level.team[ team ].voteDisplayString,
		          sizeof( level.team[ team ].voteDisplayString ), va( " for '%s'", reason ) );
	}

	G_LogPrintf( "%s: %d \"%s^*\": %s",
	             team == TEAM_NONE ? "CallVote" : "CallTeamVote",
	             ( int )( ent - g_entities ), ent->client->pers.netname, level.team[ team ].voteString );

	if ( team == TEAM_NONE )
	{
		trap_SendServerCommand( -1, va( "print_tr %s %s %s", QQ( N_("$1$^* called a vote: $2$") ),
		                                Quote( ent->client->pers.netname ), Quote( level.team[ team ].voteDisplayString ) ) );
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
					trap_SendServerCommand( i, va( "print_tr %s %s %s", QQ( N_("$1$^* called a team vote: $2t$") ),
					                               Quote( ent->client->pers.netname ), Quote( level.team[ team ].voteDisplayString ) ) );
				}
				else if ( G_admin_permission( &g_entities[ i ], ADMF_ADMINCHAT ) )
				{
					trap_SendServerCommand( i, va( "chat -1 %d ^3%s\"^3 called a team vote (%ss): \"%s",
					                               SAY_ADMINS, Quote( ent->client->pers.netname ), BG_TeamName( team ),
					                               Quote( level.team[ team ].voteDisplayString ) ) );
				}
			}
		}
	}

	Color::StripColors( ent->client->pers.netname, caller, sizeof( caller ) );

	level.team[ team ].voteTime = level.time;
	trap_SetConfigstring( CS_VOTE_TIME + team,
	                      va( "%d", level.team[ team ].voteTime ) );
	trap_SetConfigstring( CS_VOTE_STRING + team,
	                      level.team[ team ].voteDisplayString );
	trap_SetConfigstring( CS_VOTE_CALLER + team,
	                      caller );

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
	else if ( !Q_strnicmp( level.team[ team ].voteString, "map", 3 ) )
	{
		G_MapLog_Result( 'm' );
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
	float    votePassThreshold = ( float ) level.team[ team ].voteThreshold / 100.0f;
	bool pass = false;
	bool quorum = true;
	char     *cmd;

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
	     ( level.team[ team ].voteYes + level.team[ team ].voteNo == level.team[ team ].numPlayers ) )
	{
		pass = ( level.team[ team ].voteYes &&
		         ( float ) level.team[ team ].voteYes / ( ( float ) level.team[ team ].voteYes + ( float ) level.team[ team ].voteNo ) > votePassThreshold );
	}
	else
	{
		if ( ( float ) level.team[ team ].voteYes >
		     ( float ) level.team[ team ].numPlayers * votePassThreshold )
		{
			pass = true;
		}
		else if ( ( float ) level.team[ team ].voteNo <=
		          ( float ) level.team[ team ].numPlayers * ( 1.0f - votePassThreshold ) )
		{
			return;
		}
	}

	// If quorum is required, check whether at least half of who could vote did
	if ( level.team[ team ].quorum && level.team[ team ].voted < floor( powf( level.team[ team ].numPlayers, 0.6 ) ) )
	{
		quorum = false;
	}

	if ( pass && quorum )
	{
		level.team[ team ].voteExecuteTime = level.time + level.team[ team ].voteDelay;
	}

	G_LogPrintf( "EndVote: %s %s %d %d %d %d",
	             team == TEAM_NONE ? "global" : BG_TeamName( team ),
	             pass ? "pass" : "fail",
	             level.team[ team ].voteYes, level.team[ team ].voteNo, level.team[ team ].numPlayers, level.team[ team ].voted );

	if ( !quorum )
	{
		cmd = va( "print_tr %s %d %d", ( team == TEAM_NONE ) ? QQ( N_("Vote failed ($1$ of $2$; quorum not reached)") ) : QQ( N_("Team vote failed ($1$ of $2$; quorum not reached)") ),
		            level.team[ team ].voteYes + level.team[ team ].voteNo, level.team[ team ].numPlayers );
	}
	else if ( pass )
	{
		cmd = va( "print_tr %s %d %d", ( team == TEAM_NONE ) ? QQ( N_("Vote passed ($1$ — $2$)") ) : QQ( N_("Team vote passed ($1$ — $2$)") ),
		            level.team[ team ].voteYes, level.team[ team ].voteNo );
	}
	else
	{
		cmd = va( "print_tr %s %d %d %.0f", ( team == TEAM_NONE ) ? QQ( N_("Vote failed ($1$ — $2$; $3$% needed)") ) : QQ( N_("Team vote failed ($1$ — $2$; $3$% needed)") ),
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
void G_Vote( gentity_t *ent, team_t team, bool voting )
{
	if ( !level.team[ team ].voteTime )
	{
		return;
	}

	if ( voting && (ent->client->pers.voted & ( 1 << team )) )
	{
		return;
	}

	if ( !voting && !( ent->client->pers.voted & ( 1 << team ) ) )
	{
		return;
	}

	ent->client->pers.voted |= 1 << team;

	//TODO maybe refactor vote no/yes in one and only one variable and divide the NLOC by 2 ?

	if ( voting )
	{
		level.team[ team ].voted++;
	}
	else
	{
		level.team[ team ].voted--;
	}

	if ( ent->client->pers.voteYes & ( 1 << team ) )
	{
		if ( voting )
		{
			level.team[ team ].voteYes++;
		}
		else
		{
			level.team[ team ].voteYes--;
		}

		trap_SetConfigstring( CS_VOTE_YES + team,
		                      va( "%d", level.team[ team ].voteYes ) );
	}

	if ( ent->client->pers.voteNo & ( 1 << team ) )
	{
		if ( voting )
		{
			level.team[ team ].voteNo++;
		}
		else
		{
			level.team[ team ].voteNo--;
		}

		trap_SetConfigstring( CS_VOTE_NO + team,
		                      va( "%d", level.team[ team ].voteNo ) );
	}
}
