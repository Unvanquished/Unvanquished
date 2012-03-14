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

// bg_pmove.c -- both games player movement code
// takes a playerstate and a usercmd as input and returns a modifed playerstate

#include "../../../../engine/qcommon/q_shared.h"
#include "bg_public.h"
#include "bg_local.h"

pmove_t *pm;
pml_t   pml;

// movement parameters
float   pm_stopspeed = 100.0f;
float   pm_duckScale = 0.25f;
float   pm_swimScale = 0.50f;
float   pm_wadeScale = 0.70f;

float   pm_accelerate = 10.0f;
float   pm_airaccelerate = 1.0f;
float   pm_wateraccelerate = 4.0f;
float   pm_flyaccelerate = 4.0f;

float   pm_friction = 6.0f;
float   pm_waterfriction = 1.0f;
float   pm_flightfriction = 6.0f;
float   pm_spectatorfriction = 5.0f;

int     c_pmove = 0;

/*
===============
PM_AddEvent

===============
*/
void PM_AddEvent ( int newEvent )
{
	BG_AddPredictableEventToPlayerstate ( newEvent, 0, pm->ps );
}

/*
===============
PM_AddTouchEnt
===============
*/
void PM_AddTouchEnt ( int entityNum )
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
static void PM_StartTorsoAnim ( int anim )
{
	if ( pm->ps->pm_type >= PM_DEAD )
	{
		return;
	}

	pm->ps->torsoAnim = ( ( pm->ps->torsoAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT )
	                    | anim;
}

/*
===================
PM_StartLegsAnim
===================
*/
static void PM_StartLegsAnim ( int anim )
{
	if ( pm->ps->pm_type >= PM_DEAD )
	{
		return;
	}

	//legsTimer is clamped too tightly for nonsegmented models
	if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
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
static void PM_ContinueLegsAnim ( int anim )
{
	if ( ( pm->ps->legsAnim & ~ANIM_TOGGLEBIT ) == anim )
	{
		return;
	}

	//legsTimer is clamped too tightly for nonsegmented models
	if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
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

	PM_StartLegsAnim ( anim );
}

/*
===================
PM_ContinueTorsoAnim
===================
*/
static void PM_ContinueTorsoAnim ( int anim )
{
	if ( ( pm->ps->torsoAnim & ~ANIM_TOGGLEBIT ) == anim )
	{
		return;
	}

	if ( pm->ps->torsoTimer > 0 )
	{
		return; // a high priority animation is running
	}

	PM_StartTorsoAnim ( anim );
}

/*
===================
PM_ForceLegsAnim
===================
*/
static void PM_ForceLegsAnim ( int anim )
{
	//legsTimer is clamped too tightly for nonsegmented models
	if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
	{
		pm->ps->legsTimer = 0;
	}
	else
	{
		pm->ps->torsoTimer = 0;
	}

	PM_StartLegsAnim ( anim );
}

/*
==================
PM_ClipVelocity

Slide off of the impacting surface
==================
*/
void PM_ClipVelocity ( vec3_t in, vec3_t normal, vec3_t out, float overbounce )
{
	float backoff;
	float change;
	int   i;

	backoff = DotProduct ( in, normal );

	//Com_Printf( "%1.0f ", backoff );

	if ( backoff < 0 )
	{
		backoff *= overbounce;
	}
	else
	{
		backoff /= overbounce;
	}

	for ( i = 0; i < 3; i++ )
	{
		change = normal[ i ] * backoff;
		//Com_Printf( "%1.0f ", change );
		out[ i ] = in[ i ] - change;
	}

	//Com_Printf( "   " );
}

/*
==================
PM_Friction

Handles both ground friction and water friction
==================
*/
static void PM_Friction ( void )
{
	vec3_t vec;
	float  *vel;
	float  speed, newspeed, control;
	float  drop;

	vel = pm->ps->velocity;

	//TA: make sure vertical velocity is NOT set to zero when wall climbing
	VectorCopy ( vel, vec );

	if ( pml.walking && ! ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
	{
		vec[ 2 ] = 0; // ignore slope movement
	}

	speed = VectorLength ( vec );

	if ( speed < 1 )
	{
		vel[ 0 ] = 0;
		vel[ 1 ] = 0; // allow sinking underwater
		// FIXME: still have z friction underwater?
		return;
	}

	drop = 0;

	// apply ground friction
	if ( pm->waterlevel <= 1 )
	{
		if ( ( pml.walking || pml.ladder ) && ! ( pml.groundTrace.surfaceFlags & SURF_SLICK ) )
		{
			// if getting knocked back, no friction
			if ( ! ( pm->ps->pm_flags & PMF_TIME_KNOCKBACK ) )
			{
				float stopSpeed = BG_FindStopSpeedForClass ( pm->ps->stats[ STAT_PCLASS ] );

				control = speed < stopSpeed ? stopSpeed : speed;
				drop += control * BG_FindFrictionForClass ( pm->ps->stats[ STAT_PCLASS ] ) * pml.frametime;
			}
		}
	}

	// apply water friction even if just wading
	if ( pm->waterlevel )
	{
		drop += speed * pm_waterfriction * pm->waterlevel * pml.frametime;
	}

	// apply flying friction
	if ( pm->ps->pm_type == PM_JETPACK )
	{
		drop += speed * pm_flightfriction * pml.frametime;
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
static void PM_Accelerate ( vec3_t wishdir, float wishspeed, float accel )
{
#if 1
	// q2 style
	int   i;
	float addspeed, accelspeed, currentspeed;

	currentspeed = DotProduct ( pm->ps->velocity, wishdir );
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

	VectorScale ( wishdir, wishspeed, wishVelocity );
	VectorSubtract ( wishVelocity, pm->ps->velocity, pushDir );
	pushLen = VectorNormalize ( pushDir );

	canPush = accel * pml.frametime * wishspeed;

	if ( canPush > pushLen )
	{
		canPush = pushLen;
	}

	VectorMA ( pm->ps->velocity, canPush, pushDir, pm->ps->velocity );
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
static float PM_CmdScale ( usercmd_t *cmd )
{
	int   max;
	float total;
	float scale;
	float modifier = 1.0f;

	if ( pm->ps->stats[ STAT_PTEAM ] == PTE_HUMANS && pm->ps->pm_type == PM_NORMAL )
	{
		if ( pm->ps->stats[ STAT_STATE ] & SS_SPEEDBOOST )
		{
			modifier *= HUMAN_SPRINT_MODIFIER;
		}
		else
		{
			modifier *= HUMAN_JOG_MODIFIER;
		}

		if ( cmd->forwardmove < 0 )
		{
			//can't run backwards
			modifier *= HUMAN_BACK_MODIFIER;
		}
		else if ( cmd->rightmove )
		{
			//can't move that fast sideways
			modifier *= HUMAN_SIDE_MODIFIER;
		}

		//must have +ve stamina to jump
		if ( pm->ps->stats[ STAT_STAMINA ] < 0 )
		{
			cmd->upmove = 0;
		}

		//slow down once stamina depletes
		if ( pm->ps->stats[ STAT_STAMINA ] <= -500 )
		{
			modifier *= ( float ) ( pm->ps->stats[ STAT_STAMINA ] + 1000 ) / 500.0f;
		}

		if ( pm->ps->stats[ STAT_STATE ] & SS_CREEPSLOWED )
		{
			if ( BG_InventoryContainsUpgrade ( UP_LIGHTARMOUR, pm->ps->stats ) ||
			     BG_InventoryContainsUpgrade ( UP_BATTLESUIT, pm->ps->stats ) )
			{
				modifier *= CREEP_ARMOUR_MODIFIER;
			}
			else
			{
				modifier *= CREEP_MODIFIER;
			}
		}
	}

	if ( pm->ps->weapon == WP_ALEVEL4 && pm->ps->pm_flags & PMF_CHARGE )
	{
		modifier *= ( 1.0f + ( pm->ps->stats[ STAT_MISC ] / ( float ) LEVEL4_CHARGE_TIME ) *
		              ( LEVEL4_CHARGE_SPEED - 1.0f ) );
	}

	//slow player if charging up for a pounce
	if ( ( pm->ps->weapon == WP_ALEVEL3 || pm->ps->weapon == WP_ALEVEL3_UPG ) &&
	     cmd->buttons & BUTTON_ATTACK2 )
	{
		modifier *= LEVEL3_POUNCE_SPEED_MOD;
	}

	//slow the player if slow locked
	if ( pm->ps->stats[ STAT_STATE ] & SS_SLOWLOCKED )
	{
		modifier *= ABUILDER_BLOB_SPEED_MOD;
	}

	if ( pm->ps->pm_type == PM_GRABBED )
	{
		modifier = 0.0f;
	}

	if ( pm->ps->pm_type != PM_SPECTATOR && pm->ps->pm_type != PM_NOCLIP )
	{
		if ( BG_FindJumpMagnitudeForClass ( pm->ps->stats[ STAT_PCLASS ] ) == 0.0f )
		{
			cmd->upmove = 0;
		}

		//prevent speed distortions for non ducking classes
		if ( ! ( pm->ps->pm_flags & PMF_DUCKED ) && pm->ps->pm_type != PM_JETPACK && cmd->upmove < 0 )
		{
			cmd->upmove = 0;
		}
	}

	max = abs ( cmd->forwardmove );

	if ( abs ( cmd->rightmove ) > max )
	{
		max = abs ( cmd->rightmove );
	}

	if ( abs ( cmd->upmove ) > max )
	{
		max = abs ( cmd->upmove );
	}

	if ( !max )
	{
		return 0;
	}

	total = sqrt ( cmd->forwardmove * cmd->forwardmove
	               + cmd->rightmove * cmd->rightmove + cmd->upmove * cmd->upmove );

	scale = ( float ) pm->ps->speed * max / ( 127.0 * total ) * modifier;

	return scale;
}

/*
================
PM_SetMovementDir

Determine the rotation of the legs reletive
to the facing dir
================
*/
static void PM_SetMovementDir ( void )
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
static void PM_CheckCharge ( void )
{
	if ( pm->ps->weapon != WP_ALEVEL4 )
	{
		return;
	}

	if ( pm->cmd.buttons & BUTTON_ATTACK2 &&
	     ! ( pm->ps->stats[ STAT_STATE ] & SS_CHARGING ) )
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
PM_CheckPounce
=============
*/
static qboolean PM_CheckPounce ( void )
{
	if ( pm->ps->weapon != WP_ALEVEL3 &&
	     pm->ps->weapon != WP_ALEVEL3_UPG )
	{
		return qfalse;
	}

	if ( pm->cmd.buttons & BUTTON_ATTACK2 )
	{
		pm->ps->pm_flags &= ~PMF_CHARGE;
		return qfalse;
	}

	if ( pm->ps->pm_flags & PMF_CHARGE )
	{
		return qfalse;
	}

	if ( pm->ps->stats[ STAT_MISC ] == 0 )
	{
		return qfalse;
	}

	pml.groundPlane = qfalse; // jumping away
	pml.walking = qfalse;

	pm->ps->pm_flags |= PMF_CHARGE;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	VectorMA ( pm->ps->velocity, pm->ps->stats[ STAT_MISC ], pml.forward, pm->ps->velocity );

	PM_AddEvent ( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0 )
	{
		if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim ( LEGS_JUMP );
		}
		else
		{
			PM_ForceLegsAnim ( NSPA_JUMP );
		}

		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else
	{
		if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim ( LEGS_JUMPB );
		}
		else
		{
			PM_ForceLegsAnim ( NSPA_JUMPBACK );
		}

		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

/*
=============
PM_CheckWallJump
=============
*/
static qboolean PM_CheckWallJump ( void )
{
	vec3_t dir, forward, right;
	vec3_t refNormal = { 0.0f, 0.0f, 1.0f };
	float  normalFraction = 1.5f;
	float  cmdFraction = 1.0f;
	float  upFraction = 1.5f;

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
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pm->ps->pm_flags |= PMF_TIME_WALLJUMP;
	pm->ps->pm_time = 200;

	pml.groundPlane = qfalse; // jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	ProjectPointOnPlane ( forward, pml.forward, pm->ps->grapplePoint );
	ProjectPointOnPlane ( right, pml.right, pm->ps->grapplePoint );

	VectorScale ( pm->ps->grapplePoint, normalFraction, dir );

	if ( pm->cmd.forwardmove > 0 )
	{
		VectorMA ( dir, cmdFraction, forward, dir );
	}
	else if ( pm->cmd.forwardmove < 0 )
	{
		VectorMA ( dir, -cmdFraction, forward, dir );
	}

	if ( pm->cmd.rightmove > 0 )
	{
		VectorMA ( dir, cmdFraction, right, dir );
	}
	else if ( pm->cmd.rightmove < 0 )
	{
		VectorMA ( dir, -cmdFraction, right, dir );
	}

	VectorMA ( dir, upFraction, refNormal, dir );
	VectorNormalize ( dir );

	VectorMA ( pm->ps->velocity, BG_FindJumpMagnitudeForClass ( pm->ps->stats[ STAT_PCLASS ] ),
	           dir, pm->ps->velocity );

	//for a long run of wall jumps the velocity can get pretty large, this caps it
	if ( VectorLength ( pm->ps->velocity ) > LEVEL2_WALLJUMP_MAXSPEED )
	{
		VectorNormalize ( pm->ps->velocity );
		VectorScale ( pm->ps->velocity, LEVEL2_WALLJUMP_MAXSPEED, pm->ps->velocity );
	}

	PM_AddEvent ( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0 )
	{
		if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim ( LEGS_JUMP );
		}
		else
		{
			PM_ForceLegsAnim ( NSPA_JUMP );
		}

		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else
	{
		if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim ( LEGS_JUMPB );
		}
		else
		{
			PM_ForceLegsAnim ( NSPA_JUMPBACK );
		}

		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

/*
=============
PM_CheckJump
=============
*/
static qboolean PM_CheckJump ( void )
{
	if ( BG_FindJumpMagnitudeForClass ( pm->ps->stats[ STAT_PCLASS ] ) == 0.0f )
	{
		return qfalse;
	}

	if ( BG_ClassHasAbility ( pm->ps->stats[ STAT_PCLASS ], SCA_WALLJUMPER ) )
	{
		return PM_CheckWallJump();
	}

	//can't jump and pounce at the same time
	if ( ( pm->ps->weapon == WP_ALEVEL3 ||
	       pm->ps->weapon == WP_ALEVEL3_UPG ) &&
	     pm->ps->stats[ STAT_MISC ] > 0 )
	{
		return qfalse;
	}

	//can't jump and charge at the same time
	if ( ( pm->ps->weapon == WP_ALEVEL4 ) &&
	     pm->ps->stats[ STAT_MISC ] > 0 )
	{
		return qfalse;
	}

	if ( ( pm->ps->stats[ STAT_PTEAM ] == PTE_HUMANS ) &&
	     ( pm->ps->stats[ STAT_STAMINA ] < 0 ) )
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

	//can't jump whilst grabbed
	if ( pm->ps->pm_type == PM_GRABBED )
	{
		pm->cmd.upmove = 0;
		return qfalse;
	}

	// must wait for jump to be released
	if ( pm->ps->pm_flags & PMF_JUMP_HELD )
	{
		// clear upmove so cmdscale doesn't lower running speed
		pm->cmd.upmove = 0;
		return qfalse;
	}

	pml.groundPlane = qfalse; // jumping away
	pml.walking = qfalse;
	pm->ps->pm_flags |= PMF_JUMP_HELD;

	//TA: take some stamina off
	if ( pm->ps->stats[ STAT_PTEAM ] == PTE_HUMANS )
	{
		pm->ps->stats[ STAT_STAMINA ] -= 500;
	}

	pm->ps->groundEntityNum = ENTITYNUM_NONE;

	//TA: jump away from wall
	if ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING )
	{
		vec3_t normal = { 0, 0, -1 };

		if ( ! ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING ) )
		{
			VectorCopy ( pm->ps->grapplePoint, normal );
		}

		VectorMA ( pm->ps->velocity, BG_FindJumpMagnitudeForClass ( pm->ps->stats[ STAT_PCLASS ] ),
		           normal, pm->ps->velocity );
	}
	else
	{
		pm->ps->velocity[ 2 ] = BG_FindJumpMagnitudeForClass ( pm->ps->stats[ STAT_PCLASS ] );
	}

	PM_AddEvent ( EV_JUMP );

	if ( pm->cmd.forwardmove >= 0 )
	{
		if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim ( LEGS_JUMP );
		}
		else
		{
			PM_ForceLegsAnim ( NSPA_JUMP );
		}

		pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
	}
	else
	{
		if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim ( LEGS_JUMPB );
		}
		else
		{
			PM_ForceLegsAnim ( NSPA_JUMPBACK );
		}

		pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
	}

	return qtrue;
}

/*
=============
PM_CheckWaterJump
=============
*/
static qboolean PM_CheckWaterJump ( void )
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
	VectorNormalize ( flatforward );

	VectorMA ( pm->ps->origin, 30, flatforward, spot );
	spot[ 2 ] += 4;
	cont = pm->pointcontents ( spot, pm->ps->clientNum );

	if ( ! ( cont & CONTENTS_SOLID ) )
	{
		return qfalse;
	}

	spot[ 2 ] += 16;
	cont = pm->pointcontents ( spot, pm->ps->clientNum );

	if ( cont )
	{
		return qfalse;
	}

	// jump out of water
	VectorScale ( pml.forward, 200, pm->ps->velocity );
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
static void PM_WaterJumpMove ( void )
{
	// waterjump has no control, but falls

	PM_StepSlideMove ( qtrue, qfalse );

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
static void PM_WaterMove ( void )
{
	int    i;
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;
	float  vel;

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

	scale = PM_CmdScale ( &pm->cmd );

	//
	// user intentions
	//
	if ( !scale )
	{
		wishvel[ 0 ] = 0;
		wishvel[ 1 ] = 0;
		wishvel[ 2 ] = -60; // sink towards bottom
	}
	else
	{
		for ( i = 0; i < 3; i++ )
		{
			wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;
		}

		wishvel[ 2 ] += scale * pm->cmd.upmove;
	}

	VectorCopy ( wishvel, wishdir );
	wishspeed = VectorNormalize ( wishdir );

	if ( wishspeed > pm->ps->speed * pm_swimScale )
	{
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate ( wishdir, wishspeed, pm_wateraccelerate );

	// make sure we can go up slopes easily under water
	if ( pml.groundPlane && DotProduct ( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0 )
	{
		vel = VectorLength ( pm->ps->velocity );
		// slide along the ground plane
		PM_ClipVelocity ( pm->ps->velocity, pml.groundTrace.plane.normal,
		                  pm->ps->velocity, OVERCLIP );

		VectorNormalize ( pm->ps->velocity );
		VectorScale ( pm->ps->velocity, vel, pm->ps->velocity );
	}

	PM_SlideMove ( qfalse );
}

/*
===================
PM_JetPackMove

Only with the jetpack
===================
*/
static void PM_JetPackMove ( void )
{
	int    i;
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;

	//normal slowdown
	PM_Friction();

	scale = PM_CmdScale ( &pm->cmd );

	// user intentions
	for ( i = 0; i < 2; i++ )
	{
		wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;
	}

	if ( pm->cmd.upmove > 0.0f )
	{
		wishvel[ 2 ] = JETPACK_FLOAT_SPEED;
	}

	if ( pm->cmd.upmove < 0.0f )
	{
		wishvel[ 2 ] = -JETPACK_SINK_SPEED;
	}

	VectorCopy ( wishvel, wishdir );
	wishspeed = VectorNormalize ( wishdir );

	PM_Accelerate ( wishdir, wishspeed, pm_flyaccelerate );

	PM_StepSlideMove ( qfalse, qfalse );

	if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
	{
		PM_ContinueLegsAnim ( LEGS_LAND );
	}
	else
	{
		PM_ContinueLegsAnim ( NSPA_LAND );
	}
}

/*
===================
PM_FlyMove

Only with the flight powerup
===================
*/
static void PM_FlyMove ( void )
{
	int    i;
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;

	// normal slowdown
	PM_Friction();

	scale = PM_CmdScale ( &pm->cmd );

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

	VectorCopy ( wishvel, wishdir );
	wishspeed = VectorNormalize ( wishdir );

	PM_Accelerate ( wishdir, wishspeed, pm_flyaccelerate );

	PM_StepSlideMove ( qfalse, qfalse );
}

/*
===================
PM_AirMove

===================
*/
static void PM_AirMove ( void )
{
	int       i;
	vec3_t    wishvel;
	float     fmove, smove;
	vec3_t    wishdir;
	float     wishspeed;
	float     scale;
	usercmd_t cmd;

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale ( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[ 2 ] = 0;
	pml.right[ 2 ] = 0;
	VectorNormalize ( pml.forward );
	VectorNormalize ( pml.right );

	for ( i = 0; i < 2; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;
	}

	wishvel[ 2 ] = 0;

	VectorCopy ( wishvel, wishdir );
	wishspeed = VectorNormalize ( wishdir );
	wishspeed *= scale;

	// not on ground, so little effect on velocity
	PM_Accelerate ( wishdir, wishspeed,
	                BG_FindAirAccelerationForClass ( pm->ps->stats[ STAT_PCLASS ] ) );

	// we may have a ground plane that is very steep, even
	// though we don't have a groundentity
	// slide along the steep plane
	if ( pml.groundPlane )
	{
		PM_ClipVelocity ( pm->ps->velocity, pml.groundTrace.plane.normal,
		                  pm->ps->velocity, OVERCLIP );
	}

	PM_StepSlideMove ( qtrue, qfalse );
}

/*
===================
PM_ClimbMove

===================
*/
static void PM_ClimbMove ( void )
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

	if ( pm->waterlevel > 2 && DotProduct ( pml.forward, pml.groundTrace.plane.normal ) > 0 )
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
	scale = PM_CmdScale ( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity ( pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity ( pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	//
	VectorNormalize ( pml.forward );
	VectorNormalize ( pml.right );

	for ( i = 0; i < 3; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;
	}

	// when going up or down slopes the wish velocity should Not be zero
//  wishvel[2] = 0;

	VectorCopy ( wishvel, wishdir );
	wishspeed = VectorNormalize ( wishdir );
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
		accelerate = BG_FindAirAccelerationForClass ( pm->ps->stats[ STAT_PCLASS ] );
	}
	else
	{
		accelerate = BG_FindAccelerationForClass ( pm->ps->stats[ STAT_PCLASS ] );
	}

	PM_Accelerate ( wishdir, wishspeed, accelerate );

	if ( ( pml.groundTrace.surfaceFlags & SURF_SLICK ) || pm->ps->pm_flags & PMF_TIME_KNOCKBACK )
	{
		pm->ps->velocity[ 2 ] -= pm->ps->gravity * pml.frametime;
	}

	vel = VectorLength ( pm->ps->velocity );

	// slide along the ground plane
	PM_ClipVelocity ( pm->ps->velocity, pml.groundTrace.plane.normal,
	                  pm->ps->velocity, OVERCLIP );

	// don't decrease velocity when going up or down a slope
	VectorNormalize ( pm->ps->velocity );
	VectorScale ( pm->ps->velocity, vel, pm->ps->velocity );

	// don't do anything if standing still
	if ( !pm->ps->velocity[ 0 ] && !pm->ps->velocity[ 1 ] && !pm->ps->velocity[ 2 ] )
	{
		return;
	}

	PM_StepSlideMove ( qfalse, qfalse );
}

/*
===================
PM_WalkMove

===================
*/
static void PM_WalkMove ( void )
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

	if ( pm->waterlevel > 2 && DotProduct ( pml.forward, pml.groundTrace.plane.normal ) > 0 )
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

	//charging
	PM_CheckCharge();

	PM_Friction();

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	cmd = pm->cmd;
	scale = PM_CmdScale ( &cmd );

	// set the movementDir so clients can rotate the legs for strafing
	PM_SetMovementDir();

	// project moves down to flat plane
	pml.forward[ 2 ] = 0;
	pml.right[ 2 ] = 0;

	// project the forward and right directions onto the ground plane
	PM_ClipVelocity ( pml.forward, pml.groundTrace.plane.normal, pml.forward, OVERCLIP );
	PM_ClipVelocity ( pml.right, pml.groundTrace.plane.normal, pml.right, OVERCLIP );
	//
	VectorNormalize ( pml.forward );
	VectorNormalize ( pml.right );

	for ( i = 0; i < 3; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;
	}

	// when going up or down slopes the wish velocity should Not be zero
//  wishvel[2] = 0;

	VectorCopy ( wishvel, wishdir );
	wishspeed = VectorNormalize ( wishdir );
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
		accelerate = BG_FindAirAccelerationForClass ( pm->ps->stats[ STAT_PCLASS ] );
	}
	else
	{
		accelerate = BG_FindAccelerationForClass ( pm->ps->stats[ STAT_PCLASS ] );
	}

	PM_Accelerate ( wishdir, wishspeed, accelerate );

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

	vel = VectorLength ( pm->ps->velocity );

	// slide along the ground plane
	PM_ClipVelocity ( pm->ps->velocity, pml.groundTrace.plane.normal,
	                  pm->ps->velocity, OVERCLIP );

	// don't decrease velocity when going up or down a slope
	VectorNormalize ( pm->ps->velocity );
	VectorScale ( pm->ps->velocity, vel, pm->ps->velocity );

	// don't do anything if standing still
	if ( !pm->ps->velocity[ 0 ] && !pm->ps->velocity[ 1 ] )
	{
		return;
	}

	PM_StepSlideMove ( qfalse, qfalse );

	//Com_Printf("velocity2 = %1.1f\n", VectorLength(pm->ps->velocity));
}

/*
===================
PM_LadderMove

Basically a rip of PM_WaterMove with a few changes
===================
*/
static void PM_LadderMove ( void )
{
	int    i;
	vec3_t wishvel;
	float  wishspeed;
	vec3_t wishdir;
	float  scale;
	float  vel;

	PM_Friction();

	scale = PM_CmdScale ( &pm->cmd );

	for ( i = 0; i < 3; i++ )
	{
		wishvel[ i ] = scale * pml.forward[ i ] * pm->cmd.forwardmove + scale * pml.right[ i ] * pm->cmd.rightmove;
	}

	wishvel[ 2 ] += scale * pm->cmd.upmove;

	VectorCopy ( wishvel, wishdir );
	wishspeed = VectorNormalize ( wishdir );

	if ( wishspeed > pm->ps->speed * pm_swimScale )
	{
		wishspeed = pm->ps->speed * pm_swimScale;
	}

	PM_Accelerate ( wishdir, wishspeed, pm_accelerate );

	//slanty ladders
	if ( pml.groundPlane && DotProduct ( pm->ps->velocity, pml.groundTrace.plane.normal ) < 0.0f )
	{
		vel = VectorLength ( pm->ps->velocity );

		// slide along the ground plane
		PM_ClipVelocity ( pm->ps->velocity, pml.groundTrace.plane.normal,
		                  pm->ps->velocity, OVERCLIP );

		VectorNormalize ( pm->ps->velocity );
		VectorScale ( pm->ps->velocity, vel, pm->ps->velocity );
	}

	PM_SlideMove ( qfalse );
}

/*
=============
PM_CheckLadder

Check to see if the player is on a ladder or not
=============
*/
static void PM_CheckLadder ( void )
{
	vec3_t  forward, end;
	trace_t trace;

	//test if class can use ladders
	if ( !BG_ClassHasAbility ( pm->ps->stats[ STAT_PCLASS ], SCA_CANUSELADDERS ) )
	{
		pml.ladder = qfalse;
		return;
	}

	VectorCopy ( pml.forward, forward );
	forward[ 2 ] = 0.0f;

	VectorMA ( pm->ps->origin, 1.0f, forward, end );

	pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, end, pm->ps->clientNum, MASK_PLAYERSOLID );

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
static void PM_DeadMove ( void )
{
	float forward;

	if ( !pml.walking )
	{
		return;
	}

	// extra friction

	forward = VectorLength ( pm->ps->velocity );
	forward -= 20;

	if ( forward <= 0 )
	{
		VectorClear ( pm->ps->velocity );
	}
	else
	{
		VectorNormalize ( pm->ps->velocity );
		VectorScale ( pm->ps->velocity, forward, pm->ps->velocity );
	}
}

/*
===============
PM_NoclipMove
===============
*/
static void PM_NoclipMove ( void )
{
	float  speed, drop, friction, control, newspeed;
	int    i;
	vec3_t wishvel;
	float  fmove, smove;
	vec3_t wishdir;
	float  wishspeed;
	float  scale;

	pm->ps->viewheight = DEFAULT_VIEWHEIGHT;

	// friction

	speed = VectorLength ( pm->ps->velocity );

	if ( speed < 1 )
	{
		VectorCopy ( vec3_origin, pm->ps->velocity );
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

		VectorScale ( pm->ps->velocity, newspeed, pm->ps->velocity );
	}

	// accelerate
	scale = PM_CmdScale ( &pm->cmd );

	fmove = pm->cmd.forwardmove;
	smove = pm->cmd.rightmove;

	for ( i = 0; i < 3; i++ )
	{
		wishvel[ i ] = pml.forward[ i ] * fmove + pml.right[ i ] * smove;
	}

	wishvel[ 2 ] += pm->cmd.upmove;

	VectorCopy ( wishvel, wishdir );
	wishspeed = VectorNormalize ( wishdir );
	wishspeed *= scale;

	PM_Accelerate ( wishdir, wishspeed, pm_accelerate );

	// move
	VectorMA ( pm->ps->origin, pml.frametime, pm->ps->velocity, pm->ps->origin );
}

//============================================================================

/*
================
PM_FootstepForSurface

Returns an event number apropriate for the groundsurface
================
*/
static int PM_FootstepForSurface ( void )
{
	//TA:
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
PM_CrashLand

Check for hard landings that generate sound events
=================
*/
static void PM_CrashLand ( void )
{
	float delta;
	float dist;
	float vel, acc;
	float t;
	float a, b, c, den;

	// decide which landing animation to use
	if ( pm->ps->pm_flags & PMF_BACKWARDS_JUMP )
	{
		if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim ( LEGS_LANDB );
		}
		else
		{
			PM_ForceLegsAnim ( NSPA_LANDBACK );
		}
	}
	else
	{
		if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			PM_ForceLegsAnim ( LEGS_LAND );
		}
		else
		{
			PM_ForceLegsAnim ( NSPA_LAND );
		}
	}

	if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
	{
		pm->ps->legsTimer = TIMER_LAND;
	}
	else
	{
		pm->ps->torsoTimer = TIMER_LAND;
	}

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

	t = ( -b - sqrt ( den ) ) / ( 2 * a );

	delta = vel + t * acc;
	delta = delta * delta * 0.0001;

	// ducking while falling doubles damage
	if ( pm->ps->pm_flags & PMF_DUCKED )
	{
		delta *= 2;
	}

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
	if ( ! ( pml.groundTrace.surfaceFlags & SURF_NODAMAGE ) )
	{
		pm->ps->stats[ STAT_FALLDIST ] = delta;

		if ( delta > AVG_FALL_DISTANCE )
		{
			PM_AddEvent ( EV_FALL_FAR );
		}
		else if ( delta > MIN_FALL_DISTANCE )
		{
			// this is a pain grunt, so don't play it if dead
			if ( pm->ps->stats[ STAT_HEALTH ] > 0 )
			{
				PM_AddEvent ( EV_FALL_MEDIUM );
			}
		}
		else
		{
			if ( delta > 7 )
			{
				PM_AddEvent ( EV_FALL_SHORT );
			}
			else
			{
				PM_AddEvent ( PM_FootstepForSurface() );
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
static int PM_CorrectAllSolid ( trace_t *trace )
{
	int    i, j, k;
	vec3_t point;

	if ( pm->debugLevel )
	{
		Com_Printf ( "%i:allsolid\n", c_pmove );
	}

	// jitter around
	for ( i = -1; i <= 1; i++ )
	{
		for ( j = -1; j <= 1; j++ )
		{
			for ( k = -1; k <= 1; k++ )
			{
				VectorCopy ( pm->ps->origin, point );
				point[ 0 ] += ( float ) i;
				point[ 1 ] += ( float ) j;
				point[ 2 ] += ( float ) k;
				pm->trace ( trace, point, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

				if ( !trace->allsolid )
				{
					point[ 0 ] = pm->ps->origin[ 0 ];
					point[ 1 ] = pm->ps->origin[ 1 ];
					point[ 2 ] = pm->ps->origin[ 2 ] - 0.25;

					pm->trace ( trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
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
static void PM_GroundTraceMissed ( void )
{
	trace_t trace;
	vec3_t  point;

	if ( pm->ps->groundEntityNum != ENTITYNUM_NONE )
	{
		// we just transitioned into freefall
		if ( pm->debugLevel )
		{
			Com_Printf ( "%i:lift\n", c_pmove );
		}

		// if they aren't in a jumping animation and the ground is a ways away, force into it
		// if we didn't do the trace, the player would be backflipping down staircases
		VectorCopy ( pm->ps->origin, point );
		point[ 2 ] -= 64.0f;

		pm->trace ( &trace, pm->ps->origin, NULL, NULL, point, pm->ps->clientNum, pm->tracemask );

		if ( trace.fraction == 1.0f )
		{
			if ( pm->cmd.forwardmove >= 0 )
			{
				if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ForceLegsAnim ( LEGS_JUMP );
				}
				else
				{
					PM_ForceLegsAnim ( NSPA_JUMP );
				}

				pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
			}
			else
			{
				if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ForceLegsAnim ( LEGS_JUMPB );
				}
				else
				{
					PM_ForceLegsAnim ( NSPA_JUMPBACK );
				}

				pm->ps->pm_flags |= PMF_BACKWARDS_JUMP;
			}
		}
	}

	if ( BG_ClassHasAbility ( pm->ps->stats[ STAT_PCLASS ], SCA_TAKESFALLDAMAGE ) )
	{
		if ( pm->ps->velocity[ 2 ] < FALLING_THRESHOLD && pml.previous_velocity[ 2 ] >= FALLING_THRESHOLD )
		{
			PM_AddEvent ( EV_FALLING );
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
static void PM_GroundClimbTrace ( void )
{
	vec3_t  surfNormal, movedir, lookdir, point;
	vec3_t  refNormal = { 0.0f, 0.0f, 1.0f };
	vec3_t  ceilingNormal = { 0.0f, 0.0f, -1.0f };
	vec3_t  toAngles, surfAngles;
	trace_t trace;
	int     i;

	//used for delta correction
	vec3_t  traceCROSSsurf, traceCROSSref, surfCROSSref;
	float   traceDOTsurf, traceDOTref, surfDOTref, rTtDOTrTsTt;
	float   traceANGsurf, traceANGref, surfANGref;
	vec3_t  horizontal = { 1.0f, 0.0f, 0.0f }; //arbituary vector perpendicular to refNormal
	vec3_t  refTOtrace, refTOsurfTOtrace, tempVec;
	int     rTtANGrTsTt;
	float   ldDOTtCs, d;
	vec3_t  abc;

	//TA: If we're on the ceiling then grapplePoint is a rotation normal.. otherwise its a surface normal.
	//    would have been nice if Carmack had left a few random variables in the ps struct for mod makers
	if ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING )
	{
		VectorCopy ( ceilingNormal, surfNormal );
	}
	else
	{
		VectorCopy ( pm->ps->grapplePoint, surfNormal );
	}

	//construct a vector which reflects the direction the player is looking wrt the surface normal
	ProjectPointOnPlane ( movedir, pml.forward, surfNormal );
	VectorNormalize ( movedir );

	VectorCopy ( movedir, lookdir );

	if ( pm->cmd.forwardmove < 0 )
	{
		VectorNegate ( movedir, movedir );
	}

	//allow strafe transitions
	if ( pm->cmd.rightmove )
	{
		VectorCopy ( pml.right, movedir );

		if ( pm->cmd.rightmove < 0 )
		{
			VectorNegate ( movedir, movedir );
		}
	}

	for ( i = 0; i <= 4; i++ )
	{
		switch ( i )
		{
			case 0:

				//we are going to step this frame so skip the transition test
				if ( PM_PredictStepMove() )
				{
					continue;
				}

				//trace into direction we are moving
				VectorMA ( pm->ps->origin, 0.25f, movedir, point );
				pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
				break;

			case 1:
				//trace straight down anto "ground" surface
				VectorMA ( pm->ps->origin, -0.25f, surfNormal, point );
				pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
				break;

			case 2:
				if ( pml.groundPlane != qfalse && PM_PredictStepMove() )
				{
					//step down
					VectorMA ( pm->ps->origin, -STEPSIZE, surfNormal, point );
					pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
				}
				else
				{
					continue;
				}

				break;

			case 3:

				//trace "underneath" BBOX so we can traverse angles > 180deg
				if ( pml.groundPlane != qfalse )
				{
					VectorMA ( pm->ps->origin, -16.0f, surfNormal, point );
					VectorMA ( point, -16.0f, movedir, point );
					pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
				}
				else
				{
					continue;
				}

				break;

			case 4:
				//fall back so we don't have to modify PM_GroundTrace too much
				VectorCopy ( pm->ps->origin, point );
				point[ 2 ] = pm->ps->origin[ 2 ] - 0.25f;
				pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );
				break;
		}

		//if we hit something
		if ( trace.fraction < 1.0f && ! ( trace.surfaceFlags & ( SURF_SKY | SURF_SLICK ) ) &&
		     ! ( trace.entityNum != ENTITYNUM_WORLD && i != 4 ) )
		{
			if ( i == 2 || i == 3 )
			{
				if ( i == 2 )
				{
					PM_StepEvent ( pm->ps->origin, trace.endpos, surfNormal );
				}

				VectorCopy ( trace.endpos, pm->ps->origin );
			}

			//calculate a bunch of stuff...
			CrossProduct ( trace.plane.normal, surfNormal, traceCROSSsurf );
			VectorNormalize ( traceCROSSsurf );

			CrossProduct ( trace.plane.normal, refNormal, traceCROSSref );
			VectorNormalize ( traceCROSSref );

			CrossProduct ( surfNormal, refNormal, surfCROSSref );
			VectorNormalize ( surfCROSSref );

			//calculate angle between surf and trace
			traceDOTsurf = DotProduct ( trace.plane.normal, surfNormal );
			traceANGsurf = RAD2DEG ( acos ( traceDOTsurf ) );

			if ( traceANGsurf > 180.0f )
			{
				traceANGsurf -= 180.0f;
			}

			//calculate angle between trace and ref
			traceDOTref = DotProduct ( trace.plane.normal, refNormal );
			traceANGref = RAD2DEG ( acos ( traceDOTref ) );

			if ( traceANGref > 180.0f )
			{
				traceANGref -= 180.0f;
			}

			//calculate angle between surf and ref
			surfDOTref = DotProduct ( surfNormal, refNormal );
			surfANGref = RAD2DEG ( acos ( surfDOTref ) );

			if ( surfANGref > 180.0f )
			{
				surfANGref -= 180.0f;
			}

			//if the trace result and old surface normal are different then we must have transided to a new
			//surface... do some stuff...
			if ( !VectorCompare ( trace.plane.normal, surfNormal ) )
			{
				//if the trace result or the old vector is not the floor or ceiling correct the YAW angle
				if ( !VectorCompare ( trace.plane.normal, refNormal ) && !VectorCompare ( surfNormal, refNormal ) &&
				     !VectorCompare ( trace.plane.normal, ceilingNormal ) && !VectorCompare ( surfNormal, ceilingNormal ) )
				{
					//behold the evil mindfuck from hell
					//it has fucked mind like nothing has fucked mind before

					//calculate reference rotated through to trace plane
					RotatePointAroundVector ( refTOtrace, traceCROSSref, horizontal, -traceANGref );

					//calculate reference rotated through to surf plane then to trace plane
					RotatePointAroundVector ( tempVec, surfCROSSref, horizontal, -surfANGref );
					RotatePointAroundVector ( refTOsurfTOtrace, traceCROSSsurf, tempVec, -traceANGsurf );

					//calculate angle between refTOtrace and refTOsurfTOtrace
					rTtDOTrTsTt = DotProduct ( refTOtrace, refTOsurfTOtrace );
					rTtANGrTsTt = ANGLE2SHORT ( RAD2DEG ( acos ( rTtDOTrTsTt ) ) );

					if ( rTtANGrTsTt > 32768 )
					{
						rTtANGrTsTt -= 32768;
					}

					CrossProduct ( refTOtrace, refTOsurfTOtrace, tempVec );
					VectorNormalize ( tempVec );

					if ( DotProduct ( trace.plane.normal, tempVec ) > 0.0f )
					{
						rTtANGrTsTt = -rTtANGrTsTt;
					}

					//phew! - correct the angle
					pm->ps->delta_angles[ YAW ] -= rTtANGrTsTt;
				}

				//construct a plane dividing the surf and trace normals
				CrossProduct ( traceCROSSsurf, surfNormal, abc );
				VectorNormalize ( abc );
				d = DotProduct ( abc, pm->ps->origin );

				//construct a point representing where the player is looking
				VectorAdd ( pm->ps->origin, lookdir, point );

				//check whether point is on one side of the plane, if so invert the correction angle
				if ( ( abc[ 0 ] * point[ 0 ] + abc[ 1 ] * point[ 1 ] + abc[ 2 ] * point[ 2 ] - d ) > 0 )
				{
					traceANGsurf = -traceANGsurf;
				}

				//find the . product of the lookdir and traceCROSSsurf
				if ( ( ldDOTtCs = DotProduct ( lookdir, traceCROSSsurf ) ) < 0.0f )
				{
					VectorInverse ( traceCROSSsurf );
					ldDOTtCs = DotProduct ( lookdir, traceCROSSsurf );
				}

				//set the correction angle
				traceANGsurf *= 1.0f - ldDOTtCs;

				if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_WALLCLIMBINGFOLLOW ) )
				{
					//correct the angle
					pm->ps->delta_angles[ PITCH ] -= ANGLE2SHORT ( traceANGsurf );
				}

				//transition from wall to ceiling
				//normal for subsequent viewangle rotations
				if ( VectorCompare ( trace.plane.normal, ceilingNormal ) )
				{
					CrossProduct ( surfNormal, trace.plane.normal, pm->ps->grapplePoint );
					VectorNormalize ( pm->ps->grapplePoint );
					pm->ps->stats[ STAT_STATE ] |= SS_WALLCLIMBINGCEILING;
				}

				//transition from ceiling to wall
				//we need to do some different angle correction here cos GPISROTVEC
				if ( VectorCompare ( surfNormal, ceilingNormal ) )
				{
					vectoangles ( trace.plane.normal, toAngles );
					vectoangles ( pm->ps->grapplePoint, surfAngles );

					pm->ps->delta_angles[ 1 ] -= ANGLE2SHORT ( ( ( surfAngles[ 1 ] - toAngles[ 1 ] ) * 2 ) - 180.0f );
				}
			}

			pml.groundTrace = trace;

			//so everything knows where we're wallclimbing (ie client side)
			pm->ps->eFlags |= EF_WALLCLIMB;

			//if we're not stuck to the ceiling then set grapplePoint to be a surface normal
			if ( !VectorCompare ( trace.plane.normal, ceilingNormal ) )
			{
				//so we know what surface we're stuck to
				VectorCopy ( trace.plane.normal, pm->ps->grapplePoint );
				pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBINGCEILING;
			}

			//IMPORTANT: break out of the for loop if we've hit something
			break;
		}
		else if ( trace.allsolid )
		{
			// do something corrective if the trace starts in a solid...
			if ( !PM_CorrectAllSolid ( &trace ) )
			{
				return;
			}
		}
	}

	if ( trace.fraction >= 1.0f )
	{
		// if the trace didn't hit anything, we are in free fall
		PM_GroundTraceMissed();
		pml.groundPlane = qfalse;
		pml.walking = qfalse;
		pm->ps->eFlags &= ~EF_WALLCLIMB;

		//just transided from ceiling to floor... apply delta correction
		if ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING )
		{
			vec3_t forward, rotated, angles;

			AngleVectors ( pm->ps->viewangles, forward, NULL, NULL );

			RotatePointAroundVector ( rotated, pm->ps->grapplePoint, forward, 180.0f );
			vectoangles ( rotated, angles );

			pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT ( angles[ YAW ] - pm->ps->viewangles[ YAW ] );
		}

		pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBINGCEILING;

		//we get very bizarre effects if we don't do this :0
		VectorCopy ( refNormal, pm->ps->grapplePoint );
		return;
	}

	pml.groundPlane = qtrue;
	pml.walking = qtrue;

	// hitting solid ground will end a waterjump
	if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
	{
		pm->ps->pm_flags &= ~ ( PMF_TIME_WATERJUMP | PMF_TIME_LAND );
		pm->ps->pm_time = 0;
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//  pm->ps->velocity[2] = 0;

	PM_AddTouchEnt ( trace.entityNum );
}

/*
=============
PM_GroundTrace
=============
*/
static void PM_GroundTrace ( void )
{
	vec3_t  point;
	vec3_t  movedir;
	vec3_t  refNormal = { 0.0f, 0.0f, 1.0f };
	trace_t trace;

	if ( BG_ClassHasAbility ( pm->ps->stats[ STAT_PCLASS ], SCA_WALLCLIMBER ) )
	{
		if ( pm->ps->persistant[ PERS_STATE ] & PS_WALLCLIMBINGTOGGLE )
		{
			//toggle wall climbing if holding crouch
			if ( pm->cmd.upmove < 0 && ! ( pm->ps->pm_flags & PMF_CROUCH_HELD ) )
			{
				if ( ! ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
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

		//just transided from ceiling to floor... apply delta correction
		if ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING )
		{
			vec3_t forward, rotated, angles;

			AngleVectors ( pm->ps->viewangles, forward, NULL, NULL );

			RotatePointAroundVector ( rotated, pm->ps->grapplePoint, forward, 180.0f );
			vectoangles ( rotated, angles );

			pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT ( angles[ YAW ] - pm->ps->viewangles[ YAW ] );
		}
	}

	pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBING;
	pm->ps->stats[ STAT_STATE ] &= ~SS_WALLCLIMBINGCEILING;
	pm->ps->eFlags &= ~EF_WALLCLIMB;

	point[ 0 ] = pm->ps->origin[ 0 ];
	point[ 1 ] = pm->ps->origin[ 1 ];
	point[ 2 ] = pm->ps->origin[ 2 ] - 0.25f;

	pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

	pml.groundTrace = trace;

	// do something corrective if the trace starts in a solid...
	if ( trace.allsolid )
	{
		if ( !PM_CorrectAllSolid ( &trace ) )
		{
			return;
		}
	}

	//make sure that the surfNormal is reset to the ground
	VectorCopy ( refNormal, pm->ps->grapplePoint );

	// if the trace didn't hit anything, we are in free fall
	if ( trace.fraction == 1.0f )
	{
		qboolean steppedDown = qfalse;

		// try to step down
		if ( pml.groundPlane != qfalse && PM_PredictStepMove() )
		{
			//step down
			point[ 0 ] = pm->ps->origin[ 0 ];
			point[ 1 ] = pm->ps->origin[ 1 ];
			point[ 2 ] = pm->ps->origin[ 2 ] - STEPSIZE;
			pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

			//if we hit something
			if ( trace.fraction < 1.0f )
			{
				PM_StepEvent ( pm->ps->origin, trace.endpos, refNormal );
				VectorCopy ( trace.endpos, pm->ps->origin );
				steppedDown = qtrue;
			}
		}

		if ( !steppedDown )
		{
			PM_GroundTraceMissed();
			pml.groundPlane = qfalse;
			pml.walking = qfalse;

			if ( BG_ClassHasAbility ( pm->ps->stats[ STAT_PCLASS ], SCA_WALLJUMPER ) )
			{
				ProjectPointOnPlane ( movedir, pml.forward, refNormal );
				VectorNormalize ( movedir );

				if ( pm->cmd.forwardmove < 0 )
				{
					VectorNegate ( movedir, movedir );
				}

				//allow strafe transitions
				if ( pm->cmd.rightmove )
				{
					VectorCopy ( pml.right, movedir );

					if ( pm->cmd.rightmove < 0 )
					{
						VectorNegate ( movedir, movedir );
					}
				}

				//trace into direction we are moving
				VectorMA ( pm->ps->origin, 0.25f, movedir, point );
				pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, point, pm->ps->clientNum, pm->tracemask );

				if ( trace.fraction < 1.0f && ! ( trace.surfaceFlags & ( SURF_SKY | SURF_SLICK ) ) &&
				     ( trace.entityNum == ENTITYNUM_WORLD ) )
				{
					if ( !VectorCompare ( trace.plane.normal, pm->ps->grapplePoint ) )
					{
						VectorCopy ( trace.plane.normal, pm->ps->grapplePoint );
						PM_CheckWallJump();
					}
				}
			}

			return;
		}
	}

	// check if getting thrown off the ground
	if ( pm->ps->velocity[ 2 ] > 0.0f && DotProduct ( pm->ps->velocity, trace.plane.normal ) > 10.0f )
	{
		if ( pm->debugLevel )
		{
			Com_Printf ( "%i:kickoff\n", c_pmove );
		}

		// go into jump animation
		if ( pm->cmd.forwardmove >= 0 )
		{
			if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
			{
				PM_ForceLegsAnim ( LEGS_JUMP );
			}
			else
			{
				PM_ForceLegsAnim ( NSPA_JUMP );
			}

			pm->ps->pm_flags &= ~PMF_BACKWARDS_JUMP;
		}
		else
		{
			if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
			{
				PM_ForceLegsAnim ( LEGS_JUMPB );
			}
			else
			{
				PM_ForceLegsAnim ( NSPA_JUMPBACK );
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
		if ( pm->debugLevel )
		{
			Com_Printf ( "%i:steep\n", c_pmove );
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
		pm->ps->pm_flags &= ~ ( PMF_TIME_WATERJUMP | PMF_TIME_LAND );
		pm->ps->pm_time = 0;
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{
		// just hit the ground
		if ( pm->debugLevel )
		{
			Com_Printf ( "%i:Land\n", c_pmove );
		}

		if ( BG_ClassHasAbility ( pm->ps->stats[ STAT_PCLASS ], SCA_TAKESFALLDAMAGE ) )
		{
			PM_CrashLand();
		}

		// don't do landing time if we were just going down a slope
		if ( pml.previous_velocity[ 2 ] < -200 )
		{
			// don't allow another jump for a little while
			pm->ps->pm_flags |= PMF_TIME_LAND;
			pm->ps->pm_time = 250;
		}
	}

	pm->ps->groundEntityNum = trace.entityNum;

	// don't reset the z velocity for slopes
//  pm->ps->velocity[2] = 0;

	PM_AddTouchEnt ( trace.entityNum );
}

/*
=============
PM_SetWaterLevel  FIXME: avoid this twice?  certainly if not moving
=============
*/
static void PM_SetWaterLevel ( void )
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
	cont = pm->pointcontents ( point, pm->ps->clientNum );

	if ( cont & MASK_WATER )
	{
		sample2 = pm->ps->viewheight - MINS_Z;
		sample1 = sample2 / 2;

		pm->watertype = cont;
		pm->waterlevel = 1;
		point[ 2 ] = pm->ps->origin[ 2 ] + MINS_Z + sample1;
		cont = pm->pointcontents ( point, pm->ps->clientNum );

		if ( cont & MASK_WATER )
		{
			pm->waterlevel = 2;
			point[ 2 ] = pm->ps->origin[ 2 ] + MINS_Z + sample2;
			cont = pm->pointcontents ( point, pm->ps->clientNum );

			if ( cont & MASK_WATER )
			{
				pm->waterlevel = 3;
			}
		}
	}
}

/*
==============
PM_CheckDuck

Sets mins, maxs, and pm->ps->viewheight
==============
*/
static void PM_CheckDuck ( void )
{
	trace_t trace;
	vec3_t  PCmins, PCmaxs, PCcmaxs;
	int     PCvh, PCcvh;

	BG_FindBBoxForClass ( pm->ps->stats[ STAT_PCLASS ], PCmins, PCmaxs, PCcmaxs, NULL, NULL );
	BG_FindViewheightForClass ( pm->ps->stats[ STAT_PCLASS ], &PCvh, &PCcvh );

	//TA: iD bug? you can still crouch when you're a spectator
	if ( pm->ps->persistant[ PERS_TEAM ] == TEAM_SPECTATOR )
	{
		PCcvh = PCvh;
	}

	pm->mins[ 0 ] = PCmins[ 0 ];
	pm->mins[ 1 ] = PCmins[ 1 ];

	pm->maxs[ 0 ] = PCmaxs[ 0 ];
	pm->maxs[ 1 ] = PCmaxs[ 1 ];

	pm->mins[ 2 ] = PCmins[ 2 ];

	if ( pm->ps->pm_type == PM_DEAD )
	{
		pm->maxs[ 2 ] = -8;
		pm->ps->viewheight = DEAD_VIEWHEIGHT;
		return;
	}

	//TA: If the standing and crouching viewheights are the same the class can't crouch
	if ( ( pm->cmd.upmove < 0 ) && ( PCvh != PCcvh ) &&
	     pm->ps->pm_type != PM_JETPACK &&
	     !BG_InventoryContainsUpgrade ( UP_BATTLESUIT, pm->ps->stats ) )
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
			pm->trace ( &trace, pm->ps->origin, pm->mins, pm->maxs, pm->ps->origin, pm->ps->clientNum, pm->tracemask );

			if ( !trace.allsolid )
			{
				pm->ps->pm_flags &= ~PMF_DUCKED;
			}
		}
	}

	if ( pm->ps->pm_flags & PMF_DUCKED )
	{
		pm->maxs[ 2 ] = PCcmaxs[ 2 ];
		pm->ps->viewheight = PCcvh;
	}
	else
	{
		pm->maxs[ 2 ] = PCmaxs[ 2 ];
		pm->ps->viewheight = PCvh;
	}
}

//===================================================================

/*
===============
PM_Footsteps
===============
*/
static void PM_Footsteps ( void )
{
	float    bobmove;
	int      old;
	qboolean footstep;

	//
	// calculate speed and cycle to be used for
	// all cyclic walking effects
	//
	if ( BG_ClassHasAbility ( pm->ps->stats[ STAT_PCLASS ], SCA_WALLCLIMBER ) && ( pml.groundPlane ) )
	{
		//TA: FIXME: yes yes i know this is wrong
		pm->xyspeed = sqrt ( pm->ps->velocity[ 0 ] * pm->ps->velocity[ 0 ]
		                     + pm->ps->velocity[ 1 ] * pm->ps->velocity[ 1 ]
		                     + pm->ps->velocity[ 2 ] * pm->ps->velocity[ 2 ] );
	}
	else
	{
		pm->xyspeed = sqrt ( pm->ps->velocity[ 0 ] * pm->ps->velocity[ 0 ]
		                     + pm->ps->velocity[ 1 ] * pm->ps->velocity[ 1 ] );
	}

	if ( pm->ps->groundEntityNum == ENTITYNUM_NONE )
	{
		// airborne leaves position in cycle intact, but doesn't advance
		if ( pm->waterlevel > 1 )
		{
			if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
			{
				PM_ContinueLegsAnim ( LEGS_SWIM );
			}
			else
			{
				PM_ContinueLegsAnim ( NSPA_SWIM );
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
				if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim ( LEGS_IDLECR );
				}
				else
				{
					PM_ContinueLegsAnim ( NSPA_STAND );
				}
			}
			else
			{
				if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim ( LEGS_IDLE );
				}
				else
				{
					PM_ContinueLegsAnim ( NSPA_STAND );
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
			if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
			{
				PM_ContinueLegsAnim ( LEGS_BACKCR );
			}
			else
			{
				if ( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
				{
					PM_ContinueLegsAnim ( NSPA_WALKRIGHT );
				}
				else if ( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
				{
					PM_ContinueLegsAnim ( NSPA_WALKLEFT );
				}
				else
				{
					PM_ContinueLegsAnim ( NSPA_WALKBACK );
				}
			}
		}
		else
		{
			if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
			{
				PM_ContinueLegsAnim ( LEGS_WALKCR );
			}
			else
			{
				if ( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
				{
					PM_ContinueLegsAnim ( NSPA_WALKRIGHT );
				}
				else if ( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
				{
					PM_ContinueLegsAnim ( NSPA_WALKLEFT );
				}
				else
				{
					PM_ContinueLegsAnim ( NSPA_WALK );
				}
			}
		}

		// ducked characters never play footsteps
	}
	else
	{
		if ( ! ( pm->cmd.buttons & BUTTON_WALKING ) )
		{
			bobmove = 0.4f; // faster speeds bob faster

			if ( pm->ps->weapon == WP_ALEVEL4 && pm->ps->pm_flags & PMF_CHARGE )
			{
				PM_ContinueLegsAnim ( NSPA_CHARGE );
			}
			else if ( pm->ps->pm_flags & PMF_BACKWARDS_RUN )
			{
				if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim ( LEGS_BACK );
				}
				else
				{
					if ( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
					{
						PM_ContinueLegsAnim ( NSPA_RUNRIGHT );
					}
					else if ( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
					{
						PM_ContinueLegsAnim ( NSPA_RUNLEFT );
					}
					else
					{
						PM_ContinueLegsAnim ( NSPA_RUNBACK );
					}
				}
			}
			else
			{
				if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim ( LEGS_RUN );
				}
				else
				{
					if ( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
					{
						PM_ContinueLegsAnim ( NSPA_RUNRIGHT );
					}
					else if ( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
					{
						PM_ContinueLegsAnim ( NSPA_RUNLEFT );
					}
					else
					{
						PM_ContinueLegsAnim ( NSPA_RUN );
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
				if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim ( LEGS_BACKWALK );
				}
				else
				{
					if ( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
					{
						PM_ContinueLegsAnim ( NSPA_WALKRIGHT );
					}
					else if ( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
					{
						PM_ContinueLegsAnim ( NSPA_WALKLEFT );
					}
					else
					{
						PM_ContinueLegsAnim ( NSPA_WALKBACK );
					}
				}
			}
			else
			{
				if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
				{
					PM_ContinueLegsAnim ( LEGS_WALK );
				}
				else
				{
					if ( pm->cmd.rightmove > 0 && !pm->cmd.forwardmove )
					{
						PM_ContinueLegsAnim ( NSPA_WALKRIGHT );
					}
					else if ( pm->cmd.rightmove < 0 && !pm->cmd.forwardmove )
					{
						PM_ContinueLegsAnim ( NSPA_WALKLEFT );
					}
					else
					{
						PM_ContinueLegsAnim ( NSPA_WALK );
					}
				}
			}
		}
	}

	bobmove *= BG_FindBobCycleForClass ( pm->ps->stats[ STAT_PCLASS ] );

	if ( pm->ps->stats[ STAT_STATE ] & SS_SPEEDBOOST )
	{
		bobmove *= HUMAN_SPRINT_MODIFIER;
	}

	// check for footstep / splash sounds
	old = pm->ps->bobCycle;
	pm->ps->bobCycle = ( int ) ( old + bobmove * pml.msec ) & 255;

	// if we just crossed a cycle boundary, play an apropriate footstep event
	if ( ( ( old + 64 ) ^ ( pm->ps->bobCycle + 64 ) ) & 128 )
	{
		if ( pm->waterlevel == 0 )
		{
			// on ground will only play sounds if running
			if ( footstep && !pm->noFootsteps )
			{
				PM_AddEvent ( PM_FootstepForSurface() );
			}
		}
		else if ( pm->waterlevel == 1 )
		{
			// splashing
			PM_AddEvent ( EV_FOOTSPLASH );
		}
		else if ( pm->waterlevel == 2 )
		{
			// wading / swimming at surface
			PM_AddEvent ( EV_SWIM );
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
static void PM_WaterEvents ( void )
{
	// FIXME?
	//
	// if just entered a water volume, play a sound
	//
	if ( !pml.previous_waterlevel && pm->waterlevel )
	{
		PM_AddEvent ( EV_WATER_TOUCH );
	}

	//
	// if just completely exited a water volume, play a sound
	//
	if ( pml.previous_waterlevel && !pm->waterlevel )
	{
		PM_AddEvent ( EV_WATER_LEAVE );
	}

	//
	// check for head just going under water
	//
	if ( pml.previous_waterlevel != 3 && pm->waterlevel == 3 )
	{
		PM_AddEvent ( EV_WATER_UNDER );
	}

	//
	// check for head just coming out of water
	//
	if ( pml.previous_waterlevel == 3 && pm->waterlevel != 3 )
	{
		PM_AddEvent ( EV_WATER_CLEAR );
	}
}

/*
===============
PM_BeginWeaponChange
===============
*/
static void PM_BeginWeaponChange ( int weapon )
{
	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS )
	{
		return;
	}

	if ( !BG_InventoryContainsWeapon ( weapon, pm->ps->stats ) && weapon != WP_NONE )
	{
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_DROPPING )
	{
		return;
	}

	//special case to prevent storing a charged up lcannon
	if ( pm->ps->weapon == WP_LUCIFER_CANNON )
	{
		pm->ps->stats[ STAT_MISC ] = 0;
	}

	PM_AddEvent ( EV_CHANGE_WEAPON );
	pm->ps->weaponstate = WEAPON_DROPPING;
	pm->ps->weaponTime += 200;
	pm->ps->persistant[ PERS_NEWWEAPON ] = weapon;

	//reset build weapon
	pm->ps->stats[ STAT_BUILDABLE ] = BA_NONE;

	if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
	{
		PM_StartTorsoAnim ( TORSO_DROP );
	}
}

/*
===============
PM_FinishWeaponChange
===============
*/
static void PM_FinishWeaponChange ( void )
{
	int weapon;

	weapon = pm->ps->persistant[ PERS_NEWWEAPON ];

	if ( weapon < WP_NONE || weapon >= WP_NUM_WEAPONS )
	{
		weapon = WP_NONE;
	}

	if ( !BG_InventoryContainsWeapon ( weapon, pm->ps->stats ) )
	{
		weapon = WP_NONE;
	}

	pm->ps->weapon = weapon;
	pm->ps->weaponstate = WEAPON_RAISING;
	pm->ps->weaponTime += 250;

	if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
	{
		PM_StartTorsoAnim ( TORSO_RAISE );
	}
}

/*
==============
PM_TorsoAnimation

==============
*/
static void PM_TorsoAnimation ( void )
{
	if ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL )
	{
		return;
	}

	if ( pm->ps->weaponstate == WEAPON_READY )
	{
		if ( pm->ps->weapon == WP_BLASTER )
		{
			PM_ContinueTorsoAnim ( TORSO_STAND2 );
		}
		else
		{
			PM_ContinueTorsoAnim ( TORSO_STAND );
		}
	}
}

/*
==============
PM_Weapon

Generates weapon events and modifes the weapon counter
==============
*/
static void PM_Weapon ( void )
{
	int      addTime = 200; //default addTime - should never be used
	int      ammo, clips, maxClips;
	qboolean attack1 = qfalse;
	qboolean attack2 = qfalse;
	qboolean attack3 = qfalse;

	// don't allow attack until all buttons are up
	if ( pm->ps->pm_flags & PMF_RESPAWNED )
	{
		return;
	}

	// ignore if spectator
	if ( pm->ps->persistant[ PERS_TEAM ] == TEAM_SPECTATOR )
	{
		return;
	}

	if ( pm->ps->stats[ STAT_STATE ] & SS_INFESTING )
	{
		return;
	}

	if ( pm->ps->stats[ STAT_STATE ] & SS_HOVELING )
	{
		return;
	}

	// check for dead player
	if ( pm->ps->stats[ STAT_HEALTH ] <= 0 )
	{
		pm->ps->weapon = WP_NONE;
		return;
	}

	// make weapon function
	if ( pm->ps->weaponTime > 0 )
	{
		pm->ps->weaponTime -= pml.msec;
	}

	// check for weapon change
	// can't change if weapon is firing, but can change
	// again if lowering or raising
	if ( pm->ps->weaponTime <= 0 || pm->ps->weaponstate != WEAPON_FIRING )
	{
		//TA: must press use to switch weapons
		if ( pm->cmd.buttons & BUTTON_USE_HOLDABLE )
		{
			if ( ! ( pm->ps->pm_flags & PMF_USE_ITEM_HELD ) )
			{
				if ( pm->cmd.weapon <= 32 )
				{
					//if trying to select a weapon, select it
					if ( pm->ps->weapon != pm->cmd.weapon )
					{
						PM_BeginWeaponChange ( pm->cmd.weapon );
					}
				}
				else if ( pm->cmd.weapon > 32 )
				{
					//if trying to toggle an upgrade, toggle it
					if ( BG_InventoryContainsUpgrade ( pm->cmd.weapon - 32, pm->ps->stats ) ) //sanity check
					{
						if ( BG_UpgradeIsActive ( pm->cmd.weapon - 32, pm->ps->stats ) )
						{
							BG_DeactivateUpgrade ( pm->cmd.weapon - 32, pm->ps->stats );
						}
						else
						{
							BG_ActivateUpgrade ( pm->cmd.weapon - 32, pm->ps->stats );
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
			PM_BeginWeaponChange ( pm->ps->persistant[ PERS_NEWWEAPON ] );
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

	if ( pm->ps->weaponstate == WEAPON_RAISING )
	{
		pm->ps->weaponstate = WEAPON_READY;

		if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			if ( pm->ps->weapon == WP_BLASTER )
			{
				PM_ContinueTorsoAnim ( TORSO_STAND2 );
			}
			else
			{
				PM_ContinueTorsoAnim ( TORSO_STAND );
			}
		}

		return;
	}

	// start the animation even if out of ammo

	BG_UnpackAmmoArray ( pm->ps->weapon, pm->ps->ammo, pm->ps->powerups, &ammo, &clips );
	BG_FindAmmoForWeapon ( pm->ps->weapon, NULL, &maxClips );

	// check for out of ammo
	if ( !ammo && !clips && !BG_FindInfinteAmmoForWeapon ( pm->ps->weapon ) )
	{
		PM_AddEvent ( EV_NOAMMO );
		pm->ps->weaponTime += 200;
		return;
	}

	//done reloading so give em some ammo
	if ( pm->ps->weaponstate == WEAPON_RELOADING )
	{
		if ( maxClips > 0 )
		{
			clips--;
			BG_FindAmmoForWeapon ( pm->ps->weapon, &ammo, NULL );
		}

		if ( BG_FindUsesEnergyForWeapon ( pm->ps->weapon ) &&
		     BG_InventoryContainsUpgrade ( UP_BATTPACK, pm->ps->stats ) )
		{
			ammo = ( int ) ( ( float ) ammo * BATTPACK_MODIFIER );
		}

		BG_PackAmmoArray ( pm->ps->weapon, pm->ps->ammo, pm->ps->powerups, ammo, clips );

		//allow some time for the weapon to be raised
		pm->ps->weaponstate = WEAPON_RAISING;
		PM_StartTorsoAnim ( TORSO_RAISE );
		pm->ps->weaponTime += 250;
		return;
	}

	// check for end of clip
	if ( ( !ammo || pm->ps->pm_flags & PMF_WEAPON_RELOAD ) && clips )
	{
		pm->ps->pm_flags &= ~PMF_WEAPON_RELOAD;

		pm->ps->weaponstate = WEAPON_RELOADING;

		//drop the weapon
		PM_StartTorsoAnim ( TORSO_DROP );

		addTime = BG_FindReloadTimeForWeapon ( pm->ps->weapon );

		pm->ps->weaponTime += addTime;
		return;
	}

	//check if non-auto primary/secondary attacks are permited
	switch ( pm->ps->weapon )
	{
		case WP_ALEVEL0:
			//venom is only autohit
			attack1 = attack2 = attack3 = qfalse;

			if ( !pm->autoWeaponHit[ pm->ps->weapon ] )
			{
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
				return;
			}

			break;

		case WP_ALEVEL3:
		case WP_ALEVEL3_UPG:
			//pouncing has primary secondary AND autohit procedures
			attack1 = pm->cmd.buttons & BUTTON_ATTACK;
			attack2 = pm->cmd.buttons & BUTTON_ATTACK2;
			attack3 = pm->cmd.buttons & BUTTON_USE_HOLDABLE;

			if ( !pm->autoWeaponHit[ pm->ps->weapon ] && !attack1 && !attack2 && !attack3 )
			{
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
				return;
			}

			break;

		case WP_LUCIFER_CANNON:
			attack1 = pm->cmd.buttons & BUTTON_ATTACK;
			attack2 = pm->cmd.buttons & BUTTON_ATTACK2;
			attack3 = pm->cmd.buttons & BUTTON_USE_HOLDABLE;

			if ( ( attack1 || pm->ps->stats[ STAT_MISC ] == 0 ) && !attack2 && !attack3 )
			{
				if ( pm->ps->stats[ STAT_MISC ] < LCANNON_TOTAL_CHARGE )
				{
					pm->ps->weaponTime = 0;
					pm->ps->weaponstate = WEAPON_READY;
					return;
				}
				else
				{
					attack1 = !attack1;
				}
			}

			//erp this looks confusing
			if ( pm->ps->stats[ STAT_MISC ] > LCANNON_MIN_CHARGE )
			{
				attack1 = !attack1;
			}
			else if ( pm->ps->stats[ STAT_MISC ] > 0 )
			{
				pm->ps->stats[ STAT_MISC ] = 0;
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
				return;
			}

			break;

		default:
			//by default primary and secondary attacks are allowed
			attack1 = pm->cmd.buttons & BUTTON_ATTACK;
			attack2 = pm->cmd.buttons & BUTTON_ATTACK2;
			attack3 = pm->cmd.buttons & BUTTON_USE_HOLDABLE;

			if ( !attack1 && !attack2 && !attack3 )
			{
				pm->ps->weaponTime = 0;
				pm->ps->weaponstate = WEAPON_READY;
				return;
			}

			break;
	}

	//TA: fire events for non auto weapons
	if ( attack3 )
	{
		if ( BG_WeaponHasThirdMode ( pm->ps->weapon ) )
		{
			//hacky special case for slowblob
			if ( pm->ps->weapon == WP_ALEVEL3_UPG && !ammo )
			{
				PM_AddEvent ( EV_NOAMMO );
				pm->ps->weaponTime += 200;
				return;
			}

			pm->ps->generic1 = WPM_TERTIARY;
			PM_AddEvent ( EV_FIRE_WEAPON3 );
			addTime = BG_FindRepeatRate3ForWeapon ( pm->ps->weapon );
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
		if ( BG_WeaponHasAltMode ( pm->ps->weapon ) )
		{
			pm->ps->generic1 = WPM_SECONDARY;
			PM_AddEvent ( EV_FIRE_WEAPON2 );
			addTime = BG_FindRepeatRate2ForWeapon ( pm->ps->weapon );
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
		PM_AddEvent ( EV_FIRE_WEAPON );
		addTime = BG_FindRepeatRate1ForWeapon ( pm->ps->weapon );
	}

	//TA: fire events for autohit weapons
	if ( pm->autoWeaponHit[ pm->ps->weapon ] )
	{
		switch ( pm->ps->weapon )
		{
			case WP_ALEVEL0:
				pm->ps->generic1 = WPM_PRIMARY;
				PM_AddEvent ( EV_FIRE_WEAPON );
				addTime = BG_FindRepeatRate1ForWeapon ( pm->ps->weapon );
				break;

			case WP_ALEVEL3:
			case WP_ALEVEL3_UPG:
				pm->ps->generic1 = WPM_SECONDARY;
				PM_AddEvent ( EV_FIRE_WEAPON2 );
				addTime = BG_FindRepeatRate2ForWeapon ( pm->ps->weapon );
				break;

			default:
				break;
		}
	}

	if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
	{
		//FIXME: this should be an option in the client weapon.cfg
		switch ( pm->ps->weapon )
		{
			case WP_FLAMER:
				if ( pm->ps->weaponstate == WEAPON_READY )
				{
					PM_StartTorsoAnim ( TORSO_ATTACK );
				}

				break;

			case WP_BLASTER:
				PM_StartTorsoAnim ( TORSO_ATTACK2 );
				break;

			default:
				PM_StartTorsoAnim ( TORSO_ATTACK );
				break;
		}
	}
	else
	{
		if ( pm->ps->weapon == WP_ALEVEL4 )
		{
			//hack to get random attack animations
			//FIXME: does pm->ps->weaponTime cycle enough?
			int num = abs ( pm->ps->weaponTime ) % 3;

			if ( num == 0 )
			{
				PM_ForceLegsAnim ( NSPA_ATTACK1 );
			}
			else if ( num == 1 )
			{
				PM_ForceLegsAnim ( NSPA_ATTACK2 );
			}
			else if ( num == 2 )
			{
				PM_ForceLegsAnim ( NSPA_ATTACK3 );
			}
		}
		else
		{
			if ( attack1 )
			{
				PM_ForceLegsAnim ( NSPA_ATTACK1 );
			}
			else if ( attack2 )
			{
				PM_ForceLegsAnim ( NSPA_ATTACK2 );
			}
			else if ( attack3 )
			{
				PM_ForceLegsAnim ( NSPA_ATTACK3 );
			}
		}

		pm->ps->torsoTimer = TIMER_ATTACK;
	}

	pm->ps->weaponstate = WEAPON_FIRING;

	// take an ammo away if not infinite
	if ( !BG_FindInfinteAmmoForWeapon ( pm->ps->weapon ) )
	{
		//special case for lCanon
		if ( pm->ps->weapon == WP_LUCIFER_CANNON && attack1 )
		{
			ammo -= ( int ) ( ceil ( ( ( float ) pm->ps->stats[ STAT_MISC ] / ( float ) LCANNON_TOTAL_CHARGE ) * 10.0f ) );

			//stay on the safe side
			if ( ammo < 0 )
			{
				ammo = 0;
			}
		}
		else
		{
			ammo--;
		}

		BG_PackAmmoArray ( pm->ps->weapon, pm->ps->ammo, pm->ps->powerups, ammo, clips );
	}
	else if ( pm->ps->weapon == WP_ALEVEL3_UPG && attack3 )
	{
		//special case for slowblob
		ammo--;
		BG_PackAmmoArray ( pm->ps->weapon, pm->ps->ammo, pm->ps->powerups, ammo, clips );
	}

	//FIXME: predicted angles miss a problem??
	if ( pm->ps->weapon == WP_CHAINGUN )
	{
		if ( pm->ps->pm_flags & PMF_DUCKED ||
		     BG_InventoryContainsUpgrade ( UP_BATTLESUIT, pm->ps->stats ) )
		{
			pm->ps->delta_angles[ PITCH ] -= ANGLE2SHORT ( ( ( random() * 0.5 ) - 0.125 ) * ( 30 / ( float ) addTime ) );
			pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT ( ( ( random() * 0.5 ) - 0.25 ) * ( 30.0 / ( float ) addTime ) );
		}
		else
		{
			pm->ps->delta_angles[ PITCH ] -= ANGLE2SHORT ( ( ( random() * 8 ) - 2 ) * ( 30.0 / ( float ) addTime ) );
			pm->ps->delta_angles[ YAW ] -= ANGLE2SHORT ( ( ( random() * 8 ) - 4 ) * ( 30.0 / ( float ) addTime ) );
		}
	}

	pm->ps->weaponTime += addTime;
}

/*
================
PM_Animate
================
*/
static void PM_Animate ( void )
{
	if ( pm->cmd.buttons & BUTTON_GESTURE )
	{
		if ( ! ( pm->ps->persistant[ PERS_STATE ] & PS_NONSEGMODEL ) )
		{
			if ( pm->ps->torsoTimer == 0 )
			{
				PM_StartTorsoAnim ( TORSO_GESTURE );
				pm->ps->torsoTimer = TIMER_GESTURE;

				PM_AddEvent ( EV_TAUNT );
			}
		}
		else
		{
			if ( pm->ps->torsoTimer == 0 )
			{
				PM_ForceLegsAnim ( NSPA_GESTURE );
				pm->ps->torsoTimer = TIMER_GESTURE;

				PM_AddEvent ( EV_TAUNT );
			}
		}
	}
}

/*
================
PM_DropTimers
================
*/
static void PM_DropTimers ( void )
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
}

/*
================
PM_UpdateViewAngles

This can be used as another entry point when only the viewangles
are being updated instead of a full move
================
*/
void PM_UpdateViewAngles ( playerState_t *ps, const usercmd_t *cmd )
{
	short  temp[ 3 ];
	int    i;
	vec3_t axis[ 3 ], rotaxis[ 3 ];
	vec3_t tempang;

	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_SPINTERMISSION )
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
		temp[ i ] = cmd->angles[ i ] + ps->delta_angles[ i ];

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

		tempang[ i ] = SHORT2ANGLE ( temp[ i ] );
	}

	//convert viewangles -> axis
	AnglesToAxis ( tempang, axis );

	if ( ! ( ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) ||
	     !BG_RotateAxis ( ps->grapplePoint, axis, rotaxis, qfalse,
	                      ps->stats[ STAT_STATE ] & SS_WALLCLIMBINGCEILING ) )
	{
		AxisCopy ( axis, rotaxis );
	}

	//convert the new axis back to angles
	AxisToAngles ( rotaxis, tempang );

	//force angles to -180 <= x <= 180
	for ( i = 0; i < 3; i++ )
	{
		while ( tempang[ i ] > 180 )
		{
			tempang[ i ] -= 360;
		}

		while ( tempang[ i ] < 180 )
		{
			tempang[ i ] += 360;
		}
	}

	//actually set the viewangles
	for ( i = 0; i < 3; i++ )
	{
		ps->viewangles[ i ] = tempang[ i ];
	}

	//pull the view into the lock point
	if ( ps->pm_type == PM_GRABBED && !BG_InventoryContainsUpgrade ( UP_BATTLESUIT, ps->stats ) )
	{
		vec3_t dir, angles;

		ByteToDir ( ps->stats[ STAT_VIEWLOCK ], dir );
		vectoangles ( dir, angles );

		for ( i = 0; i < 3; i++ )
		{
			float diff = AngleSubtract ( ps->viewangles[ i ], angles[ i ] );

			while ( diff > 180.0f )
			{
				diff -= 360.0f;
			}

			while ( diff < -180.0f )
			{
				diff += 360.0f;
			}

			if ( diff < -90.0f )
			{
				ps->delta_angles[ i ] += ANGLE2SHORT ( fabs ( diff ) - 90.0f );
			}
			else if ( diff > 90.0f )
			{
				ps->delta_angles[ i ] -= ANGLE2SHORT ( fabs ( diff ) - 90.0f );
			}

			if ( diff < 0.0f )
			{
				ps->delta_angles[ i ] += ANGLE2SHORT ( fabs ( diff ) * 0.05f );
			}
			else if ( diff > 0.0f )
			{
				ps->delta_angles[ i ] -= ANGLE2SHORT ( fabs ( diff ) * 0.05f );
			}
		}
	}
}

/*
================
PmoveSingle

================
*/
void trap_SnapVector ( float *v );

void PmoveSingle ( pmove_t *pmove )
{
	int ammo, clips;

	pm = pmove;

	BG_UnpackAmmoArray ( pm->ps->weapon, pm->ps->ammo, pm->ps->powerups, &ammo, &clips );

	// this counter lets us debug movement problems with a journal
	// by setting a conditional breakpoint fot the previous frame
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
	if ( abs ( pm->cmd.forwardmove ) > 64 || abs ( pm->cmd.rightmove ) > 64 )
	{
		pm->cmd.buttons &= ~BUTTON_WALKING;
	}

	// set the talk balloon flag
	if ( pm->cmd.buttons & BUTTON_TALK )
	{
		pm->ps->eFlags |= EF_TALK;
	}
	else
	{
		pm->ps->eFlags &= ~EF_TALK;
	}

	// set the firing flag for continuous beam weapons
	if ( ! ( pm->ps->pm_flags & PMF_RESPAWNED ) && pm->ps->pm_type != PM_INTERMISSION &&
	     ( pm->cmd.buttons & BUTTON_ATTACK ) &&
	     ( ( ammo > 0 || clips > 0 ) || BG_FindInfinteAmmoForWeapon ( pm->ps->weapon ) ) )
	{
		pm->ps->eFlags |= EF_FIRING;
	}
	else
	{
		pm->ps->eFlags &= ~EF_FIRING;
	}

	// set the firing flag for continuous beam weapons
	if ( ! ( pm->ps->pm_flags & PMF_RESPAWNED ) && pm->ps->pm_type != PM_INTERMISSION &&
	     ( pm->cmd.buttons & BUTTON_ATTACK2 ) &&
	     ( ( ammo > 0 || clips > 0 ) || BG_FindInfinteAmmoForWeapon ( pm->ps->weapon ) ) )
	{
		pm->ps->eFlags |= EF_FIRING2;
	}
	else
	{
		pm->ps->eFlags &= ~EF_FIRING2;
	}

	// set the firing flag for continuous beam weapons
	if ( ! ( pm->ps->pm_flags & PMF_RESPAWNED ) && pm->ps->pm_type != PM_INTERMISSION &&
	     ( pm->cmd.buttons & BUTTON_USE_HOLDABLE ) &&
	     ( ( ammo > 0 || clips > 0 ) || BG_FindInfinteAmmoForWeapon ( pm->ps->weapon ) ) )
	{
		pm->ps->eFlags |= EF_FIRING3;
	}
	else
	{
		pm->ps->eFlags &= ~EF_FIRING3;
	}

	// clear the respawned flag if attack and use are cleared
	if ( pm->ps->stats[ STAT_HEALTH ] > 0 &&
	     ! ( pm->cmd.buttons & ( BUTTON_ATTACK | BUTTON_USE_HOLDABLE ) ) )
	{
		pm->ps->pm_flags &= ~PMF_RESPAWNED;
	}

	// if talk button is down, dissallow all other input
	// this is to prevent any possible intercept proxy from
	// adding fake talk balloons
	if ( pmove->cmd.buttons & BUTTON_TALK )
	{
		pmove->cmd.buttons = BUTTON_TALK;
		pmove->cmd.forwardmove = 0;
		pmove->cmd.rightmove = 0;
		pmove->cmd.upmove = 0;
	}

	// clear all pmove local vars
	memset ( &pml, 0, sizeof ( pml ) );

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
	VectorCopy ( pm->ps->origin, pml.previous_origin );

	// save old velocity for crashlanding
	VectorCopy ( pm->ps->velocity, pml.previous_velocity );

	pml.frametime = pml.msec * 0.001;

	AngleVectors ( pm->ps->viewangles, pml.forward, pml.right, pml.up );

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

	if ( pm->ps->pm_type >= PM_DEAD )
	{
		pm->cmd.forwardmove = 0;
		pm->cmd.rightmove = 0;
		pm->cmd.upmove = 0;
	}

	if ( pm->ps->pm_type == PM_SPECTATOR )
	{
		// update the viewangles
		PM_UpdateViewAngles ( pm->ps, &pm->cmd );
		PM_CheckDuck();
		PM_FlyMove();
		PM_DropTimers();
		return;
	}

	if ( pm->ps->pm_type == PM_NOCLIP )
	{
		PM_UpdateViewAngles ( pm->ps, &pm->cmd );
		PM_NoclipMove();
		PM_DropTimers();
		return;
	}

	if ( pm->ps->pm_type == PM_FREEZE )
	{
		return; // no movement at all
	}

	if ( pm->ps->pm_type == PM_INTERMISSION || pm->ps->pm_type == PM_SPINTERMISSION )
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
	PM_UpdateViewAngles ( pm->ps, &pm->cmd );

	if ( pm->ps->pm_type == PM_DEAD || pm->ps->pm_type == PM_GRABBED )
	{
		PM_DeadMove();
	}

	PM_DropTimers();

	if ( pm->ps->pm_type == PM_JETPACK )
	{
		PM_JetPackMove();
	}
	else if ( pm->ps->pm_flags & PMF_TIME_WATERJUMP )
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
		if ( BG_ClassHasAbility ( pm->ps->stats[ STAT_PCLASS ], SCA_WALLCLIMBER ) &&
		     ( pm->ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
		{
			PM_ClimbMove(); //TA: walking on any surface
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

	PM_Animate();

	// set groundentity, watertype, and waterlevel
	PM_GroundTrace();
	//TA: must update after every GroundTrace() - yet more clock cycles down the drain :( (14 vec rotations/frame)
	// update the viewangles
	PM_UpdateViewAngles ( pm->ps, &pm->cmd );

	PM_SetWaterLevel();

	// weapons
	PM_Weapon();

	// torso animation
	PM_TorsoAnimation();

	// footstep events / legs animations
	PM_Footsteps();

	// entering / leaving water splashes
	PM_WaterEvents();

	// snap some parts of playerstate to save network bandwidth
	trap_SnapVector ( pm->ps->velocity );
}

/*
================
Pmove

Can be called by either the server or the client
================
*/
void Pmove ( pmove_t *pmove )
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
		PmoveSingle ( pmove );

		if ( pmove->ps->pm_flags & PMF_JUMP_HELD )
		{
			pmove->cmd.upmove = 20;
		}
	}
}
