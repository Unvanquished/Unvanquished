#define X             0
#define Y             35
#define W             250
#define H             ((10*(ELEM_H+ELEM_GAP))+120)
#define TOFF_X        (0-(W/2))
#define ELEM_H        16
#define ELEM_GAP      4
#define BUTT_W        35
#define BUTT_H        35
#define BORDER        10
#define RESCOMBO_OFF  8

#define CVAR( NAME, CVR, POS, LENGTH ) \
	itemDef \
	{ \
	type ITEM_TYPE_EDITFIELD \
	style WINDOW_STYLE_EMPTY \
	text NAME \
	cvar CVR \
	maxchars LENGTH \
	rect X (Y+(POS*(ELEM_H+ELEM_GAP))) W ELEM_H \
	textalign ALIGN_RIGHT \
	textalignx TOFF_X \
	textvalign VALIGN_CENTER \
	textscale .25 \
	forecolor 1 1 1 1 \
	visible MENU_TRUE \
	}

#define MULTI( TEXT, CVR, LIST, ACTION, POS ) \
	itemDef \
	{ \
	type ITEM_TYPE_MULTI \
	text TEXT \
	cvar CVR \
	cvarFloatList LIST \
	rect X (Y+(POS*(ELEM_H+ELEM_GAP))) W ELEM_H \
	textalign ALIGN_RIGHT \
	textalignx TOFF_X \
	textvalign VALIGN_CENTER \
	textscale .25 \
	forecolor 1 1 1 1 \
	visible MENU_TRUE \
	action { ACTION } \
	}
	
#define COMBO( TEXT, FEEDER, POS ) \
	itemDef \
	{ \
		type ITEM_TYPE_TEXT \
		text TEXT \
		rect X (Y+(POS*(ELEM_H+ELEM_GAP))) (W/2) ELEM_H \
		textalign ALIGN_RIGHT \
		textvalign VALIGN_CENTER \
		textscale .25 \
		forecolor 1 1 1 1 \
		visible MENU_TRUE \
		decoration \
	} \
	itemDef \
	{ \
		rect ((W/2)+RESCOMBO_OFF) (Y+(POS*(ELEM_H+ELEM_GAP))) ((W/2)-(2*BORDER)) ELEM_H \
		type ITEM_TYPE_COMBOBOX \
		style WINDOW_STYLE_FILLED \
		elementwidth ((W/2)-(2*BORDER)) \
		elementheight ELEM_H \
		dropitems 5 \
		textscale .25 \
		elementtype LISTBOX_TEXT \
		feeder FEEDER \
		border WINDOW_BORDER_FULL \
		borderColor 1 1 1 1 \
		forecolor 1 1 1 1 \
		backcolor MENU_TEAL_TRANS \
		outlinecolor 0.1 0.1 0.1 0.5 \
		visible MENU_TRUE \
		doubleclick \
		{ \
		play "sound/misc/menu1.wav"; \
		} \
	}

#define SLIDER( TEXT, CVR, FIRST, DEFAULT, LAST, POS ) \
itemDef \
{ \
	type ITEM_TYPE_SLIDER \
	text TEXT \
	cvarfloat CVR FIRST DEFAULT  LAST \
	rect X (Y+(POS*(ELEM_H+ELEM_GAP))) W ELEM_H \
	textalign ALIGN_RIGHT \
	textalignx TOFF_X \
	textvalign VALIGN_CENTER \
	textscale .25 \
	forecolor 1 1 1 1 \
	visible MENU_TRUE \
}

#define YESNO( TEXT, CVR, ACTION, POS ) \
itemDef \
{ \
	type ITEM_TYPE_YESNO \
	text TEXT \
	cvar CVR \
	rect X (Y+(POS*(ELEM_H+ELEM_GAP))) W ELEM_H \
	textalign ALIGN_RIGHT \
	textalignx TOFF_X \
	textvalign VALIGN_CENTER \
	textscale .25 \
	forecolor 1 1 1 1 \
	visible MENU_TRUE \
	action { ACTION } \
}
