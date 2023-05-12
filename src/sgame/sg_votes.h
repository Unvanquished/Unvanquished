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
#include "sg_local.h"

enum VoteType
{
	V_ANY,     // Vote can be called either as a team vote or public vote
	V_TEAM,    // Team vote only
	V_PUBLIC,  // Public vote only
};

enum VoteTarget
{
	T_NONE,    // No arg required
	T_PLAYER,  // Arg is a player
	T_OTHER,   // Arg is a random string
};

enum VoteOptions
{
	VOTE_ALWAYS,   // default
	VOTE_BEFORE,   // within the first N minutes
	VOTE_AFTER,    // not within the first N minutes
	VOTE_REMAIN,   // within N/2 minutes before the timelimit
	VOTE_NO_AUTO,  // don't automatically vote 'yes'
	VOTE_SD_NOTSOON, // Sudden Death soon
};


// Function definition on handling a vote.
// Args:
//   ent: vote caller
//   team: team for which the vote was called if a teamvote, TEAM_NONE if public vote
//   cmd: callvote or callteamvote
//   arg: vote argument if VoteTarget is not T_NONE. Will be empty if VoteTarget is T_NONE
//   reason: reason if reasonNeeded is qyes or qmaybe. Will be empty if qno and maybe empty if qmaybe.
//   name: Sanitized target name if VoteTarget is T_PLAYER. nullptr otherwise.
//   clientNum: Vote target is VoteTarget is T_PLAYER. -1 otherwise.
//   id: Namelog id if VoteTarget is T_PLAYER. -1 otherwise.
using VoteHandler =
	std::function<bool( gentity_t* ent, team_t team, std::string& cmd, std::string& arg,
                        std::string& reason, std::string& name, int clientNum, int id )>;

struct VoteDefinition
{
	bool stopOnIntermission;  // Don't allow votes of this kind during intermission
	VoteType type;
	VoteTarget target;
	bool adminImmune;  // from needing a reason and from being the target
	bool quorum;       // At least half of eligible voters are required to vote in order to pass
	qtrinary reasonNeeded;  // Whether a reason is required. NOTE: This field is also abused for
	                        // multiarg votes
	Cvar::Cvar<int>* percentage;  // Cvar which holds at what percentage this vote passes
	VoteOptions special;          //  Controls vote options of when the vote can be called
	// Cvar argument for when vote can be called.
	// Only applicable when special is [VOTE_BEFORE, VOTE_AFTER, VOTE_REMAIN].
	Cvar::Cvar<int>* specialCvar;
	// where a reason requirement is configurable (reasonNeeded must be true)
	Cvar::Cvar<bool>* reasonFlag;
	// Vote function that sets the vote text and the vote display
	VoteHandler handler;
};

// Handle a vote. Expected to already have args pushed to the arg stack and called from sg_cmds.cpp
void G_HandleVote( gentity_t* ent );
// Add a custom vote. Returns false if the vote was not added, which can happen if a vote of the
// same name already exists or if the vote argument is empty. voteTemplate is the command that is
// run if the vote passes displayTemplate is the human friendly text shown to players when deciding
// on the vote Several special subsititions are allowed in the voteTemplate and displayTemplate:
// $team$ -- plural team name (humans, aliens)
// $arg$ -- The argument if VoteTarget is not T_NONE. If VoteTarget is T_PLAYER, arg will be
//          will be whatever the caller chose to enter to match the player (ie, partial name, slot
//          number, etc)
// $reason$ -- The reason for the vote. Will be empty is reasonNeeded is set to qno or qmaybe and no
//             reason was passed in.
// $slot$  -- The target slot $namelogId$ -- The target namelog ID. You
//            probably want the slot, not this.
//
// See dist/configs/game/admin.dat for examples of votes.
bool G_AddCustomVote( std::string vote, VoteDefinition def, std::string voteTemplate,
                      std::string displayTemplate );
bool G_ParseVoteType( Str::StringRef s, VoteType* type );
Str::StringRef G_VoteTypeString( VoteType type );
bool G_ParseVoteTarget( Str::StringRef s, VoteTarget* type );
Str::StringRef G_VoteTargetString( VoteTarget type );
bool G_ParseVoteOptions( Str::StringRef s, VoteOptions* type );
Str::StringRef G_VoteOptionsString( VoteOptions type );
bool G_ParseReasonNeeded( Str::StringRef s, qtrinary* tri );
Str::StringRef G_ReasonNeededString( qtrinary tri );
