#define COMMON_HUD_R 1.0
#define COMMON_HUD_G 0.0
#define COMMON_HUD_B 0.0
#include "ui/AlienCentered/tremulous_common_hud.h"

//////////////////
//STATIC OBJECTS//
//////////////////

//BACKGROUND
itemDef
{
	name "background"
	rect 223 403 201 150
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
	decoration
	forecolor .48 .02 .03 1
	style WINDOW_STYLE_SHADER
	background "ui/assets/alien/background.tga"
}



///////////////////
//DYNAMIC OBJECTS//
///////////////////


//WALLCLIMB
itemDef
{
	name "wallclimb"
	rect 222 411 194 66
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.5
	backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0
	background "ui/assets/alien/wallclimb.tga"
	ownerdraw CG_PLAYER_WALLCLIMBING
}


//BOOSTED BACKGROUND
itemDef
{
	name "boostedbg"
        rect 10 385 40 66
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B  0.4
	backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0
	background "ui/assets/alien/poisonbg.tga"
	ownerdraw CG_PLAYER_BOOSTED
}

//BOOSTED
itemDef
{
	name "boosted"
    rect 10 385 40 66
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B  0.1
	backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0
	background "ui/assets/alien/poison.tga"
	ownerdraw CG_PLAYER_BOOSTED
}

itemDef
{
	name "boosted1"
	rect 10 385 40 66
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B  0.7
	background "ui/assets/alien/poison.tga"
	ownerdraw CG_PLAYER_BOOSTED_METER
}



//HEALTH BAR
itemDef
{
	name "healthbar"
	rect 266 445 104 13
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
	decoration
	forecolor .48 .02 .03 1
	backColor 0 0 0 0
	background "ui/assets/human/buildstat/health.tga"
	ownerdraw CG_PLAYER_HEALTH_BAR
}

//HEALTH
itemDef
{
	name "health"
	rect 277 444 50 13
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
      textalign ALIGN_CENTER
	decoration
	forecolor .75 .26 .25 1
	ownerdraw CG_PLAYER_HEALTH
}



//CROSS
itemDef
{
	name "cross"
	rect 260 442 24 24
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.5
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
	rect 299 402 40 40
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
	rect 256 460 15 15
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
	textScale 0.4
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	ownerdraw CG_PLAYER_ALIEN_EVOS
}

itemDef
    {
      name "creditstext"
      type ITEM_TYPE_TEXT
      text "evolution points"
      style WINDOW_STYLE_EMPTY
	aspectBias ALIGN_CENTER
      textstyle ITEM_TEXTSTYLE_PLAIN
      rect 283 458 101 15
      textscale .35
      textalign ALIGN_LEFT
     forecolor .51 .51 .51 1
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
	rect 266 445 104 13
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.5
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
	textvalign VALIGN_TOP
    textscale .35
    textalign ALIGN_LEFT
    forecolor .37 .16 .17  1
	rect 3 462 200 30
    forecolor .51 .51 .51 1
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

