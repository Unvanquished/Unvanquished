#define COMMON_HUD_R 1.0
#define COMMON_HUD_G 0.0
#define COMMON_HUD_B 0.0
#define ALIEN_RED .686 .003 0
#define ALIEN_BG  0.8  0.1  0.1
#include "ui/tremulous_common_hud.h"

//////////////////
//STATIC OBJECTS//
//////////////////

//BACKGROUND
itemDef
{
	name "rightbg"
	rect 436 376 204 104
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	decoration
	forecolor ALIEN_BG 0.75
	style WINDOW_STYLE_SHADER
	background "ui/assets/khaos/right.tga"
}

itemDef
{
	name "leftbg"
	rect 0 376 204 104
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor ALIEN_BG 0.75
	style WINDOW_STYLE_SHADER
	background "ui/assets/khaos/left.tga"
}


///////////////////
//DYNAMIC OBJECTS//
///////////////////


//WALLCLIMB
itemDef
{
	name "wallclimb"
	rect 297 410 45 20
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.5
	backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0
	background "ui/assets/khaos/wallwalk.tga"
	ownerdraw CG_PLAYER_WALLCLIMBING
}


//BOOSTED BACKGROUND

itemDef
{
	name "boosted1"
	rect 488 445 152 18
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	decoration
	forecolor ALIEN_RED  1
	background "ui/assets/human/buildstat/health.tga"
	ownerdraw CG_PLAYER_BOOSTED_METER
}

itemDef
{
	name "boosted"
    rect 610 433 25 30
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	decoration
	forecolor 1 1 1  0.7
	backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0
	background "ui/assets/khaos/skull.tga"
	ownerdraw CG_PLAYER_BOOSTED
}

//HEALTH BAR
itemDef
{
	name "healthbar"
	rect 0 448 152 18
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor ALIEN_RED 1
	backColor 0.51 1 .93 .32
	background "ui/assets/human/buildstat/health.tga"
	ownerdraw CG_PLAYER_HEALTH_BAR
}

//CROSS
itemDef
{
	name "cross"
	rect 0 446.5 25 25
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor 1 1 1 0.5
	ownerdraw CG_PLAYER_HEALTH_CROSS
}

// //SMALL LOWER CROSS
// itemDef
// {
// 	name "cross2"
// 	rect 279 449 15 15
// 	aspectBias ALIGN_CENTER
// 	visible MENU_TRUE
// 	decoration
// 	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
// 	backColor 0 0 0 0
// 	background "ui/assets/neutral/cross.tga"
// 	style WINDOW_STYLE_SHADER
// }
//
// //SMALL UPPER CROSS
// itemDef
// {
// 	name "cross3"
// 	rect 283 437 15 15
// 	aspectBias ALIGN_CENTER
// 	visible MENU_TRUE
// 	decoration
// 	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
// 	backColor 0 0 0 0
// 	background "ui/assets/neutral/cross.tga"
// 	style WINDOW_STYLE_SHADER
// }


//ALIEN CLASS ICON
itemDef
{
	name "alien-icon"
	rect 297 432 45 45
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.5
	ownerdraw CG_PLAYER_WEAPONICON
}

//ORGANS
itemDef
{
	name "organs"
	rect 500 463 15 15
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	textScale 0.32
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	ownerdraw CG_PLAYER_ALIEN_EVOS
}

itemDef
    {
      name "creditstext"
      type ITEM_TYPE_TEXT
      text "Evolution Points"
      style WINDOW_STYLE_EMPTY
	aspectBias ALIGN_RIGHT
      textstyle ITEM_TEXTSTYLE_PLAIN
      rect 530 462.5 101 15
      textscale .32
      textalign ALIGN_LEFT
     forecolor 1 1 1 1
     visible MENU_TRUE
      decoration
}

//ALIENSENSE
itemDef
{
	name "aliensense"
	rect 20 20 600 400
	visible MENU_TRUE
	decoration
	ownerdraw CG_PLAYER_ALIEN_SENSE
}

//CHARGE BAR
itemDef
{
	name "charge"
	rect 0 427 152 18
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor ALIEN_RED 1
	ownerdraw CG_PLAYER_CHARGE_BAR
	background "ui/assets/human/buildstat/health.tga"
}

// //CHARGE BAR BG
// itemDef
// {
// 	name "chargebg"
// 	rect 288 422 64 16
// 	aspectBias ALIGN_CENTER
// 	visible MENU_TRUE
// 	decoration
// 	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.25
// 	ownerdraw CG_PLAYER_CHARGE_BAR_BG
// 	background "ui/assets/neutral/charge_bg_h.tga"
// }

//TEAM OVERLAY
itemDef
{
	name "teamoverlay"
	rect BORDER 175 250 128
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.5
	textscale 0.85
	ownerdraw CG_TEAMOVERLAY
}
//STAGE REPORT
itemDef
{
	name stagereport
	textalign ALIGN_LEFT
	textvalign VALIGN_BOTTOM
    textscale .29
    textalign ALIGN_LEFT
    forecolor 1 1 1  1
	rect 3 462 200 15
	decoration
	visible MENU_TRUE
	ownerdraw CG_STAGE_REPORT_TEXT
}

//CROSSHAIR HEALTH METER
itemDef
{
	name "crosshairhealth"
	rect 305 250 30 10
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
	decoration
	forecolor .48 .02 .03 .3
	cvarTest "hud_crosshairbars"
	showCvar {1}
	background "ui/assets/neutral/crescent_bottom.tga"
	ownerdraw CG_PLAYER_HEALTH_METER
}

//HEALTH
itemDef
{
	name "health"
	rect 40 450.75 50 13
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	textalign ALIGN_CENTER
	decoration
	forecolor 1 1 1 1
	ownerdraw CG_PLAYER_HEALTH
}



