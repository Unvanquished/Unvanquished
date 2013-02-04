#define _CVAR( TYPE, NAME, TEXT, CVR, LENGTH, POS ) \
    itemDef \
    { \
      name NAME \
      type TYPE \
      text TEXT \
      cvar CVR \
      maxchars LENGTH \
      rect (OPTIONS_X+BORDER) (OPTIONS_Y+ELEM_OFF_Y+(POS*ELEM_H)) (OPTIONS_W-(2*BORDER)) ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx ELEM_OFF_X \
      textscale .36 \
      visible MENU_TRUE \
    }

#define CVAR( NAME, TEXT, CVR, LENGTH, POS ) \
       _CVAR( ITEM_TYPE_EDITFIELD, NAME, TEXT, CVR, LENGTH, POS )

#define CVAR_INT( NAME, TEXT, CVR, LENGTH, POS ) \
       _CVAR( ITEM_TYPE_NUMERICFIELD, NAME, TEXT, CVR, LENGTH, POS )

#define MULTI( NAME, TEXT, CVR, LIST, ACTION, POS ) \
    itemDef \
    { \
      name NAME \
      type ITEM_TYPE_MULTI \
      text TEXT \
      cvar CVR \
      cvarFloatList LIST \
      rect (OPTIONS_X+BORDER) (OPTIONS_Y+ELEM_OFF_Y+(POS*ELEM_H)) (OPTIONS_W-(2*BORDER)) ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx ELEM_OFF_X \
      textscale .36 \
      visible MENU_TRUE \
          action { ACTION } \
    }

#define YESNO( NAME, TEXT, CVR, ACTION, POS ) \
    itemDef \
    { \
      name NAME \
      type ITEM_TYPE_YESNO \
      text TEXT \
      cvar CVR \
      rect (OPTIONS_X+BORDER) (OPTIONS_Y+ELEM_OFF_Y+(POS*ELEM_H)) (OPTIONS_W-(2*BORDER)) ELEM_H \
      textalign ALIGN_RIGHT \
      textvalign VALIGN_CENTER \
      textalignx ELEM_OFF_X \
      textscale .36 \
      visible MENU_TRUE \
          action { ACTION } \
    }

