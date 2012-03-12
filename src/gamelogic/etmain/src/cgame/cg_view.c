/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

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

In addition, the Daemon Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the Daemon 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

// cg_view.c -- setup all the parameters (position, angle, etc)
// for a 3D rendering
#include "cg_local.h"

//========================
extern pmove_t  cg_pmove;

//========================

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

Testgun will cause the model to follow the player around and supress the real
view weapon model.  The default frame 0 of most guns is completely off screen,
so you will probably have to cycle a couple frames to see it.

"nextframe", "prevframe", "nextskin", and "prevskin" commands will change the
frame or skin of the testmodel.  These are bound to F5, F6, F7, and F8 in
q3default.cfg.

If a gun is being tested, the "gun_x", "gun_y", and "gun_z" variables will let
you adjust the positioning.

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
void CG_TestModel_f(void)
{
	vec3_t          angles;

	memset(&cg.testModelEntity, 0, sizeof(cg.testModelEntity));
	if(trap_Argc() < 2)
	{
		return;
	}

	Q_strncpyz(cg.testModelName, CG_Argv(1), MAX_QPATH);
	cg.testModelEntity.hModel = trap_R_RegisterModel(cg.testModelName);

	if(trap_Argc() == 3)
	{
		cg.testModelEntity.backlerp = atof(CG_Argv(2));
		cg.testModelEntity.frame = 1;
		cg.testModelEntity.oldframe = 0;
	}
	if(!cg.testModelEntity.hModel)
	{
		CG_Printf("Can't register model\n");
		return;
	}

	VectorMA(cg.refdef.vieworg, 100, cg.refdef.viewaxis[0], cg.testModelEntity.origin);

	angles[PITCH] = 0;
	angles[YAW] = 180 + cg.refdefViewAngles[1];
	angles[ROLL] = 0;

	AnglesToAxis(angles, cg.testModelEntity.axis);
	cg.testGun = qfalse;
}

/*
=================
CG_TestGun_f

Replaces the current view weapon with the given model
=================
*/
void CG_TestGun_f(void)
{
	CG_TestModel_f();
	cg.testGun = qtrue;
	cg.testModelEntity.renderfx = RF_MINLIGHT | RF_DEPTHHACK | RF_FIRST_PERSON;
}


void CG_TestModelNextFrame_f(void)
{
	cg.testModelEntity.frame++;
	CG_Printf("frame %i\n", cg.testModelEntity.frame);
}

void CG_TestModelPrevFrame_f(void)
{
	cg.testModelEntity.frame--;
	if(cg.testModelEntity.frame < 0)
	{
		cg.testModelEntity.frame = 0;
	}
	CG_Printf("frame %i\n", cg.testModelEntity.frame);
}

void CG_TestModelNextSkin_f(void)
{
	cg.testModelEntity.skinNum++;
	CG_Printf("skin %i\n", cg.testModelEntity.skinNum);
}

void CG_TestModelPrevSkin_f(void)
{
	cg.testModelEntity.skinNum--;
	if(cg.testModelEntity.skinNum < 0)
	{
		cg.testModelEntity.skinNum = 0;
	}
	CG_Printf("skin %i\n", cg.testModelEntity.skinNum);
}

static void CG_AddTestModel(void)
{
	int             i;

	// re-register the model, because the level may have changed
	cg.testModelEntity.hModel = trap_R_RegisterModel(cg.testModelName);
	if(!cg.testModelEntity.hModel)
	{
		CG_Printf("Can't register model\n");
		return;
	}

	// if testing a gun, set the origin reletive to the view origin
	if(cg.testGun)
	{
		VectorCopy(cg.refdef.vieworg, cg.testModelEntity.origin);
		VectorCopy(cg.refdef.viewaxis[0], cg.testModelEntity.axis[0]);
		VectorCopy(cg.refdef.viewaxis[1], cg.testModelEntity.axis[1]);
		VectorCopy(cg.refdef.viewaxis[2], cg.testModelEntity.axis[2]);

		// allow the position to be adjusted
		for(i = 0; i < 3; i++)
		{
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[0][i] * cg_gun_x.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[1][i] * cg_gun_y.value;
			cg.testModelEntity.origin[i] += cg.refdef.viewaxis[2][i] * cg_gun_z.value;
		}
	}

	trap_R_AddRefEntityToScene(&cg.testModelEntity);
}



//============================================================================


/*
=================
CG_CalcVrect

Sets the coordinates of the rendered window
=================
*/

//static float letterbox_frac = 1.0f;   // used for transitioning to letterbox for cutscenes // TODO: add to cg. // TTimo: unused
void CG_Letterbox(float xsize, float ysize, qboolean center)
{
// normal aspect is xx:xx
// letterbox is yy:yy  (85% of 'normal' height)
	if(cg_letterbox.integer)
	{
		float           lbheight, lbdiff;

		lbheight = ysize * 0.85;
		lbdiff = ysize - lbheight;

		if(!center)
		{
			int             offset = (cgs.glconfig.vidHeight * (.5f * lbdiff)) / 100;

			offset &= ~1;
			cg.refdef.y += offset;
		}

		ysize = lbheight;
//      if(letterbox_frac != 0) {
//          letterbox_frac -= 0.01f;    // (SA) TODO: make non fps dependant
//          if(letterbox_frac < 0)
//              letterbox_frac = 0;
//          ysize += (lbdiff * letterbox_frac);
//      }
//  } else {
//      if(letterbox_frac != 1) {
//          letterbox_frac += 0.01f;    // (SA) TODO: make non fps dependant
//          if(letterbox_frac > 1)
//              letterbox_frac = 1;
//          ysize = lbheight + (lbdiff * letterbox_frac);
//      }
	}

	cg.refdef.width = cgs.glconfig.vidWidth * xsize / 100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight * ysize / 100;
	cg.refdef.height &= ~1;

	if(center)
	{
		cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width) / 2;
		cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height) / 2;
	}
}

static void CG_CalcVrect(void)
{
	if(cg.showGameView)
	{
		float           x, y, w, h;

		x = LIMBO_3D_X;
		y = LIMBO_3D_Y;
		w = LIMBO_3D_W;
		h = LIMBO_3D_H;

		CG_AdjustFrom640(&x, &y, &w, &h);

		cg.refdef.x = x;
		cg.refdef.y = y;
		cg.refdef.width = w;
		cg.refdef.height = h;

		CG_Letterbox((LIMBO_3D_W / 640.f) * 100, (LIMBO_3D_H / 480.f) * 100, qfalse);
		return;
	}

	CG_Letterbox(100, 100, qtrue);
}

//==============================================================================

/*
===============
CG_OffsetThirdPersonView

===============
*/
#define FOCUS_DISTANCE  400		//800   //512
void CG_OffsetThirdPersonView(void)
{
	vec3_t          forward, right, up;
	vec3_t          view;
	vec3_t          focusAngles;
	trace_t         trace;
	static vec3_t   mins = { -4, -4, -4 };
	static vec3_t   maxs = { 4, 4, 4 };
	vec3_t          focusPoint;
	float           focusDist;
	float           forwardScale, sideScale;

	cg.refdef_current->vieworg[2] += cg.predictedPlayerState.viewheight;

	VectorCopy(cg.refdefViewAngles, focusAngles);

	// rain - if dead, look at medic or allow freelook if none in range
	if(cg.predictedPlayerState.stats[STAT_HEALTH] <= 0)
	{
		// rain - #254 - force yaw to 0 if we're tracking a medic
		if(cg.snap->ps.viewlocked != 7)
		{
			// rain - do short2angle AFTER the network part
			focusAngles[YAW] = SHORT2ANGLE(cg.predictedPlayerState.stats[STAT_DEAD_YAW]);
			cg.refdefViewAngles[YAW] = SHORT2ANGLE(cg.predictedPlayerState.stats[STAT_DEAD_YAW]);
		}
	}

	if(focusAngles[PITCH] > 45)
	{
		focusAngles[PITCH] = 45;	// don't go too far overhead
	}
	AngleVectors(focusAngles, forward, NULL, NULL);

	if(cg_thirdPerson.integer == 2)
	{
		VectorCopy(cg.predictedPlayerState.origin, focusPoint);
	}
	else
	{
		VectorMA(cg.refdef_current->vieworg, FOCUS_DISTANCE, forward, focusPoint);
	}

	VectorCopy(cg.refdef_current->vieworg, view);

	view[2] += 8;
	cg.refdefViewAngles[PITCH] *= 0.5;

	AngleVectors(cg.refdefViewAngles, forward, right, up);

	forwardScale = cos(cg_thirdPersonAngle.value / 180 * M_PI);
	sideScale = sin(cg_thirdPersonAngle.value / 180 * M_PI);
	VectorMA(view, -cg_thirdPersonRange.value * forwardScale, forward, view);
	VectorMA(view, -cg_thirdPersonRange.value * sideScale, right, view);

	// trace a ray from the origin to the viewpoint to make sure the view isn't
	// in a solid block.  Use an 8 by 8 block to prevent the view from near clipping anything

	CG_Trace(&trace, cg.refdef_current->vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID);

	if(trace.fraction != 1.0)
	{
		VectorCopy(trace.endpos, view);
		view[2] += (1.0 - trace.fraction) * 32;
		// try another trace to this position, because a tunnel may have the ceiling
		// close enogh that this is poking out

		CG_Trace(&trace, cg.refdef_current->vieworg, mins, maxs, view, cg.predictedPlayerState.clientNum, MASK_SOLID);
		VectorCopy(trace.endpos, view);
	}


	VectorCopy(view, cg.refdef_current->vieworg);

	// select pitch to look at focus point from vieword
	VectorSubtract(focusPoint, cg.refdef_current->vieworg, focusPoint);
	focusDist = sqrt(focusPoint[0] * focusPoint[0] + focusPoint[1] * focusPoint[1]);
	if(focusDist < 1)
	{
		focusDist = 1;			// should never happen
	}
	cg.refdefViewAngles[PITCH] = -180 / M_PI * atan2(focusPoint[2], focusDist);
	cg.refdefViewAngles[YAW] -= cg_thirdPersonAngle.value;
}


// this causes a compiler bug on mac MrC compiler
static void CG_StepOffset(void)
{
	int             timeDelta;

	// smooth out stair climbing
	timeDelta = cg.time - cg.stepTime;
	// Ridah
	if(timeDelta < 0)
	{
		cg.stepTime = cg.time;
	}
	if(timeDelta < STEP_TIME)
	{
		cg.refdef_current->vieworg[2] -= cg.stepChange * (STEP_TIME - timeDelta) / STEP_TIME;
	}
}

/*
================
CG_KickAngles
================
*/
void CG_KickAngles(void)
{
	const vec3_t    centerSpeed = { 2400, 2400, 2400 };
	const float     recoilCenterSpeed = 200;
	const float     recoilIgnoreCutoff = 15;
	const float     recoilMaxSpeed = 50;
	const vec3_t    maxKickAngles = { 10, 10, 10 };
	float           idealCenterSpeed, kickChange;
	int             i, frametime, t;
	float           ft;

#define STEP 20
	char            buf[32];	// NERVE - SMF

	// this code is frametime-dependant, so split it up into small chunks
	//cg.kickAngles[PITCH] = 0;
	cg.recoilPitchAngle = 0;
	for(t = cg.frametime; t > 0; t -= STEP)
	{
		if(t > STEP)
		{
			frametime = STEP;
		}
		else
		{
			frametime = t;
		}

		ft = ((float)frametime / 1000);

		// kickAngles is spring-centered
		for(i = 0; i < 3; i++)
		{
			if(cg.kickAVel[i] || cg.kickAngles[i])
			{
				// apply centering forces to kickAvel
				if(cg.kickAngles[i] && frametime)
				{
					idealCenterSpeed = -(2.0 * (cg.kickAngles[i] > 0) - 1.0) * centerSpeed[i];
					if(idealCenterSpeed)
					{
						cg.kickAVel[i] += idealCenterSpeed * ft;
					}
				}
				// add the kickAVel to the kickAngles
				kickChange = cg.kickAVel[i] * ft;
				if(cg.kickAngles[i] && (cg.kickAngles[i] < 0) != (kickChange < 0))
				{				// slower when returning to center
					kickChange *= 0.06;
				}
				// check for crossing back over the center point
				if(!cg.kickAngles[i] || ((cg.kickAngles[i] + kickChange) < 0) == (cg.kickAngles[i] < 0))
				{
					cg.kickAngles[i] += kickChange;
					if(!cg.kickAngles[i] && frametime)
					{
						cg.kickAVel[i] = 0;
					}
					else if(fabs(cg.kickAngles[i]) > maxKickAngles[i])
					{
						cg.kickAngles[i] = maxKickAngles[i] * ((2 * (cg.kickAngles[i] > 0)) - 1);
						cg.kickAVel[i] = 0;	// force Avel to return us to center rather than keep going outside range
					}
				}
				else
				{				// about to cross, so just zero it out
					cg.kickAngles[i] = 0;
					cg.kickAVel[i] = 0;
				}
			}
		}

		// recoil is added to input viewangles per frame
		if(cg.recoilPitch)
		{
			// apply max recoil
			if(fabs(cg.recoilPitch) > recoilMaxSpeed)
			{
				if(cg.recoilPitch > 0)
				{
					cg.recoilPitch = recoilMaxSpeed;
				}
				else
				{
					cg.recoilPitch = -recoilMaxSpeed;
				}
			}
			// apply centering forces to kickAvel
			if(frametime)
			{
				idealCenterSpeed = -(2.0 * (cg.recoilPitch > 0) - 1.0) * recoilCenterSpeed * ft;
				if(idealCenterSpeed)
				{
					if(fabs(idealCenterSpeed) < fabs(cg.recoilPitch))
					{
						cg.recoilPitch += idealCenterSpeed;
					}
					else
					{			// back zero out
						cg.recoilPitch = 0;
					}
				}
			}
		}
		if(fabs(cg.recoilPitch) > recoilIgnoreCutoff)
		{
			cg.recoilPitchAngle += cg.recoilPitch * ft;
		}
	}

	// NERVE - SMF - only change cg_recoilPitch cvar when we need to
	trap_Cvar_VariableStringBuffer("cg_recoilPitch", buf, sizeof(buf));

	if(atof(buf) != cg.recoilPitchAngle)
	{
		// encode the kick angles into a 24bit number, for sending to the client exe
		trap_Cvar_Set("cg_recoilPitch", va("%f", cg.recoilPitchAngle));
	}
}


/*
CG_Concussive
*/
void CG_Concussive(centity_t * cent)
{
	float           length;

//  vec3_t  dir, forward;
	vec3_t          vec;

//  float   dot;

	//
	float           pitchRecoilAdd, pitchAdd;
	float           yawRandom;
	vec3_t          recoil;

	//

	if(!cg.renderingThirdPerson && cent->currentState.density == cg.snap->ps.clientNum)
	{
		//
		pitchRecoilAdd = 0;
		pitchAdd = 0;
		yawRandom = 0;
		//

		VectorSubtract(cg.snap->ps.origin, cent->currentState.origin, vec);
		length = VectorLength(vec);

		// pitchAdd = 12+rand()%3;
		// yawRandom = 6;

		if(length > 1024)
		{
			return;
		}

		pitchAdd = (32 / length) * 64;
		yawRandom = (32 / length) * 64;

		// recoil[YAW] = crandom()*yawRandom;
		if(rand() % 100 > 50)
		{
			recoil[YAW] = -yawRandom;
		}
		else
		{
			recoil[YAW] = yawRandom;
		}

		recoil[ROLL] = -recoil[YAW];	// why not
		recoil[PITCH] = -pitchAdd;
		// scale it up a bit (easier to modify this while tweaking)
		VectorScale(recoil, 30, recoil);
		// set the recoil
		VectorCopy(recoil, cg.kickAVel);
		// set the recoil
		cg.recoilPitch -= pitchRecoilAdd;

	}
}


/*
==============
CG_ZoomSway
	sway for scoped weapons.
	this takes aimspread into account so the view settles after a bit
==============
*/
static void CG_ZoomSway(void)
{
	float           spreadfrac;
	float           phase;

	if(!cg.zoomval)
	{							// not zoomed
		return;
	}

	if(cg.snap->ps.eFlags & EF_MG42_ACTIVE || cg.snap->ps.eFlags & EF_AAGUN_ACTIVE)
	{							// don't draw when on mg_42
		return;
	}

	spreadfrac = (float)cg.snap->ps.aimSpreadScale / 255.0;

	phase = cg.time / 1000.0 * ZOOM_PITCH_FREQUENCY * M_PI * 2;
	cg.refdefViewAngles[PITCH] += ZOOM_PITCH_AMPLITUDE * sin(phase) * (spreadfrac + ZOOM_PITCH_MIN_AMPLITUDE);

	phase = cg.time / 1000.0 * ZOOM_YAW_FREQUENCY * M_PI * 2;
	cg.refdefViewAngles[YAW] += ZOOM_YAW_AMPLITUDE * sin(phase) * (spreadfrac + ZOOM_YAW_MIN_AMPLITUDE);

}



/*
===============
CG_OffsetFirstPersonView

===============
*/
static void CG_OffsetFirstPersonView(void)
{
	float          *origin;
	float          *angles;
	float           bob;
	float           ratio;
	float           delta;
	float           speed;
	float           f;
	vec3_t          predictedVelocity;
	int             timeDelta;
	qboolean        useLastValidBob = qfalse;

	if(cg.snap->ps.pm_type == PM_INTERMISSION)
	{
		return;
	}

	origin = cg.refdef_current->vieworg;
	angles = cg.refdefViewAngles;

	if(cg.snap->ps.weapon == WP_MOBILE_MG42_SET)
	{
		float           yawDiff = cg.refdefViewAngles[YAW] - cg.pmext.mountedWeaponAngles[YAW];
		vec3_t          forward, point;
		float           oldZ = origin[2];

		AngleVectors(cg.pmext.mountedWeaponAngles, forward, NULL, NULL);

		if(yawDiff > 180)
		{
			yawDiff -= 360;
		}
		else if(yawDiff < -180)
		{
			yawDiff += 360;
		}

		VectorMA(origin, 31, forward, point);
		AngleVectors(cg.refdefViewAngles, forward, NULL, NULL);
		VectorMA(point, -32, forward, origin);

		origin[2] = oldZ;
	}
	else if(cg.snap->ps.weapon == WP_MORTAR_SET)
	{
		float           yawDiff = cg.refdefViewAngles[YAW] - cg.pmext.mountedWeaponAngles[YAW];
		vec3_t          forward, point;
		float           oldZ = origin[2];

		AngleVectors(cg.pmext.mountedWeaponAngles, forward, NULL, NULL);

		if(yawDiff > 180)
		{
			yawDiff -= 360;
		}
		else if(yawDiff < -180)
		{
			yawDiff += 360;
		}

		VectorMA(origin, 31, forward, point);
		AngleVectors(cg.refdefViewAngles, forward, NULL, NULL);
		VectorMA(point, -32, forward, origin);

		origin[2] = oldZ;
	}

	// if dead, fix the angle and don't add any kick
	if(!(cg.snap->ps.pm_flags & PMF_LIMBO) && cg.snap->ps.stats[STAT_HEALTH] <= 0)
	{
		angles[ROLL] = 40;
		angles[PITCH] = -15;

		// rain - #254 - force yaw to 0 if we're tracking a medic
		// rain - medic tracking doesn't seem to happen in this case?
		if(cg.snap->ps.viewlocked == 7)
		{
			angles[YAW] = 0;
		}
		else
		{
			// rain - do short2angle AFTER the network part
			angles[YAW] = SHORT2ANGLE(cg.snap->ps.stats[STAT_DEAD_YAW]);
		}

		origin[2] += cg.predictedPlayerState.viewheight;
		return;
	}

	// add angles based on weapon kick
	VectorAdd(angles, cg.kick_angles, angles);

	// RF, add new weapon kick angles
	CG_KickAngles();
	VectorAdd(angles, cg.kickAngles, angles);
	// RF, pitch is already added
	//angles[0] -= cg.kickAngles[PITCH];

	// add angles based on damage kick
	if(cg.damageTime)
	{
		ratio = cg.time - cg.damageTime;
		if(ratio < DAMAGE_DEFLECT_TIME)
		{
			ratio /= DAMAGE_DEFLECT_TIME;
			angles[PITCH] += ratio * cg.v_dmg_pitch;
			angles[ROLL] += ratio * cg.v_dmg_roll;
		}
		else
		{
			ratio = 1.0 - (ratio - DAMAGE_DEFLECT_TIME) / DAMAGE_RETURN_TIME;
			if(ratio > 0)
			{
				angles[PITCH] += ratio * cg.v_dmg_pitch;
				angles[ROLL] += ratio * cg.v_dmg_roll;
			}
		}
	}

	// add pitch based on fall kick
#if 0
	ratio = (cg.time - cg.landTime) / FALL_TIME;
	if(ratio < 0)
	{
		ratio = 0;
	}
	angles[PITCH] += ratio * cg.fall_value;
#endif

	// add angles based on velocity
	VectorCopy(cg.predictedPlayerState.velocity, predictedVelocity);

	delta = DotProduct(predictedVelocity, cg.refdef_current->viewaxis[0]);
	angles[PITCH] += delta * cg_runpitch.value;

	delta = DotProduct(predictedVelocity, cg.refdef_current->viewaxis[1]);
	angles[ROLL] -= delta * cg_runroll.value;

	// add angles based on bob

	// make sure the bob is visible even at low speeds
	speed = cg.xyspeed > 200 ? cg.xyspeed : 200;

	if(!cg.bobfracsin && cg.lastvalidBobfracsin > 0)
	{
		// 200 msec to get back to center from 1
		// that's 1/200 per msec = 0.005 per msec
		cg.lastvalidBobfracsin -= 0.005 * cg.frametime;
		useLastValidBob = qtrue;
	}

	delta = useLastValidBob ? cg.lastvalidBobfracsin * cg_bobpitch.value * speed : cg.bobfracsin * cg_bobpitch.value * speed;
	if(cg.predictedPlayerState.pm_flags & PMF_DUCKED)
	{
		delta *= 3;				// crouching
	}
	angles[PITCH] += delta;
	delta = useLastValidBob ? cg.lastvalidBobfracsin * cg_bobroll.value * speed : cg.bobfracsin * cg_bobroll.value * speed;
	if(cg.predictedPlayerState.pm_flags & PMF_DUCKED)
	{
		delta *= 3;				// crouching accentuates roll
	}
	if(useLastValidBob)
	{
		if(cg.lastvalidBobcycle & 1)
		{
			delta = -delta;
		}
	}
	else if(cg.bobcycle & 1)
	{
		delta = -delta;
	}
	angles[ROLL] += delta;

	/*if (cg.predictedPlayerState.eFlags & EF_PRONE) {
	   delta = useLastValidBob ? cg.lastvalidBobfracsin * cg_bobyaw.value * speed * 15 : cg.bobfracsin * cg_bobyaw.value * speed * 15;
	   if( useLastValidBob ) {
	   if( cg.lastvalidBobcycle & 1 )
	   delta = -delta;
	   } else if (cg.bobcycle & 1)
	   delta = -delta;

	   angles[YAW] += delta;
	   } */

//===================================

	// add view height
	origin[2] += cg.predictedPlayerState.viewheight;

	// smooth out duck height changes
	timeDelta = cg.time - cg.duckTime;
	if(cg.predictedPlayerState.eFlags & EF_PRONE)
	{
		if(timeDelta < 0)
		{						// Ridah
			cg.duckTime = cg.time - PRONE_TIME;
		}
		if(timeDelta < PRONE_TIME)
		{
			cg.refdef_current->vieworg[2] -= cg.duckChange * (PRONE_TIME - timeDelta) / PRONE_TIME;
		}
	}
	else
	{
		if(timeDelta < 0)
		{						// Ridah
			cg.duckTime = cg.time - DUCK_TIME;
		}
		if(timeDelta < DUCK_TIME)
		{
			cg.refdef_current->vieworg[2] -= cg.duckChange * (DUCK_TIME - timeDelta) / DUCK_TIME;
		}
	}

	// add bob height
	bob = cg.bobfracsin * cg.xyspeed * cg_bobup.value;
	if(bob > 6)
	{
		bob = 6;
	}

	origin[2] += bob;


	// add fall height
	delta = cg.time - cg.landTime;
	if(delta < 0)
	{							// Ridah
		cg.landTime = cg.time - (LAND_DEFLECT_TIME + LAND_RETURN_TIME);
	}
	if(delta < LAND_DEFLECT_TIME)
	{
		f = delta / LAND_DEFLECT_TIME;
		cg.refdef_current->vieworg[2] += cg.landChange * f;
	}
	else if(delta < LAND_DEFLECT_TIME + LAND_RETURN_TIME)
	{
		delta -= LAND_DEFLECT_TIME;
		f = 1.0 - (delta / LAND_RETURN_TIME);
		cg.refdef_current->vieworg[2] += cg.landChange * f;
	}

	// add step offset
	CG_StepOffset();

	CG_ZoomSway();

	// adjust for 'lean'
	if(cg.predictedPlayerState.leanf != 0)
	{
		//add leaning offset
		vec3_t          right;

		cg.refdefViewAngles[2] += cg.predictedPlayerState.leanf / 2.0f;
		AngleVectors(cg.refdefViewAngles, NULL, right, NULL);
		VectorMA(cg.refdef_current->vieworg, cg.predictedPlayerState.leanf, right, cg.refdef_current->vieworg);
	}

	// add kick offset

	VectorAdd(origin, cg.kick_origin, origin);

	// pivot the eye based on a neck length
#if 0
	{
#define NECK_LENGTH     8
		vec3_t          forward, up;

		cg.refdef_current->vieworg[2] -= NECK_LENGTH;
		AngleVectors(cg.refdefViewAngles, forward, NULL, up);
		VectorMA(cg.refdef_current->vieworg, 3, forward, cg.refdef_current->vieworg);
		VectorMA(cg.refdef_current->vieworg, NECK_LENGTH, up, cg.refdef_current->vieworg);
	}
#endif
}

//======================================================================

//
// Zoom controls
//


// probably move to server variables
float           zoomTable[ZOOM_MAX_ZOOMS][2] = {
// max {out,in}
	{0, 0},

	{36, 8},					//  binoc
	{20, 4},					//  sniper
	{60, 20},					//  snooper
	{55, 55},					//  fg42
	{55, 55}					//  mg42
};

void CG_AdjustZoomVal(float val, int type)
{
	cg.zoomval += val;
	if(cg.zoomval > zoomTable[type][ZOOM_OUT])
	{
		cg.zoomval = zoomTable[type][ZOOM_OUT];
	}
	if(cg.zoomval < zoomTable[type][ZOOM_IN])
	{
		cg.zoomval = zoomTable[type][ZOOM_IN];
	}
}

void CG_ZoomIn_f(void)
{
	// Gordon: fixed being able to "latch" your zoom by weaponcheck + quick zoomin
	// OSP - change for zoom view in demos
	if(cg_entities[cg.snap->ps.clientNum].currentState.weapon == WP_GARAND_SCOPE)
	{
		CG_AdjustZoomVal(-(cg_zoomStepSniper.value), ZOOM_SNIPER);
	}
	else if(cg_entities[cg.snap->ps.clientNum].currentState.weapon == WP_K43_SCOPE)
	{
		CG_AdjustZoomVal(-(cg_zoomStepSniper.value), ZOOM_SNIPER);
	}
	else if(cg.zoomedBinoc)
	{
		CG_AdjustZoomVal(-(cg_zoomStepSniper.value), ZOOM_SNIPER);	// JPW NERVE per atvi request all use same vals to match menu (was zoomStepBinoc, ZOOM_BINOC);
	}
}

void CG_ZoomOut_f(void)
{
	if(cg_entities[cg.snap->ps.clientNum].currentState.weapon == WP_GARAND_SCOPE)
	{
		CG_AdjustZoomVal(cg_zoomStepSniper.value, ZOOM_SNIPER);
	}
	else if(cg_entities[cg.snap->ps.clientNum].currentState.weapon == WP_K43_SCOPE)
	{
		CG_AdjustZoomVal(cg_zoomStepSniper.value, ZOOM_SNIPER);
	}
	else if(cg.zoomedBinoc)
	{
		CG_AdjustZoomVal(cg_zoomStepSniper.value, ZOOM_SNIPER);	// JPW NERVE per atvi request BINOC);
	}
}


/*
==============
CG_Zoom
==============
*/
void CG_Zoom(void)
{
	// OSP - Fix for demo playback
	if((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback)
	{
		cg.predictedPlayerState.eFlags = cg.snap->ps.eFlags;
		cg.predictedPlayerState.weapon = cg.snap->ps.weapon;

		// check for scope wepon in use, and switch to if necessary
		// OSP - spec/demo scaling allowances
		if(cg.predictedPlayerState.weapon == WP_FG42SCOPE)
		{
			cg.zoomval = (cg.zoomval == 0) ? cg_zoomDefaultSniper.value : cg.zoomval;	// JPW NERVE was DefaultFG, changed per atvi req
		}
		else if(cg.predictedPlayerState.weapon == WP_GARAND_SCOPE)
		{
			cg.zoomval = (cg.zoomval == 0) ? cg_zoomDefaultSniper.value : cg.zoomval;
		}
		else if(cg.predictedPlayerState.weapon == WP_K43_SCOPE)
		{
			cg.zoomval = (cg.zoomval == 0) ? cg_zoomDefaultSniper.value : cg.zoomval;
		}
		else if(!(cg.predictedPlayerState.eFlags & EF_ZOOMING))
		{
			cg.zoomval = 0;
		}
	}
	if(cg.predictedPlayerState.eFlags & EF_ZOOMING)
	{
		if(cg.zoomedBinoc)
		{
			return;
		}
		cg.zoomedBinoc = qtrue;
		cg.zoomTime = cg.time;
		cg.zoomval = cg_zoomDefaultSniper.value;	// JPW NERVE was DefaultBinoc, changed per atvi req
	}
	else
	{
		if(cg.zoomedBinoc)
		{
			cg.zoomedBinoc = qfalse;
			cg.zoomTime = cg.time;

			// check for scope weapon in use, and switch to if necessary
			if(cg.weaponSelect == WP_FG42SCOPE)
			{
				cg.zoomval = cg_zoomDefaultSniper.value;	// JPW NERVE was DefaultFG, changed per atvi req
			}
			else if(cg.weaponSelect == WP_GARAND_SCOPE)
			{
				cg.zoomval = cg_zoomDefaultSniper.value;
			}
			else if(cg.weaponSelect == WP_K43_SCOPE)
			{
				cg.zoomval = cg_zoomDefaultSniper.value;
			}
			else
			{
				cg.zoomval = 0;
			}
		}
		else
		{
//bani - we now sanity check to make sure we can't zoom non-zoomable weapons
//zinx - fix for #423 - don't sanity check while following
			if(!((cg.snap->ps.pm_flags & PMF_FOLLOW) || cg.demoPlayback))
			{
				switch (cg.weaponSelect)
				{
					case WP_FG42SCOPE:
					case WP_GARAND_SCOPE:
					case WP_K43_SCOPE:
						break;
					default:
						cg.zoomval = 0;
						break;
				}
			}
		}
	}
}


/*
====================
CG_CalcFov

Fixed fov at intermissions, otherwise account for fov variable and zooms.
====================
*/
#define WAVE_AMPLITUDE  1
#define WAVE_FREQUENCY  0.4

static int CG_CalcFov(void)
{
	static float    lastfov = 90;	// for transitions back from zoomed in modes
	float           x;
	float           phase;
	float           v;
	int             contents;
	float           fov_x, fov_y;
	float           zoomFov;
	float           f;
	int             inwater;

	CG_Zoom();

	if(cg.predictedPlayerState.stats[STAT_HEALTH] <= 0 && !(cg.snap->ps.pm_flags & PMF_FOLLOW))
	{
		cg.zoomedBinoc = qfalse;
		cg.zoomTime = 0;
		cg.zoomval = 0;
	}

	// CHRUKER: b088 - Players field of view at intermission is different if you had deployed a MG42 (fixation moved further down)
	fov_x = cg_fov.value; // cs: moved this out for compiler warning
	if(cg.predictedPlayerState.pm_type != PM_INTERMISSION)
	{
		if(!developer.integer)
		{
			if(fov_x < 90)
			{
				fov_x = 90;
			}
			else if(fov_x > 160)
			{
				fov_x = 160;
			}
		}

		if(!cg.renderingThirdPerson || developer.integer)
		{
			// account for zooms
			if(cg.zoomval)
			{
				zoomFov = cg.zoomval;	// (SA) use user scrolled amount

				if(zoomFov < 1)
				{
					zoomFov = 1;
				}
				else if(zoomFov > 160)
				{
					zoomFov = 160;
				}
			}
			else
			{
				zoomFov = lastfov;
			}

			// do smooth transitions for the binocs
			if(cg.zoomedBinoc)
			{					// binoc zooming in
				f = (cg.time - cg.zoomTime) / (float)ZOOM_TIME;
				if(f > 1.0)
				{
					fov_x = zoomFov;
				}
				else
				{
					fov_x = fov_x + f * (zoomFov - fov_x);
				}
				lastfov = fov_x;
			}
			else if(cg.zoomval)
			{					// zoomed by sniper/snooper
				fov_x = cg.zoomval;
				lastfov = fov_x;
			}
			else
			{					// binoc zooming out
				f = (cg.time - cg.zoomTime) / (float)ZOOM_TIME;
				if(f < 1.0)
				{
					fov_x = zoomFov + f * (fov_x - zoomFov);
				}
			}
		}
	}

	cg.refdef_current->rdflags &= ~RDF_SNOOPERVIEW;

	// Arnout: mg42 zoom
	if(cg.snap->ps.persistant[PERS_HWEAPON_USE])
	{
		fov_x = 55;
	}
	else if(cg.snap->ps.weapon == WP_MOBILE_MG42_SET)
	{
		fov_x = 55;
	}
	else if(cg.snap->ps.eFlags & EF_MOUNTEDTANK)
	{
		fov_x = 75;
	}

	// CHRUKER: b088 - Players field of view at intermission is different if you had deployed a MG42
	if ( cg.predictedPlayerState.pm_type == PM_INTERMISSION )
	{
		// if in intermission, use a fixed value
		fov_x = 90;
	} // b088

	if(cg.showGameView)
	{
		fov_x = fov_y = 60.f;
	}

	// Arnout: this is weird... (but ensures square pixel ratio!)
	x = cg.refdef_current->width / tan(fov_x / 360 * M_PI);
	fov_y = atan2(cg.refdef_current->height, x);
	fov_y = fov_y * 360 / M_PI;
	// And this seems better - but isn't really
	//fov_y = fov_x / cgs.glconfig.windowAspect;

	// warp if underwater
	//if ( cg_pmove.waterlevel == 3 ) {
	contents = CG_PointContents(cg.refdef.vieworg, -1);
	if(contents & (CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA))
	{
		phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
		v = WAVE_AMPLITUDE * sin(phase);
		fov_x += v;
		fov_y -= v;
		inwater = qtrue;
		cg.refdef_current->rdflags |= RDF_UNDERWATER;
	}
	else
	{
		cg.refdef_current->rdflags &= ~RDF_UNDERWATER;
		inwater = qfalse;
	}

	// set it
	cg.refdef_current->fov_x = fov_x;
	cg.refdef_current->fov_y = fov_y;

/*
	if( cg.predictedPlayerState.eFlags & EF_PRONE ) {
		cg.zoomSensitivity = cg.refdef.fov_y / 500.0;
	} else
*/
	// rain - allow freelook when dead until we tap out into limbo
	if(cg.snap->ps.pm_type == PM_FREEZE || (cg.snap->ps.pm_type == PM_DEAD && (cg.snap->ps.pm_flags & PMF_LIMBO)) ||
	   cg.snap->ps.pm_flags & PMF_TIME_LOCKPLAYER)
	{
		// No movement for pauses
		cg.zoomSensitivity = 0;
	}
	else if(!cg.zoomedBinoc)
	{
		// NERVE - SMF - fix for zoomed in/out movement bug
		if(cg.zoomval)
		{
			cg.zoomSensitivity = 0.6 * (cg.zoomval / 90.f);	// NERVE - SMF - changed to get less sensitive as you zoom in
//              cg.zoomSensitivity = 0.1;
		}
		else
		{
			cg.zoomSensitivity = 1;
		}
		// -NERVE - SMF
	}
	else
	{
		cg.zoomSensitivity = cg.refdef_current->fov_y / 75.0;
	}

	return inwater;
}

/*
===============
CG_WaterBlendBlob
===============
*/

#define MAX_DROPS 20
#define WATER_OUT_TIME 1500
#define WATER_IN_TIME 150

int num_drops = 0;
float dropx[MAX_DROPS];
float dropy[MAX_DROPS];
float drops[MAX_DROPS];
qhandle_t dropShader[MAX_DROPS];

qboolean waswater = qfalse;
int watertime = 0;
int waterintime = 0;

float dropAccel[MAX_DROPS][2];

static void CG_WaterBlendBlob( int inwater ) {
	int i, t;
	float phase, v, in;
	refEntity_t ent;

    if ( inwater ){
        if (!waswater) // new in water
        waterintime = cg.time + WATER_IN_TIME;

        waswater = qtrue;

        if(!num_drops)
        {
            for( i = 0; i < MAX_DROPS; i ++)
            {
                dropx[i] = 8 * crandom();
                dropy[i] = 8 * crandom();
                drops[i] = 6 + crandom() * 2;
                dropShader[i] = trap_R_RegisterShader(va("hud/drop%i", ((int) ( 2 + 2 * crandom ()))));

                dropAccel[i][0] = 0.0f;
                dropAccel[i][1] = 0.0f;

                num_drops++;
            }
        }
    }


    // when coming out of water, draw drops
    if(!inwater && waswater) // init the drops
    {
        watertime = cg.time + WATER_OUT_TIME;
        waswater = qfalse;
    }

	t = watertime - cg.time;

	if(t <= 0 && !inwater) {
		num_drops = 0;
		return;
	}

	if(!num_drops) {
		return;
	}

    phase = cg.time / 1000.0 * WAVE_FREQUENCY * M_PI * 2;
    if( cg.time < waterintime) {
        in = 1.0f - ( 1.0f / WATER_IN_TIME * ( waterintime - cg.time ) );
	} else {
        in = 1.0f;
	}

	v = ( 0.7f + 0.1f * sin(phase) ) * in;

    // do the drops
    for( i = 0; i < MAX_DROPS; i ++) {
        memset(&ent, 0, sizeof(ent));
        ent.reType = RT_SPRITE;
        ent.renderfx = RF_FIRST_PERSON;

        dropx[i] += dropAccel[i][0];
		dropy[i] += dropAccel[i][1];

        VectorMA(cg.refdef.vieworg, 8, cg.refdef.viewaxis[0], ent.origin);
        VectorMA(ent.origin, dropx[i] + sin(phase * 0.1f ), cg.refdef.viewaxis[1], ent.origin);
        VectorMA(ent.origin, dropy[i] + cos(phase * 0.1f ), cg.refdef.viewaxis[2], ent.origin);

        ent.customShader = dropShader[i];

        if(inwater) {
            ent.shaderTime = v;
		} else {
            ent.shaderTime = v / WATER_OUT_TIME * t;
		}

        ent.radius = drops[i] * ( 0.2f + ent.shaderTime );

        trap_R_AddRefEntityToScene(&ent);
	}

}



/*
==============
CG_UnderwaterSounds
==============
*/
#define UNDERWATER_BIT 16
static void CG_UnderwaterSounds(void)
{
//  trap_S_AddLoopingSound( cent->lerpOrigin, vec3_origin, cgs.media.underWaterSound, 255, 0 );
	//trap_S_AddLoopingSound(cg.snap->ps.origin, vec3_origin, cgs.media.underWaterSound, 255 | (1 << UNDERWATER_BIT), 0);
}


/*
===============
CG_DamageBlendBlob

===============
*/
static void CG_DamageBlendBlob(void)
{
	int             t, i;
	int             maxTime;
	refEntity_t     ent;
	qboolean        pointDamage;
	viewDamage_t   *vd;
	float           redFlash;

	// Gordon: no damage blend blobs if in limbo or spectator, and in the limbo menu
	if((cg.snap->ps.pm_flags & PMF_LIMBO || cgs.clientinfo[cg.clientNum].team == TEAM_SPECTATOR) && cg.showGameView)
	{
		return;
	}

	// ragePro systems can't fade blends, so don't obscure the screen
	if(cgs.glconfig.hardwareType == GLHW_RAGEPRO)
	{
		return;
	}

	redFlash = 0;

	for(i = 0; i < MAX_VIEWDAMAGE; i++)
	{
		vd = &cg.viewDamage[i];

		if(!vd->damageValue)
		{
			continue;
		}

		maxTime = vd->damageDuration;
		t = cg.time - vd->damageTime;
		if(t <= 0 || t >= maxTime)
		{
			vd->damageValue = 0;
			continue;
		}

		pointDamage = !(!vd->damageX && !vd->damageY);

		// if not point Damage, only do flash blend
		if(!pointDamage)
		{
			redFlash += 10.0 * (1.0 - (float)t / maxTime);
			continue;
		}

		memset(&ent, 0, sizeof(ent));
		ent.reType = RT_SPRITE;
		ent.renderfx = RF_FIRST_PERSON;

		VectorMA(cg.refdef_current->vieworg, 8, cg.refdef_current->viewaxis[0], ent.origin);
		VectorMA(ent.origin, vd->damageX * -8, cg.refdef_current->viewaxis[1], ent.origin);
		VectorMA(ent.origin, vd->damageY * 8, cg.refdef_current->viewaxis[2], ent.origin);

		ent.radius = vd->damageValue * 0.4 * (0.5 + 0.5 * (float)t / maxTime) * (0.75 + 0.5 * fabs(sin(vd->damageTime)));

		ent.customShader = cgs.media.viewBloodAni[(int)(floor(((float)t / maxTime) * 4.9))];	//cgs.media.viewBloodShader;
		ent.shaderRGBA[0] = 255;
		ent.shaderRGBA[1] = 255;
		ent.shaderRGBA[2] = 255;
		ent.shaderRGBA[3] = 255 * ((cg_bloodDamageBlend.value > 1.0f) ? 1.0f :
								   (cg_bloodDamageBlend.value < 0.0f) ? 0.0f : cg_bloodDamageBlend.value);

		trap_R_AddRefEntityToScene(&ent);

		redFlash += ent.radius;
	}
}

/*
===============
CG_DrawScreenFade
===============
*/
static void CG_DrawScreenFade(void)
{
/* moved over to cg_draw.c
	static int lastTime;
	int elapsed, time;
	refEntity_t		ent;

	if (cgs.fadeStartTime + cgs.fadeDuration < cg.time) {
		cgs.fadeAlphaCurrent = cgs.fadeAlpha;
	} else if (cgs.fadeAlphaCurrent != cgs.fadeAlpha) {
		elapsed = (time = trap_Milliseconds()) - lastTime;	// we need to use trap_Milliseconds() here since the cg.time gets modified upon reloading
		lastTime = time;
		if (elapsed < 500 && elapsed > 0) {
			if (cgs.fadeAlphaCurrent > cgs.fadeAlpha) {
				cgs.fadeAlphaCurrent -= ((float)elapsed/(float)cgs.fadeDuration);
				if (cgs.fadeAlphaCurrent < cgs.fadeAlpha)
					cgs.fadeAlphaCurrent = cgs.fadeAlpha;
			} else {
				cgs.fadeAlphaCurrent += ((float)elapsed/(float)cgs.fadeDuration);
				if (cgs.fadeAlphaCurrent > cgs.fadeAlpha)
					cgs.fadeAlphaCurrent = cgs.fadeAlpha;
			}
		}
	}
	// now draw the fade
	if (cgs.fadeAlphaCurrent > 0.0) {
		memset( &ent, 0, sizeof( ent ) );
		ent.reType = RT_SPRITE;
		ent.renderfx = RF_FIRST_PERSON;

		VectorMA( cg.refdef_current->vieworg, 8, cg.refdef_current->viewaxis[0], ent.origin );
		ent.radius = 80;	// occupy entire screen
		ent.customShader = cgs.media.viewFadeBlack;
		ent.shaderRGBA[3] = (int)(255.0 * cgs.fadeAlphaCurrent);

		trap_R_AddRefEntityToScene( &ent );
	}
*/
}

/*
===============
CG_CalcViewValues

Sets cg.refdef view values
===============
*/
int CG_CalcViewValues(void)
{
	playerState_t  *ps;

	memset(cg.refdef_current, 0, sizeof(cg.refdef));

	// strings for in game rendering
	// Q_strncpyz( cg.refdef.text[0], "Park Ranger", sizeof(cg.refdef_current->text[0]) );
	// Q_strncpyz( cg.refdef.text[1], "19", sizeof(cg.refdef_current->text[1]) );

	// calculate size of 3D view
	CG_CalcVrect();

	ps = &cg.predictedPlayerState;

	if(cg.cameraMode)
	{
		vec3_t          origin, angles;
		float           fov = 90;
		float           x;

		if(trap_getCameraInfo(CAM_PRIMARY, cg.time, &origin, &angles, &fov))
		{
			VectorCopy(origin, cg.refdef_current->vieworg);
			angles[ROLL] = 0;
			angles[PITCH] = -angles[PITCH];	// (SA) compensate for reversed pitch (this makes the game match the editor, however I'm guessing the real fix is to be done there)
			VectorCopy(angles, cg.refdefViewAngles);
			AnglesToAxis(cg.refdefViewAngles, cg.refdef_current->viewaxis);

			x = cg.refdef.width / tan(fov / 360 * M_PI);
			cg.refdef_current->fov_y = atan2(cg.refdef_current->height, x);
			cg.refdef_current->fov_y = cg.refdef_current->fov_y * 360 / M_PI;
			cg.refdef_current->fov_x = fov;

			// FIXME: this is really really bad
			trap_SendClientCommand(va("setCameraOrigin %f %f %f", origin[0], origin[1], origin[2]));
			return 0;

		}
		else
		{
			cg.cameraMode = qfalse;
			trap_Cvar_Set("cg_letterbox", "0");
			trap_SendClientCommand("stopCamera");
			trap_stopCamera(CAM_PRIMARY);	// camera off in client

			CG_Fade(0, 0, 0, 255, 0, 0);	// go black
			CG_Fade(0, 0, 0, 0, cg.time + 200, 1500);	// then fadeup
		}
	}

	// intermission view
	if(ps->pm_type == PM_INTERMISSION)
	{
		VectorCopy(ps->origin, cg.refdef_current->vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
		AnglesToAxis(cg.refdefViewAngles, cg.refdef_current->viewaxis);
		return CG_CalcFov();
	}

	if(cg.bobfracsin > 0 && !ps->bobCycle)
	{
		cg.lastvalidBobcycle = cg.bobcycle;
		cg.lastvalidBobfracsin = cg.bobfracsin;
	}

	cg.bobcycle = (ps->bobCycle & 128) >> 7;
	cg.bobfracsin = fabs(sin((ps->bobCycle & 127) / 127.0 * M_PI));
	cg.xyspeed = sqrt(ps->velocity[0] * ps->velocity[0] + ps->velocity[1] * ps->velocity[1]);


	if(cg.showGameView)
	{
		VectorCopy(cgs.ccPortalPos, cg.refdef_current->vieworg);
		if(cg.showGameView && cgs.ccPortalEnt != -1)
		{
			vec3_t          vec;

			VectorSubtract(cg_entities[cgs.ccPortalEnt].lerpOrigin, cg.refdef_current->vieworg, vec);
			vectoangles(vec, cg.refdefViewAngles);
		}
		else
		{
			VectorCopy(cgs.ccPortalAngles, cg.refdefViewAngles);
		}
	}
	else if(cg.renderingThirdPerson && (ps->eFlags & EF_MG42_ACTIVE || ps->eFlags & EF_AAGUN_ACTIVE))
	{							// Arnout: see if we're attached to a gun
		centity_t      *mg42 = &cg_entities[ps->viewlocked_entNum];
		vec3_t          forward;

		AngleVectors(ps->viewangles, forward, NULL, NULL);
		VectorMA(mg42->currentState.pos.trBase, -36, forward, cg.refdef_current->vieworg);
		cg.refdef_current->vieworg[2] = ps->origin[2];
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
	}
	else if(ps->eFlags & EF_MOUNTEDTANK)
	{
		centity_t      *tank = &cg_entities[cg_entities[cg.snap->ps.clientNum].tagParent];

		VectorCopy(tank->mountedMG42Player.origin, cg.refdef_current->vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
	}
	else
	{
		VectorCopy(ps->origin, cg.refdef_current->vieworg);
		VectorCopy(ps->viewangles, cg.refdefViewAngles);
	}

	if(!cg.showGameView)
	{
		// add error decay
		if(cg_errorDecay.value > 0)
		{
			int             t;
			float           f;

			t = cg.time - cg.predictedErrorTime;
			f = (cg_errorDecay.value - t) / cg_errorDecay.value;
			if(f > 0 && f < 1)
			{
				VectorMA(cg.refdef_current->vieworg, f, cg.predictedError, cg.refdef_current->vieworg);
			}
			else
			{
				cg.predictedErrorTime = 0;
			}
		}

		// Ridah, lock the viewangles if the game has told us to
		if(ps->viewlocked)
		{

			/*
			   if (ps->viewlocked == 4)
			   {
			   centity_t *tent;
			   tent = &cg_entities[ps->viewlocked_entNum];
			   VectorCopy (tent->currentState.apos.trBase, cg.refdefViewAngles);
			   }
			   else
			 */
			// DHM - Nerve :: don't bother evaluating if set to 7 (look at medic)
			if(ps->viewlocked != 7 && ps->viewlocked != 3 && ps->viewlocked != 2)
			{
				BG_EvaluateTrajectory(&cg_entities[ps->viewlocked_entNum].currentState.apos, cg.time, cg.refdefViewAngles, qtrue,
									  cg_entities[ps->viewlocked_entNum].currentState.effect2Time);
			}

			if(ps->viewlocked == 2)
			{
				cg.refdefViewAngles[0] += crandom();
				cg.refdefViewAngles[1] += crandom();
			}
		}

		if(cg.renderingThirdPerson)
		{
			// back away from character
			CG_OffsetThirdPersonView();
		}
		else
		{

			// offset for local bobbing and kicks
			CG_OffsetFirstPersonView();

			if(cg.editingSpeakers)
			{
				CG_SetViewanglesForSpeakerEditor();
			}
		}

		// Ridah, lock the viewangles if the game has told us to
		if(ps->viewlocked == 7)
		{
			centity_t      *tent;
			vec3_t          vec;

			tent = &cg_entities[ps->viewlocked_entNum];
			VectorCopy(tent->lerpOrigin, vec);
			VectorSubtract(vec, cg.refdef_current->vieworg, vec);
			vectoangles(vec, cg.refdefViewAngles);
		}
		else if(ps->viewlocked == 4)
		{
			vec3_t          fwd;

			AngleVectors(cg.refdefViewAngles, fwd, NULL, NULL);
			VectorMA(cg_entities[ps->viewlocked_entNum].lerpOrigin, 16, fwd, cg.refdef_current->vieworg);
		}
		else if(ps->viewlocked)
		{
			vec3_t          fwd;
			float           oldZ;

			// set our position to be behind it
			oldZ = cg.refdef_current->vieworg[2];
			AngleVectors(cg.refdefViewAngles, fwd, NULL, NULL);
			if(cg.predictedPlayerState.eFlags & EF_AAGUN_ACTIVE)
			{
				VectorMA(cg_entities[ps->viewlocked_entNum].lerpOrigin, 0, fwd, cg.refdef_current->vieworg);
			}
			else
			{
				VectorMA(cg_entities[ps->viewlocked_entNum].lerpOrigin, -34, fwd, cg.refdef_current->vieworg);
			}
			cg.refdef_current->vieworg[2] = oldZ;
		}
		// done.
	}

	// position eye reletive to origin
	AnglesToAxis(cg.refdefViewAngles, cg.refdef_current->viewaxis);

	if(cg.hyperspace)
	{
		cg.refdef.rdflags |= RDF_NOWORLDMODEL | RDF_HYPERSPACE;
	}

	// field of view
	return CG_CalcFov();
}


//=========================================================================

char           *CG_MustParse(char **pString, const char *pErrorMsg)
{
	char           *token = COM_Parse(pString);

	if(!*token)
	{
		CG_Error("%s", pErrorMsg);
	}
	return token;
}

void CG_ParseSkyBox(void)
{
	int             fogStart, fogEnd;
	char           *cstr, *token;
	vec4_t          fogColor;

	cstr = (char *)CG_ConfigString(CS_SKYBOXORG);

	if(!*cstr)
	{
		cg.skyboxEnabled = qfalse;
		return;
	}

	token = CG_MustParse(&cstr, "CG_ParseSkyBox: error parsing skybox configstring\n");
	cg.skyboxViewOrg[0] = atof(token);

	token = CG_MustParse(&cstr, "CG_ParseSkyBox: error parsing skybox configstring\n");
	cg.skyboxViewOrg[1] = atof(token);

	token = CG_MustParse(&cstr, "CG_ParseSkyBox: error parsing skybox configstring\n");
	cg.skyboxViewOrg[2] = atof(token);

	token = CG_MustParse(&cstr, "CG_ParseSkyBox: error parsing skybox configstring\n");
	cg.skyboxViewFov = atoi(token);

	if(!cg.skyboxViewFov)
	{
		cg.skyboxViewFov = 90;
	}

	// setup fog the first time, ignore this part of the configstring after that
	token = CG_MustParse(&cstr, "CG_ParseSkyBox: error parsing skybox configstring.  No fog state\n");
	if(atoi(token))
	{							// this camera has fog
		token = CG_MustParse(&cstr, "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[0]\n");
		fogColor[0] = atof(token);

		token = CG_MustParse(&cstr, "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[1]\n");
		fogColor[1] = atof(token);

		token = CG_MustParse(&cstr, "CG_DrawSkyBoxPortal: error parsing skybox configstring.  No fog[2]\n");
		fogColor[2] = atof(token);

		token = COM_ParseExt(&cstr, qfalse);
		fogStart = atoi(token);

		token = COM_ParseExt(&cstr, qfalse);
		fogEnd = atoi(token);

		trap_R_SetFog(FOG_PORTALVIEW, fogStart, fogEnd, fogColor[0], fogColor[1], fogColor[2], 1.1f);
	}
	else
	{
		trap_R_SetFog(FOG_PORTALVIEW, 0, 0, 0, 0, 0, 0);	// init to null
	}

	cg.skyboxEnabled = qtrue;
}

/*
==============
CG_ParseTagConnects
==============
*/

void CG_ParseTagConnects(void)
{
	int             i;

	for(i = CS_TAGCONNECTS; i < CS_TAGCONNECTS + MAX_TAGCONNECTS; i++)
	{
		CG_ParseTagConnect(i);
	}
}

void CG_ParseTagConnect(int tagNum)
{
	char           *token, *pString = (char *)CG_ConfigString(tagNum);	// Gordon: bleh, i hate that cast away of the const
	int             entNum;

	if(!*pString)
	{
		return;
	}

	token = CG_MustParse(&pString, "Invalid TAGCONNECT configstring\n");

	entNum = atoi(token);
	if(entNum < 0 || entNum >= MAX_GENTITIES)
	{
		CG_Error("Invalid TAGCONNECT entitynum\n");
	}

	token = CG_MustParse(&pString, "Invalid TAGCONNECT configstring\n");

	cg_entities[entNum].tagParent = atoi(token);
	if(cg_entities[entNum].tagParent < 0 || cg_entities[entNum].tagParent >= MAX_GENTITIES)
	{
		CG_Error("Invalid TAGCONNECT tagparent\n");
	}

	token = CG_MustParse(&pString, "Invalid TAGCONNECT configstring\n");
	Q_strncpyz(cg_entities[entNum].tagName, token, MAX_QPATH);
}

/*
==============
CG_DrawSkyBoxPortal
==============
*/
void CG_DrawSkyBoxPortal(qboolean fLocalView)
{
	refdef_t        rd;
	static float    lastfov = 90;	// for transitions back from zoomed in modes


	if(!cg_skybox.integer || !cg.skyboxEnabled)
	{
		return;
	}

	memcpy(&rd, cg.refdef_current, sizeof(refdef_t));
	VectorCopy(cg.skyboxViewOrg, rd.vieworg);

// Updates for window views... remove me when things have been verified
#if 0
	fov_x = cg.skyboxViewFov;

	if(cg.predictedPlayerState.pm_type == PM_INTERMISSION)
	{
		// if in intermission, use a fixed value
		fov_x = 90;
	}
	else
	{
		// user selectable
		fov_x = cg_fov.value;
		if(fov_x < 1)
		{
			fov_x = 1;
		}
		else if(fov_x > 160)
		{
			fov_x = 160;
		}

		// account for zooms
		if(cg.zoomval)
		{
			zoomFov = cg.zoomval;	// (SA) use user scrolled amount

			if(zoomFov < 1)
			{
				zoomFov = 1;
			}
			else if(zoomFov > 160)
			{
				zoomFov = 160;
			}
		}
		else
		{
			zoomFov = lastfov;
		}

		// do smooth transitions for the binocs
		if(cg.zoomedBinoc)
		{						// binoc zooming in
			f = (cg.time - cg.zoomTime) / (float)ZOOM_TIME;
			if(f > 1.0)
			{
				fov_x = zoomFov;
			}
			else
			{
				fov_x = fov_x + f * (zoomFov - fov_x);
			}
			lastfov = fov_x;
		}
		else if(cg.zoomval)
		{						// zoomed by sniper/snooper
			fov_x = cg.zoomval;
			lastfov = fov_x;
		}
		else
		{						// binoc zooming out
			f = (cg.time - cg.zoomTime) / (float)ZOOM_TIME;
			if(f > 1.0)
			{
				fov_x = fov_x;
			}
			else
			{
				fov_x = zoomFov + f * (fov_x - zoomFov);
			}
		}
	}

	cg.refdef_current->rdflags &= ~RDF_SNOOPERVIEW;

	// Arnout: mg42 zoom
	if(cg.snap->ps.persistant[PERS_HWEAPON_USE] || cg.predictedPlayerState.pm_flags & PMF_PRONE_BIPOD)
	{
		fov_x = 55;
	}

	cg.refdef_current->time = cg.time;

	x = cg.refdef_current->width / tan(fov_x / 360 * M_PI);
	fov_y = atan2(cg.refdef_current->height, x);
	fov_y = fov_y * 360 / M_PI;

	cg.refdef_current->fov_x = fov_x;
	cg.refdef_current->fov_y = fov_y;

	cg.refdef_current->rdflags |= RDF_SKYBOXPORTAL;

	// draw the skybox
	trap_R_RenderScene(cg.refdef_current);

	cg.refdef = backuprefdef;
#endif

	if(fLocalView)
	{
		float           fov_x;
		float           fov_y;
		float           x;
		float           zoomFov;
		float           f;

		if(cg.predictedPlayerState.pm_type == PM_INTERMISSION)
		{
			// if in intermission, use a fixed value
			fov_x = 90;
		}
		else
		{
			// user selectable
			fov_x = cg_fov.value;
			if(fov_x < 1)
			{
				fov_x = 1;
			}
			else if(fov_x > 160)
			{
				fov_x = 160;
			}

			// account for zooms
			if(cg.zoomval)
			{
				zoomFov = cg.zoomval;	// (SA) use user scrolled amount
				if(zoomFov < 1)
				{
					zoomFov = 1;
				}
				else if(zoomFov > 160)
				{
					zoomFov = 160;
				}
			}
			else
			{
				zoomFov = lastfov;
			}

			// do smooth transitions for the binocs
			if(cg.zoomedBinoc)
			{					// binoc zooming in
				f = (cg.time - cg.zoomTime) / (float)ZOOM_TIME;
				fov_x = (f > 1.0) ? zoomFov : fov_x + f * (zoomFov - fov_x);
				lastfov = fov_x;
			}
			else if(cg.zoomval)
			{					// zoomed by sniper/snooper
				fov_x = cg.zoomval;
				lastfov = fov_x;
			}
			else
			{					// binoc zooming out
				f = (cg.time - cg.zoomTime) / (float)ZOOM_TIME;
				fov_x = (f > 1.0) ? fov_x : zoomFov + f * (fov_x - zoomFov);
			}
		}

		rd.rdflags &= ~RDF_SNOOPERVIEW;

		if(BG_PlayerMounted(cg.snap->ps.eFlags) || cg.predictedPlayerState.weapon == WP_MOBILE_MG42_SET)
		{
			fov_x = 55;
		}

		x = rd.width / tan(fov_x / 360 * M_PI);
		fov_y = atan2(rd.height, x);
		fov_y = fov_y * 360 / M_PI;

		rd.fov_x = fov_x;
		rd.fov_y = fov_y;
	}

	rd.time = cg.time;
	rd.rdflags |= RDF_SKYBOXPORTAL;

	// draw the skybox
	trap_R_RenderScene(&rd);
}

//=========================================================================

/*
**  Frustum code
*/

// some culling bits
typedef struct plane_s
{
	vec3_t          normal;
	float           dist;
} plane_t;

static plane_t  frustum[4];

//
//  CG_SetupFrustum
//
void CG_SetupFrustum(void)
{
	int             i;
	float           xs, xc;
	float           ang;

	ang = cg.refdef_current->fov_x / 180 * M_PI * 0.5f;
	xs = sin(ang);
	xc = cos(ang);

	VectorScale(cg.refdef_current->viewaxis[0], xs, frustum[0].normal);
	VectorMA(frustum[0].normal, xc, cg.refdef_current->viewaxis[1], frustum[0].normal);

	VectorScale(cg.refdef_current->viewaxis[0], xs, frustum[1].normal);
	VectorMA(frustum[1].normal, -xc, cg.refdef_current->viewaxis[1], frustum[1].normal);

	ang = cg.refdef.fov_y / 180 * M_PI * 0.5f;
	xs = sin(ang);
	xc = cos(ang);

	VectorScale(cg.refdef_current->viewaxis[0], xs, frustum[2].normal);
	VectorMA(frustum[2].normal, xc, cg.refdef_current->viewaxis[2], frustum[2].normal);

	VectorScale(cg.refdef_current->viewaxis[0], xs, frustum[3].normal);
	VectorMA(frustum[3].normal, -xc, cg.refdef_current->viewaxis[2], frustum[3].normal);

	for(i = 0; i < 4; i++)
	{
		frustum[i].dist = DotProduct(cg.refdef_current->vieworg, frustum[i].normal);
	}
}

//
//  CG_CullPoint - returns true if culled
//
qboolean CG_CullPoint(vec3_t pt)
{
	int             i;
	plane_t        *frust;

	// check against frustum planes
	for(i = 0; i < 4; i++)
	{
		frust = &frustum[i];

		if((DotProduct(pt, frust->normal) - frust->dist) < 0)
		{
			return (qtrue);
		}
	}

	return (qfalse);
}

qboolean CG_CullPointAndRadius(const vec3_t pt, vec_t radius)
{
	int             i;
	plane_t        *frust;

	// check against frustum planes
	for(i = 0; i < 4; i++)
	{
		frust = &frustum[i];

		if((DotProduct(pt, frust->normal) - frust->dist) < -radius)
		{
			return (qtrue);
		}
	}

	return (qfalse);
}

//=========================================================================

extern void     CG_SetupDlightstyles(void);


//#define DEBUGTIME_ENABLED
#ifdef DEBUGTIME_ENABLED
#define DEBUGTIME elapsed = ( trap_Milliseconds() - dbgTime ); if ( dbgCnt++ == 1 ) {CG_Printf( "t%i:%i ", dbgCnt, elapsed = ( trap_Milliseconds() - dbgTime ) ); } dbgTime += elapsed;
#else
#define DEBUGTIME
#endif

#ifdef _DEBUG
//#define FAKELAG
#ifdef FAKELAG
extern int      snapshotDelayTime;
#endif							// FAKELAG
#endif							// _DEBUG

/*
=================
CG_DrawActiveFrame

Generates and draws a game scene and status information at the given time.
=================
*/

//static int lightningtime = 0;
//static int lightningsequencetime = 0;
//static int lightningsequencecounter = 0;

qboolean        CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle);

void CG_DrawActiveFrame(int serverTime, stereoFrame_t stereoView, qboolean demoPlayback)
{
	int             inwater;

#ifdef DEBUGTIME_ENABLED
	int             dbgTime = trap_Milliseconds(), elapsed;
	int             dbgCnt = 0;
#endif

	cg.time = serverTime;
	cgDC.realTime = cg.time;
	cg.demoPlayback = demoPlayback;

#ifdef FAKELAG
	cg.time -= snapshotDelayTime;
#endif							// _DEBUG


#ifdef DEBUGTIME_ENABLED
	CG_Printf("\n");
#endif
	DEBUGTIME
		// update cvars
		CG_UpdateCvars();

	DEBUGTIME
		// if we are only updating the screen as a loading
		// pacifier, don't even try to read snapshots
		if(cg.infoScreenText[0] != 0)
	{
		CG_DrawInformation(qfalse);
		return;
	}

	CG_PB_ClearPolyBuffers();

	CG_UpdatePMLists();

	// any looped sounds will be respecified as entities
	// are added to the render list
	trap_S_ClearLoopingSounds(qfalse);

	CG_UpdateBufferedSoundScripts();

	DEBUGTIME
		// set up cg.snap and possibly cg.nextSnap
		CG_ProcessSnapshots();

	DEBUGTIME
		// if we haven't received any snapshots yet, all
		// we can draw is the information screen
		if(!cg.snap || (cg.snap->snapFlags & SNAPFLAG_NOT_ACTIVE))
	{
		CG_DrawInformation(qfalse);
		return;
	}

	// check for server set weapons we might not know about
	// (FIXME: this is a hack for the time being since a scripted "selectweapon" does
	// not hit the first snap, the server weapon set in cg_playerstate.c line 219 doesn't
	// do the trick)
	if(!cg.weaponSelect && cg.snap->ps.weapon)
	{
		cg.weaponSelect = cg.snap->ps.weapon;
		cg.weaponSelectTime = cg.time;
	}

	if(cg.weaponSelect == WP_FG42SCOPE)
	{
		float           spd;

		spd = VectorLength(cg.snap->ps.velocity);
		if(spd > 180.0f)
		{
			CG_FinishWeaponChange(WP_FG42SCOPE, WP_FG42);
		}
	}

	DEBUGTIME if    (!cg.lightstylesInited)
	{
		CG_SetupDlightstyles();
	}

	DEBUGTIME
		// if we have been told not to render, don't
		if(cg_norender.integer)
	{
		return;
	}

	// this counter will be bumped for every valid scene we generate
	cg.clientFrame++;

	// update cg.predictedPlayerState
	CG_PredictPlayerState();

	DEBUGTIME
		// OSP -- MV handling
		if(cg.mvCurrentMainview != NULL && cg.snap->ps.pm_type != PM_INTERMISSION)
	{
		CG_mvDraw(cg.mvCurrentMainview);
		// FIXME: not valid for demo playback
		cg.zoomSensitivity = mv_sensitivity.value / int_sensitivity.value;
	}
	else
	{
		// clear all the render lists
		trap_R_ClearScene();

		DEBUGTIME
			// decide on third person view
			cg.renderingThirdPerson = cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0) || cg.showGameView;

		// build cg.refdef
		inwater = CG_CalcViewValues();
		CG_SetupFrustum();

		DEBUGTIME
			// RF, draw the skyboxportal
			CG_DrawSkyBoxPortal(qtrue);

		DEBUGTIME if    (inwater)
		{
			CG_UnderwaterSounds();
		}

		DEBUGTIME
			// build the render lists
			if(!cg.hyperspace)
		{
			CG_AddPacketEntities();	// adter calcViewValues, so predicted player state is correct
			CG_AddMarks();

			DEBUGTIME       CG_AddScriptSpeakers();

			DEBUGTIME
				// Rafael particles
				CG_AddParticles();
			// done.

			DEBUGTIME       CG_AddLocalEntities();

			DEBUGTIME       CG_AddSmokeSprites();

			DEBUGTIME       CG_AddAtmosphericEffects();
		}

		// Rafael mg42
		if(!cg.showGameView && !cgs.dbShowing)
		{
			if(!cg.snap->ps.persistant[PERS_HWEAPON_USE])
			{
				CG_AddViewWeapon(&cg.predictedPlayerState);
			}
			else
			{
				if(cg.time - cg.predictedPlayerEntity.overheatTime < 3000)
				{
					vec3_t          muzzle;

					CG_CalcMuzzlePoint(cg.snap->ps.clientNum, muzzle);

					muzzle[2] -= 32;

					if(!(rand() % 3))
					{
						float           alpha;

						alpha = 1.0f - ((float)(cg.time - cg.predictedPlayerEntity.overheatTime) / 3000.0f);
						alpha *= 0.25f;	// .25 max alpha
						CG_ParticleImpactSmokePuffExtended(cgs.media.smokeParticleShader, muzzle, 1000, 8, 20, 30, alpha, 8.f);
					}
				}
			}
		}

		// NERVE - SMF - play buffered voice chats
		CG_PlayBufferedVoiceChats();

		DEBUGTIME
			// Ridah, trails
			if(!cg.hyperspace)
		{
			CG_AddFlameChunks();
			CG_AddTrails();		// this must come last, so the trails dropped this frame get drawn
		}
		// done.

		DEBUGTIME
			// finish up the rest of the refdef
			if(cg.testModelEntity.hModel)
		{
			CG_AddTestModel();
		}
		
		DEBUGTIME
			// first person blend blobs, done after AnglesToAxis
			if(!cg.renderingThirdPerson) 
		{
			CG_DamageBlendBlob();
			CG_WaterBlendBlob(inwater);
		}

		cg.refdef.time = cg.time;
		memcpy(cg.refdef.areamask, cg.snap->areamask, sizeof(cg.refdef.areamask));

		DEBUGTIME
			// warning sounds when powerup is wearing off
			//CG_PowerupTimerSounds();
			// make sure the lagometerSample and frame timing isn't done twice when in stereo
			if(stereoView != STEREO_RIGHT)
		{
			cg.frametime = cg.time - cg.oldTime;
			if(cg.frametime < 0)
			{
				cg.frametime = 0;
			}
			cg.oldTime = cg.time;
			CG_AddLagometerFrameInfo();
		}

		DEBUGTIME
			// Ridah, fade the screen
			CG_DrawScreenFade();

		DEBUGTIME
			// DHM - Nerve :: let client system know our predicted origin
			trap_SetClientLerpOrigin(cg.refdef.vieworg[0], cg.refdef.vieworg[1], cg.refdef.vieworg[2]);

		// actually issue the rendering calls
		CG_DrawActive(stereoView);

		DEBUGTIME
			// update audio positions
			trap_S_Respatialize(cg.snap->ps.clientNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater);
	}

	if(cg_stats.integer)
	{
		CG_Printf("cg.clientFrame:%i\n", cg.clientFrame);
	}

	DEBUGTIME
		// let the client system know what our weapon, holdable item and zoom settings are
		trap_SetUserCmdValue(cg.weaponSelect, cg.showGameView ? 0x01 : 0x00, cg.zoomSensitivity, cg.identifyClientRequest);
}
