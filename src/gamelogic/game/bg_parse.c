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

// bg_parser.c -- parsers for configuration files

#include "../../engine/qcommon/q_shared.h"
#include "bg_public.h"

int                                trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void                               trap_FS_Read( void *buffer, int len, fileHandle_t f );
void                               trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void                               trap_FS_FCloseFile( fileHandle_t f );

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
    qboolean defined;
    void *var;
} configVar_t;

//Definition of the config vars
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
float LEVEL1_GRAB_RANGE;
float LEVEL1_GRAB_U_RANGE;
int   LEVEL1_GRAB_TIME;
int   LEVEL1_GRAB_U_TIME;
float LEVEL1_PCLOUD_RANGE;
int   LEVEL1_PCLOUD_TIME;
float LEVEL1_REGEN_MOD;
float LEVEL1_UPG_REGEN_MOD;
int   LEVEL1_REGEN_SCOREINC;
int   LEVEL1_UPG_REGEN_SCOREINC;

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
float LEVEL4_CRUSH_DAMAGE_PER_V;
int   LEVEL4_CRUSH_DAMAGE;
int   LEVEL4_CRUSH_REPEAT;

int   LIGHTARMOUR_POISON_PROTECTION;
int   LIGHTARMOUR_PCLOUD_PROTECTION;

float HELMET_RANGE;
int   HELMET_POISON_PROTECTION;
int   HELMET_PCLOUD_PROTECTION;

float BATTPACK_MODIFIER;

float JETPACK_FLOAT_SPEED;
float JETPACK_SINK_SPEED;
int   JETPACK_DISABLE_TIME;
float JETPACK_DISABLE_CHANCE;

int   BSUIT_POISON_PROTECTION;
int   BSUIT_PCLOUD_PROTECTION;

int   MEDKIT_POISON_IMMUNITY_TIME;
int   MEDKIT_STARTUP_TIME;
int   MEDKIT_STARTUP_SPEED;

float REACTOR_BASESIZE;
float REACTOR_ATTACK_RANGE;
int   REACTOR_ATTACK_REPEAT;
int   REACTOR_ATTACK_DAMAGE;
float REACTOR_ATTACK_DCC_RANGE;
int   REACTOR_ATTACK_DCC_REPEAT;
int   REACTOR_ATTACK_DCC_DAMAGE;

float REPEATER_BASESIZE;

float MGTURRET_RANGE;
int   MGTURRET_ANGULARSPEED;
int   MGTURRET_ACCURACY_TO_FIRE;
int   MGTURRET_VERTICALCAP;
int   MGTURRET_SPREAD;
int   MGTURRET_DMG;
int   MGTURRET_SPINUP_TIME;

float TESLAGEN_RANGE;
int   TESLAGEN_REPEAT;
int   TESLAGEN_DMG;

int   DC_ATTACK_PERIOD;
int   DC_HEALRATE;
int   DC_RANGE;

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

int GRENADE_DAMAGE;
float GRENADE_RANGE;
float GRENADE_SPEED;

int SHOTGUN_PELLETS;
int SHOTGUN_SPREAD;
int SHOTGUN_DMG;
int SHOTGUN_RANGE;

int LASGUN_REPEAT;
float LASGUN_K_SCALE;
int LASGUN_DAMAGE;

int MDRIVER_DMG;
int MDRIVER_REPEAT;
float MDRIVER_K_SCALE;

int CHAINGUN_SPREAD;
int CHAINGUN_DMG;

int FLAMER_DMG;
int FLAMER_FLIGHTSPLASHDAMAGE;
int FLAMER_SPLASHDAMAGE;
int FLAMER_RADIUS;
int FLAMER_SIZE;
float FLAMER_LIFETIME;
float FLAMER_SPEED;
float FLAMER_LAG;

int PRIFLE_DMG;
int PRIFLE_SPEED;
int PRIFLE_SIZE;

int LCANNON_DAMAGE;
int LCANNON_RADIUS;
int LCANNON_SIZE;
int LCANNON_SECONDARY_DAMAGE;
int LCANNON_SECONDARY_RADIUS;
int LCANNON_SECONDARY_SPEED;
int LCANNON_SECONDARY_RELOAD;
int LCANNON_SPEED;
int LCANNON_CHARGE_TIME_MAX;
int LCANNON_CHARGE_TIME_MIN;
int LCANNON_CHARGE_TIME_WARN;
int LCANNON_CHARGE_AMMO;

static configVar_t bg_configVars[] =
{
	{"b_dcc_healRange", INTEGER, qfalse, &DC_RANGE},
	{"b_dcc_healRate", INTEGER, qfalse, &DC_HEALRATE},
	{"b_dcc_warningPeriod", INTEGER, qfalse, &DC_ATTACK_PERIOD},

	{"b_mgturret_accuracyToFire", INTEGER, qfalse, &MGTURRET_ACCURACY_TO_FIRE},
	{"b_mgturret_angularSpeed", INTEGER, qfalse, &MGTURRET_ANGULARSPEED},
	{"b_mgturret_attackDamage", INTEGER, qfalse, &MGTURRET_DMG},
	{"b_mgturret_attackSpread", INTEGER, qfalse, &MGTURRET_SPREAD},
	{"b_mgturret_fireRange", FLOAT, qfalse, &MGTURRET_RANGE},
	{"b_mgturret_spinupTime", INTEGER, qfalse, &MGTURRET_SPINUP_TIME},
	{"b_mgturret_verticalCap", INTEGER, qfalse, &MGTURRET_VERTICALCAP},

	{"b_reactor_powerRadius", FLOAT, qfalse, &REACTOR_BASESIZE},
	{"b_reactor_zapAttackDamage", INTEGER, qfalse, &REACTOR_ATTACK_DAMAGE},
	{"b_reactor_zapAttackDamageDCC", INTEGER, qfalse, &REACTOR_ATTACK_DCC_DAMAGE},
	{"b_reactor_zapAttackRange", FLOAT, qfalse, &REACTOR_ATTACK_RANGE},
	{"b_reactor_zapAttackRangeDCC", FLOAT, qfalse, &REACTOR_ATTACK_DCC_RANGE},
	{"b_reactor_zapAttackRepeat", INTEGER, qfalse, &REACTOR_ATTACK_REPEAT},
	{"b_reactor_zapAttackRepeatDCC", INTEGER, qfalse, &REACTOR_ATTACK_DCC_REPEAT},

	{"b_repeater_powerRadius", FLOAT, qfalse, &REPEATER_BASESIZE},

	{"b_tesla_zapAttackDamage", INTEGER, qfalse, &TESLAGEN_DMG},
	{"b_tesla_zapAttackRange", FLOAT, qfalse, &TESLAGEN_RANGE},
	{"b_tesla_zapAttackRepeat", INTEGER, qfalse, &TESLAGEN_REPEAT},

	{"u_battpack_ammoCapacityModifier", FLOAT, qfalse, &BATTPACK_MODIFIER},

	{"u_bsuit_poisonCloudProtection", INTEGER, qfalse, &BSUIT_POISON_PROTECTION},
	{"u_bsuit_poisonProtection", INTEGER, qfalse, &BSUIT_PCLOUD_PROTECTION},

	{"u_helmet_poisonCloudProtection", INTEGER, qfalse, &HELMET_POISON_PROTECTION},
	{"u_helmet_poisonProtection", INTEGER, qfalse, &HELMET_PCLOUD_PROTECTION},
	{"u_helmet_radarRange", FLOAT, qfalse, &HELMET_RANGE},

	{"u_jetpack_disableChance", FLOAT, qfalse, &JETPACK_DISABLE_CHANCE},
	{"u_jetpack_disableTime", INTEGER, qfalse, &JETPACK_DISABLE_TIME},
	{"u_jetpack_riseSpeed", FLOAT, qfalse, &JETPACK_FLOAT_SPEED},
	{"u_jetpack_sinkSpeed", FLOAT, qfalse, &JETPACK_SINK_SPEED},

	{"u_larmour_poisonCloudProtection", INTEGER, qfalse, &LIGHTARMOUR_POISON_PROTECTION},
	{"u_larmour_poisonProtection", INTEGER, qfalse, &LIGHTARMOUR_PCLOUD_PROTECTION},

	{"u_medkit_poisonImmunityTime", INTEGER, qfalse, &MEDKIT_POISON_IMMUNITY_TIME},
	{"u_medkit_startupSpeed", INTEGER, qfalse, &MEDKIT_STARTUP_SPEED},
	{"u_medkit_startupTime", INTEGER, qfalse, &MEDKIT_STARTUP_TIME},

	{"w_abuild_blobDmg", INTEGER, qfalse, &ABUILDER_BLOB_DMG},
	{"w_abuild_blobSlowTime", INTEGER, qfalse, &ABUILDER_BLOB_TIME},
	{"w_abuild_blobSpeed", FLOAT, qfalse, &ABUILDER_BLOB_SPEED},
	{"w_abuild_blobSpeedMod", FLOAT, qfalse, &ABUILDER_BLOB_SPEED_MOD},
	{"w_abuild_clawDmg", INTEGER, qfalse, &ABUILDER_CLAW_DMG},
	{"w_abuild_clawRange", FLOAT, qfalse, &ABUILDER_CLAW_RANGE},
	{"w_abuild_clawWidth", FLOAT, qfalse, &ABUILDER_CLAW_WIDTH},

	{"w_blaster_damage", INTEGER, qfalse, &BLASTER_DMG },
	{"w_blaster_size", INTEGER, qfalse, &BLASTER_SIZE },
	{"w_blaster_speed", INTEGER, qfalse, &BLASTER_SPEED },
	{"w_blaster_spread", INTEGER, qfalse, &BLASTER_SPREAD },

	{"w_chaingun_damage", INTEGER, qfalse, &CHAINGUN_DMG },
	{"w_chaingun_spread", INTEGER, qfalse, &CHAINGUN_SPREAD },

	{"w_flamer_damage", INTEGER, qfalse, &FLAMER_DMG },
	{"w_flamer_flightSplashDamage", INTEGER, qfalse, &FLAMER_FLIGHTSPLASHDAMAGE },
	{"w_flamer_lag", FLOAT, qfalse, &FLAMER_LAG },
	{"w_flamer_lifeTime", FLOAT, qfalse, &FLAMER_LIFETIME },
	{"w_flamer_radius", INTEGER, qfalse, &FLAMER_RADIUS },
	{"w_flamer_size", INTEGER, qfalse, &FLAMER_SIZE },
	{"w_flamer_speed", FLOAT, qfalse, &FLAMER_SPEED },
	{"w_flamer_splashDamage", INTEGER, qfalse, &FLAMER_SPLASHDAMAGE },

	{"w_grenade_damage", INTEGER, qfalse, &GRENADE_DAMAGE },
	{"w_grenade_range", FLOAT, qfalse, &GRENADE_RANGE },
	{"w_grenade_speed", FLOAT, qfalse, &GRENADE_SPEED },

	{"w_lcannon_chargeAmmo", INTEGER, qfalse, &LCANNON_CHARGE_AMMO },
	{"w_lcannon_chargeTimeMax", INTEGER, qfalse, &LCANNON_CHARGE_TIME_MAX },
	{"w_lcannon_chargeTimeMin", INTEGER, qfalse, &LCANNON_CHARGE_TIME_MIN },
	{"w_lcannon_chargeTimeWarn", INTEGER, qfalse, &LCANNON_CHARGE_TIME_WARN },
	{"w_lcannon_damage", INTEGER, qfalse, &LCANNON_DAMAGE },
	{"w_lcannon_radius", INTEGER, qfalse, &LCANNON_RADIUS },
	{"w_lcannon_secondaryDamage", INTEGER, qfalse, &LCANNON_SECONDARY_DAMAGE },
	{"w_lcannon_secondaryRadius", INTEGER, qfalse, &LCANNON_SECONDARY_RADIUS },
	{"w_lcannon_secondaryReload", INTEGER, qfalse, &LCANNON_SECONDARY_RELOAD },
	{"w_lcannon_secondarySpeed", INTEGER, qfalse, &LCANNON_SECONDARY_SPEED },
	{"w_lcannon_size", INTEGER, qfalse, &LCANNON_SIZE },
	{"w_lcannon_speed", INTEGER, qfalse, &LCANNON_SPEED },

	{"w_level0_biteDmg", INTEGER, qfalse, &LEVEL0_BITE_DMG},
	{"w_level0_biteRange", FLOAT, qfalse, &LEVEL0_BITE_RANGE},
	{"w_level0_biteRepeat", INTEGER, qfalse, &LEVEL0_BITE_REPEAT},
	{"w_level0_biteWidth", FLOAT, qfalse, &LEVEL0_BITE_WIDTH},

	{"w_level1upg_clawRange", INTEGER, qfalse, &LEVEL1_CLAW_U_RANGE},
	{"w_level1upg_grabRange", FLOAT, qfalse, &LEVEL1_GRAB_U_RANGE},
	{"w_level1upg_grabTime", INTEGER, qfalse, &LEVEL1_GRAB_U_TIME},
	{"w_level1upg_poisonCloudDuration", INTEGER, qfalse, &LEVEL1_PCLOUD_TIME},
	{"w_level1upg_poisonCloudRange", FLOAT, qfalse, &LEVEL1_PCLOUD_RANGE},
	{"w_level1upg_regenMod", FLOAT, qfalse, &LEVEL1_UPG_REGEN_MOD},
	{"w_level1upg_regenScoreGain", INTEGER, qfalse, &LEVEL1_UPG_REGEN_SCOREINC},

	{"w_level1_clawDmg", INTEGER, qfalse, &LEVEL1_CLAW_DMG},
	{"w_level1_clawRange", FLOAT, qfalse, &LEVEL1_CLAW_RANGE},
	{"w_level1_clawWidth", FLOAT, qfalse, &LEVEL1_CLAW_WIDTH},
	{"w_level1_grabRange", FLOAT, qfalse, &LEVEL1_GRAB_RANGE},
	{"w_level1_grabTime", INTEGER, qfalse, &LEVEL1_GRAB_TIME},
	{"w_level1_regenMod", FLOAT, qfalse, &LEVEL1_REGEN_MOD},
	{"w_level1_regenScoreGain", INTEGER, qfalse, &LEVEL1_REGEN_SCOREINC},

	{"w_level2upg_clawRange", INTEGER, qfalse, &LEVEL2_CLAW_U_RANGE},
	{"w_level2upg_zapChainFalloff", FLOAT, qfalse, &LEVEL2_AREAZAP_CHAIN_FALLOFF},
	{"w_level2upg_zapChainRange", FLOAT, qfalse, &LEVEL2_AREAZAP_CHAIN_RANGE},
	{"w_level2upg_zapDmg", INTEGER, qfalse, &LEVEL2_AREAZAP_DMG},
	{"w_level2upg_zapRange", FLOAT, qfalse, &LEVEL2_AREAZAP_RANGE},
	{"w_level2upg_zapTime", INTEGER, qfalse, &LEVEL2_AREAZAP_TIME},
	{"w_level2upg_zapWidth", FLOAT, qfalse, &LEVEL2_AREAZAP_WIDTH},

	{"w_level2_clawDmg", INTEGER, qfalse, &LEVEL2_CLAW_DMG},
	{"w_level2_clawRange", FLOAT, qfalse, &LEVEL2_CLAW_RANGE},
	{"w_level2_clawWidth", FLOAT, qfalse, &LEVEL2_CLAW_WIDTH},
	{"w_level2_maxWalljumpSpeed", FLOAT, qfalse, &LEVEL2_WALLJUMP_MAXSPEED},

	{"w_level3upg_ballDmg", INTEGER, qfalse, &LEVEL3_BOUNCEBALL_DMG},
	{"w_level3upg_ballRadius", INTEGER, qfalse, &LEVEL3_BOUNCEBALL_RADIUS},
	{"w_level3upg_ballRegen", INTEGER, qfalse, &LEVEL3_BOUNCEBALL_REGEN},
	{"w_level3upg_ballSpeed", FLOAT, qfalse, &LEVEL3_BOUNCEBALL_SPEED},
	{"w_level3upg_clawRange", FLOAT, qfalse, &LEVEL3_CLAW_UPG_RANGE},
	{"w_level3upg_pounceDuration", INTEGER, qfalse, &LEVEL3_POUNCE_TIME_UPG},
	{"w_level3upg_pounceJumpMagnitude", INTEGER, qfalse, &LEVEL3_POUNCE_JUMP_MAG_UPG},
	{"w_level3upg_pounceRange", FLOAT, qfalse, &LEVEL3_POUNCE_UPG_RANGE},

	{"w_level3_clawDmg", INTEGER, qfalse, &LEVEL3_CLAW_DMG},
	{"w_level3_clawRange", FLOAT, qfalse, &LEVEL3_CLAW_RANGE},
	{"w_level3_clawWidth", FLOAT, qfalse, &LEVEL3_CLAW_WIDTH},
	{"w_level3_pounceCancelTime", INTEGER, qfalse, &LEVEL3_POUNCE_TIME},
	{"w_level3_pounceDmg", INTEGER, qfalse, &LEVEL3_POUNCE_DMG},
	{"w_level3_pounceDuration", INTEGER, qfalse, &LEVEL3_POUNCE_TIME},
	{"w_level3_pounceJumpMagnitude", INTEGER, qfalse, &LEVEL3_POUNCE_JUMP_MAG},
	{"w_level3_pounceRange", FLOAT, qfalse, &LEVEL3_POUNCE_RANGE},
	{"w_level3_pounceRepeat", INTEGER, qfalse, &LEVEL3_POUNCE_REPEAT},
	{"w_level3_pounceSpeedMod", FLOAT, qfalse, &LEVEL3_POUNCE_SPEED_MOD},
	{"w_level3_pounceWidth", FLOAT, qfalse, &LEVEL3_POUNCE_WIDTH},

	{"w_level4_clawDmg", INTEGER, qfalse, &LEVEL4_CLAW_DMG},
	{"w_level4_clawHeight", FLOAT, qfalse, &LEVEL4_CLAW_HEIGHT},
	{"w_level4_clawRange", FLOAT, qfalse, &LEVEL4_CLAW_RANGE},
	{"w_level4_clawWidth", FLOAT, qfalse, &LEVEL4_CLAW_WIDTH},
	{"w_level4_crushDmg", INTEGER, qfalse, &LEVEL4_CRUSH_DAMAGE},
	{"w_level4_crushDmgPerFallingVelocity", FLOAT, qfalse, &LEVEL4_CRUSH_DAMAGE_PER_V},
	{"w_level4_crushRepeat", INTEGER, qfalse, &LEVEL4_CRUSH_REPEAT},
	{"w_level4_trampleChargeMax", INTEGER, qfalse, &LEVEL4_TRAMPLE_CHARGE_MAX},
	{"w_level4_trampleChargeMin", INTEGER, qfalse, &LEVEL4_TRAMPLE_CHARGE_MIN},
	{"w_level4_trampleChargeTrigger", INTEGER, qfalse, &LEVEL4_TRAMPLE_CHARGE_TRIGGER},
	{"w_level4_trampleDmg", INTEGER, qfalse, &LEVEL4_TRAMPLE_DMG},
	{"w_level4_trampleDuration", INTEGER, qfalse, &LEVEL4_TRAMPLE_DURATION},
	{"w_level4_trampleRepeat", INTEGER, qfalse, &LEVEL4_TRAMPLE_REPEAT},
	{"w_level4_trampleSpeed", FLOAT, qfalse, &LEVEL4_TRAMPLE_SPEED},
	{"w_level4_trampleStopPenalty", INTEGER, qfalse, &LEVEL4_TRAMPLE_STOP_PENALTY},

	{"w_lgun_damage", INTEGER, qfalse, &LASGUN_DAMAGE },
	{"w_lgun_k_scale", FLOAT, qfalse, &LASGUN_K_SCALE },
	{"w_lgun_repeat", INTEGER, qfalse, &LASGUN_REPEAT },

	{"w_mdriver_damage", INTEGER, qfalse, &MDRIVER_DMG },
	{"w_mdriver_k_scale", FLOAT, qfalse, &MDRIVER_K_SCALE },
	{"w_mdriver_repeat", INTEGER, qfalse, &MDRIVER_REPEAT },

	{"w_prifle_damage", INTEGER, qfalse, &PRIFLE_DMG },
	{"w_prifle_size", INTEGER, qfalse, &PRIFLE_SIZE },
	{"w_prifle_speed", INTEGER, qfalse, &PRIFLE_SPEED },

	{"w_psaw_damage", INTEGER, qfalse, &PAINSAW_DAMAGE },
	{"w_psaw_height", FLOAT, qfalse, &PAINSAW_HEIGHT },
	{"w_psaw_range", FLOAT, qfalse, &PAINSAW_RANGE },
	{"w_psaw_width", FLOAT, qfalse, &PAINSAW_WIDTH },

	{"w_rifle_damage", INTEGER, qfalse, &RIFLE_DMG },
	{"w_rifle_spread", INTEGER, qfalse, &RIFLE_SPREAD },

	{"w_shotgun_damage", INTEGER, qfalse, &SHOTGUN_DMG },
	{"w_shotgun_pellets", INTEGER, qfalse, &SHOTGUN_PELLETS },
	{"w_shotgun_range", INTEGER, qfalse, &SHOTGUN_RANGE },
	{"w_shotgun_spread", INTEGER, qfalse, &SHOTGUN_SPREAD },
};

static const size_t bg_numConfigVars = ARRAY_LEN( bg_configVars );

/*
======================
BG_ReadWholeFile

Helper function that tries to read a whole file in a buffer. Should it be in bg_parse.c?
======================
*/

qboolean BG_ReadWholeFile( const char *filename, char *buffer, int size)
{
    fileHandle_t f;
    int len;

    len = trap_FS_FOpenFile( filename, &f, FS_READ );

    if ( len < 0 )
    {
        Com_Printf( S_ERROR "file %s doesn't exist\n", filename );
        return qfalse;
    }

    if ( len == 0 || len >= size - 1 )
    {
        trap_FS_FCloseFile( f );
        Com_Printf( S_ERROR "file %s is %s\n", filename,
                    len == 0 ? "empty" : "too long" );
        return qfalse;
    }

    trap_FS_Read( buffer, len, f );
    buffer[ len ] = 0;
    trap_FS_FCloseFile( f );

    return qtrue;
}

static int BG_StageFromNumber( int i )
{
    if( i == 1 )
    {
        return (1 << S1) | (1 << S2) | (1 << S3);
    }
    if( i == 2 )
    {
        return (1 << S2) | (1 << S3);
    }
    if( i == 3 )
    {
        return 1 << S3;
    }

    return 0;
}

static int BG_ParseTeam(char* token)
{
    if ( !Q_stricmp( token, "aliens" ) )
    {
        return TEAM_ALIENS;
    }
    else if ( !Q_stricmp( token, "humans" ) )
    {
        return TEAM_HUMANS;
    }
    else
    {
        Com_Printf( S_ERROR "unknown team value '%s'\n", token );
        return -1;
    }
}

static int BG_ParseSlotList(char** text)
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
        else
        {
            Com_Printf( S_ERROR "unknown slot '%s'\n", token );
        }
    }

    return slots;
}

int configVarComparator(const void* a, const void* b)
{
    const configVar_t *ca = (const configVar_t*) a;
    const configVar_t *cb = (const configVar_t*) b;
    return Q_stricmp(ca->name, cb->name);
}

configVar_t* BG_FindConfigVar(const char *varName)
{
    return bsearch(&varName, bg_configVars, bg_numConfigVars, sizeof(configVar_t), configVarComparator);
}

qboolean BG_ParseConfigVar(configVar_t *var, char **text, const char *filename)
{
    char *token;

    token = COM_Parse( text );

    if( !*token )
    {
        Com_Printf( S_COLOR_RED "ERROR: %s expected argument for '%s'\n", filename, var->name );
        return qfalse;
    }

    if( var->type == INTEGER)
    {
        *((int*) var->var) = atoi( token );
    }
    else if( var->type == FLOAT)
    {
        *((float*) var->var) = atof( token );
    }

    var->defined = qtrue;

    return qtrue;
}

qboolean BG_CheckConfigVars( void )
{
    int i;
    int ok = qtrue;

    for( i = 0; i < bg_numConfigVars; i++)
    {
        if( !bg_configVars[i].defined )
        {
            ok = qfalse;
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
    char *token;
    char text_buffer[ 20000 ];
    char* text;
    configVar_t* var;
    int defined = 0;
    enum
    {
      HUMANNAME = 1 << 1,
      DESCRIPTION = 1 << 2,
      NORMAL = 1 << 3,
      BUILDPOINTS = 1 << 4,
      STAGE = 1 << 5,
      HEALTH = 1 << 6,
      DEATHMOD = 1 << 7,
      TEAM = 1 << 8,
      BUILDWEAPON = 1 << 9,
      BUILDTIME = 1 << 10,
      VALUE = 1 << 11,
      RADAR = 1 << 12,
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
        else if ( !Q_stricmp( token, "buildPoints" ) )
        {
            PARSE(text, token);

            ba->buildPoints = atoi(token);

            defined |= BUILDPOINTS;
        }
        else if ( !Q_stricmp( token, "stage" ) )
        {
            PARSE(text, token);

            //It depends heavily on the definition of S1 S2 S3
            ba->stages = BG_StageFromNumber( atoi(token) );

            defined |= STAGE;
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

            ba->team = BG_ParseTeam(token);

            defined |= TEAM;
        }
        else if ( !Q_stricmp( token, "buildWeapon" ) )
        {
            PARSE(text, token);

            if ( !Q_stricmp( token, "allAlien" ) )
            {
                ba->buildWeapon = ( 1 << WP_ABUILD ) | ( 1 << WP_ABUILD2 );
            }
            else if ( !Q_stricmp( token, "advAlien" ) )
            {
                ba->buildWeapon = ( 1 << WP_ABUILD2 );
            }
            else if ( !Q_stricmp( token, "human" ) )
            {
                ba->buildWeapon = ( 1 << WP_HBUILD );
            }
            else
            {
                Com_Printf( S_ERROR "unknown buildWeapon value '%s'\n", token );
            }

            defined |= BUILDWEAPON;
        }
        else if ( !Q_stricmp( token, "thinkPeriod" ) )
        {
            PARSE(text, token);

            ba->nextthink = atoi(token);
        }
        else if ( !Q_stricmp( token, "buildTime" ) )
        {
            PARSE(text, token);

            ba->buildTime = atoi(token);

            defined |= BUILDTIME;
        }
        else if ( !Q_stricmp( token, "usable" ) )
        {
            ba->usable = qtrue;
        }
        else if ( !Q_stricmp( token, "attackRange" ) )
        {
            PARSE(text, token);

            ba->turretRange = atoi(token);
        }
        else if ( !Q_stricmp( token, "attackSpeed" ) )
        {
            PARSE(text, token);

            ba->turretFireSpeed = atoi(token);
        }
        else if ( !Q_stricmp( token, "attackType" ) )
        {
            PARSE(text, token);

            if ( !Q_stricmp( token, "none" ) )
            {
                ba->turretProjType = WP_NONE;
            }
            else if ( !Q_stricmp( token, "lockBlob" ) )
            {
                ba->turretProjType = WP_LOCKBLOB_LAUNCHER;
            }
            else if ( !Q_stricmp( token, "hive" ) )
            {
                ba->turretProjType = WP_HIVE;
            }
            else if ( !Q_stricmp( token, "mgturret" ) )
            {
                ba->turretProjType = WP_MGTURRET;
            }
            else if ( !Q_stricmp( token, "tesla" ) )
            {
                ba->turretProjType = WP_TESLAGEN;
            }
            else
            {
                Com_Printf( S_ERROR "unknown attackType value '%s'\n", token );
            }
        }
        else if ( !Q_stricmp( token, "minNormal" ) )
        {
            PARSE(text, token);

            ba->minNormal = atof(token);
            defined |= NORMAL;
        }

        else if ( !Q_stricmp( token, "allowInvertNormal" ) )
        {
            ba->invertNormal = qtrue;
        }
        else if ( !Q_stricmp( token, "needsCreep" ) )
        {
            ba->creepTest = qtrue;
        }
        else if ( !Q_stricmp( token, "creepSize" ) )
        {
            PARSE(text, token);

            ba->creepSize = atoi(token);
        }
        else if ( !Q_stricmp( token, "dccTest" ) )
        {
            ba->dccTest = qtrue;
        }
        else if ( !Q_stricmp( token, "transparentTest" ) )
        {
            ba->transparentTest = qtrue;
        }
        else if ( !Q_stricmp( token, "unique" ) )
        {
            ba->uniqueTest = qtrue;
        }
        else if ( !Q_stricmp( token, "reward" ) )
        {
            PARSE(text, token);

            ba->value = atoi(token);

            defined |= VALUE;
        }
        else if ( !Q_stricmp( token, "radarFadeOut" ) )
        {
            PARSE(text, token);

            ba->radarFadeOut = atof(token);
            defined |= RADAR;
        }
        else if( (var = BG_FindConfigVar( va( "b_%s_%s", ba->name, token ) ) ) != NULL )
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
    else if ( !( defined & STAGE) ) { token = "stage"; }
    else if ( !( defined & HEALTH) ) { token = "health"; }
    else if ( !( defined & DEATHMOD) ) { token = "meansOfDeath"; }
    else if ( !( defined & TEAM) ) { token = "team"; }
    else if ( !( defined & BUILDWEAPON) ) { token = "buildWeapon"; }
    else if ( !( defined & BUILDTIME) ) { token = "buildTime"; }
    else if ( !( defined & VALUE) ) { token = "reward"; }
    else if ( !( defined & RADAR) ) { token = "radarFadeOut"; }
    else if ( !( defined & NORMAL) ) { token = "minNormal"; }

    if ( strlen( token ) > 0 )
    {
        Com_Printf( S_ERROR "%s not defined in %s\n",
                    token, filename );
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
    char *token;
    char text_buffer[ 20000 ];
    char* text;
    int defined = 0;
    enum
    {
      MODEL = 1 << 0,
      MODELSCALE = 1 << 1,
      MINS = 1 << 2,
      MAXS = 1 << 3,
      ZOFFSET = 1 << 4,
      OLDSCALE = 1 << 5,
      OLDOFFSET = 1 << 6
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
        Com_Printf( S_ERROR "%s not defined in %s\n",
                    token, filename );
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
    char *token;
    char text_buffer[ 20000 ];
    char* text;
    configVar_t* var;
    int defined = 0;
    enum
    {
      INFO = 1 << 0,
      FOVCVAR = 1 << 1,
      STAGE = 1 << 2,
      HEALTH = 1 << 3,
      FALLDAMAGE = 1 << 4,
      REGEN = 1 << 5,
      FOV = 1 << 6,
      STEPTIME = 1 << 7,
      SPEED = 1 << 8,
      ACCELERATION = 1 << 9,
      AIRACCELERATION = 1 << 10,
      FRICTION = 1 << 11,
      STOPSPEED = 1 << 12,
      JUMPMAGNITUDE = 1 << 13,
      KNOCKBACKSCALE = 1 << 14,
      COST = 1 << 15,
      VALUE = 1 << 16,
      RADAR = 1 << 17,
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
        else if ( !Q_stricmp( token, "stage" ) )
        {
            PARSE(text, token);

            ca->stages = BG_StageFromNumber( atoi(token) );

            defined |= STAGE;
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
        //Abilities ...
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
        else if ( !Q_stricmp( token, "knockbackScale" ) )
        {
            PARSE(text, token);

            ca->knockbackScale = atof( token );

            defined |= KNOCKBACKSCALE;
        }
        else if ( !Q_stricmp( token, "cost" ) )
        {
            PARSE(text, token);

            ca->cost = atoi( token );

            defined |= COST;
        }
        else if ( !Q_stricmp( token, "value" ) )
        {
            PARSE(text, token);

            ca->value = atoi( token );

            defined |= VALUE;
        }
        else if ( !Q_stricmp( token, "radarFadeOut" ) )
        {
            PARSE(text, token);

            ca->radarFadeOut = atof( token );

            defined |= RADAR;
        }
        else if( (var = BG_FindConfigVar( va( "c_%s_%s", ca->name, token ) ) ) != NULL )
        {
            BG_ParseConfigVar( var, &text, filename );
        }
        else
        {
            Com_Printf( S_ERROR "%s: unknown token '%s'\n", filename, token );
        }
    }

    if ( !( defined & INFO ) ) { token = "description"; }
    else if ( !( defined & FOVCVAR ) ) { token = "fovCvar"; }
    else if ( !( defined & STAGE ) ) { token = "stage"; }
    else if ( !( defined & HEALTH ) ) { token = "health"; }
    else if ( !( defined & FALLDAMAGE ) ) { token = "fallDamage"; }
    else if ( !( defined & REGEN ) ) { token = "regen"; }
    else if ( !( defined & FOV ) ) { token = "fov"; }
    else if ( !( defined & STEPTIME ) ) { token = "stepTime"; }
    else if ( !( defined & SPEED ) ) { token = "speed"; }
    else if ( !( defined & ACCELERATION ) ) { token = "acceleration"; }
    else if ( !( defined & AIRACCELERATION ) ) { token = "airAcceleration"; }
    else if ( !( defined & FRICTION ) ) { token = "friction"; }
    else if ( !( defined & STOPSPEED ) ) { token = "stopSpeed"; }
    else if ( !( defined & JUMPMAGNITUDE ) ) { token = "jumpMagnitude"; }
    else if ( !( defined & KNOCKBACKSCALE ) ) { token = "knockbackScale"; }
    else if ( !( defined & COST ) ) { token = "cost"; }
    else if ( !( defined & VALUE ) ) { token = "value"; }
    else if ( !( defined & RADAR ) ) { token = "radarFadeOut"; }
    else { token = ""; }

    if ( strlen( token ) > 0 )
    {
        Com_Printf( S_ERROR "%s not defined in %s\n",
                    token, filename );
    }
}

/*
======================
BG_ParseClassModelFile

Parses a configuration file describing the model of a class
======================
*/
void BG_ParseClassModelFile( const char *filename, classModelConfig_t *cc )
{
    char *token;
    char text_buffer[ 20000 ];
    char* text;
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
        Com_Printf( S_ERROR "%s not defined in %s\n",
                    token, filename );
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
    char *token;
    char text_buffer[ 20000 ];
    char* text;
    configVar_t* var;
    int defined = 0;
    enum
    {
      NAME = 1 << 0,
      INFO = 1 << 1,
      STAGE = 1 << 2,
      PRICE = 1 << 3,
      RATE = 1 << 4,
      AMMO = 1 << 5,
      TEAM = 1 << 6,
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
        else if ( !Q_stricmp( token, "stage" ) )
        {
            PARSE(text, token);

            wa->stages = BG_StageFromNumber( atoi( token ) );

            defined |= STAGE;
        }
        else if ( !Q_stricmp( token, "usedSlots" ) )
        {
            wa->slots = BG_ParseSlotList( &text );
        }
        else if ( !Q_stricmp( token, "price" ) )
        {
            PARSE(text, token);

            wa->price = atoi( token );

            defined |= PRICE;
        }
        else if ( !Q_stricmp( token, "infiniteAmmo" ) )
        {
            wa->infiniteAmmo = qtrue;

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
            wa->usesEnergy = qtrue;
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
        else if ( !Q_stricmp( token, "knockback" ) )
        {
            PARSE(text, token);

            wa->knockbackScale = atof( token );
        }
        else if ( !Q_stricmp( token, "hasAltMode" ) )
        {
            wa->hasAltMode = qtrue;
        }
        else if ( !Q_stricmp( token, "hasThirdMode" ) )
        {
            wa->hasThirdMode = qtrue;
        }
        else if ( !Q_stricmp( token, "isPurchasable" ) )
        {
            wa->purchasable = qtrue;
        }
        else if ( !Q_stricmp( token, "isLongRanged" ) )
        {
            wa->longRanged = qtrue;
        }
        else if ( !Q_stricmp( token, "canZoom" ) )
        {
            wa->canZoom = qtrue;
        }
        else if ( !Q_stricmp( token, "zoomFov" ) )
        {
            PARSE(text, token);

            wa->zoomFov = atof( token );
        }
        else if ( !Q_stricmp( token, "team" ) )
        {
            PARSE(text, token);

            wa->team = BG_ParseTeam( token );

            defined |= TEAM;
        }
        else if( (var = BG_FindConfigVar( va( "w_%s_%s", wa->name, token ) ) ) != NULL )
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
    else if ( !( defined & STAGE ) ) { token = "stage"; }
    else if ( !( defined & PRICE ) ) { token = "price"; }
    else if ( !( defined & RATE ) ) { token = "primaryAttackRate"; }
    else if ( !( defined & AMMO ) ) { token = "maxAmmo or infiniteAmmo"; }
    else if ( !( defined & TEAM ) ) { token = "team"; }
    else { token = ""; }

    if ( strlen( token ) > 0 )
    {
        Com_Printf( S_ERROR "%s not defined in %s\n",
                    token, filename );
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
    char *token;
    char text_buffer[ 20000 ];
    char* text;
    configVar_t* var;
    int defined = 0;
    enum
    {
      NAME = 1 << 0,
      PRICE = 1 << 1,
      INFO = 1 << 2,
      STAGE = 1 << 3,
      ICON = 1 << 4,
      TEAM = 1 << 5,
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
        else if ( !Q_stricmp( token, "stage" ) )
        {
            PARSE(text, token);

            ua->stages = BG_StageFromNumber( atoi( token ) );

            defined |= STAGE;
        }
        else if ( !Q_stricmp( token, "usedSlots" ) )
        {
            ua->slots = BG_ParseSlotList( &text );
        }
        else if ( !Q_stricmp( token, "icon" ) )
        {
            PARSE(text, token);

            if ( !Q_stricmp( token, "null" ) )
            {
                ua->icon = NULL;
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

            ua->team = BG_ParseTeam( token );

            defined |= TEAM;
        }
        else if ( !Q_stricmp( token, "isPurchasable" ) )
        {
            ua->purchasable = qtrue;
        }
        else if ( !Q_stricmp( token, "isUsable" ) )
        {
            ua->usable = qtrue;
        }
        else if( (var = BG_FindConfigVar( va( "u_%s_%s", ua->name, token ) ) ) != NULL )
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
    else if ( !( defined & STAGE ) ) { token = "stage"; }
    else if ( !( defined & PRICE ) ) { token = "price"; }
    else if ( !( defined & ICON ) ) { token = "icon"; }
    else if ( !( defined & TEAM ) ) { token = "team"; }
    else { token = ""; }

    if ( strlen( token ) > 0 )
    {
        Com_Printf( S_ERROR "%s not defined in %s\n",
                    token, filename );
    }
}


