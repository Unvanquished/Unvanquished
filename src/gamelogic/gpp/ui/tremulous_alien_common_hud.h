#define COMMON_HUD_R 1.0
#define COMMON_HUD_G 0.0
#define COMMON_HUD_B 0.0
#include "ui/tremulous_common_hud.h"

//////////////////
//STATIC OBJECTS//
//////////////////

//LEFT RING CIRCLE
itemDef
{
  name "left-ring-circle"
  rect 47.5 410 25 25
  aspectBias ALIGN_LEFT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.25
  style WINDOW_STYLE_SHADER
  background "ui/assets/neutral/circle.tga"
}

//LEFT ARM
itemDef
{
  name "left-arm"
  rect 77 404.75 104 52.5
  aspectBias ALIGN_LEFT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.25
  style WINDOW_STYLE_SHADER
  background "ui/assets/alien/left-arm.tga"
}

//LEFT ARM CIRCLE
itemDef
{
  name "left-arm-circle"
  rect 150 417.5 25 25
  aspectBias ALIGN_LEFT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.25
  style WINDOW_STYLE_SHADER
  background "ui/assets/neutral/circle.tga"
}

//RIGHT RING CIRCLE
itemDef
{
  name "right-ring-circle"
  rect 567 410 25 25
  aspectBias ALIGN_RIGHT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.25
  style WINDOW_STYLE_SHADER
  background "ui/assets/neutral/circle.tga"
}

//RIGHT ARM
itemDef
{
  name "right-arm"
  rect 459 404.75 104 52.5
  aspectBias ALIGN_RIGHT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.25
  style WINDOW_STYLE_SHADER
  background "ui/assets/alien/right-arm.tga"
}

///////////////////
//DYNAMIC OBJECTS//
///////////////////

//BOLT
itemDef
{
  name "bolt"
  rect 52.5 412.5 15 20
  aspectBias ALIGN_LEFT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.8
  backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.2
  background "ui/assets/alien/bolt.tga"
  ownerdraw CG_PLAYER_BOOST_BOLT
}

//CROSS
itemDef
{
  name "cross"
  rect 150 417.5 25 25
  aspectBias ALIGN_LEFT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.5
  ownerdraw CG_PLAYER_HEALTH_CROSS
}

//LEFT RING
itemDef
{
  name "left-ring"
  rect 7.25 369.5 90.5 106
  aspectBias ALIGN_LEFT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.8
  backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.2
  background "ui/assets/alien/left-ring.tga"
  ownerdraw CG_PLAYER_BOOSTED
}

//LEFT SPIKES
itemDef
{
  name "left-spikes"
  rect 18.5 381 59 83
  aspectBias ALIGN_LEFT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1.0
  backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.2
  background "ui/assets/alien/left-spikes.tga"
  ownerdraw CG_PLAYER_WALLCLIMBING
}

//RIGHT RING
itemDef
{
  name "right-ring"
  rect 542.25 369.5 90.5 106
  aspectBias ALIGN_RIGHT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.8
  backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.2
  background "ui/assets/alien/right-ring.tga"
  ownerdraw CG_PLAYER_BOOSTED
}

//RIGHT SPIKES
itemDef
{
  name "right-spikes"
  rect 562.5 381 59 83
  aspectBias ALIGN_RIGHT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1.0
  backcolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.2
  background "ui/assets/alien/right-spikes.tga"
  ownerdraw CG_PLAYER_WALLCLIMBING
}

//HEALTH
itemDef
{
  name "health"
  rect 78.5 421.5 60 15
  aspectBias ALIGN_LEFT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B .5
  ownerdraw CG_PLAYER_HEALTH
}

//ALIEN CLASS ICON
itemDef
{
  name "alien-icon"
  rect 465 417.5 25 25
  aspectBias ALIGN_RIGHT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.6
  ownerdraw CG_PLAYER_WEAPONICON
}

//ORGANS
itemDef
{
  name "organs"
  rect 570 416 15 15
  aspectBias ALIGN_RIGHT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 1
  ownerdraw CG_PLAYER_CREDITS_VALUE_NOPAD
}

//CREDITS FRACTION
itemDef
{
  name "credits-background"
  rect 567 410 25 25
  aspectBias ALIGN_RIGHT
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.25
  background "ui/assets/neutral/circle.tga"
  ownerdraw CG_PLAYER_CREDITS_FRACTION
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
  rect 292 426 56 8
  aspectBias ALIGN_CENTER
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.5
  ownerdraw CG_PLAYER_CHARGE_BAR
  background "ui/assets/neutral/charge_cap_h.tga"
}

//CHARGE BAR BG
itemDef
{
  name "chargebg"
  rect 288 422 64 16
  aspectBias ALIGN_CENTER
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.25
  ownerdraw CG_PLAYER_CHARGE_BAR_BG
  background "ui/assets/neutral/charge_bg_h.tga"
}

//TEAM OVERLAY
itemDef
{
  name "teamoverlay"
  rect BORDER 175 200 128
  style WINDOW_STYLE_EMPTY
  visible MENU_TRUE
  decoration
  forecolor COMMON_HUD_R COMMON_HUD_G COMMON_HUD_B 0.75
  textscale 0.85
  ownerdraw CG_TEAMOVERLAY
}
