/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 1999-2005 Id Software, Inc.
Copyright (C) 2000-2009 Darklegion Development

This file is part of the Unvanquished GPL Source Code (Unvanquished Source Code).

Unvanquished is free software; you can redistribute it
and/or modify it under the terms of the GNU General Public License as
published by the Free Software Foundation; either version 2 of the License,
or (at your option) any later version.

Unvanquished is distributed in the hope that it will be
useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Unvanquished; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

===========================================================================
*/

#ifndef CG_LOCAL_H
#define CG_LOCAL_H

#include <memory>
#include <vector>

#include "common/Common.h"
#include "common/KeyIdentification.h"
#include "engine/qcommon/q_shared.h"
#include "engine/renderer/tr_types.h"
#include "engine/client/cg_api.h"
#include "shared/bg_public.h"
#include "engine/client/keycodes.h"
#include "cg_ui.h"
#include "cg_skeleton_modifier.h"

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

#define MAX_STEP_CHANGE                32.0f

#define MAX_VERTS_ON_POLY              10
#define MAX_MARK_POLYS                 256

#define STAT_MINUS                     10 // num frame for '-' stats digit

#define CGAME_CHAR_WIDTH               32
#define CGAME_CHAR_HEIGHT              48

#define MAX_MINIMAP_ZONES              32

enum footstep_t
{
  FOOTSTEP_GENERAL,
  FOOTSTEP_FLESH,
  FOOTSTEP_METAL,
  FOOTSTEP_SPLASH,
  FOOTSTEP_CUSTOM,
  FOOTSTEP_NONE,

  FOOTSTEP_TOTAL
};

enum impactSound_t
{
  IMPACTSOUND_DEFAULT,
  IMPACTSOUND_METAL,
  IMPACTSOUND_FLESH
};

enum jetPackState_t
{
  JPS_INACTIVE,
  JPS_ACTIVE
};

enum jetpackAnimNumber_t
{
  JANIM_NONE,

  JANIM_SLIDEOUT,
  JANIM_SLIDEIN,

  MAX_JETPACK_ANIMATIONS
};

//======================================================================

// when changing animation, set animationTime to frameTime + lerping time
// The current lerp will finish out, then it will lerp to the new animation
struct lerpFrame_t
{
	int           oldFrame;
	int           oldFrameTime; // time when ->oldFrame was exactly on

	int           frame;
	int           frameTime; // time when ->frame will be exactly on

	bool          animationEnded;

	float         backlerp;

	float         yawAngle;
	bool      yawing;
	float         pitchAngle;
	bool      pitching;

	int           animationNumber; // may include ANIM_TOGGLEBIT
	animation_t   *animation;
	int           animationTime; // time when the first frame of the animation will be exact

	// added for smooth blending between animations on change

	int         old_animationNumber; // may include ANIM_TOGGLEBIT
	animation_t *old_animation;

	float       blendlerp;
	float       blendtime;
};

// debugging values:

extern int   debug_anim_current;
extern int   debug_anim_old;
extern float debug_anim_blend;

//======================================================================

//attachment system
enum attachmentType_t
{
  AT_STATIC,
  AT_TAG,
  AT_CENT,
  AT_PARTICLE
};

//forward declaration for particle_t
struct particle_t;

struct attachment_t
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
	particle_t *particle;
};

//======================================================================

// Maximum .particle files to load.
#define MAX_PARTICLE_FILES        128

#define MAX_PS_SHADER_FRAMES      32
#define MAX_PS_MODELS             8

// Maximum ejector definitions in a particle system definition.
#define MAX_EJECTORS_PER_SYSTEM   4
// Maximum particle definitions in an ejector definition.
#define MAX_PARTICLES_PER_EJECTOR 4

/* Maximum particle systems, ejectors and particle definitions to load from .particle files.

One particle file may contain more than one particle system definitions. Making particle system
definition maximum greater than particle file maximum makes sure it's not obviously low. */
#define MAX_BASEPARTICLE_SYSTEMS  (MAX_PARTICLE_FILES + 564)
#define MAX_BASEPARTICLE_EJECTORS (MAX_BASEPARTICLE_SYSTEMS * MAX_EJECTORS_PER_SYSTEM)
#define MAX_BASEPARTICLES         (MAX_BASEPARTICLE_EJECTORS * MAX_PARTICLES_PER_EJECTOR)

/* Maximum particle systems, ejectors and particle objects to process and render.

Not all particle system, ejector or particle definitions will instantiate particle
systems, ejectors or particles at a given time.

A single particle system definition can instantiate many particle systems
and then many ejectors, and a single ejector with a single particle definition
can instantiate many particles.

The chosen numeric values for those maximums are arbitrary, but making them greater
than or equal to the maximum of their definitions makes sure they're not obviously low. */
#define MAX_PARTICLE_SYSTEMS      MAX_BASEPARTICLE_SYSTEMS
#define MAX_PARTICLE_EJECTORS     (MAX_PARTICLE_SYSTEMS * MAX_EJECTORS_PER_SYSTEM)
/* FIXME: Something in the game is preventing to render more than a thousand particle textures.
#define MAX_PARTICLES             (MAX_PARTICLE_EJECTORS * MAX_PARTICLES_PER_EJECTOR) */
#define MAX_PARTICLES             1024

#define PARTICLES_INFINITE        -1
#define PARTICLES_SAME_AS_INITIAL -2

//COMPILE TIME STRUCTURES
enum pMoveType_t
{
  PMT_STATIC,
  PMT_STATIC_TRANSFORM,
  PMT_TAG,
  PMT_CENT_ANGLES,
  PMT_NORMAL,
  PMT_LAST_NORMAL,
  PMT_OPPORTUNISTIC_NORMAL
};

enum pDirType_t
{
  PMD_LINEAR,
  PMD_POINT
};

struct pMoveValues_t
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
};

struct pLerpValues_t
{
	int   delay;
	float delayRandFrac;

	float initial;
	float initialRandFrac;

	float final;
	float finalRandFrac;

	float randFrac;
};

//particle template
struct baseParticle_t
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
};

//ejector template
struct baseParticleEjector_t
{
	baseParticle_t *particles[ MAX_PARTICLES_PER_EJECTOR ];
	int            numParticles;

	pLerpValues_t  eject; //zero period indicates creation of all particles at once

	int            totalParticles; //can be infinite
	float          totalParticlesRandFrac;
};

//particle system template
struct baseParticleSystem_t
{
	char                  name[ MAX_QPATH ];
	baseParticleEjector_t *ejectors[ MAX_EJECTORS_PER_SYSTEM ];
	int                   numEjectors;

	bool              thirdPersonOnly;
	bool              registered; //whether or not the assets for this particle have been loaded
};

//RUN TIME STRUCTURES
struct particleSystem_t
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
};

struct particleEjector_t
{
	baseParticleEjector_t *class_;
	particleSystem_t *parent;

	pLerpValues_t    ejectPeriod;

	int              count;
	int              totalParticles;

	int              nextEjectionTime;

	bool         valid;
};

//used for actual particle evaluation
struct particle_t
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
};

//======================================================================

//trail system stuff
#define MAX_TRAIL_FILES        128

#define MAX_BEAMS_PER_SYSTEM   4

#define MAX_BASETRAIL_SYSTEMS  64
#define MAX_BASETRAIL_BEAMS    (MAX_BASETRAIL_SYSTEMS * MAX_BEAMS_PER_SYSTEM)

#define MAX_TRAIL_SYSTEMS      50
#define MAX_TRAIL_BEAMS        (MAX_TRAIL_SYSTEMS * MAX_BEAMS_PER_SYSTEM)
#define MAX_TRAIL_BEAM_NODES   128

#define MAX_TRAIL_BEAM_JITTERS 4

enum trailBeamTextureType_t
{
  TBTT_STRETCH,
  TBTT_REPEAT
};

struct baseTrailJitter_t
{
	float magnitude;
	int   period;
};

//beam template
struct baseTrailBeam_t
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

	bool          dynamicLight;
	float         dLightRadius;
	byte          dLightColor[ 3 ];

	int               numJitters;
	baseTrailJitter_t jitters[ MAX_TRAIL_BEAM_JITTERS ];
	bool          jitterAttachments;
};

//trail system template
struct baseTrailSystem_t
{
	char            name[ MAX_QPATH ];
	baseTrailBeam_t *beams[ MAX_BEAMS_PER_SYSTEM ];
	int             numBeams;

	int             lifeTime;
	bool        thirdPersonOnly;
	bool        registered; //whether or not the assets for this trail have been loaded
};

struct trailSystem_t
{
	baseTrailSystem_t   *class_;

	attachment_t frontAttachment;
	attachment_t backAttachment;

	int          birthTime;
	int          destroyTime;
	bool     valid;
};

struct trailBeamNode_t
{
	vec3_t                 refPosition;
	vec3_t                 position;

	int                    timeLeft;

	float                  textureCoord;
	float                  halfWidth;
	byte                   alpha;
	byte                   color[ 3 ];

	vec2_t                 jitters[ MAX_TRAIL_BEAM_JITTERS ];

	trailBeamNode_t *prev;

	trailBeamNode_t *next;

	bool               used;
};

struct trailBeam_t
{
	baseTrailBeam_t   *class_;
	trailSystem_t   *parent;

	trailBeamNode_t nodePool[ MAX_TRAIL_BEAM_NODES ];
	trailBeamNode_t *nodes;

	int             lastEvalTime;

	bool        valid;

	int             nextJitterTimes[ MAX_TRAIL_BEAM_JITTERS ];
};

//======================================================================

// player entities need to track more information
// than any other type of entity.

// smoothing of view and model for WW transitions
#define   MAXSMOOTHS 32

struct smooth_t
{
	float  time;
	float  timeMod;

	vec3_t rotAxis;
	float  rotAngle;
};

struct playerEntity_t
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
};

struct lightFlareStatus_t
{
	float    lastRadius; //caching of likely flare radius
	float    lastRatio; //caching of likely flare ratio
	int      lastTime; //last time flare was visible/occluded
	bool status; //flare is visible?
	qhandle_t hTest;
};

struct buildableStatus_t
{
	int      lastTime; // Last time status was visible
	bool visible; // Status is visible?
};

struct buildableCache_t
{
	vec3_t cachedOrigin; // If any of the values differ from their
	vec3_t cachedAngles; //  cached versions, then the cache is invalid
	vec3_t cachedNormal;
	buildable_t cachedType;
	vec3_t axis[ 3 ];
	vec3_t origin;
};

//=================================================

#define MAX_CBEACONS 1024

// the point of keeping the beacon data separately from centities is
// to be able to handle virtual beacons (client-side beacons) without
// having to add fake centities

// static data belonging to a specific beacon
// here's all beacon info that can't be deduced at will (e.g. because
// it depends on past states)
struct cbeacon_t
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
	Color::Color  color;
	float         t_occlusion;

	bool      clamped;
	vec2_t        clamp_dir;
};

struct beaconsConfig_t
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
	Color::Color  colorNeutral;
	Color::Color  colorAlien;
	Color::Color  colorHuman;
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
};

// strings to display on the libRocket HUD
struct beaconRocket_t
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
};

//======================================================================

// centity_t has a direct correspondence with gentity_t in the game, but
// only the entityState_t is directly communicated to the cgame
struct centity_t
{
	entityState_t  currentState; // from cg.frame
	entityState_t  nextState; // from cg.nextFrame, if available
	bool       interpolate; // true if next is valid to interpolate to
	bool       currentValid; // true if cg.frame holds this entity
	bool valid;

	int            muzzleFlashTime; // move to playerEntity?
	int            previousEvent;

	int            trailTime; // so missile trails can handle dropped initial packets
	int            miscTime;
	int            snapShotTime; // last time this entity was found in a snapshot

	playerEntity_t pe;

	bool       extrapolated; // false if origin / angles is an interpolation
	vec3_t         rawOrigin;
	vec3_t         rawAngles;

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

	trailSystem_t         *muzzleTS;
	int                   muzzleTSDeathTime;

	particleSystem_t      *missilePS;
	trailSystem_t         *missileTS;

	float                 radarVisibility;

	int                   pvsEnterTime;

	cbeacon_t             beacon;

	// Content flags derived from the entity state
	// HACK: This is not an exact copy of the content flags the server sees.
	// If this is desired, it needs to be made part of entityState_t instead.
	int                   contents;
};

//======================================================================

struct markPoly_t
{
	markPoly_t *prevMark, *nextMark;

	int               time;
	qhandle_t         markShader;
	bool          alphaFade; // fade alpha instead of rgb
	float             color[ 4 ];
	poly_t            poly;
	polyVert_t        verts[ MAX_VERTS_ON_POLY ];
};

//======================================================================

struct score_t
{
	int       client;
	int       score;
	int       ping;
	int       time;
	int       team;
	weapon_t  weapon;
	upgrade_t upgrade;
};

// each client has an associated clientInfo_t
// that contains media references necessary to present the
// client model and other color coded effects
// this is regenerated each time a client's configstring changes,
// usually as a result of a userinfo (name, model, etc) change
#define MAX_CUSTOM_SOUNDS 32
struct clientInfo_t
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

	bool    iqm; // true if model is an iqm model
	bool    fixedlegs; // true if legs yaw is always the same as torso yaw
	bool    fixedtorso; // true if torso never changes yaw
	bool    nonsegmented; // true if model is Q2 style nonsegmented
	bool    skeletal; // true if model is a skeletal model

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

	sfxHandle_t customFootsteps[ 4 ];
	sfxHandle_t customMetalFootsteps[ 4 ];

	char        voice[ MAX_VOICE_NAME_LEN ];
	int         voiceTime;
	std::vector<std::shared_ptr<SkeletonModifier>> modifiers;

};

struct weaponInfoMode_t
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
};

// each WP_* weapon enum has an associated weaponInfo_t
// that contains media references necessary to present the
// weapon and its effects
struct weaponInfo_t
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
	float            crossHairSizeNoBorder; // For crosshair outlines

	sfxHandle_t      readySound;

	bool         disableIn3rdPerson;
	vec3_t           rotation;
	vec3_t           posOffs;
	char             rotationBone[ 50 ];
	vec_t            scale;

	weaponInfoMode_t wim[ WPM_NUM_WEAPONMODES ];
};

struct upgradeInfo_t
{
	bool  registered;
	const char *humanName;

	qhandle_t upgradeIcon;
};

struct classInfo_t
{
	qhandle_t classIcon;
};

struct sound_t
{
	bool    looped;
	bool    enabled;

	sfxHandle_t sound;
};

struct buildableInfo_t
{
	qhandle_t   models[ MAX_BUILDABLE_MODELS ];
	animation_t animations[ MAX_BUILDABLE_ANIMATIONS ];

	//same number of sounds as animations
	sound_t  sounds[ MAX_BUILDABLE_ANIMATIONS ];

	qhandle_t buildableIcon;

	bool md5;
};

//======================================================================

struct minimapZone_t
{
    vec3_t    boundsMin, boundsMax;
    vec2_t    imageMin, imageMax;
    float     scale;
    qhandle_t image;
};

struct minimap_t
{
    bool     active;
    bool     defined;
    int          lastZone;
    int          nZones;
    Color::Color bgColor;
    float        scale;
    struct
    {
        qhandle_t playerArrow, teamArrow;
    } gfx;
    minimapZone_t zones[ MAX_MINIMAP_ZONES ];
};

//======================================================================

// all cg.stepTime, cg.duckTime, cg.landTime, etc are set to cg.time when the action
// occurs, and they will have visible effects for #define STEP_TIME or whatever msec after

#define MAX_PREDICTED_EVENTS     16

#define NUM_SAVED_STATES         ( CMD_BACKUP + 2 )

// After this many msec the crosshair name fades out completely
#define CROSSHAIR_CLIENT_TIMEOUT 1000

struct cgBinaryShaderSetting_t
{
	Color::Color32Bit color;
	bool drawIntersection;
	bool drawFrontline;
};

enum sayType_t
{
	SAY_TYPE_NONE,
	SAY_TYPE_PUBLIC,
	SAY_TYPE_TEAM,
	SAY_TYPE_ADMIN,
	SAY_TYPE_COMMAND,
};

#include "Filter.h"

struct WeaponOffsets
{
	vec3_t bob;

	vec3_t angles;
	vec3_t angvel;

	WeaponOffsets operator+=( WeaponOffsets );
	WeaponOffsets operator*( float );
};

#define NUM_BINARY_SHADERS 256

struct cg_t
{
	int      clientFrame; // incremented each frame

	int      clientNum;

	bool demoPlayback;
	bool loading; // don't defer players at initial startup
	bool intermissionStarted;

	int currentCmdNumber; // usercmd_t number for this frame

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
	int      teamScores[ 2 ];
	int teamPlayerCount[ NUM_TEAMS ];
	score_t  scores[ MAX_CLIENTS ];
	bool showScores;
	char     killerName[ MAX_NAME_LENGTH ];
	char     spectatorList[ MAX_STRING_CHARS ]; // list of names
	int      spectatorTime; // next time to offset
	bool scoreInvalidated; // needs update on next RocketUpdate

	// centerprinting
	int  centerPrintTime;
	float centerPrintSizeFactor;
	char centerPrint[ MAX_STRING_CHARS ];

	// crosshair client ID
	int crosshairEntityNum;
	int crosshairEntityTime;
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
	int                     weapon1Time; // time when BTN_ATTACK went t->f f->t
	int                     weapon2Time; // time when BTN_ATTACK2 went t->f f->t
	int                     weapon3Time; // time when BTN_ATTACK3 went t->f f->t
	bool                weapon1Firing;
	bool                weapon2Firing;
	bool                weapon3Firing;

	vec3_t                  lastNormal; // view smoothage
	vec3_t                  lastVangles; // view smoothage
	smooth_t                sList[ MAXSMOOTHS ]; // WW smoothing

	int                     forwardMoveTime; // for struggling
	int                     rightMoveTime;
	int                     upMoveTime;

	/* loading */
	std::string mapLongName;
	std::string mapAuthors;
	std::string loadingText;
	float                   loadingFraction; // loading percentages
	float                   mediaLoadingFraction;
	float                   buildableLoadingFraction;
	float                   characterLoadingFraction;
	float navmeshLoadingFraction;
	bool loadingNavmesh;

	int                     lastBuildAttempt;
	int                     lastEvolveAttempt;

	float                   painBlendValue;
	float                   painBlendTarget;
	int                     lastHealth;
	bool                wasDeadLastFrame;

	int                     lastPredictedCommand;
	int                     lastServerTime;
	playerState_t           savedPmoveStates[ NUM_SAVED_STATES ];
	int                     stateHead, stateTail;
	int                     ping;

	qhandle_t               lastHealthCross;
	float                   healthCrossFade;
	int                     nearUsableBuildable;

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

	// pmove params
	struct {
		int synchronous;
		int fixed;
		int msec;
		int accurate;
	} pmoveParams;

	Filter<WeaponOffsets>   weaponOffsetsFilter;
};

enum rocketElementType_t
{
	ELEMENT_ALL,
	ELEMENT_GAME,
	ELEMENT_LOADING,
	ELEMENT_HUMANS,
	ELEMENT_ALIENS,
	ELEMENT_BOTH,
};

enum class rocketVarType_t {
	ROCKET_STRING,
	ROCKET_FLOAT,
	ROCKET_INT,
	ROCKET_COLOR
};

enum rocketMenuType_t {
	ROCKETMENU_MAIN,
	ROCKETMENU_CONNECTING,
	ROCKETMENU_LOADING,
	ROCKETMENU_DOWNLOADING,
	ROCKETMENU_INGAME_MENU,
	ROCKETMENU_TEAMSELECT,
	ROCKETMENU_HUMANSPAWN,
	ROCKETMENU_ALIENSPAWN,
	ROCKETMENU_ALIENBUILD,
	ROCKETMENU_HUMANBUILD,
	ROCKETMENU_ARMOURYBUY,
	ROCKETMENU_ALIENEVOLVE,
	ROCKETMENU_CHAT,
	ROCKETMENU_BEACONS,
	ROCKETMENU_ERROR,
	ROCKETMENU_WELCOME,
	ROCKETMENU_NUM_TYPES
};

enum rocketInnerRMLParseTypes_t {
	RP_QUAKE = 1 << 0,
	RP_EMOTICONS = 1 << 1,
};

struct rocketMenu_t
{
	const char *path;
	const char *id;
};

#define MAX_SERVERS 2048
#define MAX_RESOLUTIONS 32
#define MAX_LANGUAGES 64
#define MAX_OUTPUTS 16
#define MAX_MODS 64
#define MAX_DEMOS 256

struct server_t
{
	char *name;
	char *label;
	int clients;
	int bots;
	int ping;
	int maxClients;
	char *mapName;
	char *addr;
};



struct resolution_t
{
	int width;
	int height;
};



struct language_t
{
	char *name;
	char *lang;
};



struct modInfo_t
{
	char *name;
	char *description;
};

struct arenaInfo_t
{
	std::string name;
	std::string longName;
	std::vector<std::string> authors;
};

struct mapInfo_t
{
	std::string mapLoadName;
};

struct rocketDataSource_t
{
	server_t servers[ AS_NUM_TYPES ][ MAX_SERVERS ];
	int serverCount[ AS_NUM_TYPES ];
	std::vector<bool> haveServerInfo[ AS_NUM_TYPES ];
	int serverIndex[ AS_NUM_TYPES ];
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

	char *alOutputs[ MAX_OUTPUTS ];
	int alOutputsCount;
	int alOutputIndex;

	int playerList[ NUM_TEAMS ][ MAX_CLIENTS ];
	int playerCount[ NUM_TEAMS ];
	int playerIndex[ NUM_TEAMS ];

	char *demoList[ MAX_DEMOS ];
	int demoCount;
	int demoIndex;

	std::vector<mapInfo_t> mapList;
	int mapIndex;

	int selectedTeamIndex;

	int selectedSpawnOptions[ NUM_TEAMS ];

	int armouryBuyList[2][ ( WP_LUCIFER_CANNON - WP_BLASTER ) + UP_NUM_UPGRADES + 1 ];
	int selectedArmouryBuyItem[2];
	int armouryBuyListCount[2];
};

struct rocketInfo_t
{
	int currentNetSrc;
	int serversLastRefresh;
	int serverStatusLastRefresh;
	int realtime;
	int keyCatcher;
	char downloadName[ MAX_STRING_CHARS ];
	cgClientState_t cstate;
	rocketMenu_t menu[ Util::ordinal(rocketMenuType_t::ROCKETMENU_NUM_TYPES) ];
	rocketMenu_t hud[ WP_NUM_WEAPONS ];
	rocketDataSource_t data;
	bool renderCursor;
	qhandle_t cursor;
	int cursorFreezeTime = -1;
	int cursorFreezeX, cursorFreezeY;
	rectDef_t cursor_pos;
};

extern rocketInfo_t rocketInfo;

// all of the model, shader, and sound references that are
// loaded at gamestate time are stored in cgMedia_t
// Other media that can be tied to clients, weapons, or items are
// stored in the clientInfo_t, itemInfo_t, and weaponInfo_t

struct cgMediaBinaryShader_t
{
	qhandle_t f1;
	qhandle_t f2;
	qhandle_t f3;
	qhandle_t b1;
	qhandle_t b2;
	qhandle_t b3;
};

struct cgMedia_t
{
	qhandle_t whiteShader;
	qhandle_t outlineShader;

	qhandle_t level2ZapTS;

	qhandle_t balloonShader;
	qhandle_t connectionShader;

	qhandle_t viewBloodShader;
	qhandle_t tracerShader;
	qhandle_t backTileShader;

	qhandle_t creepShader;

	qhandle_t scannerBlipShader;
	qhandle_t scannerBlipBldgShader;
	qhandle_t scannerLineShader;

	qhandle_t numberShaders[ 11 ];

	qhandle_t shadowMarkShader;
	qhandle_t wakeMarkShader;

	// buildable shaders
	qhandle_t             greenBuildShader;
	qhandle_t             yellowBuildShader;
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
	sfxHandle_t turretSpinupSound;

	sfxHandle_t grenadeBounceSound0;
	sfxHandle_t grenadeBounceSound1;

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

	sfxHandle_t alienBuildableDying1;
	sfxHandle_t alienBuildableDying2;
	sfxHandle_t alienBuildableExplosion;
	sfxHandle_t alienBuildablePrebuild;
	sfxHandle_t humanBuildableDyingLarge;
	sfxHandle_t humanBuildableDying;
	sfxHandle_t humanBuildableExplosion;
	sfxHandle_t humanBuildablePrebuild;
	sfxHandle_t humanBuildableDamage[ 4 ];

	sfxHandle_t alienL4ChargePrepare;
	sfxHandle_t alienL4ChargeStart;

	//light armour
	qhandle_t   larmourHeadSkin;
	qhandle_t   larmourLegsSkin;
	qhandle_t   larmourTorsoSkin;

	qhandle_t   jetpackModel;
	qhandle_t   radarModel;

	sfxHandle_t itemFillSound;

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

	qhandle_t   reactorZapTS;

	sfxHandle_t lCannonWarningSound;
	sfxHandle_t lCannonWarningSound2;

	sfxHandle_t rocketpodLockonSound;

	qhandle_t   buildWeaponTimerPie[ 8 ];
	qhandle_t   healthCross;
	qhandle_t   healthCross2X;
	qhandle_t   healthCross3X;
	qhandle_t   healthCross4X;
	qhandle_t   healthCrossMedkit;
	qhandle_t   healthCrossPoisoned;

	qhandle_t   neutralCgrade;
	qhandle_t   redCgrade;
	qhandle_t   tealCgrade;
	qhandle_t   desaturatedCgrade;

	qhandle_t sniperScopeShader;
	qhandle_t lgunScopeShader;

	animation_t jetpackAnims[ MAX_JETPACK_ANIMATIONS ];

	qhandle_t   beaconIconArrow;
	qhandle_t   beaconNoTarget;
	qhandle_t   beaconTagScore;

	sfxHandle_t timerBeaconExpiredSound;

	qhandle_t   damageIndicatorFont;
	sfxHandle_t killSound;
};

struct buildStat_t
{
	qhandle_t frameShader;
	qhandle_t overlayShader;
	qhandle_t noPowerShader;
	qhandle_t markedShader;
	Color::Color healthSevereColor;
	Color::Color healthHighColor;
	Color::Color healthElevatedColor;
	Color::Color healthGuardedColor;
	Color::Color healthLowColor;
	int          frameHeight;
	int          frameWidth;
	int          healthPadding;
	int          overlayHeight;
	int          overlayWidth;
	float        verticalMargin;
	float        horizontalMargin;
	Color::Color foreColor;
	Color::Color backColor;
	bool  loaded;
};

// More global variables, like cg_t. This used to say, "It will NOT be cleared when a tournament
// restart is done, allowing all clients to begin playing instantly", but cg_t is never cleared
// either so there is no difference.
struct cgs_t
{
	GameStateCSs gameState; // gamestate from server
	glconfig_t  glconfig; // rendering configuration
	float       screenXScale; // derived from glconfig
	float       screenYScale;
	float       aspectScale;

	int         processedSnapshotNum; // the number of snapshots cgame has requested

	// parsed from serverinfo
	int      timelimit;
	int      maxclients;
	char     mapname[ MAX_QPATH ];

	float    devolveMaxBaseDistance; // used for evolve/devolve ui

	float    momentumHalfLife; // used for momentum bar (un)lock markers
	float    unlockableMinTime;  // used for momentum bar (un)lock markers

	float    buildPointBudgetPerMiner;
	float    buildPointRecoveryInitialRate;
	float    buildPointRecoveryRateHalfLife;

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
	clientInfo_t corpseinfo[ PCL_NUM_CLASSES ];

	buildStat_t  alienBuildStat;
	buildStat_t  humanBuildStat;

	// media
	cgMedia_t    media;

	voice_t      *voices;
	clientList_t ignoreList;

	beaconsConfig_t  bc;
};

struct consoleCommand_t
{
	const char *cmd;
	void     ( *function )();
};

enum shaderColorEnum_t
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
};

enum rangeMarker_t
{
	RM_SPHERE,
	RM_SPHERICAL_CONE_64,
	RM_SPHERICAL_CONE_240,
};

enum altShader_t
{
  CG_ALTSHADER_DEFAULT, // must be first
  CG_ALTSHADER_UNPOWERED,
  CG_ALTSHADER_DEAD,
  CG_ALTSHADER_IDLE,
  CG_ALTSHADER_IDLE2
};

//==============================================================================

extern  cgs_t               cgs;
extern  cg_t                cg;
extern  centity_t           cg_entities[ MAX_GENTITIES ];
extern int cg_highestActiveEntity;

extern  weaponInfo_t        cg_weapons[ 32 ];
extern  upgradeInfo_t       cg_upgrades[ 32 ];
extern  classInfo_t         cg_classes[ PCL_NUM_CLASSES ];
extern  buildableInfo_t     cg_buildables[ BA_NUM_BUILDABLES ];

extern  const vec3_t        cg_shaderColors[ SHC_NUM_SHADER_COLORS ];

extern  markPoly_t          cg_markPolys[ MAX_MARK_POLYS ];

extern Cvar::Cvar<int> cg_teslaTrailTime;
extern Cvar::Cvar<float> cg_runpitch;
extern Cvar::Cvar<float> cg_runroll;
extern Cvar::Cvar<float> cg_swingSpeed;
extern shadowingMode_t cg_shadows;
extern Cvar::Cvar<bool> cg_playerShadows;
extern Cvar::Cvar<bool> cg_buildableShadows;
extern Cvar::Cvar<bool> cg_drawTimer;
extern Cvar::Range<Cvar::Cvar<int>> cg_drawClock;
extern Cvar::Cvar<bool> cg_drawFPS;
extern Cvar::Range<Cvar::Cvar<int>> cg_drawCrosshair;
extern Cvar::Cvar<bool> cg_drawCrosshairHit;
extern Cvar::Range<Cvar::Cvar<int>> cg_crosshairStyle;
extern Cvar::Cvar<float> cg_crosshairColorRed;
extern Cvar::Cvar<float> cg_crosshairColorGreen;
extern Cvar::Cvar<float> cg_crosshairColorBlue;
extern Cvar::Cvar<float> cg_crosshairColorAlpha;
extern Cvar::Range<Cvar::Cvar<int>> cg_crosshairOutlineStyle;
extern Cvar::Cvar<float> cg_crosshairOutlineScale;
extern Cvar::Cvar<float> cg_crosshairOutlineOffset;
extern Cvar::Cvar<float> cg_crosshairOutlineColorRed;
extern Cvar::Cvar<float> cg_crosshairOutlineColorGreen;
extern Cvar::Cvar<float> cg_crosshairOutlineColorBlue;
extern Cvar::Cvar<float> cg_crosshairOutlineColorAlpha;
extern Cvar::Range<Cvar::Cvar<int>> cg_drawCrosshairFriendFoe;
extern Cvar::Range<Cvar::Cvar<int>> cg_drawCrosshairNames;
extern Cvar::Cvar<bool> cg_drawBuildableHealth;
extern Cvar::Cvar<bool> cg_drawMinimap;
extern Cvar::Cvar<int> cg_minimapActive;
extern Cvar::Cvar<float> cg_crosshairSize;
extern Cvar::Range<Cvar::Cvar<int>> cg_teamOverlayUserinfo;
extern Cvar::Cvar<bool> cg_draw2D;
extern Cvar::Cvar<bool> cg_animSpeed;
extern Cvar::Cvar<bool> cg_debugAnim;
extern Cvar::Cvar<bool> cg_debugEvents;
extern Cvar::Cvar<float> cg_errorDecay;
extern Cvar::Cvar<bool> cg_nopredict;
extern Cvar::Cvar<int> cg_debugMove;
extern Cvar::Cvar<bool> cg_noPlayerAnims;
extern Log::Logger predictionLog;
extern Cvar::Cvar<bool> cg_footsteps;
extern Cvar::Cvar<bool> cg_addMarks;
extern Cvar::Cvar<int> cg_viewsize;
extern Cvar::Range<Cvar::Cvar<int>> cg_drawGun;
extern Cvar::Cvar<float> cg_gun_x;
extern Cvar::Cvar<float> cg_gun_y;
extern Cvar::Cvar<float> cg_gun_z;
extern Cvar::Cvar<bool> cg_mirrorgun;
extern Cvar::Range<Cvar::Cvar<float>> cg_tracerChance;
extern Cvar::Cvar<float> cg_tracerWidth;
extern Cvar::Cvar<float> cg_tracerLength;
extern Cvar::Cvar<bool> cg_thirdPerson;
extern Cvar::Cvar<float> cg_thirdPersonAngle;
extern Cvar::Range<Cvar::Cvar<int>> cg_thirdPersonShoulderViewMode;
extern Cvar::Cvar<bool> cg_staticDeathCam;
extern Cvar::Cvar<bool> cg_thirdPersonPitchFollow;
extern Cvar::Cvar<float> cg_thirdPersonRange;
extern Cvar::Cvar<bool> cg_lagometer;
extern Cvar::Range<Cvar::Cvar<int>> cg_drawSpeed;
extern Cvar::Cvar<int> cg_maxSpeedTimeWindow;
extern Cvar::Cvar<bool> cg_blood;
extern Cvar::Cvar<bool> cg_teamChatsOnly;
extern Cvar::Cvar<bool> cg_noVoiceChats;
extern Cvar::Cvar<bool> cg_noVoiceText;
extern Cvar::Cvar<bool> cg_smoothClients;
extern Cvar::Cvar<bool> cg_noTaunt;
extern Cvar::Cvar<bool> cg_drawSurfNormal;
extern Cvar::Range<Cvar::Cvar<int>> cg_drawBBOX;
extern Cvar::Cvar<bool> cg_drawEntityInfo;
extern Cvar::Cvar<int> cg_wwSmoothTime;
extern Cvar::Cvar<bool> cg_bounceParticles;
extern Cvar::Cvar<int> cg_consoleLatency;
extern Cvar::Range<Cvar::Cvar<int>> cg_lightFlare;
extern Cvar::Range<Cvar::Cvar<int>> cg_debugParticles;
extern Cvar::Cvar<bool> cg_debugPVS;
extern Cvar::Range<Cvar::Cvar<int>> cg_disableWarningDialogs;
extern Cvar::Cvar<bool> cg_tutorial;

extern Cvar::Cvar<bool> cg_rangeMarkerDrawSurface;
extern Cvar::Cvar<bool> cg_rangeMarkerDrawIntersection;
extern Cvar::Cvar<bool> cg_rangeMarkerDrawFrontline;
extern Cvar::Range<Cvar::Cvar<float>> cg_rangeMarkerSurfaceOpacity;
extern Cvar::Range<Cvar::Cvar<float>> cg_rangeMarkerLineOpacity;
extern Cvar::Cvar<float> cg_rangeMarkerLineThickness;
extern Cvar::Cvar<bool> cg_rangeMarkerForBlueprint;
extern Cvar::Modified<Cvar::Cvar<std::string>> cg_rangeMarkerBuildableTypes;
extern Cvar::Cvar<bool> cg_rangeMarkerWhenSpectating;
extern int cg_buildableRangeMarkerMask;
extern Cvar::Range<Cvar::Cvar<float>> cg_binaryShaderScreenScale;

extern Cvar::Cvar<float> cg_painBlendUpRate;
extern Cvar::Cvar<float> cg_painBlendDownRate;
extern Cvar::Cvar<float> cg_painBlendMax;
extern Cvar::Cvar<float> cg_painBlendScale;
extern Cvar::Cvar<float> cg_painBlendZoom;

extern Cvar::Range<Cvar::Cvar<int>> cg_stickySpec;
extern Cvar::Range<Cvar::Cvar<int>> cg_sprintToggle;
extern Cvar::Range<Cvar::Cvar<int>> cg_unlagged;

extern Cvar::Cvar<std::string> cg_cmdGrenadeThrown;

extern Cvar::Cvar<bool> cg_debugVoices;

extern Cvar::Cvar<bool> cg_optimizePrediction;
extern Cvar::Cvar<bool> cg_projectileNudge;

extern Cvar::Cvar<bool> cg_emoticonsInMessages;

extern Cvar::Cvar<bool> cg_chatTeamPrefix;

extern Cvar::Cvar<float> cg_animBlend;

extern Cvar::Cvar<float> cg_motionblur;
extern Cvar::Cvar<float> cg_motionblurMinSpeed;
extern Cvar::Cvar<bool> ui_chatPromptColors;
extern Cvar::Cvar<bool> cg_spawnEffects;
extern Cvar::Cvar<std::string> cg_sayCommand;

extern Cvar::Cvar<bool> cg_lazyLoadModels;

//
// cg_main.c
//
const char *CG_ConfigString( int index );
const char *CG_Argv( int arg );
const char *CG_Args();

void       CG_StartMusic();

void       CG_NotifyHooks();
void       CG_UpdateCvars();

int        CG_CrosshairPlayer();
void       CG_KeyEvent( Keyboard::Key key, bool down );
void       CG_MouseEvent( int dx, int dy );
void       CG_MousePosEvent( int x, int y );
void       CG_FocusEvent( bool focus );
bool   CG_ClientIsReady( int clientNum );
void       CG_BuildSpectatorString();

void       CG_UpdateBuildableRangeMarkerMask();
void       CG_RegisterGrading( int slot, const char *str );

void CG_Init( int serverMessageNum, int clientNum, const glconfig_t& gl, const GameStateCSs& gameState );
void CG_Shutdown();

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
void     CG_FillRect( float x, float y, float width, float height, const Color::Color& color );
void     CG_DrawPic( float x, float y, float width, float height, qhandle_t hShader );
void     CG_DrawRotatedPic( float x, float y, float width, float height, qhandle_t hShader, float angle );
void     CG_DrawNoStretchPic( float x, float y, float width, float height, qhandle_t hShader );
void     CG_DrawFadePic( float x, float y, float width, float height, const Color::Color& fcolor,
                         const Color::Color& tcolor, float amount, qhandle_t hShader );
void     CG_SetClipRegion( float x, float y, float w, float h );
void     CG_ClearClipRegion();
void     CG_EnableScissor( bool enable );
void     CG_SetScissor( int x, int y, int w, int h );

Color::Color CG_FadeColor( int startMsec, int totalMsec );
float    CG_FadeAlpha( int startMsec, int totalMsec );
void     CG_TileClear();
void     CG_DrawRect( float x, float y, float width, float height, float size, const Color::Color& color );
void     CG_DrawSides( float x, float y, float w, float h, float size );
void     CG_DrawTopBottom( float x, float y, float w, float h, float size );
bool CG_WorldToScreen( vec3_t point, float *x, float *y );
char     CG_GetColorCharForHealth( int clientnum );
void     CG_DrawSphere( const vec3_t center, float radius, int customShader, const Color::Color& shaderRGBA );
void     CG_DrawSphericalCone( const vec3_t tip, const vec3_t rotation, float radius,
                               bool a240, int customShader, const Color::Color& shaderRGBA );
void     CG_DrawRangeMarker( rangeMarker_t rmType, const vec3_t origin, float range, const vec3_t angles, Color::Color rgba );

#define CG_ExponentialFade( value, target, lambda ) \
		ExponentialFade( (value), (target), (lambda), (float)cg.frametime * 0.001f );

//
// cg_draw.c
//

void CG_AddLagometerFrameInfo();
void CG_AddLagometerSnapshotInfo( snapshot_t *snap );
void CG_AddSpeed();
void CG_CenterPrint( const char *str, float sizeFactor );
void CG_DrawActive();
void       CG_RunMenuScript( char **args );
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
void     CG_AlienBuildableDying( buildable_t buildable, vec3_t origin );
void     CG_AlienBuildableExplosion( vec3_t origin, vec3_t dir );
const centity_t *CG_LookupMainBuildable();
float    CG_DistanceToBase();

//
// cg_animation.c
//
int CG_AnimNumber( int anim );
void CG_RunLerpFrame( lerpFrame_t *lf );
void CG_RunMD5LerpFrame( lerpFrame_t *lf, bool animChanged );
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
void CG_PredictPlayerState();

//
// cg_events.c
//
void CG_CheckEvents( centity_t *cent );
void CG_EntityEvent( centity_t *cent, vec3_t position );
void CG_PainEvent( centity_t *cent, int health );
void CG_OnPlayerWeaponChange();
void CG_Rocket_UpdateArmouryMenu();
void CG_OnMapRestart();

//
// cg_ents.c
//
void CG_DrawBoundingBox( int type, vec3_t origin, vec3_t mins, vec3_t maxs );
void CG_SetEntitySoundPosition( centity_t *cent );
void CG_AddPacketEntities();
void CG_AdjustPositionForMover( const vec3_t in, int moverNum, int fromTime, int toTime, vec3_t out,
                                vec3_t angles_in, vec3_t angles_out );
void CG_PositionEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
                             qhandle_t parentModel, const char *tagName );
void CG_PositionRotatedEntityOnTag( refEntity_t *entity, const refEntity_t *parent,
                                    qhandle_t parentModel, const char *tagName );
void CG_TransformSkeleton( refSkeleton_t *skel, const vec_t scale );

team_t CG_Team(const entityState_t &es);
inline team_t CG_Team(const centity_t *ent) {
	return CG_Team(ent->currentState);
}

// The team of yourself or the followed player.
// To get the team you joined at the title screen, use
//         cgs.clientinfo[ cg.clientNum ].team
// But beware that this function is probably the one you want.
inline team_t CG_MyTeam() {
	return static_cast<team_t>(cg.predictedPlayerState.persistant[PERS_TEAM]);
}

inline bool CG_IsOnMyTeam(const centity_t *ent) {
	return CG_Team(ent) == CG_MyTeam();
}
inline bool CG_IsOnMyTeam(const entityState_t &es) {
	return CG_Team(es) == CG_MyTeam();
}

int CG_Health(const entityState_t &es);
inline int CG_Health(const centity_t *ent) {
	return CG_Health(ent->currentState);
}
inline bool CG_IsAlive(const entityState_t &es) {
	return CG_Health(es) > 0;
}
inline bool CG_IsAlive(const centity_t *ent) {
	return CG_Health(ent->currentState) > 0;
}

//
// cg_weapons.c
//
weapon_t CG_FindNextWeapon( playerState_t *ps );
void CG_NextWeapon_f();
void CG_PrevWeapon_f();
void CG_Weapon_f();
void CG_SelectNextInventoryItem_f();

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
float CG_ChargeProgress();

//
// cg_minimap.c
//
void CG_InitMinimap();
void CG_DrawMinimap( const rectDef_t *rect, const Color::Color& color );

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
void     CG_InitConsoleCommands();
void     CG_RequestScores();
void     CG_HideScores_f();
void     CG_ShowScores_f();

//
// cg_servercmds.c
//
void CG_ExecuteServerCommands( snapshot_t* snap );
void CG_SetMapNameFromServerinfo();
void CG_ParseServerinfo();
void CG_SetConfigValues();
void CG_ShaderStateChanged();
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
void          CG_DestroyTrailSystem( trailSystem_t *ts );

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
std::string   CG_BeaconName( const cbeacon_t *b );

//
//===============================================

// cg_drawCrosshair* settings
enum
{
  CROSSHAIR_ALWAYSOFF,
  CROSSHAIR_RANGEDONLY,
  CROSSHAIR_ALWAYSON
};

// cg_crosshairStyle* settings
enum {
	CROSSHAIR_STYLE_DEFAULT,
	CROSSHAIR_STYLE_CUSTOM
};

// cg_crosshairOutlineStyle* settings
enum {
	CROSSHAIR_OUTLINE_STYLE_NONE,
	CROSSHAIR_OUTLINE_STYLE_AUTO,
	CROSSHAIR_OUTLINE_STYLE_CUSTOM
};

//
// cg_utils.c
//
bool   CG_ParseColor( byte *c, const char **text_p );
const char *CG_GetShaderNameFromHandle( const qhandle_t shader );
void       CG_ReadableSize( char *buf, int bufsize, int value );
void       CG_PrintTime( char *buf, int bufsize, int time );
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
bool CG_Rocket_LoadCursor( Str::StringRef cursorPath );
void CG_Rocket_EnableCursor( bool enable );

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
void CG_Rocket_UpdateElement( const char* tag );
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
void CG_Rocket_BuildServerList();
void CG_Rocket_BuildPlayerList( const char *table );

//
// cg_rocket_progressbar.c
//
float CG_Rocket_ProgressBarValue( Str::StringRef name );

//
// cg_gameinfo.c
//
void CG_LoadArenas( Str::StringRef mapname );
arenaInfo_t CG_GetArenaInfo( Str::StringRef mapName );
void CG_LoadMapList();

//
// Rocket Functions
//
void Rocket_Init();
void Rocket_Shutdown();
void Rocket_Render();
void Rocket_Update();
void Rocket_LoadDocument( const char *path );
void Rocket_DocumentAction( const char *name, const char *action );
bool Rocket_GetEvent(std::string& cmdText);
void Rocket_DeleteEvent();
void Rocket_RegisterDataSource( const char *name );
void Rocket_DSAddRow( const char *name, const char *table, const char *data );
void Rocket_DSChangeRow( const char *name, const char *table, const int row, const char *data );
void Rocket_DSRemoveRow( const char *name, const char *table, const int row );
void Rocket_DSClearTable( const char *name, const char *table );
void Rocket_SetInnerRML( const char* text, int parseFlags );
void Rocket_SetInnerRMLRaw( const char* RML );
void Rocket_QuakeToRMLBuffer( const char *in, char *out, int length );
void Rocket_GetEventParameters( char *params, int length );
void Rocket_RegisterDataFormatter( const char *name );
void Rocket_DataFormatterRawData( int handle, char *name, int nameLength, char *data, int dataLength );
void Rocket_DataFormatterFormattedData( int handle, const char *data, bool parseQuake );
void Rocket_GetElementTag( char *tag, int length );
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
void Rocket_ProcessKeyInput( Keyboard::Key key, bool down );
void Rocket_ProcessTextInput( int c );
void Rocket_MouseMove( int x, int y );
void Rocket_RegisterProperty( const char *name, const char *defaultValue, bool inherited, bool force_layout, const char *parseAs );
void Rocket_ShowScoreboard( const char *name, bool show );
void Rocket_SetDataSelectIndex( int index );
void Rocket_LoadFont( const char *font );
void Rocket_Rocket_f( void );
void Rocket_Lua_f( void );
void Rocket_RocketDebug_f();
void Rocket_UpdateLanguage();

//
// CombatFeedback.cpp
//

namespace CombatFeedback
{
  void Event(entityState_t *es);
  void DrawDamageIndicators(void);
}

#endif
