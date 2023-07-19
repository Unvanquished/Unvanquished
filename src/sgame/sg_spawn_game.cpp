/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Unvanquished Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Unvanquished
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "sg_local.h"
#include "sg_spawn.h"
#include "Entities.h"

/*
=================================================================================

game_score

=================================================================================
*/

static void game_score_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	if ( !activator )
	{
		return;
	}

	G_AddCreditsToScore( activator, self->mapEntity.config.amount );
}

void SP_game_score( gentity_t *self )
{
	if ( !self->mapEntity.config.amount )
	{
		if( G_SpawnInt( "count", "0", &self->mapEntity.config.amount) )
		{
			G_WarnAboutDeprecatedEntityField( self, "amount", "count", ENT_V_RENAMED );
		}
		else
		{
			self->mapEntity.config.amount = 1;
		}
	}

	self->act = game_score_act;
}

/*
=================================================================================

game_end

=================================================================================
*/
static void game_end_act( gentity_t *self, gentity_t*, gentity_t* )
{
	if ( level.unconditionalWin == TEAM_NONE ) // only if not yet triggered
	{
		level.unconditionalWin = self->mapEntity.conditions.team;
	}
}

void SP_game_end( gentity_t *self )
{
	if(!Q_stricmp(self->classname, "target_human_win"))
	{
		self->mapEntity.conditions.team = TEAM_HUMANS;
	}
	else if(!Q_stricmp(self->classname, "target_alien_win"))
	{
		self->mapEntity.conditions.team = TEAM_ALIENS;
	}

	self->act = game_end_act;
}

/*
=================================================================================

game_funds

=================================================================================
*/

static void game_funds_act( gentity_t *self, gentity_t*, gentity_t *activator )
{
	if( !activator )
	{
		return;
	}

	G_AddCreditToClient( activator->client, self->mapEntity.amount, true );
}

static void game_funds_reset( gentity_t *self )
{
	G_ResetIntField( &self->mapEntity.amount, false, self->mapEntity.config.amount, self->mapEntity.eclass->config.amount, 0);
}

void SP_game_funds( gentity_t *self )
{
	self->act = game_funds_act;
	self->reset = game_funds_reset;
}


/*
=================================================================================

game_kill

=================================================================================
*/
static void game_kill_act( gentity_t*, gentity_t*, gentity_t *activator )
{
	if ( !activator )
	{
		return;
	}

	Entities::Kill(activator, MOD_TELEFRAG);
}

void SP_game_kill( gentity_t *self )
{
	self->act = game_kill_act;
}
