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

#include "cg_local.h"

static float CG_Rocket_GetLoadProgress()
{
	if ( cg.loadingNavmesh )
	{
		return cg.navmeshLoadingFraction;
	}

	return (
		cg.loadingFraction
		+ cg.mediaLoadingFraction
		+ cg.buildableLoadingFraction
		+ cg.characterLoadingFraction
		) / 4.0;
}

static float CG_Rocket_GetBuildTimerProgress()
{
	static int misc = 0;
	static int max = 0;
	playerState_t *ps = &cg.snap->ps;
	weapon_t weapon = BG_GetPlayerWeapon( ps );

	// Not building anything
	if ( weapon != WP_HBUILD && weapon != WP_ABUILD && weapon != WP_ABUILD2 )
	{
		return 0;
	}

	// Building something new. Note max value.
	if ( ps->stats[ STAT_MISC ] > 0 && misc <= 0 )
	{
		max = ps->stats[ STAT_MISC ];
	}

	// Avoid division by zero.
	if ( max == 0 )
	{
		return 0;
	}

	misc = ps->stats[ STAT_MISC ];

	return ( float ) misc / ( float ) max;
}

static float CG_Rocket_GetStaminaProgress()
{
	playerState_t *ps = &cg.snap->ps;
	float         stamina = ps->stats[ STAT_STAMINA ];

	return ( stamina / ( float ) STAMINA_MAX );
}

static float CG_Rocket_GetPoisonProgress()
{
	static int time = -1;

	if ( cg.snap->ps.stats[ STAT_STATE ] & SS_BOOSTED )
	{
		if ( time == -1 || cg.snap->ps.stats[ STAT_STATE ] & SS_BOOSTEDNEW )
		{
			time = cg.time;
		}

		return 1 - ( ( ( float )cg.time - time ) / BOOST_TIME );
	}

	else
	{
		time = -1;
		return 0;
	}

}

static float CG_Rocket_GetPlayerHealthProgress()
{
	playerState_t *ps = &cg.snap->ps;

	return ( float )ps->stats[ STAT_HEALTH ] / ( float )BG_Class( ps->stats[ STAT_CLASS ] )->health;
}

static float CG_Rocket_GetPlayerAmmoProgress()
{
	if ( cg.snap->ps.weaponstate == WEAPON_RELOADING )
	{
		float maxDelay;
		playerState_t *ps = &cg.snap->ps;
		centity_t *cent = &cg_entities[ ps->clientNum ];

		maxDelay = ( float ) BG_Weapon( cent->currentState.weapon )->reloadTime;
		return ( maxDelay - ( float ) ps->weaponTime ) / maxDelay;

	}

	else
	{
		int      maxAmmo;
		weapon_t weapon;

		weapon = BG_PrimaryWeapon( cg.snap->ps.stats );
		maxAmmo = BG_Weapon( weapon )->maxAmmo;

		if ( maxAmmo <= 0 )
		{
			return 0;
		}

		return ( float )cg.snap->ps.ammo / ( float )maxAmmo;
	}
}

float CG_Rocket_FuelProgress()
{
	int   fuel;

	if ( !BG_InventoryContainsUpgrade( UP_JETPACK, cg.snap->ps.stats ) )
	{
		return 0;
	}

	fuel     = cg.snap->ps.stats[ STAT_FUEL ];
	return ( float )fuel / ( float )JETPACK_FUEL_MAX;
}

float CG_Rocket_DownloadProgress()
{
	float count = trap_Cvar_VariableValue("cl_downloadCount");
	float total = trap_Cvar_VariableValue("cl_downloadSize");
	if (total > 0 && count >= 0 && count <= total )
	{
		return count / total;
	}
	return 0.0f;
}


struct progressBarCmd_t
{
	const char *command;
	float( *get )();
	rocketElementType_t type;
};

static const progressBarCmd_t progressBarCmdList[] =
{
	{ "ammo", &CG_Rocket_GetPlayerAmmoProgress, ELEMENT_HUMANS },
	{ "btimer", &CG_Rocket_GetBuildTimerProgress, ELEMENT_BOTH },
	{ "charge", &CG_ChargeProgress, ELEMENT_BOTH },
	{ "download", &CG_Rocket_DownloadProgress, ELEMENT_ALL },
	{ "fuel", &CG_Rocket_FuelProgress, ELEMENT_HUMANS },
	{ "health", &CG_Rocket_GetPlayerHealthProgress, ELEMENT_BOTH },
	{ "overall", &CG_Rocket_GetLoadProgress, ELEMENT_LOADING },
	{ "poison", &CG_Rocket_GetPoisonProgress, ELEMENT_ALIENS },
	{ "stamina", &CG_Rocket_GetStaminaProgress, ELEMENT_HUMANS },
};

static const size_t progressBarCmdListCount = ARRAY_LEN( progressBarCmdList );

static int progressBarCmdCmp( const void *a, const void *b )
{
	return Q_stricmp( ( const char * ) a, ( ( progressBarCmd_t * ) b )->command );
}

float CG_Rocket_ProgressBarValue( Str::StringRef name )
{
	progressBarCmd_t *cmd;

	// Get the progressBar command
	cmd = ( progressBarCmd_t * ) bsearch( name.c_str(), progressBarCmdList, progressBarCmdListCount, sizeof( progressBarCmd_t ), progressBarCmdCmp );

	if ( cmd && CG_Rocket_IsCommandAllowed( cmd->type ) )
	{
		return cmd->get();
	}

	return 0;
}
