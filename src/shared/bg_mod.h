/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

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

struct meansOfDeathData_t
{
	meansOfDeath_t number;
	const char     *name;
};

static const meansOfDeathData_t bg_meansOfDeathData[] =
{
	{ MOD_UNKNOWN, "MOD_UNKNOWN" },
	{ MOD_SHOTGUN, "MOD_SHOTGUN" },
	{ MOD_BLASTER, "MOD_BLASTER" },
	{ MOD_PAINSAW, "MOD_PAINSAW" },
	{ MOD_MACHINEGUN, "MOD_MACHINEGUN" },
	{ MOD_CHAINGUN, "MOD_CHAINGUN" },
	{ MOD_PRIFLE, "MOD_PRIFLE" },
	{ MOD_MDRIVER, "MOD_MDRIVER" },
	{ MOD_LASGUN, "MOD_LASGUN" },
	{ MOD_LCANNON, "MOD_LCANNON" },
	{ MOD_LCANNON_SPLASH, "MOD_LCANNON_SPLASH" },
	{ MOD_FLAMER, "MOD_FLAMER" },
	{ MOD_FLAMER_SPLASH, "MOD_FLAMER_SPLASH" },
	{ MOD_BURN, "MOD_BURN" },
	{ MOD_GRENADE, "MOD_GRENADE" },
	{ MOD_FIREBOMB, "MOD_FIREBOMB" },
	{ MOD_WEIGHT_H, "MOD_WEIGHT_H" },
	{ MOD_WATER, "MOD_WATER" },
	{ MOD_SLIME, "MOD_SLIME" },
	{ MOD_LAVA, "MOD_LAVA" },
	{ MOD_CRUSH, "MOD_CRUSH" },
	{ MOD_TELEFRAG, "MOD_TELEFRAG" },
	{ MOD_SLAP, "MOD_SLAP"},
	{ MOD_FALLING, "MOD_FALLING" },
	{ MOD_SUICIDE, "MOD_SUICIDE" },
	{ MOD_TARGET_LASER, "MOD_TARGET_LASER" },
	{ MOD_TRIGGER_HURT, "MOD_TRIGGER_HURT" },

	{ MOD_ABUILDER_CLAW, "MOD_ABUILDER_CLAW" },
	{ MOD_LEVEL0_BITE, "MOD_LEVEL0_BITE" },
	{ MOD_LEVEL1_CLAW, "MOD_LEVEL1_CLAW" },
	{ MOD_LEVEL3_CLAW, "MOD_LEVEL3_CLAW" },
	{ MOD_LEVEL3_POUNCE, "MOD_LEVEL3_POUNCE" },
	{ MOD_LEVEL3_BOUNCEBALL, "MOD_LEVEL3_BOUNCEBALL" },
	{ MOD_LEVEL2_CLAW, "MOD_LEVEL2_CLAW" },
	{ MOD_LEVEL2_ZAP, "MOD_LEVEL2_ZAP" },
	{ MOD_LEVEL4_CLAW, "MOD_LEVEL4_CLAW" },
	{ MOD_LEVEL4_TRAMPLE, "MOD_LEVEL4_TRAMPLE" },
	{ MOD_WEIGHT_A, "MOD_WEIGHT_A" },

	{ MOD_SLOWBLOB, "MOD_SLOWBLOB" },
	{ MOD_POISON, "MOD_POISON" },
	{ MOD_SWARM, "MOD_SWARM" },

	{ MOD_HSPAWN, "MOD_HSPAWN" },
	{ MOD_ROCKETPOD, "MOD_ROCKETPOD" },
	{ MOD_MGTURRET, "MOD_MGTURRET" },
	{ MOD_REACTOR, "MOD_REACTOR" },

	{ MOD_ASPAWN, "MOD_ASPAWN" },
	{ MOD_ATUBE, "MOD_ATUBE" },
	{ MOD_SPIKER, "MOD_SPIKER" },
	{ MOD_OVERMIND, "MOD_OVERMIND" },
	{ MOD_DECONSTRUCT, "MOD_DECONSTRUCT" },
	{ MOD_REPLACE, "MOD_REPLACE" },
	{ MOD_BUILDLOG_REVERT, "MOD_BUILDLOG_REVERT" },
};

static const size_t bg_numMeansOfDeath = ARRAY_LEN( bg_meansOfDeathData );