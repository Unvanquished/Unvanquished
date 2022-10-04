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
	V_ANY,
	V_TEAM,
	V_PUBLIC,
};

enum VoteTarget
{
	T_NONE,
	T_PLAYER,
	T_OTHER
};

enum VoteOptions
{
	VOTE_ALWAYS,   // default
	VOTE_BEFORE,   // within the first N minutes
	VOTE_AFTER,    // not within the first N minutes
	VOTE_REMAIN,   // within N/2 minutes before SD
	VOTE_NO_AUTO,  // don't automatically vote 'yes'
};

using VoteHandler =
	std::function<bool( gentity_t* ent, team_t team, std::string& cmd, std::string& arg,
                        std::string& reason, std::string& name, int clientNum, int id )>;

struct VoteDefinition
{
	bool stopOnIntermission;
	int type;
	int target;
	bool adminImmune;  // from needing a reason and from being the target
	bool quorum;
	qtrinary reasonNeeded;
	Cvar::Cvar<int>* percentage;
	int special;
	Cvar::Cvar<int>* specialCvar;
	Cvar::Cvar<bool>*
		reasonFlag;  // where a reason requirement is configurable (reasonNeeded must be true)
	VoteHandler handler;
};

void G_HandleVote( gentity_t* ent );
bool G_AddCustomVote( std::string vote, VoteDefinition def, std::string voteTemplate,
                      std::string displayTemplate );
bool ParseVoteType( Str::StringRef s, VoteType* type );
Str::StringRef VoteTypeString( VoteType type );
bool ParseVoteTarget( Str::StringRef s, VoteTarget* type );
Str::StringRef VoteTargetString( VoteTarget type );
bool ParseVoteOptions( Str::StringRef s, VoteOptions* type );
Str::StringRef VoteOptionsString( VoteOptions type );
