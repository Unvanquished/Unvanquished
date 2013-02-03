#define CVAR( NAME, TEXT, CVR, LENGTH, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_EDITFIELD \
      style WINDOW_STYLE_EMPTY \
      text TEXT \
      cvar CVR \
      maxchars LENGTH \
      rect CONTENT_X (CONTENT_Y+(POS*ELEM_H)) CONTENT_W ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx CONTENT_OFF \
      textscale .25 \
      visible MENU_FALSE \
    }

#define MULTI( NAME, TEXT, CVR, LIST, ACTION, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_MULTI \
      text TEXT \
      cvar CVR \
      cvarFloatList LIST \
      rect CONTENT_X (CONTENT_Y+(POS*ELEM_H)) CONTENT_W ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx CONTENT_OFF \
      textscale .25 \
      visible MENU_FALSE \
	  action { ACTION } \
    }

#define COMBO( NAME, TEXT, FEEDER, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_TEXT \
      text TEXT \
      rect CONTENT_X (CONTENT_Y+(POS*ELEM_H)) (CONTENT_W/2) ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textscale .25 \
      visible MENU_TRUE \
      decoration \
    } \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      rect (CONTENT_X+(CONTENT_W/2)+RESCOMBO_OFF) (CONTENT_Y+(POS*ELEM_H)) ((SCONTENT_W/2)-(2*RESCOMBO_OFF)) ELEM_H \
      type ITEM_TYPE_COMBOBOX \
      style WINDOW_STYLE_FILLED \
      elementwidth ((CONTENT_W/2)-(2*BORDER)) \
      elementheight ELEM_H \
      dropitems 5 \
      textscale .25 \
      elementtype LISTBOX_TEXT \
      feeder FEEDER \
      border WINDOW_BORDER_FULL \
      borderColor 1 1 1 1 \
      backcolor MENU_TEAL_TRANS \
      outlinecolor 0.1 0.1 0.1 0.5 \
      visible MENU_TRUE \
      doubleclick \
      { \
          play "sound/misc/menu1.wav"; \
      } \
    }

#define SLIDER( NAME, TEXT, CVR, FIRST, DEFAULT, LAST, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_SLIDER \
      text TEXT \
      cvarfloat CVR FIRST DEFAULT LAST  \
      rect CONTENT_X (CONTENT_Y+(POS*ELEM_H)) CONTENT_W ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx CONTENT_OFF \
      textscale .25 \
      visible MENU_FALSE \
    }

#define YESNO( NAME, TEXT, CVR, ACTION, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_YESNO \
      text TEXT \
      cvar CVR \
      rect CONTENT_X (CONTENT_Y+(POS*ELEM_H)) CONTENT_W ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx CONTENT_OFF \
      textscale .25 \
      visible MENU_FALSE \
	  action { ACTION } \
    }

#define BIND( NAME, TEXT, CVR, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_BIND \
      text TEXT \
      cvar CVR \
      rect CONTENT_X (SCONTENT_Y+(POS*ELEM_H)) CONTENT_W ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx CONTENT_OFF \
      textscale .25 \
      visible MENU_FALSE \
      action \
      { \
        play "sound/misc/menu1.wav"; \
      } \
    }

#define BUTTON( NAME, TEXT, ACTION, POS ) \
    itemDef \
    { \
      name NAME \
	  text TEXT \
      group optionsGrp \
      style WINDOW_STYLE_EMPTY \
      rect CONTENT_X (CONTENT_Y+(POS*ELEM_H)) CONTENT_W ELEM_H \
      type ITEM_TYPE_BUTTON \
      textalign ALIGN_CENTER \
      textvalign VALIGN_CENTER \
      textscale .3 \
      visible MENU_FALSE \
      action { ACTION } \
    }

//System (+65 x)

#define MULTIX( NAME, TEXT, CVR, LIST, ACTION, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_MULTI \
      text TEXT \
      cvar CVR \
      cvarStrList { LIST } \
      rect SCONTENT_X (SCONTENT_Y+(POS*ELEM_H)) SCONTENT_W ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx SCONTENT_OFF \
      textscale .25 \
      visible MENU_FALSE \
	  action { ACTION } \
    }

#define COMBOX( NAME, TEXT, FEEDER, POS) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_TEXT \
      text TEXT \
      rect SCONTENT_X (SCONTENT_Y+(POS*ELEM_H)) (SCONTENT_W/2) ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textscale .25 \
      visible MENU_TRUE \
    } \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      rect (SCONTENT_X+(SCONTENT_W/2)+RESCOMBO_OFF) (SCONTENT_Y+(POS*ELEM_H)) ((SCONTENT_W/2)-(2*RESCOMBO_OFF)) ELEM_H \
      type ITEM_TYPE_COMBOBOX \
      style WINDOW_STYLE_FILLED \
      elementwidth ((SCONTENT_W/2)-(2*BORDER)) \
      elementheight ELEM_H \
      dropitems 5 \
      textscale .25 \
      elementtype LISTBOX_TEXT \
      feeder FEEDER \
      border WINDOW_BORDER_FULL \
      borderColor 1 1 1 1 \
      backcolor MENU_TEAL_TRANS \
      outlinecolor 0.1 0.1 0.1 0.5 \
      visible MENU_TRUE \
      doubleclick \
      { \
        play "sound/misc/menu1.wav"; \
      } \
    }

#define SLIDERX( NAME, TEXT, CVR, FIRST, DEFAULT, LAST, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_SLIDER \
      text TEXT \
      cvarfloat CVR FIRST DEFAULT LAST  \
      rect SCONTENT_X (SCONTENT_Y+(POS*ELEM_H)) SCONTENT_W ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx SCONTENT_OFF \
      textscale .25 \
      visible MENU_FALSE \
    }

#define YESNOX( NAME, TEXT, CVR, ACTION, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_YESNO \
      text TEXT \
      cvar CVR \
      rect SCONTENT_X (SCONTENT_Y+(POS*ELEM_H)) SCONTENT_W ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx SCONTENT_OFF \
      textscale .25 \
      visible MENU_FALSE \
	  action { ACTION } \
    }

#define BINDX( NAME, TEXT, CVR, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_BIND \
      text TEXT \
      cvar CVR \
      rect SCONTENT_X (SCONTENT_Y+(POS*ELEM_H)) SCONTENT_W ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx SCONTENT_OFF \
      textscale .25 \
      visible MENU_FALSE \
      action \
      { \
        play "sound/misc/menu1.wav"; \
      } \
    }

#define BUTTONX( NAME, TEXT, ACTION, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      type ITEM_TYPE_BUTTON \
      text TEXT \
      textscale .25 \
      style WINDOW_STYLE_EMPTY \
      rect SCONTENT_X (SCONTENT_Y+(POS*ELEM_H)) SCONTENT_W ELEM_H \
      textalign ALIGN_CENTER \
      textvalign VALIGN_CENTER \
      visible MENU_FALSE \
      action { ACTION } \
    }

#define TEXTX( NAME, TEXT, POS ) \
    itemDef \
    { \
      name NAME \
      group optionsGrp \
      style WINDOW_STYLE_FILLED \
      type ITEM_TYPE_TEXT \
      text TEXT \
      rect SCONTENT_X (SCONTENT_Y+(POS*ELEM_H)) SCONTENT_W ELEM_H \
      textalign ALIGN_CENTER \
      textvalign VALIGN_CENTER \
      textscale .25 \
      visible MENU_FALSE \
      decoration \
      action \
      { \
        play "sound/misc/menu1.wav"; \
      } \
    }
