/*
===========================================================================
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2006 Tim Angus

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

#include "../../../../engine/qcommon/q_shared.h"
#include "bg_public.h"

int                           trap_FS_FOpenFile( const char *qpath, fileHandle_t *f, fsMode_t mode );
void                          trap_FS_Read( void *buffer, int len, fileHandle_t f );
void                          trap_FS_Write( const void *buffer, int len, fileHandle_t f );
void                          trap_FS_FCloseFile( fileHandle_t f );
void                          trap_FS_Seek( fileHandle_t f, long offset, fsOrigin_t origin ); // fsOrigin_t

buildableAttributes_t         bg_buildableList[] =
{
	{
		BA_A_SPAWN,                                                 //int       buildNum;
		"eggpod",                                                   //char      *buildName;
		"Egg",                                                      //char      *humanName;
		"team_alien_spawn",                                         //char      *entityName;
		{ "models/buildables/eggpod/eggpod.md3",        0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -15,                                          -15, -15 }, //vec3_t    mins;
		{ 15,                                           15,  15 },  //vec3_t    maxs;
		0.0f,                                                       //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		ASPAWN_BP,                                                  //int       buildPoints;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		ASPAWN_HEALTH,                                              //int       health;
		ASPAWN_REGEN,                                               //int       regenRate;
		ASPAWN_SPLASHDAMAGE,                                        //int       splashDamage;
		ASPAWN_SPLASHRADIUS,                                        //int       splashRadius;
		MOD_ASPAWN,                                                 //int       meansOfDeath;
		BIT_ALIENS,                                                 //int       team;
		( 1 << WP_ABUILD ) | ( 1 << WP_ABUILD2 ),                   //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		100,                                                        //int       nextthink;
		ASPAWN_BT,                                                  //int       buildTime;
		qfalse,                                                     //qboolean  usable;
		0,                                                          //int       turretRange;
		0,                                                          //int       turretFireSpeed;
		WP_NONE,                                                    //weapon_t  turretProjType;
		0.5f,                                                       //float     minNormal;
		qtrue,                                                      //qboolean  invertNormal;
		qfalse,                                                     //qboolean  creepTest;
		ASPAWN_CREEPSIZE,                                           //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qfalse                                                      //qboolean  reactorTest;
	},
	{
		BA_A_BARRICADE,                                             //int       buildNum;
		"barricade",                                                //char      *buildName;
		"Barricade",                                                //char      *humanName;
		"team_alien_barricade",                                     //char      *entityName;
		{ "models/buildables/barricade/barricade.md3",  0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -35,                                          -35, -15 }, //vec3_t    mins;
		{ 35,                                           35,  60 },  //vec3_t    maxs;
		0.0f,                                                       //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		BARRICADE_BP,                                               //int       buildPoints;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		BARRICADE_HEALTH,                                           //int       health;
		BARRICADE_REGEN,                                            //int       regenRate;
		BARRICADE_SPLASHDAMAGE,                                     //int       splashDamage;
		BARRICADE_SPLASHRADIUS,                                     //int       splashRadius;
		MOD_ASPAWN,                                                 //int       meansOfDeath;
		BIT_ALIENS,                                                 //int       team;
		( 1 << WP_ABUILD ) | ( 1 << WP_ABUILD2 ),                   //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		100,                                                        //int       nextthink;
		BARRICADE_BT,                                               //int       buildTime;
		qfalse,                                                     //qboolean  usable;
		0,                                                          //int       turretRange;
		0,                                                          //int       turretFireSpeed;
		WP_NONE,                                                    //weapon_t  turretProjType;
		0.707f,                                                     //float     minNormal;
		qfalse,                                                     //qboolean  invertNormal;
		qtrue,                                                      //qboolean  creepTest;
		BARRICADE_CREEPSIZE,                                        //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qfalse                                                      //qboolean  reactorTest;
	},
	{
		BA_A_BOOSTER,                                              //int       buildNum;
		"booster",                                                 //char      *buildName;
		"Booster",                                                 //char      *humanName;
		"team_alien_booster",                                      //char      *entityName;
		{ "models/buildables/booster/booster.md3",      0,   0, 0 },
		1.0f,                                                      //float     modelScale;
		{ -26,                                          -26, -9 }, //vec3_t     mins;
		{ 26,                                           26,  9 },  //vec3_t     maxs;
		0.0f,                                                      //float     zOffset;
		TR_GRAVITY,                                                //trType_t  traj;
		0.0,                                                       //float     bounce;
		BOOSTER_BP,                                                //int       buildPoints;
		( 1 << S2 ) | ( 1 << S3 ),                                 //int  stages
		BOOSTER_HEALTH,                                            //int       health;
		BOOSTER_REGEN,                                             //int       regenRate;
		BOOSTER_SPLASHDAMAGE,                                      //int       splashDamage;
		BOOSTER_SPLASHRADIUS,                                      //int       splashRadius;
		MOD_ASPAWN,                                                //int       meansOfDeath;
		BIT_ALIENS,                                                //int       team;
		( 1 << WP_ABUILD ) | ( 1 << WP_ABUILD2 ),                  //weapon_t  buildWeapon;
		BANIM_IDLE1,                                               //int       idleAnim;
		100,                                                       //int       nextthink;
		BOOSTER_BT,                                                //int       buildTime;
		qfalse,                                                    //qboolean  usable;
		0,                                                         //int       turretRange;
		0,                                                         //int       turretFireSpeed;
		WP_NONE,                                                   //weapon_t  turretProjType;
		0.707f,                                                    //float     minNormal;
		qfalse,                                                    //qboolean  invertNormal;
		qtrue,                                                     //qboolean  creepTest;
		BOOSTER_CREEPSIZE,                                         //int       creepSize;
		qfalse,                                                    //qboolean  dccTest;
		qfalse                                                     //qboolean  reactorTest;
	},
	{
		BA_A_ACIDTUBE,                                              //int       buildNum;
		"acid_tube",                                                //char      *buildName;
		"Acid Tube",                                                //char      *humanName;
		"team_alien_acid_tube",                                     //char      *entityName;
		{ "models/buildables/acid_tube/acid_tube.md3",  0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -25,                                          -25, -25 }, //vec3_t    mins;
		{ 25,                                           25,  25 },  //vec3_t    maxs;
		-15.0f,                                                     //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		ACIDTUBE_BP,                                                //int       buildPoints;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		ACIDTUBE_HEALTH,                                            //int       health;
		ACIDTUBE_REGEN,                                             //int       regenRate;
		ACIDTUBE_SPLASHDAMAGE,                                      //int       splashDamage;
		ACIDTUBE_SPLASHRADIUS,                                      //int       splashRadius;
		MOD_ATUBE,                                                  //int       meansOfDeath;
		BIT_ALIENS,                                                 //int       team;
		( 1 << WP_ABUILD ) | ( 1 << WP_ABUILD2 ),                   //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		200,                                                        //int       nextthink;
		ACIDTUBE_BT,                                                //int       buildTime;
		qfalse,                                                     //qboolean  usable;
		0,                                                          //int       turretRange;
		0,                                                          //int       turretFireSpeed;
		WP_NONE,                                                    //weapon_t  turretProjType;
		0.0f,                                                       //float     minNormal;
		qtrue,                                                      //qboolean  invertNormal;
		qtrue,                                                      //qboolean  creepTest;
		ACIDTUBE_CREEPSIZE,                                         //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qfalse                                                      //qboolean  reactorTest;
	},
	{
		BA_A_HIVE,                                                  //int       buildNum;
		"hive",                                                     //char      *buildName;
		"Hive",                                                     //char      *humanName;
		"team_alien_hive",                                          //char      *entityName;
		{ "models/buildables/acid_tube/acid_tube.md3",  0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -35,                                          -35, -25 }, //vec3_t    mins;
		{ 35,                                           35,  25 },  //vec3_t    maxs;
		-15.0f,                                                     //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		HIVE_BP,                                                    //int       buildPoints;
		( 1 << S3 ),                                                //int  stages
		HIVE_HEALTH,                                                //int       health;
		HIVE_REGEN,                                                 //int       regenRate;
		HIVE_SPLASHDAMAGE,                                          //int       splashDamage;
		HIVE_SPLASHRADIUS,                                          //int       splashRadius;
		MOD_ASPAWN,                                                 //int       meansOfDeath;
		BIT_ALIENS,                                                 //int       team;
		( 1 << WP_ABUILD ) | ( 1 << WP_ABUILD2 ),                   //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		500,                                                        //int       nextthink;
		HIVE_BT,                                                    //int       buildTime;
		qfalse,                                                     //qboolean  usable;
		0,                                                          //int       turretRange;
		0,                                                          //int       turretFireSpeed;
		WP_HIVE,                                                    //weapon_t  turretProjType;
		0.0f,                                                       //float     minNormal;
		qtrue,                                                      //qboolean  invertNormal;
		qtrue,                                                      //qboolean  creepTest;
		HIVE_CREEPSIZE,                                             //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qfalse                                                      //qboolean  reactorTest;
	},
	{
		BA_A_TRAPPER,                                               //int       buildNum;
		"trapper",                                                  //char      *buildName;
		"Trapper",                                                  //char      *humanName;
		"team_alien_trapper",                                       //char      *entityName;
		{ "models/buildables/trapper/trapper.md3",      0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -15,                                          -15, -15 }, //vec3_t    mins;
		{ 15,                                           15,  15 },  //vec3_t    maxs;
		0.0f,                                                       //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		TRAPPER_BP,                                                 //int       buildPoints;
		( 1 << S2 ) | ( 1 << S3 ),                                  //int  stages //NEEDS ADV BUILDER SO S2 AND UP
		TRAPPER_HEALTH,                                             //int       health;
		TRAPPER_REGEN,                                              //int       regenRate;
		TRAPPER_SPLASHDAMAGE,                                       //int       splashDamage;
		TRAPPER_SPLASHRADIUS,                                       //int       splashRadius;
		MOD_ASPAWN,                                                 //int       meansOfDeath;
		BIT_ALIENS,                                                 //int       team;
		( 1 << WP_ABUILD ) | ( 1 << WP_ABUILD2 ),                   //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		100,                                                        //int       nextthink;
		TRAPPER_BT,                                                 //int       buildTime;
		qfalse,                                                     //qboolean  usable;
		TRAPPER_RANGE,                                              //int       turretRange;
		TRAPPER_REPEAT,                                             //int       turretFireSpeed;
		WP_LOCKBLOB_LAUNCHER,                                       //weapon_t  turretProjType;
		0.0f,                                                       //float     minNormal;
		qtrue,                                                      //qboolean  invertNormal;
		qtrue,                                                      //qboolean  creepTest;
		TRAPPER_CREEPSIZE,                                          //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qfalse                                                      //qboolean  reactorTest;
	},
	{
		BA_A_OVERMIND,                                              //int       buildNum;
		"overmind",                                                 //char      *buildName;
		"Overmind",                                                 //char      *humanName;
		"team_alien_overmind",                                      //char      *entityName;
		{ "models/buildables/overmind/overmind.md3",    0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -45,                                          -45, -15 }, //vec3_t    mins;
		{ 45,                                           45,  95 },  //vec3_t    maxs;
		0.0f,                                                       //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		OVERMIND_BP,                                                //int       buildPoints;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		OVERMIND_HEALTH,                                            //int       health;
		OVERMIND_REGEN,                                             //int       regenRate;
		OVERMIND_SPLASHDAMAGE,                                      //int       splashDamage;
		OVERMIND_SPLASHRADIUS,                                      //int       splashRadius;
		MOD_ASPAWN,                                                 //int       meansOfDeath;
		BIT_ALIENS,                                                 //int       team;
		( 1 << WP_ABUILD ) | ( 1 << WP_ABUILD2 ),                   //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		OVERMIND_ATTACK_REPEAT,                                     //int       nextthink;
		OVERMIND_BT,                                                //int       buildTime;
		qfalse,                                                     //qboolean  usable;
		0,                                                          //int       turretRange;
		0,                                                          //int       turretFireSpeed;
		WP_NONE,                                                    //weapon_t  turretProjType;
		0.95f,                                                      //float     minNormal;
		qfalse,                                                     //qboolean  invertNormal;
		qfalse,                                                     //qboolean  creepTest;
		OVERMIND_CREEPSIZE,                                         //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qtrue                                                       //qboolean  reactorTest;
	},
	{
		BA_A_HOVEL,                                                 //int       buildNum;
		"hovel",                                                    //char      *buildName;
		"Hovel",                                                    //char      *humanName;
		"team_alien_hovel",                                         //char      *entityName;
		{ "models/buildables/hovel/hovel.md3",          0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -50,                                          -50, -20 }, //vec3_t    mins;
		{ 50,                                           50,  20 },  //vec3_t    maxs;
		0.0f,                                                       //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		HOVEL_BP,                                                   //int       buildPoints;
		( 1 << S3 ),                                                //int  stages
		HOVEL_HEALTH,                                               //int       health;
		HOVEL_REGEN,                                                //int       regenRate;
		HOVEL_SPLASHDAMAGE,                                         //int       splashDamage;
		HOVEL_SPLASHRADIUS,                                         //int       splashRadius;
		MOD_ASPAWN,                                                 //int       meansOfDeath;
		BIT_ALIENS,                                                 //int       team;
		( 1 << WP_ABUILD ) | ( 1 << WP_ABUILD2 ),                   //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		150,                                                        //int       nextthink;
		HOVEL_BT,                                                   //int       buildTime;
		qtrue,                                                      //qboolean  usable;
		0,                                                          //int       turretRange;
		0,                                                          //int       turretFireSpeed;
		WP_NONE,                                                    //weapon_t  turretProjType;
		0.95f,                                                      //float     minNormal;
		qfalse,                                                     //qboolean  invertNormal;
		qtrue,                                                      //qboolean  creepTest;
		HOVEL_CREEPSIZE,                                            //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qtrue                                                       //qboolean  reactorTest;
	},
	{
		BA_H_SPAWN,                                                //int       buildNum;
		"telenode",                                                //char      *buildName;
		"Telenode",                                                //char      *humanName;
		"team_human_spawn",                                        //char      *entityName;
		{ "models/buildables/telenode/telenode.md3",    0,   0, 0 },
		1.0f,                                                      //float     modelScale;
		{ -40,                                          -40, -4 }, //vec3_t    mins;
		{ 40,                                           40,  4 },  //vec3_t    maxs;
		0.0f,                                                      //float     zOffset;
		TR_GRAVITY,                                                //trType_t  traj;
		0.0,                                                       //float     bounce;
		HSPAWN_BP,                                                 //int       buildPoints;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                   //int  stages
		HSPAWN_HEALTH,                                             //int       health;
		0,                                                         //int       regenRate;
		HSPAWN_SPLASHDAMAGE,                                       //int       splashDamage;
		HSPAWN_SPLASHRADIUS,                                       //int       splashRadius;
		MOD_HSPAWN,                                                //int       meansOfDeath;
		BIT_HUMANS,                                                //int       team;
		( 1 << WP_HBUILD ) | ( 1 << WP_HBUILD2 ),                  //weapon_t  buildWeapon;
		BANIM_IDLE1,                                               //int       idleAnim;
		100,                                                       //int       nextthink;
		HSPAWN_BT,                                                 //int       buildTime;
		qfalse,                                                    //qboolean  usable;
		0,                                                         //int       turretRange;
		0,                                                         //int       turretFireSpeed;
		WP_NONE,                                                   //weapon_t  turretProjType;
		0.95f,                                                     //float     minNormal;
		qfalse,                                                    //qboolean  invertNormal;
		qfalse,                                                    //qboolean  creepTest;
		0,                                                         //int       creepSize;
		qfalse,                                                    //qboolean  dccTest;
		qfalse                                                     //qboolean  reactorTest;
	},
	{
		BA_H_MEDISTAT,                                             //int       buildNum;
		"medistat",                                                //char      *buildName;
		"Medistation",                                             //char      *humanName;
		"team_human_medistat",                                     //char      *entityName;
		{ "models/buildables/medistat/medistat.md3",    0,   0, 0 },
		1.0f,                                                      //float     modelScale;
		{ -35,                                          -35, -7 }, //vec3_t    mins;
		{ 35,                                           35,  7 },  //vec3_t    maxs;
		0.0f,                                                      //float     zOffset;
		TR_GRAVITY,                                                //trType_t  traj;
		0.0,                                                       //float     bounce;
		MEDISTAT_BP,                                               //int       buildPoints;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                   //int  stages
		MEDISTAT_HEALTH,                                           //int       health;
		0,                                                         //int       regenRate;
		MEDISTAT_SPLASHDAMAGE,                                     //int       splashDamage;
		MEDISTAT_SPLASHRADIUS,                                     //int       splashRadius;
		MOD_HSPAWN,                                                //int       meansOfDeath;
		BIT_HUMANS,                                                //int       team;
		( 1 << WP_HBUILD ) | ( 1 << WP_HBUILD2 ),                  //weapon_t  buildWeapon;
		BANIM_IDLE1,                                               //int       idleAnim;
		100,                                                       //int       nextthink;
		MEDISTAT_BT,                                               //int       buildTime;
		qfalse,                                                    //qboolean  usable;
		0,                                                         //int       turretRange;
		0,                                                         //int       turretFireSpeed;
		WP_NONE,                                                   //weapon_t  turretProjType;
		0.95f,                                                     //float     minNormal;
		qfalse,                                                    //qboolean  invertNormal;
		qfalse,                                                    //qboolean  creepTest;
		0,                                                         //int       creepSize;
		qfalse,                                                    //qboolean  dccTest;
		qfalse                                                     //qboolean  reactorTest;
	},
	{
		BA_H_MGTURRET,         //int       buildNum;
		"mgturret",            //char      *buildName;
		"Machinegun Turret",   //char      *humanName;
		"team_human_mgturret", //char      *entityName;
		{
			"models/buildables/mgturret/turret_base.md3",
			"models/buildables/mgturret/turret_barrel.md3",
			"models/buildables/mgturret/turret_top.md3", 0
		},
		1.0f,                                                       //float     modelScale;
		{ -25,                                          -25, -20 }, //vec3_t    mins;
		{ 25,                                           25,  20 },  //vec3_t    maxs;
		0.0f,                                                       //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		MGTURRET_BP,                                                //int       buildPoints;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		MGTURRET_HEALTH,                                            //int       health;
		0,                                                          //int       regenRate;
		MGTURRET_SPLASHDAMAGE,                                      //int       splashDamage;
		MGTURRET_SPLASHRADIUS,                                      //int       splashRadius;
		MOD_HSPAWN,                                                 //int       meansOfDeath;
		BIT_HUMANS,                                                 //int       team;
		( 1 << WP_HBUILD ) | ( 1 << WP_HBUILD2 ),                   //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		50,                                                         //int       nextthink;
		MGTURRET_BT,                                                //int       buildTime;
		qfalse,                                                     //qboolean  usable;
		MGTURRET_RANGE,                                             //int       turretRange;
		MGTURRET_REPEAT,                                            //int       turretFireSpeed;
		WP_MGTURRET,                                                //weapon_t  turretProjType;
		0.95f,                                                      //float     minNormal;
		qfalse,                                                     //qboolean  invertNormal;
		qfalse,                                                     //qboolean  creepTest;
		0,                                                          //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qfalse                                                      //qboolean  reactorTest;
	},
	{
		BA_H_TESLAGEN,                                              //int       buildNum;
		"tesla",                                                    //char      *buildName;
		"Tesla Generator",                                          //char      *humanName;
		"team_human_tesla",                                         //char      *entityName;
		{ "models/buildables/tesla/tesla.md3",          0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -22,                                          -22, -40 }, //vec3_t    mins;
		{ 22,                                           22,  40 },  //vec3_t    maxs;
		0.0f,                                                       //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		TESLAGEN_BP,                                                //int       buildPoints;
		( 1 << S3 ),                                                //int       stages
		TESLAGEN_HEALTH,                                            //int       health;
		0,                                                          //int       regenRate;
		TESLAGEN_SPLASHDAMAGE,                                      //int       splashDamage;
		TESLAGEN_SPLASHRADIUS,                                      //int       splashRadius;
		MOD_HSPAWN,                                                 //int       meansOfDeath;
		BIT_HUMANS,                                                 //int       team;
		( 1 << WP_HBUILD2 ),                                        //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		150,                                                        //int       nextthink;
		TESLAGEN_BT,                                                //int       buildTime;
		qfalse,                                                     //qboolean  usable;
		TESLAGEN_RANGE,                                             //int       turretRange;
		TESLAGEN_REPEAT,                                            //int       turretFireSpeed;
		WP_TESLAGEN,                                                //weapon_t  turretProjType;
		0.95f,                                                      //float     minNormal;
		qfalse,                                                     //qboolean  invertNormal;
		qfalse,                                                     //qboolean  creepTest;
		0,                                                          //int       creepSize;
		qtrue,                                                      //qboolean  dccTest;
		qfalse                                                      //qboolean  reactorTest;
	},
	{
		BA_H_DCC,                                                   //int       buildNum;
		"dcc",                                                      //char      *buildName;
		"Defence Computer",                                         //char      *humanName;
		"team_human_dcc",                                           //char      *entityName;
		{ "models/buildables/dcc/dcc.md3",              0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -35,                                          -35, -13 }, //vec3_t    mins;
		{ 35,                                           35,  47 },  //vec3_t    maxs;
		0.0f,                                                       //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		DC_BP,                                                      //int       buildPoints;
		( 1 << S2 ) | ( 1 << S3 ),                                  //int       stages
		DC_HEALTH,                                                  //int       health;
		0,                                                          //int       regenRate;
		DC_SPLASHDAMAGE,                                            //int       splashDamage;
		DC_SPLASHRADIUS,                                            //int       splashRadius;
		MOD_HSPAWN,                                                 //int       meansOfDeath;
		BIT_HUMANS,                                                 //int       team;
		( 1 << WP_HBUILD2 ),                                        //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		100,                                                        //int       nextthink;
		DC_BT,                                                      //int       buildTime;
		qfalse,                                                     //qboolean  usable;
		0,                                                          //int       turretRange;
		0,                                                          //int       turretFireSpeed;
		WP_NONE,                                                    //weapon_t  turretProjType;
		0.95f,                                                      //float     minNormal;
		qfalse,                                                     //qboolean  invertNormal;
		qfalse,                                                     //qboolean  creepTest;
		0,                                                          //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qfalse                                                      //qboolean  reactorTest;
	},
	{
		BA_H_ARMOURY,                                               //int       buildNum;
		"arm",                                                      //char      *buildName;
		"Armoury",                                                  //char      *humanName;
		"team_human_armoury",                                       //char      *entityName;
		{ "models/buildables/arm/arm.md3",              0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -40,                                          -40, -13 }, //vec3_t    mins;
		{ 40,                                           40,  50 },  //vec3_t    maxs;
		0.0f,                                                       //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		ARMOURY_BP,                                                 //int       buildPoints;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		ARMOURY_HEALTH,                                             //int       health;
		0,                                                          //int       regenRate;
		ARMOURY_SPLASHDAMAGE,                                       //int       splashDamage;
		ARMOURY_SPLASHRADIUS,                                       //int       splashRadius;
		MOD_HSPAWN,                                                 //int       meansOfDeath;
		BIT_HUMANS,                                                 //int       team;
		( 1 << WP_HBUILD ) | ( 1 << WP_HBUILD2 ),                   //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		100,                                                        //int       nextthink;
		ARMOURY_BT,                                                 //int       buildTime;
		qtrue,                                                      //qboolean  usable;
		0,                                                          //int       turretRange;
		0,                                                          //int       turretFireSpeed;
		WP_NONE,                                                    //weapon_t  turretProjType;
		0.95f,                                                      //float     minNormal;
		qfalse,                                                     //qboolean  invertNormal;
		qfalse,                                                     //qboolean  creepTest;
		0,                                                          //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qfalse                                                      //qboolean  reactorTest;
	},
	{
		BA_H_REACTOR,                                               //int       buildNum;
		"reactor",                                                  //char      *buildName;
		"Reactor",                                                  //char      *humanName;
		"team_human_reactor",                                       //char      *entityName;
		{ "models/buildables/reactor/reactor.md3",      0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -50,                                          -50, -15 }, //vec3_t    mins;
		{ 50,                                           50,  95 },  //vec3_t    maxs;
		0.0f,                                                       //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		REACTOR_BP,                                                 //int       buildPoints;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		REACTOR_HEALTH,                                             //int       health;
		0,                                                          //int       regenRate;
		REACTOR_SPLASHDAMAGE,                                       //int       splashDamage;
		REACTOR_SPLASHRADIUS,                                       //int       splashRadius;
		MOD_HSPAWN,                                                 //int       meansOfDeath;
		BIT_HUMANS,                                                 //int       team;
		( 1 << WP_HBUILD ) | ( 1 << WP_HBUILD2 ),                   //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		REACTOR_ATTACK_REPEAT,                                      //int       nextthink;
		REACTOR_BT,                                                 //int       buildTime;
		qtrue,                                                      //qboolean  usable;
		0,                                                          //int       turretRange;
		0,                                                          //int       turretFireSpeed;
		WP_NONE,                                                    //weapon_t  turretProjType;
		0.95f,                                                      //float     minNormal;
		qfalse,                                                     //qboolean  invertNormal;
		qfalse,                                                     //qboolean  creepTest;
		0,                                                          //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qtrue                                                       //qboolean  reactorTest;
	},
	{
		BA_H_REPEATER,                                              //int       buildNum;
		"repeater",                                                 //char      *buildName;
		"Repeater",                                                 //char      *humanName;
		"team_human_repeater",                                      //char      *entityName;
		{ "models/buildables/repeater/repeater.md3",    0,   0, 0 },
		1.0f,                                                       //float     modelScale;
		{ -15,                                          -15, -15 }, //vec3_t    mins;
		{ 15,                                           15,  25 },  //vec3_t    maxs;
		0.0f,                                                       //float     zOffset;
		TR_GRAVITY,                                                 //trType_t  traj;
		0.0,                                                        //float     bounce;
		REPEATER_BP,                                                //int       buildPoints;
		( 1 << S2 ) | ( 1 << S3 ),                                  //int  stages
		REPEATER_HEALTH,                                            //int       health;
		0,                                                          //int       regenRate;
		REPEATER_SPLASHDAMAGE,                                      //int       splashDamage;
		REPEATER_SPLASHRADIUS,                                      //int       splashRadius;
		MOD_HSPAWN,                                                 //int       meansOfDeath;
		BIT_HUMANS,                                                 //int       team;
		( 1 << WP_HBUILD ) | ( 1 << WP_HBUILD2 ),                   //weapon_t  buildWeapon;
		BANIM_IDLE1,                                                //int       idleAnim;
		100,                                                        //int       nextthink;
		REPEATER_BT,                                                //int       buildTime;
		qtrue,                                                      //qboolean  usable;
		0,                                                          //int       turretRange;
		0,                                                          //int       turretFireSpeed;
		WP_NONE,                                                    //weapon_t  turretProjType;
		0.95f,                                                      //float     minNormal;
		qfalse,                                                     //qboolean  invertNormal;
		qfalse,                                                     //qboolean  creepTest;
		0,                                                          //int       creepSize;
		qfalse,                                                     //qboolean  dccTest;
		qfalse                                                      //qboolean  reactorTest;
	}
};

int                           bg_numBuildables = sizeof( bg_buildableList ) / sizeof( bg_buildableList[ 0 ] );

//separate from bg_buildableList to work around char struct init bug
buildableAttributeOverrides_t bg_buildableOverrideList[ BA_NUM_BUILDABLES ];

/*
==============
BG_FindBuildNumForName
==============
*/
int BG_FindBuildNumForName( char *name )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( !Q_stricmp( bg_buildableList[ i ].buildName, name ) )
		{
			return bg_buildableList[ i ].buildNum;
		}
	}

	//wimp out
	return BA_NONE;
}

/*
==============
BG_FindBuildNumForEntityName
==============
*/
int BG_FindBuildNumForEntityName( char *name )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( !Q_stricmp( bg_buildableList[ i ].entityName, name ) )
		{
			return bg_buildableList[ i ].buildNum;
		}
	}

	//wimp out
	return BA_NONE;
}

/*
==============
BG_FindNameForBuildNum
==============
*/
char *BG_FindNameForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].buildName;
		}
	}

	//wimp out
	return 0;
}

/*
==============
BG_FindHumanNameForBuildNum
==============
*/
char *BG_FindHumanNameForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].humanName;
		}
	}

	//wimp out
	return 0;
}

/*
==============
BG_FindEntityNameForBuildNum
==============
*/
char *BG_FindEntityNameForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].entityName;
		}
	}

	//wimp out
	return 0;
}

/*
==============
BG_FindModelsForBuildNum
==============
*/
char *BG_FindModelsForBuildable( int bclass, int modelNum )
{
	int i;

	if ( bg_buildableOverrideList[ bclass ].models[ modelNum ][ 0 ] != 0 )
	{
		return bg_buildableOverrideList[ bclass ].models[ modelNum ];
	}

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].models[ modelNum ];
		}
	}

	//wimp out
	return 0;
}

/*
==============
BG_FindModelScaleForBuildable
==============
*/
float BG_FindModelScaleForBuildable( int bclass )
{
	int i;

	if ( bg_buildableOverrideList[ bclass ].modelScale != 0.0f )
	{
		return bg_buildableOverrideList[ bclass ].modelScale;
	}

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].modelScale;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindModelScaleForBuildable( %d )\n", bclass );
	return 1.0f;
}

/*
==============
BG_FindBBoxForBuildable
==============
*/
void BG_FindBBoxForBuildable( int bclass, vec3_t mins, vec3_t maxs )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			if ( mins != NULL )
			{
				VectorCopy( bg_buildableList[ i ].mins, mins );

				if ( VectorLength( bg_buildableOverrideList[ bclass ].mins ) )
				{
					VectorCopy( bg_buildableOverrideList[ bclass ].mins, mins );
				}
			}

			if ( maxs != NULL )
			{
				VectorCopy( bg_buildableList[ i ].maxs, maxs );

				if ( VectorLength( bg_buildableOverrideList[ bclass ].maxs ) )
				{
					VectorCopy( bg_buildableOverrideList[ bclass ].maxs, maxs );
				}
			}

			return;
		}
	}

	if ( mins != NULL )
	{
		VectorCopy( bg_buildableList[ 0 ].mins, mins );
	}

	if ( maxs != NULL )
	{
		VectorCopy( bg_buildableList[ 0 ].maxs, maxs );
	}
}

/*
==============
BG_FindZOffsetForBuildable
==============
*/
float BG_FindZOffsetForBuildable( int bclass )
{
	int i;

	if ( bg_buildableOverrideList[ bclass ].zOffset != 0.0f )
	{
		return bg_buildableOverrideList[ bclass ].zOffset;
	}

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].zOffset;
		}
	}

	return 0.0f;
}

/*
==============
BG_FindTrajectoryForBuildable
==============
*/
trType_t BG_FindTrajectoryForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].traj;
		}
	}

	return TR_GRAVITY;
}

/*
==============
BG_FindBounceForBuildable
==============
*/
float BG_FindBounceForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].bounce;
		}
	}

	return 0.0;
}

/*
==============
BG_FindBuildPointsForBuildable
==============
*/
int BG_FindBuildPointsForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].buildPoints;
		}
	}

	return 1000;
}

/*
==============
BG_FindStagesForBuildable
==============
*/
qboolean BG_FindStagesForBuildable( int bclass, stage_t stage )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			if ( bg_buildableList[ i ].stages & ( 1 << stage ) )
			{
				return qtrue;
			}
			else
			{
				return qfalse;
			}
		}
	}

	return qfalse;
}

/*
==============
BG_FindHealthForBuildable
==============
*/
int BG_FindHealthForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].health;
		}
	}

	return 1000;
}

/*
==============
BG_FindRegenRateForBuildable
==============
*/
int BG_FindRegenRateForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].regenRate;
		}
	}

	return 0;
}

/*
==============
BG_FindSplashDamageForBuildable
==============
*/
int BG_FindSplashDamageForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].splashDamage;
		}
	}

	return 50;
}

/*
==============
BG_FindSplashRadiusForBuildable
==============
*/
int BG_FindSplashRadiusForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].splashRadius;
		}
	}

	return 200;
}

/*
==============
BG_FindMODForBuildable
==============
*/
int BG_FindMODForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].meansOfDeath;
		}
	}

	return MOD_UNKNOWN;
}

/*
==============
BG_FindTeamForBuildable
==============
*/
int BG_FindTeamForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].team;
		}
	}

	return BIT_NONE;
}

/*
==============
BG_FindBuildWeaponForBuildable
==============
*/
weapon_t BG_FindBuildWeaponForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].buildWeapon;
		}
	}

	return WP_NONE;
}

/*
==============
BG_FindAnimForBuildable
==============
*/
int BG_FindAnimForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].idleAnim;
		}
	}

	return BANIM_IDLE1;
}

/*
==============
BG_FindNextThinkForBuildable
==============
*/
int BG_FindNextThinkForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].nextthink;
		}
	}

	return 100;
}

/*
==============
BG_FindBuildTimeForBuildable
==============
*/
int BG_FindBuildTimeForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].buildTime;
		}
	}

	return 10000;
}

/*
==============
BG_FindUsableForBuildable
==============
*/
qboolean BG_FindUsableForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].usable;
		}
	}

	return qfalse;
}

/*
==============
BG_FindFireSpeedForBuildable
==============
*/
int BG_FindFireSpeedForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].turretFireSpeed;
		}
	}

	return 1000;
}

/*
==============
BG_FindRangeForBuildable
==============
*/
int BG_FindRangeForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].turretRange;
		}
	}

	return 1000;
}

/*
==============
BG_FindProjTypeForBuildable
==============
*/
weapon_t BG_FindProjTypeForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].turretProjType;
		}
	}

	return WP_NONE;
}

/*
==============
BG_FindMinNormalForBuildable
==============
*/
float BG_FindMinNormalForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].minNormal;
		}
	}

	return 0.707f;
}

/*
==============
BG_FindInvertNormalForBuildable
==============
*/
qboolean BG_FindInvertNormalForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].invertNormal;
		}
	}

	return qfalse;
}

/*
==============
BG_FindCreepTestForBuildable
==============
*/
int BG_FindCreepTestForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].creepTest;
		}
	}

	return qfalse;
}

/*
==============
BG_FindCreepSizeForBuildable
==============
*/
int BG_FindCreepSizeForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].creepSize;
		}
	}

	return CREEP_BASESIZE;
}

/*
==============
BG_FindDCCTestForBuildable
==============
*/
int BG_FindDCCTestForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].dccTest;
		}
	}

	return qfalse;
}

/*
==============
BG_FindUniqueTestForBuildable
==============
*/
int BG_FindUniqueTestForBuildable( int bclass )
{
	int i;

	for ( i = 0; i < bg_numBuildables; i++ )
	{
		if ( bg_buildableList[ i ].buildNum == bclass )
		{
			return bg_buildableList[ i ].reactorTest;
		}
	}

	return qfalse;
}

/*
==============
BG_FindOverrideForBuildable
==============
*/
static buildableAttributeOverrides_t *BG_FindOverrideForBuildable( int bclass )
{
	return &bg_buildableOverrideList[ bclass ];
}

/*
======================
BG_ParseBuildableFile

Parses a configuration file describing a builable
======================
*/
static qboolean BG_ParseBuildableFile( const char *filename, buildableAttributeOverrides_t *bao )
{
	char         *text_p;
	int          i;
	int          len;
	char         *token;
	char         text[ 20000 ];
	fileHandle_t f;
	float        scale;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len <= 0 )
	{
		return qfalse;
	}

	if ( len >= sizeof( text ) - 1 )
	{
		Com_Printf( S_COLOR_RED "ERROR: Buildable file %s too long\n", filename );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse( &text_p );

		if ( !token )
		{
			break;
		}

		if ( !Q_stricmp( token, "" ) )
		{
			break;
		}

		if ( !Q_stricmp( token, "model" ) )
		{
			int index = 0;

			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			index = atoi( token );

			if ( index < 0 )
			{
				index = 0;
			}
			else if ( index > 3 )
			{
				index = 3;
			}

			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			Q_strncpyz( bao->models[ index ], token, sizeof( bao->models[ 0 ] ) );

			continue;
		}
		else if ( !Q_stricmp( token, "modelScale" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			scale = atof( token );

			if ( scale < 0.0f )
			{
				scale = 0.0f;
			}

			bao->modelScale = scale;

			continue;
		}
		else if ( !Q_stricmp( token, "mins" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( &text_p );

				if ( !token )
				{
					break;
				}

				bao->mins[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "maxs" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( &text_p );

				if ( !token )
				{
					break;
				}

				bao->maxs[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "zOffset" ) )
		{
			float offset;

			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			offset       = atof( token );

			bao->zOffset = offset;

			continue;
		}

		Com_Printf( S_COLOR_RED "ERROR: unknown token '%s'\n", token );
		return qfalse;
	}

	return qtrue;
}

/*
===============
BG_InitBuildableOverrides

Set any overrides specfied by file
===============
*/
void BG_InitBuildableOverrides( void )
{
	int                           i;
	buildableAttributeOverrides_t *bao;

	for ( i = BA_NONE + 1; i < BA_NUM_BUILDABLES; i++ )
	{
		bao = BG_FindOverrideForBuildable( i );

		BG_ParseBuildableFile( va( "overrides/buildables/%s.cfg", BG_FindNameForBuildable( i ) ), bao );
	}
}

////////////////////////////////////////////////////////////////////////////////

classAttributes_t         bg_classList[] =
{
	{
		PCL_NONE,                                                   //int     classnum;
		"spectator",                                                //char    *className;
		"Spectator",                                                //char    *humanName;
		"",                                                         //char    *modelname;
		1.0f,                                                       //float   modelScale;
		"",                                                         //char    *skinname;
		1.0f,                                                       //float   shadowScale;
		"",                                                         //char    *hudname;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		{ -15,                    -15,                  -15      }, //vec3_t  mins;
		{ 15,                     15,                   15       }, //vec3_t  maxs;
		{ 15,                     15,                   15       }, //vec3_t  crouchmaxs;
		{ -15,                    -15,                  -15      }, //vec3_t  deadmins;
		{ 15,                     15,                   15       }, //vec3_t  deadmaxs;
		0.0f,                                                       //float   zOffset
		0, 0,                                                       //int     viewheight, crouchviewheight;
		0,                                                          //int     health;
		0.0f,                                                       //float   fallDamage;
		0,                                                          //int     regenRate;
		0,                                                          //int     abilities;
		WP_NONE,                                                    //weapon_t  startWeapon
		0.0f,                                                       //float   buildDist;
		90,                                                         //int     fov;
		0.000f,                                                     //float   bob;
		1.0f,                                                       //float   bobCycle;
		0,                                                          //int     steptime;
		600,                                                        //float   speed;
		10.0f,                                                      //float   acceleration;
		1.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		100.0f,                                                     //float   stopSpeed;
		270.0f,                                                     //float   jumpMagnitude;
		1.0f,                                                       //float   knockbackScale;
		{ PCL_NONE,               PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
		0,                                                          //int     cost;
		0                                                           //int     value;
	},
	{
		PCL_ALIEN_BUILDER0,                                         //int     classnum;
		"builder",                                                  //char    *className;
		"Builder",                                                  //char    *humanName;
		"builder",                                                  //char    *modelname;
		1.0f,                                                       //float   modelScale;
		"default",                                                  //char    *skinname;
		1.0f,                                                       //float   shadowScale;
		"alien_builder_hud",                                        //char    *hudname;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		{ -15,                    -15,                  -20      }, //vec3_t  mins;
		{ 15,                     15,                   20       }, //vec3_t  maxs;
		{ 15,                     15,                   20       }, //vec3_t  crouchmaxs;
		{ -15,                    -15,                  -4       }, //vec3_t  deadmins;
		{ 15,                     15,                   4        }, //vec3_t  deadmaxs;
		0.0f,                                                       //float   zOffset
		0, 0,                                                       //int     viewheight, crouchviewheight;
		ABUILDER_HEALTH,                                            //int     health;
		0.2f,                                                       //float   fallDamage;
		ABUILDER_REGEN,                                             //int     regenRate;
		SCA_TAKESFALLDAMAGE | SCA_FOVWARPS | SCA_ALIENSENSE,        //int     abilities;
		WP_ABUILD,                                                  //weapon_t  startWeapon
		95.0f,                                                      //float   buildDist;
		80,                                                         //int     fov;
		0.001f,                                                     //float   bob;
		2.0f,                                                       //float   bobCycle;
		150,                                                        //int     steptime;
		ABUILDER_SPEED,                                             //float   speed;
		10.0f,                                                      //float   acceleration;
		1.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		100.0f,                                                     //float   stopSpeed;
		195.0f,                                                     //float   jumpMagnitude;
		1.0f,                                                       //float   knockbackScale;
		{ PCL_ALIEN_BUILDER0_UPG, PCL_ALIEN_LEVEL0,     PCL_NONE }, //int     children[ 3 ];
		ABUILDER_COST,                                              //int     cost;
		ABUILDER_VALUE                                              //int     value;
	},
	{
		PCL_ALIEN_BUILDER0_UPG,                                     //int     classnum;
		"builderupg",                                               //char    *classname;
		"Advanced Builder",                                         //char    *humanname;
		"builder",                                                  //char    *modelname;
		1.0f,                                                       //float   modelScale;
		"advanced",                                                 //char    *skinname;
		1.0f,                                                       //float   shadowScale;
		"alien_builder_hud",                                        //char    *hudname;
		( 1 << S2 ) | ( 1 << S3 ),                                  //int  stages
		{ -20,                    -20,                  -20      }, //vec3_t  mins;
		{ 20,                     20,                   20       }, //vec3_t  maxs;
		{ 20,                     20,                   20       }, //vec3_t  crouchmaxs;
		{ -20,                    -20,                  -4       }, //vec3_t  deadmins;
		{ 20,                     20,                   4        }, //vec3_t  deadmaxs;
		0.0f,                                                       //float   zOffset
		0, 0,                                                       //int     viewheight, crouchviewheight;
		ABUILDER_UPG_HEALTH,                                        //int     health;
		0.0f,                                                       //float   fallDamage;
		ABUILDER_UPG_REGEN,                                         //int     regenRate;
		SCA_FOVWARPS | SCA_WALLCLIMBER | SCA_ALIENSENSE,            //int     abilities;
		WP_ABUILD2,                                                 //weapon_t  startWeapon
		105.0f,                                                     //float   buildDist;
		110,                                                        //int     fov;
		0.001f,                                                     //float   bob;
		2.0f,                                                       //float   bobCycle;
		100,                                                        //int     steptime;
		ABUILDER_UPG_SPEED,                                         //float   speed;
		10.0f,                                                      //float   acceleration;
		1.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		100.0f,                                                     //float   stopSpeed;
		270.0f,                                                     //float   jumpMagnitude;
		1.0f,                                                       //float   knockbackScale;
		{ PCL_ALIEN_LEVEL0,       PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
		ABUILDER_UPG_COST,                                          //int     cost;
		ABUILDER_UPG_VALUE                                          //int     value;
	},
	{
		PCL_ALIEN_LEVEL0,                                           //int     classnum;
		"level0",                                                   //char    *classname;
		"Soldier",                                                  //char    *humanname;
		"jumper",                                                   //char    *modelname;
		0.2f,                                                       //float   modelScale;
		"default",                                                  //char    *skinname;
		0.3f,                                                       //float   shadowScale;
		"alien_general_hud",                                        //char    *hudname;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		{ -15,                    -15,                  -15      }, //vec3_t  mins;
		{ 15,                     15,                   15       }, //vec3_t  maxs;
		{ 15,                     15,                   15       }, //vec3_t  crouchmaxs;
		{ -15,                    -15,                  -4       }, //vec3_t  deadmins;
		{ 15,                     15,                   4        }, //vec3_t  deadmaxs;
		-8.0f,                                                      //float   zOffset
		0, 0,                                                       //int     viewheight, crouchviewheight;
		LEVEL0_HEALTH,                                              //int     health;
		0.0f,                                                       //float   fallDamage;
		LEVEL0_REGEN,                                               //int     regenRate;
		SCA_WALLCLIMBER | SCA_NOWEAPONDRIFT |
		SCA_FOVWARPS | SCA_ALIENSENSE,                              //int     abilities;
		WP_ALEVEL0,                                                 //weapon_t  startWeapon
		0.0f,                                                       //float   buildDist;
		140,                                                        //int     fov;
		0.0f,                                                       //float   bob;
		2.5f,                                                       //float   bobCycle;
		25,                                                         //int     steptime;
		LEVEL0_SPEED,                                               //float   speed;
		10.0f,                                                      //float   acceleration;
		1.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		400.0f,                                                     //float   stopSpeed;
		250.0f,                                                     //float   jumpMagnitude;
		2.0f,                                                       //float   knockbackScale;
		{ PCL_ALIEN_LEVEL1,       PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
		LEVEL0_COST,                                                //int     cost;
		LEVEL0_VALUE                                                //int     value;
	},
	{
		PCL_ALIEN_LEVEL1,                                           //int     classnum;
		"level1",                                                   //char    *classname;
		"Hydra",                                                    //char    *humanname;
		"spitter",                                                  //char    *modelname;
		0.6f,                                                       //float   modelScale;
		"default",                                                  //char    *skinname;
		1.0f,                                                       //float   shadowScale;
		"alien_general_hud",                                        //char    *hudname;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		{ -18,                    -18,                  -18      }, //vec3_t  mins;
		{ 18,                     18,                   18       }, //vec3_t  maxs;
		{ 18,                     18,                   18       }, //vec3_t  crouchmaxs;
		{ -18,                    -18,                  -4       }, //vec3_t  deadmins;
		{ 18,                     18,                   4        }, //vec3_t  deadmaxs;
		0.0f,                                                       //float   zOffset
		0, 0,                                                       //int     viewheight, crouchviewheight;
		LEVEL1_HEALTH,                                              //int     health;
		0.0f,                                                       //float   fallDamage;
		LEVEL1_REGEN,                                               //int     regenRate;
		SCA_NOWEAPONDRIFT |
		SCA_FOVWARPS | SCA_WALLCLIMBER | SCA_ALIENSENSE,            //int     abilities;
		WP_ALEVEL1,                                                 //weapon_t  startWeapon
		0.0f,                                                       //float   buildDist;
		120,                                                        //int     fov;
		0.001f,                                                     //float   bob;
		1.8f,                                                       //float   bobCycle;
		60,                                                         //int     steptime;
		LEVEL1_SPEED,                                               //float   speed;
		10.0f,                                                      //float   acceleration;
		1.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		300.0f,                                                     //float   stopSpeed;
		270.0f,                                                     //float   jumpMagnitude;
		1.2f,                                                       //float   knockbackScale;
		{ PCL_ALIEN_LEVEL2,       PCL_ALIEN_LEVEL1_UPG, PCL_NONE }, //int     children[ 3 ];
		LEVEL1_COST,                                                //int     cost;
		LEVEL1_VALUE                                                //int     value;
	},
	{
		PCL_ALIEN_LEVEL1_UPG,                                       //int     classnum;
		"level1upg",                                                //char    *classname;
		"Hydra Upgrade",                                            //char    *humanname;
		"spitter",                                                  //char    *modelname;
		0.7f,                                                       //float   modelScale;
		"blue",                                                     //char    *skinname;
		1.0f,                                                       //float   shadowScale;
		"alien_general_hud",                                        //char    *hudname;
		( 1 << S2 ) | ( 1 << S3 ),                                  //int  stages
		{ -20,                    -20,                  -20      }, //vec3_t  mins;
		{ 20,                     20,                   20       }, //vec3_t  maxs;
		{ 20,                     20,                   20       }, //vec3_t  crouchmaxs;
		{ -20,                    -20,                  -4       }, //vec3_t  deadmins;
		{ 20,                     20,                   4        }, //vec3_t  deadmaxs;
		0.0f,                                                       //float   zOffset
		0, 0,                                                       //int     viewheight, crouchviewheight;
		LEVEL1_UPG_HEALTH,                                          //int     health;
		0.0f,                                                       //float   fallDamage;
		LEVEL1_UPG_REGEN,                                           //int     regenRate;
		SCA_NOWEAPONDRIFT | SCA_FOVWARPS |
		SCA_WALLCLIMBER | SCA_ALIENSENSE,                           //int     abilities;
		WP_ALEVEL1_UPG,                                             //weapon_t  startWeapon
		0.0f,                                                       //float   buildDist;
		120,                                                        //int     fov;
		0.001f,                                                     //float   bob;
		1.8f,                                                       //float   bobCycle;
		60,                                                         //int     steptime;
		LEVEL1_UPG_SPEED,                                           //float   speed;
		10.0f,                                                      //float   acceleration;
		1.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		300.0f,                                                     //float   stopSpeed;
		270.0f,                                                     //float   jumpMagnitude;
		1.1f,                                                       //float   knockbackScale;
		{ PCL_ALIEN_LEVEL2,       PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
		LEVEL1_UPG_COST,                                            //int     cost;
		LEVEL1_UPG_VALUE                                            //int     value;
	},
	{
		PCL_ALIEN_LEVEL2,                                           //int     classnum;
		"level2",                                                   //char    *classname;
		"Chimera",                                                  //char    *humanname;
		"tarantula",                                                //char    *modelname;
		0.75f,                                                      //float   modelScale;
		"default",                                                  //char    *skinname;
		1.0f,                                                       //float   shadowScale;
		"alien_general_hud",                                        //char    *hudname;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		{ -22,                    -22,                  -22      }, //vec3_t  mins;
		{ 22,                     22,                   22       }, //vec3_t  maxs;
		{ 22,                     22,                   22       }, //vec3_t  crouchmaxs;
		{ -22,                    -22,                  -4       }, //vec3_t  deadmins;
		{ 22,                     22,                   4        }, //vec3_t  deadmaxs;
		0.0f,                                                       //float   zOffset
		10, 10,                                                     //int     viewheight, crouchviewheight;
		LEVEL2_HEALTH,                                              //int     health;
		0.0f,                                                       //float   fallDamage;
		LEVEL2_REGEN,                                               //int     regenRate;
		SCA_NOWEAPONDRIFT | SCA_WALLJUMPER |
		SCA_FOVWARPS | SCA_ALIENSENSE,                              //int     abilities;
		WP_ALEVEL2,                                                 //weapon_t  startWeapon
		0.0f,                                                       //float   buildDist;
		90,                                                         //int     fov;
		0.001f,                                                     //float   bob;
		1.5f,                                                       //float   bobCycle;
		80,                                                         //int     steptime;
		LEVEL2_SPEED,                                               //float   speed;
		10.0f,                                                      //float   acceleration;
		2.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		100.0f,                                                     //float   stopSpeed;
		400.0f,                                                     //float   jumpMagnitude;
		0.8f,                                                       //float   knockbackScale;
		{ PCL_ALIEN_LEVEL3,       PCL_ALIEN_LEVEL2_UPG, PCL_NONE }, //int     children[ 3 ];
		LEVEL2_COST,                                                //int     cost;
		LEVEL2_VALUE                                                //int     value;
	},
	{
		PCL_ALIEN_LEVEL2_UPG,                                       //int     classnum;
		"level2upg",                                                //char    *classname;
		"Chimera Upgrade",                                          //char    *humanname;
		"tarantula",                                                //char    *modelname;
		0.9f,                                                       //float   modelScale;
		"red",                                                      //char    *skinname;
		1.0f,                                                       //float   shadowScale;
		"alien_general_hud",                                        //char    *hudname;
		( 1 << S2 ) | ( 1 << S3 ),                                  //int  stages
		{ -24,                    -24,                  -24      }, //vec3_t  mins;
		{ 24,                     24,                   24       }, //vec3_t  maxs;
		{ 24,                     24,                   24       }, //vec3_t  crouchmaxs;
		{ -24,                    -24,                  -4       }, //vec3_t  deadmins;
		{ 24,                     24,                   4        }, //vec3_t  deadmaxs;
		0.0f,                                                       //float   zOffset
		12, 12,                                                     //int     viewheight, crouchviewheight;
		LEVEL2_UPG_HEALTH,                                          //int     health;
		0.0f,                                                       //float   fallDamage;
		LEVEL2_UPG_REGEN,                                           //int     regenRate;
		SCA_NOWEAPONDRIFT | SCA_WALLJUMPER |
		SCA_FOVWARPS | SCA_ALIENSENSE,                              //int     abilities;
		WP_ALEVEL2_UPG,                                             //weapon_t  startWeapon
		0.0f,                                                       //float   buildDist;
		90,                                                         //int     fov;
		0.001f,                                                     //float   bob;
		1.5f,                                                       //float   bobCycle;
		80,                                                         //int     steptime;
		LEVEL2_UPG_SPEED,                                           //float   speed;
		10.0f,                                                      //float   acceleration;
		2.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		100.0f,                                                     //float   stopSpeed;
		400.0f,                                                     //float   jumpMagnitude;
		0.7f,                                                       //float   knockbackScale;
		{ PCL_ALIEN_LEVEL3,       PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
		LEVEL2_UPG_COST,                                            //int     cost;
		LEVEL2_UPG_VALUE                                            //int     value;
	},
	{
		PCL_ALIEN_LEVEL3,                                           //int     classnum;
		"level3",                                                   //char    *classname;
		"Dragoon",                                                  //char    *humanname;
		"prowl",                                                    //char    *modelname;
		1.0f,                                                       //float   modelScale;
		"default",                                                  //char    *skinname;
		1.0f,                                                       //float   shadowScale;
		"alien_general_hud",                                        //char    *hudname;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		{ -32,                    -32,                  -21      }, //vec3_t  mins;
		{ 32,                     32,                   21       }, //vec3_t  maxs;
		{ 32,                     32,                   21       }, //vec3_t  crouchmaxs;
		{ -32,                    -32,                  -4       }, //vec3_t  deadmins;
		{ 32,                     32,                   4        }, //vec3_t  deadmaxs;
		0.0f,                                                       //float   zOffset
		24, 24,                                                     //int     viewheight, crouchviewheight;
		LEVEL3_HEALTH,                                              //int     health;
		0.0f,                                                       //float   fallDamage;
		LEVEL3_REGEN,                                               //int     regenRate;
		SCA_NOWEAPONDRIFT |
		SCA_FOVWARPS | SCA_ALIENSENSE,                              //int     abilities;
		WP_ALEVEL3,                                                 //weapon_t  startWeapon
		0.0f,                                                       //float   buildDist;
		110,                                                        //int     fov;
		0.0005f,                                                    //float   bob;
		1.3f,                                                       //float   bobCycle;
		90,                                                         //int     steptime;
		LEVEL3_SPEED,                                               //float   speed;
		10.0f,                                                      //float   acceleration;
		1.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		200.0f,                                                     //float   stopSpeed;
		270.0f,                                                     //float   jumpMagnitude;
		0.5f,                                                       //float   knockbackScale;
		{ PCL_ALIEN_LEVEL4,       PCL_ALIEN_LEVEL3_UPG, PCL_NONE }, //int     children[ 3 ];
		LEVEL3_COST,                                                //int     cost;
		LEVEL3_VALUE                                                //int     value;
	},
	{
		PCL_ALIEN_LEVEL3_UPG,                                       //int     classnum;
		"level3upg",                                                //char    *classname;
		"Dragoon Upgrade",                                          //char    *humanname;
		"prowl",                                                    //char    *modelname;
		1.0f,                                                       //float   modelScale;
		"default",                                                  //char    *skinname;
		1.0f,                                                       //float   shadowScale;
		"alien_general_hud",                                        //char    *hudname;
		( 1 << S3 ),                                                //int  stages
		{ -32,                    -32,                  -21      }, //vec3_t  mins;
		{ 32,                     32,                   21       }, //vec3_t  maxs;
		{ 32,                     32,                   21       }, //vec3_t  crouchmaxs;
		{ -32,                    -32,                  -4       }, //vec3_t  deadmins;
		{ 32,                     32,                   4        }, //vec3_t  deadmaxs;
		0.0f,                                                       //float   zOffset
		27, 27,                                                     //int     viewheight, crouchviewheight;
		LEVEL3_UPG_HEALTH,                                          //int     health;
		0.0f,                                                       //float   fallDamage;
		LEVEL3_UPG_REGEN,                                           //int     regenRate;
		SCA_NOWEAPONDRIFT |
		SCA_FOVWARPS | SCA_ALIENSENSE,                              //int     abilities;
		WP_ALEVEL3_UPG,                                             //weapon_t  startWeapon
		0.0f,                                                       //float   buildDist;
		110,                                                        //int     fov;
		0.0005f,                                                    //float   bob;
		1.3f,                                                       //float   bobCycle;
		90,                                                         //int     steptime;
		LEVEL3_UPG_SPEED,                                           //float   speed;
		10.0f,                                                      //float   acceleration;
		1.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		200.0f,                                                     //float   stopSpeed;
		270.0f,                                                     //float   jumpMagnitude;
		0.4f,                                                       //float   knockbackScale;
		{ PCL_ALIEN_LEVEL4,       PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
		LEVEL3_UPG_COST,                                            //int     cost;
		LEVEL3_UPG_VALUE                                            //int     value;
	},
	{
		PCL_ALIEN_LEVEL4,                                           //int     classnum;
		"level4",                                                   //char    *classname;
		"Big Mofo",                                                 //char    *humanname;
		"mofo",                                                     //char    *modelname;
		1.0f,                                                       //float   modelScale;
		"default",                                                  //char    *skinname;
		2.0f,                                                       //float   shadowScale;
		"alien_general_hud",                                        //char    *hudname;
		( 1 << S3 ),                                                //int  stages
		{ -30,                    -30,                  -20      }, //vec3_t  mins;
		{ 30,                     30,                   20       }, //vec3_t  maxs;
		{ 30,                     30,                   20       }, //vec3_t  crouchmaxs;
		{ -15,                    -15,                  -4       }, //vec3_t  deadmins;
		{ 15,                     15,                   4        }, //vec3_t  deadmaxs;
		0.0f,                                                       //float   zOffset
		35, 35,                                                     //int     viewheight, crouchviewheight;
		LEVEL4_HEALTH,                                              //int     health;
		0.0f,                                                       //float   fallDamage;
		LEVEL4_REGEN,                                               //int     regenRate;
		SCA_NOWEAPONDRIFT |
		SCA_FOVWARPS | SCA_ALIENSENSE,                              //int     abilities;
		WP_ALEVEL4,                                                 //weapon_t  startWeapon
		0.0f,                                                       //float   buildDist;
		90,                                                         //int     fov;
		0.001f,                                                     //float   bob;
		1.1f,                                                       //float   bobCycle;
		100,                                                        //int     steptime;
		LEVEL4_SPEED,                                               //float   speed;
		10.0f,                                                      //float   acceleration;
		1.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		100.0f,                                                     //float   stopSpeed;
		170.0f,                                                     //float   jumpMagnitude;
		0.1f,                                                       //float   knockbackScale;
		{ PCL_NONE,               PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
		LEVEL4_COST,                                                //int     cost;
		LEVEL4_VALUE                                                //int     value;
	},
	{
		PCL_HUMAN,                                                  //int     classnum;
		"human_base",                                               //char    *classname;
		"Human",                                                    //char    *humanname;
		"sarge",                                                    //char    *modelname;
		1.0f,                                                       //float   modelScale;
		"default",                                                  //char    *skinname;
		1.0f,                                                       //float   shadowScale;
		"human_hud",                                                //char    *hudname;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ),                    //int  stages
		{ -15,                    -15,                  -24      }, //vec3_t  mins;
		{ 15,                     15,                   32       }, //vec3_t  maxs;
		{ 15,                     15,                   16       }, //vec3_t  crouchmaxs;
		{ -15,                    -15,                  -4       }, //vec3_t  deadmins;
		{ 15,                     15,                   4        }, //vec3_t  deadmaxs;
		0.0f,                                                       //float   zOffset
		26, 12,                                                     //int     viewheight, crouchviewheight;
		100,                                                        //int     health;
		1.0f,                                                       //float   fallDamage;
		0,                                                          //int     regenRate;
		SCA_TAKESFALLDAMAGE |
		SCA_CANUSELADDERS,                                          //int     abilities;
		WP_NONE,                                                    //special-cased in g_client.c          //weapon_t  startWeapon
		110.0f,                                                     //float   buildDist;
		90,                                                         //int     fov;
		0.002f,                                                     //float   bob;
		1.0f,                                                       //float   bobCycle;
		100,                                                        //int     steptime;
		1.0f,                                                       //float   speed;
		10.0f,                                                      //float   acceleration;
		1.0f,                                                       //float   airAcceleration;
		6.0f,                                                       //float   friction;
		100.0f,                                                     //float   stopSpeed;
		220.0f,                                                     //float   jumpMagnitude;
		1.0f,                                                       //float   knockbackScale;
		{ PCL_NONE,               PCL_NONE,             PCL_NONE }, //int     children[ 3 ];
		0,                                                          //int     cost;
		0                                                           //int     value;
	},
	{
		//this isn't a real class, but a dummy to force the client to precache the model
		//FIXME: one day do this in a less hacky fashion
		PCL_HUMAN_BSUIT, "human_bsuit", "bsuit",

		"keel",
		1.0f,
		"default",
		1.0f,

		"bsuit", ( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), { 0,                      0,                    0        },    { 0, 0, 0, },
		{ 0,                      0,                    0,       },    { 0, 0, 0, }, { 0, 0, 0, }, 0.0f, 0, 0, 0, 0.0f, 0, 0, WP_NONE, 0.0f, 0,
		0.0f, 1.0f, 0, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 270.0f, 1.0f, { PCL_NONE,               PCL_NONE,             PCL_NONE },    0, 0
	}
};

int                       bg_numPclasses = sizeof( bg_classList ) / sizeof( bg_classList[ 0 ] );

//separate from bg_classList to work around char struct init bug
classAttributeOverrides_t bg_classOverrideList[ PCL_NUM_CLASSES ];

/*
==============
BG_FindClassNumForName
==============
*/
int BG_FindClassNumForName( char *name )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( !Q_stricmp( bg_classList[ i ].className, name ) )
		{
			return bg_classList[ i ].classNum;
		}
	}

	//wimp out
	return PCL_NONE;
}

/*
==============
BG_FindNameForClassNum
==============
*/
char *BG_FindNameForClassNum( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].className;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindNameForClassNum\n" );
	//wimp out
	return 0;
}

/*
==============
BG_FindHumanNameForClassNum
==============
*/
char *BG_FindHumanNameForClassNum( int pclass )
{
	int i;

	if ( bg_classOverrideList[ pclass ].humanName[ 0 ] != 0 )
	{
		return bg_classOverrideList[ pclass ].humanName;
	}

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].humanName;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindHumanNameForClassNum\n" );
	//wimp out
	return 0;
}

/*
==============
BG_FindModelNameForClass
==============
*/
char *BG_FindModelNameForClass( int pclass )
{
	int i;

	if ( bg_classOverrideList[ pclass ].modelName[ 0 ] != 0 )
	{
		return bg_classOverrideList[ pclass ].modelName;
	}

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].modelName;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindModelNameForClass\n" );
	//note: must return a valid modelName!
	return bg_classList[ 0 ].modelName;
}

/*
==============
BG_FindModelScaleForClass
==============
*/
float BG_FindModelScaleForClass( int pclass )
{
	int i;

	if ( bg_classOverrideList[ pclass ].modelScale != 0.0f )
	{
		return bg_classOverrideList[ pclass ].modelScale;
	}

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].modelScale;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindModelScaleForClass( %d )\n", pclass );
	return 1.0f;
}

/*
==============
BG_FindSkinNameForClass
==============
*/
char *BG_FindSkinNameForClass( int pclass )
{
	int i;

	if ( bg_classOverrideList[ pclass ].skinName[ 0 ] != 0 )
	{
		return bg_classOverrideList[ pclass ].skinName;
	}

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].skinName;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindSkinNameForClass\n" );
	//note: must return a valid modelName!
	return bg_classList[ 0 ].skinName;
}

/*
==============
BG_FindShadowScaleForClass
==============
*/
float BG_FindShadowScaleForClass( int pclass )
{
	int i;

	if ( bg_classOverrideList[ pclass ].shadowScale != 0.0f )
	{
		return bg_classOverrideList[ pclass ].shadowScale;
	}

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].shadowScale;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindShadowScaleForClass( %d )\n", pclass );
	return 1.0f;
}

/*
==============
BG_FindHudNameForClass
==============
*/
char *BG_FindHudNameForClass( int pclass )
{
	int i;

	if ( bg_classOverrideList[ pclass ].hudName[ 0 ] != 0 )
	{
		return bg_classOverrideList[ pclass ].hudName;
	}

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].hudName;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindHudNameForClass\n" );
	//note: must return a valid hudName!
	return bg_classList[ 0 ].hudName;
}

/*
==============
BG_FindStagesForClass
==============
*/
qboolean BG_FindStagesForClass( int pclass, stage_t stage )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			if ( bg_classList[ i ].stages & ( 1 << stage ) )
			{
				return qtrue;
			}
			else
			{
				return qfalse;
			}
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindStagesForClass\n" );
	return qfalse;
}

/*
==============
BG_FindBBoxForClass
==============
*/
void BG_FindBBoxForClass( int pclass, vec3_t mins, vec3_t maxs, vec3_t cmaxs, vec3_t dmins, vec3_t dmaxs )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			if ( mins != NULL )
			{
				VectorCopy( bg_classList[ i ].mins, mins );

				if ( VectorLength( bg_classOverrideList[ pclass ].mins ) )
				{
					VectorCopy( bg_classOverrideList[ pclass ].mins, mins );
				}
			}

			if ( maxs != NULL )
			{
				VectorCopy( bg_classList[ i ].maxs, maxs );

				if ( VectorLength( bg_classOverrideList[ pclass ].maxs ) )
				{
					VectorCopy( bg_classOverrideList[ pclass ].maxs, maxs );
				}
			}

			if ( cmaxs != NULL )
			{
				VectorCopy( bg_classList[ i ].crouchMaxs, cmaxs );

				if ( VectorLength( bg_classOverrideList[ pclass ].crouchMaxs ) )
				{
					VectorCopy( bg_classOverrideList[ pclass ].crouchMaxs, cmaxs );
				}
			}

			if ( dmins != NULL )
			{
				VectorCopy( bg_classList[ i ].deadMins, dmins );

				if ( VectorLength( bg_classOverrideList[ pclass ].deadMins ) )
				{
					VectorCopy( bg_classOverrideList[ pclass ].deadMins, dmins );
				}
			}

			if ( dmaxs != NULL )
			{
				VectorCopy( bg_classList[ i ].deadMaxs, dmaxs );

				if ( VectorLength( bg_classOverrideList[ pclass ].deadMaxs ) )
				{
					VectorCopy( bg_classOverrideList[ pclass ].deadMaxs, dmaxs );
				}
			}

			return;
		}
	}

	if ( mins != NULL )
	{
		VectorCopy( bg_classList[ 0 ].mins,        mins );
	}

	if ( maxs != NULL )
	{
		VectorCopy( bg_classList[ 0 ].maxs,        maxs );
	}

	if ( cmaxs != NULL )
	{
		VectorCopy( bg_classList[ 0 ].crouchMaxs,  cmaxs );
	}

	if ( dmins != NULL )
	{
		VectorCopy( bg_classList[ 0 ].deadMins,    dmins );
	}

	if ( dmaxs != NULL )
	{
		VectorCopy( bg_classList[ 0 ].deadMaxs,    dmaxs );
	}
}

/*
==============
BG_FindZOffsetForClass
==============
*/
float BG_FindZOffsetForClass( int pclass )
{
	int i;

	if ( bg_classOverrideList[ pclass ].zOffset != 0.0f )
	{
		return bg_classOverrideList[ pclass ].zOffset;
	}

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].zOffset;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindZOffsetForClass\n" );
	return 0.0f;
}

/*
==============
BG_FindViewheightForClass
==============
*/
void BG_FindViewheightForClass( int pclass, int *viewheight, int *cViewheight )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			if ( viewheight != NULL )
			{
				*viewheight = bg_classList[ i ].viewheight;
			}

			if ( cViewheight != NULL )
			{
				*cViewheight = bg_classList[ i ].crouchViewheight;
			}

			return;
		}
	}

	if ( viewheight != NULL )
	{
		*viewheight = bg_classList[ 0 ].viewheight;
	}

	if ( cViewheight != NULL )
	{
		*cViewheight = bg_classList[ 0 ].crouchViewheight;
	}
}

/*
==============
BG_FindHealthForClass
==============
*/
int BG_FindHealthForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].health;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindHealthForClass\n" );
	return 100;
}

/*
==============
BG_FindFallDamageForClass
==============
*/
float BG_FindFallDamageForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].fallDamage;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindFallDamageForClass\n" );
	return 100;
}

/*
==============
BG_FindRegenRateForClass
==============
*/
int BG_FindRegenRateForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].regenRate;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindRegenRateForClass\n" );
	return 0;
}

/*
==============
BG_FindFovForClass
==============
*/
int BG_FindFovForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].fov;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindFovForClass\n" );
	return 90;
}

/*
==============
BG_FindBobForClass
==============
*/
float BG_FindBobForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].bob;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindBobForClass\n" );
	return 0.002;
}

/*
==============
BG_FindBobCycleForClass
==============
*/
float BG_FindBobCycleForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].bobCycle;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindBobCycleForClass\n" );
	return 1.0f;
}

/*
==============
BG_FindSpeedForClass
==============
*/
float BG_FindSpeedForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].speed;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindSpeedForClass\n" );
	return 1.0f;
}

/*
==============
BG_FindAccelerationForClass
==============
*/
float BG_FindAccelerationForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].acceleration;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindAccelerationForClass\n" );
	return 10.0f;
}

/*
==============
BG_FindAirAccelerationForClass
==============
*/
float BG_FindAirAccelerationForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].airAcceleration;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindAirAccelerationForClass\n" );
	return 1.0f;
}

/*
==============
BG_FindFrictionForClass
==============
*/
float BG_FindFrictionForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].friction;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindFrictionForClass\n" );
	return 6.0f;
}

/*
==============
BG_FindStopSpeedForClass
==============
*/
float BG_FindStopSpeedForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].stopSpeed;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindStopSpeedForClass\n" );
	return 100.0f;
}

/*
==============
BG_FindJumpMagnitudeForClass
==============
*/
float BG_FindJumpMagnitudeForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].jumpMagnitude;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindJumpMagnitudeForClass\n" );
	return 270.0f;
}

/*
==============
BG_FindKnockbackScaleForClass
==============
*/
float BG_FindKnockbackScaleForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].knockbackScale;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindKnockbackScaleForClass\n" );
	return 1.0f;
}

/*
==============
BG_FindSteptimeForClass
==============
*/
int BG_FindSteptimeForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].steptime;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindSteptimeForClass\n" );
	return 200;
}

/*
==============
BG_ClassHasAbility
==============
*/
qboolean BG_ClassHasAbility( int pclass, int ability )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return ( bg_classList[ i ].abilities & ability );
		}
	}

	return qfalse;
}

/*
==============
BG_FindStartWeaponForClass
==============
*/
weapon_t BG_FindStartWeaponForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].startWeapon;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindStartWeaponForClass\n" );
	return WP_NONE;
}

/*
==============
BG_FindBuildDistForClass
==============
*/
float BG_FindBuildDistForClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].buildDist;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindBuildDistForClass\n" );
	return 0.0f;
}

/*
==============
BG_ClassCanEvolveFromTo
==============
*/
int BG_ClassCanEvolveFromTo( int fclass, int tclass, int credits, int num )
{
	int i, j, cost;

	cost = BG_FindCostOfClass( tclass );

	//base case
	if ( credits < cost )
	{
		return -1;
	}

	if ( fclass == PCL_NONE || tclass == PCL_NONE )
	{
		return -1;
	}

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == fclass )
		{
			for ( j = 0; j < 3; j++ )
			{
				if ( bg_classList[ i ].children[ j ] == tclass )
				{
					return num + cost;
				}
			}

			for ( j = 0; j < 3; j++ )
			{
				int sub;

				cost = BG_FindCostOfClass( bg_classList[ i ].children[ j ] );
				sub  = BG_ClassCanEvolveFromTo( bg_classList[ i ].children[ j ],
				                                tclass, credits - cost, num + cost );

				if ( sub >= 0 )
				{
					return sub;
				}
			}

			return -1; //may as well return by this point
		}
	}

	return -1;
}

/*
==============
BG_FindValueOfClass
==============
*/
int BG_FindValueOfClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].value;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindValueOfClass\n" );
	return 0;
}

/*
==============
BG_FindCostOfClass
==============
*/
int BG_FindCostOfClass( int pclass )
{
	int i;

	for ( i = 0; i < bg_numPclasses; i++ )
	{
		if ( bg_classList[ i ].classNum == pclass )
		{
			return bg_classList[ i ].cost;
		}
	}

	Com_Printf( S_COLOR_YELLOW "WARNING: fallthrough in BG_FindCostOfClass\n" );
	return 0;
}

/*
==============
BG_FindOverrideForClass
==============
*/
static classAttributeOverrides_t *BG_FindOverrideForClass( int pclass )
{
	return &bg_classOverrideList[ pclass ];
}

/*
======================
BG_ParseClassFile

Parses a configuration file describing a class
======================
*/
static qboolean BG_ParseClassFile( const char *filename, classAttributeOverrides_t *cao )
{
	char         *text_p;
	int          i;
	int          len;
	char         *token;
	char         text[ 20000 ];
	fileHandle_t f;
	float        scale = 0.0f;

	// load the file
	len = trap_FS_FOpenFile( filename, &f, FS_READ );

	if ( len <= 0 )
	{
		return qfalse;
	}

	if ( len >= sizeof( text ) - 1 )
	{
		Com_Printf( S_COLOR_RED "ERROR: Class file %s too long\n", filename );
		return qfalse;
	}

	trap_FS_Read( text, len, f );
	text[ len ] = 0;
	trap_FS_FCloseFile( f );

	// parse the text
	text_p = text;

	// read optional parameters
	while ( 1 )
	{
		token = COM_Parse( &text_p );

		if ( !token )
		{
			break;
		}

		if ( !Q_stricmp( token, "" ) )
		{
			break;
		}

		if ( !Q_stricmp( token, "model" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			Q_strncpyz( cao->modelName, token, sizeof( cao->modelName ) );

			continue;
		}
		else if ( !Q_stricmp( token, "skin" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			Q_strncpyz( cao->skinName, token, sizeof( cao->skinName ) );

			continue;
		}
		else if ( !Q_stricmp( token, "hud" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			Q_strncpyz( cao->hudName, token, sizeof( cao->hudName ) );

			continue;
		}
		else if ( !Q_stricmp( token, "modelScale" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			scale = atof( token );

			if ( scale < 0.0f )
			{
				scale = 0.0f;
			}

			cao->modelScale = scale;

			continue;
		}
		else if ( !Q_stricmp( token, "shadowScale" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			scale = atof( token );

			if ( scale < 0.0f )
			{
				scale = 0.0f;
			}

			cao->shadowScale = scale;

			continue;
		}
		else if ( !Q_stricmp( token, "mins" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( &text_p );

				if ( !token )
				{
					break;
				}

				cao->mins[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "maxs" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( &text_p );

				if ( !token )
				{
					break;
				}

				cao->maxs[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "deadMins" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( &text_p );

				if ( !token )
				{
					break;
				}

				cao->deadMins[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "deadMaxs" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( &text_p );

				if ( !token )
				{
					break;
				}

				cao->deadMaxs[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "crouchMaxs" ) )
		{
			for ( i = 0; i <= 2; i++ )
			{
				token = COM_Parse( &text_p );

				if ( !token )
				{
					break;
				}

				cao->crouchMaxs[ i ] = atof( token );
			}

			continue;
		}
		else if ( !Q_stricmp( token, "zOffset" ) )
		{
			float offset;

			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			offset       = atof( token );

			cao->zOffset = offset;

			continue;
		}
		else if ( !Q_stricmp( token, "name" ) )
		{
			token = COM_Parse( &text_p );

			if ( !token )
			{
				break;
			}

			Q_strncpyz( cao->humanName, token, sizeof( cao->humanName ) );

			continue;
		}

		Com_Printf( S_COLOR_RED "ERROR: unknown token '%s'\n", token );
		return qfalse;
	}

	return qtrue;
}

/*
===============
BG_InitClassOverrides

Set any overrides specfied by file
===============
*/
void BG_InitClassOverrides( void )
{
	int                       i;
	classAttributeOverrides_t *cao;

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		cao = BG_FindOverrideForClass( i );

		BG_ParseClassFile( va( "overrides/classes/%s.cfg", BG_FindNameForClassNum( i ) ), cao );
	}
}

////////////////////////////////////////////////////////////////////////////////

weaponAttributes_t bg_weapons[] =
{
	{
		WP_BLASTER,                              //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		0,                                       //int       slots;
		"blaster",                               //char      *weaponName;
		"Blaster",                               //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		BLASTER_REPEAT,                          //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		WP_MACHINEGUN,                           //int       weaponNum;
		RIFLE_PRICE,                             //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"rifle",                                 //char      *weaponName;
		"Rifle",                                 //char      *weaponHumanName;
		RIFLE_CLIPSIZE,                          //int       maxAmmo;
		RIFLE_MAXCLIPS,                          //int       maxClips;
		qfalse,                                  //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		RIFLE_REPEAT,                            //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		RIFLE_RELOAD,                            //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qtrue,                                   //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		WP_SHOTGUN,                              //int       weaponNum;
		SHOTGUN_PRICE,                           //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"shotgun",                               //char      *weaponName;
		"Shotgun",                               //char      *weaponHumanName;
		SHOTGUN_SHELLS,                          //int       maxAmmo;
		SHOTGUN_MAXCLIPS,                        //int       maxClips;
		qfalse,                                  //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		SHOTGUN_REPEAT,                          //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		SHOTGUN_RELOAD,                          //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qtrue,                                   //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		WP_FLAMER,                 //int       weaponNum;
		FLAMER_PRICE,              //int       price;
		( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,               //int       slots;
		"flamer",                  //char      *weaponName;
		"Flame Thrower",           //char      *weaponHumanName;
		FLAMER_GAS,                //int       maxAmmo;
		0,                         //int       maxClips;
		qfalse,                    //int       infiniteAmmo;
		qfalse,                    //int       usesEnergy;
		FLAMER_REPEAT,             //int       repeatRate1;
		0,                         //int       repeatRate2;
		0,                         //int       repeatRate3;
		0,                         //int       reloadTime;
		qfalse,                    //qboolean  hasAltMode;
		qfalse,                    //qboolean  hasThirdMode;
		qfalse,                    //qboolean  canZoom;
		90.0f,                     //float     zoomFov;
		qtrue,                     //qboolean  purchasable;
		0,                         //int       buildDelay;
		WUT_HUMANS                 //WUTeam_t  team;
	},
	{
		WP_CHAINGUN,                             //int       weaponNum;
		CHAINGUN_PRICE,                          //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"chaingun",                              //char      *weaponName;
		"Chaingun",                              //char      *weaponHumanName;
		CHAINGUN_BULLETS,                        //int       maxAmmo;
		0,                                       //int       maxClips;
		qfalse,                                  //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		CHAINGUN_REPEAT,                         //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qtrue,                                   //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		WP_MASS_DRIVER,                          //int       weaponNum;
		MDRIVER_PRICE,                           //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"mdriver",                               //char      *weaponName;
		"Mass Driver",                           //char      *weaponHumanName;
		MDRIVER_CLIPSIZE,                        //int       maxAmmo;
		MDRIVER_MAXCLIPS,                        //int       maxClips;
		qfalse,                                  //int       infiniteAmmo;
		qtrue,                                   //int       usesEnergy;
		MDRIVER_REPEAT,                          //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		MDRIVER_RELOAD,                          //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qtrue,                                   //qboolean  canZoom;
		20.0f,                                   //float     zoomFov;
		qtrue,                                   //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		WP_PULSE_RIFLE,            //int       weaponNum;
		PRIFLE_PRICE,              //int       price;
		( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,               //int       slots;
		"prifle",                  //char      *weaponName;
		"Pulse Rifle",             //char      *weaponHumanName;
		PRIFLE_CLIPS,              //int       maxAmmo;
		PRIFLE_MAXCLIPS,           //int       maxClips;
		qfalse,                    //int       infiniteAmmo;
		qtrue,                     //int       usesEnergy;
		PRIFLE_REPEAT,             //int       repeatRate1;
		0,                         //int       repeatRate2;
		0,                         //int       repeatRate3;
		PRIFLE_RELOAD,             //int       reloadTime;
		qfalse,                    //qboolean  hasAltMode;
		qfalse,                    //qboolean  hasThirdMode;
		qfalse,                    //qboolean  canZoom;
		90.0f,                     //float     zoomFov;
		qtrue,                     //qboolean  purchasable;
		0,                         //int       buildDelay;
		WUT_HUMANS                 //WUTeam_t  team;
	},
	{
		WP_LUCIFER_CANNON,    //int       weaponNum;
		LCANNON_PRICE,        //int       price;
		( 1 << S3 ),          //int  stages
		SLOT_WEAPON,          //int       slots;
		"lcannon",            //char      *weaponName;
		"Lucifer Cannon",     //char      *weaponHumanName;
		LCANNON_AMMO,         //int       maxAmmo;
		0,                    //int       maxClips;
		qfalse,               //int       infiniteAmmo;
		qtrue,                //int       usesEnergy;
		LCANNON_REPEAT,       //int       repeatRate1;
		LCANNON_CHARGEREPEAT, //int       repeatRate2;
		0,                    //int       repeatRate3;
		LCANNON_RELOAD,       //int       reloadTime;
		qtrue,                //qboolean  hasAltMode;
		qfalse,               //qboolean  hasThirdMode;
		qfalse,               //qboolean  canZoom;
		90.0f,                //float     zoomFov;
		qtrue,                //qboolean  purchasable;
		0,                    //int       buildDelay;
		WUT_HUMANS            //WUTeam_t  team;
	},
	{
		WP_LAS_GUN,                              //int       weaponNum;
		LASGUN_PRICE,                            //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"lgun",                                  //char      *weaponName;
		"Las Gun",                               //char      *weaponHumanName;
		LASGUN_AMMO,                             //int       maxAmmo;
		0,                                       //int       maxClips;
		qfalse,                                  //int       infiniteAmmo;
		qtrue,                                   //int       usesEnergy;
		LASGUN_REPEAT,                           //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		LASGUN_RELOAD,                           //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qtrue,                                   //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		WP_PAIN_SAW,                             //int       weaponNum;
		PAINSAW_PRICE,                           //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"psaw",                                  //char      *weaponName;
		"Pain Saw",                              //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		PAINSAW_REPEAT,                          //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qtrue,                                   //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		WP_GRENADE,                //int       weaponNum;
		GRENADE_PRICE,             //int       price;
		( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_NONE,                 //int       slots;
		"grenade",                 //char      *weaponName;
		"Grenade",                 //char      *weaponHumanName;
		1,                         //int       maxAmmo;
		0,                         //int       maxClips;
		qfalse,                    //int       infiniteAmmo;
		qfalse,                    //int       usesEnergy;
		GRENADE_REPEAT,            //int       repeatRate1;
		0,                         //int       repeatRate2;
		0,                         //int       repeatRate3;
		0,                         //int       reloadTime;
		qfalse,                    //qboolean  hasAltMode;
		qfalse,                    //qboolean  hasThirdMode;
		qfalse,                    //qboolean  canZoom;
		90.0f,                     //float     zoomFov;
		qfalse,                    //qboolean  purchasable;
		0,                         //int       buildDelay;
		WUT_HUMANS                 //WUTeam_t  team;
	},
	{
		WP_HBUILD,                               //int       weaponNum;
		HBUILD_PRICE,                            //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"ckit",                                  //char      *weaponName;
		"Construction Kit",                      //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		HBUILD_REPEAT,                           //int       repeatRate1;
		HBUILD_REPEAT,                           //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qtrue,                                   //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qtrue,                                   //qboolean  purchasable;
		HBUILD_DELAY,                            //int       buildDelay;
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		WP_HBUILD2,                //int       weaponNum;
		HBUILD2_PRICE,             //int       price;
		( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,               //int       slots;
		"ackit",                   //char      *weaponName;
		"Adv Construction Kit",    //char      *weaponHumanName;
		0,                         //int       maxAmmo;
		0,                         //int       maxClips;
		qtrue,                     //int       infiniteAmmo;
		qfalse,                    //int       usesEnergy;
		HBUILD2_REPEAT,            //int       repeatRate1;
		HBUILD2_REPEAT,            //int       repeatRate2;
		0,                         //int       repeatRate3;
		0,                         //int       reloadTime;
		qtrue,                     //qboolean  hasAltMode;
		qfalse,                    //qboolean  hasThirdMode;
		qfalse,                    //qboolean  canZoom;
		90.0f,                     //float     zoomFov;
		qtrue,                     //qboolean  purchasable;
		HBUILD2_DELAY,             //int       buildDelay;
		WUT_HUMANS                 //WUTeam_t  team;
	},
	{
		WP_ABUILD,                               //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"abuild",                                //char      *weaponName;
		"Alien build weapon",                    //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		ABUILDER_BUILD_REPEAT,                   //int       repeatRate1;
		ABUILDER_BUILD_REPEAT,                   //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qtrue,                                   //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qtrue,                                   //qboolean  purchasable;
		ABUILDER_BASE_DELAY,                     //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_ABUILD2,                              //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"abuildupg",                             //char      *weaponName;
		"Alien build weapon2",                   //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		ABUILDER_BUILD_REPEAT,                   //int       repeatRate1;
		ABUILDER_CLAW_REPEAT,                    //int       repeatRate2;
		ABUILDER_BLOB_REPEAT,                    //int       repeatRate3;
		0,                                       //int       reloadTime;
		qtrue,                                   //qboolean  hasAltMode;
		qtrue,                                   //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qtrue,                                   //qboolean  purchasable;
		ABUILDER_ADV_DELAY,                      //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_ALEVEL0,                              //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"level0",                                //char      *weaponName;
		"Bite",                                  //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		LEVEL0_BITE_REPEAT,                      //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_ALEVEL1,                              //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"level1",                                //char      *weaponName;
		"Claws",                                 //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		LEVEL1_CLAW_REPEAT,                      //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_ALEVEL1_UPG,                          //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"level1upg",                             //char      *weaponName;
		"Claws Upgrade",                         //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		LEVEL1_CLAW_U_REPEAT,                    //int       repeatRate1;
		LEVEL1_PCLOUD_REPEAT,                    //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qtrue,                                   //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_ALEVEL2,                              //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"level2",                                //char      *weaponName;
		"Bite",                                  //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		LEVEL2_CLAW_REPEAT,                      //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_ALEVEL2_UPG,                          //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"level2upg",                             //char      *weaponName;
		"Zap",                                   //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		LEVEL2_CLAW_U_REPEAT,                    //int       repeatRate1;
		LEVEL2_AREAZAP_REPEAT,                   //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qtrue,                                   //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_ALEVEL3,                              //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"level3",                                //char      *weaponName;
		"Pounce",                                //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		LEVEL3_CLAW_REPEAT,                      //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_ALEVEL3_UPG,                          //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"level3upg",                             //char      *weaponName;
		"Pounce (upgrade)",                      //char      *weaponHumanName;
		3,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		LEVEL3_CLAW_U_REPEAT,                    //int       repeatRate1;
		0,                                       //int       repeatRate2;
		LEVEL3_BOUNCEBALL_REPEAT,                //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qtrue,                                   //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_ALEVEL4,                              //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"level4",                                //char      *weaponName;
		"Charge",                                //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		LEVEL4_CLAW_REPEAT,                      //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_LOCKBLOB_LAUNCHER,                    //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"lockblob",                              //char      *weaponName;
		"Lock Blob",                             //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		500,                                     //int       repeatRate1;
		500,                                     //int       repeatRate2;
		500,                                     //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_HIVE,                                 //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"hive",                                  //char      *weaponName;
		"Hive",                                  //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		500,                                     //int       repeatRate1;
		500,                                     //int       repeatRate2;
		500,                                     //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_ALIENS                               //WUTeam_t  team;
	},
	{
		WP_MGTURRET,                             //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"mgturret",                              //char      *weaponName;
		"Machinegun Turret",                     //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qfalse,                                  //int       usesEnergy;
		0,                                       //int       repeatRate1;
		0,                                       //int       repeatRate2;
		0,                                       //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		WP_TESLAGEN,                             //int       weaponNum;
		0,                                       //int       price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_WEAPON,                             //int       slots;
		"teslagen",                              //char      *weaponName;
		"Tesla Generator",                       //char      *weaponHumanName;
		0,                                       //int       maxAmmo;
		0,                                       //int       maxClips;
		qtrue,                                   //int       infiniteAmmo;
		qtrue,                                   //int       usesEnergy;
		500,                                     //int       repeatRate1;
		500,                                     //int       repeatRate2;
		500,                                     //int       repeatRate3;
		0,                                       //int       reloadTime;
		qfalse,                                  //qboolean  hasAltMode;
		qfalse,                                  //qboolean  hasThirdMode;
		qfalse,                                  //qboolean  canZoom;
		90.0f,                                   //float     zoomFov;
		qfalse,                                  //qboolean  purchasable;
		0,                                       //int       buildDelay;
		WUT_HUMANS                               //WUTeam_t  team;
	}
};

int                bg_numWeapons = sizeof( bg_weapons ) / sizeof( bg_weapons[ 0 ] );

/*
==============
BG_FindPriceForWeapon
==============
*/
int BG_FindPriceForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].price;
		}
	}

	return 100;
}

/*
==============
BG_FindStagesForWeapon
==============
*/
qboolean BG_FindStagesForWeapon( int weapon, stage_t stage )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			if ( bg_weapons[ i ].stages & ( 1 << stage ) )
			{
				return qtrue;
			}
			else
			{
				return qfalse;
			}
		}
	}

	return qfalse;
}

/*
==============
BG_FindSlotsForWeapon
==============
*/
int BG_FindSlotsForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].slots;
		}
	}

	return SLOT_WEAPON;
}

/*
==============
BG_FindNameForWeapon
==============
*/
char *BG_FindNameForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].weaponName;
		}
	}

	//wimp out
	return 0;
}

/*
==============
BG_FindWeaponNumForName
==============
*/
int BG_FindWeaponNumForName( char *name )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( !Q_stricmp( bg_weapons[ i ].weaponName, name ) )
		{
			return bg_weapons[ i ].weaponNum;
		}
	}

	//wimp out
	return WP_NONE;
}

/*
==============
BG_FindHumanNameForWeapon
==============
*/
char *BG_FindHumanNameForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].weaponHumanName;
		}
	}

	//wimp out
	return 0;
}

/*
==============
BG_FindAmmoForWeapon
==============
*/
void BG_FindAmmoForWeapon( int weapon, int *maxAmmo, int *maxClips )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			if ( maxAmmo != NULL )
			{
				*maxAmmo = bg_weapons[ i ].maxAmmo;
			}

			if ( maxClips != NULL )
			{
				*maxClips = bg_weapons[ i ].maxClips;
			}

			//no need to keep going
			break;
		}
	}
}

/*
==============
BG_FindInfinteAmmoForWeapon
==============
*/
qboolean BG_FindInfinteAmmoForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].infiniteAmmo;
		}
	}

	return qfalse;
}

/*
==============
BG_FindUsesEnergyForWeapon
==============
*/
qboolean BG_FindUsesEnergyForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].usesEnergy;
		}
	}

	return qfalse;
}

/*
==============
BG_FindRepeatRate1ForWeapon
==============
*/
int BG_FindRepeatRate1ForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].repeatRate1;
		}
	}

	return 1000;
}

/*
==============
BG_FindRepeatRate2ForWeapon
==============
*/
int BG_FindRepeatRate2ForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].repeatRate2;
		}
	}

	return 1000;
}

/*
==============
BG_FindRepeatRate3ForWeapon
==============
*/
int BG_FindRepeatRate3ForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].repeatRate3;
		}
	}

	return 1000;
}

/*
==============
BG_FindReloadTimeForWeapon
==============
*/
int BG_FindReloadTimeForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].reloadTime;
		}
	}

	return 1000;
}

/*
==============
BG_WeaponHasAltMode
==============
*/
qboolean BG_WeaponHasAltMode( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].hasAltMode;
		}
	}

	return qfalse;
}

/*
==============
BG_WeaponHasThirdMode
==============
*/
qboolean BG_WeaponHasThirdMode( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].hasThirdMode;
		}
	}

	return qfalse;
}

/*
==============
BG_WeaponCanZoom
==============
*/
qboolean BG_WeaponCanZoom( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].canZoom;
		}
	}

	return qfalse;
}

/*
==============
BG_FindZoomFovForWeapon
==============
*/
float BG_FindZoomFovForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].zoomFov;
		}
	}

	return qfalse;
}

/*
==============
BG_FindPurchasableForWeapon
==============
*/
qboolean BG_FindPurchasableForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].purchasable;
		}
	}

	return qfalse;
}

/*
==============
BG_FindBuildDelayForWeapon
==============
*/
int BG_FindBuildDelayForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].buildDelay;
		}
	}

	return 0;
}

/*
==============
BG_FindTeamForWeapon
==============
*/
WUTeam_t BG_FindTeamForWeapon( int weapon )
{
	int i;

	for ( i = 0; i < bg_numWeapons; i++ )
	{
		if ( bg_weapons[ i ].weaponNum == weapon )
		{
			return bg_weapons[ i ].team;
		}
	}

	return WUT_NONE;
}

////////////////////////////////////////////////////////////////////////////////

upgradeAttributes_t bg_upgrades[] =
{
	{
		UP_LIGHTARMOUR,                          //int   upgradeNum;
		LIGHTARMOUR_PRICE,                       //int   price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_TORSO | SLOT_ARMS | SLOT_LEGS,      //int   slots;
		"larmour",                               //char  *upgradeName;
		"Light Armour",                          //char  *upgradeHumanName;
		"icons/iconu_larmour",
		qtrue,                                   //qboolean purchasable
		qfalse,                                  //qboolean usable
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		UP_HELMET,                 //int   upgradeNum;
		HELMET_PRICE,              //int   price;
		( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_HEAD,                 //int   slots;
		"helmet",                  //char  *upgradeName;
		"Helmet",                  //char  *upgradeHumanName;
		"icons/iconu_helmet",
		qtrue,                     //qboolean purchasable
		qfalse,                    //qboolean usable
		WUT_HUMANS                 //WUTeam_t  team;
	},
	{
		UP_MEDKIT,                               //int   upgradeNum;
		MEDKIT_PRICE,                            //int   price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_NONE,                               //int   slots;
		"medkit",                                //char  *upgradeName;
		"Medkit",                                //char  *upgradeHumanName;
		"icons/iconu_atoxin",
		qfalse,                                  //qboolean purchasable
		qtrue,                                   //qboolean usable
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		UP_BATTPACK,                             //int   upgradeNum;
		BATTPACK_PRICE,                          //int   price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_BACKPACK,                           //int   slots;
		"battpack",                              //char  *upgradeName;
		"Battery Pack",                          //char  *upgradeHumanName;
		"icons/iconu_battpack",
		qtrue,                                   //qboolean purchasable
		qfalse,                                  //qboolean usable
		WUT_HUMANS                               //WUTeam_t  team;
	},
	{
		UP_JETPACK,                //int   upgradeNum;
		JETPACK_PRICE,             //int   price;
		( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_BACKPACK,             //int   slots;
		"jetpack",                 //char  *upgradeName;
		"Jet Pack",                //char  *upgradeHumanName;
		"icons/iconu_jetpack",
		qtrue,                     //qboolean purchasable
		qtrue,                     //qboolean usable
		WUT_HUMANS                 //WUTeam_t  team;
	},
	{
		UP_BATTLESUIT,                                                  //int   upgradeNum;
		BSUIT_PRICE,                                                    //int   price;
		( 1 << S3 ),                                                    //int  stages
		SLOT_HEAD | SLOT_TORSO | SLOT_ARMS | SLOT_LEGS | SLOT_BACKPACK, //int   slots;
		"bsuit",                                                        //char  *upgradeName;
		"Battlesuit",                                                   //char  *upgradeHumanName;
		"icons/iconu_bsuit",
		qtrue,                                                          //qboolean purchasable
		qfalse,                                                         //qboolean usable
		WUT_HUMANS                                                      //WUTeam_t  team;
	},
	{
		UP_GRENADE,                //int   upgradeNum;
		GRENADE_PRICE,             //int   price;
		( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_NONE,                 //int   slots;
		"gren",                    //char  *upgradeName;
		"Grenade",                 //char  *upgradeHumanName;
		0,
		qtrue,                     //qboolean purchasable
		qtrue,                     //qboolean usable
		WUT_HUMANS                 //WUTeam_t  team;
	},
	{
		UP_AMMO,                                 //int   upgradeNum;
		0,                                       //int   price;
		( 1 << S1 ) | ( 1 << S2 ) | ( 1 << S3 ), //int  stages
		SLOT_NONE,                               //int   slots;
		"ammo",                                  //char  *upgradeName;
		"Ammunition",                            //char  *upgradeHumanName;
		0,
		qtrue,                                   //qboolean purchasable
		qfalse,                                  //qboolean usable
		WUT_HUMANS                               //WUTeam_t  team;
	}
};

int                 bg_numUpgrades = sizeof( bg_upgrades ) / sizeof( bg_upgrades[ 0 ] );

/*
==============
BG_FindPriceForUpgrade
==============
*/
int BG_FindPriceForUpgrade( int upgrade )
{
	int i;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		if ( bg_upgrades[ i ].upgradeNum == upgrade )
		{
			return bg_upgrades[ i ].price;
		}
	}

	return 100;
}

/*
==============
BG_FindStagesForUpgrade
==============
*/
qboolean BG_FindStagesForUpgrade( int upgrade, stage_t stage )
{
	int i;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		if ( bg_upgrades[ i ].upgradeNum == upgrade )
		{
			if ( bg_upgrades[ i ].stages & ( 1 << stage ) )
			{
				return qtrue;
			}
			else
			{
				return qfalse;
			}
		}
	}

	return qfalse;
}

/*
==============
BG_FindSlotsForUpgrade
==============
*/
int BG_FindSlotsForUpgrade( int upgrade )
{
	int i;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		if ( bg_upgrades[ i ].upgradeNum == upgrade )
		{
			return bg_upgrades[ i ].slots;
		}
	}

	return SLOT_NONE;
}

/*
==============
BG_FindNameForUpgrade
==============
*/
char *BG_FindNameForUpgrade( int upgrade )
{
	int i;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		if ( bg_upgrades[ i ].upgradeNum == upgrade )
		{
			return bg_upgrades[ i ].upgradeName;
		}
	}

	//wimp out
	return 0;
}

/*
==============
BG_FindUpgradeNumForName
==============
*/
int BG_FindUpgradeNumForName( char *name )
{
	int i;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		if ( !Q_stricmp( bg_upgrades[ i ].upgradeName, name ) )
		{
			return bg_upgrades[ i ].upgradeNum;
		}
	}

	//wimp out
	return UP_NONE;
}

/*
==============
BG_FindHumanNameForUpgrade
==============
*/
char *BG_FindHumanNameForUpgrade( int upgrade )
{
	int i;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		if ( bg_upgrades[ i ].upgradeNum == upgrade )
		{
			return bg_upgrades[ i ].upgradeHumanName;
		}
	}

	//wimp out
	return 0;
}

/*
==============
BG_FindIconForUpgrade
==============
*/
char *BG_FindIconForUpgrade( int upgrade )
{
	int i;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		if ( bg_upgrades[ i ].upgradeNum == upgrade )
		{
			return bg_upgrades[ i ].icon;
		}
	}

	//wimp out
	return 0;
}

/*
==============
BG_FindPurchasableForUpgrade
==============
*/
qboolean BG_FindPurchasableForUpgrade( int upgrade )
{
	int i;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		if ( bg_upgrades[ i ].upgradeNum == upgrade )
		{
			return bg_upgrades[ i ].purchasable;
		}
	}

	return qfalse;
}

/*
==============
BG_FindUsableForUpgrade
==============
*/
qboolean BG_FindUsableForUpgrade( int upgrade )
{
	int i;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		if ( bg_upgrades[ i ].upgradeNum == upgrade )
		{
			return bg_upgrades[ i ].usable;
		}
	}

	return qfalse;
}

/*
==============
BG_FindTeamForUpgrade
==============
*/
WUTeam_t BG_FindTeamForUpgrade( int upgrade )
{
	int i;

	for ( i = 0; i < bg_numUpgrades; i++ )
	{
		if ( bg_upgrades[ i ].upgradeNum == upgrade )
		{
			return bg_upgrades[ i ].team;
		}
	}

	return WUT_NONE;
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
			deltaTime = ( atTime - tr->trTime ) / ( float )tr->trDuration;
			phase     = sin( deltaTime * M_PI * 2 );
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
			deltaTime    = ( atTime - tr->trTime ) * 0.001;               // milliseconds to seconds
			VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
			result[ 2 ] -= 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime; // FIXME: local gravity...
			break;

		case TR_BUOYANCY:
			deltaTime    = ( atTime - tr->trTime ) * 0.001;               // milliseconds to seconds
			VectorMA( tr->trBase, deltaTime, tr->trDelta, result );
			result[ 2 ] += 0.5 * DEFAULT_GRAVITY * deltaTime * deltaTime; // FIXME: local gravity...
			break;

		default:
			Com_Error( ERR_DROP, "BG_EvaluateTrajectory: unknown trType: %i", tr->trTime );
			break;
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
			deltaTime = ( atTime - tr->trTime ) / ( float )tr->trDuration;
			phase     = cos( deltaTime * M_PI * 2 ); // derivative of sin = cos
			phase    *= 0.5;
			VectorScale( tr->trDelta, phase, result );
			break;

		case TR_LINEAR_STOP:
			if ( atTime > tr->trTime + tr->trDuration )
			{
				VectorClear( result );
				return;
			}

			VectorCopy( tr->trDelta, result );
			break;

		case TR_GRAVITY:
			deltaTime    = ( atTime - tr->trTime ) * 0.001; // milliseconds to seconds
			VectorCopy( tr->trDelta, result );
			result[ 2 ] -= DEFAULT_GRAVITY * deltaTime;     // FIXME: local gravity...
			break;

		case TR_BUOYANCY:
			deltaTime    = ( atTime - tr->trTime ) * 0.001; // milliseconds to seconds
			VectorCopy( tr->trDelta, result );
			result[ 2 ] += DEFAULT_GRAVITY * deltaTime;     // FIXME: local gravity...
			break;

		default:
			Com_Error( ERR_DROP, "BG_EvaluateTrajectoryDelta: unknown trType: %i", tr->trTime );
			break;
	}
}

char *eventnames[] =
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
	"EV_WATER_TOUCH",              // foot touches
	"EV_WATER_LEAVE",              // foot leaves
	"EV_WATER_UNDER",              // head touches
	"EV_WATER_CLEAR",              // head leaves

	"EV_NOAMMO",
	"EV_CHANGE_WEAPON",
	"EV_FIRE_WEAPON",
	"EV_FIRE_WEAPON2",
	"EV_FIRE_WEAPON3",

	"EV_PLAYER_RESPAWN",           //TA: for fovwarp effects
	"EV_PLAYER_TELEPORT_IN",
	"EV_PLAYER_TELEPORT_OUT",

	"EV_GRENADE_BOUNCE",           // eventParm will be the soundindex

	"EV_GENERAL_SOUND",
	"EV_GLOBAL_SOUND",             // no attenuation

	"EV_BULLET_HIT_FLESH",
	"EV_BULLET_HIT_WALL",

	"EV_SHOTGUN",

	"EV_MISSILE_HIT",
	"EV_MISSILE_MISS",
	"EV_MISSILE_MISS_METAL",
	"EV_TESLATRAIL",
	"EV_BULLET",                   // otherEntity is the shooter

	"EV_LEV1_GRAB",
	"EV_LEV4_CHARGE_PREPARE",
	"EV_LEV4_CHARGE_START",

	"EV_PAIN",
	"EV_DEATH1",
	"EV_DEATH2",
	"EV_DEATH3",
	"EV_OBITUARY",

	"EV_GIB_PLAYER",               // gib a previously living player

	"EV_BUILD_CONSTRUCT",          //TA
	"EV_BUILD_DESTROY",            //TA
	"EV_BUILD_DELAY",              //TA: can't build yet
	"EV_BUILD_REPAIR",             //TA: repairing buildable
	"EV_BUILD_REPAIRED",           //TA: buildable has full health
	"EV_HUMAN_BUILDABLE_EXPLOSION",
	"EV_ALIEN_BUILDABLE_EXPLOSION",
	"EV_ALIEN_ACIDTUBE",

	"EV_MEDKIT_USED",

	"EV_ALIEN_EVOLVE",
	"EV_ALIEN_EVOLVE_FAILED",

	"EV_DEBUG_LINE",
	"EV_STOPLOOPINGSOUND",
	"EV_TAUNT",

	"EV_OVERMIND_ATTACK", //TA: overmind under attack
	"EV_OVERMIND_DYING",  //TA: overmind close to death
	"EV_OVERMIND_SPAWNS", //TA: overmind needs spawns

	"EV_DCC_ATTACK",      //TA: dcc under attack

	"EV_RPTUSE_SOUND"     //TA: trigger a sound
};

/*
===============
BG_AddPredictableEventToPlayerstate

Handles the sequence numbers
===============
*/

void trap_Cvar_VariableStringBuffer( const char *var_name, char *buffer, int bufsize );

void BG_AddPredictableEventToPlayerstate( int newEvent, int eventParm, playerState_t *ps )
{
#ifdef _DEBUG
	{
		char buf[ 256 ];
		trap_Cvar_VariableStringBuffer( "showevents", buf, sizeof( buf ) );

		if ( atof( buf ) != 0 )
		{
#ifdef QAGAME
			Com_Printf( " game event svt %5d -> %5d: num = %20s parm %d\n",
			ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[ newEvent ], eventParm );
#else
			Com_Printf( "Cgame event svt %5d -> %5d: num = %20s parm %d\n",
			ps->pmove_framecount /*ps->commandTime*/, ps->eventSequence, eventnames[ newEvent ], eventParm );
#endif
		}
	}
#endif
	ps->events[ ps->eventSequence & ( MAX_EVENTS - 1 ) ]     = newEvent;
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
	else if ( ps->persistant[ PERS_TEAM ] == TEAM_SPECTATOR )
	{
		s->eType = ET_INVISIBLE;
	}
	else
	{
		s->eType = ET_PLAYER;
	}

	s->number     = ps->clientNum;

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

	//TA: i need for other things :)
	//s->angles2[YAW] = ps->movementDir;
	s->time2     = ps->movementDir;
	s->legsAnim  = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum; // ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	s->eFlags    = ps->eFlags;

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
		s->event     = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}
	else if ( ps->entityEventSequence < ps->eventSequence )
	{
		int seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_EVENTS )
		{
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}

		seq          = ps->entityEventSequence & ( MAX_EVENTS - 1 );
		s->event     = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon          = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	//store items held and active items in otherEntityNum
	s->modelindex      = 0;
	s->modelindex2     = 0;

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

	//TA: use powerups field to store team/class info:
	s->powerups = ps->stats[ STAT_PTEAM ] | ( ps->stats[ STAT_PCLASS ] << 8 );

	//TA: have to get the surfNormal thru somehow...
	VectorCopy( ps->grapplePoint, s->angles2 );

	if ( ps->stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING )
	{
		s->eFlags |= EF_WALLCLIMBCEILING;
	}

	s->loopSound = ps->loopSound;
	s->generic1  = ps->generic1;

	if ( s->generic1 <= WPM_NONE || s->generic1 >= WPM_NUM_WEAPONMODES )
	{
		s->generic1 = WPM_PRIMARY;
	}
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
	else if ( ps->persistant[ PERS_TEAM ] == TEAM_SPECTATOR )
	{
		s->eType = ET_INVISIBLE;
	}
	else
	{
		s->eType = ET_PLAYER;
	}

	s->number     = ps->clientNum;

	s->pos.trType = TR_LINEAR_STOP;
	VectorCopy( ps->origin, s->pos.trBase );

	if ( snap )
	{
		SnapVector( s->pos.trBase );
	}

	// set the trDelta for flag direction and linear prediction
	VectorCopy( ps->velocity, s->pos.trDelta );
	// set the time for linear prediction
	s->pos.trTime     = time;
	// set maximum extra polation time
	s->pos.trDuration = 50; // 1000 / sv_fps (default = 20)

	s->apos.trType    = TR_INTERPOLATE;
	VectorCopy( ps->viewangles, s->apos.trBase );

	if ( snap )
	{
		SnapVector( s->apos.trBase );
	}

	//TA: i need for other things :)
	//s->angles2[YAW] = ps->movementDir;
	s->time2     = ps->movementDir;
	s->legsAnim  = ps->legsAnim;
	s->torsoAnim = ps->torsoAnim;
	s->clientNum = ps->clientNum; // ET_PLAYER looks here instead of at number
	// so corpses can also reference the proper config
	s->eFlags    = ps->eFlags;

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
		s->event     = ps->externalEvent;
		s->eventParm = ps->externalEventParm;
	}
	else if ( ps->entityEventSequence < ps->eventSequence )
	{
		int seq;

		if ( ps->entityEventSequence < ps->eventSequence - MAX_EVENTS )
		{
			ps->entityEventSequence = ps->eventSequence - MAX_EVENTS;
		}

		seq          = ps->entityEventSequence & ( MAX_EVENTS - 1 );
		s->event     = ps->events[ seq ] | ( ( ps->entityEventSequence & 3 ) << 8 );
		s->eventParm = ps->eventParms[ seq ];
		ps->entityEventSequence++;
	}

	s->weapon          = ps->weapon;
	s->groundEntityNum = ps->groundEntityNum;

	//store items held and active items in otherEntityNum
	s->modelindex      = 0;
	s->modelindex2     = 0;

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

	//TA: use powerups field to store team/class info:
	s->powerups = ps->stats[ STAT_PTEAM ] | ( ps->stats[ STAT_PCLASS ] << 8 );

	//TA: have to get the surfNormal thru somehow...
	VectorCopy( ps->grapplePoint, s->angles2 );

	if ( ps->stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING )
	{
		s->eFlags |= EF_WALLCLIMBCEILING;
	}

	s->loopSound = ps->loopSound;
	s->generic1  = ps->generic1;

	if ( s->generic1 <= WPM_NONE || s->generic1 >= WPM_NUM_WEAPONMODES )
	{
		s->generic1 = WPM_PRIMARY;
	}
}

/*
========================
BG_UnpackAmmoArray

Extract the ammo quantity from the array
========================
*/
void BG_UnpackAmmoArray( int weapon, int psAmmo[], int psAmmo2[], int *ammo, int *clips )
{
	int ammoarray[ 32 ];
	int i;

	for ( i = 0; i <= 15; i++ )
	{
		ammoarray[ i ] = psAmmo[ i ];
	}

	for ( i = 16; i <= 31; i++ )
	{
		ammoarray[ i ] = psAmmo2[ i - 16 ];
	}

	if ( ammo != NULL )
	{
		*ammo = ammoarray[ weapon ] & 0x0FFF;
	}

	if ( clips != NULL )
	{
		*clips = ( ammoarray[ weapon ] >> 12 ) & 0x0F;
	}
}

/*
========================
BG_PackAmmoArray

Pack the ammo quantity into the array
========================
*/
void BG_PackAmmoArray( int weapon, int psAmmo[], int psAmmo2[], int ammo, int clips )
{
	int weaponvalue;

	weaponvalue = ammo | ( clips << 12 );

	if ( weapon <= 15 )
	{
		psAmmo[ weapon ] = weaponvalue;
	}
	else if ( weapon >= 16 )
	{
		psAmmo2[ weapon - 16 ] = weaponvalue;
	}
}

/*
========================
BG_WeaponIsFull

Check if a weapon has full ammo
========================
*/
qboolean BG_WeaponIsFull( weapon_t weapon, int stats[], int psAmmo[], int psAmmo2[]  )
{
	int maxAmmo, maxClips;
	int ammo, clips;

	BG_FindAmmoForWeapon( weapon, &maxAmmo, &maxClips );
	BG_UnpackAmmoArray( weapon, psAmmo, psAmmo2, &ammo, &clips );

	if ( BG_InventoryContainsUpgrade( UP_BATTPACK, stats ) )
	{
		maxAmmo = ( int )( ( float )maxAmmo * BATTPACK_MODIFIER );
	}

	return ( maxAmmo == ammo ) && ( maxClips == clips );
}

/*
========================
BG_AddWeaponToInventory

Give a player a weapon
========================
*/
void BG_AddWeaponToInventory( int weapon, int stats[]  )
{
	int weaponList;

	weaponList             = ( stats[ STAT_WEAPONS ] & 0x0000FFFF ) | ( ( stats[ STAT_WEAPONS2 ] << 16 ) & 0xFFFF0000 );

	weaponList            |= ( 1 << weapon );

	stats[ STAT_WEAPONS ]  = weaponList & 0x0000FFFF;
	stats[ STAT_WEAPONS2 ] = ( weaponList & 0xFFFF0000 ) >> 16;

	if ( stats[ STAT_SLOTS ] & BG_FindSlotsForWeapon( weapon ) )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: Held items conflict with weapon %d\n", weapon );
	}

	stats[ STAT_SLOTS ] |= BG_FindSlotsForWeapon( weapon );
}

/*
========================
BG_RemoveWeaponToInventory

Take a weapon from a player
========================
*/
void BG_RemoveWeaponFromInventory( int weapon, int stats[]  )
{
	int weaponList;

	weaponList             = ( stats[ STAT_WEAPONS ] & 0x0000FFFF ) | ( ( stats[ STAT_WEAPONS2 ] << 16 ) & 0xFFFF0000 );

	weaponList            &= ~( 1 << weapon );

	stats[ STAT_WEAPONS ]  = weaponList & 0x0000FFFF;
	stats[ STAT_WEAPONS2 ] = ( weaponList & 0xFFFF0000 ) >> 16;

	stats[ STAT_SLOTS ]   &= ~BG_FindSlotsForWeapon( weapon );
}

/*
========================
BG_InventoryContainsWeapon

Does the player hold a weapon?
========================
*/
qboolean BG_InventoryContainsWeapon( int weapon, int stats[]  )
{
	int weaponList;

	weaponList = ( stats[ STAT_WEAPONS ] & 0x0000FFFF ) | ( ( stats[ STAT_WEAPONS2 ] << 16 ) & 0xFFFF0000 );

	return( weaponList & ( 1 << weapon ) );
}

/*
========================
BG_AddUpgradeToInventory

Give the player an upgrade
========================
*/
void BG_AddUpgradeToInventory( int item, int stats[]  )
{
	stats[ STAT_ITEMS ] |= ( 1 << item );

	if ( stats[ STAT_SLOTS ] & BG_FindSlotsForUpgrade( item ) )
	{
		Com_Printf( S_COLOR_YELLOW "WARNING: Held items conflict with upgrade %d\n", item );
	}

	stats[ STAT_SLOTS ] |= BG_FindSlotsForUpgrade( item );
}

/*
========================
BG_RemoveUpgradeFromInventory

Take an upgrade from the player
========================
*/
void BG_RemoveUpgradeFromInventory( int item, int stats[]  )
{
	stats[ STAT_ITEMS ] &= ~( 1 << item );

	stats[ STAT_SLOTS ] &= ~BG_FindSlotsForUpgrade( item );
}

/*
========================
BG_InventoryContainsUpgrade

Does the player hold an upgrade?
========================
*/
qboolean BG_InventoryContainsUpgrade( int item, int stats[]  )
{
	return( stats[ STAT_ITEMS ] & ( 1 << item ) );
}

/*
========================
BG_ActivateUpgrade

Activates an upgrade
========================
*/
void BG_ActivateUpgrade( int item, int stats[]  )
{
	stats[ STAT_ACTIVEITEMS ] |= ( 1 << item );
}

/*
========================
BG_DeactivateUpgrade

Deactivates an upgrade
========================
*/
void BG_DeactivateUpgrade( int item, int stats[]  )
{
	stats[ STAT_ACTIVEITEMS ] &= ~( 1 << item );
}

/*
========================
BG_UpgradeIsActive

Is this upgrade active?
========================
*/
qboolean BG_UpgradeIsActive( int item, int stats[]  )
{
	return( stats[ STAT_ACTIVEITEMS ] & ( 1 << item ) );
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
	vec3_t refNormal     = { 0.0f, 0.0f, 1.0f };
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
BG_PositionBuildableRelativeToPlayer

Find a place to build a buildable
===============
*/
void BG_PositionBuildableRelativeToPlayer( const playerState_t *ps,
    const vec3_t mins, const vec3_t maxs,
    void ( *trace )( trace_t *, const vec3_t, const vec3_t,
                     const vec3_t, const vec3_t, int, int ),
    vec3_t outOrigin, vec3_t outAngles, trace_t *tr )
{
	vec3_t forward, entityOrigin, targetOrigin;
	vec3_t angles, playerOrigin, playerNormal;
	float  buildDist;

	if ( ps->stats[ STAT_STATE ] & SS_WALLCLIMBING )
	{
		if ( ps->stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING )
		{
			VectorSet( playerNormal, 0.0f, 0.0f, -1.0f );
		}
		else
		{
			VectorCopy( ps->grapplePoint, playerNormal );
		}
	}
	else
	{
		VectorSet( playerNormal, 0.0f, 0.0f, 1.0f );
	}

	VectorCopy( ps->viewangles, angles );
	VectorCopy( ps->origin, playerOrigin );
	buildDist = BG_FindBuildDistForClass( ps->stats[ STAT_PCLASS ] );

	AngleVectors( angles, forward, NULL, NULL );
	ProjectPointOnPlane( forward, forward, playerNormal );
	VectorNormalize( forward );

	VectorMA( playerOrigin, buildDist, forward, entityOrigin );

	VectorCopy( entityOrigin, targetOrigin );

	//so buildings can be placed facing slopes
	VectorMA( entityOrigin, 32, playerNormal, entityOrigin );

	//so buildings drop to floor
	VectorMA( targetOrigin, -128, playerNormal, targetOrigin );

	( *trace )( tr, entityOrigin, mins, maxs, targetOrigin, ps->clientNum, MASK_PLAYERSOLID );
	VectorCopy( tr->endpos, entityOrigin );
	VectorMA( entityOrigin, 0.1f, playerNormal, outOrigin );
	vectoangles( forward, outAngles );
}

/*
===============
BG_GetValueOfHuman

Returns the kills value of some human player
===============
*/
int BG_GetValueOfHuman( playerState_t *ps )
{
	int   i, worth = 0;
	float portion;

	for ( i = UP_NONE + 1; i < UP_NUM_UPGRADES; i++ )
	{
		if ( BG_InventoryContainsUpgrade( i, ps->stats ) )
		{
			worth += BG_FindPriceForUpgrade( i );
		}
	}

	for ( i = WP_NONE + 1; i < WP_NUM_WEAPONS; i++ )
	{
		if ( BG_InventoryContainsWeapon( i, ps->stats ) )
		{
			worth += BG_FindPriceForWeapon( i );
		}
	}

	portion = worth / ( float )HUMAN_MAXED;

	if ( portion < 0.01f )
	{
		portion = 0.01f;
	}
	else if ( portion > 1.0f )
	{
		portion = 1.0f;
	}

	return ceil( ALIEN_MAX_SINGLE_KILLS * portion );
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

/*
===============
BG_ParseCSVEquipmentList
===============
*/
void BG_ParseCSVEquipmentList( const char *string, weapon_t *weapons, int weaponsSize,
                               upgrade_t *upgrades, int upgradesSize )
{
	char     buffer[ MAX_STRING_CHARS ];
	int      i   = 0, j = 0;
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
			weapons[ i ] = BG_FindWeaponNumForName( q );
		}

		if ( upgradesSize )
		{
			upgrades[ j ] = BG_FindUpgradeNumForName( q );
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
void BG_ParseCSVClassList( const char *string, pClass_t *classes, int classesSize )
{
	char     buffer[ MAX_STRING_CHARS ];
	int      i   = 0;
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

		classes[ i ] = BG_FindClassNumForName( q );

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
	int      i   = 0;
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

		buildables[ i ] = BG_FindClassNumForName( q );

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

/*
============
BG_UpgradeClassAvailable
============
*/
qboolean BG_UpgradeClassAvailable( playerState_t *ps )
{
	int     i;
	char    buffer[ MAX_STRING_CHARS ];
	stage_t currentStage;

	trap_Cvar_VariableStringBuffer( "g_alienStage", buffer, MAX_STRING_CHARS );
	currentStage = atoi( buffer );

	for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
	{
		if ( BG_ClassCanEvolveFromTo( ps->stats[ STAT_PCLASS ], i,
		                              ps->persistant[ PERS_CREDIT ], 0 ) >= 0 &&
		     BG_FindStagesForClass( i, currentStage ) &&
		     BG_ClassIsAllowed( i ) )
		{
			return qtrue;
		}
	}

	return qfalse;
}

typedef struct gameElements_s
{
	buildable_t buildables[ BA_NUM_BUILDABLES ];
	pClass_t    classes[ PCL_NUM_CLASSES ];
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
qboolean BG_ClassIsAllowed( pClass_t class )
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
