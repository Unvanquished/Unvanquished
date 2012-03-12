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

/*
 * name:		cg_main.c
 *
 * desc:		initialization and primary entry point for cgame
 *
*/


#include "cg_local.h"

displayContextDef_t cgDC;

void            CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback);
void            CG_Shutdown(void);
qboolean        CG_CheckExecKey(int key);
extern itemDef_t *g_bindItem;
extern qboolean g_waitingForKey;

/*
================
vmMain

This is the only way control passes into the module.
This must be the very first function compiled into the .q3vm file
================
*/
#if __GNUC__ >= 4
#pragma GCC visibility push(default)
#endif

#ifdef IPHONE
int baseq3_cgame_vmMain( int command, int arg0, int arg1, int arg2, int arg3, int arg4, int arg5, int arg6, int arg7, int arg8, int arg9, int arg10, int arg11  ) {
#else
intptr_t vmMain( intptr_t command, intptr_t arg0, intptr_t arg1, intptr_t arg2, intptr_t arg3, intptr_t arg4, intptr_t arg5, intptr_t arg6, intptr_t arg7, intptr_t arg8, intptr_t arg9, intptr_t arg10, intptr_t arg11  ) {
#endif // IPHONE

#if __GNUC__ >= 4
#pragma GCC visibility pop
#endif
	switch (command)
	{
		case CG_INIT:
			CG_Init(arg0, arg1, arg2, arg3);
			cgs.initing = qfalse;
			return 0;
		case CG_SHUTDOWN:
			CG_Shutdown();
			return 0;
		case CG_CONSOLE_COMMAND:
			return CG_ConsoleCommand();
		case CG_DRAW_ACTIVE_FRAME:
			CG_DrawActiveFrame(arg0, arg1, arg2);
			return 0;
		case CG_CONSOLE_TEXT:
			//CG_AddNotifyText( );
			return 0;
		case CG_CROSSHAIR_PLAYER:
			return CG_CrosshairPlayer();
		case CG_LAST_ATTACKER:
			return CG_LastAttacker();
		case CG_KEY_EVENT:
			CG_KeyEvent(arg0, arg1);
			return 0;
		case CG_MOUSE_EVENT:
			cgDC.cursorx = cgs.cursorX;
			cgDC.cursory = cgs.cursorY;
			CG_MouseEvent(arg0, arg1);
			return 0;
		case CG_EVENT_HANDLING:
			CG_EventHandling(arg0, qtrue);
			return 0;
		case CG_GET_TAG:
			return CG_GetTag(arg0, (char *)arg1, (orientation_t *) arg2);
		case CG_CHECKEXECKEY:
			return CG_CheckExecKey(arg0);
		case CG_WANTSBINDKEYS:
			return (g_waitingForKey && g_bindItem) ? qtrue : qfalse;
		case CG_MESSAGERECEIVED:
			return -1;
		
		case CG_COMPLETE_COMMAND:
			//CG_CompleteCommand( arg0 );
			return 0;

		case CG_VOIP_STRING:
			//return (intptr_t)CG_VoIPString( );
			return 0;
		
		default:
			CG_Error("vmMain: unknown command %i", (int)command);
			break;
	}
	return -1;
}

cg_t            cg;
cgs_t           cgs;
centity_t       cg_entities[MAX_GENTITIES];
weaponInfo_t    cg_weapons[MAX_WEAPONS];
itemInfo_t      cg_items[MAX_ITEMS];

vmCvar_t        cg_railTrailTime;
vmCvar_t        cg_centertime;
vmCvar_t        cg_runpitch;
vmCvar_t        cg_runroll;
vmCvar_t        cg_bobup;
vmCvar_t        cg_bobpitch;
vmCvar_t        cg_bobroll;
vmCvar_t        cg_bobyaw;
vmCvar_t        cg_swingSpeed;
vmCvar_t        cg_shadows;
vmCvar_t        cg_gibs;
vmCvar_t        cg_draw2D;
vmCvar_t        cg_drawFPS;
vmCvar_t        cg_drawSnapshot;
vmCvar_t        cg_drawCrosshair;
vmCvar_t        cg_drawCrosshairNames;
vmCvar_t        cg_drawCrosshairPickups;
vmCvar_t        cg_weaponCycleDelay;	//----(SA)  added
vmCvar_t        cg_cycleAllWeaps;
vmCvar_t        cg_useWeapsForZoom;
vmCvar_t        cg_crosshairSize;
vmCvar_t        cg_crosshairX;
vmCvar_t        cg_crosshairY;
vmCvar_t        cg_crosshairHealth;
vmCvar_t        cg_teamChatsOnly;
vmCvar_t        cg_noVoiceChats;	// NERVE - SMF
vmCvar_t        cg_noVoiceText;	// NERVE - SMF
vmCvar_t        cg_drawStatus;
vmCvar_t        cg_animSpeed;
vmCvar_t        cg_drawSpreadScale;
vmCvar_t        cg_debugAnim;
vmCvar_t        cg_debugPosition;
vmCvar_t        cg_debugEvents;
vmCvar_t        cg_errorDecay;
vmCvar_t        cg_nopredict;
vmCvar_t        cg_noPlayerAnims;
vmCvar_t        cg_showmiss;
vmCvar_t        cg_footsteps;
vmCvar_t        cg_markTime;
vmCvar_t        cg_brassTime;
vmCvar_t        cg_letterbox;	//----(SA) added
vmCvar_t        cg_drawGun;
vmCvar_t        cg_cursorHints;	//----(SA)  added
vmCvar_t        cg_gun_frame;
vmCvar_t        cg_gun_x;
vmCvar_t        cg_gun_y;
vmCvar_t        cg_gun_z;
vmCvar_t        cg_tracerChance;
vmCvar_t        cg_tracerWidth;
vmCvar_t        cg_tracerLength;
vmCvar_t        cg_tracerSpeed;
vmCvar_t        cg_autoswitch;
vmCvar_t        cg_ignore;
vmCvar_t        cg_fov;
vmCvar_t        cg_zoomFov;
vmCvar_t        cg_zoomStepBinoc;
vmCvar_t        cg_zoomStepSniper;
vmCvar_t        cg_zoomStepSnooper;
vmCvar_t        cg_zoomStepFG;	//----(SA)  added
vmCvar_t        cg_zoomDefaultBinoc;
vmCvar_t        cg_zoomDefaultSniper;
vmCvar_t        cg_zoomDefaultSnooper;
vmCvar_t        cg_zoomDefaultFG;	//----(SA)  added
vmCvar_t        cg_thirdPerson;
vmCvar_t        cg_thirdPersonRange;
vmCvar_t        cg_thirdPersonAngle;
vmCvar_t        cg_stereoSeparation;
vmCvar_t        cg_lagometer;

#ifdef ALLOW_GSYNC
vmCvar_t        cg_synchronousClients;
#endif							// ALLOW_GSYNC
vmCvar_t        cg_teamChatTime;
vmCvar_t        cg_teamChatHeight;
vmCvar_t        cg_stats;
vmCvar_t        cg_buildScript;
vmCvar_t        cg_coronafardist;
vmCvar_t        cg_coronas;
vmCvar_t        cg_paused;
vmCvar_t        cg_blood;
vmCvar_t        cg_predictItems;
vmCvar_t        cg_deferPlayers;
vmCvar_t        cg_drawTeamOverlay;
vmCvar_t        cg_enableBreath;
vmCvar_t        cg_autoactivate;
vmCvar_t        cg_blinktime;	//----(SA)  added

vmCvar_t        cg_smoothClients;
vmCvar_t        pmove_fixed;
vmCvar_t        pmove_msec;

// Rafael - particle switch
vmCvar_t        cg_wolfparticles;

// done

// Ridah
vmCvar_t        cg_gameType;
vmCvar_t        cg_bloodTime;
vmCvar_t        cg_norender;
vmCvar_t        cg_skybox;

// ydnar: say, team say, etc.
vmCvar_t        cg_message;
vmCvar_t        cg_messageType;
vmCvar_t        cg_messagePlayer;
vmCvar_t        cg_messagePlayerName;
vmCvar_t        cg_movespeed;
vmCvar_t        cg_cameraMode;
vmCvar_t        cg_cameraOrbit;
vmCvar_t        cg_cameraOrbitDelay;
vmCvar_t        cg_timescaleFadeEnd;
vmCvar_t        cg_timescaleFadeSpeed;
vmCvar_t        cg_timescale;
vmCvar_t        cg_smallFont;
vmCvar_t        cg_bigFont;
vmCvar_t        cg_noTaunt;		// NERVE - SMF
vmCvar_t        cg_voiceSpriteTime;	// DHM - Nerve

vmCvar_t        cg_animState;

vmCvar_t        cg_drawCompass;
vmCvar_t        cg_drawNotifyText;
vmCvar_t        cg_quickMessageAlt;
vmCvar_t        cg_popupLimboMenu;
vmCvar_t        cg_descriptiveText;

// -NERVE - SMF

vmCvar_t        cg_redlimbotime;
vmCvar_t        cg_bluelimbotime;

vmCvar_t        cg_antilag;

vmCvar_t        developer;

// OSP
vmCvar_t        authLevel;

vmCvar_t        cf_wstats;		// Font scale for +wstats window
vmCvar_t        cf_wtopshots;	// Font scale for +wtopshots window

//vmCvar_t  cg_announcer;
vmCvar_t        cg_autoAction;
vmCvar_t        cg_autoReload;
vmCvar_t        cg_bloodDamageBlend;
vmCvar_t        cg_bloodFlash;
vmCvar_t        cg_complaintPopUp;
vmCvar_t        cg_crosshairAlpha;
vmCvar_t        cg_crosshairAlphaAlt;
vmCvar_t        cg_crosshairColor;
vmCvar_t        cg_crosshairColorAlt;
vmCvar_t        cg_crosshairPulse;
vmCvar_t        cg_drawReinforcementTime;
vmCvar_t        cg_drawWeaponIconFlash;
vmCvar_t        cg_noAmmoAutoSwitch;
vmCvar_t        cg_printObjectiveInfo;
vmCvar_t        cg_specHelp;
vmCvar_t        cg_uinfo;
vmCvar_t        cg_useScreenshotJPEG;

vmCvar_t        ch_font;

vmCvar_t        demo_avifpsF1;
vmCvar_t        demo_avifpsF2;
vmCvar_t        demo_avifpsF3;
vmCvar_t        demo_avifpsF4;
vmCvar_t        demo_avifpsF5;
vmCvar_t        demo_drawTimeScale;
vmCvar_t        demo_infoWindow;

vmCvar_t        mv_sensitivity;

vmCvar_t        int_cl_maxpackets;
vmCvar_t        int_cl_timenudge;
vmCvar_t        int_m_pitch;
vmCvar_t        int_sensitivity;
vmCvar_t        int_timescale;
vmCvar_t        int_ui_blackout;

// -OSP

vmCvar_t        cg_rconPassword;
vmCvar_t        cg_refereePassword;
vmCvar_t        cg_atmosphericEffects;

// START Mad Doc - TDF
vmCvar_t        cg_drawRoundTimer;

// END Mad Doc - TDF

vmCvar_t        cg_fastSolids;
vmCvar_t        cg_instanttapout;

vmCvar_t        cg_debugSkills;
vmCvar_t        cg_drawFireteamOverlay;
vmCvar_t        cg_drawSmallPopupIcons;

//bani - demo recording cvars
vmCvar_t        cl_demorecording;
vmCvar_t        cl_demofilename;
vmCvar_t        cl_demooffset;

//bani - wav recording cvars
vmCvar_t        cl_waverecording;
vmCvar_t        cl_wavefilename;
vmCvar_t        cl_waveoffset;
vmCvar_t        cg_recording_statusline;

typedef struct
{
	vmCvar_t       *vmCvar;
	char           *cvarName;
	char           *defaultString;
	int             cvarFlags;
	int             modificationCount;
} cvarTable_t;

cvarTable_t     cvarTable[] = {
	{&cg_ignore, "cg_ignore", "0", 0},	// used for debugging
	{&cg_autoswitch, "cg_autoswitch", "2", CVAR_ARCHIVE},
	{&cg_drawGun, "cg_drawGun", "1", CVAR_ARCHIVE},
	{&cg_gun_frame, "cg_gun_frame", "0", CVAR_TEMP},
	{&cg_cursorHints, "cg_cursorHints", "1", CVAR_ARCHIVE},
	{&cg_zoomFov, "cg_zoomfov", "22.5", CVAR_ARCHIVE},
	{&cg_zoomDefaultBinoc, "cg_zoomDefaultBinoc", "22.5", CVAR_ARCHIVE},
	{&cg_zoomDefaultSniper, "cg_zoomDefaultSniper", "20", CVAR_ARCHIVE},	// JPW NERVE changed per atvi req
	{&cg_zoomDefaultSnooper, "cg_zoomDefaultSnooper", "40", CVAR_ARCHIVE},	// JPW NERVE made temp
	{&cg_zoomDefaultFG, "cg_zoomDefaultFG", "55", CVAR_ARCHIVE},	//----(SA)    added // JPW NERVE made temp
	{&cg_zoomStepBinoc, "cg_zoomStepBinoc", "3", CVAR_ARCHIVE},
	{&cg_zoomStepSniper, "cg_zoomStepSniper", "2", CVAR_ARCHIVE},
	{&cg_zoomStepSnooper, "cg_zoomStepSnooper", "5", CVAR_ARCHIVE},
	{&cg_zoomStepFG, "cg_zoomStepFG", "10", CVAR_ARCHIVE},	//----(SA)    added
	{&cg_fov, "cg_fov", "90", CVAR_ARCHIVE},
	{&cg_letterbox, "cg_letterbox", "0", CVAR_TEMP},	//----(SA)    added
	{&cg_stereoSeparation, "cg_stereoSeparation", "0.4", CVAR_ARCHIVE},
	{&cg_shadows, "cg_shadows", "1", CVAR_ARCHIVE},
	{&cg_gibs, "cg_gibs", "1", CVAR_ARCHIVE},
//bani - #127 - we now draw reticles always in non demoplayback
//  { &cg_draw2D, "cg_draw2D", "1", CVAR_CHEAT }, // JPW NERVE changed per atvi req to prevent sniper rifle zoom cheats
	{&cg_draw2D, "cg_draw2D", "1", CVAR_ARCHIVE},
	{&cg_drawSpreadScale, "cg_drawSpreadScale", "1", CVAR_ARCHIVE},
	{&cg_drawStatus, "cg_drawStatus", "1", CVAR_ARCHIVE},
	{&cg_drawFPS, "cg_drawFPS", "0", CVAR_ARCHIVE},
	{&cg_drawSnapshot, "cg_drawSnapshot", "0", CVAR_ARCHIVE},
	{&cg_drawCrosshair, "cg_drawCrosshair", "1", CVAR_ARCHIVE},
	{&cg_drawCrosshairNames, "cg_drawCrosshairNames", "1", CVAR_ARCHIVE},
	{&cg_drawCrosshairPickups, "cg_drawCrosshairPickups", "1", CVAR_ARCHIVE},
	{&cg_useWeapsForZoom, "cg_useWeapsForZoom", "1", CVAR_ARCHIVE},
	{&cg_weaponCycleDelay, "cg_weaponCycleDelay", "150", CVAR_ARCHIVE},	//----(SA)    added
	{&cg_cycleAllWeaps, "cg_cycleAllWeaps", "1", CVAR_ARCHIVE},
	{&cg_crosshairSize, "cg_crosshairSize", "48", CVAR_ARCHIVE},
	{&cg_crosshairHealth, "cg_crosshairHealth", "0", CVAR_ARCHIVE},
	{&cg_crosshairX, "cg_crosshairX", "0", CVAR_ARCHIVE},
	{&cg_crosshairY, "cg_crosshairY", "0", CVAR_ARCHIVE},
	{&cg_brassTime, "cg_brassTime", "2500", CVAR_ARCHIVE},	// JPW NERVE
	{&cg_markTime, "cg_marktime", "20000", CVAR_ARCHIVE},
	{&cg_lagometer, "cg_lagometer", "0", CVAR_ARCHIVE},
	{&cg_railTrailTime, "cg_railTrailTime", "400", CVAR_ARCHIVE},
	{&cg_gun_x, "cg_gunX", "0", CVAR_CHEAT},
	{&cg_gun_y, "cg_gunY", "0", CVAR_CHEAT},
	{&cg_gun_z, "cg_gunZ", "0", CVAR_CHEAT},
	{&cg_centertime, "cg_centertime", "5", CVAR_CHEAT},	// DHM - Nerve :: changed from 3 to 5
	{&cg_runpitch, "cg_runpitch", "0.002", CVAR_ARCHIVE},
	{&cg_runroll, "cg_runroll", "0.005", CVAR_ARCHIVE},
	{&cg_bobup, "cg_bobup", "0.005", CVAR_ARCHIVE},
	{&cg_bobpitch, "cg_bobpitch", "0.002", CVAR_ARCHIVE},
	{&cg_bobroll, "cg_bobroll", "0.002", CVAR_ARCHIVE},
	{&cg_bobyaw, "cg_bobyaw", "0.002", CVAR_ARCHIVE},

	// JOSEPH 10-27-99
	{&cg_autoactivate, "cg_autoactivate", "1", CVAR_ARCHIVE},
	// END JOSEPH

	// Ridah, more fluid rotations
	{&cg_swingSpeed, "cg_swingSpeed", "0.1", CVAR_CHEAT},	// was 0.3 for Q3
	{&cg_bloodTime, "cg_bloodTime", "120", CVAR_ARCHIVE},

	{&cg_skybox, "cg_skybox", "1", CVAR_CHEAT},
	// done.

	// ydnar: say, team say, etc.
	{&cg_message, "cg_message", "1", CVAR_TEMP},
	{&cg_messageType, "cg_messageType", "1", CVAR_TEMP},
	{&cg_messagePlayer, "cg_messagePlayer", "", CVAR_TEMP},
	{&cg_messagePlayerName, "cg_messagePlayerName", "", CVAR_TEMP},

	{&cg_animSpeed, "cg_animspeed", "1", CVAR_CHEAT},
	{&cg_debugAnim, "cg_debuganim", "0", CVAR_CHEAT},
	{&cg_debugPosition, "cg_debugposition", "0", CVAR_CHEAT},
	{&cg_debugEvents, "cg_debugevents", "0", CVAR_CHEAT},
	{&cg_errorDecay, "cg_errordecay", "100", 0},
	{&cg_nopredict, "cg_nopredict", "0", CVAR_CHEAT},
	{&cg_noPlayerAnims, "cg_noplayeranims", "0", CVAR_CHEAT},
	{&cg_showmiss, "cg_showmiss", "0", 0},
	{&cg_footsteps, "cg_footsteps", "1", CVAR_CHEAT},
	{&cg_tracerChance, "cg_tracerchance", "0.4", CVAR_CHEAT},
	{&cg_tracerWidth, "cg_tracerwidth", "0.8", CVAR_CHEAT},
	{&cg_tracerSpeed, "cg_tracerSpeed", "4500", CVAR_CHEAT},
	{&cg_tracerLength, "cg_tracerlength", "160", CVAR_CHEAT},
	{&cg_thirdPersonRange, "cg_thirdPersonRange", "80", CVAR_CHEAT},	// JPW NERVE per atvi req
	{&cg_thirdPersonAngle, "cg_thirdPersonAngle", "0", CVAR_CHEAT},
	{&cg_thirdPerson, "cg_thirdPerson", "0", CVAR_ARCHIVE},	// JPW NERVE per atvi req
	{&cg_teamChatTime, "cg_teamChatTime", "8000", CVAR_ARCHIVE},
	{&cg_teamChatHeight, "cg_teamChatHeight", "8", CVAR_ARCHIVE},
	{&cg_coronafardist, "cg_coronafardist", "1536", CVAR_ARCHIVE},
	{&cg_coronas, "cg_coronas", "1", CVAR_ARCHIVE},
	{&cg_predictItems, "cg_predictItems", "1", CVAR_ARCHIVE},
	{&cg_deferPlayers, "cg_deferPlayers", "1", CVAR_ARCHIVE},
	{&cg_drawTeamOverlay, "cg_drawTeamOverlay", "2", CVAR_ARCHIVE},
	{&cg_stats, "cg_stats", "0", 0},
	{&cg_blinktime, "cg_blinktime", "100", CVAR_ARCHIVE},	//----(SA)   added

	{&cg_enableBreath, "cg_enableBreath", "1", CVAR_SERVERINFO},
	{&cg_cameraOrbit, "cg_cameraOrbit", "0", CVAR_CHEAT},
	{&cg_cameraOrbitDelay, "cg_cameraOrbitDelay", "50", CVAR_ARCHIVE},
	{&cg_timescaleFadeEnd, "cg_timescaleFadeEnd", "1", 0},
	{&cg_timescaleFadeSpeed, "cg_timescaleFadeSpeed", "0", 0},
	{&cg_timescale, "timescale", "1", 0},
//  { &cg_smoothClients, "cg_smoothClients", "0", CVAR_USERINFO | CVAR_ARCHIVE},
	{&cg_cameraMode, "com_cameraMode", "0", CVAR_CHEAT},

	{&pmove_fixed, "pmove_fixed", "0", 0},
	{&pmove_msec, "pmove_msec", "8", 0},

	{&cg_noTaunt, "cg_noTaunt", "0", CVAR_ARCHIVE},	// NERVE - SMF
	{&cg_voiceSpriteTime, "cg_voiceSpriteTime", "6000", CVAR_ARCHIVE},	// DHM - Nerve

	{&cg_smallFont, "ui_smallFont", "0.25", CVAR_ARCHIVE},
	{&cg_bigFont, "ui_bigFont", "0.4", CVAR_ARCHIVE},

	{&cg_teamChatsOnly, "cg_teamChatsOnly", "0", CVAR_ARCHIVE},
	{&cg_noVoiceChats, "cg_noVoiceChats", "0", CVAR_ARCHIVE},	// NERVE - SMF
	{&cg_noVoiceText, "cg_noVoiceText", "0", CVAR_ARCHIVE},	// NERVE - SMF

	// the following variables are created in other parts of the system,
	// but we also reference them here

	{&cg_buildScript, "com_buildScript", "0", 0},	// force loading of all possible data amd error on failures
	{&cg_paused, "cl_paused", "0", CVAR_ROM},

	{&cg_blood, "cg_showblood", "1", CVAR_ARCHIVE},
#ifdef ALLOW_GSYNC
	{&cg_synchronousClients, "g_synchronousClients", "0", CVAR_SYSTEMINFO | CVAR_CHEAT},	// communicated by systeminfo
#endif							// ALLOW_GSYNC

	// Rafael - particle switch
	{&cg_wolfparticles, "cg_wolfparticles", "1", CVAR_ARCHIVE},
	{&cg_gameType, "g_gametype", "0", 0},	// communicated by systeminfo
	{&cg_norender, "cg_norender", "0", 0},	// only used during single player, to suppress rendering until the server is ready
	{&cg_bluelimbotime, "", "30000", 0},	// communicated by systeminfo
	{&cg_redlimbotime, "", "30000", 0},	// communicated by systeminfo
	{&cg_movespeed, "g_movespeed", "76", 0},	// actual movespeed of player
	{&cg_animState, "cg_animState", "0", CVAR_CHEAT},
	{&cg_drawCompass, "cg_drawCompass", "1", CVAR_ARCHIVE},
	{&cg_drawNotifyText, "cg_drawNotifyText", "1", CVAR_ARCHIVE},
	{&cg_quickMessageAlt, "cg_quickMessageAlt", "0", CVAR_ARCHIVE},
	{&cg_popupLimboMenu, "cg_popupLimboMenu", "1", CVAR_ARCHIVE},
	{&cg_descriptiveText, "cg_descriptiveText", "1", CVAR_ARCHIVE},
	{&cg_antilag, "g_antilag", "1", 0},
	{&developer, "developer", "0", CVAR_CHEAT},
	{&cf_wstats, "cf_wstats", "1.2", CVAR_ARCHIVE},
	{&cf_wtopshots, "cf_wtopshots", "1.0", CVAR_ARCHIVE},
	//{ &cg_announcer, "cg_announcer", "1", CVAR_ARCHIVE },
	{&cg_autoAction, "cg_autoAction", "0", CVAR_ARCHIVE},
	{&cg_autoReload, "cg_autoReload", "1", CVAR_ARCHIVE},
	{&cg_bloodDamageBlend, "cg_bloodDamageBlend", "1.0", CVAR_ARCHIVE},
	{&cg_bloodFlash, "cg_bloodFlash", "1.0", CVAR_ARCHIVE},
	{&cg_complaintPopUp, "cg_complaintPopUp", "1", CVAR_ARCHIVE},
	{&cg_crosshairAlpha, "cg_crosshairAlpha", "1.0", CVAR_ARCHIVE},
	{&cg_crosshairAlphaAlt, "cg_crosshairAlphaAlt", "1.0", CVAR_ARCHIVE},
	{&cg_crosshairColor, "cg_crosshairColor", "White", CVAR_ARCHIVE},
	{&cg_crosshairColorAlt, "cg_crosshairColorAlt", "White", CVAR_ARCHIVE},
	{&cg_crosshairPulse, "cg_crosshairPulse", "1", CVAR_ARCHIVE},
	{&cg_drawReinforcementTime, "cg_drawReinforcementTime", "1", CVAR_ARCHIVE},
	{&cg_drawWeaponIconFlash, "cg_drawWeaponIconFlash", "0", CVAR_ARCHIVE},
	{&cg_noAmmoAutoSwitch, "cg_noAmmoAutoSwitch", "1", CVAR_ARCHIVE},
	{&cg_printObjectiveInfo, "cg_printObjectiveInfo", "1", CVAR_ARCHIVE},
	{&cg_specHelp, "cg_specHelp", "1", CVAR_ARCHIVE},
	{&cg_uinfo, "cg_uinfo", "0", CVAR_ROM | CVAR_USERINFO},
	{&cg_useScreenshotJPEG, "cg_useScreenshotJPEG", "1", CVAR_ARCHIVE},

	{&demo_avifpsF1, "demo_avifpsF1", "0", CVAR_ARCHIVE},
	{&demo_avifpsF2, "demo_avifpsF2", "10", CVAR_ARCHIVE},
	{&demo_avifpsF3, "demo_avifpsF3", "15", CVAR_ARCHIVE},
	{&demo_avifpsF4, "demo_avifpsF4", "20", CVAR_ARCHIVE},
	{&demo_avifpsF5, "demo_avifpsF5", "24", CVAR_ARCHIVE},
	{&demo_drawTimeScale, "demo_drawTimeScale", "1", CVAR_ARCHIVE},
	{&demo_infoWindow, "demo_infoWindow", "1", CVAR_ARCHIVE},

#ifdef MV_SUPPORT
	{&mv_sensitivity, "mv_sensitivity", "20", CVAR_ARCHIVE},
#endif

	// Engine mappings
	{&int_cl_maxpackets, "cl_maxpackets", "30", CVAR_ARCHIVE},
	{&int_cl_timenudge, "cl_timenudge", "0", CVAR_ARCHIVE},
	{&int_m_pitch, "m_pitch", "0.022", CVAR_ARCHIVE},
	{&int_sensitivity, "sensitivity", "5", CVAR_ARCHIVE},
	{&int_ui_blackout, "ui_blackout", "0", CVAR_ROM},
	// -OSP

	{&cg_atmosphericEffects, "cg_atmosphericEffects", "1", CVAR_ARCHIVE},
	{&authLevel, "authLevel", "0", CVAR_TEMP | CVAR_ROM},

	{&cg_rconPassword, "auth_rconPassword", "", CVAR_TEMP},
	{&cg_refereePassword, "auth_refereePassword", "", CVAR_TEMP},

	{&cg_drawRoundTimer, "cg_drawRoundTimer", "1", CVAR_ARCHIVE},

	// Gordon: optimization cvars: 18/12/02 enabled by default now
	{&cg_fastSolids, "cg_fastSolids", "1", CVAR_ARCHIVE},

	{&cg_instanttapout, "cg_instanttapout", "0", CVAR_ARCHIVE},
	{&cg_debugSkills, "cg_debugSkills", "0", 0},
	{NULL, "cg_etVersion", "", CVAR_USERINFO | CVAR_ROM},
	{&cg_drawFireteamOverlay, "cg_drawFireteamOverlay", "1", CVAR_ARCHIVE},
	{&cg_drawSmallPopupIcons, "cg_drawSmallPopupIcons", "0", CVAR_ARCHIVE},

	//bani - demo recording cvars
	{&cl_demorecording, "cl_demorecording", "0", CVAR_ROM},
	{&cl_demofilename, "cl_demofilename", "", CVAR_ROM},
	{&cl_demooffset, "cl_demooffset", "0", CVAR_ROM},
	//bani - wav recording cvars
	{&cl_waverecording, "cl_waverecording", "0", CVAR_ROM},
	{&cl_wavefilename, "cl_wavefilename", "", CVAR_ROM},
	{&cl_waveoffset, "cl_waveoffset", "0", CVAR_ROM},
	{&cg_recording_statusline, "cg_recording_statusline", "9", CVAR_ARCHIVE},
};

int             cvarTableSize = sizeof(cvarTable) / sizeof(cvarTable[0]);
qboolean        cvarsLoaded = qfalse;
void            CG_setClientFlags(void);


/*
=================
CG_RegisterCvars
=================
*/
void CG_RegisterCvars(void)
{
	int             i;
	cvarTable_t    *cv;
	char            var[MAX_TOKEN_CHARS];

	trap_Cvar_Set("cg_letterbox", "0");	// force this for people who might have it in their

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
	{
		trap_Cvar_Register(cv->vmCvar, cv->cvarName, cv->defaultString, cv->cvarFlags);
		if(cv->vmCvar != NULL)
		{
			// rain - force the update to range check this cvar on first run
			if(cv->vmCvar == &cg_errorDecay)
			{
				cv->modificationCount = !cv->vmCvar->modificationCount;
			}
			else
			{
				cv->modificationCount = cv->vmCvar->modificationCount;
			}
		}
	}

	// see if we are also running the server on this machine
	trap_Cvar_VariableStringBuffer("sv_running", var, sizeof(var));
	cgs.localServer = atoi(var);

	// Gordon: um, here, why?
	CG_setClientFlags();
	BG_setCrosshair(cg_crosshairColor.string, cg.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor");
	BG_setCrosshair(cg_crosshairColorAlt.string, cg.xhairColorAlt, cg_crosshairAlphaAlt.value, "cg_crosshairColorAlt");

	cvarsLoaded = qtrue;
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars(void)
{
	int             i;
	qboolean        fSetFlags = qfalse;
	cvarTable_t    *cv;

	if(!cvarsLoaded)
	{
		return;
	}

	for(i = 0, cv = cvarTable; i < cvarTableSize; i++, cv++)
	{
		if(cv->vmCvar)
		{
			trap_Cvar_Update(cv->vmCvar);
			if(cv->modificationCount != cv->vmCvar->modificationCount)
			{
				cv->modificationCount = cv->vmCvar->modificationCount;

				// Check if we need to update any client flags to be sent to the server
				if(cv->vmCvar == &cg_autoAction || cv->vmCvar == &cg_autoReload ||
				   cv->vmCvar == &int_cl_timenudge || cv->vmCvar == &int_cl_maxpackets ||
				   cv->vmCvar == &cg_autoactivate || cv->vmCvar == &cg_predictItems)
				{
					fSetFlags = qtrue;
				}
				else if(cv->vmCvar == &cg_crosshairColor || cv->vmCvar == &cg_crosshairAlpha)
				{
					BG_setCrosshair(cg_crosshairColor.string, cg.xhairColor, cg_crosshairAlpha.value, "cg_crosshairColor");
				}
				else if(cv->vmCvar == &cg_crosshairColorAlt || cv->vmCvar == &cg_crosshairAlphaAlt)
				{
					BG_setCrosshair(cg_crosshairColorAlt.string, cg.xhairColorAlt, cg_crosshairAlphaAlt.value,
									"cg_crosshairColorAlt");
				}
				else if(cv->vmCvar == &cg_rconPassword && *cg_rconPassword.string)
				{
					trap_SendConsoleCommand(va("rconAuth %s", cg_rconPassword.string));
				}
				else if(cv->vmCvar == &cg_refereePassword && *cg_refereePassword.string)
				{
					trap_SendConsoleCommand(va("ref %s", cg_refereePassword.string));
				}
				else if(cv->vmCvar == &demo_infoWindow)
				{
					if(demo_infoWindow.integer == 0 && cg.demohelpWindow == SHOW_ON)
					{
						CG_ShowHelp_On(&cg.demohelpWindow);
					}
					else if(demo_infoWindow.integer > 0 && cg.demohelpWindow != SHOW_ON)
					{
						CG_ShowHelp_On(&cg.demohelpWindow);
					}
				}
				else if(cv->vmCvar == &cg_errorDecay)
				{
					// rain - cap errordecay because
					// prediction is EXTREMELY broken
					// right now.
					if(cg_errorDecay.value < 0.0)
					{
						trap_Cvar_Set("cg_errorDecay", "0");
					}
					else if(cg_errorDecay.value > 500.0)
					{
						trap_Cvar_Set("cg_errorDecay", "500");
					}
				}
			}
		}
	}

	// Send any relevent updates
	if(fSetFlags)
	{
		CG_setClientFlags();
	}
}

void CG_setClientFlags(void)
{
	if(cg.demoPlayback)
	{
		return;
	}

	cg.pmext.bAutoReload = (cg_autoReload.integer > 0);
	trap_Cvar_Set("cg_uinfo", va("%d %d %d",
								 // Client Flags
								 (((cg_autoReload.integer > 0) ? CGF_AUTORELOAD : 0) |
								  ((cg_autoAction.integer & AA_STATSDUMP) ? CGF_STATSDUMP : 0) |
								  ((cg_autoactivate.integer > 0) ? CGF_AUTOACTIVATE : 0) |
								  ((cg_predictItems.integer > 0) ? CGF_PREDICTITEMS : 0)
								  // Add more in here, as needed
								 ),
								 // Timenudge
								 int_cl_timenudge.integer,
								 // MaxPackets
								 int_cl_maxpackets.integer));
}

int CG_CrosshairPlayer(void)
{
	if(cg.time > (cg.crosshairClientTime + 1000))
	{
		return -1;
	}
	return cg.crosshairClientNum;
}

int CG_LastAttacker(void)
{
	// OSP - used for messaging clients in the currect active window
	if(cg.mvTotalClients > 0)
	{
		return (cg.mvCurrentActive->mvInfo & MV_PID);
	}
	// OSP
	return ((!cg.attackerTime) ? -1 : cg.snap->ps.persistant[PERS_ATTACKER]);
}

void QDECL CG_Printf(const char *msg, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);
	if(!Q_strncmp(text, "[cgnotify]", 10))
	{
		char            buf[1024];

		if(!cg_drawNotifyText.integer)
		{
			Q_strncpyz(buf, &text[10], 1013);
			trap_Print(buf);
			return;
		}

		CG_AddToNotify(&text[10]);
		Q_strncpyz(buf, &text[10], 1013);
		Q_strncpyz(text, "[skipnotify]", 13);
		Q_strcat(text, 1011, buf);
	}

	trap_Print(text);
}

void QDECL CG_Error(const char *msg, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Error(text);
}

#ifndef CGAME_HARD_LINKED
// this is only here so the functions in q_shared.c and bg_*.c can link (FIXME)

void QDECL Com_Error(int level, const char *error, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, error);
	Q_vsnprintf(text, sizeof(text), error, argptr);
	va_end(argptr);

	trap_Error( text );
}

void QDECL Com_Printf(const char *msg, ...)
{
	va_list         argptr;
	char            text[1024];

	va_start(argptr, msg);
	Q_vsnprintf(text, sizeof(text), msg, argptr);
	va_end(argptr);

	trap_Error( text );
}

#endif

/*
================
CG_Argv
================
*/
const char     *CG_Argv(int arg)
{
	static char     buffer[MAX_STRING_CHARS];

	trap_Argv(arg, buffer, sizeof(buffer));

	return buffer;
}


// Cleans a string for filesystem compatibility
void CG_nameCleanFilename(const char *pszIn, char *pszOut, unsigned int dwOutSize)
{
	unsigned int    dwCurrLength = 0;

	while(*pszIn && dwCurrLength < dwOutSize)
	{
		if(*pszIn == 27 || *pszIn == '^')
		{
			pszIn++;
			dwCurrLength++;

			if(*pszIn)
			{
				pszIn++;		// skip color code
				dwCurrLength++;
				continue;
			}
		}

		// Illegal Windows characters
		if(*pszIn == '\\' || *pszIn == '/' || *pszIn == ':' || *pszIn == '"' ||
		   *pszIn == '*' || *pszIn == '?' || *pszIn == '<' || *pszIn == '>' || *pszIn == '|' || *pszIn == '.')
		{
			pszIn++;
			dwCurrLength++;
			continue;
		}

		if(*pszIn <= 32)
		{
			pszIn++;
			dwCurrLength++;
			continue;
		}

		*pszOut++ = *pszIn++;
		dwCurrLength++;
	}

	*pszOut = 0;
}

// Standard naming for screenshots/demos
char           *CG_generateFilename(void)
{
	qtime_t         ct;

//  int index = (cg.snap == NULL || (cg.snap->ps.pm_flags & PMF_LIMBO)) ? cg.clientNum : cg.snap->ps.clientNum;
//  char strCleanName[64];
	const char     *pszServerInfo = CG_ConfigString(CS_SERVERINFO);

//  const char *pszPlayerInfo = CG_ConfigString(CS_PLAYERS + index);

	trap_RealTime(&ct);
//  CG_nameCleanFilename(Info_ValueForKey(pszPlayerInfo, "n"), strCleanName, sizeof(strCleanName));
	return (va("%04i-%02i-%02i_%02i%02i%02i_%s%s",
	           1900 + ct.tm_year, ct.tm_mon + 1, ct.tm_mday,
	           ct.tm_hour, ct.tm_min, ct.tm_sec,
	           Info_ValueForKey(pszServerInfo, "mapname"), (cg.mvTotalClients < 1) ? "" : "_MVD"));
}

int CG_findClientNum(char *s)
{
	int             id;
	char            s2[64], n2[64];
	qboolean        fIsNumber = qtrue;

	// See if its a number or string
	for(id = 0; id < (int)strlen(s) && s[id] != 0; id++)	// CHRUKER: b075 - Added the (int) type casting
	{
		if(s[id] < '0' || s[id] > '9')
		{
			fIsNumber = qfalse;
			break;
		}
	}

	// numeric values are just slot numbers
	if(fIsNumber)
	{
		id = atoi(s);
		if(id >= 0 && id < cgs.maxclients && cgs.clientinfo[id].infoValid)
		{
			return (id);
		}
	}

	// check for a name match
	BG_cleanName(s, s2, sizeof(s2), qfalse);
	for(id = 0; id < cgs.maxclients; id++)
	{
		if(!cgs.clientinfo[id].infoValid)
		{
			continue;
		}

		BG_cleanName(cgs.clientinfo[id].name, n2, sizeof(n2), qfalse);
		if(!Q_stricmp(n2, s2))
		{
			return (id);
		}
	}

	CG_Printf("[cgnotify]%s ^3%s^7 %s.\n", CG_TranslateString("User"), s, CG_TranslateString("is not on the server"));
	return (-1);
}

void CG_printConsoleString(char *str)
{
	CG_Printf("[skipnotify]%s", str);
}

void CG_LoadObjectiveData(void)
{
	pc_token_t      token, token2;
	int             handle;

	if(cg_gameType.integer == GT_WOLF_LMS)
	{
		handle = trap_PC_LoadSource(va("maps/%s_lms.objdata", Q_strlwr(cgs.rawmapname)));
	}
	else
	{
		handle = trap_PC_LoadSource(va("maps/%s.objdata", Q_strlwr(cgs.rawmapname)));
	}

	if(!handle)
	{
		return;
	}

	while(1)
	{
		if(!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		if(!Q_stricmp(token.string, "wm_mapdescription"))
		{
			if(!trap_PC_ReadToken(handle, &token))
			{
				CG_Printf("^1ERROR: bad objdata line : team parameter required\n");
				break;
			}

			if(!trap_PC_ReadToken(handle, &token2))
			{
				CG_Printf("^1ERROR: bad objdata line : description parameter required\n");
				break;
			}

			if(!Q_stricmp(token.string, "axis"))
			{
				Q_strncpyz(cg.objMapDescription_Axis, token2.string, sizeof(cg.objMapDescription_Axis));
			}
			else if(!Q_stricmp(token.string, "allied"))
			{
				Q_strncpyz(cg.objMapDescription_Allied, token2.string, sizeof(cg.objMapDescription_Allied));
			}
			else if(!Q_stricmp(token.string, "neutral"))
			{
				Q_strncpyz(cg.objMapDescription_Neutral, token2.string, sizeof(cg.objMapDescription_Neutral));
			}
		}
		else if(!Q_stricmp(token.string, "wm_objective_axis_desc"))
		{
			int             i;

			if(!PC_Int_Parse(handle, &i))
			{
				CG_Printf("^1ERROR: bad objdata line : number parameter required\n");
				break;
			}

			if(!trap_PC_ReadToken(handle, &token))
			{
				CG_Printf("^1ERROR: bad objdata line :  description parameter required\n");
				break;
			}

			i--;

			if(i < 0 || i >= MAX_OBJECTIVES)
			{
				CG_Printf("^1ERROR: bad objdata line : invalid objective number\n");
				break;
			}

			Q_strncpyz(cg.objDescription_Axis[i], token.string, sizeof(cg.objDescription_Axis[i]));
		}
		else if(!Q_stricmp(token.string, "wm_objective_allied_desc"))
		{
			int             i;

			if(!PC_Int_Parse(handle, &i))
			{
				CG_Printf("^1ERROR: bad objdata line : number parameter required\n");
				break;
			}

			if(!trap_PC_ReadToken(handle, &token))
			{
				CG_Printf("^1ERROR: bad objdata line :  description parameter required\n");
				break;
			}

			i--;

			if(i < 0 || i >= MAX_OBJECTIVES)
			{
				CG_Printf("^1ERROR: bad objdata line : invalid objective number\n");
				break;
			}

			Q_strncpyz(cg.objDescription_Allied[i], token.string, sizeof(cg.objDescription_Allied[i]));
		}
	}

	trap_PC_FreeSource(handle);
}

//========================================================================
void CG_SetupDlightstyles(void)
{
	int             i, j;
	char           *str;
	char           *token;
	int             entnum;
	centity_t      *cent;

	cg.lightstylesInited = qtrue;

	for(i = 1; i < MAX_DLIGHT_CONFIGSTRINGS; i++)
	{
		str = (char *)CG_ConfigString(CS_DLIGHTS + i);
		if(!strlen(str))
		{
			break;
		}

		token = COM_Parse(&str);	// ent num
		entnum = atoi(token);
		cent = &cg_entities[entnum];

		token = COM_Parse(&str);	// stylestring
		Q_strncpyz(cent->dl_stylestring, token, strlen(token));

		token = COM_Parse(&str);	// offset
		cent->dl_frame = atoi(token);
		cent->dl_oldframe = cent->dl_frame - 1;
		if(cent->dl_oldframe < 0)
		{
			cent->dl_oldframe = strlen(cent->dl_stylestring);
		}

		token = COM_Parse(&str);	// sound id
		cent->dl_sound = atoi(token);

		token = COM_Parse(&str);	// attenuation
		cent->dl_atten = atoi(token);

		for(j = 0; j < (int)strlen(cent->dl_stylestring); j++) // CHRUKER: b075 - Added the (int) type casting
		{

			cent->dl_stylestring[j] += cent->dl_atten;	// adjust character for attenuation/amplification

			// clamp result
			if(cent->dl_stylestring[j] < 'a')
			{
				cent->dl_stylestring[j] = 'a';
			}
			if(cent->dl_stylestring[j] > 'z')
			{
				cent->dl_stylestring[j] = 'z';
			}
		}

		cent->dl_backlerp = 0.0;
		cent->dl_time = cg.time;
	}

}

//========================================================================

/*
=================
CG_RegisterItemSounds

The server says this item is used on this level
=================
*/
static void CG_RegisterItemSounds(int itemNum)
{
	gitem_t        *item;
	char            data[MAX_QPATH];
	char           *s, *start;
	int             len;

	item = &bg_itemlist[itemNum];

	if(item->pickup_sound && *item->pickup_sound)
	{
		trap_S_RegisterSound(item->pickup_sound, qfalse);
	}

	// parse the space seperated precache string for other media
	s = item->sounds;
	if(!s || !s[0])
	{
		return;
	}

	while(*s)
	{
		start = s;
		while(*s && *s != ' ')
		{
			s++;
		}

		len = s - start;
		if(len >= MAX_QPATH || len < 5)
		{
			CG_Error("PrecacheItem: %s has bad precache string", item->classname);
			return;
		}
		memcpy(data, start, len);
		data[len] = 0;
		if(*s)
		{
			s++;
		}

		if(!strcmp(data + len - 3, "wav"))
		{
			trap_S_RegisterSound(data, qfalse);
		}
	}
}


/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds(void)
{
	int             i;
	char            name[MAX_QPATH];
	const char     *soundName;
	bg_speaker_t   *speaker;

	// NERVE - SMF - voice commands
	CG_LoadVoiceChats();

	// Ridah, init sound scripts
	CG_SoundInit();
	// done.

	BG_ClearScriptSpeakerPool();

	BG_LoadSpeakerScript(va("sound/maps/%s.sps", cgs.rawmapname));

	for(i = 0; i < BG_NumScriptSpeakers(); i++)
	{
		speaker = BG_GetScriptSpeaker(i);

		speaker->noise = trap_S_RegisterSound(speaker->filename, qfalse);
	}

	cgs.media.noAmmoSound = trap_S_RegisterSound("sound/weapons/misc/fire_dry.wav", qfalse);
	cgs.media.noFireUnderwater = trap_S_RegisterSound("sound/weapons/misc/fire_water.wav", qfalse);
	cgs.media.selectSound = trap_S_RegisterSound("sound/weapons/misc/change.wav", qfalse);
	cgs.media.landHurt = trap_S_RegisterSound("sound/player/land_hurt.wav", qfalse);
	cgs.media.gibSound = trap_S_RegisterSound("sound/player/gib.wav", qfalse);
	cgs.media.dynamitebounce1 = trap_S_RegisterSound("sound/weapons/dynamite/dynamite_bounce.wav", qfalse);
	cgs.media.satchelbounce1 = trap_S_RegisterSound("sound/weapons/satchel/satchel_bounce.wav", qfalse);
	cgs.media.landminebounce1 = trap_S_RegisterSound("sound/weapons/landmine/mine_bounce.wav", qfalse);

	cgs.media.watrInSound = trap_S_RegisterSound("sound/player/water_in.wav", qfalse);
	cgs.media.watrOutSound = trap_S_RegisterSound("sound/player/water_out.wav", qfalse);
	cgs.media.watrUnSound = trap_S_RegisterSound("sound/player/water_un.wav", qfalse);
	cgs.media.watrGaspSound = trap_S_RegisterSound("sound/player/gasp.wav", qfalse);
	cgs.media.underWaterSound = trap_S_RegisterSound("sound/player/underwater.wav", qfalse);

	for(i = 0; i < 2; i++)
	{
		cgs.media.grenadebounce[FOOTSTEP_NORMAL][i] =
			cgs.media.grenadebounce[FOOTSTEP_GRAVEL][i] =
			cgs.media.grenadebounce[FOOTSTEP_SPLASH][i] =
			trap_S_RegisterSound(va("sound/weapons/grenade/bounce_hard%i.wav", i + 1), qfalse);

		cgs.media.grenadebounce[FOOTSTEP_METAL][i] =
			cgs.media.grenadebounce[FOOTSTEP_ROOF][i] =
			trap_S_RegisterSound(va("sound/weapons/grenade/bounce_metal%i.wav", i + 1), qfalse);

		cgs.media.grenadebounce[FOOTSTEP_WOOD][i] =
			trap_S_RegisterSound(va("sound/weapons/grenade/bounce_wood%i.wav", i + 1), qfalse);

		cgs.media.grenadebounce[FOOTSTEP_GRASS][i] =
			cgs.media.grenadebounce[FOOTSTEP_SNOW][i] =
			cgs.media.grenadebounce[FOOTSTEP_CARPET][i] =
			trap_S_RegisterSound(va("sound/weapons/grenade/bounce_soft%i.wav", i + 1), qfalse);

	}

	cgs.media.landSound[FOOTSTEP_NORMAL] = trap_S_RegisterSound("sound/player/footsteps/stone_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_SPLASH] = trap_S_RegisterSound("sound/player/footsteps/water_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_METAL] = trap_S_RegisterSound("sound/player/footsteps/metal_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_WOOD] = trap_S_RegisterSound("sound/player/footsteps/wood_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_GRASS] = trap_S_RegisterSound("sound/player/footsteps/grass_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_GRAVEL] = trap_S_RegisterSound("sound/player/footsteps/gravel_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_ROOF] = trap_S_RegisterSound("sound/player/footsteps/roof_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_SNOW] = trap_S_RegisterSound("sound/player/footsteps/snow_jump.wav", qfalse);
	cgs.media.landSound[FOOTSTEP_CARPET] = trap_S_RegisterSound("sound/player/footsteps/carpet_jump.wav", qfalse);

	for(i = 0; i < 4; i++)
	{
		Com_sprintf(name, sizeof(name), "sound/player/footsteps/stone%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_NORMAL][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/water%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SPLASH][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/metal%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_METAL][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/wood%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_WOOD][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/grass%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRASS][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/gravel%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_GRAVEL][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/roof%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_ROOF][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/snow%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_SNOW][i] = trap_S_RegisterSound(name, qfalse);

		Com_sprintf(name, sizeof(name), "sound/player/footsteps/carpet%i.wav", i + 1);
		cgs.media.footsteps[FOOTSTEP_CARPET][i] = trap_S_RegisterSound(name, qfalse);
	}

	for(i = 1; i < bg_numItems; i++)
	{
		CG_RegisterItemSounds(i);
	}

	for(i = 1; i < MAX_SOUNDS; i++)
	{
		soundName = CG_ConfigString(CS_SOUNDS + i);
		if(!soundName[0])
		{
			break;
		}
		if(soundName[0] == '*')
		{
			continue;			// custom sound
		}

		// Ridah, register sound scripts seperately
		if(!strstr(soundName, ".wav"))
		{
			CG_SoundScriptPrecache(soundName);
		}
		else
		{
			cgs.gameSounds[i] = trap_S_RegisterSound(soundName, qfalse);	// FIXME: allow option to compress?
		}
	}

/*
	// OSP
	cgs.media.countFight = trap_S_RegisterSound( "sound/osp/fight.wav" );
	cgs.media.countPrepare = trap_S_RegisterSound( "sound/osp/prepare.wav" );
	cgs.media.goatAxis = trap_S_RegisterSound( "sound/osp/goat.wav" );
	cgs.media.winAllies = trap_S_RegisterSound( "sound/osp/winallies.wav" );
	cgs.media.winAxis = trap_S_RegisterSound( "sound/osp/winaxis.wav" );
	// OSP
*/

	cgs.media.flameSound = trap_S_RegisterSound("sound/weapons/flamethrower/flame_burn.wav", qfalse);
	cgs.media.flameBlowSound = trap_S_RegisterSound("sound/weapons/flamethrower/flame_pilot.wav", qfalse);
	cgs.media.flameStartSound = trap_S_RegisterSound("sound/weapons/flamethrower/flame_up.wav", qfalse);
	cgs.media.flameStreamSound = trap_S_RegisterSound("sound/weapons/flamethrower/flame_fire.wav", qfalse);
	cgs.media.flameCrackSound = 0;	// -trap_S_RegisterSound( "sound/world/firecrack1.wav", qfalse );
	cgs.media.grenadePulseSound4 = trap_S_RegisterSound("sound/weapons/grenade/gren_timer4.wav", qfalse);
	cgs.media.grenadePulseSound3 = trap_S_RegisterSound("sound/weapons/grenade/gren_timer3.wav", qfalse);
	cgs.media.grenadePulseSound2 = trap_S_RegisterSound("sound/weapons/grenade/gren_timer2.wav", qfalse);
	cgs.media.grenadePulseSound1 = trap_S_RegisterSound("sound/weapons/grenade/gren_timer1.wav", qfalse);


	cgs.media.boneBounceSound = trap_S_RegisterSound("sound/world/boardbreak.wav", qfalse);	// TODO: need a real sound for this

	cgs.media.sfx_rockexp = trap_S_RegisterSound("sound/weapons/rocket/rocket_expl.wav", qfalse);
	cgs.media.sfx_rockexpDist = trap_S_RegisterSound("sound/weapons/rocket/rocket_expl_far.wav", qfalse);

	cgs.media.sfx_artilleryExp[0] = trap_S_RegisterSound("sound/weapons/artillery/artillery_expl_1.wav", qfalse);
	cgs.media.sfx_artilleryExp[1] = trap_S_RegisterSound("sound/weapons/artillery/artillery_expl_2.wav", qfalse);
	cgs.media.sfx_artilleryExp[2] = trap_S_RegisterSound("sound/weapons/artillery/artillery_expl_3.wav", qfalse);
	cgs.media.sfx_artilleryDist = trap_S_RegisterSound("sound/weapons/artillery/artillery_expl_far.wav", qfalse);

	cgs.media.sfx_airstrikeExp[0] = trap_S_RegisterSound("sound/weapons/airstrike/airstrike_expl_1.wav", qfalse);
	cgs.media.sfx_airstrikeExp[1] = trap_S_RegisterSound("sound/weapons/airstrike/airstrike_expl_2.wav", qfalse);
	cgs.media.sfx_airstrikeExp[2] = trap_S_RegisterSound("sound/weapons/airstrike/airstrike_expl_3.wav", qfalse);
	cgs.media.sfx_airstrikeDist = trap_S_RegisterSound("sound/weapons/airstrike/airstrike_expl_far.wav", qfalse);

	cgs.media.sfx_dynamiteexp = trap_S_RegisterSound("sound/weapons/dynamite/dynamite_expl.wav", qfalse);
	cgs.media.sfx_dynamiteexpDist = trap_S_RegisterSound("sound/weapons/dynamite/dynamite_expl_far.wav", qfalse);

	cgs.media.sfx_satchelexp = trap_S_RegisterSound("sound/weapons/satchel/satchel_expl.wav", qfalse);
	cgs.media.sfx_satchelexpDist = trap_S_RegisterSound("sound/weapons/satchel/satchel_expl_far.wav", qfalse);
	cgs.media.sfx_landmineexp = trap_S_RegisterSound("sound/weapons/landmine/mine_expl.wav", qfalse);
	cgs.media.sfx_landmineexpDist = trap_S_RegisterSound("sound/weapons/landmine/mine_expl_far.wav", qfalse);
	cgs.media.sfx_mortarexp[0] = trap_S_RegisterSound("sound/weapons/mortar/mortar_expl1.wav", qfalse);
	cgs.media.sfx_mortarexp[1] = trap_S_RegisterSound("sound/weapons/mortar/mortar_expl2.wav", qfalse);
	cgs.media.sfx_mortarexp[2] = trap_S_RegisterSound("sound/weapons/mortar/mortar_expl3.wav", qfalse);
	cgs.media.sfx_mortarexp[3] = trap_S_RegisterSound("sound/weapons/mortar/mortar_expl.wav", qfalse);
	cgs.media.sfx_mortarexpDist = trap_S_RegisterSound("sound/weapons/mortar/mortar_expl_far.wav", qfalse);
	cgs.media.sfx_grenexp = trap_S_RegisterSound("sound/weapons/grenade/gren_expl.wav", qfalse);
	cgs.media.sfx_grenexpDist = trap_S_RegisterSound("sound/weapons/grenade/gren_expl_far.wav", qfalse);
	cgs.media.sfx_rockexpWater = trap_S_RegisterSound("sound/weapons/grenade/gren_expl_water.wav", qfalse);


	for(i = 0; i < 3; i++)
	{
		// Gordon: bouncy shell sounds \o/
		cgs.media.sfx_brassSound[BRASSSOUND_METAL][i] =
			trap_S_RegisterSound(va("sound/weapons/misc/shell_metal%i.wav", i + 1), qfalse);
		cgs.media.sfx_brassSound[BRASSSOUND_SOFT][i] =
			trap_S_RegisterSound(va("sound/weapons/misc/shell_soft%i.wav", i + 1), qfalse);
		cgs.media.sfx_brassSound[BRASSSOUND_STONE][i] =
			trap_S_RegisterSound(va("sound/weapons/misc/shell_stone%i.wav", i + 1), qfalse);
		cgs.media.sfx_brassSound[BRASSSOUND_WOOD][i] =
			trap_S_RegisterSound(va("sound/weapons/misc/shell_wood%i.wav", i + 1), qfalse);
		cgs.media.sfx_rubbleBounce[i] = trap_S_RegisterSound(va("sound/world/debris%i.wav", i + 1), qfalse);
	}
	cgs.media.sfx_knifehit[0] = trap_S_RegisterSound("sound/weapons/knife/knife_hit1.wav", qfalse);
	cgs.media.sfx_knifehit[1] = trap_S_RegisterSound("sound/weapons/knife/knife_hit2.wav", qfalse);
	cgs.media.sfx_knifehit[2] = trap_S_RegisterSound("sound/weapons/knife/knife_hit3.wav", qfalse);
	cgs.media.sfx_knifehit[3] = trap_S_RegisterSound("sound/weapons/knife/knife_hit4.wav", qfalse);
	cgs.media.sfx_knifehit[4] = trap_S_RegisterSound("sound/weapons/knife/knife_hitwall1.wav", qfalse);

	for(i = 0; i < 5; i++)
	{
		cgs.media.sfx_bullet_fleshhit[i] = trap_S_RegisterSound(va("sound/weapons/impact/flesh%i.wav", i + 1), qfalse);
		cgs.media.sfx_bullet_metalhit[i] = trap_S_RegisterSound(va("sound/weapons/impact/metal%i.wav", i + 1), qfalse);
		cgs.media.sfx_bullet_woodhit[i] = trap_S_RegisterSound(va("sound/weapons/impact/wood%i.wav", i + 1), qfalse);
		cgs.media.sfx_bullet_glasshit[i] = trap_S_RegisterSound(va("sound/weapons/impact/glass%i.wav", i + 1), qfalse);
		cgs.media.sfx_bullet_stonehit[i] = trap_S_RegisterSound(va("sound/weapons/impact/stone%i.wav", i + 1), qfalse);
		cgs.media.sfx_bullet_waterhit[i] = trap_S_RegisterSound(va("sound/weapons/impact/water%i.wav", i + 1), qfalse);
	}

	cgs.media.uniformPickup = trap_S_RegisterSound("sound/misc/body_pickup.wav", qfalse);
	cgs.media.buildDecayedSound = trap_S_RegisterSound("sound/world/build_abort.wav", qfalse);

	cgs.media.sndLimboSelect = trap_S_RegisterSound("sound/menu/select.wav", qfalse);
	cgs.media.sndLimboFocus = trap_S_RegisterSound("sound/menu/focus.wav", qfalse);
	cgs.media.sndLimboFilter = trap_S_RegisterSound("sound/menu/filter.wav", qfalse);
	cgs.media.sndLimboCancel = trap_S_RegisterSound("sound/menu/cancel.wav", qfalse);

	cgs.media.sndRankUp = trap_S_RegisterSound("sound/misc/rank_up.wav", qfalse);
	cgs.media.sndSkillUp = trap_S_RegisterSound("sound/misc/skill_up.wav", qfalse);

	cgs.media.sndMedicCall[0] = trap_S_RegisterSound("sound/chat/axis/medic.wav", qfalse);
	cgs.media.sndMedicCall[1] = trap_S_RegisterSound("sound/chat/allies/medic.wav", qfalse);


	// FIXME: send as a special event
	trap_S_RegisterSound("sound/weapons/artillery/artillery_fly_1.wav", qfalse);
	trap_S_RegisterSound("sound/weapons/artillery/artillery_fly_2.wav", qfalse);
	trap_S_RegisterSound("sound/weapons/artillery/artillery_fly_3.wav", qfalse);
	trap_S_RegisterSound("sound/weapons/airstrike/airstrike_plane.wav", qfalse);


	if(cg_buildScript.integer)
	{
		CG_PrecacheFXSounds();
	}
}


//===================================================================================

/*
=================
CG_RegisterGraphics

This function may execute for a couple of minutes with a slow disk.
=================
*/

qboolean        CG_RegisterClientSkin(bg_playerclass_t * classInfo);
qboolean        CG_RegisterClientModelname(bg_playerclass_t * classInfo);
void            WM_RegisterWeaponTypeShaders();

static void CG_RegisterGraphics(void)
{
	char            name[1024];
	int             i;
	static char    *sb_nums[11] = {
		"gfx/2d/numbers/zero_32b",
		"gfx/2d/numbers/one_32b",
		"gfx/2d/numbers/two_32b",
		"gfx/2d/numbers/three_32b",
		"gfx/2d/numbers/four_32b",
		"gfx/2d/numbers/five_32b",
		"gfx/2d/numbers/six_32b",
		"gfx/2d/numbers/seven_32b",
		"gfx/2d/numbers/eight_32b",
		"gfx/2d/numbers/nine_32b",
		"gfx/2d/numbers/minus_32b",
	};

	CG_LoadingString(cgs.mapname);

	trap_R_LoadWorldMap(cgs.mapname);

	CG_LoadingString("entities");

	numSplinePaths = 0;
	numPathCorners = 0;

	cg.numOIDtriggers2 = 0;

	BG_ClearAnimationPool();

	BG_ClearCharacterPool();

	BG_InitWeaponStrings();

	CG_ParseEntitiesFromString();

	CG_LoadObjectiveData();

	// precache status bar pics
	CG_LoadingString("game media");

	CG_LoadingString(" - textures");

//bani - dynamic shader api example
//replaces a fueldump texture with a dynamically generated one.
#ifdef TEST_API_DYNAMICSHADER
	trap_R_LoadDynamicShader("my_terrain1_2", "my_terrain1_2\n\
{\n\
	qer_editorimage textures/stone/mxsnow3.tga\n\
	q3map_baseshader textures/fueldump/terrain_base\n\
	{\n\
		map textures/stone/mxrock1aa.tga\n\
		rgbGen identity\n\
		tcgen environment\n\
	}\n\
	{\n\
		lightmap $lightmap\n\
		blendFunc GL_DST_COLOR GL_ZERO\n\
		rgbgen identity\n\
	}\n\
}\n\
");

	trap_R_RegisterShader("my_terrain1_2");
	trap_R_RemapShader("textures/fueldump/terrain1_2", "my_terrain1_2", "0");
#endif

	for(i = 0; i < 11; i++)
	{
		cgs.media.numberShaders[i] = trap_R_RegisterShader(sb_nums[i]);
	}

	cgs.media.fleshSmokePuffShader = trap_R_RegisterShader("fleshimpactsmokepuff");	// JPW NERVE
	cgs.media.nerveTestShader = trap_R_RegisterShader("jpwtest1");
	cgs.media.idTestShader = trap_R_RegisterShader("jpwtest2");
	cgs.media.hud1Shader = trap_R_RegisterShader("jpwhud1");
	cgs.media.hud2Shader = trap_R_RegisterShader("jpwhud2");
	cgs.media.hud3Shader = trap_R_RegisterShader("jpwhud3");
	cgs.media.hud4Shader = trap_R_RegisterShader("jpwhud4");
	cgs.media.hud5Shader = trap_R_RegisterShader("jpwhud5");
	cgs.media.smokePuffShader = trap_R_RegisterShader("smokePuff");

	// RF, blood cloud
	cgs.media.bloodCloudShader = trap_R_RegisterShader("bloodCloud");

	// OSP - MV cursor
//  cgs.media.cursor = trap_R_RegisterShaderNoMip( "ui/assets/mvcursor.tga" );

	// Rafael - cannon
	cgs.media.smokePuffShaderdirty = trap_R_RegisterShader("smokePuffdirty");
	cgs.media.smokePuffShaderb1 = trap_R_RegisterShader("smokePuffblack1");
	cgs.media.smokePuffShaderb2 = trap_R_RegisterShader("smokePuffblack2");
	cgs.media.smokePuffShaderb3 = trap_R_RegisterShader("smokePuffblack3");
	cgs.media.smokePuffShaderb4 = trap_R_RegisterShader("smokePuffblack4");
	cgs.media.smokePuffShaderb5 = trap_R_RegisterShader("smokePuffblack5");
	// done

	// Rafael - bleedanim
	for(i = 0; i < 5; i++)
	{
		cgs.media.viewBloodAni[i] = trap_R_RegisterShader(va("viewBloodBlend%i", i + 1));
	}

	cgs.media.viewFlashBlood = trap_R_RegisterShader("viewFlashBlood");
	for(i = 0; i < 16; i++)
	{
		cgs.media.viewFlashFire[i] = trap_R_RegisterShader(va("viewFlashFire%i", i + 1));
	}

	cgs.media.smokePuffRageProShader = trap_R_RegisterShader("smokePuffRagePro");
	cgs.media.shotgunSmokePuffShader = trap_R_RegisterShader("shotgunSmokePuff");
	cgs.media.bloodTrailShader = trap_R_RegisterShader("bloodTrail");
	cgs.media.lagometerShader = trap_R_RegisterShader("lagometer");
	cgs.media.reticleShaderSimple = trap_R_RegisterShader("gfx/misc/reticlesimple");
	cgs.media.binocShaderSimple = trap_R_RegisterShader("gfx/misc/binocsimple");
	cgs.media.snowShader = trap_R_RegisterShader("snow_tri");
	cgs.media.oilParticle = trap_R_RegisterShader("oilParticle");
	cgs.media.oilSlick = trap_R_RegisterShader("oilSlick");
	cgs.media.waterBubbleShader = trap_R_RegisterShader("waterBubble");
	cgs.media.tracerShader = trap_R_RegisterShader("gfx/misc/tracer");
	cgs.media.usableHintShader = trap_R_RegisterShader("gfx/2d/usableHint");
	cgs.media.notUsableHintShader = trap_R_RegisterShader("gfx/2d/notUsableHint");
	cgs.media.doorHintShader = trap_R_RegisterShader("gfx/2d/doorHint");
	cgs.media.doorRotateHintShader = trap_R_RegisterShader("gfx/2d/doorRotateHint");

	// Arnout: these were never used in default wolf
	cgs.media.doorLockHintShader = trap_R_RegisterShader("gfx/2d/lockedhint");
	cgs.media.doorRotateLockHintShader = trap_R_RegisterShader("gfx/2d/lockedhint");
	cgs.media.mg42HintShader = trap_R_RegisterShader("gfx/2d/mg42Hint");
	cgs.media.breakableHintShader = trap_R_RegisterShader("gfx/2d/breakableHint");
	cgs.media.chairHintShader = trap_R_RegisterShader("gfx/2d/chairHint");
	cgs.media.alarmHintShader = trap_R_RegisterShader("gfx/2d/alarmHint");
	cgs.media.healthHintShader = trap_R_RegisterShader("gfx/2d/healthHint");
	cgs.media.treasureHintShader = trap_R_RegisterShader("gfx/2d/treasureHint");
	cgs.media.knifeHintShader = trap_R_RegisterShader("gfx/2d/knifeHint");
	cgs.media.ladderHintShader = trap_R_RegisterShader("gfx/2d/ladderHint");
	cgs.media.buttonHintShader = trap_R_RegisterShader("gfx/2d/buttonHint");
	cgs.media.waterHintShader = trap_R_RegisterShader("gfx/2d/waterHint");
	cgs.media.cautionHintShader = trap_R_RegisterShader("gfx/2d/cautionHint");
	cgs.media.dangerHintShader = trap_R_RegisterShader("gfx/2d/dangerHint");
	cgs.media.secretHintShader = trap_R_RegisterShader("gfx/2d/secretHint");
	cgs.media.qeustionHintShader = trap_R_RegisterShader("gfx/2d/questionHint");
	cgs.media.exclamationHintShader = trap_R_RegisterShader("gfx/2d/exclamationHint");
	cgs.media.clipboardHintShader = trap_R_RegisterShader("gfx/2d/clipboardHint");
	cgs.media.weaponHintShader = trap_R_RegisterShader("gfx/2d/weaponHint");
	cgs.media.ammoHintShader = trap_R_RegisterShader("gfx/2d/ammoHint");
	cgs.media.armorHintShader = trap_R_RegisterShader("gfx/2d/armorHint");
	cgs.media.powerupHintShader = trap_R_RegisterShader("gfx/2d/powerupHint");
	cgs.media.holdableHintShader = trap_R_RegisterShader("gfx/2d/holdableHint");
	cgs.media.inventoryHintShader = trap_R_RegisterShader("gfx/2d/inventoryHint");

	cgs.media.friendShader = trap_R_RegisterShaderNoMip("gfx/2d/friendlycross.tga");

	// (SA) not used yet
//  cgs.media.hintPlrFriendShader   = trap_R_RegisterShader( "gfx/2d/hintPlrFriend" );
//  cgs.media.hintPlrNeutralShader  = trap_R_RegisterShader( "gfx/2d/hintPlrNeutral" );
//  cgs.media.hintPlrEnemyShader    = trap_R_RegisterShader( "gfx/2d/hintPlrEnemy" );
//  cgs.media.hintPlrUnknownShader  = trap_R_RegisterShader( "gfx/2d/hintPlrUnknown" );

	cgs.media.buildHintShader = trap_R_RegisterShader("gfx/2d/buildHint");	// DHM - Nerve
	cgs.media.disarmHintShader = trap_R_RegisterShader("gfx/2d/disarmHint");	// DHM - Nerve
	cgs.media.reviveHintShader = trap_R_RegisterShader("gfx/2d/reviveHint");	// DHM - Nerve
	cgs.media.dynamiteHintShader = trap_R_RegisterShader("gfx/2d/dynamiteHint");	// DHM - Nerve

	cgs.media.tankHintShader = trap_R_RegisterShaderNoMip("gfx/2d/tankHint");
	cgs.media.satchelchargeHintShader = trap_R_RegisterShaderNoMip("gfx/2d/satchelchargeHint"),
		cgs.media.landmineHintShader = trap_R_RegisterShaderNoMip("gfx/2d/landmineHint");
	cgs.media.uniformHintShader = trap_R_RegisterShaderNoMip("gfx/2d/uniformHint");
	cgs.media.waypointAttackShader = trap_R_RegisterShaderNoMip("sprites/waypoint_attack");
	cgs.media.waypointDefendShader = trap_R_RegisterShaderNoMip("sprites/waypoint_defend");
	cgs.media.waypointRegroupShader = trap_R_RegisterShaderNoMip("sprites/waypoint_regroup");
	// TAT - load up the bot shader as well
//  cgs.media.waypointBotShader     = trap_R_RegisterShaderNoMip( "sprites/botorder" );
//  cgs.media.waypointBotQueuedShader=trap_R_RegisterShaderNoMip( "sprites/botqueuedorder" );
//  cgs.media.waypointCompassAttackShader   = trap_R_RegisterShaderNoMip( "sprites/waypoint_attack_compass" );
//  cgs.media.waypointCompassDefendShader   = trap_R_RegisterShaderNoMip( "sprites/waypoint_defend_compass" );
//  cgs.media.waypointCompassRegroupShader  = trap_R_RegisterShaderNoMip( "sprites/waypoint_regroup_compass" );
//  cgs.media.commandCentreWoodShader       = trap_R_RegisterShaderNoMip( "ui/assets2/commandMap" );
	if(cgs.ccLayers)
	{
		for(i = 0; i < cgs.ccLayers; i++)
		{
			cgs.media.commandCentreMapShader[i] = trap_R_RegisterShaderNoMip(va("levelshots/%s_%i_cc.tga", cgs.rawmapname, i));
			cgs.media.commandCentreMapShaderTrans[i] =
				trap_R_RegisterShaderNoMip(va("levelshots/%s_%i_cc_trans", cgs.rawmapname, i));
			cgs.media.commandCentreAutomapShader[i] =
				trap_R_RegisterShaderNoMip(va("levelshots/%s_%i_cc_automap", cgs.rawmapname, i));
		}
	}
	else
	{
		cgs.media.commandCentreMapShader[0] = trap_R_RegisterShaderNoMip(va("levelshots/%s_cc.tga", cgs.rawmapname));
		cgs.media.commandCentreMapShaderTrans[0] = trap_R_RegisterShaderNoMip(va("levelshots/%s_cc_trans", cgs.rawmapname));
		cgs.media.commandCentreAutomapShader[0] = trap_R_RegisterShaderNoMip(va("levelshots/%s_cc_automap", cgs.rawmapname));
	}
	cgs.media.commandCentreAutomapMaskShader = trap_R_RegisterShaderNoMip("levelshots/automap_mask");
	cgs.media.commandCentreAutomapBorderShader = trap_R_RegisterShaderNoMip("ui/assets2/maptrim_long");
	cgs.media.commandCentreAutomapBorder2Shader = trap_R_RegisterShaderNoMip("ui/assets2/maptrim_long2");
	cgs.media.commandCentreAutomapCornerShader = trap_R_RegisterShaderNoMip("ui/assets2/maptrim_edge.tga");
	cgs.media.commandCentreAxisMineShader = trap_R_RegisterShaderNoMip("sprites/landmine_axis");
	cgs.media.commandCentreAlliedMineShader = trap_R_RegisterShaderNoMip("sprites/landmine_allied");
	cgs.media.commandCentreSpawnShader[0] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_flagaxis");
	cgs.media.commandCentreSpawnShader[1] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_flagallied");
	cgs.media.compassConstructShader = trap_R_RegisterShaderNoMip("sprites/construct.tga");

	// Mad Doc - TDF
	//cgs.media.ingameAutomapBackground = trap_R_RegisterShaderNoMip("ui/assets2/ingame/mapbackground");

	//cgs.media.hudBorderVert = trap_R_RegisterShaderNoMip( "ui/assets2/border_vert.tga" );
	//cgs.media.hudBorderVert2 = trap_R_RegisterShaderNoMip( "ui/assets2/border_vert2.tga" );

	cgs.media.compassDestroyShader = trap_R_RegisterShaderNoMip("sprites/destroy.tga");
	cgs.media.slashShader = trap_R_RegisterShaderNoMip("gfx/2d/numbers/slash");
	cgs.media.compass2Shader = trap_R_RegisterShaderNoMip("gfx/2d/compass2.tga");
	cgs.media.compassShader = trap_R_RegisterShaderNoMip("gfx/2d/compass.tga");
	cgs.media.buddyShader = trap_R_RegisterShaderNoMip("sprites/buddy.tga");

	for(i = 0; i < NUM_CROSSHAIRS; i++)
	{
		cgs.media.crosshairShader[i] = trap_R_RegisterShader(va("gfx/2d/crosshair%c", 'a' + i));
		cg.crosshairShaderAlt[i] = trap_R_RegisterShader(va("gfx/2d/crosshair%c_alt", 'a' + i));
	}

	for(i = 0; i < SK_NUM_SKILLS; i++)
	{
		cgs.media.medals[i] = trap_R_RegisterShaderNoMip(va("gfx/limbo/medals0%i", i));
	}

	cgs.media.backTileShader = trap_R_RegisterShader("gfx/2d/backtile");
	cgs.media.noammoShader = trap_R_RegisterShader("icons/noammo");

	cgs.media.teamStatusBar = trap_R_RegisterShader("gfx/2d/colorbar.tga");

	//cgs.media.redColorBar =       trap_R_RegisterShader("redcolorbar");
	//cgs.media.blueColorBar =  trap_R_RegisterShader("bluecolorbar");
	cgs.media.hudSprintBar = trap_R_RegisterShader("sprintbar");

	cgs.media.hudAlliedHelmet = trap_R_RegisterShader("AlliedHelmet");
	cgs.media.hudAxisHelmet = trap_R_RegisterShader("AxisHelmet");

	CG_LoadingString(" - models");

	cgs.media.machinegunBrassModel = trap_R_RegisterModel("models/weapons2/shells/m_shell.md3");
	cgs.media.panzerfaustBrassModel = trap_R_RegisterModel("models/weapons2/shells/pf_shell.md3");

	// Rafael
	cgs.media.smallgunBrassModel = trap_R_RegisterModel("models/weapons2/shells/sm_shell.md3");

	//----(SA) wolf debris
	cgs.media.debBlock[0] = trap_R_RegisterModel("models/mapobjects/debris/brick1.md3");
	cgs.media.debBlock[1] = trap_R_RegisterModel("models/mapobjects/debris/brick2.md3");
	cgs.media.debBlock[2] = trap_R_RegisterModel("models/mapobjects/debris/brick3.md3");
	cgs.media.debBlock[3] = trap_R_RegisterModel("models/mapobjects/debris/brick4.md3");
	cgs.media.debBlock[4] = trap_R_RegisterModel("models/mapobjects/debris/brick5.md3");
	cgs.media.debBlock[5] = trap_R_RegisterModel("models/mapobjects/debris/brick6.md3");

	cgs.media.debRock[0] = trap_R_RegisterModel("models/mapobjects/debris/rubble1.md3");
	cgs.media.debRock[1] = trap_R_RegisterModel("models/mapobjects/debris/rubble2.md3");
	cgs.media.debRock[2] = trap_R_RegisterModel("models/mapobjects/debris/rubble3.md3");


	cgs.media.debWood[0] = trap_R_RegisterModel("models/gibs/wood/wood1.md3");
	cgs.media.debWood[1] = trap_R_RegisterModel("models/gibs/wood/wood2.md3");
	cgs.media.debWood[2] = trap_R_RegisterModel("models/gibs/wood/wood3.md3");
	cgs.media.debWood[3] = trap_R_RegisterModel("models/gibs/wood/wood4.md3");
	cgs.media.debWood[4] = trap_R_RegisterModel("models/gibs/wood/wood5.md3");
	cgs.media.debWood[5] = trap_R_RegisterModel("models/gibs/wood/wood6.md3");

	cgs.media.debFabric[0] = trap_R_RegisterModel("models/shards/fabric1.md3");
	cgs.media.debFabric[1] = trap_R_RegisterModel("models/shards/fabric2.md3");
	cgs.media.debFabric[2] = trap_R_RegisterModel("models/shards/fabric3.md3");

	//----(SA) end

	cgs.media.spawnInvincibleShader = trap_R_RegisterShader("sprites/shield");
	cgs.media.scoreEliminatedShader = trap_R_RegisterShader("sprites/skull");
	cgs.media.medicReviveShader = trap_R_RegisterShader("sprites/medic_revive");

	//cgs.media.vehicleShader = trap_R_RegisterShader( "sprites/vehicle" );
	cgs.media.destroyShader = trap_R_RegisterShader("sprites/destroy");

	cgs.media.voiceChatShader = trap_R_RegisterShader("sprites/voiceChat");
	cgs.media.balloonShader = trap_R_RegisterShader("sprites/balloon3");

	cgs.media.objectiveShader = trap_R_RegisterShader("sprites/objective");

	cgs.media.bloodExplosionShader = trap_R_RegisterShader("bloodExplosion");

	//cgs.media.bleedExplosionShader = trap_R_RegisterShader( "bleedExplosion" );

	//----(SA)  water splash
	cgs.media.waterSplashModel = trap_R_RegisterModel("models/weaphits/bullet.md3");
	cgs.media.waterSplashShader = trap_R_RegisterShader("waterSplash");
	//----(SA)  end

	// Ridah, spark particles
	cgs.media.sparkParticleShader = trap_R_RegisterShader("sparkParticle");
	cgs.media.smokeTrailShader = trap_R_RegisterShader("smokeTrail");
	//cgs.media.lightningBoltShader = trap_R_RegisterShader( "lightningBolt" );
	cgs.media.flamethrowerFireStream = trap_R_RegisterShader("flamethrowerFireStream");
	cgs.media.flamethrowerBlueStream = trap_R_RegisterShader("flamethrowerBlueStream");
	cgs.media.onFireShader2 = trap_R_RegisterShader("entityOnFire1");
	cgs.media.onFireShader = trap_R_RegisterShader("entityOnFire2");
	cgs.media.viewFadeBlack = trap_R_RegisterShader("viewFadeBlack");
	cgs.media.sparkFlareShader = trap_R_RegisterShader("sparkFlareParticle");
	cgs.media.spotLightShader = trap_R_RegisterShader("spotLight");
	cgs.media.spotLightBeamShader = trap_R_RegisterShader("lightBeam");
	cgs.media.bulletParticleTrailShader = trap_R_RegisterShader("bulletParticleTrail");
	cgs.media.smokeParticleShader = trap_R_RegisterShader("smokeParticle");

	// DHM - Nerve :: bullet hitting dirt
	cgs.media.dirtParticle1Shader = trap_R_RegisterShader("dirt_splash");
	cgs.media.dirtParticle2Shader = trap_R_RegisterShader("water_splash");
	//cgs.media.dirtParticle3Shader = trap_R_RegisterShader( "dirtParticle3" );

	cgs.media.genericConstructionShader = trap_R_RegisterShader("textures/sfx/construction");
	//cgs.media.genericConstructionShaderBrush =    trap_R_RegisterShader( "textures/sfx/construction" );
	//cgs.media.genericConstructionShaderModel =    trap_R_RegisterShader( "textures/sfx/construction_model" );
	cgs.media.alliedUniformShader = trap_R_RegisterShader("sprites/uniform_allied");
	cgs.media.axisUniformShader = trap_R_RegisterShader("sprites/uniform_axis");

	// used in:
	// command map
	cgs.media.ccFilterPics[0] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_axis");
	cgs.media.ccFilterPics[1] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_allied");
	cgs.media.ccFilterPics[2] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_spawn");

	cgs.media.ccFilterPics[3] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_bo");
	cgs.media.ccFilterPics[4] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_healthammo");
	cgs.media.ccFilterPics[5] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_construction");
	cgs.media.ccFilterPics[6] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_destruction");
	cgs.media.ccFilterPics[7] = trap_R_RegisterShaderNoMip("gfx/limbo/filter_objective");
	//cgs.media.ccFilterPics[7] = trap_R_RegisterShaderNoMip( "gfx/limbo/filter_waypoint" );
	//cgs.media.ccFilterPics[8] = trap_R_RegisterShaderNoMip( "gfx/limbo/filter_objective" );

	cgs.media.ccFilterBackOn = trap_R_RegisterShaderNoMip("gfx/limbo/filter_back_on");
	cgs.media.ccFilterBackOff = trap_R_RegisterShaderNoMip("gfx/limbo/filter_back_off");
/*
#define CC_FILTER_AXIS			(1 << 0)
#define CC_FILTER_ALLIES		(1 << 1)
#define CC_FILTER_SPAWNS		(1 << 2)
#define CC_FILTER_CMDPOST		(1 << 3) // TODO
#define CC_FILTER_HACABINETS	(1 << 4) // TODO
#define CC_FILTER_CONSTRUCTIONS	(1 << 5)
#define CC_FILTER_DESTRUCTIONS	(1 << 6)
#define CC_FILTER_WAYPOINTS		(1 << 7)
#define CC_FILTER_OBJECTIVES	(1 << 8) // TODO
*/

	// used in:
	//  statsranksmedals
	//  command map
	//  limbo menu
	cgs.media.ccStamps[0] = trap_R_RegisterShaderNoMip("ui/assets2/stamp_complete");
	cgs.media.ccStamps[1] = trap_R_RegisterShaderNoMip("ui/assets2/stamp_failed");

	//cgs.media.ccArrow =                   trap_R_RegisterShaderNoMip( "ui/assets2/arrow_up" );
	cgs.media.ccPlayerHighlight = trap_R_RegisterShaderNoMip("ui/assets/mp_player_highlight.tga");
	cgs.media.ccConstructIcon[0] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_constaxis");
	cgs.media.ccConstructIcon[1] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_constallied");
	cgs.media.ccDestructIcon[0][0] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_axisgren");
	cgs.media.ccDestructIcon[0][1] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_alliedgren");
	cgs.media.ccDestructIcon[1][0] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_satchel");
	cgs.media.ccDestructIcon[1][1] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_satchel");
	cgs.media.ccDestructIcon[2][0] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_dynamite");
	cgs.media.ccDestructIcon[2][1] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_dynamite");
	cgs.media.ccTankIcon = trap_R_RegisterShaderNoMip("gfx/limbo/cm_churchill");

	cgs.media.ccCmdPost[0] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_bo_axis");
	cgs.media.ccCmdPost[1] = trap_R_RegisterShaderNoMip("gfx/limbo/cm_bo_allied");

	cgs.media.ccMortarHit = trap_R_RegisterShaderNoMip("gfx/limbo/mort_hit");
	cgs.media.ccMortarTarget = trap_R_RegisterShaderNoMip("gfx/limbo/mort_target");
	cgs.media.ccMortarTargetArrow = trap_R_RegisterShaderNoMip("gfx/limbo/mort_targetarrow");


	cgs.media.skillPics[SK_BATTLE_SENSE] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_battlesense");
	cgs.media.skillPics[SK_EXPLOSIVES_AND_CONSTRUCTION] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_engineer");
	cgs.media.skillPics[SK_FIRST_AID] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_medic");
	cgs.media.skillPics[SK_SIGNALS] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_fieldops");
	cgs.media.skillPics[SK_LIGHT_WEAPONS] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_lightweap");
	cgs.media.skillPics[SK_HEAVY_WEAPONS] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_soldier");
	cgs.media.skillPics[SK_MILITARY_INTELLIGENCE_AND_SCOPED_WEAPONS] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_covertops");

	/*cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_MOVETOLOC] =  trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_default" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_CONSTRUCT] = trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_construct" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_USEDYNAMITE] =   trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_usedynamite" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_REPAIR] =        trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_fixgun" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_MOUNTGUN] =      trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_mountgun" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_OPENDOOR] =      trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_opendoor" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_REVIVE] =        trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_revive" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_GETDISGUISE] =   trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_getdisguise" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_HEAL] =          trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_heal" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_AMMO] =          trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_ammo" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_DISARM] =        trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_disarmdynamite" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_ATTACK] =        trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_attack" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_COVER] =     trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_cover" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_RECON] =     trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_recon" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_SMOKEBOMB] = trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_smoke" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_FINDMINES] = trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_findmines" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_PLANTMINE] = trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_plantmine" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_ARTILLERY] = trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_artillery" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_AIRSTRIKE] = trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_airstrike" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_GRENADELAUNCH]=  trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_grenadelaunch" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_PICKUPITEM] =    trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_pickup" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_PANZERFAUST] =   trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_panzerfaust" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_FLAMETHROW] =    trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_flamethrow" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_MG42] =          trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_mg42" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_MOUNTEDATTACK]=  trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_mountedattack" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_KNIFEATTACK] =   trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_knifeattack" );
	   cgs.media.SPTeamOverlayBotOrders[BOT_ACTION_LOCKPICK] =      trap_R_RegisterShaderNoMip( "ui/assets2/ingame/bot_action_lockpick" ); */

	WM_RegisterWeaponTypeShaders();

	CG_LoadRankIcons();

	// Gordon: limbo menu setup
	CG_LimboPanel_Init();

	CG_ChatPanel_Setup();

	CG_Fireteams_Setup();

	cgs.media.waypointMarker = trap_R_RegisterModel("models/multiplayer/flagpole/flag_waypoint.md3");

	cgs.media.railCoreShader = trap_R_RegisterShaderNoMip("railCore");	// (SA) for debugging server traces
	cgs.media.ropeShader = trap_R_RegisterShader("textures/props/cable_m01");

	cgs.media.thirdPersonBinocModel = trap_R_RegisterModel("models/multiplayer/binocs/binocs.md3");	// NERVE - SMF
	cgs.media.flamebarrel = trap_R_RegisterModel("models/furniture/barrel/barrel_a.md3");
	cgs.media.mg42muzzleflash = trap_R_RegisterModel("models/weapons2/machinegun/mg42_flash.md3");

	// Rafael shards
	cgs.media.shardGlass1 = trap_R_RegisterModel("models/shards/glass1.md3");
	cgs.media.shardGlass2 = trap_R_RegisterModel("models/shards/glass2.md3");
	cgs.media.shardWood1 = trap_R_RegisterModel("models/shards/wood1.md3");
	cgs.media.shardWood2 = trap_R_RegisterModel("models/shards/wood2.md3");
	cgs.media.shardMetal1 = trap_R_RegisterModel("models/shards/metal1.md3");
	cgs.media.shardMetal2 = trap_R_RegisterModel("models/shards/metal2.md3");
//  cgs.media.shardCeramic1 = trap_R_RegisterModel( "models/shards/ceramic1.md3" );
//  cgs.media.shardCeramic2 = trap_R_RegisterModel( "models/shards/ceramic2.md3" );
	// done

	cgs.media.shardRubble1 = trap_R_RegisterModel("models/mapobjects/debris/brick000.md3");
	cgs.media.shardRubble2 = trap_R_RegisterModel("models/mapobjects/debris/brick001.md3");
	cgs.media.shardRubble3 = trap_R_RegisterModel("models/mapobjects/debris/brick002.md3");

	for(i = 0; i < MAX_LOCKER_DEBRIS; i++)
	{
		Com_sprintf(name, sizeof(name), "models/mapobjects/debris/personal%i.md3", i + 1);
		cgs.media.shardJunk[i] = trap_R_RegisterModel(name);
	}

	memset(cg_items, 0, sizeof(cg_items));
	memset(cg_weapons, 0, sizeof(cg_weapons));

// TODO: FIXME:  REMOVE REGISTRATION OF EACH MODEL FOR EVERY LEVEL LOAD

	//----(SA)  okay, new stuff to intialize rather than doing it at level load time (or "give all" time)
	//          (I'm certainly not against being efficient here, but I'm tired of the rocket launcher effect only registering
	//          sometimes and want it to work for sure for this demo)

	CG_LoadingString(" - weapons");
	for(i = WP_KNIFE; i < WP_NUM_WEAPONS; i++)
	{
		// DHM - Nerve :: Only register weapons we use in WolfMP
		if(BG_WeaponInWolfMP(i))
		{
			CG_RegisterWeapon(i, qfalse);
		}
	}

	CG_LoadingString(" - items");
	for(i = 1; i < bg_numItems; i++)
	{
		CG_RegisterItemVisuals(i);
	}

	cgs.media.grenadeExplosionShader = trap_R_RegisterShader("grenadeExplosion");
	cgs.media.rocketExplosionShader = trap_R_RegisterShader("rocketExplosion");

	cgs.media.hWeaponSnd = trap_S_RegisterSound("sound/weapons/mg42/mg42_fire.wav", qfalse);
	cgs.media.hWeaponEchoSnd = trap_S_RegisterSound("sound/weapons/mg42/mg42_far.wav", qfalse);
	cgs.media.hWeaponHeatSnd = trap_S_RegisterSound("sound/weapons/mg42/mg42_heat.wav", qfalse);

	cgs.media.hWeaponSnd_2 = trap_S_RegisterSound("sound/weapons/browning/browning_fire.wav", qfalse);
	cgs.media.hWeaponEchoSnd_2 = trap_S_RegisterSound("sound/weapons/browning/browning_far.wav", qfalse);
	cgs.media.hWeaponHeatSnd_2 = trap_S_RegisterSound("sound/weapons/browning/browning_heat.wav", qfalse);

//  cgs.media.hflakWeaponSnd =      trap_S_RegisterSound( "sound/vehicles/misc/20mm_fire.wav", qfalse );

	cgs.media.minePrimedSound = trap_S_RegisterSound("sound/weapons/landmine/mine_on.wav", qfalse);

	// wall marks
	cgs.media.bulletMarkShader = trap_R_RegisterShaderNoMip("gfx/damage/bullet_mrk");
	cgs.media.burnMarkShader = trap_R_RegisterShaderNoMip("gfx/damage/burn_med_mrk");
	cgs.media.shadowFootShader = trap_R_RegisterShaderNoMip("markShadowFoot");
	cgs.media.shadowTorsoShader = trap_R_RegisterShaderNoMip("markShadowTorso");
	cgs.media.wakeMarkShader = trap_R_RegisterShaderNoMip("wake");
	cgs.media.wakeMarkShaderAnim = trap_R_RegisterShaderNoMip("wakeAnim");	// (SA)

	//----(SA)  added
	cgs.media.bulletMarkShaderMetal = trap_R_RegisterShaderNoMip("gfx/damage/metal_mrk");
	cgs.media.bulletMarkShaderWood = trap_R_RegisterShaderNoMip("gfx/damage/wood_mrk");
	cgs.media.bulletMarkShaderGlass = trap_R_RegisterShaderNoMip("gfx/damage/glass_mrk");

	for(i = 0; i < 5; i++)
	{
		char            name[32];

		//Com_sprintf( name, sizeof(name), "textures/decals/blood%i", i+1 );
		//cgs.media.bloodMarkShaders[i] = trap_R_RegisterShader( name );
		Com_sprintf(name, sizeof(name), "blood_dot%i", i + 1);
		cgs.media.bloodDotShaders[i] = trap_R_RegisterShader(name);
	}

	CG_LoadingString(" - inline models");

	// register the inline models
	cgs.numInlineModels = trap_CM_NumInlineModels();
	// TAT 12/23/2002 - as a safety check, let's not let the number of models exceed MAX_MODELS
	if(cgs.numInlineModels > MAX_MODELS)
	{
		CG_Error("CG_RegisterGraphics: Too many inline models: %i\n", cgs.numInlineModels);
		//CG_Printf( S_COLOR_RED "WARNING: CG_RegisterGraphics: Too many inline models: %i\n", cgs.numInlineModels );
		//cgs.numInlineModels = MAX_MODELS;
	}

	for(i = 1; i < cgs.numInlineModels; i++)
	{
		char            name[10];
		vec3_t          mins, maxs;
		int             j;

		Com_sprintf(name, sizeof(name), "*%i", i);
		cgs.inlineDrawModel[i] = trap_R_RegisterModel(name);
		trap_R_ModelBounds(cgs.inlineDrawModel[i], mins, maxs);
		for(j = 0; j < 3; j++)
		{
			cgs.inlineModelMidpoints[i][j] = mins[j] + 0.5 * (maxs[j] - mins[j]);
		}
	}

	CG_LoadingString(" - server models");

	// register all the server specified models
	for(i = 1; i < MAX_MODELS; i++)
	{
		const char     *modelName;

		modelName = CG_ConfigString(CS_MODELS + i);
		if(!modelName[0])
		{
			break;
		}
		cgs.gameModels[i] = trap_R_RegisterModel(modelName);
	}

	for(i = 1; i < MAX_MODELS; i++)
	{
		const char     *skinName;

		skinName = CG_ConfigString(CS_SKINS + i);
		if(!skinName[0])
		{
			break;
		}
		cgs.gameModelSkins[i] = trap_R_RegisterSkin(skinName);
	}

	for(i = 1; i < MAX_CS_SHADERS; i++)
	{
		const char     *shaderName;

		shaderName = CG_ConfigString(CS_SHADERS + i);
		if(!shaderName[0])
		{
			break;
		}
		cgs.gameShaders[i] =
			shaderName[0] == '*' ? trap_R_RegisterShader(shaderName + 1) : trap_R_RegisterShaderNoMip(shaderName);
		Q_strncpyz(cgs.gameShaderNames[i], shaderName[0] == '*' ? shaderName + 1 : shaderName, MAX_QPATH);
	}

	for(i = 1; i < MAX_CHARACTERS; i++)
	{
		const char     *characterName;

		characterName = CG_ConfigString(CS_CHARACTERS + i);
		if(!characterName[0])
		{
			break;
		}

		if(!BG_FindCharacter(characterName))
		{
			cgs.gameCharacters[i] = BG_FindFreeCharacter(characterName);

			Q_strncpyz(cgs.gameCharacters[i]->characterFile, characterName, sizeof(cgs.gameCharacters[i]->characterFile));

			if(!CG_RegisterCharacter(characterName, cgs.gameCharacters[i]))
			{
				CG_Error("ERROR: CG_RegisterGraphics: failed to load character file '%s'\n", characterName);
			}
		}
	}

	CG_LoadingString(" - particles");
	CG_ClearParticles();

	InitSmokeSprites();

	CG_LoadingString(" - classes");

	CG_RegisterPlayerClasses();

	CG_InitPMGraphics();

	// mounted gun on tank models
	cgs.media.hMountedMG42Base = trap_R_RegisterModel("models/mapobjects/tanks_sd/mg42nestbase.md3");
	cgs.media.hMountedMG42Nest = trap_R_RegisterModel("models/mapobjects/tanks_sd/mg42nest.md3");
	cgs.media.hMountedMG42 = trap_R_RegisterModel("models/mapobjects/tanks_sd/mg42.md3");
	cgs.media.hMountedBrowning = trap_R_RegisterModel("models/multiplayer/browning/thirdperson.md3");

	// FIXME: temp models
	cgs.media.hMountedFPMG42 = trap_R_RegisterModel("models/multiplayer/mg42/v_mg42.md3");
	cgs.media.hMountedFPBrowning = trap_R_RegisterModel("models/multiplayer/browning/tankmounted.md3");

	// medic icon for commandmap
	cgs.media.medicIcon = trap_R_RegisterShaderNoMip("sprites/voiceMedic");

	trap_R_RegisterFont("ariblk.ttf", 27, &cgs.media.limboFont1);
	trap_R_RegisterFont("ariblk.ttf", 16, &cgs.media.limboFont1_lo);
	trap_R_RegisterFont("courbd.ttf", 30, &cgs.media.limboFont2);

	cgs.media.medal_back = trap_R_RegisterShaderNoMip("gfx/limbo/medal_back");

	cgs.media.limboNumber_roll = trap_R_RegisterShaderNoMip("gfx/limbo/number_roll");
	cgs.media.limboNumber_back = trap_R_RegisterShaderNoMip("gfx/limbo/number_back");
	cgs.media.limboStar_roll = trap_R_RegisterShaderNoMip("gfx/limbo/skill_roll");
	cgs.media.limboStar_back = trap_R_RegisterShaderNoMip("gfx/limbo/skill_back");
	cgs.media.limboLight_on = trap_R_RegisterShaderNoMip("gfx/limbo/redlight_on");
	cgs.media.limboLight_on2 = trap_R_RegisterShaderNoMip("gfx/limbo/redlight_on02");
	cgs.media.limboLight_off = trap_R_RegisterShaderNoMip("gfx/limbo/redlight_off");

	cgs.media.limboWeaponNumber_off = trap_R_RegisterShaderNoMip("gfx/limbo/but_weap_off");
	cgs.media.limboWeaponNumber_on = trap_R_RegisterShaderNoMip("gfx/limbo/but_weap_on");
	cgs.media.limboWeaponCard = trap_R_RegisterShaderNoMip("gfx/limbo/weap_card");

	cgs.media.limboWeaponCardSurroundH = trap_R_RegisterShaderNoMip("gfx/limbo/butsur_hor");
	cgs.media.limboWeaponCardSurroundV = trap_R_RegisterShaderNoMip("gfx/limbo/butsur_vert");
	cgs.media.limboWeaponCardSurroundC = trap_R_RegisterShaderNoMip("gfx/limbo/butsur_corn");

	cgs.media.limboWeaponCardOOS = trap_R_RegisterShaderNoMip("gfx/limbo/outofstock");

	cgs.media.limboClassButtons[PC_ENGINEER] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_engineer");
	cgs.media.limboClassButtons[PC_SOLDIER] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_soldier");
	cgs.media.limboClassButtons[PC_COVERTOPS] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_covertops");
	cgs.media.limboClassButtons[PC_FIELDOPS] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_fieldops");
	cgs.media.limboClassButtons[PC_MEDIC] = trap_R_RegisterShaderNoMip("gfx/limbo/ic_medic");
	cgs.media.limboSkillsBS = trap_R_RegisterShaderNoMip("gfx/limbo/ic_battlesense");
	cgs.media.limboSkillsLW = trap_R_RegisterShaderNoMip("gfx/limbo/ic_lightweap");
	//cgs.media.limboClassButtonBack =          trap_R_RegisterShaderNoMip( "gfx/limbo/but_class"       );

	cgs.media.limboClassButton2Back_on = trap_R_RegisterShaderNoMip("gfx/limbo/skill_back_on");
	cgs.media.limboClassButton2Back_off = trap_R_RegisterShaderNoMip("gfx/limbo/skill_back_off");
	cgs.media.limboClassButton2Wedge_off = trap_R_RegisterShaderNoMip("gfx/limbo/skill_4pieces_off");
	cgs.media.limboClassButton2Wedge_on = trap_R_RegisterShaderNoMip("gfx/limbo/skill_4pieces_on");
	cgs.media.limboClassButtons2[PC_ENGINEER] = trap_R_RegisterShaderNoMip("gfx/limbo/skill_engineer");
	cgs.media.limboClassButtons2[PC_SOLDIER] = trap_R_RegisterShaderNoMip("gfx/limbo/skill_soldier");
	cgs.media.limboClassButtons2[PC_COVERTOPS] = trap_R_RegisterShaderNoMip("gfx/limbo/skill_covops");
	cgs.media.limboClassButtons2[PC_FIELDOPS] = trap_R_RegisterShaderNoMip("gfx/limbo/skill_fieldops");
	cgs.media.limboClassButtons2[PC_MEDIC] = trap_R_RegisterShaderNoMip("gfx/limbo/skill_medic");


	cgs.media.limboTeamButtonBack_on = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_on");
	cgs.media.limboTeamButtonBack_off = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_off");
	cgs.media.limboTeamButtonAllies = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_allied");
	cgs.media.limboTeamButtonAxis = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_axis");
	cgs.media.limboTeamButtonSpec = trap_R_RegisterShaderNoMip("gfx/limbo/but_team_spec");


	cgs.media.limboBlendThingy = trap_R_RegisterShaderNoMip("gfx/limbo/cc_blend");
	cgs.media.limboWeaponBlendThingy = trap_R_RegisterShaderNoMip("gfx/limbo/weap_blend");

	//cgs.media.limboCursor_on =                    trap_R_RegisterShaderNoMip( "gfx/limbo/et_cursor_on"    );
	//cgs.media.limboCursor_off =                   trap_R_RegisterShaderNoMip( "gfx/limbo/et_cursor_off"   );

	cgs.media.limboCounterBorder = trap_R_RegisterShaderNoMip("gfx/limbo/number_border");

	cgs.media.hudPowerIcon = trap_R_RegisterShaderNoMip("gfx/hud/ic_power");
	cgs.media.hudSprintIcon = trap_R_RegisterShaderNoMip("gfx/hud/ic_stamina");
	cgs.media.hudHealthIcon = trap_R_RegisterShaderNoMip("gfx/hud/ic_health");

	cgs.media.limboWeaponCard1 = trap_R_RegisterShaderNoMip("gfx/limbo/weaponcard01");
	cgs.media.limboWeaponCard2 = trap_R_RegisterShaderNoMip("gfx/limbo/weaponcard02");
	cgs.media.limboWeaponCardArrow = trap_R_RegisterShaderNoMip("gfx/limbo/weap_dnarrow.tga");


	cgs.media.limboObjectiveBack[0] = trap_R_RegisterShaderNoMip("gfx/limbo/objective_back_axis");
	cgs.media.limboObjectiveBack[1] = trap_R_RegisterShaderNoMip("gfx/limbo/objective_back_allied");
	cgs.media.limboObjectiveBack[2] = trap_R_RegisterShaderNoMip("gfx/limbo/objective_back");

	cgs.media.limboClassBar = trap_R_RegisterShaderNoMip("gfx/limbo/lightup_bar");

	cgs.media.limboBriefingButtonOn = trap_R_RegisterShaderNoMip("gfx/limbo/but_play_on");
	cgs.media.limboBriefingButtonOff = trap_R_RegisterShaderNoMip("gfx/limbo/but_play_off");
	cgs.media.limboBriefingButtonStopOn = trap_R_RegisterShaderNoMip("gfx/limbo/but_stop_on");
	cgs.media.limboBriefingButtonStopOff = trap_R_RegisterShaderNoMip("gfx/limbo/but_stop_off");

	cgs.media.limboSpectator = trap_R_RegisterShaderNoMip("gfx/limbo/spectator");
	cgs.media.limboRadioBroadcast = trap_R_RegisterShaderNoMip("ui/assets/radio_tower");

	cgs.media.cursorIcon = trap_R_RegisterShaderNoMip("ui/assets/3_cursor3");

	cgs.media.hudDamagedStates[0] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood01.skin");
	cgs.media.hudDamagedStates[1] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood02.skin");
	cgs.media.hudDamagedStates[2] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood03.skin");
	cgs.media.hudDamagedStates[3] = trap_R_RegisterSkin("models/players/hud/damagedskins/blood04.skin");

	cgs.media.browningIcon = trap_R_RegisterShaderNoMip("icons/iconw_browning_1_select");

	cgs.media.axisFlag = trap_R_RegisterShaderNoMip("gfx/limbo/flag_axis");
	cgs.media.alliedFlag = trap_R_RegisterShaderNoMip("gfx/limbo/flag_allied");
	cgs.media.disconnectIcon = trap_R_RegisterShaderNoMip("gfx/2d/net");

	for(i = 0; i < 6; i++)
	{
		cgs.media.fireteamicons[i] = trap_R_RegisterShaderNoMip(va("gfx/hud/fireteam/fireteam%i", i + 1));
	}

	CG_LoadingString(" - game media done");
}

/*
===================
CG_RegisterClients

===================
*/
static void CG_RegisterClients(void)
{
	int             i;

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		const char     *clientInfo;

		clientInfo = CG_ConfigString(CS_PLAYERS + i);
		if(!clientInfo[0])
		{
			continue;
		}
//      CG_LoadingClient( i );
		CG_NewClientInfo(i);
	}
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/

const char     *CG_ConfigString(int index)
{
	if(index < 0 || index >= MAX_CONFIGSTRINGS)
	{
		CG_Error("CG_ConfigString: bad index: %i", index);
	}
	return cgs.gameState.stringData + cgs.gameState.stringOffsets[index];
}

int CG_ConfigStringCopy(int index, char *buff, int buffsize)
{
	Q_strncpyz(buff, CG_ConfigString(index), buffsize);
	return strlen(buff);
}


//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic(void)
{
	char           *s;
	char            parm1[MAX_QPATH], parm2[MAX_QPATH];

	// start the background music
	s = (char *)CG_ConfigString(CS_MUSIC);
	Q_strncpyz(parm1, COM_Parse(&s), sizeof(parm1));
	Q_strncpyz(parm2, COM_Parse(&s), sizeof(parm2));

	if(strlen(parm1))
	{
		trap_S_StartBackgroundTrack(parm1, parm2);
	}
}

/*
==============
CG_QueueMusic
==============
*/
void CG_QueueMusic(void)
{
	char           *s;
	char            parm[MAX_QPATH];

	// prepare the next background track
	s = (char *)CG_ConfigString(CS_MUSIC_QUEUE);
	Q_strncpyz(parm, COM_Parse(&s), sizeof(parm));

	// even if no strlen(parm).  we want to be able to clear the queue

	// TODO: \/     the values stored in here will be made accessable so
	//              it doesn't have to go through startbackgroundtrack() (which is stupid)
	trap_S_StartBackgroundTrack(parm, "");
}

#if 0							//DAJ unused
char           *CG_GetMenuBuffer(const char *filename)
{
	int             len;
	fileHandle_t    f;
	static char     buf[MAX_MENUFILE];

	len = trap_FS_FOpenFile(filename, &f, FS_READ);
	if(!f)
	{
		trap_Print(va(S_COLOR_RED "menu file not found: %s, using default\n", filename));
		return NULL;
	}
	if(len >= MAX_MENUFILE)
	{
		trap_Print(va(S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", filename, len, MAX_MENUFILE));
		trap_FS_FCloseFile(f);
		return NULL;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	return buf;
}
#endif
//
// ==============================
// new hud stuff ( mission pack )
// ==============================
//
qboolean CG_Asset_Parse(int handle)
{
	pc_token_t      token;
	const char     *tempStr;

	if(!trap_PC_ReadToken(handle, &token))
	{
		return qfalse;
	}
	if(Q_stricmp(token.string, "{") != 0)
	{
		return qfalse;
	}

	while(1)
	{
		if(!trap_PC_ReadToken(handle, &token))
		{
			return qfalse;
		}

		if(Q_stricmp(token.string, "}") == 0)
		{
			return qtrue;
		}

		// font
		if(Q_stricmp(token.string, "font") == 0)
		{
			int             pointSize, fontIndex;

			if(!PC_Int_Parse(handle, &fontIndex) || !PC_String_Parse(handle, &tempStr) || !PC_Int_Parse(handle, &pointSize))
			{
				return qfalse;
			}
			if(fontIndex < 0 || fontIndex >= 6)
			{
				return qfalse;
			}
			cgDC.registerFont(tempStr, pointSize, &cgDC.Assets.fonts[fontIndex]);
			continue;
		}

		// gradientbar
		if(Q_stricmp(token.string, "gradientbar") == 0)
		{
			if(!PC_String_Parse(handle, &tempStr))
			{
				return qfalse;
			}
			cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(tempStr);
			continue;
		}

		// enterMenuSound
		if(Q_stricmp(token.string, "menuEnterSound") == 0)
		{
			if(!PC_String_Parse(handle, &tempStr))
			{
				return qfalse;
			}
			cgDC.Assets.menuEnterSound = trap_S_RegisterSound(tempStr, qtrue);
			continue;
		}

		// exitMenuSound
		if(Q_stricmp(token.string, "menuExitSound") == 0)
		{
			if(!PC_String_Parse(handle, &tempStr))
			{
				return qfalse;
			}
			cgDC.Assets.menuExitSound = trap_S_RegisterSound(tempStr, qtrue);
			continue;
		}

		// itemFocusSound
		if(Q_stricmp(token.string, "itemFocusSound") == 0)
		{
			if(!PC_String_Parse(handle, &tempStr))
			{
				return qfalse;
			}
			cgDC.Assets.itemFocusSound = trap_S_RegisterSound(tempStr, qtrue);
			continue;
		}

		// menuBuzzSound
		if(Q_stricmp(token.string, "menuBuzzSound") == 0)
		{
			if(!PC_String_Parse(handle, &tempStr))
			{
				return qfalse;
			}
			cgDC.Assets.menuBuzzSound = trap_S_RegisterSound(tempStr, qtrue);
			continue;
		}

		if(Q_stricmp(token.string, "cursor") == 0)
		{
			if(!PC_String_Parse(handle, &cgDC.Assets.cursorStr))
			{
				return qfalse;
			}
			cgDC.Assets.cursor = trap_R_RegisterShaderNoMip(cgDC.Assets.cursorStr);
			continue;
		}

		if(Q_stricmp(token.string, "fadeClamp") == 0)
		{
			if(!PC_Float_Parse(handle, &cgDC.Assets.fadeClamp))
			{
				return qfalse;
			}
			continue;
		}

		if(Q_stricmp(token.string, "fadeCycle") == 0)
		{
			if(!PC_Int_Parse(handle, &cgDC.Assets.fadeCycle))
			{
				return qfalse;
			}
			continue;
		}

		if(Q_stricmp(token.string, "fadeAmount") == 0)
		{
			if(!PC_Float_Parse(handle, &cgDC.Assets.fadeAmount))
			{
				return qfalse;
			}
			continue;
		}

		if(Q_stricmp(token.string, "shadowX") == 0)
		{
			if(!PC_Float_Parse(handle, &cgDC.Assets.shadowX))
			{
				return qfalse;
			}
			continue;
		}

		if(Q_stricmp(token.string, "shadowY") == 0)
		{
			if(!PC_Float_Parse(handle, &cgDC.Assets.shadowY))
			{
				return qfalse;
			}
			continue;
		}

		if(Q_stricmp(token.string, "shadowColor") == 0)
		{
			if(!PC_Color_Parse(handle, &cgDC.Assets.shadowColor))
			{
				return qfalse;
			}
			cgDC.Assets.shadowFadeClamp = cgDC.Assets.shadowColor[3];
			continue;
		}
	}
	//return qfalse;
}

void CG_ParseMenu(const char *menuFile)
{
	pc_token_t      token;
	int             handle;

	handle = trap_PC_LoadSource(menuFile);
	if(!handle)
	{
		handle = trap_PC_LoadSource("ui/testhud.menu");
	}
	if(!handle)
	{
		return;
	}

	while(1)
	{
		if(!trap_PC_ReadToken(handle, &token))
		{
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//  Com_Printf( "Missing { in menu file\n" );
		//  break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//  Com_Printf( "Too many menus!\n" );
		//  break;
		//}

		if(token.string[0] == '}')
		{
			break;
		}

		if(Q_stricmp(token.string, "assetGlobalDef") == 0)
		{
			if(CG_Asset_Parse(handle))
			{
				continue;
			}
			else
			{
				break;
			}
		}


		if(Q_stricmp(token.string, "menudef") == 0)
		{
			// start a new menu
			Menu_New(handle);
		}
	}
	trap_PC_FreeSource(handle);
}

qboolean CG_Load_Menu(char **p)
{
	char           *token;

	token = COM_ParseExt(p, qtrue);

	if(token[0] != '{')
	{
		return qfalse;
	}

	while(1)
	{

		token = COM_ParseExt(p, qtrue);

		if(Q_stricmp(token, "}") == 0)
		{
			return qtrue;
		}

		if(!token || token[0] == 0)
		{
			return qfalse;
		}

		CG_ParseMenu(token);
	}
	return qfalse;
}



void CG_LoadMenus(const char *menuFile)
{
	char           *token;
	char           *p;
	int             len, start;
	fileHandle_t    f;
	static char     buf[MAX_MENUDEFFILE];

	start = trap_Milliseconds();

	len = trap_FS_FOpenFile(menuFile, &f, FS_READ);
	if(!f)
	{
		Com_Printf(S_COLOR_YELLOW "menu file not found: %s, using default\n", menuFile);
		len = trap_FS_FOpenFile("ui/hud.txt", &f, FS_READ);
		if(!f)
		{
			trap_Error(S_COLOR_RED "default menu file not found: ui/hud.txt, unable to continue!\n");
		}
	}

	if(len >= MAX_MENUDEFFILE)
	{
		trap_Error(va(S_COLOR_RED "menu file too large: %s is %i, max allowed is %i", menuFile, len, MAX_MENUDEFFILE));
		trap_FS_FCloseFile(f);
		return;
	}

	trap_FS_Read(buf, len, f);
	buf[len] = 0;
	trap_FS_FCloseFile(f);

	COM_Compress(buf);

	Menu_Reset();

	p = buf;

	while(1)
	{
		token = COM_ParseExt(&p, qtrue);
		if(!token || token[0] == 0 || token[0] == '}')
		{
			break;
		}

		//if ( Q_stricmp( token, "{" ) ) {
		//  Com_Printf( "Missing { in menu file\n" );
		//  break;
		//}

		//if ( menuCount == MAX_MENUS ) {
		//  Com_Printf( "Too many menus!\n" );
		//  break;
		//}

		if(Q_stricmp(token, "}") == 0)
		{
			break;
		}

		if(Q_stricmp(token, "loadmenu") == 0)
		{
			if(CG_Load_Menu(&p))
			{
				continue;
			}
			else
			{
				break;
			}
		}
	}

	Com_Printf("UI menu load time = %d milli seconds\n", trap_Milliseconds() - start);

}



static qboolean CG_OwnerDrawHandleKey(int ownerDraw, int flags, float *special, int key)
{
	return qfalse;
}


static int CG_FeederCount(float feederID)
{
	int             i, count;

	count = 0;
	if(feederID == FEEDER_REDTEAM_LIST)
	{
		for(i = 0; i < cg.numScores; i++)
		{
			if(cg.scores[i].team == TEAM_AXIS)
			{
				count++;
			}
		}
	}
	else if(feederID == FEEDER_BLUETEAM_LIST)
	{
		for(i = 0; i < cg.numScores; i++)
		{
			if(cg.scores[i].team == TEAM_ALLIES)
			{
				count++;
			}
		}
	}
	else if(feederID == FEEDER_SCOREBOARD)
	{
		return cg.numScores;
	}
	return count;
}




///////////////////////////
///////////////////////////

static clientInfo_t *CG_InfoFromScoreIndex(int index, int team, int *scoreIndex)
{
	int             i, count;

	count = 0;
	for(i = 0; i < cg.numScores; i++)
	{
		if(cg.scores[i].team == team)
		{
			if(count == index)
			{
				*scoreIndex = i;
				return &cgs.clientinfo[cg.scores[i].client];
			}
			count++;
		}
	}

	*scoreIndex = index;
	return &cgs.clientinfo[cg.scores[index].client];
}

static const char *CG_FeederItemText(float feederID, int index, int column, qhandle_t * handle, int *numhandles)
{
	int             scoreIndex = 0;
	clientInfo_t   *info = NULL;
	int             team = -1;
	score_t        *sp = NULL;

	*handle = -1;

	if(feederID == FEEDER_REDTEAM_LIST)
	{
		team = TEAM_AXIS;
	}
	else if(feederID == FEEDER_BLUETEAM_LIST)
	{
		team = TEAM_ALLIES;
	}

	info = CG_InfoFromScoreIndex(index, team, &scoreIndex);
	sp = &cg.scores[scoreIndex];

	if(info && info->infoValid)
	{
		switch (column)
		{
			case 0:
				break;
			case 3:
				return info->name;
				break;
			case 4:
				return va("%i", info->score);
				break;
			case 5:
				return va("%4i", sp->time);
				break;
			case 6:
				if(sp->ping == -1)
				{
					return "connecting";
				}
				return va("%4i", sp->ping);
				break;
		}
	}

	return "";
}

static qhandle_t CG_FeederItemImage(float feederID, int index)
{
	return 0;
}

static void CG_FeederSelection(float feederID, int index)
{
	int             i, count;
	int             team = (feederID == FEEDER_REDTEAM_LIST) ? TEAM_AXIS : TEAM_ALLIES;

	count = 0;
	for(i = 0; i < cg.numScores; i++)
	{
		if(cg.scores[i].team == team)
		{
			if(index == count)
			{
				cg.selectedScore = i;
			}
			count++;
		}
	}
}

float CG_Cvar_Get(const char *cvar)
{
	char            buff[128];

	memset(buff, 0, sizeof(buff));
	trap_Cvar_VariableStringBuffer(cvar, buff, sizeof(buff));
	return atof(buff);
}

void CG_Text_PaintWithCursor(float x, float y, float scale, vec4_t color, const char *text, int cursorPos, char cursor, int limit,
							 int style)
{
	CG_Text_Paint(x, y, scale, color, text, 0, limit, style);
}

static int CG_OwnerDrawWidth(int ownerDraw, float scale)
{
	switch (ownerDraw)
	{
		default:
			break;
	}
	return 0;
}

static int CG_PlayCinematic(const char *name, float x, float y, float w, float h)
{
	return trap_CIN_PlayCinematic(name, x, y, w, h, CIN_loop);
}

static void CG_StopCinematic(int handle)
{
	trap_CIN_StopCinematic(handle);
}

static void CG_DrawCinematic(int handle, float x, float y, float w, float h)
{
	trap_CIN_SetExtents(handle, x, y, w, h);
	trap_CIN_DrawCinematic(handle);
}

static void CG_RunCinematicFrame(int handle)
{
	trap_CIN_RunCinematic(handle);
}




/*
=================
CG_LoadHudMenu();

=================
*/
void CG_LoadHudMenu()
{
	cgDC.registerShaderNoMip = &trap_R_RegisterShaderNoMip;
	cgDC.setColor = &trap_R_SetColor;
	cgDC.drawHandlePic = &CG_DrawPic;
	cgDC.drawStretchPic = &trap_R_DrawStretchPic;
	cgDC.drawText = &CG_Text_Paint;
	cgDC.drawTextExt = &CG_Text_Paint_Ext;
	cgDC.textWidth = &CG_Text_Width;
	cgDC.textWidthExt = &CG_Text_Width_Ext;
	cgDC.textHeight = &CG_Text_Height;
	cgDC.textHeightExt = &CG_Text_Height_Ext;
	cgDC.textFont = &CG_Text_SetActiveFont;
	cgDC.registerModel = &trap_R_RegisterModel;
	cgDC.modelBounds = &trap_R_ModelBounds;
	cgDC.fillRect = &CG_FillRect;
	cgDC.drawRect = &CG_DrawRect;
	cgDC.drawSides = &CG_DrawSides;
	cgDC.drawTopBottom = &CG_DrawTopBottom;
	cgDC.clearScene = &trap_R_ClearScene;
	cgDC.addRefEntityToScene = &trap_R_AddRefEntityToScene;
	cgDC.renderScene = &trap_R_RenderScene;
	cgDC.registerFont = &trap_R_RegisterFont;
	cgDC.ownerDrawItem = &CG_OwnerDraw;
	cgDC.getValue = &CG_GetValue;
	cgDC.ownerDrawVisible = &CG_OwnerDrawVisible;
	cgDC.runScript = &CG_RunMenuScript;
	cgDC.getTeamColor = &CG_GetTeamColor;
	cgDC.setCVar = trap_Cvar_Set;
	cgDC.getCVarString = trap_Cvar_VariableStringBuffer;
	cgDC.getCVarValue = CG_Cvar_Get;
	cgDC.drawTextWithCursor = &CG_Text_PaintWithCursor;
	cgDC.setOverstrikeMode = &trap_Key_SetOverstrikeMode;
	cgDC.getOverstrikeMode = &trap_Key_GetOverstrikeMode;
	cgDC.startLocalSound = &trap_S_StartLocalSound;
	cgDC.ownerDrawHandleKey = &CG_OwnerDrawHandleKey;
	cgDC.feederCount = &CG_FeederCount;
	cgDC.feederItemImage = &CG_FeederItemImage;
	cgDC.feederItemText = &CG_FeederItemText;
	cgDC.feederSelection = &CG_FeederSelection;
	cgDC.setBinding = &trap_Key_SetBinding;	// NERVE - SMF
	cgDC.getBindingBuf = &trap_Key_GetBindingBuf;	// NERVE - SMF
	cgDC.getKeysForBinding = &trap_Key_KeysForBinding;
	cgDC.keynumToStringBuf = &trap_Key_KeynumToStringBuf;	// NERVE - SMF
	cgDC.translateString = &CG_TranslateString;	// NERVE - SMF
	//cgDC.executeText = &trap_Cmd_ExecuteText;
	cgDC.Error = &Com_Error;
	cgDC.Print = &Com_Printf;
	cgDC.ownerDrawWidth = &CG_OwnerDrawWidth;
	//cgDC.Pause = &CG_Pause;
	cgDC.registerSound = &trap_S_RegisterSound;
	cgDC.startBackgroundTrack = &trap_S_StartBackgroundTrack;
	cgDC.stopBackgroundTrack = &trap_S_StopBackgroundTrack;
	cgDC.playCinematic = &CG_PlayCinematic;
	cgDC.stopCinematic = &CG_StopCinematic;
	cgDC.drawCinematic = &CG_DrawCinematic;
	cgDC.runCinematicFrame = &CG_RunCinematicFrame;
	cgDC.descriptionForCampaign = &CG_DescriptionForCampaign;
	cgDC.nameForCampaign = &CG_NameForCampaign;
	cgDC.add2dPolys = &trap_R_Add2dPolys;
	cgDC.updateScreen = &trap_UpdateScreen;
	cgDC.getHunkData = &trap_GetHunkData;
	cgDC.getConfigString = &CG_ConfigStringCopy;

	cgDC.xscale = cgs.screenXScale;
	cgDC.yscale = cgs.screenYScale;

	Init_Display(&cgDC);

	Menu_Reset();

//  CG_LoadMenus("ui/hud.txt");

	CG_Text_SetActiveFont(0);
}

void CG_AssetCache()
{
	cgDC.Assets.gradientBar = trap_R_RegisterShaderNoMip(ASSET_GRADIENTBAR);
	cgDC.Assets.fxBasePic = trap_R_RegisterShaderNoMip(ART_FX_BASE);
	cgDC.Assets.fxPic[0] = trap_R_RegisterShaderNoMip(ART_FX_RED);
	cgDC.Assets.fxPic[1] = trap_R_RegisterShaderNoMip(ART_FX_YELLOW);
	cgDC.Assets.fxPic[2] = trap_R_RegisterShaderNoMip(ART_FX_GREEN);
	cgDC.Assets.fxPic[3] = trap_R_RegisterShaderNoMip(ART_FX_TEAL);
	cgDC.Assets.fxPic[4] = trap_R_RegisterShaderNoMip(ART_FX_BLUE);
	cgDC.Assets.fxPic[5] = trap_R_RegisterShaderNoMip(ART_FX_CYAN);
	cgDC.Assets.fxPic[6] = trap_R_RegisterShaderNoMip(ART_FX_WHITE);
	cgDC.Assets.scrollBar = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR);
	cgDC.Assets.scrollBarArrowDown = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWDOWN);
	cgDC.Assets.scrollBarArrowUp = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWUP);
	cgDC.Assets.scrollBarArrowLeft = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWLEFT);
	cgDC.Assets.scrollBarArrowRight = trap_R_RegisterShaderNoMip(ASSET_SCROLLBAR_ARROWRIGHT);
	cgDC.Assets.scrollBarThumb = trap_R_RegisterShaderNoMip(ASSET_SCROLL_THUMB);
	cgDC.Assets.sliderBar = trap_R_RegisterShaderNoMip(ASSET_SLIDER_BAR);
	cgDC.Assets.sliderThumb = trap_R_RegisterShaderNoMip(ASSET_SLIDER_THUMB);
}


extern qboolean initTrails;
void            CG_ClearTrails(void);
extern qboolean initparticles;
void            CG_ClearParticles(void);

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/
#ifdef _DEBUG
#define DEBUG_INITPROFILE_INIT int elapsed, dbgTime = trap_Milliseconds();
#define DEBUG_INITPROFILE_EXEC( f ) if ( developer.integer ) { CG_Printf( "^5%s passed in %i msec\n", f, elapsed = trap_Milliseconds() - dbgTime );  dbgTime += elapsed; }
#endif							// _DEBUG
void CG_Init(int serverMessageNum, int serverCommandSequence, int clientNum, qboolean demoPlayback)
{
	const char     *s;
	int             i;

#ifdef _DEBUG
	DEBUG_INITPROFILE_INIT
#endif							// _DEBUG
//  int startat = trap_Milliseconds();
		// clear everything
		memset(&cgs, 0, sizeof(cgs));
	memset(&cg, 0, sizeof(cg));
	memset(cg_entities, 0, sizeof(cg_entities));
	memset(cg_weapons, 0, sizeof(cg_weapons));
	memset(cg_items, 0, sizeof(cg_items));

	cgs.initing = qtrue;

	for(i = 0; i < MAX_CLIENTS; i++)
	{
		cg.artilleryRequestTime[i] = -99999;
	}

	CG_InitStatsDebug();

	cgs.ccZoomFactor = 1.f;

	// OSP - sync to main refdef
	cg.refdef_current = &cg.refdef;

	// get the rendering configuration from the client system
	trap_GetGlconfig(&cgs.glconfig);
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0;

	// RF, init the anim scripting
	cgs.animScriptData.soundIndex = CG_SoundScriptPrecache;
	cgs.animScriptData.playSound = CG_SoundPlayIndexedScript;

	cg.clientNum = clientNum;	// NERVE - SMF - TA merge

	cgs.processedSnapshotNum = serverMessageNum;
	cgs.serverCommandSequence = serverCommandSequence;

	cgs.ccRequestedObjective = -1;
	cgs.ccCurrentCamObjective = -2;

	// CHRUKER: b079 - Background images on the loading screen were not visible on the first call
	trap_R_SetColor( NULL );

	// load a few needed things before we do any screen updates
	cgs.media.charsetShader = trap_R_RegisterShader("gfx/2d/hudchars");	//trap_R_RegisterShader( "gfx/2d/bigchars" );
	// JOSEPH 4-17-00
	cgs.media.menucharsetShader = trap_R_RegisterShader("gfx/2d/hudchars");
	// END JOSEPH
	cgs.media.whiteShader = trap_R_RegisterShader("white");
	cgs.media.charsetProp = trap_R_RegisterShaderNoMip("menu/art/font1_prop.tga");
	cgs.media.charsetPropGlow = trap_R_RegisterShaderNoMip("menu/art/font1_prop_glo.tga");
	cgs.media.charsetPropB = trap_R_RegisterShaderNoMip("menu/art/font2_prop.tga");

	CG_RegisterCvars();

	CG_InitConsoleCommands();

	// Gordon: moved this up so it's initialized for the loading screen
	CG_LoadHudMenu();			// load new hud stuff
	CG_AssetCache();

	// get the gamestate from the client system
	trap_GetGameState(&cgs.gameState);

	cg.warmupCount = -1;

	CG_ParseServerinfo();
	CG_ParseWolfinfo();			// NERVE - SMF

	cgs.campaignInfoLoaded = qfalse;
	if(cgs.gametype == GT_WOLF_CAMPAIGN)
	{
		CG_LocateCampaign();
	}
	else if(cgs.gametype == GT_WOLF_STOPWATCH || cgs.gametype == GT_WOLF_LMS || cgs.gametype == GT_WOLF)
	{
		CG_LocateArena();
	}

	CG_ClearTrails();
	CG_ClearParticles();

	InitSmokeSprites();

	// check version
	s = CG_ConfigString(CS_GAME_VERSION);
	if(strcmp(s, GAME_VERSION))
	{
		CG_Error("Client/Server game mismatch: '%s/%s'", GAME_VERSION, s);
	}
	trap_Cvar_Set("cg_etVersion", GAME_VERSION_DATED);	// So server can check

	s = CG_ConfigString(CS_LEVEL_START_TIME);
	cgs.levelStartTime = atoi(s);

	s = CG_ConfigString(CS_INTERMISSION_START_TIME);
	cgs.intermissionStartTime = atoi(s);


	// OSP
	CG_ParseServerVersionInfo(CG_ConfigString(CS_VERSIONINFO));
	CG_ParseReinforcementTimes(CG_ConfigString(CS_REINFSEEDS));

	CG_initStrings();
	CG_windowInit();

	cgs.smokeWindDir = crandom();

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC("initialization")
#endif							// DEBUG
		// load the new map
		CG_LoadingString("collision map");

	trap_CM_LoadMap(cgs.mapname);

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC("loadmap")
#endif							// DEBUG
		String_Init();

	cg.loading = qtrue;			// force players to load instead of defer

	CG_LoadingString("sounds");

	CG_RegisterSounds();

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC("sounds")
#endif							// DEBUG
		CG_LoadingString("graphics");

	CG_RegisterGraphics();

	CG_LoadingString("flamechunks");

	CG_InitFlameChunks();		// RF, register and clear all flamethrower resources

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC("graphics")
#endif							// DEBUG
		CG_LoadingString("clients");

	CG_RegisterClients();		// if low on memory, some clients will be deferred

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC("clients")
#endif							// DEBUG
		cg.loading = qfalse;	// future players will be deferred

	CG_InitLocalEntities();

	BG_BuildSplinePaths();

	CG_InitMarkPolys();

	// remove the last loading update
	cg.infoScreenText[0] = 0;

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_StartMusic();

	cg.lightstylesInited = qfalse;

	CG_LoadingString("");

	CG_ShaderStateChanged();

	CG_ChargeTimesChanged();

	trap_S_ClearLoopingSounds(qfalse);
	trap_S_ClearSounds(qfalse);

	cg.teamWonRounds[1] = atoi(CG_ConfigString(CS_ROUNDSCORES1));
	cg.teamWonRounds[0] = atoi(CG_ConfigString(CS_ROUNDSCORES2));

	cg.filtercams = atoi(CG_ConfigString(CS_FILTERCAMS)) ? qtrue : qfalse;

	CG_ParseFireteams();

	CG_ParseOIDInfos();

	CG_InitPM();

	CG_ParseSpawns();

	CG_ParseTagConnects();

#ifdef _DEBUG
	DEBUG_INITPROFILE_EXEC("misc")
#endif							// DEBUG
		CG_ParseSkyBox();

	CG_SetupCabinets();

	if(!CG_IsSinglePlayer())
	{
		trap_S_FadeAllSound(1.0f, 0, qfalse);	// fade sound up
	}

	// OSP
	cgs.dumpStatsFile = 0;
	cgs.dumpStatsTime = 0;
//  CG_Printf("Time taken: %i\n", trap_Milliseconds() - startat);
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown(void)
{
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data

	CG_EventHandling(CGAME_EVENT_NONE, qtrue);
	if(cg.demoPlayback)
	{
		trap_Cvar_Set("timescale", "1");
	}
}

// returns true if game is single player (or coop)
qboolean CG_IsSinglePlayer(void)
{
	if(cg_gameType.integer == GT_SINGLE_PLAYER || cgs.gametype == GT_COOP)
	{
		return qtrue;
	}

	return qfalse;
}


qboolean CG_CheckExecKey(int key)
{
	if(!cg.showFireteamMenu)
	{
		return qfalse;
	}

	return CG_FireteamCheckExecKey(key, qfalse);
}
