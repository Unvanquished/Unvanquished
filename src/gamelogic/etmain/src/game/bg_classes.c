/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "../../../../engine/qcommon/q_shared.h"
#include "bg_public.h"

bg_playerclass_t bg_allies_playerclasses[ NUM_PLAYER_CLASSES ] =
{
	{
		PC_SOLDIER,
		"characters/temperate/allied/soldier.char",
		"ui/assets/mp_gun_blue.tga",
		"ui/assets/mp_arrow_blue.tga",
		{
			WP_THOMPSON,
			WP_MOBILE_MG42,
			WP_FLAMETHROWER,
			WP_PANZERFAUST,
			WP_MORTAR
		}
		,
	}
	,

	{
		PC_MEDIC,
		"characters/temperate/allied/medic.char",
		"ui/assets/mp_health_blue.tga",
		"ui/assets/mp_arrow_blue.tga",
		{
			WP_THOMPSON,
		}
		,
	}
	,

	{
		PC_ENGINEER,
		"characters/temperate/allied/engineer.char",
		"ui/assets/mp_wrench_blue.tga",
		"ui/assets/mp_arrow_blue.tga",
		{
			WP_THOMPSON,
			WP_CARBINE,
		}
		,
	}
	,

	{
		PC_FIELDOPS,
		"characters/temperate/allied/fieldops.char",
		"ui/assets/mp_ammo_blue.tga",
		"ui/assets/mp_arrow_blue.tga",
		{
			WP_THOMPSON,
		}
		,
	}
	,

	{
		PC_COVERTOPS,
		"characters/temperate/allied/cvops.char",
		"ui/assets/mp_spy_blue.tga",
		"ui/assets/mp_arrow_blue.tga",
		{
			WP_STEN,
			WP_FG42,
			WP_GARAND,
		}
		,
	}
	,
};

bg_playerclass_t bg_axis_playerclasses[ NUM_PLAYER_CLASSES ] =
{
	{
		PC_SOLDIER,
		"characters/temperate/axis/soldier.char",
		"ui/assets/mp_gun_red.tga",
		"ui/assets/mp_arrow_red.tga",
		{
			WP_MP40,
			WP_MOBILE_MG42,
			WP_FLAMETHROWER,
			WP_PANZERFAUST,
			WP_MORTAR
		}
		,
	}
	,

	{
		PC_MEDIC,
		"characters/temperate/axis/medic.char",
		"ui/assets/mp_health_red.tga",
		"ui/assets/mp_arrow_red.tga",
		{
			WP_MP40,
		}
		,
	}
	,

	{
		PC_ENGINEER,
		"characters/temperate/axis/engineer.char",
		"ui/assets/mp_wrench_red.tga",
		"ui/assets/mp_arrow_red.tga",
		{
			WP_MP40,
			WP_KAR98,
		}
		,
	}
	,

	{
		PC_FIELDOPS,
		"characters/temperate/axis/fieldops.char",
		"ui/assets/mp_ammo_red.tga",
		"ui/assets/mp_arrow_red.tga",
		{
			WP_MP40,
		}
		,
	}
	,

	{
		PC_COVERTOPS,
		"characters/temperate/axis/cvops.char",
		"ui/assets/mp_spy_red.tga",
		"ui/assets/mp_arrow_red.tga",
		{
			WP_STEN,
			WP_FG42,
			WP_K43,
		}
		,
	}
	,
};

bg_playerclass_t *BG_GetPlayerClassInfo( int team, int cls )
{
	bg_playerclass_t *teamList;

	if( cls < PC_SOLDIER || cls >= NUM_PLAYER_CLASSES )
	{
		cls = PC_SOLDIER;
	}

	switch( team )
	{
		default:
		case TEAM_AXIS:
			teamList = bg_axis_playerclasses;
			break;

		case TEAM_ALLIES:
			teamList = bg_allies_playerclasses;
			break;
	}

	return &teamList[ cls ];
}

bg_playerclass_t *BG_PlayerClassForPlayerState( playerState_t *ps )
{
	return BG_GetPlayerClassInfo( ps->persistant[ PERS_TEAM ], ps->stats[ STAT_PLAYER_CLASS ] );
}

qboolean BG_ClassHasWeapon( bg_playerclass_t *classInfo, weapon_t weap )
{
	int i;

	if( !weap )
	{
		return qfalse;
	}

	for( i = 0; i < MAX_WEAPS_PER_CLASS; i++ )
	{
		if( classInfo->classWeapons[ i ] == weap )
		{
			return qtrue;
		}
	}

	return qfalse;
}

qboolean BG_WeaponIsPrimaryForClassAndTeam( int classnum, team_t team, weapon_t weapon )
{
	bg_playerclass_t *classInfo;

	if( team == TEAM_ALLIES )
	{
		classInfo = &bg_allies_playerclasses[ classnum ];

		if( BG_ClassHasWeapon( classInfo, weapon ) )
		{
			return qtrue;
		}
	}
	else if( team == TEAM_AXIS )
	{
		classInfo = &bg_axis_playerclasses[ classnum ];

		if( BG_ClassHasWeapon( classInfo, weapon ) )
		{
			return qtrue;
		}
	}

	return qfalse;
}

const char     *BG_ShortClassnameForNumber( int classNum )
{
	switch( classNum )
	{
		case PC_SOLDIER:
			return "Soldr";

		case PC_MEDIC:
			return "Medic";

		case PC_ENGINEER:
			return "Engr";

		case PC_FIELDOPS:
			return "FdOps";

		case PC_COVERTOPS:
			return "CvOps";

		default:
			return "^1ERROR";
	}
}

const char     *BG_ClassnameForNumber( int classNum )
{
	switch( classNum )
	{
		case PC_SOLDIER:
			return "Soldier";

		case PC_MEDIC:
			return "Medic";

		case PC_ENGINEER:
			return "Engineer";

		case PC_FIELDOPS:
			return "Field Ops";

		case PC_COVERTOPS:
			return "Covert Ops";

		default:
			return "^1ERROR";
	}
}

const char     *BG_ClassLetterForNumber( int classNum )
{
	switch( classNum )
	{
		case PC_SOLDIER:
			return "S";

		case PC_MEDIC:
			return "M";

		case PC_ENGINEER:
			return "E";

		case PC_FIELDOPS:
			return "F";

		case PC_COVERTOPS:
			return "C";

		default:
			return "^1E";
	}
}

int BG_ClassTextToClass( char *token )
{
	if( !Q_stricmp( token, "soldier" ) )
	{
		return PC_SOLDIER;
	}
	else if( !Q_stricmp( token, "medic" ) )
	{
		return PC_MEDIC;
	}
	else if( !Q_stricmp( token, "lieutenant" ) )
	{
		// FIXME: remove from missionpack
		return PC_FIELDOPS;
	}
	else if( !Q_stricmp( token, "fieldops" ) )
	{
		return PC_FIELDOPS;
	}
	else if( !Q_stricmp( token, "engineer" ) )
	{
		return PC_ENGINEER;
	}
	else if( !Q_stricmp( token, "covertops" ) )
	{
		return PC_COVERTOPS;
	}

	return -1;
}

skillType_t BG_ClassSkillForClass( int classnum )
{
	skillType_t classskill[ NUM_PLAYER_CLASSES ] = { SK_HEAVY_WEAPONS, SK_FIRST_AID, SK_EXPLOSIVES_AND_CONSTRUCTION, SK_SIGNALS,
	    SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS
	                                               };

	if( classnum < 0 || classnum >= NUM_PLAYER_CLASSES )
	{
		return SK_BATTLE_SENSE;
	}

	return classskill[ classnum ];
}
