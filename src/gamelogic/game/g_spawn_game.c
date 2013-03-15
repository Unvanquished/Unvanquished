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

/*
=================================================================================

game_score

=================================================================================
*/

void game_score_act( gentityCallDefinition_t* target, gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( !activator )
	{
		return;
	}

	AddScore( activator, self->config.amount );
}

void SP_game_score( gentity_t *self )
{
	if ( !self->config.amount )
	{
		if( G_SpawnInt( "count", "0", &self->config.amount) )
		{
			G_WarnAboutDeprecatedEntityField( self, "amount", "count", ENT_V_RENAMED );
		}
		else
		{
			self->config.amount = 1;
		}
	}

	self->act = game_score_act;
}

/*
=================================================================================

game_end

=================================================================================
*/
void game_end_act( gentityCallDefinition_t* target, gentity_t *self, gentity_t *other, gentity_t *activator )
{
	if ( level.unconditionalWin == TEAM_NONE ) // only if not yet triggered
	{
		level.unconditionalWin = self->conditions.team;
	}
}

void SP_target_alien_win( gentity_t *self )
{
	self->conditions.team = TEAM_ALIENS;
	self->act = game_end_act;
}

void SP_target_human_win( gentity_t *self )
{
	self->conditions.team = TEAM_HUMANS;
	self->act = game_end_act;
}

void SP_game_end( gentity_t *self )
{
	self->act = game_end_act;
}
