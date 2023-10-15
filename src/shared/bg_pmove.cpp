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

// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modified playerstate

#include "engine/qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_gameplay.h"

#define TIMER_LAND        130
#define TIMER_GESTURE     ( 34 * 66 + 50 )
#define TIMER_ATTACK      500 //nonsegmented models

#define FALLING_THRESHOLD -900.0f //what vertical speed to start falling sound at

// movement parameters
#define pm_duckScale         (0.25f)
#define pm_swimScale         (0.50f)

#define pm_accelerate        (10.0f)
#define pm_wateraccelerate   (4.0f)
#define pm_flyaccelerate     (4.0f)

#define pm_waterfriction     (1.125f)
#define pm_spectatorfriction (5.0f)


// all of the locals will be zeroed before each
// pmove, just to make damn sure we don't have
// any differences when running on client or server
struct pml_t
{
	vec3_t   forward, right, up;
	float    frametime;

	int      msec;

	bool walking;
	bool groundPlane;
	bool ladder;
	trace_t  groundTrace;

	float    impactSpeed;

	vec3_t   previous_origin;
	vec3_t   previous_velocity;
	int      previous_waterlevel;

};

static pmove_t *pm;
pml_t   pml;

static int c_pmove = 0;

static void Slide( vec3_t wishdir, float wishspeed, playerState_t &ps );
static void PM_AddEvent( int newEvent );
static bool PM_SlideMove( bool gravity );
static bool PM_StepSlideMove( bool gravity, bool predictive );
static bool PM_PredictStepMove();
static void PM_StepEvent( const vec3_t from, const vec3_t to, const vec3_t normal );

static bool PM_Paralyzed( pmtype_t pmt )
{
	switch( pmt )
	{
		case PM_NORMAL:
		case PM_NOCLIP:
		case PM_SPECTATOR:
		case PM_GRABBED:
			return false;
		case PM_DEAD:
		case PM_FREEZE:
		case PM_INTERMISSION:
			return true;
	}
	ASSERT_UNREACHABLE();
}

//TODO: deprecate (to allow typesafety)
static bool PM_Paralyzed( int pmt )
{
	return PM_Paralyzed( static_cast<pmtype_t>( pmt ) );
}

static bool IsSegmentedModel( playerState_t const* ps )
{
	return !( ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL );
}

// well... this only checks for sky and slick, so it might
// need changes. At least now, there's a single place.
static bool hitGrippingSurface( trace_t const& tr )
{
	 return tr.fraction < 1.0f
		 && !( tr.surfaceFlags & ( SURF_SKY | SURF_SLICK ) );
}

/*
===============
PM_AddEvent

===============
*/
static void PM_AddEvent( int newEvent )
{
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
static void PM_AddTouchEnt( int entityNum )
{
	int i;

	if ( entityNum == ENTITYNUM_WORLD )
	{
		return;
	}

	if ( pm->numtouch == MAXTOUCH )
	{
		return;
	}

	// see if it is already added
	for ( i = 0; i < pm->numtouch; i++ )
	{
		if ( pm->touchents[ i ] == entityNum )
		{
			return;
		}
	}

	// add it
	pm->touchents[ pm->numtouch ] = entityNum;
	pm->numtouch++;
}

/*
===================
PM_StartTorsoAnim
===================
*/
static void PM_StartTorsoAnim( int anim )
{
	if ( PM_Paralyzed( pm->ps->pm_type ) )
	{
		return;
	}

	pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
	                    | anim;
}

/*
===================
PM_StartWeaponAnim
===================
*/
static void PM_StartWeaponAnim( int anim )
{
	if ( PM_Paralyzed( pm->ps->pm_type ) )
	{
		return;
	}

	pm->ps->weaponAnim = ( ( pm->ps->weaponAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
	                     | anim;
}

/*
===================
PM_StartLegsAnim
===================
*/
static void PM_StartLegsAnim( int anim )
{
	playerState_t * ps = pm->ps;
	if ( PM_Paralyzed( ps->pm_type ) )
	{
		return;
	}

	//legsTimer is clamped too tightly for nonsegmented models
	if ( IsSegmentedModel( ps ) )
	{
		if ( pm->ps->legsTimer > 0 )
		{
			return; // a high priority animation is running
		}
	}
	else
	{
		if ( pm->ps->torsoTimer > 0 )
		{
			return; // a high priority animation is running
		}
	}

	pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | anim;
}

/*
===================
PM_ContinueLegsAnim
===================
*/
static void PM_ContinueLegsAnim( int anim )
{
	if ( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == anim )
	{
		return;
	}

	//legsTimer is clamped too tightly for nonsegmented models
	if ( IsSegmentedModel( pm->ps ) )
	{
		if ( pm->ps->legsTimer > 0 )
		{
			return; // a high priority animation is running
		}
	}
	else
	{
		if ( pm->ps->torsoTimer > 0 )
		{
			return; // a high priority animation is running
		}
	}

	PM_StartLegsAnim( anim );
}

/*
===================
PM_ContinueTorsoAnim
===================
*/
static void PM_ContinueTorsoAnim( int anim )
{
	if ( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) == anim )
	{
		return;
	}

	if ( pm->ps->torsoTimer > 0 )
	{
		return; // a high priority animation is running
	}

	PM_StartTorsoAnim( anim );
}

/*
===================
PM_ContinueWeaponAnim
===================
*/
static void PM_ContinueWeaponAnim( int anim )
{
	if ( ( pm->ps->weaponAnim & ~ANIM_TOGGLEBIT ) == anim )
	{
		return;
	}

	PM_StartWeaponAnim( anim );
}

/*
===================
PM_ForceLegsAnim
===================
*/
static void PM_ForceLegsAnim( int anim )
{
	//legsTimer is clamped too tightly for nonsegmented models
	if ( IsSegmentedModel( pm->ps ) )
	{
		pm->ps->legsTimer = 0;
	}
	else
	{
		pm->ps->torsoTimer = 0;
	}

	PM_StartLegsAnim( anim );
}

/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
static void PM_ClipVelocity( const vec3_t in, const vec3_t normal, vec3_t out )
{
	float t = -DotProduct( in, normal );
	VectorMA( in, t, normal, out );
}

/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction()
{
	vec3_t &vel = pm->ps->velocity;

	// make sure vertical velocity is NOT set to zero when wall climbing
	vec3_t vec;
	VectorCopy( vel, vec );

	if ( pml.walking && !( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
	{
		vec[ 2 ] = 0; // ignore slope movement
	}
	float speed = VectorLength( vec );

	if ( speed < 0.1f )
	{
		return;
	}

	float drop = 0.0f;

	// apply ground friction
	if ( pm->waterlevel <= 1 )
	{
		if ( ( pml.walking || pml.ladder ) && !( pml.groundTrace.surfaceFlags & SURF_SLICK ) )
		{
			// if getting knocked back, no friction
			if ( !( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) )
			{
				float stopSpeed = BG_Class( pm->ps->stats[ STAT_CLASS ] )->stopSpeed;
				float friction = BG_Class( pm->ps->stats[ STAT_CLASS ] )->friction;

				if ( pm->ps->stats[ STAT_STATE ] & SS_SLIDING )
				{
					friction *= HUMAN_SLIDE_FRICTION_MODIFIER;
				}

				float control = speed < stopSpeed ? stopSpeed : speed;
				drop = control * friction * pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel )
	{
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR || pm->ps->pm_type == PM_NOCLIP )
	{
		drop += speed * pm_spectatorfriction * pml.frametime;
	}

	// scale the velocity
	float newspeed = speed - drop;

	if ( newspeed < 0.0f )
	{
		newspeed = 0.0f;
	}

	newspeed /= speed;

	vel[ 0 ] = vel[ 0 ] * newspeed;
	vel[ 1 ] = vel[ 1 ] * newspeed;
	vel[ 2 ] = vel[ 2 ] * newspeed;
}

/*
==============
PM_Accelerate

Handles user intended acceleration
==============
*/
static void PM_Accelerate( const vec3_t wishdir, float wishspeed, float accel )
{
#if 1
	// q2 style
	float currentspeed = DotProduct( pm->ps->velocity, wishdir );
	float addspeed = wishspeed - currentspeed;

	if ( addspeed <= 0 )
	{
		return;
	}

	float accelspeed = std::min( accel * pml.frametime * wishspeed, addspeed );
	VectorMA( pm->ps->velocity, accelspeed, wishdir, pm->ps->velocity );
#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t wishVelocity;
	vec3_t pushDir;

	VectorScale( wishdir, wishspeed, wishVelocity );
	VectorSubtract( wishVelocity, pm->ps->velocity, pushDir );
	float pushLen = VectorNormalize( pushDir );

	float canPush = std::min( accel * pml.frametime * wishspeed, pushLen );
	VectorMA( pm->ps->velocity, canPush, pushDir, pm->ps->velocity );
#endif
}

/*
============
PM_CmdScale

Returns the scale factor to apply to cmd movements
This allows the clients to use axial -127 to 127 values for all directions
without getting a sqrt(2) distortion in speed.
============
*/
static float PM_CmdScale( usercmd_t *cmd, bool zFlight )
{
	float modifier = 1.0f;
	int   staminaJumpCost = BG_Class( pm->ps->stats[ STAT_CLASS ] )->staminaJumpCost;
	int   stamina = pm->ps->stats[ STAT_STAMINA ];

	if ( pm->ps->persistant[ PERS_TEAM ] == TEAM_HUMANS && pm->ps->pm_type == PM_NORMAL )
	{
		bool wasSprinting, sprint;

		wasSprinting = sprint = pm->ps->stats[ STAT_STATE ] & SS_SPEEDBOOST;

		// check whether player wants to sprint
		if ( pm->ps->persistant[ PERS_STATE ] & PS_SPRINTTOGGLE )
		{
			// toggle mode
			if ( usercmdButtonPressed( cmd->buttons, BTN_SPRINT ) &&
			     !( pm->ps->pm_flags & PMF_SPRINTHELD ) )
			{
				sprint = !sprint;
				pm->ps->pm_flags |= PMF_SPRINTHELD;
			}
			else if ( pm->ps->pm_flags & PMF_SPRINTHELD &&
			          !usercmdButtonPressed( cmd->buttons, BTN_SPRINT ) )
			{
				pm->ps->pm_flags &= ~PMF_SPRINTHELD;
			}
		}
		else
		{
			// non-toggle mode
			sprint = usercmdButtonPressed( cmd->buttons, BTN_SPRINT );
		}

		// check if sprinting is allowed
		if ( sprint )
		{
			if ( wasSprinting && stamina <= 0 )
			{
				// we were sprinting but ran out of stamina
				sprint = false;
			}
			else if ( !wasSprinting && stamina < staminaJumpCost )
			{
				// stamina is too low to start sprinting
				sprint = false;
			}
		}

		// start or stop sprint
		if ( sprint )
		{
			pm->ps->stats[ STAT_STATE ] |= SS_SPEEDBOOST;
		}
		else
		{
			pm->ps->stats[ STAT_STATE ] &= ~SS_SPEEDBOOST;
		}

		// Walk overrides sprint. We keep the state that we want to be sprinting
		// above but don't apply the modifier, and in g_active we skip taking
		// the stamina too.
		// TODO: Try to move code upwards so sprinting isn't activated in the first place.
		if ( sprint && !usercmdButtonPressed( cmd->buttons, BTN_WALKING ) )
		{
			modifier *= BG_Class( pm->ps->stats[ STAT_CLASS ] )->sprintMod;
		}
		else
		{
			modifier *= HUMAN_JOG_MODIFIER;
		}

		// Apply modifiers for strafing and going backwards
		if ( cmd->forwardmove < 0 )
		{
			modifier *= HUMAN_BACK_MODIFIER;
		}
		else if ( cmd->rightmove )
		{
			modifier *= HUMAN_SIDE_MODIFIER;
		}

		// Cancel jump if low on stamina
		if ( !zFlight && stamina < staminaJumpCost )
		{
			cmd->upmove = 0;
		}

		// Apply creepslow modifier
		// TODO: Move modifer into upgrade/class config files of armour items/classes
		if ( pm->ps->stats[ STAT_STATE ] & SS_CREEPSLOWED )
		{
			if ( BG_InventoryContainsUpgrade( UP_LIGHTARMOUR, pm->ps->stats ) ||
			     BG_InventoryContainsUpgrade( UP_MEDIUMARMOUR, pm->ps->stats ) ||
			     BG_InventoryContainsUpgrade( UP_BATTLESUIT,  pm->ps->stats ) )
			{
				modifier *= CREEP_ARMOUR_MODIFIER;
			}
			else
			{
				modifier *= CREEP_MODIFIER;
			}
		}

		// Apply level1 slow modifier
		if ( pm->ps->stats[ STAT_STATE2 ] & SS2_LEVEL1SLOW )
		{
			modifier *= LEVEL1_SLOW_MOD;
		}
	}

	// Go faster when using the Tyrant charge attack
	if ( pm->ps->weapon == WP_ALEVEL4 && pm->ps->pm_flags & PMF_CHARGE )
	{
		modifier *= 1.0f + ( pm->ps->weaponCharge * ( LEVEL4_TRAMPLE_SPEED - 1.0f ) /
		                     LEVEL4_TRAMPLE_DURATION );
	}

	// Slow down when charging up for a pounce
	if ( ( pm->ps->weapon == WP_ALEVEL3 || pm->ps->weapon == WP_ALEVEL3_UPG ) &&
	     usercmdButtonPressed( cmd->buttons, BTN_ATTACK2 ) )
	{
		modifier *= LEVEL3_POUNCE_SPEED_MOD;
	}

	// Slow down when slow locked
	// TODO: Introduce armour modifiers for slowlocking
	if ( pm->ps->stats[ STAT_STATE ] & SS_SLOWLOCKED )
	{
		modifier *= ABUILDER_BLOB_SPEED_MOD;
	}

	// Stand still when grabbed
	if ( pm->ps->pm_type == PM_GRABBED )
	{
		modifier = 0.0f;
	}

	float max = std::max( std::abs( cmd->forwardmove ), std::abs( cmd->rightmove ) );
	float total = Square( cmd->forwardmove ) + Square( cmd->rightmove );

	if ( zFlight )
	{
		max = std::max( static_cast<float>( std::abs( cmd->upmove ) ), max );

		total += Square( cmd->upmove );
	}

	if ( max <= 0.0f )
	{
		return 0.0f;
	}

	total = sqrtf( total );

	return pm->ps->speed * max / ( 127.0f * total ) * modifier;
}

/*
================
PM_SetMovementDir

Determine the rotation of the legs relative to the facing dir
================
*/
static void PM_SetMovementDir()
{
	if ( pm->cmd.forwardmove || pm->cmd.rightmove )
	{
		if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove > 0 )
		{
			pm->ps->movementDir = 0;
		}
		else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove > 0 )
		{
			pm->ps->movementDir = 1;
		}
		else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove == 0 )
		{
			pm->ps->movementDir = 2;
		}
		else if ( pm->cmd.rightmove < 0 && pm->cmd.forwardmove < 0 )
		{
			pm->ps->movementDir = 3;
		}
		else if ( pm->cmd.rightmove == 0 && pm->cmd.forwardmove < 0 )
		{
			pm->ps->movementDir = 4;
		}
		else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove < 0 )
		{
			pm->ps->movementDir = 5;
		}
		else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove == 0 )
		{
			pm->ps->movementDir = 6;
		}
		else if ( pm->cmd.rightmove > 0 && pm->cmd.forwardmove > 0 )
		{
			pm->ps->movementDir = 7;
		}
	}
	else
	{
		// if they aren't actively going directly sideways,
		// change the animation to the diagonal so they
		// don't stop too crooked
		if ( pm->ps->movementDir == 2 )
		{
			pm->ps->movementDir = 1;
		}
		else if ( pm->ps->movementDir == 6 )
		{
			pm->ps->movementDir = 7;
		}
	}
}

/*
=============
PM_CheckCharge
=============
*/
static void PM_CheckCharge()
{
	if ( pm->ps->weapon != WP_ALEVEL4 )
	{
		return;
	}

	if ( usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK2 ) &&
	     !( pm->ps->stats[ STAT_STATE ] & SS_CHARGING ) )
	{
		pm->ps->pm_flags &= ~PMF_CHARGE;
		return;
	}

	if ( pm->ps->weaponCharge > 0 )
	{
		pm->ps->pm_flags |= PMF_CHARGE;
	}
	else
	{
		pm->ps->pm_flags &= ~PMF_CHARGE;
	}
}

/*
=============
PM_CheckWaterPounce
=============
*/
static void PM_CheckWaterPounce()
{
	// Check for valid class
	switch ( pm->ps->weapon )
	{
		case WP_ALEVEL1:
		case WP_ALEVEL3:
		case WP_ALEVEL3_UPG:
			break;

		default:
			return;
	}

	// We were pouncing, but we've landed into water
	if ( ( pm->waterlevel > 1 ) && ( pm->ps->pm_flags & PMF_CHARGE ) )
	{
		pm->ps->pm_flags &= ~PMF_CHARGE;

		switch ( pm->ps->weapon )
		{
			case WP_ALEVEL3:
			case WP_ALEVEL3_UPG:
				pm->ps->weaponTime += LEVEL3_POUNCE_REPEAT;
				break;
		}
	}
}

/*
=============
PM_PlayJumpingAnimation
=============
*/
static void PM_PlayJumpingAnimation()
{
	bool forward = pm->cmd.forwardmove >= 0;
	if ( IsSegmentedModel( pm->ps ) )
	{
		PM_ForceLegsAnim( forward ? LEGS_JUMP : LEGS_JUMPB );
	}
	else
	{
		PM_ForceLegsAnim( forward ? NSPA_JUMP : NSPA_JUMPBACK );
	}

	if ( forward )
	{
		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else
	{
		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}
}

/*
=============
PM_CheckPounce
=============
*/
static bool PM_CheckPounce()
{
	const static vec3_t up = { 0.0f, 0.0f, 1.0f };

	int      jumpMagnitude;
	vec3_t   jumpDirection;
	float    pitch, pitchToGround, pitchToRef;

	// Check for valid class
	switch ( pm->ps->weapon )
	{
		case WP_ALEVEL1:
		case WP_ALEVEL3:
		case WP_ALEVEL3_UPG:
			break;

		default:
			return false;
	}

	// Handle landing
	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE && ( pm->ps->pm_flags & PMF_CHARGE ) )
	{
		// end the pounce
		pm->ps->pm_flags &= ~PMF_CHARGE;

		// goon pounce delays bite attacks
		switch ( pm->ps->weapon )
		{
			case WP_ALEVEL3:
			case WP_ALEVEL3_UPG:
				pm->ps->weaponTime += LEVEL3_POUNCE_REPEAT;
				break;
		}

		return false;
	}

	// Check class-specific conditions for starting a pounce
	switch ( pm->ps->weapon )
	{
		case WP_ALEVEL1:
			// Check if player wants to pounce
			if ( !usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK2 ) )
			{
				return false;
			}

			// Check if cooldown is over
			if ( pm->ps->weaponCharge > 0 )
			{
				return false;
			}
			break;

		case WP_ALEVEL3:
		case WP_ALEVEL3_UPG:
			// Don't pounce while still charging
			if ( usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK2 ) )
			{
				pm->ps->pm_flags &= ~PMF_CHARGE;

				return false;
			}

			// Pounce if minimum charge time has passed and the charge button isn't held anymore
			if ( pm->ps->weaponCharge < LEVEL3_POUNCE_TIME_MIN )
			{
				return false;
			}
			break;
	}

	// Check if already pouncing
	if ( pm->ps->pm_flags & PMF_CHARGE )
	{
		return false;
	}

	// Check if in air
	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{
		return false;
	}

	// Calculate jump parameters
	switch ( pm->ps->weapon )
	{
		case WP_ALEVEL1:
			// wallwalking (ground surface normal is off more than 45° from Z direction)
			if ( pm->ps->groundEntityNum == ENTITYNUM_WORLD && acosf( pml.groundTrace.plane.normal[ 2 ] ) > M_PI_4 )
			{
				// get jump magnitude
				jumpMagnitude = LEVEL1_WALLPOUNCE_MAGNITUDE;

				// if looking in the direction of the surface, jump in opposite normal direction
				if ( DotProduct( pml.groundTrace.plane.normal, pml.forward ) < 0.0f )
				{
					VectorCopy( pml.groundTrace.plane.normal, jumpDirection );
				}
				// otherwise try to find a trajectory to the surface point the player is looking at
				else
				{
					trace_t  trace;
					vec3_t   traceTarget, endpos, trajDir1, trajDir2;
					vec2_t   trajAngles;
					float    zCorrection;
					bool foundTrajectory;

					VectorMA( pm->ps->origin, 10000.0f, pml.forward, traceTarget );

					pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, traceTarget,
					           pm->ps->clientNum, MASK_SOLID, 0 );

					foundTrajectory = ( trace.fraction < 1.0f );

					// HACK: make sure we don't just tangent if we want to attach to the ceiling
					//       this is done by moving the target point inside a ceiling's surface
					if ( foundTrajectory )
					{
						VectorCopy( trace.endpos, endpos );

						if ( trace.plane.normal[ 2 ] < 0.0f )
						{
							zCorrection = 64.0f * trace.plane.normal[ 2 ];

							if ( pm->debugLevel > 0 )
							{
								Log::Notice( "[PM_CheckPounce] Aiming at ceiling; move target into surface by %.2f\n",
								            zCorrection );
							}

							VectorMA( endpos, zCorrection, trace.plane.normal, endpos );
						}

						if ( pm->debugLevel > 0 )
						{
							Log::Notice( "[PM_CheckPounce] Trajectory target has a distance of %.1f qu\n",
							            Distance( pm->ps->origin, endpos ) );
						}
					}

					// if there is a possible trajectory they come in pairs, use the shorter one
					if ( foundTrajectory &&
					     BG_GetTrajectoryPitch( pm->ps->origin, endpos, jumpMagnitude, pm->ps->gravity,
					                            trajAngles, trajDir1, trajDir2 ) )
					{
						int iter;

						if ( pm->debugLevel > 0 )
						{
							Log::Notice( "[PM_CheckPounce] Found trajectory angles: "
							            "%.1f° ( %.2f, %.2f, %.2f ), %.1f° ( %.2f, %.2f, %.2f )\n",
							            180.0f / M_PI * trajAngles[ 0 ],
							            trajDir1[ 0 ], trajDir1[ 1 ], trajDir1[ 2 ],
							            180.0f / M_PI * trajAngles[ 1 ],
							            trajDir2[ 0 ], trajDir2[ 1 ], trajDir2[ 2 ] );
						}

						if ( trajDir1[ 2 ] < trajDir2[ 2 ] )
						{
							if ( pm->debugLevel > 0 )
							{
								Log::Notice("[PM_CheckPounce] Using angle #1\n");
							}

							VectorCopy( trajDir1, jumpDirection );
						}
						else
						{
							if ( pm->debugLevel > 0 )
							{
								Log::Notice("[PM_CheckPounce] Using angle #2\n");
							}

							VectorCopy( trajDir2, jumpDirection );
						}

						// HACK: make sure we get off the ceiling if jumping to an adjacent wall
						//       this is done by subsequently rotating the jump direction in surface
						//       normal direction until its angle is below a threshold (acos(0.1) ~= 85°)
						for ( iter = 0; DotProduct( jumpDirection, pml.groundTrace.plane.normal ) <= 0.1f; iter++ )
						{
							if ( iter > 10 )
							{
								if ( pm->debugLevel > 0 )
								{
									Log::Notice("[PM_CheckPounce] Giving up adjusting jump direction.\n");
								}

								break;
							}

							VectorMA( jumpDirection, 0.1f, pml.groundTrace.plane.normal, jumpDirection );
							VectorNormalize( jumpDirection );

							if ( pm->debugLevel > 0 )
							{
								Log::Notice("[PM_CheckPounce] Adjusting jump direction to get off the surface: "
								           "( %.2f, %.2f, %.2f )\n",
								           jumpDirection[ 0 ], jumpDirection[ 1 ], jumpDirection[ 2 ] );
							}
						}
					}
					// resort to jumping in view direction
					else
					{
						if ( pm->debugLevel > 0 )
						{
							Log::Notice("[PM_CheckPounce] Failed to find a trajectory\n");
						}

						VectorCopy( pml.forward, jumpDirection );
					}
				}

				// add cooldown
				pm->ps->weaponCharge = LEVEL1_WALLPOUNCE_COOLDOWN;
			}
			// moving foward or standing still
			else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove == 0 ) )
			{
				// get jump direction
				VectorCopy( pml.forward, jumpDirection );
				jumpDirection[ 2 ] = fabsf( jumpDirection[ 2 ] );

				// get pitch towards ground surface
				pitchToGround = M_PI_2 - acosf( DotProduct( pml.groundTrace.plane.normal, jumpDirection ) );

				// get pitch towards XY reference plane
				pitchToRef = M_PI_2 - acosf( DotProduct( up, jumpDirection ) );

				// use the advantageous pitch; allows using an upwards gradiant as a ramp
				pitch = std::min( pitchToGround, pitchToRef );

				// pitches above 45° or below LEVEL1_POUNCE_MINPITCH will result in less than the maximum jump length
				if ( pitch > M_PI_4 )
				{
					pitch = M_PI_4;
				}
				else if ( pitch < LEVEL1_POUNCE_MINPITCH )
				{
					pitch = LEVEL1_POUNCE_MINPITCH;
				}

				// calculate jump magnitude for given pitch so that jump length is fixed
				jumpMagnitude = ( int )sqrtf( LEVEL1_POUNCE_DISTANCE * pm->ps->gravity / sinf( 2.0f * pitch ) );

				// add cooldown
				pm->ps->weaponCharge = LEVEL1_POUNCE_COOLDOWN;
			}
			// going backwards or strafing
			else if ( pm->cmd.forwardmove < 0 || pm->cmd.rightmove != 0 )
			{
				// get jump direction
				if ( pm->cmd.forwardmove < 0 )
				{
					VectorNegate( pml.forward, jumpDirection );
				}
				else if ( pm->cmd.rightmove < 0 )
				{
					VectorNegate( pml.right, jumpDirection );
				}
				else
				{
					VectorCopy( pml.right, jumpDirection );
				}

				jumpDirection[ 2 ] = LEVEL1_SIDEPOUNCE_DIR_Z;

				// get jump magnitude
				jumpMagnitude = LEVEL1_SIDEPOUNCE_MAGNITUDE;

				// add cooldown
				pm->ps->weaponCharge = LEVEL1_SIDEPOUNCE_COOLDOWN;
			}
			// compilers don't get my epic dijkstra-if
			else
			{
				return false;
			}

			break;

		case WP_ALEVEL3:
		case WP_ALEVEL3_UPG:
			if ( pm->ps->weapon == WP_ALEVEL3 )
			{
				jumpMagnitude = pm->ps->weaponCharge
				              * LEVEL3_POUNCE_JUMP_MAG / LEVEL3_POUNCE_TIME;
			}
			else
			{
				jumpMagnitude = pm->ps->weaponCharge
				              * LEVEL3_POUNCE_JUMP_MAG_UPG / LEVEL3_POUNCE_TIME_UPG;
			}

			VectorCopy( pml.forward, jumpDirection );

			// save payload
			pm->pmext->pouncePayload = pm->ps->weaponCharge;

			// reset charge bar
			pm->ps->weaponCharge = 0;

			break;

		default:
			return false;
	}

	// Prepare a simulated jump
	pml.groundPlane = false;
	pml.walking = false;
	pm->ps->pm_flags |= PMF_CHARGE;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	// Jump
	VectorMA( pm->ps->velocity, jumpMagnitude, jumpDirection, pm->ps->velocity );
	PM_AddEvent( EV_JUMP );
	PM_PlayJumpingAnimation();

	// We started to pounce
	return true;
}

/*
=============
PM_CheckWallJump

Used by marauders.
=============
*/
static bool PM_CheckWallJump()
{
	vec3_t  dir, forward, right, movedir, point;
	float   normalFraction = 1.5f;
	float   cmdFraction = 1.0f;
	float   upFraction = 1.5f;
	trace_t trace;

	static const vec3_t  refNormal = { 0.0f, 0.0f, 1.0f };

	if ( !( BG_Class( pm->ps->stats[ STAT_CLASS ] )->abilities & SCA_WALLJUMPER ) )
	{
		return false;
	}

	ProjectPointOnPlane( movedir, pml.forward, refNormal );
	VectorNormalize( movedir );

	if ( pm->cmd.forwardmove < 0 )
	{
		VectorNegate( movedir, movedir );
	}

	//allow strafe transitions
	if ( pm->cmd.rightmove )
	{
		VectorCopy( pml.right, movedir );

		if ( pm->cmd.rightmove < 0 )
		{
			VectorNegate( movedir, movedir );
		}
	}

	//trace into direction we are moving
	VectorMA( pm->ps->origin, 0.25f, movedir, point );
	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum,
	           pm->tracemask, 0 );

	if ( !hitGrippingSurface( trace ) || trace.plane.normal[ 2 ] >= MIN_WALK_NORMAL )
	{
		return false;
	}

	VectorCopy( trace.plane.normal, pm->ps->grapplePoint );

	if ( pm->ps->pm_flags & PMF_RESPAWNED )
	{
		return false; // don't allow jump until all buttons are up
	}

	if ( pm->cmd.upmove < 10 )
	{
		// not holding jump
		return false;
	}

	if ( pm->ps->pm_flags & PMF_TIME_WALLJUMP )
	{
		return false;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD &&
	     pm->ps->grapplePoint[ 2 ] == 1.0f )
	{
		return false;
	}

	pm->ps->pm_flags |= PMF_TIME_WALLJUMP;
	pm->ps->pm_time = 200;

	pml.groundPlane = false; // jumping away
	pml.walking = false;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	ProjectPointOnPlane( forward, pml.forward, pm->ps->grapplePoint );
	ProjectPointOnPlane( right, pml.right, pm->ps->grapplePoint );

	VectorScale( pm->ps->grapplePoint, normalFraction, dir );

	if ( pm->cmd.forwardmove > 0 )
	{
		VectorMA( dir, cmdFraction, forward, dir );
	}
	else if ( pm->cmd.forwardmove < 0 )
	{
		VectorMA( dir, -cmdFraction, forward, dir );
	}

	if ( pm->cmd.rightmove > 0 )
	{
		VectorMA( dir, cmdFraction, right, dir );
	}
	else if ( pm->cmd.rightmove < 0 )
	{
		VectorMA( dir, -cmdFraction, right, dir );
	}

	VectorMA( dir, upFraction, refNormal, dir );
	VectorNormalize( dir );

	VectorMA( pm->ps->velocity, BG_Class( pm->ps->stats[ STAT_CLASS ] )->jumpMagnitude,
	          dir, pm->ps->velocity );

	//for a long run of wall jumps the velocity can get pretty large, this caps it
	if ( VectorLength( pm->ps->velocity ) > LEVEL2_WALLJUMP_MAXSPEED )
	{
		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, LEVEL2_WALLJUMP_MAXSPEED, pm->ps->velocity );
	}

	PM_AddEvent( EV_JUMP );
	PM_PlayJumpingAnimation();

	return true;
}

/*
=============
PM_CheckWallRun

Used by humans.
=============
*/
static bool PM_CheckWallRun()
{
	trace_t trace;

	float jumpMag = BG_Class( pm->ps->stats[ STAT_CLASS ] )->jumpMagnitude;

	if ( !( BG_Class( pm->ps->stats[ STAT_CLASS ] )->abilities & SCA_WALLRUNNER ) )
		return false;

	// can't wallrun when firing jetpack
	if ( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
	{
		return false;
	}

	if ( pm->ps->pm_flags & PMF_RESPAWNED )
		return false; // don't allow jump until all buttons are up

	if ( pm->cmd.upmove < 10 )
		return false; // not holding jump

	if ( pm->ps->pm_flags & PMF_TIME_WALLJUMP )
		return false;

	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove )
		return false;

	vec3_t dir;
	if ( pm->cmd.rightmove )
	{
		VectorCopy( pml.right, dir );
		VectorScale( dir, ( pm->cmd.rightmove < 0 ? -1 : 1 ), dir );
	}
	else
	{
		VectorCopy( pml.forward, dir );
		VectorScale( dir, ( pm->cmd.forwardmove < 0 ? -1 : 1 ), dir );
	}
	vec3_t trace_end;
	VectorMA( pm->ps->origin, 0.25f, dir, trace_end );
	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, trace_end, pm->ps->clientNum, pm->tracemask, 0);

	if ( !hitGrippingSurface( trace ) || trace.plane.normal[ 2 ] >= MIN_WALK_NORMAL )
	{
		return false;
	}

	pm->ps->pm_flags |= PMF_TIME_WALLJUMP;
	pm->ps->pm_time = 200;

	pml.groundPlane = false; // jumping away
	pml.walking = false;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	vec3_t tmp = { 0.f, 0.f, 0.9f };
	VectorAdd( tmp, trace.plane.normal, tmp );
	VectorNormalize( tmp );
	VectorMA( pm->ps->velocity, jumpMag, tmp, pm->ps->velocity );

	PM_AddEvent( EV_JUMP );
	PM_PlayJumpingAnimation();

	return true;
}

/**
 * @brief PM_CheckJetpack
 * @return true if and only if thrust was applied
 */
static bool PM_CheckJetpack()
{
	// do not use jetpack on ladders
	if ( pml.ladder )
	{
		return false;
	}
	static const vec3_t thrustDir = { 0.0f, 0.0f, 1.0f };
	team_t team = static_cast<team_t>( pm->ps->persistant[ PERS_TEAM ] );

	if ( pm->ps->pm_type != PM_NORMAL ||
	     team != TEAM_HUMANS ||
	     !BG_InventoryContainsUpgrade( UP_JETPACK, pm->ps->stats ) )
	{
		pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_ACTIVE;
		pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_WARM;
		pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_ENABLED;

		return false;
	}

	// enable jetpack when in air
	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE &&
	     !( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ENABLED ) )
	{
		if ( pm->debugLevel > 0 )
		{
			Log::Notice( "[PM_CheckJetpack] %sJetpack enabled\n", Color::ToString( Color::Cyan ) );
		}

		pm->ps->stats[ STAT_STATE2 ] |= SS2_JETPACK_ENABLED;
		PM_AddEvent( EV_JETPACK_ENABLE );

		return false;
	}

	// if jump key not held stop active thrust
	if ( pm->cmd.upmove < 10 )
	{
		if ( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
		{
			if ( pm->debugLevel > 0 && pm->cmd.upmove < 10 )
			{
				Log::Notice( "[PM_CheckJetpack] %sJetpack thrust stopped (jump key released)\n", Color::ToString( Color::LtOrange ) );
			}

			pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_ACTIVE;
			PM_AddEvent( EV_JETPACK_STOP );
		}

		return false;
	}

	// sanity check that jetpack is enabled at this point
	if ( !( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ENABLED ) )
	{
		if ( pm->debugLevel > 0 )
		{
			Log::Notice( "[PM_CheckJetpack] %sCan't start jetpack thrust (jetpack not enabled)\n", Color::ToString( Color::Red ) );
		}

		return false;
	}

	// check ignite conditions
	if ( !( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_WARM ) )
	{
		int sideVelocity = sqrtf( pm->ps->velocity[ 0 ] * pm->ps->velocity[ 0 ] +
		                      pm->ps->velocity[ 1 ] * pm->ps->velocity[ 1 ] );

		// we got off ground by jumping and are not yet in free fall, where free fall is defined as
		// (1) fall speed bigger than sideways speed (not strafe jumping)
		// (2) fall speed bigger than jump magnitude (not jumping up and down on solid ground)
		if ( ( pm->ps->pm_flags & PMF_JUMPED ) && !( -pm->ps->velocity[ 2 ] > sideVelocity &&
		     -pm->ps->velocity[ 2 ] > BG_Class( pm->ps->stats[ STAT_CLASS ] )->jumpMagnitude ) )
		{
			// require the jump key to be held since the jump
			if ( !( pm->ps->pm_flags & PMF_JUMP_HELD ) )
			{
				return false;
			}

			// minimum fuel required to start from a jump
			if ( pm->ps->stats[ STAT_FUEL ] < JETPACK_FUEL_LOW )
			{
				return false;
			}

			// wait until at highest spot
			if ( !( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE ) && pm->ps->velocity[ 2 ] > 0.0f )
			{
				return false;
			}
		}

		// use some fuel for ignition
		pm->ps->stats[ STAT_FUEL ] -= JETPACK_FUEL_IGNITE;
		if ( pm->ps->stats[ STAT_FUEL ] < 0 ) pm->ps->stats[ STAT_FUEL ] = 0;

		// ignite
		pm->ps->stats[ STAT_STATE2 ] |= SS2_JETPACK_WARM;
		PM_AddEvent( EV_JETPACK_IGNITE );
	}

	// stop thrusting if completely out of fuel
	if ( pm->ps->stats[ STAT_FUEL ] < JETPACK_FUEL_USAGE )
	{
		if ( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
		{
			if ( pm->debugLevel > 0 )
			{
				Log::Notice( "[PM_CheckJetpack] %sJetpack thrust stopped (out of fuel)\n", Color::ToString( Color::LtOrange ) );
			}

			pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_ACTIVE;
			PM_AddEvent( EV_JETPACK_STOP );
		}

		return false;
	}

	// start thrusting if possible
	if ( !( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE ) )
	{
		// minimum fuel required
		if ( pm->ps->stats[ STAT_FUEL ] < JETPACK_FUEL_STOP )
		{
			return false;
		}

		if ( pm->debugLevel > 0 )
		{
			Log::Notice( "[PM_CheckJetpack] %sJetpack thrust started\n", Color::ToString( Color::Green ) );
		}

		pm->ps->stats[ STAT_STATE2 ] |= SS2_JETPACK_ACTIVE;
		PM_AddEvent( EV_JETPACK_START );
	}

	// clear the jumped flag as the reason we are in air now is the jetpack
	pm->ps->pm_flags &= ~PMF_JUMPED;

	// thrust
	PM_Accelerate( thrustDir, JETPACK_TARGETSPEED, JETPACK_ACCELERATION );

	// remove fuel
	pm->ps->stats[ STAT_FUEL ] -= pml.msec * JETPACK_FUEL_USAGE;
	if ( pm->ps->stats[ STAT_FUEL ] < 0 ) pm->ps->stats[ STAT_FUEL ] = 0;

	return true;
}

/**
 * @brief Restores jetpack fuel
 * @return true if and only if fuel has been restored
 */
static bool PM_CheckJetpackRestoreFuel()
{
	// don't restore fuel when full or jetpack active
	if ( pm->ps->stats[ STAT_FUEL ] == JETPACK_FUEL_MAX ||
	     pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
	{
		return false;
	}

	pm->ps->stats[ STAT_FUEL ] += pml.msec * JETPACK_FUEL_RESTORE;

	if ( pm->ps->stats[ STAT_FUEL ] > JETPACK_FUEL_MAX )
	{
		pm->ps->stats[ STAT_FUEL ] = JETPACK_FUEL_MAX;
	}

	return true;
}

/**
 * @brief Disables the jetpack. Without force, the call can get ignored based on previous velocity.
 */
static void PM_LandJetpack( bool force )
{
	float angle, sideVelocity;

	// when low on fuel, always force a landing
	if ( pm->ps->stats[ STAT_FUEL ] < JETPACK_FUEL_LOW )
	{
		force = true;
	}

	sideVelocity = sqrtf( pml.previous_velocity[ 0 ] * pml.previous_velocity[ 0 ] +
	                      pml.previous_velocity[ 1 ] * pml.previous_velocity[ 1 ] );

	angle = atan2f( -pml.previous_velocity[ 2 ], sideVelocity );

	// allow the player to jump instead of land for some impacts
	if ( !force )
	{
		if ( angle > 0.0f && angle < M_PI_4 ) // 45°
		{
			if ( pm->debugLevel > 0 )
			{
				Log::Notice( "[PM_LandJetpack] Landing ignored (hit surface at %.0f°)\n", RAD2DEG( angle ) );
			}

			return;
		}
	}

	if ( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
	{
		if ( pm->debugLevel > 0 )
		{
			Log::Notice( "[PM_LandJetpack] %sJetpack thrust stopped (hit surface at %.0f°)%s\n",
			            Color::ToString( Color::LtOrange ),
			            RAD2DEG( angle ),
			            force ? " ^1(FORCED)" : "" );
		}

		pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_ACTIVE;

		PM_AddEvent( EV_JETPACK_STOP );

		// HACK: mark the jump key held so there is no immediate jump on landing
		pm->ps->pm_flags |= PMF_JUMP_HELD;
	}

	if ( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ENABLED )
	{
		if ( pm->debugLevel > 0 )
		{
			Log::Notice( "[PM_LandJetpack] %sJetpack disabled (hit surface at %.0f°)%s\n",
			            Color::ToString( Color::Yellow ),
			            RAD2DEG( angle ),
			            force ? " ^1(FORCED)" : "" );
		}

		pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_WARM;
		pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_ENABLED;

		PM_AddEvent( EV_JETPACK_DISABLE );
	}
}

static bool PM_CheckJump()
{
	// can't jump while in air
	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{
		return false;
	}

	// check if holding jump key
	if ( pm->cmd.upmove < 10 )
	{
		return false;
	}

	class_t pcl = static_cast<class_t>( pm->ps->stats[ STAT_CLASS ] );
	classAttributes_t const* attr = BG_Class( pcl );
	team_t team = static_cast<team_t>( pm->ps->persistant[ PERS_TEAM ] );
	// needs jump ability
	if ( attr->jumpMagnitude <= 0.0f )
	{
		return false;
	}

	// can't jump and pounce at the same time
	// TODO: This prevents jumps in an unintuitive manner, since the charge
	//       meter has nothing to do with the land time.
	if ( ( pcl == PCL_ALIEN_LEVEL3 || pcl == PCL_ALIEN_LEVEL3_UPG ) &&
	     pm->ps->weaponCharge > 0 )
	{
		return false;
	}

	// can't jump and charge at the same time
	if ( pcl == PCL_ALIEN_LEVEL4 && pm->ps->weaponCharge > 0 )
	{
		return false;
	}

	// don't allow jump until all buttons are up (?)
	if ( pm->ps->pm_flags & PMF_RESPAWNED )
	{
		return false;
	}

	// can't jump whilst grabbed
	if ( pm->ps->pm_type == PM_GRABBED )
	{
		return false;
	}

	int staminaJumpCost = attr->staminaJumpCost;
	bool jetpackJump = false;

	// humans need stamina or jetpack to jump
	if ( ( team == TEAM_HUMANS ) &&
	     ( pm->ps->stats[ STAT_STAMINA ] < staminaJumpCost ) )
	{
		// use jetpack instead of stamina to take off
		if ( BG_InventoryContainsUpgrade( UP_JETPACK, pm->ps->stats ) &&
		     pm->ps->stats[ STAT_FUEL ] > JETPACK_FUEL_LOW )
		{
			jetpackJump = true;
		}
		else
		{
			return false;
		}
	}

	// take some stamina off
	if ( !jetpackJump && team == TEAM_HUMANS )
	{
		pm->ps->stats[ STAT_STAMINA ] -= staminaJumpCost;
	}

	// go into jump mode
	pml.groundPlane = false;
	pml.walking     = false;
	pm->ps->pm_flags |= PMF_JUMP_HELD;
	pm->ps->pm_flags |= PMF_JUMPED;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	// don't allow walljump for a short while after jumping from the ground
	// TODO: There was an issue about this potentially having side effects.
	if ( BG_ClassHasAbility( pcl, SCA_WALLJUMPER ) || BG_ClassHasAbility( pcl, SCA_WALLRUNNER ) )
	{
		pm->ps->pm_flags |= PMF_TIME_WALLJUMP;
		pm->ps->pm_time = 200;
	}

	// jump in surface normal direction
	glm::vec3 normal = BG_GetClientNormal( pm->ps );

	// retrieve jump magnitude
	float magnitude = attr->jumpMagnitude;

	// if jetpack is active or being used for the jump, scale down jump magnitude
	if ( jetpackJump || pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
	{
		if ( pm->debugLevel > 0 )
		{
			Log::Notice( "[PM_CheckJump] Using jetpack: Decreasing jump magnitude to %.0f%%\n",
			            JETPACK_JUMPMAG_REDUCTION * 100.0f );
		}

		magnitude *= JETPACK_JUMPMAG_REDUCTION;
	}

	// sanity clip velocity Z
	if ( pm->ps->velocity[ 2 ] < 0.0f )
	{
		pm->ps->velocity[ 2 ] = 0.0f;
	}

	VectorMA( pm->ps->velocity, magnitude, &normal[0], pm->ps->velocity );

	PM_AddEvent( EV_JUMP );
	PM_PlayJumpingAnimation();

	return true;
}

static bool PM_CheckWaterJump()
{
	vec3_t spot;
	int    cont;
	vec3_t flatforward;

	if ( pm->ps->pm_time )
	{
		return false;
	}

	// check for water jump
	if ( pm->waterlevel != 2 )
	{
		return false;
	}

	flatforward[ 0 ] = pml.forward[ 0 ];
	flatforward[ 1 ] = pml.forward[ 1 ];
	flatforward[ 2 ] = 0;
	VectorNormalize( flatforward );

	VectorMA( pm->ps->origin, 30, flatforward, spot );
	spot[ 2 ] += 4;
	cont = pm->pointcontents( spot, pm->ps->clientNum );

	if ( !( cont & CONTENTS_SOLID ) )
	{
		return false;
	}

	spot[ 2 ] += 16;
	cont = pm->pointcontents( spot, pm->ps->clientNum );

	if ( cont )
	{
		return false;
	}

	// jump out of water
	VectorScale( pml.forward, 200, pm->ps->velocity );
	pm->ps->velocity[ 2 ] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return true;
}

//============================================================================

/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove()
{
	// waterjump has no control, but falls

	PM_StepSlideMove( true, false );

	pm->ps->velocity[ 2 ] -= pm->ps->gravity * pml.frametime;

	if ( pm->ps->velocity[ 2 ] < 0 )
	{
		// cancel as soon as we are falling down again
		pm->ps->pm_flags &= ~PMF_ALL_TIMES;
		pm->ps->pm_time = 0;
	}
}

/*
===================
PM_WaterMove

===================
*/
static void PM_WaterMove()
{
	// if pouncing, stop
	PM_CheckWaterPounce();

	if ( PM_CheckWaterJump() )
	{
		PM_WaterJumpMove();
		return;
	}

#if 0

	// jump = head for surface
	if ( pm->cmd.upmove >= 10 )
	{
		if ( pm->ps->velocity[ 2 ] > -300 )
		{
			if ( pm->watertype == CONTENTS_WATER )
			{
				pm->ps->velocity[ 2 ] = 100;
			}
			else if ( pm->watertype == CONTENTS_SLIME )
			{
				pm->ps->velocity[ 2 ] = 80;
			}
			else
			{
				pm->ps->velocity[ 2 ] = 50;
			}
		}
	}

#endif
	PM_Friction();

	float scale = PM_CmdScale( &pm->cmd, true );

	//
	// user intentions
	//

	vec3_t wishvel;
	for ( int i = 0; i < 3; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * pm->cmd.forwardmove
		             + pml.right[ i ]   * pm->cmd.rightmove;
	}
	wishvel[ 2 ] += pm->cmd.upmove;

	vec3_t wishdir;
	VectorCopy( wishvel, wishdir );
	float wishspeed = VectorNormalize( wishdir ) * scale;

	if ( wishspeed > pm->ps->speed * pm_swimScale )
	{
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate( wishdir, wishspeed, pm_wateraccelerate );

	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 )
	{
		float vel = VectorLength( pm->ps->velocity );
		// slide along the ground plane
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
	}

	PM_SlideMove( false );
}

/**
 * @brief Used for both free spectating and noclip mode
 */
static void PM_GhostMove( bool noclip )
{
	PM_Friction();

	float scale = PM_CmdScale( &pm->cmd, true );

	vec3_t wishvel;
	for ( int i = 0; i < 3; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * pm->cmd.forwardmove +
		               pml.right[ i ]   * pm->cmd.rightmove;
	}
	wishvel[ 2 ] += pm->cmd.upmove;

	vec3_t wishdir;
	VectorCopy( wishvel, wishdir );
	float wishspeed = VectorNormalize( wishdir ) * scale;

	PM_Accelerate( wishdir, wishspeed, pm_flyaccelerate );

	if ( noclip )
	{
		VectorMA( pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin );
	}
	else
	{
		PM_StepSlideMove( false, false );
	}
}

/*
===================
PM_AirMove

===================
*/
static void PM_AirMove()
{
	PM_CheckWallJump();
	PM_CheckWallRun();
	PM_CheckJetpack();

	PM_Friction();

	float fmove = pm->cmd.forwardmove;
	float smove = pm->cmd.rightmove;

	usercmd_t cmd = pm->cmd;
	float scale = PM_CmdScale( &cmd, false );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[ 2 ] = 0;
	pml.right[ 2 ] = 0;
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	vec3_t wishvel;
	wishvel[ 0 ] = pml.forward[ 0 ] * fmove + pml.right[ 0 ] * smove;
	wishvel[ 1 ] = pml.forward[ 1 ] * fmove + pml.right[ 1 ] * smove;
	wishvel[ 2 ] = 0;

	vec3_t wishdir;
	VectorCopy( wishvel, wishdir );
	float wishspeed = VectorNormalize( wishdir ) * scale;

	// not on ground, so little effect on velocity
	PM_Accelerate( wishdir, wishspeed,
	               BG_Class( pm->ps->stats[ STAT_CLASS ] )->airAcceleration );

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane )
	{
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );
	}

	PM_StepSlideMove( true, false );
}

/*
===================
PM_ClimbMove

===================
*/
static void PM_ClimbMove()
{
	PM_Friction();

	float fmove = pm->cmd.forwardmove;
	float smove = pm->cmd.rightmove;

	usercmd_t cmd = pm->cmd;
	float scale = PM_CmdScale( &cmd, false );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity( pml.forward, pml.groundTrace.plane.normal, pml.forward );
	PM_ClipVelocity( pml.right, pml.groundTrace.plane.normal, pml.right );
	//
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	vec3_t wishvel;
	for ( int i = 0; i < 3; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * fmove
		             + pml.right[ i ] * smove;
	}

	// when going up or down slopes the wish velocity should Not be zero
//  wishvel[2] = 0;

	vec3_t wishdir;
	VectorCopy( wishvel, wishdir );
	float wishspeed = VectorNormalize( wishdir ) * scale;

	// clamp the speed lower if ducking
	if ( pm->ps->pm_flags & PMF_DUCKED )
	{
		if ( wishspeed > pm->ps->speed * pm_duckScale )
		{
			wishspeed = pm->ps->speed * pm_duckScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel )
	{
		float waterScale;

		waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - ( 1.0 - pm_swimScale ) * waterScale;

		if ( wishspeed > pm->ps->speed * waterScale )
		{
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	Slide( wishdir, wishspeed, *pm->ps );

	float vel = VectorLength( pm->ps->velocity );

	// slide along the ground plane
	PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

	// don't decrease velocity when going up or down a slope
	VectorNormalize( pm->ps->velocity );
	VectorScale( pm->ps->velocity, vel, pm->ps->velocity );

	// don't do anything if standing still
	if ( !pm->ps->velocity[ 0 ] && !pm->ps->velocity[ 1 ] && !pm->ps->velocity[ 2 ] )
	{
		return;
	}

	PM_StepSlideMove( false, false );
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove()
{
	// Slide
	if ( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_SLIDER )
		&& pm->cmd.upmove < 0
		&& VectorLength(pm->ps->velocity) > HUMAN_SLIDE_THRESHOLD )
	{
		pm->ps->stats[ STAT_STATE ] |= SS_SLIDING;
		PM_StepSlideMove( false, true );
		PM_Friction();
		return;
	}
	pm->ps->stats[ STAT_STATE ] &= ~SS_SLIDING;

	// if PM_Land didn't stop the jetpack (e.g. to allow for a jump) but we didn't get away
	// from the ground, stop it now
	PM_LandJetpack( true );

	PM_CheckCharge();

	PM_Friction();

	float fmove = pm->cmd.forwardmove;
	float smove = pm->cmd.rightmove;

	usercmd_t cmd = pm->cmd;
	float scale = PM_CmdScale( &cmd, false );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[ 2 ] = 0;
	pml.right[ 2 ] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity( pml.forward, pml.groundTrace.plane.normal, pml.forward );
	PM_ClipVelocity( pml.right, pml.groundTrace.plane.normal, pml.right );
	//
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	vec3_t wishvel;
	for ( int i = 0; i < 3; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * fmove
		             + pml.right[ i ] * smove;
	}

	// when going up or down slopes the wish velocity should Not be zero
//  wishvel[2] = 0;

	vec3_t wishdir;
	VectorCopy( wishvel, wishdir );
	float wishspeed = VectorNormalize( wishdir ) * scale;

	// clamp the speed lower if ducking
	if ( pm->ps->pm_flags & PMF_DUCKED )
	{
		if ( wishspeed > pm->ps->speed * pm_duckScale )
		{
			wishspeed = pm->ps->speed * pm_duckScale;
		}
	}

	// clamp the speed lower if wading or walking on the bottom
	if ( pm->waterlevel )
	{
		float waterScale;

		waterScale = pm->waterlevel / 3.0;
		waterScale = 1.0 - ( 1.0 - pm_swimScale ) * waterScale;

		if ( wishspeed > pm->ps->speed * waterScale )
		{
			wishspeed = pm->ps->speed * waterScale;
		}
	}

	Slide( wishdir, wishspeed, *pm->ps );

	// slide along the ground plane
	PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

	// don't do anything if standing still
	if ( !pm->ps->velocity[ 0 ] && !pm->ps->velocity[ 1 ] )
	{
		return;
	}

	PM_StepSlideMove( false, false );
}

/*
===================
PM_LadderMove

Basically a rip of PM_WaterMove with a few changes
===================
*/
static void PM_LadderMove()
{
	PM_Friction();

	float scale = PM_CmdScale( &pm->cmd, true );

	vec3_t wishvel;
	for ( int i = 0; i < 3; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * pm->cmd.forwardmove
		             + pml.right[ i ]   * pm->cmd.rightmove;
	}
	wishvel[ 2 ] += pm->cmd.upmove;

	vec3_t wishdir;
	VectorCopy( wishvel, wishdir );
	float wishspeed = VectorNormalize( wishdir ) * scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	//slanty ladders
	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0.0f )
	{
		float vel = VectorLength( pm->ps->velocity );

		// slide along the ground plane
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
	}

	PM_SlideMove( false );
}

/*
=============
PM_CheckLadder

Check to see if the player is on a ladder or not
=============
*/
static void PM_CheckLadder()
{
	vec3_t  forward, end;
	trace_t trace;

	//test if class can use ladders
	if ( !BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_CANUSELADDERS ) )
	{
		pml.ladder = false;
		return;
	}

	VectorCopy( pml.forward, forward );
	forward[ 2 ] = 0.0f;

	VectorMA( pm->ps->origin, 1.0f, forward, end );

	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum,
	           MASK_PLAYERSOLID, 0 );

	pml.ladder = ( trace.fraction < 1.0f ) && ( trace.surfaceFlags & SURF_LADDER );
}

/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove()
{
	if ( !pml.walking )
	{
		return;
	}

	// extra friction
	float forward = VectorLength( pm->ps->velocity );
	forward -= 20;

	if ( forward <= 0 )
	{
		VectorClear( pm->ps->velocity );
	}
	else
	{
		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, forward, pm->ps->velocity );
	}
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number appropriate for the groundsurface
================
*/
static int PM_FootstepForSurface()
{
	if ( pm->ps->stats[ STAT_STATE ] & SS_CREEPSLOWED )
	{
		return EV_FOOTSTEP_SQUELCH;
	}

	if ( pml.groundTrace.surfaceFlags & SURF_NOSTEPS )
	{
		return 0;
	}

	if ( pml.groundTrace.surfaceFlags & SURF_METAL )
	{
		return EV_FOOTSTEP_METAL;
	}

	return EV_FOOTSTEP;
}

/*
=================
PM_Land

Play landing animation
=================
*/
static void PM_Land()
{
	PM_LandJetpack( false ); // don't force a stop, sometimes we can push off with a jump

	// decide which landing animation to use
	bool backward = pm->ps->pm_flags & PMF_BACKWARDS_JUMP;
	if ( IsSegmentedModel( pm->ps ) )
	{
		PM_ForceLegsAnim( backward ? LEGS_LANDB : LEGS_LAND );
		pm->ps->legsTimer = TIMER_LAND;
	}
	else
	{
		PM_ForceLegsAnim( backward ? NSPA_LANDBACK : NSPA_LAND );
		pm->ps->torsoTimer = TIMER_LAND; //this is weird, but I'm just refactoring here.
	}

	// potential jump ended
	pm->ps->pm_flags &= ~(PMF_JUMPED | PMF_BACKWARDS_JUMP);
}

/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand()
{
	float delta;
	float dist;
	float vel, acc;
	float t;
	float a, b, c, den;

	// calculate the exact velocity on landing
	dist = pm->ps->origin[ 2 ] - pml.previous_origin[ 2 ];
	vel = pml.previous_velocity[ 2 ];
	acc = -pm->ps->gravity;

	a = acc / 2;
	b = vel;
	c = -dist;

	den = b * b - 4 * a * c;

	if ( den < 0 )
	{
		return;
	}

	t = ( -b - sqrtf( den ) ) / ( 2 * a );

	delta = vel + t * acc;
	delta = delta * delta * 0.0001f;

	// if underwater, no damages, if standing on water,
	// reduce them.
	switch ( pm->waterlevel )
	{
		case 3: // never take falling damage if completely underwater
			return;
		case 2:
			delta *= 0.25;
			break;
		case 1:
			delta *= 0.5;
			break;
	}

	if ( delta < 1 )
	{
		return;
	}

	// create a local entity event to play the sound

	// start footstep cycle over
	pm->ps->bobCycle = 0;

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if (  pml.groundTrace.surfaceFlags & SURF_NODAMAGE )
	{
		return;
	}

	pm->ps->stats[ STAT_FALLDIST ] = delta;

	if ( PM_Live( pm->ps->pm_type ) )
	{
		if ( delta > AVG_FALL_DISTANCE )
		{
			PM_AddEvent( EV_FALL_FAR );
		}
		else if ( delta > MIN_FALL_DISTANCE )
		{
			PM_AddEvent( EV_FALL_MEDIUM );
		}
		else if ( delta > 7 )
		{
			PM_AddEvent( EV_FALL_SHORT );
		}
		else
		{
			PM_AddEvent( PM_FootstepForSurface() );
		}
	}
}

/*
=============
PM_CorrectAllSolid
=============
*/
static int PM_CorrectAllSolid( trace_t *trace )
{
	int    i, j, k;
	vec3_t point;

	if ( pm->debugLevel > 1 )
	{
		Log::Notice( "%i:allsolid\n", c_pmove );
	}

	// jitter around
	for ( i = -1; i <= 1; i++ )
	{
		for ( j = -1; j <= 1; j++ )
		{
			for ( k = -1; k <= 1; k++ )
			{
				VectorCopy( pm->ps->origin, point );
				point[ 0 ] += ( float ) i;
				point[ 1 ] += ( float ) j;
				point[ 2 ] += ( float ) k;
				pm->trace( trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum,
				           pm->tracemask, 0 );

				if ( !trace->allsolid )
				{
					point[ 0 ] = pm->ps->origin[ 0 ];
					point[ 1 ] = pm->ps->origin[ 1 ];
					point[ 2 ] = pm->ps->origin[ 2 ] - 0.25;

					pm->trace( trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum,
					           pm->tracemask, 0 );
					pml.groundTrace = *trace;
					return true;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = false;
	pml.walking = false;

	return false;
}

/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed()
{
	trace_t trace;
	vec3_t  point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
	{
		// we just transitioned into freefall
		if ( pm->debugLevel > 1 )
		{
			Log::Notice( "%i:lift\n", c_pmove );
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[ 2 ] -= 64.0f;

		pm->trace( &trace, pm->ps->origin, nullptr, nullptr, point, pm->ps->clientNum, pm->tracemask, 0 );

		if ( trace.fraction == 1.0f )
		{
			PM_PlayJumpingAnimation();
		}
	}

	if ( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_TAKESFALLDAMAGE ) )
	{
		if ( pm->ps->velocity[ 2 ] < FALLING_THRESHOLD && pml.previous_velocity[ 2 ] >= FALLING_THRESHOLD )
		{
			PM_AddEvent( EV_FALLING );
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = false;
	pml.walking = false;
}

/*
=============
PM_GroundClimbTrace
=============
*/
// ATP - Attachpoints for wallwalking
// order is important!
enum attachPoint_t {
	GCT_ATP_MOVEDIRECTION,
	GCT_ATP_GROUND,
	GCT_ATP_STEPMOVE,
	GCT_ATP_UNDERNEATH,
	GCT_ATP_CEILING,
	GCT_ATP_FALLBACK,
	NUM_GCT_ATP
};

static void PM_GroundClimbTrace()
{
	vec3_t      surfNormal, moveDir, lookDir, point, velocityDir;
	vec3_t      toAngles, surfAngles;
	trace_t     trace;
	const float eps = 0.000001f;

	//used for delta correction
	vec3_t      traceCROSSsurf, traceCROSSref, surfCROSSref;
	float       traceDOTsurf, traceDOTref, surfDOTref, rTtDOTrTsTt;
	float       traceANGsurf, traceANGref, surfANGref;
	vec3_t      refTOtrace, refTOsurfTOtrace, tempVec;
	int         rTtANGrTsTt;
	float       ldDOTtCs, d;
	vec3_t      abc;

	static const vec3_t refNormal = { 0.0f, 0.0f, 1.0f };      // really the floor's normal. refactor?
	static const vec3_t ceilingNormal = { 0.0f, 0.0f, -1.0f };
	static const vec3_t horizontal = { 1.0f, 0.0f, 0.0f };     // arbitrary vector orthogonal to refNormal

	BG_GetClientNormal( pm->ps, surfNormal );

	// construct a vector which reflects the direction the player is looking at
	// with respect to the surface normal
	ProjectPointOnPlane( moveDir, pml.forward, surfNormal );
	VectorNormalize( moveDir );

	VectorCopy( moveDir, lookDir );

	if ( pm->cmd.forwardmove < 0 )
	{
		VectorNegate( moveDir, moveDir );
	}

	// allow strafe transitions
	if ( pm->cmd.rightmove )
	{
		VectorCopy( pml.right, moveDir );

		if ( pm->cmd.rightmove < 0 )
		{
			VectorNegate( moveDir, moveDir );
		}
	}

	// get direction of velocity
	VectorCopy( pm->ps->velocity, velocityDir );
	VectorNormalize( velocityDir );

	// try to attach to a surface
	for ( int i = GCT_ATP_MOVEDIRECTION; i <= NUM_GCT_ATP; i++ )
	{
		attachPoint_t atp = static_cast<attachPoint_t>( i );
		switch ( atp )
		{
			case GCT_ATP_MOVEDIRECTION:
				// we are going to step this frame so skip the transition test
				if ( PM_PredictStepMove() )
				{
					continue;
				}

				// trace into direction we are moving
				VectorMA( pm->ps->origin, 0.25f, moveDir, point );
				pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum,
				           pm->tracemask, 0 );

				break;

			case GCT_ATP_GROUND:
				// trace straight down onto "ground" surface
				// mask out CONTENTS_BODY to not hit other players and avoid the camera flipping out
				// when wallwalkers touch
				VectorMA( pm->ps->origin, -0.25f, surfNormal, point );
				pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum,
				           pm->tracemask, CONTENTS_BODY );

				break;

			case GCT_ATP_STEPMOVE:
				if ( pml.groundPlane && PM_PredictStepMove() )
				{
					// step down
					VectorMA( pm->ps->origin, -STEPSIZE, surfNormal, point );
					pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum,
					           pm->tracemask, 0 );
				}
				else
				{
					continue;
				}

				break;

			case GCT_ATP_UNDERNEATH:
				// trace "underneath" BBOX so we can traverse angles > 180deg
				if ( pml.groundPlane )
				{
					VectorMA( pm->ps->origin, -16.0f, surfNormal, point );
					VectorMA( point, -16.0f, moveDir, point );
					pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum,
					           pm->tracemask, 0 );
				}
				else
				{
					continue;
				}

				break;

			case GCT_ATP_CEILING:
				// attach to the ceiling if we get close enough during a jump that goes roughly upwards
				if ( velocityDir[ 2 ] > 0.2f ) // acosf( 0.2f ) ~= 80°
				{
					VectorMA( pm->ps->origin, -16.0f, ceilingNormal, point );
					pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum,
					           pm->tracemask, CONTENTS_BODY );
					break;
				}
				else
				{
					continue;
				}

			case GCT_ATP_FALLBACK:
				// fall back so we don't have to modify PM_GroundTrace too much
				VectorCopy( pm->ps->origin, point );
				point[ 2 ] = pm->ps->origin[ 2 ] - 0.25f;
				pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum,
				           pm->tracemask, 0 );

				break;
			case NUM_GCT_ATP: // WARNING: this is actually reached, for example when jumping while wallwalking
				break;
		}

		// check if we hit something
		if ( hitGrippingSurface( trace ) // TODO I smell a bug on next line (could certainly be simplified, too!)
				&& !( trace.entityNum != ENTITYNUM_WORLD && atp != GCT_ATP_CEILING ) )
		{
			// check if we attached to a new surface (?)
			if ( atp == GCT_ATP_STEPMOVE || atp == GCT_ATP_UNDERNEATH || atp == GCT_ATP_CEILING )
			{
				// add step event if necessary
				if ( atp == GCT_ATP_STEPMOVE )
				{
					PM_StepEvent( pm->ps->origin, trace.endpos, surfNormal );
				}

				// snap our origin to the new surface
				VectorCopy( trace.endpos, pm->ps->origin );
			}

			CrossProduct( trace.plane.normal, surfNormal, traceCROSSsurf );
			VectorNormalize( traceCROSSsurf );

			CrossProduct( trace.plane.normal, refNormal, traceCROSSref );
			VectorNormalize( traceCROSSref );

			CrossProduct( surfNormal, refNormal, surfCROSSref );
			VectorNormalize( surfCROSSref );

			// calculate angle between surf and trace
			traceDOTsurf = DotProduct( trace.plane.normal, surfNormal );
			traceANGsurf = RAD2DEG( acosf( traceDOTsurf ) );

			if ( traceANGsurf > 180.0f )
			{
				traceANGsurf -= 180.0f;
			}

			// calculate angle between trace and ref
			traceDOTref = DotProduct( trace.plane.normal, refNormal );
			traceANGref = RAD2DEG( acosf( traceDOTref ) );

			if ( traceANGref > 180.0f )
			{
				traceANGref -= 180.0f;
			}

			// calculate angle between surf and ref
			surfDOTref = DotProduct( surfNormal, refNormal );
			surfANGref = RAD2DEG( acosf( surfDOTref ) );

			if ( surfANGref > 180.0f )
			{
				surfANGref -= 180.0f;
			}

			// if the trace result and old surface normal are different then we must have transided to a new surface
			if ( !VectorCompareEpsilon( trace.plane.normal, surfNormal, eps ) )
			{
				// if the trace result or the old vector is not the floor or ceiling correct the YAW angle
				if ( !VectorCompareEpsilon( trace.plane.normal, refNormal, eps ) &&
				     !VectorCompareEpsilon( surfNormal, refNormal, eps ) &&
				     !VectorCompareEpsilon( trace.plane.normal, ceilingNormal, eps ) &&
				     !VectorCompareEpsilon( surfNormal, ceilingNormal, eps ) )
				{
					//behold the evil mindfuck from hell
					//it has fucked mind like nothing has fucked mind before

					//calculate reference rotated through to trace plane
					RotatePointAroundVector( refTOtrace, traceCROSSref, horizontal, -traceANGref );

					//calculate reference rotated through to surf plane then to trace plane
					RotatePointAroundVector( tempVec, surfCROSSref, horizontal, -surfANGref );
					RotatePointAroundVector( refTOsurfTOtrace, traceCROSSsurf, tempVec, -traceANGsurf );

					//calculate angle between refTOtrace and refTOsurfTOtrace
					rTtDOTrTsTt = DotProduct( refTOtrace, refTOsurfTOtrace );
					rTtANGrTsTt = ANGLE2SHORT( RAD2DEG( acosf( rTtDOTrTsTt ) ) );

					if ( rTtANGrTsTt > 32768 )
					{
						rTtANGrTsTt -= 32768;
					}

					CrossProduct( refTOtrace, refTOsurfTOtrace, tempVec );
					VectorNormalize( tempVec );

					if ( DotProduct( trace.plane.normal, tempVec ) > 0.0f )
					{
						rTtANGrTsTt = -rTtANGrTsTt;
					}

					//phew! - correct the angle
					pm->ps->delta_angles[ YAW ] -= rTtANGrTsTt;
				}

				// construct a plane dividing the surf and trace normals
				CrossProduct( traceCROSSsurf, surfNormal, abc );
				VectorNormalize( abc );
				d = DotProduct( abc, pm->ps->origin );

				// construct a point representing where the player is looking
				VectorAdd( pm->ps->origin, lookDir, point );

				// check whether point is on one side of the plane, if so invert the correction angle
				if ( ( abc[ 0 ] * point[ 0 ] + abc[ 1 ] * point[ 1 ] + abc[ 2 ] * point[ 2 ] - d ) > 0 )
				{
					traceANGsurf = -traceANGsurf;
				}

				// find the . product of the lookdir and traceCROSSsurf
				if ( ( ldDOTtCs = DotProduct( lookDir, traceCROSSsurf ) ) < 0.0f )
				{
					VectorInverse( traceCROSSsurf );
					ldDOTtCs = DotProduct( lookDir, traceCROSSsurf );
				}

				// set the correction angle
				traceANGsurf *= 1.0f - ldDOTtCs;

				if ( !( pm->ps->persistant[ PERS_STATE ] & PS_WALLCLIMBINGFOLLOW ) )
				{
					//correct the angle
					pm->ps->delta_angles[ PITCH ] -= ANGLE2SHORT( traceANGsurf );
				}

				// transition to ceiling
				// calculate axis for subsequent viewangle rotations
				if ( VectorCompareEpsilon( trace.plane.normal, ceilingNormal, eps ) )
				{
					if ( fabsf( DotProduct( surfNormal, trace.plane.normal ) ) < ( 1.0f - eps ) )
					{
						// we had a smooth transition, rotate along old surface x new surface
						CrossProduct( surfNormal, trace.plane.normal, pm->ps->grapplePoint );
						VectorNormalize( pm->ps->grapplePoint ); // necessary?
					}
					else
					{
						// rotate view around itself
						VectorCopy( pml.forward, pm->ps->grapplePoint );
						pm->ps->grapplePoint[ 2 ] = 0.0f;

						// sanity check grapplePoint, use an arbitrary axis as fallback
						if ( VectorLength( pm->ps->grapplePoint ) < eps )
						{
							VectorCopy( horizontal, pm->ps->grapplePoint );
						}
						else
						{
							VectorNormalize( pm->ps->grapplePoint );
						}
					}

					// we are now on the ceiling
					pm->ps->eFlags |= EF_WALLCLIMBCEILING;
				}

				// transition from ceiling
				// we need to do some different angle correction here because grapplePoint is a rotation axis
				if ( VectorCompareEpsilon( surfNormal, ceilingNormal, eps ) )
				{
					vectoangles( trace.plane.normal, toAngles );
					vectoangles( pm->ps->grapplePoint, surfAngles );

					pm->ps->delta_angles[ 1 ] -= ANGLE2SHORT( ( ( surfAngles[ 1 ] - toAngles[ 1 ] ) * 2 ) - 180.0f );
				}
			}

			// we have ground
			pml.groundTrace = trace;

			// we are climbing on a wall
			pm->ps->eFlags |= EF_WALLCLIMB;

			// if we're not at the ceiling, set grapplePoint to be a surface normal
			if ( !VectorCompareEpsilon( trace.plane.normal, ceilingNormal, eps ) )
			{
				VectorCopy( trace.plane.normal, pm->ps->grapplePoint );
				pm->ps->eFlags &= ~EF_WALLCLIMBCEILING;
			}

			// we hit something, so stop searching for a surface to attach to
			break;
		}
		else if ( trace.allsolid )
		{
			// do something corrective if the trace starts in a solid
			if ( !PM_CorrectAllSolid( &trace ) )
			{
				return;
			}
		}
	}

	// check if we are in free wall (the last trace didn't hit)
	if ( trace.fraction >= 1.0f )
	{
		PM_GroundTraceMissed();
		pml.groundPlane = false;
		pml.walking = false;

		// if we were wallwalking the last frame, apply delta correction
		if( pm->ps->eFlags & EF_WALLCLIMB || pm->ps->eFlags & EF_WALLCLIMBCEILING)
		{
			vec3_t forward, rotated, angles;

			if ( pm->ps->eFlags & EF_WALLCLIMBCEILING )
			{
				// we were walking on the ceiling
				VectorSet( surfNormal, 0, 0, -1 );
			}
			else
			{
				// we were walking on a wall or the floor
				VectorCopy( pm->ps->grapplePoint, surfNormal );
			}

			// delta correction is only necessary if our view was upside down
			// the actual problem is with BG_RotateAxis()
			// the rotation applied there causes our view to turn around
			if ( surfNormal[ 2 ] < 0 )
			{
				vec3_t xNormal;

				AngleVectors( pm->ps->viewangles, forward, nullptr, nullptr );

				if ( pm->ps->eFlags & EF_WALLCLIMBCEILING )
				{
					// we were walking on the ceiling
					VectorCopy( pm->ps->grapplePoint, xNormal );
				}
				else
				{
					// we were walking on a wall
					CrossProduct( surfNormal, refNormal, xNormal );
					VectorNormalize( xNormal );
				}

				RotatePointAroundVector( rotated, xNormal, forward, 180 );
				vectoangles( rotated, angles );
				pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT( angles[ YAW ] - pm->ps->viewangles[ YAW ] );
			}

			// we are not wallwalking anymore
			pm->ps->eFlags &= ~EF_WALLCLIMB;
			pm->ps->eFlags &= ~EF_WALLCLIMBCEILING;
		}

		// rotate view back to horizontal (?)
		VectorCopy( refNormal, pm->ps->grapplePoint );

		return;
	}

	// we are on a surface
	pml.groundPlane = true;
	pml.walking = true;

	// hitting solid ground will end a waterjump
	if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
	{
		pm->ps->pm_flags &= ~PMF_TIME_WATERJUMP;
		pm->ps->pm_time = 0;
	}

	pm->ps->groundEntityNum = trace.entityNum;

	PM_AddTouchEnt( trace.entityNum );
}

/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace()
{
	vec3_t  point;
	trace_t trace;

	static const vec3_t refNormal = { 0.0f, 0.0f, 1.0f };

	if ( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_WALLCLIMBER ) )
	{
		if ( pm->ps->persistant[ PERS_STATE ] & PS_WALLCLIMBINGTOGGLE )
		{
			//toggle wall climbing if holding crouch
			if ( pm->cmd.upmove < 0 && !( pm->ps->pm_flags & PMF_CROUCH_HELD ) )
			{
				pm->ps->stats[ STAT_STATE ] ^= SS_WALLCLIMBING;
				pm->ps->pm_flags |= PMF_CROUCH_HELD;
			}
			else if ( pm->cmd.upmove >= 0 )
			{
				pm->ps->pm_flags &= ~PMF_CROUCH_HELD;
			}
		}
		else
		{
			if ( pm->cmd.upmove < 0 )
			{
				pm->ps->stats[ STAT_STATE ] |= SS_WALLCLIMBING;
			}
			else
			{
				pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBING;
			}
		}

		if ( pm->ps->pm_type == PM_DEAD )
		{
			pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBING;
		}

		if ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING )
		{
			PM_GroundClimbTrace();
			return;
		}

		//just transitioned from ceiling to floor... apply delta correction
		if( pm->ps->eFlags & EF_WALLCLIMB || pm->ps->eFlags & EF_WALLCLIMBCEILING)
		{
			vec3_t  forward, rotated, angles;
			vec3_t  surfNormal = {0,0,-1};

			if(!(pm->ps->eFlags & EF_WALLCLIMBCEILING))
				VectorCopy(pm->ps->grapplePoint,surfNormal);

			//only correct if we were on a ceiling
			if(surfNormal[2] < 0)
			{
				//The actual problem is with BG_RotateAxis()
				//The rotation applied there causes our view to turn around
				//We correct this here
				vec3_t xNormal;
				AngleVectors( pm->ps->viewangles, forward, nullptr, nullptr );
				if(pm->ps->eFlags & EF_WALLCLIMBCEILING)
				{
					VectorCopy(pm->ps->grapplePoint, xNormal);
				}
				else {
					CrossProduct( surfNormal, refNormal, xNormal );
					VectorNormalize( xNormal );
				}
				RotatePointAroundVector(rotated, xNormal, forward, 180);
				vectoangles( rotated, angles );
				pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT( angles[ YAW ] - pm->ps->viewangles[ YAW ] );
			}
		}
	}

	pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBING;
	pm->ps->eFlags &= ~( EF_WALLCLIMB | EF_WALLCLIMBCEILING );

	point[ 0 ] = pm->ps->origin[ 0 ];
	point[ 1 ] = pm->ps->origin[ 1 ];
	point[ 2 ] = pm->ps->origin[ 2 ] - 0.25f;

	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum,
	           pm->tracemask, 0 );

	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid && !PM_CorrectAllSolid( &trace ) )
	{
		return;
	}

	//make sure that the surfNormal is reset to the ground
	VectorCopy( refNormal, pm->ps->grapplePoint );

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0f )
	{
		bool steppedDown = false;

		// try to step down
		if ( pml.groundPlane && PM_PredictStepMove() )
		{
			//step down
			point[ 0 ] = pm->ps->origin[ 0 ];
			point[ 1 ] = pm->ps->origin[ 1 ];
			point[ 2 ] = pm->ps->origin[ 2 ] - STEPSIZE;
			pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum,
			           pm->tracemask, 0 );

			//if we hit something
			if ( trace.fraction < 1.0f )
			{
				PM_StepEvent( pm->ps->origin, trace.endpos, refNormal );
				VectorCopy( trace.endpos, pm->ps->origin );
				steppedDown = true;
			}
		}

		if ( !steppedDown )
		{
			PM_GroundTraceMissed();
			pml.groundPlane = false;
			pml.walking = false;

			return;
		}
	}
	else if ( trace.fraction > 0.01f )
	{
		// close the gap with the "ground" so that the player is not slightly hovering
		VectorCopy( trace.endpos, pm->ps->origin );
		if ( pm->debugLevel > 1 )
		{
			Log::Notice( "%i:close gap with ground", c_pmove );
		}
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[ 2 ] > 0.0f && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10.0f )
	{
		if ( pm->debugLevel > 1 )
		{
			Log::Notice( "%i:kickoff\n", c_pmove );
		}

		// go into jump animation
		PM_PlayJumpingAnimation();

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = false;
		pml.walking = false;
		return;
	}

	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[ 2 ] < MIN_WALK_NORMAL )
	{
		if ( pm->debugLevel > 1 )
		{
			Log::Notice( "%i:steep\n", c_pmove );
		}

		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = true;
		pml.walking = false;
		return;
	}

	pml.groundPlane = true;
	pml.walking = true;

	// hitting solid ground will end a waterjump
	if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
	{
		pm->ps->pm_flags &= ~PMF_TIME_WATERJUMP;
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{
		// just hit the ground
		if ( pm->debugLevel > 1 )
		{
			Log::Notice( "%i:Land\n", c_pmove );
		}

		// communicate the impact velocity to the server
		VectorCopy( pml.previous_velocity, pm->pmext->fallImpactVelocity );

		PM_Land();

		if ( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_TAKESFALLDAMAGE ) )
		{
			PM_CrashLand();
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
	//pm->ps->velocity[2] = 0;

	PM_AddTouchEnt( trace.entityNum );
}

/*
=============
PM_SetWaterLevel  FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel()
{
	vec3_t point;
	int    cont;
	int    sample1;
	int    sample2;

	//
	// get waterlevel, accounting for ducking
	//
	pm->waterlevel = 0;
	pm->watertype = 0;

	vec3_t mins;
	BG_ClassBoundingBox( pm->ps->stats[ STAT_CLASS ], mins, nullptr, nullptr, nullptr, nullptr );

	point[ 0 ] = pm->ps->origin[ 0 ];
	point[ 1 ] = pm->ps->origin[ 1 ];
	point[ 2 ] = pm->ps->origin[ 2 ] + mins[2] + 1;
	cont = pm->pointcontents( point, pm->ps->clientNum );

	if ( cont & MASK_WATER )
	{
		sample2 = pm->ps->viewheight - mins[2];
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[ 2 ] = pm->ps->origin[ 2 ] + mins[2] + sample1;
		cont = pm->pointcontents( point, pm->ps->clientNum );

		if ( cont & MASK_WATER )
		{
			pm->waterlevel = 2;
			point[ 2 ] = pm->ps->origin[ 2 ] + mins[2] + sample2;
			cont = pm->pointcontents( point, pm->ps->clientNum );

			if ( cont & MASK_WATER )
			{
				pm->waterlevel = 3;
			}
		}
	}
}

/*
==============
PM_SetViewheight
==============
*/
static void PM_SetViewheight()
{
	classModelConfig_t *cfg = BG_ClassModelConfig( pm->ps->stats[ STAT_CLASS ] );
	pm->ps->viewheight = ( pm->ps->pm_flags & PMF_DUCKED ) ? cfg->crouchViewheight : cfg->viewheight;
}

/*
==============
PM_CheckDuck

Sets mins and maxs, and calls PM_SetViewheight
==============
*/
static void PM_CheckDuck()
{
	trace_t trace;
	vec3_t  PCmaxs, PCcmaxs;
	playerState_t *ps = pm->ps;

	BG_ClassBoundingBox( ps->stats[ STAT_CLASS ], pm->mins, PCmaxs, PCcmaxs, nullptr, nullptr );

	pm->maxs[ 0 ] = PCmaxs[ 0 ];
	pm->maxs[ 1 ] = PCmaxs[ 1 ];

	if ( ps->pm_type == PM_DEAD )
	{
		pm->maxs[ 2 ] = -8;
		ps->viewheight = pm->mins[ 2 ] + DEAD_VIEWHEIGHT;
		return;
	}

	// If the standing and crouching bboxes are the same the class can't crouch
	if ( pm->cmd.upmove < 0 && !VectorCompare( PCmaxs, PCcmaxs ) )
	{
		// duck
		ps->pm_flags |= PMF_DUCKED;
	}
	else
	{
		// stand up if possible
		if ( ps->pm_flags & PMF_DUCKED )
		{
			// try to stand up
			pm->maxs[ 2 ] = PCmaxs[ 2 ];
			pm->trace( &trace, ps->origin, pm->mins, pm->maxs, ps->origin,
			           ps->clientNum, pm->tracemask, 0 );

			if ( !trace.allsolid )
			{
				ps->pm_flags &= ~PMF_DUCKED;
			}
		}
	}

	pm->maxs[ 2 ] = ps->pm_flags & PMF_DUCKED ? PCcmaxs[ 2 ] : PCmaxs[ 2 ];

	PM_SetViewheight();
}

//===================================================================

/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps()
{
	float    bobmove;
	int      old;
	bool footstep;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	if ( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_WALLCLIMBER ) && pml.groundPlane )
	{
		// FIXME: yes yes i know this is wrong
		// I'm not sure, but I guess what's wrong here is that velocity
		// should be calculated depending on the correct normal.
		// TBH, if that was the only problem around, I'd say fix it now,
		// but that's not, from far.
		pm->xyspeed = sqrtf( pm->ps->velocity[ 0 ] * pm->ps->velocity[ 0 ]
		                   + pm->ps->velocity[ 1 ] * pm->ps->velocity[ 1 ]
		                   + pm->ps->velocity[ 2 ] * pm->ps->velocity[ 2 ] );
	}
	else
	{
		pm->xyspeed = sqrtf( pm->ps->velocity[ 0 ] * pm->ps->velocity[ 0 ]
		                   + pm->ps->velocity[ 1 ] * pm->ps->velocity[ 1 ] );
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{
		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 )
		{
			if ( IsSegmentedModel( pm->ps ) )
			{
				PM_ContinueLegsAnim( LEGS_SWIM );
			}
			else
			{
				PM_ContinueLegsAnim( NSPA_SWIM );
			}
		}

		return;
	}

	bool ducked = pm->ps->pm_flags & PMF_DUCKED;
	// if not trying to move or sliding
	if ( pm->ps->stats[ STAT_STATE ] & SS_SLIDING ||
	   ( !pm->cmd.forwardmove && !pm->cmd.rightmove ) )
	{
		if ( pm->xyspeed < 5 || pm->ps->stats[ STAT_STATE ] & SS_SLIDING )
		{
			pm->ps->bobCycle = 0; // start at beginning of cycle again

			if ( IsSegmentedModel( pm->ps ) )
			{
				PM_ContinueLegsAnim( ducked ? LEGS_IDLECR : LEGS_IDLE );
			}
			else
			{
				PM_ContinueLegsAnim( NSPA_STAND );
			}
		}

		return;
	}

	footstep = false;

	bool backrun = pm->ps->pm_flags & PMF_BACKWARDS_RUN;
	if ( ducked ) //ofc, nonsegmented models don't really crouch...
	{
		bobmove = 0.5; // ducked characters bob much faster

		if ( IsSegmentedModel( pm->ps ) )
		{
			PM_ContinueLegsAnim( backrun ? LEGS_BACKCR : LEGS_WALKCR );
		}
		else
		{
			if ( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
			{
				PM_ContinueLegsAnim( NSPA_WALKRIGHT );
			}
			else if ( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
			{
				PM_ContinueLegsAnim( NSPA_WALKLEFT );
			}
			else if ( backrun )
			{
				PM_ContinueLegsAnim( NSPA_WALKBACK );
			}
			else
			{
				PM_ContinueLegsAnim( NSPA_WALK );
			}
		}
		// ducked characters never play footsteps
	}
	else
	{
		bool running = !usercmdButtonPressed( pm->cmd.buttons, BTN_WALKING );
		if ( running )
		{
			bobmove = 0.4f; // faster speeds bob faster

			if ( pm->ps->weapon == WP_ALEVEL4 && pm->ps->pm_flags & PMF_CHARGE )
			{
				PM_ContinueLegsAnim( NSPA_CHARGE );
			}
			else
			{
				if ( IsSegmentedModel( pm->ps ) )
				{
					PM_ContinueLegsAnim( backrun ? LEGS_BACK : LEGS_RUN );
				}
				else
				{
					if ( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
					{
						PM_ContinueLegsAnim( NSPA_RUNRIGHT );
					}
					else if ( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
					{
						PM_ContinueLegsAnim( NSPA_RUNLEFT );
					}
					else if ( backrun )
					{
						PM_ContinueLegsAnim( NSPA_RUNBACK );
					}
					else
					{
						PM_ContinueLegsAnim( NSPA_RUN );
					}
				}
			}

			footstep = true;
		}
		else
		{
			bobmove = 0.3f; // walking bobs slow

			if ( IsSegmentedModel( pm->ps ) )
			{
				PM_ContinueLegsAnim( backrun ? LEGS_BACKWALK : LEGS_WALK );
			}
			else
			{
				if ( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
				{
					PM_ContinueLegsAnim( NSPA_WALKRIGHT );
				}
				else if ( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
				{
					PM_ContinueLegsAnim( NSPA_WALKLEFT );
				}
				else if ( backrun )
				{
					PM_ContinueLegsAnim( NSPA_WALKBACK );
				}
				else
				{
					PM_ContinueLegsAnim( NSPA_WALK );
				}
			}
		}
	}

	classAttributes_t const* pcl = BG_Class( pm->ps->stats[ STAT_CLASS ] );
	bobmove *= pcl->bobCycle;

	if ( pm->ps->stats[ STAT_STATE ] & SS_SPEEDBOOST && pm->ps->groundEntityNum != ENTITYNUM_NONE )
	{
		bobmove *= pcl->sprintMod;
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = ( int )( old + bobmove * pml.msec ) & 255;

	// if we just crossed a cycle boundary, play an appropriate footstep event
	if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 )
	{
		switch( pm->waterlevel )
		{
			case 0: // on ground will only play sounds if running
				if ( footstep && !pm->noFootsteps )
				{
					PM_AddEvent( PM_FootstepForSurface() );
				}
				break;
			case 1: // splashing
				PM_AddEvent( EV_FOOTSPLASH );
				break;
			case 2: // wading / swimming at surface
				PM_AddEvent( EV_SWIM );
				break;
			case 3: // no sound when completely underwater
				break;
		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents()
{
	// FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if ( !pml.previous_waterlevel && pm->waterlevel )
	{
		PM_AddEvent( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if ( pml.previous_waterlevel && !pm->waterlevel )
	{
		PM_AddEvent( EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if ( pml.previous_waterlevel != 3 && pm->waterlevel == 3 )
	{
		PM_AddEvent( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if ( pml.previous_waterlevel == 3 && pm->waterlevel != 3 )
	{
		PM_AddEvent( EV_WATER_CLEAR );
	}
}

/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange( int weapon )
{
	if ( weapon <= WP_NONE || weapon >= WP_NUM_WEAPONS )
	{
		return;
	}

	if ( !BG_InventoryContainsWeapon( weapon, pm->ps->stats ) )
	{
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_DROPPING )
	{
		return;
	}

	// cancel a reload
	pm->ps->pm_flags &= ~PMF_WEAPON_RELOAD;

	if ( pm->ps->weaponstate == WEAPON_RELOADING )
	{
		pm->ps->weaponTime = 0;
	}

	pm->ps->weaponCharge = 0;

	// prevent flamer effect from continuing
	pm->ps->generic1 = WPM_NOTFIRING;

	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 200;
	pm->ps->persistant[ PERS_NEWWEAPON ] = weapon;

	//reset build weapon
	pm->ps->stats[ STAT_BUILDABLE ] = BA_NONE;

	if ( IsSegmentedModel( pm->ps ) )
	{
		PM_StartTorsoAnim( TORSO_DROP );
		PM_StartWeaponAnim( WANIM_DROP );
	}
}

/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange()
{
	int weapon;

	PM_AddEvent( EV_CHANGE_WEAPON );
	weapon = pm->ps->persistant[ PERS_NEWWEAPON ];

	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS )
	{
		weapon = WP_NONE;
	}

	if ( !BG_InventoryContainsWeapon( weapon, pm->ps->stats ) )
	{
		weapon = WP_NONE;
	}

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += 250;

	if ( IsSegmentedModel( pm->ps ) )
	{
		PM_StartTorsoAnim( TORSO_RAISE );
		PM_StartWeaponAnim( WANIM_RAISE );
	}
}

static void HandleDeconstructButton()
{
	if ( usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK ) ||
	     ( pm->ps->weaponstate != WEAPON_READY && pm->ps->weaponstate != WEAPON_FIRING ) )
	{
		// cancel if player starts to build something, or if the weapon is in a weird state
		pm->ps->weaponCharge = 0;
		return;
	}

	if ( pm->ps->weaponCharge < 0 )
	{
		// weaponCharge==-1 is analogous to the WEAPON_NEEDS_RESET state when overcharging the luci
		if ( !usercmdButtonPressed( pm->cmd.buttons, BTN_DECONSTRUCT ) )
		{
			pm->ps->weaponCharge = 0;
		}
		return;
	}

	if ( usercmdButtonPressed( pm->cmd.buttons, BTN_DECONSTRUCT ) )
	{
		if ( pm->ps->weaponCharge >= BUILDER_MAX_SHORT_DECONSTRUCT_CHARGE && pm->pmext->cancelDeconstructCharge )
		{
			pm->ps->weaponCharge = -1;
			return;
		}

		int oldWeaponCharge = pm->ps->weaponCharge;
		pm->ps->weaponCharge = std::min( pm->ps->weaponCharge + pml.msec, BUILDER_LONG_DECONSTRUCT_CHARGE );
		if ( oldWeaponCharge < BUILDER_MAX_SHORT_DECONSTRUCT_CHARGE &&
		     pm->ps->weaponCharge >= BUILDER_MAX_SHORT_DECONSTRUCT_CHARGE )
		{
			// Lock on to a target at this point. If the target is dead it will be deconned immediately.
			// If the target is not valid for deconstruction, the player receives a warning (this is why
			// we wait until we know it's a long press to select the target, because it is still valid
			// to mark some targets which are protected from deconning).
			PM_AddEvent( EV_DECONSTRUCT_SELECT_TARGET );
			pm->pmext->cancelDeconstructCharge = false;
		}
		// only fire if the build timer is not running
		if ( pm->ps->weaponCharge >= BUILDER_LONG_DECONSTRUCT_CHARGE && pm->ps->stats[ STAT_MISC ] == 0 )
		{
			PM_AddEvent( EV_FIRE_DECONSTRUCT_LONG );
			pm->ps->weaponCharge = -1;
		}
	}
	else if ( pm->ps->weaponCharge > 0 )
	{
		if ( pm->ps->weaponCharge < BUILDER_MAX_SHORT_DECONSTRUCT_CHARGE )
		{
			PM_AddEvent( EV_FIRE_DECONSTRUCT );
		}
		pm->ps->weaponCharge = 0;
	}
}

/*
==============
PM_TorsoAnimation

==============
*/
static void PM_TorsoAnimation()
{
	if ( pm->ps->weaponstate == WEAPON_READY )
	{
		if ( IsSegmentedModel( pm->ps ) )
		{
			PM_ContinueTorsoAnim( TORSO_STAND );
		}

		PM_ContinueWeaponAnim( WANIM_IDLE );
	}
}

/*
==============
PM_Weapon

Generates weapon events and modifies the weapon counter
==============
*/
static void PM_Weapon()
{
	int      addTime = 200; //default addTime - should never be used
	bool attack1 = usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK );
	bool attack2 = usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK2 );
	bool attack3 = usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK3 );

	// Ignore weapons in some cases
	if ( pm->ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
	{
		return;
	}

	// Check for dead player
	if ( pm->ps->stats[ STAT_HEALTH ] <= 0 )
	{
		pm->ps->weapon = WP_NONE;
		return;
	}

	// Pounce cooldown (Mantis)
	if ( pm->ps->weapon == WP_ALEVEL1 )
	{
		pm->ps->weaponCharge -= pml.msec;

		if ( pm->ps->weaponCharge < 0 )
		{
			pm->ps->weaponCharge = 0;
		}
	}

	// Charging for a pounce or canceling a pounce (Dragoon)
	if ( pm->ps->weapon == WP_ALEVEL3 || pm->ps->weapon == WP_ALEVEL3_UPG )
	{
		int max = pm->ps->weapon == WP_ALEVEL3 ? LEVEL3_POUNCE_TIME : LEVEL3_POUNCE_TIME_UPG;

		if ( usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK2 ) )
		{
			pm->ps->weaponCharge += pml.msec;
		}
		else
		{
			pm->ps->weaponCharge -= pml.msec;
		}

		pm->ps->weaponCharge = Math::Clamp(pm->ps->weaponCharge, 0, max);
	}

	// Trample charge mechanics
	if ( pm->ps->weapon == WP_ALEVEL4 )
	{
		// Charging up
		if ( !( pm->ps->stats[ STAT_STATE ] & SS_CHARGING ) )
		{
			// Charge button held
			if ( pm->ps->weaponCharge < LEVEL4_TRAMPLE_CHARGE_TRIGGER &&
			     usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK2 ) )
			{
				pm->ps->stats[ STAT_STATE ] &= ~SS_CHARGING;

				if ( pm->cmd.forwardmove > 0 )
				{
					int    charge = pml.msec;
					vec3_t dir, vel;

					AngleVectors( pm->ps->viewangles, dir, nullptr, nullptr );
					VectorCopy( pm->ps->velocity, vel );
					vel[ 2 ] = 0;
					dir[ 2 ] = 0;
					VectorNormalize( vel );
					VectorNormalize( dir );

					charge *= DotProduct( dir, vel );

					pm->ps->weaponCharge += charge;
				}
				else
				{
					pm->ps->weaponCharge = 0;
				}
			}

			// Charge button released
			else
			{
				if ( pm->ps->weaponCharge > LEVEL4_TRAMPLE_CHARGE_MIN )
				{
					if ( pm->ps->weaponCharge > LEVEL4_TRAMPLE_CHARGE_MAX )
					{
						pm->ps->weaponCharge = LEVEL4_TRAMPLE_CHARGE_MAX;
					}

					pm->ps->weaponCharge = pm->ps->weaponCharge *
					                       LEVEL4_TRAMPLE_DURATION /
					                       LEVEL4_TRAMPLE_CHARGE_MAX;
					pm->ps->stats[ STAT_STATE ] |= SS_CHARGING;
					PM_AddEvent( EV_LEV4_TRAMPLE_START );
				}
				else
				{
					pm->ps->weaponCharge -= pml.msec;
				}
			}
		}

		// Discharging
		else
		{
			if ( pm->ps->weaponCharge < LEVEL4_TRAMPLE_CHARGE_MIN )
			{
				pm->ps->weaponCharge = 0;
			}
			else
			{
				pm->ps->weaponCharge -= pml.msec;
			}

			// If the charger has stopped moving take a chunk of charge away
			if ( VectorLength( pm->ps->velocity ) < 64.0f || pm->cmd.rightmove )
			{
				pm->ps->weaponCharge -= LEVEL4_TRAMPLE_STOP_PENALTY * pml.msec;
			}
		}

		// Charge is over
		if ( pm->ps->weaponCharge <= 0 || pm->cmd.forwardmove <= 0 )
		{
			pm->ps->weaponCharge = 0;
			pm->ps->stats[ STAT_STATE ] &= ~SS_CHARGING;
		}
	}

	// Charging up a Lucifer Cannon
	pm->ps->eFlags &= ~EF_WARN_CHARGE;

	if ( pm->ps->weapon == WP_LUCIFER_CANNON )
	{
		// Charging up
		if ( !pm->ps->weaponTime && pm->ps->weaponstate != WEAPON_NEEDS_RESET &&
		     usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK ) )
		{
			pm->ps->weaponCharge += pml.msec;

			if ( pm->ps->weaponCharge >= LCANNON_CHARGE_TIME_MAX )
			{
				pm->ps->weaponCharge = LCANNON_CHARGE_TIME_MAX;
			}

			if ( pm->ps->weaponCharge > pm->ps->ammo * LCANNON_CHARGE_TIME_MAX /
			     LCANNON_CHARGE_AMMO )
			{
				pm->ps->weaponCharge = pm->ps->ammo * LCANNON_CHARGE_TIME_MAX /
				                             LCANNON_CHARGE_AMMO;
			}
		}

		// Set overcharging flag so other players can hear the warning beep
		if ( pm->ps->weaponCharge > LCANNON_CHARGE_TIME_WARN )
		{
			pm->ps->eFlags |= EF_WARN_CHARGE;
		}
	}

	if ( pm->ps->weapon == WP_ABUILD || pm->ps->weapon == WP_ABUILD2 || pm->ps->weapon == WP_HBUILD )
	{
		HandleDeconstructButton();
	}

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED )
	{
		return;
	}

	// no bite during pounce
	if ( ( pm->ps->weapon == WP_ALEVEL3 || pm->ps->weapon == WP_ALEVEL3_UPG )
	     && usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK )
	     && ( pm->ps->pm_flags & PMF_CHARGE ) )
	{
		return;
	}

	// pump weapon delays (repeat times etc)
	if ( pm->ps->weaponTime > 0 )
	{
		pm->ps->weaponTime -= pml.msec;
	}

	if ( pm->ps->weaponTime < 0 )
	{
		pm->ps->weaponTime = 0;
	}

	// no slash during charge
	if ( pm->ps->stats[ STAT_STATE ] & SS_CHARGING )
	{
		return;
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if ( BG_PlayerCanChangeWeapon( pm->ps ) )
	{
		//something external thinks a weapon change is necessary
		if ( pm->ps->pm_flags & PMF_WEAPON_SWITCH )
		{
			pm->ps->pm_flags &= ~PMF_WEAPON_SWITCH;

			if ( pm->ps->weapon != WP_NONE )
			{
				// drop the current weapon
				PM_BeginWeaponChange( pm->ps->persistant[ PERS_NEWWEAPON ] );
			}
			else
			{
				// no current weapon, so just raise the new one
				PM_FinishWeaponChange();
			}
		}
	}

	if ( pm->ps->weaponTime > 0 )
	{
		return;
	}

	// change weapon if time
	if ( pm->ps->weaponstate == WEAPON_DROPPING )
	{
		PM_FinishWeaponChange();
		return;
	}

	// Set proper animation
	if ( pm->ps->weaponstate == WEAPON_RAISING )
	{
		pm->ps->weaponstate = WEAPON_READY;

		if ( IsSegmentedModel( pm->ps ) )
		{
			PM_ContinueTorsoAnim( TORSO_STAND );
		}

		PM_ContinueWeaponAnim( WANIM_IDLE );

		return;
	}

	// check for out of ammo
	if ( !pm->ps->ammo && !pm->ps->clips && !BG_Weapon( pm->ps->weapon )->infiniteAmmo )
	{
		if ( attack1 ||
		     ( BG_Weapon( pm->ps->weapon )->hasAltMode && attack2 ) ||
		     ( BG_Weapon( pm->ps->weapon )->hasThirdMode && attack3 ) )
		{
			PM_AddEvent( EV_NOAMMO );
			pm->ps->weaponTime += 500;
		}

		if ( pm->ps->weaponstate == WEAPON_FIRING )
		{
			pm->ps->weaponstate = WEAPON_READY;
		}

		return;
	}

	//done reloading so give em some ammo
	if ( pm->ps->weaponstate == WEAPON_RELOADING )
	{
		pm->ps->clips--;
		pm->ps->ammo = BG_Weapon( pm->ps->weapon )->maxAmmo;

		//allow some time for the weapon to be raised
		pm->ps->weaponstate = WEAPON_RAISING;
		PM_StartTorsoAnim( TORSO_RAISE );
		pm->ps->weaponTime += 250;
		return;
	}

	//start reloading
	if ( !BG_Weapon( pm->ps->weapon )->infiniteAmmo &&
	     ( pm->ps->ammo <= 0 || ( pm->ps->pm_flags & PMF_WEAPON_RELOAD ) ) &&
	     pm->ps->clips > 0 )
	{
		pm->ps->pm_flags &= ~PMF_WEAPON_RELOAD;
		pm->ps->weaponstate = WEAPON_RELOADING;

		//drop the weapon
		PM_StartTorsoAnim( TORSO_DROP );
		PM_StartWeaponAnim( WANIM_RELOAD );
		BG_AddPredictableEventToPlayerstate( EV_WEAPON_RELOAD, pm->ps->weapon, pm->ps );

		pm->ps->weaponTime += BG_Weapon( pm->ps->weapon )->reloadTime;
		return;
	}

	//check if non-auto primary/secondary attacks are permited
	switch ( pm->ps->weapon )
	{
		case WP_ALEVEL0:
			//venom is only autohit
			return;

		case WP_ALEVEL3:
		case WP_ALEVEL3_UPG:

			//pouncing has primary secondary AND autohit procedures
			// pounce is autohit
			if ( !attack1 && !attack2 && !attack3 )
			{
				return;
			}

			break;

		case WP_LUCIFER_CANNON:
			attack3 = false;

			// Prevent firing of the Lucifer Cannon after an overcharge
			if ( pm->ps->weaponstate == WEAPON_NEEDS_RESET )
			{
				if ( attack1 )
				{
					return;
				}

				pm->ps->weaponstate = WEAPON_READY;
			}

			// Can't fire secondary while primary is charging
			if ( attack1 || pm->ps->weaponCharge > 0 )
			{
				attack2 = false;
			}

			if ( ( attack1 || pm->ps->weaponCharge == 0 ) && !attack2 )
			{
				pm->ps->weaponTime = 0;

				// Charging
				if ( pm->ps->weaponCharge < LCANNON_CHARGE_TIME_MAX )
				{
					pm->ps->weaponstate = WEAPON_READY;
					return;
				}

				// Overcharge
				pm->ps->weaponstate = WEAPON_NEEDS_RESET;
			}

			if ( pm->ps->weaponCharge > LCANNON_CHARGE_TIME_MIN )
			{
				// Fire primary attack
				attack1 = true;
				attack2 = false;
			}
			else if ( pm->ps->weaponCharge > 0 )
			{
				// Not enough charge
				pm->ps->weaponCharge = 0;
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
				return;
			}
			else if ( !attack2 )
			{
				// Idle
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
				return;
			}

			break;

		case WP_MASS_DRIVER:
		case WP_LAS_GUN:
			attack2 = attack3 = false;
			// attack2 is handled on the client for zooming (cg_view.c)

			if ( !attack1 )
			{
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
				return;
			}

			break;

		default:
			if ( !attack1 && !attack2 && !attack3 )
			{
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
				return;
			}

			break;
	}

	// fire events for non auto weapons
	if ( attack3 )
	{
		if ( BG_Weapon( pm->ps->weapon )->hasThirdMode )
		{
			//hacky special case for slowblob
			if ( pm->ps->weapon == WP_ALEVEL3_UPG && !pm->ps->ammo )
			{
				pm->ps->weaponTime += 200;
				return;
			}

			pm->ps->generic1 = WPM_TERTIARY;
			PM_AddEvent( EV_FIRE_WEAPON3 );
			addTime = BG_Weapon( pm->ps->weapon )->repeatRate3;
		}
		else
		{
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			pm->ps->generic1 = WPM_NOTFIRING;
			return;
		}
	}
	else if ( attack2 )
	{
		if ( BG_Weapon( pm->ps->weapon )->hasAltMode )
		{
			pm->ps->generic1 = WPM_SECONDARY;
			PM_AddEvent( EV_FIRE_WEAPON2 );
			addTime = BG_Weapon( pm->ps->weapon )->repeatRate2;
		}
		else
		{
			pm->ps->weaponTime = 0;
			pm->ps->weaponstate = WEAPON_READY;
			pm->ps->generic1 = WPM_NOTFIRING;
			return;
		}
	}
	else if ( attack1 )
	{
		pm->ps->generic1 = WPM_PRIMARY;
		PM_AddEvent( EV_FIRE_WEAPON );
		addTime = BG_Weapon( pm->ps->weapon )->repeatRate1;
	}

	// fire events for autohit weapons
	if ( pm->autoWeaponHit[ pm->ps->weapon ] )
	{
		switch ( pm->ps->weapon )
		{
			case WP_ALEVEL0:
				pm->ps->generic1 = WPM_PRIMARY;
				PM_AddEvent( EV_FIRE_WEAPON );
				addTime = BG_Weapon( pm->ps->weapon )->repeatRate1;
				break;

			case WP_ALEVEL3:
			case WP_ALEVEL3_UPG:
				pm->ps->generic1 = WPM_SECONDARY;
				PM_AddEvent( EV_FIRE_WEAPON2 );
				addTime = BG_Weapon( pm->ps->weapon )->repeatRate2;
				break;

			default:
				break;
		}
	}

	if ( IsSegmentedModel( pm->ps ) )
	{
		//FIXME: this should be an option in the client weapon.cfg
		switch ( pm->ps->weapon )
		{
			case WP_FLAMER:
				if ( pm->ps->weaponstate == WEAPON_READY )
				{
					PM_StartTorsoAnim( TORSO_ATTACK );
					PM_StartWeaponAnim( WANIM_ATTACK1 );
				}

				break;

			case WP_BLASTER:
				PM_StartTorsoAnim( TORSO_ATTACK_BLASTER );
				PM_StartWeaponAnim( WANIM_ATTACK1 );
				break;

			case WP_PAIN_SAW:
				PM_StartTorsoAnim( TORSO_ATTACK_PSAW );
				PM_StartWeaponAnim( WANIM_ATTACK1 );
				break;

			default:
				if ( attack1 )
				{
					PM_StartTorsoAnim( TORSO_ATTACK );
					PM_StartWeaponAnim( WANIM_ATTACK1 );
					break;
				}
				else if ( attack2 )
				{
					PM_StartTorsoAnim( TORSO_ATTACK );
					PM_StartWeaponAnim( WANIM_ATTACK2 );
					break;
				}
		}
	}
	else
	{
		int num = rand();

		//FIXME: it would be nice to have these hard coded policies in
		//       weapon.cfg
		switch ( pm->ps->weapon )
		{
			case WP_ALEVEL1:
				if ( attack1 )
				{
					num /= RAND_MAX / 6 + 1;
					PM_ForceLegsAnim( NSPA_ATTACK1 );
					PM_StartWeaponAnim( WANIM_ATTACK1 + num );
				}

				break;

			case WP_ALEVEL2_UPG:
				if ( attack2 )
				{
					PM_ForceLegsAnim( NSPA_ATTACK2 );
					PM_StartWeaponAnim( WANIM_ATTACK7 );
				}
				DAEMON_FALLTHROUGH;

			case WP_ALEVEL2:
				if ( attack1 )
				{
					num /= RAND_MAX / 3 + 1;
					PM_ForceLegsAnim( NSPA_ATTACK1 + num );
					num = rand() / ( RAND_MAX / 6 + 1 );
					PM_StartWeaponAnim( WANIM_ATTACK1 + num );
				}

				break;

			case WP_ALEVEL4:
				num /= RAND_MAX / 3 + 1;
				PM_ForceLegsAnim( NSPA_ATTACK1 + num );
				num = rand() / ( RAND_MAX / 6 + 1 );
				PM_StartWeaponAnim( WANIM_ATTACK1 + num );
				break;

			default:
				if ( attack1 )
				{
					PM_ForceLegsAnim( NSPA_ATTACK1 );
					PM_StartWeaponAnim( WANIM_ATTACK1 );
				}
				else if ( attack2 )
				{
					PM_ForceLegsAnim( NSPA_ATTACK2 );
					PM_StartWeaponAnim( WANIM_ATTACK2 );
				}
				else if ( attack3 )
				{
					PM_ForceLegsAnim( NSPA_ATTACK3 );
					PM_StartWeaponAnim( WANIM_ATTACK3 );
				}

				break;
		}

		pm->ps->torsoTimer = TIMER_ATTACK;
	}

	if ( pm->ps->weaponstate != WEAPON_NEEDS_RESET )
	{
		pm->ps->weaponstate = WEAPON_FIRING;
	}

	// take an ammo away if not infinite
	if ( !BG_Weapon( pm->ps->weapon )->infiniteAmmo ||
	     ( pm->ps->weapon == WP_ALEVEL3_UPG && attack3 ) )
	{
		// Special case for lcannon
		if ( pm->ps->weapon == WP_LUCIFER_CANNON && attack1 && !attack2 )
		{
			pm->ps->ammo -= ( pm->ps->weaponCharge * LCANNON_CHARGE_AMMO +
			                  LCANNON_CHARGE_TIME_MAX - 1 ) / LCANNON_CHARGE_TIME_MAX;
		}
		else
		{
			pm->ps->ammo--;
		}

		// Stay on the safe side
		if ( pm->ps->ammo < 0 )
		{
			pm->ps->ammo = 0;
		}
	}

	//FIXME: predicted angles miss a problem??
	if ( pm->ps->weapon == WP_CHAINGUN )
	{
		if ( pm->ps->pm_flags & PMF_DUCKED ||
		     BG_InventoryContainsUpgrade( UP_BATTLESUIT, pm->ps->stats ) )
		{
			pm->ps->delta_angles[ PITCH ] -= ANGLE2SHORT( ( ( random() * 0.5 ) - 0.125 ) * ( 30 / ( float ) addTime ) );
			pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT( ( ( random() * 0.5 ) - 0.25 ) * ( 30.0 / ( float ) addTime ) );
		}
		else
		{
			pm->ps->delta_angles[ PITCH ] -= ANGLE2SHORT( ( ( random() * 8 ) - 2 ) * ( 30.0 / ( float ) addTime ) );
			pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT( ( ( random() * 8 ) - 4 ) * ( 30.0 / ( float ) addTime ) );
		}
	}

	pm->ps->weaponTime += addTime;
}

/*
================
PM_Animate
================
*/
static void PM_Animate()
{
	if ( PM_Paralyzed( pm->ps->pm_type )
			|| pm->ps->tauntTimer > 0
			|| pm->ps->torsoTimer != 0 )
	{
		return;
	}

	bool doit = false;
	bool gesture = usercmdButtonPressed( pm->cmd.buttons, BTN_GESTURE );
	bool rally  = usercmdButtonPressed( pm->cmd.buttons, BTN_RALLY );
	if ( IsSegmentedModel( pm->ps ) )
	{
		if ( gesture )
		{
			int wpAnim = TORSO_GESTURE_BLASTER + ( pm->ps->weapon - WP_BLASTER );
			//and now I know why build stuff must be last in the weapon list...
			PM_StartTorsoAnim( wpAnim > WP_LUCIFER_CANNON ?  TORSO_GESTURE_CKIT : wpAnim );
			doit = true;
		}
		// This code could likely be purged, really (but double
		// check that!).
		else if ( rally )
		{
			PM_StartTorsoAnim( TORSO_RALLY );
			doit = true;
		}
	}
	else if ( gesture || rally )
	{
		PM_ForceLegsAnim( NSPA_GESTURE );
		doit = true;
	}

	if ( doit )
	{
		pm->ps->torsoTimer = TIMER_GESTURE;
		pm->ps->tauntTimer = TIMER_GESTURE;
		PM_AddEvent( EV_TAUNT );
	}
}

/*
================
PM_DropTimers
================
*/
static void PM_DropAnimTimer( int & timer, int decr )
{
	if ( timer > 0 )
	{
		timer -= decr;

		if ( timer < 0 )
		{
			timer = 0;
		}
	}
}

static void PM_DropTimers()
{
	// drop misc timing counter
	if ( pm->ps->pm_time )
	{
		if ( pml.msec >= pm->ps->pm_time )
		{
			pm->ps->pm_flags &= ~PMF_ALL_TIMES;
			pm->ps->pm_time = 0;
		}
		else
		{
			pm->ps->pm_time -= pml.msec;
		}
	}

	// drop animation counter
	PM_DropAnimTimer( pm->ps->legsTimer , pml.msec );
	PM_DropAnimTimer( pm->ps->torsoTimer, pml.msec );
	PM_DropAnimTimer( pm->ps->tauntTimer, pml.msec );
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
void PM_UpdateViewAngles( playerState_t *ps, const usercmd_t *cmd )
{
	short  temp[ 3 ];
	int    i;
	vec3_t axis[ 3 ], rotaxis[ 3 ];
	vec3_t tempang;

	if ( ps->pm_type == PM_INTERMISSION )
	{
		return; // no view changes at all
	}

	if ( ps->persistant[ PERS_SPECSTATE ] == SPECTATOR_NOT && ps->stats[ STAT_HEALTH ] <= 0 )
	{
		return; // no view changes at all
	}

	// circularly clamp the angles with deltas
	for ( i = 0; i < 3; i++ )
	{
		if ( i == ROLL )
		{
			// Guard against speed hack
			temp[ i ] = ps->delta_angles[ i ];

#ifdef BUILD_CGAME
			// Assert here so that if cmd->angles[i] becomes non-zero
			// for a legitimate reason we can tell where and why it's
			// being ignored
			ASSERT_EQ(cmd->angles[i], 0);
#endif
		}
		else
		{
			temp[ i ] = cmd->angles[ i ] + ps->delta_angles[ i ];
		}

		if ( i == PITCH )
		{
			// don't let the player look up or down more than 90 degrees
			if ( temp[ i ] > 16000 )
			{
				ps->delta_angles[ i ] = 16000 - cmd->angles[ i ];
				temp[ i ] = 16000;
			}
			else if ( temp[ i ] < -16000 )
			{
				ps->delta_angles[ i ] = -16000 - cmd->angles[ i ];
				temp[ i ] = -16000;
			}
		}

		tempang[ i ] = SHORT2ANGLE( temp[ i ] );
	}

	//convert viewangles -> axis
	AnglesToAxis( tempang, axis );

	if ( !( ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) ||
	     !BG_RotateAxis( ps->grapplePoint, axis, rotaxis, false,
	                     ps->eFlags & EF_WALLCLIMBCEILING ) )
	{
		AxisCopy( axis, rotaxis );
	}

	//convert the new axis back to angles
	AxisToAngles( rotaxis, tempang );

	//force angles to -180 <= x <= 180
	for ( i = 0; i < 3; i++ )
	{
		tempang[ i ] -= 360.0f * floor( ( tempang[ i ] + 180.0f ) / 360.0f );
	}

	//actually set the viewangles
	VectorCopy( tempang, ps->viewangles );

	//pull the view into the lock point
	if ( ps->pm_type == PM_GRABBED && !BG_InventoryContainsUpgrade( UP_BATTLESUIT, ps->stats ) )
	{
		vec3_t dir, angles;

		ByteToDir( ps->stats[ STAT_VIEWLOCK ], dir );
		vectoangles( dir, angles );

		for ( i = 0; i < 3; i++ )
		{
			float diff = AngleSubtract( ps->viewangles[ i ], angles[ i ] );


			diff -= 360.0f * floor( ( diff + 180.0f ) / 360.0f );

			if ( diff < -90.0f )
			{
				ps->delta_angles[ i ] += ANGLE2SHORT( fabsf( diff ) - 90.0f );
			}
			else if ( diff > 90.0f )
			{
				ps->delta_angles[ i ] -= ANGLE2SHORT( fabsf( diff ) - 90.0f );
			}

			if ( diff < 0.0f )
			{
				ps->delta_angles[ i ] += ANGLE2SHORT( fabsf( diff ) * 0.05f );
			}
			else if ( diff > 0.0f )
			{
				ps->delta_angles[ i ] -= ANGLE2SHORT( fabsf( diff ) * 0.05f );
			}
		}
	}
}

static void PM_HumanStaminaEffects()
{
	const classAttributes_t *ca;
	int      *stats;
	bool crouching, stopped, walking;

	if ( pm->ps->persistant[ PERS_TEAM ] != TEAM_HUMANS )
	{
		return;
	}

	stats     = pm->ps->stats;
	ca        = BG_Class( stats[ STAT_CLASS ] );
	stopped   = ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove == 0 );
	crouching = ( pm->ps->pm_flags & PMF_DUCKED );
	walking   = usercmdButtonPressed( pm->cmd.buttons, BTN_WALKING );

	// Use/Restore stamina
	if ( stats[ STAT_STATE2 ] & SS2_JETPACK_WARM )
	{
		stats[ STAT_STAMINA ] += ( int )( pml.msec * ca->staminaJogRestore * 0.001f );
	}
	else if ( stopped )
	{
		stats[ STAT_STAMINA ] += ( int )( pml.msec * ca->staminaStopRestore * 0.001f );
	}
	else if ( ( stats[ STAT_STATE ] & SS_SPEEDBOOST ) && !walking && !crouching ) // walk/crouch overrides sprint
	{
		stats[ STAT_STAMINA ] -= ( int )( pml.msec * ca->staminaSprintCost * 0.001f );
	}
	else if ( walking || crouching )
	{
		stats[ STAT_STAMINA ] += ( int )( pml.msec * ca->staminaWalkRestore * 0.001f );
	}
	else // assume jogging
	{
		stats[ STAT_STAMINA ] += ( int )( pml.msec * ca->staminaJogRestore * 0.001f );
	}

	// Remove stamina based on status effects
	if ( stats[ STAT_STATE2 ] & SS2_LEVEL1SLOW )
	{
		stats[ STAT_STAMINA ] -= pml.msec * STAMINA_LEVEL1SLOW_TAKE;
	}

	// Check stamina limits
	stats[ STAT_STAMINA ] = Math::Clamp( stats[ STAT_STAMINA ], 0, STAMINA_MAX );
}

/*
================
PmoveSingle

================
*/
// set the firing flag for continuous beam weapons
static void SetFireBeam( buttonNumber_t btn )
{
	int firingEvent;
	switch( btn )
	{
		case BTN_ATTACK:
			firingEvent = EF_FIRING;
			break;
		case BTN_ATTACK2:
			firingEvent = EF_FIRING2;
			break;
		case BTN_ATTACK3:
			firingEvent = EF_FIRING3;
			break;
		default:
			ASSERT_UNREACHABLE();
	}

	if ( !( pm->ps->pm_flags & PMF_RESPAWNED ) && pm->ps->pm_type != PM_INTERMISSION &&
			usercmdButtonPressed( pm->cmd.buttons, btn ) &&
			( ( pm->ps->ammo > 0 || pm->ps->clips > 0 ) || BG_Weapon( pm->ps->weapon )->infiniteAmmo ) )
	{
		pm->ps->eFlags |= firingEvent;
	}
	else
	{
		pm->ps->eFlags &= ~firingEvent;
	}
}

// This helper is purely for readability. Returns true when
// player is walking on a wall.
static bool IsWallwalking( playerState_t const& ps )
{
		return BG_ClassHasAbility( ps.stats[ STAT_CLASS ], SCA_WALLCLIMBER )
			&& ( ps.stats[ STAT_STATE ] & SS_WALLCLIMBING );
}

static void PmoveSingle( pmove_t *pmove )
{
	pm = pmove;

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint for the previous frame
	c_pmove++;

	// clear results
	pm->numtouch = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	if ( pm->ps->stats[ STAT_HEALTH ] <= 0 )
	{
		pm->tracemask &= ~CONTENTS_BODY; // corpses can fly through bodies
	}

	// make sure walking button is clear if they are running, to avoid
	// proxy no-footsteps cheats
	if ( abs( pm->cmd.forwardmove ) > 64 || abs( pm->cmd.rightmove ) > 64 )
	{
		usercmdReleaseButton( pm->cmd.buttons, BTN_WALKING );
	}

	SetFireBeam( BTN_ATTACK );
	SetFireBeam( BTN_ATTACK2 );
	SetFireBeam( BTN_ATTACK3 );

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[ STAT_HEALTH ] > 0 &&
	     !( usercmdButtonPressed( pm->cmd.buttons, BTN_ATTACK ) ) )
	{
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, disallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( usercmdButtonPressed( pmove->cmd.buttons, BTN_TALK ) )
	{
		usercmdClearButtons( pmove->cmd.buttons );
		usercmdPressButton( pmove->cmd.buttons, BTN_TALK );
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;

		if ( pmove->cmd.upmove > 0 )
		{
			pmove->cmd.upmove = 0;
		}
	}

	// clear all pmove local vars
	memset( &pml, 0, sizeof( pml ) );

	// determine the time
	pml.msec = pmove->cmd.serverTime - pm->ps->commandTime;
	pml.msec = Math::Clamp( pml.msec, 1, 200 );

	pm->ps->commandTime = pmove->cmd.serverTime;

	// save old org in case we get stuck
	VectorCopy( pm->ps->origin, pml.previous_origin );

	// save old velocity for crashlanding
	VectorCopy( pm->ps->velocity, pml.previous_velocity );

	pml.frametime = pml.msec * 0.001;

	AngleVectors( pm->ps->viewangles, pml.forward, pml.right, pml.up );

	if ( pm->cmd.upmove < 10 )
	{
		// not holding jump
		pm->ps->pm_flags &= ~PMF_JUMP_HELD;
	}

	// decide if backpedaling animations should be used
	if ( pm->cmd.forwardmove < 0 )
	{
		pm->ps->pm_flags |= PMF_BACKWARDS_RUN;
	}
	else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove ) )
	{
		pm->ps->pm_flags &= ~PMF_BACKWARDS_RUN;
	}

	if ( PM_Paralyzed( pm->ps->pm_type ) )
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	switch ( pm->ps->pm_type )
	{
		case PM_SPECTATOR:
			PM_UpdateViewAngles( pm->ps, &pm->cmd );
			PM_CheckDuck(); //never seen any spectator crounching!
			PM_GhostMove( false );
			PM_DropTimers();
			return;

		case PM_NOCLIP:
			PM_UpdateViewAngles( pm->ps, &pm->cmd );
			PM_GhostMove( true );
			PM_SetViewheight();
			PM_Weapon();
			PM_DropTimers();
			return;

		case PM_FREEZE:
		case PM_INTERMISSION:
			return;

		default:
			break;
	}

	// set watertype, and waterlevel
	PM_SetWaterLevel();
	pml.previous_waterlevel = pmove->waterlevel;

	// set mins, maxs, and viewheight
	PM_CheckDuck();

	PM_CheckLadder();

	// set groundentity
	PM_GroundTrace();

	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd );

	if ( pm->ps->pm_type == PM_DEAD || pm->ps->pm_type == PM_GRABBED )
	{
		PM_DeadMove();
	}

	PM_DropTimers();

	if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
	{
		PM_WaterJumpMove();
	}
	else if ( pm->waterlevel > 1 )
	{
		PM_WaterMove();
	}
	else if ( pml.ladder )
	{
		PM_LadderMove();
	}
	else if ( pml.walking )
	{
		if ( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 )
		{
			PM_WaterMove();
		}
		else if ( PM_CheckJump() || PM_CheckPounce() )
		{
			if ( pm->waterlevel > 1 )
			{
				PM_WaterMove();
			}
			else
			{
				PM_AirMove();
			}
		}
		else if ( IsWallwalking( *pm->ps ) )
		{
			PM_ClimbMove(); // walking on any surface
		}
		else
		{
			PM_WalkMove(); // walking on ground
		}
	}
	else
	{
		PM_AirMove();
	}

	// restore jetpack fuel if possible
	PM_CheckJetpackRestoreFuel();

	// restore or remove stamina
	PM_HumanStaminaEffects();

	PM_Animate();

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();

	// update the viewangles
	PM_UpdateViewAngles( pm->ps, &pm->cmd );

	PM_SetWaterLevel();

	// weapons
	PM_Weapon();

	// torso animation
	PM_TorsoAnimation();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	if ( !pmove->pmove_accurate )
	{
		// snap some parts of playerstate to save network bandwidth
		SnapVector( pm->ps->velocity );
	}
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove( pmove_t *pmove )
{
	int finalTime;

	finalTime = pmove->cmd.serverTime;

	if ( finalTime < pmove->ps->commandTime )
	{
		return; // should not happen
	}

	if ( finalTime > pmove->ps->commandTime + 1000 )
	{
		pmove->ps->commandTime = finalTime - 1000;
	}

	pmove->ps->pmove_framecount = ( pmove->ps->pmove_framecount + 1 ) & ( ( 1 << PS_PMOVEFRAMECOUNTBITS ) - 1 );

	// chop the move up if it is too long, to prevent framerate
	// dependent behavior
	while ( pmove->ps->commandTime != finalTime )
	{
		int msec;

		msec = finalTime - pmove->ps->commandTime;

		if ( pmove->pmove_fixed )
		{
			if ( msec > pmove->pmove_msec )
			{
				msec = pmove->pmove_msec;
			}
		}
		else
		{
			if ( msec > 66 )
			{
				msec = 66;
			}
		}

		pmove->cmd.serverTime = pmove->ps->commandTime + msec;
		PmoveSingle( pmove );
	}
}

/*
==================
PM_SlideMove

Returns true if the velocity was clipped in some way
==================
*/
#define MAX_CLIP_PLANES 5
static bool  PM_SlideMove( bool gravity )
{
	int     bumpcount, numbumps;
	vec3_t  dir;
	float   d;
	int     numplanes;
	vec3_t  planes[ MAX_CLIP_PLANES ];
	vec3_t  primal_velocity;
	vec3_t  clipVelocity;
	int     i, j, k;
	trace_t trace;
	vec3_t  end;
	float   time_left;
	float   into;
	vec3_t  endVelocity;
	vec3_t  endClipVelocity;

	numbumps = 4;

	VectorCopy( pm->ps->velocity, primal_velocity );
	VectorCopy( pm->ps->velocity, endVelocity );

	if ( gravity )
	{
		endVelocity[ 2 ] -= pm->ps->gravity * pml.frametime;
		pm->ps->velocity[ 2 ] = ( pm->ps->velocity[ 2 ] + endVelocity[ 2 ] ) * 0.5;
		primal_velocity[ 2 ] = endVelocity[ 2 ];

		if ( pml.groundPlane )
		{
			// slide along the ground plane
			PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );
		}
	}

	time_left = pml.frametime;

	// never turn against the ground plane
	if ( pml.groundPlane )
	{
		numplanes = 1;
		VectorCopy( pml.groundTrace.plane.normal, planes[ 0 ] );
	}
	else
	{
		numplanes = 0;
	}

	// never turn against original velocity
	VectorNormalize2( pm->ps->velocity, planes[ numplanes ] );
	numplanes++;

	for ( bumpcount = 0; bumpcount < numbumps; bumpcount++ )
	{
		// calculate position we are trying to move to
		VectorMA( pm->ps->origin, time_left, pm->ps->velocity, end );

		// see if we can make it there
		// spectators ignore movers, so that they can noclip through doors
		pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum,
		           pm->tracemask, ( pm->ps->pm_type == PM_SPECTATOR ) ? CONTENTS_MOVER : 0 );

		if ( trace.allsolid )
		{
			// entity is completely trapped in another solid
			pm->ps->velocity[ 2 ] = 0; // don't build up falling damage, but allow sideways acceleration
			return true;
		}

		if ( trace.fraction > 0 )
		{
			// actually covered some distance
			VectorCopy( trace.endpos, pm->ps->origin );
		}

		if ( trace.fraction == 1 )
		{
			break; // moved the entire distance
		}

		// save entity for contact
		PM_AddTouchEnt( trace.entityNum );

		time_left -= time_left * trace.fraction;

		if ( numplanes >= MAX_CLIP_PLANES )
		{
			// this shouldn't really happen
			VectorClear( pm->ps->velocity );
			return true;
		}

		//
		// if this is the same plane we hit before, nudge velocity
		// out along it, which fixes some epsilon issues with
		// non-axial planes
		//
		for ( i = 0; i < numplanes; i++ )
		{
			if ( DotProduct( trace.plane.normal, planes[ i ] ) > 0.99 )
			{
				VectorAdd( trace.plane.normal, pm->ps->velocity, pm->ps->velocity );
				break;
			}
		}

		if ( i < numplanes )
		{
			continue;
		}

		VectorCopy( trace.plane.normal, planes[ numplanes ] );
		numplanes++;

		//
		// modify velocity so it parallels all of the clip planes
		//

		// find a plane that it enters
		for ( i = 0; i < numplanes; i++ )
		{
			into = DotProduct( pm->ps->velocity, planes[ i ] );

			if ( into >= 0.1 )
			{
				continue; // move doesn't interact with the plane
			}

			// see how hard we are hitting things
			if ( -into > pml.impactSpeed )
			{
				pml.impactSpeed = -into;
			}

			// slide along the plane
			PM_ClipVelocity( pm->ps->velocity, planes[ i ], clipVelocity );

			// slide along the plane
			PM_ClipVelocity( endVelocity, planes[ i ], endClipVelocity );

			// see if there is a second plane that the new move enters
			for ( j = 0; j < numplanes; j++ )
			{
				if ( j == i )
				{
					continue;
				}

				if ( DotProduct( clipVelocity, planes[ j ] ) >= 0.1 )
				{
					continue; // move doesn't interact with the plane
				}

				// try clipping the move to the plane
				PM_ClipVelocity( clipVelocity, planes[ j ], clipVelocity );
				PM_ClipVelocity( endClipVelocity, planes[ j ], endClipVelocity );

				// see if it goes back into the first clip plane
				if ( DotProduct( clipVelocity, planes[ i ] ) >= 0 )
				{
					continue;
				}

				// slide the original velocity along the crease
				CrossProduct( planes[ i ], planes[ j ], dir );
				VectorNormalize( dir );
				d = DotProduct( dir, pm->ps->velocity );
				VectorScale( dir, d, clipVelocity );

				CrossProduct( planes[ i ], planes[ j ], dir );
				VectorNormalize( dir );
				d = DotProduct( dir, endVelocity );
				VectorScale( dir, d, endClipVelocity );

				// see if there is a third plane the new move enters
				for ( k = 0; k < numplanes; k++ )
				{
					if ( k == i || k == j )
					{
						continue;
					}

					if ( DotProduct( clipVelocity, planes[ k ] ) >= 0.1 )
					{
						continue; // move doesn't interact with the plane
					}

					// stop dead at a triple plane interaction
					VectorClear( pm->ps->velocity );
					return true;
				}
			}

			// if we have fixed all interactions, try another move
			VectorCopy( clipVelocity, pm->ps->velocity );
			VectorCopy( endClipVelocity, endVelocity );
			break;
		}
	}

	if ( gravity )
	{
		VectorCopy( endVelocity, pm->ps->velocity );
	}

	// don't change velocity if in a timer (FIXME: is this correct?)
	if ( pm->ps->pm_time )
	{
		VectorCopy( primal_velocity, pm->ps->velocity );
	}

	return ( bumpcount != 0 );
}

/*
==================
PM_StepSlideMove
==================
*/
static bool PM_StepSlideMove( bool gravity, bool predictive )
{
	vec3_t   start_o, start_v;
	vec3_t   down_o, down_v;
	trace_t  trace;
	vec3_t   normal;
	vec3_t   step_v, step_vNormal;
	vec3_t   up, down;
	float    stepSize;
	bool stepped = false;

	BG_GetClientNormal( pm->ps, normal );

	VectorCopy( pm->ps->origin, start_o );
	VectorCopy( pm->ps->velocity, start_v );

	VectorMA( start_o, -STEPSIZE, normal, down );
	pm->trace( &trace, start_o, pm->mins, pm->maxs, down, pm->ps->clientNum, pm->tracemask, 0 );

	if ( !PM_SlideMove( gravity ) )
	{
		//we can step down
		if ( trace.fraction > 0.01f && trace.fraction < 1.0f &&
		     !trace.allsolid && pml.groundPlane )
		{
			if ( pm->debugLevel > 1 )
			{
				Log::Notice( "%d: step down\n", c_pmove );
			}

			stepped = true;
		}
	}
	else
	{
		// never step up when you still have up velocity
		if ( DotProduct( trace.plane.normal, pm->ps->velocity ) > 0.0f &&
		     ( trace.fraction == 1.0f || DotProduct( trace.plane.normal, normal ) < 0.7f ) )
		{
			return stepped;
		}

		// never step up when flying upwards with the jetpack
		if ( pm->ps->velocity[ 2 ] > 0.0f && ( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE ) )
		{
			return stepped;
		}

		VectorCopy( pm->ps->origin, down_o );
		VectorCopy( pm->ps->velocity, down_v );

		VectorMA( start_o, STEPSIZE, normal, up );

		// test the player position if they were a stepheight higher
		pm->trace( &trace, start_o, pm->mins, pm->maxs, up, pm->ps->clientNum, pm->tracemask, 0 );

		if ( trace.allsolid )
		{
			if ( pm->debugLevel > 1 )
			{
				Log::Notice( "%i:bend can't step\n", c_pmove );
			}

			return stepped; // can't step up
		}

		VectorSubtract( trace.endpos, start_o, step_v );
		VectorCopy( step_v, step_vNormal );
		VectorNormalize( step_vNormal );

		stepSize = DotProduct( normal, step_vNormal ) * VectorLength( step_v );
		// try slidemove from this position
		VectorCopy( trace.endpos, pm->ps->origin );
		VectorCopy( start_v, pm->ps->velocity );

		if ( PM_SlideMove( gravity ) == 0 )
		{
			if ( pm->debugLevel > 1 )
			{
				Log::Notice( "%d: step up\n", c_pmove );
			}

			stepped = true;
		}

		// push down the final amount
		VectorMA( pm->ps->origin, -stepSize, normal, down );
		pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, down, pm->ps->clientNum,
		           pm->tracemask, 0 );

		if ( !trace.allsolid )
		{
			VectorCopy( trace.endpos, pm->ps->origin );
		}

		if ( trace.fraction < 1.0f )
		{
			PM_ClipVelocity( pm->ps->velocity, trace.plane.normal, pm->ps->velocity );
		}
	}

	if ( !predictive && stepped )
	{
		PM_StepEvent( start_o, pm->ps->origin, normal );
	}

	return stepped;
}

/*
==================
PM_PredictStepMove
==================
*/
static bool PM_PredictStepMove()
{
	vec3_t   velocity, origin;
	float    impactSpeed;
	bool stepped = false;

	VectorCopy( pm->ps->velocity, velocity );
	VectorCopy( pm->ps->origin, origin );
	impactSpeed = pml.impactSpeed;

	if ( PM_StepSlideMove( false, true ) )
	{
		stepped = true;
	}

	VectorCopy( velocity, pm->ps->velocity );
	VectorCopy( origin, pm->ps->origin );
	pml.impactSpeed = impactSpeed;

	return stepped;
}

/*
==================
PM_StepEvent
==================
*/
void PM_StepEvent( const vec3_t from, const vec3_t to, const vec3_t normal )
{
	float  size;
	vec3_t delta, dNormal;

	VectorSubtract( from, to, delta );
	VectorCopy( delta, dNormal );
	VectorNormalize( dNormal );

	size = DotProduct( normal, dNormal ) * VectorLength( delta );

	if ( size > 0.0f )
	{
		if ( size > 2.0f )
		{
			if ( size < 7.0f )
			{
				PM_AddEvent( EV_STEPDN_4 );
			}
			else if ( size < 11.0f )
			{
				PM_AddEvent( EV_STEPDN_8 );
			}
			else if ( size < 15.0f )
			{
				PM_AddEvent( EV_STEPDN_12 );
			}
			else
			{
				PM_AddEvent( EV_STEPDN_16 );
			}
		}
	}
	else
	{
		size = fabs( size );

		if ( size > 2.0f )
		{
			if ( size < 7.0f )
			{
				PM_AddEvent( EV_STEP_4 );
			}
			else if ( size < 11.0f )
			{
				PM_AddEvent( EV_STEP_8 );
			}
			else if ( size < 15.0f )
			{
				PM_AddEvent( EV_STEP_12 );
			}
			else
			{
				PM_AddEvent( EV_STEP_16 );
			}
		}
	}

	if ( pm->debugLevel > 1 )
	{
		Log::Notice( "%i:stepped\n", c_pmove );
	}
}

void Slide( vec3_t wishdir, float wishspeed, playerState_t &ps )
{
	float accelerate;
	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	bool slid = ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || ps.pm_flags & PMF_TIME_KNOCKBACK;
	classAttributes_t const* pcl = BG_Class( ps.stats[ STAT_CLASS ] );
	accelerate = slid ? pcl->airAcceleration : pcl->acceleration;
	PM_Accelerate( wishdir, wishspeed, accelerate );

	if ( slid )
	{
		ps.velocity[ 2 ] -= ps.gravity * pml.frametime;
	}
}
