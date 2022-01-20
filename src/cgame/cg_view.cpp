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

// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering

#include "cg_local.h"

/*
=============================================================================

MODEL TESTING

The viewthing and gun positioning tools from Q2 have been integrated and
enhanced into a single model testing facility.

Model viewing can begin with either "testmodel <modelname>" or "testgun <modelname>".

The names must be the full pathname after the basedir, like
"models/weapons/v_launch/tris.md3" or "players/male/tris.md3"

Testmodel will create a fake entity 100 units in front of the current view
position, directly facing the viewer.  It will remain immobile, so you can
move around it to view it from different angles.

Testgun will cause the model to follow the player around and suppress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "cg_gunX", "cg_gunY", and "cg_gunZ" variables
will let you adjust the positioning.

Note that none of the model testing features update while the game is paused, so
it may be convenient to test with deathmatch set to 1 so that bringing down the
console doesn't pause the game.

=============================================================================
*/

/*
=================
CG_TestModel_f

Creates an entity in front of the current position, which
can then be moved around
=================
*/
void CG_TestModel_f()
{
	vec3_t angles;

	cg.testModelEntity.~refEntity_t();
	new(&cg.testModelEntity) refEntity_t{};

	cg.testModelBarrelEntity.~refEntity_t();
	new(&cg.testModelBarrelEntity) refEntity_t{};

	if ( trap_Argc() < 2 )
	{
		return;
	}

	Q_strncpyz( cg.testModelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );

	Q_strncpyz( cg.testModelBarrelName, CG_Argv( 1 ), MAX_QPATH );
	cg.testModelBarrelName[ strlen( cg.testModelBarrelName ) - 4 ] = '\0';
	Q_strcat( cg.testModelBarrelName, MAX_QPATH, "_barrel.md3" );
	cg.testModelBarrelEntity.hModel = trap_R_RegisterModel( cg.testModelBarrelName );

	if ( trap_Argc() == 3 )
	{
		cg.testModelEntity.backlerp = static_cast<float>( atof( CG_Argv( 2 ) ) );
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}

	if ( !cg.testModelEntity.hModel )
	{
		Log::Warn( "Can't register model" );
		return;
	}

	VectorMA( cg.refdef.vieworg, 100, cg.refdef.viewaxis[ 0 ], cg.testModelEntity.origin );

	angles[ PITCH ] = 0;
	angles[ YAW ] = 180 + cg.refdefViewAngles[ 1 ];
	angles[ ROLL ] = 0;

	AnglesToAxis( angles, cg.testModelEntity.axis );
	cg.testGun = false;

	if ( cg.testModelBarrelEntity.hModel )
	{
		angles[ YAW ] = 0;
		angles[ PITCH ] = 0;
		angles[ ROLL ] = 0;
		AnglesToAxis( angles, cg.testModelBarrelEntity.axis );
	}

	cg.testModelEntity.skeleton.scale = 1.0f;
}

/*
=================
CG_TestGun_f

Replaces the current view weapon with the given model
=================
*/
void CG_TestGun_f()
{
	CG_TestModel_f();
	cg.testGun = true;
	cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;
}

void CG_TestModelNextFrame_f()
{
	cg.testModelEntity.frame++;
	Log::Notice( "frame %i", cg.testModelEntity.frame );
}

void CG_TestModelPrevFrame_f()
{
	cg.testModelEntity.frame--;

	if ( cg.testModelEntity.frame < 0 )
	{
		cg.testModelEntity.frame = 0;
	}

	Log::Notice( "frame %i", cg.testModelEntity.frame );
}

void CG_TestModelNextSkin_f()
{
	cg.testModelEntity.skinNum++;
	Log::Notice( "skin %i", cg.testModelEntity.skinNum );
}

void CG_TestModelPrevSkin_f()
{
	cg.testModelEntity.skinNum--;

	if ( cg.testModelEntity.skinNum < 0 )
	{
		cg.testModelEntity.skinNum = 0;
	}

	Log::Notice( "skin %i", cg.testModelEntity.skinNum );
}

static void CG_AddTestModel()
{
	int i;

	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap_R_RegisterModel( cg.testModelName );
	cg.testModelBarrelEntity.hModel = trap_R_RegisterModel( cg.testModelBarrelName );

	if ( !cg.testModelEntity.hModel )
	{
		Log::Warn( "Can't register model" );
		return;
	}

	// if testing a gun, set the origin relative to the view origin
	if ( cg.testGun )
	{
		VectorCopy( cg.refdef.vieworg, cg.testModelEntity.origin );
		VectorCopy( cg.refdef.viewaxis[ 0 ], cg.testModelEntity.axis[ 0 ] );
		VectorCopy( cg.refdef.viewaxis[ 1 ], cg.testModelEntity.axis[ 1 ] );
		VectorCopy( cg.refdef.viewaxis[ 2 ], cg.testModelEntity.axis[ 2 ] );

		// allow the position to be adjusted
		for ( i = 0; i < 3; i++ )
		{
			cg.testModelEntity.origin[ i ] += cg.refdef.viewaxis[ 0 ][ i ] * cg_gun_x.Get();
			cg.testModelEntity.origin[ i ] += cg.refdef.viewaxis[ 1 ][ i ] * cg_gun_y.Get();
			cg.testModelEntity.origin[ i ] += cg.refdef.viewaxis[ 2 ][ i ] * cg_gun_z.Get();
		}
	}

	trap_R_AddRefEntityToScene( &cg.testModelEntity );

	if ( cg.testModelBarrelEntity.hModel )
	{
		CG_PositionEntityOnTag( &cg.testModelBarrelEntity, &cg.testModelEntity,
		                        cg.testModelEntity.hModel, "tag_barrel" );

		trap_R_AddRefEntityToScene( &cg.testModelBarrelEntity );
	}
}

//============================================================================

/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/
static void CG_CalcVrect()
{
	int size;

	// the intermission should always be in fullscreen
	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		size = 100;
	}
	else
	{
		size = cg_viewsize.Get();
	}

	cg.refdef.width = cgs.glconfig.vidWidth * size / 100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight * size / 100;
	cg.refdef.height &= ~1;

	cg.refdef.x = ( cgs.glconfig.vidWidth - cg.refdef.width ) / 2;
	cg.refdef.y = ( cgs.glconfig.vidHeight - cg.refdef.height ) / 2;
}

//==============================================================================

/*
===============
CG_OffsetThirdPersonView

===============
*/
void CG_OffsetThirdPersonView()
{
	int           i;
	vec3_t        forward, right, up;
	vec3_t        view;
	trace_t       trace;
	static vec3_t mins = { -8, -8, -8 };
	static vec3_t maxs = { 8, 8, 8 };
	vec3_t        focusPoint;
	vec3_t        surfNormal;
	int           cmdNum;
	usercmd_t     cmd, oldCmd;
	float         range;
	vec3_t        mouseInputAngles;
	vec3_t        rotationAngles;
	vec3_t        axis[ 3 ], rotaxis[ 3 ];
	float         deltaPitch;
	static float  pitch;
	static vec3_t killerPos = { 0, 0, 0 };

	// If cg_thirdpersonShoulderViewMode == 2, do shoulder view instead
	// If cg_thirdpersonShoulderViewMode == 1, do shoulder view when chasing
	//   a wallwalker because it's really erratic to watch
	if ( cg_thirdPersonShoulderViewMode.Get() == 2 )
	{
		CG_OffsetShoulderView();
		return;
	}

	BG_GetClientNormal( &cg.predictedPlayerState, surfNormal );
	// Set the view origin to the class's view height
	VectorMA( cg.refdef.vieworg, cg.predictedPlayerState.viewheight, surfNormal, cg.refdef.vieworg );

	// Set the focus point where the camera will look (at the player's vieworg)
	VectorCopy( cg.refdef.vieworg, focusPoint );

	bool noclip = cg.predictedPlayerState.pm_type == PM_NOCLIP;
	bool alive = cg.predictedPlayerState.stats[ STAT_HEALTH ] > 0;
	bool spectate = cg.predictedPlayerState.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT;
	// pretend the spectator is alive
	alive |= spectate;

	// If player is dead, we want the player to be between us and the killer
	// so pretend that the player was looking at the killer, then place cam behind them.
	if ( !alive )
	{
		int killerEntNum = cg.predictedPlayerState.stats[ STAT_VIEWLOCK ];

		// already looking at ourself
		if ( killerEntNum != cg.snap->ps.clientNum )
		{
			vec3_t lookDirection;

			if ( !cg.wasDeadLastFrame || !cg_staticDeathCam.Get() )
			{
				VectorCopy( cg_entities[ killerEntNum ].lerpOrigin, killerPos );
				cg.wasDeadLastFrame = true;
			}

			VectorSubtract( killerPos, cg.refdef.vieworg, lookDirection );
			vectoangles( lookDirection, cg.refdefViewAngles );
		}
	}

	// get cg_thirdPersonRange
	range = cg_thirdPersonRange.Get();

	// Calculate the angle of the camera's position around the player.
	// Unless in demo, PLAYING in third person, or in dead-third-person cam, allow the player
	// to control camera position offsets using the mouse position.
	if ( cg.demoPlayback || ( ( cg.snap->ps.pm_flags & PMF_FOLLOW ) && alive ) )
	{
		// Collect our input values from the mouse.
		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd( cmdNum, &cmd );
		trap_GetUserCmd( cmdNum - 1, &oldCmd );

		// Prevent pitch from wrapping and clamp it within a [-75, 90] range.
		// Cgame has no access to ps.delta_angles[] here, so we need to reproduce
		// it ourselves.
		deltaPitch = SHORT2ANGLE( cmd.angles[ PITCH ] - oldCmd.angles[ PITCH ] );

		if ( fabs( deltaPitch ) < 200.0f )
		{
			pitch += deltaPitch;
		}

		mouseInputAngles[ PITCH ] = pitch;
		mouseInputAngles[ YAW ] = -1.0f * SHORT2ANGLE( cmd.angles[ YAW ] );  // yaw is inverted
		mouseInputAngles[ ROLL ] = 0.0f;

		for ( i = 0; i < 3; i++ )
		{
			mouseInputAngles[ i ] = AngleNormalize180( mouseInputAngles[ i ] );
		}

		// Set the rotation angles to be the view angles offset by the mouse input
		// Ignore the original pitch though; it's too jerky otherwise
		if ( !cg_thirdPersonPitchFollow.Get() )
		{
			cg.refdefViewAngles[ PITCH ] = 0.0f;
		}

		for ( i = 0; i < 3; i++ )
		{
			rotationAngles[ i ] = AngleNormalize180( cg.refdefViewAngles[ i ] ) + mouseInputAngles[ i ];
			AngleNormalize180( rotationAngles[ i ] );
		}

		// Don't let pitch go too high/too low or the camera flips around and
		// that's really annoying.
		// However, when we're not on the floor or ceiling (wallwalk) pitch
		// may not be pitch, so just let it go.
		if ( surfNormal[ 2 ] > 0.5f || surfNormal[ 2 ] < -0.5f )
		{
			rotationAngles[ PITCH ] = Math::Clamp( rotationAngles[ PITCH ], -85.0f, 85.0f );
		}

		// Perform the rotations specified by rotationAngles.
		AnglesToAxis( rotationAngles, axis );

		if ( !( cg.snap->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING ) ||
		     !BG_RotateAxis( cg.snap->ps.grapplePoint, axis, rotaxis, false,
		                     cg.snap->ps.eFlags & EF_WALLCLIMBCEILING ) )
		{
			AxisCopy( axis, rotaxis );
		}

		// Convert the new axis back to angles.
		AxisToAngles( rotaxis, rotationAngles );
	}
	else
	{
		if ( alive )
		{
			// If we're playing the game in third person, the viewangles already
			// take care of our mouselook, so just use them.
			for ( i = 0; i < 3; i++ )
			{
				rotationAngles[ i ] = cg.refdefViewAngles[ i ];
			}
		}
		else // dead
		{
			rotationAngles[ PITCH ] = 20.0f;
			rotationAngles[ YAW ] = cg.refdefViewAngles[ YAW ];
		}
	}

	rotationAngles[ YAW ] -= cg_thirdPersonAngle.Get();

	// Move the camera range distance back.
	AngleVectors( rotationAngles, forward, right, up );
	VectorCopy( cg.refdef.vieworg, view );
	VectorMA( view, -range, forward, view );

	// Ensure that the current camera position isn't out of bounds and that there
	// is nothing between the camera and the player.

	if ( !noclip )
	{
		// Trace a ray from the origin to the viewpoint to make sure the view isn't
		// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything
		CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID, 0 );

		if ( trace.fraction != 1.0f )
		{
			VectorCopy( trace.endpos, view );
			view[ 2 ] += ( 1.0f - trace.fraction ) * 32;
			// Try another trace to this position, because a tunnel may have the ceiling
			// close enough that this is poking out.
			CG_Trace( &trace, cg.refdef.vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID, 0 );
			VectorCopy( trace.endpos, view );
		}
	}

	// Set the camera position to what we calculated.
	VectorCopy( view, cg.refdef.vieworg );

	// The above checks may have moved the camera such that the existing viewangles
	// may not still face the player. Recalculate them to do so.
	// but if we're dead, don't bother because we'd rather see what killed us
	if ( alive )
	{
		VectorSubtract( focusPoint, cg.refdef.vieworg, focusPoint );
		vectoangles( focusPoint, cg.refdefViewAngles );
	}
}

/*
===============
CG_OffsetShoulderView

===============
*/
void CG_OffsetShoulderView()
{
	int          i;
	int          cmdNum;
	usercmd_t    cmd, oldCmd;
	vec3_t       rotationAngles;
	vec3_t       axis[ 3 ], rotaxis[ 3 ];
	float        deltaMousePitch;
	static float mousePitch;
	vec3_t       forward, right, up;
	classModelConfig_t *classModelConfig;

	// Ignore following pitch; it's too jerky otherwise.
	if ( !cg_thirdPersonPitchFollow.Get() )
	{
		cg.refdefViewAngles[ PITCH ] = 0.0f;
	}

	AngleVectors( cg.refdefViewAngles, forward, right, up );

	classModelConfig = BG_ClassModelConfig( cg.snap->ps.stats[ STAT_CLASS ] );
	VectorMA( cg.refdef.vieworg, classModelConfig->shoulderOffsets[ 0 ], forward, cg.refdef.vieworg );
	VectorMA( cg.refdef.vieworg, classModelConfig->shoulderOffsets[ 1 ], right, cg.refdef.vieworg );
	VectorMA( cg.refdef.vieworg, classModelConfig->shoulderOffsets[ 2 ], up, cg.refdef.vieworg );

	// If someone is playing like this, the rest is already taken care of
	// so just get the firstperson effects and leave.
	if ( !cg.demoPlayback && !( cg.snap->ps.pm_flags & PMF_FOLLOW ) )
	{
		CG_OffsetFirstPersonView();
		return;
	}

	// Get mouse input for camera rotation.
	cmdNum = trap_GetCurrentCmdNumber();
	trap_GetUserCmd( cmdNum, &cmd );
	trap_GetUserCmd( cmdNum - 1, &oldCmd );

	// Prevent pitch from wrapping and clamp it within a [30, -50] range.
	// Cgame has no access to ps.delta_angles[] here, so we need to reproduce
	// it ourselves here.
	deltaMousePitch = SHORT2ANGLE( cmd.angles[ PITCH ] - oldCmd.angles[ PITCH ] );

	if ( fabs( deltaMousePitch ) < 200.0f )
	{
		mousePitch += deltaMousePitch;
	}

	// Handle pitch.
	rotationAngles[ PITCH ] = mousePitch;
	rotationAngles[ PITCH ] = AngleNormalize180( rotationAngles[ PITCH ] + AngleNormalize180( cg.refdefViewAngles[ PITCH ] ) );
	rotationAngles[ PITCH ] = Math::Clamp( rotationAngles[ PITCH ], -90.0f, 90.0f );

	// Yaw and Roll are much easier.
	rotationAngles[ YAW ] = SHORT2ANGLE( cmd.angles[ YAW ] ) + cg.refdefViewAngles[ YAW ];
	rotationAngles[ ROLL ] = 0.0f;

	// Perform the rotations.
	AnglesToAxis( rotationAngles, axis );

	if ( !( cg.snap->ps.stats[ STAT_STATE ] & SS_WALLCLIMBING ) ||
	     !BG_RotateAxis( cg.snap->ps.grapplePoint, axis, rotaxis, false,
	                     cg.snap->ps.eFlags & EF_WALLCLIMBCEILING ) )
	{
		AxisCopy( axis, rotaxis );
	}

	AxisToAngles( rotaxis, rotationAngles );

	// Actually set the viewangles.
	for ( i = 0; i < 3; i++ )
	{
		cg.refdefViewAngles[ i ] = rotationAngles[ i ];
	}

	// Now run the first person stuff so we get various effects added.
	CG_OffsetFirstPersonView();
}

// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset()
{
	float         steptime;
	int           timeDelta;
	vec3_t        normal;
	playerState_t *ps = &cg.predictedPlayerState;

	BG_GetClientNormal( ps, normal );

	steptime = BG_Class( ps->stats[ STAT_CLASS ] )->steptime;

	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;

	if ( timeDelta < steptime )
	{
		float stepChange = cg.stepChange
		                   * ( steptime - timeDelta ) / steptime;

		VectorMA( cg.refdef.vieworg, -stepChange, normal, cg.refdef.vieworg );
	}
}

/*
===============
CG_OffsetFirstPersonView

===============
*/
void CG_OffsetFirstPersonView()
{
	float         *origin;
	float         *angles;
	float         bob;
	float         ratio;
	float         delta;
	float         speed;
	float         f;
	vec3_t        predictedVelocity;
	int           timeDelta;
	float         bob2;
	vec3_t        normal, baseOrigin;
	playerState_t *ps = &cg.predictedPlayerState;

	BG_GetClientNormal( ps, normal );

	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		return;
	}

	origin = cg.refdef.vieworg;
	angles = cg.refdefViewAngles;

	VectorCopy( origin, baseOrigin );

	bool alive = ps->stats[ STAT_HEALTH ] > 0;
	bool spectate = ps->persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT;
	// pretend the spectator is alive
	alive |= spectate;

	// if dead, fix the angle and don't add any kick
	if ( !alive )
	{
		angles[ ROLL ] = 40;
		angles[ PITCH ] = -15;
		angles[ YAW ] = cg.snap->ps.stats[ STAT_VIEWLOCK ];
		origin[ 2 ] += cg.predictedPlayerState.viewheight;
		return;
	}

	// add angles based on damage kick
	if ( cg.damageTime )
	{
		ratio = cg.time - cg.damageTime;

		if ( ratio < DAMAGE_DEFLECT_TIME )
		{
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[ PITCH ] += ratio * cg.v_dmg_pitch;
			angles[ ROLL ] += ratio * cg.v_dmg_roll;
		}
		else
		{
			ratio = 1.0 - ( ratio - DAMAGE_DEFLECT_TIME ) / DAMAGE_RETURN_TIME;

			if ( ratio > 0 )
			{
				angles[ PITCH ] += ratio * cg.v_dmg_pitch;
				angles[ ROLL ] += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = ( cg.time - cg.landTime ) / FALL_TIME;

	if ( ratio < 0 )
	{
		ratio = 0;
	}

	angles[ PITCH ] += ratio * cg.fall_value;
#endif

	// add angles based on velocity
	VectorCopy( cg.predictedPlayerState.velocity, predictedVelocity );

	delta = DotProduct( predictedVelocity, cg.refdef.viewaxis[ 0 ] );
	angles[ PITCH ] += delta * cg_runpitch.Get();

	delta = DotProduct( predictedVelocity, cg.refdef.viewaxis[ 1 ] );
	angles[ ROLL ] -= delta * cg_runroll.Get();

	// add angles based on bob
	// bob amount is class-dependent

	if ( cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
	{
		bob2 = 0.0f;
	}
	else
	{
		bob2 = BG_Class( cg.predictedPlayerState.stats[ STAT_CLASS ] )->bob;
	}

#define LEVEL4_FEEDBACK 10.0f

	//give a charging player some feedback
	if ( ps->weapon == WP_ALEVEL4 )
	{
		if ( ps->weaponCharge > 0 )
		{
			float fraction = ( float ) ps->weaponCharge /
			                 LEVEL4_TRAMPLE_CHARGE_MAX;

			if ( fraction > 1.0f )
			{
				fraction = 1.0f;
			}

			bob2 *= ( 1.0f + fraction * LEVEL4_FEEDBACK );
		}
	}

	if ( bob2 != 0.0f )
	{
		// make sure the bob is visible even at low speeds
		speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

		delta = cg.bobfracsin * ( bob2 ) * speed;

		if ( cg.predictedPlayerState.pm_flags & PMF_DUCKED )
		{
			delta *= 3; // crouching
		}

		angles[ PITCH ] += delta;
		delta = cg.bobfracsin * ( bob2 ) * speed;

		if ( cg.predictedPlayerState.pm_flags & PMF_DUCKED )
		{
			delta *= 3; // crouching accentuates roll
		}

		if ( cg.bobcycle & 1 )
		{
			delta = -delta;
		}

		angles[ ROLL ] += delta;
	}

#define LEVEL3_FEEDBACK 20.0f

	//provide some feedback for pouncing
	if ( ( cg.predictedPlayerState.weapon == WP_ALEVEL3 ||
	       cg.predictedPlayerState.weapon == WP_ALEVEL3_UPG ) &&
	     cg.predictedPlayerState.weaponCharge > 0 )
	{
		float  fraction1, fraction2;
		vec3_t forward;

		AngleVectors( angles, forward, nullptr, nullptr );
		VectorNormalize( forward );

		fraction1 = ( float ) cg.predictedPlayerState.weaponCharge /
		            LEVEL3_POUNCE_TIME_UPG;

		if ( fraction1 > 1.0f )
		{
			fraction1 = 1.0f;
		}

		fraction2 = -sinf( fraction1 * M_PI_2 );

		VectorMA( origin, LEVEL3_FEEDBACK * fraction2, forward, origin );
	}

#define STRUGGLE_DIST 5.0f
#define STRUGGLE_TIME 250

	//allow the player to struggle a little whilst grabbed
	if ( cg.predictedPlayerState.pm_type == PM_GRABBED )
	{
		vec3_t    forward, right, up;
		usercmd_t cmd;
		int       cmdNum;
		float     fFraction, rFraction, uFraction;

		cmdNum = trap_GetCurrentCmdNumber();
		trap_GetUserCmd( cmdNum, &cmd );

		AngleVectors( angles, forward, right, up );

		fFraction = ( float )( cg.time - cg.forwardMoveTime ) / STRUGGLE_TIME;
		rFraction = ( float )( cg.time - cg.rightMoveTime ) / STRUGGLE_TIME;
		uFraction = ( float )( cg.time - cg.upMoveTime ) / STRUGGLE_TIME;

		if ( fFraction > 1.0f )
		{
			fFraction = 1.0f;
		}

		if ( rFraction > 1.0f )
		{
			rFraction = 1.0f;
		}

		if ( uFraction > 1.0f )
		{
			uFraction = 1.0f;
		}

		if ( cmd.forwardmove > 0 )
		{
			VectorMA( origin, STRUGGLE_DIST * fFraction, forward, origin );
		}
		else if ( cmd.forwardmove < 0 )
		{
			VectorMA( origin, -STRUGGLE_DIST * fFraction, forward, origin );
		}
		else
		{
			cg.forwardMoveTime = cg.time;
		}

		if ( cmd.rightmove > 0 )
		{
			VectorMA( origin, STRUGGLE_DIST * rFraction, right, origin );
		}
		else if ( cmd.rightmove < 0 )
		{
			VectorMA( origin, -STRUGGLE_DIST * rFraction, right, origin );
		}
		else
		{
			cg.rightMoveTime = cg.time;
		}

		if ( cmd.upmove > 0 )
		{
			VectorMA( origin, STRUGGLE_DIST * uFraction, up, origin );
		}
		else if ( cmd.upmove < 0 )
		{
			VectorMA( origin, -STRUGGLE_DIST * uFraction, up, origin );
		}
		else
		{
			cg.upMoveTime = cg.time;
		}
	}

	// this *feels* more realistic for humans <- this comment feels very descriptive
	if ( CG_MyTeam() == TEAM_HUMANS &&
	     cg.predictedPlayerState.pm_type == PM_NORMAL )
	{
		angles[ PITCH ] += cg.bobfracsin * bob2 * 0.5;
	}

	// add view height
	VectorMA( origin, ps->viewheight, normal, origin );

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;

	if ( timeDelta < DUCK_TIME )
	{
		cg.refdef.vieworg[ 2 ] -= cg.duckChange
		                          * ( DUCK_TIME - timeDelta ) / DUCK_TIME;
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * bob2;

	if ( bob > 6 )
	{
		bob = 6;
	}

	VectorMA( origin, bob, normal, origin );

	// add fall height
	delta = cg.time - cg.landTime;

	if ( delta < LAND_DEFLECT_TIME )
	{
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef.vieworg[ 2 ] += cg.landChange * f;
	}
	else if ( delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME )
	{
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - ( delta / LAND_RETURN_TIME );
		cg.refdef.vieworg[ 2 ] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();
}

//======================================================================

/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
#define WAVE_AMPLITUDE 1.0f
#define WAVE_FREQUENCY 0.4f

#define FOVWARPTIME    400.0f
#define BASE_FOV_Y     73.739792f // atan2( 3, 4 / tan( 90 ) )
#define MAX_FOV_Y      120.0f
#define MAX_FOV_WARP_Y 127.5f

static int CG_CalcFov()
{
	float     y;
	float     phase;
	float     v;
	int       contents;
	float     fov_x, fov_y;
	float     zoomFov;
	float     f;
	int       inwater;
	float     attribFov;
	usercmd_t cmd;
	usercmd_t oldcmd;
	int       cmdNum;

	cmdNum = trap_GetCurrentCmdNumber();
	trap_GetUserCmd( cmdNum, &cmd );
	trap_GetUserCmd( cmdNum - 1, &oldcmd );

	// Cycle between follow and third-person follow modes on mouse middle click.
	if ( usercmdButtonPressed( cmd.buttons, BTN_ATTACK3 ) && !usercmdButtonPressed( oldcmd.buttons, BTN_ATTACK3 ) )
	{
		if ( cg.snap->ps.pm_flags & PMF_FOLLOW )
		{
			if ( !cg.chaseFollow )
			{
				cg.chaseFollow = true;
			}
			else
			{
				cg.chaseFollow = false;
			}
		}
	}

	// Start and stoop to follow on mouse right click.
	if ( usercmdButtonPressed( cmd.buttons, BTN_ATTACK2 ) && !usercmdButtonPressed( oldcmd.buttons, BTN_ATTACK2 ) )
	{
		if ( ( cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT )
			|| ( cg.snap->ps.pm_flags & PMF_FOLLOW ) )
		{
			trap_SendClientCommand( "follow\n" );
		}
	}

	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION ||
	     ( cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT ) ||
	     ( cg.renderingThirdPerson ) )
	{
		// if in intermission or third person, use a fixed value
		fov_y = BASE_FOV_Y;
	}
	else
	{
		// don't lock the fov globally - we need to be able to change it
		if ( ( attribFov = trap_Cvar_VariableValue( BG_Class( cg.predictedPlayerState.stats[ STAT_CLASS ] )->fovCvar ) ) != 0.0f )
		{
			attribFov = Math::Clamp( attribFov, 80.0f, 140.0f );
		}
		else
		{
			attribFov = BG_Class( cg.predictedPlayerState.stats[ STAT_CLASS ] )->fov;
		}
		attribFov *= 0.75;
		fov_y = Math::Clamp( attribFov, 1.0f, MAX_FOV_Y );

		if ( cg.spawnTime > ( cg.time - FOVWARPTIME ) &&
		     BG_ClassHasAbility( cg.predictedPlayerState.stats[ STAT_CLASS ], SCA_FOVWARPS ) )
		{
			float fraction = ( float )( cg.time - cg.spawnTime ) / FOVWARPTIME;

			fov_y = MAX_FOV_WARP_Y - ( ( MAX_FOV_WARP_Y - fov_y ) * fraction );
		}

		// account for zooms
		zoomFov = BG_Weapon( cg.predictedPlayerState.weapon )->zoomFov * 0.75f;
		zoomFov = Math::Clamp( zoomFov, 1.0f, attribFov );

		// only do all the zoom stuff if the client CAN zoom
		// FIXME: zoom control is currently hard coded to WBTN_ATTACK2
		if ( BG_Weapon( cg.predictedPlayerState.weapon )->canZoom )
		{
			if ( cg.zoomed )
			{
				f = ( cg.time - cg.zoomTime ) / ( float ) ZOOM_TIME;

				if ( f > 1.0f )
				{
					fov_y = zoomFov;
				}
				else
				{
					fov_y = fov_y + f * ( zoomFov - fov_y );
				}

				// WBTN_ATTACK2 isn't held so unzoom next time
				if ( !usercmdButtonPressed( cmd.buttons, BTN_ATTACK2 ) || cg.snap->ps.weaponstate == WEAPON_RELOADING || cgs.clientinfo[ cg.clientNum ].team == TEAM_NONE )
				{
					cg.zoomed = false;
					cg.zoomTime = std::min( cg.time,
					                   cg.time + cg.time - cg.zoomTime - ZOOM_TIME );
				}
			}
			else
			{
				f = ( cg.time - cg.zoomTime ) / ( float ) ZOOM_TIME;

				if ( f < 1.0f )
				{
					fov_y = zoomFov + f * ( fov_y - zoomFov );
				}

				// WBTN_ATTACK2 is held so zoom next time.
				//
				// We check if the player is really part of a team,
				// because a specator may be following a player that
				// has the scoped gun. This avoids https://github.com/Unvanquished/Unvanquished/issues/1399
				// If a spectator is following someone, then
				// cg.predictedPlayerState.weapon is the weapon of
				// the followed player.
				if ( usercmdButtonPressed( cmd.buttons, BTN_ATTACK2 ) && cg.snap->ps.weaponstate != WEAPON_RELOADING && cgs.clientinfo[ cg.clientNum ].team != TEAM_NONE )
				{
					cg.zoomed = true;
					cg.zoomTime = std::min( cg.time,
					                   cg.time + cg.time - cg.zoomTime - ZOOM_TIME );
				}
			}
		}
		else if ( cg.zoomed )
		{
			cg.zoomed = false;
		}
	}

	y = cg.refdef.height / tanf( 0.5f * DEG2RAD( fov_y ) );
	fov_x = atan2f( cg.refdef.width, y );
	fov_x = 2.0f * RAD2DEG( fov_x );

	// warp if underwater
	contents = CG_PointContents( cg.refdef.vieworg, -1 );

	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) )
	{
		phase = cg.time / 1000.0f * WAVE_FREQUENCY * M_PI * 2.0f;
		v = WAVE_AMPLITUDE * sinf( phase );
		fov_x += v;
		fov_y -= v;
		inwater = true;
	}
	else
	{
		inwater = false;
	}

	// set it
	cg.refdef.fov_x = fov_x;
	cg.refdef.fov_y = fov_y;

	if ( !cg.zoomed )
	{
		cg.zoomSensitivity = 1.0f;
	}
	else
	{
		cg.zoomSensitivity = cg.refdef.fov_y / 75.0f;
	}

	return inwater;
}

#define NORMAL_HEIGHT 64.0f
#define NORMAL_WIDTH  6.0f

/*
===============
CG_DrawSurfNormal

Draws a vector against
the surface player is looking at
===============
*/
static void CG_DrawSurfNormal()
{
	trace_t    tr;
	vec3_t     end, temp;
	polyVert_t normal[ 4 ];
	vec4_t     color = { 0.0f, 255.0f, 0.0f, 128.0f };

	VectorMA( cg.refdef.vieworg, 8192, cg.refdef.viewaxis[ 0 ], end );

	CG_Trace( &tr, cg.refdef.vieworg, nullptr, nullptr, end, cg.predictedPlayerState.clientNum,
	          MASK_SOLID, 0 );

	VectorCopy( tr.endpos, normal[ 0 ].xyz );
	normal[ 0 ].st[ 0 ] = 0;
	normal[ 0 ].st[ 1 ] = 0;
	Vector4Copy( color, normal[ 0 ].modulate );

	VectorMA( tr.endpos, NORMAL_WIDTH, cg.refdef.viewaxis[ 1 ], temp );
	VectorCopy( temp, normal[ 1 ].xyz );
	normal[ 1 ].st[ 0 ] = 0;
	normal[ 1 ].st[ 1 ] = 1;
	Vector4Copy( color, normal[ 1 ].modulate );

	VectorMA( tr.endpos, NORMAL_HEIGHT, tr.plane.normal, temp );
	VectorMA( temp, NORMAL_WIDTH, cg.refdef.viewaxis[ 1 ], temp );
	VectorCopy( temp, normal[ 2 ].xyz );
	normal[ 2 ].st[ 0 ] = 1;
	normal[ 2 ].st[ 1 ] = 1;
	Vector4Copy( color, normal[ 2 ].modulate );

	VectorMA( tr.endpos, NORMAL_HEIGHT, tr.plane.normal, temp );
	VectorCopy( temp, normal[ 3 ].xyz );
	normal[ 3 ].st[ 0 ] = 1;
	normal[ 3 ].st[ 1 ] = 0;
	Vector4Copy( color, normal[ 3 ].modulate );

	trap_R_AddPolyToScene( cgs.media.outlineShader, 4, normal );
}

/*
===============
CG_addSmoothOp
===============
*/
void CG_addSmoothOp( vec3_t rotAxis, float rotAngle, float timeMod )
{
	int i;

	//iterate through smooth array
	for ( i = 0; i < MAXSMOOTHS; i++ )
	{
		//found an unused index in the smooth array
		if ( cg.sList[ i ].time + cg_wwSmoothTime.Get() < cg.time )
		{
			//copy to array and stop
			VectorCopy( rotAxis, cg.sList[ i ].rotAxis );
			cg.sList[ i ].rotAngle = rotAngle;
			cg.sList[ i ].time = cg.time;
			cg.sList[ i ].timeMod = timeMod;
			return;
		}
	}

	//no free indices in the smooth array
}

/*
===============
CG_smoothWWTransitions
===============
*/
static void CG_smoothWWTransitions( playerState_t *ps, const vec3_t in, vec3_t out )
{
	vec3_t   surfNormal, rotAxis, temp;
	int      i;
	float    stLocal, sFraction, rotAngle;
	float    smoothTime, timeMod;
	bool performed = false;
	vec3_t   inAxis[ 3 ], lastAxis[ 3 ], outAxis[ 3 ];

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW )
	{
		VectorCopy( in, out );
		return;
	}

	//set surfNormal
	BG_GetClientNormal( ps, surfNormal );

	AnglesToAxis( in, inAxis );

	//if we are moving from one surface to another smooth the transition
	if( !VectorCompareEpsilon( surfNormal, cg.lastNormal, 0.01f ) )
	{
		AnglesToAxis( cg.lastVangles, lastAxis );

		rotAngle = DotProduct( inAxis[ 0 ], lastAxis[ 0 ] ) +
		           DotProduct( inAxis[ 1 ], lastAxis[ 1 ] ) +
		           DotProduct( inAxis[ 2 ], lastAxis[ 2 ] );

		// if inAxis and lastAxis collinear, prevent NaN on acos( -1 )
		if ( rotAngle < -0.9999f )
		{
			rotAngle = 180.0f;
		}
		else
		{
			rotAngle = RAD2DEG( acosf( ( rotAngle - 1.0f ) / 2.0f ) );
		}

		CrossProduct( lastAxis[ 0 ], inAxis[ 0 ], temp );
		VectorCopy( temp, rotAxis );
		CrossProduct( lastAxis[ 1 ], inAxis[ 1 ], temp );
		VectorAdd( rotAxis, temp, rotAxis );
		CrossProduct( lastAxis[ 2 ], inAxis[ 2 ], temp );
		VectorAdd( rotAxis, temp, rotAxis );

		VectorNormalize( rotAxis );

		timeMod = 1.0f;

		//add the op
		CG_addSmoothOp( rotAxis, rotAngle, timeMod );
	}

	//iterate through ops
	for ( i = MAXSMOOTHS - 1; i >= 0; i-- )
	{
		smoothTime = ( int )( cg_wwSmoothTime.Get() * cg.sList[ i ].timeMod );

		//if this op has time remaining, perform it
		if ( cg.time < cg.sList[ i ].time + smoothTime )
		{
			stLocal = 1.0f - ( ( ( cg.sList[ i ].time + smoothTime ) - cg.time ) / smoothTime );
			sFraction = - ( cosf( stLocal * M_PI ) + 1.0f ) / 2.0f;

			RotatePointAroundVector( outAxis[ 0 ], cg.sList[ i ].rotAxis,
			                         inAxis[ 0 ], sFraction * cg.sList[ i ].rotAngle );
			RotatePointAroundVector( outAxis[ 1 ], cg.sList[ i ].rotAxis,
			                         inAxis[ 1 ], sFraction * cg.sList[ i ].rotAngle );
			RotatePointAroundVector( outAxis[ 2 ], cg.sList[ i ].rotAxis,
			                         inAxis[ 2 ], sFraction * cg.sList[ i ].rotAngle );

			AxisCopy( outAxis, inAxis );
			performed = true;
		}
	}

	//if we performed any ops then return the smoothed angles
	//otherwise simply return the in angles
	if ( performed )
	{
		AxisToAngles( outAxis, out );
	}
	else
	{
		VectorCopy( in, out );
	}

	//copy the current normal to the lastNormal
	VectorCopy( in, cg.lastVangles );
	VectorCopy( surfNormal, cg.lastNormal );
}

/*
===============
CG_smoothWJTransitions
===============
*/
static void CG_smoothWJTransitions( playerState_t *ps, const vec3_t in, vec3_t out )
{
	int      i;
	float    stLocal, sFraction;
	bool performed = false;
	vec3_t   inAxis[ 3 ], outAxis[ 3 ];

	Q_UNUSED(ps);

	if ( cg.snap->ps.pm_flags & PMF_FOLLOW )
	{
		VectorCopy( in, out );
		return;
	}

	AnglesToAxis( in, inAxis );

	//iterate through ops
	for ( i = MAXSMOOTHS - 1; i >= 0; i-- )
	{
		//if this op has time remaining, perform it
		if ( cg.time < cg.sList[ i ].time + cg_wwSmoothTime.Get() )
		{
			stLocal = ( ( cg.sList[ i ].time + cg_wwSmoothTime.Get() ) - cg.time ) / cg_wwSmoothTime.Get();
			sFraction = 1.0f - ( ( cosf( stLocal * M_PI * 2.0f ) + 1.0f ) / 2.0f );

			RotatePointAroundVector( outAxis[ 0 ], cg.sList[ i ].rotAxis,
			                         inAxis[ 0 ], sFraction * cg.sList[ i ].rotAngle );
			RotatePointAroundVector( outAxis[ 1 ], cg.sList[ i ].rotAxis,
			                         inAxis[ 1 ], sFraction * cg.sList[ i ].rotAngle );
			RotatePointAroundVector( outAxis[ 2 ], cg.sList[ i ].rotAxis,
			                         inAxis[ 2 ], sFraction * cg.sList[ i ].rotAngle );

			AxisCopy( outAxis, inAxis );
			performed = true;
		}
	}

	//if we performed any ops then return the smoothed angles
	//otherwise simply return the in angles
	if ( performed )
	{
		AxisToAngles( outAxis, out );
	}
	else
	{
		VectorCopy( in, out );
	}
}

/*
===============
CG_CalcColorGradingForPoint

Sets cg.refdef.gradingWeights
===============
*/
static void CG_CalcColorGradingForPoint( vec3_t loc )
{
	int   i, j;
	float dist, weight;
	int   selectedIdx[3] = { 0, 0, 0 };
	float selectedWeight[3] = { 0.0f, 0.0f, 0.0f };
	float totalWeight = 0.0f;
	int freeSlot = -1;
	bool haveGlobal = false;

	// the first allocated grading is special in that it may be global
	i = 0;

	if ( cgs.gameGradingTextures[0] && cgs.gameGradingModels[0] == -1 )
	{
		selectedIdx[0] = 0; // shouldn't be needed
		selectedWeight[0] = 2.0f; // won't be sorted down
		haveGlobal = true;
		i = 1;
	}

	for(; i < MAX_GRADING_TEXTURES; i++ )
	{
		if( !cgs.gameGradingTextures[i] )
		{
			continue;
		}

		dist = trap_CM_DistanceToModel( loc, cgs.gameGradingModels[i] );
		weight = 1.0f - dist / cgs.gameGradingDistances[i];
		weight = Math::Clamp( weight, 0.0f, 1.0f ); // Maths::clampFraction( weight )

		// search 3 greatest weights
		if( weight <= selectedWeight[2] )
		{
			continue;
		}

		for( j = 1; j >= 0; j-- )
		{
			if( weight <= selectedWeight[j] )
			{
				break;
			}

			selectedIdx[j+1] = selectedIdx[j];
			selectedWeight[j+1] = selectedWeight[j];
		}

		selectedIdx[j+1] = i;
		selectedWeight[j+1] = weight;
	}

	i = 0;

	if( haveGlobal )
	{
		trap_SetColorGrading( 1, cgs.gameGradingTextures[0] );
		i = 1;
	}

	for(; i < 3; i++ )
	{
		if( selectedWeight[i] > 0.0f )
		{
			trap_SetColorGrading( i + 1, cgs.gameGradingTextures[selectedIdx[i]] );
			totalWeight += selectedWeight[i];
		}
		else
		{
			freeSlot = i;
		}
	}

	if( !haveGlobal && totalWeight < 1.0f )
	{
		if(freeSlot >= 0)
		{
			//If there is a free slot, use it with the neutral cgrade
			//to make sure that using only the 3 map grade will always be ok
			trap_SetColorGrading( freeSlot + 1, cgs.media.neutralCgrade);
			selectedWeight[freeSlot] = 1.0f - totalWeight;
			totalWeight = 1.0f;
		}
	}

	cg.refdef.gradingWeights[0] = 0.0f;
	cg.refdef.gradingWeights[1] = haveGlobal ? ( 1.0f - totalWeight ) : ( selectedWeight[0] / totalWeight );
	cg.refdef.gradingWeights[2] = totalWeight == 0.0f ? 0.0f : selectedWeight[1] / totalWeight;
	cg.refdef.gradingWeights[3] = totalWeight == 0.0f ? 0.0f : selectedWeight[2] / totalWeight;
}

static void CG_ChooseCgradingEffectAndFade( const playerState_t* ps, qhandle_t* effect, float* fade, float* fadeRate )
{
	int health = ps->stats[ STAT_HEALTH ];
	int team = ps->persistant[ PERS_TEAM ];
	bool playing = team == TEAM_HUMANS || team == TEAM_ALIENS;

	//the player has spawned once and is dead or in the intermission
	if ( cg_spawnEffects.Get() && playing && ( health <= 0 || cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT) )
	{
		*effect = cgs.media.desaturatedCgrade;
		*fade = 1.0;
		*fadeRate = 0.004;
	}
	//no other effects for now
	else
	{
		*fade = 0.0;
	}
}

static bool CG_InstantCgradingEffectAndFade( const playerState_t* ps, qhandle_t* effect, float* fade )
{
	Q_UNUSED(ps);

	if (cg.zoomed)
	{
		*effect = cgs.media.tealCgrade;
		*fade = 0.4f;
		return true;
	}

	return false;
}

static void CG_AddColorGradingEffects( const playerState_t* ps )
{
	//TODO: make a struct for these
	static qhandle_t currentEffect = 0;
	static float currentFade = 0.0f;
	qhandle_t targetEffect = 0;
	float targetFade = 0.0f;
	qhandle_t instantEffect = 0;
	float instantFade = 0.0f;
	qhandle_t finalEffect = 0;
	float finalFade = 0.0f;

	float fadeRate = 0.0005;

	float fadeChange = fadeRate * cg.frametime;
	float factor;

	//Choose which effect we want
	CG_ChooseCgradingEffectAndFade( ps, &targetEffect, &targetFade, &fadeRate );

	//As we have only one cgrade slot for the effect we transition
	//smoothly from the current (effect, fading) to the target effect.
	if(currentEffect == targetEffect)
	{
		if(currentFade < targetFade)
		{
			currentFade = std::min(targetFade, currentFade + fadeChange);
		}
		else if(currentFade > targetFade)
		{
			currentFade = std::max(targetFade, currentFade - fadeChange);
		}
	}
	else if(currentFade <= 0.0f)
	{
		currentEffect = targetEffect;
	}
	else
	{
		currentFade = std::max(0.0f, currentFade - fadeChange);
	}

	//Instant cgrading effects have the priority
	if( CG_InstantCgradingEffectAndFade( ps, &instantEffect, &instantFade ) )
	{
		finalEffect = instantEffect;
		finalFade = instantFade;
	}
	else
	{
		finalEffect = currentEffect;
		finalFade = currentFade;
	}

	//Apply the chosen cgrade
	trap_SetColorGrading( 0, finalEffect);
	factor = 1.0f - finalFade;
	cg.refdef.gradingWeights[0] = finalFade;
	cg.refdef.gradingWeights[1] *= factor;
	cg.refdef.gradingWeights[2] *= factor;
	cg.refdef.gradingWeights[3] *= factor;
}

/*
===============
CG_AddReverbEffects
===============
*/
static void CG_AddReverbEffects( vec3_t loc )
{
	int   i, j;
	float dist, weight;
	int   selectedIdx[3] = { 0, 0, 0 };
	float selectedWeight[3] = { 0.0f, 0.0f, 0.0f };
	float totalWeight = 0.0f;
	bool haveGlobal = false;

	// the first allocated reverb is special in that it may be global
	i = 0;

	if ( cgs.gameReverbEffects[0][0] && cgs.gameGradingModels[0] == -1 )
	{
		selectedIdx[0] = 0;
		selectedWeight[0] = 2.0f; // won't be sorted down
		haveGlobal = true;
		i = 1;
	}

	for(; i < MAX_REVERB_EFFECTS; i++ )
	{
		if( !cgs.gameReverbEffects[i][0] )
		{
			continue;
		}

		dist = trap_CM_DistanceToModel( loc, cgs.gameReverbModels[i] );
		weight = 1.0f - dist / cgs.gameReverbDistances[i];
		weight = Math::Clamp( weight, 0.0f, 1.0f ); // Maths::clampFraction( weight )

		// search 3 greatest weights
		if( weight <= selectedWeight[2] )
		{
			continue;
		}

		for( j = 1; j >= 0; j-- )
		{
			if( weight <= selectedWeight[j] )
			{
				break;
			}

			selectedIdx[j+1] = selectedIdx[j];
			selectedWeight[j+1] = selectedWeight[j];
		}

		selectedIdx[j+1] = i;
		selectedWeight[j+1] = weight;
	}

	i = haveGlobal ? 1 : 0;

	for(; i < 3; i++ )
	{
		totalWeight += selectedWeight[i];
	}

	if (haveGlobal)
	{
		if (totalWeight > 1.0f)
		{
			selectedWeight[0] = 0;
		}
		else
		{
			selectedWeight[0] = 1.0f - totalWeight;
			totalWeight = 1.0f;
		}
	}

	if (totalWeight == 0.0f)
	{
		for(i = 0; i < 3; i++)
		{
			selectedWeight[i] = 0.0f;
		}
	}
	else
	{
		for(i = 0; i < 3; i++)
		{
			selectedWeight[i] /= totalWeight;
		}
	}

	for (i = 0; i < 3; i++)
	{
		// The mapper defined intensity is between 0 and 2 to have saner defaults (the presets are very strong)
		trap_S_SetReverb(i, cgs.gameReverbEffects[selectedIdx[i]], selectedWeight[i] / 2.0f * cgs.gameReverbIntensities[selectedIdx[i]]);
	}
}

/*
===============
CG_StartShadowCaster

Helper function to add a inverse dynamic light to create shadows for the
following models.
===============
*/
void CG_StartShadowCaster( vec3_t origin, vec3_t mins, vec3_t maxs ) {
	vec3_t ambientLight, directedLight, lightDir;
	vec3_t lightPos;
	trace_t tr;
	vec3_t traceMins = { -3.0f, -3.0f, -3.0f };
	vec3_t traceMaxs = {  3.0f,  3.0f,  3.0f };
	float maxLightDist = Distance( maxs, mins );

	// find a point to place the light source by tracing in the
	// average light direction
	trap_R_LightForPoint( origin, ambientLight, directedLight, lightDir );
	VectorMA( origin, 3.0f * maxLightDist, lightDir, lightPos );

	CG_Trace( &tr, origin, traceMins, traceMaxs, lightPos, 0, MASK_OPAQUE, 0 );

	if( !tr.startsolid ) {
		VectorCopy( tr.endpos, lightPos );
	}

	trap_R_AddLightToScene( lightPos, 2.0f * Distance( lightPos, origin ),
				3.0f, directedLight[0], directedLight[1],
				directedLight[2], 0,
				REF_RESTRICT_DLIGHT | REF_INVERSE_DLIGHT );
}
/*
===============
CG_EndShadowCaster

Helper function to terminate the list of models for the last shadow caster.
following models.
===============
*/
void CG_EndShadowCaster() {
	trap_R_AddLightToScene( vec3_origin, 0.0f, 0.0f,
				0.0f, 0.0f, 0.0f,
				0, 0 );
}

/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
static int CG_CalcViewValues()
{
	playerState_t *ps;

	memset( &cg.refdef, 0, sizeof( cg.refdef ) );

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;

	CG_CalcColorGradingForPoint( ps->origin );
	CG_AddColorGradingEffects( ps );

	CG_AddReverbEffects( ps->origin );

	// intermission view
	if ( ps->pm_type == PM_INTERMISSION || ps->pm_type == PM_FREEZE ||
	     ps->pm_type == PM_SPECTATOR )
	{
		VectorCopy( ps->origin, cg.refdef.vieworg );
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
		AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

		return CG_CalcFov();
	}

	cg.bobcycle = ( ps->bobCycle & 128 ) >> 7;
	cg.bobfracsin = fabs( sinf( ( ps->bobCycle & 127 ) / 127.0 * M_PI ) );
	cg.xyspeed = sqrtf( ps->velocity[ 0 ] * ps->velocity[ 0 ] +
	                    ps->velocity[ 1 ] * ps->velocity[ 1 ] );

	// to avoid jerking, the bob velocity shouldn't be too high
	if ( cg.xyspeed > 300.0f )
	{
		cg.xyspeed = 300.0f;
	}

	VectorCopy( ps->origin, cg.refdef.vieworg );

	if ( BG_ClassHasAbility( ps->stats[ STAT_CLASS ], SCA_WALLCLIMBER ) )
	{
		CG_smoothWWTransitions( ps, ps->viewangles, cg.refdefViewAngles );
	}
	else if ( BG_ClassHasAbility( ps->stats[ STAT_CLASS ], SCA_WALLJUMPER ) )
	{
		CG_smoothWJTransitions( ps, ps->viewangles, cg.refdefViewAngles );
	}
	else
	{
		VectorCopy( ps->viewangles, cg.refdefViewAngles );
	}

	//clumsy logic, but it needs to be this way around because the CS propagation
	//delay screws things up otherwise
	if ( !BG_ClassHasAbility( ps->stats[ STAT_CLASS ], SCA_WALLJUMPER ) )
	{
		if ( !( ps->stats[ STAT_STATE ] & SS_WALLCLIMBING ) )
		{
			VectorSet( cg.lastNormal, 0.0f, 0.0f, 1.0f );
		}
	}

	// add error decay
	if ( cg_errorDecay.Get() > 0 )
	{
		int   t;
		float f;

		t = cg.time - cg.predictedErrorTime;
		f = ( cg_errorDecay.Get() - t ) / cg_errorDecay.Get();

		if ( f > 0 && f < 1 )
		{
			VectorMA( cg.refdef.vieworg, f, cg.predictedError, cg.refdef.vieworg );
		}
		else
		{
			cg.predictedErrorTime = 0;
		}
	}

	//shut off the poison cloud effect if it's still on the go
	if ( cg.snap->ps.stats[ STAT_HEALTH ] > 0 )
	{
		cg.wasDeadLastFrame = false;
	}

	if ( cg.renderingThirdPerson )
	{
		// back away from character
		CG_OffsetThirdPersonView();
	}
	else
	{
		float speed;

		// offset for local bobbing and kicks
		CG_OffsetFirstPersonView();

		// Compute motion blur vector
		speed = VectorNormalize2( cg.snap->ps.velocity, cg.refdef.blurVec );

		speed = (speed - cg_motionblurMinSpeed.Get());
		if( speed < 0.0f ) speed = 0.0f;

		VectorScale( cg.refdef.blurVec, speed * cg_motionblur.Get(),
			     cg.refdef.blurVec );

	}

	// position eye relative to origin
	AnglesToAxis( cg.refdefViewAngles, cg.refdef.viewaxis );

	if ( cg.hyperspace )
	{
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	//draw the surface normal looking at
	if ( cg_drawSurfNormal.Get() )
	{
		CG_DrawSurfNormal();
	}

	// field of view
	return CG_CalcFov();
}

//=========================================================================

static cplane_t  frustum[4];

/*
=================
CG_SetupFrustum
=================
*/
void CG_SetupFrustum()
{
	int             i;
	float           xs, xc;
	float           ang;

	ang = cg.refdef.fov_x / 180 * M_PI_2;
	xs = sinf(ang);
	xc = cosf(ang);

	VectorScale(cg.refdef.viewaxis[0], xs, frustum[0].normal);
	VectorMA(frustum[0].normal, xc, cg.refdef.viewaxis[1], frustum[0].normal);

	VectorScale(cg.refdef.viewaxis[0], xs, frustum[1].normal);
	VectorMA(frustum[1].normal, -xc, cg.refdef.viewaxis[1], frustum[1].normal);

	ang = cg.refdef.fov_y / 180 * M_PI_2;
	xs = sinf(ang);
	xc = cosf(ang);

	VectorScale(cg.refdef.viewaxis[0], xs, frustum[2].normal);
	VectorMA(frustum[2].normal, xc, cg.refdef.viewaxis[2], frustum[2].normal);

	VectorScale(cg.refdef.viewaxis[0], xs, frustum[3].normal);
	VectorMA(frustum[3].normal, -xc, cg.refdef.viewaxis[2], frustum[3].normal);

	for(i = 0; i < 4; i++)
	{
		frustum[i].dist = DotProduct(cg.refdef.vieworg, frustum[i].normal);
		frustum[i].type = PLANE_NON_AXIAL;
		SetPlaneSignbits(&frustum[i]);
	}
}

/*
=================
CG_CullBox

returns true if culled
=================
*/
bool CG_CullBox(vec3_t mins, vec3_t maxs)
{
	int              i;
	cplane_t         *frust;

	//check against frustum planes
	for( i = 0; i < 4; i++ )
	{
		frust = &frustum[i];

		if( BoxOnPlaneSide(mins, maxs, frust ) == 2 )
			return true;
	}
	return false;
}

/*
=================
CG_CullPointAndRadius

returns true if culled
=================
*/
bool CG_CullPointAndRadius(const vec3_t pt, vec_t radius)
{
	int             i;
	cplane_t        *frust;

	// check against frustum planes
	for( i = 0; i < 4; i++)
	{
		frust = &frustum[i];

		if( ( DotProduct(pt, frust->normal) - frust->dist ) < -radius )
		{
			return true;
		}
	}

	return false;
}

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/
void CG_DrawActiveFrame( int serverTime, bool demoPlayback )
{
	int inwater;

	cg.time = serverTime;
	cg.demoPlayback = demoPlayback;

	CG_NotifyHooks();

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds( false );

	// clear all the render lists
	trap_R_ClearScene();

	// set up cg.snap and possibly cg.nextSnap
	CG_ProcessSnapshots();

	// if we haven't received any snapshots yet, all
	// we can draw is the information screen
	if ( !cg.snap || ( cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE ) )
	{
		return;
	}

	// let the client system know what our weapon and zoom settings are
	trap_SetUserCmdValue( cg.weaponSelect, 0, cg.zoomSensitivity );

	if ( cg.clientFrame == 0 )
	{
		trap_S_ClearLoopingSounds( true );
		CG_StartMusic();
	}

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	// update unlockables data (needs valid predictedPlayerState)
	CG_UpdateUnlockables( &cg.predictedPlayerState );

	// update cvars (needs valid unlockables data)
	CG_UpdateCvars();

	bool alive = cg.snap->ps.stats[ STAT_HEALTH ] > 0;
	bool spectate = cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT;
	// pretend the spectator is alive
	alive |= spectate;

	// decide on third person view
	cg.renderingThirdPerson = ( cg_thirdPerson.Get() || !alive
		|| ( cg.chaseFollow && cg.snap->ps.pm_flags & PMF_FOLLOW ) );

	// update speedometer
	CG_AddSpeed();

	// build cg.refdef
	inwater = CG_CalcViewValues();

	//build culling planes
	CG_SetupFrustum();

	// build the render lists
	if ( !cg.hyperspace )
	{
		CG_AddPacketEntities(); // after calcViewValues, so predicted player state is correct
		CG_AddMarks();
	}

	CG_AddViewWeapon( &cg.predictedPlayerState );

	//after CG_AddViewWeapon
	if ( !cg.hyperspace )
	{
		CG_AddParticles();
		CG_AddTrails();
	}

	// finish up the rest of the refdef
	if ( cg.testModelEntity.hModel )
	{
		CG_AddTestModel();
	}

	cg.refdef.time = cg.time;
	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );

	// update audio positions
	if (cg_thirdPerson.Get()) {
		trap_S_Respatialize( -1, cg.refdef.vieworg, cg.refdef.viewaxis, inwater );
	} else {
		trap_S_Respatialize( cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater );
	}

	cg.frametime = cg.time - cg.oldTime;

	if ( cg.frametime < 0 )
	{
		cg.frametime = 0;
	}

	cg.oldTime = cg.time;
	CG_AddLagometerFrameInfo();

	// actually issue the rendering calls
	CG_DrawActive();
}
