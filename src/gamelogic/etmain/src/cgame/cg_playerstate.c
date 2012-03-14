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

// cg_playerstate.c -- this file acts on changes in a new playerState_t
// With normal play, this will be done after local prediction, but when
// following another player or playing back a demo, it will be checked
// when the snapshot transitions like all the other entities

#include "cg_local.h"

/*
==============
CG_CheckAmmo

If the ammo has gone low enough to generate the warning, play a sound
==============
*/
void CG_CheckAmmo ( void )
{
	int i;
	int total;
	int weapons[ MAX_WEAPONS / ( sizeof ( int ) * 8 ) ];

	// see about how many seconds of ammo we have remaining
	memcpy ( weapons, cg.snap->ps.weapons, sizeof ( weapons ) );

	if ( !weapons[ 0 ] && !weapons[ 1 ] )
	{
		// (SA) we start out with no weapons, so don't make a click on startup
		return;
	}

	total = 0;

	for ( i = 0; i < WP_NUM_WEAPONS; i++ )
	{
		if ( ! ( weapons[ 0 ] & ( 1 << i ) ) )
		{
			continue;
		}

		switch ( i )
		{
			case WP_PANZERFAUST:
			case WP_GRENADE_LAUNCHER:
			case WP_GRENADE_PINEAPPLE:
			case WP_LUGER:
			case WP_COLT:
			case WP_AKIMBO_COLT:
			case WP_AKIMBO_SILENCEDCOLT:
			case WP_AKIMBO_LUGER:
			case WP_AKIMBO_SILENCEDLUGER:
			case WP_SILENCER:
			case WP_MP40:
			case WP_THOMPSON:
			case WP_STEN:
			case WP_GARAND:
			case WP_FG42:
			case WP_FG42SCOPE:
			case WP_KAR98:
			case WP_GPG40:
			case WP_CARBINE:
			case WP_M7:
			case WP_MOBILE_MG42:
			case WP_MOBILE_MG42_SET:
			case WP_K43:
			case WP_MORTAR_SET:
			default:
				total += cg.snap->ps.ammo[ BG_FindAmmoForWeapon ( i ) ] * 1000;
				break;
//          default:
//              total += cg.snap->ps.ammo[BG_FindAmmoForWeapon(i)] * 200;
//              break;
		}

		if ( total >= 5000 )
		{
			cg.lowAmmoWarning = 0;
			return;
		}
	}

	if ( !cg.lowAmmoWarning )
	{
		// play a sound on this transition
		trap_S_StartLocalSound ( cgs.media.noAmmoSound, CHAN_LOCAL_SOUND );
	}

	if ( total == 0 )
	{
		cg.lowAmmoWarning = 2;
	}
	else
	{
		cg.lowAmmoWarning = 1;
	}
}

/*
==============
CG_DamageFeedback
==============
*/
void CG_DamageFeedback ( int yawByte, int pitchByte, int damage )
{
	float        left, front, up;
	float        kick;
	int          health;
	float        scale;
	vec3_t       dir;
	vec3_t       angles;
	float        dist;
	float        yaw, pitch;
	int          slot;
	viewDamage_t *vd;

	// show the attacking player's head and name in corner
	cg.attackerTime = cg.time;

	// the lower on health you are, the greater the view kick will be
	health = cg.snap->ps.stats[ STAT_HEALTH ];

	if ( health < 40 )
	{
		scale = 1;
	}
	else
	{
		scale = 40.0 / health;
	}

	kick = damage * scale;

	if ( kick < 5 )
	{
		kick = 5;
	}

	if ( kick > 10 )
	{
		kick = 10;
	}

	// find a free slot
	for ( slot = 0; slot < MAX_VIEWDAMAGE; slot++ )
	{
		if ( cg.viewDamage[ slot ].damageTime + cg.viewDamage[ slot ].damageDuration < cg.time )
		{
			break;
		}
	}

	if ( slot == MAX_VIEWDAMAGE )
	{
		return; // no free slots, never override or splats will suddenly disappear
	}

	vd = &cg.viewDamage[ slot ];

	// if yaw and pitch are both 255, make the damage always centered (falling, etc)
	if ( yawByte == 255 && pitchByte == 255 )
	{
		vd->damageX = 0;
		vd->damageY = 0;
		cg.v_dmg_roll = 0;
		cg.v_dmg_pitch = -kick;
	}
	else
	{
		// positional
		pitch = pitchByte / 255.0 * 360;
		yaw = yawByte / 255.0 * 360;

		angles[ PITCH ] = pitch;
		angles[ YAW ] = yaw;
		angles[ ROLL ] = 0;

		AngleVectors ( angles, dir, NULL, NULL );
		VectorSubtract ( vec3_origin, dir, dir );

		front = DotProduct ( dir, cg.refdef.viewaxis[ 0 ] );
		left = DotProduct ( dir, cg.refdef.viewaxis[ 1 ] );
		up = DotProduct ( dir, cg.refdef.viewaxis[ 2 ] );

		dir[ 0 ] = front;
		dir[ 1 ] = left;
		dir[ 2 ] = 0;
		dist = VectorLength ( dir );

		if ( dist < 0.1 )
		{
			dist = 0.1;
		}

		cg.v_dmg_roll = kick * left;

		cg.v_dmg_pitch = -kick * front;

		if ( front <= 0.1 )
		{
			front = 0.1;
		}

		vd->damageX = crandom() * 0.3 + -left / front;
		vd->damageY = crandom() * 0.3 + up / dist;
	}

	// clamp the position
	if ( vd->damageX > 1.0 )
	{
		vd->damageX = 1.0;
	}

	if ( vd->damageX < -1.0 )
	{
		vd->damageX = -1.0;
	}

	if ( vd->damageY > 1.0 )
	{
		vd->damageY = 1.0;
	}

	if ( vd->damageY < -1.0 )
	{
		vd->damageY = -1.0;
	}

	// don't let the screen flashes vary as much
	if ( kick > 10 )
	{
		kick = 10;
	}

	vd->damageValue = kick;
	cg.v_dmg_time = cg.time + DAMAGE_TIME;
	vd->damageTime = cg.snap->serverTime;
	vd->damageDuration = kick * 50 * ( 1 + 2 * ( !vd->damageX && !vd->damageY ) );
	cg.damageTime = cg.snap->serverTime;
	cg.damageIndex = slot;
}

/*
================
CG_Respawn

A respawn happened this snapshot
================
*/
void CG_Respawn ( qboolean revived )
{
	cg.serverRespawning = qfalse; // Arnout: just in case

	// no error decay on player movement
	cg.thisFrameTeleport = qtrue;

	// need to reset client-side weapon animations
	cg.predictedPlayerState.weapAnim = ( ( cg.predictedPlayerState.weapAnim & ANIM_TOGGLEBIT ) ^ ANIM_TOGGLEBIT ) | PM_IdleAnimForWeapon ( cg.snap->ps.weapon ); // reset weapon animations
	cg.predictedPlayerState.weaponstate = WEAPON_READY; // hmm, set this?  what to?

	// display weapons available
	cg.weaponSelectTime = cg.time;

	cg.cursorHintIcon = 0;
	cg.cursorHintTime = 0;

	cg.cameraMode = qfalse; //----(SA)  get out of camera for sure

	// select the weapon the server says we are using
	cg.weaponSelect = cg.snap->ps.weapon;
	// DHM - Nerve :: Clear even more things on respawn
	cg.zoomedBinoc = qfalse;
	cg.zoomedScope = qfalse;
	cg.zoomTime = 0;
	cg.zoomval = 0;

	trap_SendConsoleCommand ( "-zoom\n" );
	cg.binocZoomTime = 0;

	// clear pmext
	memset ( &cg.pmext, 0, sizeof ( cg.pmext ) );

	cg.pmext.bAutoReload = ( cg_autoReload.integer > 0 );

	cg.pmext.sprintTime = SPRINTTIME;

	if ( !revived )
	{
		cgs.limboLoadoutSelected = qfalse;
	}

	if ( cg.predictedPlayerState.stats[ STAT_PLAYER_CLASS ] == PC_COVERTOPS )
	{
		cg.pmext.silencedSideArm = 1;
	}

	cg.proneMovingTime = 0;

	// reset fog to world fog (if present)
	trap_R_SetFog ( FOG_CMD_SWITCHFOG, FOG_MAP, 20, 0, 0, 0, 0 );
	// dhm - end
}

extern char *eventnames[];

/*
==============
CG_CheckPlayerstateEvents
==============
*/
void CG_CheckPlayerstateEvents_wolf ( playerState_t *ps, playerState_t *ops )
{
	int       i;
	int       event;
	centity_t *cent;

	/*
	        if ( ps->externalEvent && ps->externalEvent != ops->externalEvent ) {
	                cent = &cg_entities[ ps->clientNum ];
	                cent->currentState.event = ps->externalEvent;
	                cent->currentState.eventParm = ps->externalEventParm;
	                CG_EntityEvent( cent, cent->lerpOrigin );
	        }
	*/
	cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];

	// go through the predictable events buffer
	for ( i = ps->eventSequence - MAX_EVENTS; i < ps->eventSequence; i++ )
	{
		if ( ps->events[ i & ( MAX_EVENTS - 1 ) ] != ops->events[ i & ( MAX_EVENTS - 1 ) ] || i >= ops->eventSequence )
		{
			event = ps->events[ i & ( MAX_EVENTS - 1 ) ];

			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[ i & ( MAX_EVENTS - 1 ) ];
			CG_EntityEvent ( cent, cent->lerpOrigin );
		}
	}
}

void CG_CheckPlayerstateEvents ( playerState_t *ps, playerState_t *ops )
{
	int       i;
	int       event;
	centity_t *cent;

	if ( ps->externalEvent && ps->externalEvent != ops->externalEvent )
	{
		cent = &cg_entities[ ps->clientNum ];
		cent->currentState.event = ps->externalEvent;
		cent->currentState.eventParm = ps->externalEventParm;
		CG_EntityEvent ( cent, cent->lerpOrigin );
	}

	cent = &cg.predictedPlayerEntity; // cg_entities[ ps->clientNum ];

	// go through the predictable events buffer
	for ( i = ps->eventSequence - MAX_EVENTS; i < ps->eventSequence; i++ )
	{
		// if we have a new predictable event
		if ( i >= ops->eventSequence
		     // or the server told us to play another event instead of a predicted event we already issued
		     // or something the server told us changed our prediction causing a different event
		     || ( i > ops->eventSequence - MAX_EVENTS && ps->events[ i & ( MAX_EVENTS - 1 ) ] != ops->events[ i & ( MAX_EVENTS - 1 ) ] ) )
		{
			event = ps->events[ i & ( MAX_EVENTS - 1 ) ];
			cent->currentState.event = event;
			cent->currentState.eventParm = ps->eventParms[ i & ( MAX_EVENTS - 1 ) ];
			CG_EntityEvent ( cent, cent->lerpOrigin );

			cg.predictableEvents[ i & ( MAX_PREDICTED_EVENTS - 1 ) ] = event;

			cg.eventSequence++;
		}
	}
}

/*
==================
CG_CheckChangedPredictableEvents
==================
*/
void CG_CheckChangedPredictableEvents ( playerState_t *ps )
{
	int       i;
	int       event;
	centity_t *cent;

	cent = &cg.predictedPlayerEntity;

	for ( i = ps->eventSequence - MAX_EVENTS; i < ps->eventSequence; i++ )
	{
		//
		if ( i >= cg.eventSequence )
		{
			continue;
		}

		// if this event is not further back in than the maximum predictable events we remember
		if ( i > cg.eventSequence - MAX_PREDICTED_EVENTS )
		{
			// if the new playerstate event is different from a previously predicted one
			if ( ps->events[ i & ( MAX_EVENTS - 1 ) ] != cg.predictableEvents[ i & ( MAX_PREDICTED_EVENTS - 1 ) ] )
			{
				event = ps->events[ i & ( MAX_EVENTS - 1 ) ];
				cent->currentState.event = event;
				cent->currentState.eventParm = ps->eventParms[ i & ( MAX_EVENTS - 1 ) ];
				CG_EntityEvent ( cent, cent->lerpOrigin );

				cg.predictableEvents[ i & ( MAX_PREDICTED_EVENTS - 1 ) ] = event;

				if ( cg_showmiss.integer )
				{
					CG_Printf ( "WARNING: changed predicted event\n" );
				}
			}
		}
	}
}

/*
==================
CG_CheckLocalSounds
==================
*/
void CG_CheckLocalSounds ( playerState_t *ps, playerState_t *ops )
{
	// health changes of more than -1 should make pain sounds
	if ( ps->stats[ STAT_HEALTH ] < ops->stats[ STAT_HEALTH ] - 1 )
	{
		if ( ps->stats[ STAT_HEALTH ] > 0 )
		{
			CG_PainEvent ( &cg.predictedPlayerEntity, ps->stats[ STAT_HEALTH ], qfalse );

			cg.painTime = cg.time;
		}
	}

	// timelimit warnings
	if ( cgs.timelimit > 0 && cgs.gamestate == GS_PLAYING )
	{
		int msec;

		msec = cg.time - cgs.levelStartTime;

		if ( cgs.timelimit > 5 && ! ( cg.timelimitWarnings & 1 ) && ( msec > ( cgs.timelimit - 5 ) * 60 * 1000 ) &&
		     ( msec < ( cgs.timelimit - 5 ) * 60 * 1000 + 1000 ) )
		{
			cg.timelimitWarnings |= 1;

			if ( ps->persistant[ PERS_TEAM ] == TEAM_AXIS )
			{
				if ( cgs.media.fiveMinuteSound_g == -1 )
				{
					CG_SoundPlaySoundScript ( cg.fiveMinuteSound_g, NULL, -1, qtrue );
				}
				else if ( cgs.media.fiveMinuteSound_g )
				{
					trap_S_StartLocalSound ( cgs.media.fiveMinuteSound_g, CHAN_ANNOUNCER );
				}
			}
			else if ( ps->persistant[ PERS_TEAM ] == TEAM_ALLIES )
			{
				if ( cgs.media.fiveMinuteSound_a == -1 )
				{
					CG_SoundPlaySoundScript ( cg.fiveMinuteSound_a, NULL, -1, qtrue );
				}
				else if ( cgs.media.fiveMinuteSound_a )
				{
					trap_S_StartLocalSound ( cgs.media.fiveMinuteSound_a, CHAN_ANNOUNCER );
				}
			}
		}

		if ( cgs.timelimit > 2 && ! ( cg.timelimitWarnings & 2 ) && ( msec > ( cgs.timelimit - 2 ) * 60 * 1000 ) &&
		     ( msec < ( cgs.timelimit - 2 ) * 60 * 1000 + 1000 ) )
		{
			cg.timelimitWarnings |= 2;

			if ( ps->persistant[ PERS_TEAM ] == TEAM_AXIS )
			{
				if ( cgs.media.twoMinuteSound_g == -1 )
				{
					CG_SoundPlaySoundScript ( cg.twoMinuteSound_g, NULL, -1, qtrue );
				}
				else if ( cgs.media.twoMinuteSound_g )
				{
					trap_S_StartLocalSound ( cgs.media.twoMinuteSound_g, CHAN_ANNOUNCER );
				}
			}
			else if ( ps->persistant[ PERS_TEAM ] == TEAM_ALLIES )
			{
				if ( cgs.media.twoMinuteSound_a == -1 )
				{
					CG_SoundPlaySoundScript ( cg.twoMinuteSound_a, NULL, -1, qtrue );
				}
				else if ( cgs.media.twoMinuteSound_a )
				{
					trap_S_StartLocalSound ( cgs.media.twoMinuteSound_a, CHAN_ANNOUNCER );
				}
			}
		}

		if ( ! ( cg.timelimitWarnings & 4 ) && ( msec > ( cgs.timelimit ) * 60 * 1000 - 30000 ) &&
		     ( msec < ( cgs.timelimit ) * 60 * 1000 - 29000 ) )
		{
			cg.timelimitWarnings |= 4;

			if ( ps->persistant[ PERS_TEAM ] == TEAM_AXIS )
			{
				if ( cgs.media.thirtySecondSound_g == -1 )
				{
					CG_SoundPlaySoundScript ( cg.thirtySecondSound_g, NULL, -1, qtrue );
				}
				else if ( cgs.media.thirtySecondSound_g )
				{
					trap_S_StartLocalSound ( cgs.media.thirtySecondSound_g, CHAN_ANNOUNCER );
				}
			}
			else if ( ps->persistant[ PERS_TEAM ] == TEAM_ALLIES )
			{
				if ( cgs.media.thirtySecondSound_a == -1 )
				{
					CG_SoundPlaySoundScript ( cg.thirtySecondSound_a, NULL, -1, qtrue );
				}
				else if ( cgs.media.thirtySecondSound_a )
				{
					trap_S_StartLocalSound ( cgs.media.thirtySecondSound_a, CHAN_ANNOUNCER );
				}
			}
		}
	}
}

/*
===============
CG_TransitionPlayerState

===============
*/
void CG_TransitionPlayerState ( playerState_t *ps, playerState_t *ops )
{
	// OSP - MV client handling
	if ( cg.mvTotalClients > 0 )
	{
		if ( ps->clientNum != ops->clientNum )
		{
			cg.thisFrameTeleport = qtrue;

			// clear voicechat
			cg.predictedPlayerEntity.voiceChatSpriteTime = 0; // CHECKME: should we do this here?
			cg_entities[ ps->clientNum ].voiceChatSpriteTime = 0;

			*ops = *ps;
		}

		CG_CheckLocalSounds ( ps, ops );
		return;
	}

	// check for changing follow mode
	if ( ps->clientNum != ops->clientNum )
	{
		cg.thisFrameTeleport = qtrue;

		// clear voicechat
		cg.predictedPlayerEntity.voiceChatSpriteTime = 0;
		cg_entities[ ps->clientNum ].voiceChatSpriteTime = 0;

		// make sure we don't get any unwanted transition effects
		*ops = *ps;

		// DHM - Nerve :: After Limbo, make sure and do a CG_Respawn
		if ( ps->clientNum == cg.clientNum )
		{
			ops->persistant[ PERS_SPAWN_COUNT ]--;
		}
	}

	if ( ps->eFlags & EF_FIRING )
	{
		cg.lastFiredWeaponTime = 0;
		cg.weaponFireTime += cg.frametime;
	}
	else
	{
		if ( cg.weaponFireTime > 500 && cg.weaponFireTime )
		{
			cg.lastFiredWeaponTime = cg.time;
		}

		cg.weaponFireTime = 0;
	}

	// damage events (player is getting wounded)
	if ( ps->damageEvent != ops->damageEvent && ps->damageCount )
	{
		CG_DamageFeedback ( ps->damageYaw, ps->damagePitch, ps->damageCount );
	}

	// respawning
	if ( ps->persistant[ PERS_SPAWN_COUNT ] != ops->persistant[ PERS_SPAWN_COUNT ] )
	{
		CG_Respawn ( ps->persistant[ PERS_REVIVE_COUNT ] != ops->persistant[ PERS_REVIVE_COUNT ] ? qtrue : qfalse );
	}

	if ( cg.mapRestart )
	{
		CG_Respawn ( qfalse );
		cg.mapRestart = qfalse;
	}

	if ( cg.snap->ps.pm_type != PM_INTERMISSION && ps->persistant[ PERS_TEAM ] != TEAM_SPECTATOR )
	{
		CG_CheckLocalSounds ( ps, ops );
	}

	// check for going low on ammo
	CG_CheckAmmo();

	if ( ps->eFlags & EF_PRONE_MOVING )
	{
		if ( ps->weapon == WP_BINOCULARS )
		{
			if ( ps->eFlags & EF_ZOOMING )
			{
				trap_SendConsoleCommand ( "-zoom\n" );
			}
		}

		if ( ! ( ops->eFlags & EF_PRONE_MOVING ) )
		{
			// ydnar: this screws up auto-switching when dynamite planted or grenade thrown/out of ammo
			//% CG_FinishWeaponChange( cg.weaponSelect, ps->nextWeapon );

			cg.proneMovingTime = cg.time;
		}
	}
	else if ( ops->eFlags & EF_PRONE_MOVING )
	{
		cg.proneMovingTime = -cg.time;
	}

	if ( ! ( ps->eFlags & EF_PRONE ) && ops->eFlags & EF_PRONE )
	{
		if ( cg.weaponSelect == WP_MOBILE_MG42_SET )
		{
			CG_FinishWeaponChange ( cg.weaponSelect, ps->nextWeapon );
		}
	}

	// run events
	CG_CheckPlayerstateEvents ( ps, ops );

	// smooth the ducking viewheight change
	if ( ps->viewheight != ops->viewheight )
	{
		cg.duckChange = ps->viewheight - ops->viewheight;
		cg.duckTime = cg.time;
	}
}
