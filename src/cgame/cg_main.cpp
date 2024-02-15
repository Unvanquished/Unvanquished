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

// cg_main.c -- initialization for cgame

#include "cg_local.h"
#include "cg_key_name.h"
#include "shared/parse.h"
#include "shared/navgen/navgen.h"
#include "shared/parse.h"

cg_t            cg;
cgs_t           cgs;
centity_t       cg_entities[ MAX_GENTITIES ];

weaponInfo_t    cg_weapons[ 32 ];
upgradeInfo_t   cg_upgrades[ 32 ];
classInfo_t     cg_classes[ PCL_NUM_CLASSES ];
buildableInfo_t cg_buildables[ BA_NUM_BUILDABLES ];

Cvar::Cvar<int> cg_teslaTrailTime("cg_teslaTrailTime", "time (ms) to show reactor zap", Cvar::NONE, 250);
Cvar::Cvar<float> cg_runpitch("cg_runpitch", "pitch angle change magnitude when running", Cvar::NONE, 0.002);
Cvar::Cvar<float> cg_runroll("cg_runroll", "roll angle magnitude change when running", Cvar::NONE, 0.005);
Cvar::Cvar<float> cg_swingSpeed("cg_swingSpeed", "something about view angles", Cvar::CHEAT, 0.3);
shadowingMode_t cg_shadows;
Cvar::Cvar<bool> cg_playerShadows("cg_playerShadows", "draw shadows of players", Cvar::NONE, true);
Cvar::Cvar<bool> cg_buildableShadows("cg_buildableShadows", "draw shadows of buildables", Cvar::NONE, false);
Cvar::Cvar<bool> cg_drawTimer("cg_drawTimer", "show game time", Cvar::NONE, true);
Cvar::Range<Cvar::Cvar<int>> cg_drawClock("cg_drawClock", "draw clock (1 = 12-hour 2 = 24-hour)", Cvar::NONE, 0, 0, 2);
Cvar::Cvar<bool> cg_drawFPS("cg_drawFPS", "show client's frames per second", Cvar::NONE, true);
Cvar::Range<Cvar::Cvar<int>> cg_drawCrosshair("cg_drawCrosshair", "draw crosshair (1 = ranged weapons, 2 = always)", Cvar::NONE, 2, 0, 2);
Cvar::Cvar<bool> cg_drawCrosshairHit("cg_drawCrosshairHit", "show damage indicator", Cvar::NONE, true);
Cvar::Range<Cvar::Cvar<int>> cg_crosshairStyle( "cg_crosshairStyle", "crosshair colour style (0 = image colour, 1 = custom colour)", Cvar::NONE, 0, 0, 1 );
Cvar::Cvar<float> cg_crosshairColorRed( "cg_crosshairColorRed", "crosshair colour red", Cvar::NONE, 1.0 );
Cvar::Cvar<float> cg_crosshairColorGreen( "cg_crosshairColorGreen", "crosshair colour green", Cvar::NONE, 1.0 );
Cvar::Cvar<float> cg_crosshairColorBlue( "cg_crosshairColorBlue", "crosshair colour blue", Cvar::NONE, 1.0 );
Cvar::Cvar<float> cg_crosshairColorAlpha( "cg_crosshairColorAlpha", "crosshair colour alpha", Cvar::NONE, 1.0 );
Cvar::Range<Cvar::Cvar<int>> cg_crosshairOutlineStyle( "cg_crosshairOutlineStyle", "crosshair outline style (0 = none, 1 = auto colour, 2 = custom colour)", Cvar::NONE, 0, 0, 2 );
Cvar::Cvar<float> cg_crosshairOutlineScale( "cg_crosshairOutlineScale", "crosshair outline scale", Cvar::NONE, 2.0 );
Cvar::Cvar<float> cg_crosshairOutlineColorRed( "cg_crosshairOutlineColorRed", "crosshair colour green", Cvar::NONE, 0.5 );
Cvar::Cvar<float> cg_crosshairOutlineColorGreen( "cg_crosshairOutlineColorGreen", "crosshair colour green", Cvar::NONE, 0.5 );
Cvar::Cvar<float> cg_crosshairOutlineColorBlue( "cg_crosshairOutlineColorBlue", "crosshair colour blue", Cvar::NONE, 0.5 );
Cvar::Cvar<float> cg_crosshairOutlineColorAlpha( "cg_crosshairOutlineColorAlpha", "crosshair colour alpha", Cvar::NONE, 1.0 );
Cvar::Range<Cvar::Cvar<int>> cg_drawCrosshairFriendFoe("cg_drawCrosshairFriendFoe", "change crosshair color over players (1 = ranged weapons 2 = all)", Cvar::NONE, 0, 0, 2);
Cvar::Range<Cvar::Cvar<int>> cg_drawCrosshairNames("cg_drawCrosshairNames", "draw name of player under crosshair (2 = also client num)", Cvar::NONE, 1, 0, 2);
Cvar::Cvar<bool> cg_drawBuildableHealth("cg_drawBuildableHealth", "show buildable health bar when builder", Cvar::NONE, true);
Cvar::Cvar<bool> cg_drawMinimap("cg_drawMinimap", "show minimap", Cvar::NONE, true);
Cvar::Cvar<int> cg_minimapActive("cg_minimapActive", "FOR INTERNAL USE", Cvar::NONE, 0);
Cvar::Cvar<float> cg_crosshairSize("cg_crosshairSize", "crosshair scale factor", Cvar::NONE, 1);
Cvar::Cvar<bool> cg_draw2D("cg_draw2D", "show HUD", Cvar::NONE, true);
Cvar::Cvar<bool> cg_debugAnim("cg_debuganim", "show animation debug logs", Cvar::CHEAT, false);
Cvar::Cvar<bool> cg_debugEvents("cg_debugevents", "log received events", Cvar::CHEAT, false);
Cvar::Cvar<float> cg_errorDecay("cg_errordecay", "recovery time after prediction error", Cvar::NONE, 100);
Cvar::Cvar<bool> cg_nopredict("cg_nopredict", "disable client-side prediction", Cvar::NONE, false);
Cvar::Cvar<int> cg_debugMove("cg_debugMove", "cgame pmove debug level", Cvar::NONE, 0);
Cvar::Cvar<bool> cg_noPlayerAnims("cg_noplayeranims", "disable player animations", Cvar::CHEAT, false);
Cvar::Cvar<bool> cg_footsteps("cg_footsteps", "make footstep sounds", Cvar::CHEAT, true);
Cvar::Cvar<bool> cg_addMarks("cg_marks", "enable marks (e.g. bullet holes)", Cvar::NONE, true);
Cvar::Cvar<int> cg_viewsize("cg_viewsize", "size of rectangle the world is drawn in", Cvar::NONE, 100);
Cvar::Range<Cvar::Cvar<int>> cg_drawGun("cg_drawGun", "draw 1st-person weapon (1 = guns, 2 = guns & claws, 3 = translucent guns, 4 = translucent guns and claws)", Cvar::NONE, 1, 0, 4);
Cvar::Cvar<float> cg_gun_x("cg_gunX", "model debugging: gun x offset", Cvar::CHEAT, 0);
Cvar::Cvar<float> cg_gun_y("cg_gunY", "model debugging: gun y offset", Cvar::CHEAT, 0);
Cvar::Cvar<float> cg_gun_z("cg_gunZ", "model debugging: gun z offset", Cvar::CHEAT, 0);
Cvar::Cvar<bool> cg_mirrorgun("cg_mirrorgun", "use left-handed gun", Cvar::NONE, false);
Cvar::Range<Cvar::Cvar<float>> cg_tracerChance("cg_tracerchance", "probability to draw line on bullet trajectory", Cvar::CHEAT, 1.0f, 0.0f, 1.0f);
Cvar::Cvar<float> cg_tracerWidth("cg_tracerwidth", "width of line on bullet trajectory", Cvar::CHEAT, 3);
Cvar::Cvar<float> cg_tracerLength("cg_tracerlength", "length of line drawn on bullet trajectory", Cvar::CHEAT, 200);
Cvar::Cvar<bool> cg_thirdPerson("cg_thirdPerson", "show own player from 3rd-person perspective", Cvar::CHEAT, false);
Cvar::Cvar<float> cg_thirdPersonAngle("cg_thirdPersonAngle", "yaw angle for 3rd-person view", Cvar::CHEAT, 0);
Cvar::Range<Cvar::Cvar<int>> cg_thirdPersonShoulderViewMode("cg_thirdPersonShoulderViewMode", "alternative chase cam position", Cvar::NONE, 1, 1, 2);
Cvar::Cvar<bool> cg_staticDeathCam("cg_staticDeathCam", "don't follow attacker movements after death", Cvar::NONE, false);
Cvar::Cvar<bool> cg_thirdPersonPitchFollow("cg_thirdPersonPitchFollow", "do follow the view pitch of the player you follow (disabled by default for comfort)", Cvar::NONE, false);
Cvar::Cvar<float> cg_thirdPersonRange("cg_thirdPersonRange", "camera distance from 3rd-person player", Cvar::NONE, 75);
Cvar::Cvar<bool> cg_lagometer("cg_lagometer", "show network latency meter", Cvar::NONE, false);
Cvar::Range<Cvar::Cvar<int>> cg_drawSpeed("cg_drawSpeed", "show speed. bitflags: 0x1 number, 0x2 graph, 0x4 ignore z-component ", Cvar::NONE, 0, 0, 7);
Cvar::Cvar<int> cg_maxSpeedTimeWindow("cg_maxSpeedTimeWindow", "cg_showSpeed's max speed is over last x milliseconds", Cvar::NONE, 2000);
Cvar::Cvar<bool> cg_blood("com_blood", "draw blood effects", Cvar::NONE, true);
Cvar::Cvar<bool> cg_teamChatsOnly("cg_teamChatsOnly", "don't show chats to all players on screen", Cvar::NONE, false);
Cvar::Range<Cvar::Cvar<int>> cg_teamOverlayUserinfo("teamoverlay", "request team overlay data from server", Cvar::USERINFO, 1, 0, 1);
Cvar::Cvar<bool> cg_noVoiceChats("cg_noVoiceChats", "don't play vsays", Cvar::NONE, false);
Cvar::Cvar<bool> cg_noVoiceText("cg_noVoiceText", "don't show text for vsays", Cvar::NONE, false);
Cvar::Cvar<bool> cg_smoothClients("cg_smoothClients", "extrapolate entity positions", Cvar::NONE, false);
Cvar::Cvar<bool> cg_noTaunt("cg_noTaunt", "disable taunt sounds", Cvar::NONE, false);
Cvar::Cvar<bool> cg_drawSurfNormal("cg_drawSurfNormal", "visualize normal vector of facing surface", Cvar::CHEAT, false);
Cvar::Range<Cvar::Cvar<int>> cg_drawBBOX("cg_drawBBOX", "show entity bounding boxes (2 = solid)", Cvar::CHEAT, 0, 0, 2);
Cvar::Cvar<bool> cg_drawEntityInfo("cg_drawEntityInfo", "show number and type of facing entity", Cvar::CHEAT, false);
Cvar::Cvar<int> cg_wwSmoothTime("cg_wwSmoothTime", "time (ms) to rotate view while wallwalking", Cvar::NONE, 150);
Cvar::Cvar<bool> cg_bounceParticles("cg_bounceParticles", "particles may bounce off surfaces, rather than destruct", Cvar::NONE, true);
Cvar::Cvar<int> cg_consoleLatency("cg_consoleLatency", "how long chat messages appear (milliseconds)", Cvar::NONE, 3000);
Cvar::Range<Cvar::Cvar<int>> cg_lightFlare("cg_lightFlare", "style of 'light flares'", Cvar::NONE, 3, 0, 3);
Cvar::Range<Cvar::Cvar<int>> cg_debugParticles("cg_debugParticles", "log level for particles", Cvar::CHEAT, 0, 0, 2);
Cvar::Cvar<bool> cg_debugPVS("cg_debugPVS", "log entities in Potentially Visible Set", Cvar::CHEAT, false);
Cvar::Range<Cvar::Cvar<int>> cg_disableWarningDialogs("cg_disableWarningDialogs", "gameplay warning style: 0 = center print, 1 = log, 2 = none", Cvar::NONE, 0, 0, 2);
Cvar::Cvar<bool> cg_tutorial("cg_tutorial", "show tutorial text", Cvar::NONE, true);

Cvar::Cvar<bool> cg_rangeMarkerDrawSurface("cg_rangeMarkerDrawSurface", "shade buildable range surfaces", Cvar::NONE, true);
Cvar::Cvar<bool> cg_rangeMarkerDrawIntersection("cg_rangeMarkerDrawIntersection", "outline insersections between buildable range surfaces", Cvar::NONE, false);
Cvar::Cvar<bool> cg_rangeMarkerDrawFrontline("cg_rangeMarkerDrawFrontline", "outline edges of buildable range surfaces", Cvar::NONE, false);
Cvar::Range<Cvar::Cvar<float>> cg_rangeMarkerSurfaceOpacity("cg_rangeMarkerSurfaceOpacity", "opacity of buildable range surfaces", Cvar::NONE, 0.08, 0, 1);
Cvar::Range<Cvar::Cvar<float>> cg_rangeMarkerLineOpacity("cg_rangeMarkerLineOpacity", "opacity of buildable range outlines", Cvar::NONE, 0.4, 0, 1);
Cvar::Cvar<float> cg_rangeMarkerLineThickness("cg_rangeMarkerLineThickness", "thickness of buildable range surface outlines", Cvar::NONE, 4.0);
Cvar::Cvar<bool> cg_rangeMarkerForBlueprint("cg_rangeMarkerForBlueprint", "show range marker when placing buildable", Cvar::NONE, true);
Cvar::Modified<Cvar::Cvar<std::string>> cg_rangeMarkerBuildableTypes("cg_rangeMarkerBuildableTypes", "list of buildables or buildable types to show range marker for", Cvar::NONE, "support");
Cvar::Cvar<bool> cg_rangeMarkerWhenSpectating("cg_rangeMarkerWhenSpectating", "show buildable rangers while spectating", Cvar::NONE, false);
int cg_buildableRangeMarkerMask;
Cvar::Range<Cvar::Cvar<float>> cg_binaryShaderScreenScale("cg_binaryShaderScreenScale", "I don't know", Cvar::NONE, 1.0, 0, 1);

Cvar::Cvar<float> cg_painBlendUpRate("cg_painBlendUpRate", "how fast the pain indicator will appear", Cvar::NONE, 10.0);
Cvar::Cvar<float> cg_painBlendDownRate("cg_painBlendDownRate", "how fast the pain indicator will disappear", Cvar::NONE, 0.5);
Cvar::Cvar<float> cg_painBlendMax("cg_painBlendMax", "upper bound on how opaque the pain indicator will be", Cvar::NONE, 0.7);
Cvar::Cvar<float> cg_painBlendScale("cg_painBlendScale", "how amplified the damage will be for the blood indicator (1->damage is barely visible, 20->damage reaches cg_painBlendMax almost instantly", Cvar::NONE, 7.0);
Cvar::Cvar<float> cg_painBlendZoom("cg_painBlendZoom", "size scale factor for the the pain indicator", Cvar::NONE, 0.65);

Cvar::Range<Cvar::Cvar<int>> cg_stickySpec("cg_stickySpec", "if 0, cycle followed player upon death", Cvar::USERINFO, 1, 0, 1);
Cvar::Range<Cvar::Cvar<int>> cg_sprintToggle("cg_sprintToggle", "toggle instead of hold to sprint", Cvar::USERINFO, 0, 0, 1);
Cvar::Range<Cvar::Cvar<int>> cg_unlagged("cg_unlagged", "lag-compensate your player (if server allows)", Cvar::USERINFO, 1, 0, 1);

Cvar::Cvar<std::string> cg_cmdGrenadeThrown("cg_cmdGrenadeThrown", "command executed upon throwing grenade", Cvar::NONE, "vsay_local grenade");

Cvar::Cvar<bool> cg_debugVoices("cg_debugVoices", "print cgame's list of vsays on startup", Cvar::NONE, false);

Cvar::Cvar<bool> cg_optimizePrediction("cg_optimizePrediction", "client-side prediction is done incrementally", Cvar::NONE, true);
Cvar::Cvar<bool> cg_projectileNudge("cg_projectileNudge", "enable client-side prediction for missiles", Cvar::NONE, true);

Cvar::Cvar<bool> cg_emoticonsInMessages("cg_emoticonsInMessages", "render emoticons in chat", Cvar::NONE, false);

Cvar::Cvar<bool> cg_chatTeamPrefix("cg_chatTeamPrefix", "show [H] or [A] before names in chat", Cvar::NONE, true);

Cvar::Cvar<bool> cg_animSpeed("cg_animspeed", "run animations? (for debugging)", Cvar::CHEAT, true);
Cvar::Cvar<float> cg_animBlend("cg_animblend", "I don't know", Cvar::NONE, 5.0);

Cvar::Cvar<float> cg_motionblur("cg_motionblur", "strength of motion blur", Cvar::NONE, 0.05);
Cvar::Cvar<float> cg_motionblurMinSpeed("cg_motionblurMinSpeed", "minimum speed to trigger motion blur", Cvar::NONE, 600);
Cvar::Cvar<bool> cg_spawnEffects("cg_spawnEffects", "desaturate world view when dead or spawning", Cvar::NONE, true);

static Cvar::Cvar<bool> cg_navgenOnLoad("cg_navgenOnLoad", "generate navmeshes when starting a local game", Cvar::NONE, true);

// search 'fovCvar' to find usage of these (names come from config files)
// 0 means use global FOV setting
static Cvar::Cvar<float> cg_fov_builder("cg_fov_builder", "field of view (degrees) for Granger", Cvar::NONE, 0);
static Cvar::Cvar<float> cg_fov_level0("cg_fov_level0", "field of view (degrees) for Dretch", Cvar::NONE, 0);
static Cvar::Cvar<float> cg_fov_level1("cg_fov_level1", "field of view (degrees) for Mantis", Cvar::NONE, 0);
static Cvar::Cvar<float> cg_fov_level2("cg_fov_level2", "field of view (degrees) for Marauder", Cvar::NONE, 0);
static Cvar::Cvar<float> cg_fov_level3("cg_fov_level3", "field of view (degrees) for Dragoon", Cvar::NONE, 0);
static Cvar::Cvar<float> cg_fov_level4("cg_fov_level4", "field of view (degrees) for Tyrant", Cvar::NONE, 0);
static Cvar::Cvar<float> cg_fov_human("cg_fov_human", "field of view (degrees) for humans", Cvar::NONE, 0);

Cvar::Cvar<bool> ui_chatPromptColors("ui_chatPromptColors", "chat prompts (e.g. 'Say:') are color-coded", Cvar::NONE, true);
Cvar::Cvar<std::string> cg_sayCommand("cg_sayCommand", "instead of talking, chat field does this command?", Cvar::NONE, "");

// CHEAT because it could be abused to join the game faster and e.g. get on your preferred team
// It's intended to aid developers who are frequently restarting the game.
// In normal play, it would be undesirable as it causes lag when someone first uses a class.
// TODO: only works for player models. Buildings and weapons are also relevant
Cvar::Cvar<bool> cg_lazyLoadModels("cg_lazyLoadModels", "load models only when needed", Cvar::CHEAT, false);

// USERINFO cvars - transmitted to the server
static Cvar::Range<Cvar::Cvar<int>> cg_disableBlueprintErrors("cg_disableBlueprintErrors", "allow placement of some currently non-buildable structures", Cvar::USERINFO, 0, 0, 1);
static Cvar::Cvar<int> cg_flySpeed("cg_flySpeed", "spectator movement speed", Cvar::USERINFO, 800);
static Cvar::Cvar<std::string> cg_voice("voice", "track selection for user's own vsays", Cvar::USERINFO, "default");
static Cvar::Range<Cvar::Cvar<int>> cg_wwFollow("cg_wwFollow", "rotate pitch angle when wallwalk normal changes", Cvar::USERINFO, 1, 0, 1);
static Cvar::Range<Cvar::Cvar<int>> cg_wwToggle("cg_wwToggle", "wallwalk key is press-and-hold (0) or toggles (1)", Cvar::USERINFO, 1, 0, 1);

/*
===============
CG_SetPVars

Set some player cvars usable in scripts
these should refer only to playerstates that belong to the client, not the followed player, ui cvars will do that already
===============
*/
static void CG_SetPVars()
{
	playerState_t *ps;

	if ( !cg.snap )
	{
		return;
	}

	ps = &cg.snap->ps;

	/* if we follow someone, the stats won't be about us, but the followed player instead */
	if ( ( ps->pm_flags & PMF_FOLLOW ) )
	{
	        return;
	}

	trap_Cvar_Set( "p_teamname", BG_TeamName( ps->persistant[ PERS_TEAM ] ) );

	switch ( ps->persistant[ PERS_TEAM ] )
	{
		case TEAM_ALIENS:
		case TEAM_HUMANS:
			break;

		default:
		case TEAM_NONE:
			trap_Cvar_Set( "p_classname", "Spectator" );
			trap_Cvar_Set( "p_weaponname", "Nothing" );
			trap_Cvar_Set( "p_class" , "0" );
			trap_Cvar_Set( "p_weapon", "0" );
			return;
	}

	trap_Cvar_Set( "p_class", va( "%d", ps->stats[ STAT_CLASS ] ) );
	trap_Cvar_Set( "p_classname", BG_Class( ps->stats[ STAT_CLASS ] )->name );
	trap_Cvar_Set( "p_weapon", va( "%d", ps->stats[ STAT_WEAPON ] ) );
	trap_Cvar_Set( "p_weaponname", BG_Weapon( ps->stats[ STAT_WEAPON ] )->humanName );
}

/*
================
CG_UpdateBuildableRangeMarkerMask
================
*/
void CG_UpdateBuildableRangeMarkerMask()
{
	constexpr int buildables_alien =
		     ( 1 << BA_A_OVERMIND ) | ( 1 << BA_A_SPAWN ) |
		     ( 1 << BA_A_ACIDTUBE ) | ( 1 << BA_A_TRAPPER ) |
		     ( 1 << BA_A_HIVE ) | ( 1 << BA_A_LEECH ) |
		     ( 1 << BA_A_BOOSTER ); // TODO: add spiker

	constexpr int buildables_human =
		     ( 1 << BA_H_REACTOR ) | ( 1 << BA_H_MGTURRET ) |
		     ( 1 << BA_H_ROCKETPOD ) | ( 1 << BA_H_DRILL );

	constexpr int buildables_support =
		     ( 1 << BA_A_OVERMIND ) | ( 1 << BA_A_SPAWN ) |
		     ( 1 << BA_A_LEECH ) | ( 1 << BA_A_BOOSTER ) |
		     ( 1 << BA_H_REACTOR ) | ( 1 << BA_H_DRILL );

	constexpr int buildables_offensive =
		     ( 1 << BA_A_ACIDTUBE ) | ( 1 << BA_A_TRAPPER ) |
		     ( 1 << BA_A_HIVE ) | ( 1 << BA_H_MGTURRET ) |
		     ( 1 << BA_H_ROCKETPOD ); // TODO: add spiker

	static_assert( (buildables_alien | buildables_human) == (buildables_support | buildables_offensive), "you probably forgot to add a buildable in either list" );
	static_assert( (buildables_alien & buildables_human) == 0, "buildable can't be both human and alien" );
	static_assert( (buildables_support & buildables_offensive) == 0, "buildables are likely not supposed to be both offensive and defensive" ); // optional

	constexpr struct { const char *key; int buildableMask; } mappings[] = {
		{ "all",            buildables_alien | buildables_human     },
		{ "alien",          buildables_alien                        },
		{ "human",          buildables_human                        },
		{ "support",        buildables_support                      },
		{ "aliensupport",   buildables_alien & buildables_support   },
		{ "humansupport",   buildables_human & buildables_support   },
		{ "offensive",      buildables_offensive                    },
		{ "alienoffensive", buildables_alien & buildables_offensive },
		{ "humanoffensive", buildables_human & buildables_offensive },
		{ "none",           0                                       },
	};

	if ( Util::optional<std::string> structureList = cg_rangeMarkerBuildableTypes.GetModifiedValue() )
	{
		int brmMask = 0;

		for (Parse_WordListSplitter marker(*std::move(structureList)); *marker; ++marker)
		{
			buildable_t buildable = BG_BuildableByName( *marker )->number;

			if ( buildable != BA_NONE )
			{
				// add it to list
				brmMask |= 1 << buildable;
			}
			else
			{
				for (auto map : mappings) {
					if ( Q_stricmp(map.key, *marker) == 0 ) {
						brmMask |= map.buildableMask;
					}
				}
			}
		}
		cg_buildableRangeMarkerMask = brmMask;
	}
}

void CG_NotifyHooks()
{
	playerState_t *ps;
	char config[ MAX_CVAR_VALUE_STRING ];
	static int lastTeam = INT_MIN; //to make sure we run the hook initially as well

	if ( !cg.snap )
	{
		return;
	}

	ps = &cg.snap->ps;
	if ( !( ps->pm_flags & PMF_FOLLOW ) )
	{
		if( lastTeam != ps->persistant[ PERS_TEAM ] )
		{
			trap_notify_onTeamChange( ps->persistant[ PERS_TEAM ] );

			CG_SetBindTeam( static_cast<team_t>( ps->persistant[ PERS_TEAM ] ) );

			/* execute team-specific config files */
			trap_Cvar_VariableStringBuffer( va( "cg_%sConfig", BG_TeamName( ps->persistant[ PERS_TEAM ] ) ), config, sizeof( config ) );
			if ( config[ 0 ] )
			{
				trap_SendConsoleCommand( ( "exec " + Cmd::Escape( config ) ).c_str() );
			}

			lastTeam = ps->persistant[ PERS_TEAM ];
		}
	}
}

/*
=================
CG_UpdateCvars
=================
*/
void CG_UpdateCvars()
{
	CG_SetPVars();
	CG_UpdateBuildableRangeMarkerMask();
}

/*
================
CG_Argv
================
*/
const char *CG_Argv( int arg )
{
	static char buffer[ MAX_STRING_CHARS ];

	trap_Argv( arg, buffer, sizeof( buffer ) );

	return buffer;
}

/*
================
CG_Args
================
*/
const char *CG_Args()
{
	static char buffer[ MAX_STRING_CHARS ];

	trap_LiteralArgs( buffer, sizeof( buffer ) );

	return buffer;
}

//========================================================================

static const char *choose( const char *first, ... )
{
	va_list    ap;
	int        count = 1;
	const char *ret;

	va_start( ap, first );
	while ( va_arg( ap, const char * ) )
	{
		++count;
	}
	va_end( ap );

	if ( count < 2 )
	{
		return first;
	}

	count = rand() % count;

	ret = first;
	va_start( ap, first );
	while ( count-- )
	{
		ret = va_arg( ap, const char * );
	}
	va_end( ap );

	return ret;
}

static void CG_UpdateMediaFraction( float fraction )
{
	cg.mediaLoadingFraction = fraction;
	trap_UpdateScreen();
}

enum cgLoadingStep_t {
	LOAD_START = 0,
	LOAD_TRAILS,
	LOAD_PARTICLES,
	LOAD_CONFIGS,
	LOAD_SOUNDS,
	LOAD_GEOMETRY,
	LOAD_ASSETS,
	LOAD_WEAPONS,
	LOAD_UPGRADES,
	LOAD_CLASSES,
	LOAD_BUILDINGS,
	LOAD_CLIENTS,
	LOAD_HUDS,
	LOAD_CHECK_NAVMESH, // only checking for existence, not generation
	LOAD_GLSL,
	LOAD_DONE
};

/*
======================
CG_UpdateLoadingProgress

======================
*/

static void CG_UpdateLoadingProgress( int step, const char *label, const char* loadingText )
{
	cg.loadingFraction = ( 1.0f * step ) / LOAD_DONE;

	Q_strncpyz( cg.loadingText, loadingText, sizeof( cg.loadingText ) );

	Log::Debug( "CG_Init: %d%% %s.", static_cast<int>( 100 * cg.loadingFraction ), label );

	if( cg.loading )
	{
		trap_UpdateScreen();
	}
}

static void CG_UpdateLoadingStep( cgLoadingStep_t step )
{
	switch (step) {
		case LOAD_START:
			/* Note: this is too early to do screen updates,
			so set cg.loading to true after printing the log. */
			CG_UpdateLoadingProgress( step, "Start", choose(_("Calling home…"), _("Sending to the front…"), nullptr) );
			cg.loading = true;
			break;

		case LOAD_TRAILS:
			CG_UpdateLoadingProgress( step, "Trails", choose(_("Tracking your movements…"), _("Letting out the magic smoke…"), nullptr) );
			break;

		case LOAD_PARTICLES:
			CG_UpdateLoadingProgress( step, "Particles", choose(_("Collecting bees for the hives…"), _("Initialising fireworks…"), _("Causing electrical faults…"), nullptr) );
			break;

		case LOAD_CONFIGS:
			CG_UpdateLoadingProgress( step, "Configurations", choose(_("Reading the manual…"), _("Looking at blueprints…"), nullptr) );
			break;

		case LOAD_SOUNDS:
			CG_UpdateLoadingProgress( step, "Sounds", choose(_("Generating annoying noises…"), _("Recording granger purring…"), nullptr) );
			break;

		case LOAD_GEOMETRY:
			CG_UpdateLoadingProgress( step, "Geometry", choose(_("Hello World!"), _("Making a scene…"), nullptr) );
			break;

		case LOAD_ASSETS:
			CG_UpdateLoadingProgress( step, "Assets", choose(_("Taking pictures of the world…"), _("Drawing smiley faces…"), nullptr) );
			break;

		case LOAD_WEAPONS:
			CG_UpdateLoadingProgress( step, "Weapons", choose(_("Sharpening the aliens' claws…"), _("Overloading lucifer cannons…"), nullptr) );
			break;

		case LOAD_UPGRADES:
			CG_UpdateLoadingProgress( step, "Upgrades", choose(_("Unwrapping jetpacks…"), _("Spinning silk for reticles…"), nullptr) );
			break;

		case LOAD_BUILDINGS:
			CG_UpdateLoadingProgress( step, "Buildings", choose(_("Adding turret spam…"), _("Awakening the overmind…"), nullptr) );
			break;

		case LOAD_CLIENTS:
			CG_UpdateLoadingProgress( step, "Clients", choose(_("Teleporting soldiers…"), _("Replicating alien DNA…"), nullptr) );
			break;

		case LOAD_HUDS:
		CG_UpdateLoadingProgress( step, "Huds", choose(_("Customizing helmets…"), nullptr) );
			break;

		case LOAD_GLSL:
			CG_UpdateLoadingProgress( step, "GLSL shaders", choose(_("Compiling GLSL shaders (please be patient)…"), nullptr) );
			break;

		case LOAD_CHECK_NAVMESH:
			CG_UpdateLoadingProgress( step, "Navmesh existence", choose(_("Surveying the map…"), nullptr) );
			break;

		case LOAD_DONE:
			CG_UpdateLoadingProgress( step, "Done", _("Done!") );
			cg.loading = false;
			break;

		default:
			break;
	}
}

/*
=================
CG_RegisterSounds

called during a precache command
=================
*/
static void CG_RegisterSounds()
{
	int        i;
	char       name[ MAX_QPATH ];
	const char *soundName;

	cgs.media.weHaveEvolved = trap_S_RegisterSound( "sound/announcements/overmindevolved", true );
	cgs.media.reinforcement = trap_S_RegisterSound( "sound/announcements/reinforcement", true );

	cgs.media.alienOvermindAttack = trap_S_RegisterSound( "sound/announcements/overmindattack", true );
	cgs.media.alienOvermindDying = trap_S_RegisterSound( "sound/announcements/overminddying", true );
	cgs.media.alienOvermindSpawns = trap_S_RegisterSound( "sound/announcements/overmindspawns", true );

	cgs.media.alienL4ChargePrepare = trap_S_RegisterSound( "sound/player/level4/charge_prepare", true );
	cgs.media.alienL4ChargeStart = trap_S_RegisterSound( "sound/player/level4/charge_start", true );

	cgs.media.selectSound = trap_S_RegisterSound( "sound/weapons/change", false );
	cgs.media.turretSpinupSound = trap_S_RegisterSound( "sound/buildables/mgturret/spinup", false );
	cgs.media.weaponEmptyClick = trap_S_RegisterSound( "sound/weapons/click", false );

	cgs.media.talkSound = trap_S_RegisterSound( "sound/feedback/talk", false );
	cgs.media.alienTalkSound = trap_S_RegisterSound( "sound/feedback/alien_talk", false );
	cgs.media.humanTalkSound = trap_S_RegisterSound( "sound/feedback/human_talk", false );
	cgs.media.landSound = trap_S_RegisterSound( "sound/player/land1", false );

	cgs.media.watrInSound = trap_S_RegisterSound( "sound/player/watr_in", false );
	cgs.media.watrOutSound = trap_S_RegisterSound( "sound/player/watr_out", false );
	cgs.media.watrUnSound = trap_S_RegisterSound( "sound/player/watr_un", false );

	cgs.media.disconnectSound = trap_S_RegisterSound( "sound/feedback/disconnect", false );

	for ( i = 0; i < 4; i++ )
	{
		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/step%i", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_GENERAL ][ i ] = trap_S_RegisterSound( name, false );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/flesh%i", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_FLESH ][ i ] = trap_S_RegisterSound( name, false );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/splash%i", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_SPLASH ][ i ] = trap_S_RegisterSound( name, false );

		Com_sprintf( name, sizeof( name ), "sound/player/footsteps/clank%i", i + 1 );
		cgs.media.footsteps[ FOOTSTEP_METAL ][ i ] = trap_S_RegisterSound( name, false );
	}

	for ( i = 1; i < MAX_SOUNDS; i++ )
	{
		soundName = CG_ConfigString( CS_SOUNDS + i );

		if ( !soundName[ 0 ] )
		{
			break;
		}

		if ( soundName[ 0 ] == '*' )
		{
			continue; // custom sound
		}

		cgs.gameSounds[ i ] = trap_S_RegisterSound( soundName, false );
	}

	cgs.media.jetpackThrustLoopSound = trap_S_RegisterSound( "sound/upgrades/jetpack/hi", false );

	cgs.media.medkitUseSound = trap_S_RegisterSound( "sound/upgrades/medkit/medkit", false );

	cgs.media.alienEvolveSound = trap_S_RegisterSound( "sound/player/alienevolve", false );

	cgs.media.alienBuildableDying1 = trap_S_RegisterSound( "sound/buildables/alien/construct1", false );
	cgs.media.alienBuildableDying2 = trap_S_RegisterSound( "sound/buildables/alien/construct2", false );
	cgs.media.alienBuildableExplosion = trap_S_RegisterSound( "sound/buildables/alien/explosion", false );
	cgs.media.alienBuildablePrebuild = trap_S_RegisterSound( "sound/buildables/alien/prebuild", false );

	cgs.media.humanBuildableDyingLarge = trap_S_RegisterSound( "sound/buildables/human/dying", false );
	cgs.media.humanBuildableDying = trap_S_RegisterSound( "sound/buildables/human/destroyed", false );
	cgs.media.humanBuildableExplosion = trap_S_RegisterSound( "sound/buildables/human/explosion", false );
	cgs.media.humanBuildablePrebuild = trap_S_RegisterSound( "sound/buildables/human/prebuild", false );

	for ( i = 0; i < 4; i++ )
	{
		cgs.media.humanBuildableDamage[ i ] = trap_S_RegisterSound(
		                                        va( "sound/buildables/human/damage%d", i ), false );
	}

	cgs.media.grenadeBounceSound0 = trap_S_RegisterSound( "models/weapons/grenade/bounce0", false );
	cgs.media.grenadeBounceSound1 = trap_S_RegisterSound( "models/weapons/grenade/bounce1", false );

	// TODO: Rename this sound.
	cgs.media.itemFillSound = trap_S_RegisterSound( "sound/buildables/repeater/use", false );

	cgs.media.buildableRepairSound = trap_S_RegisterSound( "sound/buildables/human/repair", false );
	cgs.media.buildableRepairedSound = trap_S_RegisterSound( "sound/buildables/human/repaired", false );

	cgs.media.lCannonWarningSound = trap_S_RegisterSound( "models/weapons/lcannon/warning", false );
	cgs.media.lCannonWarningSound2 = trap_S_RegisterSound( "models/weapons/lcannon/warning2", false );

	cgs.media.rocketpodLockonSound = trap_S_RegisterSound( "sound/buildables/rocketpod/lockon", false );

	cgs.media.timerBeaconExpiredSound = trap_S_RegisterSound( "sound/feedback/beacons/timer-expired", false );
	cgs.media.killSound = trap_S_RegisterSound( "sound/feedback/damage/bell", false );
}

//===================================================================================

/*
=================
CG_RegisterGrading
=================
*/
void CG_RegisterGrading( int slot, const char *str )
{
	int   model;
	float dist;
	char  texture[MAX_QPATH];

	if( !str || !*str ) {
		cgs.gameGradingTextures[ slot ]  = 0;
		cgs.gameGradingModels[ slot ]    = 0;
		cgs.gameGradingDistances[ slot ] = 0.0f;
		return;
	}

	sscanf(str, "%d %f %63s", &model, &dist, texture);
	cgs.gameGradingTextures[ slot ] =
		trap_R_RegisterShader(texture, (RegisterShaderFlags_t) ( RSF_NOMIP | RSF_NOLIGHTSCALE ) );
	cgs.gameGradingModels[ slot ] = model;
	cgs.gameGradingDistances[ slot ] = dist;
}

/*
=================
CG_RegisterReverb
=================
*/
static void CG_RegisterReverb( int slot, const char *str )
{
	int   model;
	float dist, intensity;
	char  name[MAX_NAME_LENGTH];

	if( !str || !*str ) {
		Q_strncpyz(cgs.gameReverbEffects[ slot ], "none", MAX_NAME_LENGTH);
		cgs.gameReverbModels[ slot ]        = 0;
		cgs.gameReverbDistances[ slot ]     = 0.0f;
		cgs.gameReverbIntensities[ slot ]   = 0.0f;
		return;
	}

	sscanf(str, "%d %f %127s %f", &model, &dist, name, &intensity);
	Q_strncpyz(cgs.gameReverbEffects[ slot ], name, MAX_NAME_LENGTH);
	cgs.gameReverbModels[ slot ] = model;
	cgs.gameReverbDistances[ slot ] = dist;
	cgs.gameReverbIntensities[ slot ] = intensity;
}

/*
=================
CG_RegisterGraphics
=================
*/
static void CG_RegisterGraphics()
{
	int         i;
	static const char *const sb_nums[ 11 ] =
	{
		"ui/assets/numbers/zero_32b",
		"ui/assets/numbers/one_32b",
		"ui/assets/numbers/two_32b",
		"ui/assets/numbers/three_32b",
		"ui/assets/numbers/four_32b",
		"ui/assets/numbers/five_32b",
		"ui/assets/numbers/six_32b",
		"ui/assets/numbers/seven_32b",
		"ui/assets/numbers/eight_32b",
		"ui/assets/numbers/nine_32b",
		"ui/assets/numbers/minus_32b",
	};
	static const char *const buildWeaponTimerPieShaders[ 8 ] =
	{
		"ui/assets/neutral/1_5pie",
		"ui/assets/neutral/3_0pie",
		"ui/assets/neutral/4_5pie",
		"ui/assets/neutral/6_0pie",
		"ui/assets/neutral/7_5pie",
		"ui/assets/neutral/9_0pie",
		"ui/assets/neutral/10_5pie",
		"ui/assets/neutral/12_0pie",
	};

	// clear any references to old media
	memset( &cg.refdef, 0, sizeof( cg.refdef ) );
	trap_R_ClearScene();

	CG_UpdateLoadingStep( LOAD_GEOMETRY );
	trap_R_LoadWorldMap( va( "maps/%s.bsp", cgs.mapname ) );

	CG_UpdateLoadingStep( LOAD_ASSETS );

	for ( i = 0; i < 11; i++ )
	{
		cgs.media.numberShaders[ i ] = trap_R_RegisterShader(sb_nums[i], (RegisterShaderFlags_t) ( RSF_NOMIP ) );
	}

	cgs.media.viewBloodShader = trap_R_RegisterShader("gfx/feedback/painblend", (RegisterShaderFlags_t) ( RSF_NOMIP ) );

	cgs.media.connectionShader = trap_R_RegisterShader("gfx/feedback/net", (RegisterShaderFlags_t) ( RSF_NOMIP ) );

	cgs.media.creepShader = trap_R_RegisterShader("gfx/buildables/creep/creep", RSF_DEFAULT );

	cgs.media.scannerBlipShader = trap_R_RegisterShader("gfx/feedback/scanner/blip", (RegisterShaderFlags_t) ( RSF_NOMIP ) );

	cgs.media.scannerBlipBldgShader = trap_R_RegisterShader("gfx/feedback/scanner/blip_bldg", (RegisterShaderFlags_t) ( RSF_NOMIP ) );

	cgs.media.scannerLineShader = trap_R_RegisterShader("gfx/feedback/scanner/stalk", (RegisterShaderFlags_t) ( RSF_NOMIP ) );

	cgs.media.tracerShader = trap_R_RegisterShader("gfx/weapons/tracer/tracer", RSF_NOMIP);

	cgs.media.backTileShader = trap_R_RegisterShader("gfx/colors/backtile", (RegisterShaderFlags_t) ( RSF_NOMIP ) );

	// building shaders
	cgs.media.greenBuildShader = trap_R_RegisterShader("gfx/buildables/common/greenbuild", RSF_DEFAULT );
	cgs.media.yellowBuildShader = trap_R_RegisterShader("gfx/buildables/common/yellowbuild", RSF_DEFAULT );
	cgs.media.redBuildShader = trap_R_RegisterShader("gfx/buildables/common/redbuild", RSF_DEFAULT );
	cgs.media.humanSpawningShader = trap_R_RegisterShader("gfx/buildables/human_base/spawning", RSF_DEFAULT );

	for ( i = 0; i < 8; i++ )
	{
		cgs.media.buildWeaponTimerPie[ i ] = trap_R_RegisterShader(buildWeaponTimerPieShaders[i], RSF_NOMIP);
	}

	// player health cross shaders
	cgs.media.healthCross = trap_R_RegisterShader("ui/assets/neutral/cross", RSF_NOMIP);
	cgs.media.healthCross2X = trap_R_RegisterShader("ui/assets/neutral/cross2", RSF_NOMIP);
	cgs.media.healthCross3X = trap_R_RegisterShader("ui/assets/neutral/cross3", RSF_NOMIP);
	cgs.media.healthCross4X = trap_R_RegisterShader("ui/assets/neutral/cross4", RSF_NOMIP);
	cgs.media.healthCrossMedkit = trap_R_RegisterShader("ui/assets/neutral/cross_medkit", RSF_NOMIP);
	cgs.media.healthCrossPoisoned = trap_R_RegisterShader("ui/assets/neutral/cross_poison", RSF_NOMIP);

	cgs.media.desaturatedCgrade = trap_R_RegisterShader("gfx/cgrading/desaturated", (RegisterShaderFlags_t) ( RSF_NOMIP | RSF_NOLIGHTSCALE ) );
	cgs.media.neutralCgrade = trap_R_RegisterShader("gfx/cgrading/neutral", (RegisterShaderFlags_t) ( RSF_NOMIP | RSF_NOLIGHTSCALE ) );
	cgs.media.redCgrade = trap_R_RegisterShader("gfx/cgrading/red-only", (RegisterShaderFlags_t) ( RSF_NOMIP | RSF_NOLIGHTSCALE ) );
	cgs.media.tealCgrade = trap_R_RegisterShader("gfx/cgrading/teal-only", (RegisterShaderFlags_t) ( RSF_NOMIP | RSF_NOLIGHTSCALE ) );

	cgs.media.balloonShader = trap_R_RegisterShader("gfx/feedback/chatballoon", (RegisterShaderFlags_t) ( RSF_NOMIP ) );

	cgs.media.disconnectPS = CG_RegisterParticleSystem( "particles/feedback/disconnect" );

	cgs.media.sniperScopeShader =
		trap_R_RegisterShader( "gfx/weapons/scope", (RegisterShaderFlags_t)( RSF_NOMIP ) );
	cgs.media.lgunScopeShader =
		trap_R_RegisterShader( "gfx/weapons/scope", (RegisterShaderFlags_t)( RSF_NOMIP ) );

	CG_UpdateMediaFraction( 0.2f );

	memset( cg_weapons, 0, sizeof( cg_weapons ) );
	memset( cg_upgrades, 0, sizeof( cg_upgrades ) );

	cgs.media.shadowMarkShader = trap_R_RegisterShader("gfx/players/common/shadow", RSF_DEFAULT);
	cgs.media.wakeMarkShader = trap_R_RegisterShader("gfx/players/common/wake", RSF_DEFAULT);

	cgs.media.alienEvolvePS = CG_RegisterParticleSystem( "particles/players/alien_base/evolve" );
	cgs.media.alienAcidTubePS = CG_RegisterParticleSystem( "particles/buildables/acide_tube/spore" );
	cgs.media.alienBoosterPS = CG_RegisterParticleSystem( "particles/buildables/booster/spore" );

	cgs.media.jetPackThrustPS = CG_RegisterParticleSystem( "particles/players/human_base/jetpack_ascend" );

	cgs.media.humanBuildableDamagedPS = CG_RegisterParticleSystem( "particles/buildables/human_base/damaged" );
	cgs.media.alienBuildableDamagedPS = CG_RegisterParticleSystem( "particles/buildables/alien_base/damaged" );
	cgs.media.humanBuildableDestroyedPS = CG_RegisterParticleSystem( "particles/buildables/human_base/destroyed" );
	cgs.media.humanBuildableNovaPS = CG_RegisterParticleSystem( "particles/buildables/human_base/nova" );
	cgs.media.alienBuildableDestroyedPS = CG_RegisterParticleSystem( "particles/buildables/alien_base/destroyed" );

	cgs.media.humanBuildableBleedPS = CG_RegisterParticleSystem( "particles/buildables/human_base/bleed" );
	cgs.media.alienBuildableBleedPS = CG_RegisterParticleSystem( "particles/buildables/alien_base/bleed" );
	cgs.media.alienBuildableBurnPS  = CG_RegisterParticleSystem( "particles/buildables/alien_base/burn" );

	cgs.media.floorFirePS = CG_RegisterParticleSystem( "particles/weapons/flamer/floorfire" );

	cgs.media.alienBleedPS = CG_RegisterParticleSystem( "particles/players/alien_base/bleed" );
	cgs.media.humanBleedPS = CG_RegisterParticleSystem( "particles/players/human_base/bleed" );

	cgs.media.sphereModel = trap_R_RegisterModel( "models/generic/sphere.md3" );
	cgs.media.sphericalCone64Model = trap_R_RegisterModel( "models/generic/sphericalCone64.md3" );
	cgs.media.sphericalCone240Model = trap_R_RegisterModel( "models/generic/sphericalCone240.md3" );

	cgs.media.plainColorShader = trap_R_RegisterShader("gfx/colors/plain", RSF_DEFAULT);
	cgs.media.binaryAlpha1Shader = trap_R_RegisterShader("gfx/binary/alpha1", RSF_DEFAULT);

	for ( i = 0; i < NUM_BINARY_SHADERS; ++i )
	{
		cgs.media.binaryShaders[ i ].f1 = trap_R_RegisterShader(va("gfx/binary/%03i_F1", i), RSF_DEFAULT);
		cgs.media.binaryShaders[ i ].f2 = trap_R_RegisterShader(va("gfx/binary/%03i_F2", i), RSF_DEFAULT);
		cgs.media.binaryShaders[ i ].f3 = trap_R_RegisterShader(va("gfx/binary/%03i_F3", i), RSF_DEFAULT);
		cgs.media.binaryShaders[ i ].b1 = trap_R_RegisterShader(va("gfx/binary/%03i_B1", i), RSF_DEFAULT);
		cgs.media.binaryShaders[ i ].b2 = trap_R_RegisterShader(va("gfx/binary/%03i_B2", i), RSF_DEFAULT);
		cgs.media.binaryShaders[ i ].b3 = trap_R_RegisterShader(va("gfx/binary/%03i_B3", i), RSF_DEFAULT);
	}

	CG_BuildableStatusParse( "ui/assets/human/buildstat.cfg", &cgs.humanBuildStat );
	CG_BuildableStatusParse( "ui/assets/alien/buildstat.cfg", &cgs.alienBuildStat );

	cgs.media.beaconIconArrow = trap_R_RegisterShader( "gfx/feedback/beacons/arrow", (RegisterShaderFlags_t) ( RSF_NOMIP ) );
	cgs.media.beaconNoTarget = trap_R_RegisterShader( "gfx/feedback/beacons/no-target", (RegisterShaderFlags_t) ( RSF_NOMIP ) );
	cgs.media.beaconTagScore = trap_R_RegisterShader( "gfx/feedback/beacons/tagscore", (RegisterShaderFlags_t) ( RSF_NOMIP ) );

	cgs.media.damageIndicatorFont = trap_R_RegisterShader( "gfx/feedback/damage/font", (RegisterShaderFlags_t) ( RSF_NOMIP ) );

	// register the inline models
	cgs.numInlineModels = CM_NumInlineModels();

	if ( cgs.numInlineModels > MAX_SUBMODELS )
	{
		Sys::Drop( "MAX_SUBMODELS (%d) exceeded by %d", MAX_SUBMODELS, cgs.numInlineModels - MAX_SUBMODELS );
	}

	for ( i = 1; i < cgs.numInlineModels; i++ )
	{
		char   name[ 10 ];
		vec3_t mins, maxs;
		int    j;

		Com_sprintf( name, sizeof( name ), "*%i", i );

		cgs.inlineDrawModel[ i ] = trap_R_RegisterModel( name );
		trap_R_ModelBounds( cgs.inlineDrawModel[ i ], mins, maxs );

		for ( j = 0; j < 3; j++ )
		{
			cgs.inlineModelMidpoints[ i ][ j ] = mins[ j ] + 0.5 * ( maxs[ j ] - mins[ j ] );
		}
	}

	// register all the server specified models
	for ( i = 1; i < MAX_MODELS; i++ )
	{
		const char *modelName;

		modelName = CG_ConfigString( CS_MODELS + i );

		if ( !modelName[ 0 ] )
		{
			break;
		}

		cgs.gameModels[ i ] = trap_R_RegisterModel( modelName );
	}

	CG_UpdateMediaFraction( 0.4f );

	// register all the server specified shaders
	for ( i = 1; i < MAX_GAME_SHADERS; i++ )
	{
		const char *shaderName;

		shaderName = CG_ConfigString( CS_SHADERS + i );

		if ( !shaderName[ 0 ] )
		{
			break;
		}

		cgs.gameShaders[ i ] = trap_R_RegisterShader(shaderName, RSF_DEFAULT);
	}

	CG_UpdateMediaFraction( 0.6f );

	// register all the server specified grading textures
	// starting with the world wide one
	for ( i = 0; i < MAX_GRADING_TEXTURES; i++ )
	{
		CG_RegisterGrading( i, CG_ConfigString( CS_GRADING_TEXTURES + i ) );
	}

	// register all the server specified reverb effects
	// starting with the world wide one
	for ( i = 0; i < MAX_REVERB_EFFECTS; i++ )
	{
		CG_RegisterReverb( i, CG_ConfigString( CS_REVERB_EFFECTS + i ) );
	}

	CG_UpdateMediaFraction( 0.8f );

	// register all the server specified particle systems
	for ( i = 1; i < MAX_GAME_PARTICLE_SYSTEMS; i++ )
	{
		const char *psName;

		psName = CG_ConfigString( CS_PARTICLE_SYSTEMS + i );

		if ( !psName[ 0 ] )
		{
			break;
		}

		cgs.gameParticleSystems[ i ] = CG_RegisterParticleSystem( ( char * ) psName );
	}

	CG_UpdateMediaFraction( 1.0f );
}

/*
=======================
CG_BuildSpectatorString

=======================
*/
void CG_BuildSpectatorString()
{
	int i;

	cg.spectatorList[ 0 ] = 0;

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( cgs.clientinfo[ i ].infoValid && cgs.clientinfo[ i ].team == TEAM_NONE )
		{
			Q_strcat( cg.spectatorList, sizeof( cg.spectatorList ),
			          va( "^*%s     ", cgs.clientinfo[ i ].name ) );
		}
	}
}

/*
===================
CG_RegisterClients

===================
*/
static void CG_RegisterClients()
{
	int i;

	//precache all the models/sounds/etc
	if ( !cg_lazyLoadModels.Get() )
	{
		for ( i = PCL_NONE + 1; i < PCL_NUM_CLASSES; i++ )
		{
			CG_PrecacheClientInfo( (class_t) i, BG_ClassModelConfig( i )->modelName,
			                       BG_ClassModelConfig( i )->skinName );

			cg.characterLoadingFraction = ( float ) i / ( float ) PCL_NUM_CLASSES;
			trap_UpdateScreen();
		}
	}

	// Borrow these variables for MD5 models so we don't have to create new ones.
	cgs.media.larmourHeadSkin = trap_R_RegisterSkin( "models/players/human_base/body_helmet.skin" );
	cgs.media.larmourLegsSkin = trap_R_RegisterSkin( "models/players/human_base/body_larmour.skin" );
	cgs.media.larmourTorsoSkin = trap_R_RegisterSkin( "models/players/human_base/body_helmetlarmour.skin" );

	cgs.media.jetpackModel = trap_R_RegisterModel( "models/players/human_base/jetpack.iqm" );
	cgs.media.radarModel = trap_R_RegisterModel( "models/players/human_base/battpack.md3" ); // HACK: Use old battpack

	CG_RegisterWeaponAnimation(
	    &cgs.media.jetpackAnims[ JANIM_NONE ],
	    "models/players/human_base/jetpack.iqm:idle",
	    false, false, false );


	CG_RegisterWeaponAnimation(
	    &cgs.media.jetpackAnims[ JANIM_SLIDEOUT ],
	    "models/players/human_base/jetpack.iqm:slideout",
	    false, false, false );

	CG_RegisterWeaponAnimation(
	    &cgs.media.jetpackAnims[ JANIM_SLIDEIN ],
	    "models/players/human_base/jetpack.iqm:slidein",
	    false, false, false );

	//load all the clientinfos of clients already connected to the server
	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		const char *clientInfo;

		clientInfo = CG_ConfigString( CS_PLAYERS + i );

		if ( !clientInfo[ 0 ] )
		{
			continue;
		}

		CG_NewClientInfo( i );
	}

	CG_BuildSpectatorString();
}

//===========================================================================

/*
=================
CG_ConfigString
=================
*/
const char *CG_ConfigString( int index )
{
	if ( index < 0 || index >= MAX_CONFIGSTRINGS )
	{
		Sys::Drop( "CG_ConfigString: bad index: %i", index );
	}

	return cgs.gameState[index].c_str();
}

//==================================================================

/*
======================
CG_StartMusic

======================
*/
void CG_StartMusic()
{
	const char *s;
	char parm1[ MAX_QPATH ], parm2[ MAX_QPATH ];

	// start the background music
	s = ( char * ) CG_ConfigString( CS_MUSIC );
	Q_strncpyz( parm1, COM_Parse( &s ), sizeof( parm1 ) );
	Q_strncpyz( parm2, COM_Parse( &s ), sizeof( parm2 ) );

	trap_S_StartBackgroundTrack( parm1, parm2 );
}

bool CG_ClientIsReady( int clientNum )
{
	clientList_t ready;

	Com_ClientListParse( &ready, CG_ConfigString( CS_CLIENTS_READY ) );

	return Com_ClientListContains( &ready, clientNum );
}

// Generate a navmesh file for each class that doesn't already have
// an existing and (according to the header) up-to-date navmesh.
// Navmesh generation uses a separate progress bar counter from the main loading bar.
// Generating navmeshes in the cgame is kind of stupid, but the engine
// is not set up to handle long loading times in the sgame.
static void GenerateNavmeshes()
{
	std::string mapName = Cvar::GetValue( "mapname" );
	std::vector<class_t> missing;
	NavgenConfig config = ReadNavgenConfig( mapName );
	bool reduceTypes;
	if ( !Cvar::ParseCvarValue( Cvar::GetValue( "g_bot_navmeshReduceTypes" ), reduceTypes ) )
	{
		ASSERT_UNREACHABLE(); // sgame should have been loaded and this cvar initialized
	}

	for ( class_t species : RequiredNavmeshes( reduceTypes ) )
	{
		fileHandle_t f;
		std::string filename = NavmeshFilename( mapName, species );
		if ( BG_FOpenGameOrPakPath( filename, f ) < 0 )
		{
			missing.push_back( species );
			continue;
		}
		NavMeshSetHeader header;
		std::string error = GetNavmeshHeader( f, config, header, mapName );
		if ( !error.empty() )
		{
			Log::Notice( "Existing navmesh file %s can't be used: %s", filename, error );
			missing.push_back( species );
		}
		trap_FS_FCloseFile( f );
	}
	if ( missing.empty() )
	{
		return;
	}

	const std::string message = _("Generating bot navigation meshes");
	cg.navmeshLoadingFraction = 0;
	cg.loadingNavmesh = true;
	Q_strncpyz( cg.loadingText, message.c_str(), sizeof(cg.loadingText) );
	trap_UpdateScreen();

	NavmeshGenerator navgen;
	navgen.Init( mapName );
	float classesCompleted = 0.3; // Assume that Init() is 0.3 times as much work as generating 1 species
	// and assume that each species takes the same amount of time, which is actually completely wrong:
	// smaller ones take much longer
	float classesTotal = classesCompleted + missing.size();
	for ( class_t species : missing )
	{
		std::string message2 = Str::Format( "%s — %s", message, BG_ClassModelConfig( species )->humanName );
		Q_strncpyz( cg.loadingText, message2.c_str(), sizeof(cg.loadingText) );
		cg.loadingFraction = classesCompleted / classesTotal;
		trap_UpdateScreen();

		navgen.StartGeneration( species );
		do
		{
			float fraction = ( classesCompleted + navgen.SpeciesFractionCompleted() ) / classesTotal;
			if ( fraction - cg.navmeshLoadingFraction > 0.01 )
			{
				cg.navmeshLoadingFraction = fraction;
				trap_UpdateScreen();
			}
		} while ( !navgen.Step() );
		++classesCompleted;
	}
	cg.loadingNavmesh = false;
}

static void SetMapInfoFromArenaInfo( arenaInfo_t arenaInfo )
{
	cg.mapLongName = arenaInfo.longName;

	if ( !arenaInfo.authors.empty() )
	{
		std::string authorNames;

		for ( std::string &author: arenaInfo.authors )
		{
			if ( author == arenaInfo.authors.front() )
			{
				authorNames = author;
			}
			else if ( author == arenaInfo.authors.back() )
			{
				authorNames = Str::Format( "%s and %s", authorNames, author );
			}
			else
			{
				authorNames = Str::Format( "%s, %s", authorNames, author );
			}
		}

		cg.mapAuthors = Str::Format( _( "by %s"), authorNames );
	}
}

static void SetMapInfo()
{
	/* When SetMapInfo() is called,

	- cgs.mapname may not work because it is not set yet.
	- Cvar::GetValue( "mapname" ) may not work because
	  only client used as a server has it set. */

	std::string mapName = Info_ValueForKey( CG_ConfigString( CS_SERVERINFO ), "mapname" );

	/* Use longname from .arena file as map name if exists,
	also read author name if exists. */
	CG_LoadArenas( mapName );

	arenaInfo_t arenaInfo = CG_GetArenaInfo( mapName);

	if ( arenaInfo.longName.empty() )
	{
		/* Use worldspawn's message key from .bsp file
		as a fallback if no other name is found.

		This may be not optimal: the server reads it from
		the BSP and the client reads it from the network. */
		const char *message = CG_ConfigString( CS_MESSAGE );

		if ( strlen( message ) != 0 ) {
			arenaInfo.longName = message;
		}
	}

	if ( arenaInfo.longName.empty() )
	{
		/* Use map basename for .bsp name as a fallback if
		not other name is found. */
		arenaInfo.longName = mapName;
	}

	SetMapInfoFromArenaInfo( arenaInfo );
}

/*
=================
CG_Init

Called after every level change or subsystem restart
Will perform callbacks to make the loading info screen update.
=================
*/

void CG_Init( int serverMessageNum, int clientNum, const glconfig_t& gl, const GameStateCSs& gameState)
{
	const char *s;

	bool fullscreen;
	if ( Cvar::ParseCvarValue( Cvar::GetValue( "r_fullscreen" ), fullscreen ) && !fullscreen )
	{
		// ungrab the mouse while loading in windowed mode
		trap_SetMouseMode( MouseMode::SystemCursor );
	}

	CG_UpdateLoadingStep( LOAD_START );

	// get the rendering configuration from the client system
	cgs.glconfig = gl;
	cgs.screenXScale = cgs.glconfig.vidWidth / 640.0f;
	cgs.screenYScale = cgs.glconfig.vidHeight / 480.0f;
	cgs.aspectScale = ( ( 640.0f * cgs.glconfig.vidHeight ) /
	( 480.0f * cgs.glconfig.vidWidth ) );
	// cg_shadows is latched so we can get it once at the beginning
	cg_shadows = Util::enum_cast<shadowingMode_t>( trap_Cvar_VariableIntegerValue( "cg_shadows" ) );

	// load a few needed things before we do any screen updates
	trap_R_SetAltShaderTokens( "unpowered,destroyed,idle,idle2" );
	cgs.media.whiteShader = trap_R_RegisterShader("gfx/colors/white", RSF_DEFAULT);
	cgs.media.outlineShader = trap_R_RegisterShader("gfx/outline", RSF_DEFAULT);

	// old servers
	cgs.gameState = gameState;

	/* Set level name and authors string to be used by the UI.
	It must be done after cgs.gameState is set or the map name
	from the BSP can't be fetched. */
	SetMapInfo();

	// Must be done before trap_UpdateScreen()
	// or the game will crash.
	cgs.processedSnapshotNum = serverMessageNum;

	// It was too early to update screen at LOAD_START time,
	// let's do it now.
	trap_UpdateScreen();

	// Set up the pmove params with sensible default values, the server params will
	// be communicated with the "pmove_params" server commands.
	// These values are the same as the default values of the servers to preserve
	// compatibility with official Alpha 34 servers, but shouldn't be necessary anymore for Alpha 35
	cg.pmoveParams.fixed = cg.pmoveParams.synchronous = 0;
	cg.pmoveParams.accurate = 1;
	cg.pmoveParams.msec = 8;

	cg.clientNum = clientNum;

	// Initialize item locking state
	BG_InitUnlockackables();

	CG_InitConsoleCommands();
	trap_S_BeginRegistration();

	cg.weaponSelect = WP_NONE;

	// copy vote display strings so they don't show up blank if we see
	// the same one directly after connecting
	Q_strncpyz( cgs.voteString[ TEAM_NONE ],
	            CG_ConfigString( CS_VOTE_STRING + TEAM_NONE ),
	            sizeof( cgs.voteString ) );
	Q_strncpyz( cgs.voteString[ TEAM_ALIENS ],
	            CG_ConfigString( CS_VOTE_STRING + TEAM_ALIENS ),
	            sizeof( cgs.voteString[ TEAM_ALIENS ] ) );
	Q_strncpyz( cgs.voteString[ TEAM_HUMANS ],
	            CG_ConfigString( CS_VOTE_STRING + TEAM_HUMANS ),
	            sizeof( cgs.voteString[ TEAM_HUMANS ] ) );

	// check version
	s = CG_ConfigString( CS_GAME_VERSION );
	//   if( strcmp( s, GAME_VERSION ) )
	//     Sys::Drop( "Client/Server game mismatch: %s/%s", GAME_VERSION, s );

	s = CG_ConfigString( CS_LEVEL_START_TIME );
	cgs.levelStartTime = atoi( s );

	CG_SetMapNameFromServerinfo();

	// load the new map
	CM_LoadMap(cgs.mapname);

	CG_InitMinimap();

	CG_UpdateLoadingStep( LOAD_TRAILS );
	CG_LoadTrailSystems();

	CG_UpdateLoadingStep( LOAD_PARTICLES );
	CG_LoadParticleSystems();

	// load configs after initializing particles and trails since it registers some
	CG_UpdateLoadingStep( LOAD_CONFIGS );
	BG_InitAllConfigs();
	// parse the serverinfo only now because infos such as
	// disabledEquipment wouldn't be parsed correctly before
	// loading the configs
	CG_ParseServerinfo();

	CG_UpdateLoadingStep( LOAD_SOUNDS );
	CG_RegisterSounds();

	cgs.voices = BG_VoiceInit();
	BG_PrintVoices( cgs.voices, cg_debugVoices.Get() );

	// It updates loading steps by itself.
	// LOAD_GEOMETRY
	// LOAD_ASSETS
	CG_RegisterGraphics();

	// load weapons upgrades and buildings after configs
	CG_UpdateLoadingStep( LOAD_WEAPONS );
	CG_InitMarkPolys();
	CG_InitWeapons();

	CG_UpdateLoadingStep( LOAD_UPGRADES );
	CG_InitUpgrades();

	CG_UpdateLoadingStep( LOAD_BUILDINGS );
	CG_InitBuildables();

	CG_UpdateLoadingStep( LOAD_CLIENTS );
	CG_InitClasses();
	CG_RegisterClients(); // if low on memory, some clients will be deferred

	CG_UpdateLoadingStep( LOAD_HUDS );
	CG_Rocket_LoadHuds();
	CG_LoadBeaconsConfig();

	CG_UpdateLoadingStep( LOAD_GLSL );
	trap_S_EndRegistration();

	if ( cg_navgenOnLoad.Get() && Cvar::GetValue( "sv_running" ) == "1" )
	{
		CG_UpdateLoadingStep( LOAD_CHECK_NAVMESH );
		GenerateNavmeshes();
	}

	if ( trap_Cvar_VariableIntegerValue( "r_lazyShaders" ) == 1 )
	{
		// EndRegistration is called after cgame initialization returns
		CG_UpdateLoadingStep( LOAD_GLSL );
	}
	else
	{
		CG_UpdateLoadingStep( LOAD_DONE );
	}

	// Make sure we have update values (scores)
	CG_SetConfigValues();

	CG_ShaderStateChanged();

	trap_Cvar_Set( "ui_winner", "" ); // Clear the previous round's winner.

	// Request server to resend pmoveParams.
	trap_SendClientCommand( "client_ready" );
}

/*
=================
CG_Shutdown

Called before every level change or subsystem restart
=================
*/
void CG_Shutdown()
{
	// some mods may need to do cleanup work here,
	// like closing files or archiving session data
	CG_Rocket_CleanUpDataSources();
	Rocket_Shutdown();
	BG_UnloadAllConfigs();
	Parse_FreeGlobalDefines();
}

const vec3_t cg_shaderColors[ SHC_NUM_SHADER_COLORS ] =
{
	{ 0.0f,   0.0f,     0.75f    }, // dark blue
	{ 0.3f,   0.35f,    0.625f   }, // light blue
	{ 0.0f,   0.625f,   0.563f   }, // green-cyan
	{ 0.313f, 0.0f,     0.625f   }, // violet
	{ 0.54f,  0.0f,     1.0f     }, // indigo
	{ 0.625f, 0.625f,   0.0f     }, // yellow
	{ 0.875f, 0.313f,   0.0f     }, // orange
	{ 0.375f, 0.625f,   0.375f   }, // light green
	{ 0.0f,   0.438f,   0.0f     }, // dark green
	{ 1.0f,   0.0f,     0.0f     }, // red
	{ 0.625f, 0.375f,   0.4f     }, // pink
	{ 0.313f, 0.313f,   0.313f   } // grey
};
