#define BORDER    10
#define MINIMAP_SIZE 128

#define STAT_W    45
#define STAT_H    22
#define STAT_X    ( W - ( BORDER + STAT_W ))
#define STAT_X_MINIMAP    ( (STAT_X) - (MINIMAP_SIZE + STAT_W) )

#define CONSOLE_W ( W - (( 3 * BORDER ) + STAT_W ))
#define CONSOLE_H 180
#define MAIN_W    ( W - ( 2 * BORDER ))

////////////////////////////////////
////MINIMAP OVERLAPPING ELEMENTS////
////////////////////////////////////

//FPS
itemDef
{
	name "fps"
	rect STAT_X BORDER STAT_W STAT_H
	aspectBias ALIGN_RIGHT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	textalign ALIGN_RIGHT
	textvalign VALIGN_CENTER
	textscale 0.3
	textstyle ITEM_TEXTSTYLE_PLAIN
	ownerdraw CG_FPS
	cvartest cg_minimapActive
	showCvar {0}
	hideCvar {1}
}
itemDef
{
	name "fps"
	rect STAT_X_MINIMAP BORDER STAT_W STAT_H
	aspectBias ALIGN_RIGHT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	textalign ALIGN_RIGHT
	textvalign VALIGN_CENTER
	textscale 0.3
	textstyle ITEM_TEXTSTYLE_PLAIN
	ownerdraw CG_FPS
	cvartest cg_minimapActive
	showCvar {1}
	hideCvar {0}
}

//TIMER
itemDef
{
	name "timer"
	rect STAT_X( ( 2 * BORDER ) + STAT_H ) STAT_W STAT_H
	aspectBias ALIGN_RIGHT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	textalign ALIGN_RIGHT
	textvalign VALIGN_CENTER
	textscale 0.3
	textstyle ITEM_TEXTSTYLE_PLAIN
	ownerdraw CG_TIMER
	cvartest cg_minimapActive
	showCvar {0}
	hideCvar {1}
}
itemDef
{
	name "timer"
	rect STAT_X_MINIMAP( ( 2 * BORDER ) + STAT_H ) STAT_W STAT_H
	aspectBias ALIGN_RIGHT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	textalign ALIGN_RIGHT
	textvalign VALIGN_CENTER
	textscale 0.3
	textstyle ITEM_TEXTSTYLE_PLAIN
	ownerdraw CG_TIMER
	cvartest cg_minimapActive
	showCvar {1}
	hideCvar {0}
}

//LAGOMETER
itemDef
{
	name "lagometer"
	rect STAT_X( ( 3 * BORDER ) + ( 2 * STAT_H ) ) STAT_W STAT_H
	aspectBias ALIGN_RIGHT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	textscale 0.3
	ownerdraw CG_LAGOMETER
	cvartest cg_minimapActive
	showCvar {0}
	hideCvar {1}
}
itemDef
{
	name "lagometer"
	rect STAT_X_MINIMAP( ( 3 * BORDER ) + ( 2 * STAT_H ) ) STAT_W STAT_H
	aspectBias ALIGN_RIGHT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	textscale 0.3
	ownerdraw CG_LAGOMETER
	cvartest cg_minimapActive
	showCvar {1}
	hideCvar {0}
}

//CLOCK
itemDef
{
	name "clock"
	rect STAT_X( ( 4 * BORDER ) + ( 3 * STAT_H ) ) STAT_W STAT_H
	aspectBias ALIGN_RIGHT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	textalign ALIGN_RIGHT
	textvalign VALIGN_CENTER
	textscale 0.3
	textstyle ITEM_TEXTSTYLE_PLAIN
	ownerdraw CG_CLOCK
	cvartest cg_minimapActive
	showCvar {0}
	hideCvar {1}
}
itemDef
{
	name "clock"
	rect STAT_X_MINIMAP( ( 4 * BORDER ) + ( 3 * STAT_H ) ) STAT_W STAT_H
	aspectBias ALIGN_RIGHT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	textalign ALIGN_RIGHT
	textvalign VALIGN_CENTER
	textscale 0.3
	textstyle ITEM_TEXTSTYLE_PLAIN
	ownerdraw CG_CLOCK
	cvartest cg_minimapActive
	showCvar {1}
	hideCvar {0}
}



//MINIMAP
itemDef
{
  name "minimap"
  rect ( 640 - MINIMAP_SIZE ) 0 MINIMAP_SIZE MINIMAP_SIZE
  visible MENU_TRUE
  decoration
  ownerdraw CG_MINIMAP
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
}

//MINIMAP BORDER
itemDef
{
  name "minimap border"
  rect ( 639 - MINIMAP_SIZE ) -1 (MINIMAP_SIZE + 2) (MINIMAP_SIZE + 2)
  visible MENU_TRUE
  decoration
  bordercolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.35
  borderSize 1
  border WINDOW_BORDER_FULL
  cvartest cg_minimapActive
  showCvar {1}
  hideCvar {0}
}

//CONSOLE
itemDef
{
	name "console"
	rect BORDER BORDER CONSOLE_W CONSOLE_H
	aspectBias ALIGN_LEFT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor 0.93 0.93 0.92 1
	textalign ALIGN_LEFT
	textvalign VALIGN_TOP
	textscale 0.3
	textstyle ITEM_TEXTSTYLE_SHADOWED
	ownerdraw CG_CONSOLE
}

//TUTORIAL
itemDef
{
	name "tutorial"
	rect BORDER 275 MAIN_W 130
	aspectBias ALIGN_LEFT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor 1 1 1 0.35
	textalign ALIGN_LEFT
	textvalign VALIGN_CENTER
	textscale 0.3
	textstyle ITEM_TEXTSTYLE_PLAIN
	ownerdraw CG_TUTORIAL
}

//SPEEDOMETER
itemDef
{
	name "speedometer"
	rect( 320 - ( STAT_W / 2 ) ) 350 STAT_W STAT_H
	aspectBias ALIGN_CENTER
	type ITEM_TYPE_OWNERDRAW
	ownerdraw CG_SPEEDOMETER
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	backColor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.2
	foreColor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.8
	textscale 0.3
}

//LOCATION
itemDef
{
	name "location"
	rect( STAT_X - 75 )( ( 5 * BORDER ) + ( 4 * STAT_H ) )( STAT_W + 75 ) STAT_H
	aspectBias ALIGN_RIGHT
	visible MENU_TRUE
	decoration
	forecolor 1 1 1 1
	textalign ALIGN_RIGHT
	textscale 0.3
	ownerdraw CG_PLAYER_LOCATION
}

//DEMO STATE
itemDef
{
	name "demoRecording"
	rect( STAT_X + ( STAT_W - 32 ) )( ( 8 * BORDER ) + ( 4 * STAT_H ) ) 32 32
	aspectBias ALIGN_RIGHT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor 1 0 0 1
	textscale 0.3
	ownerdraw CG_DEMO_RECORDING
	background "ui/assets/neutral/circle"
}
itemDef
{
	name "demoPlayback"
	rect( STAT_X + ( STAT_W - 32 ) )( ( 8 * BORDER ) + ( 4 * STAT_H ) ) 32 32
	aspectBias ALIGN_RIGHT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor 1 1 1 1
	textscale 0.3
	ownerdraw CG_DEMO_PLAYBACK
	background "ui/assets/forwardarrow"
}

//SNAPSHOT
itemDef
{
	name "snapshot"
	rect BORDER( H - ( BORDER + STAT_H ) ) MAIN_W STAT_H
	aspectBias ALIGN_LEFT
	style WINDOW_STYLE_EMPTY
	visible MENU_TRUE
	decoration
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
	textalign ALIGN_LEFT
	textvalign VALIGN_CENTER
	textscale 0.4
	textstyle ITEM_TEXTSTYLE_PLAIN
	ownerdraw CG_SNAPSHOT
}

//PLAYER NAME
itemDef
{
	name "playername"
	rect 200 275 240 25
	aspectBias ALIGN_CENTER
	visible MENU_TRUE
	decoration
	textScale .5
	ownerdraw CG_PLAYER_CROSSHAIRNAMES
	textstyle ITEM_TEXTSTYLE_SHADOWED
}

//CROSSHAIR
itemDef
{
	name "crosshair"
	visible MENU_TRUE
	decoration
	rect 320 240 0 0
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B .35
	ownerdraw CG_PLAYER_CROSSHAIR
}

//CROSSHAIR INDICATOR
itemDef
{
	name "crosshairIndicator"
	visible MENU_TRUE
	decoration
	rect 320 240 0 0
	forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B .35
	ownerdraw CG_PLAYER_CROSSHAIR_INDICATOR
}

//SPECTATOR TEXT
itemDef
{
	name "followtext"
	rect 200 375 240 25
	foreColor 1 1 1 1
	aspectBias ALIGN_CENTER
	textalign ALIGN_CENTER
	textvalign VALIGN_TOP
	visible MENU_TRUE
	decoration
	textScale .7
	textStyle ITEM_TEXTSTYLE_SHADOWED
	ownerdraw CG_FOLLOW
}
