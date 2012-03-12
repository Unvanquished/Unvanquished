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

//  --> WP_* to WS_* conversion
//
// WS_MAX = no equivalent/not used
//
// FIXME: Remove everything that maps to WS_MAX to save space
//
// Gordon: why not er, just map directly to the stat?
// JK: er, several reasons: its less than half the size of a brute-force approach,
//                          gives us a simple map to what we actually care about, and the
//                          fact that we dont want to go above (if at all possible) 32 bits
static const weap_ws_convert_t aWeapID[WP_NUM_WEAPONS] = {

	{WP_NONE, WS_MAX},			// 0

	// German weapons
	{WP_KNIFE, WS_KNIFE},
	{WP_LUGER, WS_LUGER},
	{WP_MP40, WS_MP40},
	{WP_GRENADE_LAUNCHER, WS_GRENADE},	// 5
	{WP_PANZERFAUST, WS_PANZERFAUST},
	{WP_FLAMETHROWER, WS_FLAMETHROWER},

	// American equivalents
	{WP_COLT, WS_COLT},			// 10
	{WP_THOMPSON, WS_THOMPSON},
	{WP_GRENADE_PINEAPPLE, WS_GRENADE},
	{WP_STEN, WS_STEN},
	{WP_MEDIC_SYRINGE, WS_SYRINGE},
	{WP_AMMO, WS_MAX},
	{WP_ARTY, WS_ARTILLERY},


	{WP_SILENCER, WS_LUGER},	// 20
	{WP_DYNAMITE, WS_DYNAMITE},
	{WP_SMOKETRAIL, WS_ARTILLERY},
	{WP_MAPMORTAR, WS_MORTAR},
	{VERYBIGEXPLOSION, WS_MAX},
	{WP_MEDKIT, WS_MAX},
	{WP_BINOCULARS, WS_MAX},

	{WP_PLIERS, WS_MAX},		// 30
	{WP_SMOKE_MARKER, WS_AIRSTRIKE},

	{WP_KAR98, WS_K43},
	{WP_CARBINE, WS_GARAND},
	{WP_GARAND, WS_GARAND},
	{WP_LANDMINE, WS_LANDMINE},	// 35
	{WP_SATCHEL, WS_MAX},
	{WP_SATCHEL_DET, WS_SATCHEL},
	{WP_TRIPMINE, WS_LANDMINE},
	{WP_SMOKE_BOMB, WS_SMOKE},

	{WP_MOBILE_MG42, WS_MG42},	// 40
	{WP_MOBILE_MG42_SET, WS_MG42},
	{WP_K43, WS_K43},
	{WP_FG42, WS_FG42},
	{WP_DUMMY_MG42, WS_MG42},	// ??
	{WP_LOCKPICK, WS_MAX},

	{WP_GPG40, WS_GRENADELAUNCHER},
	{WP_M7, WS_GRENADELAUNCHER},
	{WP_SILENCED_COLT, WS_COLT},	// 50
	{WP_GARAND_SCOPE, WS_GARAND},
	{WP_K43_SCOPE, WS_K43},
	{WP_FG42SCOPE, WS_FG42},
	{WP_MORTAR_SET, WS_MORTAR},
	{WP_MORTAR, WS_MORTAR},
	{WP_MEDIC_ADRENALINE, WS_MAX},	// 55

	{WP_AKIMBO_SILENCEDCOLT, WS_COLT},
	{WP_AKIMBO_SILENCEDLUGER, WS_LUGER},
	{WP_AKIMBO_COLT, WS_COLT},
	{WP_AKIMBO_LUGER, WS_LUGER},
};


// Get right stats index based on weapon id
extWeaponStats_t BG_WeapStatForWeapon(weapon_t iWeaponID)
{
	weapon_t        i;

	for(i = WP_NONE; i < WP_NUM_WEAPONS; i++)
	{
		if(iWeaponID == aWeapID[i].iWeapon)
		{
			return aWeapID[i].iWS;
		}
	}

	return WS_MAX;
}
