/*
===========================================================================

Wolfenstein: Enemy Territory GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company. 

This file is part of the Wolfenstein: Enemy Territory GPL Source Code (Wolf ET Source Code).  

Wolf ET Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Wolf ET Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Wolf ET Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Wolf: ET Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU General Public License which accompanied the Wolf ET Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

#define WINDOW_FUI( WINDOW_TEXT, GRADIENT_START_OFFSET )														\
	itemDef {																									\
		name		"window"																					\
		group		GROUP_NAME																					\
		rect		0 0 WINDOW_WIDTH WINDOW_HEIGHT																\
		style		WINDOW_STYLE_FILLED																			\
		backcolor	0 0 0 .2																					\
		border		WINDOW_BORDER_FULL																			\
		bordercolor	.5 .5 .5 .5																					\
		visible		1																							\
		decoration																								\
	}																											\
																												\
	itemDef {																									\
		name		"titlebar"																					\
		group		GROUP_NAME																					\
		rect		2 2 GRADIENT_START_OFFSET 24																\
		style		WINDOW_STYLE_FILLED																			\
		backcolor	.16 .2 .17 .8																				\
		visible		1																							\
		decoration																								\
	}																											\
																												\
	itemDef {																									\
		name		"titlebargradient"																			\
		group		GROUP_NAME																					\
		rect		$evalint(GRADIENT_START_OFFSET+2) 2 $evalint(WINDOW_WIDTH-(GRADIENT_START_OFFSET+4)) 24		\
		style		WINDOW_STYLE_GRADIENT																		\
		backcolor	.16 .2 .17 .8																				\
		visible		1																							\
		decoration																								\
	}																											\
																												\
	itemDef {																									\
		name		"windowtitle"																				\
		group		GROUP_NAME																					\
		rect		2 2 $evalint(WINDOW_WIDTH-4) 24																\
		text		WINDOW_TEXT																					\
		textfont	UI_FONT_ARIBLK_27																			\
		textscale	.4																							\
		textalignx	3																							\
		textaligny	20																							\
		forecolor	.6 .6 .6 1																					\
		border		WINDOW_BORDER_FULL																			\
		bordercolor	.1 .1 .1 .2																					\
		visible		1																							\
		decoration																								\
	}

#define WINDOW_INGAME( WINDOW_TEXT, GRADIENT_START_OFFSET )														\
	itemDef {																									\
		name		"window"																					\
		group		GROUP_NAME																					\
		rect		0 0 WINDOW_WIDTH WINDOW_HEIGHT																\
		style		WINDOW_STYLE_FILLED																			\
		backcolor	0 0 0 .6																					\
		border		WINDOW_BORDER_FULL																			\
		bordercolor	.5 .5 .5 .5																					\
		visible		1																							\
		decoration																								\
	}																											\
																												\
	itemDef {																									\
		name		"titlebar"																					\
		group		GROUP_NAME																					\
		rect		2 2 GRADIENT_START_OFFSET 24																\
		style		WINDOW_STYLE_FILLED																			\
		backcolor	.16 .2 .17 .8																				\
		visible		1																							\
		decoration																								\
	}																											\
																												\
	itemDef {																									\
		name		"titlebargradient"																			\
		group		GROUP_NAME																					\
		rect		$evalint(GRADIENT_START_OFFSET+2) 2 $evalint(WINDOW_WIDTH-(GRADIENT_START_OFFSET+4)) 24		\
		style		WINDOW_STYLE_GRADIENT																		\
		backcolor	.16 .2 .17 .8																				\
		visible		1																							\
		decoration																								\
	}																											\
																												\
	itemDef {																									\
		name		"windowtitle"																				\
		group		GROUP_NAME																					\
		rect		2 2 $evalint(WINDOW_WIDTH-4) 24																\
		text		WINDOW_TEXT																					\
		textfont	UI_FONT_ARIBLK_27																			\
		textscale	.4																							\
		textalignx	3																							\
		textaligny	20																							\
		forecolor	.6 .6 .6 1																					\
		border		WINDOW_BORDER_FULL																			\
		bordercolor	.1 .1 .1 .2																					\
		visible		1																							\
		decoration																								\
	}

#ifdef FUI
#define WINDOW WINDOW_FUI
#else
#define WINDOW WINDOW_INGAME
#endif

#define SUBWINDOW( SUBWINDOW_X, SUBWINDOW_Y, SUBWINDOW_W, SUBWINDOW_H, SUBWINDOW_TEXT )	\
	itemDef {																									\
		name		"subwindow"##SUBWINDOW_TEXT																	\
		group		GROUP_NAME																					\
		rect		$evalfloat(SUBWINDOW_X) $evalfloat(SUBWINDOW_Y) $evalfloat(SUBWINDOW_W) $evalfloat(SUBWINDOW_H)	\
		style		WINDOW_STYLE_FILLED																			\
		backcolor	0 0 0 .2																					\
		border		WINDOW_BORDER_FULL																			\
		bordercolor	.5 .5 .5 .5																					\
		visible		1																							\
		decoration																								\
	}																											\
																												\
	itemDef {																									\
		name		"subwindowtitle"##SUBWINDOW_TEXT															\
		group		GROUP_NAME																					\
		rect		$evalfloat((SUBWINDOW_X)+2) $evalfloat((SUBWINDOW_Y)+2) $evalfloat((SUBWINDOW_W)-4) 12		\
		text		SUBWINDOW_TEXT																				\
		textfont	UI_FONT_ARIBLK_16																			\
		textscale	.19																							\
		textalignx	3																							\
		textaligny	10																							\
		style		WINDOW_STYLE_FILLED																			\
		backcolor	.16 .2 .17 .8																				\
		forecolor	.6 .6 .6 1																					\
		visible		1																							\
		decoration																								\
	}

#define SUBWINDOWBLACK( SUBWINDOWBLACK_X, SUBWINDOWBLACK_Y, SUBWINDOWBLACK_W, SUBWINDOWBLACK_H, SUBWINDOWBLACK_TEXT )	\
	itemDef {																									\
		name		"subwindowblack"##SUBWINDOWBLACK_TEXT														\
		group		GROUP_NAME																					\
		rect		$evalfloat(SUBWINDOWBLACK_X) $evalfloat(SUBWINDOWBLACK_Y) $evalfloat(SUBWINDOWBLACK_W) $evalfloat(SUBWINDOWBLACK_H)	\
		style		WINDOW_STYLE_FILLED																			\
		backcolor	0 0 0 .85																					\
		border		WINDOW_BORDER_FULL																			\
		bordercolor	.5 .5 .5 .5																					\
		visible		1																							\
		decoration																								\
	}																											\
																												\
	itemDef {																									\
		name		"subwindowblacktitle"##SUBWINDOWBLACK_TEXT															\
		group		GROUP_NAME																					\
		rect		$evalfloat((SUBWINDOWBLACK_X)+2) $evalfloat((SUBWINDOWBLACK_Y)+2) $evalfloat((SUBWINDOWBLACK_W)-4) 12		\
		text		SUBWINDOWBLACK_TEXT																			\
		textfont	UI_FONT_ARIBLK_16																			\
		textscale	.19																							\
		textalignx	3																							\
		textaligny	10																							\
		style		WINDOW_STYLE_FILLED																			\
		backcolor	.16 .2 .17 .8																				\
		forecolor	.6 .6 .6 1																					\
		visible		1																							\
		decoration																								\
	}

#define BUTTON( BUTTON_X, BUTTON_Y, BUTTON_W, BUTTON_H, BUTTON_TEXT, BUTTON_TEXT_SCALE, BUTTON_TEXT_ALIGN_Y, BUTTON_ACTION )					\
	itemDef {															\
		name		"bttn"##BUTTON_TEXT									\
		group		GROUP_NAME											\
		rect		$evalfloat(BUTTON_X) $evalfloat(BUTTON_Y) $evalfloat(BUTTON_W) $evalfloat(BUTTON_H)					\
		type		ITEM_TYPE_BUTTON									\
		text		BUTTON_TEXT											\
		textfont	UI_FONT_COURBD_30									\
		textscale	BUTTON_TEXT_SCALE									\
		textalign	ITEM_ALIGN_CENTER									\
		textalignx	$evalfloat(0.5*(BUTTON_W))							\
		textaligny	BUTTON_TEXT_ALIGN_Y									\
		style		WINDOW_STYLE_FILLED									\
		backcolor	.3 .3 .3 .4											\
		forecolor	.6 .6 .6 1											\
		border		WINDOW_BORDER_FULL									\
		bordercolor	.1 .1 .1 .5											\
		visible		1													\
																		\
		mouseEnter {													\
			setitemcolor "bttn"##BUTTON_TEXT forecolor .9 .9 .9 1 ;		\
			setitemcolor "bttn"##BUTTON_TEXT backcolor .5 .5 .5 .4		\
		}																\
																		\
		mouseExit {														\
			setitemcolor "bttn"##BUTTON_TEXT forecolor .6 .6 .6 1 ;		\
			setitemcolor "bttn"##BUTTON_TEXT backcolor .3 .3 .3 .4		\
		}																\
																		\
		action {														\
			setitemcolor "bttn"##BUTTON_TEXT forecolor .6 .6 .6 1 ;		\
			setitemcolor "bttn"##BUTTON_TEXT backcolor .3 .3 .3 .4 ;	\
			play "sound/menu/select.wav" ;								\
			BUTTON_ACTION												\
		}																\
	}

#define BUTTONEXT( BUTTONEXT_X, BUTTONEXT_Y, BUTTONEXT_W, BUTTONEXT_H, BUTTONEXT_TEXT, BUTTONEXT_TEXT_SCALE, BUTTONEXT_TEXT_ALIGN_Y, BUTTONEXT_ACTION, BUTTONEXT_EXT )					\
	itemDef {															\
		name		"bttnext"##BUTTONEXT_TEXT							\
		group		GROUP_NAME											\
		rect		$evalfloat(BUTTONEXT_X) $evalfloat(BUTTONEXT_Y) $evalfloat(BUTTONEXT_W) $evalfloat(BUTTONEXT_H)					\
		type		ITEM_TYPE_BUTTON									\
		text		BUTTONEXT_TEXT										\
		textfont	UI_FONT_COURBD_30									\
		textscale	BUTTONEXT_TEXT_SCALE								\
		textalign	ITEM_ALIGN_CENTER									\
		textalignx	$evalfloat(0.5*(BUTTONEXT_W))						\
		textaligny	BUTTONEXT_TEXT_ALIGN_Y								\
		style		WINDOW_STYLE_FILLED									\
		backcolor	.3 .3 .3 .4											\
		forecolor	.6 .6 .6 1											\
		border		WINDOW_BORDER_FULL									\
		bordercolor	.1 .1 .1 .5											\
		visible		1													\
																		\
		mouseEnter {													\
			setitemcolor "bttnext"##BUTTONEXT_TEXT forecolor .9 .9 .9 1 ;		\
			setitemcolor "bttnext"##BUTTONEXT_TEXT backcolor .5 .5 .5 .4		\
		}																\
																		\
		mouseExit {														\
			setitemcolor "bttnext"##BUTTONEXT_TEXT forecolor .6 .6 .6 1 ;		\
			setitemcolor "bttnext"##BUTTONEXT_TEXT backcolor .3 .3 .3 .4		\
		}																\
																		\
		action {														\
			setitemcolor "bttnext"##BUTTONEXT_TEXT forecolor .6 .6 .6 1 ;		\
			setitemcolor "bttnext"##BUTTONEXT_TEXT backcolor .3 .3 .3 .4 ;	\
			play "sound/menu/select.wav" ;								\
			BUTTONEXT_ACTION											\
		}																\
																		\
		BUTTONEXT_EXT													\
	}

#define NAMEDBUTTON( NAMEDBUTTON_NAME, NAMEDBUTTON_X, NAMEDBUTTON_Y, NAMEDBUTTON_W, NAMEDBUTTON_H, NAMEDBUTTON_TEXT, NAMEDBUTTON_TEXT_SCALE, NAMEDBUTTON_TEXT_ALIGN_Y, NAMEDBUTTON_ACTION )					\
	itemDef {															\
		name		NAMEDBUTTON_NAME									\
		group		GROUP_NAME											\
		rect		$evalfloat(NAMEDBUTTON_X) $evalfloat(NAMEDBUTTON_Y) $evalfloat(NAMEDBUTTON_W) $evalfloat(NAMEDBUTTON_H)					\
		type		ITEM_TYPE_BUTTON									\
		text		NAMEDBUTTON_TEXT									\
		textfont	UI_FONT_COURBD_30									\
		textscale	NAMEDBUTTON_TEXT_SCALE								\
		textalign	ITEM_ALIGN_CENTER									\
		textalignx	$evalfloat(0.5*(NAMEDBUTTON_W))						\
		textaligny	NAMEDBUTTON_TEXT_ALIGN_Y							\
		style		WINDOW_STYLE_FILLED									\
		backcolor	.3 .3 .3 .4											\
		forecolor	.6 .6 .6 1											\
		border		WINDOW_BORDER_FULL									\
		bordercolor	.1 .1 .1 .5											\
		visible		1													\
																		\
		mouseEnter {													\
			setitemcolor NAMEDBUTTON_NAME forecolor .9 .9 .9 1 ;		\
			setitemcolor NAMEDBUTTON_NAME backcolor .5 .5 .5 .4			\
		}																\
																		\
		mouseExit {														\
			setitemcolor NAMEDBUTTON_NAME forecolor .6 .6 .6 1 ;		\
			setitemcolor NAMEDBUTTON_NAME backcolor .3 .3 .3 .4			\
		}																\
																		\
		action {														\
			setitemcolor NAMEDBUTTON_NAME forecolor .6 .6 .6 1 ;		\
			setitemcolor NAMEDBUTTON_NAME backcolor .3 .3 .3 .4 ;		\
			play "sound/menu/select.wav" ;								\
			NAMEDBUTTON_ACTION											\
		}																\
	}

#define NAMEDBUTTONEXT( NAMEDBUTTONEXT_NAME, NAMEDBUTTONEXT_X, NAMEDBUTTONEXT_Y, NAMEDBUTTONEXT_W, NAMEDBUTTONEXT_H, NAMEDBUTTONEXT_TEXT, NAMEDBUTTONEXT_TEXT_SCALE, NAMEDBUTTONEXT_TEXT_ALIGN_Y, NAMEDBUTTONEXT_ACTION, NAMEDBUTTONEXT_EXT )					\
	itemDef {															\
		name		NAMEDBUTTONEXT_NAME									\
		group		GROUP_NAME											\
		rect		$evalfloat(NAMEDBUTTONEXT_X) $evalfloat(NAMEDBUTTONEXT_Y) $evalfloat(NAMEDBUTTONEXT_W) $evalfloat(NAMEDBUTTONEXT_H)					\
		type		ITEM_TYPE_BUTTON									\
		text		NAMEDBUTTONEXT_TEXT									\
		textfont	UI_FONT_COURBD_30									\
		textscale	NAMEDBUTTONEXT_TEXT_SCALE							\
		textalign	ITEM_ALIGN_CENTER									\
		textalignx	$evalfloat(0.5*(NAMEDBUTTONEXT_W))					\
		textaligny	NAMEDBUTTONEXT_TEXT_ALIGN_Y							\
		style		WINDOW_STYLE_FILLED									\
		backcolor	.3 .3 .3 .4											\
		forecolor	.6 .6 .6 1											\
		border		WINDOW_BORDER_FULL									\
		bordercolor	.1 .1 .1 .5											\
		visible		1													\
																		\
		mouseEnter {													\
			setitemcolor NAMEDBUTTONEXT_NAME forecolor .9 .9 .9 1 ;		\
			setitemcolor NAMEDBUTTONEXT_NAME backcolor .5 .5 .5 .4		\
		}																\
																		\
		mouseExit {														\
			setitemcolor NAMEDBUTTONEXT_NAME forecolor .6 .6 .6 1 ;		\
			setitemcolor NAMEDBUTTONEXT_NAME backcolor .3 .3 .3 .4		\
		}																\
																		\
		action {														\
			setitemcolor NAMEDBUTTONEXT_NAME forecolor .6 .6 .6 1 ;		\
			setitemcolor NAMEDBUTTONEXT_NAME backcolor .3 .3 .3 .4 ;	\
			play "sound/menu/select.wav" ;								\
			NAMEDBUTTONEXT_ACTION										\
		}																\
																		\
		NAMEDBUTTONEXT_EXT												\
	}

#define EDITFIELD( EDITFIELD_X, EDITFIELD_Y, EDITFIELD_W, EDITFIELD_H, EDITFIELD_TEXT, EDITFIELD_TEXT_SCALE, EDITFIELD_TEXT_ALIGN_Y, EDITFIELD_CVAR, EDITFIELD_MAXCHARS, EDITFIELD_MAXPAINTCHARS, EDITFIELD_TOOLTIP )	\
	itemDef {															\
		name		"efback"##EDITFIELD_TEXT							\
		group		GROUP_NAME											\
		rect		$evalfloat((EDITFIELD_X)+.5*(EDITFIELD_W)+6) $evalfloat(EDITFIELD_Y) $evalfloat(.5*(EDITFIELD_W)-6) $evalfloat(EDITFIELD_H)	\
		style		WINDOW_STYLE_FILLED									\
		backcolor	.5 .5 .5 .2											\
		visible		1													\
		decoration														\
	}																	\
																		\
	itemDef {															\
		name			"ef"##EDITFIELD_TEXT							\
      	group			GROUP_NAME										\
      	rect			$evalfloat(EDITFIELD_X) $evalfloat(EDITFIELD_Y) $evalfloat(EDITFIELD_W) $evalfloat(EDITFIELD_H)	\
		type			ITEM_TYPE_EDITFIELD								\
		text			EDITFIELD_TEXT									\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		EDITFIELD_TEXT_SCALE							\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(EDITFIELD_W))					\
		textaligny		EDITFIELD_TEXT_ALIGN_Y							\
		forecolor		.6 .6 .6 1										\
		cvar			EDITFIELD_CVAR									\
		maxChars		EDITFIELD_MAXCHARS								\
		maxPaintChars	EDITFIELD_MAXPAINTCHARS							\
		visible			1												\
		tooltip			EDITFIELD_TOOLTIP								\
    }

#define EDITFIELDLEFT( EDITFIELDLEFT_X, EDITFIELDLEFT_Y, EDITFIELDLEFT_W, EDITFIELDLEFT_H, EDITFIELDLEFT_TEXT, EDITFIELDLEFT_TEXT_SCALE, EDITFIELDLEFT_TEXT_ALIGN_Y, EDITFIELDLEFT_CVAR, EDITFIELDLEFT_MAXCHARS, EDITFIELDLEFT_MAXPAINTCHARS, EDITFIELDLEFT_TOOLTIP )	\
	itemDef {															\
		name			"efleft"##EDITFIELDLEFT_TEXT					\
      	group			GROUP_NAME										\
      	rect			$evalfloat(EDITFIELDLEFT_X) $evalfloat(EDITFIELDLEFT_Y) $evalfloat(EDITFIELDLEFT_W) $evalfloat(EDITFIELDLEFT_H)	\
		type			ITEM_TYPE_EDITFIELD								\
		text			EDITFIELDLEFT_TEXT								\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		EDITFIELDLEFT_TEXT_SCALE						\
		textaligny		EDITFIELDLEFT_TEXT_ALIGN_Y						\
		forecolor		.6 .6 .6 1										\
		cvar			EDITFIELDLEFT_CVAR								\
		maxChars		EDITFIELDLEFT_MAXCHARS							\
		maxPaintChars	EDITFIELDLEFT_MAXPAINTCHARS						\
		visible			1												\
		tooltip			EDITFIELDLEFT_TOOLTIP							\
    }

#define NUMERICFIELD( NUMERICFIELD_X, NUMERICFIELD_Y, NUMERICFIELD_W, NUMERICFIELD_H, NUMERICFIELD_TEXT, NUMERICFIELD_TEXT_SCALE, NUMERICFIELD_TEXT_ALIGN_Y, NUMERICFIELD_CVAR, NUMERICFIELD_MAXCHARS, NUMERICFIELD_TOOLTIP )	\
	itemDef {															\
		name		"nfback"##NUMERICFIELD_TEXT							\
		group		GROUP_NAME											\
		rect		$evalfloat((NUMERICFIELD_X)+.5*(NUMERICFIELD_W)+6) $evalfloat(NUMERICFIELD_Y) $evalfloat(.5*(NUMERICFIELD_W)-6) $evalfloat(NUMERICFIELD_H)	\
		style		WINDOW_STYLE_FILLED									\
		backcolor	.5 .5 .5 .2											\
		visible		1													\
		decoration														\
	}																	\
																		\    
	itemDef {															\
		name			"nf"##NUMERICFIELD_TEXT							\
      	group			GROUP_NAME										\
      	rect			$evalfloat(NUMERICFIELD_X) $evalfloat(NUMERICFIELD_Y) $evalfloat(NUMERICFIELD_W) $evalfloat(NUMERICFIELD_H)	\
		type			ITEM_TYPE_NUMERICFIELD							\
		text			NUMERICFIELD_TEXT								\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		NUMERICFIELD_TEXT_SCALE							\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(NUMERICFIELD_W))				\
		textaligny		NUMERICFIELD_TEXT_ALIGN_Y						\
		forecolor		.6 .6 .6 1										\
		cvar			NUMERICFIELD_CVAR								\
		maxChars		NUMERICFIELD_MAXCHARS							\
		visible			1												\
		tooltip			NUMERICFIELD_TOOLTIP							\
    }

#define NUMERICFIELDLEFTEXT( NUMERICFIELDLEFTEXT_X, NUMERICFIELDLEFTEXT_Y, NUMERICFIELDLEFTEXT_W, NUMERICFIELDLEFTEXT_H, NUMERICFIELDLEFTEXT_TEXT, NUMERICFIELDLEFTEXT_TEXT_SCALE, NUMERICFIELDLEFTEXT_TEXT_ALIGN_Y, NUMERICFIELDLEFTEXT_CVAR, NUMERICFIELDLEFTEXT_MAXCHARS, NUMERICFIELDLEFTEXT_EXT, NUMERICFIELDLEFTEXT_TOOLTIP )	\
	itemDef {															\
		name			"nfleftext"##NUMERICFIELDLEFTEXT_TEXT			\
      	group			GROUP_NAME										\
      	rect			$evalfloat(NUMERICFIELDLEFTEXT_X) $evalfloat(NUMERICFIELDLEFTEXT_Y) $evalfloat(NUMERICFIELDLEFTEXT_W) $evalfloat(NUMERICFIELDLEFTEXT_H)	\
		type			ITEM_TYPE_NUMERICFIELD							\
		text			NUMERICFIELDLEFTEXT_TEXT						\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		NUMERICFIELDLEFTEXT_TEXT_SCALE					\
		textaligny		NUMERICFIELDLEFTEXT_TEXT_ALIGN_Y				\
		forecolor		.6 .6 .6 1										\
		cvar			NUMERICFIELDLEFTEXT_CVAR						\
		maxChars		NUMERICFIELDLEFTEXT_MAXCHARS					\
		visible			1												\
		tooltip			NUMERICFIELDLEFTEXT_TOOLTIP						\
																		\
		NUMERICFIELDLEFTEXT_EXT											\
    }

#define YESNO( YESNO_X, YESNO_Y, YESNO_W, YESNO_H, YESNO_TEXT, YESNO_TEXT_SCALE, YESNO_TEXT_ALIGN_Y, YESNO_CVAR, YESNO_TOOLTIP )	\
    itemDef {															\
		name			"yn"##YESNO_TEXT								\
      	group			GROUP_NAME										\
      	rect			$evalfloat(YESNO_X) $evalfloat(YESNO_Y) $evalfloat(YESNO_W) $evalfloat(YESNO_H)	\
		type			ITEM_TYPE_YESNO									\
		text			YESNO_TEXT										\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		YESNO_TEXT_SCALE								\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(YESNO_W))						\
		textaligny		YESNO_TEXT_ALIGN_Y								\
		forecolor		.6 .6 .6 1										\
		cvar			YESNO_CVAR										\
		visible			1												\
		tooltip			YESNO_TOOLTIP									\
																		\
		mouseEnter {													\
			setitemcolor "yn"##YESNO_TEXT forecolor .9 .9 .9 1 ;		\
		}																\
																		\
		mouseExit {														\
			setitemcolor "yn"##YESNO_TEXT forecolor .6 .6 .6 1 ;		\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
		}																\
    }

#define YESNOALIGNX( YESNOALIGNX_X, YESNOALIGNX_Y, YESNOALIGNX_W, YESNOALIGNX_H, YESNOALIGNX_TEXT, YESNOALIGNX_TEXT_SCALE, YESNOALIGNX_TEXT_ALIGN_X, YESNOALIGNX_TEXT_ALIGN_Y, YESNOALIGNX_CVAR, YESNOALIGNX_TOOLTIP )	\
    itemDef {															\
		name			"ynalx"##YESNOALIGNX_TEXT								\
      	group			GROUP_NAME										\
      	rect			$evalfloat(YESNOALIGNX_X) $evalfloat(YESNOALIGNX_Y) $evalfloat(YESNOALIGNX_W) $evalfloat(YESNOALIGNX_H)	\
		type			ITEM_TYPE_YESNO									\
		text			YESNOALIGNX_TEXT										\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		YESNOALIGNX_TEXT_SCALE								\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(YESNOALIGNX_W)+YESNOALIGNX_TEXT_ALIGN_X)						\
		textaligny		YESNOALIGNX_TEXT_ALIGN_Y								\
		forecolor		.6 .6 .6 1										\
		cvar			YESNOALIGNX_CVAR										\
		visible			1												\
		tooltip			YESNOALIGNX_TOOLTIP								\
																		\
		mouseEnter {													\
			setitemcolor "ynalx"##YESNOALIGNX_TEXT forecolor .9 .9 .9 1 ;		\
		}																\
																		\
		mouseExit {														\
			setitemcolor "ynalx"##YESNOALIGNX_TEXT forecolor .6 .6 .6 1 ;		\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
		}																\
    }

#define YESNOACTION( YESNOACTION_X, YESNOACTION_Y, YESNOACTION_W, YESNOACTION_H, YESNOACTION_TEXT, YESNOACTION_TEXT_SCALE, YESNOACTION_TEXT_ALIGN_Y, YESNOACTION_CVAR, YESNOACTION_ACTION, YESNOACTION_TOOLTIP )	\
    itemDef {															\
		name			"ynaction"##YESNOACTION_TEXT					\
      	group			GROUP_NAME										\
      	rect			$evalfloat(YESNOACTION_X) $evalfloat(YESNOACTION_Y) $evalfloat(YESNOACTION_W) $evalfloat(YESNOACTION_H)	\
		type			ITEM_TYPE_YESNO									\
		text			YESNOACTION_TEXT								\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		YESNOACTION_TEXT_SCALE							\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(YESNOACTION_W))					\
		textaligny		YESNOACTION_TEXT_ALIGN_Y						\
		forecolor		.6 .6 .6 1										\
		cvar			YESNOACTION_CVAR								\
		visible			1												\
		tooltip			YESNOACTION_TOOLTIP								\
																		\
		mouseEnter {													\
			setitemcolor "ynaction"##YESNOACTION_TEXT forecolor .9 .9 .9 1 ;	\
		}																\
																		\
		mouseExit {														\
			setitemcolor "ynaction"##YESNOACTION_TEXT forecolor .6 .6 .6 1 ;	\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
			YESNOACTION_ACTION											\
		}																\
    }

#define CHECKBOX( CHECKBOX_X, CHECKBOX_Y, CHECKBOX_W, CHECKBOX_H, CHECKBOX_TEXT, CHECKBOX_TEXT_SCALE, CHECKBOX_TEXT_ALIGN_Y, CHECKBOX_CVAR, CHECKBOX_TOOLTIP )	\
    itemDef {															\
		name			"check"##CHECKBOX_TEXT							\
      	group			GROUP_NAME										\
      	rect			$evalfloat(CHECKBOX_X) $evalfloat(CHECKBOX_Y) $evalfloat(CHECKBOX_W) $evalfloat(CHECKBOX_H)	\
		type			ITEM_TYPE_CHECKBOX								\
		text			CHECKBOX_TEXT									\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		CHECKBOX_TEXT_SCALE								\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(CHECKBOX_W))					\
		textaligny		CHECKBOX_TEXT_ALIGN_Y							\
		forecolor		.6 .6 .6 1										\
		cvar			CHECKBOX_CVAR									\
		visible			1												\
		tooltip			CHECKBOX_TOOLTIP								\
																		\
		mouseEnter {													\
			setitemcolor "check"##CHECKBOX_TEXT forecolor .9 .9 .9 1 ;	\
		}																\
																		\
		mouseExit {														\
			setitemcolor "check"##CHECKBOX_TEXT forecolor .6 .6 .6 1 ;	\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
		}																\
    }

#define CHECKBOXALIGNX( CHECKBOXALIGNX_X, CHECKBOXALIGNX_Y, CHECKBOXALIGNX_W, CHECKBOXALIGNX_H, CHECKBOXALIGNX_TEXT, CHECKBOXALIGNX_TEXT_SCALE, CHECKBOXALIGNX_TEXT_ALIGN_X, CHECKBOXALIGNX_TEXT_ALIGN_Y, CHECKBOXALIGNX_CVAR, CHECKBOXALIGNX_TOOLTIP )	\
    itemDef {															\
		name			"checkalx"##CHECKBOXALIGNX_TEXT							\
      	group			GROUP_NAME										\
      	rect			$evalfloat(CHECKBOXALIGNX_X) $evalfloat(CHECKBOXALIGNX_Y) $evalfloat(CHECKBOXALIGNX_W) $evalfloat(CHECKBOXALIGNX_H)	\
		type			ITEM_TYPE_CHECKBOX								\
		text			CHECKBOXALIGNX_TEXT									\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		CHECKBOXALIGNX_TEXT_SCALE								\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(CHECKBOXALIGNX_W)+CHECKBOXALIGNX_TEXT_ALIGN_X)					\
		textaligny		CHECKBOXALIGNX_TEXT_ALIGN_Y							\
		forecolor		.6 .6 .6 1										\
		cvar			CHECKBOXALIGNX_CVAR									\
		visible			1												\
		tooltip			CHECKBOXALIGNX_TOOLTIP							\
																		\
		mouseEnter {													\
			setitemcolor "checkalx"##CHECKBOXALIGNX_TEXT forecolor .9 .9 .9 1 ;	\
		}																\
																		\
		mouseExit {														\
			setitemcolor "checkalx"##CHECKBOXALIGNX_TEXT forecolor .6 .6 .6 1 ;	\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
		}																\
    }

#define CHECKBOXNOTEXT( CHECKBOXNOTEXT_NAME, CHECKBOXNOTEXT_X, CHECKBOXNOTEXT_Y, CHECKBOXNOTEXT_W, CHECKBOXNOTEXT_H, CHECKBOXNOTEXT_CVAR, CHECKBOXNOTEXT_TOOLTIP )	\
    itemDef {															\
		name			CHECKBOXNOTEXT_NAME								\
      	group			GROUP_NAME										\
      	rect			$evalfloat(CHECKBOXNOTEXT_X) $evalfloat(CHECKBOXNOTEXT_Y) $evalfloat(CHECKBOXNOTEXT_W) $evalfloat(CHECKBOXNOTEXT_H)	\
		type			ITEM_TYPE_CHECKBOX								\
		forecolor		.6 .6 .6 1										\
		cvar			CHECKBOXNOTEXT_CVAR								\
		visible			1												\
		tooltip			CHECKBOXNOTEXT_TOOLTIP							\
																		\
		mouseEnter {													\
			setitemcolor CHECKBOXNOTEXT_NAME forecolor .9 .9 .9 1 ;		\
		}																\
																		\
		mouseExit {														\
			setitemcolor CHECKBOXNOTEXT_NAME forecolor .6 .6 .6 1 ;		\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
		}																\
    }

#define CHECKBOXNOTEXTACTION( CHECKBOXNOTEXTACTION_NAME, CHECKBOXNOTEXTACTION_X, CHECKBOXNOTEXTACTION_Y, CHECKBOXNOTEXTACTION_W, CHECKBOXNOTEXTACTION_H, CHECKBOXNOTEXTACTION_CVAR, CHECKBOXNOTEXTACTION_ACTION, CHECKBOXNOTEXTACTION_TOOLTIP )	\
    itemDef {															\
		name			CHECKBOXNOTEXTACTION_NAME						\
      	group			GROUP_NAME										\
      	rect			$evalfloat(CHECKBOXNOTEXTACTION_X) $evalfloat(CHECKBOXNOTEXTACTION_Y) $evalfloat(CHECKBOXNOTEXTACTION_W) $evalfloat(CHECKBOXNOTEXTACTION_H)	\
		type			ITEM_TYPE_CHECKBOX								\
		forecolor		.6 .6 .6 1										\
		cvar			CHECKBOXNOTEXTACTION_CVAR						\
		visible			1												\
		tooltip			CHECKBOXNOTEXTACTION_TOOLTIP					\
																		\
		mouseEnter {													\
			setitemcolor CHECKBOXNOTEXTACTION_NAME forecolor .9 .9 .9 1 ;		\
		}																\
																		\
		mouseExit {														\
			setitemcolor CHECKBOXNOTEXTACTION_NAME forecolor .6 .6 .6 1 ;		\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
			CHECKBOXNOTEXTACTION_ACTION									\
		}																\
    }

#define CHECKBOXACTION( CHECKBOXACTION_X, CHECKBOXACTION_Y, CHECKBOXACTION_W, CHECKBOXACTION_H, CHECKBOXACTION_TEXT, CHECKBOXACTION_TEXT_SCALE, CHECKBOXACTION_TEXT_ALIGN_Y, CHECKBOXACTION_CVAR, CHECKBOXACTION_ACTION, CHECKBOXACTION_TOOLTIP )	\
    itemDef {															\
		name			"checkaction"##CHECKBOXACTION_TEXT				\
      	group			GROUP_NAME										\
      	rect			$evalfloat(CHECKBOXACTION_X) $evalfloat(CHECKBOXACTION_Y) $evalfloat(CHECKBOXACTION_W) $evalfloat(CHECKBOXACTION_H)	\
		type			ITEM_TYPE_CHECKBOX								\
		text			CHECKBOXACTION_TEXT								\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		CHECKBOXACTION_TEXT_SCALE						\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(CHECKBOXACTION_W))				\
		textaligny		CHECKBOXACTION_TEXT_ALIGN_Y						\
		forecolor		.6 .6 .6 1										\
		cvar			CHECKBOXACTION_CVAR								\
		visible			1												\
		tooltip			CHECKBOXACTION_TOOLTIP							\
																		\
		mouseEnter {													\
			setitemcolor "checkaction"##CHECKBOXACTION_TEXT forecolor .9 .9 .9 1 ;	\
		}																\
																		\
		mouseExit {														\
			setitemcolor "checkaction"##CHECKBOXACTION_TEXT forecolor .6 .6 .6 1 ;	\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
			CHECKBOXACTION_ACTION										\
		}																\
    }

#define CHECKBOXALIGNXACTION( CHECKBOXALIGNXACTION_X, CHECKBOXALIGNXACTION_Y, CHECKBOXALIGNXACTION_W, CHECKBOXALIGNXACTION_H, CHECKBOXALIGNXACTION_TEXT, CHECKBOXALIGNXACTION_TEXT_SCALE, CHECKBOXALIGNXACTION_TEXT_ALIGN_X, CHECKBOXALIGNXACTION_TEXT_ALIGN_Y, CHECKBOXALIGNXACTION_CVAR, CHECKBOXALIGNXACTION_ACTION, CHECKBOXALIGNXACTION_TOOLTIP )	\
    itemDef {															\
		name			"checkactionalx"##CHECKBOXALIGNXACTION_TEXT				\
      	group			GROUP_NAME										\
      	rect			$evalfloat(CHECKBOXALIGNXACTION_X) $evalfloat(CHECKBOXALIGNXACTION_Y) $evalfloat(CHECKBOXALIGNXACTION_W) $evalfloat(CHECKBOXALIGNXACTION_H)	\
		type			ITEM_TYPE_CHECKBOX								\
		text			CHECKBOXALIGNXACTION_TEXT								\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		CHECKBOXALIGNXACTION_TEXT_SCALE						\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(CHECKBOXALIGNXACTION_W)+CHECKBOXALIGNXACTION_TEXT_ALIGN_X)				\
		textaligny		CHECKBOXALIGNXACTION_TEXT_ALIGN_Y						\
		forecolor		.6 .6 .6 1										\
		cvar			CHECKBOXALIGNXACTION_CVAR								\
		visible			1												\
		tooltip			CHECKBOXALIGNXACTION_TOOLTIP							\
																		\
		mouseEnter {													\
			setitemcolor "checkactionalx"##CHECKBOXALIGNXACTION_TEXT forecolor .9 .9 .9 1 ;	\
		}																\
																		\
		mouseExit {														\
			setitemcolor "checkactionalx"##CHECKBOXALIGNXACTION_TEXT forecolor .6 .6 .6 1 ;	\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
			CHECKBOXALIGNXACTION_ACTION									\
		}																\
    }

#define TRICHECKBOXACTION( TRICHECKBOXACTION_X, TRICHECKBOXACTION_Y, TRICHECKBOXACTION_W, TRICHECKBOXACTION_H, TRICHECKBOXACTION_TEXT, TRICHECKBOXACTION_TEXT_SCALE, TRICHECKBOXACTION_TEXT_ALIGN_Y, TRICHECKBOXACTION_CVAR, TRICHECKBOXACTION_ACTION, TRICHECKBOXACTION_TOOLTIP )	\
    itemDef {															\
		name			"tricheckaction"##TRICHECKBOXACTION_TEXT			\
      	group			GROUP_NAME										\
      	rect			$evalfloat(TRICHECKBOXACTION_X) $evalfloat(TRICHECKBOXACTION_Y) $evalfloat(TRICHECKBOXACTION_W) $evalfloat(TRICHECKBOXACTION_H)	\
		type			ITEM_TYPE_TRICHECKBOX							\
		text			TRICHECKBOXACTION_TEXT							\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		TRICHECKBOXACTION_TEXT_SCALE					\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(TRICHECKBOXACTION_W))			\
		textaligny		TRICHECKBOXACTION_TEXT_ALIGN_Y					\
		forecolor		.6 .6 .6 1										\
		cvar			TRICHECKBOXACTION_CVAR							\
		visible			1												\
		tooltip			TRICHECKBOXACTION_TOOLTIP						\
																		\
		mouseEnter {													\
			setitemcolor "tricheckaction"##TRICHECKBOXACTION_TEXT forecolor .9 .9 .9 1 ;	\
		}																\
																		\
		mouseExit {														\
			setitemcolor "tricheckaction"##TRICHECKBOXACTION_TEXT forecolor .6 .6 .6 1 ;	\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
			TRICHECKBOXACTION_ACTION									\
		}																\
    }

#define TRICHECKBOXACTIONMULTI( TRICHECKBOXACTIONMULTI_X, TRICHECKBOXACTIONMULTI_Y, TRICHECKBOXACTIONMULTI_W, TRICHECKBOXACTIONMULTI_H, TRICHECKBOXACTIONMULTI_TEXT_SCALE, TRICHECKBOXACTIONMULTI_TEXT_ALIGN_Y, TRICHECKBOXACTIONMULTI_CVAR, TRICHECKBOXACTIONMULTI_CVARLIST, TRICHECKBOXACTIONMULTI_ACTION, TTRICHECKBOXACTIONMULTI_TOOLTIP )	\
    itemDef {															\
		name			"tricheckactionmulti"##TRICHECKBOXACTIONMULTI_CVAR		\
      	group			GROUP_NAME										\
      	rect			$evalfloat(TRICHECKBOXACTIONMULTI_X) $evalfloat(TRICHECKBOXACTIONMULTI_Y) $evalfloat(TRICHECKBOXACTIONMULTI_W) $evalfloat(TRICHECKBOXACTIONMULTI_H)	\
		type			ITEM_TYPE_TRICHECKBOX							\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		TRICHECKBOXACTIONMULTI_TEXT_SCALE				\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(TRICHECKBOXACTIONMULTI_W))		\
		textaligny		TRICHECKBOXACTIONMULTI_TEXT_ALIGN_Y				\
		forecolor		.6 .6 .6 1										\
		cvar			TRICHECKBOXACTIONMULTI_CVAR						\
		TRICHECKBOXACTIONMULTI_CVARLIST									\
		visible			1												\
		tooltip			TTRICHECKBOXACTIONMULTI_TOOLTIP					\
																		\
		mouseEnter {													\
			setitemcolor "tricheckactionmulti"##TRICHECKBOXACTIONMULTI_CVAR forecolor .9 .9 .9 1 ;	\
		}																\
																		\
		mouseExit {														\
			setitemcolor "tricheckactionmulti"##TRICHECKBOXACTIONMULTI_CVAR forecolor .6 .6 .6 1 ;	\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
			TRICHECKBOXACTIONMULTI_ACTION								\
		}																\
    }

#define MULTI( MULTI_X, MULTI_Y, MULTI_W, MULTI_H, MULTI_TEXT, MULTI_TEXT_SCALE, MULTI_TEXT_ALIGN_Y, MULTI_CVAR, MULTI_CVARLIST, MULTI_TOOLTIP )	\
    itemDef {															\
		name			"multi"##MULTI_TEXT								\
      	group			GROUP_NAME										\
		rect			$evalfloat(MULTI_X) $evalfloat(MULTI_Y) $evalfloat(MULTI_W) $evalfloat(MULTI_H)	\
		type			ITEM_TYPE_MULTI									\
		text			MULTI_TEXT										\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		MULTI_TEXT_SCALE								\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(MULTI_W))						\
		textaligny		MULTI_TEXT_ALIGN_Y								\
		forecolor		.6 .6 .6 1										\
		cvar			MULTI_CVAR										\
		MULTI_CVARLIST													\
		visible			1												\
		tooltip			MULTI_TOOLTIP									\
																		\
		mouseEnter {													\
			setitemcolor "multi"##MULTI_TEXT forecolor .9 .9 .9 1 ;		\
		}																\
																		\
		mouseExit {														\
			setitemcolor "multi"##MULTI_TEXT forecolor .6 .6 .6 1 ;		\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
		}																\
    }

#define MULTILEFT( MULTILEFT_X, MULTILEFT_Y, MULTILEFT_W, MULTILEFT_H, MULTILEFT_TEXT, MULTILEFT_TEXT_SCALE, MULTILEFT_TEXT_ALIGN_Y, MULTILEFT_CVAR, MULTILEFT_CVARLIST, MULTILEFT_TOOLTIP )	\
    itemDef {															\
		name			"multileft"##MULTILEFT_TEXT								\
      	group			GROUP_NAME										\
		rect			$evalfloat(MULTILEFT_X) $evalfloat(MULTILEFT_Y) $evalfloat(MULTILEFT_W) $evalfloat(MULTILEFT_H)	\
		type			ITEM_TYPE_MULTI									\
		text			MULTILEFT_TEXT										\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		MULTILEFT_TEXT_SCALE								\
		textaligny		MULTILEFT_TEXT_ALIGN_Y								\
		forecolor		.6 .6 .6 1										\
		cvar			MULTILEFT_CVAR										\
		MULTILEFT_CVARLIST													\
		visible			1												\
		tooltip			MULTILEFT_TOOLTIP								\
																		\
		mouseEnter {													\
			setitemcolor "multileft"##MULTILEFT_TEXT forecolor .9 .9 .9 1 ;		\
		}																\
																		\
		mouseExit {														\
			setitemcolor "multileft"##MULTILEFT_TEXT forecolor .6 .6 .6 1 ;		\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
		}																\
    }

#define MULTIACTION( MULTIACTION_X, MULTIACTION_Y, MULTIACTION_W, MULTIACTION_H, MULTIACTION_TEXT, MULTIACTION_TEXT_SCALE, MULTIACTION_TEXT_ALIGN_Y, MULTIACTION_CVAR, MULTIACTION_CVARLIST, MULTIACTION_ACTION, MULTIACTION_TOOLTIP )	\
    itemDef {															\
		name			"multiaction"##MULTIACTION_TEXT								\
      	group			GROUP_NAME										\
		rect			$evalfloat(MULTIACTION_X) $evalfloat(MULTIACTION_Y) $evalfloat(MULTIACTION_W) $evalfloat(MULTIACTION_H)	\
		type			ITEM_TYPE_MULTI									\
		text			MULTIACTION_TEXT								\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		MULTIACTION_TEXT_SCALE							\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(MULTIACTION_W))					\
		textaligny		MULTIACTION_TEXT_ALIGN_Y						\
		forecolor		.6 .6 .6 1										\
		cvar			MULTIACTION_CVAR								\
		MULTIACTION_CVARLIST											\
		visible			1												\
		tooltip			MULTIACTION_TOOLTIP								\
																		\
		mouseEnter {													\
			setitemcolor "multiaction"##MULTIACTION_TEXT forecolor .9 .9 .9 1 ;		\
		}																\
																		\
		mouseExit {														\
			setitemcolor "multiaction"##MULTIACTION_TEXT forecolor .6 .6 .6 1 ;		\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
			MULTIACTION_ACTION											\
		}																\
    }

#define MULTIACTIONLEFT( MULTIACTIONLEFT_X, MULTIACTIONLEFT_Y, MULTIACTIONLEFT_W, MULTIACTIONLEFT_H, MULTIACTIONLEFT_TEXT, MULTIACTIONLEFT_TEXT_SCALE, MULTIACTIONLEFT_TEXT_ALIGN_Y, MULTIACTIONLEFT_CVAR, MULTIACTIONLEFT_CVARLIST, MULTIACTIONLEFT_ACTION, MULTIACTIONLEFT_TOOLTIP )	\
    itemDef {															\
		name			"multiactionleft"##MULTIACTIONLEFT_TEXT			\
      	group			GROUP_NAME										\
		rect			$evalfloat(MULTIACTIONLEFT_X) $evalfloat(MULTIACTIONLEFT_Y) $evalfloat(MULTIACTIONLEFT_W) $evalfloat(MULTIACTIONLEFT_H)	\
		type			ITEM_TYPE_MULTI									\
		text			MULTIACTIONLEFT_TEXT							\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		MULTIACTIONLEFT_TEXT_SCALE						\
		textaligny		MULTIACTIONLEFT_TEXT_ALIGN_Y					\
		forecolor		.6 .6 .6 1										\
		cvar			MULTIACTIONLEFT_CVAR							\
		MULTIACTIONLEFT_CVARLIST										\
		visible			1												\
		tooltip			MULTIACTIONLEFT_TOOLTIP							\
																		\
		mouseEnter {													\
			setitemcolor "multiactionleft"##MULTIACTIONLEFT_TEXT forecolor .9 .9 .9 1 ;		\
		}																\
																		\
		mouseExit {														\
			setitemcolor "multiactionleft"##MULTIACTIONLEFT_TEXT forecolor .6 .6 .6 1 ;		\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
			MULTIACTIONLEFT_ACTION										\
		}																\
    }

#define SLIDER( SLIDER_X, SLIDER_Y, SLIDER_W, SLIDER_H, SLIDER_TEXT, SLIDER_TEXT_SCALE, SLIDER_TEXT_ALIGN_Y, SLIDER_CVARFLOAT, SLIDER_TOOLTIP )	\
    itemDef {															\
		name			"slider"##SLIDER_TEXT								\
      	group			GROUP_NAME										\
		rect			$evalfloat(SLIDER_X) $evalfloat(SLIDER_Y) $evalfloat(SLIDER_W) $evalfloat(SLIDER_H)	\
		type			ITEM_TYPE_SLIDER								\
		text			SLIDER_TEXT										\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		SLIDER_TEXT_SCALE								\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(SLIDER_W))						\
		textaligny		SLIDER_TEXT_ALIGN_Y								\
		forecolor		.6 .6 .6 1										\
		cvarFloat		SLIDER_CVARFLOAT								\
		visible			1												\
		tooltip			SLIDER_TOOLTIP									\
																		\
		mouseEnter {													\
			setitemcolor "slider"##SLIDER_TEXT forecolor .9 .9 .9 1 ;		\
		}																\
																		\
		mouseExit {														\
			setitemcolor "slider"##SLIDER_TEXT forecolor .6 .6 .6 1 ;		\
		}																\
    }

#define BIND( BIND_X, BIND_Y, BIND_W, BIND_H, BIND_TEXT, BIND_TEXT_SCALE, BIND_TEXT_ALIGN_Y, BIND_CVAR, BIND_TOOLTIP )	\
    itemDef {															\
		name			"bind"##BIND_TEXT								\
      	group			GROUP_NAME										\
		rect			$evalfloat(BIND_X) $evalfloat(BIND_Y) $evalfloat(BIND_W) $evalfloat(BIND_H)	\
		type			ITEM_TYPE_BIND									\
		text			BIND_TEXT										\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		BIND_TEXT_SCALE									\
		textalign		ITEM_ALIGN_RIGHT								\
		textalignx		$evalfloat(0.5*(BIND_W))						\
		textaligny		BIND_TEXT_ALIGN_Y								\
		forecolor		.6 .6 .6 1										\
		cvar			BIND_CVAR										\
		visible			1												\
		tooltip			BIND_TOOLTIP									\
																		\
		mouseEnter {													\
			setitemcolor "bind"##BIND_TEXT forecolor .9 .9 .9 1 ;		\
		}																\
																		\
		mouseExit {														\
			setitemcolor "bind"##BIND_TEXT forecolor .6 .6 .6 1 ;		\
		}																\
																		\
		action {														\
			play "sound/menu/filter.wav" ;								\
		}																\
    }

#define LABEL( LABEL_X, LABEL_Y, LABEL_W, LABEL_H, LABEL_TEXT, LABEL_TEXT_SCALE, LABEL_TEXT_ALIGN, LABEL_TEXT_ALIGN_X, LABEL_TEXT_ALIGN_Y )	\
    itemDef {															\
		name			"label"##LABEL_TEXT								\
      	group			GROUP_NAME										\
		rect			$evalfloat(LABEL_X) $evalfloat(LABEL_Y) $evalfloat(LABEL_W) $evalfloat(LABEL_H)	\
		type			ITEM_TYPE_TEXT									\
		text			LABEL_TEXT										\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		LABEL_TEXT_SCALE								\
		textalign		LABEL_TEXT_ALIGN								\
		textalignx		$evalfloat(LABEL_TEXT_ALIGN_X)					\
		textaligny		$evalfloat(LABEL_TEXT_ALIGN_Y)					\
		forecolor		.6 .6 .6 1										\
		visible			1												\
		decoration														\
		autowrapped														\
    }

#define LABELWHITE( LABELWHITE_X, LABELWHITE_Y, LABELWHITE_W, LABELWHITE_H, LABELWHITE_TEXT, LABELWHITE_TEXT_SCALE, LABELWHITE_TEXT_ALIGN, LABELWHITE_TEXT_ALIGN_X, LABELWHITE_TEXT_ALIGN_Y )	\
    itemDef {															\
		name			"labelwhite"##LABELWHITE_TEXT					\
      	group			GROUP_NAME										\
		rect			$evalfloat(LABELWHITE_X) $evalfloat(LABELWHITE_Y) $evalfloat(LABELWHITE_W) $evalfloat(LABELWHITE_H)	\
		type			ITEM_TYPE_TEXT									\
		text			LABELWHITE_TEXT									\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		LABELWHITE_TEXT_SCALE							\
		textalign		LABELWHITE_TEXT_ALIGN							\
		textalignx		$evalfloat(LABELWHITE_TEXT_ALIGN_X)				\
		textaligny		$evalfloat(LABELWHITE_TEXT_ALIGN_Y)				\
		forecolor		1 1 1 1											\
		visible			1												\
		decoration														\
		autowrapped														\
    }

#define CVARLABEL( CVARLABEL_X, CVARLABEL_Y, CVARLABEL_W, CVARLABEL_H, CVARLABEL_CVAR, CVARLABEL_TEXT_SCALE, CVARLABEL_TEXT_ALIGN, CVARLABEL_TEXT_ALIGN_X, CVARLABEL_TEXT_ALIGN_Y )	\
    itemDef {															\
		name			"cvarlabel"##CVARLABEL_CVAR						\
      	group			GROUP_NAME										\
		rect			$evalfloat(CVARLABEL_X) $evalfloat(CVARLABEL_Y) $evalfloat(CVARLABEL_W) $evalfloat(CVARLABEL_H)	\
		type			ITEM_TYPE_TEXT									\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		CVARLABEL_TEXT_SCALE							\
		textalign		CVARLABEL_TEXT_ALIGN							\
		textalignx		CVARLABEL_TEXT_ALIGN_X							\
		textaligny		CVARLABEL_TEXT_ALIGN_Y							\
		forecolor		.6 .6 .6 1										\
		cvar			CVARLABEL_CVAR									\
		visible			1												\
		decoration														\
		autowrapped														\
    }

#define CVARFLOATLABEL( CVARFLOATLABEL_X, CVARFLOATLABEL_Y, CVARFLOATLABEL_W, CVARFLOATLABEL_H, CVARFLOATLABEL_CVAR, CVARFLOATLABEL_TEXT_SCALE, CVARFLOATLABEL_TEXT_ALIGN, CVARFLOATLABEL_TEXT_ALIGN_X, CVARFLOATLABEL_TEXT_ALIGN_Y )	\
    itemDef {															\
		name			"cvarfloatlabel"##CVARFLOATLABEL_CVAR						\
      	group			GROUP_NAME										\
		rect			$evalfloat(CVARFLOATLABEL_X) $evalfloat(CVARFLOATLABEL_Y) $evalfloat(CVARFLOATLABEL_W) $evalfloat(CVARFLOATLABEL_H)	\
		type			ITEM_TYPE_TEXT									\
		textfont		UI_FONT_COURBD_21								\
		textstyle		ITEM_TEXTSTYLE_SHADOWED							\
		textscale		CVARFLOATLABEL_TEXT_SCALE							\
		textalign		CVARFLOATLABEL_TEXT_ALIGN							\
		textalignx		CVARFLOATLABEL_TEXT_ALIGN_X							\
		textaligny		CVARFLOATLABEL_TEXT_ALIGN_Y							\
		forecolor		.6 .6 .6 1										\
		cvar			CVARFLOATLABEL_CVAR									\
		visible			1												\
		decoration														\
		textasfloat														\
    }
