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

#define SOUNDEVENT( sound ) trap_S_StartLocalSound( sound, CHAN_LOCAL_SOUND )

#define SOUND_SELECT SOUNDEVENT( cgs.media.sndLimboSelect )
#define SOUND_FOCUS  SOUNDEVENT( cgs.media.sndLimboFocus )
#define SOUND_FILTER SOUNDEVENT( cgs.media.sndLimboFilter )
#define SOUND_CANCEL SOUNDEVENT( cgs.media.sndLimboCancel )

void                CG_DrawBorder ( float x, float y, float w, float h, qboolean fill, qboolean drawMouseOver );

team_t              teamOrder[ 3 ] =
{
	TEAM_ALLIES,
	TEAM_AXIS,
	TEAM_SPECTATOR,
};

panel_button_text_t nameEditFont =
{
	0.22f,                   0.24f,
	{ 1.f,                   1.f,  1.f, 0.8f },
	ITEM_TEXTSTYLE_SHADOWED, 0,
	&cgs.media.limboFont2,
};

panel_button_text_t classBarFont =
{
	0.22f,                 0.24f,
	{ 0.f,                 0.f,  0.f, 0.8f },
	0,                     0,
	&cgs.media.limboFont2,
};

panel_button_text_t titleLimboFont =
{
	0.24f,                 0.28f,
	{ 1.f,                 1.f,  1.f, 0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_text_t titleLimboFontBig =
{
	0.3f,                  0.3f,
	{ 1.f,                 1.f, 1.f,  0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_text_t titleLimboFontBigCenter =
{
	0.3f,                  0.3f,
	{ 1.f,                 1.f,              1.f,  0.6f },
	0,                     ITEM_ALIGN_CENTER,
	&cgs.media.limboFont1,
};

panel_button_text_t spawnLimboFont =
{
	0.18f,                 0.22f,
	{ 1.f,                 1.f,  1.f, 0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_text_t weaponButtonFont =
{
	0.33f,                 0.33f,
	{ 0.f,                 0.f,  0.f, 0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_text_t weaponPanelNameFont =
{
	0.20f,                 0.24f,
	{ 1.0f,                1.0f, 1.0f,  0.4f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_text_t weaponPanelFilterFont =
{
	0.17f,                    0.17f,
	{ 1.0f,                   1.0f, 1.0f,  0.6f },
	0,                        0,
	&cgs.media.limboFont1_lo,
};

panel_button_text_t weaponPanelStatsFont =
{
	0.15f,                    0.17f,
	{ 1.0f,                   1.0f, 1.0f,  0.6f },
	0,                        0,
	&cgs.media.limboFont1_lo,
};

panel_button_text_t weaponPanelStatsPercFont =
{
	0.2f,                  0.2f,
	{ 1.0f,                1.0f, 1.0f,    0.6f },
	0,                     0,
	&cgs.media.limboFont1,
};

panel_button_text_t objectivePanelTxt =
{
	0.2f,                  0.2f,
	{ 0.0f,                0.0f, 0.0f,    0.5f },
	0,                     0,
	&cgs.media.limboFont2,
};

panel_button_t      rightLimboPannel =
{
	"gfx/limbo/limbo_back",
	NULL,
	{ 440,                    0,  200, 480 },
	{ 0,                      0,  0,   0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Img,
	NULL,
};

#define MEDAL_PIC_GAP   ( ( MEDAL_PIC_SIZE - ( MEDAL_PIC_WIDTH * MEDAL_PIC_COUNT ) ) / ( MEDAL_PIC_COUNT + 1.f ) )
#define MEDAL_PIC_COUNT 7.f
#define MEDAL_PIC_WIDTH 22.f
#define MEDAL_PIC_X     450.f
#define MEDAL_PIC_SIZE  ( 630.f - MEDAL_PIC_X )
#define MEDAL_PIC( number  )                            \
        panel_button_t medalPic ## number = {             \
    NULL,                                                                   \
    NULL,                                                                   \
    { MEDAL_PIC_X + MEDAL_PIC_GAP + ( number * ( MEDAL_PIC_GAP + MEDAL_PIC_WIDTH ) ),119,                                                                             MEDAL_PIC_WIDTH, 26 }, \
    { number,                  0,                                                                               0,               0, 0, 0, 0, 0},                \
    NULL, /* font    */                              \
    NULL, /* keyDown */                                      \
    NULL, /* keyUp */                                      \
    CG_LimboPanel_RenderMedal,                              \
    NULL,                                                                   \
  }

MEDAL_PIC ( 0 );
MEDAL_PIC ( 1 );
MEDAL_PIC ( 2 );
MEDAL_PIC ( 3 );
MEDAL_PIC ( 4 );
MEDAL_PIC ( 5 );
MEDAL_PIC ( 6 );

#define TEAM_COUNTER_GAP         ( ( TEAM_COUNTER_SIZE - ( TEAM_COUNTER_WIDTH * TEAM_COUNTER_COUNT ) ) / ( TEAM_COUNTER_COUNT + 1.f ) )
#define TEAM_COUNTER_COUNT       3.f
#define TEAM_COUNTER_WIDTH       20.f
#define TEAM_COUNTER_X           432.f
#define TEAM_COUNTER_SIZE        ( 660.f - TEAM_COUNTER_X )
#define TEAM_COUNTER_BUTTON_DIFF -24.f
#define TEAM_COUNTER_SPACING     4.f

#define TEAM_COUNTER( number  )                         \
        panel_button_t teamCounter ## number = {          \
    NULL,                                                                   \
    NULL,                                                                   \
    { TEAM_COUNTER_X + TEAM_COUNTER_GAP + ( number * ( TEAM_COUNTER_GAP + TEAM_COUNTER_WIDTH ) ),236,                                                                                         TEAM_COUNTER_WIDTH, 14 },  \
    { 1,                         number,                                                                                      0,                  0, 0, 0, 0, 0},                \
    NULL, /* font    */                              \
    NULL, /* keyDown */                                      \
    NULL, /* keyUp */                                      \
    CG_LimboPanel_RenderCounter,                    \
    NULL,                                                                   \
  };                                                                                      \
        panel_button_t teamCounterLight ## number = { \
    NULL,                                                                   \
    NULL,                                                                   \
    { TEAM_COUNTER_X + TEAM_COUNTER_GAP + ( number * ( TEAM_COUNTER_GAP + TEAM_COUNTER_WIDTH ) ) - 20,236,                                                                                              16, 16 }, \
    { 1,                       number,                                                                                           0,  0, 0, 0, 0, 0},                \
    NULL, /* font    */                              \
    NULL, /* keyDown */                                      \
    NULL, /* keyUp */                                      \
    CG_LimboPanel_RenderLight,                              \
    NULL,                                                                   \
  };                                                                                      \
        panel_button_t teamButton ## number = {           \
    NULL,                                                                   \
    NULL,                                                                   \
    { TEAM_COUNTER_X + TEAM_COUNTER_GAP + ( number * ( TEAM_COUNTER_GAP + TEAM_COUNTER_WIDTH ) + ( TEAM_COUNTER_BUTTON_DIFF / 2.f ) ) - 17 + TEAM_COUNTER_SPACING, \
      188 + TEAM_COUNTER_SPACING, \
      TEAM_COUNTER_WIDTH - TEAM_COUNTER_BUTTON_DIFF + 20 - 2 * TEAM_COUNTER_SPACING, \
      44 - 2 * TEAM_COUNTER_SPACING },      \
    { number,                                                                        0,0, 0, 0, 0, 0, 0 },                \
    NULL, /* font    */                              \
    CG_LimboPanel_TeamButton_KeyDown, /* keyDown */ \
    NULL, /* keyUp */                                      \
    CG_LimboPanel_RenderTeamButton,                 \
    NULL,                                                                   \
  }

TEAM_COUNTER ( 0 );
TEAM_COUNTER ( 1 );
TEAM_COUNTER ( 2 );

#define CLASS_COUNTER_GAP         ( ( CLASS_COUNTER_SIZE - ( CLASS_COUNTER_WIDTH * CLASS_COUNTER_COUNT ) ) / ( CLASS_COUNTER_COUNT + 1.f ) )
#define CLASS_COUNTER_COUNT       5.f
#define CLASS_COUNTER_WIDTH       20.f
#define CLASS_COUNTER_X           435.f
#define CLASS_COUNTER_SIZE        ( 645.f - CLASS_COUNTER_X )
#define CLASS_COUNTER_LIGHT_DIFF  4.f
#define CLASS_COUNTER_BUTTON_DIFF -18.f
#define CLASS_COUNTER( number  )                        \
        panel_button_t classCounter ## number = {         \
    NULL,                                                                   \
    NULL,                                                                   \
    { CLASS_COUNTER_X + CLASS_COUNTER_GAP + ( number * ( CLASS_COUNTER_GAP + CLASS_COUNTER_WIDTH ) ),302,                                                                                             CLASS_COUNTER_WIDTH, 14 }, \
    { 0,                         number,                                                                                          0,                   0, 0, 0, 0, 0},                \
    NULL, /* font    */                              \
    NULL, /* keyDown */                                      \
    NULL, /* keyUp */                                      \
    CG_LimboPanel_RenderCounter,                    \
    NULL,                                                                   \
  };                                                                                      \
        panel_button_t classButton ## number = {          \
    NULL,                                                                   \
    NULL,                                                                   \
    { CLASS_COUNTER_X + CLASS_COUNTER_GAP + ( number * ( CLASS_COUNTER_GAP + CLASS_COUNTER_WIDTH ) ) + ( CLASS_COUNTER_BUTTON_DIFF / 2.f ),266,                                                                                                                                   CLASS_COUNTER_WIDTH - CLASS_COUNTER_BUTTON_DIFF, 34 },       \
    { 0,                             number,                                                                                                                                0,                                               0, 0, 0, 0, 0},                \
    NULL, /* font    */                              \
    CG_LimboPanel_ClassButton_KeyDown, /* keyDown */      \
    NULL, /* keyUp */                                      \
    CG_LimboPanel_RenderClassButton,                \
    NULL,                                                                   \
  }

/*panel_button_t classCounterLight##number = { \
        NULL, \
        NULL, \
        { CLASS_COUNTER_X + CLASS_COUNTER_GAP + (number*(CLASS_COUNTER_GAP + CLASS_COUNTER_WIDTH)) + (CLASS_COUNTER_LIGHT_DIFF/2.f), 266, CLASS_COUNTER_WIDTH - CLASS_COUNTER_LIGHT_DIFF, 16 }, \
        { 0, number, 0, 0, 0, 0, 0, 0 }, \
        NULL, \
        CG_LimboPanel_ClassButton_KeyDown, \
        NULL, \
        CG_LimboPanel_RenderLight, \
        NULL, \
};                      \*/

panel_button_t  classBar =
{
	"gfx/limbo/lightup_bar",
	NULL,
	{ 470,                    320, 140, 20 },
	{ 0,                      0,  0,   0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Img,
	NULL,
};

panel_button_t         classBarText =
{
	NULL,
	NULL,
	{ 460,                      334,   160, 16 },
	{ 0,                        0,     0,   0, 0, 0, 0, 0},
	&classBarFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_ClassBar_Draw,
	NULL,
};

CLASS_COUNTER ( 0 );
CLASS_COUNTER ( 1 );
CLASS_COUNTER ( 2 );
CLASS_COUNTER ( 3 );
CLASS_COUNTER ( 4 );

#define FILTER_BUTTON( number ) \
        panel_button_t filterButton ## number = { \
    NULL,                                                           \
    NULL,                                                           \
    { 15,                      54 + ( number * 31 ), 26, 26 },     \
    { number,                  0,                    0,  0, 0, 0, 0, 0},        \
    NULL, /* font   */                      \
    CG_LimboPanel_Filter_KeyDown, /* keyDown  */                              \
    NULL, /* keyUp  */                              \
    CG_LimboPanel_Filter_Draw,                      \
    NULL,                                                           \
  }

FILTER_BUTTON ( 0 );
FILTER_BUTTON ( 1 );
FILTER_BUTTON ( 2 );
FILTER_BUTTON ( 3 );
FILTER_BUTTON ( 4 );
FILTER_BUTTON ( 5 );
FILTER_BUTTON ( 6 );
FILTER_BUTTON ( 7 );

panel_button_t filterTitleText =
{
	NULL,
	"FILTERS",
	{ 8,                       36, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&weaponPanelFilterFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

#define LEFT_FRAME( shader, number, x, y, w, h ) \
        panel_button_t leftFrame0 ## number = {   \
    shader,                                                         \
    NULL,                                                           \
    { x,                       y, w, h },                                         \
    { 0,                       0, 0, 0, 0, 0, 0, 0},                     \
    NULL, /* font    */                      \
    NULL, /* keyDown */                              \
    NULL, /* keyUp */                              \
    BG_PanelButtonsRender_Img,                      \
    NULL,                                                           \
  }

#define LF_X1 64
#define LF_X2 416
#define LF_X3 440

#define LF_W1 ( LF_X1 - 0 )
#define LF_W2 ( LF_X2 - LF_X1 )
#define LF_W3 ( LF_X3 - LF_X2 )

#define LF_Y1 23
#define LF_Y2 375
#define LF_Y3 480

#define LF_H1 ( LF_Y1 - 0 )
#define LF_H2 ( LF_Y2 - LF_Y1 )
#define LF_H3 ( LF_Y3 - LF_Y2 )

LEFT_FRAME ( "gfx/limbo/limbo_frame01", 1, 0, 0, LF_W1, LF_H1 );
LEFT_FRAME ( "gfx/limbo/limbo_frame02", 2, LF_X1, 0, LF_W2, LF_H1 );
LEFT_FRAME ( "gfx/limbo/limbo_frame03", 3, LF_X2, 0, LF_W3, LF_H1 );

LEFT_FRAME ( "gfx/limbo/limbo_frame04", 4, LF_X2, LF_Y1, LF_W3, LF_H2 );

LEFT_FRAME ( "gfx/limbo/limbo_frame05", 5, LF_X2, LF_Y2, LF_W3, LF_H3 );
LEFT_FRAME ( "gfx/limbo/limbo_frame06", 6, LF_X1, LF_Y2, LF_W2, LF_H3 );
LEFT_FRAME ( "gfx/limbo/limbo_frame07", 7, 0, LF_Y2, LF_W1, LF_H3 );

LEFT_FRAME ( "gfx/limbo/limbo_frame08", 8, 0, LF_Y1, LF_W1, LF_H2 );

panel_button_t playerLimboHead =
{
	NULL,
	NULL,
	{ 456,                   30,   68, 84 },
	{ 0,                     0,    0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderHead,
	NULL,
};

panel_button_t playerXPCounterText =
{
	NULL,
	"XP",
	{ 546,                        108,   60, 16 },
	{ 2,                          0,     0,  0, 0, 0, 0, 0},
	&spawnLimboFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanelRenderText_NoLMS,
	NULL,
};

panel_button_t playerXPCounter =
{
	NULL,
	NULL,
	{ 564,                      96,   60, 16 },
	{ 2,                        0,    0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderCounter,
	NULL,
};

panel_button_t playerSkillCounter0 =
{
	NULL,
	NULL,
	{ 552,                      36,   60, 16 },
	{ 4,                        0,    0,  0, 0, 0, 4, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderCounter,
	NULL,
};

panel_button_t playerSkillCounter1 =
{
	NULL,
	NULL,
	{ 552,                      56,   60, 16 },
	{ 4,                        1,    0,  0, 0, 0, 4, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderCounter,
	NULL,
};

panel_button_t playerSkillCounter2 =
{
	NULL,
	NULL,
	{ 552,                      76,   60, 16 },
	{ 4,                        2,    0,  0, 0, 0, 4, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderCounter,
	NULL,
};

panel_button_t playerSkillIcon0 =
{
	NULL,
	NULL,
	{ 532,                        36,   16, 16 },
	{ 0,                          0,    0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderSkillIcon,
	NULL,
};

panel_button_t playerSkillIcon1 =
{
	NULL,
	NULL,
	{ 532,                        56,   16, 16 },
	{ 1,                          0,    0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderSkillIcon,
	NULL,
};

panel_button_t playerSkillIcon2 =
{
	NULL,
	NULL,
	{ 532,                        76,   16, 16 },
	{ 2,                          0,    0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderSkillIcon,
	NULL,
};

// =======================

panel_button_t mapTimeCounter =
{
	NULL,
	NULL,
	{ 276,                      5,   20, 14 },
	{ 5,                        0,   0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderCounter,
	NULL,
};

panel_button_t mapTimeCounter2 =
{
	NULL,
	NULL,
	{ 252,                      5,   20, 14 },
	{ 5,                        1,   0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderCounter,
	NULL,
};

panel_button_t mapTimeCounterText =
{
	NULL,
	"MISSION TIME",
	{ 172,                     16, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&spawnLimboFont, /* font        */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

// =======================

panel_button_t respawnCounter =
{
	NULL,
	NULL,
	{ 400,                      5,   20, 14 },
	{ 3,                        0,   0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderCounter,
	NULL,
};

panel_button_t respawnCounterText =
{
	NULL,
	"REINFORCEMENTS",
	{ 300,                     16, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&spawnLimboFont, /* font        */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

// =======================

panel_button_t limboTitleText =
{
	NULL,
	"COMMAND MAP",
	{ 8,                       16, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&titleLimboFont, /* font        */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t playerSetupText =
{
	NULL,
	"PLAYER SETUP",
	{ 448,                     16, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&titleLimboFont, /* font        */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t skillsText =
{
	NULL,
	"SKILLS",
	{ 532,                             32, 0, 0 },
	{ 0,                               0,  0, 0, 0, 0, 0, 0},
	&weaponPanelStatsFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanelRenderText_SkillsText,
	NULL,
};

// =======================

panel_button_t weaponPanel =
{
	NULL,
	NULL,
	{ 455,                    353,   140, 56 },
	{ 0,                      0,     0,   0, 0, 0, 0, 0},
	NULL, /* font     */
	CG_LimboPanel_WeaponPanel_KeyDown, /* keyDown  */
	CG_LimboPanel_WeaponPanel_KeyUp, /* keyUp    */
	CG_LimboPanel_WeaponPanel,
	NULL,
};

panel_button_t weaponLight1 =
{
	NULL,
	NULL,
	{ 605,                     362,   20, 20 },
	{ 0,                       0,     0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	CG_LimboPanel_WeaponLights_KeyDown, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_WeaponLights,
	NULL,
};

panel_button_t weaponLight1Text =
{
	NULL,
	"1",
	{ 609,                     378,   0, 0 },
	{ 0,                       0,     0, 0, 0, 0, 0, 0},
	&weaponButtonFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t weaponLight2 =
{
	NULL,
	NULL,
	{ 605,                     386,   20, 20 },
	{ 1,                       0,     0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	CG_LimboPanel_WeaponLights_KeyDown, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_WeaponLights,
	NULL,
};

panel_button_t weaponLight2Text =
{
	NULL,
	"2",
	{ 609,                     402,   0, 0 },
	{ 0,                       0,     0, 0, 0, 0, 0, 0},
	&weaponButtonFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t weaponStatsShotsText =
{
	NULL,
	"SHOTS",
	{ 460,                     422, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&weaponPanelStatsFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t weaponStatsShotsCounter =
{
	NULL,
	NULL,
	{ 460,                      426,   40, 14 },
	{ 6,                        0,     0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderCounter,
	NULL,
};

panel_button_t weaponStatsHitsText =
{
	NULL,
	"HITS",
	{ 516,                     422, 0, 0 },
	{ 0,                       0,   0, 0, 0, 0, 0, 0},
	&weaponPanelStatsFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t weaponStatsHitsCounter =
{
	NULL,
	NULL,
	{ 516,                      426,   40, 14 },
	{ 6,                        1,     0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderCounter,
	NULL,
};

panel_button_t weaponStatsAccText =
{
	NULL,
	"ACC",
	{ 570,                     422,  0, 0 },
	{ 0,                       0,    0, 0, 0, 0, 0, 0},
	&weaponPanelStatsFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t weaponStatsAccCounter =
{
	NULL,
	NULL,
	{ 570,                      426,   30, 14 },
	{ 6,                        2,     0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderCounter,
	NULL,
};

panel_button_t weaponStatsAccPercentage =
{
	NULL,
	"%",
	{ 600,                     436,   0, 0 },
	{ 0,                       0,     0, 0, 0, 0, 0, 0},
	&weaponPanelStatsPercFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

// =======================

panel_button_t commandmapPanel =
{
	NULL,
	NULL,
	{ CC_2D_X,                     CC_2D_Y,       CC_2D_W, CC_2D_H },
	{ 0,                           0,             0,       0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderCommandMap,
	NULL,
};

// =======================

panel_button_t objectivePanel =
{
	NULL,
	NULL,
	{ 8,                              398, 240, 74 },
	{ 0,                              0,   0,   0, 0, 0, 0, 0},
	NULL, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderObjectiveBack,
	NULL,
};

panel_button_t objectivePanelText =
{
	NULL,
	NULL,
	{ 8,                              398, 240, 74 },
	{ 0,                              0,   0,   0, 0, 0, 0, 0},
	&objectivePanelTxt, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_RenderObjectiveText,
	NULL,
};

panel_button_t objectivePanelTitle =
{
	NULL,
	"OBJECTIVES",
	{ 8,                       392, 0, 0 },
	{ 0,                       0,  0, 0, 0, 0, 0, 0},
	&titleLimboFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t objectivePanelButtonUp =
{
	"gfx/limbo/but_objective_up",
	NULL,
	{ 252,                       416, 24, 24 },
	{ 0,                         0,  0,  0, 0, 0, 0, 1},
	NULL, /* font     */
	CG_LimboPanel_ObjectiveText_KeyDown, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Img,
	NULL,
};

panel_button_t briefingButton =
{
	NULL,
	NULL,
	{ 252,                            388,   24, 24 },
	{ 0,                              0,     0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	CG_LimboPanel_BriefingButton_KeyDown, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_BriefingButton_Draw,
	NULL,
};

panel_button_t objectivePanelButtonDown =
{
	"gfx/limbo/but_objective_dn",
	NULL,
	{ 252,                       444, 24, 24 },
	{ 0,                         0,  0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	CG_LimboPanel_ObjectiveText_KeyDown, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Img,
	NULL,
};

// =======================

panel_button_t okButtonText =
{
	NULL,
	"OK",
	{ 484,                     469,   100, 40 },
	{ 0,                       0,     0,   0, 0, 0, 0, 0},
	&titleLimboFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t okButton =
{
	NULL,
	NULL,
	{ 454 + 2,                454 + 2,       82 - 4, 18 - 4 },
	{ 0,                      0,             0,      0, 0, 0, 0, 0},
	NULL, /* font     */
	CG_LimboPanel_OkButton_KeyDown, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_Border_Draw,
	NULL,
};

panel_button_t cancelButtonText =
{
	NULL,
	"CANCEL",
	{ 556,                     469, 100, 40 },
	{ 0,                       0,  0,   0, 0, 0, 0, 0},
	&titleLimboFont, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t cancelButton =
{
	NULL,
	NULL,
	{ 543 + 2,                454 + 2,       82 - 4, 18 - 4 },
	{ 0,                      0,             0,      0, 0, 0, 0, 0},
	NULL, /* font     */
	CG_LimboPanel_CancelButton_KeyDown, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_Border_Draw,
	NULL,
};

// =======================

panel_button_t nameEdit =
{
	NULL,
	"limboname",
	{ 480,                       150, 120, 20 },
	{ 0,                         0,  0,   0, 0, 0, 0, 0},
	&nameEditFont, /* font     */
	BG_PanelButton_EditClick, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButton_RenderEdit,
	CG_LimboPanel_NameEditFinish,
};

panel_button_t plusButton =
{
	NULL,
	NULL,
	{ 19,                     320,  18, 14 },
	{ 12,                     0,    0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	CG_LimboPanel_PlusButton_KeyDown, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_Border_Draw,
	NULL,
};

panel_button_t plusButtonText =
{
	NULL,
	"+",
	{ 19,                      321,  18, 14 },
	{ 12,                      0,    0,  0, 0, 0, 0, 0},
	&titleLimboFontBigCenter, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t minusButton =
{
	NULL,
	NULL,
	{ 19,                     346,  18, 14 },
	{ 12,                     0,    0,  0, 0, 0, 0, 0},
	NULL, /* font     */
	CG_LimboPanel_MinusButton_KeyDown, /* keyDown  */
	NULL, /* keyUp    */
	CG_LimboPanel_Border_Draw,
	NULL,
};

panel_button_t minusButtonText =
{
	NULL,
	"-",
	{ 19,                      346,  18, 14 },
	{ 12,                      0,    0,  0, 0, 0, 0, 0},
	&titleLimboFontBigCenter, /* font     */
	NULL, /* keyDown  */
	NULL, /* keyUp    */
	BG_PanelButtonsRender_Text,
	NULL,
};

panel_button_t *limboPanelButtons[] =
{
	&rightLimboPannel,

	&classCounter0,           &classCounter1,             &classCounter2,         &classCounter3, &classCounter4,
//  &classCounterLight0,    &classCounterLight1,    &classCounterLight2,    &classCounterLight3,    &classCounterLight4,
	&classButton0,            &classButton1,              &classButton2,          &classButton3,  &classButton4,

	&classBar,                &classBarText,

	&leftFrame01,             &leftFrame02,               &leftFrame03,           &leftFrame04,
	&leftFrame05,             &leftFrame06,               &leftFrame07,           &leftFrame08,

	&filterButton0,           &filterButton1,             &filterButton2,         &filterButton3, &filterButton4,
	&filterButton5,           &filterButton6,             &filterButton7, /* &filterButton8, */
	&filterTitleText,

	&medalPic0,               &medalPic1,                 &medalPic2,             &medalPic3,     &medalPic4,    &medalPic5, &medalPic6,

	&teamCounter0,            &teamCounter1,              &teamCounter2,
	&teamCounterLight0,       &teamCounterLight1,         &teamCounterLight2,
	&teamButton0,             &teamButton1,               &teamButton2,

	&playerLimboHead,
	&playerXPCounter,         &playerXPCounterText,

	&respawnCounter,          &respawnCounterText,
	&mapTimeCounter,          &mapTimeCounter2,           &mapTimeCounterText,

	&playerSkillCounter0,     &playerSkillCounter1,       &playerSkillCounter2,
	&playerSkillIcon0,        &playerSkillIcon1,          &playerSkillIcon2,

	&objectivePanel,          &objectivePanelTitle,       &objectivePanelText,
	&objectivePanelButtonUp,  &objectivePanelButtonDown,

	&limboTitleText,
	&playerSetupText,
	&skillsText,

	&commandmapPanel,

	&okButton,                &okButtonText,
	&cancelButton,            &cancelButtonText,

	&nameEdit,

	&weaponLight1,            &weaponLight2,
	&weaponLight1Text,        &weaponLight2Text,
	&weaponPanel,
	&weaponStatsShotsText,    &weaponStatsHitsText,       &weaponStatsAccText,
	&weaponStatsShotsCounter, &weaponStatsHitsCounter,    &weaponStatsAccCounter,
	&weaponStatsAccPercentage,

	&briefingButton,

	&plusButton,              &plusButtonText,
	&minusButton,             &minusButtonText,

	NULL,
};

qboolean CG_LimboPanel_BriefingButton_KeyDown ( panel_button_t *button, int key )
{
	if ( cg_gameType.integer == GT_WOLF_LMS )
	{
		return qfalse;
	}

	if ( key == K_MOUSE1 )
	{
		SOUND_SELECT;

		if ( cg.limboEndCinematicTime > cg.time )
		{
			trap_S_StopStreamingSound ( -1 );
			cg.limboEndCinematicTime = 0;

			return qtrue;
		}

		cg.limboEndCinematicTime = cg.time + CG_SoundPlaySoundScript ( va ( "news_%s", cgs.rawmapname ), NULL, -1, qfalse );

		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_BriefingButton_Draw ( panel_button_t *button )
{
	if ( cg_gameType.integer == GT_WOLF_LMS )
	{
		return;
	}

	if ( cg.limboEndCinematicTime > cg.time )
	{
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h,
		             BG_CursorInRect ( &button->rect ) ? cgs.media.limboBriefingButtonStopOn : cgs.media.limboBriefingButtonStopOff );
	}
	else
	{
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h,
		             BG_CursorInRect ( &button->rect ) ? cgs.media.limboBriefingButtonOn : cgs.media.limboBriefingButtonOff );
	}
}

void CG_LimboPanel_NameEditFinish ( panel_button_t *button )
{
	char buffer[ 256 ];

	trap_Cvar_VariableStringBuffer ( button->text, buffer, 256 );
	trap_Cvar_Set ( "name", buffer );
}

qboolean CG_LimboPanel_CancelButton_KeyDown ( panel_button_t *button, int key )
{
	if ( key == K_MOUSE1 )
	{
		SOUND_CANCEL;

		if ( cgs.limboLoadoutModified )
		{
			trap_SendClientCommand ( "rs" );

			cgs.limboLoadoutSelected = qfalse;
		}

		CG_EventHandling ( CGAME_EVENT_NONE, qfalse );

		return qtrue;
	}

	return qfalse;
}

qboolean CG_LimboPanel_PlusButton_KeyDown ( panel_button_t *button, int key )
{
	if ( key == K_MOUSE1 )
	{
		SOUND_SELECT;

		cgs.ccZoomFactor /= 0.75f;

		if ( cgs.ccZoomFactor > 1.f )
		{
			cgs.ccZoomFactor = 1.f;
		}

		return qtrue;
	}

	return qfalse;
}

qboolean CG_LimboPanel_MinusButton_KeyDown ( panel_button_t *button, int key )
{
	if ( key == K_MOUSE1 )
	{
		SOUND_SELECT;

		cgs.ccZoomFactor *= 0.75f;

		if ( cgs.ccZoomFactor < ( 0.75f * 0.75f * 0.75f * 0.75f * 0.75f ) )
		{
			cgs.ccZoomFactor = ( 0.75f * 0.75f * 0.75f * 0.75f * 0.75f );
		}

		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_SendSetupMsg ( qboolean forceteam )
{
	weapon_t   weap1, weap2;
	const char *str;
	team_t     team;

	if ( forceteam )
	{
		team = CG_LimboPanel_GetTeam();
	}
	else
	{
		team = cgs.clientinfo[ cg.clientNum ].team;
	}

	if ( team == TEAM_SPECTATOR )
	{
		if ( forceteam )
		{
			if ( cgs.clientinfo[ cg.clientNum ].team != TEAM_SPECTATOR )
			{
				trap_SendClientCommand ( "team s 0 0 0\n" );
			}

			CG_EventHandling ( CGAME_EVENT_NONE, qfalse );
		}

		return;
	}

	weap1 = CG_LimboPanel_GetSelectedWeaponForSlot ( 1 );
	weap2 = CG_LimboPanel_GetSelectedWeaponForSlot ( 0 );

	switch ( team )
	{
		case TEAM_AXIS:
			str = "r";
			break;

		case TEAM_ALLIES:
			str = "b";
			break;

		default:
			str = NULL; // rain - don't go spec
			break;
	}

	// rain - if this happens, we're dazed and confused, abort
	if ( !str )
	{
		return;
	}

	trap_SendClientCommand ( va ( "team %s %i %i %i\n", str, CG_LimboPanel_GetClass(), weap1, weap2 ) );

	if ( forceteam )
	{
		CG_EventHandling ( CGAME_EVENT_NONE, qfalse );
	}

	// print center message
	switch ( CG_LimboPanel_GetTeam() )
	{
		case TEAM_AXIS:
			str = "Axis";
			break;

		case TEAM_ALLIES:
			str = "Allied";
			break;

		default: // rain - added default
			str = "unknown";
			break;
	}

	{
		weaponType_t *wt = WM_FindWeaponTypeForWeapon ( weap1 );

		CG_PriorityCenterPrint ( va
		                         ( "You will spawn as an %s %s with a %s.", str, BG_ClassnameForNumber ( CG_LimboPanel_GetClass() ),
		                           wt ? wt->desc : "^1UNKNOWN WEAPON" ), SCREEN_HEIGHT - 88, SMALLCHAR_WIDTH, -1 );
	}

	cgs.limboLoadoutSelected = qtrue;
	cgs.limboLoadoutModified = qtrue;
}

qboolean CG_LimboPanel_OkButton_KeyDown ( panel_button_t *button, int key )
{
	if ( key == K_MOUSE1 )
	{
		SOUND_SELECT;

		CG_LimboPanel_SendSetupMsg ( qtrue );

		return qtrue;
	}

	return qfalse;
}

qboolean CG_LimboPanel_TeamButton_KeyDown ( panel_button_t *button, int key )
{
	if ( key == K_MOUSE1 )
	{
		SOUND_SELECT;

		if ( cgs.ccSelectedTeam != button->data[ 0 ] )
		{
			int oldmax = CG_LimboPanel_GetMaxObjectives();

			cgs.ccSelectedTeam = button->data[ 0 ];

			if ( cgs.ccSelectedObjective == oldmax )
			{
				cgs.ccSelectedObjective = CG_LimboPanel_GetMaxObjectives();
			}

			CG_LimboPanel_SetSelectedWeaponNumForSlot ( 0, 0 );

			CG_LimboPanel_RequestWeaponStats();

			cgs.limboLoadoutModified = qtrue;
		}

		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_RenderTeamButton ( panel_button_t *button )
{
//  vec4_t clr = { 1.f, 1.f, 1.f, 1.0f };
	vec4_t    clr2 = { 1.f, 1.f, 1.f, 0.4f };

	qhandle_t shader;

	trap_R_SetColor ( colorBlack );
	CG_DrawPic ( button->rect.x + 1, button->rect.y + 1, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_off );

	trap_R_SetColor ( NULL );
	CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_off );

	if ( CG_LimboPanel_GetTeam() == teamOrder[ button->data[ 0 ] ] )
	{
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_on );
	}
	else if ( BG_CursorInRect ( &button->rect ) )
	{
		trap_R_SetColor ( clr2 );
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboTeamButtonBack_on );
		trap_R_SetColor ( NULL );
	}

	switch ( button->data[ 0 ] )
	{
		case 0:
			shader = cgs.media.limboTeamButtonAllies;
			break;

		case 1:
			shader = cgs.media.limboTeamButtonAxis;
			break;

		case 2:
			shader = cgs.media.limboTeamButtonSpec;
			break;

		default:
			return;
	}

	trap_R_SetColor ( NULL );
	CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, shader );
}

qboolean CG_LimboPanel_ClassButton_KeyDown ( panel_button_t *button, int key )
{
	if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
	{
		return qfalse;
	}

	if ( key == K_MOUSE1 )
	{
		SOUND_SELECT;

		if ( cgs.ccSelectedClass != button->data[ 1 ] )
		{
			cgs.ccSelectedClass = button->data[ 1 ];

			CG_LimboPanel_SetSelectedWeaponNumForSlot ( 0, 0 );

			CG_LimboPanel_RequestWeaponStats();

			CG_LimboPanel_SendSetupMsg ( qfalse );
		}

		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_ClassBar_Draw ( panel_button_t *button )
{
	const char *text = NULL;
	char       buffer[ 64 ];
	float      w;

	if ( BG_CursorInRect ( &medalPic0.rect ) )
	{
		text = skillNames[ 0 ];
	}
	else if ( BG_CursorInRect ( &medalPic1.rect ) )
	{
		text = skillNames[ 1 ];
	}
	else if ( BG_CursorInRect ( &medalPic2.rect ) )
	{
		text = skillNames[ 2 ];
	}
	else if ( BG_CursorInRect ( &medalPic3.rect ) )
	{
		text = skillNames[ 3 ];
	}
	else if ( BG_CursorInRect ( &medalPic4.rect ) )
	{
		text = skillNames[ 4 ];
	}
	else if ( BG_CursorInRect ( &medalPic5.rect ) )
	{
		text = skillNames[ 5 ];
	}
	else if ( BG_CursorInRect ( &medalPic6.rect ) )
	{
		text = skillNames[ 6 ];
	}
	else if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
	{
		text = "JOIN A TEAM";
	}
	else if ( BG_CursorInRect ( &classButton0.rect ) )
	{
		text = BG_ClassnameForNumber ( 0 );
	}
	else if ( BG_CursorInRect ( &classButton1.rect ) )
	{
		text = BG_ClassnameForNumber ( 1 );
	}
	else if ( BG_CursorInRect ( &classButton2.rect ) )
	{
		text = BG_ClassnameForNumber ( 2 );
	}
	else if ( BG_CursorInRect ( &classButton3.rect ) )
	{
		text = BG_ClassnameForNumber ( 3 );
	}
	else if ( BG_CursorInRect ( &classButton4.rect ) )
	{
		text = BG_ClassnameForNumber ( 4 );
	}

	if ( !text )
	{
		text = BG_ClassnameForNumber ( CG_LimboPanel_GetClass() );
	}

	Q_strncpyz ( buffer, text, sizeof ( buffer ) );
	Q_strupr ( buffer );

	w = CG_Text_Width_Ext ( buffer, button->font->scalex, 0, button->font->font );
	CG_Text_Paint_Ext ( button->rect.x + ( button->rect.w - w ) * 0.5f, button->rect.y, button->font->scalex, button->font->scaley,
	                    button->font->colour, buffer, 0, 0, button->font->style, button->font->font );
}

void CG_LimboPanel_RenderClassButton ( panel_button_t *button )
{
	vec4_t clr = { 1.f, 1.f, 1.f, 0.4f };
	vec4_t clr2 = { 1.f, 1.f, 1.f, 0.75f };
	vec4_t clr3 = { 1.f, 1.f, 1.f, 0.6f };

	int    i;
	float  s0, t0, s1, t1;
	float  x, y, w, h;

	CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButton2Back_off );

	if ( CG_LimboPanel_GetTeam() != TEAM_SPECTATOR )
	{
		if ( button->data[ 1 ] == CG_LimboPanel_GetClass() )
		{
			CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButton2Back_on );
		}
		else if ( BG_CursorInRect ( &button->rect ) )
		{
			trap_R_SetColor ( clr );
			CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButton2Back_on );
			trap_R_SetColor ( NULL );
		}
	}

	for ( i = 0; i < 4; i++ )
	{
		if ( cgs.clientinfo[ cg.clientNum ].skill[ BG_ClassSkillForClass ( button->data[ 1 ] ) ] <= i )
		{
			break;
		}

		if ( i == 0 || i == 1 )
		{
			s0 = 0.5f;
			s1 = 1.0f;
		}
		else
		{
			s0 = 0.0f;
			s1 = 0.5f;
		}

		if ( i == 1 || i == 2 )
		{
			t0 = 0.5f;
			t1 = 1.0f;
		}
		else
		{
			t0 = 0.0f;
			t1 = 0.5f;
		}

		x = button->rect.x + button->rect.w * s0;
		y = button->rect.y + button->rect.h * t0;
		w = button->rect.w * 0.5f;
		h = button->rect.h * 0.5f;

		CG_AdjustFrom640 ( &x, &y, &w, &h );

		if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
		{
			trap_R_DrawStretchPic ( x, y, w, h, s0, t0, s1, t1, cgs.media.limboClassButton2Wedge_off );
		}
		else
		{
			if ( button->data[ 1 ] == CG_LimboPanel_GetClass() )
			{
				trap_R_DrawStretchPic ( x, y, w, h, s0, t0, s1, t1, cgs.media.limboClassButton2Wedge_on );
			}
			else if ( BG_CursorInRect ( &button->rect ) )
			{
				trap_R_SetColor ( clr3 );
				trap_R_DrawStretchPic ( x, y, w, h, s0, t0, s1, t1, cgs.media.limboClassButton2Wedge_on );
				trap_R_SetColor ( NULL );
			}
			else
			{
				trap_R_DrawStretchPic ( x, y, w, h, s0, t0, s1, t1, cgs.media.limboClassButton2Wedge_off );
			}
		}
	}

	if ( CG_LimboPanel_GetTeam() != TEAM_SPECTATOR && button->data[ 1 ] == CG_LimboPanel_GetClass() )
	{
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButtons2[ button->data[ 1 ] ] );
	}
	else
	{
		trap_R_SetColor ( clr2 );
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboClassButtons2[ button->data[ 1 ] ] );
		trap_R_SetColor ( NULL );
	}
}

int CG_LimboPanel_GetMaxObjectives ( void )
{
	if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
	{
		return 0;
	}

	return atoi ( Info_ValueForKey ( CG_ConfigString ( CS_MULTI_INFO ), "numobjectives" ) );
}

qboolean CG_LimboPanel_ObjectiveText_KeyDown ( panel_button_t *button, int key )
{
	int max;

	if ( key == K_MOUSE1 )
	{
		SOUND_SELECT;

		max = CG_LimboPanel_GetMaxObjectives();

		if ( button->data[ 7 ] == 0 )
		{
			if ( ++cgs.ccSelectedObjective > max )
			{
				cgs.ccSelectedObjective = 0;
			}
		}
		else
		{
			if ( --cgs.ccSelectedObjective < 0 )
			{
				cgs.ccSelectedObjective = max;
			}
		}

		CG_LimboPanel_RequestObjective();

		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_RenderObjectiveText ( panel_button_t *button )
{
	const char *cs;
	char       *info, *s, *p;
	float      y;
	char       buffer[ 1024 ];
	int        status = 0;

	if ( cg_gameType.integer == GT_WOLF_LMS )
	{
		//cs = CG_ConfigString( CS_MULTI_MAPDESC );
		//Q_strncpyz( buffer, cs, sizeof(buffer) );

		Q_strncpyz ( buffer, cg.objMapDescription_Neutral, sizeof ( buffer ) );
	}
	else
	{
		if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
		{
			//cs = CG_ConfigString( CS_MULTI_MAPDESC3 );
			//Q_strncpyz( buffer, cs, sizeof(buffer) );

			Q_strncpyz ( buffer, cg.objMapDescription_Neutral, sizeof ( buffer ) );
		}
		else
		{
			if ( cgs.ccSelectedObjective != CG_LimboPanel_GetMaxObjectives() )
			{
				cs = CG_ConfigString ( CS_MULTI_OBJECTIVE );

				if ( CG_LimboPanel_GetTeam() == TEAM_AXIS )
				{
					//info = Info_ValueForKey( cs, "axis_desc" );
					info = cg.objDescription_Axis[ cgs.ccSelectedObjective ];
					status = atoi ( Info_ValueForKey ( cs, va ( "x%i", cgs.ccSelectedObjective + 1 ) ) );
				}
				else
				{
					//info = Info_ValueForKey( cs, "allied_desc" );
					info = cg.objDescription_Allied[ cgs.ccSelectedObjective ];
					status = atoi ( Info_ValueForKey ( cs, va ( "a%i", cgs.ccSelectedObjective + 1 ) ) );
				}

				if ( ! ( info && *info ) )
				{
					info = "No Information Supplied";
				}

				Q_strncpyz ( buffer, info, sizeof ( buffer ) );
			}
			else
			{
				//cs = CG_ConfigString( CG_LimboPanel_GetTeam() == TEAM_AXIS ? CS_MULTI_MAPDESC2 : CS_MULTI_MAPDESC );
				//Q_strncpyz( buffer, cs, sizeof(buffer) );

				if ( CG_LimboPanel_GetTeam() == TEAM_AXIS )
				{
					Q_strncpyz ( buffer, cg.objMapDescription_Axis, sizeof ( buffer ) );
				}
				else
				{
					Q_strncpyz ( buffer, cg.objMapDescription_Allied, sizeof ( buffer ) );
				}
			}
		}
	}

	while ( ( s = strchr ( buffer, '*' ) ) )
	{
		*s = '\n';
	}

	CG_FitTextToWidth_Ext ( buffer, button->font->scalex, button->rect.w - 16, sizeof ( buffer ), &cgs.media.limboFont2 );

	y = button->rect.y + 12;

	s = p = buffer;

	while ( *p )
	{
		if ( *p == '\n' )
		{
			*p++ = '\0';
			CG_Text_Paint_Ext ( button->rect.x + 4, y, button->font->scalex, button->font->scaley, button->font->colour, s, 0, 0, 0,
			                    &cgs.media.limboFont2 );
			y += 8;
			s = p;
		}
		else
		{
			p++;
		}
	}

	if ( cg_gameType.integer != GT_WOLF_LMS && CG_LimboPanel_GetTeam() != TEAM_SPECTATOR )
	{
		const char *ofTxt;
		float      w, x;

		if ( cgs.ccSelectedObjective == CG_LimboPanel_GetMaxObjectives() )
		{
			ofTxt = va ( "1of%i", CG_LimboPanel_GetMaxObjectives() + 1 );
		}
		else
		{
			ofTxt = va ( "%iof%i", cgs.ccSelectedObjective + 2, CG_LimboPanel_GetMaxObjectives() + 1 );
		}

		w = CG_Text_Width_Ext ( ofTxt, 0.2f, 0, &cgs.media.limboFont2 );

		x = button->rect.x + button->rect.w - w - 4;

		CG_Text_Paint_Ext ( x, button->rect.y + button->rect.h - 2, 0.2f, 0.2f, colorBlack, ofTxt, 0, 0, 0, &cgs.media.limboFont2 );
	}

	if ( status == 1 )
	{
		CG_DrawPic ( button->rect.x + 87, button->rect.y + 8, button->rect.w - 174, button->rect.h - 8, cgs.media.ccStamps[ 0 ] );
	}
	else if ( status == 2 )
	{
		CG_DrawPic ( button->rect.x + 87, button->rect.y + 8, button->rect.w - 174, button->rect.h - 8, cgs.media.ccStamps[ 1 ] );
	}
}

void CG_LimboPanel_RenderObjectiveBack ( panel_button_t *button )
{
//  int max = CG_LimboPanel_GetMaxObjectives();

//  if( cgs.ccSelectedObjective == max ) {
	CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h,
	             cgs.media.limboObjectiveBack[ TEAM_SPECTATOR - TEAM_AXIS ] );
//  } else {
//      CG_DrawPic( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboObjectiveBack[ CG_LimboPanel_GetTeam() - TEAM_AXIS ] );
//  }
}

void CG_LimboPanel_RenderCommandMap ( panel_button_t *button )
{
	CG_DrawMap ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.ccFilter, NULL, qtrue, 1.f, qtrue );
	CG_CommandMap_DrawHighlightText();
}

qboolean CG_LimboPanel_RenderLight_GetValue ( panel_button_t *button )
{
	switch ( button->data[ 0 ] )
	{
		case 0:
			return ( CG_LimboPanel_GetClass() == button->data[ 1 ] ) ? qtrue : qfalse;

		case 1:
			return ( CG_LimboPanel_GetTeam() == teamOrder[ button->data[ 1 ] ] ) ? qtrue : qfalse;
	}

	return qfalse;
}

void CG_LimboPanel_RenderLight ( panel_button_t *button )
{
	if ( CG_LimboPanel_RenderLight_GetValue ( button ) )
	{
//      if( !button->data[2] || (button->data[2] - cg.time < 0) ) {
		button->data[ 3 ] = button->data[ 3 ] ^ 1;
//          if( button->data[3] ) {
//              button->data[2] = cg.time + rand() % 200;
//          } else {
//              button->data[2] = cg.time + rand() % 1000;
//          }
//      }

		CG_DrawPic ( button->rect.x - 4, button->rect.y - 2, button->rect.w + 4, button->rect.h + 4,
		             button->data[ 3 ] ? cgs.media.limboLight_on2 : cgs.media.limboLight_on );
	}
	else
	{
		CG_DrawPic ( button->rect.x - 4, button->rect.y - 2, button->rect.w + 4, button->rect.h + 4, cgs.media.limboLight_off );
	}
}

void CG_DrawPlayerHead ( rectDef_t *rect, bg_character_t *character, bg_character_t *headcharacter, float yaw, float pitch,
                         qboolean drawHat, hudHeadAnimNumber_t animation, qhandle_t painSkin, int rank, qboolean spectator )
{
	float       len;
	vec3_t      origin;
	vec3_t      mins, maxs, angles;
	float       x, y, w, h;
	refdef_t    refdef;
	refEntity_t head, hat, mrank;

	if ( !character )
	{
		return;
	}

	trap_R_SaveViewParms();

	x = rect->x;
	y = rect->y;
	w = rect->w;
	h = rect->h;

	CG_AdjustFrom640 ( &x, &y, &w, &h );

	memset ( &refdef, 0, sizeof ( refdef ) );

	refdef.rdflags = RDF_NOWORLDMODEL;
	AxisClear ( refdef.viewaxis );

	refdef.fov_x = 8;
	refdef.fov_y = 10;

	refdef.x = x;
	refdef.y = y;
	refdef.width = w;
	refdef.height = h;

	refdef.time = cg.time;

	trap_R_ClearScene();

	// offset the origin y and z to center the head
	trap_R_ModelBounds ( character->hudhead, mins, maxs );

	origin[ 2 ] = -0.7 * ( mins[ 2 ] + maxs[ 2 ] );
	origin[ 1 ] = 0.5 * ( mins[ 1 ] + maxs[ 1 ] );

	// calculate distance so the head nearly fills the box
	// assume heads are taller than wide
	len = 3.5f * ( maxs[ 2 ] - mins[ 2 ] );
	origin[ 0 ] = len / tan ( 20 / 2 ); // 0.268;  // len / tan( fov/2 )

	angles[ PITCH ] = pitch;
	angles[ YAW ] = yaw;
	angles[ ROLL ] = 0;

	memset ( &head, 0, sizeof ( head ) );
	AnglesToAxis ( angles, head.axis );
	VectorCopy ( origin, head.origin );
	head.hModel = headcharacter->hudhead;
	head.customSkin = headcharacter->hudheadskin;
	head.renderfx = RF_NOSHADOW | RF_FORCENOLOD; // no stencil shadows

	// ydnar: light the model with the current lightgrid
	//VectorCopy( cg.refdef.vieworg, head.lightingOrigin );
	if ( !cg.showGameView )
	{
		head.renderfx |= /*RF_LIGHTING_ORIGIN | */ RF_MINLIGHT;
	}

	CG_HudHeadAnimation ( headcharacter, &cg.predictedPlayerEntity.pe.hudhead, &head.oldframe, &head.frame, &head.backlerp,
	                      animation );

	if ( drawHat )
	{
		memset ( &hat, 0, sizeof ( hat ) );
		hat.hModel = character->accModels[ ACC_HAT ];
		hat.customSkin = character->accSkins[ ACC_HAT ];
		hat.renderfx = RF_NOSHADOW | RF_FORCENOLOD; // no stencil shadows

		// ydnar: light the model with the current lightgrid
		//VectorCopy( cg.refdef.vieworg, hat.lightingOrigin );
		if ( !cg.showGameView )
		{
			hat.renderfx |= /*RF_LIGHTING_ORIGIN | */ RF_MINLIGHT;
		}

		CG_PositionEntityOnTag ( &hat, &head, "tag_mouth", 0, NULL );

		if ( rank )
		{
			memset ( &mrank, 0, sizeof ( mrank ) );

			mrank.hModel = character->accModels[ ACC_RANK ];
			mrank.customShader = rankicons[ rank ][ 1 ].shader;
			mrank.renderfx = RF_NOSHADOW | RF_FORCENOLOD; // no stencil shadows

			CG_PositionEntityOnTag ( &mrank, &head, "tag_mouth", 0, NULL );
		}
	}

	head.shaderRGBA[ 0 ] = 255;
	head.shaderRGBA[ 1 ] = 255;
	head.shaderRGBA[ 2 ] = 255;
	head.shaderRGBA[ 3 ] = 255;

	hat.shaderRGBA[ 0 ] = 255;
	hat.shaderRGBA[ 1 ] = 255;
	hat.shaderRGBA[ 2 ] = 255;
	hat.shaderRGBA[ 3 ] = 255;

	mrank.shaderRGBA[ 0 ] = 255;
	mrank.shaderRGBA[ 1 ] = 255;
	mrank.shaderRGBA[ 2 ] = 255;
	mrank.shaderRGBA[ 3 ] = 255;

	/*if( spectator ) {
	   head.customShader = cgs.media.limboSpectator;
	   head.customSkin = 0;
	   } */
	trap_R_AddRefEntityToScene ( &head );

	if ( painSkin )
	{
		head.customShader = 0;
		head.customSkin = painSkin;
		trap_R_AddRefEntityToScene ( &head );
	}

	if ( drawHat )
	{
		/*if( spectator ) {
		   hat.customShader = cgs.media.limboSpectator;
		   hat.customSkin = 0;
		   } */
		trap_R_AddRefEntityToScene ( &hat );

		if ( rank )
		{
			trap_R_AddRefEntityToScene ( &mrank );
		}
	}

	trap_R_RenderScene ( &refdef );

//bani - render to texture api example
//draws the player head on one of the fueldump textures.
#ifdef TEST_API_RENDERTOTEXTURE
	{
		static int texid = 0;

		if ( !texid )
		{
			texid = trap_R_GetTextureId ( "textures/stone/mxsnow3.tga" );
		}

		trap_R_RenderToTexture ( texid, 0, 0, 256, 256 );
	}
#endif

	trap_R_RestoreViewParms();
}

void CG_LimboPanel_RenderHead ( panel_button_t *button )
{
	vec4_t clrBack = { 0.05f, 0.05f, 0.05f, 1.f };

	if ( CG_LimboPanel_GetTeam() != TEAM_SPECTATOR )
	{
		CG_FillRect ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, clrBack );
		CG_DrawPlayerHead ( &button->rect, CG_LimboPanel_GetCharacter(), CG_LimboPanel_GetCharacter(), 180, 0, qtrue, HD_IDLE4, 0,
		                    cgs.clientinfo[ cg.clientNum ].rank, qfalse );
	}
	else
	{
		//CG_FillRect( button->rect.x, button->rect.y, button->rect.w, button->rect.h, colorBlack );
		//CG_DrawPlayerHead( &button->rect, BG_GetCharacter( TEAM_ALLIES, PC_SOLDIER ), BG_GetCharacter( TEAM_ALLIES, PC_SOLDIER ), 180, 0, qtrue, HD_IDLE4, 0, 0, qtrue );

		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboSpectator );
	}

	VectorSet ( clrBack, .6f, .6f, .6f );
	trap_R_SetColor ( clrBack );

	// top / bottom
	CG_DrawPic ( button->rect.x, button->rect.y - 2, button->rect.w, 2, cgs.media.limboWeaponCardSurroundH );
	CG_DrawPicST ( button->rect.x, button->rect.y + button->rect.h, button->rect.w, 2, 0.f, 1.f, 1.f, 0.f,
	               cgs.media.limboWeaponCardSurroundH );

	CG_DrawPic ( button->rect.x - 2, button->rect.y, 2, button->rect.h, cgs.media.limboWeaponCardSurroundV );
	CG_DrawPicST ( button->rect.x + button->rect.w, button->rect.y, 2, button->rect.h, 1.f, 0.f, 0.f, 1.f,
	               cgs.media.limboWeaponCardSurroundV );

	CG_DrawPicST ( button->rect.x - 2, button->rect.y - 2, 2, 2, 0.f, 0.f, 1.f, 1.f, cgs.media.limboWeaponCardSurroundC );
	CG_DrawPicST ( button->rect.x + button->rect.w, button->rect.y - 2, 2, 2, 1.f, 0.f, 0.f, 1.f,
	               cgs.media.limboWeaponCardSurroundC );
	CG_DrawPicST ( button->rect.x + button->rect.w, button->rect.y + button->rect.h, 2, 2, 1.f, 1.f, 0.f, 0.f,
	               cgs.media.limboWeaponCardSurroundC );
	CG_DrawPicST ( button->rect.x - 2, button->rect.y + button->rect.h, 2, 2, 0.f, 1.f, 1.f, 0.f,
	               cgs.media.limboWeaponCardSurroundC );

	trap_R_SetColor ( NULL );
}

qboolean CG_LimboPanel_Filter_KeyDown ( panel_button_t *button, int key )
{
	if ( key == K_MOUSE1 )
	{
		SOUND_FILTER;

		cgs.ccFilter ^= ( 1 << button->data[ 0 ] );
		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_Filter_Draw ( panel_button_t *button )
{
	if ( cgs.ccFilter & ( 1 << button->data[ 0 ] ) )
	{
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.ccFilterBackOff );
	}
	else
	{
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.ccFilterBackOn );
	}

//  CG_DrawPic( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.ccFilterPics[button->data[0]] );
	CG_DrawPic ( button->rect.x + 1, button->rect.y + 1, button->rect.w - 2, button->rect.h - 2,
	             cgs.media.ccFilterPics[ button->data[ 0 ] ] );
}

void CG_LimboPanel_RenderSkillIcon ( panel_button_t *button )
{
	qhandle_t shader;

	if ( cg_gameType.integer == GT_WOLF_LMS /*|| CG_LimboPanel_GetTeam() == TEAM_SPECTATOR */ )
	{
		return;
	}

	switch ( button->data[ 0 ] )
	{
		case 0:
			shader = cgs.media.limboSkillsBS;
			break;

		case 1:
			shader = cgs.media.limboSkillsLW;
			break;

		case 2:
			shader = cgs.media.limboClassButtons[ CG_LimboPanel_GetClass() ];
			break;

		default:
			return;
	}

	CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, shader );
}

qboolean CG_LimboPanel_WeaponLights_KeyDown ( panel_button_t *button, int key )
{
	if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
	{
		return qfalse;
	}

	if ( key == K_MOUSE1 )
	{
		SOUND_SELECT;

		cgs.ccSelectedWeaponNumber = button->data[ 0 ];
		CG_LimboPanel_RequestWeaponStats();
		return qtrue;
	}

	return qfalse;
}

void CG_LimboPanel_WeaponLights ( panel_button_t *button )
{
	if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
	{
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponNumber_off );
	}
	else
	{
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h,
		             button->data[ 0 ] ==
		             cgs.ccSelectedWeaponNumber ? cgs.media.limboWeaponNumber_on : cgs.media.limboWeaponNumber_off );
	}
}

qboolean CG_LimboPanel_WeaponPanel_KeyDown ( panel_button_t *button, int key )
{
	button->data[ 7 ] = 0;

	if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
	{
		return qfalse;
	}

	if ( key == K_MOUSE1 )
	{
		SOUND_SELECT;

		BG_PanelButtons_SetFocusButton ( button );
		return qtrue;
	}

	return qfalse;
}

qboolean CG_LimboPanel_WeaponPanel_KeyUp ( panel_button_t *button, int key )
{
	int       cnt, i;
	rectDef_t rect;

	if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
	{
		return qfalse;
	}

	if ( key == K_MOUSE1 )
	{
		if ( BG_PanelButtons_GetFocusButton() == button )
		{
			memcpy ( &rect, &button->rect, sizeof ( rect ) );
			rect.y -= rect.h;

			cnt = CG_LimboPanel_WeaponCount();

			for ( i = 1; i < cnt; i++, rect.y -= rect.h )
			{
				if ( !BG_CursorInRect ( &rect ) )
				{
					continue;
				}

				if ( !CG_LimboPanel_GetSelectedWeaponNum() )
				{
					CG_LimboPanel_SetSelectedWeaponNum ( i );
					CG_LimboPanel_SendSetupMsg ( qfalse );
				}
				else
				{
					if ( i <= CG_LimboPanel_GetSelectedWeaponNum() )
					{
						CG_LimboPanel_SetSelectedWeaponNum ( i - 1 );
						CG_LimboPanel_SendSetupMsg ( qfalse );
					}
					else
					{
						CG_LimboPanel_SetSelectedWeaponNum ( i );
						CG_LimboPanel_SendSetupMsg ( qfalse );
					}
				}
			}

			BG_PanelButtons_SetFocusButton ( NULL );

			return qtrue;
		}
	}

	return qfalse;
}

void CG_LimboPanel_WeaponPanel_DrawWeapon ( rectDef_t *rect, weapon_t weap, qboolean highlight, const char *ofTxt,
    qboolean disabled )
{
	weaponType_t *wt = WM_FindWeaponTypeForWeapon ( weap );
	qhandle_t    shader = cgs.media.limboWeaponCard;
	int          width = CG_Text_Width_Ext ( ofTxt, 0.2f, 0, &cgs.media.limboFont2 );
	float        x = rect->x + rect->w - width - 4;
	vec4_t       clr;

	if ( !wt )
	{
		return;
	}

	CG_DrawPic ( rect->x, rect->y, rect->w, rect->h, shader );

//  CG_DrawRect( rect->x, rect->y, rect->w, rect->h, 1, colorWhite );
	if ( wt->desc )
	{
		if ( highlight && BG_CursorInRect ( rect ) )
		{
			Vector4Copy ( weaponPanelNameFont.colour, clr );
			clr[ 3 ] *= 1.5;
			CG_Text_Paint_Ext ( rect->x + 4, rect->y + 12, weaponPanelNameFont.scalex, weaponPanelNameFont.scaley, clr, wt->desc, 0,
			                    0, weaponPanelNameFont.style, weaponPanelNameFont.font );
		}
		else
		{
			CG_Text_Paint_Ext ( rect->x + 4, rect->y + 12, weaponPanelNameFont.scalex, weaponPanelNameFont.scaley,
			                    weaponPanelNameFont.colour, wt->desc, 0, 0, weaponPanelNameFont.style, weaponPanelNameFont.font );
		}
	}

	{
		float x2, y2, w, h, s0, s1, t0, t1;

		trap_R_SetColor ( NULL );

		x2 = rect->x;
		y2 = rect->y + ( rect->h * 0.25f );

		CG_LimboPanel_GetWeaponCardIconData ( weap, &shader, &w, &h, &s0, &t0, &s1, &t1 );

		w *= rect->w;
		h *= rect->h * 0.75f;

		CG_DrawPicST ( x2, y2, w, h, s0, t0, s1, t1, shader );

		if ( disabled )
		{
			vec4_t clr = { 1.f, 1.f, 1.f, 0.6f };

			trap_R_SetColor ( clr );
			CG_DrawPic ( x2, y2 + 4 + ( h - 16 ) * 0.5f, w, 16, cgs.media.limboWeaponCardOOS );
			trap_R_SetColor ( NULL );
		}
	}

	CG_Text_Paint_Ext ( x, rect->y + rect->h - 2, 0.2f, 0.2f, colorBlack, ofTxt, 0, 0, 0, &cgs.media.limboFont2 );
}

#define BRDRSIZE 4
void CG_DrawBorder ( float x, float y, float w, float h, qboolean fill, qboolean drawMouseOver )
{
	vec4_t clrBack = { 0.1f, 0.1f, 0.1f, 1.f };
	vec4_t clrBack2 = { 0.2f, 0.2f, 0.2f, 1.f };

	// top / bottom
	CG_DrawPic ( x, y - BRDRSIZE, w, BRDRSIZE, cgs.media.limboWeaponCardSurroundH );
	CG_DrawPicST ( x, y + h, w, BRDRSIZE, 0.f, 1.f, 1.f, 0.f, cgs.media.limboWeaponCardSurroundH );

	CG_DrawPic ( x - BRDRSIZE, y, BRDRSIZE, h, cgs.media.limboWeaponCardSurroundV );
	CG_DrawPicST ( x + w, y, BRDRSIZE, h, 1.f, 0.f, 0.f, 1.f, cgs.media.limboWeaponCardSurroundV );

	CG_DrawPicST ( x - BRDRSIZE, y - BRDRSIZE, BRDRSIZE, BRDRSIZE, 0.f, 0.f, 1.f, 1.f, cgs.media.limboWeaponCardSurroundC );
	CG_DrawPicST ( x + w, y - BRDRSIZE, BRDRSIZE, BRDRSIZE, 1.f, 0.f, 0.f, 1.f, cgs.media.limboWeaponCardSurroundC );
	CG_DrawPicST ( x + w, y + h, BRDRSIZE, BRDRSIZE, 1.f, 1.f, 0.f, 0.f, cgs.media.limboWeaponCardSurroundC );
	CG_DrawPicST ( x - BRDRSIZE, y + h, BRDRSIZE, BRDRSIZE, 0.f, 1.f, 1.f, 0.f, cgs.media.limboWeaponCardSurroundC );

	if ( fill )
	{
		if ( drawMouseOver )
		{
			rectDef_t rect;

			rect.x = x;
			rect.y = y;
			rect.w = w;
			rect.h = h;

			if ( BG_CursorInRect ( &rect ) )
			{
				CG_FillRect ( x, y, w, h, clrBack2 );
			}
			else
			{
				CG_FillRect ( x, y, w, h, clrBack );
			}
		}
		else
		{
			CG_FillRect ( x, y, w, h, clrBack );
		}
	}
}

void CG_LimboPanel_Border_Draw ( panel_button_t *button )
{
	CG_DrawBorder ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, qtrue, qtrue );
}

void CG_LimboPanel_WeaponPanel ( panel_button_t *button )
{
	weapon_t weap = CG_LimboPanel_GetSelectedWeapon();
	int      cnt = CG_LimboPanel_WeaponCount();

	if ( cgs.ccSelectedWeapon2 >= CG_LimboPanel_WeaponCount_ForSlot ( 0 ) )
	{
		cgs.ccSelectedWeapon2 = CG_LimboPanel_WeaponCount_ForSlot ( 0 ) - 1;
	}

	if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
	{
		vec4_t clr = { 0.f, 0.f, 0.f, 0.4f };

		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponCard );

		trap_R_SetColor ( clr );
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponBlendThingy );
		trap_R_SetColor ( NULL );

		CG_Text_Paint_Ext ( button->rect.x + 4, button->rect.y + 12, weaponPanelNameFont.scalex, weaponPanelNameFont.scaley,
		                    weaponPanelNameFont.colour, "SPECTATOR", 0, 0, weaponPanelNameFont.style, weaponPanelNameFont.font );

		return;
	}

	if ( BG_PanelButtons_GetFocusButton() == button && cnt > 1 )
	{
		int       i, x;
		rectDef_t rect;

		memcpy ( &rect, &button->rect, sizeof ( rect ) );

		CG_LimboPanel_WeaponPanel_DrawWeapon ( &rect, weap, qtrue, va ( "%iof%i", CG_LimboPanel_GetSelectedWeaponNum() + 1, cnt ),
		                                       CG_LimboPanel_RealWeaponIsDisabled ( weap ) );

		if ( BG_CursorInRect ( &rect ) )
		{
			if ( button->data[ 7 ] != 0 )
			{
				SOUND_FOCUS;

				button->data[ 7 ] = 0;
			}
		}

		rect.y -= rect.h;

		// render in expanded mode ^
		for ( i = 0, x = 1; i < cnt; i++ )
		{
			weapon_t cycleWeap = CG_LimboPanel_GetWeaponForNumber ( i, cgs.ccSelectedWeaponNumber, qtrue );

			if ( cycleWeap != weap )
			{
				CG_LimboPanel_WeaponPanel_DrawWeapon ( &rect, cycleWeap, qtrue, va ( "%iof%i", i + 1, cnt ),
				                                       CG_LimboPanel_RealWeaponIsDisabled ( cycleWeap ) );

				if ( BG_CursorInRect ( &rect ) )
				{
					if ( button->data[ 7 ] != x )
					{
						SOUND_FOCUS;

						button->data[ 7 ] = x;
					}
				}

				rect.y -= rect.h;
				x++;
			}
		}

		CG_DrawBorder ( button->rect.x, button->rect.y - ( ( cnt - 1 ) * button->rect.h ), button->rect.w, button->rect.h * cnt, qfalse,
		                qfalse );
	}
	else
	{
		vec4_t clr = { 0.f, 0.f, 0.f, 0.4f };
		vec4_t clr2 = { 1.f, 1.f, 1.f, 0.4f };

		// render in normal mode
		CG_LimboPanel_WeaponPanel_DrawWeapon ( &button->rect, weap, cnt > 1 ? qtrue : qfalse,
		                                       va ( "%iof%i", CG_LimboPanel_GetSelectedWeaponNum() + 1, cnt ),
		                                       CG_LimboPanel_RealWeaponIsDisabled ( weap ) );

		if ( cnt <= 1 || !BG_CursorInRect ( &button->rect ) )
		{
			trap_R_SetColor ( clr2 );
		}

		CG_DrawPic ( button->rect.x + button->rect.w - 20, button->rect.y + 4, 16, 12, cgs.media.limboWeaponCardArrow );

		trap_R_SetColor ( clr );
		CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.limboWeaponBlendThingy );
		trap_R_SetColor ( NULL );
	}
}

void CG_LimboPanel_RenderCounterNumber ( float x, float y, float w, float h, float number, qhandle_t shaderBack,
    qhandle_t shaderRoll, int numbuttons )
{
	float numberS = ( ( ( numbuttons - 1 ) - number ) + 0 ) * ( 1.f / numbuttons );
	float numberE = ( ( ( numbuttons - 1 ) - number ) + 1 ) * ( 1.f / numbuttons );

	CG_AdjustFrom640 ( &x, &y, &w, &h );
	trap_R_DrawStretchPic ( x, y, w, h, 0, 0, 1, 1, shaderBack );
	trap_R_DrawStretchPic ( x, y, w, h, 0, numberS, 1, numberE, shaderRoll );
}

int CG_LimboPanel_RenderCounter_ValueForButton ( panel_button_t *button )
{
	int i, count = 0;

	switch ( button->data[ 0 ] )
	{
		case 0: // class counts
			if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR || CG_LimboPanel_GetRealTeam() != CG_LimboPanel_GetTeam() )
			{
				return 0; // dont give class counts unless we are on that team (or spec)
			}

			for ( i = 0; i < MAX_CLIENTS; i++ )
			{
				if ( !cgs.clientinfo[ i ].infoValid )
				{
					continue;
				}

				if ( cgs.clientinfo[ i ].team != CG_LimboPanel_GetTeam() || cgs.clientinfo[ i ].cls != button->data[ 1 ] )
				{
					continue;
				}

				count++;
			}

			return count;

		case 1: // team counts
			for ( i = 0; i < MAX_CLIENTS; i++ )
			{
				if ( !cgs.clientinfo[ i ].infoValid )
				{
					continue;
				}

				if ( cgs.clientinfo[ i ].team != teamOrder[ button->data[ 1 ] ] )
				{
					continue;
				}

				count++;
			}

			return count;

		case 2: // xp
			return cg.xp;

		case 3: // respawn time
			return CG_CalculateReinfTime_Float ( qtrue );

		case 4: // skills
			switch ( button->data[ 1 ] )
			{
				case 0:
					count = cgs.clientinfo[ cg.clientNum ].skill[ SK_BATTLE_SENSE ];
					break;

				case 1:
					count = cgs.clientinfo[ cg.clientNum ].skill[ SK_LIGHT_WEAPONS ];
					break;

				case 2:
					count = cgs.clientinfo[ cg.clientNum ].skill[ BG_ClassSkillForClass ( CG_LimboPanel_GetClass() ) ];
					break;
			}

			return ( 1 << count ) - 1;

		case 5: // clock
			if ( !cgs.timelimit )
			{
				return 0;
			}

			count = ( ( cgs.timelimit * 60 * 1000 ) - ( cg.time - cgs.levelStartTime ) ) / 1000;

			switch ( button->data[ 1 ] )
			{
				case 0: // secs
					return count % 60;

				case 1: // mins
					return count / 60;
			}

			return 0;

		case 6: // stats
			switch ( button->data[ 1 ] )
			{
				case 0:
					return cgs.ccWeaponShots;

				case 1:
					return cgs.ccWeaponHits;

				case 2:
					return cgs.ccWeaponShots != 0 ? 100 * cgs.ccWeaponHits / cgs.ccWeaponShots : 0;
			}
	}

	return 0;
}

int CG_LimboPanel_RenderCounter_RollTimeForButton ( panel_button_t *button )
{
	float diff;

	switch ( button->data[ 0 ] )
	{
		case 0: // class counts
		case 1: // team counts
			return 100.f;

		case 4: // skills
			return 1000.f;

		case 6: // stats
			diff = Q_fabs ( button->data[ 3 ] - CG_LimboPanel_RenderCounter_ValueForButton ( button ) );

			if ( diff < 5 )
			{
				return 200.f / diff;
			}
			else
			{
				return 50.f;
			}

		case 5: // clock
		case 3: // respawn time
		case 2: // xp
			return 50.f;
	}

	return 1000.f;
}

int CG_LimboPanel_RenderCounter_MaxChangeForButton ( panel_button_t *button )
{
	switch ( button->data[ 0 ] )
	{
		case 2: // xp
		case 6: // stats
			return 5;
	}

	return 1;
}

int CG_LimboPanel_RenderCounter_NumRollers ( panel_button_t *button )
{
	switch ( button->data[ 0 ] )
	{
		case 0: // class counts
		case 1: // team counts
		case 5: // clock
		case 3: // respawn time
			return 2;

		case 4: // skills
			if ( cg_gameType.integer == GT_WOLF_LMS /*|| CG_LimboPanel_GetTeam() == TEAM_SPECTATOR */ )
			{
				return 0;
			}

			return 4;

		case 6: // stats
			switch ( button->data[ 1 ] )
			{
				case 0:
				case 1:
					return 4;

				case 2:
					return 3;
			}

		case 2: // xp
			if ( cg_gameType.integer == GT_WOLF_LMS )
			{
				return 0;
			}

			return 6;
	}

	return 0;
}

qboolean CG_LimboPanel_RenderCounter_CountsDown ( panel_button_t *button )
{
	switch ( button->data[ 0 ] )
	{
		case 4: // skill
		case 2: // xp
			return qfalse;

		default:
			break;
	}

	return qtrue;
}

qboolean CG_LimboPanel_RenderCounter_CountsUp ( panel_button_t *button )
{
	switch ( button->data[ 0 ] )
	{
		case 4: // skill
		case 3: // respawn time
		case 5: // clock
			return qfalse;

		default:
			break;
	}

	return qtrue;
}

qboolean CG_LimboPanel_RenderCounter_StartSet ( panel_button_t *button )
{
	switch ( button->data[ 0 ] )
	{
		case 3: // respawn time
		case 5: // clock
			return qtrue;

		default:
			break;
	}

	return qfalse;
}

void CG_LimboPanel_RenderMedal ( panel_button_t *button )
{
	CG_DrawPic ( button->rect.x, button->rect.y, button->rect.w, button->rect.h, cgs.media.medal_back );

	if ( cgs.clientinfo[ cg.clientNum ].medals[ button->data[ 0 ] ] )
	{
		CG_DrawPic ( button->rect.x - 2, button->rect.y, button->rect.w + 4, button->rect.h, cgs.media.medals[ button->data[ 0 ] ] );
	}
}

qboolean CG_LimboPanel_RenderCounter_IsReversed ( panel_button_t *button )
{
	switch ( button->data[ 0 ] )
	{
		case 4: // skill
			return qtrue;

		default:
			break;
	}

	return qfalse;
}

void CG_LimboPanel_RenderCounter_GetShaders ( panel_button_t *button, qhandle_t *shaderBack, qhandle_t *shaderRoll,
    int *numimages )
{
	switch ( button->data[ 0 ] )
	{
		case 4: // skills
			*shaderBack = cgs.media.limboStar_back;
			*shaderRoll = cgs.media.limboStar_roll;
			*numimages = 2;
			return;

		default:
			*shaderBack = cgs.media.limboNumber_back;
			*shaderRoll = cgs.media.limboNumber_roll;
			*numimages = 10;
			return;
	}
}

void CG_LimboPanelRenderText_NoLMS ( panel_button_t *button )
{
	if ( cg_gameType.integer == GT_WOLF_LMS )
	{
		return;
	}

	BG_PanelButtonsRender_Text ( button );
}

void CG_LimboPanelRenderText_SkillsText ( panel_button_t *button )
{
	if ( cg_gameType.integer == GT_WOLF_LMS /*|| CG_LimboPanel_GetTeam() == TEAM_SPECTATOR */ )
	{
		return;
	}

	BG_PanelButtonsRender_Text ( button );
}

#define MAX_ROLLERS       8
#define COUNTER_ROLLTOTAL ( cg.time - button->data[ 4 ] )
// Gordon: this function is mental, i love it :)
void CG_LimboPanel_RenderCounter ( panel_button_t *button )
{
	float     x, w;
	float     count[ MAX_ROLLERS ];
	int       i, j;
	qhandle_t shaderBack;
	qhandle_t shaderRoll;
	int       numimages;

	float     counter_rolltime = CG_LimboPanel_RenderCounter_RollTimeForButton ( button );
	int       num = CG_LimboPanel_RenderCounter_NumRollers ( button );
	int       value = CG_LimboPanel_RenderCounter_ValueForButton ( button );

	if ( num > MAX_ROLLERS )
	{
		num = MAX_ROLLERS;
	}

	CG_LimboPanel_RenderCounter_GetShaders ( button, &shaderBack, &shaderRoll, &numimages );

	if ( COUNTER_ROLLTOTAL < counter_rolltime )
	{
		// we're rolling
		float frac = ( COUNTER_ROLLTOTAL / counter_rolltime );

		for ( i = 0, j = 1; i < num; i++, j *= numimages )
		{
			int valueOld = ( button->data[ 3 ] / j ) % numimages;
			int valueNew = ( button->data[ 5 ] / j ) % numimages;

			if ( valueNew == valueOld )
			{
				count[ i ] = valueOld;
			}
			else if ( ( valueNew > valueOld ) != ( button->data[ 5 ] > button->data[ 3 ] ) )
			{
				// we're flipping around so....
				if ( button->data[ 5 ] > button->data[ 3 ] )
				{
					count[ i ] = valueOld + frac;
				}
				else
				{
					count[ i ] = valueOld - frac;
				}
			}
			else
			{
				// normal flip
				count[ i ] = valueOld + ( ( valueNew - valueOld ) * frac );
			}
		}
	}
	else
	{
		if ( button->data[ 3 ] != button->data[ 5 ] )
		{
			button->data[ 3 ] = button->data[ 5 ];
		}
		else if ( value != button->data[ 3 ] )
		{
			int maxchange = abs ( value - button->data[ 3 ] );

			if ( maxchange > CG_LimboPanel_RenderCounter_MaxChangeForButton ( button ) )
			{
				maxchange = CG_LimboPanel_RenderCounter_MaxChangeForButton ( button );
			}

			if ( value > button->data[ 3 ] )
			{
				if ( CG_LimboPanel_RenderCounter_CountsUp ( button ) )
				{
					button->data[ 5 ] = button->data[ 3 ] + maxchange;
				}
				else
				{
//                  button->data[3] =
					button->data[ 5 ] = value;
				}
			}
			else
			{
				if ( CG_LimboPanel_RenderCounter_CountsDown ( button ) )
				{
					button->data[ 5 ] = button->data[ 3 ] - maxchange;
				}
				else
				{
//                  button->data[3] =
					button->data[ 5 ] = value;
				}
			}

			button->data[ 4 ] = cg.time;
		}

		for ( i = 0, j = 1; i < num; i++, j *= numimages )
		{
			count[ i ] = ( int ) ( button->data[ 3 ] / j );
		}
	}

	x = button->rect.x;
	w = button->rect.w / ( float ) num;

	if ( CG_LimboPanel_RenderCounter_IsReversed ( button ) )
	{
		for ( i = 0; i < num; i++ )
		{
			CG_LimboPanel_RenderCounterNumber ( x, button->rect.y, w, button->rect.h, count[ i ], shaderBack, shaderRoll, numimages );

			x += w + button->data[ 6 ];
		}
	}
	else
	{
		for ( i = num - 1; i >= 0; i-- )
		{
			CG_LimboPanel_RenderCounterNumber ( x, button->rect.y, w, button->rect.h, count[ i ], shaderBack, shaderRoll, numimages );

			x += w + button->data[ 6 ];
		}
	}

	if ( button->data[ 0 ] == 0 || button->data[ 0 ] == 1 )
	{
		CG_DrawPic ( button->rect.x - 2, button->rect.y - 2, button->rect.w * 1.4f, button->rect.h + 7,
		             cgs.media.limboCounterBorder );
	}
}

void CG_LimboPanel_Setup ( void )
{
	panel_button_t   *button;
	panel_button_t   **buttons = limboPanelButtons;
	clientInfo_t     *ci = &cgs.clientinfo[ cg.clientNum ];
	bg_playerclass_t *classinfo;
	int              i;
	char             buffer[ 256 ];

	cgs.limboLoadoutModified = qfalse;

//  if( !cgs.playedLimboMusic ) {
//      trap_S_StartBackgroundTrack( "sound/music/menu_briefing.wav", "", 0 );
//      cgs.playedLimboMusic = qtrue;
//  }

	trap_Cvar_VariableStringBuffer ( "name", buffer, 256 );
	trap_Cvar_Set ( "limboname", buffer );

	if ( cgs.ccLayers )
	{
		cgs.ccSelectedLayer = CG_CurLayerForZ ( ( int ) cg.predictedPlayerEntity.lerpOrigin[ 2 ] );
	}

	for ( ; *buttons; buttons++ )
	{
		button = ( *buttons );

		if ( button->onDraw == CG_LimboPanel_RenderCounter )
		{
			if ( CG_LimboPanel_RenderCounter_StartSet ( button ) )
			{
				button->data[ 3 ] = button->data[ 5 ] = CG_LimboPanel_RenderCounter_ValueForButton ( button );
				button->data[ 4 ] = 0;
			}
		}
	}

	if ( !cgs.limboLoadoutSelected )
	{
		bg_playerclass_t *classInfo = CG_LimboPanel_GetPlayerClass();

		for ( i = 0; i < MAX_WEAPS_PER_CLASS; i++ )
		{
			if ( !classInfo->classWeapons[ i ] )
			{
				cgs.ccSelectedWeapon = 0;
				break;
			}

			if ( classInfo->classWeapons[ i ] == cgs.clientinfo[ cg.clientNum ].latchedweapon )
			{
				cgs.ccSelectedWeapon = i;
				break;
			}
		}

		if ( cgs.ccSelectedWeapon2 >= CG_LimboPanel_WeaponCount_ForSlot ( 0 ) )
		{
			cgs.ccSelectedWeapon2 = CG_LimboPanel_WeaponCount_ForSlot ( 0 ) - 1;
		}

		for ( i = 0; i < 3; i++ )
		{
			if ( teamOrder[ i ] == ci->team )
			{
				cgs.ccSelectedTeam = i;
			}
		}

		if ( ci->team != TEAM_SPECTATOR )
		{
			cgs.ccSelectedClass = ci->cls;
		}
	}

	CG_LimboPanel_RequestWeaponStats();
	cgs.ccRequestedObjective = cgs.ccSelectedObjective = CG_LimboPanel_GetMaxObjectives();
	CG_LimboPanel_RequestObjective();

	cgs.ccSelectedObjective = CG_LimboPanel_GetMaxObjectives();
	cgs.ccSelectedWeaponNumber = 1;

	classinfo = CG_LimboPanel_GetPlayerClass();

	if ( CG_LimboPanel_WeaponIsDisabled ( cgs.ccSelectedWeapon ) )
	{
		// set weapon to default if disabled
		// NOTE classWeapons[0] must NEVER be disabled
		cgs.ccSelectedWeapon = 0; //classinfo->classWeapons[0];
	}
}

void CG_LimboPanel_Init ( void )
{
	BG_PanelButtonsSetup ( limboPanelButtons );
}

qboolean CG_LimboPanel_Draw ( void )
{
	static panel_button_t *lastHighlight;
	panel_button_t        *hilight;

//  panel_button_t** buttons = limboPanelButtons;

	hilight = BG_PanelButtonsGetHighlightButton ( limboPanelButtons );

	if ( hilight && hilight != lastHighlight )
	{
		lastHighlight = hilight;
//      SOUND_FOCUS;
	}

	if ( cg.limboEndCinematicTime > cg.time )
	{
		//% CG_DrawPic( LIMBO_3D_X, LIMBO_3D_Y, LIMBO_3D_W, LIMBO_3D_H, cgs.media.limboRadioBroadcast );
		CG_DrawPic ( LIMBO_3D_X + 4, LIMBO_3D_Y - 8, LIMBO_3D_W - 8, LIMBO_3D_W - 8, cgs.media.limboRadioBroadcast );
	}

	BG_PanelButtonsRender ( limboPanelButtons );

	trap_R_SetColor ( NULL );
	CG_DrawPic ( cgDC.cursorx, cgDC.cursory, 32, 32, cgs.media.cursorIcon );

	if ( cgs.ccRequestedObjective != -1 )
	{
		if ( cg.time - cgs.ccLastObjectiveRequestTime > 1000 )
		{
			if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
			{
				if ( cgs.ccCurrentCamObjective != -1 || cgs.ccPortalEnt != -1 )
				{
					CG_LimboPanel_RequestObjective();
				}
			}
			else
			{
				if ( ( cgs.ccRequestedObjective == cgs.ccSelectedObjective &&
				       ( cgs.ccCurrentCamObjective != cgs.ccRequestedObjective || cgs.ccPortalEnt != -1 ) ) )
				{
					if ( !
					     ( cgs.ccRequestedObjective == CG_LimboPanel_GetMaxObjectives() && cgs.ccCurrentCamObjective == -1 &&
					       cgs.ccPortalEnt == -1 ) )
					{
						CG_LimboPanel_RequestObjective();
					}
				}
			}
		}
	}

	return qtrue;
}

void CG_LimboPanel_KeyHandling ( int key, qboolean down )
{
	int b1, b2;

	if ( BG_PanelButtonsKeyEvent ( key, down, limboPanelButtons ) )
	{
		return;
	}

	if ( down )
	{
		cgDC.getKeysForBinding ( "openlimbomenu", &b1, &b2 );

		if ( ( b1 != -1 && b1 == key ) || ( b2 != -1 && b2 == key ) )
		{
			CG_EventHandling ( CGAME_EVENT_NONE, qfalse );
			return;
		}
	}

	if ( down && key )
	{
		if ( CG_CommandCentreSpawnPointClick() )
		{
			return;
		}
	}
}

void CG_LimboPanel_GetWeaponCardIconData ( weapon_t weap, qhandle_t *shader, float *w, float *h, float *s0, float *t0, float *s1,
    float *t1 )
{
	// setup the shader
	switch ( weap )
	{
		case WP_MORTAR:
		case WP_PANZERFAUST:
		case WP_FLAMETHROWER:
		case WP_FG42:
		case WP_MOBILE_MG42:
		case WP_MP40:
		case WP_STEN:
		case WP_THOMPSON:
			*shader = cgs.media.limboWeaponCard1;
			break;

		case WP_COLT:
		case WP_LUGER:
		case WP_AKIMBO_COLT:
		case WP_AKIMBO_LUGER:
		case WP_AKIMBO_SILENCEDCOLT:
		case WP_AKIMBO_SILENCEDLUGER:
		case WP_SILENCED_COLT:
		case WP_SILENCER:
		case WP_CARBINE:
		case WP_GARAND:
		case WP_KAR98:
		case WP_K43:
			*shader = cgs.media.limboWeaponCard2;
			break;

		default: // shouldn't happen
			*shader = 0;
	}

	// setup s co-ords
	switch ( weap )
	{
		case WP_SILENCED_COLT:
		case WP_SILENCER:
		case WP_LUGER:
		case WP_COLT:
			*s0 = 0;
			*s1 = 0.5f;
			break;

		default:
			*s0 = 0;
			*s1 = 1;
			break;
	}

	// setup t co-ords
	switch ( weap )
	{
		case WP_AKIMBO_SILENCEDLUGER:
		case WP_SILENCER:
		case WP_MORTAR:
			*t0 = 0 / 8.f;
			*t1 = 1 / 8.f;
			break;

		case WP_AKIMBO_SILENCEDCOLT:
		case WP_SILENCED_COLT:
		case WP_PANZERFAUST:
			*t0 = 1 / 8.f;
			*t1 = 2 / 8.f;
			break;

		case WP_LUGER:
		case WP_AKIMBO_LUGER:
		case WP_FLAMETHROWER:
			*t0 = 2 / 8.f;
			*t1 = 3 / 8.f;
			break;

		case WP_AKIMBO_COLT:
		case WP_COLT:
		case WP_FG42:
			*t0 = 3 / 8.f;
			*t1 = 4 / 8.f;
			break;

		case WP_CARBINE:
		case WP_MOBILE_MG42:
			*t0 = 4 / 8.f;
			*t1 = 5 / 8.f;
			break;

		case WP_KAR98:
		case WP_MP40:
			*t0 = 5 / 8.f;
			*t1 = 6 / 8.f;
			break;

		case WP_K43:
		case WP_STEN:
			*t0 = 6 / 8.f;
			*t1 = 7 / 8.f;
			break;

		case WP_GARAND:
		case WP_THOMPSON:
			*t0 = 7 / 8.f;
			*t1 = 8 / 8.f;
			break;

		default: // shouldn't happen
			*t0 = 0.0;
			*t1 = 1.0;
			break;
	}

	*h = 1.f;

	switch ( weap )
	{
		case WP_SILENCED_COLT:
		case WP_SILENCER:
		case WP_COLT:
		case WP_LUGER:
			*w = 0.5f;
			break;

		default:
			*w = 1.f;
			break;
	}
}

// Gordon: Utility funcs
team_t CG_LimboPanel_GetTeam ( void )
{
	return teamOrder[ cgs.ccSelectedTeam ];
}

team_t CG_LimboPanel_GetRealTeam ( void )
{
	return cgs.clientinfo[ cg.clientNum ].team == TEAM_SPECTATOR ? CG_LimboPanel_GetTeam() : cgs.clientinfo[ cg.clientNum ].team;
}

int CG_LimboPanel_GetClass ( void )
{
	return cgs.ccSelectedClass;
}

bg_character_t *CG_LimboPanel_GetCharacter ( void )
{
	return BG_GetCharacter ( CG_LimboPanel_GetTeam(), CG_LimboPanel_GetClass() );
}

bg_playerclass_t *CG_LimboPanel_GetPlayerClass ( void )
{
	return BG_GetPlayerClassInfo ( CG_LimboPanel_GetTeam(), CG_LimboPanel_GetClass() );
}

int CG_LimboPanel_WeaponCount ( void )
{
	return CG_LimboPanel_WeaponCount_ForSlot ( cgs.ccSelectedWeaponNumber );
}

int CG_LimboPanel_WeaponCount_ForSlot ( int number )
{
	if ( number == 1 )
	{
		bg_playerclass_t *classInfo = CG_LimboPanel_GetPlayerClass();
		int              cnt = 0, i;

		for ( i = 0; i < MAX_WEAPS_PER_CLASS; i++ )
		{
			if ( !classInfo->classWeapons[ i ] )
			{
				break;
			}

			cnt++;
		}

		return cnt;
	}
	else
	{
		if ( cgs.clientinfo[ cg.clientNum ].skill[ SK_HEAVY_WEAPONS ] >= 4 && CG_LimboPanel_GetClass() == PC_SOLDIER )
		{
			if ( cgs.clientinfo[ cg.clientNum ].skill[ SK_LIGHT_WEAPONS ] >= 4 )
			{
				return 3;
			}
			else
			{
				return 2;
			}
		}
		else
		{
			if ( cgs.clientinfo[ cg.clientNum ].skill[ SK_LIGHT_WEAPONS ] >= 4 )
			{
				return 2;
			}
			else
			{
				return 1;
			}
		}
	}
}

int CG_LimboPanel_GetWeaponNumberForPos ( int pos )
{
	int i, cnt = 0;

	if ( cgs.ccSelectedWeaponNumber == 0 )
	{
		return pos;
	}

	if ( pos < 0 || pos > CG_LimboPanel_WeaponCount() )
	{
		return 0;
	}

	for ( i = 0; i <= pos; i++ )
	{
		while ( CG_LimboPanel_WeaponIsDisabled ( i + cnt ) )
		{
			cnt++;
		}
	}

	return pos + cnt;
}

weapon_t CG_LimboPanel_GetWeaponForNumber ( int number, int slot, qboolean ignoreDisabled )
{
	bg_playerclass_t *classInfo;

	if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
	{
		return WP_NONE;
	}

	classInfo = CG_LimboPanel_GetPlayerClass();

	if ( !classInfo )
	{
		return WP_NONE;
	}

	if ( slot == 1 )
	{
		if ( !ignoreDisabled && CG_LimboPanel_WeaponIsDisabled ( number ) )
		{
			if ( !number )
			{
				CG_Error ( "ERROR: Class weapon 0 disabled\n" );
				return WP_NONE;
			}
			else
			{
				return classInfo->classWeapons[ 0 ];
			}
		}

		return classInfo->classWeapons[ number ];
	}
	else
	{
		if ( cgs.clientinfo[ cg.clientNum ].skill[ SK_HEAVY_WEAPONS ] >= 4 && CG_LimboPanel_GetClass() == PC_SOLDIER )
		{
			if ( cgs.clientinfo[ cg.clientNum ].skill[ SK_LIGHT_WEAPONS ] >= 4 )
			{
				if ( number == 2 )
				{
					return CG_LimboPanel_GetTeam() == TEAM_AXIS ? WP_MP40 : WP_THOMPSON;
				}
			}
			else
			{
				if ( number == 1 )
				{
					return CG_LimboPanel_GetTeam() == TEAM_AXIS ? WP_MP40 : WP_THOMPSON;
				}
			}
		}

		if ( cgs.clientinfo[ cg.clientNum ].skill[ SK_LIGHT_WEAPONS ] >= 4 )
		{
			if ( number >= 1 )
			{
				if ( CG_LimboPanel_GetClass() == PC_COVERTOPS )
				{
					return CG_LimboPanel_GetTeam() == TEAM_AXIS ? WP_AKIMBO_SILENCEDLUGER : WP_AKIMBO_SILENCEDCOLT;
				}
				else
				{
					return CG_LimboPanel_GetTeam() == TEAM_AXIS ? WP_AKIMBO_LUGER : WP_AKIMBO_COLT;
				}
			}
		}

		if ( number == 0 )
		{
			if ( CG_LimboPanel_GetClass() == PC_COVERTOPS )
			{
				return CG_LimboPanel_GetTeam() == TEAM_AXIS ? WP_SILENCER : WP_SILENCED_COLT;
			}
			else
			{
				return CG_LimboPanel_GetTeam() == TEAM_AXIS ? WP_LUGER : WP_COLT;
			}
		}

		return 0;
	}
}

weapon_t CG_LimboPanel_GetSelectedWeaponForSlot ( int index )
{
	return CG_LimboPanel_GetWeaponForNumber ( index == 1 ? cgs.ccSelectedWeapon : cgs.ccSelectedWeapon2, index, qfalse );
}

void CG_LimboPanel_SetSelectedWeaponNumForSlot ( int index, int number )
{
	if ( index == 0 )
	{
		cgs.ccSelectedWeapon = number;
	}
	else
	{
		cgs.ccSelectedWeapon2 = number;
	}
}

weapon_t CG_LimboPanel_GetSelectedWeapon ( void )
{
	return CG_LimboPanel_GetWeaponForNumber ( CG_LimboPanel_GetSelectedWeaponNum(), cgs.ccSelectedWeaponNumber, qfalse );
}

int CG_LimboPanel_GetSelectedWeaponNum ( void )
{
	if ( !cgs.ccSelectedWeaponNumber )
	{
		return cgs.ccSelectedWeapon2;
	}

	if ( CG_LimboPanel_WeaponIsDisabled ( cgs.ccSelectedWeapon ) )
	{
		CG_LimboPanel_SetSelectedWeaponNumForSlot ( 0, 0 );
	}

	return cgs.ccSelectedWeapon;
}

void CG_LimboPanel_RequestWeaponStats ( void )
{
	extWeaponStats_t weapStat = CG_LimboPanel_GetSelectedWeaponStat();

	if ( weapStat == WS_MAX )
	{
		// Bleh?
		return;
	}

	trap_SendClientCommand ( va ( "ws %i", weapStat ) );
}

void CG_LimboPanel_RequestObjective ( void )
{
	int max = CG_LimboPanel_GetMaxObjectives();

	if ( cgs.ccSelectedObjective != max && CG_LimboPanel_GetTeam() != TEAM_SPECTATOR )
	{
		trap_SendClientCommand ( va ( "obj %i", cgs.ccSelectedObjective ) );
	}
	else
	{
		trap_SendClientCommand ( va ( "obj %i", -1 ) );
	}

	cgs.ccRequestedObjective = cgs.ccSelectedObjective;
	cgs.ccLastObjectiveRequestTime = cg.time;
}

void CG_LimboPanel_SetSelectedWeaponNum ( int number )
{
	if ( cgs.ccSelectedWeaponNumber == 1 )
	{
		if ( !CG_LimboPanel_WeaponIsDisabled ( number ) )
		{
			cgs.ccSelectedWeapon = number;
		}
	}
	else
	{
		cgs.ccSelectedWeapon2 = number;
	}

	CG_LimboPanel_RequestWeaponStats();
}

extWeaponStats_t CG_LimboPanel_GetSelectedWeaponStat ( void )
{
	return BG_WeapStatForWeapon ( CG_LimboPanel_GetSelectedWeapon() );
}

int CG_LimboPanel_TeamCount ( weapon_t weap )
{
	int i, cnt;

	if ( weap == -1 )
	{
		// we aint checking for a weapon, so always include ourselves
		cnt = 1;
	}
	else
	{
		// we ARE checking for a weapon, so ignore ourselves
		cnt = 0;
	}

	for ( i = 0; i < MAX_CLIENTS; i++ )
	{
		if ( i == cg.clientNum )
		{
			continue;
		}

		if ( !cgs.clientinfo[ i ].infoValid )
		{
			continue;
		}

		if ( cgs.clientinfo[ i ].team != CG_LimboPanel_GetTeam() )
		{
			continue;
		}

		if ( weap != -1 )
		{
			if ( cgs.clientinfo[ i ].weapon != weap && cgs.clientinfo[ i ].latchedweapon != weap )
			{
				continue;
			}
		}

		cnt++;
	}

	return cnt;
}

qboolean CG_IsHeavyWeapon ( weapon_t weap )
{
	int i;

	for ( i = 0; i < NUM_HEAVY_WEAPONS; i++ )
	{
		if ( bg_heavyWeapons[ i ] == weap )
		{
			return qtrue;
		}
	}

	return qfalse;
}

qboolean CG_LimboPanel_WeaponIsDisabled ( int index )
{
	bg_playerclass_t *classinfo;
	int              count, wcount;

	if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
	{
		return qtrue;
	}

	classinfo = CG_LimboPanel_GetPlayerClass();

	if ( !CG_IsHeavyWeapon ( classinfo->classWeapons[ index ] ) )
	{
		return qfalse;
	}

	count = CG_LimboPanel_TeamCount ( -1 );
	wcount = CG_LimboPanel_TeamCount ( classinfo->classWeapons[ index ] );

	if ( wcount >= ceil ( count * cgs.weaponRestrictions ) )
	{
		return qtrue;
	}

	return qfalse;
}

qboolean CG_LimboPanel_RealWeaponIsDisabled ( weapon_t weap )
{
	int count, wcount;

	if ( CG_LimboPanel_GetTeam() == TEAM_SPECTATOR )
	{
		return qtrue;
	}

	if ( !CG_IsHeavyWeapon ( weap ) )
	{
		return qfalse;
	}

	count = CG_LimboPanel_TeamCount ( -1 );
	wcount = CG_LimboPanel_TeamCount ( weap );

	if ( wcount >= ceil ( count * cgs.weaponRestrictions ) )
	{
		return qtrue;
	}

	return qfalse;
}
