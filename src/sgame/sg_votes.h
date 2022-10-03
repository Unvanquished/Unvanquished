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

enum VoteType {
	V_TEAM, V_PUBLIC, V_ANY
};
enum VoteTarget{
	T_NONE, T_PLAYER, T_OTHER
};
enum VoteOptions{
	VOTE_ALWAYS, // default
	VOTE_BEFORE, // within the first N minutes
	VOTE_AFTER,  // not within the first N minutes
	VOTE_REMAIN, // within N/2 minutes before SD
	VOTE_NO_AUTO,// don't automatically vote 'yes'
};

struct VoteDefinition {
    const char     *name;
	bool        stopOnIntermission;
	int             type;
	int             target;
	bool        adminImmune; // from needing a reason and from being the target
	bool        quorum;
	qtrinary        reasonNeeded;
	Cvar::Cvar<int> *percentage;
	int             special;
	Cvar::Cvar<int> *specialCvar;
	Cvar::Cvar<bool> *reasonFlag; // where a reason requirement is configurable (reasonNeeded must be true)
};


void G_HandleVote( gentity_t *ent );
