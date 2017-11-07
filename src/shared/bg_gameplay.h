/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

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

===========================================================================
*/

#ifndef G_GAMEPLAY_H_
#define G_GAMEPLAY_H_

/*
 * ALIEN weapons
 */

extern int   ABUILDER_CLAW_DMG;
extern float ABUILDER_CLAW_RANGE;
extern float ABUILDER_CLAW_WIDTH;
extern int   ABUILDER_BLOB_DMG;
extern float ABUILDER_BLOB_SPEED;
extern float ABUILDER_BLOB_SPEED_MOD;
extern int   ABUILDER_BLOB_TIME;
#define ABUILDER_BLOB_FIRE_IMMUNITY   3000   // in ms, friendly buildables gain immunity for fire on hit
#define ABUILDER_BLOB_FIRE_STOP_RANGE 20     // granger spit that hits a surface kills environmental fire in this range

extern int   LEVEL0_BITE_DMG;
extern float LEVEL0_BITE_RANGE;
extern float LEVEL0_BITE_WIDTH;
extern int   LEVEL0_BITE_REPEAT;

extern int   LEVEL1_CLAW_DMG;
extern float LEVEL1_CLAW_RANGE;
extern float LEVEL1_CLAW_U_RANGE;
extern float LEVEL1_CLAW_WIDTH;
#define LEVEL1_POUNCE_DISTANCE        300 // pitch between LEVEL1_POUNCE_MINPITCH and pi/4 results in this distance
#define LEVEL1_POUNCE_MINPITCH        M_PI / 12.0f // 15°, minimum pitch that will result in full pounce distance
#define LEVEL1_POUNCE_COOLDOWN        2000
#define LEVEL1_WALLPOUNCE_MAGNITUDE   600
#define LEVEL1_WALLPOUNCE_COOLDOWN    1200
#define LEVEL1_SIDEPOUNCE_MAGNITUDE   400
#define LEVEL1_SIDEPOUNCE_DIR_Z       0.4f // in ]0.0f,1.0f], fixed Z-coordinate of sidepounce
#define LEVEL1_SIDEPOUNCE_COOLDOWN    750
#define LEVEL1_SLOW_TIME              1000
#define LEVEL1_SLOW_MOD               0.75f

extern int   LEVEL2_CLAW_DMG;
extern float LEVEL2_CLAW_RANGE;
extern float LEVEL2_CLAW_U_RANGE;
extern float LEVEL2_CLAW_WIDTH;
extern int   LEVEL2_AREAZAP_DMG;
extern float LEVEL2_AREAZAP_RANGE;
extern float LEVEL2_AREAZAP_CHAIN_RANGE;
extern float LEVEL2_AREAZAP_CHAIN_FALLOFF;
extern float LEVEL2_AREAZAP_WIDTH;
extern int   LEVEL2_AREAZAP_TIME;
extern float LEVEL2_WALLJUMP_MAXSPEED;
#define LEVEL2_AREAZAP_MAX_TARGETS 5

extern int   LEVEL3_CLAW_DMG;
extern float LEVEL3_CLAW_RANGE;
extern float LEVEL3_CLAW_UPG_RANGE;
extern float LEVEL3_CLAW_WIDTH;
extern int   LEVEL3_POUNCE_DMG;
extern float LEVEL3_POUNCE_RANGE;
extern float LEVEL3_POUNCE_UPG_RANGE;
extern float LEVEL3_POUNCE_WIDTH;
extern int   LEVEL3_POUNCE_TIME;
extern int   LEVEL3_POUNCE_TIME_UPG;
extern int   LEVEL3_POUNCE_TIME_MIN;
extern int   LEVEL3_POUNCE_REPEAT;
extern float LEVEL3_POUNCE_SPEED_MOD;
extern int   LEVEL3_POUNCE_JUMP_MAG;
extern int   LEVEL3_POUNCE_JUMP_MAG_UPG;
extern int   LEVEL3_BOUNCEBALL_DMG;
extern float LEVEL3_BOUNCEBALL_SPEED;
extern int   LEVEL3_BOUNCEBALL_RADIUS;
extern int   LEVEL3_BOUNCEBALL_REGEN;
extern int   LEVEL3_BOUNCEBALL_REGEN_BOOSTER;
extern int   LEVEL3_BOUNCEBALL_REGEN_CREEP;

extern int   LEVEL4_CLAW_DMG;
extern float LEVEL4_CLAW_RANGE;
extern float LEVEL4_CLAW_WIDTH;
extern float LEVEL4_CLAW_HEIGHT;
extern int   LEVEL4_TRAMPLE_DMG;
extern float LEVEL4_TRAMPLE_SPEED;
extern int   LEVEL4_TRAMPLE_CHARGE_MIN;
extern int   LEVEL4_TRAMPLE_CHARGE_MAX;
extern int   LEVEL4_TRAMPLE_CHARGE_TRIGGER;
extern int   LEVEL4_TRAMPLE_DURATION;
extern int   LEVEL4_TRAMPLE_STOP_PENALTY;
extern int   LEVEL4_TRAMPLE_REPEAT;
extern float LEVEL4_CRUSH_DAMAGE_PER_V;
extern int   LEVEL4_CRUSH_DAMAGE;
extern int   LEVEL4_CRUSH_REPEAT;

/*
 * ALIEN buildables
 */

#define CREEP_BASESIZE          700
#define CREEP_TIMEOUT           1000
#define CREEP_MODIFIER          0.5f
#define CREEP_ARMOUR_MODIFIER   0.75f
#define CREEP_SCALEDOWN_TIME    3000

#define BARRICADE_SHRINKPROP    0.25f
#define BARRICADE_SHRINKTIMEOUT 500

#define BOOST_TIME              20000
#define BOOST_WARN_TIME         15000
#define BOOST_REPEAT_ANIM       2000

#define ACIDTUBE_RANGE          300.0f

#define SPIKER_SENSE_RANGE      250.0f // an enemy needs to be this close to consider an attack
#define SPIKER_SPIKE_RANGE      500.0f // this is just an estimate used for predicting damage
#define SPIKER_MISSILEROWS      4
#define SPIKER_MISSILES         26   // actual value +/- SPIKER_MISSILEROWS
#define SPIKER_ROWOFFSET        0.5f // 0.0: Spikes are shot upwards, 1.0: Spikes are shot sideways
#define SPIKER_COOLDOWN         5000

#define TRAPPER_RANGE           400

#define HIVE_SENSE_RANGE        500.0f
#define HIVE_LIFETIME           3000
#define HIVE_SPEED              320.0f
#define HIVE_DIR_CHANGE_PERIOD  500

#define LOCKBLOB_REPEAT         1000
#define LOCKBLOB_RANGE          400.0f
#define LOCKBLOB_SPEED          500.0f
#define LOCKBLOB_LOCKTIME       5000
#define LOCKBLOB_DOT            0.85f // max angle = acos( LOCKBLOB_DOT )

/*
 * ALIEN misc
 */

#define ALIENSENSE_RANGE         1500.0f
#define ALIENSENSE_BORDER_FRAC   0.2f // In this outer fraction of the range beacons are faded.

#define REGEN_BOOSTER_RANGE      200.0f
#define REGEN_TEAMMATE_RANGE     300.0f

#define ALIEN_POISON_TIME        10000
#define ALIEN_POISON_DMG         5
#define ALIEN_POISON_DIVIDER     ( 1.0f / 1.32f ) //about 1.0/((time)th root of damage)

#define ALIEN_SPAWN_REPEAT_TIME  10000

#define ALIEN_CLIENT_REGEN_WAIT    2000 // in ms
#define ALIEN_BUILDABLE_REGEN_WAIT 2000 // in ms

#define ALIEN_REGEN_NOCREEP_MIN  0.5f // minimum off creep regen when half life is active. must be > 0.

#define ALIEN_MAX_CREDITS        2000 // CREDITS_PER_EVO converts this to evos for display
#define ALIEN_TK_SUICIDE_PENALTY 150

/*
 * HUMAN weapons
 */

extern int   BLASTER_SPREAD;
extern int   BLASTER_SPEED;
extern int   BLASTER_DMG;
extern int   BLASTER_SIZE;

extern int   RIFLE_SPREAD;
extern int   RIFLE_DMG;

extern int   PAINSAW_DAMAGE;
extern float PAINSAW_RANGE;
extern float PAINSAW_WIDTH;
extern float PAINSAW_HEIGHT;

extern int   SHOTGUN_DMG;
extern int   SHOTGUN_RANGE;
extern int   SHOTGUN_PELLETS;
extern int   SHOTGUN_SPREAD;

extern int   LASGUN_DAMAGE;

extern int   MDRIVER_DMG;

extern int   CHAINGUN_SPREAD;
extern int   CHAINGUN_DMG;

extern int   FLAMER_DMG;
extern int   FLAMER_FLIGHTDAMAGE;
extern int   FLAMER_SPLASHDAMAGE;
extern int   FLAMER_RADIUS;
extern int   FLAMER_SIZE;
extern float FLAMER_LIFETIME;
extern float FLAMER_SPEED;
extern float FLAMER_LAG;
extern float FLAMER_IGNITE_RADIUS;
extern float FLAMER_IGNITE_CHANCE;
extern float FLAMER_IGNITE_SPLCHANCE;
#define FLAMER_DAMAGE_MAXDST_MOD 0.5f    // damage decreases linearly from full damage to this during missile lifetime
#define FLAMER_SPLASH_MINDST_MOD 0.5f    // splash damage increases linearly from this to full damage during lifetime
#define FLAMER_LEAVE_FIRE_CHANCE 0.3f

extern int   PRIFLE_DMG;
extern int   PRIFLE_SPEED;
#define PRIFLE_DAMAGE_FULL_TIME  0 // in ms, full damage for this time
#define PRIFLE_DAMAGE_HALF_LIFE  0 // in ms, damage half life time after full damage period, 0 = off
extern int   PRIFLE_SIZE;

extern int   LCANNON_DAMAGE;
extern int   LCANNON_RADIUS;
#define LCANNON_DAMAGE_FULL_TIME 0 // in ms, full damage for this time
#define LCANNON_DAMAGE_HALF_LIFE 0 // in ms, damage half life time after full damage period, 0 = off
extern int   LCANNON_SIZE;
extern int   LCANNON_SECONDARY_DAMAGE;
extern int   LCANNON_SECONDARY_RADIUS;
extern int   LCANNON_SECONDARY_SPEED;
extern int   LCANNON_SPEED;
extern int   LCANNON_CHARGE_TIME_MAX;
extern int   LCANNON_CHARGE_TIME_MIN;
extern int   LCANNON_CHARGE_TIME_WARN;
extern int   LCANNON_CHARGE_AMMO;

#define HBUILD_HEALRATE          10

/*
 * HUMAN upgrades
 */

#define RADAR_RANGE           1000.0f

extern int   MEDKIT_POISON_IMMUNITY_TIME;
extern int   MEDKIT_STARTUP_TIME;
extern int   MEDKIT_STARTUP_SPEED;

/*
 * HUMAN buildables
 */

extern float REACTOR_BASESIZE;
extern float REPEATER_BASESIZE;

#define MGTURRET_ATTACK_PERIOD  125
#define MGTURRET_RANGE          350
#define MGTURRET_SPREAD         200

#define ROCKETPOD_RANGE         2000
#define ROCKETPOD_ATTACK_PERIOD 1000

#define ROCKET_TURN_PERIOD      50
#define ROCKET_TURN_ANGLE       8.0f
#define ROCKET_LIFETIME         3000

extern float REACTOR_ATTACK_RANGE;
extern int   REACTOR_ATTACK_REPEAT;
extern int   REACTOR_ATTACK_DAMAGE;

/*
 * HUMAN misc
 */

#define HUMAN_JOG_MODIFIER            1.0f
#define HUMAN_BACK_MODIFIER           0.8f
#define HUMAN_SIDE_MODIFIER           0.9f
#define HUMAN_LAND_FRICTION           3.0f
#define HUMAN_DODGE_COOLDOWN          1500
#define HUMAN_DODGE_SIDE_MULTIPLIER   2.0f
#define HUMAN_DODGE_SLOW_MULTIPLIER   0.5f
#define HUMAN_DODGE_VERTICAL_MULTIPLIER 0.5f
#define HUMAN_SLIDE_FRICTION_MODIFIER 0.05f
#define HUMAN_SLIDE_THRESHOLD         400.0f

#define STAMINA_MAX                   30000
#define STAMINA_MEDISTAT_RESTORE      450  // 1/(100 ms). stacks.
#define STAMINA_LEVEL1SLOW_TAKE       6    // 1/ms

#define HUMAN_SPAWN_REPEAT_TIME       10000

#define HUMAN_BUILDABLE_REGEN_WAIT    5000

#define HUMAN_MAX_CREDITS             2000
#define HUMAN_TK_SUICIDE_PENALTY      150

#define HUMAN_AMMO_REFILL_PERIOD      2000 // don't refill ammo more frequently than this

#define JETPACK_TARGETSPEED           350.0f
#define JETPACK_ACCELERATION          3.0f
#define JETPACK_JUMPMAG_REDUCTION     0.25f
#define JETPACK_FUEL_MAX              30000 // needs to be < 2^15
#define JETPACK_FUEL_USAGE            6     // in 1/ms
#define JETPACK_FUEL_PER_DMG          300   // per damage point received (before armor mod is applied)
#define JETPACK_FUEL_RESTORE          3     // in 1/ms
#define JETPACK_FUEL_IGNITE           JETPACK_FUEL_MAX / 20      // used when igniting the engine
#define JETPACK_FUEL_LOW              JETPACK_FUEL_MAX / 5       // jetpack doesn't start from a jump below this
#define JETPACK_FUEL_STOP             JETPACK_FUEL_RESTORE * 150 // jetpack doesn't activate below this
#define JETPACK_FUEL_REFUEL           JETPACK_FUEL_MAX - JETPACK_FUEL_USAGE * 1000

/*
 * Misc
 */

#define QU_TO_METER                        0.03125 // in m/qu

#define ENTITY_USE_RANGE                   64.0f
#define ENTITY_BUY_RANGE                   128.0f

// fire
#define FIRE_MIN_DISTANCE                  20.0f

// fall distance
#define MIN_FALL_DISTANCE                  30.0f  // the fall distance at which fall damage kicks in
#define MAX_FALL_DISTANCE                  120.0f // the fall distance at which maximum damage is dealt
#define AVG_FALL_DISTANCE                  (( MIN_FALL_DISTANCE + MAX_FALL_DISTANCE ) / 2.0f )

// impact and weight damage
#define IMPACTDMG_JOULE_TO_DAMAGE          0.002f  // in 1/J
#define WEIGHTDMG_DMG_MODIFIER             0.25f   // multiply with weight difference to get DPS
#define WEIGHTDMG_DPS_THRESHOLD            10      // ignore weight damage per second below this
#define WEIGHTDMG_REPEAT                   200     // in ms, low value reduces damage precision

// buildable explosion
#define HUMAN_DETONATION_DELAY             4000
#define DETONATION_DELAY_RAND_RANGE        0.25f

// buildable limits
#define HUMAN_BUILDDELAY_MOD               0.6f
#define ALIEN_BUILDDELAY_MOD               0.6f

// base attack warnings
#define ATTACKWARN_PRIMARY_PERIOD         7500
#define ATTACKWARN_NEARBY_PERIOD          15000

// score
#define SCORE_PER_CREDIT                   0.02f // used to convert credit rewards to score points
#define SCORE_PER_MOMENTUM                 1.0f  // used to convert momentum rewards to score points
#define HUMAN_BUILDER_SCOREINC             50    // in credits/10s
#define ALIEN_BUILDER_SCOREINC             50    // in credits/10s

// funds (values are in credits, 1 evo = 100 credits)
#define CREDITS_PER_EVO                    100   // Used when alien credits are displayed as evos
#define PLAYER_BASE_VALUE                  200   // base credit value of a player
#define PLAYER_PRICE_TO_VALUE              0.5f  // fraction of upgrade price added to player value
#define DEFAULT_FREEKILL_PERIOD            "120" // in s

// resources
#define RGS_RANGE                          1000.0f // must be > 0
#define DEFAULT_BP_INITIAL_BUDGET          "80"    // in BP
#define DEFAULT_BP_BUDGET_PER_MINER        "50"    // in BP
#define DEFAULT_BP_RECOVERY_INITIAL_RATE   "16"    // in BP/min
#define DEFAULT_BP_RECOVERY_RATE_HALF_LIFE "10"    // in min

// momentum
#define MOMENTUM_MAX                     300.0f
#define MOMENTUM_PER_CREDIT              0.01f // used to award momentum based on credit rewards
#define DEFAULT_MOMENTUM_HALF_LIFE       "5"   // in min
#define DEFAULT_CONF_REWARD_DOUBLE_TIME  "30"  // in min
#define DEFAULT_UNLOCKABLE_MIN_TIME      "30"  // in s
#define DEFAULT_MOMENTUM_BASE_MOD        "0.7"
#define DEFAULT_MOMENTUM_KILL_MOD        "1.3"
#define DEFAULT_MOMENTUM_BUILD_MOD       "0.6"
#define DEFAULT_MOMENTUM_DECON_MOD       "1.0" // used on top of build mod
#define DEFAULT_MOMENTUM_DESTROY_MOD     "0.8"
#define MAIN_STRUCTURE_MOMENTUM_VALUE    20    // momentum reward for destroying OM/RC
#define MINER_MOMENTUM_VALUE             10    // momentum reward for destroying Drill/Leech

#define MAXIMUM_BUILD_TIME                 20000 // used for pie timer
#define BUILDABLE_START_HEALTH_FRAC        0.25f

#endif // G_GAMEPLAY_H_
