#define TITLE_BACKGROUND \
    background "ui/assets/mainmenu.jpg" \
    style WINDOW_STYLE_SHADER \
    aspectBias ASPECT_NONE

#define TITLE_LAYOUT \
    itemDef \
    { \
      name background \
      type ITEM_TYPE_TEXT \
      style WINDOW_STYLE_SHADER \
      rect 439 376 182 140 \
      background "ui/assets/dretch1.png" \
      visible MENU_TRUE \
      decoration \
      nostretch \
    } \
    itemDef \
    { \
      name background \
      type ITEM_TYPE_TEXT \
      style WINDOW_STYLE_SHADER \
      rect 439 376 182 140 \
      background "ui/assets/dretch1.png" \
      visible MENU_TRUE \
      decoration \
      nostretch \
    } \
    itemDef \
    { \
      name background \
      type ITEM_TYPE_TEXT \
      style WINDOW_STYLE_SHADER \
      rect 97 341 246 139 \
      background "ui/assets/dretch2.png" \
      visible MENU_TRUE \
      decoration \
      nostretch \
    } \
    itemDef \
    { \
      name background \
      type ITEM_TYPE_TEXT \
      style WINDOW_STYLE_SHADER \
      rect 404 350 75 51 \
      background "ui/assets/dretch3.png" \
      visible MENU_TRUE \
      decoration \
      nostretch \
    } \
    itemDef \
    { \
      name background \
      type ITEM_TYPE_TEXT \
      style WINDOW_STYLE_SHADER \
      rect 223 199 400 320 \
      background "ui/assets/human.png" \
      visible MENU_TRUE \
      decoration \
      nostretch \
    }

#define MMBUTTON( TXT, ACTION, POS ) \
    itemDef \
    { \
      name mainmenu \
      text TXT \
      type ITEM_TYPE_BUTTON \
      style WINDOW_STYLE_SHADER \
      textstyle ITEM_TEXTSTYLE_PLAIN \
      rect X (Y+(POS*ELEM_H)+(POS*ELEM_O)) W ELEM_H \
      textscale .416 \
      textalign ALIGN_CENTER \
      forecolor .12 .93 .99 1 \
      visible MENU_TRUE \
      textaligny ALIGNY \
      background "ui/assets/button.png" \
      action { play "sound/misc/menu1.wav"; ACTION } \
      onFocus \
      { \
		setbackground "ui/assets/button_highlight.png" \
      } \
      leaveFocus \
      { \
		setbackground "ui/assets/button.png" \
      } \
      nostretch \
    }