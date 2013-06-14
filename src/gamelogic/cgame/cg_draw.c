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

// cg_draw.c -- draw all of the graphical elements during
// active (after loading) gameplay

#include "cg_local.h"

void CG_MouseEvent( int x, int y )
{
	int n;

	if ( ( cg.predictedPlayerState.pm_type == PM_NORMAL ||
	       cg.predictedPlayerState.pm_type == PM_SPECTATOR ) &&
	     cg.showScores == qfalse )
	{
		trap_Key_SetCatcher( 0 );
		return;
	}

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
}

void CG_KeyEvent( int key, int chr, int flags )
{
	if ( !( flags & ( 1 << KEYEVSTATE_DOWN ) ) )
	{
		return;
	}

	if ( cg.predictedPlayerState.pm_type == PM_NORMAL ||
	     ( cg.predictedPlayerState.pm_type == PM_SPECTATOR &&
	       cg.showScores == qfalse ) )
	{
		trap_Key_SetCatcher( 0 );
		return;
	}
}

int CG_ClientNumFromName( const char *p )
{
	int i;

	for ( i = 0; i < cgs.maxclients; i++ )
	{
		if ( cgs.clientinfo[ i ].infoValid &&
		     Q_stricmp( cgs.clientinfo[ i ].name, p ) == 0 )
		{
			return i;
		}
	}

	return -1;
}

void CG_RunMenuScript( char **args )
{
}

//END TA UI

/*
================
CG_DrawLighting

================
*/
static void CG_DrawLighting( void )
{
	centity_t *cent;

	cent = &cg_entities[ cg.snap->ps.clientNum ];

	//fade to black if stamina is low
	if ( ( cg.snap->ps.stats[ STAT_STAMINA ] < STAMINA_BLACKOUT_LEVEL ) &&
	     ( cg.snap->ps.stats[ STAT_TEAM ] == TEAM_HUMANS ) )
	{
		vec4_t black = { 0, 0, 0, 0 };
		black[ 3 ] = 1.0 - ( ( float )( cg.snap->ps.stats[ STAT_STAMINA ] + STAMINA_MAX ) / ( STAMINA_MAX + STAMINA_BLACKOUT_LEVEL ) );
		trap_R_SetColor( black );
		CG_DrawPic( 0, 0, 640, 480, cgs.media.whiteShader );
		trap_R_SetColor( NULL );
	}
}

/*
===============================================================================

CENTER PRINTING

===============================================================================
*/

/*
==============
CG_CenterPrint

Called for important messages that should stay in the center of the screen
for a few moments
==============
*/
void CG_CenterPrint( const char *str, int y, int charWidth )
{
//TODO
}

/*
===================
CG_DrawCenterString
===================
*/
static void CG_DrawCenterString( void )
{
	char  *start;
	int   l;
	int   x, y, w;
	int   h;
	float *color;

	if ( !cg.centerPrintTime )
	{
		return;
	}

	color = CG_FadeColor( cg.centerPrintTime, 1000 * cg_centertime.value );

	if ( !color )
	{
		return;
	}

	trap_R_SetColor( color );

	start = cg.centerPrint;

	y = cg.centerPrintY - cg.centerPrintLines * BIGCHAR_HEIGHT / 2;

// TODO
}

//==============================================================================

//FIXME: both vote notes are hardcoded, change to ownerdrawn?

/*
=================
CG_DrawVote
=================
*/
static void CG_DrawVote( team_t team )
{
	char   *s;
	int    sec;
	int    offset = 0;
	vec4_t white = { 1.0f, 1.0f, 1.0f, 1.0f };
	char   yeskey[ 32 ] = "", nokey[ 32 ] = "";

	if ( !cgs.voteTime[ team ] )
	{
		return;
	}

	// play a talk beep whenever it is modified
	if ( cgs.voteModified[ team ] )
	{
		cgs.voteModified[ team ] = qfalse;
		trap_S_StartLocalSound( cgs.media.talkSound, CHAN_LOCAL_SOUND );
	}

	sec = ( VOTE_TIME - ( cg.time - cgs.voteTime[ team ] ) ) / 1000;

	if ( sec < 0 )
	{
		sec = 0;
	}

	if ( cg_tutorial.integer )
	{
		Com_sprintf( yeskey, sizeof( yeskey ), "[%s]",
		             CG_KeyBinding( va( "%svote yes", team == TEAM_NONE ? "" : "team" ), team ) );
		Com_sprintf( nokey, sizeof( nokey ), "[%s]",
		             CG_KeyBinding( va( "%svote no", team == TEAM_NONE ? "" : "team" ), team ) );
	}

	if ( team != TEAM_NONE )
	{
		offset = 80;
	}

	s = va( "%sVOTE(%i): %s",
	        team == TEAM_NONE ? "" : "TEAM", sec, cgs.voteString[ team ] );
	// TODO
}

/*
=================
CG_DrawIntermission
=================
*/
static void CG_DrawIntermission( void )
{
// TODO
}

/*
=================
CG_DrawQueue
=================
*/
static qboolean CG_DrawQueue( void )
{
	float  w;
	vec4_t color;
	int    position;
	char   buffer[ MAX_STRING_CHARS ];

	if ( !( cg.snap->ps.pm_flags & PMF_QUEUED ) )
	{
		return qfalse;
	}

	color[ 0 ] = 1;
	color[ 1 ] = 1;
	color[ 2 ] = 1;
	color[ 3 ] = 1;

	position = cg.snap->ps.persistant[ PERS_QUEUEPOS ] + 1;

	if ( position < 1 )
	{
		return qfalse;
	}

	if ( position == 1 )
	{
		Com_sprintf( buffer, MAX_STRING_CHARS, _("You are at the front of the spawn queue") );
	}
	else
	{
		Com_sprintf( buffer, MAX_STRING_CHARS, _("You are at position %d in the spawn queue"), position );
	}

// TODO

	if ( cg.snap->ps.persistant[ PERS_SPAWNS ] == 0 )
	{
		Com_sprintf( buffer, MAX_STRING_CHARS, _("There are no spawns remaining") );
	}
	else
	{
		Com_sprintf( buffer, MAX_STRING_CHARS,
		             P_( "There is 1 spawn remaining", "There are %d spawns remaining",
		                cg.snap->ps.persistant[ PERS_SPAWNS ]),
		             cg.snap->ps.persistant[ PERS_SPAWNS ] );
	}

// TODO

	return qtrue;
}

/*
=================
CG_DrawWarmup
=================
*/
static void CG_DrawWarmup( void )
{
	int   sec = 0;
	int   w;
	int   h;
	float size = 0.5f;
	char  text[ MAX_STRING_CHARS ];

	if ( !cg.warmupTime )
	{
		return;
	}

	sec = ( cg.warmupTime - cg.time ) / 1000;

	if ( sec < 0 )
	{
		return;
	}

	strncpy( text, _( "Warmup Time:" ), sizeof( text ) );
//TODO

	Com_sprintf( text, sizeof( text ), "%s", sec ? va( "%d", sec ) : _("FIGHT!") );

//TODO
}

//==================================================================================

/*
=================
CG_Draw2D
=================
*/
static void CG_Draw2D( void )
{

	// fading to black if stamina runs out
	// (only 2D that can't be disabled)
	CG_DrawLighting();

	if ( cg_draw2D.integer == 0 )
	{
		return;
	}

	if ( cg.snap->ps.pm_type == PM_INTERMISSION )
	{
		CG_DrawVote( TEAM_NONE );
		CG_DrawVote( cg.predictedPlayerState.stats[ STAT_TEAM ] );
		CG_DrawIntermission();
		return;
	}

	if ( cg.snap->ps.persistant[ PERS_SPECSTATE ] == SPECTATOR_NOT &&
	     cg.snap->ps.stats[ STAT_HEALTH ] > 0 && !cg.zoomed )
	{
		CG_DrawBuildableStatus();
	}

	if ( cg.zoomed )
	{
		vec4_t black = { 0.0f, 0.0f, 0.0f, 0.5f };
		trap_R_DrawStretchPic( ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), 0, cgs.glconfig.vidHeight, cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.scopeShader );
		trap_R_SetColor( black );
		trap_R_DrawStretchPic( 0, 0, ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.whiteShader );
		trap_R_DrawStretchPic( cgs.glconfig.vidWidth - ( ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ) ), 0, ( cgs.glconfig.vidWidth / 2 ) - ( cgs.glconfig.vidHeight / 2 ), cgs.glconfig.vidHeight, 0, 0, 1, 1, cgs.media.whiteShader );
		trap_R_SetColor( NULL );
	}
	else
	{
		//TODO: Draw HUD
	}

	CG_DrawVote( TEAM_NONE );
	CG_DrawVote( cg.predictedPlayerState.stats[ STAT_TEAM ] );
	CG_DrawWarmup();
	CG_DrawQueue();

	if ( !cg.scoreBoardShowing )
	{
		CG_DrawCenterString();
	}
}

/*
===============
CG_ScalePainBlendTCs
===============
*/
static void CG_ScalePainBlendTCs( float *s1, float *t1, float *s2, float *t2 )
{
	*s1 -= 0.5f;
	*t1 -= 0.5f;
	*s2 -= 0.5f;
	*t2 -= 0.5f;

	*s1 *= cg_painBlendZoom.value;
	*t1 *= cg_painBlendZoom.value;
	*s2 *= cg_painBlendZoom.value;
	*t2 *= cg_painBlendZoom.value;

	*s1 += 0.5f;
	*t1 += 0.5f;
	*s2 += 0.5f;
	*t2 += 0.5f;
}

#define PAINBLEND_BORDER 0.15f

/*
===============
CG_PainBlend
===============
*/
static void CG_PainBlend( void )
{
	vec4_t    color;
	int       damage;
	float     damageAsFracOfMax;
	qhandle_t shader = cgs.media.viewBloodShader;
	float     x, y, w, h;
	float     s1, t1, s2, t2;

	if ( cg.snap->ps.persistant[ PERS_SPECSTATE ] != SPECTATOR_NOT || cg.intermissionStarted )
	{
		return;
	}

	damage = cg.lastHealth - cg.snap->ps.stats[ STAT_HEALTH ];

	if ( damage < 0 )
	{
		damage = 0;
	}

	damageAsFracOfMax = ( float ) damage / cg.snap->ps.stats[ STAT_MAX_HEALTH ];
	cg.lastHealth = cg.snap->ps.stats[ STAT_HEALTH ];

	cg.painBlendValue += damageAsFracOfMax * cg_painBlendScale.value;

	if ( cg.painBlendValue > 0.0f )
	{
		cg.painBlendValue -= ( cg.frametime / 1000.0f ) *
		                     cg_painBlendDownRate.value;
	}

	if ( cg.painBlendValue > 1.0f )
	{
		cg.painBlendValue = 1.0f;
	}
	else if ( cg.painBlendValue <= 0.0f )
	{
		cg.painBlendValue = 0.0f;
		return;
	}

	if ( cg.snap->ps.stats[ STAT_TEAM ] == TEAM_ALIENS )
	{
		VectorSet( color, 0.43f, 0.8f, 0.37f );
	}
	else if ( cg.snap->ps.stats[ STAT_TEAM ] == TEAM_HUMANS )
	{
		VectorSet( color, 0.8f, 0.0f, 0.0f );
	}

	if ( cg.painBlendValue > cg.painBlendTarget )
	{
		cg.painBlendTarget += ( cg.frametime / 1000.0f ) *
		                      cg_painBlendUpRate.value;
	}
	else if ( cg.painBlendValue < cg.painBlendTarget )
	{
		cg.painBlendTarget = cg.painBlendValue;
	}

	if ( cg.painBlendTarget > cg_painBlendMax.value )
	{
		cg.painBlendTarget = cg_painBlendMax.value;
	}

	color[ 3 ] = cg.painBlendTarget;

	trap_R_SetColor( color );

	//left
	x = 0.0f;
	y = 0.0f;
	w = PAINBLEND_BORDER * 640.0f;
	h = 480.0f;
	CG_AdjustFrom640( &x, &y, &w, &h );
	s1 = 0.0f;
	t1 = 0.0f;
	s2 = PAINBLEND_BORDER;
	t2 = 1.0f;
	CG_ScalePainBlendTCs( &s1, &t1, &s2, &t2 );
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, shader );

	//right
	x = 640.0f - ( PAINBLEND_BORDER * 640.0f );
	y = 0.0f;
	w = PAINBLEND_BORDER * 640.0f;
	h = 480.0f;
	CG_AdjustFrom640( &x, &y, &w, &h );
	s1 = 1.0f - PAINBLEND_BORDER;
	t1 = 0.0f;
	s2 = 1.0f;
	t2 = 1.0f;
	CG_ScalePainBlendTCs( &s1, &t1, &s2, &t2 );
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, shader );

	//top
	x = PAINBLEND_BORDER * 640.0f;
	y = 0.0f;
	w = 640.0f - ( 2 * PAINBLEND_BORDER * 640.0f );
	h = PAINBLEND_BORDER * 480.0f;
	CG_AdjustFrom640( &x, &y, &w, &h );
	s1 = PAINBLEND_BORDER;
	t1 = 0.0f;
	s2 = 1.0f - PAINBLEND_BORDER;
	t2 = PAINBLEND_BORDER;
	CG_ScalePainBlendTCs( &s1, &t1, &s2, &t2 );
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, shader );

	//bottom
	x = PAINBLEND_BORDER * 640.0f;
	y = 480.0f - ( PAINBLEND_BORDER * 480.0f );
	w = 640.0f - ( 2 * PAINBLEND_BORDER * 640.0f );
	h = PAINBLEND_BORDER * 480.0f;
	CG_AdjustFrom640( &x, &y, &w, &h );
	s1 = PAINBLEND_BORDER;
	t1 = 1.0f - PAINBLEND_BORDER;
	s2 = 1.0f - PAINBLEND_BORDER;
	t2 = 1.0f;
	CG_ScalePainBlendTCs( &s1, &t1, &s2, &t2 );
	trap_R_DrawStretchPic( x, y, w, h, s1, t1, s2, t2, shader );

	trap_R_SetColor( NULL );
}

/*
=====================
CG_ResetPainBlend
=====================
*/
void CG_ResetPainBlend( void )
{
	cg.painBlendValue = 0.0f;
	cg.painBlendTarget = 0.0f;
	cg.lastHealth = cg.snap->ps.stats[ STAT_HEALTH ];
}

/*
================
CG_DrawBinaryShadersFinalPhases
================
*/
static void CG_DrawBinaryShadersFinalPhases( void )
{
	float      ss;
	char       str[ 20 ];
	float      f, l, u;
	polyVert_t verts[ 4 ] =
	{
		{ { 0, 0, 0 }, { 0, 0 }, { 255, 255, 255, 255 } },
		{ { 0, 0, 0 }, { 1, 0 }, { 255, 255, 255, 255 } },
		{ { 0, 0, 0 }, { 1, 1 }, { 255, 255, 255, 255 } },
		{ { 0, 0, 0 }, { 0, 1 }, { 255, 255, 255, 255 } }
	};
	int        i, j, k;

	if ( !cg.numBinaryShadersUsed )
	{
		return;
	}

	ss = cg_binaryShaderScreenScale.value;

	if ( ss <= 0.0f )
	{
		cg.numBinaryShadersUsed = 0;
		return;
	}
	else if ( ss > 1.0f )
	{
		ss = 1.0f;
	}

	ss = sqrt( ss );

	trap_Cvar_VariableStringBuffer( "r_znear", str, sizeof( str ) );
	f = atof( str ) + 0.01;
	l = f * tan( DEG2RAD( cg.refdef.fov_x / 2 ) ) * ss;
	u = f * tan( DEG2RAD( cg.refdef.fov_y / 2 ) ) * ss;

	VectorMA( cg.refdef.vieworg, f, cg.refdef.viewaxis[ 0 ], verts[ 0 ].xyz );
	VectorMA( verts[ 0 ].xyz, l, cg.refdef.viewaxis[ 1 ], verts[ 0 ].xyz );
	VectorMA( verts[ 0 ].xyz, u, cg.refdef.viewaxis[ 2 ], verts[ 0 ].xyz );
	VectorMA( verts[ 0 ].xyz, -2 * l, cg.refdef.viewaxis[ 1 ], verts[ 1 ].xyz );
	VectorMA( verts[ 1 ].xyz, -2 * u, cg.refdef.viewaxis[ 2 ], verts[ 2 ].xyz );
	VectorMA( verts[ 0 ].xyz, -2 * u, cg.refdef.viewaxis[ 2 ], verts[ 3 ].xyz );

	trap_R_AddPolyToScene( cgs.media.binaryAlpha1Shader, 4, verts );

	for ( i = 0; i < cg.numBinaryShadersUsed; ++i )
	{
		for ( j = 0; j < 4; ++j )
		{
			for ( k = 0; k < 3; ++k )
			{
				verts[ j ].modulate[ k ] = cg.binaryShaderSettings[ i ].color[ k ];
			}
		}

		if ( cg.binaryShaderSettings[ i ].drawFrontline )
		{
			trap_R_AddPolyToScene( cgs.media.binaryShaders[ i ].f3, 4, verts );
		}

		if ( cg.binaryShaderSettings[ i ].drawIntersection )
		{
			trap_R_AddPolyToScene( cgs.media.binaryShaders[ i ].b3, 4, verts );
		}
	}

	cg.numBinaryShadersUsed = 0;
}

/*
=====================
CG_DrawActive

Perform all drawing needed to completely fill the screen
=====================
*/
void CG_DrawActive( stereoFrame_t stereoView )
{
	float  separation;
	vec3_t baseOrg;

	// optionally draw the info screen instead
	if ( !cg.snap )
	{
		return;
	}

	switch ( stereoView )
	{
		case STEREO_CENTER:
			separation = 0;
			break;

		case STEREO_LEFT:
			separation = -cg_stereoSeparation.value / 2;
			break;

		case STEREO_RIGHT:
			separation = cg_stereoSeparation.value / 2;
			break;

		default:
			separation = 0;
			CG_Error( "CG_DrawActive: Undefined stereoView" );
	}

	// clear around the rendered view if sized down
	CG_TileClear();

	// offset vieworg appropriately if we're doing stereo separation
	VectorCopy( cg.refdef.vieworg, baseOrg );

	if ( separation != 0 )
	{
		VectorMA( cg.refdef.vieworg, -separation, cg.refdef.viewaxis[ 1 ],
		          cg.refdef.vieworg );
	}

	CG_DrawBinaryShadersFinalPhases();

	// draw 3D view
	trap_R_RenderScene( &cg.refdef );

	// restore original viewpoint if running stereo
	if ( separation != 0 )
	{
		VectorCopy( baseOrg, cg.refdef.vieworg );
	}

	// first person blend blobs, done after AnglesToAxis
	if ( !cg.renderingThirdPerson )
	{
		CG_PainBlend();
	}

	// draw status bar and other floating elements
	CG_Draw2D();
}
