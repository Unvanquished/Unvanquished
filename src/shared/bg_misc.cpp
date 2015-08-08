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

#include "engine/qcommon/q_shared.h"
#include "bg_public.h"

#define N_(x) x

int                                trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
int                                trap_FS_Read( void *buffer, int len, fileHandle_t f );
int                                trap_FS_Write( const void *buffer, int len, fileHandle_t f );
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
	{ BA_A_SPAWN,     "eggpod",    "team_alien_spawn"     },
	{ BA_A_OVERMIND,  "overmind",  "team_alien_overmind"  },
	{ BA_A_BARRICADE, "barricade", "team_alien_barricade" },
	{ BA_A_ACIDTUBE,  "acid_tube", "team_alien_acid_tube" },
	{ BA_A_TRAPPER,   "trapper",   "team_alien_trapper"   },
	{ BA_A_BOOSTER,   "booster",   "team_alien_booster"   },
	{ BA_A_HIVE,      "hive",      "team_alien_hive"      },
	{ BA_A_LEECH,     "leech",     "team_alien_leech"     },
	{ BA_A_SPIKER,    "spiker",    "team_alien_spiker"    },
	{ BA_H_SPAWN,     "telenode",  "team_human_spawn"     },
	{ BA_H_MGTURRET,  "mgturret",  "team_human_mgturret"  },
	{ BA_H_ROCKETPOD, "rocketpod", "team_human_rocketpod" },
	{ BA_H_ARMOURY,   "arm",       "team_human_armoury"   },
	{ BA_H_MEDISTAT,  "medistat",  "team_human_medistat"  },
 	{ BA_H_DRILL,     "drill",     "team_human_drill"     },
	{ BA_H_REACTOR,   "reactor",   "team_human_reactor"   },
	{ BA_H_REPEATER,  "repeater",  "team_human_repeater"  }
};

static const size_t bg_numBuildables = ARRAY_LEN( bg_buildableNameList );

static buildableAttributes_t bg_buildableList[ ARRAY_LEN( bg_buildableNameList ) ];

static const buildableAttributes_t nullBuildable {};

/*
==============
BG_BuildableByName
==============
*/
const buildableAttributes_t *BG_BuildableByName( const char *name )
{
	for ( unsigned i = 0; i < bg_numBuildables; i++ )
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
	for ( unsigned i = 0; i < bg_numBuildables; i++ )
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
const buildableAttributes_t *BG_Buildable( int buildable )
{
	return ( buildable > BA_NONE && buildable < BA_NUM_BUILDABLES ) ?
	       &bg_buildableList[ buildable - 1 ] : &nullBuildable;
}

/*
===============
BG_InitBuildableAttributes
===============
*/
void BG_InitBuildableAttributes()
{
	const buildableName_t *bh;
	buildableAttributes_t *ba;

	for ( unsigned i = 0; i < bg_numBuildables; i++ )
	{
		bh = &bg_buildableNameList[i];
		ba = &bg_buildableList[i];

		//Initialise default values for buildables
		Com_Memset( ba, 0, sizeof( buildableAttributes_t ) );

		ba->number = bh->number;
		ba->name = bh->name;
		ba->entityName = bh->classname;

		ba->traj = TR_GRAVITY;
		ba->bounce = 0.0;
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
buildableModelConfig_t *BG_BuildableModelConfig( int buildable )
{
	return &bg_buildableModelConfigList[ buildable ];
}

/*
==============
BG_BuildableBoundingBox
==============
*/
void BG_BuildableBoundingBox( int buildable,
                              vec3_t mins, vec3_t maxs )
{
	buildableModelConfig_t *buildableModelConfig = BG_BuildableModelConfig( buildable );

	if ( mins != nullptr )
	{
		VectorCopy( buildableModelConfig->mins, mins );
	}

	if ( maxs != nullptr )
	{
		VectorCopy( buildableModelConfig->maxs, maxs );
	}
}

/*
===============
BG_InitBuildableModelConfigs
===============
*/
void BG_InitBuildableModelConfigs()
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
} classData_t;

static classData_t bg_classData[] =
{
	{
		PCL_NONE, //int     number;
		"spectator", //char    *name;
		WP_NONE //weapon_t  startWeapon;
	},
	{
		PCL_ALIEN_BUILDER0, //int     number;
		"builder", //char    *name;
		WP_ABUILD //weapon_t  startWeapon;
	},
	{
		PCL_ALIEN_BUILDER0_UPG, //int     number;
		"builderupg", //char    *name;
		WP_ABUILD2 //weapon_t  startWeapon;
	},
	{
		PCL_ALIEN_LEVEL0, //int     number;
		"level0", //char    *name;
		WP_ALEVEL0 //weapon_t  startWeapon;
	},
	{
		PCL_ALIEN_LEVEL1, //int     number;
		"level1", //char    *name;
		WP_ALEVEL1 //weapon_t  startWeapon;
	},
	{
		PCL_ALIEN_LEVEL2, //int     number;
		"level2", //char    *name;
		WP_ALEVEL2 //weapon_t  startWeapon;
	},
	{
		PCL_ALIEN_LEVEL2_UPG, //int     number;
		"level2upg", //char    *name;
		WP_ALEVEL2_UPG //weapon_t  startWeapon;
	},
	{
		PCL_ALIEN_LEVEL3, //int     number;
		"level3", //char    *name;
		WP_ALEVEL3 //weapon_t  startWeapon;
	},
	{
		PCL_ALIEN_LEVEL3_UPG, //int     number;
		"level3upg", //char    *name;
		WP_ALEVEL3_UPG //weapon_t  startWeapon;
	},
	{
		PCL_ALIEN_LEVEL4, //int     number;
		"level4", //char    *name;
		WP_ALEVEL4 //weapon_t  startWeapon;
	},
	{
		PCL_HUMAN_NAKED, //int     number;
		"human_naked", //char    *name;
		WP_NONE //special-cased in g_client.c          //weapon_t  startWeapon;
	},
    {
		PCL_HUMAN_LIGHT, //int     number;
		"human_light", //char    *name;
		WP_NONE //special-cased in g_client.c          //weapon_t  startWeapon;
	},
    {
		PCL_HUMAN_MEDIUM, //int     number;
		"human_medium", //char    *name;
		WP_NONE //special-cased in g_client.c          //weapon_t  startWeapon;
	},
	{
		PCL_HUMAN_BSUIT, //int     number;
		"human_bsuit", //char    *name;
		WP_NONE //special-cased in g_client.c          //weapon_t  startWeapon;
	}
};

static const size_t bg_numClasses = ARRAY_LEN( bg_classData );

static classAttributes_t bg_classList[ ARRAY_LEN( bg_classData ) ];

static const classAttributes_t nullClass {};
static /*const*/ classModelConfig_t nullClassModelConfig {};

/*
==============
BG_ClassByName
==============
*/
const classAttributes_t *BG_ClassByName( const char *name )
{
	if ( name )
	{
		for ( unsigned i = 0; i < bg_numClasses; i++ )
		{
			if ( !Q_stricmp( bg_classList[ i ].name, name ) )
			{
				return &bg_classList[ i ];
			}
		}
	}

	return &nullClass;
}

/*
==============
BG_Class
==============
*/
const classAttributes_t *BG_Class( int pClass )
{
	return ( pClass >= PCL_NONE && pClass < PCL_NUM_CLASSES ) ?
	       &bg_classList[ pClass ] : &nullClass;
}

static classModelConfig_t bg_classModelConfigList[ PCL_NUM_CLASSES ];

/*
==============
BG_ClassModelConfigByName
==============
*/
classModelConfig_t *BG_ClassModelConfigByName( const char *name )
{
	if ( name )
	{
		for ( unsigned i = 0; i < bg_numClasses; i++ )
		{
			if ( !Q_stricmp( bg_classModelConfigList[ i ].humanName, name ) )
			{
				return &bg_classModelConfigList[ i ];
			}
		}
	}

	return &nullClassModelConfig;
}

/*
==============
BG_ClassModelConfig
==============
*/
classModelConfig_t *BG_ClassModelConfig( int pClass )
{
	return &bg_classModelConfigList[ pClass ];
}

/*
==============
BG_ClassBoundingBox
==============
*/
void BG_ClassBoundingBox( int pClass,
                          vec3_t mins, vec3_t maxs,
                          vec3_t cmaxs, vec3_t dmins, vec3_t dmaxs )
{
	classModelConfig_t *classModelConfig = BG_ClassModelConfig( pClass );

	if ( mins != nullptr )
	{
		VectorCopy( classModelConfig->mins, mins );
	}

	if ( maxs != nullptr )
	{
		VectorCopy( classModelConfig->maxs, maxs );
	}

	if ( cmaxs != nullptr )
	{
		VectorCopy( classModelConfig->crouchMaxs, cmaxs );
	}

	if ( dmins != nullptr )
	{
		VectorCopy( classModelConfig->deadMins, dmins );
	}

	if ( dmaxs != nullptr )
	{
		VectorCopy( classModelConfig->deadMaxs, dmaxs );
	}
}

team_t BG_ClassTeam( int pClass )
{
	return BG_Class( pClass )->team;
}

/*
==============
BG_ClassHasAbility
==============
*/
bool BG_ClassHasAbility( int pClass, int ability )
{
	int abilities = BG_Class( pClass )->abilities;

	return abilities & ability;
}

/*
==============
BG_ClassCanEvolveFromTo
==============
*/
int BG_ClassCanEvolveFromTo( int from, int to, int credits )
{
	int fromCost, toCost, evolveCost;

	if ( from == to ||
	     from <= PCL_NONE || from >= PCL_NUM_CLASSES ||
	     to <= PCL_NONE || to >= PCL_NUM_CLASSES )
	{
		return -1;
	}

	if ( !BG_ClassUnlocked( to ) || BG_ClassDisabled( to ) )
	{
		return -1;
	}

	fromCost = BG_Class( from )->cost;
	toCost = BG_Class( to )->cost;

	// classes w/o a cost are for spawning only
	if ( toCost == 0 )
	{
		// (adv.) granger may evolve into adv. granger or dretch at no cost
		if ( ( from == PCL_ALIEN_BUILDER0 || from == PCL_ALIEN_BUILDER0_UPG ) &&
		     ( to == PCL_ALIEN_BUILDER0_UPG || to == PCL_ALIEN_LEVEL0 ) )
		{
			return 0;
		}

		return -1;
	}

	// don't allow devolving
	if ( toCost <= fromCost )
	{
		return -1;
	}

	evolveCost = toCost - fromCost;

	if ( credits < evolveCost )
	{
		return -1;
	}

	return evolveCost;
}

/*
==============
BG_AlienCanEvolve
==============
*/
bool BG_AlienCanEvolve( int from, int credits )
{
	int to;

	for ( to = PCL_NONE + 1; to < PCL_NUM_CLASSES; to++ )
	{
		if ( BG_ClassCanEvolveFromTo( from, to, credits ) >= 0 )
		{
			return true;
		}
	}

	return false;
}

/*
===============
BG_InitClassAttributes
===============
*/
void BG_InitClassAttributes()
{
	const classData_t *cd;
	classAttributes_t *ca;

	for ( unsigned i = 0; i < bg_numClasses; i++ )
	{
		cd = &bg_classData[i];
		ca = &bg_classList[i];

		ca->number = cd->number;
		ca->name = cd->name;
		ca->startWeapon = cd->startWeapon;

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
void BG_InitClassModelConfigs()
{
	int           i;
	classModelConfig_t *cc;

	for ( i = PCL_NONE; i < PCL_NUM_CLASSES; i++ )
	{
		cc = BG_ClassModelConfig( i );

		BG_ParseClassModelFile( va( "configs/classes/%s.model.cfg",
		                       BG_Class( i )->name ), cc );

		cc->segmented = cc->modelName[0] ? BG_NonSegModel( va( "models/players/%s/animation.cfg", cc->modelName ) ) : false;
	}
}

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	weapon_t number;
	const char* name;
} weaponData_t;

static const weaponData_t bg_weaponsData[] =
{
	{ WP_ALEVEL0,           "level0"    },
	{ WP_ALEVEL1,           "level1"    },
	{ WP_ALEVEL2,           "level2"    },
	{ WP_ALEVEL2_UPG,       "level2upg" },
	{ WP_ALEVEL3,           "level3"    },
	{ WP_ALEVEL3_UPG,       "level3upg" },
	{ WP_ALEVEL4,           "level4"    },
	{ WP_BLASTER,           "blaster"   },
	{ WP_MACHINEGUN,        "rifle"     },
	{ WP_PAIN_SAW,          "psaw"      },
	{ WP_SHOTGUN,           "shotgun"   },
	{ WP_LAS_GUN,           "lgun"      },
	{ WP_MASS_DRIVER,       "mdriver"   },
	{ WP_CHAINGUN,          "chaingun"  },
	{ WP_FLAMER,            "flamer"    },
	{ WP_PULSE_RIFLE,       "prifle"    },
	{ WP_LUCIFER_CANNON,    "lcannon"   },
	{ WP_LOCKBLOB_LAUNCHER, "lockblob"  },
	{ WP_HIVE,              "hive"      },
	{ WP_ROCKETPOD,         "rocketpod" },
	{ WP_MGTURRET,          "mgturret"  },
	{ WP_ABUILD,            "abuild"    },
	{ WP_ABUILD2,           "abuildupg" },
	{ WP_HBUILD,            "ckit"      }
};

static const size_t bg_numWeapons = ARRAY_LEN( bg_weaponsData );

static weaponAttributes_t bg_weapons[ ARRAY_LEN( bg_weaponsData ) ];

static const weaponAttributes_t nullWeapon {};

weapon_t BG_WeaponNumberByName( const char *name )
{
	for ( unsigned i = 0; i < bg_numWeapons; i++ )
	{
		if ( !Q_stricmp( bg_weaponsData[ i ].name, name ) )
		{
			return bg_weaponsData[ i ].number;
		}
	}

	return ( weapon_t )0;
}

const weaponAttributes_t *BG_WeaponByName( const char *name )
{
	weapon_t weapon = BG_WeaponNumberByName( name );

	if ( weapon )
	{
		return &bg_weapons[ weapon ];
	}
	else
	{
		return &nullWeapon;
	}
}

/*
==============
BG_Weapon
==============
*/
const weaponAttributes_t *BG_Weapon( int weapon )
{
	return ( weapon > WP_NONE && weapon < WP_NUM_WEAPONS ) ?
	       &bg_weapons[ weapon - 1 ] : &nullWeapon;
}

/*
===============
BG_InitWeaponAttributes
===============
*/
void BG_InitWeaponAttributes()
{
	const weaponData_t *wd;
	weaponAttributes_t *wa;

	for ( unsigned i = 0; i < bg_numWeapons; i++ )
	{
		wd = &bg_weaponsData[i];
		wa = &bg_weapons[i];

		Com_Memset( wa, 0, sizeof( weaponAttributes_t ) );

		wa->number = wd->number;
		wa->name   = wd->name;

		// set default values for optional fields
		wa->knockbackScale = 1.0f;

		BG_ParseWeaponAttributeFile( va( "configs/weapon/%s.attr.cfg", wa->name ), wa );
	}
}



////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	upgrade_t number;
	const char* name;
} upgradeData_t;


static const upgradeData_t bg_upgradesData[] =
{
	{ UP_LIGHTARMOUR, "larmour"  },
	{ UP_MEDIUMARMOUR,"marmour"  },
	{ UP_BATTLESUIT,  "bsuit"    },

	{ UP_RADAR,       "radar"    },
	{ UP_JETPACK,     "jetpack"  },

	{ UP_GRENADE,     "gren"     },
	{ UP_FIREBOMB,    "firebomb" },

	{ UP_MEDKIT,      "medkit"   }
};

static const size_t bg_numUpgrades = ARRAY_LEN( bg_upgradesData );

static upgradeAttributes_t bg_upgrades[ ARRAY_LEN( bg_upgradesData ) ];

static const upgradeAttributes_t nullUpgrade {};

/*
==============
BG_UpgradeByName
==============
*/
const upgradeAttributes_t *BG_UpgradeByName( const char *name )
{
	for ( unsigned i = 0; i < bg_numUpgrades; i++ )
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
const upgradeAttributes_t *BG_Upgrade( int upgrade )
{
	return ( upgrade > UP_NONE && upgrade < UP_NUM_UPGRADES ) ?
	       &bg_upgrades[ upgrade - 1 ] : &nullUpgrade;
}

/*
===============
BG_InitUpgradeAttributes
===============
*/
void BG_InitUpgradeAttributes()
{
	const upgradeData_t *ud;
	upgradeAttributes_t *ua;

	for ( unsigned i = 0; i < bg_numUpgrades; i++ )
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

typedef struct
{
	missile_t   number;
	const char* name;
} missileData_t;

static const missileData_t bg_missilesData[] =
{
  { MIS_FLAMER,       "flamer"       },
  { MIS_BLASTER,      "blaster"      },
  { MIS_PRIFLE,       "prifle"       },
  { MIS_LCANNON,      "lcannon"      },
  { MIS_LCANNON2,     "lcannon2"     },
  { MIS_GRENADE,      "grenade"      },
  { MIS_FIREBOMB,     "firebomb"     },
  { MIS_FIREBOMB_SUB, "firebomb_sub" },
  { MIS_HIVE,         "hive"         },
  { MIS_LOCKBLOB,     "lockblob"     },
  { MIS_SLOWBLOB,     "slowblob"     },
  { MIS_BOUNCEBALL,   "bounceball"   },
  { MIS_ROCKET,       "rocket"       },
  { MIS_SPIKER,       "spiker"       }
};

static const size_t              bg_numMissiles = ARRAY_LEN( bg_missilesData );
static missileAttributes_t       bg_missiles[ ARRAY_LEN( bg_missilesData ) ];
static const missileAttributes_t nullMissile {};

/*
==============
BG_MissileByName
==============
*/
const missileAttributes_t *BG_MissileByName( const char *name )
{
	for ( unsigned i = 0; i < bg_numMissiles; i++ )
	{
		if ( !Q_stricmp( bg_missiles[ i ].name, name ) )
		{
			return &bg_missiles[ i ];
		}
	}

	return &nullMissile;
}

/*
==============
BG_Missile
==============
*/
const missileAttributes_t *BG_Missile( int missile )
{
	return ( missile > MIS_NONE && missile < MIS_NUM_MISSILES ) ?
	       &bg_missiles[ missile - 1 ] : &nullMissile;
}

/*
===============
BG_InitMissileAttributes
===============
*/
void BG_InitMissileAttributes()
{
	const missileData_t *md;
	missileAttributes_t *ma;

	for ( unsigned i = 0; i < bg_numMissiles; i++ )
	{
		md = &bg_missilesData[i];
		ma = &bg_missiles[i];

		Com_Memset( ma, 0, sizeof( missileAttributes_t ) );

		ma->name   = md->name;
		ma->number = md->number;

		// for simplicity, read both from a single file
		BG_ParseMissileAttributeFile( va( "configs/missiles/%s.missile.cfg", ma->name ), ma );
		BG_ParseMissileDisplayFile(   va( "configs/missiles/%s.missile.cfg", ma->name ), ma );
	}
}

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	meansOfDeath_t number;
	const char     *name;
} meansOfDeathData_t;

static const meansOfDeathData_t bg_meansOfDeathData[] =
{
	{ MOD_ABUILDER_CLAW, "MOD_ABUILDER_CLAW" },
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
	{ MOD_NOCREEP, "MOD_NOCREEP" }
};

static const size_t bg_numMeansOfDeath = ARRAY_LEN( bg_meansOfDeathData );

/*
==============
BG_MeansOfDeathByName
==============
*/
meansOfDeath_t BG_MeansOfDeathByName( const char *name )
{
	for ( unsigned i = 0; i < bg_numMeansOfDeath; i++ )
	{
		if ( !Q_stricmp( bg_meansOfDeathData[ i ].name, name ) )
		{
			return bg_meansOfDeathData[ i ].number;
		}
	}

	return MOD_UNKNOWN;
}

////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	beaconType_t  number;
	const char*   name;
	int           flags;
} beaconData_t;

static const beaconAttributes_t nullBeacon {};

static const beaconData_t bg_beaconsData[ ] =
{
	{ BCT_POINTER,       "pointer",       BCF_IMPORTANT | BCF_PER_PLAYER | BCF_PRECISE },
	{ BCT_TIMER,         "timer",         BCF_IMPORTANT | BCF_PER_PLAYER },
	{ BCT_TAG,           "tag",           BCF_RESERVED },
	{ BCT_BASE,          "base",          BCF_RESERVED },
	{ BCT_ATTACK,        "attack",        BCF_IMPORTANT },
	{ BCT_DEFEND,        "defend",        BCF_IMPORTANT },
	{ BCT_REPAIR,        "repair",        BCF_IMPORTANT },
	{ BCT_HEALTH,        "health",        BCF_RESERVED },
	{ BCT_AMMO,          "ammo",          BCF_RESERVED }
};

static const size_t bg_numBeacons = ARRAY_LEN( bg_beaconsData );
static beaconAttributes_t bg_beacons[ ARRAY_LEN( bg_beaconsData ) ];

/*
================
BG_BeaconByName
================
*/
const beaconAttributes_t *BG_BeaconByName( const char *name )
{
	for ( unsigned i = 0; i < bg_numBeacons; i++ )
		if ( !Q_stricmp( bg_beacons[ i ].name, name ) )
			return bg_beacons + i;

	return nullptr;
}

/*
================
BG_Beacon
================
*/
const beaconAttributes_t *BG_Beacon( int index )
{
	if( index > BCT_NONE && index < NUM_BEACON_TYPES )
		return bg_beacons + index - 1;

	return &nullBeacon;
}

/*
===============
BG_InitBeaconAttributes
===============
*/
void BG_InitBeaconAttributes()
{
	const beaconData_t *bd;
	beaconAttributes_t *ba;

	for ( unsigned i = 0; i < bg_numBeacons; i++ )
	{
		bd = bg_beaconsData + i;
		ba = bg_beacons + i;

		Com_Memset( ba, 0, sizeof( beaconAttributes_t ) );

		ba->number = bd->number;
		ba->name = bd->name;
		ba->flags = bd->flags;

		BG_ParseBeaconAttributeFile( va( "configs/beacons/%s.beacon.cfg", bd->name ), ba );
	}

	//hardcoded
	bg_beacons[ BCT_TIMER - 1 ].decayTime = BEACON_TIMER_TIME + 1000;
}

////////////////////////////////////////////////////////////////////////////////

/*
================
BG_InitAllConfigs

================
*/

bool config_loaded = false;

void BG_InitAllConfigs()
{
	BG_InitBuildableAttributes();
	BG_InitBuildableModelConfigs();
	BG_InitClassAttributes();
	BG_InitClassModelConfigs();
	BG_InitWeaponAttributes();
	BG_InitUpgradeAttributes();
	BG_InitMissileAttributes();
	BG_InitBeaconAttributes();

	BG_CheckConfigVars();

	config_loaded = true;
}

/*
================
BG_UnloadAllConfigs

================
*/

void BG_UnloadAllConfigs()
{
    // Frees all the strings that were allocated when the config files were read

    // When the game starts VMs are shutdown before they are even started
    if(!config_loaded){
        return;
    }
    config_loaded = false;

    for ( unsigned i = 0; i < bg_numBuildables; i++ )
    {
        buildableAttributes_t *ba = &bg_buildableList[i];

        if ( ba )
        {
            BG_Free( (char *)ba->humanName );
            BG_Free( (char *)ba->info );
        }
    }

    for ( unsigned i = 0; i < bg_numClasses; i++ )
    {
        classAttributes_t *ca = &bg_classList[i];

        if ( ca )
        {
            // Do not free the statically allocated empty string
            if( ca->info && *ca->info != '\0' )
            {
                BG_Free( (char *)ca->info );
            }

            if( ca->fovCvar && *ca->fovCvar != '\0' )
            {
                BG_Free( (char *)ca->fovCvar );
            }
        }
    }

    for ( unsigned i = PCL_NONE; i < PCL_NUM_CLASSES; i++ )
    {
        BG_Free(BG_ClassModelConfig( i )->humanName);
    }

    for ( unsigned i = 0; i < bg_numWeapons; i++ )
    {
        weaponAttributes_t *wa = &bg_weapons[i];

        if ( wa )
        {
            BG_Free( (char *)wa->humanName );

            if( wa->info && *wa->info != '\0' )
            {
                BG_Free( (char *)wa->info );
            }
        }
    }

    for ( unsigned i = 0; i < bg_numUpgrades; i++ )
    {
        upgradeAttributes_t *ua = &bg_upgrades[i];

        if ( ua )
        {
            BG_Free( (char *)ua->humanName );

            if( ua->info && *ua->info != '\0' )
            {
                BG_Free( (char *)ua->info );
            }
        }
    }

    for ( unsigned i = 0; i < bg_numBeacons; i++ )
    {
		    beaconAttributes_t *ba = bg_beacons + i;

		    if ( ba )
		    {
				    BG_Free( (char *)ba->humanName );

#ifdef BUILD_CGAME
						BG_Free( (char *)ba->text[0] );
#endif
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

  "EV_JETPACK_ENABLE",  // enable jets
  "EV_JETPACK_DISABLE", // disable jets
  "EV_JETPACK_IGNITE",  // ignite engine
  "EV_JETPACK_START",   // start thrusting
  "EV_JETPACK_STOP",    // stop thrusting

  "EV_NOAMMO",
  "EV_CHANGE_WEAPON",
  "EV_FIRE_WEAPON",
  "EV_FIRE_WEAPON2",
  "EV_FIRE_WEAPON3",
  "EV_WEAPON_RELOAD",

  "EV_PLAYER_RESPAWN", // for fovwarp effects
  "EV_PLAYER_TELEPORT_IN",
  "EV_PLAYER_TELEPORT_OUT",

  "EV_GRENADE_BOUNCE", // eventParm will be the soundindex

  "EV_GENERAL_SOUND",
  "EV_GLOBAL_SOUND", // no attenuation

  "EV_WEAPON_HIT_ENTITY",
  "EV_WEAPON_HIT_ENVIRONMENT",

  "EV_SHOTGUN",
  "EV_MASS_DRIVER",

  "EV_MISSILE_HIT_ENTITY",
  "EV_MISSILE_HIT_ENVIRONMENT",
  "EV_MISSILE_HIT_METAL", // necessary?
  "EV_TESLATRAIL",
  "EV_BULLET", // otherEntity is the shooter

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
  "EV_HUMAN_BUILDABLE_DYING",
  "EV_HUMAN_BUILDABLE_EXPLOSION",
  "EV_ALIEN_BUILDABLE_EXPLOSION",
  "EV_ALIEN_ACIDTUBE",
  "EV_ALIEN_BOOSTER",

  "EV_MEDKIT_USED",

  "EV_ALIEN_EVOLVE",
  "EV_ALIEN_EVOLVE_FAILED",

  "EV_STOPLOOPINGSOUND",
  "EV_TAUNT",

  "EV_OVERMIND_ATTACK_1", // overmind under attack
  "EV_OVERMIND_ATTACK_2", // overmind under attack
  "EV_OVERMIND_DYING", // overmind close to death
  "EV_OVERMIND_SPAWNS", // overmind needs spawns

  "EV_REACTOR_ATTACK_1", // reactor under attack
  "EV_REACTOR_ATTACK_2", // reactor under attack
  "EV_REACTOR_DYING", // reactor destroyed

  "EV_WARN_ATTACK", // a building has been destroyed and the destruction noticed by a nearby om/rc/rrep

  "EV_MGTURRET_SPINUP", // turret spinup sound should play

  "EV_AMMO_REFILL",     // ammo for clipless weapon has been refilled
  "EV_CLIPS_REFILL",    // weapon clips have been refilled
  "EV_FUEL_REFILL",     // jetpack fuel has been refilled

  "EV_HIT", // notify client of a hit

  "EV_MOMENTUM" // notify client of generated momentum
};

/*
===============
BG_EventName
===============
*/
const char *BG_EventName( int num )
{
	if ( num < 0 || num >= (int) ARRAY_LEN( eventnames ) )
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
#ifdef BUILD_SGAME
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
void BG_PlayerStateToEntityState( playerState_t *ps, entityState_t *s, bool snap )
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

	// HACK: store held items in modelindex
	s->modelindex = 0;

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( BG_InventoryContainsUpgrade( i, ps->stats ) )
		{
			s->modelindex |= 1 << i;
		}
	}

	// set "public state" flags
	s->modelindex2 = 0;

	// copy jetpack state
	if ( ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ENABLED )
	{
		s->modelindex2 |= PF_JETPACK_ENABLED;
	}
	else
	{
		s->modelindex2 &= ~PF_JETPACK_ENABLED;
	}

	if ( ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
	{
		s->modelindex2 |= PF_JETPACK_ACTIVE;
	}
	else
	{
		s->modelindex2 &= ~PF_JETPACK_ACTIVE;
	}

	// use misc field to store team/class info:
	s->misc = ps->persistant[ PERS_TEAM ] | ( ps->stats[ STAT_CLASS ] << 8 );

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
void BG_PlayerStateToEntityStateExtraPolate( playerState_t *ps, entityState_t *s, int time, bool snap )
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

	// HACK: store held items in modelindex
	s->modelindex = 0;

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( BG_InventoryContainsUpgrade( i, ps->stats ) )
		{
			s->modelindex |= 1 << i;
		}
	}

	// set "public state" flags
	s->modelindex2 = 0;

	// copy jetpack state
	if ( ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ENABLED )
	{
		s->modelindex2 |= PF_JETPACK_ENABLED;
	}
	else
	{
		s->modelindex2 &= ~PF_JETPACK_ENABLED;
	}

	if ( ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
	{
		s->modelindex2 |= PF_JETPACK_ACTIVE;
	}
	else
	{
		s->modelindex2 &= ~PF_JETPACK_ACTIVE;
	}

	// use misc field to store team/class info:
	s->misc = ps->persistant[ PERS_TEAM ] | ( ps->stats[ STAT_CLASS ] << 8 );

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
bool BG_WeaponIsFull( int weapon, int ammo, int clips )
{
	int maxAmmo, maxClips;

	maxAmmo = BG_Weapon( weapon )->maxAmmo;
	maxClips = BG_Weapon( weapon )->maxClips;

	return ( maxAmmo == ammo ) && ( maxClips == clips );
}

/*
========================
BG_InventoryContainsWeapon

Does the player hold a weapon?
========================
*/
bool BG_InventoryContainsWeapon( int weapon, const int stats[] )
{
	// humans always have a blaster
	// HACK: Determine team by checking for STAT_CLASS since we merged STAT_TEAM into PERS_TEAM
	//       This hack will vanish as soon as the blast isn't the only possible sidearm weapon anymore
	if ( BG_ClassTeam( stats[ STAT_CLASS ] ) == TEAM_HUMANS && weapon == WP_BLASTER )
	{
		return true;
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

	// HACK: Determine team by checking for STAT_CLASS since we merged STAT_TEAM into PERS_TEAM
	//       This hack will vanish as soon as the blast isn't the only possible sidearm weapon anymore
	if ( BG_ClassTeam( stats[ STAT_CLASS ] ) == TEAM_HUMANS )
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
				Com_Printf( S_WARNING "held item %d conflicts with "
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
bool BG_InventoryContainsUpgrade( int item, int stats[] )
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
bool BG_UpgradeIsActive( int item, int stats[] )
{
	return ( stats[ STAT_ACTIVEITEMS ] & ( 1 << item ) );
}

/*
===============
BG_RotateAxis

Shared axis rotation function
===============
*/
bool BG_RotateAxis( vec3_t surfNormal, vec3_t inAxis[ 3 ],
                        vec3_t outAxis[ 3 ], bool inverse, bool ceiling )
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
		return false;
	}

	return true;
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
                     const vec3_t, const vec3_t, int, int, int ),
    vec3_t outOrigin, vec3_t outAngles, trace_t *tr )
{
	vec3_t aimDir, forward, entityOrigin, targetOrigin;
	vec3_t angles, playerOrigin, playerNormal;
	float  buildDist;

	BG_GetClientNormal( ps, playerNormal );

	VectorCopy( ps->viewangles, angles );
	VectorCopy( ps->origin, playerOrigin );

	AngleVectors( angles, aimDir, nullptr, nullptr );
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
	( *trace )( tr, entityOrigin, mins, maxs, targetOrigin, ps->clientNum, MASK_DEADSOLID, 0 );
	VectorCopy( tr->endpos, outOrigin );
	vectoangles( forward, outAngles );
}

/**
 * @brief Calculates the "value" of a player as a base value plus a fraction of the price the
 *        player paid for upgrades.
 * @param ps
 * @return Player value
 */
int BG_GetValueOfPlayer( playerState_t *ps )
{
	int price, upgradeNum;

	if ( !ps )
	{
		return 0;
	}

	price = 0;

	switch ( ps->persistant[ PERS_TEAM ] )
	{
		case TEAM_HUMANS:
			// Add upgrade price
			for ( upgradeNum = UP_NONE + 1; upgradeNum < UP_NUM_UPGRADES; upgradeNum++ )
			{
				if ( BG_InventoryContainsUpgrade( upgradeNum, ps->stats ) )
				{
					price += BG_Upgrade( upgradeNum )->price;
				}
			}

			// Add weapon price
			for ( upgradeNum = WP_NONE + 1; upgradeNum < WP_NUM_WEAPONS; upgradeNum++ )
			{
				if ( BG_InventoryContainsWeapon( upgradeNum, ps->stats ) )
				{
					price += BG_Weapon( upgradeNum )->price;
				}
			}

			break;

		case TEAM_ALIENS:
			price += BG_Class( ps->stats[ STAT_CLASS ] )->cost;
			break;

		default:
			return 0;
	}

	return PLAYER_BASE_VALUE + ( int )( ( float )price * PLAYER_PRICE_TO_VALUE );
}

/*
=================
BG_PlayerCanChangeWeapon
=================
*/
bool BG_PlayerCanChangeWeapon( playerState_t *ps )
{
	// Do not allow Lucifer Cannon "canceling" via weapon switch
	if ( ps->weapon == WP_LUCIFER_CANNON &&
	     ps->stats[ STAT_MISC ] > LCANNON_CHARGE_TIME_MIN )
	{
		return false;
	}

	return ps->weaponTime <= 0 || ps->weaponstate != WEAPON_FIRING;
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
		return (weapon_t) ps->persistant[ PERS_NEWWEAPON ];
	}

	return (weapon_t) ps->weapon;
}

/*
=================
BG_PlayerLowAmmo

Checks if a player is running low on ammo
Also returns whether the gun uses energy or not
=================
*/
bool BG_PlayerLowAmmo( const playerState_t *ps, bool *energy )
{
  int weapon;
	const weaponAttributes_t *wattr;

	// look for the primary weapon
	for( weapon = WP_NONE + 1; weapon < WP_NUM_WEAPONS; weapon++ )
		if( weapon != WP_BLASTER )
			if( BG_InventoryContainsWeapon( weapon, ps->stats ) )
				goto found;

	return false; // got only blaster

found:

	wattr = BG_Weapon( weapon );

	if( wattr->infiniteAmmo )
		return false;

	*energy = wattr->usesEnergy;

	if( wattr->maxClips )
		return ( ps->clips <= wattr->maxClips * 0.25 );

	return ( ps->ammo <= wattr->maxAmmo * 0.25 );
}

/*
===============
atof_neg

atof with an allowance for negative values
===============
*/
float atof_neg( char *token, bool allowNegative )
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
int atoi_neg( char *token, bool allowNegative )
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
void BG_PackEntityNumbers( entityState_t *es, const int *entityNums, unsigned int count )
{
	unsigned int i;

	if ( count > MAX_NUM_PACKED_ENTITY_NUMS )
	{
		count = MAX_NUM_PACKED_ENTITY_NUMS;
		Com_Printf( S_WARNING "A maximum of %d entity numbers can be "
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
int BG_UnpackEntityNumbers( entityState_t *es, int *entityNums, unsigned int count )
{
	unsigned int i;

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
	bool EOS = false;

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
			EOS = true;
		}

		*p = '\0';

		//strip leading whitespace
		while ( *q == ' ' )
		{
			q++;
		}

		if ( weaponsSize )
		{
			weapons[ i ] = BG_WeaponNumberByName( q );
		}

		if ( upgradesSize )
		{
			upgrades[ j ] = BG_UpgradeByName( q )->number;
		}

		if ( weaponsSize && weapons[ i ] == WP_NONE &&
		     upgradesSize && upgrades[ j ] == UP_NONE )
		{
			Com_Printf( S_WARNING "unknown equipment %s\n", q );
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
	bool EOS = false;

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
			EOS = true;
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
			Com_Printf( S_WARNING "unknown class %s\n", q );
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
	bool EOS = false;

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
			EOS = true;
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
			Com_Printf( S_WARNING "unknown buildable %s\n", q );
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
void BG_InitAllowedGameElements()
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
bool BG_WeaponDisabled( int weapon )
{
	int i;

	for ( i = 0; i < WP_NUM_WEAPONS &&
	      bg_disabledGameElements.weapons[ i ] != WP_NONE; i++ )
	{
		if ( bg_disabledGameElements.weapons[ i ] == weapon )
		{
			return true;
		}
	}

	return false;
}

/*
============
BG_UpgradeIsAllowed
============
*/
bool BG_UpgradeDisabled( int upgrade )
{
	int i;

	for ( i = 0; i < UP_NUM_UPGRADES &&
	      bg_disabledGameElements.upgrades[ i ] != UP_NONE; i++ )
	{
		if ( bg_disabledGameElements.upgrades[ i ] == upgrade )
		{
			return true;
		}
	}

	return false;
}

/*
============
BG_ClassDisabled
============
*/
bool BG_ClassDisabled( int class_ )
{
	int i;

	for ( i = 0; i < PCL_NUM_CLASSES &&
	      bg_disabledGameElements.classes[ i ] != PCL_NONE; i++ )
	{
		if ( bg_disabledGameElements.classes[ i ] == class_ )
		{
			return true;
		}
	}

	return false;
}

/*
============
BG_BuildableIsAllowed
============
*/
bool BG_BuildableDisabled( int buildable )
{
	int i;

	for ( i = 0; i < BA_NUM_BUILDABLES &&
	      bg_disabledGameElements.buildables[ i ] != BA_NONE; i++ )
	{
		if ( bg_disabledGameElements.buildables[ i ] == buildable )
		{
			return true;
		}
	}

	return false;
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
			return (weapon_t) i;
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

	numFiles = trap_FS_GetFileList( "emoticons", "x1.crn", fileList,
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

		if ( fileLen - 8 >= MAX_EMOTICON_NAME_LEN )
		{
			Com_Printf( S_COLOR_YELLOW "emoticon file name \"%s\" too long (≥ %d)\n",
			            filePtr, MAX_EMOTICON_NAME_LEN + 8 );
			continue;
		}

		if ( !trap_FS_FOpenFile( va( "emoticons/%s", filePtr ), nullptr, FS_READ ) )
		{
			Com_Printf( S_COLOR_YELLOW "could not open \"emoticons/%s\"\n", filePtr );
			continue;
		}

		Q_strncpyz( emoticons[ count ].name, filePtr, fileLen - 8 + 1 );
#ifndef BUILD_SGAME
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
const char *BG_TeamName( int team )
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

const char *BG_TeamNamePlural( int team )
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

	if ( copy == nullptr )
	{
		return nullptr;
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
