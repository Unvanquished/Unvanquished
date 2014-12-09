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

#define MAX_HUD_ITEMS	128
#define MAX_HUD_STRINGS (16*1024)

#define HUD_TEXT_WIDTH 10
#define HUD_TEXT_HEIGHT 13
#define HUD_TEXT_SPACING 13
#define HUD_SCRIPT_LENGTH 50
#define HUD_FLOAT "%.3f"

typedef enum {
	hudTypeNone,
	hudTypeHandler,
	hudTypeValue,
	hudTypeFloat,
	hudTypeButton,
	hudTypeCvar,
	hudTypeText,
	hudTypeCheck
} hudType_t;

typedef struct {
	float		x,y;
	hudType_t	type;
	const		char *text, *cvar;
	int			textLen;
	float		*value;
	int			*time;
	int			handler;
	int			showMask;
} hudItem_t;

static hudItem_t hudItems [MAX_HUD_ITEMS];
static int hudItemsUsed;

typedef enum {
	hudPlayTime,
	hudEditName,
	hudViewName,

	hudCamCheckPos,
	hudCamCheckAngles,
	hudCamCheckFov,
	hudCamLength,
	hudCamSpeed,
	hudCamSmoothPos,
	hudCamSmoothAngles,
	hudCamPosX, hudCamPosY, hudCamPosZ,
	hudCamPitch, hudCamYaw, hudCamRoll,
	hudCamFov,

	hudEffectCheckPos,
	hudEffectCheckAngles,
	hudEffectCheckSize,
	hudEffectCheckColor,
	hudEffectCheckActive,
	hudEffectScript,
	hudEffectShader,
	hudEffectModel,
	hudEffectSize,
	hudEffectColor,
	hudEffectPosX, hudEffectPosY, hudEffectPosZ,
	hudEffectPitch, hudEffectYaw, hudEffectRoll,

	hudChaseDistance,
	hudChaseTarget,
	hudChasePosX, hudChasePosY, hudChasePosZ,
	hudChasePitch, hudChaseYaw, hudChaseRoll,
	hudChaseFov,

	hudLineTime, 
	hudLineOffset,
	hudLineSpeed,
	hudLineStart,
	hudLineEnd,
	
	hudDofFocus,
	hudDofRadius,
	hudDofTarget,

	hudScriptInit,
	hudScriptRun,

	hudLogBase
} hudHandler_t;


#define MASK_CAM				0x00010
#define MASK_SCRIPT				0x00100
#define MASK_EFFECT				0x00200
#define MASK_CAPTURE			0x00020
#define MASK_LINE				0x00040
#define MASK_CHASE				0x00080
#define MASK_DOF				0x00400

#define MASK_HUD				0x1
#define MASK_POINT				0x2
#define MASK_EDIT				0x4
#define MASK_ACTIVE				0x8

#define MASK_CAM_POINT			( MASK_CAM | MASK_POINT )
#define MASK_CAM_HUD			( MASK_CAM | MASK_HUD )
#define MASK_CAM_EDIT			( MASK_CAM | MASK_EDIT )
#define MASK_CAM_EDITHUD		( MASK_CAM | MASK_EDIT | MASK_HUD )

#define MASK_CHASE_EDIT			( MASK_CHASE | MASK_EDIT )

#define MASK_LINE_HUD			( MASK_LINE | MASK_HUD )

#define MASK_CHASE_POINT		( MASK_CHASE | MASK_POINT )

#define MASK_DOF_EDIT			( MASK_DOF | MASK_EDIT )

#define MASK_EFFECT_HUD			( MASK_EFFECT | MASK_HUD )
#define MASK_EFFECT_EDIT		( MASK_EFFECT | MASK_EDIT )
#define MASK_EFFECT_EDITHUD		( MASK_EFFECT | MASK_EDIT | MASK_HUD )
#define MASK_EFFECT_ACTIVEHUD	( MASK_EFFECT | MASK_ACTIVE | MASK_HUD )

#define MASK_SCRIPT_POINT		( MASK_SCRIPT | MASK_POINT )

static struct {
	int cursorX, cursorY;
	int showMask;
	int keyCatcher;
	struct {
		int  cursor;
		hudItem_t *item;
		char line[512];
	} edit;
	struct {
		float *origin, *angles, *fov;
		int *flags;
		demoCameraPoint_t *point;
	} cam;
	struct {
		float *origin, *angles, *distance;
		int *flags;
		demoChasePoint_t *point;
	} chase;
	struct {
		float *focus, *radius;
		demoDofPoint_t *point;
	} dof;
	demoScriptPoint_t *scriptPoint;
	demoLinePoint_t *linePoint;
	const char	*logLines[LOGLINES];
} hud;

extern void trap_SCR_DrawString( int x, int y, float size, const char *string, const float *setColor, qboolean forceColor );
static void hudDrawText( float x, float y, const char *buf, const vec4_t color) {
	if (!buf)
		return;
	//entFIXME: adjust positions?
	trap_SCR_DrawString(x, y, 16.0f, buf, color, qtrue);
//	trap_Rocket_AddTextElement(buf, buf, x, y);
//	CG_DrawStringExt( x, y, buf, color, qtrue, qtrue, HUD_TEXT_WIDTH/cgs.aspectScale, HUD_TEXT_HEIGHT , -1 );
//	CG_Text_Paint( x, y, 0.3f, color, buf, 0,0,ITEM_TEXTSTYLE_SHADOWED );
}

static void hudMakeTarget( int targetNum, char *dst, int dstSize) {
	centity_t *cent = demoTargetEntity( targetNum );
	if (cent) {
		const char *type;
		switch (cent->currentState.eType) {
		case ET_PLAYER:
			type = "player";
			break;
		case ET_MISSILE:
			type = "missile";
			break;
		case ET_ITEM:
			type = "item";
			break;
		case ET_MOVER:
			type = "mover";
			break;
		default:
			type = "other";
			break;
		}
		Com_sprintf(dst, dstSize, "%d %s", targetNum, type );
	} else {
		Com_sprintf(dst, dstSize, "%d invalid", targetNum );
	}
}

const char *demoTimeString( int time ) {
	static char retBuf[32];
	int msec = time % 1000;
	int secs = (time / 1000);
	int mins = (secs / 60);
	secs %= 60;
	Com_sprintf( retBuf, sizeof(retBuf), "%d:%02d.%03d", mins, secs, msec  );
	return retBuf;
}

static int hudGetChecked( hudItem_t *item, vec4_t color ) {
	Vector4Copy( colorWhite, color );
	switch ( item->handler ) {
	case hudCamCheckPos:
		return (hud.cam.flags[0] & CAM_ORIGIN);
	case hudCamCheckAngles:
		return (hud.cam.flags[0] & CAM_ANGLES);
	case hudCamCheckFov:
		return (hud.cam.flags[0] & CAM_FOV);
	}
	return 0;
}

static void hudToggleButton( hudItem_t *item, int change ) {
	int sA = demo.camera.smoothAngles, sP = demo.camera.smoothPos;
	if (!change)
		return;
	switch ( item->handler ) {
	case hudCamSmoothAngles:
		if ( change < 0 ) {
			if ( sA > 0)
				sA--;
			else
				sA = angleLast - 1;
		} else {
			if ( sA < angleLast - 1)
				sA++;
			else
				sA = 0;
		}
		demo.camera.smoothAngles = (angleInterpolate_t)sA;
		return;
	case hudCamSmoothPos:
		if ( change < 0 ) {
			if ( sP > 0)
				sP--;
			else
				sP = posLast - 1;
		} else {
			if ( sP < posLast - 1)
				sP++;
			else
				sP = 0;
		}
		demo.camera.smoothPos = (posInterpolate_t)sP;
		return;
	}
}

static void hudToggleChecked( hudItem_t *item ) {
	switch ( item->handler ) {
	case hudCamCheckPos:
		hud.cam.flags[0] ^= CAM_ORIGIN;
		break;
	case hudCamCheckAngles:
		hud.cam.flags[0] ^= CAM_ANGLES;
		break;
	case hudCamCheckFov:
		hud.cam.flags[0] ^= CAM_FOV;
		break;
	}
}

static float *hudGetFloat( hudItem_t *item ) {
	switch ( item->handler ) {
	case hudCamFov:
		return hud.cam.fov;
	case hudCamPosX:
	case hudCamPosY:
	case hudCamPosZ:
		return hud.cam.origin + item->handler - hudCamPosX;
	case hudCamPitch:
	case hudCamYaw:
	case hudCamRoll:
		return hud.cam.angles + item->handler - hudCamPitch;
	case hudChasePosX:
	case hudChasePosY:
	case hudChasePosZ:
		return hud.chase.origin + item->handler - hudChasePosX;
	case hudChasePitch:
	case hudChaseYaw:
	case hudChaseRoll:
		return hud.chase.angles + item->handler - hudChasePitch;
	case hudChaseDistance:
		return hud.chase.distance;
	case hudDofFocus:
		return hud.dof.focus;
	case hudDofRadius:
		return hud.dof.radius;
	default:
		break;
	}
	return 0;
}

static void hudGetHandler( hudItem_t *item, char *buf, int bufSize ) {
	int effectCount;

	buf[0] = 0;
	if (item->handler >= hudLogBase && item->handler < hudLogBase + LOGLINES) {
		const char *l = hud.logLines[item->handler - hudLogBase];
		if (!l)
			return;
		Q_strncpyz( buf, l, bufSize );
		return;
    }
	switch ( item->handler ) {
	case hudPlayTime:
		Com_sprintf( buf, bufSize, "%s", demoTimeString(demo.play.time ));
		return;
	case hudEditName:
		switch (demo.editType) {
		case editCamera:
			Com_sprintf( buf, bufSize, "Camera%s", demo.camera.locked ? " locked" : "" );
			break;
		case editChase:
			Com_sprintf( buf, bufSize, "Chase%s", demo.chase.locked ? " locked" : "" );
			break;
		case editLine:
			Com_sprintf( buf, bufSize, "Line%s", demo.line.locked ? " locked" : "" );
			break;
		case editDof:
			Com_sprintf( buf, bufSize, "Dof%s", demo.dof.locked ? " locked" : "" );
			break;
		case editScript:
			Com_sprintf( buf, bufSize, "Script%s", demo.script.locked ? " locked" : "" );
			break;
		}
		return;
	case hudViewName:
		switch (demo.viewType) {
		case viewCamera:
			Com_sprintf( buf, bufSize, "Camera");
			break;
		case viewChase:
			Com_sprintf( buf, bufSize, "Chase");
			break;
		default:
			return;
		}
		return;
	case hudCamLength:
		Com_sprintf( buf, bufSize, "%.01f", hud.cam.point->len );
		return;
	case hudCamSpeed:
		if (!hud.cam.point->prev)
			Q_strncpyz( buf, "First, ", bufSize );
		else
			Com_sprintf( buf, bufSize, "%.01f, ", 1000 * hud.cam.point->prev->len / (hud.cam.point->time - hud.cam.point->prev->time));
		bufSize -= strlen( buf );
		buf += strlen( buf );
		if (!hud.cam.point->next)
			Q_strncpyz( buf, "Last", bufSize );
		else 
			Com_sprintf( buf, bufSize, "%.01f", 1000 * hud.cam.point->len / (hud.cam.point->next->time - hud.cam.point->time));
		return;
	case hudCamSmoothPos:
		switch ( demo.camera.smoothPos ) {
		case posLinear:
			Q_strncpyz( buf, "Linear", bufSize );
			break;
		case posCatmullRom:
			Q_strncpyz( buf, "CatmullRom", bufSize );
			break;
		default:
		case posBezier:
			Q_strncpyz( buf, "Bezier", bufSize );
			break;

		}
		return;
	case hudCamSmoothAngles:
		switch ( demo.camera.smoothAngles ) {
		case angleLinear:
			Q_strncpyz( buf, "Linear", bufSize );
			break;
		default:
		case angleQuat:
			Q_strncpyz( buf, "Quat", bufSize );
			break;
		}
		return;
	case hudDofTarget:
		hudMakeTarget( demo.dof.target, buf, bufSize );
		return;
	case hudChaseTarget:
		hudMakeTarget( demo.chase.target, buf, bufSize );
		return;
	case hudLineTime:
		Com_sprintf( buf, bufSize, "%s", demoTimeString( demo.line.time ));
		return;
	case hudLineOffset:
		Com_sprintf( buf, bufSize, "%d.%03d", demo.line.offset / 1000, demo.line.offset % 1000 );
		return;
	case hudLineSpeed:
		if (demo.line.locked && hud.linePoint) {
			demoLinePoint_t *point = hud.linePoint;
			if (!point->prev) {
				Com_sprintf( buf, bufSize, "First" );
			} else {
				Com_sprintf( buf, bufSize, HUD_FLOAT, (point->demoTime - point->prev->demoTime) / (float)(point->time - point->prev->time));
			}
		} else {
			Com_sprintf( buf, bufSize, HUD_FLOAT, demo.line.speed );
		}
		return;
//	case HUD_LINE_SPEED_NEXT:
//		if (demo.line.locked && hud.linePoint) {
//			demoLinePoint_t *point = hud.linePoint;
//			if (!point->next)
//				return "Last";
//			Com_sprintf( buf, bufSize, "%.03f", (point->next->demoTime - point->demoTime) / (float)(point->next->time - point->time));
//			return;
//		}
//		return "";
	case hudLineStart:
		if ( demo.capture.start > 0 )
			Com_sprintf( buf, bufSize, "%s", demoTimeString( demo.capture.start ));
		else
			Com_sprintf( buf, bufSize, "None" );
		return;
	case hudLineEnd:
		if (demo.capture.end > 0 )
			Com_sprintf( buf, bufSize, "%s", demoTimeString( demo.capture.end ));
		else 
			Com_sprintf( buf, bufSize, "None" );
		return;
	}
}

static void hudGetText( hudItem_t *item, char *buf, int bufSize, qboolean edit ) {
	int i;
	unsigned char c[4];

	buf[0] = 0;
	switch ( item->handler ) {
	case hudScriptInit:
		if (!hud.scriptPoint )
			break;
		Com_sprintf( buf, bufSize, "%s", hud.scriptPoint->init );
		break;
	case hudScriptRun:
		if (!hud.scriptPoint )
			break;
		Com_sprintf( buf, bufSize, "%s", hud.scriptPoint->run );
		break;
	}
}




static void hudSetText( hudItem_t *item, const char *buf ) {
	int i, val;
	
	switch ( item->handler ) {
	case hudScriptInit:
		if (!hud.scriptPoint )
			break;
		Q_strncpyz( hud.scriptPoint->init, buf, sizeof( hud.scriptPoint->init ) );
		break;
	case hudScriptRun:
		if (!hud.scriptPoint )
			break;
		Q_strncpyz( hud.scriptPoint->run, buf, sizeof( hud.scriptPoint->run ) );
		break;
	}
}

static float hudItemWidth( hudItem_t *item  ) {
	char buf[512];
	float w, *f;

	w = item->textLen * HUD_TEXT_WIDTH/cgs.aspectScale;
	switch (item->type ) {
	case hudTypeHandler:
	case hudTypeButton:
		hudGetHandler( item, buf, sizeof(buf) );
		w += strlen( buf ) * HUD_TEXT_WIDTH/cgs.aspectScale;
		break;
	case hudTypeText:
		hudGetText( item, buf, sizeof(buf), qfalse );
		w += strlen( buf ) * HUD_TEXT_WIDTH/cgs.aspectScale;
		break;
	case hudTypeValue:
		Com_sprintf( buf, sizeof( buf ), HUD_FLOAT, item->value[0]);
		w += strlen( buf ) * HUD_TEXT_WIDTH/cgs.aspectScale;
		break;
	case hudTypeFloat:
		f = hudGetFloat( item );
		if (!f)
			break;
		Com_sprintf( buf, sizeof( buf ), HUD_FLOAT, f[0] );
		w += strlen( buf ) * HUD_TEXT_WIDTH/cgs.aspectScale;
		break;
	case hudTypeCheck:
		w += HUD_TEXT_WIDTH/cgs.aspectScale;
		break;
	case hudTypeCvar:
		trap_Cvar_VariableStringBuffer( item->cvar, buf, sizeof( buf ));
		w += strlen( buf ) * HUD_TEXT_WIDTH/cgs.aspectScale;
		break;
	}
	return w;
}

static void hudDrawItem( hudItem_t *item ) {
	char buf[512];
	int checked;
	float x,y, *f;

	x = item->x;
	y = item->y;

	if ( item == hud.edit.item ) {
		if ( item->textLen ) {
			hudDrawText( x, y, item->text, colorRed );
			x += item->textLen * HUD_TEXT_WIDTH/cgs.aspectScale;
		}
		hudDrawText( x, y, hud.edit.line, colorRed );
		if ( demo.serverTime & 512 ) {
			float x = item->x + (item->textLen + hud.edit.cursor) * HUD_TEXT_WIDTH/cgs.aspectScale;
			float y = item->y;
			if ( trap_Key_GetOverstrikeMode()) {
				CG_FillRect( x, y + HUD_TEXT_SPACING - 3 , HUD_TEXT_WIDTH/cgs.aspectScale, 3, colorRed );
			} else {
				CG_FillRect( x, y , HUD_TEXT_WIDTH/cgs.aspectScale, HUD_TEXT_SPACING, colorRed );
			}
		}
		return;
	}
	if ( item->textLen ) {
		float *color = colorWhite;
		if ( hud.keyCatcher & KEYCATCH_CGAME ) {
			switch ( item->type ) {
			case hudTypeFloat:
			case hudTypeValue:
			case hudTypeCvar:
			case hudTypeText:
				color = colorYellow;
				break;
			case hudTypeCheck:
			case hudTypeButton:
				color = colorGreen;
				break;
			}
		} 
		hudDrawText( item->x, item->y, item->text, color );
		x += item->textLen * HUD_TEXT_WIDTH/cgs.aspectScale;
	}
	switch (item->type ) {
	case hudTypeButton:
	case hudTypeHandler:
		hudGetHandler( item, buf, sizeof( buf ));
		hudDrawText( x, y, buf, colorWhite );
		break;
	case hudTypeText:
		hudGetText( item, buf, sizeof(buf), qfalse );
		hudDrawText( x, y, buf, colorWhite );
		break;
	case hudTypeValue:
		Com_sprintf( buf, sizeof( buf ), HUD_FLOAT, item->value[0] );
		hudDrawText( x, y, buf, colorWhite );
		break;
	case hudTypeFloat:
		f = hudGetFloat( item );
		if (!f)
			break;
		Com_sprintf( buf, sizeof( buf ), HUD_FLOAT, f[0] );
		hudDrawText( x, y, buf, colorWhite );
		break;
	case hudTypeCvar:
		trap_Cvar_VariableStringBuffer( item->cvar, buf, sizeof( buf ));
		hudDrawText( x, y, buf, colorWhite );
		break;
	case hudTypeCheck:
		checked = hudGetChecked( item, colorWhite );
		CG_DrawPic( x, y, HUD_TEXT_SPACING*cgs.aspectScale, HUD_TEXT_SPACING, checked ? 
			demo.media.switchOn : demo.media.switchOff ); 
		break;
	}
}

void hudDraw( void ) {
	int i;int logIndex;
	hudItem_t *item;

	if (demo.editType == editNone)
		return;

	if ( mov_hudOverlay.string[0] ) {
		qhandle_t shader = trap_R_RegisterShader( mov_hudOverlay.string, (RegisterShaderFlags_t)( RSF_DEFAULT | RSF_NOMIP ) );
		if ( shader ) {
			trap_R_DrawStretchPic( 0, 0, cgs.glconfig.vidWidth, cgs.glconfig.vidHeight, 0,0,1,1, shader );
		}
	}

	switch (demo.editType) {
	case editCamera:
		hud.showMask = MASK_CAM;
		if ( demo.camera.locked ) {
			hud.cam.point = cameraPointSynch(  demo.play.time );
			if (!hud.cam.point || hud.cam.point->time != demo.play.time || demo.play.fraction) {
				hud.cam.point = 0;
			} else {
				hud.cam.angles = hud.cam.point->angles;
				hud.cam.origin = hud.cam.point->origin;
				hud.cam.fov = &hud.cam.point->fov;
				hud.cam.flags = &hud.cam.point->flags;
				hud.showMask |= MASK_EDIT | MASK_POINT;
			}
		} else {
			hud.cam.angles = demo.camera.angles;
			hud.cam.origin = demo.camera.origin;
			hud.cam.fov = &demo.camera.fov;
			hud.cam.flags = &demo.camera.flags;
			hud.cam.point = 0;
			hud.showMask |= MASK_EDIT;
		}
		break;
	case editLine:
		hud.showMask = MASK_LINE;
		break;
	case editScript:
		hud.showMask = MASK_SCRIPT;
		hud.scriptPoint = scriptPointSynch( demo.play.time );
		if ( !hud.scriptPoint || hud.scriptPoint->time != demo.play.time || demo.play.fraction ) {
			hud.scriptPoint = 0;
		} else {
			hud.showMask |= MASK_POINT;
		}
		break;
	case editChase:
		hud.showMask = MASK_CHASE;
		if ( demo.chase.locked ) {
			hud.chase.point = chasePointSynch(  demo.play.time );
			if (!hud.chase.point || hud.chase.point->time != demo.play.time || demo.play.fraction) {
				hud.chase.point = 0;
			} else {
				hud.showMask |= MASK_EDIT | MASK_POINT;
				hud.chase.angles = hud.chase.point->angles;
				hud.chase.origin = hud.chase.point->origin;
				hud.chase.distance = &hud.chase.point->distance;
			}
		} else {
			hud.chase.angles = demo.chase.angles;
			hud.chase.origin = demo.chase.origin;
			hud.chase.distance = &demo.chase.distance;
			hud.chase.point = 0;
			hud.showMask |= MASK_EDIT;
		}
		break;
	case editDof:
		hud.showMask = MASK_DOF;
		if ( demo.dof.locked ) {
			hud.dof.point = dofPointSynch(  demo.play.time );
			if (!hud.dof.point || hud.dof.point->time != demo.play.time || demo.play.fraction) {
				hud.dof.point = 0;
			} else {
				hud.dof.focus = &hud.dof.point->focus;
				hud.dof.radius = &hud.dof.point->radius;
				hud.showMask |= MASK_EDIT | MASK_POINT;
			}
		} else {
			hud.dof.focus = &demo.dof.focus;
			hud.dof.radius = &demo.dof.radius;
			hud.dof.point = 0;
			hud.showMask |= MASK_EDIT;
		}
		break;
	default:
		hud.showMask = 0;
		break;
	}	
	hud.keyCatcher = trap_Key_GetCatcher();
	if ( hud.keyCatcher & KEYCATCH_CGAME ) {
		hud.showMask |= MASK_HUD;
	}
	/* Prepare the camera control information */

	hud.linePoint = linePointSynch( demo.play.time );
	logIndex = LOGLINES - 1;
	for (i=0; i < LOGLINES;	i++) {
		int which = (demo.log.lastline + LOGLINES - i) % LOGLINES;
		hud.logLines[LOGLINES - 1 - i] = 0;
		if (demo.log.times[which] + 3000 > demo.serverTime)
			hud.logLines[logIndex--] = demo.log.lines[which];
	}
	// Check if the selected edit item is still valid
	if ( hud.edit.item ) {
		if ( !(hud.keyCatcher & KEYCATCH_CGAME ))
			hud.edit.item = 0;
		else if ( (hud.edit.item->showMask & hud.showMask) != hud.edit.item->showMask )
			hud.edit.item = 0;
	}
	for (i=0; i < hudItemsUsed; i++) {
		item = &hudItems[i];
		if ((hud.showMask & item->showMask) != item->showMask )
			continue;
		hudDrawItem( item );
	}

	if ( hud.keyCatcher & KEYCATCH_CGAME ) {
		float x,y,w,h;
		x = hud.cursorX - 16*cgs.aspectScale;
		y = hud.cursorY - 16;
		w = 32;
		h = 32;
		CG_AdjustFrom640( &x, &y, &w, &h );
		trap_R_DrawStretchPic( x,y,w*cgs.aspectScale,h, 0,0,1,1, demo.media.mouseCursor );
	} else {
		hud.edit.item = 0;
	}
}

static hudItem_t *hudAddItem( float x, float y, int showMask, const char *text ) {
	hudItem_t *item;
	if (hudItemsUsed >= MAX_HUD_ITEMS ) 
		CG_Error( "Demo too many hud items" );
	item = &hudItems[hudItemsUsed];
	item->x = x * HUD_TEXT_WIDTH/cgs.aspectScale;
	item->y = y * HUD_TEXT_SPACING + 50;
	item->type = hudTypeNone;
	item->showMask = showMask;
	item->handler = 0;
	item->text = text;
	if (item->text )
		item->textLen = strlen( text );
	else
		item->textLen = 0;
	hudItemsUsed++;
	return item;
}

static void hudAddHandler( float x, float y, int showMask, const char *text, int handler ) {
	hudItem_t *item = hudAddItem( x, y, showMask, text );
	item->handler = handler;
	item->type = hudTypeHandler;
}

static void hudAddValue( float x, float y, int showMask, const char *text, float *value) {
	hudItem_t *item = hudAddItem( x, y, showMask, text );
	item->value = value;
	item->type = hudTypeValue;
}
static void hudAddFloat( float x, float y, int showMask, const char *text, int handler ) {
	hudItem_t *item = hudAddItem( x, y, showMask, text );
	item->handler = handler;
	item->type = hudTypeFloat;
}

static void hudAddCheck( float x, float y, int showMask, const char *text, int handler ) {
	hudItem_t *item = hudAddItem( x, y, showMask, text );
	item->handler = handler;
	item->type = hudTypeCheck;
}
static void hudAddCvar( float x, float y, int showMask, const char *text, const char *cvar ) {
	hudItem_t *item = hudAddItem( x, y, showMask, text );
	item->cvar = cvar;
	item->type = hudTypeCvar;
}
static void hudAddButton( float x, float y, int showMask, const char *text, int handler ) {
	hudItem_t *item = hudAddItem( x, y, showMask, text );
	item->handler = handler;
	item->type = hudTypeButton;
}
static void hudAddText( float x, float y, int showMask, const char *text, int handler ) {
	hudItem_t *item = hudAddItem( x, y, showMask, text );
	item->handler = handler;
	item->type = hudTypeText;
}

void hudToggleInput(void) {
	int oldCatcher = trap_Key_GetCatcher();
	if ( oldCatcher & KEYCATCH_CGAME ) {
		oldCatcher &= ~(KEYCATCH_CGAME | KEYCATCH_CGAMEEXEC );
	} else {
		oldCatcher |= (KEYCATCH_CGAME | KEYCATCH_CGAMEEXEC );
	}
	trap_Key_SetCatcher( oldCatcher );
}

void hudInitTables(void) {
	int i;
	memset( hudItems, 0, sizeof( hudItems ));
	hudItemsUsed = 0;

	/* Setup the hudItems */
	hudAddHandler(   0,  0,  0, "Time:", hudPlayTime );
	hudAddValue(     0,  1,  0, "Speed:", &demo.play.speed );
	hudAddHandler(   0,  2,  0, "View:", hudViewName );
	hudAddHandler(   0,  3,  0, "Edit:", hudEditName );

	for (i = 0; i < LOGLINES; i++) 
		hudAddHandler(   0,  25+i, 0, 0, hudLogBase+i );

	// Camera Items
	hudAddFloat(   0,  4, MASK_CAM_EDIT, "PosX:",  hudCamPosX );
	hudAddFloat(   0,  5, MASK_CAM_EDIT, "PosY:",  hudCamPosY );
	hudAddFloat(   0,  6, MASK_CAM_EDIT, "PosZ:",  hudCamPosZ );
	hudAddFloat(   0,  7, MASK_CAM_EDIT, "Pitc:",  hudCamPitch );
	hudAddFloat(   0,  8, MASK_CAM_EDIT, "Yaw :",  hudCamYaw );
	hudAddFloat(   0,  9, MASK_CAM_EDIT, "Roll:",  hudCamRoll );
	hudAddFloat(   0, 10, MASK_CAM_EDIT, "Fov :",  hudCamFov );
	hudAddHandler( 0, 11, MASK_CAM_POINT, "Length:", hudCamLength );
	hudAddHandler( 0, 12, MASK_CAM_POINT, "Speed:", hudCamSpeed );
	hudAddCheck(  0,  14, MASK_CAM_EDITHUD, "Pos", hudCamCheckPos );
	hudAddCheck(  5,  14, MASK_CAM_EDITHUD, "Ang", hudCamCheckAngles );
	hudAddCheck(  10, 14, MASK_CAM_EDITHUD, "Fov", hudCamCheckFov );
	hudAddButton(  0, 15, MASK_CAM_HUD, "SmoothPos:", hudCamSmoothPos );
	hudAddButton(  0, 16, MASK_CAM_HUD, "SmoothAngles:", hudCamSmoothAngles );

	// Chase items
	hudAddFloat(   0,  4, MASK_CHASE_EDIT, "PosX:",  hudChasePosX );
	hudAddFloat(   0,  5, MASK_CHASE_EDIT, "PosY:",  hudChasePosY );
	hudAddFloat(   0,  6, MASK_CHASE_EDIT, "PosZ:",  hudChasePosZ );
	hudAddFloat(   0,  7, MASK_CHASE_EDIT, "Pitc:",  hudChasePitch);
	hudAddFloat(   0,  8, MASK_CHASE_EDIT, "Yaw :",  hudChaseYaw );
	hudAddFloat(   0,  9, MASK_CHASE_EDIT, "Roll:",  hudChaseRoll );
	hudAddFloat(   0, 10, MASK_CHASE_EDIT, "Distance:", hudChaseDistance );
	hudAddHandler( 0, 11, MASK_CHASE, "Target:", hudChaseTarget );

	// Chase items
	hudAddFloat(   0,  4, MASK_EFFECT_EDIT, "PosX:",  hudEffectPosX );
	hudAddFloat(   0,  5, MASK_EFFECT_EDIT, "PosY:",  hudEffectPosY );
	hudAddFloat(   0,  6, MASK_EFFECT_EDIT, "PosZ:",  hudEffectPosZ );
	hudAddFloat(   0,  7, MASK_EFFECT_EDIT, "Pitc:",  hudEffectPitch);
	hudAddFloat(   0,  8, MASK_EFFECT_EDIT, "Yaw :",  hudEffectYaw );
	hudAddFloat(   0,  9, MASK_EFFECT_EDIT, "Roll:",  hudEffectRoll );
	hudAddFloat(   0, 10, MASK_EFFECT_EDIT, "Size:",  hudEffectSize );
	hudAddText(   0,  11, MASK_EFFECT_EDIT, "Color:", hudEffectColor );

	hudAddCheck(  0,  12, MASK_EFFECT_EDITHUD, "Pos", hudEffectCheckPos );
	hudAddCheck(  5,  12, MASK_EFFECT_EDITHUD, "Ang", hudEffectCheckAngles );
	hudAddCheck(  10, 12, MASK_EFFECT_EDITHUD, "Size", hudEffectCheckSize );

	hudAddCheck(   0, 13, MASK_EFFECT_ACTIVEHUD, "Active", hudEffectCheckActive );
	hudAddText(    0, 14, MASK_EFFECT_ACTIVEHUD, "Script:", hudEffectScript );
	hudAddText(    0, 15, MASK_EFFECT_ACTIVEHUD, "Shader:", hudEffectShader );
	hudAddText(    0, 16, MASK_EFFECT_ACTIVEHUD, "Model:", hudEffectModel );

	//Line offset items
	hudAddHandler(   0,   4, MASK_LINE, "Time:", hudLineTime );
	hudAddHandler(   0,   5, MASK_LINE, "Offset:", hudLineOffset );
	hudAddHandler(   0,   6, MASK_LINE, "Speed:", hudLineSpeed );

	hudAddText( 0, 4 , MASK_SCRIPT_POINT, "Init:", hudScriptInit );
	hudAddText( 0, 5 , MASK_SCRIPT_POINT, "Run:", hudScriptRun );

	// Capture items
	hudAddHandler(   0,   8, MASK_LINE, "Start:", hudLineStart );
	hudAddHandler(   0,   9, MASK_LINE, "End:", hudLineEnd );
	hudAddCvar(   0,  10, MASK_LINE_HUD, "Fps:", "mov_captureFPS" );
	hudAddCvar(   0,  11, MASK_LINE_HUD, "BlurFrames:", "mme_blurFrames" );
	hudAddCvar(   0,  12, MASK_LINE_HUD, "BlurOverlap:", "mme_blurOverlap" );
	hudAddCvar(   0,  13, MASK_LINE_HUD, "saveDepth:", "mme_saveDepth" );
	hudAddCvar(   0,  14, MASK_LINE_HUD, "depthFocus:", "mme_depthFocus" );
	hudAddCvar(   0,  15, MASK_LINE_HUD, "depthRange:", "mme_depthRange" );
	hudAddCvar(   0,  16, MASK_LINE_HUD, "saveStencil:", "mme_saveStencil" );
	hudAddCvar(   0,  17, MASK_LINE_HUD, "MusicFile:", "mov_musicFile" );
	hudAddCvar(   0,  18, MASK_LINE_HUD, "MusicStart:", "mov_musicStart" );
	
	// Depth of field Items
	hudAddFloat(   0,  4, MASK_DOF_EDIT, "Focus:",  hudDofFocus );
	hudAddFloat(   0,  5, MASK_DOF_EDIT, "Radius:",  hudDofRadius );
	hudAddHandler( 0,  6, MASK_DOF_EDIT, "Target:", hudDofTarget );

}

static hudItem_t *hudItemAt( float x, float y ) {
	int i;
	for ( i = 0; i< hudItemsUsed; i++ ) {
		float w, h;
		hudItem_t *item = hudItems + i;
		if ((hud.showMask & item->showMask) != item->showMask )
			continue;
		if ( item->x > x || item->y > y )
			continue;
		w = x - item->x;
		h = y - item->y;
		if ( h > HUD_TEXT_HEIGHT )
			continue;
		if ( w > hudItemWidth( item ))
			continue;
		return item;
	}
	return 0;
}


static void hudEditItem( hudItem_t *item, const char *buf ) {
	float *f;
	switch ( item->type ) {
	case hudTypeFloat:
		f = hudGetFloat( item );
		if (!f)
			return;
		*f = atof( buf );
		break;
	case hudTypeValue:
		item->value[0] = atof( buf );
		break;
	case hudTypeCvar:
		trap_Cvar_Set( item->cvar, buf );
		break;
	case hudTypeText:
		hudSetText( item, buf );
		break;
	}
}

qboolean CG_KeyEvent(int key, qboolean down) {
	int catchMask;
	int len;

	if (!down)
		return qfalse;

	catchMask = trap_Key_GetCatcher();

	len = strlen( hud.edit.line );
	if ( key == K_MOUSE1 ) {
		hudItem_t *item;

		item = hudItemAt( hud.cursorX, hud.cursorY );
		hud.edit.item = 0;
		if ( item ) {
			float *f;
			switch ( item->type ) {
			case hudTypeValue:
				Com_sprintf( hud.edit.line, sizeof( hud.edit.line ), HUD_FLOAT, item->value[0] );
				hud.edit.cursor = strlen( hud.edit.line );
				hud.edit.item = item;
				trap_Key_SetCatcher( KEYCATCH_CGAME | (catchMask &~KEYCATCH_CGAMEEXEC));
				break;
			case hudTypeCvar:
				trap_Cvar_VariableStringBuffer( item->cvar, hud.edit.line, sizeof( hud.edit.line ));
				hud.edit.cursor = strlen( hud.edit.line );
				hud.edit.item = item;
				trap_Key_SetCatcher( KEYCATCH_CGAME | (catchMask &~KEYCATCH_CGAMEEXEC));
				break;
			case hudTypeFloat:
				f = hudGetFloat( item );
				if (!f)
					break;
				Com_sprintf( hud.edit.line, sizeof( hud.edit.line ), HUD_FLOAT, *f );
				hud.edit.cursor = strlen( hud.edit.line );
				hud.edit.item = item;
				trap_Key_SetCatcher( KEYCATCH_CGAME | (catchMask &~KEYCATCH_CGAMEEXEC));
				break;
			case hudTypeText:
				hudGetText( item, hud.edit.line, sizeof( hud.edit.line ), qtrue );
				hud.edit.cursor = strlen( hud.edit.line );
				hud.edit.item = item;
				trap_Key_SetCatcher( KEYCATCH_CGAME | (catchMask &~KEYCATCH_CGAMEEXEC));
				break;
			case hudTypeButton:
				hudToggleButton( item, 1 );
				break;
			case hudTypeCheck:
				hudToggleChecked( item );
				break;
			}
		}
		return qtrue;
	//Further keypresses only handled when waiting for input
	} else if ( catchMask & KEYCATCH_CGAMEEXEC ) {
		return qfalse;
	} else if ( key == K_DEL || key == K_KP_DEL ) {
		if ( hud.edit.cursor < len ) {
			memmove( hud.edit.line + hud.edit.cursor, 
				hud.edit.line + hud.edit.cursor + 1, len - hud.edit.cursor );
		}
	} else if ( key == K_RIGHTARROW || key == K_KP_RIGHTARROW ) {
		if ( hud.edit.cursor < len ) {
			hud.edit.cursor++;
		}
	} else if ( key == K_LEFTARROW || key == K_KP_LEFTARROW ) {
		if ( hud.edit.cursor > 0 ) {
			hud.edit.cursor--;
		}
	} else if ( key == K_HOME || key == K_KP_HOME || ( tolower(key) == 'a' && trap_Key_IsDown( K_CTRL ) ) ) {
		hud.edit.cursor = 0;
	} else if ( key == K_END || key == K_KP_END || ( tolower(key) == 'e' && trap_Key_IsDown( K_CTRL ) ) ) {
		hud.edit.cursor = len;
	} else if ( key == K_TAB) {
		hud.edit.item = 0;
		trap_Key_SetCatcher( trap_Key_GetCatcher() & ~(KEYCATCH_CGAME|KEYCATCH_CGAMEEXEC) );
	} else if ( key == K_INS || key == K_KP_INS ) {
		trap_Key_SetOverstrikeMode( !trap_Key_GetOverstrikeMode() );
	} else if ( key == K_ENTER || key == K_KP_ENTER ) {
		hudEditItem( hud.edit.item, hud.edit.line );
		hud.edit.item = 0;
		trap_Key_SetCatcher( catchMask | KEYCATCH_CGAME  | KEYCATCH_CGAMEEXEC );
	}
	return qtrue;
}

void CG_MouseEvent(int dx, int dy) {
	// update mouse screen position
	hud.cursorX += dx;
	if (hud.cursorX < 0)
		hud.cursorX = 0;
	else if (hud.cursorX > SCREEN_WIDTH)
		hud.cursorX = SCREEN_WIDTH;

	hud.cursorY += dy;
	if (hud.cursorY < 0)
		hud.cursorY = 0;
	else if (hud.cursorY > SCREEN_HEIGHT)
		hud.cursorY = SCREEN_HEIGHT;
}