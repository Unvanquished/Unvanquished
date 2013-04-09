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

#ifndef UNVANQUISHED_H_
#define UNVANQUISHED_H_
//==================================================================

/*
 * ALIEN weapons
 *
 * _REPEAT  - time in msec until the weapon can be used again
 * _DMG     - amount of damage the weapon does
 *
 * ALIEN_WDMG_MODIFIER - overall damage modifier for coarse tuning
 *
 */
#define ALIEN_WDMG_MODIFIER           1.0f
#define ADM(d) ((int)((float)d * ALIEN_WDMG_MODIFIER ))

#define ABUILDER_CLAW_DMG             ADM(20)
#define ABUILDER_CLAW_RANGE           64.0f
#define ABUILDER_CLAW_WIDTH           4.0f
#define ABUILDER_BLOB_DMG             ADM(4)
#define ABUILDER_BLOB_SPEED           800.0f
#define ABUILDER_BLOB_SPEED_MOD       0.5f
#define ABUILDER_BLOB_TIME            2000

#define LEVEL0_BITE_DMG               ADM(36)
#define LEVEL0_BITE_RANGE             64.0f
#define LEVEL0_BITE_WIDTH             6.0f
#define LEVEL0_BITE_REPEAT            500

#define LEVEL1_CLAW_DMG               ADM(32)
#define LEVEL1_CLAW_RANGE             80.0f // Claw and grab range normalized. Not sure on this one, but it was pretty widely requested.
#define LEVEL1_CLAW_U_RANGE           LEVEL1_CLAW_RANGE + 3.0f
#define LEVEL1_CLAW_WIDTH             10.0f
#define LEVEL1_CLAW_K_SCALE           1.0f
#define LEVEL1_GRAB_RANGE             80.0f // Claw and grab range normalized.
#define LEVEL1_GRAB_U_RANGE           LEVEL1_GRAB_RANGE + 3.0f
#define LEVEL1_GRAB_TIME              300
#define LEVEL1_GRAB_U_TIME            300
#define LEVEL1_PCLOUD_DMG             ADM(4)
#define LEVEL1_PCLOUD_RANGE           120.0f
#define LEVEL1_PCLOUD_TIME            10000
#define LEVEL1_REGEN_MOD              2.0f
#define LEVEL1_UPG_REGEN_MOD          3.0f
#define LEVEL1_REGEN_SCOREINC         AVM(100) // score added for healing per 10s
#define LEVEL1_UPG_REGEN_SCOREINC     AVM(200)

#define LEVEL2_CLAW_DMG               ADM(40)
#define LEVEL2_CLAW_RANGE             80.0f
#define LEVEL2_CLAW_U_RANGE           LEVEL2_CLAW_RANGE + 2.0f
#define LEVEL2_CLAW_WIDTH             14.0f
#define LEVEL2_AREAZAP_DMG            ADM(60)
#define LEVEL2_AREAZAP_RANGE          200.0f
#define LEVEL2_AREAZAP_CHAIN_RANGE    150.0f
#define LEVEL2_AREAZAP_CHAIN_FALLOFF  8.0f
#define LEVEL2_AREAZAP_WIDTH          15.0f
#define LEVEL2_AREAZAP_TIME           1000
#define LEVEL2_AREAZAP_MAX_TARGETS    5
#define LEVEL2_WALLJUMP_MAXSPEED      1000.0f

#define LEVEL3_CLAW_DMG               ADM(80)
#define LEVEL3_CLAW_RANGE             80.0f // Increased claw range
#define LEVEL3_CLAW_UPG_RANGE         LEVEL3_CLAW_RANGE + 3.0f
#define LEVEL3_CLAW_WIDTH             11.0f //
#define LEVEL3_POUNCE_DMG             ADM(75) // Reduced damage. Pounce is very powerful as it is.
#define LEVEL3_POUNCE_RANGE           55.0f // Pounce range raised by 7.0. May need to be nerfed, still want to test like this though.
#define LEVEL3_POUNCE_UPG_RANGE       LEVEL3_POUNCE_RANGE + 3.0f
#define LEVEL3_POUNCE_WIDTH           12.0f // Pounce width narrowed. May scale this down even further.
#define LEVEL3_POUNCE_TIME            800 // Reduced this by 100ms, may need nerfing.
#define LEVEL3_POUNCE_TIME_UPG        800 // Evened with standard goon. I'd like to keep this here even if standard goon charge time is raised.
#define LEVEL3_POUNCE_TIME_MIN        200 // msec before which pounce cancels
#define LEVEL3_POUNCE_REPEAT          400 // msec before a new pounce starts
#define LEVEL3_POUNCE_SPEED_MOD       0.85f // Whales: Reduced the slowdown from a charged pounce by 0.10
#define LEVEL3_POUNCE_JUMP_MAG        750 // Raised by 50,
#define LEVEL3_POUNCE_JUMP_MAG_UPG    850 // Raised by 50.
#define LEVEL3_BOUNCEBALL_DMG         ADM(110)
#define LEVEL3_BOUNCEBALL_SPEED       1000.0f
#define LEVEL3_BOUNCEBALL_RADIUS      75
#define LEVEL3_BOUNCEBALL_REGEN       12500 // Reduced regen time.

#define LEVEL4_CLAW_DMG               ADM(100)
#define LEVEL4_CLAW_RANGE             100.0f
#define LEVEL4_CLAW_WIDTH             14.0f
#define LEVEL4_CLAW_HEIGHT            20.0f

#define LEVEL4_TRAMPLE_DMG            ADM(85) // 111 -> 85
#define LEVEL4_TRAMPLE_SPEED          2.0f
#define LEVEL4_TRAMPLE_CHARGE_MIN     375 // minimum msec to start a charge
#define LEVEL4_TRAMPLE_CHARGE_MAX     1000 // msec to maximum charge stored
#define LEVEL4_TRAMPLE_CHARGE_TRIGGER 3000 // msec charge starts on its own
#define LEVEL4_TRAMPLE_DURATION       3000 // msec trample lasts on full charge
#define LEVEL4_TRAMPLE_STOP_PENALTY   1 // charge lost per msec when stopped
#define LEVEL4_TRAMPLE_REPEAT         300 // Time until charge rehits a player Ishq: 100->300

#define LEVEL4_CRUSH_DAMAGE_PER_V     0.5f // damage per falling velocity
#define LEVEL4_CRUSH_DAMAGE           120 // to players only
#define LEVEL4_CRUSH_REPEAT           500 // player damage repeat

/*
 * ALIEN classes
 *
 * _SPEED   - fraction of Q3A run speed the class can move
 * _REGEN   - health per second regained
 *
 * ALIEN_HLTH_MODIFIER - overall health modifier for coarse tuning
 *
 */

#define ALIEN_HLTH_MODIFIER  1.0f
#define AHM(h) ((int)((float)h * ALIEN_HLTH_MODIFIER ))

#define ALIEN_VALUE_MODIFIER 1.0f
#define AVM(h) ((int)((float)h * ALIEN_VALUE_MODIFIER ))

#define ABUILDER_SPEED       0.9f
#define ABUILDER_VALUE       AVM(240)
#define ABUILDER_HEALTH      AHM(50)
#define ABUILDER_REGEN       ( 0.04f * ABUILDER_HEALTH )
#define ABUILDER_COST        0

#define ABUILDER_UPG_SPEED   0.9f
#define ABUILDER_UPG_VALUE   AVM(300)
#define ABUILDER_UPG_HEALTH  AHM(75)
#define ABUILDER_UPG_REGEN   ( 0.04f * ABUILDER_UPG_HEALTH )
#define ABUILDER_UPG_COST    0

#define LEVEL0_SPEED         1.4f
#define LEVEL0_VALUE         AVM(180)
#define LEVEL0_HEALTH        AHM(25)
#define LEVEL0_REGEN         ( 0.05f * LEVEL0_HEALTH )
#define LEVEL0_COST          0

#define LEVEL1_SPEED         1.25f
#define LEVEL1_VALUE         AVM(270)
#define LEVEL1_HEALTH        AHM(60)
#define LEVEL1_REGEN         ( 0.03f * LEVEL1_HEALTH )
#define LEVEL1_COST          1

#define LEVEL1_UPG_SPEED     1.25f
#define LEVEL1_UPG_VALUE     AVM(330)
#define LEVEL1_UPG_HEALTH    AHM(80)
#define LEVEL1_UPG_REGEN     ( 0.03f * LEVEL1_UPG_HEALTH )
#define LEVEL1_UPG_COST      1

#define LEVEL2_SPEED         1.2f
#define LEVEL2_VALUE         AVM(420)
#define LEVEL2_HEALTH        AHM(150)
#define LEVEL2_REGEN         ( 0.03f * LEVEL2_HEALTH )
#define LEVEL2_COST          1

#define LEVEL2_UPG_SPEED     1.2f
#define LEVEL2_UPG_VALUE     AVM(540)
#define LEVEL2_UPG_HEALTH    AHM(175)
#define LEVEL2_UPG_REGEN     ( 0.03f * LEVEL2_UPG_HEALTH )
#define LEVEL2_UPG_COST      1

#define LEVEL3_SPEED         1.2f // Raised goon speed by .1 to match backpedalling humans. May need nerf.
#define LEVEL3_VALUE         AVM(600)
#define LEVEL3_HEALTH        AHM(200)
#define LEVEL3_REGEN         ( 0.03f * LEVEL3_HEALTH )
#define LEVEL3_COST          1

#define LEVEL3_UPG_SPEED     1.2f // Raised by .1 to match standard goon.
#define LEVEL3_UPG_VALUE     AVM(720)
#define LEVEL3_UPG_HEALTH    AHM(250)
#define LEVEL3_UPG_REGEN     ( 0.03f * LEVEL3_UPG_HEALTH )
#define LEVEL3_UPG_COST      1

#define LEVEL4_SPEED         1.1f // Lowered by .1
#define LEVEL4_VALUE         AVM(960)
#define LEVEL4_HEALTH        AHM(350)
#define LEVEL4_REGEN         ( 0.025f * LEVEL4_HEALTH )
#define LEVEL4_COST          2

/*
 * ALIEN buildables
 *
 * CREEP_BASESIZE - the maximum distance a buildable can be from an egg/overmind
 *
 */

#define CREEP_BASESIZE          700
#define CREEP_TIMEOUT           1000
#define CREEP_MODIFIER          0.5f
#define CREEP_ARMOUR_MODIFIER   0.75f
#define CREEP_SCALEDOWN_TIME    3000

#define PCLOUD_MODIFIER         0.5f
#define PCLOUD_ARMOUR_MODIFIER  0.75f

#define BARRICADE_SHRINKPROP    0.25f
#define BARRICADE_SHRINKTIMEOUT 500

#define BOOSTER_REGEN_MOD       3.0f
#define BOOST_TIME              20000
#define BOOST_WARN_TIME         15000

#define ACIDTUBE_DAMAGE         8
#define ACIDTUBE_RANGE          300.0f
#define ACIDTUBE_REPEAT         300
#define ACIDTUBE_REPEAT_ANIM    2000

#define TRAPPER_RANGE           400

#define HIVE_SENSE_RANGE        500.0f
#define HIVE_LIFETIME           3000
#define HIVE_REPEAT             3000
#define HIVE_DMG                80
#define HIVE_SPEED              320.0f
#define HIVE_DIR_CHANGE_PERIOD  500

#define LEECH_LIFETIME           3000
#define LEECH_REPEAT             3000
#define LEECH_K_SCALE            1.0f
#define LEECH_DMG                80
#define LEECH_SPEED              320.0f
#define LEECH_DIR_CHANGE_PERIOD  500

#define LOCKBLOB_SPEED          500.0f
#define LOCKBLOB_LOCKTIME       5000
#define LOCKBLOB_DOT            0.85f // max angle = acos( LOCKBLOB_DOT )

#define OVERMIND_HEALTH         750
#define OVERMIND_ATTACK_RANGE   150.0f

/*
 * ALIEN misc
 *
 * ALIENSENSE_RANGE - the distance alien sense is useful for
 *
 */

#define ALIENSENSE_RANGE         1000.0f
#define REGEN_BOOST_RANGE        200.0f

#define ALIEN_POISON_TIME        10000
#define ALIEN_POISON_DMG         5
#define ALIEN_POISON_DIVIDER     ( 1.0f / 1.32f ) //about 1.0/((time)th root of damage)

#define ALIEN_SPAWN_REPEAT_TIME  10000

#define ALIEN_REGEN_DAMAGE_TIME  2000 //msec since damage that regen starts again
#define ALIEN_REGEN_NOCREEP_MOD  ( 1.0f / 3.0f ) //regen off creep

#define ALIEN_MAX_FRAGS          9
#define ALIEN_MAX_CREDITS        ( ALIEN_MAX_FRAGS * ALIEN_CREDITS_PER_KILL )
#define ALIEN_CREDITS_PER_KILL   400
#define ALIEN_TK_SUICIDE_PENALTY 350

/*
 * HUMAN weapons
 *
 * _REPEAT  - time between firings
 * _RELOAD  - time needed to reload
 * _PRICE   - amount in credits weapon costs
 *
 * HUMAN_WDMG_MODIFIER - overall damage modifier for coarse tuning
 *
 */

#define HUMAN_WDMG_MODIFIER      1.0f
#define HDM(d) ((int)((float)d * HUMAN_WDMG_MODIFIER ))

#define BLASTER_SPREAD           200
#define BLASTER_SPEED            1400
#define BLASTER_DMG              HDM(10)
#define BLASTER_SIZE             5

#define RIFLE_SPREAD             250 // Raised by 50.
#define RIFLE_DMG                HDM(5)

#define PAINSAW_DAMAGE           HDM(11)
#define PAINSAW_RANGE            64.0f
#define PAINSAW_WIDTH            0.0f
#define PAINSAW_HEIGHT           8.0f

#define GRENADE_DAMAGE           HDM(310)
#define GRENADE_RANGE            192.0f
#define GRENADE_SPEED            400.0f

#define SHOTGUN_PELLETS          11 //used to sync server and client side
#define SHOTGUN_SPREAD           790
#define SHOTGUN_DMG              HDM(5)
#define SHOTGUN_RANGE            ( 8192 * 12 )

#define LASGUN_REPEAT            200
#define LASGUN_K_SCALE           1.0f
#define LASGUN_DAMAGE            HDM(9)

#define MDRIVER_DMG              HDM(40)
#define MDRIVER_REPEAT           1000
#define MDRIVER_K_SCALE          1.0f

#define CHAINGUN_SPREAD          900
#define CHAINGUN_DMG             HDM(6)

#define FLAMER_DMG               HDM(14) // 20->15->14
#define FLAMER_FLIGHTSPLASHDAMAGE HDM(1)
#define FLAMER_SPLASHDAMAGE      HDM(6) // 10->7->6
#define FLAMER_RADIUS            25 //  Radius lowered by 25
#define FLAMER_SIZE              15 // missile bounding box
#define FLAMER_LIFETIME          750.0f // Raised by 50.
#define FLAMER_SPEED             500.0f
#define FLAMER_LAG               0.65f // the amount of player velocity that is added to the fireball

#define PRIFLE_DMG               HDM(9)
#define PRIFLE_SPEED             1200
#define PRIFLE_SIZE              5

#define LCANNON_DAMAGE           HDM(265)
#define LCANNON_RADIUS           150 // primary splash damage radius
#define LCANNON_SIZE             5 // missile bounding box radius
#define LCANNON_SECONDARY_DAMAGE HDM(30)
#define LCANNON_SECONDARY_RADIUS 75 // secondary splash damage radius
#define LCANNON_SECONDARY_SPEED  1400
#define LCANNON_SECONDARY_RELOAD 2000
#define LCANNON_SPEED            700
#define LCANNON_CHARGE_TIME_MAX  3000
#define LCANNON_CHARGE_TIME_MIN  100
#define LCANNON_CHARGE_TIME_WARN 2000
#define LCANNON_CHARGE_AMMO      10 // ammo cost of a full charge shot

#define HBUILD_HEALRATE          18

/*
 * HUMAN upgrades
 */

#define LIGHTARMOUR_POISON_PROTECTION 1
#define LIGHTARMOUR_PCLOUD_PROTECTION 1000

#define HELMET_RANGE                  1000.0f
#define HELMET_POISON_PROTECTION      1
#define HELMET_PCLOUD_PROTECTION      1000

#define BATTPACK_PRICE                100
#define BATTPACK_MODIFIER             1.5f //modifier for extra energy storage available

#define JETPACK_FLOAT_SPEED           128.0f //up movement speed
#define JETPACK_SINK_SPEED            192.0f //down movement speed
#define JETPACK_DISABLE_TIME          1000 //time to disable the jetpack when player damaged
#define JETPACK_DISABLE_CHANCE        0.3f

#define BSUIT_PRICE                   400
#define BSUIT_POISON_PROTECTION       3
#define BSUIT_PCLOUD_PROTECTION       3000

#define MEDKIT_POISON_IMMUNITY_TIME   2000 // Added two second poison immunity from medkit.
#define MEDKIT_STARTUP_TIME           4000
#define MEDKIT_STARTUP_SPEED          5

/*
 * HUMAN buildables
 *
 * REACTOR_BASESIZE - the maximum distance a buildable can be from a reactor
 * REPEATER_BASESIZE - the maximum distance a buildable can be from a repeater
 *
 */

#define REACTOR_BASESIZE          1000
#define REPEATER_BASESIZE         500
#define HUMAN_DETONATION_DELAY    5000

#define MGTURRET_RANGE            400.0
#define MGTURRET_REPEAT           150.0
#define MGTURRET_ANGULARSPEED     12
#define MGTURRET_ACCURACY_TO_FIRE 0
#define MGTURRET_VERTICALCAP      30 // +/- maximum pitch
#define MGTURRET_SPREAD           200
#define MGTURRET_DMG              HDM(8)
#define MGTURRET_SPINUP_TIME      750 // time between target sighted and fire

#define TESLAGEN_RANGE            250.0
#define TESLAGEN_REPEAT           200.0
#define TESLAGEN_DMG              HDM(10)

#define DC_ATTACK_PERIOD          10000 // how often to spam "under attack"
#define DC_HEALRATE               4
#define DC_RANGE                  1000

#define REACTOR_ATTACK_RANGE      100.0f
#define REACTOR_ATTACK_REPEAT     1000
#define REACTOR_ATTACK_DAMAGE     40
#define REACTOR_ATTACK_DCC_REPEAT 1000
#define REACTOR_ATTACK_DCC_RANGE  150.0f
#define REACTOR_ATTACK_DCC_DAMAGE 40

/*
 * HUMAN misc
 */

#define HUMAN_SPRINT_MODIFIER         1.2f
#define HUMAN_JOG_MODIFIER            1.0f
#define HUMAN_BACK_MODIFIER           0.8f
#define HUMAN_SIDE_MODIFIER           0.9f
#define HUMAN_DODGE_SIDE_MODIFIER     2.9f
#define HUMAN_DODGE_SLOWED_MODIFIER   0.9f
#define HUMAN_DODGE_UP_MODIFIER       0.5f
#define HUMAN_DODGE_TIMEOUT           2500 // Reduced dodge cooldown.
#define HUMAN_LAND_FRICTION           3.0f

#define STAMINA_STOP_RESTORE          30
#define STAMINA_WALK_RESTORE          15
#define STAMINA_MEDISTAT_RESTORE      30 // stacked on STOP or WALK
#define STAMINA_SPRINT_TAKE           6
#define STAMINA_JUMP_TAKE             250 // Doubled jump requirement. Can perform ~8 jumps.
#define STAMINA_DODGE_TAKE            750 // Tripled dodge stamina requirement.
#define STAMINA_MAX                   2000 // Doubled maximum stamina.
#define STAMINA_BREATHING_LEVEL       0
#define STAMINA_SLOW_LEVEL            -1000 // doubled to match doubled stamina
#define STAMINA_BLACKOUT_LEVEL        -1600 // Doubled to match doubled stamina

#define HUMAN_SPAWN_REPEAT_TIME       10000
#define HUMAN_REGEN_DAMAGE_TIME       2000 //msec since damage before dcc repairs

#define HUMAN_MAX_CREDITS             2000
#define HUMAN_TK_SUICIDE_PENALTY      150

#define HUMAN_BUILDER_SCOREINC        50 // builders receive this many points every 10 seconds
#define ALIEN_BUILDER_SCOREINC        AVM(100) // builders receive this many points every 10 seconds

/*
 * Misc
 */

#define MIN_FALL_DISTANCE                  30.0f //the fall distance at which fall damage kicks in
#define MAX_FALL_DISTANCE                  120.0f //the fall distance at which maximum damage is dealt
#define AVG_FALL_DISTANCE                  (( MIN_FALL_DISTANCE + MAX_FALL_DISTANCE ) / 2.0f )

#define DEFAULT_FREEKILL_PERIOD            "120" //seconds
#define FREEKILL_ALIEN                     ALIEN_CREDITS_PER_KILL
#define FREEKILL_HUMAN                     LEVEL0_VALUE

#define RGS_RANGE                          750.0f
#define DEFAULT_INITIAL_BUILD_POINTS       "50"
#define DEFAULT_INITIAL_MINE_RATE          "10"
#define DEFAULT_MINE_RATE_HALF_LIFE        "15"

#define DEFAULT_CONFIDENCE_SUM_PERIOD      "10"

#define DEFAULT_ALIEN_STAGE1_BELOW         "50"
#define DEFAULT_ALIEN_STAGE2_ABOVE         "100"
#define DEFAULT_ALIEN_STAGE2_BELOW         "150"
#define DEFAULT_ALIEN_STAGE3_ABOVE         "200"
#define DEFAULT_ALIEN_MAX_STAGE            "2"

#define DEFAULT_HUMAN_STAGE1_BELOW         "50"
#define DEFAULT_HUMAN_STAGE2_ABOVE         "100"
#define DEFAULT_HUMAN_STAGE2_BELOW         "150"
#define DEFAULT_HUMAN_STAGE3_ABOVE         "200"
#define DEFAULT_HUMAN_MAX_STAGE            "2"

#define MAXIMUM_BUILD_TIME                 20000 // used for pie timer

#endif /* UNVANQUISHED_H_ */
