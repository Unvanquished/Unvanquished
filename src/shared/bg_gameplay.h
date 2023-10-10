/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012-2013 Unvanquished Developers

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Unvanquished Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished Source Code.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

#ifndef G_GAMEPLAY_H_
#define G_GAMEPLAY_H_

/*
 *============
 * Constants
 *============
 */

/*
 * Common constants for the build weapons for both teams.
 */

// Pressing +deconstruct for less than this many milliseconds toggles the deconstruction mark
extern const int   BUILDER_MAX_SHORT_DECONSTRUCT_CHARGE;
// Holding +deconstruct for this many milliseconds force-deconstructs a buildable
extern const int   BUILDER_LONG_DECONSTRUCT_CHARGE;

extern const float BUILDER_DECONSTRUCT_RANGE;

/*
 * ALIEN weapons
 */

extern const int   LEVEL1_POUNCE_DISTANCE; // pitch between LEVEL1_POUNCE_MINPITCH and pi/4 results in this distance
extern const float LEVEL1_POUNCE_MINPITCH; // minimum pitch that will result in full pounce distance
extern const int   LEVEL1_POUNCE_COOLDOWN;
extern const int   LEVEL1_WALLPOUNCE_MAGNITUDE;
extern const int   LEVEL1_WALLPOUNCE_COOLDOWN;
extern const int   LEVEL1_SIDEPOUNCE_MAGNITUDE;
extern const float LEVEL1_SIDEPOUNCE_DIR_Z; // in ]0.0f,1.0f], fixed Z-coordinate of sidepounce
extern const int   LEVEL1_SIDEPOUNCE_COOLDOWN;
extern const int   LEVEL1_SLOW_TIME;
extern const float LEVEL1_SLOW_MOD;

/*
 * ALIEN buildables
 */

extern const int   CREEP_BASESIZE;
extern const int   CREEP_TIMEOUT;
extern const float CREEP_MODIFIER;
extern const float CREEP_ARMOUR_MODIFIER;
extern const int   CREEP_SCALEDOWN_TIME;

extern const float BARRICADE_SHRINKPROP;
extern const int   BARRICADE_SHRINKTIMEOUT;

extern const int   BOOST_TIME;
extern const int   BOOST_WARN_TIME;
extern const int   BOOST_REPEAT_ANIM;

extern const float ACIDTUBE_RANGE;

extern const float SPIKER_SENSE_RANGE; // an enemy needs to be this close to consider an attack

extern const float TRAPPER_RANGE;

extern const float HIVE_SENSE_RANGE;
extern const float HIVE_SPEED;

extern const int   LOCKBLOB_REPEAT;
extern const float LOCKBLOB_RANGE;
extern const float LOCKBLOB_SPEED;
extern const int   LOCKBLOB_LOCKTIME;
extern const float LOCKBLOB_DOT; // max angle = acos( LOCKBLOB_DOT )

/*
 * ALIEN misc
 */

extern const float ALIENSENSE_RANGE;
extern const float ALIENSENSE_BORDER_FRAC; // In this outer fraction of the range beacons are faded.

extern const float REGEN_BOOSTER_RANGE;
extern const float REGEN_TEAMMATE_RANGE;

extern const int   ALIEN_SPAWN_REPEAT_TIME;

extern const int   ALIEN_CLIENT_REGEN_WAIT; // in ms
extern const int   ALIEN_BUILDABLE_REGEN_WAIT; // in ms

extern const float ALIEN_REGEN_NOCREEP_MIN; // minimum off creep regen when half life is active. must be > 0.

extern const int   ALIEN_MAX_CREDITS; // CREDITS_PER_EVO converts this to evos for display
extern const int   ALIEN_TK_SUICIDE_PENALTY;

/*
 * HUMAN weapons
 */

extern const float FLAMER_DAMAGE_MAXDST_MOD; // damage decreases linearly from full damage to this during missile lifetime
extern const float FLAMER_SPLASH_MINDST_MOD; // splash damage increases linearly from this to full damage during lifetime
extern const float FLAMER_LEAVE_FIRE_CHANCE;

extern const int   PRIFLE_DAMAGE_FULL_TIME; // in ms, full damage for this time
extern const int   PRIFLE_DAMAGE_HALF_LIFE; // in ms, damage half life time after full damage period, 0 = off

extern const int   LCANNON_DAMAGE_FULL_TIME; // in ms, full damage for this time
extern const int   LCANNON_DAMAGE_HALF_LIFE; // in ms, damage half life time after full damage period, 0 = off

extern const int   HBUILD_HEALRATE;

/*
 * HUMAN upgrades
 */

extern const float RADAR_RANGE;

/*
 * HUMAN buildables
 */

extern const int   MGTURRET_ATTACK_PERIOD;
extern const float MGTURRET_RANGE;
extern const int   MGTURRET_SPREAD;

extern const float ROCKETPOD_RANGE;
extern const int   ROCKETPOD_ATTACK_PERIOD;

extern const float ROCKET_TURN_ANGLE;

/*
 * HUMAN misc
 */

extern const float HUMAN_JOG_MODIFIER;
extern const float HUMAN_BACK_MODIFIER;
extern const float HUMAN_SIDE_MODIFIER;
extern const float HUMAN_SLIDE_FRICTION_MODIFIER;
extern const float HUMAN_SLIDE_THRESHOLD;

extern const int   STAMINA_MAX;
extern const int   STAMINA_MEDISTAT_RESTORE; // 1/(100 ms). stacks.
extern const int   STAMINA_LEVEL1SLOW_TAKE; // 1/ms

extern const int   HUMAN_SPAWN_REPEAT_TIME;

extern const int   HUMAN_BUILDABLE_REGEN_WAIT;

extern const int   HUMAN_MAX_CREDITS;
extern const int   HUMAN_TK_SUICIDE_PENALTY;

extern const int   HUMAN_AMMO_REFILL_PERIOD; // don't refill ammo more frequently than this

extern const float JETPACK_TARGETSPEED;
extern const float JETPACK_ACCELERATION;
extern const float JETPACK_JUMPMAG_REDUCTION;
extern const int   JETPACK_FUEL_MAX; // needs to be < 2^15
extern const int   JETPACK_FUEL_USAGE; // in 1/ms
extern const int   JETPACK_FUEL_PER_DMG; // per damage point received (before armor mod is applied)
extern const int   JETPACK_FUEL_RESTORE; // in 1/ms
extern const float JETPACK_FUEL_IGNITE; // used when igniting the engine
extern const float JETPACK_FUEL_LOW; // jetpack doesn't start from a jump below this
extern const float JETPACK_FUEL_STOP; // jetpack doesn't activate below this
extern const float JETPACK_FUEL_REFUEL;

/*
 * Misc
 */

#define QU_TO_METER 0.03125f // in m/qu

extern const float ENTITY_USE_RANGE;

// fire
extern const float FIRE_MIN_DISTANCE;
extern const float FIRE_DAMAGE_RADIUS;

// fall distance
extern const float MIN_FALL_DISTANCE; // the fall distance at which fall damage kicks in
extern const float MAX_FALL_DISTANCE; // the fall distance at which maximum damage is dealt
extern const int   AVG_FALL_DISTANCE;

// impact and weight damage
extern const float IMPACTDMG_JOULE_TO_DAMAGE; // in 1/J
extern const float WEIGHTDMG_DMG_MODIFIER; // multiply with weight difference to get DPS
extern const int   WEIGHTDMG_DPS_THRESHOLD; // ignore weight damage per second below this
extern const int   WEIGHTDMG_REPEAT; // in ms, low value reduces damage precision

// buildable explosion
extern const int   HUMAN_DETONATION_DELAY;
extern const float DETONATION_DELAY_RAND_RANGE;

// buildable limits
extern const float HUMAN_BUILDDELAY_MOD;
extern const float ALIEN_BUILDDELAY_MOD;

// base attack warnings
extern const int   ATTACKWARN_PRIMARY_PERIOD;
extern const int   ATTACKWARN_NEARBY_PERIOD;

// how long you can sustain underwater before taking damage
#define OXYGEN_MAX_TIME       12000
// how many bits are needed to store this number (plus a margin cgame needs)
#define LOW_OXYGEN_TIME_BITS  14

// score
extern const float SCORE_PER_CREDIT; // used to convert credit rewards to score points
extern const float SCORE_PER_MOMENTUM; // used to convert momentum rewards to score points
extern const int   HUMAN_BUILDER_SCOREINC; // in credits/10s
extern const int   ALIEN_BUILDER_SCOREINC; // in credits/10s

// funds (values are in credits, 1 evo = 100 credits)
extern const int   CREDITS_PER_EVO; // Used when alien credits are displayed as evos
extern const int   PLAYER_BASE_VALUE; // base credit value of a player
extern const float PLAYER_PRICE_TO_VALUE; // fraction of upgrade price added to player value
extern const int   DEFAULT_FREEKILL_PERIOD; // in s

// resources
extern const float RGS_RANGE; // must be > 0
extern const int   DEFAULT_BP_INITIAL_BUDGET; // in BP
extern const int   DEFAULT_BP_BUDGET_PER_MINER; // in BP
extern const int   DEFAULT_BP_RECOVERY_INITIAL_RATE; // in BP/min
extern const int   DEFAULT_BP_RECOVERY_RATE_HALF_LIFE; // in min

// momentum
extern const float MOMENTUM_MAX;
extern const float MOMENTUM_PER_CREDIT; // used to award momentum based on credit rewards
extern const int   DEFAULT_MOMENTUM_HALF_LIFE; // in min
extern const int   DEFAULT_CONF_REWARD_DOUBLE_TIME; // in min
extern const int   DEFAULT_UNLOCKABLE_MIN_TIME; // in s
extern const float DEFAULT_MOMENTUM_BASE_MOD;
extern const float DEFAULT_MOMENTUM_KILL_MOD;
extern const float DEFAULT_MOMENTUM_BUILD_MOD;
extern const float DEFAULT_MOMENTUM_DECON_MOD; // used on top of build mod
extern const float DEFAULT_MOMENTUM_DESTROY_MOD;
extern const int   MAIN_STRUCTURE_MOMENTUM_VALUE; // momentum reward for destroying OM/RC
extern const int   MINER_MOMENTUM_VALUE; // momentum reward for destroying Drill/Leech

extern const float BUILDABLE_START_HEALTH_FRAC;

/*
 *====================
 * Config file values
 *====================
 */

/*
 * Those values are not defined in cpp files, but come from config files.
 * You should modify config files in the unvanquished dpk to set their values.
 */

/*
 * ALIEN weapons
 */

extern int   ABUILDER_CLAW_DMG;
extern float ABUILDER_CLAW_RANGE;
extern float ABUILDER_CLAW_WIDTH;
extern float ABUILDER_BLOB_SPEED;
extern float ABUILDER_BLOB_SPEED_MOD;
extern int   ABUILDER_BLOB_TIME;

extern int   LEVEL0_BITE_DMG;
extern float LEVEL0_BITE_RANGE;
extern float LEVEL0_BITE_WIDTH;
extern int   LEVEL0_BITE_REPEAT;

extern int   LEVEL1_CLAW_DMG;
extern float LEVEL1_CLAW_RANGE;
extern float LEVEL1_CLAW_WIDTH;

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
extern float LEVEL3_BOUNCEBALL_SPEED;
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

/*
 * HUMAN weapons
 */

extern int   BLASTER_SPEED;

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

extern int   FLAMER_SIZE;
extern float FLAMER_SPEED;
extern float FLAMER_LAG;
extern float FLAMER_IGNITE_RADIUS;
extern float FLAMER_IGNITE_CHANCE;
extern float FLAMER_IGNITE_SPLCHANCE;

extern int   PRIFLE_SPEED;

extern int   LCANNON_DAMAGE;
extern int   LCANNON_CHARGE_TIME_MAX;
extern int   LCANNON_CHARGE_TIME_MIN;
extern int   LCANNON_CHARGE_TIME_WARN;
extern int   LCANNON_CHARGE_AMMO;

/*
 * HUMAN upgrades
 */

extern int   MEDKIT_POISON_IMMUNITY_TIME;
extern int   MEDKIT_STARTUP_TIME;
extern int   MEDKIT_STARTUP_SPEED;

// movement
#define MIN_WALK_NORMAL   0.7f // can't walk on very steep slopes
#define STEPSIZE          18

#endif // G_GAMEPLAY_H_
