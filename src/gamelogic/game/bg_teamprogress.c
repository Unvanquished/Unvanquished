/*
===========================================================================

Copyright 2013 Unvanquished Developers

This file is part of Daemon.

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#include "../../engine/qcommon/q_shared.h"
#include "bg_public.h"

#ifdef GAME
#include "g_local.h"
#endif

#ifdef CGAME
#include "../../engine/client/cg_api.h"
#endif

#ifdef UI
#include "../../engine/client/ui_api.h"
#endif

// -----------
// definitions
// -----------

// TODO: Make LOCK_RATIO a (synchronized) cvar
#define LOCK_RATIO      0.8f

#define UNLOCKABLES_CALCULATION_PERIOD 1000

#define NUM_UNLOCKABLES WP_NUM_WEAPONS + UP_NUM_UPGRADES + BA_NUM_BUILDABLES + PCL_NUM_CLASSES

typedef enum
{
	UNLT_WEAPON,
	UNLT_UPGRADE,
	UNLT_BUILDABLE,
	UNLT_CLASS,
	UNLT_NUM_UNLOCKABLETYPES
} unlockableType_t;

typedef struct unlockable_s
{
	int      type;
	int      num;
	team_t   team;
	qboolean unlocked;
	qboolean statusKnown;
	qboolean threshold;
} unlockable_t;

// ----
// data
// ----

unlockable_t     unlockables[ NUM_UNLOCKABLES ];
int              unlockablesTypeOffset[ UNLT_NUM_UNLOCKABLETYPES ];
unlockableType_t unlockablesType[ NUM_UNLOCKABLES ];
team_t           unlockablesTeamKnowledge;

// -------
// methods
// -------

static const char *UnlockableHumanName( unlockable_t *unlockable )
{
	switch ( unlockable->type )
	{
		case UNLT_WEAPON:    return BG_Weapon( unlockable->num )->humanName;
		case UNLT_UPGRADE:   return BG_Upgrade( unlockable->num )->humanName;
		case UNLT_BUILDABLE: return BG_Buildable( unlockable->num )->humanName;
		case UNLT_CLASS:     return BG_ClassModelConfig( unlockable->num )->humanName;
	}

	Com_Error( ERR_FATAL, "UnlockableHumanName: Unlockable has unknown type" );
	return NULL;
}

static qboolean Disabled( unlockable_t *unlockable )
{
	switch ( unlockable->type )
	{
		case UNLT_WEAPON:    return BG_WeaponDisabled( unlockable->num );
		case UNLT_UPGRADE:   return BG_UpgradeDisabled( unlockable->num );
		case UNLT_BUILDABLE: return BG_BuildableDisabled( unlockable->num );
		case UNLT_CLASS:     return BG_ClassDisabled( unlockable->num );
	}

	Com_Error( ERR_FATAL, "Disabled: Unlockable has unknown type" );
	return qfalse;
}

void BG_InitUnlockackables( void )
{
	memset( unlockables, 0, sizeof( unlockables ) );

	unlockablesTypeOffset[ UNLT_WEAPON ]    = 0;
	unlockablesTypeOffset[ UNLT_UPGRADE ]   = WP_NUM_WEAPONS;
	unlockablesTypeOffset[ UNLT_BUILDABLE ] = unlockablesTypeOffset[ UNLT_UPGRADE ]   + UP_NUM_UPGRADES;
	unlockablesTypeOffset[ UNLT_CLASS ]     = unlockablesTypeOffset[ UNLT_BUILDABLE ] + BA_NUM_BUILDABLES;

	unlockablesTeamKnowledge = TEAM_NONE;

#ifdef GAME
	G_UpdateUnlockables();
#endif
}

#ifdef GAME
void G_UpdateUnlockables( void )
{
	int              itemNum = 0, unlockableNum, unlockThreshold, confidence;
	unlockable_t     *unlockable;
	unlockableType_t unlockableType = 0;
	team_t           team;

	static int nextCalculation = 0;

	if ( level.time < nextCalculation )
	{
		return;
	}

	for ( unlockableNum = 0; unlockableNum < NUM_UNLOCKABLES; unlockableNum++ )
	{
		unlockable = &unlockables[ unlockableNum ];

		// also iterate over item types, itemNum is a per-type counter
		while ( unlockableType < UNLT_NUM_UNLOCKABLETYPES - 1 &&
		        unlockableNum == unlockablesTypeOffset[ unlockableType + 1 ] )
		{
			unlockableType++;
			itemNum = 0;
		}

		switch ( unlockableType )
		{
			case UNLT_WEAPON:
				team            = BG_Weapon( itemNum )->team;
				unlockThreshold = BG_Weapon( itemNum )->unlockThreshold;
				break;

			case UNLT_UPGRADE:
				team            = TEAM_HUMANS;
				unlockThreshold = BG_Upgrade( itemNum )->unlockThreshold;
				break;

			case UNLT_BUILDABLE:
				team            = BG_Buildable( itemNum )->team;
				unlockThreshold = BG_Buildable( itemNum )->unlockThreshold;
				break;

			case UNLT_CLASS:
				team            = TEAM_ALIENS;
				unlockThreshold = BG_Class( itemNum )->unlockThreshold;
				break;

			default:
				Com_Error( ERR_FATAL, "G_UpdateUnlockables: Unknown unlockable type" );
		}

		unlockThreshold = MAX( unlockThreshold, 0 );
		confidence = ( int )level.team[ team ].confidence[ CONFIDENCE_SUM ];

		unlockable->type        = unlockableType;
		unlockable->num         = itemNum;
		unlockable->team        = team;
		unlockable->statusKnown = qtrue;
		unlockable->threshold   = unlockThreshold;

		// calculate the item's locking state
		unlockable->unlocked = (
		    !unlockThreshold || confidence >= unlockThreshold ||
		    ( unlockable->unlocked && confidence >= unlockThreshold * LOCK_RATIO )
		);

		itemNum++;

		/*Com_Printf( "G_UpdateUnlockables: Team %s, Type %s, Item %s, Confidence %d, Threshold %d, "
		            "Unlocked %d, Synchronize %d\n",
		            BG_TeamName( team ), UnlockableTypeName( unlockable ), UnlockableName( unlockable ),
		            confidence, unlockThreshold, unlockable->unlocked, unlockable->synchronize );*/
	}

	// GAME knows about all teams
	unlockablesTeamKnowledge = TEAM_ALL;

	nextCalculation = level.time + UNLOCKABLES_CALCULATION_PERIOD;
}
#endif

#ifdef GAME
int G_ExportUnlockablesToMask( team_t team )
{
	int unlockable, teamUnlockableNum = 0, mask = 0, teamNum;

	// maintain a cache
	static int      cachedMask[ NUM_TEAMS ];
	static qboolean cacheValid[ NUM_TEAMS ];
	static qboolean firstRun = qtrue;
	static int      nextMaskCalculation = 0;

	// cache is invalid on first run
	if ( firstRun )
	{
		firstRun = qfalse;

		for ( teamNum = 0; teamNum < NUM_TEAMS; teamNum++ )
		{
			cacheValid[ NUM_TEAMS ] = qfalse;
		}
	}

	// use cache if it's recent enough
	if ( level.time < nextMaskCalculation && cacheValid[ team ] )
	{
		return cachedMask[ team ];
	}

	// build the mask
	for ( unlockable = 0; unlockable < NUM_UNLOCKABLES; unlockable++ )
	{
		if ( unlockables[ unlockable ].threshold && unlockables[ unlockable ].team == team )
		{
			if ( teamUnlockableNum > 15 ) // 16 bit available for transmission
			{
				Com_Error( ERR_FATAL, "G_ExportUnlockablesToMask: Number of unlockable items for a team exceeded" );
			}

			if ( !unlockables[ unlockable ].statusKnown )
			{
				Com_Error( ERR_FATAL, "G_ExportUnlockablesToMask: Called before G_UpdateUnlockables" );
			}

			if ( unlockables[ unlockable ].unlocked )
			{
				mask |= ( 1 << teamUnlockableNum );
			}

			teamUnlockableNum++;
		}
	}

	// cache the result
	cacheValid[ team ] = qtrue;
	cachedMask[ team ] = mask;
	nextMaskCalculation = level.time + UNLOCKABLES_CALCULATION_PERIOD;

	return mask;
}
#endif

static void InformUnlockableStatusChange( unlockable_t *unlockable, qboolean unlocked )
{
	if ( Disabled( unlockable ) )
	{
		return;
	}

	if ( unlocked )
	{
		Com_Printf( "%s " S_COLOR_GREEN "unlocked\n", UnlockableHumanName( unlockable ) );
	}
	else
	{
		Com_Printf( "%s " S_COLOR_RED   "locked\n",   UnlockableHumanName( unlockable ) );
	}
}

void BG_ImportUnlockablesFromMask( team_t team, int mask )
{
	int              unlockableNum, teamUnlockableNum = 0, itemNum = 0, unlockThreshold, teamNum;
	unlockable_t     *unlockable;
	unlockableType_t unlockableType = 0;
	team_t           currentTeam;
	qboolean         newStatus;

	// maintain a cache to prevent redundant imports
	static qboolean cacheValid = qfalse;
	static team_t   lastMask   = 0;
	static team_t   lastTeam   = TEAM_NONE;

	// just import if cached mask is outdated or team has changed
	if ( cacheValid && team == lastTeam && mask == lastMask )
	{
		return;
	}

	// cache input
	cacheValid = qtrue;
	lastMask   = mask;
	lastTeam   = team;

	for ( unlockableNum = 0; unlockableNum < NUM_UNLOCKABLES; unlockableNum++ )
	{
		unlockable = &unlockables[ unlockableNum ];

		// also iterate over item types, itemNum is a per-type counter
		if ( unlockableType < UNLT_NUM_UNLOCKABLETYPES - 1 &&
		     unlockableNum == unlockablesTypeOffset[ unlockableType + 1 ] )
		{
			unlockableType++;
			itemNum = 0;
		}

		switch ( unlockableType )
		{
			case UNLT_WEAPON:
				currentTeam     = BG_Weapon( itemNum )->team;
				unlockThreshold = BG_Weapon( itemNum )->unlockThreshold;
				break;

			case UNLT_UPGRADE:
				currentTeam     = TEAM_HUMANS;
				unlockThreshold = BG_Upgrade( itemNum )->unlockThreshold;
				break;

			case UNLT_BUILDABLE:
				currentTeam     = BG_Buildable( itemNum )->team;
				unlockThreshold = BG_Buildable( itemNum )->unlockThreshold;
				break;

			case UNLT_CLASS:
				currentTeam     = TEAM_ALIENS;
				unlockThreshold = BG_Class( itemNum )->unlockThreshold;
				break;

			default:
				Com_Error( ERR_FATAL, "BG_ImportUnlockablesFromMask: Unknown unlockable type" );
		}

		unlockThreshold = MAX( unlockThreshold, 0 );

		unlockable->type        = unlockableType;
		unlockable->num         = itemNum;
		unlockable->team        = currentTeam;
		unlockable->threshold   = unlockThreshold;

		// retrieve the item's locking state
		if ( !unlockThreshold )
		{
			unlockable->statusKnown = qtrue;
			unlockable->unlocked    = qtrue;
		}
		else if ( currentTeam == team )
		{
			newStatus = mask & ( 1 << teamUnlockableNum );

#ifdef CGAME
			// inform client on status change
			if ( unlockablesTeamKnowledge == team && unlockable->statusKnown &&
			     unlockable->unlocked != newStatus )
			{
				InformUnlockableStatusChange( unlockable, newStatus );
			}
#endif

			unlockable->statusKnown = qtrue;
			unlockable->unlocked    = newStatus;

			teamUnlockableNum++;
		}
		else
		{
			unlockable->statusKnown = qfalse;
			unlockable->unlocked    = qfalse;
		}

		itemNum++;
	}

#ifdef CGAME
	// export team and mask into cvar for UI
	trap_Cvar_Set( "ui_unlockables", va( "%d %d", team, mask ) );
#endif

	// we only know the state for one team
	unlockablesTeamKnowledge = team;
}

#ifdef UI
void UI_UpdateUnlockables( void )
{
	char   buffer[ MAX_TOKEN_CHARS ];
	team_t team;
	int    mask;

	trap_Cvar_VariableStringBuffer( "ui_unlockables", buffer, sizeof( buffer ) );
	sscanf( buffer, "%d %d", ( int * )&team, &mask );

	BG_ImportUnlockablesFromMask( team, mask );
}
#endif

static INLINE qboolean Unlocked( unlockableType_t type, int itemNum )
{
	return unlockables[ unlockablesTypeOffset[ type ] + itemNum ].unlocked;
}

static INLINE void CheckStatusKnowledge( unlockableType_t type, int itemNum )
{
	unlockable_t dummy;

	if ( !unlockables[ unlockablesTypeOffset[ type ] + itemNum ].statusKnown )
	{
		dummy.type = type;
		dummy.num  = itemNum;

		Com_Printf( S_WARNING "Asked for the status of unlockable item %s but the status is unknown.\n",
		            UnlockableHumanName( &dummy ) );
	}
}

qboolean BG_WeaponUnlocked( weapon_t weapon )
{
	CheckStatusKnowledge( UNLT_WEAPON, ( int )weapon );

	return Unlocked( UNLT_WEAPON, ( int )weapon );
}

qboolean BG_UpgradeUnlocked( upgrade_t upgrade )
{
	CheckStatusKnowledge( UNLT_UPGRADE, ( int )upgrade );

	return Unlocked( UNLT_UPGRADE, ( int )upgrade );
}

qboolean BG_BuildableUnlocked( buildable_t buildable )
{
	CheckStatusKnowledge( UNLT_BUILDABLE, ( int )buildable );

	return Unlocked( UNLT_BUILDABLE, ( int )buildable );
}

qboolean BG_ClassUnlocked( class_t class_ )
{
	CheckStatusKnowledge( UNLT_CLASS, ( int )class_ );

	return Unlocked( UNLT_CLASS, ( int )class_ );
}
