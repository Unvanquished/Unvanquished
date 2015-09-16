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
#ifndef CG_LOCAL_H
#define CG_LOCAL_H

#include "engine/qcommon/q_shared.h"
#include "engine/renderer/tr_types.h"
#include "engine/client/cg_api.h"
#include "shared/bg_public.h"
#include "engine/client/keycodes.h"
#include "cg_ui.h"

// The entire cgame module is unloaded and reloaded on each level change,
// so there is no persistent data between levels on the client side.
// If you absolutely need something stored, it can either be kept
// by the server in the server stored userinfos, or stashed in a cvar.

#define FADE_TIME                      300
#define DAMAGE_DEFLECT_TIME            100
#define DAMAGE_RETURN_TIME             400
#define DAMAGE_TIME                    500
#define LAND_DEFLECT_TIME              150
#define LAND_RETURN_TIME               300
#define DUCK_TIME                      100
#define PAIN_TWITCH_TIME               200
#define WEAPON_SELECT_TIME             1400
#define ZOOM_TIME                      150
#define MUZZLE_FLASH_TIME              20

#define MAX_STEP_CHANGE                32

#define MAX_VERTS_ON_POLY              10
#define MAX_MARK_POLYS                 256

#define STAT_MINUS                     10 // num frame for '-' stats digit

#define CHAR_WIDTH                     32
#define CHAR_HEIGHT                    48

#define MAX_LOADING_LABEL_LENGTH       32

#define MAX_MINIMAP_ZONES              32

typedef enum
{
  FOOTSTEP_GENERAL,
  FOOTSTEP_FLESH,
  FOOTSTEP_METAL,
  FOOTSTEP_SPLASH,
  FOOTSTEP_CUSTOM,
  FOOTSTEP_NONE,

  FOOTSTEP_TOTAL
} footstep_t;

typedef enum
{
  IMPACTSOUND_DEFAULT,
  IMPACTSOUND_METAL,
  IMPACTSOUND_FLESH
} impactSound_t;

typedef enum
{
  JPS_INACTIVE,
  JPS_ACTIVE
} jetPackState_t;

typedef enum
{
  JANIM_NONE,

  JANIM_SLIDEOUT,
  JANIM_SLIDEIN,

  MAX_JETPACK_ANIMATIONS
} jetpackAnimNumber_t;

//======================================================================

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
typedef struct
{
	int           oldFrame;
	int           oldFrameTime; // time when ->oldFrame was exactly on

	int           frame;
	int           frameTime; // time when ->frame will be exactly on

	float         backlerp;

	float         yawAngle;
	bool      yawing;
	float         pitchAngle;
	bool      pitching;

	int           animationNumber; // may include ANIM_TOGGLEBIT
	animation_t   *animation;
	int           animationTime; // time when the first frame of the animation will be exact
	float         animationScale;

	// added for smooth blending between animations on change

	int         old_animationNumber; // may include ANIM_TOGGLEBIT
	animation_t *old_animation;

	float       blendlerp;
	float       blendtime;
} lerpFrame_t;

// debugging values:

extern int   debug_anim_current;
extern int   debug_anim_old;
extern float debug_anim_blend;

//======================================================================

//attachment system
typedef enum
{
  AT_STATIC,
  AT_TAG,
  AT_CENT,
  AT_PARTICLE
} attachmentType_t;

//forward declaration for particle_t
struct particle_s;

typedef struct attachment_s
{
	attachmentType_t type;
	bool         attached;

	bool         staticValid;
	bool         tagValid;
	bool         centValid;
	bool         particleValid;

	bool         hasOffset;
	vec3_t           offset;

	vec3_t           lastValidAttachmentPoint;

	//AT_STATIC
	vec3_t origin;

	//AT_TAG
	refEntity_t re; //FIXME: should be pointers?
	refEntity_t parent; //
	qhandle_t   model;
	char        tagName[ MAX_STRING_CHARS ];

	//AT_CENT
	int centNum;

	//AT_PARTICLE
	struct particle_s *particle;
} attachment_t;

//======================================================================

//particle system stuff
#define MAX_PARTICLE_FILES        128

#define MAX_PS_SHADER_FRAMES      32
#define MAX_PS_MODELS             8
#define MAX_EJECTORS_PER_SYSTEM   4
#define MAX_PARTICLES_PER_EJECTOR 4

#define MAX_BASEPARTICLE_SYSTEMS  192
#define MAX_BASEPARTICLE_EJECTORS MAX_BASEPARTICLE_SYSTEMS * MAX_EJECTORS_PER_SYSTEM
#define MAX_BASEPARTICLES         MAX_BASEPARTICLE_EJECTORS * MAX_PARTICLES_PER_EJECTOR

#define MAX_PARTICLE_SYSTEMS      48
#define MAX_PARTICLE_EJECTORS     MAX_PARTICLE_SYSTEMS * MAX_EJECTORS_PER_SYSTEM
#define MAX_PARTICLES             MAX_PARTICLE_EJECTORS * 5

#define PARTICLES_INFINITE        -1
#define PARTICLES_SAME_AS_INITIAL -2

//COMPILE TIME STRUCTURES
typedef enum
{
  PMT_STATIC,
  PMT_STATIC_TRANSFORM,
  PMT_TAG,
  PMT_CENT_ANGLES,
  PMT_NORMAL,
  PMT_LAST_NORMAL,
  PMT_OPPORTUNISTIC_NORMAL
} pMoveType_t;

typedef enum
{
  PMD_LINEAR,
  PMD_POINT
} pDirType_t;

typedef struct pMoveValues_u
{
	pDirType_t dirType;

	//PMD_LINEAR
	vec3_t dir;
	float  dirRandAngle;

	//PMD_POINT
	vec3_t point;
	float  pointRandAngle;

	float  mag;
	float  magRandFrac;

	float  parentVelFrac;
	float  parentVelFracRandFrac;
} pMoveValues_t;

typedef struct pLerpValues_s
{
	int   delay;
	float delayRandFrac;

	float initial;
	float initialRandFrac;

	float final;
	float finalRandFrac;

	float randFrac;
} pLerpValues_t;

//particle template
typedef struct baseParticle_s
{
	vec3_t        displacement;
	vec3_t        randDisplacement;
	float         normalDisplacement;

	pMoveType_t   velMoveType;
	pMoveValues_t velMoveValues;

	pMoveType_t   accMoveType;
	pMoveValues_t accMoveValues;

	int           lifeTime;
	float         lifeTimeRandFrac;

	float         bounceFrac;
	float         bounceFracRandFrac;
	bool      bounceCull;

	char          bounceMarkName[ MAX_QPATH ];
	qhandle_t     bounceMark;
	float         bounceMarkRadius;
	float         bounceMarkRadiusRandFrac;
	float         bounceMarkCount;
	float         bounceMarkCountRandFrac;

	char          bounceSoundName[ MAX_QPATH ];
	qhandle_t     bounceSound;
	float         bounceSoundCount;
	float         bounceSoundCountRandFrac;

	pLerpValues_t radius;
	int           physicsRadius;
	pLerpValues_t alpha;
	pLerpValues_t rotation;

	bool      dynamicLight;
	pLerpValues_t dLightRadius;
	byte          dLightColor[ 3 ];

	int           colorDelay;
	float         colorDelayRandFrac;
	byte          initialColor[ 3 ];
	byte          finalColor[ 3 ];

	char          childSystemName[ MAX_QPATH ];
	qhandle_t     childSystemHandle;

	char          onDeathSystemName[ MAX_QPATH ];
	qhandle_t     onDeathSystemHandle;

	char          childTrailSystemName[ MAX_QPATH ];
	qhandle_t     childTrailSystemHandle;

	//particle invariant stuff
	char        shaderNames[ MAX_PS_SHADER_FRAMES ][ MAX_QPATH ];
	qhandle_t   shaders[ MAX_PS_SHADER_FRAMES ];
	int         numFrames;
	float       framerate;

	char        modelNames[ MAX_PS_MODELS ][ MAX_QPATH ];
	qhandle_t   models[ MAX_PS_MODELS ];
	int         numModels;
	animation_t modelAnimation;

	bool    overdrawProtection;
	bool    realLight;
	bool    cullOnStartSolid;

	float       scaleWithCharge;
} baseParticle_t;

//ejector template
typedef struct baseParticleEjector_s
{
	baseParticle_t *particles[ MAX_PARTICLES_PER_EJECTOR ];
	int            numParticles;

	pLerpValues_t  eject; //zero period indicates creation of all particles at once

	int            totalParticles; //can be infinite
	float          totalParticlesRandFrac;
} baseParticleEjector_t;

//particle system template
typedef struct baseParticleSystem_s
{
	char                  name[ MAX_QPATH ];
	baseParticleEjector_t *ejectors[ MAX_EJECTORS_PER_SYSTEM ];
	int                   numEjectors;

	bool              thirdPersonOnly;
	bool              registered; //whether or not the assets for this particle have been loaded
} baseParticleSystem_t;

//RUN TIME STRUCTURES
typedef struct particleSystem_s
{
	baseParticleSystem_t  *class_;

	attachment_t attachment;

	bool     valid;
	bool     lazyRemove; //mark this system for later removal

	//for PMT_NORMAL
	bool normalValid;
	vec3_t   normal;
	//for PMT_LAST_NORMAL and PMT_OPPORTUNISTIC_NORMAL
	bool lastNormalIsCurrent;
	vec3_t   lastNormal;

	int      charge;
} particleSystem_t;

typedef struct particleEjector_s
{
	baseParticleEjector_t *class_;
	particleSystem_t *parent;

	pLerpValues_t    ejectPeriod;

	int              count;
	int              totalParticles;

	int              nextEjectionTime;

	bool         valid;
} particleEjector_t;

//used for actual particle evaluation
typedef struct particle_s
{
	baseParticle_t    *class_;
	particleEjector_t *parent;

	particleSystem_t  *childParticleSystem;

	int               birthTime;
	int               lifeTime;

	float             bounceMarkRadius;
	int               bounceMarkCount;
	int               bounceSoundCount;
	bool          atRest;

	vec3_t            origin;
	vec3_t            velocity;

	pMoveType_t       accMoveType;
	pMoveValues_t     accMoveValues;

	int               lastEvalTime;

	int               nextChildTime;

	pLerpValues_t     radius;
	pLerpValues_t     alpha;
	pLerpValues_t     rotation;

	pLerpValues_t     dLightRadius;

	int               colorDelay;

	qhandle_t         model;
	lerpFrame_t       lf;
	vec3_t            lastAxis[ 3 ];

	bool          valid;
	int               frameWhenInvalidated;

	int               sortKey;
} particle_t;

//======================================================================

//trail system stuff
#define MAX_TRAIL_FILES        128

#define MAX_BEAMS_PER_SYSTEM   4

#define MAX_BASETRAIL_SYSTEMS  64
#define MAX_BASETRAIL_BEAMS    MAX_BASETRAIL_SYSTEMS * MAX_BEAMS_PER_SYSTEM

#define MAX_TRAIL_SYSTEMS      32
#define MAX_TRAIL_BEAMS        MAX_TRAIL_SYSTEMS * MAX_BEAMS_PER_SYSTEM
#define MAX_TRAIL_BEAM_NODES   128

#define MAX_TRAIL_BEAM_JITTERS 4

typedef enum
{
  TBTT_STRETCH,
  TBTT_REPEAT
} trailBeamTextureType_t;

typedef struct baseTrailJitter_s
{
	float magnitude;
	int   period;
} baseTrailJitter_t;

//beam template
typedef struct baseTrailBeam_s
{
	int   numSegments;
	float frontWidth;
	float backWidth;
	float frontAlpha;
	float backAlpha;
	byte  frontColor[ 3 ];
	byte  backColor[ 3 ];

	// the time it takes for a segment to vanish (single attached only)
	int segmentTime;

	// the time it takes for a beam to fade out (double attached only)
	int                    fadeOutTime;

	char                   shaderName[ MAX_QPATH ];
	qhandle_t              shader;

	trailBeamTextureType_t textureType;

	//TBTT_STRETCH
	float frontTextureCoord;
	float backTextureCoord;

	//TBTT_REPEAT
	float             repeatLength;
	bool          clampToBack;

	bool          realLight;

	int               numJitters;
	baseTrailJitter_t jitters[ MAX_TRAIL_BEAM_JITTERS ];
	bool          jitterAttachments;
} baseTrailBeam_t;

//trail system template
typedef struct baseTrailSystem_s
{
	char            name[ MAX_QPATH ];
	baseTrailBeam_t *beams[ MAX_BEAMS_PER_SYSTEM ];
	int             numBeams;

	int             lifeTime;
	bool        thirdPersonOnly;
	bool        registered; //whether or not the assets for this trail have been loaded
} baseTrailSystem_t;

typedef struct trailSystem_s
{
	baseTrailSystem_t   *class_;

	attachment_t frontAttachment;
	attachment_t backAttachment;

	int          birthTime;
	int          destroyTime;
	bool     valid;
} trailSystem_t;

typedef struct trailBeamNode_s
{
	vec3_t                 refPosition;
	vec3_t                 position;

	int                    timeLeft;

	float                  textureCoord;
	float                  halfWidth;
	byte                   alpha;
	byte                   color[ 3 ];

	vec2_t                 jitters[ MAX_TRAIL_BEAM_JITTERS ];

	struct trailBeamNode_s *prev;

	struct trailBeamNode_s *next;

	bool               used;
} trailBeamNode_t;

typedef struct trailBeam_s
{
	baseTrailBeam_t   *class_;
	trailSystem_t   *parent;

	trailBeamNode_t nodePool[ MAX_TRAIL_BEAM_NODES ];
	trailBeamNode_t *nodes;

	int             lastEvalTime;

	bool        valid;

	int             nextJitterTimes[ MAX_TRAIL_BEAM_JITTERS ];
} trailBeam_t;

//======================================================================

// player entities need to track more information
// than any other type of entity.

// smoothing of view and model for WW transitions
#define   MAXSMOOTHS 32

typedef struct
{
	float  time;
	float  timeMod;

	vec3_t rotAxis;
	float  rotAngle;
} smooth_t;

typedef struct
{
	lerpFrame_t legs, torso, nonseg, weapon, jetpack;
	int         painTime;
	int         painDirection; // flip from 0 to 1

	// machinegun spinning
	float    barrelAngle;
	int      barrelTime;
	bool barrelSpinning;

	vec3_t   lastNormal;
	vec3_t   lastAxis[ 3 ];
	smooth_t sList[ MAXSMOOTHS ];

	vec3_t   lastMinimapPos;
	float    lastMinimapAngle;
	float    minimapFading;
	bool minimapFadingOut;
} playerEntity_t;

typedef struct lightFlareStatus_s
{
	float    lastRadius; //caching of likely flare radius
	float    lastRatio; //caching of likely flare ratio
	int      lastTime; //last time flare was visible/occluded
	bool status; //flare is visible?
	qhandle_t hTest;
} lightFlareStatus_t;

typedef struct buildableStatus_s
{
	int      lastTime; // Last time status was visible
	bool visible; // Status is visible?
} buildableStatus_t;

typedef struct buildableCache_s
{
	vec3_t cachedOrigin; // If any of the values differ from their
	vec3_t cachedAngles; //  cached versions, then the cache is invalid
	vec3_t cachedNormal;
	buildable_t cachedType;
	vec3_t axis[ 3 ];
	vec3_t origin;
} buildableCache_t;

//=================================================

#define MAX_CBEACONS 50

// the point of keeping the beacon data separately from centities is
// to be able to handle virtual beacons (client-side beacons) without
// having to add fake centities

// static data belonging to a specific beacon
// here's all beacon info that can't be deduced at will (e.g. because
// it depends on past states)
typedef struct
{
	bool      old;
	bool      eventFired;

	beaconType_t  type;
	vec3_t        origin;
	int           flags;
	int           oldFlags;
	int           ctime;
	int           etime;
	int           mtime;
	int           data;
	team_t        ownerTeam;
	int           owner;
	int           target;
	float         alphaMod; // A modifier that can be set before the drawing phase begins.

	// cache
	float         dot;
	float         dist;

	// drawing
	vec2_t        pos;
	float         scale;
	float         size;
	vec4_t        color;
	float         t_occlusion;

	bool      clamped;
	vec2_t        clamp_dir;
} cbeacon_t;

typedef struct
{
	// behavior
	int           fadeIn;
	int           fadeOut;
	float         highlightAngle; //angle in config, its cosine on runtime
	float         highlightScale;
	float         fadeMinAlpha;
	float         fadeMaxAlpha;
	float         fadeMinDist; //runtime
	float         fadeMaxDist; //runtime

	// drawing
	vec4_t        colorNeutral;
	vec4_t        colorAlien;
	vec4_t        colorHuman;
	float         fadeInAlpha;
	float         fadeInScale;

	// HUD
	float         hudSize;
	float         hudMinSize;
	float         hudMaxSize;
	float         hudAlpha;
	float         hudAlphaImportant;
	vec2_t        hudCenter;    //runtime
	vec2_t        hudRect[ 2 ]; //runtime

	// minimap
	float         minimapScale;
	float         minimapAlpha;
} beaconsConfig_t;

// strings to display on the libRocket HUD
typedef struct
{
	qhandle_t    icon;
	float        iconAlpha;

	char         name[ 128 ];
	float        nameAlpha;

	char         distance[ 48 ];
	float        distanceAlpha;

	char         info[ 128 ];
	float        infoAlpha;

	char         age[ 48 ];
	float        ageAlpha;

	char         owner[ 128 ];
	float        ownerAlpha;
} beaconRocket_t;

//======================================================================

// centity_t has a direct correspondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
typedef struct centity_s
{
	entityState_t  currentState; // from cg.frame
	entityState_t  nextState; // from cg.nextFrame, if available
	bool       interpolate; // true if next is valid to interpolate to
	bool       currentValid; // true if cg.frame holds this entity

	int            muzzleFlashTime; // move to playerEntity?
	int            muzzleFlashTime2; // move to playerEntity?
	int            muzzleFlashTime3; // move to playerEntity?
	int            previousEvent;
	int            teleportFlag;

	int            trailTime; // so missile trails can handle dropped initial packets
	int            dustTrailTime;
	int            miscTime;
	int            snapShotTime; // last time this entity was found in a snapshot

	playerEntity_t pe;

	int            errorTime; // decay the error from this time
	vec3_t         errorOrigin;
	vec3_t         errorAngles;

	bool       extrapolated; // false if origin / angles is an interpolation
	vec3_t         rawOrigin;
	vec3_t         rawAngles;

	vec3_t         beamEnd;

	// exact interpolated position of entity on this frame
	vec3_t                lerpOrigin;
	vec3_t                lerpAngles;

	lerpFrame_t           lerpFrame;

	buildableAnimNumber_t buildableAnim; //persistent anim number
	buildableAnimNumber_t oldBuildableAnim; //to detect when new anims are set
	bool              buildableIdleAnim; //to check if new idle anim
	particleSystem_t      *buildablePS;
	particleSystem_t      *buildableStatusPS; // used for steady effects like fire
	buildableStatus_t     buildableStatus;
	buildableCache_t      buildableCache; // so we don't recalculate things
	float                 lastBuildableHealth;
	int                   lastBuildableDamageSoundTime;

	vec3_t                overmindEyeAngle;

	lightFlareStatus_t    lfs;

	int               doorState;

	bool              animInit;
	bool              animPlaying;
	bool              animLastState;

	particleSystem_t      *muzzlePS;
	bool              muzzlePsTrigger;

	particleSystem_t      *jetPackPS[ 2 ];
	jetPackState_t        jetPackState;
	lerpFrame_t           jetpackLerpFrame;
	jetpackAnimNumber_t   jetpackAnim;

	particleSystem_t      *entityPS;
	bool              entityPSMissing;

	trailSystem_t         *level2ZapTS[ LEVEL2_AREAZAP_MAX_TARGETS ];
	int                   level2ZapTime;

	trailSystem_t         *muzzleTS;
	int                   muzzleTSDeathTime;

	particleSystem_t      *missilePS;
	trailSystem_t         *missileTS;

	float                 radarVisibility;

	bool              valid;
	bool              oldValid;
	int                   pvsEnterTime;

	struct centity_s      *nextLocation;

	cbeacon_t             beacon;

	// Content flags derived from the entity state
	// HACK: This is not an exact copy of the content flags the server sees.
	// If this is desired, it needs to be made part of entityState_t instead.
	int                   contents;
} centity_t;

//======================================================================

typedef struct markPoly_s
{
	struct markPoly_s *prevMark, *nextMark;

	int               time;
	qhandle_t         markShader;
	bool          alphaFade; // fade alpha instead of rgb
	float             color[ 4 ];
	poly_t            poly;
	polyVert_t        verts[ MAX_VERTS_ON_POLY ];
} markPoly_t;

//======================================================================

typedef struct
{
	int       client;
	int       score;
	int       ping;
	int       time;
	int       team;
	weapon_t  weapon;
	upgrade_t upgrade;
} score_t;

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define MAX_CUSTOM_SOUNDS 32
typedef struct
{
	bool infoValid;

	char     name[ MAX_NAME_LENGTH ];
	team_t   team;

	int      score; // updated by score servercmds
	int      location; // location index for team mode
	int      health; // you only get this info about your teammates
	int      upgrade;
	int      curWeaponClass; // sends current weapon for H, current class for A
	int      credit;

	// when clientinfo is changed, the loading of models/skins/sounds
	// can be deferred until you are dead, to prevent hitches in
	// gameplay
	char        modelName[ MAX_QPATH ];
	char        skinName[ MAX_QPATH ];

	bool    newAnims; // true if using the new mission pack animations
	bool    fixedlegs; // true if legs yaw is always the same as torso yaw
	bool    fixedtorso; // true if torso never changes yaw
	bool    nonsegmented; // true if model is Q2 style nonsegmented
	bool    md5; // true if model is in the md5 model format

	vec3_t      headOffset; // move head in icon views
	footstep_t  footsteps;
	gender_t    gender; // from model

	qhandle_t   legsModel;
	qhandle_t   legsSkin;

	qhandle_t   torsoModel;
	qhandle_t   torsoSkin;

	qhandle_t   headModel;
	qhandle_t   headSkin;

	qhandle_t   nonSegModel; //non-segmented model system
	qhandle_t   nonSegSkin; //non-segmented model system

	qhandle_t   bodyModel; //md5 model format
	qhandle_t   bodySkin; //md5 model format

	qhandle_t   modelIcon;

	animation_t animations[ MAX_PLAYER_TOTALANIMATIONS ];

	sfxHandle_t sounds[ MAX_CUSTOM_SOUNDS ];

	vec_t       modelScale;

	int         torsoControlBone;
	int         leftShoulderBone;
	int         rightShoulderBone;

	int         legBones[ MAX_BONES ];
	int         numLegBones;

	int         weaponAdjusted; // bitmask of all weapons that have hand deltas
	int         handBones[ MAX_BONES ];
	int         numHandBones;

	sfxHandle_t customFootsteps[ 4 ];
	sfxHandle_t customMetalFootsteps[ 4 ];

	char        voice[ MAX_VOICE_NAME_LEN ];
	int         voiceTime;
} clientInfo_t;

typedef struct weaponInfoMode_s
{
	float       flashDlight;
	float       flashDlightIntensity;
	vec3_t      flashDlightColor;
	sfxHandle_t flashSound[ 4 ]; // fast firing weapons randomly choose
	bool    continuousFlash;

	sfxHandle_t firingSound;

	qhandle_t   muzzleParticleSystem;

	bool    alwaysImpact;
	qhandle_t   impactParticleSystem;
	qhandle_t   impactMark;
	qhandle_t   impactMarkSize;
	sfxHandle_t impactSound[ 4 ]; //random impact sound
	sfxHandle_t impactFleshSound[ 4 ]; //random impact sound
	sfxHandle_t reloadSound;
} weaponInfoMode_t;

// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
typedef struct weaponInfo_s
{
	bool         registered;
	bool         md5;
	const char       *humanName;

	qhandle_t        handsModel; // the hands don't actually draw, they just position the weapon
	qhandle_t        weaponModel;
	qhandle_t        barrelModel;
	qhandle_t        flashModel;

	qhandle_t        weaponModel3rdPerson;
	qhandle_t        barrelModel3rdPerson;
	qhandle_t        flashModel3rdPerson;

	animation_t      animations[ MAX_WEAPON_ANIMATIONS ];
	bool         noDrift;

	vec3_t           weaponMidpoint; // so it will rotate centered instead of by tag

	qhandle_t        weaponIcon;
	qhandle_t        ammoIcon;

	qhandle_t        crossHair;
	qhandle_t        crossHairIndicator;
	int              crossHairSize;

	sfxHandle_t      readySound;

	bool         disableIn3rdPerson;
	vec3_t           rotation;
	vec3_t           posOffs;
	char             rotationBone[ 50 ];
	vec_t            scale;

	weaponInfoMode_t wim[ WPM_NUM_WEAPONMODES ];
} weaponInfo_t;

typedef struct upgradeInfo_s
{
	bool  registered;
	const char *humanName;

	qhandle_t upgradeIcon;
} upgradeInfo_t;

typedef struct
{
	qhandle_t classIcon;
} classInfo_t;

typedef struct
{
	bool    looped;
	bool    enabled;

	sfxHandle_t sound;
} sound_t;

typedef struct
{
	qhandle_t   models[ MAX_BUILDABLE_MODELS ];
	animation_t animations[ MAX_BUILDABLE_ANIMATIONS ];

	//same number of sounds as animations
	sound_t  sounds[ MAX_BUILDABLE_ANIMATIONS ];

	qhandle_t buildableIcon;

	bool md5;
} buildableInfo_t;

//======================================================================

typedef struct
{
    vec3_t    boundsMin, boundsMax;
    vec2_t    imageMin, imageMax;
    float     scale;
    qhandle_t image;
} minimapZone_t;

typedef struct
{
    bool     active;
    bool     defined;
    int          lastZone;
    int          nZones;
    float        bgColor[4];
    float        scale;
    struct
    {
        qhandle_t playerArrow, teamArrow;
    } gfx;
    minimapZone_t zones[ MAX_MINIMAP_ZONES ];
} minimap_t;

//======================================================================

typedef struct
{
	vec3_t alienBuildablePos[ MAX_GENTITIES ];
	float  alienBuildableIntensity[ MAX_GENTITIES ];
	int    numAlienBuildables;

	vec3_t humanBuildablePos[ MAX_GENTITIES ];
	float  humanBuildableIntensity[ MAX_GENTITIES ];
	int    numHumanBuildables;

	vec3_t alienClientPos[ MAX_CLIENTS ];
	float  alienClientIntensity[ MAX_CLIENTS ];
	int    numAlienClients;

	vec3_t humanClientPos[ MAX_CLIENTS ];
	float  humanClientIntensity[ MAX_CLIENTS ];
	int    numHumanClients;

	int    lastUpdateTime;
	vec3_t origin;
	vec3_t vangles;
} entityPos_t;

typedef struct
{
	int time;
	int length;
} consoleLine_t;

#define MAX_CONSOLE_TEXT  8192
#define MAX_CONSOLE_LINES 32

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS     16

#define NUM_SAVED_STATES         ( CMD_BACKUP + 2 )

// After this many msec the crosshair name fades out completely
#define CROSSHAIR_CLIENT_TIMEOUT 1000

typedef struct
{
	byte     color[ 3 ];
	bool drawIntersection;
	bool drawFrontline;
} cgBinaryShaderSetting_t;

typedef enum
{
	SAY_TYPE_NONE,
	SAY_TYPE_PUBLIC,
	SAY_TYPE_TEAM,
	SAY_TYPE_ADMIN,
	SAY_TYPE_COMMAND,
} sayType_t;

#include "Filter.h"

struct WeaponOffsets
{
	Vec3 bob;

	Vec3 angles;
	Vec3 angvel;

	WeaponOffsets operator+=( WeaponOffsets );
	WeaponOffsets operator*( float );
};

#define NUM_BINARY_SHADERS 256

typedef struct
{
	int      clientFrame; // incremented each frame

	int      clientNum;

	bool demoPlayback;
	bool loading; // don't defer players at initial startup
	bool intermissionStarted;

	// there are only one or two snapshot_t that are relevent at a time
	int        latestSnapshotNum; // the number of snapshots the client system has received
	int        latestSnapshotTime; // the time from latestSnapshotNum, so we don't need to read the snapshot yet

	snapshot_t *snap; // cg.snap->serverTime <= cg.time
	snapshot_t *nextSnap; // cg.nextSnap->serverTime > cg.time, or nullptr
	snapshot_t activeSnapshots[ 2 ];

	float      frameInterpolation; // (float)( cg.time - cg.frame->serverTime ) /
	// (cg.nextFrame->serverTime - cg.frame->serverTime)

	bool thisFrameTeleport;
	bool nextFrameTeleport;

	int      frametime; // cg.time - cg.oldTime

	int      time; // this is the time value that the client
	// is rendering at.
	int      oldTime; // time at last frame, used for missile trails and prediction checking

	int      physicsTime; // either cg.snap->time or cg.nextSnap->time

	bool mapRestart; // set on a map restart to set back the weapon

	bool renderingThirdPerson; // during deaths, chasecams, etc

	// prediction state
	bool      hyperspace; // true if prediction has hit a trigger_teleport
	playerState_t predictedPlayerState;
	pmoveExt_t    pmext;
	centity_t     predictedPlayerEntity;
	bool      validPPS; // clear until the first call to CG_PredictPlayerState
	int           predictedErrorTime;
	vec3_t        predictedError;

	int           eventSequence;
	int           predictableEvents[ MAX_PREDICTED_EVENTS ];

	float         stepChange; // for stair up smoothing
	int           stepTime;

	float         duckChange; // for duck viewheight smoothing
	int           duckTime;

	float         landChange; // for landing hard
	int           landTime;

	// input state sent to server
	int weaponSelect;

	// auto rotating items
	vec3_t autoAngles;
	vec3_t autoAxis[ 3 ];
	vec3_t autoAnglesFast;
	vec3_t autoAxisFast[ 3 ];

	// view rendering
	refdef_t refdef;
	vec3_t   refdefViewAngles; // will be converted to refdef.viewaxis

	// zoom key
	bool zoomed;
	int      zoomTime;
	float    zoomSensitivity;

	// scoreboard
	int      scoresRequestTime;
	int      numScores;
	int      selectedScore;
	int      teamScores[ 2 ];
	score_t  scores[ MAX_CLIENTS ];
	bool showScores;
	int      scoreFadeTime;
	char     killerName[ MAX_NAME_LENGTH ];
	char     spectatorList[ MAX_STRING_CHARS ]; // list of names
	int      spectatorTime; // next time to offset
	float    spectatorOffset; // current offset from start
	bool scoreInvalidated; // needs update on next RocketUpdate

	// centerprinting
	int  centerPrintTime;
	int  centerPrintCharWidth;
	int  centerPrintY;
	char centerPrint[ MAX_STRING_CHARS ];
	int  centerPrintLines;

	// crosshair client ID
	int      crosshairClientNum;
	int      crosshairClientTime;
	bool crosshairFriend;
	bool crosshairFoe;

	// whether we hit
	int hitTime;

	// attacking player
	int attackerTime;

	// warmup countdown
	int warmupTime;

	//==========================

	int weaponSelectTime;
	int weaponAnimation;
	int weaponAnimationTime;

	// blend blobs
	float damageTime;
	float damageX, damageY, damageValue;

	// view movement
	float    v_dmg_time;
	float    v_dmg_pitch;
	float    v_dmg_roll;

	bool chaseFollow;

	// temp working variables for player view
	float bobfracsin;
	int   bobcycle;
	float xyspeed;

	//minimap
	minimap_t               minimap;

	// development tool
	refEntity_t             testModelEntity;
	refEntity_t             testModelBarrelEntity;
	char                    testModelName[ MAX_QPATH ];
	char                    testModelBarrelName[ MAX_QPATH ];
	bool                testGun;

	int                     spawnTime; // fovwarp
	int                     weapon1Time; // time when BUTTON_ATTACK went t->f f->t
	int                     weapon2Time; // time when BUTTON_ATTACK2 went t->f f->t
	int                     weapon3Time; // time when BUTTON_USE_HOLDABLE went t->f f->t
	bool                weapon1Firing;
	bool                weapon2Firing;
	bool                weapon3Firing;

	int                     poisonedTime;

	vec3_t                  lastNormal; // view smoothage
	vec3_t                  lastVangles; // view smoothage
	smooth_t                sList[ MAXSMOOTHS ]; // WW smoothing

	int                     forwardMoveTime; // for struggling
	int                     rightMoveTime;
	int                     upMoveTime;

	/* loading */
	char                    currentLoadingLabel[ MAX_LOADING_LABEL_LENGTH ];
	float                   charModelFraction; // loading percentages
	float                   mediaFraction;
	float                   buildablesFraction;

	int                     lastBuildAttempt;
	int                     lastEvolveAttempt;

	char                    consoleText[ MAX_CONSOLE_TEXT ];
	consoleLine_t           consoleLines[ MAX_CONSOLE_LINES ];
	int                     numConsoleLines;

	float                   painBlendValue;
	float                   painBlendTarget;
	int                     lastHealth;
	bool                wasDeadLastFrame;

	int                     lastPredictedCommand;
	int                     lastServerTime;
	playerState_t           savedPmoveStates[ NUM_SAVED_STATES ];
	int                     stateHead, stateTail;
	int                     ping;

	float                   chargeMeterAlpha;
	float                   chargeMeterValue;
	qhandle_t               lastHealthCross;
	float                   healthCrossFade;
	int                     nearUsableBuildable;

	int                     nextWeaponClickTime;

	int                     numBinaryShadersUsed;
	cgBinaryShaderSetting_t binaryShaderSettings[ NUM_BINARY_SHADERS ];
	sayType_t               sayType;

	// momentum
	float                   momentumGained;
	int                     momentumGainedTime;

	// beacons
	cbeacon_t               *beacons[ MAX_CBEACONS ];
	int                     beaconCount;
	cbeacon_t               *highlightedBeacon;
	beaconRocket_t          beaconRocket;

	int                     tagScoreTime;

	// pmove params
	struct {
		int synchronous;
		int fixed;
		int msec;
		int accurate;
	} pmoveParams;

	Filter<WeaponOffsets>   weaponOffsetsFilter;
} cg_t;

typedef struct
{
	const char *path;
	const char *id;
} rocketMenu_t;

#define MAX_SERVERS 2048
#define MAX_RESOLUTIONS 32
#define MAX_LANGUAGES 64
#define MAX_INPUTS 16
#define MAX_OUTPUTS 16
#define MAX_MODS 64
#define MAX_DEMOS 256
#define MAX_MAPS 128

typedef struct server_s
{
	char *name;
	char *label;
	int clients;
	int bots;
	int ping;
	int maxClients;
	char *mapName;
	char *addr;
} server_t;



typedef struct resolution_s
{
	int width;
	int height;
} resolution_t;



typedef struct language_s
{
	char *name;
	char *lang;
} language_t;



typedef struct modInfo_s
{
	char *name;
	char *description;
} modInfo_t;

typedef struct
{
	char *mapName;
	char *mapLoadName;
} mapInfo_t;

typedef struct
{
	server_t servers[ AS_FAVORITES + 1 ][ MAX_SERVERS ];
	int serverCount[ AS_FAVORITES + 1 ];
	int serverIndex[ AS_FAVORITES + 1 ];
	bool buildingServerInfo;
	bool retrievingServers;

	resolution_t resolutions[ MAX_RESOLUTIONS ];
	int resolutionCount;
	int resolutionIndex;

	language_t languages[ MAX_LANGUAGES ];
	int languageCount;
	int languageIndex;

	modInfo_t modList[ MAX_MODS ];
	int modCount;
	int modIndex;

	char *voipInputs[ MAX_INPUTS ];
	int voipInputsCount;
	int voipInputIndex;

	char *alOutputs[ MAX_OUTPUTS ];
	int alOutputsCount;
	int alOutputIndex;

	int playerList[ NUM_TEAMS ][ MAX_CLIENTS ];
	int playerCount[ NUM_TEAMS ];
	int playerIndex[ NUM_TEAMS ];

	char *demoList[ MAX_DEMOS ];
	int demoCount;
	int demoIndex;

	mapInfo_t mapList[ MAX_MAPS ];
	int mapCount;
	int mapIndex;

	int selectedTeamIndex;

	int selectedHumanSpawnItem;

	int selectedAlienSpawnClass;

	int armouryBuyList[3][ ( WP_LUCIFER_CANNON - WP_BLASTER ) + UP_NUM_UPGRADES + 1 ];
	int selectedArmouryBuyItem[3];
	int armouryBuyListCount[3];

	int armourySellList[ WP_NUM_WEAPONS + UP_NUM_UPGRADES ];
	int selectedArmourySellItem;
	int armourySellListCount;

	int alienEvolveList[ PCL_NUM_CLASSES ];
	int selectedAlienEvolve;
	int alienEvolveListCount;

	int humanBuildList[ BA_NUM_BUILDABLES ];
	int selectedHumanBuild;
	int humanBuildListCount;

	int alienBuildList[ BA_NUM_BUILDABLES ];
	int selectedAlienBuild;
	int alienBuildListCount;

	int beaconList[ NUM_BEACON_TYPES ];
	int selectedBeacon;
	int beaconListCount;
} rocketDataSource_t;

typedef struct
{
	int currentNetSrc;
	int serversLastRefresh;
	int serverStatusLastRefresh;
	int realtime;
	int keyCatcher;
	char downloadName[ MAX_STRING_CHARS ];
	cgClientState_t cstate;
	rocketMenu_t menu[ ROCKETMENU_NUM_TYPES ];
	rocketMenu_t hud[ WP_NUM_WEAPONS ];
	rocketDataSource_t data;
} rocketInfo_t;

extern rocketInfo_t rocketInfo;

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, and weaponInfo_t

typedef struct
{
	qhandle_t f1;
	qhandle_t f2;
	qhandle_t f3;
	qhandle_t b1;
	qhandle_t b2;
	qhandle_t b3;
} cgMediaBinaryShader_t;

typedef struct
{
	qhandle_t charsetShader;
	qhandle_t whiteShader;
	qhandle_t outlineShader;

	qhandle_t level2ZapTS;

	qhandle_t balloonShader;
	qhandle_t connectionShader;

	qhandle_t viewBloodShader;
	qhandle_t tracerShader;
	qhandle_t crosshairShader[ WP_NUM_WEAPONS ];
	qhandle_t backTileShader;

	qhandle_t creepShader;

	qhandle_t scannerShader;
	qhandle_t scannerBlipShader;
	qhandle_t scannerBlipBldgShader;
	qhandle_t scannerLineShader;

	qhandle_t teamOverlayShader;

	qhandle_t numberShaders[ 11 ];

	qhandle_t shadowMarkShader;
	qhandle_t wakeMarkShader;

	// buildable shaders
	qhandle_t             greenBuildShader;
	qhandle_t             redBuildShader;
	qhandle_t             humanSpawningShader;

	qhandle_t             sphereModel;
	qhandle_t             sphericalCone64Model;
	qhandle_t             sphericalCone240Model;

	qhandle_t             plainColorShader;
	qhandle_t             binaryAlpha1Shader;
	cgMediaBinaryShader_t binaryShaders[ NUM_BINARY_SHADERS ];

	// disconnect
	qhandle_t disconnectPS;
	qhandle_t disconnectSound;

	// sounds
	sfxHandle_t weaponEmptyClick;
	sfxHandle_t selectSound;
	sfxHandle_t footsteps[ FOOTSTEP_TOTAL ][ 4 ];
	sfxHandle_t talkSound;
	sfxHandle_t alienTalkSound;
	sfxHandle_t humanTalkSound;
	sfxHandle_t landSound;
	sfxHandle_t fallSound;
	sfxHandle_t turretSpinupSound;

	sfxHandle_t hardBounceSound1;
	sfxHandle_t hardBounceSound2;

	sfxHandle_t watrInSound;
	sfxHandle_t watrOutSound;
	sfxHandle_t watrUnSound;

	sfxHandle_t jetpackThrustLoopSound;

	qhandle_t   jetPackThrustPS;

	sfxHandle_t medkitUseSound;

	sfxHandle_t weHaveEvolved;
	sfxHandle_t reinforcement;

	sfxHandle_t alienOvermindAttack;
	sfxHandle_t alienOvermindDying;
	sfxHandle_t alienOvermindSpawns;

	sfxHandle_t alienBuildableExplosion;
	sfxHandle_t alienBuildablePrebuild;
	sfxHandle_t humanBuildableDying;
	sfxHandle_t humanBuildableExplosion;
	sfxHandle_t humanBuildablePrebuild;
	sfxHandle_t humanBuildableDamage[ 4 ];

	sfxHandle_t alienL4ChargePrepare;
	sfxHandle_t alienL4ChargeStart;

	qhandle_t   cursor;
	qhandle_t   selectCursor;
	qhandle_t   sizeCursor;

	//light armour
	qhandle_t   larmourHeadSkin;
	qhandle_t   larmourLegsSkin;
	qhandle_t   larmourTorsoSkin;

	qhandle_t   jetpackModel;
	qhandle_t   jetpackFlashModel;
	qhandle_t   radarModel;

	sfxHandle_t repeaterUseSound;

	sfxHandle_t buildableRepairSound;
	sfxHandle_t buildableRepairedSound;

	qhandle_t   alienEvolvePS;
	qhandle_t   alienAcidTubePS;
	qhandle_t   alienBoosterPS;

	sfxHandle_t alienEvolveSound;

	qhandle_t   humanBuildableDamagedPS;
	qhandle_t   humanBuildableDestroyedPS;
	qhandle_t   humanBuildableNovaPS;
	qhandle_t   alienBuildableDamagedPS;
	qhandle_t   alienBuildableDestroyedPS;

	qhandle_t   alienBleedPS;
	qhandle_t   humanBleedPS;
	qhandle_t   alienBuildableBleedPS;
	qhandle_t   humanBuildableBleedPS;
	qhandle_t   alienBuildableBurnPS;

	qhandle_t   floorFirePS;

	qhandle_t   teslaZapTS;

	sfxHandle_t lCannonWarningSound;
	sfxHandle_t lCannonWarningSound2;

	sfxHandle_t rocketpodLockonSound;

	qhandle_t   buildWeaponTimerPie[ 8 ];
	qhandle_t   healthCross;
	qhandle_t   healthCross2X;
	qhandle_t   healthCross3X;
	qhandle_t   healthCrossMedkit;
	qhandle_t   healthCrossPoisoned;

	qhandle_t   neutralCgrade;
	qhandle_t   redCgrade;
	qhandle_t   tealCgrade;
	qhandle_t   desaturatedCgrade;

	qhandle_t   scopeShader;

	animation_t jetpackAnims[ MAX_JETPACK_ANIMATIONS ];

	qhandle_t   beaconIconArrow;
	qhandle_t   beaconNoTarget;
	qhandle_t   beaconTagScore;

	sfxHandle_t timerBeaconExpiredSound;
} cgMedia_t;

typedef struct
{
	qhandle_t frameShader;
	qhandle_t overlayShader;
	qhandle_t noPowerShader;
	qhandle_t markedShader;
	vec4_t    healthSevereColor;
	vec4_t    healthHighColor;
	vec4_t    healthElevatedColor;
	vec4_t    healthGuardedColor;
	vec4_t    healthLowColor;
	int       frameHeight;
	int       frameWidth;
	int       healthPadding;
	int       overlayHeight;
	int       overlayWidth;
	float     verticalMargin;
	float     horizontalMargin;
	vec4_t    foreColor;
	vec4_t    backColor;
	bool  loaded;
} buildStat_t;

// The client game static (cgs) structure hold everything
// loaded or calculated from the gamestate.  It will NOT
// be cleared when a tournement restart is done, allowing
// all clients to begin playing instantly
typedef struct
{
	GameStateCSs gameState; // gamestate from server
	glconfig_t  glconfig; // rendering configuration
	float       screenXScale; // derived from glconfig
	float       screenYScale;
	float       screenXBias;
	float       aspectScale;

	int         processedSnapshotNum; // the number of snapshots cgame has requested

	// parsed from serverinfo
	int      timelimit;
	int      maxclients;
	char     mapname[ MAX_QPATH ];
	int      powerReactorRange;
	int      powerRepeaterRange;
	float    momentumHalfLife; // used for momentum bar (un)lock markers
	float    unlockableMinTime;  // used for momentum bar (un)lock markers

	int      voteTime[ NUM_TEAMS ];
	int      voteYes[ NUM_TEAMS ];
	int      voteNo[ NUM_TEAMS ];
	char     voteCaller[ NUM_TEAMS ][ MAX_NAME_LENGTH ];
	bool voteModified[ NUM_TEAMS ]; // beep whenever changed
	char     voteString[ NUM_TEAMS ][ MAX_STRING_TOKENS ];

	int      levelStartTime;

	//
	// locally derived information from gamestate
	//
	qhandle_t    gameModels[ MAX_MODELS ];
	qhandle_t    gameShaders[ MAX_GAME_SHADERS ];
	qhandle_t    gameGradingTextures[ MAX_GRADING_TEXTURES ];
	qhandle_t    gameGradingModels[ MAX_GRADING_TEXTURES ];
	float        gameGradingDistances[ MAX_GRADING_TEXTURES ];
	char         gameReverbEffects[ MAX_REVERB_EFFECTS ][ MAX_NAME_LENGTH ];
	qhandle_t    gameReverbModels[ MAX_REVERB_EFFECTS ];
	float        gameReverbDistances[ MAX_REVERB_EFFECTS ];
	float        gameReverbIntensities[ MAX_REVERB_EFFECTS ];
	qhandle_t    gameParticleSystems[ MAX_GAME_PARTICLE_SYSTEMS ];
	sfxHandle_t  gameSounds[ MAX_SOUNDS ];

	int          numInlineModels;
	qhandle_t    inlineDrawModel[ MAX_SUBMODELS ];
	vec3_t       inlineModelMidpoints[ MAX_SUBMODELS ];

	clientInfo_t clientinfo[ MAX_CLIENTS ];

	bool     teamInfoReceived;

	// corpse info
	clientInfo_t corpseinfo[ MAX_CLIENTS ];

	int          cursorX;
	int          cursorY;
	void         *capturedItem;
	qhandle_t    activeCursor;

	buildStat_t  alienBuildStat;
	buildStat_t  humanBuildStat;

	// media
	cgMedia_t    media;

	voice_t      *voices;
	clientList_t ignoreList;

	beaconsConfig_t  bc;
} cgs_t;

typedef struct
{
	const char *cmd;
	void     ( *function )();
} consoleCommand_t;

typedef enum
{
  SHC_DARK_BLUE,
  SHC_LIGHT_BLUE,
  SHC_GREEN_CYAN,
  SHC_VIOLET,
  SHC_INDIGO,
  SHC_YELLOW,
  SHC_ORANGE,
  SHC_LIGHT_GREEN,
  SHC_DARK_GREEN,
  SHC_RED,
  SHC_PINK,
  SHC_GREY,
  SHC_NUM_SHADER_COLORS
} shaderColorEnum_t;

typedef enum
{
	RM_SPHERE,
	RM_SPHERICAL_CONE_64,
	RM_SPHERICAL_CONE_240,
} rangeMarker_t;

typedef enum
{
	ELEMENT_ALL,
	ELEMENT_GAME,
	ELEMENT_LOADING,
	ELEMENT_HUMANS,
	ELEMENT_ALIENS,
	ELEMENT_DEAD,
	ELEMENT_BOTH,
} rocketElementType_t;

typedef enum
{
  CG_ALTSHADER_DEFAULT, // must be first
  CG_ALTSHADER_UNPOWERED,
  CG_ALTSHADER_DEAD
} altShader_t;

//==============================================================================

extern  cgs_t               cgs;
extern  cg_t                cg;
extern  centity_t           cg_entities[ MAX_GENTITIES ];

extern  weaponInfo_t        cg_weapons[ 32 ];
extern  upgradeInfo_t       cg_upgrades[ 32 ];
extern  classInfo_t         cg_classes[ PCL_NUM_CLASSES ];
extern  buildableInfo_t     cg_buildables[ BA_NUM_BUILDABLES ];

extern  const vec3_t        cg_shaderColors[ SHC_NUM_SHADER_COLORS ];

extern  markPoly_t          cg_markPolys[ MAX_MARK_POLYS ];

extern  vmCvar_t            cg_teslaTrailTime;
extern  vmCvar_t            cg_centertime;
extern  vmCvar_t            cg_runpitch;
extern  vmCvar_t            cg_runroll;
extern  vmCvar_t            cg_swingSpeed;
extern  vmCvar_t            cg_shadows;
extern  vmCvar_t            cg_playerShadows;
extern  vmCvar_t            cg_buildableShadows;
extern  vmCvar_t            cg_drawTimer;
extern  vmCvar_t            cg_drawClock;
extern  vmCvar_t            cg_drawFPS;
extern  vmCvar_t            cg_drawDemoState;
extern  vmCvar_t            cg_drawSnapshot;
extern  vmCvar_t            cg_drawChargeBar;
extern  vmCvar_t            cg_drawCrosshair;
extern  vmCvar_t            cg_drawCrosshairHit;
extern  vmCvar_t            cg_drawCrosshairFriendFoe;
extern  vmCvar_t            cg_drawCrosshairNames;
extern  vmCvar_t            cg_drawBuildableHealth;
extern  vmCvar_t            cg_drawMinimap;
extern  vmCvar_t            cg_minimapActive;
extern  vmCvar_t            cg_crosshairSize;
extern  vmCvar_t            cg_crosshairFile;
extern  vmCvar_t            cg_drawTeamOverlay;
extern  vmCvar_t            cg_teamOverlaySortMode;
extern  vmCvar_t            cg_teamOverlayMaxPlayers;
extern  vmCvar_t            cg_teamOverlayUserinfo;
extern  vmCvar_t            cg_draw2D;
extern  vmCvar_t            cg_animSpeed;
extern  vmCvar_t            cg_debugAnim;
extern  vmCvar_t            cg_debugPosition;
extern  vmCvar_t            cg_debugEvents;
extern  vmCvar_t            cg_errorDecay;
extern  vmCvar_t            cg_nopredict;
extern  vmCvar_t            cg_debugMove;
extern  vmCvar_t            cg_noPlayerAnims;
extern  vmCvar_t            cg_showmiss;
extern  vmCvar_t            cg_footsteps;
extern  vmCvar_t            cg_addMarks;
extern  vmCvar_t            cg_viewsize;
extern  vmCvar_t            cg_drawGun;
extern  vmCvar_t            cg_gun_frame;
extern  vmCvar_t            cg_gun_x;
extern  vmCvar_t            cg_gun_y;
extern  vmCvar_t            cg_gun_z;
extern  vmCvar_t            cg_mirrorgun;
extern  vmCvar_t            cg_tracerChance;
extern  vmCvar_t            cg_tracerWidth;
extern  vmCvar_t            cg_tracerLength;
extern  vmCvar_t            cg_thirdPerson;
extern  vmCvar_t            cg_thirdPersonAngle;
extern  vmCvar_t            cg_thirdPersonShoulderViewMode;
extern  vmCvar_t            cg_staticDeathCam;
extern  vmCvar_t            cg_thirdPersonPitchFollow;
extern  vmCvar_t            cg_thirdPersonRange;
extern  vmCvar_t            cg_lagometer;
extern  vmCvar_t            cg_drawSpeed;
extern  vmCvar_t            cg_maxSpeedTimeWindow;
extern  vmCvar_t            cg_stats;
extern  vmCvar_t            cg_paused;
extern  vmCvar_t            cg_blood;
extern  vmCvar_t            cg_teamOverlayUserinfo;
extern  vmCvar_t            cg_teamChatsOnly;
extern  vmCvar_t            cg_noVoiceChats;
extern  vmCvar_t            cg_noVoiceText;
extern  vmCvar_t            cg_hudFiles;
extern  vmCvar_t            cg_hudFilesEnable;
extern  vmCvar_t            cg_smoothClients;
extern  vmCvar_t            cg_timescaleFadeEnd;
extern  vmCvar_t            cg_timescaleFadeSpeed;
extern  vmCvar_t            cg_timescale;
extern  vmCvar_t            cg_noTaunt;
extern  vmCvar_t            cg_drawSurfNormal;
extern  vmCvar_t            cg_drawBBOX;
extern  vmCvar_t            cg_drawEntityInfo;
extern  vmCvar_t            cg_wwSmoothTime;
extern  vmCvar_t            cg_disableBlueprintErrors;
extern  vmCvar_t            cg_depthSortParticles;
extern  vmCvar_t            cg_bounceParticles;
extern  vmCvar_t            cg_consoleLatency;
extern  vmCvar_t            cg_lightFlare;
extern  vmCvar_t            cg_debugParticles;
extern  vmCvar_t            cg_debugTrails;
extern  vmCvar_t            cg_debugPVS;
extern  vmCvar_t            cg_disableWarningDialogs;
extern  vmCvar_t            cg_disableUpgradeDialogs;
extern  vmCvar_t            cg_disableBuildDialogs;
extern  vmCvar_t            cg_disableCommandDialogs;
extern  vmCvar_t            cg_disableScannerPlane;
extern  vmCvar_t            cg_tutorial;

extern  vmCvar_t            cg_rangeMarkerDrawSurface;
extern  vmCvar_t            cg_rangeMarkerDrawIntersection;
extern  vmCvar_t            cg_rangeMarkerDrawFrontline;
extern  vmCvar_t            cg_rangeMarkerSurfaceOpacity;
extern  vmCvar_t            cg_rangeMarkerLineOpacity;
extern  vmCvar_t            cg_rangeMarkerLineThickness;
extern  vmCvar_t            cg_rangeMarkerForBlueprint;
extern  vmCvar_t            cg_rangeMarkerBuildableTypes;
extern  vmCvar_t            cg_rangeMarkerWhenSpectating;
extern  vmCvar_t            cg_buildableRangeMarkerMask;
extern  vmCvar_t            cg_binaryShaderScreenScale;

extern  vmCvar_t            cg_painBlendUpRate;
extern  vmCvar_t            cg_painBlendDownRate;
extern  vmCvar_t            cg_painBlendMax;
extern  vmCvar_t            cg_painBlendScale;
extern  vmCvar_t            cg_painBlendZoom;

extern  vmCvar_t            cg_stickySpec;
extern  vmCvar_t            cg_sprintToggle;
extern  vmCvar_t            cg_unlagged;

extern  vmCvar_t            cg_cmdGrenadeThrown;
extern  vmCvar_t            cg_cmdNeedHealth;

extern  vmCvar_t            cg_debugVoices;

extern  vmCvar_t            ui_currentClass;
extern  vmCvar_t            ui_carriage;
extern  vmCvar_t            ui_dialog;
extern  vmCvar_t            ui_voteActive;
extern  vmCvar_t            ui_alienTeamVoteActive;
extern  vmCvar_t            ui_humanTeamVoteActive;
extern  vmCvar_t            ui_unlockables;

extern vmCvar_t             cg_debugRandom;

extern vmCvar_t             cg_optimizePrediction;
extern vmCvar_t             cg_projectileNudge;

extern vmCvar_t             cg_voice;

extern vmCvar_t             cg_emoticons;
extern vmCvar_t             cg_emoticonsInMessages;

extern vmCvar_t             cg_chatTeamPrefix;

extern vmCvar_t             cg_animBlend;

extern vmCvar_t             cg_highPolyPlayerModels;
extern vmCvar_t             cg_highPolyBuildableModels;
extern vmCvar_t             cg_highPolyWeaponModels;
extern vmCvar_t             cg_motionblur;
extern vmCvar_t             cg_motionblurMinSpeed;
extern vmCvar_t             ui_chatPromptColors;
extern vmCvar_t             cg_spawnEffects;
extern vmCvar_t             cg_sayCommand;

//
// Rocket cvars
//

extern vmCvar_t            rocket_hudFile;
extern vmCvar_t            rocket_menuFile;
//
// cg_main.c
//
const char *CG_ConfigString( int index );
const char *CG_Argv( int arg );
const char *CG_Args();

void QDECL CG_Printf( const char *msg, ... ) PRINTF_LIKE(1);
void QDECL NORETURN CG_Error( const char *msg, ... ) PRINTF_LIKE(1);

void       CG_StartMusic();

void       CG_NotifyHooks();
void       CG_UpdateCvars();

int        CG_CrosshairPlayer();
void       CG_LoadMenus( const char *menuFile );
void       CG_KeyEvent( int key, bool down );
void       CG_MouseEvent( int x, int y );
bool   CG_ClientIsReady( int clientNum );
void       CG_BuildSpectatorString();

bool   CG_FileExists( const char *filename );
void       CG_RemoveNotifyLine();
void       CG_AddNotifyText();
void       CG_UpdateBuildableRangeMarkerMask();
void       CG_RegisterGrading( int slot, const char *str );

//
// cg_view.c
//
void     CG_addSmoothOp( vec3_t rotAxis, float rotAngle, float timeMod );
void     CG_TestModel_f();
void     CG_TestGun_f();
void     CG_TestModelNextFrame_f();
void     CG_TestModelPrevFrame_f();
void     CG_TestModelNextSkin_f();
void     CG_TestModelPrevSkin_f();
void     CG_AddBufferedSound( sfxHandle_t sfx );
bool CG_CullBox(vec3_t mins, vec3_t maxs);
bool CG_CullPointAndRadius(const vec3_t pt, vec_t radius);
void     CG_DrawActiveFrame( int serverTime, bool demoPlayback );
void     CG_OffsetFirstPersonView();
void     CG_OffsetThirdPersonView();
void     CG_OffsetShoulderView();
void     CG_StartShadowCaster( vec3_t origin, vec3_t mins, vec3_t maxs );
void     CG_EndShadowCaster();

//
// cg_drawtools.c
//
void     CG_DrawPlane( vec3_t origin, vec3_t down, vec3_t right, qhandle_t shader );
void     CG_AdjustFrom640( float *x, float *y, float *w, float *h );
void     CG_FillRect( float x, float y, float width, float height, const float *color );
void     CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void     CG_DrawRotatedPic( float x, float y, float width, float height, qhandle_t hShader, float angle );
void     CG_DrawNoStretchPic( float x, float y, float width, float height, qhandle_t hShader );
void     CG_DrawFadePic( float x, float y, float width, float height, vec4_t fcolor,
                         vec4_t tcolor, float amount, qhandle_t hShader );
void     CG_SetClipRegion( float x, float y, float w, float h );
void     CG_ClearClipRegion();
void     CG_EnableScissor( bool enable );
void     CG_SetScissor( int x, int y, int w, int h );

int      CG_DrawStrlen( const char *str );

float    *CG_FadeColor( int startMsec, int totalMsec );
float    CG_FadeAlpha( int startMsec, int totalMsec );
void     CG_TileClear();
void     CG_DrawRect( float x, float y, float width, float height, float size, const float *color );
void     CG_DrawSides( float x, float y, float w, float h, float size );
void     CG_DrawTopBottom( float x, float y, float w, float h, float size );
bool CG_WorldToScreen( vec3_t point, float *x, float *y );
char     CG_GetColorCharForHealth( int clientnum );
void     CG_DrawSphere( const vec3_t center, float radius, int customShader, const float *shaderRGBA );
void     CG_DrawSphericalCone( const vec3_t tip, const vec3_t rotation, float radius,
                               bool a240, int customShader, const float *shaderRGBA );
void     CG_DrawRangeMarker( rangeMarker_t rmType, const vec3_t origin, float range, const vec3_t angles, vec4_t rgba );

#define CG_ExponentialFade( value, target, lambda ) \
ExponentialFade( (value), (target), (lambda), (float)cg.frametime * 0.001 );

//
// cg_draw.c
//

void CG_AlignText( rectDef_t *rect, const char *text, float scale, float w, float h, int align, int valign,float *x, float *y );
void CG_AddLagometerFrameInfo();
void CG_AddLagometerSnapshotInfo( snapshot_t *snap );
void CG_AddSpeed();
void CG_CenterPrint( const char *str, int y, int charWidth );
void CG_DrawActive();
void CG_OwnerDraw( rectDef_t *rect, float text_x,
                   float text_y, int ownerDraw, int ownerDrawFlags,
                   int align, int textalign, int textvalign,
                   float borderSize, float scale, vec4_t foreColor,
                   vec4_t backColor, qhandle_t shader, int textStyle );
float      CG_GetValue( int ownerDraw );
void       CG_RunMenuScript( char **args );
void       CG_SetPrintString( int type, const char *p );
const char *CG_GetKillerText();
void       CG_DrawLoadingScreen();
void       CG_ResetPainBlend();
void       CG_DrawField( float x, float y, int width, float cw, float ch, int value );

//
// cg_players.c
//
void        CG_Player( centity_t *cent );
void        CG_Corpse( centity_t *cent );
void        CG_ResetPlayerEntity( centity_t *cent );
void        CG_NewClientInfo( int clientNum );

void        CG_PrecacheClientInfo( class_t class_, const char *model, const char *skin );
sfxHandle_t CG_CustomSound( int clientNum, const char *soundName );
void        CG_PlayerDisconnect( vec3_t org );
centity_t   *CG_GetLocation( vec3_t );
centity_t   *CG_GetPlayerLocation();

void        CG_InitClasses();

//
// cg_buildable.c
//
void     CG_GhostBuildable( int buildableInfo );
void     CG_Buildable( centity_t *cent );
void     CG_BuildableStatusParse( const char *filename, buildStat_t *bs );
void     CG_DrawBuildableStatus();
void     CG_InitBuildables();
void     CG_HumanBuildableDying( buildable_t buildable, vec3_t origin );
void     CG_HumanBuildableExplosion( buildable_t buildable, vec3_t origin, vec3_t dir );
void     CG_AlienBuildableExplosion( vec3_t origin, vec3_t dir );
bool CG_GetBuildableRangeMarkerProperties( buildable_t bType, rangeMarker_t *rmType, float *range, vec3_t rgb );

//
// cg_animation.c
//
void CG_RunLerpFrame( lerpFrame_t *lf, float scale );
void CG_RunMD5LerpFrame( lerpFrame_t *lf, float scale, bool animChanged );
void CG_BlendLerpFrame( lerpFrame_t *lf );
void CG_BuildAnimSkeleton( const lerpFrame_t *lf, refSkeleton_t *newSkeleton, const refSkeleton_t *oldSkeleton );

//
// cg_animmapobj.c
//
void CG_AnimMapObj( centity_t *cent );
void CG_ModelDoor( centity_t *cent );

//
// cg_predict.c
//

void CG_BuildSolidList();
int  CG_PointContents( const vec3_t point, int passEntityNum );
void CG_Trace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs,
               const vec3_t end, int skipNumber, int mask, int skipmask );
void CG_CapTrace( trace_t *result, const vec3_t start, const vec3_t mins, const vec3_t maxs,
                  const vec3_t end, int skipNumber, int mask, int skipmask );
void CG_BiSphereTrace( trace_t *result, const vec3_t start, const vec3_t end,
                       const float startRadius, const float endRadius, int skipNumber, int mask,
                       int skipmask );
void CG_PredictPlayerState();

//
// cg_events.c
//
void CG_CheckEvents( centity_t *cent );
void CG_EntityEvent( centity_t *cent, vec3_t position );
void CG_PainEvent( centity_t *cent, int health );
void CG_OnPlayerWeaponChange();
void CG_OnPlayerUpgradeChange();
void CG_OnMapRestart();

//
// cg_ents.c
//
void CG_DrawBoundingBox( int type, vec3_t origin, vec3_t mins, vec3_t maxs );
void CG_SetEntitySoundPosition( centity_t *cent );
void CG_AddPacketEntities();
void CG_Beam( centity_t *cent );
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out,
                                vec3_t angles_in, vec3_t angles_out );
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
                             qhandle_t parentModel, const char *tagName );
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
                                    qhandle_t parentModel, const char *tagName );
void CG_TransformSkeleton( refSkeleton_t *skel, const vec_t scale );

//
// cg_weapons.c
//
void CG_NextWeapon_f();
void CG_PrevWeapon_f();
void CG_Weapon_f();

void CG_InitUpgrades();
void CG_RegisterUpgrade( int upgradeNum );
void CG_InitWeapons();
void CG_RegisterWeapon( int weaponNum );
bool CG_RegisterWeaponAnimation( animation_t *anim, const char *filename, bool loop, bool reversed,
    bool clearOrigin );

void CG_HandleFireWeapon( centity_t *cent, weaponMode_t weaponMode );
void CG_HandleFireShotgun( entityState_t *es );

void CG_HandleWeaponHitEntity( entityState_t *es, vec3_t origin );
void CG_HandleWeaponHitWall( entityState_t *es, vec3_t origin );
void CG_HandleMissileHitEntity( entityState_t *es, vec3_t origin );
void CG_HandleMissileHitWall( entityState_t *es, vec3_t origin );

void CG_AddViewWeapon( playerState_t *ps );
void CG_AddPlayerWeapon( refEntity_t *parent, playerState_t *ps, centity_t *cent );
void CG_DrawHumanInventory();
void CG_DrawItemSelectText();
float CG_ChargeProgress();

//
// cg_minimap.c
//
void CG_InitMinimap();
void CG_DrawMinimap( const rectDef_t *rect, const vec4_t color );

//
// cg_marks.c
//
void CG_InitMarkPolys();
void CG_AddMarks();
void CG_ImpactMark( qhandle_t markShader,
                    const vec3_t origin, const vec3_t dir,
                    float orientation,
                    float r, float g, float b, float a,
                    bool alphaFade,
                    float radius, bool temporary );

//
// cg_snapshot.c
//
void CG_ProcessSnapshots();

//
// cg_consolecmds.c
//
bool ConsoleCommand();
void     CG_InitConsoleCommands();
void     CG_RequestScores();
void     CG_HideScores_f();
void     CG_ShowScores_f();

//
// cg_servercmds.c
//
void CG_ExecuteServerCommands( snapshot_t* snap );
void CG_ParseServerinfo();
void CG_SetConfigValues();
void CG_ShaderStateChanged();
void CG_CompleteCommand( int argNum );
void CG_CenterPrint_f();

//
// cg_playerstate.c
//
void CG_Respawn();
void CG_TransitionPlayerState( playerState_t *ps, playerState_t *ops );
void CG_CheckChangedPredictableEvents( playerState_t *ps );

//
// cg_attachment.c
//
bool CG_AttachmentPoint( attachment_t *a, vec3_t v );
bool CG_AttachmentDir( attachment_t *a, vec3_t v );

bool CG_AttachmentAxis( attachment_t *a, vec3_t axis[ 3 ] );
bool CG_AttachmentVelocity( attachment_t *a, vec3_t v );
int      CG_AttachmentCentNum( attachment_t *a );

bool CG_Attached( attachment_t *a );

void     CG_AttachToPoint( attachment_t *a );
void     CG_AttachToCent( attachment_t *a );
void     CG_AttachToTag( attachment_t *a );
void     CG_AttachToParticle( attachment_t *a );
void     CG_SetAttachmentPoint( attachment_t *a, vec3_t v );
void     CG_SetAttachmentCent( attachment_t *a, centity_t *cent );
void     CG_SetAttachmentTag( attachment_t *a, refEntity_t *parent,
                              qhandle_t model, const char *tagName );
void     CG_SetAttachmentParticle( attachment_t *a, particle_t *p );

void     CG_SetAttachmentOffset( attachment_t *a, vec3_t v );

//
// cg_particles.c
//
void             CG_LoadParticleSystems();
qhandle_t        CG_RegisterParticleSystem( const char *name );

particleSystem_t *CG_SpawnNewParticleSystem( qhandle_t psHandle );
void             CG_DestroyParticleSystem( particleSystem_t **ps );

bool         CG_IsParticleSystemInfinite( particleSystem_t *ps );
bool         CG_IsParticleSystemValid( particleSystem_t **ps );

void             CG_SetParticleSystemNormal( particleSystem_t *ps, vec3_t normal );
void             CG_SetParticleSystemLastNormal( particleSystem_t *ps, const vec3_t normal );

void             CG_AddParticles();

void             CG_ParticleSystemEntity( centity_t *cent );

void             CG_TestPS_f();
void             CG_DestroyTestPS_f();

//
// cg_trails.c
//
void          CG_LoadTrailSystems();
qhandle_t     CG_RegisterTrailSystem( const char *name );

trailSystem_t *CG_SpawnNewTrailSystem( qhandle_t psHandle );
void          CG_DestroyTrailSystem( trailSystem_t **ts );

bool      CG_IsTrailSystemValid( trailSystem_t **ts );

void          CG_AddTrails();

void          CG_TestTS_f();
void          CG_DestroyTestTS_f();

//
// cg_tutorial.c
//
const char *CG_TutorialText();

//
// cg_beacon.c
//

void          CG_LoadBeaconsConfig();
void          CG_RunBeacons();
qhandle_t     CG_BeaconIcon( const cbeacon_t *b );
qhandle_t     CG_BeaconDescriptiveIcon( const cbeacon_t *b );
const char    *CG_BeaconName( const cbeacon_t *b, char *out, size_t len );

//
//===============================================

// cg_drawCrosshair* settings
enum
{
  CROSSHAIR_ALWAYSOFF,
  CROSSHAIR_RANGEDONLY,
  CROSSHAIR_ALWAYSON
};

//
// cg_utils.c
//
bool   CG_ParseColor( byte *c, const char **text_p );
const char *CG_GetShaderNameFromHandle( const qhandle_t shader );
void       CG_ReadableSize( char *buf, int bufsize, int value );
void       CG_PrintTime( char *buf, int bufsize, int time );
void CG_FormatSI( char *buf, int size, float num, int sf, const char *unit );
void CG_SetKeyCatcher( int catcher );

//
// cg_rocket.c
//

void CG_Rocket_Init( glconfig_t gl );
void CG_Rocket_LoadHuds();
void CG_Rocket_Frame( cgClientState_t state );
const char *CG_Rocket_GetTag();
const char *CG_Rocket_GetAttribute( const char *attribute );
int CG_StringToNetSource( const char *src );
const char *CG_NetSourceToString( int netSrc );
const char *CG_Rocket_QuakeToRML( const char *in );
bool CG_Rocket_IsCommandAllowed( rocketElementType_t type );

//
// cg_rocket_events.c
//
void CG_Rocket_ProcessEvents();

//
// cg_rocket_dataformatter.c
//
void CG_Rocket_FormatData( int handle );
void CG_Rocket_RegisterDataFormatters();

//
// cg_rocket_draw.c
//
void CG_Rocket_RenderElement( const char* tag );
void CG_Rocket_RegisterElements( void );

//
// cg_rocket_datasource.c
//
void CG_Rocket_BuildDataSource( const char *dataSrc, const char *table );
void CG_Rocket_SortDataSource( const char *dataSource, const char *name, const char *sortBy );
void CG_Rocket_CleanUpServerList( const char *table );
void CG_Rocket_RegisterDataSources();
void CG_Rocket_CleanUpDataSources();
void CG_Rocket_ExecDataSource( const char *dataSource, const char *table );
void CG_Rocket_SetDataSourceIndex( const char *dataSource, const char *table, int index );
int CG_Rocket_GetDataSourceIndex( const char *dataSource, const char *table );
void CG_Rocket_FilterDataSource( const char *dataSource, const char *table, const char *filter );
void CG_Rocket_BuildServerInfo();
void CG_Rocket_BuildServerList( const char *args );
void CG_Rocket_BuildArmourySellList( const char *table );
void CG_Rocket_BuildArmouryBuyList( const char *table );
void CG_Rocket_BuildPlayerList( const char *table );

//
// cg_rocket_progressbar.c
//
float CG_Rocket_ProgressBarValue( Str::StringRef name );

//
// cg_gameinfo.c
//
void CG_LoadArenas();

//
// Rocket Functions
//
void Rocket_Init();
void Rocket_Shutdown();
void Rocket_Render();
void Rocket_Update();
void Rocket_LoadDocument( const char *path );
void Rocket_LoadCursor( const char *path );
void Rocket_DocumentAction( const char *name, const char *action );
bool Rocket_GetEvent(std::string& cmdText);
void Rocket_DeleteEvent();
void Rocket_RegisterDataSource( const char *name );
void Rocket_DSAddRow( const char *name, const char *table, const char *data );
void Rocket_DSChangeRow( const char *name, const char *table, const int row, const char *data );
void Rocket_DSRemoveRow( const char *name, const char *table, const int row );
void Rocket_DSClearTable( const char *name, const char *table );
void Rocket_SetInnerRMLById( const char* name, const char* id, const char* RML, int parseFlags );
void Rocket_SetInnerRML( const char* RML, int parseFlags );
void Rocket_QuakeToRMLBuffer( const char *in, char *out, int length );
void Rocket_GetEventParameters( char *params, int length );
void Rocket_RegisterDataFormatter( const char *name );
void Rocket_DataFormatterRawData( int handle, char *name, int nameLength, char *data, int dataLength );
void Rocket_DataFormatterFormattedData( int handle, const char *data, bool parseQuake );
void Rocket_GetElementTag( char *tag, int length );
void Rocket_SetElementDimensions( float x, float y );
void Rocket_RegisterElement( const char *tag );
void Rocket_SetAttribute( const char *name, const char *id, const char *attribute, const char *value );
void Rocket_GetAttribute( const char *name, const char *id, const char *attribute, char *out, int length );
void Rocket_GetProperty( const char *name, void *out, int len, rocketVarType_t type );
void Rocket_GetElementAbsoluteOffset( float *x, float *y );
void Rocket_GetElementDimensions( float *w, float *h );
void Rocket_SetClass( const char *in, bool activate );
void Rocket_SetPropertyById( const char *id, const char *property, const char *value );
void Rocket_SetActiveContext( int catcher );
void Rocket_AddConsoleText(Str::StringRef text);
void Rocket_InitializeHuds( int size );
void Rocket_LoadUnit( const char *path );
void Rocket_AddUnitToHud( int weapon, const char *id );
void Rocket_ShowHud( int weapon );
void Rocket_ClearHud( unsigned weapon );
void Rocket_InitKeys();
keyNum_t Rocket_ToQuake( int key );
void Rocket_ProcessKeyInput( int key, bool down );
void Rocket_ProcessTextInput( int key );
void Rocket_MouseMove( int x, int y );
void Rocket_RegisterProperty( const char *name, const char *defaultValue, bool inherited, bool force_layout, const char *parseAs );
void Rocket_ShowScoreboard( const char *name, bool show );
void Rocket_SetDataSelectIndex( int index );
void Rocket_LoadFont( const char *font );
void Rocket_Rocket_f( void );
void Rocket_Lua_f( void );
void Rocket_RocketDebug_f();
#endif

