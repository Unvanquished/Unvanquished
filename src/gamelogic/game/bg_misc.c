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

// bg_misc.c -- both games misc functions, all completely stateless

#include "../../engine/qcommon/q_shared.h"
#include "bg_public.h"

#define N_(x) x

int                                trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void                               trap_FS_Read( void *buffer, int len, fileHandle_t f );
void                               trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void                               trap_FS_FCloseFile( fileHandle_t f );
void                               trap_FS_Seek( fileHandle_t f, long offset, fsOrigin_t origin );  // fsOrigin_t
int                                trap_FS_GetFileList( const char *path, const char *extension, char *listbuf, int bufsize );
void                               trap_QuoteString( const char *, char *, int );

typedef struct
{
	buildable_t number;
	const char* name;
	const char* classname;
} buildableName_t;

static const buildableName_t bg_buildableNameList[] =
{
	{
		BA_A_SPAWN,
		"eggpod",
		"team_alien_spawn",
	},
	{
		BA_A_OVERMIND,
		"overmind",
		"team_alien_overmind",
	},
	{
		BA_A_BARRICADE,
		"barricade",
		"team_alien_barricade",
	},
	{
		BA_A_ACIDTUBE,
		"acid_tube",
		"team_alien_acid_tube",
	},
	{
		BA_A_TRAPPER,
		"trapper",
		"team_alien_trapper",
	},
	{
		BA_A_BOOSTER,
		"booster",
		"team_alien_booster",
	},
	{
		BA_A_HIVE,
		"hive",
		"team_alien_hive",
	},
	{
		BA_H_SPAWN,
		"telenode",
		"team_human_spawn",
	},
	{
		BA_H_MGTURRET,
		"mgturret",
		"team_human_mgturret",
	},
	{
		BA_H_TESLAGEN,
		"tesla",
		"team_human_tesla",
	},
	{
		BA_H_ARMOURY,
		"arm",
		"team_human_armoury",
	},
	{
		BA_H_DCC,
		"dcc",
		"team_human_dcc",
	},
	{
		BA_H_MEDISTAT,
		"medistat",
		"team_human_medistat",
	},
	{
		BA_H_REACTOR,
		"reactor",
		"team_human_reactor",
	},
	{
		BA_H_REPEATER,
		"repeater",
		"team_human_repeater",
	}
};

static const size_t bg_numBuildables = ARRAY_LEN( bg_buildableNameList );

static buildableAttributes_t bg_buildableList[ ARRAY_LEN( bg_buildableNameList ) ];

static const buildableAttributes_t nullBuildable = { 0 };

/*
==============
BG_BuildableByName
==============
*/
const buildableAttributes_t *BG_BuildableByName( const char *name )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( !Q_stricmp( bg_buildableList[ i ].name, name ) )
		{
			return &bg_buildableList[ i ];
		}
	}

	return &nullBuildable;
}

/*
==============
BG_BuildableByEntityName
==============
*/
const buildableAttributes_t *BG_BuildableByEntityName( const char *name )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( !Q_stricmp( bg_buildableList[ i ].entityName, name ) )
		{
			return &bg_buildableList[ i ];
		}
	}

	return &nullBuildable;
}

/*
==============
BG_Buildable
==============
*/
const buildableAttributes_t *BG_Buildable( buildable_t buildable )
{
	return ( buildable > BA_NONE && buildable < BA_NUM_BUILDABLES ) ?
	       &bg_buildableList[ buildable - 1 ] : &nullBuildable;
}

/*
==============
BG_BuildableAllowedInStage
==============
*/
qboolean BG_BuildableAllowedInStage( buildable_t buildable,
                                     stage_t stage )
{
	int stages = BG_Buildable( buildable )->stages;

	if ( stages & ( 1 << stage ) )
	{
		return qtrue;
	}
	else
	{
		return qfalse;
	}
}

/*
===============
BG_InitBuildableAttributes
===============
*/
void BG_InitBuildableAttributes( void )
{
	int i;
	const buildableName_t *bh;
	buildableAttributes_t *ba;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		bh = &bg_buildableNameList[i];
		ba = &bg_buildableList[i];

		//Initialise default values for buildables
		Com_Memset( ba, 0, sizeof( buildableAttributes_t ) );

		ba->number = bh->number;
		ba->name = bh->name;
		ba->entityName = bh->classname;

		ba->idleAnim = BANIM_IDLE1;
		ba->traj = TR_GRAVITY;
		ba->bounce = 0.0;
		ba->nextthink = 100;
		ba->turretProjType = WP_NONE;
		ba->minNormal = 0.0;

		BG_ParseBuildableAttributeFile( va( "configs/buildables/%s.attr.cfg", ba->name ), ba );
	}
}

static buildableModelConfig_t bg_buildableModelConfigList[ BA_NUM_BUILDABLES ];

/*
==============
BG_BuildableModelConfig
==============
*/
buildableModelConfig_t *BG_BuildableModelConfig( buildable_t buildable )
{
	return &bg_buildableModelConfigList[ buildable ];
}

/*
==============
BG_BuildableBoundingBox
==============
*/
void BG_BuildableBoundingBox( buildable_t buildable,
                              vec3_t mins, vec3_t maxs )
{
	buildableModelConfig_t *buildableModelConfig = BG_BuildableModelConfig( buildable );

	if ( mins != NULL )
	{
		VectorCopy( buildableModelConfig->mins, mins );
	}

	if ( maxs != NULL )
	{
		VectorCopy( buildableModelConfig->maxs, maxs );
	}
}

/*
===============
BG_InitBuildableModelConfigs
===============
*/
void BG_InitBuildableModelConfigs( void )
{
	int               i;
	buildableModelConfig_t *bc;

	for ( i = BA_NONE + 1; i < BA_NUM_BUILDABLES; i++ )
	{
		bc = BG_BuildableModelConfig( i );
		Com_Memset( bc, 0, sizeof( buildableModelConfig_t ) );

		BG_ParseBuildableModelFile( va( "configs/buildables/%s.model.cfg",
		                           BG_Buildable( i )->name ), bc );
	}
}

////////////////////////////////////////////////////////////////////////////////
typedef struct
{
	class_t number;
	const char* name;
	weapon_t startWeapon;
	int children[ 3 ];
} classData_t;

static classData_t bg_classData[] =
{
	{
		PCL_NONE, //int     number;
		"spectator", //char    *name;
		WP_NONE, //weapon_t  startWeapon;
		{ PCL_NONE,               PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_ALIEN_BUILDER0, //int     number;
		"builder", //char    *name;
		WP_ABUILD, //weapon_t  startWeapon;
		{ PCL_ALIEN_BUILDER0_UPG, PCL_ALIEN_LEVEL0,     PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_ALIEN_BUILDER0_UPG, //int     number;
		"builderupg", //char    *name;
		WP_ABUILD2, //weapon_t  startWeapon;
		{ PCL_ALIEN_LEVEL0,       PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_ALIEN_LEVEL0, //int     number;
		"level0", //char    *name;
		WP_ALEVEL0, //weapon_t  startWeapon;
		{ PCL_ALIEN_LEVEL1,       PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_ALIEN_LEVEL1, //int     number;
		"level1", //char    *name;
		WP_ALEVEL1, //weapon_t  startWeapon;
		{ PCL_ALIEN_LEVEL2,       PCL_ALIEN_LEVEL1_UPG, PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_ALIEN_LEVEL1_UPG, //int     number;
		"level1upg", //char    *name;
		WP_ALEVEL1_UPG, //weapon_t  startWeapon;
		{ PCL_ALIEN_LEVEL2,       PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_ALIEN_LEVEL2, //int     number;
		"level2", //char    *name;
		WP_ALEVEL2, //weapon_t  startWeapon;
		{ PCL_ALIEN_LEVEL3,       PCL_ALIEN_LEVEL2_UPG, PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_ALIEN_LEVEL2_UPG, //int     number;
		"level2upg", //char    *name;
		WP_ALEVEL2_UPG, //weapon_t  startWeapon;
		{ PCL_ALIEN_LEVEL3,       PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_ALIEN_LEVEL3, //int     number;
		"level3", //char    *name;
		WP_ALEVEL3, //weapon_t  startWeapon;
		{ PCL_ALIEN_LEVEL4,       PCL_ALIEN_LEVEL3_UPG, PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_ALIEN_LEVEL3_UPG, //int     number;
		"level3upg", //char    *name;
		WP_ALEVEL3_UPG, //weapon_t  startWeapon;
		{ PCL_ALIEN_LEVEL4,       PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_ALIEN_LEVEL4, //int     number;
		"level4", //char    *name;
		WP_ALEVEL4, //weapon_t  startWeapon;
		{ PCL_NONE,               PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_HUMAN, //int     number;
		"human_base", //char    *name;
		WP_NONE, //special-cased in g_client.c          //weapon_t  startWeapon;
		{ PCL_NONE,               PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
	},
	{
		PCL_HUMAN_BSUIT, //int     number;
		"human_bsuit", //char    *name;
		WP_NONE, //special-cased in g_client.c          //weapon_t  startWeapon;
		{ PCL_NONE,               PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
	}
};

static const size_t bg_numClasses = ARRAY_LEN( bg_classData );

static classAttributes_t bg_classList[ ARRAY_LEN( bg_classData ) ];

static const classAttributes_t nullClass = { 0 };

/*
==============
BG_ClassByName
==============
*/
const classAttributes_t *BG_ClassByName( const char *name )
{
	int i;

	for ( i = 0; i < bg_numClasses; i++ )
	{
		if ( !Q_stricmp( bg_classList[ i ].name, name ) )
		{
			return &bg_classList[ i ];
		}
	}

	return &nullClass;
}

/*
==============
BG_Class
==============
*/
const classAttributes_t *BG_Class( class_t pClass )
{
	return ( pClass >= PCL_NONE && pClass < PCL_NUM_CLASSES ) ?
	       &bg_classList[ pClass ] : &nullClass;
}

/*
==============
BG_ClassAllowedInStage
==============
*/
qboolean BG_ClassAllowedInStage( class_t pClass,
                                 stage_t stage )
{
	int stages = BG_Class( pClass )->stages;

	return stages & ( 1 << stage );
}

static classModelConfig_t bg_classModelConfigList[ PCL_NUM_CLASSES ];

/*
==============
BG_ClassModelConfig
==============
*/
classModelConfig_t *BG_ClassModelConfig( class_t pClass )
{
	return &bg_classModelConfigList[ pClass ];
}

/*
==============
BG_ClassBoundingBox
==============
*/
void BG_ClassBoundingBox( class_t pClass,
                          vec3_t mins, vec3_t maxs,
                          vec3_t cmaxs, vec3_t dmins, vec3_t dmaxs )
{
	classModelConfig_t *classModelConfig = BG_ClassModelConfig( pClass );

	if ( mins != NULL )
	{
		VectorCopy( classModelConfig->mins, mins );
	}

	if ( maxs != NULL )
	{
		VectorCopy( classModelConfig->maxs, maxs );
	}

	if ( cmaxs != NULL )
	{
		VectorCopy( classModelConfig->crouchMaxs, cmaxs );
	}

	if ( dmins != NULL )
	{
		VectorCopy( classModelConfig->deadMins, dmins );
	}

	if ( dmaxs != NULL )
	{
		VectorCopy( classModelConfig->deadMaxs, dmaxs );
	}
}

/*
==============
BG_ClassHasAbility
==============
*/
qboolean BG_ClassHasAbility( class_t pClass, int ability )
{
	int abilities = BG_Class( pClass )->abilities;

	return abilities & ability;
}

/*
==============
BG_ClassCanEvolveFromTo
==============
*/
int BG_ClassCanEvolveFromTo( class_t fclass,
                             class_t tclass,
                             int credits, int stage,
                             int cost )
{
	int i, j, best, value;

	if ( credits < cost || fclass == PCL_NONE || tclass == PCL_NONE ||
	     fclass == tclass )
	{
		return -1;
	}

	for ( i = 0; i < bg_numClasses; i++ )
	{
		if ( bg_classList[ i ].number != fclass )
		{
			continue;
		}

		best = credits + 1;

		for ( j = 0; j < 3; j++ )
		{
			int thruClass, evolveCost;

			thruClass = bg_classList[ i ].children[ j ];

			if ( thruClass == PCL_NONE || !BG_ClassAllowedInStage( thruClass, stage ) ||
			     !BG_ClassIsAllowed( thruClass ) )
			{
				continue;
			}

			evolveCost = BG_Class( thruClass )->cost * ALIEN_CREDITS_PER_KILL;

			if ( thruClass == tclass )
			{
				value = cost + evolveCost;
			}
			else
			{
				value = BG_ClassCanEvolveFromTo( thruClass, tclass, credits, stage,
				                                 cost + evolveCost );
			}

			if ( value >= 0 && value < best )
			{
				best = value;
			}
		}

		return best <= credits ? best : -1;
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_ClassCanEvolveFromTo\n" );
	return -1;
}

/*
==============
BG_AlienCanEvolve
==============
*/
qboolean BG_AlienCanEvolve( class_t pClass, int credits, int stage )
{
	int i, j, tclass;

	for ( i = 0; i < bg_numClasses; i++ )
	{
		if ( bg_classList[ i ].number != pClass )
		{
			continue;
		}

		for ( j = 0; j < 3; j++ )
		{
			tclass = bg_classList[ i ].children[ j ];

			if ( tclass != PCL_NONE && BG_ClassAllowedInStage( tclass, stage ) &&
			     BG_ClassIsAllowed( tclass ) &&
			     credits >= BG_Class( tclass )->cost * ALIEN_CREDITS_PER_KILL )
			{
				return qtrue;
			}
		}

		return qfalse;
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_AlienCanEvolve\n" );
	return qfalse;
}

/*
===============
BG_InitClassAttributes
===============
*/
void BG_InitClassAttributes( void )
{
	int i;
	const classData_t *cd;
	classAttributes_t *ca;

	for ( i = 0; i < bg_numClasses; i++ )
	{
		cd = &bg_classData[i];
		ca = &bg_classList[i];

		ca->number = cd->number;
		ca->name = cd->name;
		ca->startWeapon = cd->startWeapon;
		ca->children[0] = cd->children[0];
		ca->children[1] = cd->children[1];
		ca->children[2] = cd->children[2];

		ca->buildDist = 0.0f;
		ca->bob = 0.0f;
		ca->bobCycle = 0.0f;
		ca->abilities = 0;

		BG_ParseClassAttributeFile( va( "configs/classes/%s.attr.cfg", ca->name ), ca );
	}
}

/*
===============
BG_InitClassModelConfigs
===============
*/
void BG_InitClassModelConfigs( void )
{
	int           i;
	classModelConfig_t *cc;

	for ( i = PCL_NONE; i < PCL_NUM_CLASSES; i++ )
	{
		cc = BG_ClassModelConfig( i );

		BG_ParseClassModelFile( va( "configs/classes/%s.model.cfg",
		                       BG_Class( i )->name ), cc );
	}
}

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	int number;
	const char* name;
} weaponData_t;

static const weaponData_t bg_weaponsData[] =
{
	{
		WP_ALEVEL0, //int       number;
		"level0", //char      *name;
	},
	{
		WP_ALEVEL1, //int       number;
		"level1", //char      *name;
	},
	{
		WP_ALEVEL1_UPG, //int       number;
		"level1upg", //char      *name;
	},
	{
		WP_ALEVEL2, //int       number;
		"level2", //char      *name;
	},
	{
		WP_ALEVEL2_UPG, //int       number;
		"level2upg", //char      *name;
	},
	{
		WP_ALEVEL3, //int       number;
		"level3", //char      *name;
	},
	{
		WP_ALEVEL3_UPG, //int       number;
		"level3upg", //char      *name;
	},
	{
		WP_ALEVEL4, //int       number;
		"level4", //char      *name;
	},
	{
		WP_BLASTER, //int       number;
		"blaster", //char      *name;
	},
	{
		WP_MACHINEGUN, //int       number;
		"rifle", //char      *name;
	},
	{
		WP_PAIN_SAW, //int       number;
		"psaw", //char      *name;
	},
	{
		WP_SHOTGUN, //int       number;
		"shotgun", //char      *name;
	},
	{
		WP_LAS_GUN, //int       number;
		"lgun", //char      *name;
	},
	{
		WP_MASS_DRIVER, //int       number;
		"mdriver", //char      *name;
	},
	{
		WP_CHAINGUN, //int       number;
		"chaingun", //char      *name;
	},
	{
		WP_FLAMER, //int       number;
		"flamer", //char      *name;
	},
	{
		WP_PULSE_RIFLE, //int       number;
		"prifle", //char      *name;
	},
	{
		WP_LUCIFER_CANNON, //int       number;
		"lcannon", //char      *name;
	},
	{
		WP_GRENADE, //int       number;
		"grenade", //char      *name;
	},
	{
		WP_LOCKBLOB_LAUNCHER, //int       number;
		"lockblob", //char      *name;
	},
	{
		WP_HIVE, //int       number;
		"hive", //char      *name;
	},
	{
		WP_TESLAGEN, //int       number;
		"teslagen", //char      *name;
	},
	{
		WP_MGTURRET, //int       number;
		"mgturret", //char      *name;
	},
	{
		WP_ABUILD, //int       number;
		"abuild", //char      *name;
	},
	{
		WP_ABUILD2, //int       number;
		"abuildupg", //char      *name;
	},
	{
		WP_HBUILD, //int       number;
		"ckit", //char      *name;
	}
};

static const size_t bg_numWeapons = ARRAY_LEN( bg_weaponsData );

static weaponAttributes_t bg_weapons[ ARRAY_LEN( bg_weaponsData ) ];

static const weaponAttributes_t nullWeapon = { 0 };

/*
==============
BG_WeaponByName
==============
*/
const weaponAttributes_t *BG_WeaponByName( const char *name )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( !Q_stricmp( bg_weapons[ i ].name, name ) )
		{
			return &bg_weapons[ i ];
		}
	}

	return &nullWeapon;
}

/*
==============
BG_Weapon
==============
*/
const weaponAttributes_t *BG_Weapon( weapon_t weapon )
{
	return ( weapon > WP_NONE && weapon < WP_NUM_WEAPONS ) ?
	       &bg_weapons[ weapon - 1 ] : &nullWeapon;
}

/*
==============
BG_WeaponAllowedInStage
==============
*/
qboolean BG_WeaponAllowedInStage( weapon_t weapon, stage_t stage )
{
	int stages = BG_Weapon( weapon )->stages;

	return stages & ( 1 << stage );
}

/*
===============
BG_InitWeaponAttributes
===============
*/
void BG_InitWeaponAttributes( void )
{
	int i;
	const weaponData_t *wd;
	weaponAttributes_t *wa;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		wd = &bg_weaponsData[i];
		wa = &bg_weapons[i];

		//Initialise default values for buildables
		Com_Memset( wa, 0, sizeof( weaponAttributes_t ) );

		wa->number = wd->number;
		wa->name = wd->name;
		wa->knockbackScale = 0.0f;

		BG_ParseWeaponAttributeFile( va( "configs/weapon/%s.attr.cfg", wa->name ), wa );
	}
}



////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	int number;
	const char* name;
} upgradeData_t;


static const upgradeData_t bg_upgradesData[] =
{
	{
		UP_LIGHTARMOUR, //int   number;
		"larmour", //char  *name;
	},
	{
		UP_HELMET, //int   number;
		"helmet", //char  *name;
	},
	{
		UP_MEDKIT, //int   number;
		"medkit", //char  *name;
	},
	{
		UP_BATTPACK, //int   number;
		"battpack", //char  *name;
	},
	{
		UP_JETPACK, //int   number;
		"jetpack", //char  *name;
	},
	{
		UP_BATTLESUIT, //int   number;
		"bsuit", //char  *name;
	},
	{
		UP_GRENADE, //int   number;
		"gren", //char  *name;
	},
	{
		UP_AMMO, //int   number;
		"ammo", //char  *name;
	}
};

static const size_t bg_numUpgrades = ARRAY_LEN( bg_upgradesData );

static upgradeAttributes_t bg_upgrades[ ARRAY_LEN( bg_upgradesData ) ];

static const upgradeAttributes_t nullUpgrade = { 0 };

/*
==============
BG_UpgradeByName
==============
*/
const upgradeAttributes_t *BG_UpgradeByName( const char *name )
{
	int i;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		if ( !Q_stricmp( bg_upgrades[ i ].name, name ) )
		{
			return &bg_upgrades[ i ];
		}
	}

	return &nullUpgrade;
}

/*
==============
BG_Upgrade
==============
*/
const upgradeAttributes_t *BG_Upgrade( upgrade_t upgrade )
{
	return ( upgrade > UP_NONE && upgrade < UP_NUM_UPGRADES ) ?
	       &bg_upgrades[ upgrade - 1 ] : &nullUpgrade;
}

/*
==============
BG_UpgradeAllowedInStage
==============
*/
qboolean BG_UpgradeAllowedInStage( upgrade_t upgrade, stage_t stage )
{
	int stages = BG_Upgrade( upgrade )->stages;

	return stages & ( 1 << stage );
}

/*
===============
BG_InitUpgradeAttributes
===============
*/
void BG_InitUpgradeAttributes( void )
{
	int i;
	const upgradeData_t *ud;
	upgradeAttributes_t *ua;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		ud = &bg_upgradesData[i];
		ua = &bg_upgrades[i];

		//Initialise default values for buildables
		Com_Memset( ua, 0, sizeof( upgradeAttributes_t ) );

		ua->number = ud->number;
		ua->name = ud->name;

		BG_ParseUpgradeAttributeFile( va( "configs/upgrades/%s.attr.cfg", ua->name ), ua );
	}
}

////////////////////////////////////////////////////////////////////////////////

/*
================
BG_InitAllConfigs

================
*/

qboolean config_loaded = qfalse;

void BG_InitAllConfigs( void )
{
	BG_InitBuildableAttributes();
	BG_InitBuildableModelConfigs();
	BG_InitClassAttributes();
	BG_InitClassModelConfigs();
	BG_InitWeaponAttributes();
	BG_InitUpgradeAttributes();

	config_loaded = qtrue;
}

/*
================
BG_UnloadAllConfigs

================
*/

void BG_UnloadAllConfigs( void )
{
    // Frees all the strings that were allocated when the config files were read
    int i;

    // When the game starts VMs are shutdown before they are even started
    if(!config_loaded){
        return;
    }
    config_loaded = qfalse;

    for ( i = 0; i < bg_numBuildables; i++ )
    {
        buildableAttributes_t *ba = &bg_buildableList[i];
        BG_Free( (char *)ba->humanName );
        BG_Free( (char *)ba->info );
    }

    for ( i = 0; i < bg_numClasses; i++ )
    {
        classAttributes_t *ca = &bg_classList[i];

        // Do not free the statically allocated empty string
        if( *ca->info != '\0' )
        {
            BG_Free( (char *)ca->info );
        }

        if( *ca->info != '\0' )
        {
            BG_Free( (char *)ca->fovCvar );
        }
    }

    for ( i = PCL_NONE; i < PCL_NUM_CLASSES; i++ )
    {
        BG_Free( (char *)BG_ClassModelConfig( i )->humanName );
    }

    for ( i = 0; i < bg_numWeapons; i++ )
    {
        weaponAttributes_t *wa = &bg_weapons[i];
        BG_Free( (char *)wa->humanName );

        if( *wa->info != '\0' )
        {
            BG_Free( (char *)wa->info );
        }
    }

    for ( i = 0; i < bg_numUpgrades; i++ )
    {
        upgradeAttributes_t *ua = &bg_upgrades[i];
        BG_Free( (char *)ua->humanName );

        if( *ua->info != '\0' )
        {
            BG_Free( (char *)ua->info );
        }
    }
}

////////////////////////////////////////////////////////////////////////////////

/*
================
BG_EvaluateTrajectory

================
*/
void BG_EvaluateTrajectory( const trajectory_t *tr, int atTime, vec3_t result )
{
	float deltaTime;
	float phase;

	switch ( tr->trType )
	{
		case TR_STATIONARY:
		case TR_INTERPOLATE:
			VectorCopy( tr->trBase, result );
			break;

		case TR_LINEAR:
			deltaTime = ( atTime - tr->trTime ) * 0.001; // milliseconds to seconds
			VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
			break;

		case TR_SINE:
			deltaTime = ( atTime - tr->trTime ) / ( float ) tr->trDuration;
			phase = sin( deltaTime * M_PI * 2 );
			VectorMA( tr->trBase, phase, tr->trDelta, result );
			break;

		case TR_LINEAR_STOP:
			if ( atTime > tr->trTime + tr->trDuration )
			{
				atTime = tr->trTime + tr->trDuration;
			}

			deltaTime = ( atTime - tr->trTime ) * 0.001; // milliseconds to seconds

			if ( deltaTime < 0 )
			{
				deltaTime = 0;
			}

			VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
			break;

		case TR_GRAVITY:
			deltaTime = ( atTime - tr->trTime ) * 0.001; // milliseconds to seconds
			VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
			result[ 2 ] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime; // FIXME: local gravity...
			break;

		case TR_BUOYANCY:
			deltaTime = ( atTime - tr->trTime ) * 0.001; // milliseconds to seconds
			VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
			result[ 2 ] += 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime; // FIXME: local gravity...
			break;

		default:
			Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
	}
}

/*
================
BG_EvaluateTrajectoryDelta

For determining velocity at a given time
================
*/
void BG_EvaluateTrajectoryDelta( const trajectory_t *tr, int atTime, vec3_t result )
{
	float deltaTime;
	float phase;

	switch ( tr->trType )
	{
		case TR_STATIONARY:
		case TR_INTERPOLATE:
			VectorClear( result );
			break;

		case TR_LINEAR:
			VectorCopy( tr->trDelta, result );
			break;

		case TR_SINE:
			deltaTime = ( atTime - tr->trTime ) / ( float ) tr->trDuration;
			phase = cos( deltaTime * M_PI * 2 );  // derivative of sin = cos
			phase *= 2 * M_PI * 1000 / tr->trDuration;
			VectorScale( tr->trDelta, phase, result );
			break;

		case TR_LINEAR_STOP:
			if ( atTime > tr->trTime + tr->trDuration || atTime < tr->trTime )
			{
				VectorClear( result );
				return;
			}

			VectorCopy( tr->trDelta, result );
			break;

		case TR_GRAVITY:
			deltaTime = ( atTime - tr->trTime ) * 0.001; // milliseconds to seconds
			VectorCopy( tr->trDelta, result );
			result[ 2 ] -= DEFAULT_GRAVITY * deltaTime; // FIXME: local gravity...
			break;

		case TR_BUOYANCY:
			deltaTime = ( atTime - tr->trTime ) * 0.001; // milliseconds to seconds
			VectorCopy( tr->trDelta, result );
			result[ 2 ] += DEFAULT_GRAVITY * deltaTime; // FIXME: local gravity...
			break;

		default:
			Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
	}
}

static const char *const eventnames[] =
{
	"EV_NONE",

	"EV_FOOTSTEP",
	"EV_FOOTSTEP_METAL",
	"EV_FOOTSTEP_SQUELCH",
	"EV_FOOTSPLASH",
	"EV_FOOTWADE",
	"EV_SWIM",

	"EV_STEP_4",
	"EV_STEP_8",
	"EV_STEP_12",
	"EV_STEP_16",

	"EV_STEPDN_4",
	"EV_STEPDN_8",
	"EV_STEPDN_12",
	"EV_STEPDN_16",

	"EV_FALL_SHORT",
	"EV_FALL_MEDIUM",
	"EV_FALL_FAR",
	"EV_FALLING",

	"EV_JUMP",
	"EV_WATER_TOUCH", // foot touches
	"EV_WATER_LEAVE", // foot leaves
	"EV_WATER_UNDER", // head touches
	"EV_WATER_CLEAR", // head leaves

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_FIRE_WEAPON2",
	"EV_FIRE_WEAPON3",

	"EV_PLAYER_RESPAWN", // for fovwarp effects
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE", // eventParm will be the soundindex

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND", // no attenuation

	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",

	"EV_SHOTGUN",
	"EV_MASS_DRIVER",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_TESLATRAIL",
	"EV_BULLET", // otherEntity is the shooter

	"EV_LEV1_GRAB",
	"EV_LEV4_TRAMPLE_PREPARE",
	"EV_LEV4_TRAMPLE_START",

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_GIB_PLAYER",

	"EV_BUILD_CONSTRUCT",
	"EV_BUILD_DESTROY",
	"EV_BUILD_DELAY", // can't build yet
	"EV_BUILD_REPAIR", // repairing buildable
	"EV_BUILD_REPAIRED", // buildable has full health
	"EV_HUMAN_BUILDABLE_EXPLOSION",
	"EV_ALIEN_BUILDABLE_EXPLOSION",
	"EV_ALIEN_ACIDTUBE",

	"EV_MEDKIT_USED",

	"EV_ALIEN_EVOLVE",
	"EV_ALIEN_EVOLVE_FAILED",

	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT",

	"EV_OVERMIND_ATTACK", // overmind under attack
	"EV_OVERMIND_DYING", // overmind close to death
	"EV_OVERMIND_SPAWNS", // overmind needs spawns

	"EV_DCC_ATTACK", // dcc under attack

	"EV_MGTURRET_SPINUP", // trigger a sound

	"EV_RPTUSE_SOUND", // trigger a sound
	"EV_LEV2_ZAP"
};

/*
===============
BG_EventName
===============
*/
const char *BG_EventName( int num )
{
	if ( num < 0 || num >= ARRAY_LEN( eventnames ) )
	{
		return "UNKNOWN";
	}

	return eventnames[ num ];
}

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps )
{
#ifndef NDEBUG
	{
		char buf[ 256 ];
		trap_Cvar_VariableStringBuffer( "showevents", buf, sizeof( buf ) );

		if ( atof( buf ) != 0 )
		{
#ifdef GAME
			Com_Printf( " game event svt %5d -> %5d: num = %20s parm %d\n",
			ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence,
			BG_EventName( newEvent ), eventParm );
#else
			Com_Printf( "Cgame event svt %5d -> %5d: num = %20s parm %d\n",
			ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence,
			BG_EventName( newEvent ), eventParm );
#endif
		}
	}
#endif
	ps->events[ ps->eventSequence & ( MAX_EVENTS - 1 ) ] = newEvent;
	ps->eventParms[ ps->eventSequence & ( MAX_EVENTS - 1 ) ] = eventParm;
	ps->eventSequence++;
}

/*
========================
BG_PlayerStateToEntityState

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, qboolean snap )
{
	int i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_type == PM_FREEZE )
	{
		s->eType = ET_INVISIBLE;
	}
	else if ( ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
	{
		s->eType = ET_INVISIBLE;
	}
	else
	{
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_INTERPOLATE;
	VectorCopy( ps->origin, s->pos.trBase );

	if ( snap )
	{
		SnapVector( s->pos.trBase );
	}

	//set the trDelta for flag direction
	VectorCopy( ps->velocity, s->pos.trDelta );

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );

	if ( snap )
	{
		SnapVector( s->apos.trBase );
	}

	s->time2 = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->weaponAnim = ps->weaponAnim;
	s->clientNum = ps->clientNum; // ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;

	if ( ps->stats[ STAT_HEALTH ] <= 0 )
	{
		s->eFlags |= EF_DEAD;
	}
	else
	{
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->stats[ STAT_STATE ] & SS_BLOBLOCKED )
	{
		s->eFlags |= EF_BLOBLOCKED;
	}
	else
	{
		s->eFlags &= ~EF_BLOBLOCKED;
	}

	if ( ps->externalEvent )
	{
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}
	else if ( ps->entityEventSequence < ps->eventSequence )
	{
		int seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_EVENTS )
		{
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}

		seq = ps->entityEventSequence & ( MAX_EVENTS - 1 );
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	//store items held and active items in modelindex and modelindex2
	s->modelindex = 0;
	s->modelindex2 = 0;

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( BG_InventoryContainsUpgrade( i, ps->stats ) )
		{
			s->modelindex |= 1 << i;

			if ( BG_UpgradeIsActive( i, ps->stats ) )
			{
				s->modelindex2 |= 1 << i;
			}
		}
	}

	// use misc field to store team/class info:
	s->misc = ps->stats[ STAT_TEAM ] | ( ps->stats[ STAT_CLASS ] << 8 );

	// have to get the surfNormal through somehow...
	VectorCopy( ps->grapplePoint, s->angles2 );

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;

	if ( s->generic1 <= WPM_NONE || s->generic1 >= WPM_NUM_WEAPONMODES )
	{
		s->generic1 = WPM_PRIMARY;
	}

	s->otherEntityNum = ps->otherEntityNum;
}

/*
========================
BG_PlayerStateToEntityStateExtraPolate

This is done after each set of usercmd_t on the server,
and after local prediction on the client
========================
*/
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, qboolean snap )
{
	int i;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPECTATOR || ps->pm_type == PM_FREEZE )
	{
		s->eType = ET_INVISIBLE;
	}
	else if ( ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
	{
		s->eType = ET_INVISIBLE;
	}
	else
	{
		s->eType = ET_PLAYER;
	}

	s->number = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( ps->origin, s->pos.trBase );

	if ( snap )
	{
		SnapVector( s->pos.trBase );
	}

	// set the trDelta for flag direction and linear prediction
	VectorCopy( ps->velocity, s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );

	if ( snap )
	{
		SnapVector( s->apos.trBase );
	}

	s->time2 = ps->movementDir;
	s->legsAnim = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->weaponAnim = ps->weaponAnim;
	s->clientNum = ps->clientNum; // ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	s->eFlags = ps->eFlags;

	if ( ps->stats[ STAT_HEALTH ] <= 0 )
	{
		s->eFlags |= EF_DEAD;
	}
	else
	{
		s->eFlags &= ~EF_DEAD;
	}

	if ( ps->stats[ STAT_STATE ] & SS_BLOBLOCKED )
	{
		s->eFlags |= EF_BLOBLOCKED;
	}
	else
	{
		s->eFlags &= ~EF_BLOBLOCKED;
	}

	if ( ps->externalEvent )
	{
		s->event = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}
	else if ( ps->entityEventSequence < ps->eventSequence )
	{
		int seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_EVENTS )
		{
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}

		seq = ps->entityEventSequence & ( MAX_EVENTS - 1 );
		s->event = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	//store items held and active items in modelindex and modelindex2
	s->modelindex = 0;
	s->modelindex2 = 0;

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( BG_InventoryContainsUpgrade( i, ps->stats ) )
		{
			s->modelindex |= 1 << i;

			if ( BG_UpgradeIsActive( i, ps->stats ) )
			{
				s->modelindex2 |= 1 << i;
			}
		}
	}

	// use misc field to store team/class info:
	s->misc = ps->stats[ STAT_TEAM ] | ( ps->stats[ STAT_CLASS ] << 8 );

	// have to get the surfNormal through somehow...
	VectorCopy( ps->grapplePoint, s->angles2 );

	s->loopSound = ps->loopSound;
	s->generic1 = ps->generic1;

	if ( s->generic1 <= WPM_NONE || s->generic1 >= WPM_NUM_WEAPONMODES )
	{
		s->generic1 = WPM_PRIMARY;
	}

	s->otherEntityNum = ps->otherEntityNum;
}

/*
========================
BG_WeaponIsFull

Check if a weapon has full ammo
========================
*/
qboolean BG_WeaponIsFull( weapon_t weapon, int stats[], int ammo, int clips )
{
	int maxAmmo, maxClips;

	maxAmmo = BG_Weapon( weapon )->maxAmmo;
	maxClips = BG_Weapon( weapon )->maxClips;

	if ( BG_InventoryContainsUpgrade( UP_BATTPACK, stats ) )
	{
		maxAmmo = ( int )( ( float ) maxAmmo * BATTPACK_MODIFIER );
	}

	return ( maxAmmo == ammo ) && ( maxClips == clips );
}

/*
========================
BG_InventoryContainsWeapon

Does the player hold a weapon?
========================
*/
qboolean BG_InventoryContainsWeapon( int weapon, int stats[] )
{
	// humans always have a blaster
	if ( stats[ STAT_TEAM ] == TEAM_HUMANS && weapon == WP_BLASTER )
	{
		return qtrue;
	}

	return ( stats[ STAT_WEAPON ] == weapon );
}

/*
========================
BG_SlotsForInventory

Calculate the slots used by an inventory and warn of conflicts
========================
*/
int BG_SlotsForInventory( int stats[] )
{
	int i, slot, slots;

	slots = BG_Weapon( stats[ STAT_WEAPON ] )->slots;

	if ( stats[ STAT_TEAM ] == TEAM_HUMANS )
	{
		slots |= BG_Weapon( WP_BLASTER )->slots;
	}

	for ( i = UP_NONE; i < UP_NUM_UPGRADES; i++ )
	{
		if ( BG_InventoryContainsUpgrade( i, stats ) )
		{
			slot = BG_Upgrade( i )->slots;

			// this check should never be true
			if ( slots & slot )
			{
				Com_Printf( S_COLOR_YELLOW "WARNING: held item %d conflicts with "
				            "inventory slot %d\n", i, slot );
			}

			slots |= slot;
		}
	}

	return slots;
}

/*
========================
BG_AddUpgradeToInventory

Give the player an upgrade
========================
*/
void BG_AddUpgradeToInventory( int item, int stats[] )
{
	stats[ STAT_ITEMS ] |= ( 1 << item );
}

/*
========================
BG_RemoveUpgradeFromInventory

Take an upgrade from the player
========================
*/
void BG_RemoveUpgradeFromInventory( int item, int stats[] )
{
	stats[ STAT_ITEMS ] &= ~( 1 << item );
}

/*
========================
BG_InventoryContainsUpgrade

Does the player hold an upgrade?
========================
*/
qboolean BG_InventoryContainsUpgrade( int item, int stats[] )
{
	return ( stats[ STAT_ITEMS ] & ( 1 << item ) );
}

/*
========================
BG_ActivateUpgrade

Activates an upgrade
========================
*/
void BG_ActivateUpgrade( int item, int stats[] )
{
	stats[ STAT_ACTIVEITEMS ] |= ( 1 << item );
}

/*
========================
BG_DeactivateUpgrade

Deactivates an upgrade
========================
*/
void BG_DeactivateUpgrade( int item, int stats[] )
{
	stats[ STAT_ACTIVEITEMS ] &= ~( 1 << item );
}

/*
========================
BG_UpgradeIsActive

Is this upgrade active?
========================
*/
qboolean BG_UpgradeIsActive( int item, int stats[] )
{
	return ( stats[ STAT_ACTIVEITEMS ] & ( 1 << item ) );
}

/*
===============
BG_RotateAxis

Shared axis rotation function
===============
*/
qboolean BG_RotateAxis( vec3_t surfNormal, vec3_t inAxis[ 3 ],
                        vec3_t outAxis[ 3 ], qboolean inverse, qboolean ceiling )
{
	vec3_t refNormal = { 0.0f, 0.0f, 1.0f };
	vec3_t ceilingNormal = { 0.0f, 0.0f, -1.0f };
	vec3_t localNormal, xNormal;
	float  rotAngle;

	//the grapplePoint being a surfNormal rotation Normal hack... see above :)
	if ( ceiling )
	{
		VectorCopy( ceilingNormal, localNormal );
		VectorCopy( surfNormal, xNormal );
	}
	else
	{
		//cross the reference normal and the surface normal to get the rotation axis
		VectorCopy( surfNormal, localNormal );
		CrossProduct( localNormal, refNormal, xNormal );
		VectorNormalize( xNormal );
	}

	//can't rotate with no rotation vector
	if ( VectorLength( xNormal ) != 0.0f )
	{
		rotAngle = RAD2DEG( acos( DotProduct( localNormal, refNormal ) ) );

		if ( inverse )
		{
			rotAngle = -rotAngle;
		}

		AngleNormalize180( rotAngle );

		//hmmm could get away with only one rotation and some clever stuff later... but i'm lazy
		RotatePointAroundVector( outAxis[ 0 ], xNormal, inAxis[ 0 ], -rotAngle );
		RotatePointAroundVector( outAxis[ 1 ], xNormal, inAxis[ 1 ], -rotAngle );
		RotatePointAroundVector( outAxis[ 2 ], xNormal, inAxis[ 2 ], -rotAngle );
	}
	else
	{
		return qfalse;
	}

	return qtrue;
}

/*
===============
BG_GetClientNormal

Get the normal for the surface the client is walking on
===============
*/
void BG_GetClientNormal( const playerState_t *ps, vec3_t normal )
{
	if ( ps->stats[ STAT_STATE ] & SS_WALLCLIMBING )
	{
		if ( ps->eFlags & EF_WALLCLIMBCEILING )
		{
			VectorSet( normal, 0.0f, 0.0f, -1.0f );
		}
		else
		{
			VectorCopy( ps->grapplePoint, normal );
		}
	}
	else
	{
		VectorSet( normal, 0.0f, 0.0f, 1.0f );
	}
}

/*
===============
BG_GetClientViewOrigin

Get the position of the client's eye, based on the client's position, the surface's normal, and client's view height
===============
*/
void BG_GetClientViewOrigin( const playerState_t *ps, vec3_t viewOrigin )
{
	vec3_t normal;
	BG_GetClientNormal( ps, normal );
	VectorMA( ps->origin, ps->viewheight, normal, viewOrigin );
}

/*
===============
BG_PositionBuildableRelativeToPlayer

Find a place to build a buildable
===============
*/
void BG_PositionBuildableRelativeToPlayer( playerState_t *ps,
    const vec3_t mins, const vec3_t maxs,
    void ( *trace )( trace_t *, const vec3_t, const vec3_t,
                     const vec3_t, const vec3_t, int, int ),
    vec3_t outOrigin, vec3_t outAngles, trace_t *tr )
{
	vec3_t aimDir, forward, entityOrigin, targetOrigin;
	vec3_t angles, playerOrigin, playerNormal;
	float  buildDist;

	BG_GetClientNormal( ps, playerNormal );

	VectorCopy( ps->viewangles, angles );
	VectorCopy( ps->origin, playerOrigin );

	AngleVectors( angles, aimDir, NULL, NULL );
	ProjectPointOnPlane( forward, aimDir, playerNormal );
	VectorNormalize( forward );

	buildDist = BG_Class( ps->stats[ STAT_CLASS ] )->buildDist * DotProduct( forward, aimDir );

	VectorMA( playerOrigin, buildDist, forward, entityOrigin );

	VectorCopy( entityOrigin, targetOrigin );

	//so buildings can be placed facing slopes
	VectorMA( entityOrigin, 32, playerNormal, entityOrigin );

	//so buildings drop to floor
	VectorMA( targetOrigin, -128, playerNormal, targetOrigin );

	// The mask is MASK_DEADSOLID on purpose to avoid collisions with other entities
	( *trace )( tr, entityOrigin, mins, maxs, targetOrigin, ps->clientNum, MASK_DEADSOLID );
	VectorCopy( tr->endpos, outOrigin );
	vectoangles( forward, outAngles );
}

/*
===============
BG_GetValueOfPlayer

Returns the credit value of a player
===============
*/
int BG_GetValueOfPlayer( playerState_t *ps )
{
	int i, worth = 0;

	worth = BG_Class( ps->stats[ STAT_CLASS ] )->value;

	// Humans have worth from their equipment as well
	if ( ps->stats[ STAT_TEAM ] == TEAM_HUMANS )
	{
		for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
		{
			if ( BG_InventoryContainsUpgrade( i, ps->stats ) )
			{
				worth += BG_Upgrade( i )->price;
			}
		}

		for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
		{
			if ( BG_InventoryContainsWeapon( i, ps->stats ) )
			{
				worth += BG_Weapon( i )->price;
			}
		}
	}

	return worth;
}

/*
=================
BG_PlayerCanChangeWeapon
=================
*/
qboolean BG_PlayerCanChangeWeapon( playerState_t *ps )
{
	// Do not allow Lucifer Cannon "canceling" via weapon switch
	if ( ps->weapon == WP_LUCIFER_CANNON &&
	     ps->stats[ STAT_MISC ] > LCANNON_CHARGE_TIME_MIN )
	{
		return qfalse;
	}

	return ps->weaponTime <= 0 || ps->weaponstate != WEAPON_FIRING;
}

/*
=================
BG_PlayerPoisonCloudTime
=================
*/
int BG_PlayerPoisonCloudTime( playerState_t *ps )
{
	int time = LEVEL1_PCLOUD_TIME;

	if ( BG_InventoryContainsUpgrade( UP_BATTLESUIT, ps->stats ) )
	{
		time -= BSUIT_PCLOUD_PROTECTION;
	}

	if ( BG_InventoryContainsUpgrade( UP_HELMET, ps->stats ) )
	{
		time -= HELMET_PCLOUD_PROTECTION;
	}

	if ( BG_InventoryContainsUpgrade( UP_LIGHTARMOUR, ps->stats ) )
	{
		time -= LIGHTARMOUR_PCLOUD_PROTECTION;
	}

	return time;
}

/*
=================
BG_GetPlayerWeapon

Returns the players current weapon or the weapon they are switching to.
Only needs to be used for human weapons.
=================
*/
weapon_t BG_GetPlayerWeapon( playerState_t *ps )
{
	if ( ps->persistant[ PERS_NEWWEAPON ] )
	{
		return ps->persistant[ PERS_NEWWEAPON ];
	}

	return ps->weapon;
}

/*
===============
atof_neg

atof with an allowance for negative values
===============
*/
float atof_neg( char *token, qboolean allowNegative )
{
	float value;

	value = atof( token );

	if ( !allowNegative && value < 0.0f )
	{
		value = 1.0f;
	}

	return value;
}

/*
===============
atoi_neg

atoi with an allowance for negative values
===============
*/
int atoi_neg( char *token, qboolean allowNegative )
{
	int value;

	value = atoi( token );

	if ( !allowNegative && value < 0 )
	{
		value = 1;
	}

	return value;
}

#define MAX_NUM_PACKED_ENTITY_NUMS 10

/*
===============
BG_PackEntityNumbers

Pack entity numbers into an entityState_t
===============
*/
void BG_PackEntityNumbers( entityState_t *es, const int *entityNums, int count )
{
	int i;

	if ( count > MAX_NUM_PACKED_ENTITY_NUMS )
	{
		count = MAX_NUM_PACKED_ENTITY_NUMS;
		Com_Printf( S_COLOR_YELLOW "WARNING: A maximum of %d entity numbers can be "
		            "packed, but BG_PackEntityNumbers was passed %d entities\n",
		            MAX_NUM_PACKED_ENTITY_NUMS, count );
	}

	es->misc = es->time = es->time2 = es->constantLight = 0;

	for ( i = 0; i < MAX_NUM_PACKED_ENTITY_NUMS; i++ )
	{
		int entityNum;

		if ( i < count )
		{
			entityNum = entityNums[ i ];
		}
		else
		{
			entityNum = ENTITYNUM_NONE;
		}

		if ( entityNum & ~GENTITYNUM_MASK )
		{
			Com_Error( ERR_FATAL, "BG_PackEntityNumbers passed an entity number (%d) which "
			           "exceeds %d bits", entityNum, GENTITYNUM_BITS );
		}

		switch ( i )
		{
			case 0:
				es->misc |= entityNum;
				break;

			case 1:
				es->time |= entityNum;
				break;

			case 2:
				es->time |= entityNum << GENTITYNUM_BITS;
				break;

			case 3:
				es->time |= entityNum << ( GENTITYNUM_BITS * 2 );
				break;

			case 4:
				es->time2 |= entityNum;
				break;

			case 5:
				es->time2 |= entityNum << GENTITYNUM_BITS;
				break;

			case 6:
				es->time2 |= entityNum << ( GENTITYNUM_BITS * 2 );
				break;

			case 7:
				es->constantLight |= entityNum;
				break;

			case 8:
				es->constantLight |= entityNum << GENTITYNUM_BITS;
				break;

			case 9:
				es->constantLight |= entityNum << ( GENTITYNUM_BITS * 2 );
				break;

			default:
				Com_Error( ERR_FATAL, "Entity index %d not handled", i );
		}
	}
}

/*
===============
BG_UnpackEntityNumbers

Unpack entity numbers from an entityState_t
===============
*/
int BG_UnpackEntityNumbers( entityState_t *es, int *entityNums, int count )
{
	int i;

	if ( count > MAX_NUM_PACKED_ENTITY_NUMS )
	{
		count = MAX_NUM_PACKED_ENTITY_NUMS;
	}

	for ( i = 0; i < count; i++ )
	{
		int *entityNum = &entityNums[ i ];

		switch ( i )
		{
			case 0:
				*entityNum = es->misc;
				break;

			case 1:
				*entityNum = es->time;
				break;

			case 2:
				*entityNum = ( es->time >> GENTITYNUM_BITS );
				break;

			case 3:
				*entityNum = ( es->time >> ( GENTITYNUM_BITS * 2 ) );
				break;

			case 4:
				*entityNum = es->time2;
				break;

			case 5:
				*entityNum = ( es->time2 >> GENTITYNUM_BITS );
				break;

			case 6:
				*entityNum = ( es->time2 >> ( GENTITYNUM_BITS * 2 ) );
				break;

			case 7:
				*entityNum = es->constantLight;
				break;

			case 8:
				*entityNum = ( es->constantLight >> GENTITYNUM_BITS );
				break;

			case 9:
				*entityNum = ( es->constantLight >> ( GENTITYNUM_BITS * 2 ) );
				break;

			default:
				Com_Error( ERR_FATAL, "Entity index %d not handled", i );
		}

		*entityNum &= GENTITYNUM_MASK;

		if ( *entityNum == ENTITYNUM_NONE )
		{
			break;
		}
	}

	return i;
}

/*
===============
BG_ParseCSVEquipmentList
===============
*/
void BG_ParseCSVEquipmentList( const char *string, weapon_t *weapons, int weaponsSize,
                               upgrade_t *upgrades, int upgradesSize )
{
	char     buffer[ MAX_STRING_CHARS ];
	int      i = 0, j = 0;
	char     *p, *q;
	qboolean EOS = qfalse;

	Q_strncpyz( buffer, string, MAX_STRING_CHARS );

	p = q = buffer;

	while ( *p != '\0' )
	{
		//skip to first , or EOS
		while ( *p != ',' && *p != '\0' )
		{
			p++;
		}

		if ( *p == '\0' )
		{
			EOS = qtrue;
		}

		*p = '\0';

		//strip leading whitespace
		while ( *q == ' ' )
		{
			q++;
		}

		if ( weaponsSize )
		{
			weapons[ i ] = BG_WeaponByName( q )->number;
		}

		if ( upgradesSize )
		{
			upgrades[ j ] = BG_UpgradeByName( q )->number;
		}

		if ( weaponsSize && weapons[ i ] == WP_NONE &&
		     upgradesSize && upgrades[ j ] == UP_NONE )
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: unknown equipment %s\n", q );
		}
		else if ( weaponsSize && weapons[ i ] != WP_NONE )
		{
			i++;
		}
		else if ( upgradesSize && upgrades[ j ] != UP_NONE )
		{
			j++;
		}

		if ( !EOS )
		{
			p++;
			q = p;
		}
		else
		{
			break;
		}

		if ( i == ( weaponsSize - 1 ) || j == ( upgradesSize - 1 ) )
		{
			break;
		}
	}

	if ( weaponsSize )
	{
		weapons[ i ] = WP_NONE;
	}

	if ( upgradesSize )
	{
		upgrades[ j ] = UP_NONE;
	}
}

/*
===============
BG_ParseCSVClassList
===============
*/
void BG_ParseCSVClassList( const char *string, class_t *classes, int classesSize )
{
	char     buffer[ MAX_STRING_CHARS ];
	int      i = 0;
	char     *p, *q;
	qboolean EOS = qfalse;

	Q_strncpyz( buffer, string, MAX_STRING_CHARS );

	p = q = buffer;

	while ( *p != '\0' && i < classesSize - 1 )
	{
		//skip to first , or EOS
		while ( *p != ',' && *p != '\0' )
		{
			p++;
		}

		if ( *p == '\0' )
		{
			EOS = qtrue;
		}

		*p = '\0';

		//strip leading whitespace
		while ( *q == ' ' )
		{
			q++;
		}

		classes[ i ] = BG_ClassByName( q )->number;

		if ( classes[ i ] == PCL_NONE )
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: unknown class %s\n", q );
		}
		else
		{
			i++;
		}

		if ( !EOS )
		{
			p++;
			q = p;
		}
		else
		{
			break;
		}
	}

	classes[ i ] = PCL_NONE;
}

/*
===============
BG_ParseCSVBuildableList
===============
*/
void BG_ParseCSVBuildableList( const char *string, buildable_t *buildables, int buildablesSize )
{
	char     buffer[ MAX_STRING_CHARS ];
	int      i = 0;
	char     *p, *q;
	qboolean EOS = qfalse;

	Q_strncpyz( buffer, string, MAX_STRING_CHARS );

	p = q = buffer;

	while ( *p != '\0' && i < buildablesSize - 1 )
	{
		//skip to first , or EOS
		while ( *p != ',' && *p != '\0' )
		{
			p++;
		}

		if ( *p == '\0' )
		{
			EOS = qtrue;
		}

		*p = '\0';

		//strip leading whitespace
		while ( *q == ' ' )
		{
			q++;
		}

		buildables[ i ] = BG_BuildableByName( q )->number;

		if ( buildables[ i ] == BA_NONE )
		{
			Com_Printf( S_COLOR_YELLOW "WARNING: unknown buildable %s\n", q );
		}
		else
		{
			i++;
		}

		if ( !EOS )
		{
			p++;
			q = p;
		}
		else
		{
			break;
		}
	}

	buildables[ i ] = BA_NONE;
}

typedef struct gameElements_s
{
	buildable_t buildables[ BA_NUM_BUILDABLES ];
	class_t     classes[ PCL_NUM_CLASSES ];
	weapon_t    weapons[ WP_NUM_WEAPONS ];
	upgrade_t   upgrades[ UP_NUM_UPGRADES ];
} gameElements_t;

static gameElements_t bg_disabledGameElements;

/*
============
BG_InitAllowedGameElements
============
*/
void BG_InitAllowedGameElements( void )
{
	char cvar[ MAX_CVAR_VALUE_STRING ];

	trap_Cvar_VariableStringBuffer( "g_disabledEquipment",
	                                cvar, MAX_CVAR_VALUE_STRING );

	BG_ParseCSVEquipmentList( cvar,
	                          bg_disabledGameElements.weapons, WP_NUM_WEAPONS,
	                          bg_disabledGameElements.upgrades, UP_NUM_UPGRADES );

	trap_Cvar_VariableStringBuffer( "g_disabledClasses",
	                                cvar, MAX_CVAR_VALUE_STRING );

	BG_ParseCSVClassList( cvar,
	                      bg_disabledGameElements.classes, PCL_NUM_CLASSES );

	trap_Cvar_VariableStringBuffer( "g_disabledBuildables",
	                                cvar, MAX_CVAR_VALUE_STRING );

	BG_ParseCSVBuildableList( cvar,
	                          bg_disabledGameElements.buildables, BA_NUM_BUILDABLES );
}

/*
============
BG_WeaponIsAllowed
============
*/
qboolean BG_WeaponIsAllowed( weapon_t weapon )
{
	int i;

	for ( i = 0; i < WP_NUM_WEAPONS &&
	      bg_disabledGameElements.weapons[ i ] != WP_NONE; i++ )
	{
		if ( bg_disabledGameElements.weapons[ i ] == weapon )
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
============
BG_UpgradeIsAllowed
============
*/
qboolean BG_UpgradeIsAllowed( upgrade_t upgrade )
{
	int i;

	for ( i = 0; i < UP_NUM_UPGRADES &&
	      bg_disabledGameElements.upgrades[ i ] != UP_NONE; i++ )
	{
		if ( bg_disabledGameElements.upgrades[ i ] == upgrade )
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
============
BG_ClassIsAllowed
============
*/
qboolean BG_ClassIsAllowed( class_t class )
{
	int i;

	for ( i = 0; i < PCL_NUM_CLASSES &&
	      bg_disabledGameElements.classes[ i ] != PCL_NONE; i++ )
	{
		if ( bg_disabledGameElements.classes[ i ] == class )
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
============
BG_BuildableIsAllowed
============
*/
qboolean BG_BuildableIsAllowed( buildable_t buildable )
{
	int i;

	for ( i = 0; i < BA_NUM_BUILDABLES &&
	      bg_disabledGameElements.buildables[ i ] != BA_NONE; i++ )
	{
		if ( bg_disabledGameElements.buildables[ i ] == buildable )
		{
			return qfalse;
		}
	}

	return qtrue;
}

/*
============
BG_PrimaryWeapon
============
*/
weapon_t BG_PrimaryWeapon( int stats[] )
{
	int i;

	for ( i = WP_NONE; i < WP_NUM_WEAPONS; i++ )
	{
		if ( BG_Weapon( i )->slots != SLOT_WEAPON )
		{
			continue;
		}

		if ( BG_InventoryContainsWeapon( i, stats ) )
		{
			return i;
		}
	}

	if ( BG_InventoryContainsWeapon( WP_BLASTER, stats ) )
	{
		return WP_BLASTER;
	}

	return WP_NONE;
}

/*
============
BG_LoadEmoticons
============
*/
int BG_LoadEmoticons( emoticon_t *emoticons, int num )
{
	int  numFiles;
	char fileList[ MAX_EMOTICONS * ( MAX_EMOTICON_NAME_LEN + 9 ) ] = { "" };
	int  i;
	char *filePtr;
	int  fileLen;
	int  count;

	numFiles = trap_FS_GetFileList( "emoticons", "x1.tga", fileList,
	                                sizeof( fileList ) );

	if ( numFiles < 1 )
	{
		return 0;
	}

	filePtr = fileList;
	fileLen = 0;
	count = 0;

	for ( i = 0; i < numFiles && count < num; i++, filePtr += fileLen + 1 )
	{
		fileLen = strlen( filePtr );

		if ( fileLen < 9 || filePtr[ fileLen - 8 ] != '_' ||
		     filePtr[ fileLen - 7 ] < '1' || filePtr[ fileLen - 7 ] > '9' )
		{
			Com_Printf( S_COLOR_YELLOW "skipping invalidly named emoticon \"%s\"\n",
			            filePtr );
			continue;
		}

		if ( fileLen - 8 > MAX_EMOTICON_NAME_LEN )
		{
			Com_Printf( S_COLOR_YELLOW "emoticon file name \"%s\" too long (>%d)\n",
			            filePtr, MAX_EMOTICON_NAME_LEN + 8 );
			continue;
		}

		if ( !trap_FS_FOpenFile( va( "emoticons/%s", filePtr ), NULL, FS_READ ) )
		{
			Com_Printf( S_COLOR_YELLOW "could not open \"emoticons/%s\"\n", filePtr );
			continue;
		}

		Q_strncpyz( emoticons[ count ].name, filePtr, fileLen - 8 + 1 );
#ifndef GAME
		emoticons[ count ].width = filePtr[ fileLen - 7 ] - '0';
#endif
		count++;
	}

	// Com_Printf( "Loaded %d of %d emoticons (MAX_EMOTICONS is %d)\n", // FIXME PLURAL
	//             count, numFiles, MAX_EMOTICONS );

	return count;
}

/*
============
BG_TeamName
============
*/
const char *BG_TeamName( team_t team )
{
	if ( team == TEAM_NONE )
	{
		return N_("spectator");
	}

	if ( team == TEAM_ALIENS )
	{
		return N_("alien");
	}

	if ( team == TEAM_HUMANS )
	{
		return N_("human");
	}

	return "<team>";
}

const char *BG_TeamNamePlural( team_t team )
{
	if ( team == TEAM_NONE )
	{
		return N_("spectators");
	}

	if ( team == TEAM_ALIENS )
	{
		return N_("aliens");
	}

	if ( team == TEAM_HUMANS )
	{
		return N_("humans");
	}

	return "<team>";
}

int cmdcmp( const void *a, const void *b )
{
	return b ? Q_stricmp( ( const char * ) a, ( ( dummyCmd_t * ) b )->name ) : 1;
}

/*
==================
Quote
==================
*/

char *Quote( const char *str )
{
	static char buffer[ 4 ][ MAX_STRING_CHARS ];
	static int index = -1;

	index = ( index + 1 ) & 3;
	trap_QuoteString( str, buffer[ index ], sizeof( buffer[ index ] ) );

	return buffer[ index ];
}

/*
=================
Substring
=================
*/

char *Substring( const char *in, int start, int count )
{
	static char buffer[ MAX_STRING_CHARS ];
	char        *buf = buffer;

	memset( &buffer, 0, sizeof( buffer ) );

	Q_strncpyz( buffer, in+start, count );

	return buf;
}

/*
=================
BG_strdup
=================
*/

char *BG_strdup( const char *string )
{
	size_t length;
	char *copy;

	length = strlen(string) + 1;
	copy = (char *)BG_Alloc(length);

	if ( copy == NULL )
	{
		return NULL;
	}

	memcpy( copy, string, length );
	return copy;
}



/*
=================
Trans_GenderContext
=================
*/

const char *Trans_GenderContext( gender_t gender )
{
	switch ( gender )
	{
		case GENDER_MALE:
			return "male";
			break;
		case GENDER_FEMALE:
			return "female";
			break;
		case GENDER_NEUTER:
			return "neuter";
			break;
		default:
			return "unknown";
			break;
	}
}
