/*
===========================================================================

OpenWolf GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the OpenWolf GPL Source Code (OpenWolf Source Code).  

OpenWolf Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

OpenWolf Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with OpenWolf Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the OpenWolf Source Code is also subject to certain additional terms. 
You should have received a copy of these additional terms immediately following the 
terms and conditions of the GNU General Public License which accompanied the OpenWolf 
Source Code.  If not, please request a copy in writing from id Software at the address 
below.

If you have questions concerning this license or the applicable additional terms, you 
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, 
Maryland 20850 USA.

===========================================================================
*/

/*
 * name:	cg_local.h
 *
 * desc:	The entire cgame module is unloaded and reloaded on each level change,
 *			so there is NO persistant data between levels on the client side.
 *			If you absolutely need something stored, it can either be kept
 *			by the server in the server stored userinfos, or stashed in a cvar.

 *
*/

#include "../../../../engine/qcommon/q_shared.h"
#include "../../../../engine/renderer/tr_types.h"
#include "../game/bg_public.h"
#include "../../../../engine/client/cg_api.h"
#include "../../../../engine/client/ui_api.h"
#include "../ui/ui_shared.h"


//#define MAX_LOCATIONS       256
#define POWERUP_BLINKS      5

#define STATS_FADE_TIME     200.0f
#define POWERUP_BLINK_TIME  1000
#define FADE_TIME           200
#define DAMAGE_DEFLECT_TIME 100
#define DAMAGE_RETURN_TIME  400
#define DAMAGE_TIME         500
#define LAND_DEFLECT_TIME   150
#define LAND_RETURN_TIME    300
#define STEP_TIME           200
#define DUCK_TIME           100
#define PAIN_TWITCH_TIME    200
#define ZOOM_TIME           150
#define MUZZLE_FLASH_TIME   30
#define SINK_TIME           1000	// time for fragments to sink into ground before going away
#define REWARD_TIME         3000

#define PRONE_TIME          500

#define PULSE_SCALE         1.5	// amount to scale up the icons when activating

#define MAX_STEP_CHANGE     32

#define MAX_VERTS_ON_POLY   10
#define MAX_MARK_POLYS      256	// JPW NERVE was 1024

#define STAT_MINUS          10	// num frame for '-' stats digit

#define ICON_SIZE           48
#define CHAR_WIDTH          32
#define CHAR_HEIGHT         48
#define TEXT_ICON_SPACE     4

#define TEAMCHAT_WIDTH      70
#define TEAMCHAT_HEIGHT     8

#define NOTIFY_WIDTH        80
#define NOTIFY_HEIGHT       5

// very large characters
#define GIANT_WIDTH         32
#define GIANT_HEIGHT        48

#define NUM_CROSSHAIRS      10

// Ridah, trails
#define STYPE_STRETCH   0
#define STYPE_REPEAT    1

#define TJFL_FADEIN     ( 1 << 0 )
#define TJFL_CROSSOVER  ( 1 << 1 )
#define TJFL_NOCULL     ( 1 << 2 )
#define TJFL_FIXDISTORT ( 1 << 3 )
#define TJFL_SPARKHEADFLARE ( 1 << 4 )
#define TJFL_NOPOLYMERGE    ( 1 << 5 )
// done.

// OSP
// Autoaction values
#define AA_DEMORECORD   0x01
#define AA_SCREENSHOT   0x02
#define AA_STATSDUMP    0x04

// Cursor
#define CURSOR_OFFSETX  13
#define CURSOR_OFFSETY  12

// Demo controls
#define DEMO_THIRDPERSONUPDATE  0
#define DEMO_RANGEDELTA         6
#define DEMO_ANGLEDELTA         4

// MV overlay
#define MVINFO_TEXTSIZE     10
#define MVINFO_RIGHT        640 - 3
#define MVINFO_TOP          100

#define MAX_WINDOW_COUNT        10
#define MAX_WINDOW_LINES        64

#define MAX_STRINGS             80
#define MAX_STRING_POOL_LENGTH  128

#define WINDOW_FONTWIDTH    8	// For non-true-type: width to scale from
#define WINDOW_FONTHEIGHT   8	// For non-true-type: height to scale from

#define WID_NONE            0x00	// General window
#define WID_STATS           0x01	// Stats (reusable due to scroll effect)
#define WID_TOPSHOTS        0x02	// Top/Bottom-shots
#define WID_MOTD            0x04	// MOTD
#define WID_DEMOHELP      	0x08    // Demo key control info
#define WID_SPECHELP      	0x10    // MV spectator key control info

#define WFX_TEXTSIZING      0x01	// Size the window based on text/font setting
#define WFX_FLASH           0x02	// Alternate between bg and b2 every half second
#define WFX_TRUETYPE        0x04	// Use truetype fonts for text
#define WFX_MULTIVIEW       0x08	// Multiview window
// These need to be last
#define WFX_FADEIN          0x10	// Fade the window in (and back out when closing)
#define WFX_SCROLLUP        0x20	// Scroll window up from the bottom (and back down when closing)
#define WFX_SCROLLDOWN      0x40	// Scroll window down from the top (and back up when closing)
#define WFX_SCROLLLEFT      0x80	// Scroll window in from the left (and back right when closing)
#define WFX_SCROLLRIGHT     0x100	// Scroll window in from the right (and back left when closing)

#define WSTATE_COMPLETE     0x00	// Window is up with startup effects complete
#define WSTATE_START        0x01	// Window is "initializing" w/effects
#define WSTATE_SHUTDOWN     0x02	// Window is shutting down with effects
#define WSTATE_OFF          0x04	// Window is completely shutdown

#define MV_PID              0x00FF	// Bits available for player IDs for MultiView windows
#define MV_SELECTED         0x0100	// MultiView selected window flag is the 9th bit

typedef struct
{
	vec4_t          colorBorder;	// Window border color
	vec4_t          colorBackground;	// Window fill color
	vec4_t          colorBackground2;	// Window fill color2 (for flashing)
	int             curX;		// Scrolling X position
	int             curY;		// Scrolling Y position
	int             effects;	// Window effects
	int             flashMidpoint;	// Flashing transition point (in ms)
	int             flashPeriod;	// Background flashing period (in ms)
	int             fontHeight;	// For non-truetype font drawing
	float           fontScaleX;	// Font scale factor
	float           fontScaleY;	// Font scale factor
	int             fontWidth;	// For non-truetype font drawing
	float           h;			// Height
	int             id;			// Window ID for special handling (i.e. stats, motd, etc.)
	qboolean        inuse;		// Activity flag
	int             lineCount;	// Number of lines to display
	int             lineHeight[MAX_WINDOW_LINES];	// Height property for each line
	char           *lineText[MAX_WINDOW_LINES];	// Text info
	float           m_x;		// Mouse X position
	float           m_y;		// Mouse Y position
	int             mvInfo;		// lower 8 = player id, 9 = is_selected
	int             targetTime;	// Time to complete any defined effect
	int             state;		// Current state of the window
	int             time;		// Current window time
	float           w;			// Width
	float           x;			// Target x-coordinate
	//    negative values will align the window from the right minus the (window width + offset(x))
	float           y;			// Target y-coordinate
	//    negative values will align the window from the bottom minus the (window height + offset(y))
} cg_window_t;

typedef struct
{
	qboolean        fActive;
	char            str[MAX_STRING_POOL_LENGTH];
} cg_string_t;

typedef struct
{
	int             activeWindows[MAX_WINDOW_COUNT];	// List of active windows
	int             numActiveWindows;	// Number of active windows in use
	cg_window_t     window[MAX_WINDOW_COUNT];	// Static allocation of all windows
} cg_windowHandler_t;

typedef struct
{
	int             pID;		// Player ID
	int             classID;	// Player's current class
	int             width;		// Width of text box
	char            info[8];	// On-screen info (w/color coding)
	qboolean        fActive;	// Overlay element is active
	cg_window_t    *w;			// Window handle (may be NULL)
} cg_mvinfo_t;

// OSP




// START Mad Doc - TDF
#define NUM_OVERLAY_FACES 1
// END Mad Doc - TDF

//=================================================

// player entities need to track more information
// than any other type of entity.

// note that not every player entity is a client entity,
// because corpses after respawn are outside the normal
// client numbering range

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct
{
	int             oldFrame;
	int             oldFrameTime;	// time when ->oldFrame was exactly on
	qhandle_t       oldFrameModel;

	int             frame;
	int             frameTime;	// time when ->frame will be exactly on
	qhandle_t       frameModel;

	float           backlerp;

	float           yawAngle;
	qboolean        yawing;
	float           pitchAngle;
	qboolean        pitching;

	int             animationNumber;	// may include ANIM_TOGGLEBIT
	int             oldAnimationNumber;	// may include ANIM_TOGGLEBIT
	animation_t    *animation;
	int             animationTime;	// time when the first frame of the animation will be exact


	// Ridah, variable speed anims
	vec3_t          oldFramePos;
	float           animSpeedScale;
	int             oldFrameSnapshotTime;
	headAnimation_t *headAnim;
	// done.

} lerpFrame_t;

typedef struct
{
	lerpFrame_t     legs, torso;
	lerpFrame_t     head;
	lerpFrame_t     weap;		//----(SA)  autonomous weapon animations
	lerpFrame_t     hudhead;

	int             painTime;
	int             painDuration;
	int             painDirection;	// flip from 0 to 1
	int             painAnimTorso;
	int             painAnimLegs;
	int             lightningFiring;

	// Ridah, so we can do fast tag grabbing
	refEntity_t     bodyRefEnt, headRefEnt, gunRefEnt;
	int             gunRefEntFrame;

	float           animSpeed;	// for manual adjustment

	int             lastFiredWeaponTime;
	int             weaponFireTime;
} playerEntity_t;

//=================================================

typedef struct tag_s
{
	vec3_t          origin;
	vec3_t          axis[3];
} tag_t;


// centity_t have a direct corespondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s
{
	entityState_t   currentState;	// from cg.frame
	entityState_t   nextState;	// from cg.nextFrame, if available
	qboolean        interpolate;	// true if next is valid to interpolate to
	qboolean        currentValid;	// true if cg.frame holds this entity

	int             muzzleFlashTime;	// move to playerEntity?
	int             overheatTime;
	int             previousEvent;
	int             previousEventSequence;	// Ridah
	int             teleportFlag;

	int             trailTime;	// so missile trails can handle dropped initial packets
	int             miscTime;
	int             soundTime;	// ydnar: so looping sounds can start when triggered

	playerEntity_t  pe;

//  int             errorTime;      // decay the error from this time
//  vec3_t          errorOrigin;
//  vec3_t          errorAngles;

//  qboolean        extrapolated;   // false if origin / angles is an interpolation
	vec3_t          rawOrigin;
	vec3_t          rawAngles;

	// exact interpolated position of entity on this frame
	vec3_t          lerpOrigin;
	vec3_t          lerpAngles;

	vec3_t          lastLerpAngles;	// (SA) for remembering the last position when a state changes
	vec3_t          lastLerpOrigin;	// Gordon: Added for linked trains player adjust prediction

	// Ridah, trail effects
	int             headJuncIndex, headJuncIndex2;
	int             lastTrailTime;
	// done.

	// Ridah
	vec3_t          fireRiseDir;	// if standing still this will be up, otherwise it'll point away from movement dir
	int             lastFuseSparkTime;

	// client side dlights
	int             dl_frame;
	int             dl_oldframe;
	float           dl_backlerp;
	int             dl_time;
	char            dl_stylestring[64];
	int             dl_sound;
	int             dl_atten;

	lerpFrame_t     lerpFrame;	//----(SA)  added
	vec3_t          highlightOrigin;	// center of the geometry.  for things like corona placement on treasure
	qboolean        usehighlightOrigin;

	refEntity_t     refEnt;
	int             processedFrame;	// frame we were last added to the scene

	int             voiceChatSprite;	// DHM - Nerve
	int             voiceChatSpriteTime;	// DHM - Nerve

	// item highlighting
	int             highlightTime;
	qboolean        highlighted;

	// spline stuff
	vec3_t          origin2;
	splinePath_t   *backspline;
	float           backdelta;
	qboolean        back;
	qboolean        moving;


	int             tankframe;
	int             tankparent;
	tag_t           mountedMG42Base;
	tag_t           mountedMG42Nest;
	tag_t           mountedMG42;
	tag_t           mountedMG42Player;
	tag_t           mountedMG42Flash;

	qboolean        akimboFire;

	// Gordon: tagconnect cleanup..
	int             tagParent;
	char            tagName[MAX_QPATH];
} centity_t;


//======================================================================

// local entities are created as a result of events or predicted actions,
// and live independantly from all server transmitted entities

typedef struct markPoly_s
{
	struct markPoly_s *prevMark, *nextMark;
	int             time;
	qhandle_t       markShader;
	qboolean        alphaFade;	// fade alpha instead of rgb
	float           color[4];
	poly_t          poly;
	polyVert_t      verts[MAX_VERTS_ON_POLY];

	int             duration;	// Ridah
} markPoly_t;

//----(SA)  moved in from cg_view.c
typedef enum
{
	ZOOM_NONE,
	ZOOM_BINOC,
	ZOOM_SNIPER,
	ZOOM_SNOOPER,
	ZOOM_FG42SCOPE,
	ZOOM_MG42,
	ZOOM_MAX_ZOOMS
} EZoom_t;

typedef enum
{
	ZOOM_OUT,					// widest angle
	ZOOM_IN						// tightest angle (approaching 0)
} EZoomInOut_t;

extern float    zoomTable[ZOOM_MAX_ZOOMS][2];

//----(SA)  end

typedef enum
{
	LE_MARK,
	LE_EXPLOSION,
	LE_SPRITE_EXPLOSION,
	LE_FRAGMENT,
	LE_MOVE_SCALE_FADE,
	LE_FALL_SCALE_FADE,
	LE_FADE_RGB,
	LE_SCALE_FADE,
	LE_SPARK,
	LE_DEBRIS,
	LE_BLOOD,
	LE_FUSE_SPARK,
//  LE_ZOMBIE_SPIRIT,
//  LE_ZOMBIE_BAT,
	LE_MOVING_TRACER,
	LE_EMITTER
} leType_t;

typedef enum
{
	LEF_PUFF_DONT_SCALE = 0x0001	// do not scale size over time
		, LEF_TUMBLE = 0x0002	// tumble over time, used for ejecting shells
		, LEF_NOFADEALPHA = 0x0004	// Ridah, sparks
		, LEF_SMOKING = 0x0008	// (SA) smoking
		, LEF_TUMBLE_SLOW = 0x0010	// slow down tumble on hitting ground
} leFlag_t;

typedef enum
{
	LEMT_NONE,
	LEMT_BLOOD
} leMarkType_t;					// fragment local entities can leave marks on walls

typedef enum
{
	LEBS_NONE,
	LEBS_BLOOD,
	LEBS_ROCK,
	LEBS_WOOD,
	LEBS_BRASS,
	LEBS_METAL,
	LEBS_BONE
} leBounceSoundType_t;			// fragment local entities can make sounds on impacts

typedef struct localEntity_s
{
	struct localEntity_s *prev, *next;
	leType_t        leType;
	int             leFlags;

	int             startTime;
	int             endTime;
	int             fadeInTime;

	float           lifeRate;	// 1.0 / (endTime - startTime)

	trajectory_t    pos;
	trajectory_t    angles;

	float           bounceFactor;	// 0.0 = no bounce, 1.0 = perfect

	float           color[4];

	float           radius;

	float           light;
	vec3_t          lightColor;

	leMarkType_t    leMarkType;	// mark to leave on fragment impact
	leBounceSoundType_t leBounceSoundType;

	refEntity_t     refEntity;

	// Ridah
	int             lightOverdraw;
	int             lastTrailTime;
	int             headJuncIndex, headJuncIndex2;
	float           effectWidth;
	int             effectFlags;
	struct localEntity_s *chain;	// used for grouping entities (like for flamethrower junctions)
	int             onFireStart, onFireEnd;
	int             ownerNum;
	int             lastSpiritDmgTime;

	int             loopingSound;

	int             breakCount;	// break-up this many times before we can break no more
	float           sizeScale;
	// done.

} localEntity_t;

//======================================================================


typedef struct
{
	int             client;
	int             score;
	int             ping;
	int             time;
	int             powerUps;
	int             team;
	int             playerClass;	// NERVE - SMF
	int             respawnsLeft;	// NERVE - SMF
} score_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define MAX_CUSTOM_SOUNDS   32
typedef struct clientInfo_s
{
	qboolean        infoValid;

	int             clientNum;

	char            name[MAX_QPATH];
	char            cleanname[MAX_QPATH];
	team_t          team;

	int             botSkill;	// 0 = not bot, 1-5 = bot
	int             score;		// updated by score servercmds
	int             location[2];	// location in 2d for team mode
	int             health;		// you only get this info about your teammates
	int             curWeapon;
	int             powerups;	// so can display quad/flag status
	int             breathPuffTime;
	int             cls;
	int             blinkTime;	//----(SA)

	int             handshake;
	int             rank;
	qboolean        ccSelected;
	int             fireteam;
	int             medals[SK_NUM_SKILLS];
	int             skill[SK_NUM_SKILLS];
	int             skillpoints[SK_NUM_SKILLS];	// filled OOB by +wstats

	char            disguiseName[MAX_QPATH];
	int             disguiseRank;

	int             weapon;
	int             secondaryweapon;
	int             latchedweapon;

	int             refStatus;

	bg_character_t *character;

	// Gordon: caching fireteam pointer here, better than trying to work it out all the time
	fireteamData_t *fireteamData;

	// Gordon: for fireteams, has been selected
	qboolean        selected;

	// Gordon: Intermission stats
	int             totalWeapAcc;
	int             kills;
	int             deaths;

	// OSP - per client MV ps info
	int             ammo;
	int             ammoclip;
	int             chargeTime;
	qboolean        fCrewgun;
	int             cursorHint;
	int             grenadeTimeLeft;	// Actual time remaining
	int             grenadeTimeStart;	// Time trigger base to compute TimeLeft
	int             hintTime;
	int             sprintTime;
	int             weapHeat;
	int             weaponState;
	int             weaponState_last;
} clientInfo_t;

typedef enum
{
	W_PART_1,
	W_PART_2,
	W_PART_3,
	W_PART_4,
	W_PART_5,
	W_PART_6,
	W_PART_7,
	W_MAX_PARTS
} barrelType_t;

typedef enum
{
	W_TP_MODEL,					//  third person model
	W_FP_MODEL,					//  first person model
	W_PU_MODEL,					//  pickup model
	W_NUM_TYPES
} modelViewType_t;

typedef struct partModel_s
{
	char            tagName[MAX_QPATH];
	qhandle_t       model;
	qhandle_t       skin[3];	// 0: neutral, 1: axis, 2: allied
} partModel_t;

typedef struct weaponModel_s
{
	qhandle_t       model;
	qhandle_t       skin[3];	// 0: neutral, 1: axis, 2: allied
} weaponModel_t;

// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s
{
	qboolean        registered;

	animation_t     weapAnimations[MAX_WP_ANIMATIONS];

	qhandle_t       handsModel;	// the hands don't actually draw, they just position the weapon

	qhandle_t       standModel;	// not drawn.  tags used for positioning weapons for pickup
	qboolean        droppedAnglesHack;

	weaponModel_t   weaponModel[W_NUM_TYPES];
	partModel_t     partModels[W_NUM_TYPES][W_MAX_PARTS];
	qhandle_t       flashModel[W_NUM_TYPES];
	qhandle_t       modModels[6];	// like the scope for the rifles

	vec3_t          flashDlightColor;
	sfxHandle_t     flashSound[4];	// fast firing weapons randomly choose
	sfxHandle_t     flashEchoSound[4];	//----(SA)  added - distant gun firing sound
	sfxHandle_t     lastShotSound[4];	// sound of the last shot can be different (mauser doesn't have bolt action on last shot for example)

	qhandle_t       weaponIcon[2];	//----(SA)  [0] is weap icon, [1] is highlight icon
	qhandle_t       ammoIcon;

	qhandle_t       missileModel;
	qhandle_t       missileAlliedSkin;
	qhandle_t       missileAxisSkin;
	sfxHandle_t     missileSound;
	void            (*missileTrailFunc) (centity_t *, const struct weaponInfo_s * wi);
	float           missileDlight;
	vec3_t          missileDlightColor;
	int             missileRenderfx;

	void            (*ejectBrassFunc) (centity_t *);

	sfxHandle_t     readySound;	// an amibient sound the weapon makes when it's /not/ firing
	sfxHandle_t     firingSound;
	sfxHandle_t     overheatSound;
	sfxHandle_t     reloadSound;
	sfxHandle_t     reloadFastSound;

	sfxHandle_t     spinupSound;	//----(SA)  added // sound started when fire button goes down, and stepped on when the first fire event happens
	sfxHandle_t     spindownSound;	//----(SA)  added // sound called if the above is running but player doesn't follow through and fire

	sfxHandle_t     switchSound;
} weaponInfo_t;


// each IT_* item has an associated itemInfo_t
// that constains media references necessary to present the
// item and its effects
typedef struct
{
	qboolean        registered;
	qhandle_t       models[MAX_ITEM_MODELS];
	qhandle_t       icons[MAX_ITEM_ICONS];
} itemInfo_t;


typedef struct
{
	int             itemNum;
} powerupInfo_t;

#define MAX_VIEWDAMAGE  8
typedef struct
{
	int             damageTime, damageDuration;
	float           damageX, damageY, damageValue;
} viewDamage_t;

#define MAX_REWARDSTACK     5

//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS    16

#define MAX_SPAWN_VARS          64
#define MAX_SPAWN_VARS_CHARS    2048


#define MAX_SPAWNPOINTS 32
#define MAX_SPAWNDESC   128

#define MAX_BUFFERED_SOUNDSCRIPTS 16

#define MAX_SOUNDSCRIPT_SOUNDS 16

typedef struct soundScriptHandle_s
{
	char            filename[MAX_QPATH];
	sfxHandle_t     sfxHandle;
} soundScriptHandle_t;

typedef struct soundScriptSound_s
{
	soundScriptHandle_t sounds[MAX_SOUNDSCRIPT_SOUNDS];

	int             numsounds;
	int             lastPlayed;

	struct soundScriptSound_s *next;
} soundScriptSound_t;

typedef struct soundScript_s
{
	int             index;
	char            name[MAX_QPATH];
	int             channel;
	int             attenuation;
	qboolean        streaming;
	qboolean        looping;
	qboolean        random;		// TODO
	int             numSounds;
	soundScriptSound_t *soundList;	// pointer into the global list of soundScriptSounds (defined below)

	struct soundScript_s *nextHash;	// next soundScript in our hashTable list position
} soundScript_t;

typedef struct
{
	int             x, y, z;
	int             yaw;
	int             data;
	char            type;
//  int             status;

//  qboolean        selected;

	vec2_t          transformed;
	vec2_t          automapTransformed;

	team_t          team;
} mapEntityData_t;

// START    xkan, 8/29/2002
// the most buddies we can have
#define MAX_NUM_BUDDY  6
// END      xkan, 8/29/2002

typedef enum
{
	SHOW_OFF,
	SHOW_SHUTDOWN,
	SHOW_ON
} showView_t;

void            CG_ParseMapEntityInfo(int axis_number, int allied_number);

#define MAX_BACKUP_STATES ( CMD_BACKUP + 2 )

typedef struct
{
	int             clientFrame;	// incremented each frame

	int             clientNum;
	int             xp;
	int             xpChangeTime;

	qboolean        demoPlayback;
	qboolean        loading;	// don't defer players at initial startup
	qboolean        intermissionStarted;	// don't play voice rewards, because game will end shortly

	// there are only one or two snapshot_t that are relevent at a time
	int             latestSnapshotNum;	// the number of snapshots the client system has received
	int             latestSnapshotTime;	// the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t     *snap;		// cg.snap->serverTime <= cg.time
	snapshot_t     *nextSnap;	// cg.nextSnap->serverTime > cg.time, or NULL
	snapshot_t      activeSnapshots[2];

	float           frameInterpolation;	// (float)( cg.time - cg.frame->serverTime ) / (cg.nextFrame->serverTime - cg.frame->serverTime)

	qboolean        thisFrameTeleport;
	qboolean        nextFrameTeleport;

	int             frametime;	// cg.time - cg.oldTime

	int             time;		// this is the time value that the client
	// is rendering at.
	int             oldTime;	// time at last frame, used for missile trails and prediction checking

	int             physicsTime;	// either cg.snap->time or cg.nextSnap->time

	int             timelimitWarnings;	// 5 min, 1 min, overtime

	qboolean        mapRestart;	// set on a map restart to set back the weapon

	qboolean        renderingThirdPerson;	// during deaths, chasecams, etc

	// prediction state
	qboolean        hyperspace;	// true if prediction has hit a trigger_teleport
	playerState_t   predictedPlayerState;
	centity_t       predictedPlayerEntity;
	qboolean        validPPS;	// clear until the first call to CG_PredictPlayerState
	int             predictedErrorTime;
	vec3_t          predictedError;

	int             eventSequence;
	int             predictableEvents[MAX_PREDICTED_EVENTS];

	float           stepChange;	// for stair up smoothing
	int             stepTime;

	float           duckChange;	// for duck viewheight smoothing
	int             duckTime;

	float           landChange;	// for landing hard
	int             landTime;

	// input state sent to server
	int             weaponSelect;

	// auto rotating items
	vec3_t          autoAnglesSlow;
	vec3_t          autoAxisSlow[3];
	vec3_t          autoAngles;
	vec3_t          autoAxis[3];
	vec3_t          autoAnglesFast;
	vec3_t          autoAxisFast[3];

	// view rendering
	refdef_t        refdef;
	vec3_t          refdefViewAngles;	// will be converted to refdef.viewaxis

	// zoom key
	qboolean        zoomed;
	qboolean        zoomedBinoc;
	int             zoomedScope;	//----(SA)  changed to int
	int             zoomTime;
	float           zoomSensitivity;
	float           zoomval;


	// information screen text during loading
	char            infoScreenText[MAX_STRING_CHARS];

	// scoreboard
	int             scoresRequestTime;
	int             numScores;
	int             selectedScore;
	int             teamScores[2];
	int             teamPlayers[TEAM_NUM_TEAMS];	// JPW NERVE for scoreboard
	score_t         scores[MAX_CLIENTS];
	qboolean        showScores;
	qboolean        scoreBoardShowing;
	int             scoreFadeTime;
	char            killerName[MAX_NAME_LENGTH];
	char            spectatorList[MAX_STRING_CHARS];	// list of names
	int             spectatorLen;	// length of list
	float           spectatorWidth;	// width in device units
	int             spectatorTime;	// next time to offset
	int             spectatorPaintX;	// current paint x
	int             spectatorPaintX2;	// current paint x
	int             spectatorOffset;	// current offset from start
	int             spectatorPaintLen;	// current offset from start

	//qboolean  showItems;
	//int           itemFadeTime;

	qboolean        lightstylesInited;

	// centerprinting
	int             centerPrintTime;
	int             centerPrintCharWidth;
	int             centerPrintY;
	char            centerPrint[1024];
	int             centerPrintLines;
	int             centerPrintPriority;	// NERVE - SMF

	// fade in/out
	int             fadeTime;
	float           fadeRate;
	vec4_t          fadeColor1;
	vec4_t          fadeColor2;

	// game stats
	int             exitStatsTime;
	int             exitStatsFade;

	// low ammo warning state
	int             lowAmmoWarning;	// 1 = low, 2 = empty

	// kill timers for carnage reward
	int             lastKillTime;

	// crosshair client ID
	int             crosshairClientNum;
	int             crosshairClientTime;

	qboolean        crosshairNotLookingAtClient;
	int             crosshairSPClientTime;
	int             crosshairVerticalShift;
	qboolean        crosshairClientNoShoot;
	qboolean        crosshairTerrain;

	int             teamFirstBlood;	// 0: allies 1: axis -1: nobody
	int             teamWonRounds[2];

	qboolean        filtercams;

	int             crosshairPowerupNum;
	int             crosshairPowerupTime;

//  int         identifyClientNum;          // NERVE - SMF
//  int         identifyClientHealth;       // NERVE - SMF
	int             identifyClientRequest;	// NERVE - SMF

//----(SA)  added
	// cursorhints
	int             cursorHintIcon;
	int             cursorHintTime;
	int             cursorHintFade;
	int             cursorHintValue;
//----(SA)  end

	// powerup active flashing
	int             powerupActive;
	int             powerupTime;

	// attacking player
	int             attackerTime;
	int             voiceTime;

	// reward tmedals
	int             rewardStack;
	int             rewardTime;
	int             rewardCount[MAX_REWARDSTACK];
	qhandle_t       rewardShader[MAX_REWARDSTACK];
	qhandle_t       rewardSound[MAX_REWARDSTACK];

	// warmup countdown
	int             warmup;
	int             warmupCount;

	//==========================

	int             itemPickup;
	int             itemPickupTime;
	int             itemPickupBlendTime;	// the pulse around the crosshair is timed seperately

	int             weaponSelectTime;
	int             weaponAnimation;
	int             weaponAnimationTime;

	// blend blobs
	viewDamage_t    viewDamage[MAX_VIEWDAMAGE];
	float           damageTime;	// last time any kind of damage was recieved
	int             damageIndex;	// slot that was filled in
	float           damageX, damageY, damageValue;

	int             grenLastTime;

	int             switchbackWeapon;
	int             lastFiredWeapon;
	int             lastFiredWeaponTime;
	int             painTime;
	int             weaponFireTime;
	int             nextIdleTime;
	int             lastIdleTimeEnd;
	int             idleAnim;
	int             lastWeapSelInBank[MAX_WEAP_BANKS_MP];	// remember which weapon was last selected in a bank for 'weaponbank' commands //----(SA)   added
// JPW FIXME NOTE: max_weap_banks > max_weap_banks_mp so this should be OK, but if that changes, change this too

	// status bar head
	float           headYaw;
	float           headEndPitch;
	float           headEndYaw;
	int             headEndTime;
	float           headStartPitch;
	float           headStartYaw;
	int             headStartTime;

	// view movement
	float           v_dmg_time;
	float           v_dmg_pitch;
	float           v_dmg_roll;

	vec3_t          kick_angles;	// weapon kicks
	vec3_t          kick_origin;

	// RF, view flames when getting burnt
	int             v_fireTime, v_noFireTime;
	vec3_t          v_fireRiseDir;

	// temp working variables for player view
	float           bobfracsin;
	int             bobcycle;
	float           lastvalidBobfracsin;
	int             lastvalidBobcycle;
	float           xyspeed;
	int             nextOrbitTime;

	// development tool
	refEntity_t     testModelEntity;
	char            testModelName[MAX_QPATH];
	qboolean        testGun;

	// RF, new kick angles
	vec3_t          kickAVel;	// for damage feedback, weapon recoil, etc
	// This is the angular velocity, to give a smooth
	// rotational feedback, rather than sudden jerks
	vec3_t          kickAngles;	// for damage feedback, weapon recoil, etc
	// NOTE: this is not transmitted through MSG.C stream
	// since weapon kicks are client-side, and damage feedback
	// is rare enough that we can transmit that as an event
	float           recoilPitch, recoilPitchAngle;

	// Duffy
	qboolean        cameraMode;	// if rendering from a camera
	// Duffy end

	// NERVE - SMF - Objective info display
	qboolean        limboMenu;

	int             oidTeam;
	int             oidPrintTime;
	int             oidPrintCharWidth;
	int             oidPrintY;
	char            oidPrint[1024];
	int             oidPrintLines;

	// for voice chat buffer
	int             voiceChatTime;
	int             voiceChatBufferIn;
	int             voiceChatBufferOut;

	int             newCrosshairIndex;
	qhandle_t       crosshairShaderAlt[NUM_CROSSHAIRS];

	int             cameraShakeTime;
	float           cameraShakePhase;
	float           cameraShakeScale;
	float           cameraShakeLength;

	qboolean        latchAutoActions;
	qboolean        latchVictorySound;
	// -NERVE - SMF

	// spawn variables
	qboolean        spawning;	// the CG_Spawn*() functions are valid
	int             numSpawnVars;
	char           *spawnVars[MAX_SPAWN_VARS][2];	// key / value pairs
	int             numSpawnVarChars;
	char            spawnVarChars[MAX_SPAWN_VARS_CHARS];

	vec2_t          mapcoordsMins;
	vec2_t          mapcoordsMaxs;
	vec2_t          mapcoordsScale;
	qboolean        mapcoordsValid;


	int             numMiscGameModels;


	qboolean        showCampaignBriefing;
	qboolean        showGameView;
	qboolean        showFireteamMenu;

	char            spawnPoints[MAX_SPAWNPOINTS][MAX_SPAWNDESC];
	vec3_t          spawnCoordsUntransformed[MAX_SPAWNPOINTS];
	vec3_t          spawnCoords[MAX_SPAWNPOINTS];
	team_t          spawnTeams[MAX_SPAWNPOINTS];
	team_t          spawnTeams_old[MAX_SPAWNPOINTS];
	int             spawnTeams_changeTime[MAX_SPAWNPOINTS];
	int             spawnPlayerCounts[MAX_SPAWNPOINTS];
	int             spawnCount;
	int             selectedSpawnPoint;

	cg_string_t     aStringPool[MAX_STRINGS];
	int             demohelpWindow;
	cg_window_t    *motdWindow;
	cg_window_t    *msgWstatsWindow;
	cg_window_t    *msgWtopshotsWindow;
	int             mv_cnt;		// Number of active MV windows
	int             mvClientList;	// Cached client listing of who is merged
	cg_window_t    *mvCurrentActive;	// Client ID of current active window (-1 = none)
	cg_window_t    *mvCurrentMainview;	// Client ID used in the main display (should always be set if mv_cnt > 0)
	cg_mvinfo_t     mvOverlay[MAX_MVCLIENTS];	// Cached info for MV overlay
	int             mvTeamList[TEAM_NUM_TEAMS][MAX_MVCLIENTS];
	int             mvTotalClients;	// Total # of clients available for MV processing
	int             mvTotalTeam[TEAM_NUM_TEAMS];
	refdef_t       *refdef_current;	// Handling of some drawing elements for MV
	qboolean        showStats;
	int             spechelpWindow;
	int             statsRequestTime;
	cg_window_t    *statsWindow;
	int             topshotsRequestTime;
	cg_window_t    *topshotsWindow;
	cg_window_t    *windowCurrent;	// Current window to update.. a bit of a hack :p
	cg_windowHandler_t winHandler;
	vec4_t          xhairColor;
	vec4_t          xhairColorAlt;

	// Arnout: allow overriding of countdown sounds
	char            fiveMinuteSound_g[MAX_QPATH];
	char            fiveMinuteSound_a[MAX_QPATH];
	char            twoMinuteSound_g[MAX_QPATH];
	char            twoMinuteSound_a[MAX_QPATH];
	char            thirtySecondSound_g[MAX_QPATH];
	char            thirtySecondSound_a[MAX_QPATH];

	pmoveExt_t      pmext;

	int             numOIDtriggers2;
	char            oidTriggerInfoAllies[MAX_OID_TRIGGERS][256];
	char            oidTriggerInfoAxis[MAX_OID_TRIGGERS][256];

	int             ltChargeTime[2];
	int             soldierChargeTime[2];
	int             engineerChargeTime[2];
	int             medicChargeTime[2];
	int             covertopsChargeTime[2];
	// START    xkan, 8/29/2002
	// which bots are currently selected
	int             selectedBotClientNumber[MAX_NUM_BUDDY];
	// END      xkan, 8/29/2002
	int             binocZoomTime;
	int             limboEndCinematicTime;
	int             proneMovingTime;
	fireteamData_t  fireTeams[32];

	// TAT 10/23/2002
	//      For the bot hud, we keep a bit mask for which bot_action icons to show
	int             botMenuIcons;
	//      And we need to know which one is the selected one
	int             botSelectedCommand;

	// START Mad Doc - TDF
	int             orderFade;
	int             orderTime;
	// END Mad Doc - TDF

	centity_t      *satchelCharge;

	playerState_t   backupStates[MAX_BACKUP_STATES];
	int             backupStateTop;
	int             backupStateTail;
	int             lastPredictedCommand;
	int             lastPhysicsTime;

	qboolean        skyboxEnabled;
	vec3_t          skyboxViewOrg;
	vec_t           skyboxViewFov;

	vec3_t          tankflashorg;

	qboolean        editingSpeakers;

	qboolean        serverRespawning;

	// mortar hud
	vec2_t          mortarFireAngles;
	int             mortarImpactTime;
	vec3_t          mortarImpactPos;
	qboolean        mortarImpactOutOfMap;

	// artillery requests
	vec3_t          artilleryRequestPos[MAX_CLIENTS];
	int             artilleryRequestTime[MAX_CLIENTS];

	soundScript_t  *bufferSoundScripts[MAX_BUFFERED_SOUNDSCRIPTS];
	int             bufferedSoundScriptEndTime;
	int             numbufferedSoundScripts;

	char            objMapDescription_Axis[384];
	char            objMapDescription_Allied[384];
	char            objMapDescription_Neutral[384];
	char            objDescription_Axis[MAX_OBJECTIVES][256];
	char            objDescription_Allied[MAX_OBJECTIVES][256];

	int             waterundertime;
	qhandle_t       hudInWaterShader;
} cg_t;

#define NUM_FUNNEL_SPRITES  21

#define MAX_LOCKER_DEBRIS   5

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, weaponInfo_t, and powerupInfo_t
typedef struct
{
	qhandle_t       charsetShader;
	// JOSEPH 4-17-00
	qhandle_t       menucharsetShader;
	// END JOSEPH
	qhandle_t       charsetProp;
	qhandle_t       charsetPropGlow;
	qhandle_t       charsetPropB;
	qhandle_t       whiteShader;

	qhandle_t       armorModel;

// JPW NERVE
	qhandle_t       hudSprintBar;
	qhandle_t       hudAxisHelmet;
	qhandle_t       hudAlliedHelmet;
	qhandle_t       redColorBar;
	qhandle_t       blueColorBar;
// jpw
	qhandle_t       teamStatusBar;

	qhandle_t       deferShader;

	// gib explosions
	qhandle_t       gibAbdomen;
	qhandle_t       gibArm;
	qhandle_t       gibChest;
	qhandle_t       gibFist;
	qhandle_t       gibFoot;
	qhandle_t       gibForearm;
	qhandle_t       gibIntestine;
	qhandle_t       gibLeg;
	qhandle_t       gibSkull;
	qhandle_t       gibBrain;

	// debris
	qhandle_t       debBlock[6];
	qhandle_t       debRock[3];
	qhandle_t       debFabric[3];
	qhandle_t       debWood[6];

	qhandle_t       targetEffectExplosionShader;

	qhandle_t       machinegunBrassModel;
	qhandle_t       panzerfaustBrassModel;	//----(SA)  added

	// Rafael
	qhandle_t       smallgunBrassModel;

	qhandle_t       shotgunBrassModel;

	qhandle_t       railRingsShader;
	qhandle_t       railCoreShader;
	qhandle_t       ropeShader;

	qhandle_t       lightningShader;

	qhandle_t       friendShader;

	qhandle_t       spawnInvincibleShader;
	qhandle_t       scoreEliminatedShader;

	qhandle_t       medicReviveShader;
	qhandle_t       voiceChatShader;
	qhandle_t       balloonShader;
	qhandle_t       objectiveShader;

	qhandle_t       vehicleShader;
	qhandle_t       destroyShader;

//  qhandle_t   selectShader;
	qhandle_t       viewBloodShader;
	qhandle_t       tracerShader;
	qhandle_t       crosshairShader[NUM_CROSSHAIRS];
	qhandle_t       lagometerShader;
	qhandle_t       backTileShader;
	qhandle_t       noammoShader;

	qhandle_t       reticleShader;
	qhandle_t       reticleShaderSimple;
	qhandle_t       snooperShader;
//  qhandle_t   snooperShaderSimple;
	qhandle_t       binocShader;
	qhandle_t       binocShaderSimple;
// JPW NERVE
	qhandle_t       fleshSmokePuffShader;	// JPW NERVE for bullet hit flesh smoke puffs
	qhandle_t       nerveTestShader;
	qhandle_t       idTestShader;
	qhandle_t       hud1Shader;
	qhandle_t       hud2Shader;
	qhandle_t       hud3Shader;
	qhandle_t       hud4Shader;
	qhandle_t       hud5Shader;
// jpw
	qhandle_t       smokePuffShader;
	qhandle_t       smokePuffRageProShader;
	qhandle_t       shotgunSmokePuffShader;
	qhandle_t       waterBubbleShader;
	qhandle_t       bloodTrailShader;

//  qhandle_t   nailPuffShader;

//----(SA)  cursor hints
	// would be nice to specify these in the menu scripts instead of permanent handles...
	qhandle_t       usableHintShader;
	qhandle_t       notUsableHintShader;
	qhandle_t       doorHintShader;
	qhandle_t       doorRotateHintShader;
	qhandle_t       doorLockHintShader;
	qhandle_t       doorRotateLockHintShader;
	qhandle_t       mg42HintShader;
	qhandle_t       breakableHintShader;
	qhandle_t       chairHintShader;
	qhandle_t       alarmHintShader;
	qhandle_t       healthHintShader;
	qhandle_t       treasureHintShader;
	qhandle_t       knifeHintShader;
	qhandle_t       ladderHintShader;
	qhandle_t       buttonHintShader;
	qhandle_t       waterHintShader;
	qhandle_t       cautionHintShader;
	qhandle_t       dangerHintShader;
	qhandle_t       secretHintShader;
	qhandle_t       qeustionHintShader;
	qhandle_t       exclamationHintShader;
	qhandle_t       clipboardHintShader;
	qhandle_t       weaponHintShader;
	qhandle_t       ammoHintShader;
	qhandle_t       armorHintShader;
	qhandle_t       powerupHintShader;
	qhandle_t       holdableHintShader;
	qhandle_t       inventoryHintShader;

	qhandle_t       hintPlrFriendShader;
	qhandle_t       hintPlrNeutralShader;
	qhandle_t       hintPlrEnemyShader;
	qhandle_t       hintPlrUnknownShader;

	// DHM - Nerve :: Multiplayer hints
	qhandle_t       buildHintShader;
	qhandle_t       disarmHintShader;
	qhandle_t       reviveHintShader;
	qhandle_t       dynamiteHintShader;
	// dhm - end


	qhandle_t       tankHintShader;
	qhandle_t       satchelchargeHintShader;
	qhandle_t       uniformHintShader;
	qhandle_t       waypointAttackShader;
	qhandle_t       waypointDefendShader;
	qhandle_t       waypointRegroupShader;
	// TAT 8/29/2002 - a shader for the bot indicator
	qhandle_t       waypointBotShader;
	//      for a queued bot order (Listen Up/Go Go Go)
	qhandle_t       waypointBotQueuedShader;
	qhandle_t       waypointCompassAttackShader;
	qhandle_t       waypointCompassDefendShader;
	qhandle_t       waypointCompassRegroupShader;
	qhandle_t       commandCentreWoodShader;
	qhandle_t       commandCentreMapShader[MAX_COMMANDMAP_LAYERS];
	qhandle_t       commandCentreMapShaderTrans[MAX_COMMANDMAP_LAYERS];
	qhandle_t       commandCentreAutomapShader[MAX_COMMANDMAP_LAYERS];
	qhandle_t       commandCentreAutomapMaskShader;
	qhandle_t       commandCentreAutomapBorderShader;
	qhandle_t       commandCentreAutomapBorder2Shader;
	qhandle_t       commandCentreAutomapCornerShader;
	qhandle_t       commandCentreAxisMineShader;
	qhandle_t       commandCentreAlliedMineShader;
	qhandle_t       commandCentreSpawnShader[2];

	// Mad Doc - TDF
	qhandle_t       ingameAutomapBackground;

	qhandle_t       landmineHintShader;
	qhandle_t       compassConstructShader;
	qhandle_t       compassDestroyShader;
	qhandle_t       buddyShader;
	qhandle_t       hudBorderVert;
	qhandle_t       hudBorderVert2;

	qhandle_t       waypointMarker;

	qhandle_t       slashShader;
	qhandle_t       compassShader;
	qhandle_t       compass2Shader;

	// Rafael
	qhandle_t       snowShader;
	qhandle_t       oilParticle;
	qhandle_t       oilSlick;
	// done.

	// Rafael - cannon
	qhandle_t       smokePuffShaderdirty;
	qhandle_t       smokePuffShaderb1;
	qhandle_t       smokePuffShaderb2;
	qhandle_t       smokePuffShaderb3;
	qhandle_t       smokePuffShaderb4;
	qhandle_t       smokePuffShaderb5;
	// done

	// Rafael - blood pool
	qhandle_t       bloodPool;

	// Ridah, viewscreen blood animation
	qhandle_t       viewBloodAni[5];
	qhandle_t       viewFlashBlood;
	qhandle_t       viewFlashFire[16];
	// done

	// Rafael shards
	qhandle_t       shardGlass1;
	qhandle_t       shardGlass2;
	qhandle_t       shardWood1;
	qhandle_t       shardWood2;
	qhandle_t       shardMetal1;
	qhandle_t       shardMetal2;
//  qhandle_t   shardCeramic1;
//  qhandle_t   shardCeramic2;
	// done

	qhandle_t       shardRubble1;
	qhandle_t       shardRubble2;
	qhandle_t       shardRubble3;


	qhandle_t       shardJunk[MAX_LOCKER_DEBRIS];

	qhandle_t       numberShaders[11];

	qhandle_t       shadowFootShader;
	qhandle_t       shadowTorsoShader;

	// wall mark shaders
	qhandle_t       wakeMarkShader;
	qhandle_t       wakeMarkShaderAnim;
	qhandle_t       bloodMarkShaders[5];
	qhandle_t       bloodDotShaders[5];
	qhandle_t       bulletMarkShader;
	qhandle_t       bulletMarkShaderMetal;
	qhandle_t       bulletMarkShaderWood;
	qhandle_t       bulletMarkShaderGlass;
	qhandle_t       burnMarkShader;

	qhandle_t       flamebarrel;
	qhandle_t       mg42muzzleflash;

	qhandle_t       waterSplashModel;
	qhandle_t       waterSplashShader;

	qhandle_t       thirdPersonBinocModel;	//----(SA)  added

	// weapon effect shaders
	qhandle_t       railExplosionShader;
	qhandle_t       bulletExplosionShader;
	qhandle_t       rocketExplosionShader;
	qhandle_t       grenadeExplosionShader;
	qhandle_t       bfgExplosionShader;
	qhandle_t       bloodExplosionShader;

	// special effects models
	qhandle_t       teleportEffectModel;
	qhandle_t       teleportEffectShader;

	// Ridah
	qhandle_t       bloodCloudShader;
	qhandle_t       sparkParticleShader;
	qhandle_t       smokeTrailShader;
	qhandle_t       fireTrailShader;
	//qhandle_t lightningBoltShader;
	qhandle_t       flamethrowerFireStream;
	qhandle_t       flamethrowerBlueStream;
	qhandle_t       flamethrowerFuelStream;
	qhandle_t       flamethrowerFuelShader;
	qhandle_t       onFireShader, onFireShader2;
	qhandle_t       viewFadeBlack;
	qhandle_t       sparkFlareShader;
	qhandle_t       funnelFireShader[NUM_FUNNEL_SPRITES];
	qhandle_t       spotLightShader;
	qhandle_t       spotLightBeamShader;
	qhandle_t       bulletParticleTrailShader;
	qhandle_t       smokeParticleShader;

	// DHM - Nerve :: bullet hitting dirt
	qhandle_t       dirtParticle1Shader;
	qhandle_t       dirtParticle2Shader;
	qhandle_t       dirtParticle3Shader;

	qhandle_t       genericConstructionShader;
	//qhandle_t genericConstructionShaderBrush;
	//qhandle_t genericConstructionShaderModel;
	qhandle_t       alliedUniformShader;
	qhandle_t       axisUniformShader;

	sfxHandle_t     sfx_artilleryExp[3];
	sfxHandle_t     sfx_artilleryDist;

	sfxHandle_t     sfx_airstrikeExp[3];
	sfxHandle_t     sfx_airstrikeDist;

	// sounds
	sfxHandle_t     noFireUnderwater;
	sfxHandle_t     selectSound;
	sfxHandle_t     landHurt;

	sfxHandle_t     footsteps[FOOTSTEP_TOTAL][4];
	sfxHandle_t     sfx_rockexp;
	sfxHandle_t     sfx_rockexpDist;
	sfxHandle_t     sfx_rockexpWater;
	sfxHandle_t     sfx_satchelexp;
	sfxHandle_t     sfx_satchelexpDist;
	sfxHandle_t     sfx_landmineexp;
	sfxHandle_t     sfx_landmineexpDist;
	sfxHandle_t     sfx_mortarexp[4];
	sfxHandle_t     sfx_mortarexpDist;
	sfxHandle_t     sfx_grenexp;
	sfxHandle_t     sfx_grenexpDist;
	sfxHandle_t     sfx_brassSound[BRASSSOUND_MAX][3];
	sfxHandle_t     sfx_rubbleBounce[3];

	sfxHandle_t     sfx_bullet_fleshhit[5];
	sfxHandle_t     sfx_bullet_metalhit[5];
	sfxHandle_t     sfx_bullet_woodhit[5];
	sfxHandle_t     sfx_bullet_glasshit[5];
	sfxHandle_t     sfx_bullet_stonehit[5];
	sfxHandle_t     sfx_bullet_waterhit[5];

	sfxHandle_t     sfx_dynamiteexp;
	sfxHandle_t     sfx_dynamiteexpDist;
	sfxHandle_t     sfx_spearhit;
	sfxHandle_t     sfx_knifehit[5];
	sfxHandle_t     gibSound;
	sfxHandle_t     noAmmoSound;
	sfxHandle_t     landSound[FOOTSTEP_TOTAL];

	sfxHandle_t     fiveMinuteSound_g, fiveMinuteSound_a;
	sfxHandle_t     twoMinuteSound_g, twoMinuteSound_a;
	sfxHandle_t     thirtySecondSound_g, thirtySecondSound_a;

	sfxHandle_t     watrInSound;
	sfxHandle_t     watrOutSound;
	sfxHandle_t     watrUnSound;
	sfxHandle_t     watrGaspSound;

	sfxHandle_t     underWaterSound;
	sfxHandle_t     fireSound;
	sfxHandle_t     waterSound;

	sfxHandle_t     grenadePulseSound4;
	sfxHandle_t     grenadePulseSound3;
	sfxHandle_t     grenadePulseSound2;
	sfxHandle_t     grenadePulseSound1;
//  sfxHandle_t sparkSounds;

	// Ridah
	sfxHandle_t     flameSound;
	sfxHandle_t     flameBlowSound;
	sfxHandle_t     flameStartSound;
	sfxHandle_t     flameStreamSound;
	sfxHandle_t     flameCrackSound;
	sfxHandle_t     boneBounceSound;

	//sfxHandle_t grenadebounce1;
	//sfxHandle_t grenadebounce2;
	sfxHandle_t     grenadebounce[FOOTSTEP_TOTAL][2];

	sfxHandle_t     dynamitebounce1;	//----(SA)  added
	sfxHandle_t     landminebounce1;

	sfxHandle_t     fkickwall;
	sfxHandle_t     fkickflesh;

	sfxHandle_t     fkickmiss;

	int             bulletHitFleshScript;

	sfxHandle_t     satchelbounce1;

	qhandle_t       cursor;
	qhandle_t       selectCursor;
	qhandle_t       sizeCursor;

	sfxHandle_t     uniformPickup;
	sfxHandle_t     minePrimedSound;
	sfxHandle_t     buildSound[4];
	sfxHandle_t     buildDecayedSound;

	sfxHandle_t     sndLimboSelect;
	sfxHandle_t     sndLimboFocus;
	sfxHandle_t     sndLimboFilter;
	sfxHandle_t     sndLimboCancel;

	sfxHandle_t     sndRankUp;
	sfxHandle_t     sndSkillUp;

	sfxHandle_t     sndMedicCall[2];

	qhandle_t       ccStamps[2];
	qhandle_t       ccFilterPics[10];
	qhandle_t       ccFilterBackOn;
	qhandle_t       ccFilterBackOff;
	qhandle_t       ccPaper;
	qhandle_t       ccPaperConsole;
	qhandle_t       ccBars[3];
	qhandle_t       ccFlags[3];
	qhandle_t       ccLeather;
	//qhandle_t ccArrow;
	qhandle_t       ccPlayerHighlight;
	qhandle_t       ccConstructIcon[2];
	qhandle_t       ccCmdPost[2];
	qhandle_t       ccDestructIcon[3][2];
	qhandle_t       ccTankIcon;
	qhandle_t       skillPics[SK_NUM_SKILLS];
	qhandle_t       ccMortarHit;
	qhandle_t       ccMortarTarget;
	qhandle_t       ccMortarTargetArrow;

	qhandle_t       currentSquadBackground;
	qhandle_t       SPTeamOverlayUnitBackground;
	qhandle_t       SPTeamOverlayUnitSelected;
	qhandle_t       SPTeamOverlayBotOrders[BOT_ACTION_MAX];
	qhandle_t       SPTeamOverlayBotOrdersBkg;
	qhandle_t       SPPlayerInfoSpecialIcon;
	qhandle_t       SPPlayerInfoHealthIcon;
	qhandle_t       SPPlayerInfoStaminaIcon;
	qhandle_t       SPPlayerInfoAmmoIcon;

	// Gordon: for commandmap
	qhandle_t       medicIcon;

	qhandle_t       hWeaponSnd;
	qhandle_t       hWeaponEchoSnd;
	qhandle_t       hWeaponHeatSnd;

	qhandle_t       hWeaponSnd_2;
	qhandle_t       hWeaponEchoSnd_2;
	qhandle_t       hWeaponHeatSnd_2;

//  qhandle_t   hflakWeaponSnd;

	qhandle_t       hMountedMG42Base;	//  trap_R_RegisterModel( "models/mapobjects/tanks_sd/mg42nestbase.md3" );
	qhandle_t       hMountedMG42Nest;	//  trap_R_RegisterModel( "models/mapobjects/tanks_sd/mg42nest.md3" );
	qhandle_t       hMountedMG42;	//  trap_R_RegisterModel( "models/mapobjects/tanks_sd/mg42.md3" );
	qhandle_t       hMountedBrowning;
	qhandle_t       hMountedFPMG42;
	qhandle_t       hMountedFPBrowning;

	// Gordon: medals
	qhandle_t       medals[SK_NUM_SKILLS];
	qhandle_t       medal_back;

	// Gordon: new limbo stuff
	fontInfo_t      limboFont1;
	fontInfo_t      limboFont1_lo;
	fontInfo_t      limboFont2;
	qhandle_t       limboNumber_roll;
	qhandle_t       limboNumber_back;
	qhandle_t       limboStar_roll;
	qhandle_t       limboStar_back;
	qhandle_t       limboWeaponNumber_off;
	qhandle_t       limboWeaponNumber_on;
	qhandle_t       limboWeaponCard;
	qhandle_t       limboWeaponCardSurroundH;
	qhandle_t       limboWeaponCardSurroundV;
	qhandle_t       limboWeaponCardSurroundC;
	qhandle_t       limboWeaponCardOOS;
	qhandle_t       limboLight_on;
	qhandle_t       limboLight_on2;
	qhandle_t       limboLight_off;

	qhandle_t       limboClassButtons[NUM_PLAYER_CLASSES];
	//qhandle_t     limboClassButtonBack;

	qhandle_t       limboClassButton2Back_on;
	qhandle_t       limboClassButton2Back_off;
	qhandle_t       limboClassButton2Wedge_on;
	qhandle_t       limboClassButton2Wedge_off;
	qhandle_t       limboClassButtons2[NUM_PLAYER_CLASSES];

//  skill_back_on
//  skill_back_off
//  skill_4pieces
//  skill_glass_top_layer
//  skill_testicon

	qhandle_t       limboTeamButtonBack_on;
	qhandle_t       limboTeamButtonBack_off;
	qhandle_t       limboTeamButtonAllies;
	qhandle_t       limboTeamButtonAxis;
	qhandle_t       limboTeamButtonSpec;
	qhandle_t       limboBlendThingy;
	qhandle_t       limboWeaponBlendThingy;
	qhandle_t       limboSkillsLW;
	qhandle_t       limboSkillsBS;
	//qhandle_t     limboCursor_on;
	//qhandle_t     limboCursor_off;
	qhandle_t       limboCounterBorder;
	qhandle_t       limboWeaponCard1;
	qhandle_t       limboWeaponCard2;
	qhandle_t       limboWeaponCardArrow;
	qhandle_t       limboObjectiveBack[3];
	qhandle_t       limboClassBar;
	qhandle_t       limboBriefingButtonOn;
	qhandle_t       limboBriefingButtonOff;
	qhandle_t       limboBriefingButtonStopOn;
	qhandle_t       limboBriefingButtonStopOff;

	qhandle_t       limboSpectator;
	qhandle_t       limboRadioBroadcast;

	qhandle_t       cursorIcon;



	qhandle_t       hudPowerIcon;
	qhandle_t       hudSprintIcon;
	qhandle_t       hudHealthIcon;

	qhandle_t       pmImages[PM_NUM_TYPES];
	qhandle_t       pmImageAlliesConstruct;
	qhandle_t       pmImageAxisConstruct;
	qhandle_t       pmImageAlliesMine;
	qhandle_t       pmImageAxisMine;
	qhandle_t       hintKey;

	qhandle_t       hudDamagedStates[4];

	qhandle_t       browningIcon;

	qhandle_t       axisFlag;
	qhandle_t       alliedFlag;

	qhandle_t       disconnectIcon;

	qhandle_t       fireteamicons[6];
} cgMedia_t;

typedef struct
{
	char            lmsdescription[1024];
	char            description[1024];
	char            axiswintext[1024];
	char            alliedwintext[1024];
	char            longname[128];
	vec2_t          mappos;
} arenaInfo_t;

typedef struct
{
	char            campaignDescription[2048];
	char            campaignName[128];
	char            mapnames[MAX_MAPS_PER_CAMPAIGN][MAX_QPATH];
	vec2_t          mappos[MAX_MAPS_PER_CAMPAIGN];
	arenaInfo_t     arenas[MAX_MAPS_PER_CAMPAIGN];
	int             mapCount;
	int             current;
	vec2_t          mapTC[2];
} cg_campaignInfo_t;

#define MAX_COMMAND_INFO MAX_CLIENTS

#define MAX_STATIC_GAMEMODELS   1024

typedef struct cg_gamemodel_s
{
	qhandle_t       model;
	vec3_t          org;
	vec3_t          axes[3];
	vec_t           radius;
} cg_gamemodel_t;

typedef struct cg_weaponstats_s
{
	int             numKills;
	int             numHits;
	int             numShots;
} cg_weaponstats_t;

typedef struct
{
	char            strWS[WS_MAX][MAX_STRING_TOKENS];
	char            strExtra[2][MAX_STRING_TOKENS];
	char            strRank[MAX_STRING_TOKENS];
	char            strSkillz[SK_NUM_SKILLS][MAX_STRING_TOKENS];
	int             cWeapons;
	int             cSkills;
	qboolean        fHasStats;
	int             nClientID;
	int             nRounds;
	int             fadeTime;
	int             show;
	int             requestTime;
} gameStats_t;

typedef struct
{
	char            strWS[WS_MAX * 2][MAX_STRING_TOKENS];
	int             cWeapons;
	int             fadeTime;
	int             show;
	int             requestTime;
} topshotStats_t;

typedef struct oidInfo_s
{
	int             spawnflags;
	qhandle_t       customimageallies;
	qhandle_t       customimageaxis;
	int             entityNum;
	int             objflags;
	char            name[MAX_QPATH];
	vec3_t          origin;
} oidInfo_t;

#define NUM_ENDGAME_AWARDS 14


// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct
{
	gameState_t     gameState;	// gamestate from server
	glconfig_t      glconfig;	// rendering configuration
	float           screenXScale;	// derived from glconfig
	float           screenYScale;
	float           screenXBias;

	int             serverCommandSequence;	// reliable command stream counter
	int             processedSnapshotNum;	// the number of snapshots cgame has requested

	qboolean        localServer;	// detected on startup by checking sv_running

	// parsed from serverinfo
	gametype_t      gametype;
	int             antilag;

	float           timelimit;	// NERVE - SMF - made this a float
	int             maxclients;
	char            mapname[MAX_QPATH];
	char            rawmapname[MAX_QPATH];
	char            redTeam[MAX_QPATH];	// A team
	char            blueTeam[MAX_QPATH];	// B team
	float           weaponRestrictions;

	int             voteTime;
	int             voteYes;
	int             voteNo;
	qboolean        voteModified;	// beep whenever changed
	char            voteString[MAX_STRING_TOKENS];

	int             teamVoteTime[2];
	int             teamVoteYes[2];
	int             teamVoteNo[2];
	qboolean        teamVoteModified[2];	// beep whenever changed
	char            teamVoteString[2][MAX_STRING_TOKENS];

	int             levelStartTime;
	int             intermissionStartTime;

	//
	// locally derived information from gamestate
	//
	qhandle_t       gameModels[MAX_MODELS];
	char            gameShaderNames[MAX_CS_SHADERS][MAX_QPATH];
	qhandle_t       gameShaders[MAX_CS_SHADERS];
	qhandle_t       gameModelSkins[MAX_MODELS];
	bg_character_t *gameCharacters[MAX_CHARACTERS];
	sfxHandle_t     gameSounds[MAX_SOUNDS];

	int             numInlineModels;
	qhandle_t       inlineDrawModel[MAX_MODELS];
	vec3_t          inlineModelMidpoints[MAX_MODELS];

	clientInfo_t    clientinfo[MAX_CLIENTS];

	// teamchat width is *3 because of embedded color codes
	char            teamChatMsgs[TEAMCHAT_HEIGHT][TEAMCHAT_WIDTH * 3 + 1];
	int             teamChatMsgTimes[TEAMCHAT_HEIGHT];
	team_t          teamChatMsgTeams[TEAMCHAT_HEIGHT];
	int             teamChatPos;
	int             teamLastChatPos;

	// New notify mechanism for obits
	char            notifyMsgs[NOTIFY_HEIGHT][NOTIFY_WIDTH * 3 + 1];
	int             notifyMsgTimes[NOTIFY_HEIGHT];
	int             notifyPos;
	int             notifyLastPos;

	int             cursorX;
	int             cursorY;
	qboolean        eventHandling;
	qboolean        mouseCaptured;
	qboolean        sizingHud;
	void           *capturedItem;
	qhandle_t       activeCursor;

	// screen fading
	float           fadeAlpha, fadeAlphaCurrent;
	int             fadeStartTime;
	int             fadeDuration;

	// media
	cgMedia_t       media;

	// player/AI model scripting (client repository)
	animScriptData_t animScriptData;

	int             currentVoiceClient;
	int             currentRound;
	float           nextTimeLimit;
	int             minclients;
	gamestate_t     gamestate;
	char           *currentCampaign;
	int             currentCampaignMap;

	int             complaintClient;	// DHM - Nerve
	int             complaintEndTime;	// DHM - Nerve
	float           smokeWindDir;	// JPW NERVE for smoke puffs & wind (arty, airstrikes, bullet impacts)

	playerStats_t   playerStats;
	int             numOIDtriggers;
	int             teamobjectiveStats[MAX_OID_TRIGGERS];

	qboolean        campaignInfoLoaded;
	cg_campaignInfo_t campaignData;

	qboolean        arenaInfoLoaded;
	arenaInfo_t     arenaData;

	centity_t      *gameManager;

	int             ccLayers;
	int             ccLayerCeils[MAX_COMMANDMAP_LAYERS];
	float           ccZoomFactor;

	int             invitationClient;
	int             invitationEndTime;

	int             applicationClient;
	int             applicationEndTime;

	int             propositionClient;
	int             propositionClient2;
	int             propositionEndTime;

	int             autoFireteamEndTime;
	int             autoFireteamNum;

	int             autoFireteamCreateEndTime;
	int             autoFireteamCreateNum;

	int             autoFireteamJoinEndTime;
	int             autoFireteamJoinNum;


	qboolean        autoMapExpanded;
	int             autoMapExpandTime;

	qboolean        autoMapOff;	// is automap on or off

	bg_character_t *offscreenCmdr;

	// OSP
	int             aviDemoRate;	// Demo playback recording
	int             aReinfOffset[TEAM_NUM_TEAMS];	// Team reinforcement offsets
	int             cursorUpdate;	// Timeout for mouse pointer view
	fileHandle_t    dumpStatsFile;	// File to dump stats
	char           *dumpStatsFileName;	// Name of file to dump stats
	int             dumpStatsTime;	// Next stats command that comes back will be written to a logfile
	int             game_versioninfo;	// game base version
	gameStats_t     gamestats;
	topshotStats_t  topshots;
	qboolean        fResize;	// MV window "resize" status
	qboolean        fSelect;	// MV window "select" status
	qboolean        fKeyPressed[256];	// Key status to get around console issues
	int             timescaleUpdate;	// Timescale display for demo playback
	int             thirdpersonUpdate;

	cg_gamemodel_t  miscGameModels[MAX_STATIC_GAMEMODELS];

	vec2_t          ccMenuPos;
	qboolean        ccMenuShowing;
	int             ccMenuType;
	mapEntityData_t ccMenuEnt;
	int             ccSelectedLayer;
	int             ccSelectedObjective;
	int             ccSelectedTeam;	// ( 1 = ALLIES, 0 = AXIS )
	int             ccSelectedWeaponNumber;
	int             ccSelectedClass;
	int             ccSelectedWeapon;
	int             ccSelectedWeapon2;
	int             ccWeaponShots;
	int             ccWeaponHits;
	vec3_t          ccPortalPos;
	vec3_t          ccPortalAngles;
	int             ccPortalEnt;
	int             ccFilter;
	int             ccCurrentCamObjective;
	int             ccRequestedObjective;
	int             ccLastObjectiveRequestTime;

	int             loadingLatch;	// ( 0 = nothing yet, 1 = latched )

//  qboolean            playedLimboMusic;

	int             dbSortedClients[MAX_CLIENTS];
	int             dbSelectedClient;

	int             dbMode;
	qboolean        dbShowing;
	qboolean        dbAccuraciesRecieved;
	qboolean        dbPlayerKillsDeathsRecieved;
	qboolean        dbWeaponStatsRecieved;
	qboolean        dbAwardsParsed;
	char           *dbAwardNames[NUM_ENDGAME_AWARDS];
	team_t          dbAwardTeams[NUM_ENDGAME_AWARDS];
	char            dbAwardNamesBuffer[1024];
	int             dbLastRequestTime;
	int             dbLastScoreRequest;
	int             dbPlayerListOffset;
	int             dbWeaponListOffset;
	cg_weaponstats_t dbWeaponStats[WS_MAX];
	int             dbChatMode;

	int             tdbAxisMapsXP[SK_NUM_SKILLS][MAX_MAPS_PER_CAMPAIGN];
	int             tdbAlliedMapsXP[SK_NUM_SKILLS][MAX_MAPS_PER_CAMPAIGN];
	int             tdbMapListOffset;
	int             tdbSelectedMap;

	int             ftMenuPos;
	int             ftMenuMode;
	int             ftMenuModeEx;

	qboolean        limboLoadoutSelected;
	qboolean        limboLoadoutModified;

	oidInfo_t       oidInfo[MAX_OID_TRIGGERS];

	qboolean        initing;
} cgs_t;

//==============================================================================

extern cgs_t    cgs;
extern cg_t     cg;
extern centity_t cg_entities[MAX_GENTITIES];
extern weaponInfo_t cg_weapons[MAX_WEAPONS];
extern itemInfo_t cg_items[MAX_ITEMS];
extern markPoly_t cg_markPolys[MAX_MARK_POLYS];

extern vmCvar_t cg_centertime;
extern vmCvar_t cg_runpitch;
extern vmCvar_t cg_runroll;
extern vmCvar_t cg_bobup;
extern vmCvar_t cg_bobpitch;
extern vmCvar_t cg_bobroll;
extern vmCvar_t cg_bobyaw;
extern vmCvar_t cg_swingSpeed;
extern vmCvar_t cg_shadows;
extern vmCvar_t cg_gibs;
extern vmCvar_t cg_draw2D;
extern vmCvar_t cg_drawFPS;
extern vmCvar_t cg_drawSnapshot;
extern vmCvar_t cg_drawCrosshair;
extern vmCvar_t cg_drawCrosshairNames;
extern vmCvar_t cg_drawCrosshairPickups;
extern vmCvar_t cg_useWeapsForZoom;
extern vmCvar_t cg_weaponCycleDelay;	//----(SA)  added
extern vmCvar_t cg_cycleAllWeaps;
extern vmCvar_t cg_drawTeamOverlay;
extern vmCvar_t cg_crosshairX;
extern vmCvar_t cg_crosshairY;
extern vmCvar_t cg_crosshairSize;
extern vmCvar_t cg_crosshairHealth;
extern vmCvar_t cg_drawStatus;
extern vmCvar_t cg_animSpeed;
extern vmCvar_t cg_debugAnim;
extern vmCvar_t cg_debugPosition;
extern vmCvar_t cg_debugEvents;
extern vmCvar_t cg_drawSpreadScale;
extern vmCvar_t cg_railTrailTime;
extern vmCvar_t cg_errorDecay;
extern vmCvar_t cg_nopredict;
extern vmCvar_t cg_noPlayerAnims;
extern vmCvar_t cg_showmiss;
extern vmCvar_t cg_footsteps;
extern vmCvar_t cg_markTime;
extern vmCvar_t cg_brassTime;
extern vmCvar_t cg_gun_frame;
extern vmCvar_t cg_gun_x;
extern vmCvar_t cg_gun_y;
extern vmCvar_t cg_gun_z;
extern vmCvar_t cg_drawGun;
extern vmCvar_t cg_cursorHints;
extern vmCvar_t cg_letterbox;	//----(SA)  added
extern vmCvar_t cg_tracerChance;
extern vmCvar_t cg_tracerWidth;
extern vmCvar_t cg_tracerLength;
extern vmCvar_t cg_tracerSpeed;
extern vmCvar_t cg_autoswitch;
extern vmCvar_t cg_ignore;
extern vmCvar_t cg_fov;
extern vmCvar_t cg_zoomFov;
extern vmCvar_t cg_zoomDefaultBinoc;
extern vmCvar_t cg_zoomDefaultSniper;
extern vmCvar_t cg_zoomDefaultFG;
extern vmCvar_t cg_zoomDefaultSnooper;
extern vmCvar_t cg_zoomStepBinoc;
extern vmCvar_t cg_zoomStepSniper;
extern vmCvar_t cg_zoomStepSnooper;
extern vmCvar_t cg_zoomStepFG;
extern vmCvar_t cg_thirdPersonRange;
extern vmCvar_t cg_thirdPersonAngle;
extern vmCvar_t cg_thirdPerson;
extern vmCvar_t cg_stereoSeparation;
extern vmCvar_t cg_lagometer;

#ifdef ALLOW_GSYNC
extern vmCvar_t cg_synchronousClients;
#endif							// ALLOW_GSYNC
extern vmCvar_t cg_teamChatTime;
extern vmCvar_t cg_teamChatHeight;
extern vmCvar_t cg_stats;
extern vmCvar_t cg_forceModel;
extern vmCvar_t cg_coronafardist;
extern vmCvar_t cg_coronas;
extern vmCvar_t cg_buildScript;
extern vmCvar_t cg_paused;
extern vmCvar_t cg_blood;
extern vmCvar_t cg_predictItems;
extern vmCvar_t cg_deferPlayers;
extern vmCvar_t cg_teamChatsOnly;
extern vmCvar_t cg_noVoiceChats;	// NERVE - SMF
extern vmCvar_t cg_noVoiceText;	// NERVE - SMF
extern vmCvar_t cg_enableBreath;
extern vmCvar_t cg_autoactivate;
extern vmCvar_t cg_smoothClients;
extern vmCvar_t pmove_fixed;
extern vmCvar_t pmove_msec;

extern vmCvar_t cg_cameraOrbit;
extern vmCvar_t cg_cameraOrbitDelay;
extern vmCvar_t cg_timescaleFadeEnd;
extern vmCvar_t cg_timescaleFadeSpeed;
extern vmCvar_t cg_timescale;
extern vmCvar_t cg_cameraMode;
extern vmCvar_t cg_smallFont;
extern vmCvar_t cg_bigFont;
extern vmCvar_t cg_noTaunt;		// NERVE - SMF
extern vmCvar_t cg_voiceSpriteTime;	// DHM - Nerve

extern vmCvar_t cg_blinktime;	//----(SA)  added

// Rafael - particle switch
extern vmCvar_t cg_wolfparticles;

// done

// Ridah
extern vmCvar_t cg_gameType;
extern vmCvar_t cg_bloodTime;
extern vmCvar_t cg_norender;
extern vmCvar_t cg_skybox;

// Rafael gameskill
//extern vmCvar_t           cg_gameSkill;
// done

// JPW NERVE
extern vmCvar_t cg_redlimbotime;
extern vmCvar_t cg_bluelimbotime;

// jpw

extern vmCvar_t cg_movespeed;

extern vmCvar_t cg_animState;

extern vmCvar_t cg_drawCompass;
extern vmCvar_t cg_drawNotifyText;
extern vmCvar_t cg_quickMessageAlt;
extern vmCvar_t cg_popupLimboMenu;
extern vmCvar_t cg_descriptiveText;

// -NERVE - SMF

extern vmCvar_t cg_antilag;

extern vmCvar_t developer;

// OSP
extern vmCvar_t authLevel;
extern vmCvar_t cf_wstats;
extern vmCvar_t cf_wtopshots;

//extern vmCvar_t           cg_announcer;
extern vmCvar_t cg_autoAction;
extern vmCvar_t cg_autoReload;
extern vmCvar_t cg_bloodDamageBlend;
extern vmCvar_t cg_bloodFlash;
extern vmCvar_t cg_complaintPopUp;
extern vmCvar_t cg_crosshairAlpha;
extern vmCvar_t cg_crosshairAlphaAlt;
extern vmCvar_t cg_crosshairColor;
extern vmCvar_t cg_crosshairColorAlt;
extern vmCvar_t cg_crosshairPulse;
extern vmCvar_t cg_drawReinforcementTime;
extern vmCvar_t cg_drawWeaponIconFlash;
extern vmCvar_t cg_muzzleFlash;
extern vmCvar_t cg_noAmmoAutoSwitch;
extern vmCvar_t cg_printObjectiveInfo;
extern vmCvar_t cg_specHelp;
extern vmCvar_t cg_specSwing;
extern vmCvar_t cg_uinfo;
extern vmCvar_t cg_useScreenshotJPEG;
extern vmCvar_t ch_font;
extern vmCvar_t demo_avifpsF1;
extern vmCvar_t demo_avifpsF2;
extern vmCvar_t demo_avifpsF3;
extern vmCvar_t demo_avifpsF4;
extern vmCvar_t demo_avifpsF5;
extern vmCvar_t demo_drawTimeScale;
extern vmCvar_t demo_infoWindow;
extern vmCvar_t mv_sensitivity;

// engine mappings
extern vmCvar_t int_cl_maxpackets;
extern vmCvar_t int_cl_timenudge;
extern vmCvar_t int_m_pitch;
extern vmCvar_t int_sensitivity;
extern vmCvar_t int_ui_blackout;

// -OSP

extern vmCvar_t cg_rconPassword;
extern vmCvar_t cg_refereePassword;
extern vmCvar_t cg_atmosphericEffects;

// START Mad Doc - TDF
extern vmCvar_t cg_drawRoundTimer;

// END Mad Doc - TDF
extern vmCvar_t cg_debugSkills;
extern vmCvar_t cg_drawFireteamOverlay;
extern vmCvar_t cg_drawSmallPopupIcons;

// Gordon: some optimization cvars
extern vmCvar_t cg_fastSolids;
extern vmCvar_t cg_instanttapout;

// bani - demo recording cvars
extern vmCvar_t cl_demorecording;
extern vmCvar_t cl_demofilename;
extern vmCvar_t cl_demooffset;
extern vmCvar_t cl_waverecording;
extern vmCvar_t cl_wavefilename;
extern vmCvar_t cl_waveoffset;
extern vmCvar_t cg_recording_statusline;

//
// cg_main.c
//
const char     *CG_ConfigString(int index);
int             CG_ConfigStringCopy(int index, char *buff, int buffsize);
const char     *CG_Argv(int arg);

float           CG_Cvar_Get(const char *cvar);

char           *CG_generateFilename(void);
int             CG_findClientNum(char *s);
void            CG_printConsoleString(char *str);

void            CG_LoadObjectiveData(void);

void QDECL      CG_Printf(const char *msg, ...);
void QDECL      CG_Error(const char *msg, ...);

void            CG_StartMusic(void);
void            CG_QueueMusic(void);

void            CG_UpdateCvars(void);

int             CG_CrosshairPlayer(void);
int             CG_LastAttacker(void);
void            CG_LoadMenus(const char *menuFile);
void            CG_KeyEvent(int key, qboolean down);
void            CG_MouseEvent(int x, int y);
void            CG_EventHandling(int type, qboolean fForced);

qboolean        CG_GetTag(int clientNum, char *tagname, orientation_t * or);
qboolean        CG_GetWeaponTag(int clientNum, char *tagname, orientation_t * or);

//
// cg_view.c
//
void            CG_TestModel_f(void);
void            CG_TestGun_f(void);
void            CG_TestModelNextFrame_f(void);
void            CG_TestModelPrevFrame_f(void);
void            CG_TestModelNextSkin_f(void);
void            CG_TestModelPrevSkin_f(void);
void            CG_ZoomDown_f(void);
void            CG_ZoomIn_f(void);
void            CG_ZoomOut_f(void);
void            CG_ZoomUp_f(void);

void            CG_SetupFrustum(void);
qboolean        CG_CullPoint(vec3_t pt);
qboolean        CG_CullPointAndRadius(const vec3_t pt, vec_t radius);

void            CG_DrawActiveFrame(int serverTime, stereoFrame_t stereoView, qboolean demoPlayback);
void            CG_DrawSkyBoxPortal(qboolean fLocalView);
void            CG_Concussive(centity_t * cent);

void            CG_Letterbox(float xsize, float ysize, qboolean center);

//
// cg_drawtools.c
//
void            CG_AdjustFrom640(float *x, float *y, float *w, float *h);
void            CG_FillRect(float x, float y, float width, float height, const float *color);
void            CG_HorizontalPercentBar(float x, float y, float width, float height, float percent);
void            CG_DrawPic(float x, float y, float width, float height, qhandle_t hShader);
void            CG_DrawPicST(float x, float y, float width, float height, float s0, float t0, float s1, float t1,
							 qhandle_t hShader);
void            CG_DrawRotatedPic(float x, float y, float width, float height, qhandle_t hShader, float angle);	// NERVE - SMF
void            CG_DrawChar(int x, int y, int width, int height, int ch);
void            CG_FilledBar(float x, float y, float w, float h, float *startColor, float *endColor, const float *bgColor,
							 float frac, int flags);
// JOSEPH 10-26-99
void            CG_DrawStretchPic(float x, float y, float width, float height, qhandle_t hShader);

// END JOSEPH
void            CG_DrawString(float x, float y, const char *string, float charWidth, float charHeight, const float *modulate);


// CHRUKER: b082 - setColor is no longer const
void            CG_DrawStringExt(int x, int y, const char *string, float *setColor,
								 qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars);
// JOSEPH 4-17-00
void            CG_DrawStringExt2(int x, int y, const char *string, const float *setColor,
								  qboolean forceColor, qboolean shadow, int charWidth, int charHeight, int maxChars);
void            CG_DrawStringExt_Shadow(int x, int y, const char *string, const float *setColor,
										qboolean forceColor, int shadow, int charWidth, int charHeight, int maxChars);
// END JOSEPH
void            CG_DrawBigString(int x, int y, const char *s, float alpha);
void            CG_DrawBigStringColor(int x, int y, const char *s, vec4_t color);
void            CG_DrawSmallString(int x, int y, const char *s, float alpha);
void            CG_DrawSmallStringColor(int x, int y, const char *s, vec4_t color);

// JOSEPH 4-25-00
void            CG_DrawBigString2(int x, int y, const char *s, float alpha);
void            CG_DrawBigStringColor2(int x, int y, const char *s, vec4_t color);

// END JOSEPH
int             CG_DrawStrlen(const char *str);

float          *CG_FadeColor(int startMsec, int totalMsec);
float          *CG_TeamColor(int team);
void            CG_TileClear(void);
void            CG_ColorForHealth(vec4_t hcolor);
void            CG_GetColorForHealth(int health, vec4_t hcolor);

#ifdef IPHONE
#define 		UI_DrawBannerString cgame_UI_DrawBannerString
#define 		UI_ProportionalStringWidth cgame_UI_ProportionalStringWidth
#define 		UI_ProportionalSizeScale cgame_UI_ProportionalSizeScale
#define 		UI_DrawProportionalString cgame_UI_DrawProportionalString
#endif // IPHONE

void            UI_DrawProportionalString(int x, int y, const char *str, int style, vec4_t color);

// new hud stuff
void            CG_DrawRect(float x, float y, float width, float height, float size, const float *color);
void            CG_DrawRect_FixedBorder(float x, float y, float width, float height, int border, const float *color);
void            CG_DrawSides(float x, float y, float w, float h, float size);
void            CG_DrawTopBottom(float x, float y, float w, float h, float size);
void            CG_DrawTopBottom_NoScale(float x, float y, float w, float h, float size);

// CHRUKER: b076 - Scoreboard background had black lines drawn twice
void			CG_DrawBottom_NoScale( float x, float y, float w, float h, float size );

// NERVE - SMF - localization functions
void            CG_InitTranslation();
char           *CG_TranslateString(const char *string);
void            CG_SaveTransTable();
void            CG_ReloadTranslation();

// -NERVE - SMF

//
// cg_draw.c, cg_newDraw.c
//
extern char     cg_fxflags;		// JPW NERVE

void            CG_InitStatsDebug(void);
void            CG_StatsDebugAddText(const char *text);

void            CG_AddLagometerFrameInfo(void);
void            CG_AddLagometerSnapshotInfo(snapshot_t * snap);
void            CG_CenterPrint(const char *str, int y, int charWidth);
void            CG_PriorityCenterPrint(const char *str, int y, int charWidth, int priority);	// NERVE - SMF
void            CG_ObjectivePrint(const char *str, int charWidth);	// NERVE - SMF
void            CG_DrawActive(stereoFrame_t stereoView);
void            CG_CheckForCursorHints(void);
void            CG_DrawTeamBackground(int x, int y, int w, int h, float alpha, int team);
void            CG_OwnerDraw(float x, float y, float w, float h, float text_x, float text_y, int ownerDraw, int ownerDrawFlags,
							 int align, float special, float scale, vec4_t color, qhandle_t shader, int textStyle);
void            CG_Text_Paint_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text, float adjust,
								  int limit, int style, fontInfo_t * font);
void            CG_Text_Paint_Centred_Ext(float x, float y, float scalex, float scaley, vec4_t color, const char *text,
										  float adjust, int limit, int style, fontInfo_t * font);
void            CG_Text_Paint(float x, float y, float scale, vec4_t color, const char *text, float adjust, int limit, int style);
void            CG_Text_SetActiveFont(int font);
int             CG_Text_Width_Ext(const char *text, float scale, int limit, fontInfo_t * font);
int             CG_Text_Width(const char *text, float scale, int limit);
int             CG_Text_Height_Ext(const char *text, float scale, int limit, fontInfo_t * font);
int             CG_Text_Height(const char *text, float scale, int limit);
float           CG_GetValue(int ownerDraw, int type);	// 'type' is relative or absolute (fractional-'0.5' or absolute- '50' health)
qboolean        CG_OwnerDrawVisible(int flags);
void            CG_RunMenuScript(char **args);
void            CG_GetTeamColor(vec4_t * color);
void            CG_Draw3DModel(float x, float y, float w, float h, qhandle_t model, qhandle_t skin, vec3_t origin, vec3_t angles);
void            CG_Text_PaintChar_Ext(float x, float y, float w, float h, float scalex, float scaley, float s, float t, float s2,
									  float t2, qhandle_t hShader);
void            CG_Text_PaintChar(float x, float y, float width, float height, float scale, float s, float t, float s2, float t2,
								  qhandle_t hShader);
qboolean        CG_YourTeamHasFlag();
qboolean        CG_OtherTeamHasFlag();
void            CG_DrawCursorhint(rectDef_t * rect);
void            CG_DrawWeapStability(rectDef_t * rect);
void            CG_DrawWeapHeat(rectDef_t * rect, int align);
void            CG_DrawPlayerWeaponIcon(rectDef_t * rect, qboolean drawHighlighted, int align, vec4_t * refcolor);
int             CG_CalculateReinfTime(qboolean menu);
float           CG_CalculateReinfTime_Float(qboolean menu);
void            CG_Fade(int r, int g, int b, int a, int time, int duration);




//
// cg_player.c
//
qboolean        CG_EntOnFire(centity_t * cent);	// Ridah
void            CG_Player(centity_t * cent);
void            CG_ResetPlayerEntity(centity_t * cent);
void            CG_AddRefEntityWithPowerups(refEntity_t * ent, int powerups, int team, entityState_t * es,
											const vec3_t fireRiseDir);
void            CG_NewClientInfo(int clientNum);
sfxHandle_t     CG_CustomSound(int clientNum, const char *soundName);
void            CG_ParseTeamXPs(int n);

// Rafael particles
extern qboolean initparticles;
int             CG_NewParticleArea(int num);

//
// cg_predict.c
//
void            CG_BuildSolidList(void);
int             CG_PointContents(const vec3_t point, int passEntityNum);
void            CG_Trace(trace_t * result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
						 int skipNumber, int mask);
void            CG_FTTrace(trace_t * result, const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end,
						   int skipNumber, int mask);
void            CG_PredictPlayerState(void);

//void CG_LoadDeferredPlayers( void );


//
// cg_events.c
//
void            CG_CheckEvents(centity_t * cent);
void            CG_EntityEvent(centity_t * cent, vec3_t position);
void            CG_PainEvent(centity_t * cent, int health, qboolean crouching);
void            CG_PrecacheFXSounds(void);


//
// cg_ents.c
//
void            CG_SetEntitySoundPosition(centity_t * cent);
void            CG_AddPacketEntities(void);
void            CG_Beam(centity_t * cent);
void            CG_AdjustPositionForMover(const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out,
										  vec3_t outDeltaAngles);
void            CG_AddCEntity(centity_t * cent);
qboolean        CG_AddCEntity_Filter(centity_t * cent);
qboolean        CG_AddLinkedEntity(centity_t * cent, qboolean ignoreframe, int atTime);
void            CG_PositionEntityOnTag(refEntity_t * entity, const refEntity_t * parent, const char *tagName, int startIndex,
									   vec3_t * offset);
void            CG_PositionRotatedEntityOnTag(refEntity_t * entity, const refEntity_t * parent, const char *tagName);


//
// cg_weapons.c
//
void            CG_LastWeaponUsed_f(void);	//----(SA)    added
void            CG_NextWeaponInBank_f(void);	//----(SA)    added
void            CG_PrevWeaponInBank_f(void);	//----(SA)    added
void            CG_AltWeapon_f(void);
void            CG_NextWeapon_f(void);
void            CG_PrevWeapon_f(void);
void            CG_Weapon_f(void);
void            CG_WeaponBank_f(void);
qboolean        CG_WeaponSelectable(int i);

void            CG_FinishWeaponChange(int lastweap, int newweap);

void            CG_RegisterWeapon(int weaponNum, qboolean force);
void            CG_RegisterItemVisuals(int itemNum);

void            CG_FireWeapon(centity_t * cent);	//----(SA) modified.

//void CG_EndFireWeapon( centity_t *cent, int firemode );   //----(SA)  added
void            CG_MissileHitWall(int weapon, int clientNum, vec3_t origin, vec3_t dir, int surfaceFlags);	//  (SA) modified to send missilehitwall surface parameters

void            CG_MissileHitWallSmall(int weapon, int clientNum, vec3_t origin, vec3_t dir);
void            CG_DrawTracer(vec3_t start, vec3_t finish);

// Rafael
void            CG_MG42EFX(centity_t * cent);

void            CG_FLAKEFX(centity_t * cent, int whichgun);

void            CG_MortarEFX(centity_t * cent);

// Ridah
qboolean        CG_MonsterUsingWeapon(centity_t * cent, int aiChar, int weaponNum);

// Rafael
void            CG_MissileHitWall2(int weapon, int clientNum, vec3_t origin, vec3_t dir);

// done

void            CG_MissileHitPlayer(centity_t * cent, int weapon, vec3_t origin, vec3_t dir, int entityNum);
qboolean        CG_CalcMuzzlePoint(int entityNum, vec3_t muzzle);
void            CG_Bullet(vec3_t end, int sourceEntityNum, vec3_t normal, qboolean flesh, int fleshEntityNum, int otherEntNum2,
						  float waterfraction, int seed);

void            CG_RailTrail(clientInfo_t * ci, vec3_t start, vec3_t end, int type);	//----(SA) added 'type'
void            CG_RailTrail2(clientInfo_t * ci, vec3_t start, vec3_t end);
void            CG_GrappleTrail(centity_t * ent, const weaponInfo_t * wi);
void            CG_AddViewWeapon(playerState_t * ps);
void            CG_AddPlayerWeapon(refEntity_t * parent, playerState_t * ps, centity_t * cent);

void            CG_OutOfAmmoChange(qboolean allowforceswitch);

//----(SA) added to header to access from outside cg_weapons.c
void            CG_AddDebris(vec3_t origin, vec3_t dir, int speed, int duration, int count);

//----(SA) done

//void CG_ClientDamage( int entnum, int enemynum, int id );

//
// cg_marks.c
//
void            CG_InitMarkPolys(void);
void            CG_AddMarks(void);
void            CG_ImpactMark(qhandle_t markShader,
							  vec3_t origin, vec4_t projection, float radius, float orientation,
							  float r, float g, float b, float a, int lifeTime);

// Rafael particles
//
// cg_particles.c
//
void            CG_ClearParticles(void);
void            CG_AddParticles(void);
void            CG_ParticleSnow(qhandle_t pshader, vec3_t origin, vec3_t origin2, int turb, float range, int snum);
void            CG_ParticleSmoke(qhandle_t pshader, centity_t * cent);
void            CG_AddParticleShrapnel(localEntity_t * le);
void            CG_ParticleSnowFlurry(qhandle_t pshader, centity_t * cent);
void            CG_ParticleBulletDebris(vec3_t org, vec3_t vel, int duration);
void            CG_ParticleDirtBulletDebris(vec3_t org, vec3_t vel, int duration);	// DHM - Nerve
void            CG_ParticleDirtBulletDebris_Core(vec3_t org, vec3_t vel, int duration, float width, float height, float alpha,
												 qhandle_t shader);
void            CG_ParticleSparks(vec3_t org, vec3_t vel, int duration, float x, float y, float speed);
void            CG_ParticleDust(centity_t * cent, vec3_t origin, vec3_t dir);
void            CG_ParticleMisc(qhandle_t pshader, vec3_t origin, int size, int duration, float alpha);

// Ridah
void            CG_ParticleExplosion(char *animStr, vec3_t origin, vec3_t vel, int duration, int sizeStart, int sizeEnd,
									 qboolean dlight);

// Rafael snow pvs check
void            CG_SnowLink(centity_t * cent, qboolean particleOn);

// done.

void            CG_ParticleImpactSmokePuff(qhandle_t pshader, vec3_t origin);
void            CG_ParticleImpactSmokePuffExtended(qhandle_t pshader, vec3_t origin, int lifetime, int vel, int acc, int maxroll, float alpha, float size);	// (SA) so I can add more parameters without screwing up the one that's there
void            CG_Particle_Bleed(qhandle_t pshader, vec3_t start, vec3_t dir, int fleshEntityNum, int duration);
void            CG_GetBleedOrigin(vec3_t head_origin, vec3_t body_origin, int fleshEntityNum);
void            CG_Particle_OilParticle(qhandle_t pshader, vec3_t origin, vec3_t origin2, int ptime, int snum);
void            CG_Particle_OilSlick(qhandle_t pshader, centity_t * cent);
void            CG_OilSlickRemove(centity_t * cent);
void            CG_ParticleBloodCloudZombie(centity_t * cent, vec3_t origin, vec3_t dir);
void            CG_ParticleBloodCloud(centity_t * cent, vec3_t origin, vec3_t dir);

// done

// Ridah, trails
//
// cg_trails.c
//
// rain - usedby for zinx's trail fixes
int             CG_AddTrailJunc(int headJuncIndex, void *usedby, qhandle_t shader, int spawnTime, int sType, vec3_t pos,
								int trailLife, float alphaStart, float alphaEnd, float startWidth, float endWidth, int flags,
								vec3_t colorStart, vec3_t colorEnd, float sRatio, float animSpeed);
int             CG_AddSparkJunc(int headJuncIndex, void *usedby, qhandle_t shader, vec3_t pos, int trailLife, float alphaStart,
								float alphaEnd, float startWidth, float endWidth);
int             CG_AddSmokeJunc(int headJuncIndex, void *usedby, qhandle_t shader, vec3_t pos, int trailLife, float alpha,
								float startWidth, float endWidth);
void            CG_AddTrails(void);
void            CG_ClearTrails(void);

// done.

//
// cg_sound.c
//

// Ridah, sound scripting
int             CG_SoundScriptPrecache(const char *name);
int             CG_SoundPlaySoundScript(const char *name, vec3_t org, int entnum, qboolean buffer);
void            CG_UpdateBufferedSoundScripts(void);

// TTimo: prototype must match animScriptData_t::playSound
void            CG_SoundPlayIndexedScript(int index, vec3_t org, int entnum);
void            CG_SoundInit(void);

// done.
void            CG_SetViewanglesForSpeakerEditor(void);
void            CG_SpeakerEditorDraw(void);
void            CG_SpeakerEditor_KeyHandling(int key, qboolean down);
void            CG_Debriefing_KeyEvent(int key, qboolean down);
void            CG_SpeakerEditorMouseMove_Handling(int x, int y);
void            CG_ActivateEditSoundMode(void);
void            CG_DeActivateEditSoundMode(void);
void            CG_ModifyEditSpeaker(void);
void            CG_UndoEditSpeaker(void);
void            CG_ToggleActiveOnScriptSpeaker(int index);
void            CG_UnsetActiveOnScriptSpeaker(int index);
void            CG_SetActiveOnScriptSpeaker(int index);
void            CG_AddScriptSpeakers(void);

// Ridah, flamethrower
void            CG_FireFlameChunks(centity_t * cent, vec3_t origin, vec3_t angles, float speedScale, qboolean firing);
void            CG_InitFlameChunks(void);
void            CG_AddFlameChunks(void);
void            CG_UpdateFlamethrowerSounds(void);
void            CG_FlameDamage(int owner, vec3_t org, float radius);

// done.

//
// cg_localents.c
//
void            CG_InitLocalEntities(void);
localEntity_t  *CG_AllocLocalEntity(void);
void            CG_AddLocalEntities(void);

//
// cg_effects.c
//
qhandle_t       getTestShader(void);

int             CG_GetOriginForTag(centity_t * cent, refEntity_t * parent, char *tagName, int startIndex, vec3_t org,
								   vec3_t axis[3]);
localEntity_t  *CG_SmokePuff(const vec3_t p, const vec3_t vel, float radius, float r, float g, float b, float a, float duration,
							 int startTime, int fadeInTime, int leFlags, qhandle_t hShader);

void            CG_BubbleTrail(vec3_t start, vec3_t end, float size, float spacing);
void            CG_SpawnEffect(vec3_t org);
void            CG_GibPlayer(centity_t * cent, vec3_t playerOrigin, vec3_t gdir);
void            CG_LoseHat(centity_t * cent, vec3_t dir);	//----(SA)  added

void            CG_Bleed(vec3_t origin, int entityNum);

localEntity_t  *CG_MakeExplosion(vec3_t origin, vec3_t dir, qhandle_t hModel, qhandle_t shader, int msec, qboolean isSprite);

// Ridah
void            CG_SparklerSparks(vec3_t origin, int count);
void            CG_ClearFlameChunks(void);
void            CG_ProjectedSpotLight(vec3_t start, vec3_t dir);

// done.

//----(SA)
void            CG_Spotlight(centity_t * cent, float *color, vec3_t start, vec3_t dir, int segs, float range, int startWidth,
							 float coneAngle, int flags);
#define SL_NOTRACE          0x001	// don't do a trace check for shortening the beam, always draw at full 'range' length
#define SL_NODLIGHT         0x002	// don't put a dlight at the end
#define SL_NOSTARTCAP       0x004	// dont' cap the start circle
#define SL_LOCKTRACETORANGE 0x010	// only trace out as far as the specified range (rather than to max spot range)
#define SL_NOFLARE          0x020	// don't draw a flare when the light is pointing at the camera
#define SL_NOIMPACT         0x040	// don't draw the impact mark
#define SL_LOCKUV           0x080	// lock the texture coordinates at the 'true' length of the requested beam.
#define SL_NOCORE           0x100	// don't draw the center 'core' beam
#define SL_TRACEWORLDONLY   0x200
//----(SA)  done

void            CG_RumbleEfx(float pitch, float yaw);

void            InitSmokeSprites(void);
void            CG_RenderSmokeGrenadeSmoke(centity_t * cent, const weaponInfo_t * weapon);
void            CG_AddSmokeSprites(void);

//
// cg_snapshot.c
//
void            CG_ProcessSnapshots(void);

//
// cg_spawn.c
//
qboolean        CG_SpawnString(const char *key, const char *defaultString, char **out);

// spawn string returns a temporary reference, you must CopyString() if you want to keep it
qboolean        CG_SpawnFloat(const char *key, const char *defaultString, float *out);
qboolean        CG_SpawnInt(const char *key, const char *defaultString, int *out);
qboolean        CG_SpawnVector(const char *key, const char *defaultString, float *out);
void            CG_ParseEntitiesFromString(void);

//
// cg_info.c
//
void            CG_LoadingString(const char *s);

//void CG_LoadingItem( int itemNum );
void            CG_LoadingClient(int clientNum);
void            CG_DrawInformation(qboolean forcerefresh);
void            CG_DemoClick(int key, qboolean down);
void            CG_ShowHelp_Off(int *status);
void            CG_ShowHelp_On(int *status);
qboolean        CG_ViewingDraw(void);

//
// cg_scoreboard.c
//
qboolean        CG_DrawScoreboard(void);

//void CG_DrawTourneyScoreboard( void );

void            CG_TransformToCommandMapCoord(float *coord_x, float *coord_y);

//qboolean CG_DrawCommandMap( void );
void            CG_CommandCentreClick(int key);
void            CG_DrawAutoMap(void);

qboolean        CG_DrawLimboMenu(void);
qboolean        CG_DrawObjectivePanel(void);
qboolean        CG_DrawFireTeamMenu(void);

qboolean        CG_LimboMenuClick(int key);
qboolean        CG_FireTeamClick(int key);
qboolean        CG_ObjectiveMenuClick(int key);

void            CG_GameViewMenuClick(int key);
void            CG_GetLimboWeaponAnim(const char **torso_anim, const char **legs_anim);
int             CG_GetLimboSelectedWeapon();

qboolean        CG_DrawMissionBriefing(void);
void            CG_MissionBriefingClick(int key);

void            CG_LoadRankIcons(void);
qboolean        CG_DrawStatsRanksMedals(void);
void            CG_StatsRanksMedalsClick(int key);

typedef struct
{
	int             pendingAnimationTime;
	const char     *pendingTorsoAnim;
	const char     *pendingLegsAnim;
} pendingAnimation_t;

typedef struct
{
	lerpFrame_t     legs;
	lerpFrame_t     torso;
	lerpFrame_t     headAnim;

	vec3_t          headOrigin;	// used for centering talking heads

	vec3_t          viewAngles;
	vec3_t          moveAngles;

	pendingAnimation_t pendingAnimations[4];
	int             numPendingAnimations;

	float           y, z;

	int             teamNum;
	int             classNum;
} playerInfo_t;

typedef enum
{
	ANIM_IDLE,
	ANIM_RAISE,
} animType_t;

qboolean        CG_DrawGameView(void);
void            CG_ParseFireteams(void);
void            CG_ParseOIDInfos(void);
oidInfo_t      *CG_OIDInfoForEntityNum(int num);

//
// cg_consolecmds.c
//
extern const char *aMonths[12];
qboolean        CG_ConsoleCommand(void);
void            CG_InitConsoleCommands(void);
void            CG_ScoresDown_f(void);
void            CG_ScoresUp_f(void);
void            CG_autoRecord_f(void);
void            CG_autoScreenShot_f(void);
void            CG_keyOn_f(void);
void            CG_keyOff_f(void);
void            CG_dumpStats_f(void);
void            CG_toggleSwing_f(void);
void			CG_toggleSpecHelp_f(void); // Dushan

//
// cg_servercmds.c
//
void            CG_ExecuteNewServerCommands(int latestSequence);
void            CG_ParseServerinfo(void);
void            CG_ParseWolfinfo(void);	// NERVE - SMF
void            CG_ParseSpawns(void);
void            CG_ParseServerVersionInfo(const char *pszVersionInfo);
void            CG_ParseReinforcementTimes(const char *pszReinfSeedString);
void            CG_SetConfigValues(void);
void            CG_ShaderStateChanged(void);
void            CG_ChargeTimesChanged(void);
void            CG_LoadVoiceChats();	// NERVE - SMF
void            CG_PlayBufferedVoiceChats();	// NERVE - SMF
void            CG_AddToNotify(const char *str);
const char     *CG_LocalizeServerCommand(const char *buf);
void            CG_wstatsParse_cmd(void);
void            CG_wtopshotsParse_cmd(qboolean doBest);
void            CG_parseWeaponStats_cmd(void (txt_dump) (char *));
void            CG_parseBestShotsStats_cmd(qboolean doTop, void (txt_dump) (char *));
void            CG_parseTopShotsStats_cmd(qboolean doTop, void (txt_dump) (char *));
void            CG_scores_cmd(void);

//
// cg_playerstate.c
//
void            CG_Respawn(qboolean revived);
void            CG_TransitionPlayerState(playerState_t * ps, playerState_t * ops);

//
// cg_atmospheric.c
//
void            CG_GenerateTracemap(void);
void            CG_EffectParse(const char *effectstr);
void            CG_AddAtmosphericEffects();

//===============================================

void            CG_SetInitialCamera(const char *name, qboolean startBlack);
void            CG_StartCamera(const char *name, qboolean startBlack);
void            CG_StartInitialCamera();
void            CG_StopCamera(void);

//----(SA)  added
int             CG_LoadCamera(const char *name);
void            CG_FreeCamera(int camNum);

//----(SA)  end

bg_playerclass_t *CG_PlayerClassForClientinfo(clientInfo_t * ci, centity_t * cent);

void            CG_FitTextToWidth(char *instr, int w, int size);
void            CG_FitTextToWidth2(char *instr, float scale, float w, int size);
void            CG_FitTextToWidth_Ext(char *instr, float scale, float w, int size, fontInfo_t * font);
int             CG_TrimLeftPixels(char *instr, float scale, float w, int size);
void            CG_FitTextToWidth_SingleLine(char *instr, float scale, float w, int size);

void            CG_LocateCampaign(void);
void            CG_LocateArena(void);
const char     *CG_DescriptionForCampaign(void);
const char     *CG_NameForCampaign(void);
void            CG_CloseMenus();

// void CG_CampaignBriefing_f( void );
void            CG_LimboMenu_f(void);

void            CG_DrawPlayer_Limbo(float x, float y, float w, float h, playerInfo_t * pi, int time, clientInfo_t * ci,
									qboolean animatedHead);
animation_t    *CG_GetLimboAnimation(playerInfo_t * pi, const char *name);

typedef struct
{
	weapon_t        weapindex;
	const char     *desc;
} weaponType_t;

extern weaponType_t weaponTypes[];
weaponType_t   *WM_FindWeaponTypeForWeapon(weapon_t weapon);

extern animation_t *lastTorsoAnim;
extern animation_t *lastLegsAnim;
extern qboolean ccInitial;

void            CG_MenuCheckPendingAnimation(playerInfo_t * pi);
void            CG_MenuPendingAnimation(playerInfo_t * pi, const char *legsAnim, const char *torsoAnim, int delay);
void            CG_MenuSetAnimation(playerInfo_t * pi, const char *legsAnim, const char *torsoAnim, qboolean force,
									qboolean clearpending);

#define CC_FILTER_AXIS          ( 1 << 0 )
#define CC_FILTER_ALLIES        ( 1 << 1 )
#define CC_FILTER_SPAWNS        ( 1 << 2 )
#define CC_FILTER_CMDPOST       ( 1 << 3 )
#define CC_FILTER_HACABINETS    ( 1 << 4 )
#define CC_FILTER_CONSTRUCTIONS ( 1 << 5 )
#define CC_FILTER_DESTRUCTIONS  ( 1 << 6 )
#define CC_FILTER_OBJECTIVES    ( 1 << 7 )
//#define CC_FILTER_WAYPOINTS       (1 << 7)
//#define CC_FILTER_OBJECTIVES  (1 << 8)

typedef struct
{
	qhandle_t       shader;
	const char     *iconname;
	int             width;
	int             height;
} rankicon_t;

extern rankicon_t rankicons[NUM_EXPERIENCE_LEVELS][2];

#define TAB_LEFT_WIDTH 178
#define TAB_LEFT_EDGE ( 640 - TAB_LEFT_WIDTH )

fireteamData_t *CG_IsOnSameFireteam(int clientNum, int clientNum2);


// START Mad Doc - TDF

#define MAX_SQUAD_SIZE 6

//
// merged the common UI elements
//
#define UI_CAMPAIGN_BRIEFING 0
#define UI_COMMAND_MAP 1
#define UI_SQUAD_SELECT 2


void            CG_DrawUITabs();
void            CG_DrawUICurrentSquad();
qboolean        CG_UICommonClick();
void            CG_DrawUISelectedSoldier(void);
void            CG_UICurrentSquadSetup(void);
void            CG_CampaignBriefingSetup(void);


#define ORDER_ICON_FADE_TIME 3500

int             CG_GetFirstSelectedBot();
void            CG_AddToJournal(char *text);

// returns true if game is single player (or coop)
qboolean        CG_IsSinglePlayer(void);

// END Mad Doc - TDF

// Gordon: Fireteam stuff

//fireteamData_t* CG_IsOnFireteam(      int clientNum );
#define /*fireteamData_t**/ CG_IsOnFireteam( /*int*/ clientNum ) /*{ return*/ cgs.clientinfo[clientNum].fireteamData	/*} */
fireteamData_t *CG_IsOnSameFireteam(int clientNum, int clientNum2);
fireteamData_t *CG_IsFireTeamLeader(int clientNum);

clientInfo_t   *CG_ClientInfoForPosition(int pos, int max);
fireteamData_t *CG_FireTeamForPosition(int pos, int max);
clientInfo_t   *CG_FireTeamPlayerForPosition(int pos, int max);

void            CG_SortClientFireteam();

void            CG_DrawFireTeamOverlay(rectDef_t * rect);
clientInfo_t   *CG_SortedFireTeamPlayerForPosition(int pos, int max);
qboolean        CG_FireteamHasClass(int classnum, qboolean selectedonly);
const char     *CG_BuildSelectedFirteamString(void);

// OSP
#define Pri( x ) CG_Printf( "[cgnotify]%s", CG_LocalizeServerCommand( x ) )
#define CPri( x ) CG_CenterPrint( CG_LocalizeServerCommand( x ), SCREEN_HEIGHT - ( SCREEN_HEIGHT * 0.2 ), SMALLCHAR_WIDTH );

// cg_multiview.c
void            CG_mvDelete_f(void);
void            CG_mvHideView_f(void);
void            CG_mvNew_f(void);
void            CG_mvShowView_f(void);
void            CG_mvSwapViews_f(void);
void            CG_mvToggleAll_f(void);
void            CG_mvToggleView_f(void);

//
cg_window_t    *CG_mvClientLocate(int pID);
void            CG_mvCreate(int pID);
cg_window_t    *CG_mvCurrent(void);
void            CG_mvDraw(cg_window_t * sw);
cg_window_t    *CG_mvFindNonMainview(void);
void            CG_mvFree(int pID);
void            CG_mvMainviewSwap(cg_window_t * av);
qboolean        CG_mvMergedClientLocate(int pID);
void            CG_mvOverlayDisplay(void);
void            CG_mvOverlayUpdate(void);
void            CG_mvOverlayClientUpdate(int pID, int index);
void            CG_mvProcessClientList(void);
void            CG_mvTransitionPlayerState(playerState_t * ps);
void            CG_mvUpdateClientInfo(int pID);
void            CG_mvWindowOverlay(int pID, float b_x, float b_y, float b_w, float b_h, float s, int wState, qboolean fSelected);
void            CG_mvZoomBinoc(float x, float y, float w, float h);
void            CG_mvZoomSniper(float x, float y, float w, float h);
void			CG_mv_KeyHandling( int _key, qboolean down );

// cg_window.c
qboolean        CG_addString(cg_window_t * w, char *buf);

void 			CG_createDemoHelpWindow(void);
void 			CG_createSpecHelpWindow(void);
void            CG_createStatsWindow(void);
void            CG_createTopShotsWindow(void);
void            CG_createWstatsMsgWindow(void);
void            CG_createWtopshotsMsgWindow(void);
void            CG_createMOTDWindow(void);
void            CG_cursorUpdate(void);
void            CG_initStrings(void);
void            CG_printWindow(char *str);
void            CG_removeStrings(cg_window_t * w);
cg_window_t    *CG_windowAlloc(int fx, int startupLength);
void            CG_windowDraw(void);
void            CG_windowFree(cg_window_t * w);
void            CG_windowInit(void);
void            CG_windowNormalizeOnText(cg_window_t * w);

// OSP

void            CG_SetupCabinets(void);

extern displayContextDef_t cgDC;
void            CG_ParseSkyBox(void);
void            CG_ParseTagConnect(int tagNum);
void            CG_ParseTagConnects(void);

//
// cg_ents.c
//

void            CG_AttachBitsToTank(centity_t * tank, refEntity_t * mg42base, refEntity_t * mg42upper, refEntity_t * mg42gun,
									refEntity_t * player, refEntity_t * flash, vec_t * playerangles, const char *tagName,
									qboolean browning);

//
// cg_character.c
//

qboolean        CG_RegisterCharacter(const char *characterFile, bg_character_t * character);
bg_character_t *CG_CharacterForClientinfo(clientInfo_t * ci, centity_t * cent);
bg_character_t *CG_CharacterForPlayerstate(playerState_t * ps);
void            CG_RegisterPlayerClasses(void);

//
// cg_polybus.c
//

polyBuffer_t   *CG_PB_FindFreePolyBuffer(qhandle_t shader, int numVerts, int numIndicies);
void            CG_PB_ClearPolyBuffers(void);
void            CG_PB_RenderPolyBuffers(void);


//
// cg_limbopanel.c
//

void            CG_LimboPanel_KeyHandling(int key, qboolean down);
int             CG_LimboPanel_GetMaxObjectives(void);

qboolean        CG_LimboPanel_WeaponLights_KeyDown(panel_button_t * button, int key);
qboolean        CG_LimboPanel_WeaponPanel_KeyDown(panel_button_t * button, int key);
qboolean        CG_LimboPanel_WeaponPanel_KeyUp(panel_button_t * button, int key);
qboolean        CG_LimboPanel_ObjectiveText_KeyDown(panel_button_t * button, int key);
qboolean        CG_LimboPanel_TeamButton_KeyDown(panel_button_t * button, int key);
qboolean        CG_LimboPanel_ClassButton_KeyDown(panel_button_t * button, int key);
qboolean        CG_LimboPanel_OkButton_KeyDown(panel_button_t * button, int key);
qboolean        CG_LimboPanel_PlusButton_KeyDown(panel_button_t * button, int key);
qboolean        CG_LimboPanel_MinusButton_KeyDown(panel_button_t * button, int key);
qboolean        CG_LimboPanel_CancelButton_KeyDown(panel_button_t * button, int key);
qboolean        CG_LimboPanel_Filter_KeyDown(panel_button_t * button, int key);
qboolean        CG_LimboPanel_BriefingButton_KeyDown(panel_button_t * button, int key);

void            CG_LimboPanel_BriefingButton_Draw(panel_button_t * button);
void            CG_LimboPanel_ClassBar_Draw(panel_button_t * button);
void            CG_LimboPanel_Filter_Draw(panel_button_t * button);
void            CG_LimboPanel_RenderSkillIcon(panel_button_t * button);
void            CG_LimboPanel_RenderTeamButton(panel_button_t * button);
void            CG_LimboPanel_RenderClassButton(panel_button_t * button);
void            CG_LimboPanel_RenderObjectiveText(panel_button_t * button);
void            CG_LimboPanel_RenderCommandMap(panel_button_t * button);
void            CG_LimboPanel_RenderObjectiveBack(panel_button_t * button);
void            CG_LimboPanel_RenderLight(panel_button_t * button);
void            CG_LimboPanel_WeaponLights(panel_button_t * button);
void            CG_LimboPanel_RenderHead(panel_button_t * button);
void            CG_LimboPanel_WeaponPanel(panel_button_t * button);
void            CG_LimboPanel_Border_Draw(panel_button_t * button);
void            CG_LimboPanel_RenderMedal(panel_button_t * button);
void            CG_LimboPanel_RenderCounter(panel_button_t * button);
void            CG_LimboPanelRenderText_NoLMS(panel_button_t * button);
void            CG_LimboPanelRenderText_SkillsText(panel_button_t * button);

void            CG_LimboPanel_NameEditFinish(panel_button_t * button);

void            CG_LimboPanel_Setup(void);
void            CG_LimboPanel_Init(void);

void            CG_LimboPanel_GetWeaponCardIconData(weapon_t weap, qhandle_t * shader, float *w, float *h, float *s0, float *t0,
													float *s1, float *t1);
void            CG_LimboPanel_RequestObjective(void);
void            CG_LimboPanel_RequestWeaponStats(void);
qboolean        CG_LimboPanel_Draw(void);
team_t          CG_LimboPanel_GetTeam(void);
team_t          CG_LimboPanel_GetRealTeam(void);
bg_character_t *CG_LimboPanel_GetCharacter(void);
int             CG_LimboPanel_GetClass(void);
int             CG_LimboPanel_WeaponCount(void);
int             CG_LimboPanel_WeaponCount_ForSlot(int number);
int             CG_LimboPanel_GetSelectedWeaponNum(void);
void            CG_LimboPanel_SetSelectedWeaponNum(int number);
bg_playerclass_t *CG_LimboPanel_GetPlayerClass(void);
weapon_t        CG_LimboPanel_GetSelectedWeapon(void);
weapon_t        CG_LimboPanel_GetWeaponForNumber(int number, int slot, qboolean ignoreDisabled);
extWeaponStats_t CG_LimboPanel_GetSelectedWeaponStat(void);
qboolean        CG_LimboPanel_WeaponIsDisabled(int weap);
qboolean        CG_LimboPanel_RealWeaponIsDisabled(weapon_t weap);
int             CG_LimboPanel_GetWeaponNumberForPos(int pos);


void            CG_LimboPanel_SetSelectedWeaponNumForSlot(int index, int number);
weapon_t        CG_LimboPanel_GetSelectedWeaponForSlot(int index);

//
// cg_commandmap.c
//
// A scissored map always has the player in the center
typedef struct mapScissor_s
{
	qboolean        circular;	// if qfalse, rect
	float           zoomFactor;
	vec2_t          tl;
	vec2_t          br;
} mapScissor_t;

int             CG_CurLayerForZ(int z);
void            CG_DrawMap(float x, float y, float w, float h, int mEntFilter, mapScissor_t * scissor, qboolean interactive,
						   float alpha, qboolean borderblend);
int             CG_DrawSpawnPointInfo(int px, int py, int pw, int ph, qboolean draw, mapScissor_t * scissor, int expand);
void            CG_DrawMortarMarker(int px, int py, int pw, int ph, qboolean draw, mapScissor_t * scissor, int expand);
void            CG_CommandMap_SetHighlightText(const char *text, float x, float y);
void            CG_CommandMap_DrawHighlightText(void);
qboolean        CG_CommandCentreSpawnPointClick(void);

#define LIMBO_3D_X  287			//% 280
#define LIMBO_3D_Y  382
#define LIMBO_3D_W  128
#define LIMBO_3D_H  96			//% 94

#define CC_2D_X 64
#define CC_2D_Y 23
#define CC_2D_W 352
#define CC_2D_H 352

void            CG_DrawPlayerHead(rectDef_t * rect, bg_character_t * character, bg_character_t * headcharacter, float yaw,
								  float pitch, qboolean drawHat, hudHeadAnimNumber_t animation, qhandle_t painSkin, int rank,
								  qboolean spectator);

//
// cg_popupmessages.c
//

void            CG_InitPM(void);
void            CG_InitPMGraphics(void);
void            CG_UpdatePMLists(void);
void            CG_AddPMItem(popupMessageType_t type, const char *message, qhandle_t shader);
void            CG_AddPMItemBig(popupMessageBigType_t type, const char *message, qhandle_t shader);
void            CG_DrawPMItems(void);
void            CG_DrawPMItemsBig(void);
const char     *CG_GetPMItemText(centity_t * cent);
void            CG_PlayPMItemSound(centity_t * cent);
qhandle_t       CG_GetPMItemIcon(centity_t * cent);
void            CG_DrawKeyHint(rectDef_t * rect, const char *binding);

//
// cg_debriefing.c
//

clientInfo_t   *CG_Debriefing_GetSelectedClientInfo(void);
void            CG_Debrieing_SetSelectedClient(int clientNum);

qboolean        CG_Debriefing_Draw(void);
void            CG_ChatPanel_Setup(void);

void            CG_Debriefing_ChatEditFinish(panel_button_t * button);
void            CG_Debriefing_BackButton_Draw(panel_button_t * button);
void            CG_Debriefing_HTMLButton_Draw(panel_button_t * button);
void            CG_Debriefing_NextButton_Draw(panel_button_t * button);
void            CG_Debriefing_ChatButton_Draw(panel_button_t * button);
void            CG_Debriefing_ReadyButton_Draw(panel_button_t * button);
qboolean        CG_Debriefing_ChatButton_KeyDown(panel_button_t * button, int key);
qboolean        CG_Debriefing_BackButton_KeyDown(panel_button_t * button, int key);
qboolean        CG_Debriefing_ReadyButton_KeyDown(panel_button_t * button, int key);
qboolean        CG_Debriefing_QCButton_KeyDown(panel_button_t * button, int key);
qboolean        CG_Debriefing_NextButton_KeyDown(panel_button_t * button, int key);

void            CG_PanelButtonsRender_Button_Ext(rectDef_t * r, const char *text);

void            CG_Debriefing_PlayerName_Draw(panel_button_t * button);
void            CG_Debriefing_PlayerRank_Draw(panel_button_t * button);
void            CG_Debriefing_PlayerMedals_Draw(panel_button_t * button);
void            CG_Debriefing_PlayerTime_Draw(panel_button_t * button);
void            CG_Debriefing_PlayerXP_Draw(panel_button_t * button);
void            CG_Debriefing_PlayerACC_Draw(panel_button_t * button);
void            CG_Debriefing_PlayerSkills_Draw(panel_button_t * button);


void            CG_DebriefingPlayerWeaponStats_Draw(panel_button_t * button);

void            CG_DebriefingXPHeader_Draw(panel_button_t * button);

void            CG_DebriefingTitle_Draw(panel_button_t * button);
void            CG_DebriefingPlayerList_Draw(panel_button_t * button);
qboolean        CG_DebriefingPlayerList_KeyDown(panel_button_t * button, int key);

void            CG_Debriefing_ChatEdit_Draw(panel_button_t * button);
void            CG_Debriefing_ChatBox_Draw(panel_button_t * button);
void            CG_Debriefing_Scrollbar_Draw(panel_button_t * button);
qboolean        CG_Debriefing_Scrollbar_KeyDown(panel_button_t * button, int key);
qboolean        CG_Debriefing_Scrollbar_KeyUp(panel_button_t * button, int key);
float           CG_Debriefing_CalcCampaignProgress(void);

const char     *CG_Debriefing_RankNameForClientInfo(clientInfo_t * ci);
const char     *CG_Debriefing_FullRankNameForClientInfo(clientInfo_t * ci);
void            CG_Debriefing_Startup(void);
void            CG_Debriefing_Shutdown(void);
qboolean        CG_Debriefing_ServerCommand(const char *cmd);
void            CG_Debriefing_MouseEvent(int x, int y);

void            CG_TeamDebriefingOutcome_Draw(panel_button_t * button);
void            CG_TeamDebriefingMapList_Draw(panel_button_t * button);
qboolean        CG_TeamDebriefingMapList_KeyDown(panel_button_t * button, int key);
void            CG_TeamDebriefingMapWinner_Draw(panel_button_t * button);
void            CG_TeamDebriefingMapShot_Draw(panel_button_t * button);
void            CG_TeamDebriefingTeamXP_Draw(panel_button_t * button);
void            CG_TeamDebriefingTeamSkillXP_Draw(panel_button_t * button);

const char     *CG_PickupItemText(int item);

void            CG_LoadPanel_DrawPin(const char *text, float px, float py, float sx, float sy, qhandle_t shader, float pinsize,
									 float backheight);
void            CG_LoadPanel_RenderCampaignPins(panel_button_t * button);
void            CG_LoadPanel_RenderMissionDescriptionText(panel_button_t * button);
void            CG_LoadPanel_RenderCampaignTypeText(panel_button_t * button);
void            CG_LoadPanel_RenderCampaignNameText(panel_button_t * button);
void            CG_LoadPanel_RenderPercentageMeter(panel_button_t * button);
void            CG_LoadPanel_RenderContinueButton(panel_button_t * button);
void            CG_LoadPanel_RenderLoadingBar(panel_button_t * button);
void            CG_LoadPanel_KeyHandling(int key, qboolean down);
qboolean        CG_LoadPanel_ContinueButtonKeyDown(panel_button_t * button, int key);
void            CG_DrawConnectScreen(qboolean interactive, qboolean forcerefresh);

qboolean        CG_Debriefing2_Maps_KeyDown(panel_button_t * button, int key);
void            CG_Debriefing2TeamSkillHeaders_Draw(panel_button_t * button);
void            CG_Debriefing2TeamSkillXP_Draw(panel_button_t * button);
void            CG_Debreifing2_MissionTitle_Draw(panel_button_t * button);
void            CG_Debreifing2_Mission_Draw(panel_button_t * button);
void            CG_Debreifing2_Maps_Draw(panel_button_t * button);
void            CG_Debreifing2_Awards_Draw(panel_button_t * button);
void            CG_PanelButtonsRender_Window(panel_button_t * button);
void            CG_PanelButtonsRender_Button(panel_button_t * button);

team_t          CG_Debriefing_FindWinningTeamForMap(void);

int             CG_CalcViewValues(void);
void            CG_HudHeadAnimation(bg_character_t * ch, lerpFrame_t * lf, int *oldframe, int *frame, float *backlerp,
									hudHeadAnimNumber_t animation);

//
// cg_fireteams.c
//

void            CG_Fireteams_KeyHandling(int key, qboolean down);
qboolean        CG_FireteamCheckExecKey(int key, qboolean doaction);
void            CG_Fireteams_Draw(void);
void            CG_Fireteams_Setup(void);

void            CG_Fireteams_MenuText_Draw(panel_button_t * button);
void            CG_Fireteams_MenuTitleText_Draw(panel_button_t * button);
