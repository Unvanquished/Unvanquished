/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Daemon GPL Source Code (Daemon Source Code).

Daemon Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

Daemon Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Daemon Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the Daemon Source Code is also subject to certain additional terms.
You should have received a copy of these additional terms immediately following the
terms and conditions of the GNU General Public License which accompanied the Daemon
Source Code.  If not, please request a copy in writing from id Software at the address
below.

If you have questions concerning this license or the applicable additional terms, you
may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville,
Maryland 20850 USA.

===========================================================================
*/

#include "client.h"

/*

key up events are sent even if in console mode

*/

#define CLIP(t) MAX( 0, MIN( MAX_TEAMS - 1, (t) ) )

field_t  g_consoleField;
field_t  chatField;
qboolean chat_irc;

qboolean key_overstrikeMode;
qboolean bindingsModified;

int      anykeydown;
qkey_t   keys[ MAX_KEYS ];

int      bindTeam = 0; // TEAM_NONE

typedef struct
{
	const char *name;
	int        keynum;
} keyname_t;

// names not in this list can either be lowercase ascii, or '0xnn' hex sequences
static const keyname_t keynames[] =
{
	{ "TAB",                    K_TAB                    },
	{ "ENTER",                  K_ENTER                  },
	{ "ESCAPE",                 K_ESCAPE                 },
	{ "SPACE",                  K_SPACE                  },
	{ "BACKSPACE",              K_BACKSPACE              },
	{ "UPARROW",                K_UPARROW                },
	{ "DOWNARROW",              K_DOWNARROW              },
	{ "LEFTARROW",              K_LEFTARROW              },
	{ "RIGHTARROW",             K_RIGHTARROW             },

	{ "BACKSLASH",              '\\'                     },

	{ "ALT",                    K_ALT                    },
	{ "CTRL",                   K_CTRL                   },
	{ "SHIFT",                  K_SHIFT                  },

	{ "COMMAND",                K_COMMAND                },

	{ "CAPSLOCK",               K_CAPSLOCK               },

	{ "F1",                     K_F1                     },
	{ "F2",                     K_F2                     },
	{ "F3",                     K_F3                     },
	{ "F4",                     K_F4                     },
	{ "F5",                     K_F5                     },
	{ "F6",                     K_F6                     },
	{ "F7",                     K_F7                     },
	{ "F8",                     K_F8                     },
	{ "F9",                     K_F9                     },
	{ "F10",                    K_F10                    },
	{ "F11",                    K_F11                    },
	{ "F12",                    K_F12                    },
	{ "F13",                    K_F13                    },
	{ "F14",                    K_F14                    },
	{ "F15",                    K_F15                    },

	{ "INS",                    K_INS                    },
	{ "DEL",                    K_DEL                    },
	{ "PGDN",                   K_PGDN                   },
	{ "PGUP",                   K_PGUP                   },
	{ "HOME",                   K_HOME                   },
	{ "END",                    K_END                    },

	{ "MOUSE1",                 K_MOUSE1                 },
	{ "MOUSE2",                 K_MOUSE2                 },
	{ "MOUSE3",                 K_MOUSE3                 },
	{ "MOUSE4",                 K_MOUSE4                 },
	{ "MOUSE5",                 K_MOUSE5                 },

	{ "MWHEELUP",               K_MWHEELUP               },
	{ "MWHEELDOWN",             K_MWHEELDOWN             },

	{ "JOY1",                   K_JOY1                   },
	{ "JOY2",                   K_JOY2                   },
	{ "JOY3",                   K_JOY3                   },
	{ "JOY4",                   K_JOY4                   },
	{ "JOY5",                   K_JOY5                   },
	{ "JOY6",                   K_JOY6                   },
	{ "JOY7",                   K_JOY7                   },
	{ "JOY8",                   K_JOY8                   },
	{ "JOY9",                   K_JOY9                   },
	{ "JOY10",                  K_JOY10                  },
	{ "JOY11",                  K_JOY11                  },
	{ "JOY12",                  K_JOY12                  },
	{ "JOY13",                  K_JOY13                  },
	{ "JOY14",                  K_JOY14                  },
	{ "JOY15",                  K_JOY15                  },
	{ "JOY16",                  K_JOY16                  },
	{ "JOY17",                  K_JOY17                  },
	{ "JOY18",                  K_JOY18                  },
	{ "JOY19",                  K_JOY19                  },
	{ "JOY20",                  K_JOY20                  },
	{ "JOY21",                  K_JOY21                  },
	{ "JOY22",                  K_JOY22                  },
	{ "JOY23",                  K_JOY23                  },
	{ "JOY24",                  K_JOY24                  },
	{ "JOY25",                  K_JOY25                  },
	{ "JOY26",                  K_JOY26                  },
	{ "JOY27",                  K_JOY27                  },
	{ "JOY28",                  K_JOY28                  },
	{ "JOY29",                  K_JOY29                  },
	{ "JOY30",                  K_JOY30                  },
	{ "JOY31",                  K_JOY31                  },
	{ "JOY32",                  K_JOY32                  },

	{ "AUX1",                   K_AUX1                   },
	{ "AUX2",                   K_AUX2                   },
	{ "AUX3",                   K_AUX3                   },
	{ "AUX4",                   K_AUX4                   },
	{ "AUX5",                   K_AUX5                   },
	{ "AUX6",                   K_AUX6                   },
	{ "AUX7",                   K_AUX7                   },
	{ "AUX8",                   K_AUX8                   },
	{ "AUX9",                   K_AUX9                   },
	{ "AUX10",                  K_AUX10                  },
	{ "AUX11",                  K_AUX11                  },
	{ "AUX12",                  K_AUX12                  },
	{ "AUX13",                  K_AUX13                  },
	{ "AUX14",                  K_AUX14                  },
	{ "AUX15",                  K_AUX15                  },
	{ "AUX16",                  K_AUX16                  },

	{ "KP_HOME",                K_KP_HOME                },
	{ "KP_7",                   K_KP_HOME                },
	{ "KP_UPARROW",             K_KP_UPARROW             },
	{ "KP_8",                   K_KP_UPARROW             },
	{ "KP_PGUP",                K_KP_PGUP                },
	{ "KP_9",                   K_KP_PGUP                },
	{ "KP_LEFTARROW",           K_KP_LEFTARROW           },
	{ "KP_4",                   K_KP_LEFTARROW           },
	{ "KP_5",                   K_KP_5                   },
	{ "KP_RIGHTARROW",          K_KP_RIGHTARROW          },
	{ "KP_6",                   K_KP_RIGHTARROW          },
	{ "KP_END",                 K_KP_END                 },
	{ "KP_1",                   K_KP_END                 },
	{ "KP_DOWNARROW",           K_KP_DOWNARROW           },
	{ "KP_2",                   K_KP_DOWNARROW           },
	{ "KP_PGDN",                K_KP_PGDN                },
	{ "KP_3",                   K_KP_PGDN                },
	{ "KP_ENTER",               K_KP_ENTER               },
	{ "KP_INS",                 K_KP_INS                 },
	{ "KP_0",                   K_KP_INS                 },
	{ "KP_DEL",                 K_KP_DEL                 },
	{ "KP_PERIOD",              K_KP_DEL                 },
	{ "KP_SLASH",               K_KP_SLASH               },
	{ "KP_MINUS",               K_KP_MINUS               },
	{ "KP_PLUS",                K_KP_PLUS                },
	{ "KP_NUMLOCK",             K_KP_NUMLOCK             },
	{ "KP_STAR",                K_KP_STAR                },
	{ "KP_EQUALS",              K_KP_EQUALS              },

	{ "PAUSE",                  K_PAUSE                  },

	{ "SEMICOLON",              ';'                      }, // because a raw semicolon separates commands

	{ "WORLD_0",                K_WORLD_0                },
	{ "WORLD_1",                K_WORLD_1                },
	{ "WORLD_2",                K_WORLD_2                },
	{ "WORLD_3",                K_WORLD_3                },
	{ "WORLD_4",                K_WORLD_4                },
	{ "WORLD_5",                K_WORLD_5                },
	{ "WORLD_6",                K_WORLD_6                },
	{ "WORLD_7",                K_WORLD_7                },
	{ "WORLD_8",                K_WORLD_8                },
	{ "WORLD_9",                K_WORLD_9                },
	{ "WORLD_10",               K_WORLD_10               },
	{ "WORLD_11",               K_WORLD_11               },
	{ "WORLD_12",               K_WORLD_12               },
	{ "WORLD_13",               K_WORLD_13               },
	{ "WORLD_14",               K_WORLD_14               },
	{ "WORLD_15",               K_WORLD_15               },
	{ "WORLD_16",               K_WORLD_16               },
	{ "WORLD_17",               K_WORLD_17               },
	{ "WORLD_18",               K_WORLD_18               },
	{ "WORLD_19",               K_WORLD_19               },
	{ "WORLD_20",               K_WORLD_20               },
	{ "WORLD_21",               K_WORLD_21               },
	{ "WORLD_22",               K_WORLD_22               },
	{ "WORLD_23",               K_WORLD_23               },
	{ "WORLD_24",               K_WORLD_24               },
	{ "WORLD_25",               K_WORLD_25               },
	{ "WORLD_26",               K_WORLD_26               },
	{ "WORLD_27",               K_WORLD_27               },
	{ "WORLD_28",               K_WORLD_28               },
	{ "WORLD_29",               K_WORLD_29               },
	{ "WORLD_30",               K_WORLD_30               },
	{ "WORLD_31",               K_WORLD_31               },
	{ "WORLD_32",               K_WORLD_32               },
	{ "WORLD_33",               K_WORLD_33               },
	{ "WORLD_34",               K_WORLD_34               },
	{ "WORLD_35",               K_WORLD_35               },
	{ "WORLD_36",               K_WORLD_36               },
	{ "WORLD_37",               K_WORLD_37               },
	{ "WORLD_38",               K_WORLD_38               },
	{ "WORLD_39",               K_WORLD_39               },
	{ "WORLD_40",               K_WORLD_40               },
	{ "WORLD_41",               K_WORLD_41               },
	{ "WORLD_42",               K_WORLD_42               },
	{ "WORLD_43",               K_WORLD_43               },
	{ "WORLD_44",               K_WORLD_44               },
	{ "WORLD_45",               K_WORLD_45               },
	{ "WORLD_46",               K_WORLD_46               },
	{ "WORLD_47",               K_WORLD_47               },
	{ "WORLD_48",               K_WORLD_48               },
	{ "WORLD_49",               K_WORLD_49               },
	{ "WORLD_50",               K_WORLD_50               },
	{ "WORLD_51",               K_WORLD_51               },
	{ "WORLD_52",               K_WORLD_52               },
	{ "WORLD_53",               K_WORLD_53               },
	{ "WORLD_54",               K_WORLD_54               },
	{ "WORLD_55",               K_WORLD_55               },
	{ "WORLD_56",               K_WORLD_56               },
	{ "WORLD_57",               K_WORLD_57               },
	{ "WORLD_58",               K_WORLD_58               },
	{ "WORLD_59",               K_WORLD_59               },
	{ "WORLD_60",               K_WORLD_60               },
	{ "WORLD_61",               K_WORLD_61               },
	{ "WORLD_62",               K_WORLD_62               },
	{ "WORLD_63",               K_WORLD_63               },
	{ "WORLD_64",               K_WORLD_64               },
	{ "WORLD_65",               K_WORLD_65               },
	{ "WORLD_66",               K_WORLD_66               },
	{ "WORLD_67",               K_WORLD_67               },
	{ "WORLD_68",               K_WORLD_68               },
	{ "WORLD_69",               K_WORLD_69               },
	{ "WORLD_70",               K_WORLD_70               },
	{ "WORLD_71",               K_WORLD_71               },
	{ "WORLD_72",               K_WORLD_72               },
	{ "WORLD_73",               K_WORLD_73               },
	{ "WORLD_74",               K_WORLD_74               },
	{ "WORLD_75",               K_WORLD_75               },
	{ "WORLD_76",               K_WORLD_76               },
	{ "WORLD_77",               K_WORLD_77               },
	{ "WORLD_78",               K_WORLD_78               },
	{ "WORLD_79",               K_WORLD_79               },
	{ "WORLD_80",               K_WORLD_80               },
	{ "WORLD_81",               K_WORLD_81               },
	{ "WORLD_82",               K_WORLD_82               },
	{ "WORLD_83",               K_WORLD_83               },
	{ "WORLD_84",               K_WORLD_84               },
	{ "WORLD_85",               K_WORLD_85               },
	{ "WORLD_86",               K_WORLD_86               },
	{ "WORLD_87",               K_WORLD_87               },
	{ "WORLD_88",               K_WORLD_88               },
	{ "WORLD_89",               K_WORLD_89               },
	{ "WORLD_90",               K_WORLD_90               },
	{ "WORLD_91",               K_WORLD_91               },
	{ "WORLD_92",               K_WORLD_92               },
	{ "WORLD_93",               K_WORLD_93               },
	{ "WORLD_94",               K_WORLD_94               },
	{ "WORLD_95",               K_WORLD_95               },

	{ "WINDOWS",                K_SUPER                  },
	{ "COMPOSE",                K_COMPOSE                },
	{ "MODE",                   K_MODE                   },
	{ "HELP",                   K_HELP                   },
	{ "PRINT",                  K_PRINT                  },
	{ "SYSREQ",                 K_SYSREQ                 },
	{ "SCROLLOCK",              K_SCROLLOCK              },
	{ "BREAK",                  K_BREAK                  },
	{ "MENU",                   K_MENU                   },
	{ "POWER",                  K_POWER                  },
	{ "EURO",                   K_EURO                   },
	{ "UNDO",                   K_UNDO                   },

	{ "XBOX360_A",              K_XBOX360_A              },
	{ "XBOX360_B",              K_XBOX360_B              },
	{ "XBOX360_X",              K_XBOX360_X              },
	{ "XBOX360_Y",              K_XBOX360_Y              },
	{ "XBOX360_LB",             K_XBOX360_LB             },
	{ "XBOX360_RB",             K_XBOX360_RB             },
	{ "XBOX360_START",          K_XBOX360_START          },
	{ "XBOX360_GUIDE",          K_XBOX360_GUIDE          },
	{ "XBOX360_LS",             K_XBOX360_LS             },
	{ "XBOX360_RS",             K_XBOX360_RS             },
	{ "XBOX360_BACK",           K_XBOX360_BACK           },
	{ "XBOX360_LT",             K_XBOX360_LT             },
	{ "XBOX360_RT",             K_XBOX360_RT             },
	{ "XBOX360_DPAD_UP",        K_XBOX360_DPAD_UP        },
	{ "XBOX360_DPAD_RIGHT",     K_XBOX360_DPAD_RIGHT     },
	{ "XBOX360_DPAD_DOWN",      K_XBOX360_DPAD_DOWN      },
	{ "XBOX360_DPAD_LEFT",      K_XBOX360_DPAD_LEFT      },
	{ "XBOX360_DPAD_RIGHTUP",   K_XBOX360_DPAD_RIGHTUP   },
	{ "XBOX360_DPAD_RIGHTDOWN", K_XBOX360_DPAD_RIGHTDOWN },
	{ "XBOX360_DPAD_LEFTUP",    K_XBOX360_DPAD_LEFTUP    },
	{ "XBOX360_DPAD_LEFTDOWN",  K_XBOX360_DPAD_LEFTDOWN  },

	{ NULL,                     0                        }
};

/*
=============================================================================

EDIT FIELDS

=============================================================================
*/

/*
===================
Field_Draw

Handles horizontal scrolling and cursor blinking
x, y, and width are in pixels
===================
*/
void Field_VariableSizeDraw( field_t *edit, int x, int y, int size, qboolean showCursor,
                             qboolean noColorEscape, float alpha )
{
	int  len;
	int  drawLen;
	int  prestep;
	char str[ MAX_STRING_CHARS ];
	int  i;
	int  offset, offsetEnd;

	drawLen = edit->widthInChars - 1; // - 1 so there is always a space for the cursor
	len = Q_UTF8Strlen( edit->buffer );

	// guarantee that cursor will be visible
	if ( len <= drawLen )
	{
		edit->scroll = prestep = 0;
	}
	else
	{
		if ( edit->scroll + drawLen > len )
		{
			edit->scroll = len - drawLen;

			if ( edit->scroll < 0 )
			{
				edit->scroll = 0;
			}
		}

		prestep = edit->scroll;
	}

	if ( prestep + drawLen > len )
	{
		drawLen = len - prestep;
	}

	// extract <drawLen> characters from the field at <prestep>
	offset = offsetEnd = Field_ScrollToOffset( edit );
	for ( i = 0; i < drawLen && edit->buffer[ offsetEnd ]; ++i )
	{
		offsetEnd += Q_UTF8Width( edit->buffer + offsetEnd );
	}

	if ( offsetEnd - offset >= MAX_STRING_CHARS )
	{
		Com_Error( ERR_DROP, "drawLen >= MAX_STRING_CHARS" );
	}

	str[ offsetEnd - offset ] = 0;
	Com_Memcpy( str, edit->buffer + offset, offsetEnd - offset );

	// draw it
	if ( size == SMALLCHAR_WIDTH )
	{
		float color[ 4 ];

		color[ 0 ] = color[ 1 ] = color[ 2 ] = 1.0;
		color[ 3 ] = alpha;
		SCR_DrawSmallStringExt( x, y, str, color, qfalse, noColorEscape );
	}
	else
	{
		// draw big string with drop shadow
		SCR_DrawBigString( x, y, str, 1.0, noColorEscape );
	}

	if ( len > drawLen )
	{
		static const float yellow[] = { 1, 1, 0, 0.25 };
		float width = SCR_ConsoleFontStringWidth( str, drawLen );

		re.SetColor( yellow );
		re.DrawStretchPic( x + ( width * prestep ) / len, y + 3,
		                   ( width * drawLen ) / len, 2,
		                   0, 0, 0, 0, cls.whiteShader );
	}

	// draw the cursor
	if ( showCursor )
	{
		static const float cyan[] = { 0, 1, 1, 0.5 };
		float xpos, width, height;

		if ( ( int )( cls.realtime >> 8 ) & 1 )
		{
			return; // off blink
		}

		re.SetColor( cyan );

		if ( size == SMALLCHAR_WIDTH )
		{
			xpos = x + SCR_ConsoleFontStringWidth( str, edit->cursor - prestep );
			height = key_overstrikeMode ? SMALLCHAR_HEIGHT / ( CONSOLE_FONT_VPADDING + 1 ) : 2;
			width = SMALLCHAR_WIDTH;
		}
		else
		{
			i = drawLen - Q_UTF8Strlen( str );
			xpos = x + ( edit->cursor - prestep - i ) * size;
			height = key_overstrikeMode ? BIGCHAR_HEIGHT / ( CONSOLE_FONT_VPADDING + 1 ) : 2;
			width = BIGCHAR_WIDTH;
		}

		re.DrawStretchPic( xpos, y + 2 - height, width, height, 0, 0, 0, 0, cls.whiteShader );
	}
}

void Field_Draw( field_t *edit, int x, int y, qboolean showCursor, qboolean noColorEscape, float alpha )
{
	Field_VariableSizeDraw( edit, x, y, SMALLCHAR_WIDTH, showCursor, noColorEscape, alpha );
}

void Field_BigDraw( field_t *edit, int x, int y, qboolean showCursor, qboolean noColorEscape )
{
	Field_VariableSizeDraw( edit, x, y, BIGCHAR_WIDTH, showCursor, noColorEscape, 1.0f );
}

/*
================
Field_Paste
================
*/
static void Field_Paste( field_t *edit, clipboard_t clip )
{
	const char *cbd;
	int        pasteLen, width;
	char       *ptr = Sys_GetClipboardData( clip );

	if ( !ptr )
	{
		return;
	}

	cbd = Com_ClearForeignCharacters( ptr );
	Z_Free( ptr );

	// send as if typed, so insert / overstrike works properly
	pasteLen = strlen( cbd );

	while ( pasteLen >= ( width = Q_UTF8Width( cbd ) ) )
	{
		Field_CharEvent( edit, cbd );

		cbd += width;
		pasteLen -= width;
	}
}

/*
=================
Field_KeyDownEvent

Performs the basic line editing functions for the console,
in-game talk, and menu fields

Key events are used for non-printable characters, others are gotten from char events.
=================
*/
void Field_KeyDownEvent( field_t *edit, int key )
{
	int len, width;
	char *s;

	key = tolower( key );
	len = Q_UTF8Strlen( edit->buffer );
	s = &edit->buffer[ Field_CursorToOffset( edit ) ];
	width = Q_UTF8Width( s );

	switch ( key )
	{
		case K_DEL:
			if ( *s )
			{
				memmove( s, s + width, strlen( s + width ) + 1 );
			}

			break;

		case K_RIGHTARROW:
			if ( keys[ K_CTRL ].down )
			{
				width = Field_CursorToOffset( edit );

				while ( edit->buffer[ width ] == ' ' )
				{
					++width;
				}
				while ( edit->buffer[ width ] && edit->buffer[ width ] != ' ' )
				{
					++width;
				}

				edit->cursor = Field_OffsetToCursor( edit, width );
			}
			else if ( edit->cursor < len )
			{
				edit->cursor++;
			}

			break;

		case K_LEFTARROW:
			if ( keys[ K_CTRL ].down )
			{
				width = Field_CursorToOffset( edit );

				while ( width && edit->buffer[ width ] == ' ' )
				{
					--width;
				}
				while ( width && edit->buffer[ width ] != ' ' )
				{
					--width;
				}

				edit->cursor = Field_OffsetToCursor( edit, width );
			}
			else if ( edit->cursor > 0 )
			{
				edit->cursor--;
			}

			break;

		case 'a':
			if ( keys[ K_CTRL ].down )
			{
		case K_HOME:
				edit->cursor = 0;
			}

			break;

		case 'e':
			if ( keys[ K_CTRL ].down )
			{
		case K_END:
				edit->cursor = len;
			}

			break;

		case K_INS:
			if ( keys[ K_SHIFT ].down )
			{
				Field_Paste( edit, SELECTION_PRIMARY );
			}
			else
			{
				key_overstrikeMode = !key_overstrikeMode;
			}
			break;

		case 't':
			if ( keys[ K_CTRL ].down && edit->cursor )
			{
				char *p, tmp[4];

				if ( edit->cursor == len )
				{
					--edit->cursor;
					s = &edit->buffer[ Field_CursorToOffset( edit ) ];
					width = Q_UTF8Width( s );
				}

				--edit->cursor;
				p = &edit->buffer[ Field_CursorToOffset( edit ) ];
				memcpy( tmp, p, s - p );
				memmove( p, s, width );
				memcpy( p + width, tmp, s - p );
				edit->cursor += 2;
			}

			break;
	}

	// Change scroll if cursor is no longer visible
	len = MIN( 5, edit->widthInChars / 4 );

	if ( edit->cursor < edit->scroll + len )
	{
		edit->scroll = edit->cursor - len;

		if ( edit->scroll < 0 )
		{
			edit->scroll = 0;
		}
	}
	else if ( edit->cursor >= edit->scroll + edit->widthInChars - len )
	{
		edit->scroll = edit->cursor - edit->widthInChars + 1 + len;
	}
}

/*
==================
Field_CharEvent
==================
*/
void Field_CharEvent( field_t *edit, const char *s )
{
	int len, width, oldWidth, offset;

	if ( *s == 'v' - 'a' + 1 ) // ctrl-v is paste
	{
		Field_Paste( edit, SELECTION_CLIPBOARD );
		return;
	}

	if ( *s == 'c' - 'a' + 1 ||
	     *s == 'u' - 'a' + 1 ) // ctrl-c or ctrl-u clear the field
	{
		Field_Clear( edit );
		return;
	}

	if ( *s == 'k' - 'a' + 1 ) // ctrl-k clears to the end of the field
	{
		edit->buffer[ Field_CursorToOffset( edit ) ] = '\0';
		return;
	}

	len = strlen( edit->buffer );

	if ( *s == 'h' - 'a' + 1 ) // ctrl-h is backspace
	{
		if ( edit->cursor )
		{
			int posFrom, posTo;

			posFrom = Field_CursorToOffset( edit );
			--edit->cursor;
			posTo = Field_CursorToOffset( edit );

			memmove( edit->buffer + posTo, edit->buffer + posFrom, len + 1 - posFrom );

			if ( edit->cursor < edit->scroll )
			{
				edit->scroll--;
			}
		}

		return;
	}

	if ( *s == 'd' - 'a' + 1 ) // ctrl-d is delete forward
	{
		int posTo = Field_CursorToOffset( edit );

		if ( edit->buffer[ posTo ] )
		{
			int posFrom = posTo + Q_UTF8Width( edit->buffer + posTo );
			memmove( edit->buffer + posTo, edit->buffer + posFrom, len + 1 - posFrom );
		}

		return;
	}

	if ( *s == 'a' - 'a' + 1 ) // ctrl-a is home
	{
		edit->cursor = 0;
		edit->scroll = 0;
		return;
	}

	if ( *s == 'e' - 'a' + 1 ) // ctrl-e is end
	{
		edit->cursor = Field_OffsetToCursor( edit, len );
		edit->scroll = edit->cursor - edit->widthInChars;
		return;
	}

	//
	// ignore any other non printable chars
	//
	if ( (unsigned char)*s < 32 || (unsigned char)*s == 0x7f )
	{
		return;
	}

	// 'unprintable' on Mac - used for cursor keys, function keys etc.
	if ( (unsigned int)( Q_UTF8CodePoint( s ) - 0xF700 ) < 0x200u )
	{
		return;
	}

	width = Q_UTF8Width( s );
	offset = Field_CursorToOffset( edit );

	// if overstrike, adjust the width according to what's being replaced
	// (at end-of-string, just insert)
	oldWidth = ( key_overstrikeMode && edit->buffer[ offset ] ) ? Q_UTF8Width( edit->buffer + offset ) : 0;

	if ( len + width - oldWidth >= MAX_EDIT_LINE )
	{
		return;
	}

	memmove( edit->buffer + offset + width,
	         edit->buffer + offset + oldWidth, len + 1 - offset - oldWidth );
	Com_Memcpy( edit->buffer + offset, s, width );
	++edit->cursor;

	do
	{
		edit->scroll++;
	} while ( edit->cursor >= edit->scroll +edit->widthInChars );
}

/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

/*
===============
CompleteCommand

Tab expansion
===============
*/
static void CompleteCommand( void )
{
	field_t *edit;
	edit = &g_consoleField;

	// only look at the first token for completion purposes
	Cmd_TokenizeString( edit->buffer );

	Field_AutoComplete( edit, "]" );
}

/*
====================
Console_Key

Handles history and console scrollback for the ingame console
====================
*/
void Console_Key( int key )
{
	// just return if any of the listed modifiers are pressed
	// - no point in passing on, since they Just Get In The Way
	if ( keys[ K_ALT     ].down || keys[ K_COMMAND ].down ||
	     keys[ K_MODE    ].down || keys[ K_SUPER   ].down )
	{
		return;
	}

	// ctrl-L clears screen
	if ( key == 'l' && keys[ K_CTRL ].down )
	{
		Cbuf_AddText( "clear\n" );
		return;
	}

	// enter finishes the line
	if ( key == K_ENTER || key == K_KP_ENTER )
	{
		con.acLength = 0;

		// if not in the game explicitly prepend a slash if needed
		if ( (cls.state != CA_ACTIVE || !cl_consoleCommand->string[0] ) && g_consoleField.buffer[ 0 ] != '\\'
		     && g_consoleField.buffer[ 0 ] != '/' )
		{
			char temp[ MAX_STRING_CHARS ];

			Q_strncpyz( temp, g_consoleField.buffer, sizeof( temp ) );
			Com_sprintf( g_consoleField.buffer, sizeof( g_consoleField.buffer ), "\\%s", temp );
			g_consoleField.cursor++;
		}

		Com_Printf( "]%s\n", g_consoleField.buffer );

		// leading slash is an explicit command
		if ( g_consoleField.buffer[ 0 ] == '\\' || g_consoleField.buffer[ 0 ] == '/' )
		{
			Cbuf_AddText( g_consoleField.buffer + 1 );  // valid command
			Cbuf_AddText( "\n" );
		}
		else
		{
			// other text will be chat messages
			if ( !g_consoleField.buffer[ 0 ] )
			{
				return; // empty lines just scroll the console without adding to history
			}
			else
			{
				Cbuf_AddText( va("%s %s\n", cl_consoleCommand->string, g_consoleField.buffer) );
			}
		}

		// copy line to history buffer
		Hist_Add( g_consoleField.buffer );

		if ( cls.state == CA_DISCONNECTED )
		{
			SCR_UpdateScreen(); // force an update, because the command
		} // may take some time

		Field_Clear( &g_consoleField );
		return;
	}

	// command completion

	if ( key == K_TAB )
	{
		CompleteCommand();
		return;
	}

	// clear the autocompletion buffer on a line-editing key input
	if ( ( key >= K_SPACE && key <= K_BACKSPACE ) || ( key == K_LEFTARROW ) || ( key == K_RIGHTARROW ) ||
	     ( key >= K_KP_LEFTARROW && key <= K_KP_RIGHTARROW ) ||
	     ( key >= K_KP_SLASH && key <= K_KP_PLUS ) || ( key >= K_KP_STAR && key <= K_KP_EQUALS ) )
	{
		con.acLength = 0;
	}

	// command history (ctrl-p ctrl-n for unix style)

	//----(SA)  added some mousewheel functionality to the console
	if ( ( key == K_MWHEELUP && keys[ K_SHIFT ].down ) || ( key == K_UPARROW ) || ( key == K_KP_UPARROW ) ||
	     ( ( tolower( key ) == 'p' ) && keys[ K_CTRL ].down ) )
	{
		Field_Set( &g_consoleField, Hist_Prev() );
		con.acLength = 0;
		return;
	}

	//----(SA)  added some mousewheel functionality to the console
	if ( ( key == K_MWHEELDOWN && keys[ K_SHIFT ].down ) || ( key == K_DOWNARROW ) || ( key == K_KP_DOWNARROW ) ||
	     ( ( tolower( key ) == 'n' ) && keys[ K_CTRL ].down ) )
	{
		const char *history = Hist_Next();

		if ( history )
		{
			Field_Set( &g_consoleField, history );
		}
		else if ( g_consoleField.buffer[ 0 ] )
		{
			Hist_Add( g_consoleField.buffer );
			Field_Clear( &g_consoleField );
		}

		con.acLength = 0;
		return;
	}

	// console scrolling
	if ( key == K_PGUP || key == K_KP_PGUP )
	{
		Con_PageUp();
		return;
	}

	if ( key == K_PGDN || key == K_KP_PGDN )
	{
		Con_PageDown();
		return;
	}

	if ( key == K_MWHEELUP ) //----(SA) added some mousewheel functionality to the console
	{
		Con_PageUp();

		if ( keys[ K_CTRL ].down ) // hold <ctrl> to accelerate scrolling
		{
			Con_PageUp();
			Con_PageUp();
		}

		return;
	}

	if ( key == K_MWHEELDOWN ) //----(SA) added some mousewheel functionality to the console
	{
		Con_PageDown();

		if ( keys[ K_CTRL ].down ) // hold <ctrl> to accelerate scrolling
		{
			Con_PageDown();
			Con_PageDown();
		}

		return;
	}

	// ctrl-home = top of console
	if ( ( key == K_HOME || key == K_KP_HOME ) && keys[ K_CTRL ].down )
	{
		Con_Top();
		return;
	}

	// ctrl-end = bottom of console
	if ( ( key == K_END || key == K_KP_END ) && keys[ K_CTRL ].down )
	{
		Con_Bottom();
		return;
	}

	// pass to the next editline routine
	Field_KeyDownEvent( &g_consoleField, key );
}

//============================================================================

qboolean Key_GetOverstrikeMode( void )
{
	return key_overstrikeMode;
}

void Key_SetOverstrikeMode( qboolean state )
{
	key_overstrikeMode = state;
}

/*
===================
Key_IsDown
===================
*/
qboolean Key_IsDown( int keynum )
{
	if ( keynum < 0 || keynum >= MAX_KEYS )
	{
		return qfalse;
	}

	return keys[ keynum ].down;
}

/*
===================
Key_StringToKeynum

Returns a key number to be used to index keys[] by looking at
the given string.  Single ascii characters return themselves, while
the K_* names are matched up.

0x11 will be interpreted as raw hex, which will allow new controlers

to be configured even if they don't have defined names.
===================
*/
int Key_StringToKeynum( const char *str )
{
	const keyname_t *kn;

	if ( !str )
	{
		return -1;
	}

	if ( !str[ 1 ] )
	{
		// single character; map upper-case ASCII letters to lower case
		int key = str[ 0 ] == '\n' ? -1 : str[ 0 ];

		return ( key >= 'A' && key <= 'Z' ) ? key + 0x20 : key;
	}

	// check for hex code
	if ( strlen( str ) == 4 )
	{
		int n = Com_HexStrToInt( str );

		if ( n >= 0 )
		{
			return n;
		}
	}

	// scan for a text match
	for ( kn = keynames; kn->name; kn++ )
	{
		if ( !Q_stricmp( str, kn->name ) )
		{
			return kn->keynum;
		}
	}

	return -1;
}

/*
===================
Key_KeynumToString

Returns a string (either a single ascii char, a K_* name, or a 0x11 hex string) for the
given keynum.
===================
*/
const char *Key_KeynumToString( int keynum )
{
	const keyname_t *kn;
	static char tinystr[ 5 ];
	int         i, j;

	if ( keynum == -1 )
	{
		return "<KEY NOT FOUND>";
	}

	if ( keynum < 0 || keynum >= MAX_KEYS )
	{
		return "<OUT OF RANGE>";
	}

	// check for printable ascii (don't use quote)
	if ( keynum > 32 && keynum < 127 && keynum != '"' && keynum != ';' && keynum != '\\' )
	{
		tinystr[ 0 ] = keynum;
		tinystr[ 1 ] = 0;
		return tinystr;
	}

	// check for a key string
	for ( kn = keynames; kn->name; kn++ )
	{
		if ( keynum == kn->keynum )
		{
			return kn->name;
		}
	}

	// make a hex string
	i = keynum >> 4;
	j = keynum & 15;

	tinystr[ 0 ] = '0';
	tinystr[ 1 ] = 'x';
	tinystr[ 2 ] = i > 9 ? i - 10 + 'a' : i + '0';
	tinystr[ 3 ] = j > 9 ? j - 10 + 'a' : j + '0';
	tinystr[ 4 ] = 0;

	return tinystr;
}

/*
===================
Key_GetTeam
Assumes 'three' teams: spectators, aliens, humans
===================
*/
static const char *const teamName[] = { "default", "aliens", "humans", "others" };

static int Key_GetTeam( const char *arg, const char *cmd )
{
	static const struct {
		char team;
		char label[11];
	} labels[] = {
		{ 0, "spectators" },
		{ 0, "default" },
		{ 1, "aliens" },
		{ 2, "humans" }
	};
	int t, l;

	if ( !*arg ) // empty string
	{
		goto fail;
	}

	for ( t = 0; arg[ t ]; ++t )
	{
		if ( !isdigit( arg[ t ] ) )
		{
			break;
		}
	}

	if ( !arg[ t ] )
	{
		t = atoi( arg );

		if ( t != CLIP( t ) )
		{
			Com_Printf( _("^3%s:^7 %d is not a valid team number\n"), cmd, t );
			return -1;
		}

		return t;
	}

	l = strlen( arg );

	for ( t = 0; t < ARRAY_LEN( labels ); ++t )
	{
		// matching initial substring
		if ( !Q_stricmpn( arg, labels[ t ].label, l ) )
		{
			return labels[ t ].team;
		}
	}

fail:
	Com_Printf( _("^3%s:^7 '%s^7' is not a valid team name\n"), cmd, arg );
	return -1;
}

/*
===================
Key_SetBinding

team == -1 clears all bindings for the key, then sets the spec/global binding
===================
*/
void Key_SetBinding( int keynum, int team, const char *binding )
{
	char *lcbinding; // fretn - make a copy of our binding lowercase
	// so name toggle scripts work again: bind x name BzZIfretn?
	// resulted into bzzifretn?

	if ( keynum < 0 || keynum >= MAX_KEYS )
	{
		return;
	}

	// free old bindings
	if ( team == -1 )
	{
		// just the team-specific ones here
		for ( team = MAX_TEAMS - 1; team; --team )
		{
			if ( keys[ keynum ].binding[ team ] )
			{
				Z_Free( keys[ keynum ].binding[ team ] );
				keys[ keynum ].binding[ team ] = NULL;
			}
		}
		// team == 0...
	}

	team = CLIP( team );

	if ( keys[ keynum ].binding[ team ] )
	{
		Z_Free( keys[ keynum ].binding[ team ] );
	}

	// set the new binding, if not null/empty
	if ( binding && binding[ 0 ] )
	{
		// allocate memory for new binding
		keys[ keynum ].binding[ team ] = CopyString( binding );
		lcbinding = CopyString( binding );
		Q_strlwr( lcbinding );  // saves doing it on all the generateHashValues in Key_GetBindingByString
		Z_Free( lcbinding );
	}
	else
	{
		keys[ keynum ].binding[ team ] = NULL;
	}

	bindingsModified = qtrue;
}

/*
===================
Key_GetBinding

-ve team no. = don't return the default binding
===================
*/
const char *Key_GetBinding( int keynum, int team )
{
	const char *bind;

	if ( keynum < 0 || keynum >= MAX_KEYS )
	{
		return NULL;
	}

	if ( team <= 0 )
	{
		return keys[ keynum ].binding[ CLIP( -team ) ];
	}

	bind = keys[ keynum ].binding[ CLIP( team ) ];
	return bind ? bind : keys[ keynum ].binding[ 0 ];
}

/*
===================
Key_GetKey
===================
*/

int Key_GetKey( const char *binding, int team )
{
	int i;

	while ( binding && team >= 0 )
	{
		for ( i = 0; i < MAX_KEYS; i++ )
		{
			if ( keys[ i ].binding[ team ] && Q_stricmp( binding, keys[ i ].binding[ team ] ) == 0 )
			{
				return i;
			}
		}

		team = team ? 0 : -1;
	}

	return -1;
}

/*
===================
Key_Unbind_f
===================
*/
void Key_Unbind_f( void )
{
	int b = Cmd_Argc();
	int team = -1;

	if ( b < 2 || b > 3 )
	{
		Com_Printf("%s", _( "unbind [team] <key>: remove commands from a key\n" ));
		return;
	}

	if ( b > 2 )
	{
		team = Key_GetTeam( Cmd_Argv( 1 ), "unbind" );

		if ( team < 0 )
		{
			return;
		}
	}

	b = Key_StringToKeynum( Cmd_Argv( b - 1 ) );

	if ( b == -1 )
	{
		Com_Printf(_( "\"%s\" isn't a valid key\n"), Cmd_Argv( 1 ) );
		return;
	}

	Key_SetBinding( b, team, NULL );
}

/*
===================
Key_Unbindall_f
===================
*/
void Key_Unbindall_f( void )
{
	int i;

	for ( i = 0; i < MAX_KEYS; i++ )
	{
		Key_SetBinding( i, -1, NULL );
	}
}

/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f( void )
{
	int        c, b;
	const char *key;
	const char *cmd = NULL;
	int        team = -1;

	int teambind = !Q_stricmp( Cmd_Argv( 0 ), "teambind" );

	c = Cmd_Argc();

	if ( c < 2 + teambind )
	{
		Com_Printf("%s", teambind ? _( "teambind <team> <key> [command] : attach a command to a key\n" ) : _( "bind <key> [command] : attach a command to a key\n" ));
		return;
	}
	else if ( c > 2 + teambind )
	{
		cmd = Cmd_Argv( 2 + teambind );
	}

	if ( teambind )
	{
		team = Key_GetTeam( Cmd_Argv( 1 ), teambind ? "teambind" : "bind" );

		if ( team < 0 )
		{
			return;
		}
	}

	key = Cmd_Argv( 1 + teambind );
	b = Key_StringToKeynum( key );

	if ( b == -1 )
	{
		Com_Printf(_( "\"%s\" isn't a valid key\n"), key );
		return;
	}

	if ( !cmd )
	{
		if ( teambind )
		{
			if ( keys[ b ].binding[ team ] )
			{
				Com_Printf( "\"%s\"[%s] = %s\n", key, teamName[ team ], Cmd_QuoteString( keys[ b ].binding[ team ] ) );
			}
			else
			{
				Com_Printf(_( "\"%s\"[%s] is not bound\n"), key, teamName[ team ] );
			}
		}
		else
		{
			qboolean bound = qfalse;
			int      i;

			for ( i = 0; i < MAX_TEAMS; ++i )
			{
				if ( keys[ b ].binding[ i ] )
				{
					Com_Printf( "\"%s\"[%s] = %s\n", key, teamName[ i ], Cmd_QuoteString( keys[ b ].binding[ i ] ) );
					bound = qtrue;
				}
			}

			if ( !bound )
			{
				Com_Printf(_( "\"%s\" is not bound\n"), key );
			}
		}


		return;
	}


	if ( c <= 3 + teambind )
	{
		Key_SetBinding( b, team, cmd );
	}
	else
	{
		// set to 3rd arg onwards (4th if team binding)
		Key_SetBinding( b, team, Cmd_ArgsFrom( 2 + teambind ) );
	}
}

/*
===================
Key_EditBind_f
===================
*/
void Key_EditBind_f( void )
{
	char           *buf;
	const char     *key, *binding;
	const char     *bindq;
	int            b;
	int            team = -1;

	b = Cmd_Argc();

	if ( b < 2 || b > 3 )
	{
		Com_Printf( "%s\n", _( "usage: editbind [team] <key>" ));
		return;
	}

	if ( b > 2 )
	{
		team = Key_GetTeam( Cmd_Argv( 1 ), "editbind" );

		if ( team < 0 )
		{
			return;
		}
	}

	key = Cmd_Argv( b - 1 );
	b = Key_StringToKeynum( key );

	if ( b == -1 )
	{
		Com_Printf(_( "\"%s\" isn't a valid key\n"), key );
		return;
	}

	binding = Key_GetBinding( b, -team );

	bindq = binding ? Cmd_QuoteString( binding ) : "";  // <- static buffer
	buf = malloc( 32 + strlen( key ) + strlen( bindq ) );

	if ( team >= 0 )
	{
		sprintf( buf, "/teambind %s %s %s", teamName[ team ], Key_KeynumToString( b ), bindq );
	}
	else
	{
		sprintf( buf, "/bind %s %s", Key_KeynumToString( b ), bindq );
	}

	Con_OpenConsole_f();
	Field_Set( &g_consoleField, buf );
	free( buf );
}

/*
============
Key_WriteBindings

Writes lines containing "bind key value"
============
*/
void Key_WriteBindings( fileHandle_t f )
{
	int i, team;

	FS_Printf( f,"%s", "unbindall\n" );

	for ( i = 0; i < MAX_KEYS; i++ )
	{
		if ( keys[ i ].binding[ 0 ] && keys[ i ].binding[ 0 ][ 0 ] )
		{
			FS_Printf( f, "bind       %s %s\n", Key_KeynumToString( i ), Cmd_QuoteString( keys[ i ].binding[ 0 ] ) );
		}

		for ( team = 1; team < MAX_TEAMS; ++team )
		{
			if ( keys[ i ].binding[ team ] && keys[ i ].binding[ team ][ 0 ] )
			{
				FS_Printf( f, "teambind %d %s %s\n", team, Key_KeynumToString( i ), Cmd_QuoteString( keys[ i ].binding[ team ] ) );
			}
		}
	}
}

/*
============
Key_Bindlist_f

============
*/
void Key_Bindlist_f( void )
{
	int i, team;

	for ( i = 0; i < MAX_KEYS; i++ )
	{
		qboolean teamSpecific = qfalse;

		for ( team = 1; team < MAX_TEAMS; ++team )
		{
			if ( keys[ i ].binding[ team ] && keys[ i ].binding[ team ][ 0 ] )
			{
				teamSpecific = qtrue;
				break;
			}
		}

		if ( !teamSpecific )
		{
			if ( keys[ i ].binding[ 0 ] && keys[ i ].binding[ 0 ][ 0 ] )
			{
				Com_Printf( "%s = %s\n", Key_KeynumToString( i ), keys[ i ].binding[ 0 ] );
			}
		}
		else
		{
			for ( team = 0; team < MAX_TEAMS; ++team )
			{
				if ( keys[ i ].binding[ team ] && keys[ i ].binding[ team ][ 0 ] )
				{
					Com_Printf( "%s[%s] = %s\n", Key_KeynumToString( i ), teamName[ team ], keys[ i ].binding[ team ] );
				}
			}
		}
	}
}

/*
============
Key_KeynameCompletion
============
*/
void Key_KeynameCompletion( void ( *callback )( const char *s ) )
{
	int i;

	for ( i = 0; keynames[ i ].name != NULL; i++ )
	{
		callback( keynames[ i ].name );
	}
}

/*
====================
Key_CompleteUnbind
====================
*/
static void Key_CompleteUnbind( char *args, int argNum )
{
	if ( argNum < 4 )
	{
		// Skip "unbind "
		char *p = Com_SkipTokens( args, 1, " " );

		if ( p > args )
		{
			Field_CompleteKeyname( argNum > 2 ? 0 : FIELD_TEAM | FIELD_TEAM_SPECTATORS | FIELD_TEAM_DEFAULT );
		}
	}
}

/*
====================
Key_CompleteBind
Key_CompleteTeambind
====================
*/
static void Key_CompleteBind_Internal( char *args, int argNum, int nameArg )
{
	// assumption: nameArg is 2 or 3
	char *p;

	if ( argNum == nameArg )
	{
		// Skip "bind "
		p = Com_SkipTokens( args, nameArg - 1, " " );

		if ( p > args )
		{
			Field_CompleteKeyname( 0 );
		}
	}
	else if ( argNum > nameArg )
	{
		// Skip "bind <key> "
		p = Com_SkipTokens( args, nameArg, " " );

		if ( p > args )
		{
			Field_CompleteCommand( p, qtrue, qtrue );
		}
	}
}

void Key_CompleteBind( char *args, int argNum )
{
	Key_CompleteBind_Internal( args, argNum, 2 );
}

void Key_CompleteTeambind( char *args, int argNum )
{
	if ( argNum == 2 )
	{
		Field_CompleteTeamname( FIELD_TEAM_SPECTATORS | FIELD_TEAM_DEFAULT );
	}
	else
	{
		Key_CompleteBind_Internal( args, argNum, 3 );
	}
}

static void Key_CompleteEditbind( char *args, int argNum )
{
	if ( argNum < 4 )
	{
		Field_CompleteKeyname( argNum > 2 ? 0 : FIELD_TEAM | FIELD_TEAM_SPECTATORS | FIELD_TEAM_DEFAULT );
	}
}

/*
===================
CL_InitKeyCommands
===================
*/
void CL_InitKeyCommands( void )
{
	// register our functions
	Cmd_AddCommand( "bind", Key_Bind_f );
	Cmd_SetCommandCompletionFunc( "bind", Key_CompleteBind );
	Cmd_AddCommand( "teambind", Key_Bind_f );
	Cmd_SetCommandCompletionFunc( "teambind", Key_CompleteTeambind );
	Cmd_AddCommand( "unbind", Key_Unbind_f );
	Cmd_SetCommandCompletionFunc( "unbind", Key_CompleteUnbind );
	Cmd_AddCommand( "unbindall", Key_Unbindall_f );
	Cmd_AddCommand( "bindlist", Key_Bindlist_f );
	Cmd_AddCommand( "editbind", Key_EditBind_f );
	Cmd_SetCommandCompletionFunc( "editbind", Key_CompleteEditbind );
}

/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
//static consoleCount = 0;
// fretn
qboolean consoleButtonWasPressed = qfalse;

void CL_KeyEvent( int key, qboolean down, unsigned time )
{
	char     *kb;
	char     cmd[ 1024 ];
	qboolean bypassMenu = qfalse; // NERVE - SMF
	qboolean onlybinds = qfalse;

	if ( !key )
	{
		return;
	}

	switch ( key )
	{
		case K_KP_PGUP:
		case K_KP_EQUALS:
		case K_KP_5:
		case K_KP_LEFTARROW:
		case K_KP_UPARROW:
		case K_KP_RIGHTARROW:
		case K_KP_DOWNARROW:
		case K_KP_END:
		case K_KP_PGDN:
		case K_KP_INS:
		case K_KP_DEL:
		case K_KP_HOME:
			if ( Sys_IsNumLockDown() )
			{
				onlybinds = qtrue;
			}

			break;
	}

	// update auto-repeat status and BUTTON_ANY status
	keys[ key ].down = down;

	if ( down )
	{
		keys[ key ].repeats++;

		if ( keys[ key ].repeats == 1 )
		{
			anykeydown++;
		}
	}
	else
	{
		keys[ key ].repeats = 0;
		anykeydown--;

		if ( anykeydown < 0 )
		{
			anykeydown = 0;
		}
	}

#ifdef MACOS_X
	if ( down && keys[ K_COMMAND ].down )
	{
		if ( key == 'f' )
		{
			Key_ClearStates();
			Cbuf_ExecuteText( EXEC_APPEND, "toggle r_fullscreen\nvid_restart\n" );
			return;
		}
		else if ( key == 'q' )
		{
			Key_ClearStates();
			Cbuf_ExecuteText( EXEC_APPEND, "quit\n" );
			return;
		}
		else if ( key == K_TAB )
		{
			Key_ClearStates();
			Cvar_SetValue( "r_minimize", 1 );
			return;
		}
	}
#else
	if ( key == K_ENTER )
	{
		if ( down )
		{
			if ( keys[ K_ALT ].down )
			{
				Cvar_SetValue( "r_fullscreen", !Cvar_VariableIntegerValue( "r_fullscreen" ) );
				return;
			}
		}
	}

	if ( cl_altTab->integer && keys[ K_ALT ].down && key == K_TAB )
	{
		Key_ClearStates();
		Cvar_SetValue( "r_minimize", 1 );
		return;
	}
#endif

	// console key is hardcoded, so the user can never unbind it
	if ( key == K_CONSOLE || ( keys[ K_SHIFT ].down && key == K_ESCAPE ) )
	{
		if ( !down )
		{
			return;
		}

		Con_ToggleConsole_f();
		Key_ClearStates();
		return;
	}

	// most keys during demo playback will bring up the menu, but non-ascii

	/* Do something better than this :)
	// keys can still be used for bound actions
	if ( down && ( key < 128 || key == K_MOUSE1 )
	         && ( clc.demoplaying || cls.state == CA_CINEMATIC ) && !cls.keyCatchers ) {

	        Cvar_Set( "nextdemo","" );
	        key = K_ESCAPE;
	}
	*/

	// escape is always handled special
	if ( key == K_ESCAPE && down )
	{
		if ( !( cls.keyCatchers & KEYCATCH_UI ) )
		{
			if ( cls.state == CA_ACTIVE && !clc.demoplaying )
			{
				// Arnout: on request
				if ( cls.keyCatchers & KEYCATCH_CONSOLE ) // get rid of the console
				{
					Con_ToggleConsole_f();
				}
				else
				{
					VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_INGAME );
				}
			}
			else
			{
				CL_Disconnect_f();
				S_StopAllSounds();
				VM_Call( uivm, UI_SET_ACTIVE_MENU, UIMENU_MAIN );
			}

			return;
		}

		VM_Call( uivm, UI_KEY_EVENT, key, down );
		return;
	}

	//
	// key up events only perform actions if the game key binding is
	// a button command (leading + sign).  These will be processed even in
	// console mode and menu mode, to keep the character from continuing
	// an action started before a mode switch.
	//
	if ( !down )
	{
		kb = keys[ key ].binding[ bindTeam ]
		   ? keys[ key ].binding[ bindTeam ] // prefer the team bind
		   : keys[ key ].binding[ 0 ];       // default to global

		if ( kb && kb[ 0 ] == '+' )
		{
			// button commands add keynum and time as parms so that multiple
			// sources can be discriminated and subframe corrected
			Com_sprintf( cmd, sizeof( cmd ), "-%s %i %i\n", kb + 1, key, time );
			Cbuf_AddText( cmd );
		}

		if ( cls.keyCatchers & KEYCATCH_UI && uivm )
		{
			if ( !onlybinds )
			{
				VM_Call( uivm, UI_KEY_EVENT, key, down );
			}
		}
		else if ( cls.keyCatchers & KEYCATCH_CGAME && cgvm )
		{
			if ( !onlybinds )
			{
				VM_Call( cgvm, CG_KEY_EVENT, key, down );
			}
		}

		return;
	}

	// distribute the key down event to the appropriate handler
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		if ( !onlybinds )
		{
			Console_Key( key );
		}
	}
	else if ( cls.keyCatchers & KEYCATCH_UI && !bypassMenu )
	{
		if ( !onlybinds )
		{
			VM_Call( uivm, UI_KEY_EVENT, key, down );
		}
	}
	else if ( cls.keyCatchers & KEYCATCH_CGAME && !bypassMenu )
	{
		if ( cgvm )
		{
			if ( !onlybinds )
			{
				VM_Call( cgvm, CG_KEY_EVENT, key, down );
			}
		}
	}
	else if ( cls.state == CA_DISCONNECTED )
	{
		if ( !onlybinds )
		{
			Console_Key( key );
		}
	}
	else
	{
		// send the bound action
		kb = keys[ key ].binding[ bindTeam ]
		   ? keys[ key ].binding[ bindTeam ] // prefer the team bind
		   : keys[ key ].binding[ 0 ];       // default to global

		if ( !kb )
		{
		}
		else if ( kb[ 0 ] == '+' )
		{
			// button commands add keynum and time as parms so that multiple
			// sources can be discriminated and subframe corrected
			Com_sprintf( cmd, sizeof( cmd ), "%s %i %i\n", kb, key, time );
			Cbuf_AddText( cmd );
		}
		else
		{
			// down-only command
			Cbuf_AddText( kb );
			Cbuf_AddText( "\n" );
		}
	}
}

/*
===================
CL_CharEvent

Characters, already shifted/capslocked/etc.
===================
*/
void CL_CharEvent( const char *key )
{
	// the console key should never be used as a char
	// ydnar: added uk equivalent of shift+`
	// the RIGHT way to do this would be to have certain keys disable the equivalent SE_CHAR event

	// fretn - this should be fixed in Com_EventLoop
	// but I can't be arsed to leave this as is

	// distribute the key down event to the appropriate handler
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		Field_CharEvent( &g_consoleField, key );
	}
	else if ( cls.keyCatchers & KEYCATCH_UI )
	{
		// VMs that don't support i18n distinguish between char and key events by looking at the 11th least significant bit.
		// Patched VMs look at the second least significant bit to determine whether the event is a char event, and at the third bit
		// to determine the original 11th least significant bit of the key.
		VM_Call( uivm, UI_KEY_EVENT, Q_UTF8Store( key ) | (1 << (K_CHAR_BIT - 1)),
				(qtrue << KEYEVSTATE_DOWN) |
				(qtrue << KEYEVSTATE_CHAR) |
				((Q_UTF8Store( key ) & (1 << (K_CHAR_BIT - 1))) >> ((K_CHAR_BIT - 1) - KEYEVSTATE_BIT)) |
				(qtrue << KEYEVSTATE_SUP) );
	}
	else if ( cls.state == CA_DISCONNECTED )
	{
		Field_CharEvent( &g_consoleField, key );
	}
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates( void )
{
	int i;

	anykeydown = 0;

	for ( i = 0; i < MAX_KEYS; i++ )
	{
		if ( keys[ i ].down )
		{
			CL_KeyEvent( i, qfalse, 0 );
		}

		keys[ i ].down = 0;
		keys[ i ].repeats = 0;
	}
}

/*
===================
Key_SetTeam
===================
*/
void Key_SetTeam( int newTeam )
{
	if ( newTeam < TEAM_NONE || newTeam >= NUM_TEAMS || newTeam >= MAX_TEAMS )
	{
		newTeam = TEAM_NONE;
	}

	if ( bindTeam != newTeam )
	{
		Com_DPrintf( "^2Setting binding team index to %d\n", newTeam );
	}

	bindTeam = newTeam;
}
