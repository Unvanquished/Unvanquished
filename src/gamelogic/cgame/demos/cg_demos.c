/*
===========================================================================
Copyright (C) 2009 Sjoerd van der Berg ( harekiet @ gmail.com )

This file is part of Quake III Arena source code.

Quake III Arena source code is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Quake III Arena source code is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Foobar; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
===========================================================================
*/

#include "cg_demos.h"

demoMain_t demo;

extern float CG_Cvar_Get(const char *cvar);

extern int CG_CalcViewValues( void );
extern void CG_InterpolatePlayerState( qboolean grabAngles );
extern void CG_Draw2D( void );
extern void CG_DrawBinaryShadersFinalPhases( void );
extern void CG_PainBlend( void );

extern void trap_MME_BlurInfo( int* total, int * index );
extern void trap_MME_Capture( const char *baseName, float fps, float focus, float radius );
extern int trap_MME_SeekTime( int seekTime );
extern void trap_MME_Music( const char *musicName, float time, float length );
extern int trap_MME_DemoInfo( mmeDemoInfo_t *info );
extern void trap_MME_TimeFraction( float timeFraction );
extern void trap_S_UpdateScale( float scale );
int lastMusicStart;

static void demoSynchMusic( int start, float length ) {
	if ( length > 0 ) {
		lastMusicStart = -1;
	} else {
		length = 0;
		lastMusicStart = start;
	}
	trap_MME_Music( mov_musicFile.string , start * 0.001f, length );
}

static void CG_DemosUpdatePlayer( void ) {
	demo.oldcmd = demo.cmd;
	trap_GetUserCmd( trap_GetCurrentCmdNumber(), &demo.cmd );
	demoMoveUpdateAngles();
	demoMoveDeltaCmd();

	if ( demo.seekEnabled ) {
		int delta;
		demo.play.fraction -= demo.serverDeltaTime * 1000 * demo.cmdDeltaAngles[YAW];
		delta = (int)demo.play.fraction;
		demo.play.time += delta;
		demo.play.fraction -= delta;

		if (demo.deltaRight) {
			int interval = mov_seekInterval.value * 1000;
			int rem = demo.play.time % interval;
			if (demo.deltaRight > 0) {
                demo.play.time += interval - rem;
			} else {
                demo.play.time -= interval + rem;
			}
			demo.play.fraction = 0;
		}
		if (demo.play.fraction < 0) {
			demo.play.fraction += 1;
			demo.play.time -= 1;
		}
		if (demo.play.time < 0) {
			demo.play.time = demo.play.fraction = 0;
		}
		return;
	}
	switch ( demo.editType ) {
	case editCamera:
		cameraMove();
		break;
	case editChase:
		demoMoveChase();
		break;
	case editDof:
		dofMove();
		break;
	case editLine:
		demoMoveLine();
		break;
	}
}


static int demoSetupView( void) {
	vec3_t forward;
	int inwater = qfalse;
	qboolean behindView = qfalse;
	int contents;

	cg.playerPredicted = qfalse;
	cg.playerCent = 0;
	demo.viewFocus = 0;
	demo.viewTarget = -1;

	switch (demo.viewType) {
	case viewChase:
		if ( demo.chase.cent && demo.chase.distance < mov_chaseRange.value ) {
			centity_t *cent = demo.chase.cent;

			if ( cent->currentState.number < MAX_CLIENTS ) {
				cg.playerCent = cent;
				cg.playerPredicted = cent == &cg.predictedPlayerEntity;
				if (!cg.playerPredicted ) {
					//Make sure lerporigin of playercent is val
					CG_CalcEntityLerpPositions( cg.playerCent );
				}
				cg.renderingThirdPerson = cg_thirdPerson.integer || (cg.snap->ps.stats[STAT_HEALTH] <= 0);
				CG_CalcViewValues();
				VectorCopy( cg.refdef.vieworg, demo.viewOrigin );
				VectorCopy( cg.refdefViewAngles, demo.viewAngles );
			} else {
				VectorCopy( cent->lerpOrigin, demo.viewOrigin );
				VectorCopy( cent->lerpAngles, demo.viewAngles );
			}
			demo.viewFov = MME_FOV;
		} else {
			memset( &cg.refdef, 0, sizeof(cg.refdef));
			Vector4Set(cg.refdef.gradingWeights, 0.0f, 0.0f, 0.0f, 1.0f);
			AngleVectors( demo.chase.angles, forward, 0, 0 );
			VectorMA( demo.chase.origin , -demo.chase.distance, forward, demo.viewOrigin );
			VectorCopy( demo.chase.angles, demo.viewAngles );
			demo.viewFov = MME_FOV;
			demo.viewTarget = demo.chase.target;
			cg.renderingThirdPerson = qtrue;
		}
		break;
	case viewCamera:
		memset( &cg.refdef, 0, sizeof(cg.refdef));
		Vector4Set(cg.refdef.gradingWeights, 0.0f, 0.0f, 0.0f, 1.0f);
		VectorCopy( demo.camera.origin, demo.viewOrigin );
		VectorCopy( demo.camera.angles, demo.viewAngles );
		demo.viewFov = demo.camera.fov + MME_FOV;
		demo.viewTarget = demo.camera.target;
		cg.renderingThirdPerson = qtrue;
		cameraMove();
		break;
	default:
		return inwater;
	}

	demo.viewAngles[YAW]	+= mov_deltaYaw.value;
	demo.viewAngles[PITCH]	+= mov_deltaPitch.value;
	demo.viewAngles[ROLL]	+= mov_deltaRoll.value;

#ifdef MME_FX
	trap_FX_VibrateView( 1.0f, demo.viewOrigin, demo.viewAngles );
#endif
	VectorCopy( demo.viewOrigin, cg.refdef.vieworg );
	AnglesToAxis( demo.viewAngles, cg.refdef.viewaxis );

	/* find focus ditance to certain target but don't apply if dof is not locked, use for drawing */
	if ( demo.dof.target >= 0 ) {
		centity_t* targetCent = demoTargetEntity( demo.dof.target );
		if ( targetCent ) {
			vec3_t targetOrigin;
			chaseEntityOrigin( targetCent, targetOrigin );
			//Find distance betwene plane of camera and this target
			demo.viewFocus = DotProduct( cg.refdef.viewaxis[0], targetOrigin ) - DotProduct( cg.refdef.viewaxis[0], cg.refdef.vieworg  );
			demo.dof.focus = demo.viewFocusOld = demo.viewFocus;
		} else {
			demo.dof.focus = demo.viewFocus = demo.viewFocusOld;
		}
		if (demo.dof.focus < 0.001f) {
			behindView = qtrue;
		}
	}
	if ( demo.dof.locked ) {
		if (!behindView) {
			demo.viewFocus = demo.dof.focus;
			demo.viewRadius = demo.dof.radius;
		} else {
			demo.viewFocus = 0.002f;		// no matter what value, just not less or equal zero
			demo.viewRadius = 0.0f;
		}
	} else if ( demo.viewTarget >= 0 ) {
		centity_t* targetCent = demoTargetEntity( demo.viewTarget );
		if ( targetCent ) {
			vec3_t targetOrigin;
			chaseEntityOrigin( targetCent, targetOrigin );
			//Find distance betwene plane of camera and this target
			demo.viewFocus = DotProduct( cg.refdef.viewaxis[0], targetOrigin ) - DotProduct( cg.refdef.viewaxis[0], cg.refdef.vieworg  );
			demo.viewRadius = CG_Cvar_Get( "mme_dofRadius" );
		}
	} else if ( demo.dof.target >= 0 ) {
		demo.viewFocus = 0;
		demo.viewRadius = 0;
	}

	cg.refdef.width = cgs.glconfig.vidWidth*cg_viewsize.integer/100;
	cg.refdef.width &= ~1;

	cg.refdef.height = cgs.glconfig.vidHeight*cg_viewsize.integer/100;
	cg.refdef.height &= ~1;

	cg.refdef.x = (cgs.glconfig.vidWidth - cg.refdef.width)/2;
	cg.refdef.y = (cgs.glconfig.vidHeight - cg.refdef.height)/2;

	cg.refdef.fov_x = demo.viewFov;
	cg.refdef.fov_y = atan2( cg.refdef.height, (cg.refdef.width / tan( demo.viewFov / 360 * M_PI )) ) * 360 / M_PI;

	contents = CG_PointContents( cg.refdef.vieworg, -1 );
	if ( contents & ( CONTENTS_WATER | CONTENTS_SLIME | CONTENTS_LAVA ) ){
		double v = WAVE_AMPLITUDE * sin(((double)cg.time + (double)cg.timeFraction) / 1000.0 * WAVE_FREQUENCY * M_PI * 2);
		cg.refdef.fov_x += v;
		cg.refdef.fov_y -= v;
		inwater = qtrue;
	} else {
		inwater = qfalse;
	}
	return inwater;
}

extern snapshot_t *CG_ReadNextSnapshot( void );
extern void CG_SetNextSnap( snapshot_t *snap );
extern void CG_TransitionSnapshot( void );

void demoProcessSnapShots( qboolean hadSkip ) {
	int i;
	snapshot_t		*snap;

	// see what the latest snapshot the client system has is
	trap_GetCurrentSnapshotNumber( &cg.latestSnapshotNum, &cg.latestSnapshotTime );
	if (hadSkip || !cg.snap) {
		cgs.processedSnapshotNum = cg.latestSnapshotNum - 2;
		if (cg.nextSnap)
			cgs.serverCommandSequence = cg.nextSnap->serverCommandSequence;
		else if (cg.snap)
			cgs.serverCommandSequence = cg.snap->serverCommandSequence;
		cg.snap = 0;
		cg.nextSnap = 0;

		for (i=-1;i<MAX_GENTITIES;i++) {
			centity_t *cent = i < 0 ? &cg.predictedPlayerEntity : &cg_entities[i];
			cent->trailTime = cg.time;
			cent->snapShotTime = cg.time;
			cent->currentValid = qfalse;
			cent->interpolate = qfalse;
			cent->muzzleFlashTime = cg.time - MUZZLE_FLASH_TIME - 1;
			cent->previousEvent = 0;
			if (cent->currentState.eType == ET_PLAYER) {
				memset( &cent->pe, 0, sizeof( cent->pe ) );
				cent->pe.legs.yawAngle = cent->lerpAngles[YAW];
				cent->pe.torso.yawAngle = cent->lerpAngles[YAW];
				cent->pe.torso.pitchAngle = cent->lerpAngles[PITCH];
			}
		}
	}

	/* Check if we have some transition between snapsnots */
	if (!cg.snap) {
		snap = CG_ReadNextSnapshot();
		if (!snap)
			return;
		cg.snap = snap;
		BG_PlayerStateToEntityState( &snap->ps, &cg_entities[ snap->ps.clientNum ].currentState, qfalse );
		CG_BuildSolidList();
		CG_ExecuteNewServerCommands( snap->serverCommandSequence );
		for ( i = 0 ; i < cg.snap->numEntities ; i++ ) {
			entityState_t *state = &cg.snap->entities[ i ];
			centity_t *cent = &cg_entities[ state->number ];
			memcpy(&cent->currentState, state, sizeof(entityState_t));
			cent->interpolate = qfalse;
			cent->currentValid = qtrue;
			if (cent->currentState.eType > ET_EVENTS)
				cent->previousEvent = 1;
			else
				cent->previousEvent = cent->currentState.event;
		}
		// Need this check because the initial weapon for spec isn't always WP_NONE
		//entTODO: remove it if not necessary
//		if ( snap->ps.persistant[ PERS_TEAM ] == TEAM_NONE )
//			trap_Rocket_ShowHud( WP_NONE );
//		else
//			trap_Rocket_ShowHud( BG_GetPlayerWeapon( &snap->ps ) );
	}
	do {
		if (!cg.nextSnap) {
			snap = CG_ReadNextSnapshot();
			if (!snap)
				break;
			CG_SetNextSnap( snap );
		}
		if ( cg.time >= cg.snap->serverTime && cg.time < cg.nextSnap->serverTime )
			break;
		//Todo our own transition checking if we wanna hear certain sounds
		CG_TransitionSnapshot();
	} while (1);
}

void CG_DemosDrawActiveFrame( int serverTime ) {
	int deltaTime;
	qboolean hadSkip;
	qboolean captureFrame;
	float captureFPS;
	float frameSpeed;
	int blurTotal, blurIndex;
	float blurFraction;
	float stereoSep = CG_Cvar_Get( "r_stereoSeparation" );

	int inwater, entityNum;

	if (!demo.initDone) {
		if ( !cg.snap ) {
			demoProcessSnapShots( qtrue );
		}
		if ( !cg.snap ) {
			CG_Error( "No Initial demo snapshot found" );
		}
		demoPlaybackInit();
	}

	cg.demoPlayback = 2;

	// if we are only updating the screen as a loading
	// pacifier, don't even try to read snapshots
	if ( cg.loading ) {
		return;
	}

	captureFrame = demo.capture.active && !demo.play.paused;
	if ( captureFrame ) {
		trap_MME_BlurInfo( &blurTotal, &blurIndex );
		captureFPS = mov_captureFPS.value;
		if ( blurTotal > 0) {
			captureFPS *= blurTotal;
			blurFraction = blurIndex / (float)blurTotal;
		} else {
			blurFraction = 0;
		}
	} else {
	}

	/* Forward the demo */
	deltaTime = serverTime - demo.serverTime;
	if (deltaTime > 50)
		deltaTime = 50;
	demo.serverTime = serverTime;
	demo.serverDeltaTime = 0.001 * deltaTime;
	cg.oldTime = cg.time;
	cg.oldTimeFraction = cg.timeFraction;

	if (demo.play.time < 0) {
		demo.play.time = demo.play.fraction = 0;
	}

	demo.play.oldTime = demo.play.time;


	/* Handle the music */
	if ( demo.play.paused ) {
		if ( lastMusicStart >= 0)
			demoSynchMusic( -1, 0 );
	} else {
		int musicStart = (demo.play.time - mov_musicStart.value * 1000 );
		if ( musicStart <= 0 ) {
			if (lastMusicStart >= 0 )
				demoSynchMusic( -1, 0 );
		} else {
			if ( demo.play.time != demo.play.lastTime || lastMusicStart < 0)
				demoSynchMusic( musicStart, 0 );
		}
	}
	/* forward the time a bit till the moment of capture */
	if ( captureFrame && demo.capture.locked && demo.play.time < demo.capture.start ) {
		int left = demo.capture.start - demo.play.time;
		if ( left > 2000) {
			left -= 1000;
			captureFrame = qfalse;
		} else if (left > 5) {
			captureFrame = qfalse;
			left = 5;
		}
		demo.play.time += left;
	} else if ( captureFrame && demo.loop.total && blurTotal ) {
		float loopFraction = demo.loop.index / (float)demo.loop.total;
		demo.play.time = demo.loop.start;
		demo.play.fraction = demo.loop.range * loopFraction;
		demo.play.time += (int)demo.play.fraction;
		demo.play.fraction -= (int)demo.play.fraction;
	} else if (captureFrame) {
		float frameDelay = 1000.0f / captureFPS;
		demo.play.fraction += frameDelay * demo.play.speed;
		demo.play.time += (int)demo.play.fraction;
		demo.play.fraction -= (int)demo.play.fraction;
	} else if ( demo.find ) {
		demo.play.time = demo.play.oldTime + 20;
		demo.play.fraction = 0;
		if ( demo.play.paused )
			demo.find = findNone;
	} else if (!demo.play.paused) {
		float delta = demo.play.fraction + deltaTime * demo.play.speed;
		demo.play.time += (int)delta;
		demo.play.fraction = delta - (int)delta;
	}

	demo.play.lastTime = demo.play.time;

	if ( demo.loop.total && captureFrame && blurTotal ) {
		//Delay till we hit the right part at the start
		int time;
		float timeFraction;
		if ( demo.loop.lineDelay && !blurIndex ) {
			time = demo.loop.start - demo.loop.lineDelay;
			timeFraction = 0;
			if ( demo.loop.lineDelay > 8 )
				demo.loop.lineDelay -= 8;
			else
				demo.loop.lineDelay = 0;
			captureFrame = qfalse;
		} else {
			if ( blurIndex == blurTotal - 1 ) {
				//We'll restart back to the start again
				demo.loop.lineDelay = 2000;
				if ( ++demo.loop.index >= demo.loop.total ) {
					demo.loop.total = 0;
				}
			}
			time = demo.loop.start;
			timeFraction = demo.loop.range * blurFraction;
		}
		time += (int)timeFraction;
		timeFraction -= (int)timeFraction;
		lineAt( time, timeFraction, &demo.line.time, &cg.timeFraction, &frameSpeed );
	} else {
		lineAt( demo.play.time, demo.play.fraction, &demo.line.time, &cg.timeFraction, &frameSpeed );
	}
	/* Set the correct time */
	cg.time = trap_MME_SeekTime( demo.line.time );
	/* cg.time is shifted ahead a bit to correct some issues.. */
	frameSpeed *= demo.play.speed;

	cg.frametime = (cg.time - cg.oldTime) + (cg.timeFraction - cg.oldTimeFraction);
	if (cg.frametime < 0) {
		int i;
		cg.frametime = 0;
		hadSkip = qtrue;
		cg.oldTime = cg.time;
		cg.oldTimeFraction = cg.timeFraction;
		CG_InitMarkPolys();
#ifdef MME_FX
		trap_FX_Reset( );
#endif

		cg.centerPrintTime = 0;
        cg.damageTime = 0;
		cg.scoreFadeTime = 0;
		cg.attackerTime = 0;
		cg.weaponSelectTime = 0;
		cg.v_dmg_time = 0;

		trap_S_ClearLoopingSounds(qtrue);
	} else if (cg.frametime > 100) {
		hadSkip = qtrue;
	} else {
		hadSkip = qfalse;
	}
	/* Make sure the random seed is the same each time we hit this frame */
	srand( (cg.time % 10000000) + cg.timeFraction * 1000);
	/* Prepare to render the screen */
	trap_Rocket_ClearText();
	CG_NotifyHooks();
	trap_S_ClearLoopingSounds(qfalse);
	trap_R_ClearScene();
	/* Update demo related information */
	trap_SetUserCmdValue( cg.weaponSelect, 0, 1, 0 );
	demoProcessSnapShots( hadSkip );
	if ( !cg.snap ) {
		return;
	}
	CG_PreparePacketEntities( );
	CG_DemosUpdatePlayer( );
	chaseUpdate( demo.play.time, demo.play.fraction );
	cameraUpdate( demo.play.time, demo.play.fraction );
	dofUpdate( demo.play.time, demo.play.fraction );
	cg.clientFrame++;
	// update cg.predictedPlayerState
	CG_InterpolatePlayerState( qfalse );
	BG_PlayerStateToEntityState( &cg.predictedPlayerState, &cg.predictedPlayerEntity.currentState, qfalse );
	cg.predictedPlayerEntity.currentValid = qtrue;
	VectorCopy( cg.predictedPlayerEntity.currentState.pos.trBase, cg.predictedPlayerEntity.lerpOrigin );
	VectorCopy( cg.predictedPlayerEntity.currentState.apos.trBase, cg.predictedPlayerEntity.lerpAngles );

	// update unlockables data (needs valid predictedPlayerState)
	CG_UpdateUnlockables( &cg.predictedPlayerState );
	// update cvars (needs valid unlockables data)
	CG_UpdateCvars();
	// update speedometer
	CG_AddSpeed();

	inwater = demoSetupView();

	CG_TileClear();
#ifdef MME_FX
	trap_FX_Begin( cg.time, cg.timeFraction );
#endif

	scriptRun( hadSkip );
	CG_AddPacketEntities();
	CG_AddMarks();
	CG_AddParticles ();

	if ( cg.playerCent == &cg.predictedPlayerEntity ) {
		CG_AddViewWeapon( &cg.predictedPlayerState  );
	} else if ( cg.playerCent && cg.playerCent->currentState.number < MAX_CLIENTS )  {
		CG_AddViewWeaponDirect( cg.playerCent, 0 );
	}
	trap_S_UpdateEntityPosition(ENTITYNUM_NONE, cg.refdef.vieworg);

	cg.refdef.time = cg.time;
	memcpy( cg.refdef.areamask, cg.snap->areamask, sizeof( cg.refdef.areamask ) );
	/* Render some extra demo related stuff */
	if (!captureFrame) {
		switch (demo.editType) {
		case editCamera:
			cameraDraw( demo.play.time, demo.play.fraction );
			break;
		case editChase:
			chaseDraw( demo.play.time, demo.play.fraction );
			break;
		case editDof:
			dofDraw( demo.play.time, demo.play.fraction );
			break;
		}
		/* Add bounding boxes for easy aiming */
		if ( demo.editType && ( usercmdButtonPressed(demo.cmd.buttons, BUTTON_ATTACK)) && ( usercmdButtonPressed(demo.cmd.buttons, BUTTON_ATTACK2))  ) {
			int i;
			centity_t *targetCent;
			for (i = 0;i<MAX_GENTITIES;i++) {
	            targetCent = demoTargetEntity( i );
				if (targetCent) {
					vec3_t container, traceStart, traceImpact, forward;
					const float *color;

					demoCentityBoxSize( targetCent, container );
					VectorSubtract( demo.viewOrigin, targetCent->lerpOrigin, traceStart );
					AngleVectors( demo.viewAngles, forward, 0, 0 );
					if (BoxTraceImpact( traceStart, forward, container, traceImpact )) {
						color = colorRed;
					} else {
						color = colorYellow;
					}
					demoDrawBox( targetCent->lerpOrigin, container, color );
				}
			}

		}
	}

	if (frameSpeed > 5)
		frameSpeed = 5;

	trap_S_UpdateScale( frameSpeed );
	if (cg.playerCent && cg.predictedPlayerState.pm_type == PM_INTERMISSION) {
		entityNum = cg.snap->ps.clientNum;
	} else if (cg.playerCent) {
		entityNum = cg.playerCent->currentState.number;
	} else {
		entityNum = ENTITYNUM_NONE;
	}
	trap_S_Respatialize( entityNum, cg.refdef.vieworg, cg.refdef.viewaxis, inwater);

#ifdef MME_FX
	trap_FX_End();
#endif
	if (captureFrame && stereoSep > 0.0f)
		trap_Cvar_Set("r_stereoSeparation", va("%f", -stereoSep));
	trap_MME_TimeFraction(cg.timeFraction);
	CG_DrawBinaryShadersFinalPhases();
	trap_R_RenderScene( &cg.refdef );
	// first person blend blobs, done after AnglesToAxis
	if ( !cg.renderingThirdPerson ) {
		CG_PainBlend();
	}
	if ( demo.viewType == viewChase && cg.playerCent && ( cg.playerCent->currentState.number < MAX_CLIENTS ) )
		CG_Draw2D();

//	CG_DrawSmallString( 0, 0, va( "height %d", cg.playerCent->pe.viewHeight ), 1 );

	if (captureFrame) {
		char fileName[MAX_OSPATH];
		Com_sprintf( fileName, sizeof( fileName ), "capture/%s/%s", mme_demoFileName.string, mov_captureName.string );
		trap_MME_Capture( fileName, captureFPS, demo.viewFocus, demo.viewRadius );
	} else {
		if (demo.editType && !cg.playerCent)
			demoDrawCrosshair();
		hudDraw();
	}
//checkCaptureEnd:
	if ( demo.capture.active && demo.capture.locked && demo.play.time > demo.capture.end  ) {
		Com_Printf( "Capturing ended\n" );
		if (demo.autoLoad) {
			trap_SendConsoleCommand( "disconnect\n" );
		}
		demo.capture.active = qfalse;
	}
}

void CG_DemosAddLog(const char *fmt, ...) {
	char *dest;
	va_list		argptr;

	demo.log.lastline++;
	if (demo.log.lastline >= LOGLINES)
		demo.log.lastline = 0;

	demo.log.times[demo.log.lastline] = demo.serverTime;
	dest = demo.log.lines[demo.log.lastline];

	va_start ( argptr, fmt );
	Q_vsnprintf( dest, sizeof(demo.log.lines[0]), fmt, argptr );
	va_end (argptr);
//	Com_Printf("%s\n", dest);
}

static void demoViewCommand_f(void) {
	const char *cmd = CG_Argv(1);
	int vT = demo.viewType;
	if (!Q_stricmp(cmd, "chase")) {
		demo.viewType = viewChase;
	} else if (!Q_stricmp(cmd, "camera")) {
		demo.viewType = viewCamera;
	} else if (!Q_stricmp(cmd, "prev")) {
		if (demo.viewType == 0)
			vT = viewLast - 1;
		else
			vT--;
		demo.viewType = (demoViewType_t)vT;
	} else if (!Q_stricmp(cmd, "next")) {
		vT++;
		if (vT >= viewLast)
			vT = 0;
		demo.viewType = (demoViewType_t)vT;
	} else {
		Com_Printf("view usage:\n" );
		Com_Printf("view camera, Change to camera view.\n" );
		Com_Printf("view chase, Change to chase view.\n" );
		Com_Printf("view next/prev, Change to next or previous view.\n" );
		return;
	}

	switch (demo.viewType) {
	case viewCamera:
		CG_DemosAddLog("View set to camera" );
		break;
	case viewChase:
		CG_DemosAddLog("View set to chase" );
		break;
	}
}

static void demoEditCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (!Q_stricmp(cmd, "none"))  {
		demo.editType = editNone;
		CG_DemosAddLog("Not editing anything");
	} else if (!Q_stricmp(cmd, "chase")) {
		if ( demo.cmd.upmove > 0 ) {
			demoViewCommand_f();
			return;
		}
		demo.editType = editChase;
		CG_DemosAddLog("Editing chase view");
	} else if (!Q_stricmp(cmd, "camera")) {
		if ( demo.cmd.upmove > 0 ) {
			demoViewCommand_f();
			return;
		}
		demo.editType = editCamera;
		CG_DemosAddLog("Editing camera");
	} else if (!Q_stricmp(cmd, "dof")) {
		demo.editType = editDof;
		CG_DemosAddLog("Editing depth of field");
	} else if (!Q_stricmp(cmd, "line")) {
		if ( demo.cmd.upmove > 0 ) {
			demoViewCommand_f();
			return;
		}
		demo.editType = editLine;
		CG_DemosAddLog("Editing timeline");
	} else if (!Q_stricmp(cmd, "script")) {
		demo.editType = editScript;
		CG_DemosAddLog("Editing script");
	} else {
		switch ( demo.editType ) {
		case editCamera:
			demoCameraCommand_f();
			break;
		case editChase:
			demoChaseCommand_f();
			break;
		case editLine:
			demoLineCommand_f();
			break;
		case editDof:
			demoDofCommand_f();
			break;
		case editScript:
			demoScriptCommand_f();
			break;
		}
	}
}

static void demoSeekTwoCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (isdigit( cmd[0] )) {
		//teh's parser for time MM:SS.MSEC, thanks *bow*
		int i;
		char *sec, *min;;
		min = (char *)cmd;
		for( i=0; min[i]!=':'&& min[i]!=0; i++ );
		if(cmd[i]==0)
			sec = 0;
		else
		{
			min[i] = 0;
			sec = min+i+1;
		}
		demo.play.time = ( atoi( min ) * 60000 + ( sec ? atof( sec ) : 0 ) * 1000 );
		demo.play.fraction = 0;
	}
}

static void demoSeekCommand_f(void) {
	const char *cmd = CG_Argv(1);
	if (cmd[0] == '+') {
		if (isdigit( cmd[1])) {
			demo.play.time += atof( cmd + 1 ) * 1000;
			demo.play.fraction = 0;
		}
	} else if (cmd[0] == '-') {
		if (isdigit( cmd[1])) {
			demo.play.time -= atof( cmd + 1 ) * 1000;
			demo.play.fraction = 0;
		}
	} else if (isdigit( cmd[0] )) {
		demo.play.time = atof( cmd ) * 1000;
		demo.play.fraction = 0;
	}
}

static void musicPlayCommand_f(void) {
	float length = 2;
	int musicStart;

	if ( trap_Argc() > 1 ) {
		length = atof( CG_Argv( 1 ) );
		if ( length <= 0)
			length = 2;
	}
	musicStart = (demo.play.time - mov_musicStart.value * 1000 );
	demoSynchMusic( musicStart, length );
}

static void demoFindCommand_f(void) {
	const char *cmd = CG_Argv(1);

	if (!Q_stricmp(cmd, "death")) {
		demo.find = findObituary;
	} else {
		demo.find = findNone;
	}
	if ( demo.find )
		demo.play.paused = qfalse;
}

void demoPlaybackInit(void) {
	char projectFile[MAX_OSPATH];

	demo.initDone = qtrue;
	demo.autoLoad = qfalse;
	demo.play.time = 0;
	demo.play.lastTime = 0;
	demo.play.fraction = 0;
	demo.play.speed = 1.0;
	demo.play.paused = 0;

	demo.move.acceleration = 8;
	demo.move.friction = 8;
	demo.move.speed = 400;

	demo.line.locked = qfalse;
	demo.line.offset = 0;
	demo.line.speed = 1.0f;
	demo.line.points = 0;

	demo.loop.total = 0;

	demo.editType = editCamera;
	demo.viewType = viewChase;

	demo.camera.flags = CAM_ORIGIN | CAM_ANGLES;

	VectorClear( demo.chase.origin );
	VectorClear( demo.chase.angles );
	VectorClear( demo.chase.velocity );

	demo.chase.distance = 0;
	demo.chase.locked = qfalse;
	demo.chase.target = -1;

	demo.dof.focus = 256.0f;
	demo.dof.radius = 5.0f;
	demo.dof.target = -1;

	demo.camera.target = -1;
	demo.camera.fov = 0;
	demo.camera.smoothPos = posBezier;
	demo.camera.smoothAngles = angleQuat;

	hudInitTables();
	demoSynchMusic( -1, 0 );

	demo.media.additiveWhiteShader = trap_R_RegisterShader( "mme_additiveWhite", RSF_DEFAULT );
	demo.media.mouseCursor = trap_R_RegisterShader( "menu/art/3_cursor2", (RegisterShaderFlags_t)( RSF_DEFAULT | RSF_NOMIP ) );
	demo.media.switchOn = trap_R_RegisterShader( "menu/art/switch_on", (RegisterShaderFlags_t)( RSF_DEFAULT | RSF_NOMIP ) );
	demo.media.switchOff = trap_R_RegisterShader( "menu/art/switch_off", (RegisterShaderFlags_t)( RSF_DEFAULT | RSF_NOMIP ) );

	trap_AddCommand("camera");
	trap_AddCommand("edit");
	trap_AddCommand("view");
	trap_AddCommand("chase");
	trap_AddCommand("dof");
	trap_AddCommand("speed");
	trap_AddCommand("pause");
	trap_AddCommand("seek");
	trap_AddCommand("demoSeek");
	trap_AddCommand("find");
	trap_AddCommand("capture");
	trap_AddCommand("hudInit");
	trap_AddCommand("hudToggle");
	trap_AddCommand("line");
	trap_AddCommand("save");
	trap_AddCommand("load");
	trap_AddCommand("+seek");
	trap_AddCommand("-seek");
	trap_AddCommand("-seek");
	trap_AddCommand("musicPlay");

	trap_SendConsoleCommand("exec mme.cfg\n");
	trap_SendConsoleCommand("exec mmedemos.cfg\n");
	trap_Cvar_Set( "mov_captureName", "" );
	trap_Cvar_VariableStringBuffer( "mme_demoStartProject", projectFile, sizeof( projectFile ));
	if (projectFile[0]) {
		trap_Cvar_Set( "mme_demoStartProject", "" );
		demo.autoLoad = demoProjectLoad( projectFile );
		if (demo.autoLoad) {
			if (!demo.capture.start && !demo.capture.end) {
				trap_Error( "Loaded project file with empty capture range\n");
			}
			/* Check if the project had a cvar for the name else use project */
			if (!mov_captureName.string[0]) {
				trap_Cvar_Set( "mov_captureName", projectFile );
				trap_Cvar_Update( &mov_captureName );
			}
			trap_SendConsoleCommand("exec mmelist.cfg\n");
			demo.play.time = demo.capture.start - 1000;
			demo.capture.locked = qtrue;
			demo.capture.active = qtrue;
		} else {
			trap_Error( va("Couldn't load project %s\n", projectFile ));
		}
	}
}

void CG_DemoEntityEvent( const centity_t* cent ) {
	switch ( cent->currentState.event ) {
	case EV_OBITUARY:
		if ( demo.find == findObituary ) {
			demo.play.paused = qtrue;
			demo.find = findNone;
		}
		break;
	}
}

qboolean CG_DemosConsoleCommand( void ) {
	const char *cmd = CG_Argv(0);
	if (!Q_stricmp(cmd, "camera")) {
		demoCameraCommand_f();
	} else if (!Q_stricmp(cmd, "view")) {
		demoViewCommand_f();
	} else if (!Q_stricmp(cmd, "edit")) {
		demoEditCommand_f();
	} else if (!Q_stricmp(cmd, "capture")) {
		demoCaptureCommand_f();
	} else if (!Q_stricmp(cmd, "seek")) {
		demoSeekCommand_f();
	} else if (!Q_stricmp(cmd, "demoSeek")) {
		demoSeekTwoCommand_f();
	} else if (!Q_stricmp(cmd, "find")) {
		demoFindCommand_f();
	} else if (!Q_stricmp(cmd, "speed")) {
		cmd = CG_Argv(1);
		if (cmd[0]) {
			demo.play.speed = atof(cmd);
		}
		CG_DemosAddLog("Play speed %f", demo.play.speed );
	} else if (!Q_stricmp(cmd, "pause")) {
		demo.play.paused = !demo.play.paused;
		if ( demo.play.paused )
			demo.find = findNone;
	} else if (!Q_stricmp(cmd, "dof")) {
		demoDofCommand_f();
	} else if (!Q_stricmp(cmd, "chase")) {
		demoChaseCommand_f();
	} else if (!Q_stricmp(cmd, "hudInit")) {
		hudInitTables();
	} else if (!Q_stricmp(cmd, "hudToggle")) {
		hudToggleInput();
	} else if (!Q_stricmp(cmd, "+seek")) {
		trap_PrepareKeyUp();
		demo.seekEnabled = qtrue;
	} else if (!Q_stricmp(cmd, "-seek")) {
		demo.seekEnabled = qfalse;
	} else if (!Q_stricmp(cmd, "line")) {
		demoLineCommand_f();
	} else if (!Q_stricmp(cmd, "load")) {
		demoLoadCommand_f();
	} else if (!Q_stricmp(cmd, "save")) {
		demoSaveCommand_f();
	//entTODO: add client override feature
//	} else if (!Q_stricmp(cmd, "clientOverride")) {
//		CG_ClientOverride_f();
	} else if (!Q_stricmp(cmd, "musicPlay")) {
		musicPlayCommand_f();
	} else {
		return CG_ConsoleCommand();
	}
	return qtrue;
}

