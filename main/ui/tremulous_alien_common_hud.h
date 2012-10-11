#define COMMON_HUD_R 1.0
#define COMMON_HUD_G 0.0
#define COMMON_HUD_B 0.0
#include "ui/tremulous_common_hud.h"

//////////////////
//STATIC OBJECTS//
//////////////////

//BACKGROUND
itemDef
{
	name "background"
	rect -24 373 258 127
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor .48 .02 .03 .3
	style WINDOW_STYLE_SHADER
	background "ui/assets/alien/hbg.tga"
}

itemDef
{
	name "health"
	rect -3 399 179 83
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor .48 .02 .03 .3
	ownerdraw CG_PLAYER_HEALTH_METER
	background "ui/assets/alien/healt.tga"
}

itemDef
{
	name "background"
	rect 406 373 258 127
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	decoration
	forecolor .48 .02 .03 .5
	style WINDOW_STYLE_SHADER
	background "ui/assets/alien/pbg.tga"
}

itemDef
{
	name "poison"
	rect 464 399 179 83
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	decoration
	forecolor .39 0 .09 .7	
ownerdraw CG_PLAYER_BOOSTED_METER
	background "ui/assets/alien/poisonbg.tga"
}
///////////////////
//DYNAMIC OBJECTS//
///////////////////


//WALLCLIMB
itemDef
{
	name "leftsmall"
	rect -16 358 251 85
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0
	backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	background "ui/assets/alien/l-spikes-small.png"
	ownerdraw CG_PLAYER_WALLCLIMBING
}

//WALLCLIMB
itemDef
{
	name "leftbig"
	rect -20 336 255 107
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0
	background "ui/assets/alien/l-spikes-big.png"
	ownerdraw CG_PLAYER_WALLCLIMBING
}

//WALLCLIMB
itemDef
{
	name "rightsmall"
	rect 405 358 251 85
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0
	backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	background "ui/assets/alien/r-spikes-small.png"
	ownerdraw CG_PLAYER_WALLCLIMBING
}

//WALLCLIMB
itemDef
{
	name "rightbig"
	rect 405 336 255 107
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0
	background "ui/assets/alien/r-spikes-big.png"
	ownerdraw CG_PLAYER_WALLCLIMBING
}


itemDef
{
	name "background"
	rect -24 394 209 96
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor 1 1 1 .9
	style WINDOW_STYLE_SHADER
	background "ui/assets/alien/ll.png"
}

itemDef
{
	name "background"
	rect 455 394 209 96
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	decoration
	forecolor 1 1 1 .9
	style WINDOW_STYLE_SHADER
	background "ui/assets/alien/rl.tga"
}


//BOOSTED
itemDef
{
	name "boosted"
    rect 450 409 29 47
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	decoration
	forecolor .54 .11 .17 .7
	backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0
	background "ui/assets/alien/poison.tga"
	ownerdraw CG_PLAYER_BOOSTED
}

//HEALTH
itemDef
{
	name "health"
	rect 30 431 70 20
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
      textalign ALIGN_RIGHT
	decoration
	forecolor .94 .13 .1  1
	ownerdraw CG_PLAYER_HEALTH
}



//CROSS
itemDef
{
	name "cross"
	rect 105 428 30 30
	aspectBias ALIGN_LEFT
	visible MENU_TRUE
	decoration
	forecolor .94 .13 .1  .5
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
	rect 568 422 40 40
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	decoration
	forecolor .87 .01 .07 .5
	ownerdraw CG_PLAYER_WEAPONICON
}

//ORGANS
itemDef
{
	name "organs"
	rect 511 460 15 15
	aspectBias ALIGN_RIGHT
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
	aspectBias ALIGN_RIGHT
      textstyle ITEM_TEXTSTYLE_PLAIN
      rect 536 460 101 15
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
	rect  150 460 340 20
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
	decoration
	forecolor .48 .02 .03 .5
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
	rect BORDER 175 200 128
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
    textalign ALIGN_CENTER
    forecolor .73 .73 .73 1
	rect 220 462 200 30
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

