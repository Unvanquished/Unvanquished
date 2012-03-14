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

// cg_multiview.c: Multiview handling
// ----------------------------------
// 02 Sep 02
// rhea@OrangeSmoothie.org
//
#include "cg_local.h"
#include "../ui/ui_shared.h"
#include "../game/bg_local.h"

void CG_CalcVrect ( void );
void CG_DrawPlayerWeaponIcon ( rectDef_t *rect, qboolean drawHighlighted, int align, vec4_t *refcolor );

// Explicit server command to add a view to the client's snapshot
void CG_mvNew_f ( void )
{
	if ( cg.demoPlayback || trap_Argc() < 2 )
	{
		return;
	}
	else
	{
		int  pID;
		char aName[ 64 ];

		trap_Args ( aName, sizeof ( aName ) );
		pID = CG_findClientNum ( aName );

		if ( pID >= 0 && !CG_mvMergedClientLocate ( pID ) )
		{
			trap_SendClientCommand ( va ( "mvadd %d\n", pID ) );
		}
	}
}

// Explicit server command to remove a view from the client's snapshot
void CG_mvDelete_f ( void )
{
	if ( cg.demoPlayback )
	{
		return;
	}
	else
	{
		int pID = -1;

		if ( trap_Argc() > 1 )
		{
			char aName[ 64 ];

			trap_Args ( aName, sizeof ( aName ) );
			pID = CG_findClientNum ( aName );
		}
		else
		{
			cg_window_t *w = cg.mvCurrentActive;

			if ( w != NULL )
			{
				pID = ( w->mvInfo & MV_PID );
			}
		}

		if ( pID >= 0 && CG_mvMergedClientLocate ( pID ) )
		{
			trap_SendClientCommand ( va ( "mvdel %d\n", pID ) );
		}
	}
}

// Swap highlighted window with main view
void CG_mvSwapViews_f ( void )
{
	if ( cg.mv_cnt >= 2 && cg.mvCurrentActive != cg.mvCurrentMainview )
	{
		CG_mvMainviewSwap ( cg.mvCurrentActive );
	}
}

// Shut down a window view for a particular MV client
void CG_mvHideView_f ( void )
{
	if ( cg.mvCurrentActive == NULL || cg.mvCurrentMainview == cg.mvCurrentActive )
	{
		return;
	}

	CG_mvFree ( cg.mvCurrentActive->mvInfo & MV_PID );
}

// Activate a window view for a particular MV client
void CG_mvShowView_f ( void )
{
	int i;

	for ( i = 0; i < cg.mvTotalClients; i++ )
	{
		if ( cg.mvOverlay[ i ].fActive )
		{
			if ( cg.mvOverlay[ i ].w == NULL )
			{
				CG_mvCreate ( cg.mvOverlay[ i ].pID );
				CG_mvOverlayUpdate();
			}

			return;
		}
	}
}

// Toggle a view window on/off
void CG_mvToggleView_f ( void )
{
	int i;

	for ( i = 0; i < cg.mvTotalClients; i++ )
	{
		if ( cg.mvOverlay[ i ].fActive )
		{
			if ( cg.mvOverlay[ i ].w != NULL )
			{
				CG_mvHideView_f();
			}
			else
			{
				CG_mvCreate ( cg.mvOverlay[ i ].pID );
				CG_mvOverlayUpdate();
			}

			return;
		}
	}
}

// Toggle all views
void CG_mvToggleAll_f ( void )
{
	if ( !cg.demoPlayback )
	{
		trap_SendClientCommand ( ( cg.mvTotalClients > 0 ) ? "mvnone\n" : "mvall\n" );

		if ( cg.mvTotalClients > 0 )
		{
			CG_EventHandling ( -CGAME_EVENT_MULTIVIEW, qfalse );
		}
	}
}

////////////////////////////////////////////////
//
// Multiview Primitives
//
//

///////////////////////////////
// Create a new view window
//
void CG_mvCreate ( int pID )
{
	cg_window_t *w;

	if ( CG_mvClientLocate ( pID ) != NULL )
	{
		return;
	}

	w = CG_windowAlloc ( WFX_MULTIVIEW, 100 );

	if ( w == NULL )
	{
		return;
	}

	// Window specific
	w->id = WID_NONE;
	w->x = ( cg.mv_cnt == 0 ) ? 0 : 30 + ( 12 * pID );
	w->y = ( cg.mv_cnt == 0 ) ? 0 : 300 + ( 5 * pID );
	w->w = ( cg.mv_cnt == 0 ) ? 640 : 128;
	w->h = ( cg.mv_cnt == 0 ) ? 480 : 96;
	w->mvInfo = ( pID & MV_PID ) | MV_SELECTED;
	w->state = ( cg.mv_cnt == 0 ) ? WSTATE_COMPLETE : WSTATE_START;

	if ( cg.mv_cnt == 0 )
	{
		cg.mvCurrentMainview = w;
		cg.mvCurrentActive = cg.mvCurrentMainview;

		if ( cg_specHelp.integer > 0 && !cg.demoPlayback )
		{
			CG_ShowHelp_On ( &cg.spechelpWindow );
			CG_EventHandling ( CGAME_EVENT_MULTIVIEW, qfalse );
		}
	}

	cg.mv_cnt++;
}

///////////////////////////
// Delete a view window
//
void CG_mvFree ( int pID )
{
	cg_window_t *w = CG_mvClientLocate ( pID );

	if ( w != NULL )
	{
		// Free it in mvDraw()
		w->targetTime = 100;
		w->time = trap_Milliseconds();
		w->state = WSTATE_SHUTDOWN;
	}
}

cg_window_t    *CG_mvClientLocate ( int pID )
{
	int                i;
	cg_window_t        *w;
	cg_windowHandler_t *wh = &cg.winHandler;

	for ( i = 0; i < wh->numActiveWindows; i++ )
	{
		w = &wh->window[ wh->activeWindows[ i ] ];

		if ( ( w->effects & WFX_MULTIVIEW ) && ( w->mvInfo & MV_PID ) == pID )
		{
			return ( w );
		}
	}

	return ( NULL );
}

// Swap a window-view with the main view
void CG_mvMainviewSwap ( cg_window_t *av )
{
	int swap_pID = ( cg.mvCurrentMainview->mvInfo & MV_PID );

	cg.mvCurrentMainview->mvInfo = ( cg.mvCurrentMainview->mvInfo & ~MV_PID ) | ( av->mvInfo & MV_PID );
	av->mvInfo = ( av->mvInfo & ~MV_PID ) | swap_pID;

	CG_mvOverlayUpdate();
}

/////////////////////////////////////////////
// Track our list of merged clients
//
void CG_mvProcessClientList ( void )
{
	int i, bit, newList = cg.snap->ps.powerups[ PW_MVCLIENTLIST ];

	cg.mvTotalClients = 0;

	for ( i = 0; i < MAX_MVCLIENTS; i++ )
	{
		bit = 1 << i;

		if ( ( cg.mvClientList & bit ) != ( newList & bit ) )
		{
			if ( ( newList & bit ) == 0 )
			{
				CG_mvFree ( i );
			}
			// If no active views at all, start up with the first one
			else if ( cg.mvCurrentMainview == NULL )
			{
				CG_mvCreate ( i );
			}
		}

		if ( newList & bit )
		{
			cg.mvTotalClients++;
		}
	}

	cg.mvClientList = newList;
	CG_mvOverlayUpdate();
}

// Give handle to the current selected MV window
cg_window_t    *CG_mvCurrent ( void )
{
	int                i;
	cg_window_t        *w;
	cg_windowHandler_t *wh = &cg.winHandler;

	for ( i = 0; i < wh->numActiveWindows; i++ )
	{
		w = &wh->window[ wh->activeWindows[ i ] ];

		if ( ( w->effects & WFX_MULTIVIEW ) && ( w->mvInfo & MV_SELECTED ) )
		{
			return ( w );
		}
	}

	return ( NULL );
}

// Give handle to any MV window that isnt the mainview
cg_window_t    *CG_mvFindNonMainview ( void )
{
	int                i;
	cg_window_t        *w;
	cg_windowHandler_t *wh = &cg.winHandler;

	// First, find a non-windowed client
	for ( i = 0; i < cg.mvTotalClients; i++ )
	{
		if ( cg.mvOverlay[ i ].w == NULL )
		{
			cg.mvCurrentMainview->mvInfo = ( cg.mvCurrentMainview->mvInfo & ~MV_PID ) | ( cg.mvOverlay[ i ].pID & MV_PID );

			CG_mvOverlayClientUpdate ( cg.mvOverlay[ i ].pID, i );
			return ( cg.mvCurrentMainview );
		}
	}

	// If none available, pull one from a window
	for ( i = 0; i < wh->numActiveWindows; i++ )
	{
		w = &wh->window[ wh->activeWindows[ i ] ];

		if ( ( w->effects & WFX_MULTIVIEW ) && w != cg.mvCurrentMainview )
		{
			CG_mvMainviewSwap ( w );
			return ( w );
		}
	}

	return ( cg.mvCurrentMainview );
}

//////////////////////////////////////////////
//
//    Rendering/Display Management
//
//////////////////////////////////////////////

///////////////////////////////////////////////////
// Update all info for a merged client
//
void CG_mvUpdateClientInfo ( int pID )
{
	if ( pID >= 0 && pID < MAX_MVCLIENTS && ( cg.mvClientList & ( 1 << pID ) ) )
	{
		int           weap = cg_entities[ pID ].currentState.weapon;
		int           id = MAX_WEAPONS - 1 - ( pID * 2 );
		clientInfo_t  *ci = &cgs.clientinfo[ pID ];
		playerState_t *ps = &cg.snap->ps;

		ci->health = ( ps->ammo[ id ] ) & 0xFF;
		ci->hintTime = ( ps->ammo[ id ] >> 8 ) & 0x0F;
		ci->weapHeat = ( ps->ammo[ id ] >> 12 ) & 0x0F;

		ci->ammo = ( ps->ammo[ id - 1 ] ) & 0x3FF;
		ci->weaponState = ( ps->ammo[ id - 1 ] >> 11 ) & 0x03;
		ci->fCrewgun = ( ps->ammo[ id - 1 ] >> 13 ) & 0x01;
		ci->cursorHint = ( ps->ammo[ id - 1 ] >> 14 ) & 0x03;

		ci->ammoclip = ( ps->ammoclip[ id - 1 ] ) & 0x1FF;
		ci->chargeTime = ( ps->ammoclip[ id - 1 ] >> 9 ) & 0x0F;
		ci->sprintTime = ( ps->ammoclip[ id - 1 ] >> 13 ) & 0x07;

		ci->weapHeat = ( int ) ( 100.0f * ( float ) ci->weapHeat / 15.0f );
		ci->chargeTime = ( ci->chargeTime == 0 ) ? -1 : ( int ) ( 100.0f * ( float ) ( ci->chargeTime - 1 ) / 15.0f );
		ci->hintTime = ( ci->hintTime == 0 ) ? -1 : ( int ) ( 100.0f * ( float ) ( ci->hintTime - 1 ) / 15.0f );
		ci->sprintTime = ( ci->sprintTime == 0 ) ? -1 : ( int ) ( 100.0f * ( float ) ( ci->sprintTime - 1 ) / 7.0f );

		if ( ci->health == 0 )
		{
			ci->weaponState = WSTATE_IDLE;
		}

		// Handle grenade pulsing for the main view
		if ( ci->weaponState != ci->weaponState_last )
		{
			ci->weaponState_last = ci->weaponState;
			ci->grenadeTimeStart = ( ci->weaponState == WSTATE_FIRE &&
			                         ( weap == WP_GRENADE_LAUNCHER || weap == WP_GRENADE_PINEAPPLE ) ) ? 4000 + cg.time : 0;
		}

		if ( ci->weaponState == WSTATE_FIRE && ( weap == WP_GRENADE_LAUNCHER || weap == WP_GRENADE_PINEAPPLE ) )
		{
			ci->grenadeTimeLeft = ci->grenadeTimeStart - cg.time;

			if ( ci->grenadeTimeLeft < 0 )
			{
				ci->grenadeTimeLeft = 0;
			}
		}
		else
		{
			ci->grenadeTimeLeft = 0;
		}
	}
}

////////////////////////////////
// Updates for main view
//
void CG_mvTransitionPlayerState ( playerState_t *ps )
{
	int          x, mult, pID = ( cg.mvCurrentMainview->mvInfo & MV_PID );
	centity_t    *cent = &cg_entities[ pID ];
	clientInfo_t *ci = &cgs.clientinfo[ pID ];

	cg.predictedPlayerEntity.currentState = cent->currentState;
	ps->clientNum = pID;
	ps->weapon = cent->currentState.weapon;
	cg.weaponSelect = ps->weapon;

	cent->currentState.eType = ET_PLAYER;
	ps->eFlags = cent->currentState.eFlags;
	cg.predictedPlayerState.eFlags = cent->currentState.eFlags;
	cg.zoomedBinoc = ( ( cent->currentState.eFlags & EF_ZOOMING ) != 0 && ci->health > 0 );

	x = cent->currentState.teamNum;

	if ( x == PC_MEDIC )
	{
		mult = cg.medicChargeTime[ ci->team - 1 ];
	}
	else if ( x == PC_ENGINEER )
	{
		mult = cg.engineerChargeTime[ ci->team - 1 ];
	}
	else if ( x == PC_FIELDOPS )
	{
		mult = cg.ltChargeTime[ ci->team - 1 ];
	}
	else if ( x == PC_COVERTOPS )
	{
		mult = cg.covertopsChargeTime[ ci->team - 1 ];
	}
	else
	{
		mult = cg.soldierChargeTime[ ci->team - 1 ];
	}

	ps->curWeapHeat = ( int ) ( ( float ) ci->weapHeat * 255.0f / 100.0f );
	ps->classWeaponTime = ( ci->chargeTime < 0 ) ? -1 : cg.time - ( int ) ( ( float ) ( mult * ci->chargeTime ) / 100.0f );

	// FIXME: moved to pmext
//  ps->sprintTime = (ci->sprintTime < 0) ? 20000 : (int)((float)ci->sprintTime / 100.0f * 20000.0f);

	ps->serverCursorHintVal = ( ci->hintTime < 0 ) ? 0 : ci->hintTime * 255 / 100;
	ps->serverCursorHint = BG_simpleHintsExpand ( ci->cursorHint, ( ( x == 2 ) ? ci->hintTime : -1 ) );

	ps->stats[ STAT_HEALTH ] = ci->health;
	ps->stats[ STAT_PLAYER_CLASS ] = x;

	// Grenade ticks
	ps->grenadeTimeLeft = ci->grenadeTimeLeft;

	// Safe as we've already pull data before clobbering
	ps->ammo[ BG_FindAmmoForWeapon ( ps->weapon ) ] = ci->ammo;
	ps->ammoclip[ BG_FindClipForWeapon ( ps->weapon ) ] = ci->ammoclip;

	ps->persistant[ PERS_SCORE ] = ci->score;
	ps->persistant[ PERS_TEAM ] = ci->team;

	VectorCopy ( cent->lerpOrigin, ps->origin );
	VectorCopy ( cent->lerpAngles, ps->viewangles );
}

void CG_OffsetThirdPersonView ( void );

///////////////////////////////////
// Draw the client view window
//
void CG_mvDraw ( cg_window_t *sw )
{
	int       pID = ( sw->mvInfo & MV_PID );
	int       x, base_pID = cg.snap->ps.clientNum;
	refdef_t  refdef;
	float     rd_x, rd_y, rd_w, rd_h;
	float     b_x, b_y, b_w, b_h;
	float     s = 1.0f;
	centity_t *cent = &cg_entities[ pID ];

	memset ( &refdef, 0, sizeof ( refdef_t ) );
	memcpy ( refdef.areamask, cg.snap->areamask, sizeof ( refdef.areamask ) );

	CG_mvUpdateClientInfo ( pID );
	cg.snap->ps.clientNum = pID;

	rd_x = sw->x;
	rd_y = sw->y;
	rd_w = sw->w;
	rd_h = sw->h;

	// Cool zoomin/out effect
	if ( sw->state != WSTATE_COMPLETE )
	{
		int tmp = trap_Milliseconds() - sw->time;

		if ( sw->state == WSTATE_START )
		{
			if ( tmp < sw->targetTime )
			{
				s = ( float ) tmp / ( float ) sw->targetTime;
				rd_x += rd_w * 0.5f * ( 1.0f - s );
				rd_y += rd_h * 0.5f * ( 1.0f - s );
				rd_w *= s;
				rd_h *= s;
			}
			else
			{
				sw->state = WSTATE_COMPLETE;
			}
		}
		else if ( sw->state == WSTATE_SHUTDOWN )
		{
			if ( tmp < sw->targetTime )
			{
				s = ( float ) tmp / ( float ) sw->targetTime;
				rd_x += rd_w * 0.5f * s;
				rd_y += rd_h * 0.5f * s;
				s = 1.0f - s;
				rd_w *= s;
				rd_h *= s;

				if ( sw == cg.mvCurrentMainview )
				{
					trap_R_ClearScene();
				}
			}
			else
			{
				// Main window is shutting down.
				// Try to swap it with another MV client, if available
				if ( sw == cg.mvCurrentMainview )
				{
					sw = CG_mvFindNonMainview();

					if ( cg.mvTotalClients > 0 )
					{
						cg.mvCurrentMainview->targetTime = 100;
						cg.mvCurrentMainview->time = trap_Milliseconds();
						cg.mvCurrentMainview->state = WSTATE_START;
					}

					// If we swap with a window, hang around so we can delete the window
					// Otherwise, if there are still active MV clients, don't close the mainview
					if ( sw == cg.mvCurrentMainview && cg.mvTotalClients > 0 )
					{
						return;
					}
				}

				CG_windowFree ( sw );

				// We were on the last viewed client when the shutdown was issued,
				// go ahead and shut down the mainview window *sniff*
				if ( --cg.mv_cnt <= 0 )
				{
					cg.mv_cnt = 0;
					cg.mvCurrentMainview = NULL;

					if ( cg.spechelpWindow == SHOW_ON )
					{
						CG_ShowHelp_Off ( &cg.spechelpWindow );
					}
				}

				CG_mvOverlayUpdate();
				return;
			}
		}
	}

	b_x = rd_x;
	b_y = rd_y;
	b_w = rd_w;
	b_h = rd_h;

	CG_AdjustFrom640 ( &rd_x, &rd_y, &rd_w, &rd_h );

	refdef.x = rd_x;
	refdef.y = rd_y;
	refdef.width = rd_w;
	refdef.height = rd_h;

	refdef.fov_x = ( cgs.clientinfo[ pID ].health > 0 && ( /*cent->currentState.weapon == WP_SNIPERRIFLE || */ // ARNOUT: this needs updating?
	                   ( cent->currentState.eFlags & EF_ZOOMING ) ) ) ?
	               cg_zoomDefaultSniper.value : ( cgs.clientinfo[ pID ].fCrewgun ) ? 55 : cg_fov.integer;

	x = refdef.width / tan ( refdef.fov_x / 360 * M_PI );
	refdef.fov_y = atan2 ( refdef.height, x ) * 360 / M_PI;

	refdef.rdflags = cg.refdef.rdflags;
	refdef.time = cg.time;

	AnglesToAxis ( cent->lerpAngles, refdef.viewaxis );
	VectorCopy ( cent->lerpOrigin, refdef.vieworg );
	VectorCopy ( cent->lerpAngles, cg.refdefViewAngles );

	cg.refdef_current = &refdef;

	trap_R_ClearScene();

	if ( sw == cg.mvCurrentMainview && cg.renderingThirdPerson )
	{
		cg.renderingThirdPerson = qtrue;
//      VectorCopy(cent->lerpOrigin, refdef.vieworg);
		CG_OffsetThirdPersonView();
		AnglesToAxis ( cg.refdefViewAngles, refdef.viewaxis );
	}
	else
	{
		cg.renderingThirdPerson = qfalse;
		refdef.vieworg[ 2 ] += ( cent->currentState.eFlags & EF_CROUCHING ) ? CROUCH_VIEWHEIGHT : DEFAULT_VIEWHEIGHT;
	}

	CG_SetupFrustum();
	CG_DrawSkyBoxPortal ( qfalse );

	if ( !cg.hyperspace )
	{
		CG_AddPacketEntities();
		CG_AddMarks();
		CG_AddParticles();
		CG_AddLocalEntities();

		CG_AddSmokeSprites();
		CG_AddAtmosphericEffects();

		CG_AddFlameChunks();
		CG_AddTrails(); // this must come last, so the trails dropped this frame get drawn
	}

	if ( sw == cg.mvCurrentMainview )
	{
		CG_DrawActive ( STEREO_CENTER );

		if ( cg.mvCurrentActive == cg.mvCurrentMainview )
		{
			trap_S_Respatialize ( cg.clientNum, refdef.vieworg, refdef.viewaxis, qfalse );
		}

		cg.snap->ps.clientNum = base_pID;
		cg.refdef_current = &cg.refdef;
		cg.renderingThirdPerson = qfalse;
		return;
	}

	memcpy ( refdef.areamask, cg.snap->areamask, sizeof ( refdef.areamask ) );
	refdef.time = cg.time;
	trap_SetClientLerpOrigin ( refdef.vieworg[ 0 ], refdef.vieworg[ 1 ], refdef.vieworg[ 2 ] );

	trap_R_RenderScene ( &refdef );

	cg.refdef_current = &cg.refdef;

#if 0
	cg.refdef_current = &refdef;
	CG_DrawStringExt ( 1, 1, ci->name, colorWhite, qtrue, qtrue, 8, 8, 0 );
	cg.refdef_current = &cg.refdef;
#endif

	CG_mvWindowOverlay ( pID, b_x, b_y, b_w, b_h, s, sw->state, ( sw == cg.mvCurrentActive ) );

	if ( sw == cg.mvCurrentActive )
	{
		trap_S_Respatialize ( cg.clientNum, refdef.vieworg, refdef.viewaxis, qfalse );
	}

	cg.snap->ps.clientNum = base_pID;
}

////////////////////////////////////////////
// Simpler overlay for windows
//
void CG_mvWindowOverlay ( int pID, float b_x, float b_y, float b_w, float b_h, float s, int wState, qboolean fSelected )
{
	int          x;
	rectDef_t    rect;
	float        fw = 8.0f, fh = 8.0f;
	centity_t    *cent = &cg_entities[ pID ];
	clientInfo_t *ci = &cgs.clientinfo[ pID ];
	const char   *p_class = "?";
	vec4_t       *noSelectBorder = &colorDkGrey;

	// Overlays for zoomed views
	if ( ci->health > 0 )
	{
		/*if(cent->currentState.weapon == WP_SNIPERRIFLE) CG_mvZoomSniper(b_x, b_y, b_w, b_h);  // ARNOUT: this needs updating?
		   else */if ( cent->currentState.eFlags & EF_ZOOMING )
		{
			CG_mvZoomBinoc ( b_x, b_y, b_w, b_h );
		}
	}

	// Text info
	fw *= s;
	fh *= s;
	x = cent->currentState.teamNum;

	if ( x == PC_SOLDIER )
	{
		p_class = "^1S";
		noSelectBorder = &colorMdRed;
	}
	else if ( x == PC_MEDIC )
	{
		p_class = "^7M";
		noSelectBorder = &colorMdGrey;
	}
	else if ( x == PC_ENGINEER )
	{
		p_class = "^5E";
		noSelectBorder = &colorMdBlue;
	}
	else if ( x == PC_FIELDOPS )
	{
		p_class = "^2F";
		noSelectBorder = &colorMdGreen;
	}
	else if ( x == PC_COVERTOPS )
	{
		p_class = "^3C";
		noSelectBorder = &colorMdYellow;
	}

	CG_DrawStringExt ( b_x + 1, b_y + b_h - ( fh * 2 + 1 + 2 ), ci->name, colorWhite, qfalse, qtrue, fw, fh, 0 );
	CG_DrawStringExt ( b_x + 1, b_y + b_h - ( fh + 2 ), va ( "%s^7%d", CG_TranslateString ( p_class ), ci->health ), colorWhite, qfalse,
	                   qtrue, fw, fh, 0 );

	p_class = va ( "%d^1/^7%d", ci->ammoclip, ci->ammo );
	x = CG_DrawStrlen ( p_class );
	CG_DrawStringExt ( b_x + b_w - ( x * fw + 1 ), b_y + b_h - ( fh + 2 ), p_class, colorWhite, qfalse, qtrue, fw, fh, 0 );

	// Weapon icon
	rect.x = b_x + b_w - ( 50 + 1 );
	rect.y = b_y + b_h - ( 25 + fh + 1 + 2 );
	rect.w = 50;
	rect.h = 25;
	cg.predictedPlayerState.grenadeTimeLeft = 0;
	cg.predictedPlayerState.weapon = cent->currentState.weapon;
	CG_DrawPlayerWeaponIcon ( &rect, ( ci->weaponState > WSTATE_IDLE ), ITEM_ALIGN_RIGHT,
	                          ( ( ci->weaponState == WSTATE_SWITCH ) ? &colorWhite :
	                            ( ci->weaponState == WSTATE_FIRE ) ? &colorRed : &colorYellow ) );

	// Sprint charge info
	if ( ci->sprintTime >= 0 )
	{
		p_class = va ( "^2S^7%d%%", ci->sprintTime );
		rect.y -= ( fh + 1 );
		x = CG_DrawStrlen ( p_class );
		CG_DrawStringExt ( b_x + b_w - ( x * fw + 1 ), rect.y, p_class, colorWhite, qfalse, qtrue, fw, fh, 0 );
	}

	// Weapon charge info
	if ( ci->chargeTime >= 0 )
	{
		p_class = va ( "^1C^7%d%%", ci->chargeTime );
		rect.y -= ( fh + 1 );
		x = CG_DrawStrlen ( p_class );
		CG_DrawStringExt ( b_x + b_w - ( x * fw + 1 ), rect.y, p_class, colorWhite, qfalse, qtrue, fw, fh, 0 );
	}

	// Cursorhint work
	if ( ci->hintTime >= 0 )
	{
		p_class = va ( "^3W:^7%d%%", ci->hintTime );
		rect.y -= ( fh + 1 );
		x = CG_DrawStrlen ( p_class );
		CG_DrawStringExt ( b_x + ( b_w - ( x * ( fw - 1 ) ) ) / 2, b_y + b_h - ( fh + 2 ), p_class, colorWhite, qfalse, qtrue, fw - 1,
		                   fh - 1, 0 );
	}

	// Finally, the window border
	if ( fSelected && wState == WSTATE_COMPLETE )
	{
		int    t = trap_Milliseconds() & 0x07FF; // 2047
		float  scale;
		vec4_t borderColor;

		if ( t > 1024 )
		{
			t = 2047 - t;
		}

		memcpy ( &borderColor, *noSelectBorder, sizeof ( vec4_t ) );
		scale = ( ( float ) t / 1137.38f ) + 0.5f;

		if ( scale <= 1.0 )
		{
			borderColor[ 0 ] *= scale;
			borderColor[ 1 ] *= scale;
			borderColor[ 2 ] *= scale;
		}
		else
		{
			scale -= 1.0;
			borderColor[ 0 ] = ( borderColor[ 0 ] + scale > 1.0 ) ? 1.0 : borderColor[ 0 ] + scale;
			borderColor[ 1 ] = ( borderColor[ 1 ] + scale > 1.0 ) ? 1.0 : borderColor[ 1 ] + scale;
			borderColor[ 2 ] = ( borderColor[ 2 ] + scale > 1.0 ) ? 1.0 : borderColor[ 2 ] + scale;
		}

		CG_DrawRect ( b_x - 1, b_y - 1, b_w + 2, b_h + 2, 2, borderColor );
	}
	else
	{
		CG_DrawRect ( b_x - 1, b_y - 1, b_w + 2, b_h + 2, 2, *noSelectBorder );
	}
}

////////////////////////////////////////////////////
//
//            MV Text Overlay Handling
//
////////////////////////////////////////////////////

char *strClassHighlights[] =
{
	S_COLOR_RED,    S_COLOR_MDRED, // Soldier
	S_COLOR_WHITE,  S_COLOR_MDGREY, // Medic
	S_COLOR_BLUE,   S_COLOR_MDBLUE, // Engineer
	S_COLOR_GREEN,  S_COLOR_MDGREEN, // Lt.
	S_COLOR_YELLOW, S_COLOR_MDYELLOW // CovertOps
};

// Update a particular client's info
void CG_mvOverlayClientUpdate ( int pID, int index )
{
	cg_window_t *w;

	cg.mvOverlay[ index ].pID = pID;
	cg.mvOverlay[ index ].classID = cg_entities[ pID ].currentState.teamNum;
	w = CG_mvClientLocate ( pID );
	cg.mvOverlay[ index ].w = w;

	if ( w != NULL )
	{
		strcpy ( cg.mvOverlay[ index ].info, va ( "%s%s%2d",
		         strClassHighlights[ cg.mvOverlay[ index ].classID * 2 ],
		         ( w == cg.mvCurrentMainview ) ? "*" : "", pID ) );
	}
	else
	{
		strcpy ( cg.mvOverlay[ index ].info, va ( "%s%2d", strClassHighlights[ ( cg.mvOverlay[ index ].classID * 2 ) + 1 ], pID ) );
	}

	cg.mvOverlay[ index ].width = CG_DrawStrlen ( cg.mvOverlay[ index ].info ) * MVINFO_TEXTSIZE;
}

// Update info on all clients received for display/cursor interaction
void CG_mvOverlayUpdate ( void )
{
	int i, cnt;

	for ( i = 0, cnt = 0; i < MAX_MVCLIENTS && cnt < cg.mvTotalClients; i++ )
	{
		if ( cg.mvClientList & ( 1 << i ) )
		{
			CG_mvOverlayClientUpdate ( i, cnt++ );
		}
	}
}

// See if we have the client in our snapshot
qboolean CG_mvMergedClientLocate ( int pID )
{
	int i;

	for ( i = 0; i < cg.mvTotalClients; i++ )
	{
		if ( cg.mvOverlay[ i ].pID == pID )
		{
			return ( qtrue );
		}
	}

	return ( qfalse );
}

// Display available client info
void CG_mvOverlayDisplay ( void )
{
	int         j, i, x, y, pID;
	cg_mvinfo_t *o;

	if ( cg.mvTotalClients < 1 )
	{
		return;
	}

	y = MVINFO_TOP - ( 2 * ( MVINFO_TEXTSIZE + 1 ) );

	for ( j = TEAM_AXIS; j <= TEAM_ALLIES; j++ )
	{
		cg.mvTotalTeam[ j ] = 0;

		for ( i = 0; i < cg.mvTotalClients; i++ )
		{
			o = &cg.mvOverlay[ i ];
			pID = o->pID;

			if ( cgs.clientinfo[ pID ].team != j )
			{
				continue;
			}

			if ( cg.mvTotalTeam[ j ] == 0 )
			{
				char *flag = ( j == TEAM_AXIS ) ? "ui/assets/ger_flag.tga" : "ui/assets/usa_flag.tga";

				y += 2 * ( MVINFO_TEXTSIZE + 1 );
				CG_DrawPic ( MVINFO_RIGHT - ( 2 * MVINFO_TEXTSIZE ), y, 2 * MVINFO_TEXTSIZE, MVINFO_TEXTSIZE,
				             trap_R_RegisterShaderNoMip ( flag ) );
			}

			// Update team list info for mouse detection
			cg.mvTeamList[ j ][ ( cg.mvTotalTeam[ j ] ) ] = i;
			cg.mvTotalTeam[ j ]++;

			// Update any class changes
			if ( o->classID != cg_entities[ pID ].currentState.teamNum )
			{
				CG_mvOverlayClientUpdate ( o->pID, i );
			}

			x = MVINFO_RIGHT - o->width;
			y += MVINFO_TEXTSIZE + 1;

			if ( o->fActive )
			{
				CG_FillRect ( x - 1, y, o->width + 2, MVINFO_TEXTSIZE + 2, colorMdYellow );

				// Draw name info only if we're hovering over the text element
				if ( ! ( cg.mvCurrentActive->mvInfo & MV_SELECTED ) || cg.mvCurrentActive == cg.mvCurrentMainview )
				{
					int w = CG_DrawStrlen ( cgs.clientinfo[ pID ].name ) * ( MVINFO_TEXTSIZE - 1 );

					CG_FillRect ( x - 1 - w - 6, y + 1, w + 2, MVINFO_TEXTSIZE - 1 + 2, colorMdGrey );
					CG_DrawStringExt ( x - w - 6, y + 1,
					                   cgs.clientinfo[ pID ].name,
					                   colorYellow, qtrue, qtrue, MVINFO_TEXTSIZE - 1, MVINFO_TEXTSIZE - 1, 0 );
				}
			}

			CG_DrawStringExt ( x, y, o->info, colorWhite, qfalse, qtrue, MVINFO_TEXTSIZE, MVINFO_TEXTSIZE, 0 );
		}
	}
}

//////////////////////////////////////
//
// Wolf-specific utilities
//
//////////////////////////////////////
void CG_mvZoomSniper ( float x, float y, float w, float h )
{
	float ws = w / 640;
	float hs = h / 480;

	// sides
	CG_FillRect ( x, y, 80.0f * ws, 480.0f * hs, colorBlack );
	CG_FillRect ( x + 560.0f * ws, y, 80.0f * ws, 480.0f * hs, colorBlack );

	// center
	if ( cgs.media.reticleShaderSimple )
	{
		CG_DrawPic ( x + 80.0f * ws, y, 480.0f * ws, 480.0f * hs, cgs.media.reticleShaderSimple );
	}

	// hairs
	CG_FillRect ( x + 84.0f * ws, y + 239.0f * hs, 177.0f * ws, 2.0f, colorBlack ); // left
	CG_FillRect ( x + 320.0f * ws, y + 242.0f * hs, 1.0f, 58.0f * hs, colorBlack ); // center top
	CG_FillRect ( x + 319.0f * ws, y + 300.0f * hs, 2.0f, 178.0f * hs, colorBlack ); // center bot
	CG_FillRect ( x + 380.0f * ws, y + 239.0f * hs, 177.0f * ws, 2.0f, colorBlack ); // right
}

void CG_mvZoomBinoc ( float x, float y, float w, float h )
{
	float ws = w / 640;
	float hs = h / 480;

	// an alternative.  This gives nice sharp lines at the expense of a few extra polys
	if ( cgs.media.binocShaderSimple )
	{
		CG_DrawPic ( x, y, 640.0f * ws, 480.0f * ws, cgs.media.binocShaderSimple );
	}

	CG_FillRect ( x + 146.0f * ws, y + 239.0f * hs, 348.0f * ws, 1, colorBlack );

	CG_FillRect ( x + 188.0f * ws, y + 234.0f * hs, 1, 13.0f * hs, colorBlack ); // ll
	CG_FillRect ( x + 234.0f * ws, y + 226.0f * hs, 1, 29.0f * hs, colorBlack ); // l
	CG_FillRect ( x + 274.0f * ws, y + 234.0f * hs, 1, 13.0f * hs, colorBlack ); // lr
	CG_FillRect ( x + 320.0f * ws, y + 213.0f * hs, 1, 55.0f * hs, colorBlack ); // center
	CG_FillRect ( x + 360.0f * ws, y + 234.0f * hs, 1, 13.0f * hs, colorBlack ); // rl
	CG_FillRect ( x + 406.0f * ws, y + 226.0f * hs, 1, 29.0f * hs, colorBlack ); // r
	CG_FillRect ( x + 452.0f * ws, y + 234.0f * hs, 1, 13.0f * hs, colorBlack ); // rr
}

void CG_mv_KeyHandling ( int _key, qboolean down )
{
	int milli = trap_Milliseconds();
	int key = _key;

	// Avoid active console keypress issues
	if ( !down && !cgs.fKeyPressed[ key ] )
	{
		return;
	}

	cgs.fKeyPressed[ key ] = down;

	switch ( key )
	{
		case K_TAB:
			if ( down ) { CG_ScoresDown_f(); }
			else { CG_ScoresUp_f(); }

			return;

			// Help info
		case K_BACKSPACE:
			if ( !down )
			{
				// Dushan - fixed comiler warning
				CG_toggleSpecHelp_f();
			}

			return;

			// Screenshot keys
		case K_F11:
			if ( !down ) { trap_SendConsoleCommand ( va ( "screenshot%s\n", ( ( cg_useScreenshotJPEG.integer ) ? "JPEG" : "" ) ) ); }

			return;

		case K_F12:
			if ( !down ) { CG_autoScreenShot_f(); }

			return;

			// Window controls
		case K_SHIFT:
		case K_CTRL:
		case K_MOUSE4:
			cgs.fResize = down;
			return;

		case K_MOUSE1:
			cgs.fSelect = down;
			return;

		case K_MOUSE2:
			if ( !down )
			{
				CG_mvSwapViews_f(); // Swap the window with the main view
			}

			return;

		case K_INS:
		case K_KP_PGUP:
		case K_MWHEELDOWN:
			if ( !down )
			{
				CG_mvShowView_f(); // Make a window for the client
			}

			return;

		case K_DEL:
		case K_KP_PGDN:
		case K_MWHEELUP:
			if ( !down )
			{
				CG_mvHideView_f(); // Delete the window for the client
			}

			return;

		case K_MOUSE3:
			if ( !down )
			{
				CG_mvToggleView_f(); // Toggle a window for the client
			}

			return;

		case 'm':
		case 'M':
			if ( !down )
			{
				CG_mvToggleAll_f();
			}

			return;

		case K_ESCAPE: // K_ESCAPE only fires on Key Up
		case K_ESCAPE | K_CHAR_FLAG:
			CG_mvToggleAll_f();
			return;

			// Third-person controls
		case K_ENTER:
			if ( !down )
			{
				trap_Cvar_Set ( "cg_thirdperson", ( ( cg_thirdPerson.integer == 0 ) ? "1" : "0" ) );
			}

			return;

		case K_UPARROW:
			if ( milli > cgs.thirdpersonUpdate )
			{
				float range = cg_thirdPersonRange.value;

				cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;
				range -= ( ( range >= 4 * DEMO_RANGEDELTA ) ? DEMO_RANGEDELTA : ( range - DEMO_RANGEDELTA ) );
				trap_Cvar_Set ( "cg_thirdPersonRange", va ( "%f", range ) );
			}

			return;

		case K_DOWNARROW:
			if ( milli > cgs.thirdpersonUpdate )
			{
				float range = cg_thirdPersonRange.value;

				cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;
				range += ( ( range >= 120 * DEMO_RANGEDELTA ) ? 0 : DEMO_RANGEDELTA );
				trap_Cvar_Set ( "cg_thirdPersonRange", va ( "%f", range ) );
			}

			return;

		case K_RIGHTARROW:
			if ( milli > cgs.thirdpersonUpdate )
			{
				float angle = cg_thirdPersonAngle.value - DEMO_ANGLEDELTA;

				cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;

				if ( angle < 0 ) { angle += 360.0f; }

				trap_Cvar_Set ( "cg_thirdPersonAngle", va ( "%f", angle ) );
			}

			return;

		case K_LEFTARROW:
			if ( milli > cgs.thirdpersonUpdate )
			{
				float angle = cg_thirdPersonAngle.value + DEMO_ANGLEDELTA;

				cgs.thirdpersonUpdate = milli + DEMO_THIRDPERSONUPDATE;

				if ( angle >= 360.0f ) { angle -= 360.0f; }

				trap_Cvar_Set ( "cg_thirdPersonAngle", va ( "%f", angle ) );
			}

			return;
	}
}
