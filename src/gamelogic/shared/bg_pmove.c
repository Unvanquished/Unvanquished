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

// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modified playerstate

#include "../../engine/qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

pmove_t *pm;
pml_t   pml;

int     c_pmove = 0;

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent( int newEvent )
{
	BG_AddPredictableEventToPlayerstate( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt( int entityNum )
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
void PM_StartTorsoAnim( int anim )
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
	if ( PM_Paralyzed( pm->ps->pm_type ) )
	{
		return;
	}

	//legsTimer is clamped too tightly for nonsegmented models
	if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
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

	pm->ps->legsAnim = ( ( pm->ps->legsAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
	                   | anim;
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
	if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
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
	if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
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
void PM_ClipVelocity( const vec3_t in, const vec3_t normal, vec3_t out )
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
static void PM_Friction( void )
{
	vec3_t vec;
	float  *vel;
	float  speed, newspeed, control;
	float  drop;

	vel = pm->ps->velocity;

	// make sure vertical velocity is NOT set to zero when wall climbing
	VectorCopy( vel, vec );

	if ( pml.walking && !( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
	{
		vec[ 2 ] = 0; // ignore slope movement
	}

	speed = VectorLength( vec );

	if ( speed < 0.1f )
		return;

	drop = 0;

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

				control = speed < stopSpeed ? stopSpeed : speed;
				drop += control * friction * pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel )
	{
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR )
	{
		drop += speed * pm_spectatorfriction * pml.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;

	if ( newspeed < 0 )
	{
		newspeed = 0;
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
	int   i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct( pm->ps->velocity, wishdir );
	addspeed = wishspeed - currentspeed;

	if ( addspeed <= 0 )
	{
		return;
	}

	accelspeed = accel * pml.frametime * wishspeed;

	if ( accelspeed > addspeed )
	{
		accelspeed = addspeed;
	}

	for ( i = 0; i < 3; i++ )
	{
		pm->ps->velocity[ i ] += accelspeed * wishdir[ i ];
	}

#else
	// proper way (avoids strafe jump maxspeed bug), but feels bad
	vec3_t wishVelocity;
	vec3_t pushDir;
	float  pushLen;
	float  canPush;

	VectorScale( wishdir, wishspeed, wishVelocity );
	VectorSubtract( wishVelocity, pm->ps->velocity, pushDir );
	pushLen = VectorNormalize( pushDir );

	canPush = accel * pml.frametime * wishspeed;

	if ( canPush > pushLen )
	{
		canPush = pushLen;
	}

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
static float PM_CmdScale( usercmd_t *cmd, qboolean zFlight )
{
	int   max;
	float total;
	float scale;
	float modifier = 1.0f;
	int   staminaJumpCost;

	staminaJumpCost = BG_Class( pm->ps->stats[ STAT_CLASS ] )->staminaJumpCost;

	if ( pm->ps->persistant[ PERS_TEAM ] == TEAM_HUMANS && pm->ps->pm_type == PM_NORMAL )
	{
		qboolean wasSprinting, sprint;

		wasSprinting = sprint = pm->ps->stats[ STAT_STATE ] & SS_SPEEDBOOST;

		// check whether player wants to sprint
		if ( pm->ps->persistant[ PERS_STATE ] & PS_SPRINTTOGGLE )
		{
			// toggle mode
			if ( usercmdButtonPressed( cmd->buttons, BUTTON_SPRINT ) &&
			     !( pm->ps->pm_flags & PMF_SPRINTHELD ) )
			{
				sprint = !sprint;
				pm->ps->pm_flags |= PMF_SPRINTHELD;
			}
			else if ( pm->ps->pm_flags & PMF_SPRINTHELD &&
			          !usercmdButtonPressed( cmd->buttons, BUTTON_SPRINT ) )
			{
				pm->ps->pm_flags &= ~PMF_SPRINTHELD;
			}
		}
		else
		{
			// non-toggle mode
			sprint = usercmdButtonPressed( cmd->buttons, BUTTON_SPRINT );
		}

		// check if sprinting is allowed
		if ( sprint )
		{
			if ( wasSprinting && pm->ps->stats[ STAT_STAMINA ] <= 0 )
			{
				// we were sprinting but ran out of stamina
				sprint = qfalse;
			}
			else if ( !wasSprinting && pm->ps->stats[ STAT_STAMINA ] < staminaJumpCost )
			{
				// stamina is too low to start sprinting
				sprint = qfalse;
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
		if ( sprint && !usercmdButtonPressed( cmd->buttons, BUTTON_WALKING ) )
		{
			modifier *= BG_Class( pm->ps->stats[ STAT_CLASS ] )->sprintMod;
		}
		else
		{
			modifier *= HUMAN_JOG_MODIFIER;
		}

		// Apply modfiers for strafing and going backwards
		if ( cmd->forwardmove < 0 )
		{
			modifier *= HUMAN_BACK_MODIFIER;
		}
		else if ( cmd->rightmove )
		{
			modifier *= HUMAN_SIDE_MODIFIER;
		}

		// Cancel jump if low on stamina
		if ( !zFlight && pm->ps->stats[ STAT_STAMINA ] < staminaJumpCost )
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
		modifier *= 1.0f + ( pm->ps->stats[ STAT_MISC ] * ( LEVEL4_TRAMPLE_SPEED - 1.0f ) /
		                     LEVEL4_TRAMPLE_DURATION );
	}

	// Slow down when charging up for a pounce
	if ( ( pm->ps->weapon == WP_ALEVEL3 || pm->ps->weapon == WP_ALEVEL3_UPG ) &&
	     usercmdButtonPressed( cmd->buttons, BUTTON_ATTACK2 ) )
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

	max = abs( cmd->forwardmove );

	if ( abs( cmd->rightmove ) > max )
	{
		max = abs( cmd->rightmove );
	}

	total = cmd->forwardmove * cmd->forwardmove + cmd->rightmove * cmd->rightmove;

	if ( zFlight )
	{
		if ( abs( cmd->upmove ) > max )
		{
			max = abs( cmd->upmove );
		}

		total += cmd->upmove * cmd->upmove;
	}

	if ( !max )
	{
		return 0;
	}

	total = sqrt( total );

	scale = ( float ) pm->ps->speed * max / ( 127.0 * total ) * modifier;

	return scale;
}

/*
================
PM_SetMovementDir

Determine the rotation of the legs relative to the facing dir
================
*/
static void PM_SetMovementDir( void )
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
static void PM_CheckCharge( void )
{
	if ( pm->ps->weapon != WP_ALEVEL4 )
	{
		return;
	}

	if ( usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK2 ) &&
	     !( pm->ps->stats[ STAT_STATE ] & SS_CHARGING ) )
	{
		pm->ps->pm_flags &= ~PMF_CHARGE;
		return;
	}

	if ( pm->ps->stats[ STAT_MISC ] > 0 )
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
static void PM_CheckWaterPounce( void )
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
static void PM_PlayJumpingAnimation( void )
{
	if ( pm->cmd.forwardmove >= 0 )
	{
		if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim( LEGS_JUMP );
		}
		else
		{
			PM_ForceLegsAnim( NSPA_JUMP );
		}

		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else
	{
		if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim( LEGS_JUMPB );
		}
		else
		{
			PM_ForceLegsAnim( NSPA_JUMPBACK );
		}

		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}
}

/*
=============
PM_CheckPounce
=============
*/
static qboolean PM_CheckPounce( void )
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
			return qfalse;
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

		return qfalse;
	}

	// Check class-specific conditions for starting a pounce
	switch ( pm->ps->weapon )
	{
		case WP_ALEVEL1:
			// Check if player wants to pounce
			if ( !usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK2 ) )
			{
				return qfalse;
			}

			// Check if cooldown is over
			if ( pm->ps->stats[ STAT_MISC ] > 0 )
			{
				return qfalse;
			}
			break;

		case WP_ALEVEL3:
		case WP_ALEVEL3_UPG:
			// Don't pounce while still charging
			if ( usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK2 ) )
			{
				pm->ps->pm_flags &= ~PMF_CHARGE;

				return qfalse;
			}

			// Pounce if minimum charge time has passed and the charge button isn't held anymore
			if ( pm->ps->stats[ STAT_MISC ] < LEVEL3_POUNCE_TIME_MIN )
			{
				return qfalse;
			}
			break;
	}

	// Check if already pouncing
	if ( pm->ps->pm_flags & PMF_CHARGE )
	{
		return qfalse;
	}

	// Check if in air
	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{
		return qfalse;
	}

	// Calculate jump parameters
	switch ( pm->ps->weapon )
	{
		case WP_ALEVEL1:
			// wallwalking (ground surface normal is off more than 45° from Z direction)
			if ( pm->ps->groundEntityNum == ENTITYNUM_WORLD && acos( pml.groundTrace.plane.normal[ 2 ] ) > M_PI / 4.0f )
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
					qboolean foundTrajectory;

					VectorMA( pm->ps->origin, 10000.0f, pml.forward, traceTarget );

					pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, traceTarget,
					           pm->ps->clientNum, MASK_SOLID );

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
								Com_Printf( "[PM_CheckPounce] Aiming at ceiling; move target into surface by %.2f\n",
								            zCorrection );
							}

							VectorMA( endpos, zCorrection, trace.plane.normal, endpos );
						}

						if ( pm->debugLevel > 0 )
						{
							Com_Printf( "[PM_CheckPounce] Trajectory target has a distance of %.1f qu\n",
							            Distance( pm->ps->origin, endpos ) );
						}
					}

					// if there is a possible trajectoy they come in pairs, use the shorter one
					if ( foundTrajectory &&
					     BG_GetTrajectoryPitch( pm->ps->origin, endpos, jumpMagnitude, pm->ps->gravity,
					                            trajAngles, trajDir1, trajDir2 ) )
					{
						int iter;

						if ( pm->debugLevel > 0 )
						{
							Com_Printf( "[PM_CheckPounce] Found trajectory angles: "
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
								Com_Printf("[PM_CheckPounce] Using angle #1\n");
							}

							VectorCopy( trajDir1, jumpDirection );
						}
						else
						{
							if ( pm->debugLevel > 0 )
							{
								Com_Printf("[PM_CheckPounce] Using angle #2\n");
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
									Com_Printf("[PM_CheckPounce] Giving up adjusting jump direction.\n");
								}

								break;
							}

							VectorMA( jumpDirection, 0.1f, pml.groundTrace.plane.normal, jumpDirection );
							VectorNormalize( jumpDirection );

							if ( pm->debugLevel > 0 )
							{
								Com_Printf("[PM_CheckPounce] Adjusting jump direction to get off the surface: "
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
							Com_Printf("[PM_CheckPounce] Failed to find a trajectory\n");
						}

						VectorCopy( pml.forward, jumpDirection );
					}
				}

				// add cooldown
				pm->ps->stats[ STAT_MISC ] = LEVEL1_WALLPOUNCE_COOLDOWN;
			}
			// moving foward or standing still
			else if ( pm->cmd.forwardmove > 0 || ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove == 0 ) )
			{
				// get jump direction
				VectorCopy( pml.forward, jumpDirection );
				jumpDirection[ 2 ] = fabs( jumpDirection[ 2 ] );

				// get pitch towards ground surface
				pitchToGround = ( M_PI / 2.0f ) - acos( DotProduct( pml.groundTrace.plane.normal, jumpDirection ) );

				// get pitch towards XY reference plane
				pitchToRef = ( M_PI / 2.0f ) - acos( DotProduct( up, jumpDirection ) );

				// use the advantageous pitch; allows using an upwards gradiant as a ramp
				pitch = MIN( pitchToGround, pitchToRef );

				// pitches above 45° or below LEVEL1_POUNCE_MINPITCH will result in less than the maximum jump length
				if ( pitch > M_PI / 4.0f )
				{
					pitch = M_PI / 4.0f;
				}
				else if ( pitch < LEVEL1_POUNCE_MINPITCH )
				{
					pitch = LEVEL1_POUNCE_MINPITCH;
				}

				// calculate jump magnitude for given pitch so that jump length is fixed
				jumpMagnitude = ( int )sqrt( LEVEL1_POUNCE_DISTANCE * pm->ps->gravity / sin( 2.0f * pitch ) );

				// add cooldown
				pm->ps->stats[ STAT_MISC ] = LEVEL1_POUNCE_COOLDOWN;
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
				pm->ps->stats[ STAT_MISC ] = LEVEL1_SIDEPOUNCE_COOLDOWN;
			}
			// compilers don't get my epic dijkstra-if
			else
			{
				return qfalse;
			}

			break;

		case WP_ALEVEL3:
		case WP_ALEVEL3_UPG:
			if ( pm->ps->weapon == WP_ALEVEL3 )
			{
				jumpMagnitude = pm->ps->stats[ STAT_MISC ]
				              * LEVEL3_POUNCE_JUMP_MAG / LEVEL3_POUNCE_TIME;
			}
			else
			{
				jumpMagnitude = pm->ps->stats[ STAT_MISC ]
				              * LEVEL3_POUNCE_JUMP_MAG_UPG / LEVEL3_POUNCE_TIME_UPG;
			}

			VectorCopy( pml.forward, jumpDirection );

			// save payload
			pm->pmext->pouncePayload = pm->ps->stats[ STAT_MISC ];

			// reset charge bar
			pm->ps->stats[ STAT_MISC ] = 0;

			break;

		default:
			return qfalse;
	}

	// Prepare a simulated jump
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_CHARGE;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	// Jump
	VectorMA( pm->ps->velocity, jumpMagnitude, jumpDirection, pm->ps->velocity );
	PM_AddEvent( EV_JUMP );
	PM_PlayJumpingAnimation();

	// We started to pounce
	return qtrue;
}

/*
=============
PM_CheckWallJump
=============
*/
static qboolean PM_CheckWallJump( void )
{
	vec3_t  dir, forward, right, movedir, point;
	float   normalFraction = 1.5f;
	float   cmdFraction = 1.0f;
	float   upFraction = 1.5f;
	trace_t trace;

	static const vec3_t  refNormal = { 0.0f, 0.0f, 1.0f };

	if ( !( BG_Class( pm->ps->stats[ STAT_CLASS ] )->abilities & SCA_WALLJUMPER ) )
	{
		return qfalse;
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
	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

	if ( trace.fraction < 1.0f &&
	     !( trace.surfaceFlags & ( SURF_SKY | SURF_SLICK ) ) &&
	     trace.plane.normal[ 2 ] < MIN_WALK_NORMAL )
	{
		VectorCopy( trace.plane.normal, pm->ps->grapplePoint );
	}
	else
	{
		return qfalse;
	}

	if ( pm->ps->pm_flags & PMF_RESPAWNED )
	{
		return qfalse; // don't allow jump until all buttons are up
	}

	if ( pm->cmd.upmove < 10 )
	{
		// not holding jump
		return qfalse;
	}

	if ( pm->ps->pm_flags & PMF_TIME_WALLJUMP )
	{
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD &&
	     pm->ps->grapplePoint[ 2 ] == 1.0f )
	{
		return qfalse;
	}

	pm->ps->pm_flags |= PMF_TIME_WALLJUMP;
	pm->ps->pm_time = 200;

	pml.groundPlane = qfalse; // jumping away
	pml.walking = qfalse;
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

	return qtrue;
}

/**
 * @brief PM_CheckJetpack
 * @return qtrue if and only if thrust was applied
 */
static qboolean PM_CheckJetpack( void )
{
	static const vec3_t thrustDir = { 0.0f, 0.0f, 1.0f };
	int                 sideVelocity;

	if ( pm->ps->pm_type != PM_NORMAL ||
	     pm->ps->persistant[ PERS_TEAM ] != TEAM_HUMANS ||
	     !BG_InventoryContainsUpgrade( UP_JETPACK, pm->ps->stats ) )
	{
		pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_ACTIVE;
		pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_WARM;
		pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_ENABLED;

		return qfalse;
	}

	// enable jetpack when in air
	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE &&
	     !( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ENABLED ) )
	{
		if ( pm->debugLevel > 0 )
		{
			Com_Printf( "[PM_CheckJetpack] " S_COLOR_CYAN "Jetpack enabled\n" );
		}

		pm->ps->stats[ STAT_STATE2 ] |= SS2_JETPACK_ENABLED;
		PM_AddEvent( EV_JETPACK_ENABLE );

		return qfalse;
	}

	// if jump key not held stop active thrust
	if ( pm->cmd.upmove < 10 )
	{
		if ( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
		{
			if ( pm->debugLevel > 0 && pm->cmd.upmove < 10 )
			{
				Com_Printf( "[PM_CheckJetpack] " S_COLOR_LTORANGE "Jetpack thrust stopped (jump key released)\n" );
			}

			pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_ACTIVE;
			PM_AddEvent( EV_JETPACK_STOP );
		}

		return qfalse;
	}

	// sanity check that jetpack is enabled at this point
	if ( !( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ENABLED ) )
	{
		if ( pm->debugLevel > 0 )
		{
			Com_Printf( "[PM_CheckJetpack] " S_COLOR_RED "Can't start jetpack thrust (jetpack not enabled)\n" );
		}

		return qfalse;
	}

	// check ignite conditions
	if ( !( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_WARM ) )
	{
		sideVelocity = sqrt( pm->ps->velocity[ 0 ] * pm->ps->velocity[ 0 ] +
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
				return qfalse;
			}

			// minimum fuel required to start from a jump
			if ( pm->ps->stats[ STAT_FUEL ] < JETPACK_FUEL_LOW )
			{
				return qfalse;
			}

			// wait until at highest spot
			if ( !( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE ) && pm->ps->velocity[ 2 ] > 0.0f )
			{
				return qfalse;
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
				Com_Printf( "[PM_CheckJetpack] " S_COLOR_LTORANGE "Jetpack thrust stopped (out of fuel)\n" );
			}

			pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_ACTIVE;
			PM_AddEvent( EV_JETPACK_STOP );
		}

		return qfalse;
	}

	// start thrusting if possible
	if ( !( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE ) )
	{
		// minimum fuel required
		if ( pm->ps->stats[ STAT_FUEL ] < JETPACK_FUEL_STOP )
		{
			return qfalse;
		}

		if ( pm->debugLevel > 0 )
		{
			Com_Printf( "[PM_CheckJetpack] " S_COLOR_GREEN "Jetpack thrust started\n" );
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

	return qtrue;
}

/**
 * @brief Restores jetpack fuel
 * @return qtrue if and only if fuel has been restored
 */
static qboolean PM_CheckJetpackRestoreFuel( void )
{
	// don't restore fuel when full or jetpack active
	if ( pm->ps->stats[ STAT_FUEL ] == JETPACK_FUEL_MAX ||
	     pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
	{
		return qfalse;
	}

	pm->ps->stats[ STAT_FUEL ] += pml.msec * JETPACK_FUEL_RESTORE;

	if ( pm->ps->stats[ STAT_FUEL ] > JETPACK_FUEL_MAX )
	{
		pm->ps->stats[ STAT_FUEL ] = JETPACK_FUEL_MAX;
	}

	return qtrue;
}

/**
 * @brief Disables the jetpack. Without force, the call can get ignored based on previous velocity.
 */
static void PM_LandJetpack( qboolean force )
{
	float angle, sideVelocity;

	// when low on fuel, always force a landing
	if ( pm->ps->stats[ STAT_FUEL ] < JETPACK_FUEL_LOW )
	{
		force = qtrue;
	}

	sideVelocity = sqrt( pml.previous_velocity[ 0 ] * pml.previous_velocity[ 0 ] +
	                     pml.previous_velocity[ 1 ] * pml.previous_velocity[ 1 ] );

	angle = atan2( -pml.previous_velocity[ 2 ], sideVelocity );

	// allow the player to jump instead of land for some impacts
	if ( !force )
	{
		if ( angle > 0.0f && angle < M_PI / 4.0f ) // 45°
		{
			if ( pm->debugLevel > 0 )
			{
				Com_Printf( "[PM_LandJetpack] Landing ignored (hit surface at %.0f°)\n", RAD2DEG( angle ) );
			}

			return;
		}
	}

	if ( pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
	{
		if ( pm->debugLevel > 0 )
		{
			Com_Printf( "[PM_LandJetpack] " S_COLOR_LTORANGE "Jetpack thrust stopped (hit surface at %.0f°)%s\n", RAD2DEG( angle ),
			            force ? S_COLOR_RED " (FORCED)" : "" );
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
			Com_Printf( "[PM_LandJetpack] " S_COLOR_YELLOW "Jetpack disabled (hit surface at %.0f°)%s\n", RAD2DEG( angle ),
			            force ? S_COLOR_RED " (FORCED)" : "" );
		}

		pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_WARM;
		pm->ps->stats[ STAT_STATE2 ] &= ~SS2_JETPACK_ENABLED;

		PM_AddEvent( EV_JETPACK_DISABLE );
	}
}

static qboolean PM_CheckJump( void )
{
	vec3_t   normal;
	int      staminaJumpCost;
	float    magnitude;
	qboolean jetpackJump;

	// can't jump while in air
	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{
		return qfalse;
	}

	// check if holding jump key
	if ( pm->cmd.upmove < 10 )
	{
		return qfalse;
	}

	// needs jump ability
	if ( BG_Class( pm->ps->stats[ STAT_CLASS ] )->jumpMagnitude <= 0.0f )
	{
		return qfalse;
	}

	// can't jump and pounce at the same time
	// TODO: This prevents jumps in an unintuitive manner, since the charge
	//       meter has nothing to do with the land time.
	if ( ( pm->ps->stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL3 ||
	       pm->ps->stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL3_UPG ) &&
	     pm->ps->stats[ STAT_MISC ] > 0 )
	{
		return qfalse;
	}

	// can't jump and charge at the same time
	if ( pm->ps->stats[ STAT_CLASS ] == PCL_ALIEN_LEVEL4 && pm->ps->stats[ STAT_MISC ] > 0 )
	{
		return qfalse;
	}

	// don't allow jump until all buttons are up (?)
	if ( pm->ps->pm_flags & PMF_RESPAWNED )
	{
		return qfalse;
	}

	// can't jump whilst grabbed
	if ( pm->ps->pm_type == PM_GRABBED )
	{
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD )
	{
		return qfalse;
	}

	staminaJumpCost = BG_Class( pm->ps->stats[ STAT_CLASS ] )->staminaJumpCost;
	jetpackJump     = qfalse;

	// humans need stamina or jetpack to jump
	if ( ( pm->ps->persistant[ PERS_TEAM ] == TEAM_HUMANS ) &&
	     ( pm->ps->stats[ STAT_STAMINA ] < staminaJumpCost ) )
	{
		// use jetpack instead of stamina to take off
		if ( BG_InventoryContainsUpgrade( UP_JETPACK, pm->ps->stats ) &&
		     pm->ps->stats[ STAT_FUEL ] > JETPACK_FUEL_LOW )
		{
			jetpackJump = qtrue;
		}
		else
		{
			return qfalse;
		}
	}

	// take some stamina off
	if ( !jetpackJump && pm->ps->persistant[ PERS_TEAM ] == TEAM_HUMANS )
	{
		pm->ps->stats[ STAT_STAMINA ] -= staminaJumpCost;
	}

	// go into jump mode
	pml.groundPlane = qfalse;
	pml.walking     = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;
	pm->ps->pm_flags |= PMF_JUMPED;
	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	// don't allow walljump for a short while after jumping from the ground
	// TODO: There was an issue about this potentially having side effects.
	if ( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_WALLJUMPER ) )
	{
		pm->ps->pm_flags |= PMF_TIME_WALLJUMP;
		pm->ps->pm_time = 200;
	}

	// jump in surface normal direction
	BG_GetClientNormal( pm->ps, normal );

	// retrieve jump magnitude
	magnitude = BG_Class( pm->ps->stats[ STAT_CLASS ] )->jumpMagnitude;

	// if jetpack is active or being used for the jump, scale down jump magnitude
	if ( jetpackJump || pm->ps->stats[ STAT_STATE2 ] & SS2_JETPACK_ACTIVE )
	{
		if ( pm->debugLevel > 0 )
		{
			Com_Printf( "[PM_CheckJump] Using jetpack: Decreasing jump magnitude to %.0f%%\n",
			            JETPACK_JUMPMAG_REDUCTION * 100.0f );
		}

		magnitude *= JETPACK_JUMPMAG_REDUCTION;
	}

	// sanity clip velocity Z
	if ( pm->ps->velocity[ 2 ] < 0.0f )
	{
		pm->ps->velocity[ 2 ] = 0.0f;
	}

	VectorMA( pm->ps->velocity, magnitude, normal, pm->ps->velocity );

	PM_AddEvent( EV_JUMP );
	PM_PlayJumpingAnimation();

	return qtrue;
}

static qboolean PM_CheckWaterJump( void )
{
	vec3_t spot;
	int    cont;
	vec3_t flatforward;

	if ( pm->ps->pm_time )
	{
		return qfalse;
	}

	// check for water jump
	if ( pm->waterlevel != 2 )
	{
		return qfalse;
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
		return qfalse;
	}

	spot[ 2 ] += 16;
	cont = pm->pointcontents( spot, pm->ps->clientNum );

	if ( cont )
	{
		return qfalse;
	}

	// jump out of water
	VectorScale( pml.forward, 200, pm->ps->velocity );
	pm->ps->velocity[ 2 ] = 350;

	pm->ps->pm_flags |= PMF_TIME_WATERJUMP;
	pm->ps->pm_time = 2000;

	return qtrue;
}

//============================================================================

/*
===================
PM_WaterJumpMove

Flying out of the water
===================
*/
static void PM_WaterJumpMove( void )
{
	// waterjump has no control, but falls

	PM_StepSlideMove( qtrue, qfalse );

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
static void PM_WaterMove( void )
{
	int    i;
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;
	float  vel;

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

	scale = PM_CmdScale( &pm->cmd, qtrue );

	//
	// user intentions
	//

	for ( i = 0; i < 3; i++ )
	{
		wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;
	}

	wishvel[ 2 ] += scale * pm->cmd.upmove;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	if ( wishspeed > pm->ps->speed * pm_swimScale )
	{
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate( wishdir, wishspeed, pm_wateraccelerate );

	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 )
	{
		vel = VectorLength( pm->ps->velocity );
		// slide along the ground plane
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
	}

	PM_SlideMove( qfalse );
}

static void PM_FlyMove( void )
{
	int    i;
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;

	PM_Friction();

	scale = PM_CmdScale( &pm->cmd, qtrue );

	//
	// user intentions
	//
	if ( !scale )
	{
		wishvel[ 0 ] = 0;
		wishvel[ 1 ] = 0;
		wishvel[ 2 ] = 0;
	}
	else
	{
		for ( i = 0; i < 3; i++ )
		{
			wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;
		}

		wishvel[ 2 ] += scale * pm->cmd.upmove;
	}

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	PM_Accelerate( wishdir, wishspeed, pm_flyaccelerate );

	PM_StepSlideMove( qfalse, qfalse );
}

/*
===================
PM_AirMove

===================
*/
static void PM_AirMove( void )
{
	int       i;
	vec3_t    wishvel;
	float     fmove, smove;
	vec3_t    wishdir;
	float     wishspeed;
	float     scale;
	usercmd_t cmd;

	PM_CheckWallJump();
	PM_CheckJetpack();

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd, qfalse );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[ 2 ] = 0;
	pml.right[ 2 ] = 0;
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	for ( i = 0; i < 2; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;
	}

	wishvel[ 2 ] = 0;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

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

	PM_StepSlideMove( qtrue, qfalse );
}

/*
===================
PM_ClimbMove

===================
*/
static void PM_ClimbMove( void )
{
	int       i;
	vec3_t    wishvel;
	float     fmove, smove;
	vec3_t    wishdir;
	float     wishspeed;
	float     scale;
	usercmd_t cmd;
	float     accelerate;
	float     vel;

	if ( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 )
	{
		// begin swimming
		PM_WaterMove();
		return;
	}

	if ( PM_CheckJump() || PM_CheckPounce() )
	{
		// jumped away
		if ( pm->waterlevel > 1 )
		{
			PM_WaterMove();
		}
		else
		{
			PM_AirMove();
		}

		return;
	}

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd, qfalse );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity( pml.forward, pml.groundTrace.plane.normal, pml.forward );
	PM_ClipVelocity( pml.right, pml.groundTrace.plane.normal, pml.right );
	//
	VectorNormalize( pml.forward );
	VectorNormalize( pml.right );

	for ( i = 0; i < 3; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;
	}

	// when going up or down slopes the wish velocity should Not be zero
//  wishvel[2] = 0;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

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

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
	{
		accelerate = BG_Class( pm->ps->stats[ STAT_CLASS ] )->airAcceleration;
	}
	else
	{
		accelerate = BG_Class( pm->ps->stats[ STAT_CLASS ] )->acceleration;
	}

	PM_Accelerate( wishdir, wishspeed, accelerate );

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
	{
		pm->ps->velocity[ 2 ] -= pm->ps->gravity * pml.frametime;
	}

	vel = VectorLength( pm->ps->velocity );

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

	PM_StepSlideMove( qfalse, qfalse );
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove( void )
{
	int       i;
	vec3_t    wishvel;
	float     fmove, smove;
	vec3_t    wishdir;
	float     wishspeed;
	float     scale;
	usercmd_t cmd;
	float     accelerate;

	if ( pm->waterlevel > 2 && DotProduct( pml.forward, pml.groundTrace.plane.normal ) > 0 )
	{
		// begin swimming
		PM_WaterMove();
		return;
	}

	if ( PM_CheckJump() || PM_CheckPounce() )
	{
		if ( pm->waterlevel > 1 )
		{
			PM_WaterMove();
		}
		else
		{
			PM_AirMove();
		}

		return;
	}

	// if PM_Land didn't stop the jetpack (e.g. to allow for a jump) but we didn't get away
	// from the ground, stop it now
	PM_LandJetpack( qtrue );

	PM_CheckCharge();

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale( &cmd, qfalse );

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

	for ( i = 0; i < 3; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;
	}

	// when going up or down slopes the wish velocity should Not be zero
//  wishvel[2] = 0;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

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

	// when a player gets hit, they temporarily lose
	// full control, which allows them to be moved a bit
	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
	{
		accelerate = BG_Class( pm->ps->stats[ STAT_CLASS ] )->airAcceleration;
	}
	else
	{
		accelerate = BG_Class( pm->ps->stats[ STAT_CLASS ] )->acceleration;
	}

	PM_Accelerate( wishdir, wishspeed, accelerate );

	//Com_Printf("velocity = %1.1f %1.1f %1.1f\n", pm->ps->velocity[0], pm->ps->velocity[1], pm->ps->velocity[2]);
	//Com_Printf("velocity1 = %1.1f\n", VectorLength(pm->ps->velocity));

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
	{
		pm->ps->velocity[ 2 ] -= pm->ps->gravity * pml.frametime;
	}
	else
	{
		// don't reset the z velocity for slopes
//    pm->ps->velocity[2] = 0;
	}

	// slide along the ground plane
	PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

	// don't do anything if standing still
	if ( !pm->ps->velocity[ 0 ] && !pm->ps->velocity[ 1 ] )
	{
		return;
	}

	PM_StepSlideMove( qfalse, qfalse );

	//Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));
}

/*
===================
PM_LadderMove

Basically a rip of PM_WaterMove with a few changes
===================
*/
static void PM_LadderMove( void )
{
	int    i;
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;
	float  vel;

	PM_Friction();

	scale = PM_CmdScale( &pm->cmd, qtrue );

	for ( i = 0; i < 3; i++ )
	{
		wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;
	}

	wishvel[ 2 ] += scale * pm->cmd.upmove;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );

	if ( wishspeed > pm->ps->speed * pm_swimScale )
	{
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	//slanty ladders
	if ( pml.groundPlane && DotProduct( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0.0f )
	{
		vel = VectorLength( pm->ps->velocity );

		// slide along the ground plane
		PM_ClipVelocity( pm->ps->velocity, pml.groundTrace.plane.normal, pm->ps->velocity );

		VectorNormalize( pm->ps->velocity );
		VectorScale( pm->ps->velocity, vel, pm->ps->velocity );
	}

	PM_SlideMove( qfalse );
}

/*
=============
PM_CheckLadder

Check to see if the player is on a ladder or not
=============
*/
static void PM_CheckLadder( void )
{
	vec3_t  forward, end;
	trace_t trace;

	//test if class can use ladders
	if ( !BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_CANUSELADDERS ) )
	{
		pml.ladder = qfalse;
		return;
	}

	VectorCopy( pml.forward, forward );
	forward[ 2 ] = 0.0f;

	VectorMA( pm->ps->origin, 1.0f, forward, end );

	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, MASK_PLAYERSOLID );

	if ( ( trace.fraction < 1.0f ) && ( trace.surfaceFlags & SURF_LADDER ) )
	{
		pml.ladder = qtrue;
	}
	else
	{
		pml.ladder = qfalse;
	}
}

/*
==============
PM_DeadMove
==============
*/
static void PM_DeadMove( void )
{
	float forward;

	if ( !pml.walking )
	{
		return;
	}

	// extra friction

	forward = VectorLength( pm->ps->velocity );
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

/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove( void )
{
	float  speed, drop, friction, control, newspeed;
	int    i;
	vec3_t wishvel;
	float  fmove, smove;
	vec3_t wishdir;
	float  wishspeed;
	float  scale;

	// friction

	speed = VectorLength( pm->ps->velocity );

	if ( speed < 1 )
	{
		VectorCopy( vec3_origin, pm->ps->velocity );
	}
	else
	{
		drop = 0;

		friction = pm_friction * 1.5; // extra friction
		control = speed < pm_stopspeed ? pm_stopspeed : speed;
		drop += control * friction * pml.frametime;

		// scale the velocity
		newspeed = speed - drop;

		if ( newspeed < 0 )
		{
			newspeed = 0;
		}

		newspeed /= speed;

		VectorScale( pm->ps->velocity, newspeed, pm->ps->velocity );
	}

	// accelerate
	scale = PM_CmdScale( &pm->cmd, qtrue );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for ( i = 0; i < 3; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;
	}

	wishvel[ 2 ] += pm->cmd.upmove;

	VectorCopy( wishvel, wishdir );
	wishspeed = VectorNormalize( wishdir );
	wishspeed *= scale;

	PM_Accelerate( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA( pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin );
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number appropriate for the groundsurface
================
*/
static int PM_FootstepForSurface( void )
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
static void PM_Land( void )
{
	PM_LandJetpack( qfalse ); // don't force a stop, sometimes we can push off with a jump

	// decide which landing animation to use
	if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP )
	{
		if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim( LEGS_LANDB );
		}
		else
		{
			PM_ForceLegsAnim( NSPA_LANDBACK );
		}
	}
	else
	{
		if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim( LEGS_LAND );
		}
		else
		{
			PM_ForceLegsAnim( NSPA_LAND );
		}
	}

	if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
	{
		pm->ps->legsTimer = TIMER_LAND;
	}
	else
	{
		pm->ps->torsoTimer = TIMER_LAND;
	}

	// potential jump ended
	pm->ps->pm_flags &= ~PMF_JUMPED;
}

/*
=================
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand( void )
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

	t = ( -b - sqrt( den ) ) / ( 2 * a );

	delta = vel + t * acc;
	delta = delta * delta * 0.0001;

	// never take falling damage if completely underwater
	if ( pm->waterlevel == 3 )
	{
		return;
	}

	// reduce falling damage if there is standing water
	if ( pm->waterlevel == 2 )
	{
		delta *= 0.25;
	}

	if ( pm->waterlevel == 1 )
	{
		delta *= 0.5;
	}

	if ( delta < 1 )
	{
		return;
	}

	// create a local entity event to play the sound

	// SURF_NODAMAGE is used for bounce pads where you don't ever
	// want to take damage or play a crunch sound
	if ( !( pml.groundTrace.surfaceFlags & SURF_NODAMAGE ) )
	{
		pm->ps->stats[ STAT_FALLDIST ] = delta;

		if ( delta > AVG_FALL_DISTANCE )
		{
			if ( PM_Live( pm->ps->pm_type ) )
			{
				PM_AddEvent( EV_FALL_FAR );
			}
		}
		else if ( delta > MIN_FALL_DISTANCE )
		{
			if ( PM_Live( pm->ps->pm_type ) )
			{
				PM_AddEvent( EV_FALL_MEDIUM );
			}
		}
		else
		{
			if ( delta > 7 )
			{
				PM_AddEvent( EV_FALL_SHORT );
			}
			else
			{
				PM_AddEvent( PM_FootstepForSurface() );
			}
		}
	}

	// start footstep cycle over
	pm->ps->bobCycle = 0;
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
		Com_Printf( "%i:allsolid\n", c_pmove );
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
				pm->trace( trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

				if ( !trace->allsolid )
				{
					point[ 0 ] = pm->ps->origin[ 0 ];
					point[ 1 ] = pm->ps->origin[ 1 ];
					point[ 2 ] = pm->ps->origin[ 2 ] - 0.25;

					pm->trace( trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
					pml.groundTrace = *trace;
					return qtrue;
				}
			}
		}
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;
	pml.groundPlane = qfalse;
	pml.walking = qfalse;

	return qfalse;
}

/*
=============
PM_GroundTraceMissed

The ground trace didn't hit a surface, so we are in freefall
=============
*/
static void PM_GroundTraceMissed( void )
{
	trace_t trace;
	vec3_t  point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
	{
		// we just transitioned into freefall
		if ( pm->debugLevel > 1 )
		{
			Com_Printf( "%i:lift\n", c_pmove );
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy( pm->ps->origin, point );
		point[ 2 ] -= 64.0f;

		pm->trace( &trace, pm->ps->origin, NULL, NULL, point, pm->ps->clientNum, pm->tracemask );

		if ( trace.fraction == 1.0f )
		{
			if ( pm->cmd.forwardmove >= 0 )
			{
				if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ForceLegsAnim( LEGS_JUMP );
				}
				else
				{
					PM_ForceLegsAnim( NSPA_JUMP );
				}

				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}
			else
			{
				if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ForceLegsAnim( LEGS_JUMPB );
				}
				else
				{
					PM_ForceLegsAnim( NSPA_JUMPBACK );
				}

				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
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
	pml.groundPlane = qfalse;
	pml.walking = qfalse;
}

/*
=============
PM_GroundClimbTrace
=============
*/
// ATP - Attachpoints for wallwalking
// order is important!
enum {
	GCT_ATP_MOVEDIRECTION,
	GCT_ATP_GROUND,
	GCT_ATP_STEPMOVE,
	GCT_ATP_UNDERNEATH,
	GCT_ATP_CEILING,
	GCT_ATP_FALLBACK,
	NUM_GCT_ATP
};

static void PM_GroundClimbTrace( void )
{
	vec3_t      surfNormal, moveDir, lookDir, point, velocityDir;
	vec3_t      toAngles, surfAngles;
	trace_t     trace;
	int         i;
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
	for ( i = 0; i <= NUM_GCT_ATP; i++ )
	{
		switch ( i )
		{
			case GCT_ATP_MOVEDIRECTION:
				// we are going to step this frame so skip the transition test
				if ( PM_PredictStepMove() )
				{
					continue;
				}

				// trace into direction we are moving
				VectorMA( pm->ps->origin, 0.25f, moveDir, point );
				pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

				break;

			case GCT_ATP_GROUND:
				// trace straight down onto "ground" surface
				// mask out CONTENTS_BODY to not hit other players and avoid the camera flipping out when
				// wallwalkers touch
				VectorMA( pm->ps->origin, -0.25f, surfNormal, point );
				pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask & ~CONTENTS_BODY );

				break;

			case GCT_ATP_STEPMOVE:
				if ( pml.groundPlane != qfalse && PM_PredictStepMove() )
				{
					// step down
					VectorMA( pm->ps->origin, -STEPSIZE, surfNormal, point );
					pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
				}
				else
				{
					continue;
				}

				break;

			case GCT_ATP_UNDERNEATH:
				// trace "underneath" BBOX so we can traverse angles > 180deg
				if ( pml.groundPlane != qfalse )
				{
					VectorMA( pm->ps->origin, -16.0f, surfNormal, point );
					VectorMA( point, -16.0f, moveDir, point );
					pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
				}
				else
				{
					continue;
				}

				break;

			case GCT_ATP_CEILING:
				// attach to the ceiling if we get close enough during a jump that goes roughly upwards
				if ( velocityDir[ 2 ] > 0.2f ) // acos( 0.2f ) ~= 80°
				{
					VectorMA( pm->ps->origin, -16.0f, ceilingNormal, point );
					pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask & ~CONTENTS_BODY );
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
				pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

				break;
		}

		// check if we hit something
		if ( trace.fraction < 1.0f && !( trace.surfaceFlags & ( SURF_SKY | SURF_SLICK ) ) &&
		     !( trace.entityNum != ENTITYNUM_WORLD && i != 4 ) )
		{
			// check if we attached to a new surface (?)
			if ( i == GCT_ATP_STEPMOVE || i == GCT_ATP_UNDERNEATH || i == GCT_ATP_CEILING )
			{
				// add step event if necessary
				if ( i == GCT_ATP_STEPMOVE )
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
			traceANGsurf = RAD2DEG( acos( traceDOTsurf ) );

			if ( traceANGsurf > 180.0f )
			{
				traceANGsurf -= 180.0f;
			}

			// calculate angle between trace and ref
			traceDOTref = DotProduct( trace.plane.normal, refNormal );
			traceANGref = RAD2DEG( acos( traceDOTref ) );

			if ( traceANGref > 180.0f )
			{
				traceANGref -= 180.0f;
			}

			// calculate angle between surf and ref
			surfDOTref = DotProduct( surfNormal, refNormal );
			surfANGref = RAD2DEG( acos( surfDOTref ) );

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
					rTtANGrTsTt = ANGLE2SHORT( RAD2DEG( acos( rTtDOTrTsTt ) ) );

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
					if ( fabs( DotProduct( surfNormal, trace.plane.normal ) ) < ( 1.0f - eps ) )
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
		pml.groundPlane = qfalse;
		pml.walking = qfalse;

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

				AngleVectors( pm->ps->viewangles, forward, NULL, NULL );

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
	pml.groundPlane = qtrue;
	pml.walking = qtrue;

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
static void PM_GroundTrace( void )
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
				if ( !( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
				{
					pm->ps->stats[ STAT_STATE ] |= SS_WALLCLIMBING;
				}
				else if ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING )
				{
					pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBING;
				}

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
			else if ( pm->cmd.upmove >= 0 )
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
				AngleVectors( pm->ps->viewangles, forward, NULL, NULL );
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

	pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid )
	{
		if ( !PM_CorrectAllSolid( &trace ) )
		{
			return;
		}
	}

	//make sure that the surfNormal is reset to the ground
	VectorCopy( refNormal, pm->ps->grapplePoint );

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0f )
	{
		qboolean steppedDown = qfalse;

		// try to step down
		if ( pml.groundPlane && PM_PredictStepMove() )
		{
			//step down
			point[ 0 ] = pm->ps->origin[ 0 ];
			point[ 1 ] = pm->ps->origin[ 1 ];
			point[ 2 ] = pm->ps->origin[ 2 ] - STEPSIZE;
			pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

			//if we hit something
			if ( trace.fraction < 1.0f )
			{
				PM_StepEvent( pm->ps->origin, trace.endpos, refNormal );
				VectorCopy( trace.endpos, pm->ps->origin );
				steppedDown = qtrue;
			}
		}

		if ( !steppedDown )
		{
			PM_GroundTraceMissed();
			pml.groundPlane = qfalse;
			pml.walking = qfalse;

			return;
		}
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[ 2 ] > 0.0f && DotProduct( pm->ps->velocity, trace.plane.normal ) > 10.0f )
	{
		if ( pm->debugLevel > 1 )
		{
			Com_Printf( "%i:kickoff\n", c_pmove );
		}

		// go into jump animation
		if ( pm->cmd.forwardmove >= 0 )
		{
			if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
			{
				PM_ForceLegsAnim( LEGS_JUMP );
			}
			else
			{
				PM_ForceLegsAnim( NSPA_JUMP );
			}

			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}
		else
		{
			if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
			{
				PM_ForceLegsAnim( LEGS_JUMPB );
			}
			else
			{
				PM_ForceLegsAnim( NSPA_JUMPBACK );
			}

			pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
		}

		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		return;
	}

	// slopes that are too steep will not be considered onground
	if ( trace.plane.normal[ 2 ] < MIN_WALK_NORMAL )
	{
		if ( pm->debugLevel > 1 )
		{
			Com_Printf( "%i:steep\n", c_pmove );
		}

		// FIXME: if they can't slide down the slope, let them
		// walk (sharp crevices)
		pm->ps->groundEntityNum = ENTITYNUM_NONE;
		pml.groundPlane = qtrue;
		pml.walking = qfalse;
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

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
			Com_Printf( "%i:Land\n", c_pmove );
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
static void PM_SetWaterLevel( void )
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

	point[ 0 ] = pm->ps->origin[ 0 ];
	point[ 1 ] = pm->ps->origin[ 1 ];
	point[ 2 ] = pm->ps->origin[ 2 ] + MINS_Z + 1;
	cont = pm->pointcontents( point, pm->ps->clientNum );

	if ( cont & MASK_WATER )
	{
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[ 2 ] = pm->ps->origin[ 2 ] + MINS_Z + sample1;
		cont = pm->pointcontents( point, pm->ps->clientNum );

		if ( cont & MASK_WATER )
		{
			pm->waterlevel = 2;
			point[ 2 ] = pm->ps->origin[ 2 ] + MINS_Z + sample2;
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
static void PM_SetViewheight( void )
{
	pm->ps->viewheight = ( pm->ps->pm_flags & PMF_DUCKED )
	                     ? BG_ClassModelConfig( pm->ps->stats[ STAT_CLASS ] )->crouchViewheight
	                     : BG_ClassModelConfig( pm->ps->stats[ STAT_CLASS ] )->viewheight;
}

/*
==============
PM_CheckDuck

Sets mins and maxs, and calls PM_SetViewheight
==============
*/
static void PM_CheckDuck( void )
{
	trace_t trace;
	vec3_t  PCmins, PCmaxs, PCcmaxs;

	BG_ClassBoundingBox( pm->ps->stats[ STAT_CLASS ], PCmins, PCmaxs, PCcmaxs, NULL, NULL );

	pm->mins[ 0 ] = PCmins[ 0 ];
	pm->mins[ 1 ] = PCmins[ 1 ];

	pm->maxs[ 0 ] = PCmaxs[ 0 ];
	pm->maxs[ 1 ] = PCmaxs[ 1 ];

	pm->mins[ 2 ] = PCmins[ 2 ];

	if ( pm->ps->pm_type == PM_DEAD )
	{
		pm->maxs[ 2 ] = -8;
		pm->ps->viewheight = PCmins[ 2 ] + DEAD_VIEWHEIGHT;
		return;
	}

	// If the standing and crouching bboxes are the same the class can't crouch
	if ( pm->cmd.upmove < 0 && !VectorCompare( PCmaxs, PCcmaxs ) )
	{
		// duck
		pm->ps->pm_flags |= PMF_DUCKED;
	}
	else
	{
		// stand up if possible
		if ( pm->ps->pm_flags & PMF_DUCKED )
		{
			// try to stand up
			pm->maxs[ 2 ] = PCmaxs[ 2 ];
			pm->trace( &trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );

			if ( !trace.allsolid )
			{
				pm->ps->pm_flags &= ~PMF_DUCKED;
			}
		}
	}

	if ( pm->ps->pm_flags & PMF_DUCKED )
	{
		pm->maxs[ 2 ] = PCcmaxs[ 2 ];
	}
	else
	{
		pm->maxs[ 2 ] = PCmaxs[ 2 ];
	}

	PM_SetViewheight();
}

//===================================================================

/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps( void )
{
	float    bobmove;
	int      old;
	qboolean footstep;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	if ( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_WALLCLIMBER ) && ( pml.groundPlane ) )
	{
		// FIXME: yes yes i know this is wrong
		pm->xyspeed = sqrt( pm->ps->velocity[ 0 ] * pm->ps->velocity[ 0 ]
		                    + pm->ps->velocity[ 1 ] * pm->ps->velocity[ 1 ]
		                    + pm->ps->velocity[ 2 ] * pm->ps->velocity[ 2 ] );
	}
	else
	{
		pm->xyspeed = sqrt( pm->ps->velocity[ 0 ] * pm->ps->velocity[ 0 ]
		                    + pm->ps->velocity[ 1 ] * pm->ps->velocity[ 1 ] );
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{
		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 )
		{
			if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
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

	// if not trying to move
	if ( !pm->cmd.forwardmove && !pm->cmd.rightmove )
	{
		if ( pm->xyspeed < 5 )
		{
			pm->ps->bobCycle = 0; // start at beginning of cycle again

			if ( pm->ps->pm_flags & PMF_DUCKED )
			{
				if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim( LEGS_IDLECR );
				}
				else
				{
					PM_ContinueLegsAnim( NSPA_STAND );
				}
			}
			else
			{
				if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim( LEGS_IDLE );
				}
				else
				{
					PM_ContinueLegsAnim( NSPA_STAND );
				}
			}
		}

		return;
	}

	footstep = qfalse;

	if ( pm->ps->pm_flags & PMF_DUCKED )
	{
		bobmove = 0.5; // ducked characters bob much faster

		if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN )
		{
			if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
			{
				PM_ContinueLegsAnim( LEGS_BACKCR );
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
				else
				{
					PM_ContinueLegsAnim( NSPA_WALKBACK );
				}
			}
		}
		else
		{
			if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
			{
				PM_ContinueLegsAnim( LEGS_WALKCR );
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
				else
				{
					PM_ContinueLegsAnim( NSPA_WALK );
				}
			}
		}

		// ducked characters never play footsteps
	}
	else
	{
		if ( !usercmdButtonPressed( pm->cmd.buttons, BUTTON_WALKING ) )
		{
			bobmove = 0.4f; // faster speeds bob faster

			if ( pm->ps->weapon == WP_ALEVEL4 && pm->ps->pm_flags & PMF_CHARGE )
			{
				PM_ContinueLegsAnim( NSPA_CHARGE );
			}
			else if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN )
			{
				if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim( LEGS_BACK );
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
					else
					{
						PM_ContinueLegsAnim( NSPA_RUNBACK );
					}
				}
			}
			else
			{
				if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim( LEGS_RUN );
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
					else
					{
						PM_ContinueLegsAnim( NSPA_RUN );
					}
				}
			}

			footstep = qtrue;
		}
		else
		{
			bobmove = 0.3f; // walking bobs slow

			if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN )
			{
				if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim( LEGS_BACKWALK );
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
					else
					{
						PM_ContinueLegsAnim( NSPA_WALKBACK );
					}
				}
			}
			else
			{
				if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim( LEGS_WALK );
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
					else
					{
						PM_ContinueLegsAnim( NSPA_WALK );
					}
				}
			}
		}
	}

	bobmove *= BG_Class( pm->ps->stats[ STAT_CLASS ] )->bobCycle;

	if ( pm->ps->stats[ STAT_STATE ] & SS_SPEEDBOOST && pm->ps->groundEntityNum != ENTITYNUM_NONE )
	{
		bobmove *= BG_Class( pm->ps->stats[ STAT_CLASS ] )->sprintMod;
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = ( int )( old + bobmove * pml.msec ) & 255;

	// if we just crossed a cycle boundary, play an appropriate footstep event
	if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 )
	{
		if ( pm->waterlevel == 0 )
		{
			// on ground will only play sounds if running
			if ( footstep && !pm->noFootsteps )
			{
				PM_AddEvent( PM_FootstepForSurface() );
			}
		}
		else if ( pm->waterlevel == 1 )
		{
			// splashing
			PM_AddEvent( EV_FOOTSPLASH );
		}
		else if ( pm->waterlevel == 2 )
		{
			// wading / swimming at surface
			PM_AddEvent( EV_SWIM );
		}
		else if ( pm->waterlevel == 3 )
		{
			// no sound when completely underwater
		}
	}
}

/*
==============
PM_WaterEvents

Generate sound events for entering and leaving water
==============
*/
static void PM_WaterEvents( void )
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

	//special case to prevent storing a charged up lcannon
	if ( pm->ps->weapon == WP_LUCIFER_CANNON )
	{
		pm->ps->stats[ STAT_MISC ] = 0;
	}

	// prevent flamer effect from continuing
	pm->ps->generic1 = WPM_NOTFIRING;

	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 200;
	pm->ps->persistant[ PERS_NEWWEAPON ] = weapon;

	//reset build weapon
	pm->ps->stats[ STAT_BUILDABLE ] = BA_NONE;

	if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
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
static void PM_FinishWeaponChange( void )
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

	if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
	{
		PM_StartTorsoAnim( TORSO_RAISE );
		PM_StartWeaponAnim( WANIM_RAISE );
	}
}

/*
==============
PM_TorsoAnimation

==============
*/
static void PM_TorsoAnimation( void )
{
	if ( pm->ps->weaponstate == WEAPON_READY )
	{
		if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			if ( pm->ps->weapon == WP_BLASTER )
			{
				PM_ContinueTorsoAnim( TORSO_STAND_BLASTER );
			}
			else
			{
				PM_ContinueTorsoAnim( TORSO_STAND );
			}
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
static void PM_Weapon( void )
{
	int      addTime = 200; //default addTime - should never be used
	qboolean attack1 = usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK );
	qboolean attack2 = usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK2 );
	qboolean attack3 = usercmdButtonPressed( pm->cmd.buttons, BUTTON_USE_HOLDABLE );

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
		pm->ps->stats[ STAT_MISC ] -= pml.msec;

		if ( pm->ps->stats[ STAT_MISC ] < 0 )
		{
			pm->ps->stats[ STAT_MISC ] = 0;
		}
	}

	// Charging for a pounce or canceling a pounce (Dragoon)
	if ( pm->ps->weapon == WP_ALEVEL3 || pm->ps->weapon == WP_ALEVEL3_UPG )
	{
		int max;

		max = pm->ps->weapon == WP_ALEVEL3 ? LEVEL3_POUNCE_TIME : LEVEL3_POUNCE_TIME_UPG;

		if ( usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK2 ) )
		{
			pm->ps->stats[ STAT_MISC ] += pml.msec;
		}
		else
		{
			pm->ps->stats[ STAT_MISC ] -= pml.msec;
		}

		if ( pm->ps->stats[ STAT_MISC ] > max )
		{
			pm->ps->stats[ STAT_MISC ] = max;
		}
		else if ( pm->ps->stats[ STAT_MISC ] < 0 )
		{
			pm->ps->stats[ STAT_MISC ] = 0;
		}
	}

	// Trample charge mechanics
	if ( pm->ps->weapon == WP_ALEVEL4 )
	{
		// Charging up
		if ( !( pm->ps->stats[ STAT_STATE ] & SS_CHARGING ) )
		{
			// Charge button held
			if ( pm->ps->stats[ STAT_MISC ] < LEVEL4_TRAMPLE_CHARGE_TRIGGER &&
			     usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK2 ) )
			{
				pm->ps->stats[ STAT_STATE ] &= ~SS_CHARGING;

				if ( pm->cmd.forwardmove > 0 )
				{
					int    charge = pml.msec;
					vec3_t dir, vel;

					AngleVectors( pm->ps->viewangles, dir, NULL, NULL );
					VectorCopy( pm->ps->velocity, vel );
					vel[ 2 ] = 0;
					dir[ 2 ] = 0;
					VectorNormalize( vel );
					VectorNormalize( dir );

					charge *= DotProduct( dir, vel );

					pm->ps->stats[ STAT_MISC ] += charge;
				}
				else
				{
					pm->ps->stats[ STAT_MISC ] = 0;
				}
			}

			// Charge button released
			else if ( !( pm->ps->stats[ STAT_STATE ] & SS_CHARGING ) )
			{
				if ( pm->ps->stats[ STAT_MISC ] > LEVEL4_TRAMPLE_CHARGE_MIN )
				{
					if ( pm->ps->stats[ STAT_MISC ] > LEVEL4_TRAMPLE_CHARGE_MAX )
					{
						pm->ps->stats[ STAT_MISC ] = LEVEL4_TRAMPLE_CHARGE_MAX;
					}

					pm->ps->stats[ STAT_MISC ] = pm->ps->stats[ STAT_MISC ] *
					                             LEVEL4_TRAMPLE_DURATION /
					                             LEVEL4_TRAMPLE_CHARGE_MAX;
					pm->ps->stats[ STAT_STATE ] |= SS_CHARGING;
					PM_AddEvent( EV_LEV4_TRAMPLE_START );
				}
				else
				{
					pm->ps->stats[ STAT_MISC ] -= pml.msec;
				}
			}
		}

		// Discharging
		else
		{
			if ( pm->ps->stats[ STAT_MISC ] < LEVEL4_TRAMPLE_CHARGE_MIN )
			{
				pm->ps->stats[ STAT_MISC ] = 0;
			}
			else
			{
				pm->ps->stats[ STAT_MISC ] -= pml.msec;
			}

			// If the charger has stopped moving take a chunk of charge away
			if ( VectorLength( pm->ps->velocity ) < 64.0f || pm->cmd.rightmove )
			{
				pm->ps->stats[ STAT_MISC ] -= LEVEL4_TRAMPLE_STOP_PENALTY * pml.msec;
			}
		}

		// Charge is over
		if ( pm->ps->stats[ STAT_MISC ] <= 0 || pm->cmd.forwardmove <= 0 )
		{
			pm->ps->stats[ STAT_MISC ] = 0;
			pm->ps->stats[ STAT_STATE ] &= ~SS_CHARGING;
		}
	}

	// Charging up a Lucifer Cannon
	pm->ps->eFlags &= ~EF_WARN_CHARGE;

	if ( pm->ps->weapon == WP_LUCIFER_CANNON )
	{
		// Charging up
		if ( !pm->ps->weaponTime && pm->ps->weaponstate != WEAPON_NEEDS_RESET &&
		     usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK ) )
		{
			pm->ps->stats[ STAT_MISC ] += pml.msec;

			if ( pm->ps->stats[ STAT_MISC ] >= LCANNON_CHARGE_TIME_MAX )
			{
				pm->ps->stats[ STAT_MISC ] = LCANNON_CHARGE_TIME_MAX;
			}

			if ( pm->ps->stats[ STAT_MISC ] > pm->ps->ammo * LCANNON_CHARGE_TIME_MAX /
			     LCANNON_CHARGE_AMMO )
			{
				pm->ps->stats[ STAT_MISC ] = pm->ps->ammo * LCANNON_CHARGE_TIME_MAX /
				                             LCANNON_CHARGE_AMMO;
			}
		}

		// Set overcharging flag so other players can hear the warning beep
		if ( pm->ps->stats[ STAT_MISC ] > LCANNON_CHARGE_TIME_WARN )
		{
			pm->ps->eFlags |= EF_WARN_CHARGE;
		}
	}

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED )
	{
		return;
	}

	// no bite during pounce
	if ( ( pm->ps->weapon == WP_ALEVEL3 || pm->ps->weapon == WP_ALEVEL3_UPG )
	     && usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK )
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
		// must press use to switch weapons
		if ( usercmdButtonPressed( pm->cmd.buttons, BUTTON_USE_HOLDABLE ) )
		{
			if ( !( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) )
			{
				if ( pm->cmd.weapon < 32 )
				{
					//if trying to select a weapon, select it
					if ( pm->ps->weapon != pm->cmd.weapon )
					{
						PM_BeginWeaponChange( pm->cmd.weapon );
					}
				}
				else
				{
					//if trying to toggle an upgrade, toggle it
					if ( BG_InventoryContainsUpgrade( pm->cmd.weapon - 32, pm->ps->stats ) )  //sanity check
					{
						if ( BG_UpgradeIsActive( pm->cmd.weapon - 32, pm->ps->stats ) )
						{
							BG_DeactivateUpgrade( pm->cmd.weapon - 32, pm->ps->stats );
						}
						else
						{
							BG_ActivateUpgrade( pm->cmd.weapon - 32, pm->ps->stats );
						}
					}
				}

				pm->ps->pm_flags |= PMF_USE_ITEM_HELD;
			}
		}
		else
		{
			pm->ps->pm_flags &= ~PMF_USE_ITEM_HELD;
		}

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

		if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			if ( pm->ps->weapon == WP_BLASTER )
			{
				PM_ContinueTorsoAnim( TORSO_STAND_BLASTER );
			}
			else
			{
				PM_ContinueTorsoAnim( TORSO_STAND );
			}
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
			attack3 = qfalse;

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
			if ( attack1 || pm->ps->stats[ STAT_MISC ] > 0 )
			{
				attack2 = qfalse;
			}

			if ( ( attack1 || pm->ps->stats[ STAT_MISC ] == 0 ) && !attack2 )
			{
				pm->ps->weaponTime = 0;

				// Charging
				if ( pm->ps->stats[ STAT_MISC ] < LCANNON_CHARGE_TIME_MAX )
				{
					pm->ps->weaponstate = WEAPON_READY;
					return;
				}

				// Overcharge
				pm->ps->weaponstate = WEAPON_NEEDS_RESET;
			}

			if ( pm->ps->stats[ STAT_MISC ] > LCANNON_CHARGE_TIME_MIN )
			{
				// Fire primary attack
				attack1 = qtrue;
				attack2 = qfalse;
			}
			else if ( pm->ps->stats[ STAT_MISC ] > 0 )
			{
				// Not enough charge
				pm->ps->stats[ STAT_MISC ] = 0;
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
			attack2 = attack3 = qfalse;
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

	if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
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
			pm->ps->ammo -= ( pm->ps->stats[ STAT_MISC ] * LCANNON_CHARGE_AMMO +
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
static void PM_Animate( void )
{
	if ( PM_Paralyzed( pm->ps->pm_type ) )
	{
		return;
	}

	if ( usercmdButtonPressed( pm->cmd.buttons, BUTTON_GESTURE ) )
	{
		if ( pm->ps->tauntTimer > 0 )
		{
			return;
		}

		if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			if ( pm->ps->torsoTimer == 0 )
			{
				PM_StartTorsoAnim( TORSO_GESTURE_BLASTER + ( pm->ps->weapon - WP_BLASTER ) > WP_LUCIFER_CANNON ?
				    TORSO_GESTURE_CKIT :
				    TORSO_GESTURE_BLASTER + ( pm->ps->weapon - WP_BLASTER ) );
				pm->ps->torsoTimer = TIMER_GESTURE;
				pm->ps->tauntTimer = TIMER_GESTURE;

				PM_AddEvent( EV_TAUNT );
			}
		}
		else
		{
			if ( pm->ps->torsoTimer == 0 )
			{
				PM_ForceLegsAnim( NSPA_GESTURE );
				pm->ps->torsoTimer = TIMER_GESTURE;
				pm->ps->tauntTimer = TIMER_GESTURE;

				PM_AddEvent( EV_TAUNT );
			}
		}
	}

	if ( usercmdButtonPressed( pm->cmd.buttons, BUTTON_RALLY ) )
	{
		if ( pm->ps->tauntTimer > 0 )
		{
			return;
		}

		if ( !( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			if ( pm->ps->torsoTimer == 0 )
			{
				PM_StartTorsoAnim( TORSO_RALLY );
				pm->ps->torsoTimer = TIMER_GESTURE;
				pm->ps->tauntTimer = TIMER_GESTURE;

				PM_AddEvent( EV_TAUNT );
			}
		}
		else
		{
			if ( pm->ps->torsoTimer == 0 )
			{
				PM_ForceLegsAnim( NSPA_GESTURE );
				pm->ps->torsoTimer = TIMER_GESTURE;
				pm->ps->tauntTimer = TIMER_GESTURE;

				PM_AddEvent( EV_TAUNT );
			}
		}
	}

}

/*
================
PM_DropTimers
================
*/
static void PM_DropTimers( void )
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
	if ( pm->ps->legsTimer > 0 )
	{
		pm->ps->legsTimer -= pml.msec;

		if ( pm->ps->legsTimer < 0 )
		{
			pm->ps->legsTimer = 0;
		}
	}

	if ( pm->ps->torsoTimer > 0 )
	{
		pm->ps->torsoTimer -= pml.msec;

		if ( pm->ps->torsoTimer < 0 )
		{
			pm->ps->torsoTimer = 0;
		}
	}

	if ( pm->ps->tauntTimer > 0 )
	{
		pm->ps->tauntTimer -= pml.msec;

		if ( pm->ps->tauntTimer < 0 )
		{
			pm->ps->tauntTimer = 0;
		}
	}
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

	if ( ps->pm_type != PM_SPECTATOR && ps->stats[ STAT_HEALTH ] <= 0 )
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
			// Assert here so that if cmd->angles[ i ] becomes non-zero
			// for a legitimate reason we can tell where and why it's
			// being ignored
			assert( cmd->angles[ i ] == 0 );
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
	     !BG_RotateAxis( ps->grapplePoint, axis, rotaxis, qfalse,
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
	for ( i = 0; i < 3; i++ )
	{
		ps->viewangles[ i ] = tempang[ i ];
	}

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
				ps->delta_angles[ i ] += ANGLE2SHORT( fabs( diff ) - 90.0f );
			}
			else if ( diff > 90.0f )
			{
				ps->delta_angles[ i ] -= ANGLE2SHORT( fabs( diff ) - 90.0f );
			}

			if ( diff < 0.0f )
			{
				ps->delta_angles[ i ] += ANGLE2SHORT( fabs( diff ) * 0.05f );
			}
			else if ( diff > 0.0f )
			{
				ps->delta_angles[ i ] -= ANGLE2SHORT( fabs( diff ) * 0.05f );
			}
		}
	}
}

static void PM_HumanStaminaEffects( void )
{
	const classAttributes_t *ca;
	int      *stats;
	qboolean crouching, stopped, walking;

	if ( pm->ps->persistant[ PERS_TEAM ] != TEAM_HUMANS )
	{
		return;
	}

	stats     = pm->ps->stats;
	ca        = BG_Class( stats[ STAT_CLASS ] );
	stopped   = ( pm->cmd.forwardmove == 0 && pm->cmd.rightmove == 0 );
	crouching = ( pm->ps->pm_flags & PMF_DUCKED );
	walking   = usercmdButtonPressed( pm->cmd.buttons, BUTTON_WALKING );

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
	if ( stats[ STAT_STAMINA ] > STAMINA_MAX )
	{
		stats[ STAT_STAMINA ] = STAMINA_MAX;
	}
	else if ( stats[ STAT_STAMINA ] < 0 )
	{
		stats[ STAT_STAMINA ] = 0;
	}
}

/*
================
PmoveSingle

================
*/
void PmoveSingle( pmove_t *pmove )
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
		usercmdReleaseButton( pm->cmd.buttons, BUTTON_WALKING );
	}

	// set the firing flag for continuous beam weapons
	if ( !( pm->ps->pm_flags & PMF_RESPAWNED ) && pm->ps->pm_type != PM_INTERMISSION &&
	     usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK ) &&
	     ( ( pm->ps->ammo > 0 || pm->ps->clips > 0 ) || BG_Weapon( pm->ps->weapon )->infiniteAmmo ) )
	{
		pm->ps->eFlags |= EF_FIRING;
	}
	else
	{
		pm->ps->eFlags &= ~EF_FIRING;
	}

	// set the firing flag for continuous beam weapons
	if ( !( pm->ps->pm_flags & PMF_RESPAWNED ) && pm->ps->pm_type != PM_INTERMISSION &&
	     usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK2 ) &&
	     ( ( pm->ps->ammo > 0 || pm->ps->clips > 0 ) || BG_Weapon( pm->ps->weapon )->infiniteAmmo ) )
	{
		pm->ps->eFlags |= EF_FIRING2;
	}
	else
	{
		pm->ps->eFlags &= ~EF_FIRING2;
	}

	// set the firing flag for continuous beam weapons
	if ( !( pm->ps->pm_flags & PMF_RESPAWNED ) && pm->ps->pm_type != PM_INTERMISSION &&
	     usercmdButtonPressed( pm->cmd.buttons, BUTTON_USE_HOLDABLE ) &&
	     ( ( pm->ps->ammo > 0 || pm->ps->clips > 0 ) || BG_Weapon( pm->ps->weapon )->infiniteAmmo ) )
	{
		pm->ps->eFlags |= EF_FIRING3;
	}
	else
	{
		pm->ps->eFlags &= ~EF_FIRING3;
	}

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[ STAT_HEALTH ] > 0 &&
	     !( usercmdButtonPressed( pm->cmd.buttons, BUTTON_ATTACK ) || usercmdButtonPressed( pm->cmd.buttons, BUTTON_USE_HOLDABLE ) ) )
	{
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( usercmdButtonPressed( pmove->cmd.buttons, BUTTON_TALK ) )
	{
		usercmdClearButtons( pmove->cmd.buttons );
		usercmdPressButton( pmove->cmd.buttons, BUTTON_TALK );
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

	if ( pml.msec < 1 )
	{
		pml.msec = 1;
	}
	else if ( pml.msec > 200 )
	{
		pml.msec = 200;
	}

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

	if ( pm->ps->pm_type == PM_SPECTATOR )
	{
		// update the viewangles
		PM_UpdateViewAngles( pm->ps, &pm->cmd );
		PM_CheckDuck();
		PM_FlyMove();
		PM_DropTimers();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP )
	{
		PM_UpdateViewAngles( pm->ps, &pm->cmd );
		PM_NoclipMove();
		PM_SetViewheight();
		PM_Weapon();
		PM_DropTimers();
		return;
	}

	if ( pm->ps->pm_type == PM_FREEZE )
	{
		return; // no movement at all
	}

	if ( pm->ps->pm_type == PM_INTERMISSION )
	{
		return; // no movement at all
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
		if ( BG_ClassHasAbility( pm->ps->stats[ STAT_CLASS ], SCA_WALLCLIMBER ) &&
		     ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
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
