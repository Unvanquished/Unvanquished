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
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA	02110-1301	USA
===========================================================================
*/

// bg_parser.c -- parsers for configuration files

#include "engine/qcommon/q_shared.h"
#include "bg_public.h"

#ifdef BUILD_CGAME
#include "cgame/cg_local.h"
#endif

int  trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
int  trap_FS_Read( void *buffer, int len, fileHandle_t f );
int  trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void trap_FS_FCloseFile( fileHandle_t f );

#define PARSE(text, token) \
	(token) = COM_Parse( &(text) ); \
	if ( !*(token) ) \
	{ \
		break; \
	}

typedef enum
{
	INTEGER,
	FLOAT
} configVarType_t;

typedef struct
{
	//The name is on top of the structure, this is useful for bsearch
	const char *name;
	configVarType_t type;
	bool defined;
	void *var;
} configVar_t;

//Definition of the config vars

// Alien weapons

int   ABUILDER_CLAW_DMG;
float ABUILDER_CLAW_RANGE;
float ABUILDER_CLAW_WIDTH;
int   ABUILDER_BLOB_DMG;
float ABUILDER_BLOB_SPEED;
float ABUILDER_BLOB_SPEED_MOD;
int   ABUILDER_BLOB_TIME;

int   LEVEL0_BITE_DMG;
float LEVEL0_BITE_RANGE;
float LEVEL0_BITE_WIDTH;
int   LEVEL0_BITE_REPEAT;

int   LEVEL1_CLAW_DMG;
float LEVEL1_CLAW_RANGE;
float LEVEL1_CLAW_U_RANGE;
float LEVEL1_CLAW_WIDTH;

int   LEVEL2_CLAW_DMG;
float LEVEL2_CLAW_RANGE;
float LEVEL2_CLAW_U_RANGE;
float LEVEL2_CLAW_WIDTH;
int   LEVEL2_AREAZAP_DMG;
float LEVEL2_AREAZAP_RANGE;
float LEVEL2_AREAZAP_CHAIN_RANGE;
float LEVEL2_AREAZAP_CHAIN_FALLOFF;
float LEVEL2_AREAZAP_WIDTH;
int   LEVEL2_AREAZAP_TIME;
float LEVEL2_WALLJUMP_MAXSPEED;

int   LEVEL3_CLAW_DMG;
float LEVEL3_CLAW_RANGE;
float LEVEL3_CLAW_UPG_RANGE;
float LEVEL3_CLAW_WIDTH;
int   LEVEL3_POUNCE_DMG;
float LEVEL3_POUNCE_RANGE;
float LEVEL3_POUNCE_UPG_RANGE;
float LEVEL3_POUNCE_WIDTH;
int   LEVEL3_POUNCE_TIME;
int   LEVEL3_POUNCE_TIME_UPG;
int   LEVEL3_POUNCE_TIME_MIN;
int   LEVEL3_POUNCE_REPEAT;
float LEVEL3_POUNCE_SPEED_MOD;
int   LEVEL3_POUNCE_JUMP_MAG;
int   LEVEL3_POUNCE_JUMP_MAG_UPG;
int   LEVEL3_BOUNCEBALL_DMG;
float LEVEL3_BOUNCEBALL_SPEED;
int   LEVEL3_BOUNCEBALL_RADIUS;
int   LEVEL3_BOUNCEBALL_REGEN;

int   LEVEL4_CLAW_DMG;
float LEVEL4_CLAW_RANGE;
float LEVEL4_CLAW_WIDTH;
float LEVEL4_CLAW_HEIGHT;
int   LEVEL4_TRAMPLE_DMG;
float LEVEL4_TRAMPLE_SPEED;
int   LEVEL4_TRAMPLE_CHARGE_MIN;
int   LEVEL4_TRAMPLE_CHARGE_MAX;
int   LEVEL4_TRAMPLE_CHARGE_TRIGGER;
int   LEVEL4_TRAMPLE_DURATION;
int   LEVEL4_TRAMPLE_STOP_PENALTY;
int   LEVEL4_TRAMPLE_REPEAT;

int   MEDKIT_POISON_IMMUNITY_TIME;
int   MEDKIT_STARTUP_TIME;
int   MEDKIT_STARTUP_SPEED;

// Human buildables

float REACTOR_BASESIZE;
float REACTOR_ATTACK_RANGE;
int   REACTOR_ATTACK_REPEAT;
int   REACTOR_ATTACK_DAMAGE;

float REPEATER_BASESIZE;

// Human Weapons

int   BLASTER_SPREAD;
int   BLASTER_SPEED;
int   BLASTER_DMG;
int   BLASTER_SIZE;

int   RIFLE_SPREAD;
int   RIFLE_DMG;

int   PAINSAW_DAMAGE;
float PAINSAW_RANGE;
float PAINSAW_WIDTH;
float PAINSAW_HEIGHT;

int   SHOTGUN_PELLETS;
int   SHOTGUN_SPREAD;
int   SHOTGUN_DMG;
int   SHOTGUN_RANGE;

int   LASGUN_DAMAGE;

int   MDRIVER_DMG;

int   CHAINGUN_SPREAD;
int   CHAINGUN_DMG;

int   FLAMER_DMG;
int   FLAMER_FLIGHTDAMAGE;
int   FLAMER_SPLASHDAMAGE;
int   FLAMER_RADIUS;
int   FLAMER_SIZE;
float FLAMER_LIFETIME;
float FLAMER_SPEED;
float FLAMER_LAG;
float FLAMER_IGNITE_RADIUS;
float FLAMER_IGNITE_CHANCE;
float FLAMER_IGNITE_SPLCHANCE;

int   PRIFLE_DMG;
int   PRIFLE_SPEED;
int   PRIFLE_SIZE;

int   LCANNON_DAMAGE;
int   LCANNON_RADIUS;
int   LCANNON_SIZE;
int   LCANNON_SECONDARY_DAMAGE;
int   LCANNON_SECONDARY_RADIUS;
int   LCANNON_SECONDARY_SPEED;
int   LCANNON_SPEED;
int   LCANNON_CHARGE_TIME_MAX;
int   LCANNON_CHARGE_TIME_MIN;
int   LCANNON_CHARGE_TIME_WARN;
int   LCANNON_CHARGE_AMMO;

// MUST BE ALPHABETICALLY SORTED!
static configVar_t bg_configVars[] =
{
	{"b_reactor_powerRadius", FLOAT, false, &REACTOR_BASESIZE},
	{"b_reactor_zapAttackDamage", INTEGER, false, &REACTOR_ATTACK_DAMAGE},
	{"b_reactor_zapAttackRange", FLOAT, false, &REACTOR_ATTACK_RANGE},
	{"b_reactor_zapAttackRepeat", INTEGER, false, &REACTOR_ATTACK_REPEAT},

	{"b_repeater_powerRadius", FLOAT, false, &REPEATER_BASESIZE},

	{"u_medkit_poisonImmunityTime", INTEGER, false, &MEDKIT_POISON_IMMUNITY_TIME},
	{"u_medkit_startupSpeed", INTEGER, false, &MEDKIT_STARTUP_SPEED},
	{"u_medkit_startupTime", INTEGER, false, &MEDKIT_STARTUP_TIME},

	{"w_abuild_blobDmg", INTEGER, false, &ABUILDER_BLOB_DMG},
	{"w_abuild_blobSlowTime", INTEGER, false, &ABUILDER_BLOB_TIME},
	{"w_abuild_blobSpeed", FLOAT, false, &ABUILDER_BLOB_SPEED},
	{"w_abuild_blobSpeedMod", FLOAT, false, &ABUILDER_BLOB_SPEED_MOD},
	{"w_abuild_clawDmg", INTEGER, false, &ABUILDER_CLAW_DMG},
	{"w_abuild_clawRange", FLOAT, false, &ABUILDER_CLAW_RANGE},
	{"w_abuild_clawWidth", FLOAT, false, &ABUILDER_CLAW_WIDTH},

	{"w_blaster_damage", INTEGER, false, &BLASTER_DMG },
	{"w_blaster_size", INTEGER, false, &BLASTER_SIZE },
	{"w_blaster_speed", INTEGER, false, &BLASTER_SPEED },
	{"w_blaster_spread", INTEGER, false, &BLASTER_SPREAD },

	{"w_chaingun_damage", INTEGER, false, &CHAINGUN_DMG },
	{"w_chaingun_spread", INTEGER, false, &CHAINGUN_SPREAD },

	{"w_flamer_damage", INTEGER, false, &FLAMER_DMG },
	{"w_flamer_flightDamage", INTEGER, false, &FLAMER_FLIGHTDAMAGE },
	{"w_flamer_igniteChance", FLOAT, false, &FLAMER_IGNITE_CHANCE },
	{"w_flamer_igniteRadius", FLOAT, false, &FLAMER_IGNITE_RADIUS },
	{"w_flamer_igniteSplChance", FLOAT, false, &FLAMER_IGNITE_SPLCHANCE },
	{"w_flamer_lag", FLOAT, false, &FLAMER_LAG },
	{"w_flamer_lifeTime", FLOAT, false, &FLAMER_LIFETIME },
	{"w_flamer_radius", INTEGER, false, &FLAMER_RADIUS },
	{"w_flamer_size", INTEGER, false, &FLAMER_SIZE },
	{"w_flamer_speed", FLOAT, false, &FLAMER_SPEED },
	{"w_flamer_splashDamage", INTEGER, false, &FLAMER_SPLASHDAMAGE },

	{"w_lcannon_chargeAmmo", INTEGER, false, &LCANNON_CHARGE_AMMO },
	{"w_lcannon_chargeTimeMax", INTEGER, false, &LCANNON_CHARGE_TIME_MAX },
	{"w_lcannon_chargeTimeMin", INTEGER, false, &LCANNON_CHARGE_TIME_MIN },
	{"w_lcannon_chargeTimeWarn", INTEGER, false, &LCANNON_CHARGE_TIME_WARN },
	{"w_lcannon_damage", INTEGER, false, &LCANNON_DAMAGE },
	{"w_lcannon_radius", INTEGER, false, &LCANNON_RADIUS },
	{"w_lcannon_secondaryDamage", INTEGER, false, &LCANNON_SECONDARY_DAMAGE },
	{"w_lcannon_secondaryRadius", INTEGER, false, &LCANNON_SECONDARY_RADIUS },
	{"w_lcannon_secondarySpeed", INTEGER, false, &LCANNON_SECONDARY_SPEED },
	{"w_lcannon_size", INTEGER, false, &LCANNON_SIZE },
	{"w_lcannon_speed", INTEGER, false, &LCANNON_SPEED },

	{"w_level0_biteDmg", INTEGER, false, &LEVEL0_BITE_DMG},
	{"w_level0_biteRange", FLOAT, false, &LEVEL0_BITE_RANGE},
	{"w_level0_biteRepeat", INTEGER, false, &LEVEL0_BITE_REPEAT},
	{"w_level0_biteWidth", FLOAT, false, &LEVEL0_BITE_WIDTH},

	{"w_level1_clawDmg", INTEGER, false, &LEVEL1_CLAW_DMG},
	{"w_level1_clawRange", FLOAT, false, &LEVEL1_CLAW_RANGE},
	{"w_level1_clawWidth", FLOAT, false, &LEVEL1_CLAW_WIDTH},

	{"w_level2upg_clawRange", FLOAT, false, &LEVEL2_CLAW_U_RANGE},
	{"w_level2upg_zapChainFalloff", FLOAT, false, &LEVEL2_AREAZAP_CHAIN_FALLOFF},
	{"w_level2upg_zapChainRange", FLOAT, false, &LEVEL2_AREAZAP_CHAIN_RANGE},
	{"w_level2upg_zapDmg", INTEGER, false, &LEVEL2_AREAZAP_DMG},
	{"w_level2upg_zapRange", FLOAT, false, &LEVEL2_AREAZAP_RANGE},
	{"w_level2upg_zapTime", INTEGER, false, &LEVEL2_AREAZAP_TIME},
	{"w_level2upg_zapWidth", FLOAT, false, &LEVEL2_AREAZAP_WIDTH},

	{"w_level2_clawDmg", INTEGER, false, &LEVEL2_CLAW_DMG},
	{"w_level2_clawRange", FLOAT, false, &LEVEL2_CLAW_RANGE},
	{"w_level2_clawWidth", FLOAT, false, &LEVEL2_CLAW_WIDTH},
	{"w_level2_maxWalljumpSpeed", FLOAT, false, &LEVEL2_WALLJUMP_MAXSPEED},

	{"w_level3upg_ballDmg", INTEGER, false, &LEVEL3_BOUNCEBALL_DMG},
	{"w_level3upg_ballRadius", INTEGER, false, &LEVEL3_BOUNCEBALL_RADIUS},
	{"w_level3upg_ballRegen", INTEGER, false, &LEVEL3_BOUNCEBALL_REGEN},
	{"w_level3upg_ballSpeed", FLOAT, false, &LEVEL3_BOUNCEBALL_SPEED},
	{"w_level3upg_clawRange", FLOAT, false, &LEVEL3_CLAW_UPG_RANGE},
	{"w_level3upg_pounceDuration", INTEGER, false, &LEVEL3_POUNCE_TIME_UPG},
	{"w_level3upg_pounceJumpMagnitude", INTEGER, false, &LEVEL3_POUNCE_JUMP_MAG_UPG},
	{"w_level3upg_pounceRange", FLOAT, false, &LEVEL3_POUNCE_UPG_RANGE},

	{"w_level3_clawDmg", INTEGER, false, &LEVEL3_CLAW_DMG},
	{"w_level3_clawRange", FLOAT, false, &LEVEL3_CLAW_RANGE},
	{"w_level3_clawWidth", FLOAT, false, &LEVEL3_CLAW_WIDTH},
	{"w_level3_pounceDmg", INTEGER, false, &LEVEL3_POUNCE_DMG},
	{"w_level3_pounceDuration", INTEGER, false, &LEVEL3_POUNCE_TIME},
	{"w_level3_pounceJumpMagnitude", INTEGER, false, &LEVEL3_POUNCE_JUMP_MAG},
	{"w_level3_pounceRange", FLOAT, false, &LEVEL3_POUNCE_RANGE},
	{"w_level3_pounceRepeat", INTEGER, false, &LEVEL3_POUNCE_REPEAT},
	{"w_level3_pounceSpeedMod", FLOAT, false, &LEVEL3_POUNCE_SPEED_MOD},
	{"w_level3_pounceTimeMin", INTEGER, false, &LEVEL3_POUNCE_TIME_MIN},
	{"w_level3_pounceWidth", FLOAT, false, &LEVEL3_POUNCE_WIDTH},

	{"w_level4_clawDmg", INTEGER, false, &LEVEL4_CLAW_DMG},
	{"w_level4_clawHeight", FLOAT, false, &LEVEL4_CLAW_HEIGHT},
	{"w_level4_clawRange", FLOAT, false, &LEVEL4_CLAW_RANGE},
	{"w_level4_clawWidth", FLOAT, false, &LEVEL4_CLAW_WIDTH},
	{"w_level4_trampleChargeMax", INTEGER, false, &LEVEL4_TRAMPLE_CHARGE_MAX},
	{"w_level4_trampleChargeMin", INTEGER, false, &LEVEL4_TRAMPLE_CHARGE_MIN},
	{"w_level4_trampleChargeTrigger", INTEGER, false, &LEVEL4_TRAMPLE_CHARGE_TRIGGER},
	{"w_level4_trampleDmg", INTEGER, false, &LEVEL4_TRAMPLE_DMG},
	{"w_level4_trampleDuration", INTEGER, false, &LEVEL4_TRAMPLE_DURATION},
	{"w_level4_trampleRepeat", INTEGER, false, &LEVEL4_TRAMPLE_REPEAT},
	{"w_level4_trampleSpeed", FLOAT, false, &LEVEL4_TRAMPLE_SPEED},
	{"w_level4_trampleStopPenalty", INTEGER, false, &LEVEL4_TRAMPLE_STOP_PENALTY},

	{"w_lgun_damage", INTEGER, false, &LASGUN_DAMAGE },

	{"w_mdriver_damage", INTEGER, false, &MDRIVER_DMG },

	{"w_prifle_damage", INTEGER, false, &PRIFLE_DMG },
	{"w_prifle_size", INTEGER, false, &PRIFLE_SIZE },
	{"w_prifle_speed", INTEGER, false, &PRIFLE_SPEED },

	{"w_psaw_damage", INTEGER, false, &PAINSAW_DAMAGE },
	{"w_psaw_height", FLOAT, false, &PAINSAW_HEIGHT },
	{"w_psaw_range", FLOAT, false, &PAINSAW_RANGE },
	{"w_psaw_width", FLOAT, false, &PAINSAW_WIDTH },

	{"w_rifle_damage", INTEGER, false, &RIFLE_DMG },
	{"w_rifle_spread", INTEGER, false, &RIFLE_SPREAD },

	{"w_shotgun_damage", INTEGER, false, &SHOTGUN_DMG },
	{"w_shotgun_pellets", INTEGER, false, &SHOTGUN_PELLETS },
	{"w_shotgun_range", INTEGER, false, &SHOTGUN_RANGE },
	{"w_shotgun_spread", INTEGER, false, &SHOTGUN_SPREAD },
};

static const size_t bg_numConfigVars = ARRAY_LEN( bg_configVars );

/*
======================
BG_ReadWholeFile

Helper function that tries to read a whole file in a buffer. Should it be in bg_parse.c?
======================
*/

bool BG_ReadWholeFile( const char *filename, char *buffer, int size)
{
	fileHandle_t f;
	int len;

	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len < 0 )
	{
		Com_Printf( S_ERROR "file %s doesn't exist\n", filename );
		return false;
	}

	if ( len == 0 || len >= size - 1 )
	{
		trap_FS_FCloseFile( f );
		Com_Printf( S_ERROR "file %s is %s\n", filename,
					len == 0 ? "empty" : "too long" );
		return false;
	}

	trap_FS_Read( buffer, len, f );
	buffer[ len ] = 0;
	trap_FS_FCloseFile( f );

	return true;
}

static team_t ParseTeam(const char* token)
{
	if ( !Q_strnicmp( token, "alien", 5 ) ) // alien(s)
	{
		return TEAM_ALIENS;
	}
	else if ( !Q_strnicmp( token, "human", 5 ) ) // human(s)
	{
		return TEAM_HUMANS;
	}
	else if ( !Q_stricmp( token, "none" ) )
	{
		return TEAM_NONE;
	}
	else
	{
		Com_Printf( S_ERROR "unknown team value '%s'\n", token );
		return TEAM_NONE;
	}
}

static int ParseSlotList(const char** text)
{
	int slots = 0;
	char* token;
	int count;

	token = COM_Parse( text );

	if ( !*token )
	{
		return slots;
	}

	count = atoi( token );

	while ( count --> 0 )
	{
		token = COM_Parse( text );

		if ( !*token )
		{
			return slots;
		}

		if ( !Q_stricmp( token, "head" ) )
		{
			slots |= SLOT_HEAD;
		}
		else if ( !Q_stricmp( token, "torso" ) )
		{
			slots |= SLOT_TORSO;
		}
		else if ( !Q_stricmp( token, "arms" ) )
		{
			slots |= SLOT_ARMS;
		}
		else if ( !Q_stricmp( token, "legs" ) )
		{
			slots |= SLOT_LEGS;
		}
		else if ( !Q_stricmp( token, "backpack" ) )
		{
			slots |= SLOT_BACKPACK;
		}
		else if ( !Q_stricmp( token, "weapon" ) )
		{
			slots |= SLOT_WEAPON;
		}
		else if ( !Q_stricmp( token, "sidearm" ) )
		{
			slots |= SLOT_SIDEARM;
		}
		else if ( !Q_stricmp( token, "grenade" ) )
		{
			slots |= SLOT_GRENADE;
		}
		else
		{
			Com_Printf( S_ERROR "unknown slot '%s'\n", token );
		}
	}

	return slots;
}

static int ParseClipmask( const char *token )
{
	if      ( !Q_stricmp( token, "MASK_ALL" ) )
	{
		return MASK_ALL;
	}
	else if ( !Q_stricmp( token, "MASK_SOLID" ) )
	{
		return MASK_SOLID;
	}
	else if ( !Q_stricmp( token, "MASK_PLAYERSOLID" ) )
	{
		return MASK_PLAYERSOLID;
	}
	else if ( !Q_stricmp( token, "MASK_DEADSOLID" ) )
	{
		return MASK_DEADSOLID;
	}
	else if ( !Q_stricmp( token, "MASK_WATER" ) )
	{
		return MASK_WATER;
	}
	else if ( !Q_stricmp( token, "MASK_OPAQUE" ) )
	{
		return MASK_OPAQUE;
	}
	else if ( !Q_stricmp( token, "MASK_SHOT" ) )
	{
		return MASK_SHOT;
	}
	else
	{
		Com_Printf( S_ERROR "unknown clipmask value '%s'\n", token );
		return 0;
	}
}

static trType_t ParseTrajectoryType( const char *token )
{
	if      ( !Q_stricmp( token, "TR_STATIONARY" ) )
	{
		return TR_STATIONARY;
	}
	else if ( !Q_stricmp( token, "TR_INTERPOLATE" ) )
	{
		return TR_INTERPOLATE;
	}
	else if ( !Q_stricmp( token, "TR_LINEAR" ) )
	{
		return TR_LINEAR;
	}
	else if ( !Q_stricmp( token, "TR_LINEAR_STOP" ) )
	{
		return TR_LINEAR_STOP;
	}
	else if ( !Q_stricmp( token, "TR_SINE" ) )
	{
		return TR_SINE;
	}
	else if ( !Q_stricmp( token, "TR_GRAVITY" ) )
	{
		return TR_GRAVITY;
	}
	else if ( !Q_stricmp( token, "TR_BUOYANCY" ) )
	{
		return TR_BUOYANCY;
	}
	else
	{
		Com_Printf( S_ERROR "unknown trajectory value '%s'\n", token );
		return TR_STATIONARY;
	}
}

int configVarComparator(const void* a, const void* b)
{
	const configVar_t *ca = (const configVar_t*) a;
	const configVar_t *cb = (const configVar_t*) b;
	return Q_stricmp(ca->name, cb->name);
}

configVar_t* BG_FindConfigVar(const char *varName)
{
	return (configVar_t*) bsearch(&varName, bg_configVars, bg_numConfigVars, sizeof(configVar_t), configVarComparator);
}

bool BG_ParseConfigVar(configVar_t *var, const char **text, const char *filename)
{
	char *token;

	token = COM_Parse( text );

	if( !*token )
	{
		Com_Printf( S_COLOR_RED "ERROR: %s expected argument for '%s'\n", filename, var->name );
		return false;
	}

	if( var->type == INTEGER)
	{
		*((int*) var->var) = atoi( token );
	}
	else if( var->type == FLOAT)
	{
		*((float*) var->var) = atof( token );
	}

	var->defined = true;

	return true;
}

bool BG_CheckConfigVars()
{
	int ok = true;

	for( unsigned i = 0; i < bg_numConfigVars; i++)
	{
		if( !bg_configVars[i].defined )
		{
			ok = false;
			Com_Printf(S_COLOR_YELLOW "WARNING: config var %s was not defined\n", bg_configVars[i].name );
		}
	}

	return ok;
}

/*
======================
BG_ParseBuildableAttributeFile

Parses a configuration file describing the attributes of a buildable
======================
*/

void BG_ParseBuildableAttributeFile( const char *filename, buildableAttributes_t *ba )
{
	const char *token;
	char text_buffer[ 20000 ];
	const char* text;
	configVar_t* var;
	int defined = 0;
	enum
	{
		HUMANNAME = 1 << 1,
		DESCRIPTION = 1 << 2,
		NORMAL = 1 << 3,
		BUILDPOINTS = 1 << 4,
		ICON = 1 << 5,
		HEALTH = 1 << 6,
		DEATHMOD = 1 << 7,
		TEAM = 1 << 8,
		BUILDWEAPON = 1 << 9,
		BUILDTIME = 1 << 10,
		UNUSED_11 = 1 << 11,
		POWERCONSUMPTION = 1 << 12,
		UNLOCKTHRESHOLD = 1 << 13
	};

	if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
	{
		return;
	}

	text = text_buffer;

	while ( 1 )
	{
		PARSE(text, token);

		if ( !Q_stricmp( token, "humanName" ) )
		{
			PARSE(text, token);

			ba->humanName = BG_strdup( token );

			defined |= HUMANNAME;
		}
		else if ( !Q_stricmp( token, "description" ) )
		{
			PARSE(text, token);

			ba->info = BG_strdup( token );

			defined |= DESCRIPTION;
		}
		else if ( !Q_stricmp( token, "icon" ) )
		{
			PARSE(text, token);

			if ( !Q_stricmp( token, "null" ) )
			{
				ba->icon = nullptr;
			}
			else
			{
				ba->icon = BG_strdup(token);
			}

			defined |= ICON;
		}
		else if ( !Q_stricmp( token, "buildPoints" ) )
		{
			PARSE(text, token);

			ba->buildPoints = atoi(token);

			defined |= BUILDPOINTS;
		}
		else if ( !Q_stricmp( token, "powerConsumption" ) )
		{
			PARSE(text, token);

			ba->powerConsumption = atoi(token);

			defined |= POWERCONSUMPTION;
		}
		else if ( !Q_stricmp( token, "health" ) )
		{
			PARSE(text, token);

			ba->health = atoi(token);

			defined |= HEALTH;
		}
		else if ( !Q_stricmp( token, "regen" ) )
		{
			PARSE(text, token);

			ba->regenRate = atoi(token);
		}
		else if ( !Q_stricmp( token, "splashDamage" ) )
		{
			PARSE(text, token);

			ba->splashDamage = atoi(token);
		}
		else if ( !Q_stricmp( token, "splashRadius" ) )
		{
			PARSE(text, token);

			ba->splashRadius = atoi(token);
		}
		else if ( !Q_stricmp( token, "weapon" ) )
		{
			PARSE(text, token);

			ba->weapon = BG_WeaponNumberByName( token );

			if ( !ba->weapon )
			{
				Com_Printf( S_ERROR "unknown weapon name '%s'\n", token );
			}
		}
		else if ( !Q_stricmp( token, "meansOfDeath" ) )
		{
			PARSE(text, token);

			if ( !Q_stricmp( token, "alienBuildable" ) )
			{
				ba->meansOfDeath = MOD_ASPAWN;
			}
			else if ( !Q_stricmp( token, "humanBuildable" ) )
			{
				ba->meansOfDeath = MOD_HSPAWN;
			}
			else
			{
				Com_Printf( S_ERROR "unknown meanOfDeath value '%s'\n", token );
			}

			defined |= DEATHMOD;
		}
		else if ( !Q_stricmp( token, "team" ) )
		{
			PARSE(text, token);

			ba->team = ParseTeam(token);

			defined |= TEAM;
		}
		else if ( !Q_stricmp( token, "buildWeapon" ) )
		{
			PARSE(text, token);

			if ( !Q_stricmp( token, "alien" ) )
			{
				ba->buildWeapon = (weapon_t) ( ( 1 << WP_ABUILD ) | ( 1 << WP_ABUILD2 ) );
			}
			else if ( !Q_stricmp( token, "human" ) )
			{
				ba->buildWeapon = (weapon_t) ( 1 << WP_HBUILD );
			}
			else
			{
				Com_Printf( S_ERROR "unknown buildWeapon value '%s'\n", token );
			}

			defined |= BUILDWEAPON;
		}
		else if ( !Q_stricmp( token, "buildTime" ) )
		{
			PARSE(text, token);

			ba->buildTime = atoi(token);

			defined |= BUILDTIME;
		}
		else if ( !Q_stricmp( token, "usable" ) )
		{
			ba->usable = true;
		}
		else if ( !Q_stricmp( token, "minNormal" ) )
		{
			PARSE(text, token);

			ba->minNormal = atof(token);
			defined |= NORMAL;
		}

		else if ( !Q_stricmp( token, "allowInvertNormal" ) )
		{
			ba->invertNormal = true;
		}
		else if ( !Q_stricmp( token, "needsCreep" ) )
		{
			ba->creepTest = true;
		}
		else if ( !Q_stricmp( token, "creepSize" ) )
		{
			PARSE(text, token);

			ba->creepSize = atoi(token);
		}
		else if ( !Q_stricmp( token, "transparentTest" ) )
		{
			ba->transparentTest = true;
		}
		else if ( !Q_stricmp( token, "unique" ) )
		{
			ba->uniqueTest = true;
		}
		else if ( !Q_stricmp( token, "unlockThreshold" ) )
		{
			PARSE(text, token);

			ba->unlockThreshold = atoi(token);
			defined |= UNLOCKTHRESHOLD;
		}
		else if( (var = BG_FindConfigVar( va( "b_%s_%s", ba->name, token ) ) ) != nullptr )
		{
			BG_ParseConfigVar( var, &text, filename );
		}
		else
		{
			Com_Printf( S_ERROR "%s: unknown token '%s'\n", filename, token );
		}
	}

	if ( !( defined & HUMANNAME) ) { token = "humanName"; }
	else if ( !( defined & DESCRIPTION) ) { token = "description"; }
	else if ( !( defined & BUILDPOINTS) ) { token = "buildPoints"; }
	else if ( !( defined & ICON) ) { token = "icon"; }
	else if ( !( defined & HEALTH) ) { token = "health"; }
	else if ( !( defined & DEATHMOD) ) { token = "meansOfDeath"; }
	else if ( !( defined & TEAM) ) { token = "team"; }
	else if ( !( defined & BUILDWEAPON) ) { token = "buildWeapon"; }
	else if ( !( defined & BUILDTIME) ) { token = "buildTime"; }
	else if ( !( defined & NORMAL) ) { token = "minNormal"; }

	if ( strlen( token ) > 0 )
	{
		Com_Printf( S_ERROR "%s not defined in %s\n", token, filename );
	}
}

/*
======================
BG_ParseBuildableModelFile

Parses a configuration file describing the model of a buildable
======================
*/
void BG_ParseBuildableModelFile( const char *filename, buildableModelConfig_t *bc )
{
	const char *token;
	char text_buffer[ 20000 ];
	const char* text;
	int defined = 0;
	enum
	{
		MODEL = 1 << 0,
		MODELSCALE = 1 << 1,
		MINS = 1 << 2,
		MAXS = 1 << 3,
		ZOFFSET = 1 << 4,
		OLDSCALE = 1 << 5,
		OLDOFFSET = 1 << 6,
		MODEL_ROTATION = 1 << 7
	};

	if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
	{
		return;
	}

	text = text_buffer;

	// read optional parameters
	while ( 1 )
	{
		PARSE(text, token);

		if ( !Q_stricmp( token, "model" ) )
		{
			int index = 0;

			PARSE(text, token);

			index = atoi( token );

			if ( index < 0 )
			{
				index = 0;
			}
			else if ( index >= MAX_BUILDABLE_MODELS )
			{
				index = MAX_BUILDABLE_MODELS - 1;
			}

			PARSE(text, token);

			Q_strncpyz( bc->models[ index ], token, sizeof( bc->models[ 0 ] ) );

			defined |= MODEL;
		}
		else if ( !Q_stricmp( token, "modelScale" ) )
		{
			float scale;

			PARSE(text, token);

			scale = atof( token );

			if ( scale < 0.0f )
			{
				scale = 0.0f;
			}

			bc->modelScale = scale;

			defined |= MODELSCALE;
		}
		else if ( !Q_stricmp( token, "modelRotation" ) )
		{
			PARSE( text, token );
			bc->modelRotation[ 0 ] = atof( token );
			PARSE( text, token );
			bc->modelRotation[ 1 ] = atof( token );
			PARSE( text, token );
			bc->modelRotation[ 2 ] = atof( token );

			defined |= MODEL_ROTATION;
		}
		else if ( !Q_stricmp( token, "mins" ) )
		{
			int i;

			for ( i = 0; i <= 2; i++ )
			{
				PARSE(text, token);

				bc->mins[ i ] = atof( token );
			}

			defined |= MINS;
		}
		else if ( !Q_stricmp( token, "maxs" ) )
		{
			int i;

			for ( i = 0; i <= 2; i++ )
			{
				PARSE(text, token);

				bc->maxs[ i ] = atof( token );
			}

			defined |= MAXS;
		}
		else if ( !Q_stricmp( token, "zOffset" ) )
		{
			PARSE(text, token);

			bc->zOffset = atof( token );

			defined |= ZOFFSET;
		}
		else if ( !Q_stricmp( token, "oldScale" ) )
		{
			PARSE(text, token);

			bc->oldScale = atof( token );

			defined |= OLDSCALE;
		}
		else if ( !Q_stricmp( token, "oldOffset" ) )
		{
			PARSE(text, token);

			bc->oldOffset = atof( token );

			defined |= OLDOFFSET;
		}
		else
		{
			Com_Printf( S_ERROR "%s: unknown token '%s'\n", filename, token );
		}
	}

	if ( !( defined & MODEL ) ) { token = "model"; }
	else if ( !( defined & MODELSCALE ) ) { token = "modelScale"; }
	else if ( !( defined & MINS ) ) { token = "mins"; }
	else if ( !( defined & MAXS ) ) { token = "maxs"; }
	else if ( !( defined & ZOFFSET ) ) { token = "zOffset"; }
	else { token = ""; }

	if ( strlen( token ) > 0 )
	{
		Com_Printf( S_ERROR "%s not defined in %s\n", token, filename );
	}
}

/*
======================
BG_ParseClassAttributeFile

Parses a configuration file describing the attributes of a class
======================
*/

void BG_ParseClassAttributeFile( const char *filename, classAttributes_t *ca )
{
	const char         *token = nullptr;
	char         text_buffer[ 20000 ];
	const char         *text;
	configVar_t  *var;
	int          defined = 0;

	enum
	{
		INFO               = 1 <<  0,
		FOVCVAR            = 1 <<  1,
		TEAM               = 1 <<  2,
		HEALTH             = 1 <<  3,
		FALLDAMAGE         = 1 <<  4,
		REGEN              = 1 <<  5,
		FOV                = 1 <<  6,
		STEPTIME           = 1 <<  7,
		SPEED              = 1 <<  8,
		ACCELERATION       = 1 <<  9,
		AIRACCELERATION    = 1 << 10,
		FRICTION           = 1 << 11,
		STOPSPEED          = 1 << 12,
		JUMPMAGNITUDE      = 1 << 13,
		ICON               = 1 << 14,
		COST               = 1 << 15,
		SPRINTMOD          = 1 << 16,
		UNUSED_17          = 1 << 17,
		MASS               = 1 << 18,
		UNLOCKTHRESHOLD    = 1 << 19,
		STAMINAJUMPCOST    = 1 << 20,
		STAMINASPRINTCOST  = 1 << 21,
		STAMINAJOGRESTORE  = 1 << 22,
		STAMINAWALKRESTORE = 1 << 23,
		STAMINASTOPRESTORE = 1 << 24
	};

	if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
	{
		return;
	}

	text = text_buffer;

	while ( 1 )
	{
		PARSE(text, token);

		if ( !Q_stricmp( token, "description" ) )
		{
			PARSE(text, token);
			if ( !Q_stricmp( token, "null" ) )
			{
				ca->info = "";
			}
			else
			{
				ca->info = BG_strdup(token);
			}
			defined |= INFO;
		}
		else if ( !Q_stricmp( token, "icon" ) )
		{
			PARSE(text, token);

			if ( !Q_stricmp( token, "null" ) )
			{
				ca->icon = nullptr;
			}
			else
			{
				ca->icon = BG_strdup(token);
			}

			defined |= ICON;
		}
		else if ( !Q_stricmp( token, "fovCvar" ) )
		{
			PARSE(text, token);
			if ( !Q_stricmp( token, "null" ) )
			{
				ca->fovCvar = "";
			}
			else
			{
				ca->fovCvar = BG_strdup(token);
			}
			defined |= FOVCVAR;
		}
		else if ( !Q_stricmp( token, "team" ) )
		{
			PARSE(text, token);
			ca->team = ParseTeam( token );
			defined |= TEAM;
		}
		else if ( !Q_stricmp( token, "health" ) )
		{
			PARSE(text, token);
			ca->health = atoi( token );
			defined |= HEALTH;
		}
		else if ( !Q_stricmp( token, "fallDamage" ) )
		{
			PARSE(text, token);
			ca->fallDamage = atof( token );
			defined |= FALLDAMAGE;
		}
		else if ( !Q_stricmp( token, "regen" ) )
		{
			PARSE(text, token);
			ca->regenRate = atof( token );
			defined |= REGEN;
		}
		else if ( !Q_stricmp( token, "wallClimber" ) )
		{
			ca->abilities |= SCA_WALLCLIMBER;
		}
		else if ( !Q_stricmp( token, "takesFallDamage" ) )
		{
			ca->abilities |= SCA_TAKESFALLDAMAGE;
		}
		else if ( !Q_stricmp( token, "fovWarps" ) )
		{
			ca->abilities |= SCA_FOVWARPS;
		}
		else if ( !Q_stricmp( token, "alienSense" ) )
		{
			ca->abilities |= SCA_ALIENSENSE;
		}
		else if ( !Q_stricmp( token, "canUseLadders" ) )
		{
			ca->abilities |= SCA_CANUSELADDERS;
		}
		else if ( !Q_stricmp( token, "wallJumper" ) )
		{
			ca->abilities |= SCA_WALLJUMPER;
		}
		else if ( !Q_stricmp( token, "buildDistance" ) )
		{
			PARSE(text, token);
			ca->buildDist = atof( token );
		}
		else if ( !Q_stricmp( token, "fov" ) )
		{
			PARSE(text, token);
			ca->fov = atoi( token );
			defined |= FOV;
		}
		else if ( !Q_stricmp( token, "bob" ) )
		{
			PARSE(text, token);
			ca->bob = atof( token );
		}
		else if ( !Q_stricmp( token, "bobCycle" ) )
		{
			PARSE(text, token);
			ca->bobCycle = atof( token );
		}
		else if ( !Q_stricmp( token, "stepTime" ) )
		{
			PARSE(text, token);
			ca->steptime = atoi( token );
			defined |= STEPTIME;
		}
		else if ( !Q_stricmp( token, "speed" ) )
		{
			PARSE(text, token);
			ca->speed = atof( token );
			defined |= SPEED;
		}
		else if ( !Q_stricmp( token, "acceleration" ) )
		{
			PARSE(text, token);
			ca->acceleration = atof( token );
			defined |= ACCELERATION;
		}
		else if ( !Q_stricmp( token, "airAcceleration" ) )
		{
			PARSE(text, token);
			ca->airAcceleration = atof( token );
			defined |= AIRACCELERATION;
		}
		else if ( !Q_stricmp( token, "friction" ) )
		{
			PARSE(text, token);
			ca->friction = atof( token );
			defined |= FRICTION;
		}
		else if ( !Q_stricmp( token, "stopSpeed" ) )
		{
			PARSE(text, token);
			ca->stopSpeed = atof( token );
			defined |= STOPSPEED;
		}
		else if ( !Q_stricmp( token, "jumpMagnitude" ) )
		{
			PARSE(text, token);
			ca->jumpMagnitude = atof( token );
			defined |= JUMPMAGNITUDE;
		}
		else if ( !Q_stricmp( token, "cost" ) )
		{
			PARSE(text, token);
			ca->cost = atoi( token );
			defined |= COST;
		}
		else if ( !Q_stricmp( token, "sprintMod" ) )
		{
			PARSE(text, token);
			ca->sprintMod = atof( token );
			defined |= SPRINTMOD;
		}
		else if ( !Q_stricmp( token, "unlockThreshold" ) )
		{
			PARSE(text, token);
			ca->unlockThreshold = atoi(token);
			defined |= UNLOCKTHRESHOLD;
		}
		else if( (var = BG_FindConfigVar( va( "c_%s_%s", ca->name, token ) ) ) != nullptr )
		{
			BG_ParseConfigVar( var, &text, filename );
		}
		else if ( !Q_stricmp( token, "mass" ) )
		{
			PARSE(text, token);
			ca->mass = atoi( token );
			defined |= MASS;
		}
		else if ( !Q_stricmp( token, "staminaJumpCost" ) )
		{
			PARSE( text, token );
			ca->staminaJumpCost = atoi( token );
			defined |= STAMINAJUMPCOST;
		}
		else if ( !Q_stricmp( token, "staminaSprintCost" ) )
		{
			PARSE( text, token );
			ca->staminaSprintCost = atoi( token );
			defined |= STAMINASPRINTCOST;
		}
		else if ( !Q_stricmp( token, "staminaJogRestore" ) )
		{
			PARSE( text, token );
			ca->staminaJogRestore = atoi( token );
			defined |= STAMINAJOGRESTORE;
		}
		else if ( !Q_stricmp( token, "staminaWalkRestore" ) )
		{
			PARSE( text, token );
			ca->staminaWalkRestore = atoi( token );
			defined |= STAMINAWALKRESTORE;
		}
		else if ( !Q_stricmp( token, "staminaStopRestore" ) )
		{
			PARSE( text, token );
			ca->staminaStopRestore = atoi( token );
			defined |= STAMINASTOPRESTORE;
		}
		else
		{
			Com_Printf( S_ERROR "%s: unknown token '%s'\n", filename, token );
		}
	}

	// check for missing mandatory fields
	{
		if      ( !( defined & INFO ) )            { token = "description"; }
		else if ( !( defined & FOVCVAR ) )         { token = "fovCvar"; }
		else if ( !( defined & TEAM ) )            { token = "team"; }
		else if ( !( defined & HEALTH ) )          { token = "health"; }
		else if ( !( defined & FALLDAMAGE ) )      { token = "fallDamage"; }
		else if ( !( defined & REGEN ) )           { token = "regen"; }
		else if ( !( defined & FOV ) )             { token = "fov"; }
		else if ( !( defined & STEPTIME ) )        { token = "stepTime"; }
		else if ( !( defined & SPEED ) )           { token = "speed"; }
		else if ( !( defined & ACCELERATION ) )    { token = "acceleration"; }
		else if ( !( defined & AIRACCELERATION ) ) { token = "airAcceleration"; }
		else if ( !( defined & FRICTION ) )        { token = "friction"; }
		else if ( !( defined & STOPSPEED ) )       { token = "stopSpeed"; }
		else if ( !( defined & JUMPMAGNITUDE ) )   { token = "jumpMagnitude"; }
		else if ( !( defined & ICON ) )            { token = "icon"; }
		else if ( !( defined & COST ) )            { token = "cost"; }
		else                                       { token = nullptr; }

		if ( token )
		{
			Com_Printf( S_ERROR "%s not defined in %s\n", token, filename );
		}
	}

	// check for missing mandatory fields for the human team
	if ( ca->team == TEAM_HUMANS )
	{
		if      ( !( defined & SPRINTMOD ) )          { token = "sprintMod"; }
		else if ( !( defined & STAMINAJUMPCOST ) )    { token = "staminaJumpCost"; }
		else if ( !( defined & STAMINASPRINTCOST ) )  { token = "staminaSprintCost"; }
		else if ( !( defined & STAMINAJOGRESTORE ) )  { token = "staminaJogRestore"; }
		else if ( !( defined & STAMINAWALKRESTORE ) ) { token = "staminaWalkRestore"; }
		else if ( !( defined & STAMINASTOPRESTORE ) ) { token = "staminaStopRestore"; }
		else                                          { token = nullptr; }

		if ( token )
		{
			Com_Printf( S_ERROR "%s (mandatory for human team) not defined in %s\n",
			            token, filename );
		}
	}
}

/*
======================
BG_NonSegModel

Reads an animation.cfg to check for nonsegmentation
======================
*/
bool BG_NonSegModel( const char *filename )
{
	const char *text_p;
	char *token;
	char text[ 20000 ];

	if ( !BG_ReadWholeFile( filename, text, sizeof( text ) ) )
	{
		return false;
	}

	// parse the text
	text_p = text;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse( &text_p );

		//EOF
		if ( !token[ 0 ] )
		{
			break;
		}

		if ( !Q_stricmp( token, "nonsegmented" ) )
		{
			return true;
		}
	}

	return false;
}

/*
======================
BG_ParseClassModelFile

Parses a configuration file describing the model of a class
======================
*/
void BG_ParseClassModelFile( const char *filename, classModelConfig_t *cc )
{
	const char *token;
	char text_buffer[ 20000 ];
	const char* text;
	int defined = 0;
	enum
	{
		MODEL = 1 << 0,
		SKIN = 1 << 1,
		HUD = 1 << 2,
		MODELSCALE = 1 << 3,
		SHADOWSCALE = 1 << 4,
		MINS = 1 << 5,
		MAXS = 1 << 6,
		DEADMINS = 1 << 7,
		DEADMAXS = 1 << 8,
		CROUCHMAXS = 1 << 9,
		VIEWHEIGHT = 1 << 10,
		CVIEWHEIGHT = 1 << 11,
		ZOFFSET = 1 << 12,
		NAME = 1 << 13,
		SHOULDEROFFSETS = 1 << 14
	};

	if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
	{
		return;
	}

	text = text_buffer;

	// read optional parameters
	while ( 1 )
	{
		PARSE(text, token);

		if ( !Q_stricmp( token, "model" ) )
		{
			PARSE(text, token);

			//Allow spectator to have an empty model
			if ( !Q_stricmp( token, "null" ) )
			{
				cc->modelName[0] = '\0';
			}
			else
			{
				Q_strncpyz( cc->modelName, token, sizeof( cc->modelName ) );
			}

			defined |= MODEL;
		}
		else if ( !Q_stricmp( token, "skin" ) )
		{
			PARSE(text, token);

			if ( !Q_stricmp( token, "null" ) )
			{
				cc->skinName[0] = '\0';
			}
			else
			{
				Q_strncpyz( cc->skinName, token, sizeof( cc->skinName ) );
			}

			defined |= SKIN;
		}
		else if ( !Q_stricmp( token, "hud" ) )
		{
			PARSE(text, token);

			if ( !Q_stricmp( token, "null" ) )
			{
				cc->hudName[0] = '\0';
			}
			else
			{
				Q_strncpyz( cc->hudName, token, sizeof( cc->hudName ) );
			}

			defined |= HUD;
		}
		else if ( !Q_stricmp( token, "modelScale" ) )
		{
			float scale;

			PARSE(text, token);

			scale = atof( token );

			if ( scale < 0.0f )
			{
				scale = 0.0f;
			}

			cc->modelScale = scale;

			defined |= MODELSCALE;
		}
		else if ( !Q_stricmp( token, "shadowScale" ) )
		{
			float scale;

			PARSE(text, token);

			scale = atof( token );

			if ( scale < 0.0f )
			{
				scale = 0.0f;
			}

			cc->shadowScale = scale;

			defined |= SHADOWSCALE;
		}
		else if ( !Q_stricmp( token, "mins" ) )
		{
			int i;

			for ( i = 0; i <= 2; i++ )
			{
				PARSE(text, token);

				cc->mins[ i ] = atof( token );
			}

			defined |= MINS;
		}
		else if ( !Q_stricmp( token, "maxs" ) )
		{
			int i;

			for ( i = 0; i <= 2; i++ )
			{
				PARSE(text, token);

				cc->maxs[ i ] = atof( token );
			}

			defined |= MAXS;
		}
		else if ( !Q_stricmp( token, "deadMins" ) )
		{
			int i;

			for ( i = 0; i <= 2; i++ )
			{
				PARSE(text, token);

				cc->deadMins[ i ] = atof( token );
			}

			defined |= DEADMINS;
		}
		else if ( !Q_stricmp( token, "deadMaxs" ) )
		{
			int i;

			for ( i = 0; i <= 2; i++ )
			{
				PARSE(text, token);

				cc->deadMaxs[ i ] = atof( token );
			}

			defined |= DEADMAXS;
		}
		else if ( !Q_stricmp( token, "crouchMaxs" ) )
		{
			int i;

			for ( i = 0; i <= 2; i++ )
			{
				PARSE(text, token);

				cc->crouchMaxs[ i ] = atof( token );
			}

			defined |= CROUCHMAXS;
		}
		else if ( !Q_stricmp( token, "viewheight" ) )
		{
			PARSE(text, token);

			cc->viewheight = atoi( token );

			defined |= VIEWHEIGHT;
		}
		else if ( !Q_stricmp( token, "crouchViewheight" ) )
		{
			PARSE(text, token);

			cc->crouchViewheight = atoi( token );

			defined |= CVIEWHEIGHT;
		}
		else if ( !Q_stricmp( token, "zOffset" ) )
		{
			PARSE(text, token);

			cc->zOffset = atof( token );

			defined |= ZOFFSET;
		}
		else if ( !Q_stricmp( token, "name" ) )
		{
			PARSE(text, token);

			cc->humanName = BG_strdup( token );

			defined |= NAME;
		}
		else if ( !Q_stricmp( token, "shoulderOffsets" ) )
		{
			int i;

			for ( i = 0; i <= 2; i++ )
			{
				PARSE(text, token);

				cc->shoulderOffsets[ i ] = atof( token );
			}

			defined |= SHOULDEROFFSETS;
		}
		else if ( !Q_stricmp( token, "useNavMesh" ) )
		{
			const classModelConfig_t *model;

			PARSE(text, token);

			model = BG_ClassModelConfigByName( token );

			if ( model && *model->modelName )
			{
				cc->navMeshClass = (class_t) ( model - BG_ClassModelConfig( PCL_NONE ) );
			}
			else
			{
				Com_Printf( S_ERROR "%s: unknown or yet-unloaded player model '%s'\n", filename, token );
			}
		}
		else
		{
			Com_Printf( S_ERROR "%s: unknown token '%s'\n", filename, token );
		}
	}

	if ( !( defined & MODEL ) ) { token = "model"; }
	else if ( !( defined & SKIN ) ) { token = "skin"; }
	else if ( !( defined & HUD ) ) { token = "hud"; }
	else if ( !( defined & MODELSCALE ) ) { token = "modelScale"; }
	else if ( !( defined & SHADOWSCALE ) ) { token = "shadowScale"; }
	else if ( !( defined & MINS ) ) { token = "mins"; }
	else if ( !( defined & MAXS ) ) { token = "maxs"; }
	else if ( !( defined & DEADMINS ) ) { token = "deadMins"; }
	else if ( !( defined & DEADMAXS ) ) { token = "deadMaxs"; }
	else if ( !( defined & CROUCHMAXS ) ) { token = "crouchMaxs"; }
	else if ( !( defined & VIEWHEIGHT ) ) { token = "viewheight"; }
	else if ( !( defined & CVIEWHEIGHT ) ) { token = "crouchViewheight"; }
	else if ( !( defined & ZOFFSET ) ) { token = "zOffset"; }
	else if ( !( defined & NAME ) ) { token = "name"; }
	else if ( !( defined & SHOULDEROFFSETS ) ) { token = "shoulderOffsets"; }
	else { token = ""; }

	if ( strlen( token ) > 0 )
	{
		Com_Printf( S_ERROR "%s not defined in %s\n", token, filename );
	}
}

/*
======================
BG_ParseWeaponAttributeFile

Parses a configuration file describing the attributes of a weapon
======================
*/
void BG_ParseWeaponAttributeFile( const char *filename, weaponAttributes_t *wa )
{
	const char *token;
	char text_buffer[ 20000 ];
	const char* text;
	configVar_t* var;
	int defined = 0;
	enum
	{
		NAME = 1 << 0,
		INFO = 1 << 1,
		// unused
		PRICE = 1 << 3,
		RATE = 1 << 4,
		AMMO = 1 << 5,
		TEAM = 1 << 6,
		UNLOCKTHRESHOLD = 1 << 7
	};

	if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
	{
		return;
	}

	text = text_buffer;

	// read optional parameters
	while ( 1 )
	{
		PARSE(text, token);

		if ( !Q_stricmp( token, "humanName" ) )
		{
			PARSE(text, token);

			wa->humanName = BG_strdup( token );

			defined |= NAME;
		}
		else if ( !Q_stricmp( token, "description" ) )
		{
			PARSE(text, token);

			if ( !Q_stricmp( token, "null" ) )
			{
				wa->info = "";
			}
			else
			{
				wa->info = BG_strdup(token);
			}

			defined |= INFO;
		}
		else if ( !Q_stricmp( token, "usedSlots" ) )
		{
			wa->slots = ParseSlotList( &text );
		}
		else if ( !Q_stricmp( token, "price" ) )
		{
			PARSE(text, token);

			wa->price = atoi( token );

			defined |= PRICE;
		}
		else if ( !Q_stricmp( token, "infiniteAmmo" ) )
		{
			wa->infiniteAmmo = true;

			defined |= AMMO;
		}
		else if ( !Q_stricmp( token, "maxAmmo" ) )
		{
			PARSE(text, token);

			wa->maxAmmo = atoi( token );

			defined |= AMMO;
		}
		else if ( !Q_stricmp( token, "maxClips" ) )
		{
			PARSE(text, token);

			wa->maxClips = atoi( token );
		}
		else if ( !Q_stricmp( token, "usesEnergy" ) )
		{
			wa->usesEnergy = true;
		}
		else if ( !Q_stricmp( token, "primaryAttackRate" ) )
		{
			PARSE(text, token);

			wa->repeatRate1 = atoi( token );

			defined |= RATE;
		}
		else if ( !Q_stricmp( token, "secondaryAttackRate" ) )
		{
			PARSE(text, token);

			wa->repeatRate2 = atoi( token );
		}
		else if ( !Q_stricmp( token, "tertiaryAttackRate" ) )
		{
			PARSE(text, token);

			wa->repeatRate3 = atoi( token );
		}
		else if ( !Q_stricmp( token, "reloadTime" ) )
		{
			PARSE(text, token);

			wa->reloadTime = atoi( token );
		}
		else if ( !Q_stricmp( token, "knockbackScale" ) )
		{
			PARSE(text, token);

			wa->knockbackScale = atof( token );
		}
		else if ( !Q_stricmp( token, "hasAltMode" ) )
		{
			wa->hasAltMode = true;
		}
		else if ( !Q_stricmp( token, "hasThirdMode" ) )
		{
			wa->hasThirdMode = true;
		}
		else if ( !Q_stricmp( token, "isPurchasable" ) )
		{
			wa->purchasable = true;
		}
		else if ( !Q_stricmp( token, "isLongRanged" ) )
		{
			wa->longRanged = true;
		}
		else if ( !Q_stricmp( token, "canZoom" ) )
		{
			wa->canZoom = true;
		}
		else if ( !Q_stricmp( token, "zoomFov" ) )
		{
			PARSE(text, token);

			wa->zoomFov = atof( token );
		}
		else if ( !Q_stricmp( token, "team" ) )
		{
			PARSE(text, token);

			wa->team = ParseTeam( token );

			defined |= TEAM;
		}
		else if ( !Q_stricmp( token, "unlockThreshold" ) )
		{
			PARSE(text, token);

			wa->unlockThreshold = atoi(token);
			defined |= UNLOCKTHRESHOLD;
		}
		else if( (var = BG_FindConfigVar( va( "w_%s_%s", wa->name, token ) ) ) != nullptr )
		{
			BG_ParseConfigVar( var, &text, filename );
		}
		else
		{
			Com_Printf( S_ERROR "%s: unknown token '%s'\n", filename, token );
		}
	}

	if ( !( defined & NAME ) ) { token = "humanName"; }
	else if ( !( defined & INFO ) ) { token = "description"; }
	else if ( !( defined & PRICE ) ) { token = "price"; }
	else if ( !( defined & RATE ) ) { token = "primaryAttackRate"; }
	else if ( !( defined & AMMO ) ) { token = "maxAmmo or infiniteAmmo"; }
	else if ( !( defined & TEAM ) ) { token = "team"; }
	else { token = ""; }

	if ( strlen( token ) > 0 )
	{
		Com_Printf( S_ERROR "%s not defined in %s\n", token, filename );
	}
}

/*
======================
BG_ParseUpgradeAttributeFile

Parses a configuration file describing the attributes of an upgrade
======================
*/
void BG_ParseUpgradeAttributeFile( const char *filename, upgradeAttributes_t *ua )
{
	const char *token;
	char text_buffer[ 20000 ];
	const char* text;
	configVar_t* var;
	int defined = 0;
	enum
	{
		NAME = 1 << 0,
		PRICE = 1 << 1,
		INFO = 1 << 2,
		// unused
		ICON = 1 << 4,
		TEAM = 1 << 5,
		UNLOCKTHRESHOLD = 1 << 6
	};

	if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
	{
		return;
	}

	text = text_buffer;

	// read optional parameters
	while ( 1 )
	{
		PARSE(text, token);

		if ( !Q_stricmp( token, "humanName" ) )
		{
			PARSE(text, token);

			ua->humanName = BG_strdup( token );

			defined |= NAME;
		}
		else if ( !Q_stricmp( token, "description" ) )
		{
			PARSE(text, token);

			if ( !Q_stricmp( token, "null" ) )
			{
				ua->info = "";
			}
			else
			{
				ua->info = BG_strdup(token);
			}

			defined |= INFO;
		}
		else if ( !Q_stricmp( token, "usedSlots" ) )
		{
			ua->slots = ParseSlotList( &text );
		}
		else if ( !Q_stricmp( token, "icon" ) )
		{
			PARSE(text, token);

			if ( !Q_stricmp( token, "null" ) )
			{
				ua->icon = nullptr;
			}
			else
			{
				ua->icon = BG_strdup(token);
			}

			defined |= ICON;
		}
		else if ( !Q_stricmp( token, "price" ) )
		{
			PARSE(text, token);

			ua->price = atoi( token );

			defined |= PRICE;
		}
		else if ( !Q_stricmp( token, "team" ) )
		{
			PARSE(text, token);

			ua->team = ParseTeam( token );

			defined |= TEAM;
		}
		else if ( !Q_stricmp( token, "isPurchasable" ) )
		{
			ua->purchasable = true;
		}
		else if ( !Q_stricmp( token, "isUsable" ) )
		{
			ua->usable = true;
		}
		else if ( !Q_stricmp( token, "unlockThreshold" ) )
		{
			PARSE(text, token);

			ua->unlockThreshold = atoi(token);
			defined |= UNLOCKTHRESHOLD;
		}
		else if( (var = BG_FindConfigVar( va( "u_%s_%s", ua->name, token ) ) ) != nullptr )
		{
			BG_ParseConfigVar( var, &text, filename );
		}
		else
		{
			Com_Printf( S_ERROR "%s: unknown token '%s'\n", filename, token );
		}
	}

	if ( !( defined & NAME ) ) { token = "humanName"; }
	else if ( !( defined & INFO ) ) { token = "description"; }
	else if ( !( defined & PRICE ) ) { token = "price"; }
	else if ( !( defined & ICON ) ) { token = "icon"; }
	else if ( !( defined & TEAM ) ) { token = "team"; }
	else { token = ""; }

	if ( strlen( token ) > 0 )
	{
		Com_Printf( S_ERROR "%s not defined in %s\n", token, filename );
	}
}


/*
======================
BG_ParseMissileAttributeFile

Parses a configuration file describing the attributes of a missile.
Can be the same as the display configuration file.
======================
*/
void BG_ParseMissileAttributeFile( const char *filename, missileAttributes_t *ma )
{
	const char *token;
	char        text_buffer[ 20000 ];
	const char        *text;
	//configVar_t *var;
	int         defined = 0;

	enum
	{
		POINT_AGAINST_WORLD   = 1 <<  0,
		DAMAGE                = 1 <<  1,
		MEANS_OF_DEATH        = 1 <<  2,
		SPLASH_DAMAGE         = 1 <<  3,
		SPLASH_RADIUS         = 1 <<  4,
		SPLASH_MEANS_OF_DEATH = 1 <<  5,
		CLIPMASK              = 1 <<  6,
		SIZE                  = 1 <<  7,
		TRAJECTORY            = 1 <<  8,
		SPEED                 = 1 <<  9,
		LAG                   = 1 << 10,
		BOUNCE_FULL           = 1 << 11,
		BOUNCE_HALF           = 1 << 12,
		BOUNCE_NO_SOUND       = 1 << 13,
		KNOCKBACK             = 1 << 14,
		LOCATIONAL_DAMAGE     = 1 << 15
	};

	if( !BG_ReadWholeFile( filename, text_buffer, sizeof(text_buffer) ) )
	{
		return;
	}

	text = text_buffer;

	while ( 1 )
	{
		PARSE( text, token );

		if      ( !Q_stricmp( token, "pointAgainstWorld" ) )
		{
			ma->pointAgainstWorld = true;
			defined |= POINT_AGAINST_WORLD;
		}
		else if ( !Q_stricmp( token, "damage" ) )
		{
			PARSE( text, token );
			ma->damage = atoi( token );
			defined |= DAMAGE;
		}
		else if ( !Q_stricmp( token, "meansOfDeath" ) )
		{
			PARSE( text, token );
			ma->meansOfDeath = BG_MeansOfDeathByName( token );
			defined |= MEANS_OF_DEATH;
		}
		else if ( !Q_stricmp( token, "splashDamage" ) )
		{
			PARSE( text, token );
			ma->splashDamage = atoi( token );
			defined |= SPLASH_DAMAGE;
		}
		else if ( !Q_stricmp( token, "splashRadius" ) )
		{
			PARSE( text, token );
			ma->splashRadius = atoi( token );
			defined |= SPLASH_RADIUS;
		}
		else if ( !Q_stricmp( token, "splashMeansOfDeath" ) )
		{
			PARSE( text, token );
			ma->splashMeansOfDeath = BG_MeansOfDeathByName( token );
			defined |= SPLASH_MEANS_OF_DEATH;
		}
		else if ( !Q_stricmp( token, "clipmask" ) )
		{
			PARSE( text, token );
			ma->clipmask = ParseClipmask( token );
			defined |= CLIPMASK;
		}
		else if ( !Q_stricmp( token, "size" ) )
		{
			PARSE( text, token );
			ma->size = atoi( token );
			defined |= SIZE;
		}
		else if ( !Q_stricmp( token, "trajectory" ) )
		{
			PARSE( text, token );
			ma->trajectoryType = ParseTrajectoryType( token );
			defined |= TRAJECTORY;
		}
		else if ( !Q_stricmp( token, "speed" ) )
		{
			PARSE( text, token );
			ma->speed = atoi( token );
			defined |= SPEED;
		}
		else if ( !Q_stricmp( token, "lag" ) )
		{
			PARSE( text, token );
			ma->lag = atof( token );
			defined |= LAG;
		}
		else if ( !Q_stricmp( token, "bounceFull" ) )
		{
			ma->flags |= EF_BOUNCE;
			defined |= BOUNCE_FULL;
		}
		else if ( !Q_stricmp( token, "bounceHalf" ) )
		{
			ma->flags |= EF_BOUNCE_HALF;
			defined |= BOUNCE_HALF;
		}
		else if ( !Q_stricmp( token, "bounceNoSound" ) )
		{
			ma->flags |= EF_NO_BOUNCE_SOUND;
			defined |= BOUNCE_NO_SOUND;
		}
		else if ( !Q_stricmp( token, "doKnockback" ) )
		{
			ma->doKnockback = true;
			defined |= KNOCKBACK;
		}
		else if ( !Q_stricmp( token, "doLocationalDamage" ) )
		{
			ma->doLocationalDamage = true;
			defined |= LOCATIONAL_DAMAGE;
		}
		/*else if( (var = BG_FindConfigVar( va( "m_%s_%s", ma->name, token ) ) ) != nullptr )
		{
			BG_ParseConfigVar( var, &text, filename );
		}*/
		/*else
		{
			Com_Printf( S_WARNING "%s: unknown token '%s'\n", filename, token );
		}*/
	}

	if      ( !( defined & DAMAGE ) )         { token = "damage"; }
	else if ( !( defined & MEANS_OF_DEATH ) ) { token = "meansOfDeath"; }
	else if ( !( defined & CLIPMASK ) )       { token = "clipmask"; }
	else if ( !( defined & SIZE ) )           { token = "size"; }
	else if ( !( defined & TRAJECTORY ) )     { token = "trajectory"; }
	else if ( !( defined & SPEED ) )          { token = "speed"; }
	else                                      { token = ""; }

	if ( strlen( token ) > 0 )
	{
		Com_Printf( S_ERROR "%s not defined in %s\n", token, filename );
	}
}

/*
======================
BG_ParseMissileDisplayFile

Parses a configuration file describing the looks of a missile and its impact.
Can be the same as the attribute configuration file.
======================
*/
void BG_ParseMissileDisplayFile( const char *filename, missileAttributes_t *ma )
{
	char        *token;
	char        text_buffer[ 20000 ];
	const char        *text;
	int         defined = 0;
#ifdef BUILD_CGAME
	int         index;
#endif

	enum
	{
		MODEL            = 1 <<  0,
		SOUND            = 1 <<  1,
		DLIGHT           = 1 <<  2,
		DLIGHT_INTENSITY = 1 <<  3,
		DLIGHT_COLOR     = 1 <<  4,
		RENDERFX         = 1 <<  5,
		SPRITE           = 1 <<  6,
		SPRITE_SIZE      = 1 <<  7,
		SPRITE_CHARGE    = 1 <<  8,
		PARTICLE_SYSTEM  = 1 <<  9,
		TRAIL_SYSTEM     = 1 << 10,
		ROTATES          = 1 << 11,
		ANIM_START_FRAME = 1 << 12,
		ANIM_NUM_FRAMES  = 1 << 13,
		ANIM_FRAME_RATE  = 1 << 14,
		ANIM_LOOPING     = 1 << 15,
		ALWAYS_IMPACT    = 1 << 16,
		IMPACT_PARTICLE  = 1 << 17,
		IMPACT_MARK      = 1 << 18,
		IMPACT_MARK_SIZE = 1 << 19,
		IMPACT_SOUND     = 1 << 20,
		IMPACT_FLESH_SND = 1 << 21,
		MODEL_SCALE      = 1 << 22,
		MODEL_ROTATION   = 1 << 23,
		IMPACT_FLIGHTDIR = 1 << 24
	};

	if( !BG_ReadWholeFile( filename, text_buffer, sizeof( text_buffer ) ) )
	{
		return;
	}

	text = text_buffer;

	while ( true )
	{
		PARSE( text, token );

		if      ( !Q_stricmp( token, "model" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			ma->model = trap_R_RegisterModel( token );
#endif
			defined |= MODEL;
		}
		else if ( !Q_stricmp( token, "modelScale" ) )
		{
			PARSE( text, token );

			ma->modelScale = atof( token );

			defined |= MODEL_SCALE;
		}
		else if ( !Q_stricmp( token, "modelRotation" ) )
		{
			PARSE( text, token );
			ma->modelRotation[ 0 ] = atof( token );
			PARSE( text, token );
			ma->modelRotation[ 1 ] = atof( token );
			PARSE( text, token );
			ma->modelRotation[ 2 ] = atof( token );

			defined |= MODEL_ROTATION;
		}
		else if ( !Q_stricmp( token, "sound" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			ma->sound = trap_S_RegisterSound( token, false );
#endif
			defined |= SOUND;
		}
		else if ( !Q_stricmp( token, "dlight" ) )
		{
			PARSE( text, token );
			ma->dlight = atof( token );
			defined |= DLIGHT;
			ma->usesDlight = true;
		}
		else if ( !Q_stricmp( token, "dlightIntensity" ) )
		{
			PARSE( text, token );
			ma->dlightIntensity = atof( token );
			defined |= DLIGHT_INTENSITY;
			ma->usesDlight = true;
		}
		else if ( !Q_stricmp( token, "dlightColor" ) )
		{
			PARSE( text, token );
			ma->dlightColor[ 0 ] = atof( token );
			PARSE( text, token );
			ma->dlightColor[ 1 ] = atof( token );
			PARSE( text, token );
			ma->dlightColor[ 2 ] = atof( token );
			defined |= DLIGHT_COLOR;
			ma->usesDlight = true;
		}
		else if ( !Q_stricmp( token, "renderfx" ) )
		{
			PARSE( text, token );
			ma->renderfx = atoi( token );
			defined |= RENDERFX;
		}
		else if ( !Q_stricmp( token, "sprite" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			ma->sprite = trap_R_RegisterShader( token, RSF_SPRITE );
#endif
			defined |= SPRITE;
			ma->usesSprite = true;
		}
		else if ( !Q_stricmp( token, "spriteSize" ) )
		{
			PARSE( text, token );
			ma->spriteSize = atoi( token );
			defined |= SPRITE_SIZE;
			ma->usesSprite = true;
		}
		else if ( !Q_stricmp( token, "spriteCharge" ) )
		{
			PARSE( text, token );
			ma->spriteCharge = atof( token );
			defined |= SPRITE_CHARGE;
			ma->usesSprite = true;
		}
		else if ( !Q_stricmp( token, "particleSystem" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			ma->particleSystem = CG_RegisterParticleSystem( token );
#endif
			defined |= PARTICLE_SYSTEM;
		}
		else if ( !Q_stricmp( token, "trailSystem" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			ma->trailSystem = CG_RegisterTrailSystem( token );
#endif
			defined |= TRAIL_SYSTEM;
		}
		else if ( !Q_stricmp( token, "rotates" ) )
		{
			ma->rotates = true;
			defined |= ROTATES;
		}
		else if ( !Q_stricmp( token, "animStartFrame" ) )
		{
			PARSE( text, token );
			ma->animStartFrame = atoi( token );
			defined |= ANIM_START_FRAME;
			ma->usesAnim = true;
		}
		else if ( !Q_stricmp( token, "animNumFrames" ) )
		{
			PARSE( text, token );
			ma->animNumFrames = atoi( token );
			defined |= ANIM_NUM_FRAMES;
			ma->usesAnim = true;
		}
		else if ( !Q_stricmp( token, "animFrameRate" ) )
		{
			PARSE( text, token );
			ma->animFrameRate = atoi( token );
			defined |= ANIM_FRAME_RATE;
			ma->usesAnim = true;
		}
		else if ( !Q_stricmp( token, "animLooping" ) )
		{
			ma->animLooping = true;
			defined |= ANIM_LOOPING;
			ma->usesAnim = true;
		}
		else if ( !Q_stricmp( token, "alwaysImpact" ) )
		{
			ma->alwaysImpact = true;
			defined |= ALWAYS_IMPACT;
		}
		else if ( !Q_stricmp( token, "impactParticleSystem" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			ma->impactParticleSystem = CG_RegisterParticleSystem( token );
#endif
			defined |= IMPACT_PARTICLE;
		}
		else if ( !Q_stricmp( token, "impactFlightDir" ) )
		{
			ma->impactFlightDirection = true;
			defined |= IMPACT_FLIGHTDIR;
		}
		else if ( !Q_stricmp( token, "impactMark" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			ma->impactMark = trap_R_RegisterShader( token, RSF_DEFAULT );
#endif
			defined |= IMPACT_MARK;
			ma->usesImpactMark = true;
		}
		else if ( !Q_stricmp( token, "impactMarkSize" ) )
		{
			PARSE( text, token );
			ma->impactMarkSize = atoi( token );
			defined |= IMPACT_MARK_SIZE;
			ma->usesImpactMark = true;
		}
		else if ( !Q_stricmp( token, "impactSound" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			index = atoi( token );
#endif
			PARSE( text, token );
#ifdef BUILD_CGAME
			if ( index >= 0 && index < 4 )
			{
				ma->impactSound[ index ] = trap_S_RegisterSound( token, false );
			}
#endif
			defined |= IMPACT_SOUND;
		}
		else if ( !Q_stricmp( token, "impactFleshSound" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			index = atoi( token );
#endif
			PARSE( text, token );
#ifdef BUILD_CGAME
			if ( index >= 0 && index < 4 )
			{
				ma->impactFleshSound[ index ] = trap_S_RegisterSound( token, false );
			}
#endif
			defined |= IMPACT_FLESH_SND;
		}
		/*else
		{
			Com_Printf( S_WARNING "%s: unknown token '%s'\n", filename, token );
		}*/
	}

	// check sprite data set for completeness
	if ( ma->usesSprite && (
	     !( defined & SPRITE        ) ||
	     !( defined & SPRITE_SIZE   ) ) )
	{
		ma->usesSprite = false;
		Com_Printf( S_ERROR "Not all mandatory sprite vars defined in %s\n", filename );
	}

	// check animation data set for completeness
	if ( ma->usesAnim && (
	     !( defined & ANIM_START_FRAME ) ||
	     !( defined & ANIM_NUM_FRAMES  ) ||
	     !( defined & ANIM_FRAME_RATE  ) ) )
	{
		ma->usesAnim = false;
		Com_Printf( S_ERROR "Not all mandatory animation vars defined in %s\n", filename );
	}

	// check dlight data set for completeness
	if ( ma->usesDlight && (
	     !( defined & DLIGHT           ) ||
	     !( defined & DLIGHT_INTENSITY ) ||
	     !( defined & DLIGHT_COLOR     ) ) )
	{
		ma->usesDlight = false;
		Com_Printf( S_ERROR "Not all mandatory dlight vars defined in %s\n", filename );
	}

	// check impactMark data set for completeness
	if ( ma->usesImpactMark && (
	     !( defined & IMPACT_MARK      ) ||
	     !( defined & IMPACT_MARK_SIZE ) ) )
	{
		ma->usesImpactMark = false;
		Com_Printf( S_ERROR "Not all mandatory impactMark vars defined in %s\n", filename );
	}

	// default values
	if ( !( defined & MODEL_SCALE ) ) ma->modelScale = 1.0f;
}

/*
======================
BG_ParseBeaconAttirubteFile

Parses a configuration file describing a beacon type
======================
*/
void BG_ParseBeaconAttributeFile( const char *filename, beaconAttributes_t *ba )
{
	char        *token;
	char        text_buffer[ 20000 ];
	const char        *text;
#ifdef BUILD_CGAME
	int         index;
#endif

	if( !BG_ReadWholeFile( filename, text_buffer, sizeof( text_buffer ) ) )
	{
		return;
	}

	text = text_buffer;

	while ( true )
	{
		PARSE( text, token );

		if      ( !Q_stricmp( token, "humanName" ) )
		{
			PARSE( text, token );
			ba->humanName = BG_strdup( token );
		}
		else if ( !Q_stricmp( token, "text" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			index = atoi( token );
#endif
			PARSE( text, token );
#ifdef BUILD_CGAME
			if( index < 0 || index >= 4 )
				Com_Printf( S_ERROR "Invalid beacon icon index %i in %s\n", index, filename );
			else
				ba->text[ index ] = BG_strdup( token );
#endif
		}
		else if ( !Q_stricmp( token, "desc" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			ba->desc = BG_strdup( token );
#endif
		}
		else if ( !Q_stricmp( token, "icon" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			index = atoi( token );
#endif
			PARSE( text, token );
#ifdef BUILD_CGAME
			if( index < 0 || index >= 4 )
				Com_Printf( S_ERROR "Invalid beacon icon index %i in %s\n", index, filename );
			else
				ba->icon[ 0 ][ index ] = trap_R_RegisterShader( token, RSF_DEFAULT );
#endif
		}
		else if ( !Q_stricmp( token, "hlIcon" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			index = atoi( token );
#endif
			PARSE( text, token );
#ifdef BUILD_CGAME
			if( index < 0 || index >= 4 )
				Com_Printf( S_ERROR "Invalid beacon highlighted icon index %i in %s\n", index, filename );
			else
				ba->icon[ 1 ][ index ] = trap_R_RegisterShader( token, RSF_DEFAULT );
#endif
		}
		else if ( !Q_stricmp( token, "inSound" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			ba->inSound = trap_S_RegisterSound( token, false );
#endif
		}
		else if ( !Q_stricmp( token, "outSound" ) )
		{
			PARSE( text, token );
#ifdef BUILD_CGAME
			ba->outSound = trap_S_RegisterSound( token, false );
#endif
		}
		else if ( !Q_stricmp( token, "decayTime" ) )
		{
			PARSE( text, token );
			ba->decayTime = atoi( token );
		}
	}
}
