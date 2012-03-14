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

#include "cg_local.h"

int CG_DrawField ( int x, int y, int width, int value, int charWidth, int charHeight, qboolean dodrawpic, qboolean leftAlign ); // NERVE - SMF

/*void CG_FitTextToWidth( char* instr, int w, int size) {
        char buffer[1024];
        char  *s, *p, *c, *ls;
        int   l;

        strcpy(buffer, instr);
        memset(instr, 0, size);

        c = s = instr;
        p = buffer;
        ls = NULL;
        l = 0;
        while(*p) {
                *c = *p++;
                l++;

                if(*c == ' ') {
                        ls = c;
                } // store last space, to try not to break mid word

                c++;

                if(*p == '\n') {
                        s = c+1;
                        l = 0;
                } else if(l > w) {
                        if(ls) {
                                *ls = '\n';
                                l = strlen(ls+1);
                        } else {
                                *c = *(c-1);
                                *(c-1) = '\n';
                                s = c++;
                                l = 0;
                        }

                        ls = NULL;
                }
        }

        if(c != buffer && (*(c-1) != '\n')) {
                *c++ = '\n';
        }

        *c = '\0';
}*/

int CG_TrimLeftPixels ( char *instr, float scale, float w, int size )
{
	char buffer[ 1024 ];
	char *p, *s;
	int  tw;
	int  i;

	Q_strncpyz ( buffer, instr, 1024 );
	memset ( instr, 0, size );

	for ( i = 0, p = buffer; *p; p++, i++ )
	{
		instr[ i ] = *p;
		tw = CG_Text_Width ( instr, scale, 0 );

		if ( tw >= w )
		{
			memset ( instr, 0, size );

			for ( s = instr, p = &buffer[ i + 1 ]; *p && ( ( s - instr ) < size ); p++, s++ )
			{
				*s = *p;
			}

			return tw - w;
		}
	}

	return -1;
}

void CG_FitTextToWidth_Ext ( char *instr, float scale, float w, int size, fontInfo_t *font )
{
	char buffer[ 1024 ];
	char *s, *p, *c, *ls;
	int  l;

	Q_strncpyz ( buffer, instr, 1024 );
	memset ( instr, 0, size );

	c = s = instr;
	p = buffer;
	ls = NULL;
	l = 0;

	while ( *p )
	{
		*c = *p++;
		l++;

		if ( *c == ' ' )
		{
			ls = c;
		} // store last space, to try not to break mid word

		c++;

		if ( *p == '\n' )
		{
			s = c + 1;
			l = 0;
		}
		else if ( CG_Text_Width_Ext ( s, scale, 0, font ) > w )
		{
			if ( ls )
			{
				*ls = '\n';
				s = ls + 1;
			}
			else
			{
				*c = * ( c - 1 );
				* ( c - 1 ) = '\n';
				s = c++;
			}

			ls = NULL;
			l = 0;
		}
	}

	if ( c != buffer && ( * ( c - 1 ) != '\n' ) )
	{
		*c++ = '\n';
	}

	*c = '\0';
}

void CG_FitTextToWidth2 ( char *instr, float scale, float w, int size )
{
	char buffer[ 1024 ];
	char *s, *p, *c, *ls;
	int  l;

	Q_strncpyz ( buffer, instr, 1024 );
	memset ( instr, 0, size );

	c = s = instr;
	p = buffer;
	ls = NULL;
	l = 0;

	while ( *p )
	{
		*c = *p++;
		l++;

		if ( *c == ' ' )
		{
			ls = c;
		} // store last space, to try not to break mid word

		c++;

		if ( *p == '\n' )
		{
			s = c + 1;
			l = 0;
		}
		else if ( CG_Text_Width ( s, scale, 0 ) > w )
		{
			if ( ls )
			{
				*ls = '\n';
				s = ls + 1;
			}
			else
			{
				*c = * ( c - 1 );
				* ( c - 1 ) = '\n';
				s = c++;
			}

			ls = NULL;
			l = 0;
		}
	}

	if ( c != buffer && ( * ( c - 1 ) != '\n' ) )
	{
		*c++ = '\n';
	}

	*c = '\0';
}

void CG_FitTextToWidth_SingleLine ( char *instr, float scale, float w, int size )
{
	char *s, *p;
	char buffer[ 1024 ];

	Q_strncpyz ( buffer, instr, 1024 );
	memset ( instr, 0, size );
	p = instr;

	for ( s = buffer; *s; s++, p++ )
	{
		*p = *s;

		if ( CG_Text_Width ( instr, scale, 0 ) > w )
		{
			*p = '\0';
			return;
		}
	}
}

/*
==============
weapIconDrawSize
==============
*/
static int weapIconDrawSize ( int weap )
{
	switch ( weap )
	{
			// weapons to not draw
//      case WP_KNIFE:
//          return 0;

			// weapons with 'wide' icons
		case WP_THOMPSON:
		case WP_MP40:
		case WP_STEN:
		case WP_PANZERFAUST:
		case WP_FLAMETHROWER:

//      case WP_SPEARGUN:
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
		case WP_GARAND_SCOPE:
		case WP_K43_SCOPE:
		case WP_MORTAR:
		case WP_MORTAR_SET:
			return 2;
	}

	return 1;
}

/*
==============
CG_DrawPlayerWeaponIcon
==============
*/
void CG_DrawPlayerWeaponIcon ( rectDef_t *rect, qboolean drawHighlighted, int align, vec4_t *refcolor )
{
	int       size;
	int       realweap; // DHM - Nerve
	qhandle_t icon;
	float     scale, halfScale;
	vec4_t    hcolor;

	VectorCopy ( *refcolor, hcolor );
	hcolor[ 3 ] = 1.f;

	if ( cg.predictedPlayerEntity.currentState.eFlags & EF_MG42_ACTIVE ||
	     cg.predictedPlayerEntity.currentState.eFlags & EF_MOUNTEDTANK )
	{
		realweap = WP_MOBILE_MG42;
	}
	else
	{
		realweap = cg.predictedPlayerState.weapon;
	}

	size = weapIconDrawSize ( realweap );

	if ( !size )
	{
		return;
	}

	if ( cg.predictedPlayerEntity.currentState.eFlags & EF_MOUNTEDTANK &&
	     cg_entities[ cg_entities[ cg_entities[ cg.snap->ps.clientNum ].tagParent ].tankparent ].currentState.density & 8 )
	{
		icon = cgs.media.browningIcon;
	}
	else
	{
		if ( drawHighlighted )
		{
			//icon = cg_weapons[ realweap ].weaponIcon[1];
			icon = cg_weapons[ realweap ].weaponIcon[ 1 ]; // we don't have icon[0];
		}
		else
		{
			icon = cg_weapons[ realweap ].weaponIcon[ 1 ];
		}
	}

	// pulsing grenade icon to help the player 'count' in their head
	if ( cg.predictedPlayerState.grenadeTimeLeft )
	{
		// grenades and dynamite set this
		// these time differently
		if ( realweap == WP_DYNAMITE )
		{
			if ( ( ( cg.grenLastTime ) % 1000 ) > ( ( cg.predictedPlayerState.grenadeTimeLeft ) % 1000 ) )
			{
				trap_S_StartLocalSound ( cgs.media.grenadePulseSound4, CHAN_LOCAL_SOUND );
			}
		}
		else
		{
			if ( ( ( cg.grenLastTime ) % 1000 ) < ( ( cg.predictedPlayerState.grenadeTimeLeft ) % 1000 ) )
			{
				switch ( cg.predictedPlayerState.grenadeTimeLeft / 1000 )
				{
					case 3:
						trap_S_StartLocalSound ( cgs.media.grenadePulseSound4, CHAN_LOCAL_SOUND );
						break;

					case 2:
						trap_S_StartLocalSound ( cgs.media.grenadePulseSound3, CHAN_LOCAL_SOUND );
						break;

					case 1:
						trap_S_StartLocalSound ( cgs.media.grenadePulseSound2, CHAN_LOCAL_SOUND );
						break;

					case 0:
						trap_S_StartLocalSound ( cgs.media.grenadePulseSound1, CHAN_LOCAL_SOUND );
						break;
				}
			}
		}

		scale = ( float ) ( ( cg.predictedPlayerState.grenadeTimeLeft ) % 1000 ) / 100.0f;
		halfScale = scale * 0.5f;

		cg.grenLastTime = cg.predictedPlayerState.grenadeTimeLeft;
	}
	else
	{
		scale = halfScale = 0;
	}

	if ( icon )
	{
		float x, y, w, h;

		if ( size == 1 )
		{
			// draw half width to match the icon asset
			// start at left
			x = rect->x - halfScale;
			y = rect->y - halfScale;
			w = rect->w / 2 + scale;
			h = rect->h + scale;

			switch ( align )
			{
				case ITEM_ALIGN_CENTER:
					x += rect->w / 4;
					break;

				case ITEM_ALIGN_RIGHT:
					x += rect->w / 2;
					break;

				case ITEM_ALIGN_LEFT:
				default:
					break;
			}
		}
		else
		{
			x = rect->x - halfScale;
			y = rect->y - halfScale;
			w = rect->w + scale;
			h = rect->h + scale;
		}

		trap_R_SetColor ( hcolor ); // JPW NERVE
		CG_DrawPic ( x, y, w, h, icon );
	}
}

#define CURSORHINT_SCALE 10

/*
==============
CG_DrawCursorHints

  cg_cursorHints.integer ==
        0:  no hints
        1:  sin size pulse
        2:  one way size pulse
        3:  alpha pulse
        4+: static image

==============
*/
void CG_DrawCursorhint ( rectDef_t *rect )
{
	float     *color;
	qhandle_t icon, icon2 = 0;
	float     scale, halfscale;

	//qboolean  redbar = qfalse;
	qboolean  yellowbar = qfalse;

	if ( !cg_cursorHints.integer )
	{
		return;
	}

	CG_CheckForCursorHints();

	switch ( cg.cursorHintIcon )
	{
		case HINT_NONE:
		case HINT_FORCENONE:
			icon = 0;
			break;

		case HINT_DOOR:
			icon = cgs.media.doorHintShader;
			break;

		case HINT_DOOR_ROTATING:
			icon = cgs.media.doorRotateHintShader;
			break;

		case HINT_DOOR_LOCKED:
			icon = cgs.media.doorLockHintShader;
			break;

		case HINT_DOOR_ROTATING_LOCKED:
			icon = cgs.media.doorRotateLockHintShader;
			break;

		case HINT_MG42:
			icon = cgs.media.mg42HintShader;
			break;

		case HINT_BREAKABLE:
			icon = cgs.media.breakableHintShader;
			break;

		case HINT_BREAKABLE_DYNAMITE:
			icon = cgs.media.dynamiteHintShader;
			break;

		case HINT_TANK:
			icon = cgs.media.tankHintShader;
			break;

		case HINT_SATCHELCHARGE:
			icon = cgs.media.satchelchargeHintShader;
			break;

		case HINT_CONSTRUCTIBLE:
			icon = cgs.media.buildHintShader;
			break;

		case HINT_UNIFORM:
			icon = cgs.media.uniformHintShader;
			break;

		case HINT_LANDMINE:
			icon = cgs.media.landmineHintShader;
			break;

		case HINT_CHAIR:
			icon = cgs.media.notUsableHintShader;

			// only show 'pickupable' if you're not armed, or are armed with a single handed weapon

			// rain - WEAPS_ONE_HANDED isn't valid anymore, because
			// WP_SILENCED_COLT uses a bit >31 (and, therefore, is too large
			// to be shifted in the way WEAPS_ONE_HANDED does on a 32-bit
			// system.) If you want to use HINT_CHAIR, you'll need to fix
			// this.
#if 0

			if ( ! ( cg.predictedPlayerState.weapon ) || WEAPS_ONE_HANDED & ( 1 << ( cg.predictedPlayerState.weapon ) ) )
			{
				icon = cgs.media.chairHintShader;
			}

#endif
			break;

		case HINT_ALARM:
			icon = cgs.media.alarmHintShader;
			break;

		case HINT_HEALTH:
			icon = cgs.media.healthHintShader;
			break;

		case HINT_TREASURE:
			icon = cgs.media.treasureHintShader;
			break;

		case HINT_KNIFE:
			icon = cgs.media.knifeHintShader;
			break;

		case HINT_LADDER:
			icon = cgs.media.ladderHintShader;
			break;

		case HINT_BUTTON:
			icon = cgs.media.buttonHintShader;
			break;

		case HINT_WATER:
			icon = cgs.media.waterHintShader;
			break;

		case HINT_CAUTION:
			icon = cgs.media.cautionHintShader;
			break;

		case HINT_DANGER:
			icon = cgs.media.dangerHintShader;
			break;

		case HINT_SECRET:
			icon = cgs.media.secretHintShader;
			break;

		case HINT_QUESTION:
			icon = cgs.media.qeustionHintShader;
			break;

		case HINT_EXCLAMATION:
			icon = cgs.media.exclamationHintShader;
			break;

		case HINT_CLIPBOARD:
			icon = cgs.media.clipboardHintShader;
			break;

		case HINT_WEAPON:
			icon = cgs.media.weaponHintShader;
			break;

		case HINT_AMMO:
			icon = cgs.media.ammoHintShader;
			break;

		case HINT_ARMOR:
			icon = cgs.media.armorHintShader;
			break;

		case HINT_POWERUP:
			icon = cgs.media.powerupHintShader;
			break;

		case HINT_HOLDABLE:
			icon = cgs.media.holdableHintShader;
			break;

		case HINT_INVENTORY:
			icon = cgs.media.inventoryHintShader;
			break;

		case HINT_PLYR_FRIEND:
			icon = cgs.media.hintPlrFriendShader;
			break;

		case HINT_PLYR_NEUTRAL:
			icon = cgs.media.hintPlrNeutralShader;
			break;

		case HINT_PLYR_ENEMY:
			icon = cgs.media.hintPlrEnemyShader;
			break;

		case HINT_PLYR_UNKNOWN:
			icon = cgs.media.hintPlrUnknownShader;
			break;

			// DHM - Nerve :: multiplayer hints
		case HINT_BUILD:
			icon = cgs.media.buildHintShader;
			break;

		case HINT_DISARM:
			icon = cgs.media.disarmHintShader;
			break;

		case HINT_REVIVE:
			icon = cgs.media.reviveHintShader;
			break;

		case HINT_DYNAMITE:
			icon = cgs.media.dynamiteHintShader;
			break;
			// dhm - end

			// Mad Doc - TDF
		case HINT_LOCKPICK:
			icon = cgs.media.doorLockHintShader; // TAT 1/30/2003 - use the locked door hint cursor
			yellowbar = qtrue; // draw the status bar in yellow so it shows up better
			break;

		case HINT_ACTIVATE:
		case HINT_PLAYER:
		default:
			icon = cgs.media.usableHintShader;
			break;
	}

	if ( !icon )
	{
		return;
	}

	// color
	color = CG_FadeColor ( cg.cursorHintTime, cg.cursorHintFade );

	if ( !color )
	{
		trap_R_SetColor ( NULL );
		return;
	}

	if ( cg_cursorHints.integer == 3 )
	{
		color[ 3 ] *= 0.5 + 0.5 * sin ( ( float ) cg.time / 150.0 );
	}

	// size
	if ( cg_cursorHints.integer >= 3 )
	{
		// no size pulsing
		scale = halfscale = 0;
	}
	else
	{
		if ( cg_cursorHints.integer == 2 )
		{
			scale = ( float ) ( ( cg.cursorHintTime ) % 1000 ) / 100.0f; // one way size pulse
		}
		else
		{
			scale = CURSORHINT_SCALE * ( 0.5 + 0.5 * sin ( ( float ) cg.time / 150.0 ) ); // sin pulse
		}

		halfscale = scale * 0.5f;
	}

	// set color and draw the hint
	trap_R_SetColor ( color );
	CG_DrawPic ( rect->x - halfscale, rect->y - halfscale, rect->w + scale, rect->h + scale, icon );

	if ( icon2 )
	{
		CG_DrawPic ( rect->x - halfscale, rect->y - halfscale, rect->w + scale, rect->h + scale, icon2 );
	}

	trap_R_SetColor ( NULL );

	// draw status bar under the cursor hint
	if ( cg.cursorHintValue )
	{
		if ( yellowbar )
		{
			Vector4Set ( color, 1, 1, 0, 1.0f );
		}
		else
		{
			Vector4Set ( color, 0, 0, 1, 0.5f );
		}

		CG_FilledBar ( rect->x, rect->y + rect->h + 4, rect->w, 8, color, NULL, NULL, ( float ) cg.cursorHintValue / 255.0f, 0 );
	}
}

float CG_GetValue ( int ownerDraw, int type )
{
	switch ( ownerDraw )
	{
		default:
			break;
	}

	return -1;
}

qboolean CG_OtherTeamHasFlag()
{
	return qfalse;
}

qboolean CG_YourTeamHasFlag()
{
	return qfalse;
}

// THINKABOUTME: should these be exclusive or inclusive..
//
qboolean CG_OwnerDrawVisible ( int flags )
{
	return qfalse;
}

#define PIC_WIDTH 12

/*
==============
CG_DrawWeapStability
        draw a bar showing current stability level (0-255), max at current weapon/ability, and 'perfect' reference mark

        probably only drawn for scoped weapons
==============
*/
void CG_DrawWeapStability ( rectDef_t *rect )
{
	vec4_t goodColor = { 0, 1, 0, 0.5f }, badColor =
	{
		1, 0, 0, 0.5f
	};

	if ( !cg_drawSpreadScale.integer )
	{
		return;
	}

	if ( cg_drawSpreadScale.integer == 1 && !BG_IsScopedWeapon ( cg.predictedPlayerState.weapon ) )
	{
		// cg_drawSpreadScale of '1' means only draw for scoped weapons, '2' means draw all the time (for debugging)
		return;
	}

	if ( cg.predictedPlayerState.weaponstate != WEAPON_READY )
	{
		return;
	}

	if ( ! ( cg.snap->ps.aimSpreadScale ) )
	{
		return;
	}

	if ( cg.renderingThirdPerson )
	{
		return;
	}

	CG_FilledBar ( rect->x, rect->y, rect->w, rect->h, goodColor, badColor, NULL, ( float ) cg.snap->ps.aimSpreadScale / 255.0f, 2 | 4 | 256 ); // flags (BAR_CENTER|BAR_VERT|BAR_LERP_COLOR)
}

/*
==============
CG_DrawWeapHeat
==============
*/
void CG_DrawWeapHeat ( rectDef_t *rect, int align )
{
	vec4_t color = { 1, 0, 0, 0.2f }, color2 =
	{
		1, 0, 0, 0.5f
	};
	int    flags = 0;

	if ( ! ( cg.snap->ps.curWeapHeat ) )
	{
		return;
	}

	if ( align != HUD_HORIZONTAL )
	{
		flags |= 4; // BAR_VERT
	}

	flags |= 1; // BAR_LEFT           - this is hardcoded now, but will be decided by the menu script
	flags |= 16; // BAR_BG         - draw the filled contrast box
//  flags|=32;      // BAR_BGSPACING_X0Y5   - different style

	flags |= 256; // BAR_COLOR_LERP

	CG_FilledBar ( rect->x, rect->y, rect->w, rect->h, color, color2, NULL, ( float ) cg.snap->ps.curWeapHeat / 255.0f, flags );
}

/*
==============
CG_OwnerDraw
==============
*/
void CG_OwnerDraw ( float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags, int align,
                    float special, float scale, vec4_t color, qhandle_t shader, int textStyle )
{
	rectDef_t rect;

	if ( cg_drawStatus.integer == 0 )
	{
		return;
	}

	rect.x = x;
	rect.y = y;
	rect.w = w;
	rect.h = h;

	switch ( ownerDraw )
	{
		default:
			break;
	}
}

void CG_MouseEvent ( int x, int y )
{
	switch ( cgs.eventHandling )
	{
		case CGAME_EVENT_SPEAKEREDITOR:
		case CGAME_EVENT_GAMEVIEW:
		case CGAME_EVENT_CAMPAIGNBREIFING:
		case CGAME_EVENT_FIRETEAMMSG:
			cgs.cursorX += x;

			if ( cgs.cursorX < 0 )
			{
				cgs.cursorX = 0;
			}
			else if ( cgs.cursorX > 640 )
			{
				cgs.cursorX = 640;
			}

			cgs.cursorY += y;

			if ( cgs.cursorY < 0 )
			{
				cgs.cursorY = 0;
			}
			else if ( cgs.cursorY > 480 )
			{
				cgs.cursorY = 480;
			}

			if ( cgs.eventHandling == CGAME_EVENT_SPEAKEREDITOR )
			{
				CG_SpeakerEditorMouseMove_Handling ( x, y );
			}

			break;

		case CGAME_EVENT_DEMO:
		case CGAME_EVENT_MULTIVIEW:
			cgs.cursorX += x;

			if ( cgs.cursorX < 0 )
			{
				cgs.cursorX = 0;
			}
			else if ( cgs.cursorX > 640 )
			{
				cgs.cursorX = 640;
			}

			cgs.cursorY += y;

			if ( cgs.cursorY < 0 )
			{
				cgs.cursorY = 0;
			}
			else if ( cgs.cursorY > 480 )
			{
				cgs.cursorY = 480;
			}

			if ( x != 0 || y != 0 )
			{
				cgs.cursorUpdate = cg.time + 5000;
			}

			break;

		default:
			if ( cg.snap->ps.pm_type == PM_INTERMISSION )
			{
				CG_Debriefing_MouseEvent ( x, y );
				return;
			}

			// default handling
			if ( ( cg.predictedPlayerState.pm_type == PM_NORMAL ||
			       cg.predictedPlayerState.pm_type == PM_SPECTATOR ) && cg.showScores == qfalse )
			{
				trap_Key_SetCatcher ( trap_Key_GetCatcher() & ~KEYCATCH_CGAME );
				return;
			}

			break;
	}
}

/*
==================
CG_EventHandling
==================
*/
void CG_EventHandling ( int type, qboolean fForced )
{
	if ( cg.demoPlayback && type == CGAME_EVENT_NONE && !fForced )
	{
		type = CGAME_EVENT_DEMO;
	}

	if ( type != CGAME_EVENT_NONE )
	{
		trap_Cvar_Set ( "cl_bypassMouseInput", 0 );
	}

	switch ( type )
	{
			// OSP - Demo support
		case CGAME_EVENT_DEMO:
			cgs.fResize = qfalse;
			cgs.fSelect = qfalse;
			cgs.cursorUpdate = cg.time + 10000;
			cgs.timescaleUpdate = cg.time + 4000;
			CG_ScoresUp_f();
			break;

		case CGAME_EVENT_SPEAKEREDITOR:
		case CGAME_EVENT_GAMEVIEW:
		case CGAME_EVENT_NONE:
		case CGAME_EVENT_CAMPAIGNBREIFING:
		case CGAME_EVENT_FIRETEAMMSG:
		case CGAME_EVENT_MULTIVIEW:
		default:

			// default handling (cleanup mostly)
			if ( cgs.eventHandling == CGAME_EVENT_GAMEVIEW )
			{
				cg.showGameView = qfalse;
				trap_S_FadeBackgroundTrack ( 0.0f, 500, 0 );

				trap_S_StopStreamingSound ( -1 );
				cg.limboEndCinematicTime = 0;

				if ( fForced )
				{
					if ( cgs.limboLoadoutModified )
					{
						trap_SendClientCommand ( "rs" );

						cgs.limboLoadoutSelected = qfalse;
					}
				}
			}
			else if ( cgs.eventHandling == CGAME_EVENT_MULTIVIEW )
			{
				if ( type == -CGAME_EVENT_MULTIVIEW )
				{
					type = CGAME_EVENT_NONE;
				}
				else
				{
					trap_Key_SetCatcher ( KEYCATCH_CGAME );
					return;
				}
			}
			else if ( cgs.eventHandling == CGAME_EVENT_SPEAKEREDITOR )
			{
				if ( type == -CGAME_EVENT_SPEAKEREDITOR )
				{
					type = CGAME_EVENT_NONE;
				}
				else
				{
					trap_Key_SetCatcher ( KEYCATCH_CGAME );
					return;
				}
			}
			else if ( cgs.eventHandling == CGAME_EVENT_CAMPAIGNBREIFING )
			{
				type = CGAME_EVENT_GAMEVIEW;
			}
			else if ( cgs.eventHandling == CGAME_EVENT_FIRETEAMMSG )
			{
				cg.showFireteamMenu = qfalse;
				trap_Cvar_Set ( "cl_bypassmouseinput", "0" );
			}
			else if ( cg.snap && cg.snap->ps.pm_type == PM_INTERMISSION && fForced )
			{
				trap_UI_Popup ( UIMENU_INGAME );
			}

			break;
	}

	cgs.eventHandling = type;

	if ( type == CGAME_EVENT_NONE )
	{
		trap_Key_SetCatcher ( trap_Key_GetCatcher() & ~KEYCATCH_CGAME );
		ccInitial = qfalse;

		if ( cg.demoPlayback && cg.demohelpWindow != SHOW_OFF )
		{
			CG_ShowHelp_Off ( &cg.demohelpWindow );
		}
	}
	else if ( type == CGAME_EVENT_GAMEVIEW )
	{
		cg.showGameView = qtrue;
		CG_LimboPanel_Setup();
		trap_Key_SetCatcher ( KEYCATCH_CGAME );
	}
	else if ( type == CGAME_EVENT_MULTIVIEW )
	{
		trap_Key_SetCatcher ( KEYCATCH_CGAME );
	}
	else if ( type == CGAME_EVENT_FIRETEAMMSG )
	{
		cgs.ftMenuPos = -1;
		cgs.ftMenuMode = 0;
		cg.showFireteamMenu = qtrue;
		trap_Cvar_Set ( "cl_bypassmouseinput", "1" );
		trap_Key_SetCatcher ( KEYCATCH_CGAME );
	}
	else
	{
		trap_Key_SetCatcher ( KEYCATCH_CGAME );
	}
}

void CG_KeyEvent ( int key, qboolean down )
{
	switch ( cgs.eventHandling )
	{
			// Demos get their own keys
		case CGAME_EVENT_DEMO:
			CG_DemoClick ( key, down );
			return;

		case CGAME_EVENT_CAMPAIGNBREIFING:
			CG_LoadPanel_KeyHandling ( key, down );
			break;

		case CGAME_EVENT_FIRETEAMMSG:
			CG_Fireteams_KeyHandling ( key, down );
			break;

		case CGAME_EVENT_GAMEVIEW:
			CG_LimboPanel_KeyHandling ( key, down );
			break;

		case CGAME_EVENT_SPEAKEREDITOR:
			CG_SpeakerEditor_KeyHandling ( key, down );
			break;

		case CGAME_EVENT_MULTIVIEW:
			CG_mv_KeyHandling ( key, down );
			break;

		default:
			if ( cg.snap->ps.pm_type == PM_INTERMISSION )
			{
				CG_Debriefing_KeyEvent ( key, down );
				return;
			}

			// default handling
			if ( !down )
			{
				return;
			}

			if ( ( cg.predictedPlayerState.pm_type == PM_NORMAL ||
			       ( cg.predictedPlayerState.pm_type == PM_SPECTATOR && cg.showScores == qfalse ) ) )
			{
				CG_EventHandling ( CGAME_EVENT_NONE, qfalse );
				return;
			}

			break;
	}
}

int CG_ClientNumFromName ( const char *p )
{
	int i;

	for ( i = 0; i < cgs.maxclients; i++ )
	{
		if ( cgs.clientinfo[ i ].infoValid && Q_stricmp ( cgs.clientinfo[ i ].name, p ) == 0 )
		{
			return i;
		}
	}

	return -1;
}

void CG_GetTeamColor ( vec4_t *color )
{
	if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_AXIS )
	{
		( *color ) [ 0 ] = 1;
		( *color ) [ 3 ] = .25f;
		( *color ) [ 1 ] = ( *color ) [ 2 ] = 0;
	}
	else if ( cg.snap->ps.persistant[ PERS_TEAM ] == TEAM_ALLIES )
	{
		( *color ) [ 0 ] = ( *color ) [ 1 ] = 0;
		( *color ) [ 2 ] = 1;
		( *color ) [ 3 ] = .25f;
	}
	else
	{
		( *color ) [ 0 ] = ( *color ) [ 2 ] = 0;
		( *color ) [ 1 ] = .17f;
		( *color ) [ 3 ] = .25f;
	}
}

void CG_RunMenuScript ( char **args )
{
}
