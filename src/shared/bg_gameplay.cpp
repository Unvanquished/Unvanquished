/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012-2023 Unvanquished Developers

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


#include "bg_gameplay.h"
#include "engine/qcommon/q_shared.h"

/*
 * This file contains gameplay constants.
 *
 * Some of the values here have to be known both by cgame and sgame. Some are
 * not and are here instead of in cvars for legacy reasons. This file exists
 * because of those shared variables; see below for the rationale.
 *
 * Note fast-changing values that need to be transmitted to the client
 * typically live in playerState_t or entityState_t.
 *
 * ###
 * # Sharing a constant or near-constant value between sgame and cgame
 * ###
 *
 * A constant symbol defined here will be included in both binaries. This makes
 * the value available on both side and is a decent compromise between
 * compilation time and network usage. The values can't be modified, or the
 * change would not be transmitted to the other side.
 *
 * A value can be shared between cgame and sgame in three ways:
 * 1. Here, using a constant symbol that will be included in both VM files
 * 2. Using config files
 * 3. Using a SERVERINFO cvar
 *
 * This file has the upside of being simple, but doesn't allow runtime
 * modification, and needs a recompilation at every change.
 *
 * Config files are supposedly more practical for large scale edits, because of
 * readability. They also have a slight advantage over this file in that they
 * don't need a modder to recompile the game. This method also doesn't allow
 * runtime changes.
 * This method has downsides. The first downside is that one has to write the
 * parsing code in addition to doing what's needed for a symbol. This is a long
 * process. The second downside is that this requires you to deal with .cfg
 * file packaging.
 *
 * Cvars have the upside of allowing runtime edits. The process of adding one
 * is more complicated: after creating the cvar with the SERVERINFO, you must
 * parse it in cgame (see g_devolveMaxBaseDistance for an example).
 * Use this solution if you expect server admins to want to change this value.
 * This solution has downsides too. One is that large scale modifications are
 * pretty unpractical and one loses the ability to have proper tools such as
 * a text editor and git, or the ability to add moder-specific comments
 * explaining why the values were modified.
 */



/*
 * Common constants for the build weapons for both teams.
 */

const int   BUILDER_MAX_SHORT_DECONSTRUCT_CHARGE = 200;
const int   BUILDER_LONG_DECONSTRUCT_CHARGE = 800;

const float BUILDER_DECONSTRUCT_RANGE = 100;

/*
 * ALIEN buildables
 */

const int   CREEP_BASESIZE = 700;
const int   CREEP_TIMEOUT = 1000;
const float CREEP_MODIFIER = 0.5f;
const float CREEP_ARMOUR_MODIFIER = 0.75f;
const int   CREEP_SCALEDOWN_TIME = 3000;

const float BARRICADE_SHRINKPROP = 0.25f;
const int   BARRICADE_SHRINKTIMEOUT = 500;

const int   BOOST_TIME = 20000;
const int   BOOST_WARN_TIME = 15000;
const int   BOOST_REPEAT_ANIM = 2000;

const float ACIDTUBE_RANGE = 300.0f;

const float SPIKER_SENSE_RANGE = 250.0f;

const float TRAPPER_RANGE = 400.0f;

const float HIVE_SENSE_RANGE = 500.0f;
const float HIVE_SPEED = 320.0f;

const int   LOCKBLOB_REPEAT = 1000;
const float LOCKBLOB_RANGE = 400.0f;
const float LOCKBLOB_SPEED = 500.0f;
const int   LOCKBLOB_LOCKTIME = 5000;
const float LOCKBLOB_DOT = 0.85f;

/*
 * ALIEN misc
 */

const float ALIENSENSE_RANGE = 1500.0f;
const float ALIENSENSE_BORDER_FRAC = 0.2f;

const float REGEN_BOOSTER_RANGE = 200.0f;
const float REGEN_TEAMMATE_RANGE = 300.0f;

const int   ALIEN_SPAWN_REPEAT_TIME = 10000;

const int   ALIEN_CLIENT_REGEN_WAIT = 2000;
const int   ALIEN_BUILDABLE_REGEN_WAIT = 2000;

const float ALIEN_REGEN_NOCREEP_MIN = 0.5f;

const int   ALIEN_MAX_CREDITS = 2000;
const int   ALIEN_TK_SUICIDE_PENALTY = 150;

const int   LEVEL1_POUNCE_DISTANCE = 300;
const float LEVEL1_POUNCE_MINPITCH = M_PI / 12.0f;
const int   LEVEL1_POUNCE_COOLDOWN = 2000;
const int   LEVEL1_WALLPOUNCE_MAGNITUDE = 600;
const int   LEVEL1_WALLPOUNCE_COOLDOWN = 1200;
const int   LEVEL1_SIDEPOUNCE_MAGNITUDE = 400;
const float LEVEL1_SIDEPOUNCE_DIR_Z = 0.4f;
const int   LEVEL1_SIDEPOUNCE_COOLDOWN = 750;
const int   LEVEL1_SLOW_TIME = 1000;
const float LEVEL1_SLOW_MOD = 0.75f;

/*
 * HUMAN weapons
 */

const float FLAMER_DAMAGE_MAXDST_MOD = 0.5f;
const float FLAMER_SPLASH_MINDST_MOD = 0.5f;
const float FLAMER_LEAVE_FIRE_CHANCE = 0.2f;

const int   PRIFLE_DAMAGE_FULL_TIME = 0;
const int   PRIFLE_DAMAGE_HALF_LIFE = 0;

const int   LCANNON_DAMAGE_FULL_TIME = 0;
const int   LCANNON_DAMAGE_HALF_LIFE = 0;

const int   HBUILD_HEALRATE = 10;

/*
 * HUMAN upgrades
 */

const float RADAR_RANGE = 1000.0f;

/*
 * HUMAN buildables
 */

const int   MGTURRET_ATTACK_PERIOD = 125;
const float MGTURRET_RANGE = 350;
const int   MGTURRET_SPREAD = 200;

const float ROCKETPOD_RANGE = 2000;
const int   ROCKETPOD_ATTACK_PERIOD = 1000;

const float ROCKET_TURN_ANGLE = 8.0f;

/*
 * HUMAN misc
 */

const float HUMAN_JOG_MODIFIER = 1.0f;
const float HUMAN_BACK_MODIFIER = 0.8f;
const float HUMAN_SIDE_MODIFIER = 0.9f;
const float HUMAN_SLIDE_FRICTION_MODIFIER = 0.05f;
const float HUMAN_SLIDE_THRESHOLD = 400.0f;

const int   STAMINA_MAX = 30000;
const int   STAMINA_MEDISTAT_RESTORE = 450;
const int   STAMINA_LEVEL1SLOW_TAKE = 6;

const int   HUMAN_SPAWN_REPEAT_TIME = 10000;

const int   HUMAN_BUILDABLE_REGEN_WAIT = 5000;

const int   HUMAN_MAX_CREDITS = 2000;
const int   HUMAN_TK_SUICIDE_PENALTY = 150;

const int   HUMAN_AMMO_REFILL_PERIOD = 2000;

const float JETPACK_TARGETSPEED = 350.0f;
const float JETPACK_ACCELERATION = 3.0f;
const float JETPACK_JUMPMAG_REDUCTION = 0.25f;
const int   JETPACK_FUEL_MAX = 30000;
const int   JETPACK_FUEL_USAGE = 6;
const int   JETPACK_FUEL_PER_DMG = 300;
const int   JETPACK_FUEL_RESTORE = 3;
const float JETPACK_FUEL_IGNITE = JETPACK_FUEL_MAX / 20;
const float JETPACK_FUEL_LOW = JETPACK_FUEL_MAX / 5;
const float JETPACK_FUEL_STOP = JETPACK_FUEL_RESTORE * 150;
const float JETPACK_FUEL_REFUEL = JETPACK_FUEL_MAX - JETPACK_FUEL_USAGE * 1000;

/*
 * Misc
 */

const float ENTITY_USE_RANGE = 128.0f;

// fire
const float FIRE_MIN_DISTANCE = 20.0f;
const float FIRE_DAMAGE_RADIUS = 60.0f;

// fall distance
const float MIN_FALL_DISTANCE = 30.0f;
const float MAX_FALL_DISTANCE = 120.0f;
const int   AVG_FALL_DISTANCE = (MIN_FALL_DISTANCE + MAX_FALL_DISTANCE ) / 2.0f;

// impact and weight damage
const float IMPACTDMG_JOULE_TO_DAMAGE = 0.002f;
const float WEIGHTDMG_DMG_MODIFIER = 0.25f;
const int   WEIGHTDMG_DPS_THRESHOLD = 10;
const int   WEIGHTDMG_REPEAT = 200;

// buildable explosion
const int   HUMAN_DETONATION_DELAY = 4000;
const float DETONATION_DELAY_RAND_RANGE = 0.25f;

// buildable limits
const float HUMAN_BUILDDELAY_MOD = 0.6f;
const float ALIEN_BUILDDELAY_MOD = 0.6f;

// base attack warnings
const int   ATTACKWARN_PRIMARY_PERIOD = 7500;
const int   ATTACKWARN_NEARBY_PERIOD = 15000;

// score
const float SCORE_PER_CREDIT = 0.02f;
const float SCORE_PER_MOMENTUM = 1.0f;
const int   HUMAN_BUILDER_SCOREINC = 50;
const int   ALIEN_BUILDER_SCOREINC = 50;

// funds (values are in credits, 1 evo = 100 credits)
const int   CREDITS_PER_EVO = 100;
const int   PLAYER_BASE_VALUE = 200;
const float PLAYER_PRICE_TO_VALUE = 0.5f;
const int   DEFAULT_FREEKILL_PERIOD = 120;

// resources
const float RGS_RANGE = 1000.0f;
const int   DEFAULT_BP_INITIAL_BUDGET = 80;
const int   DEFAULT_BP_BUDGET_PER_MINER = 50;
const int   DEFAULT_BP_RECOVERY_INITIAL_RATE = 16;
const int   DEFAULT_BP_RECOVERY_RATE_HALF_LIFE = 10;

// momentum
const float MOMENTUM_MAX = 300.0f;
const float MOMENTUM_PER_CREDIT = 0.01f;
const int   DEFAULT_MOMENTUM_HALF_LIFE = 5;
const int   DEFAULT_CONF_REWARD_DOUBLE_TIME = 30;
const int   DEFAULT_UNLOCKABLE_MIN_TIME = 30;
const float DEFAULT_MOMENTUM_BASE_MOD = 0.7f;
const float DEFAULT_MOMENTUM_KILL_MOD = 1.3f;
const float DEFAULT_MOMENTUM_BUILD_MOD = 0.6f;
const float DEFAULT_MOMENTUM_DECON_MOD = 1.0f;
const float DEFAULT_MOMENTUM_DESTROY_MOD = 0.8f;
const int   MAIN_STRUCTURE_MOMENTUM_VALUE = 20;
const int   MINER_MOMENTUM_VALUE = 10;

const float BUILDABLE_START_HEALTH_FRAC = 0.25f;
