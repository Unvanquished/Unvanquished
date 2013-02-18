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

#include "g_local.h"
#include "g_spawn.h"

//==========================================================

/*QUAKED target_score (1 0 0) (-8 -8 -8) (8 8 8)
"count" number of points to add, default 1

The activator is given this many points.
*/
void game_score_use( gentity_t *ent, gentity_t *other, gentity_t *activator )
{
	if ( !activator )
	{
		return;
	}

	AddScore( activator, ent->count );
}

void SP_game_score( gentity_t *ent )
{
	if ( !ent->count )
	{
		ent->count = 1;
	}

	ent->use = game_score_use;
}

/*
===============
target_win_use
===============
*/
void game_end_use( gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( level.unconditionalWin == TEAM_NONE ) // only if not yet triggered
	{
		level.unconditionalWin = self->conditions.team;
	}
}

/*
===============
SP_target_alien_win
===============
*/
void SP_target_alien_win( gentity_t *self )
{
	self->conditions.team = TEAM_ALIENS;
	self->use = game_end_use;
}

/*
===============
SP_target_human_win
===============
*/
void SP_target_human_win( gentity_t *self )
{
	self->conditions.team = TEAM_HUMANS;
	self->use = game_end_use;
}

/*
===============
SP_target_win
===============
*/
void SP_game_end( gentity_t *self )
{
	G_SpawnInt( "team", "0", ( int * ) &self->conditions.team );
	self->use = game_end_use;
}
