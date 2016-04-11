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
#include "qcommon/q_unicode.h"
#include "framework/CommandSystem.h"
#ifdef BUILD_CLIENT
#include <SDL.h>
#endif

/*

key up events are sent even if in console mode

*/

#define CLIP(t) Math::Clamp( (t), 0, MAX_TEAMS - 1 )

Console::Field g_consoleField(INT_MAX);
bool chat_irc;

bool key_overstrikeMode;
bool bindingsModified;

int      anykeydown;
qkey_t   keys[Util::ordinal(keyNum_t::MAX_KEYS)];

int      bindTeam = DEFAULT_BINDING;

static struct {
	int          key;
	unsigned int time;
	bool     valid;
	int          check;
} plusCommand;

struct keyname_t
{
	const char *name;
	keyNum_t keynum;
};

// names not in this list can either be lowercase ascii, or '0xnn' hex sequences
static const keyname_t keynames[] =
{
	{ "TAB",                    keyNum_t::K_TAB          },
	{ "ENTER",                  keyNum_t::K_ENTER        },
	{ "ESCAPE",                 keyNum_t::K_ESCAPE       },
	{ "SPACE",                  keyNum_t::K_SPACE        },
	{ "BACKSPACE",              keyNum_t::K_BACKSPACE    },
	{ "UPARROW",                keyNum_t::K_UPARROW      },
	{ "DOWNARROW",              keyNum_t::K_DOWNARROW    },
	{ "LEFTARROW",              keyNum_t::K_LEFTARROW    },
	{ "RIGHTARROW",             keyNum_t::K_RIGHTARROW   },

	{ "BACKSLASH",              Util::enum_cast<keyNum_t>(+'\\') },

	{ "ALT",                    keyNum_t::K_ALT          },
	{ "CTRL",                   keyNum_t::K_CTRL         },
	{ "SHIFT",                  keyNum_t::K_SHIFT        },

	{ "COMMAND",                keyNum_t::K_COMMAND      },

	{ "CAPSLOCK",               keyNum_t::K_CAPSLOCK     },

	{ "F1",                     keyNum_t::K_F1           },
	{ "F2",                     keyNum_t::K_F2           },
	{ "F3",                     keyNum_t::K_F3           },
	{ "F4",                     keyNum_t::K_F4           },
	{ "F5",                     keyNum_t::K_F5           },
	{ "F6",                     keyNum_t::K_F6           },
	{ "F7",                     keyNum_t::K_F7           },
	{ "F8",                     keyNum_t::K_F8           },
	{ "F9",                     keyNum_t::K_F9           },
	{ "F10",                    keyNum_t::K_F10          },
	{ "F11",                    keyNum_t::K_F11          },
	{ "F12",                    keyNum_t::K_F12          },
	{ "F13",                    keyNum_t::K_F13          },
	{ "F14",                    keyNum_t::K_F14          },
	{ "F15",                    keyNum_t::K_F15          },

	{ "INS",                    keyNum_t::K_INS          },
	{ "DEL",                    keyNum_t::K_DEL          },
	{ "PGDN",                   keyNum_t::K_PGDN         },
	{ "PGUP",                   keyNum_t::K_PGUP         },
	{ "HOME",                   keyNum_t::K_HOME         },
	{ "END",                    keyNum_t::K_END          },

	{ "MOUSE1",                 keyNum_t::K_MOUSE1       },
	{ "MOUSE2",                 keyNum_t::K_MOUSE2       },
	{ "MOUSE3",                 keyNum_t::K_MOUSE3       },
	{ "MOUSE4",                 keyNum_t::K_MOUSE4       },
	{ "MOUSE5",                 keyNum_t::K_MOUSE5       },

	{ "MWHEELUP",               keyNum_t::K_MWHEELUP     },
	{ "MWHEELDOWN",             keyNum_t::K_MWHEELDOWN   },

	{ "JOY1",                   keyNum_t::K_JOY1         },
	{ "JOY2",                   keyNum_t::K_JOY2         },
	{ "JOY3",                   keyNum_t::K_JOY3         },
	{ "JOY4",                   keyNum_t::K_JOY4         },
	{ "JOY5",                   keyNum_t::K_JOY5         },
	{ "JOY6",                   keyNum_t::K_JOY6         },
	{ "JOY7",                   keyNum_t::K_JOY7         },
	{ "JOY8",                   keyNum_t::K_JOY8         },
	{ "JOY9",                   keyNum_t::K_JOY9         },
	{ "JOY10",                  keyNum_t::K_JOY10        },
	{ "JOY11",                  keyNum_t::K_JOY11        },
	{ "JOY12",                  keyNum_t::K_JOY12        },
	{ "JOY13",                  keyNum_t::K_JOY13        },
	{ "JOY14",                  keyNum_t::K_JOY14        },
	{ "JOY15",                  keyNum_t::K_JOY15        },
	{ "JOY16",                  keyNum_t::K_JOY16        },
	{ "JOY17",                  keyNum_t::K_JOY17        },
	{ "JOY18",                  keyNum_t::K_JOY18        },
	{ "JOY19",                  keyNum_t::K_JOY19        },
	{ "JOY20",                  keyNum_t::K_JOY20        },
	{ "JOY21",                  keyNum_t::K_JOY21        },
	{ "JOY22",                  keyNum_t::K_JOY22        },
	{ "JOY23",                  keyNum_t::K_JOY23        },
	{ "JOY24",                  keyNum_t::K_JOY24        },
	{ "JOY25",                  keyNum_t::K_JOY25        },
	{ "JOY26",                  keyNum_t::K_JOY26        },
	{ "JOY27",                  keyNum_t::K_JOY27        },
	{ "JOY28",                  keyNum_t::K_JOY28        },
	{ "JOY29",                  keyNum_t::K_JOY29        },
	{ "JOY30",                  keyNum_t::K_JOY30        },
	{ "JOY31",                  keyNum_t::K_JOY31        },
	{ "JOY32",                  keyNum_t::K_JOY32        },

	{ "AUX1",                   keyNum_t::K_AUX1         },
	{ "AUX2",                   keyNum_t::K_AUX2         },
	{ "AUX3",                   keyNum_t::K_AUX3         },
	{ "AUX4",                   keyNum_t::K_AUX4         },
	{ "AUX5",                   keyNum_t::K_AUX5         },
	{ "AUX6",                   keyNum_t::K_AUX6         },
	{ "AUX7",                   keyNum_t::K_AUX7         },
	{ "AUX8",                   keyNum_t::K_AUX8         },
	{ "AUX9",                   keyNum_t::K_AUX9         },
	{ "AUX10",                  keyNum_t::K_AUX10        },
	{ "AUX11",                  keyNum_t::K_AUX11        },
	{ "AUX12",                  keyNum_t::K_AUX12        },
	{ "AUX13",                  keyNum_t::K_AUX13        },
	{ "AUX14",                  keyNum_t::K_AUX14        },
	{ "AUX15",                  keyNum_t::K_AUX15        },
	{ "AUX16",                  keyNum_t::K_AUX16        },

	{ "KP_HOME",                keyNum_t::K_KP_HOME      },
	{ "KP_7",                   keyNum_t::K_KP_HOME      },
	{ "KP_UPARROW",             keyNum_t::K_KP_UPARROW   },
	{ "KP_8",                   keyNum_t::K_KP_UPARROW   },
	{ "KP_PGUP",                keyNum_t::K_KP_PGUP      },
	{ "KP_9",                   keyNum_t::K_KP_PGUP      },
	{ "KP_LEFTARROW",           keyNum_t::K_KP_LEFTARROW },
	{ "KP_4",                   keyNum_t::K_KP_LEFTARROW },
	{ "KP_5",                   keyNum_t::K_KP_5         },
	{ "KP_RIGHTARROW",          keyNum_t::K_KP_RIGHTARROW},
	{ "KP_6",                   keyNum_t::K_KP_RIGHTARROW},
	{ "KP_END",                 keyNum_t::K_KP_END       },
	{ "KP_1",                   keyNum_t::K_KP_END       },
	{ "KP_DOWNARROW",           keyNum_t::K_KP_DOWNARROW },
	{ "KP_2",                   keyNum_t::K_KP_DOWNARROW },
	{ "KP_PGDN",                keyNum_t::K_KP_PGDN      },
	{ "KP_3",                   keyNum_t::K_KP_PGDN      },
	{ "KP_ENTER",               keyNum_t::K_KP_ENTER     },
	{ "KP_INS",                 keyNum_t::K_KP_INS       },
	{ "KP_0",                   keyNum_t::K_KP_INS       },
	{ "KP_DEL",                 keyNum_t::K_KP_DEL       },
	{ "KP_PERIOD",              keyNum_t::K_KP_DEL       },
	{ "KP_SLASH",               keyNum_t::K_KP_SLASH     },
	{ "KP_MINUS",               keyNum_t::K_KP_MINUS     },
	{ "KP_PLUS",                keyNum_t::K_KP_PLUS      },
	{ "KP_NUMLOCK",             keyNum_t::K_KP_NUMLOCK   },
	{ "KP_STAR",                keyNum_t::K_KP_STAR      },
	{ "KP_EQUALS",              keyNum_t::K_KP_EQUALS    },

	{ "PAUSE",                  keyNum_t::K_PAUSE        },

	{ "SEMICOLON",              Util::enum_cast<keyNum_t>(+';') }, // because a raw semicolon separates commands

	{ "WORLD_0",                keyNum_t::K_WORLD_0      },
	{ "WORLD_1",                keyNum_t::K_WORLD_1      },
	{ "WORLD_2",                keyNum_t::K_WORLD_2      },
	{ "WORLD_3",                keyNum_t::K_WORLD_3      },
	{ "WORLD_4",                keyNum_t::K_WORLD_4      },
	{ "WORLD_5",                keyNum_t::K_WORLD_5      },
	{ "WORLD_6",                keyNum_t::K_WORLD_6      },
	{ "WORLD_7",                keyNum_t::K_WORLD_7      },
	{ "WORLD_8",                keyNum_t::K_WORLD_8      },
	{ "WORLD_9",                keyNum_t::K_WORLD_9      },
	{ "WORLD_10",               keyNum_t::K_WORLD_10     },
	{ "WORLD_11",               keyNum_t::K_WORLD_11     },
	{ "WORLD_12",               keyNum_t::K_WORLD_12     },
	{ "WORLD_13",               keyNum_t::K_WORLD_13     },
	{ "WORLD_14",               keyNum_t::K_WORLD_14     },
	{ "WORLD_15",               keyNum_t::K_WORLD_15     },
	{ "WORLD_16",               keyNum_t::K_WORLD_16     },
	{ "WORLD_17",               keyNum_t::K_WORLD_17     },
	{ "WORLD_18",               keyNum_t::K_WORLD_18     },
	{ "WORLD_19",               keyNum_t::K_WORLD_19     },
	{ "WORLD_20",               keyNum_t::K_WORLD_20     },
	{ "WORLD_21",               keyNum_t::K_WORLD_21     },
	{ "WORLD_22",               keyNum_t::K_WORLD_22     },
	{ "WORLD_23",               keyNum_t::K_WORLD_23     },
	{ "WORLD_24",               keyNum_t::K_WORLD_24     },
	{ "WORLD_25",               keyNum_t::K_WORLD_25     },
	{ "WORLD_26",               keyNum_t::K_WORLD_26     },
	{ "WORLD_27",               keyNum_t::K_WORLD_27     },
	{ "WORLD_28",               keyNum_t::K_WORLD_28     },
	{ "WORLD_29",               keyNum_t::K_WORLD_29     },
	{ "WORLD_30",               keyNum_t::K_WORLD_30     },
	{ "WORLD_31",               keyNum_t::K_WORLD_31     },
	{ "WORLD_32",               keyNum_t::K_WORLD_32     },
	{ "WORLD_33",               keyNum_t::K_WORLD_33     },
	{ "WORLD_34",               keyNum_t::K_WORLD_34     },
	{ "WORLD_35",               keyNum_t::K_WORLD_35     },
	{ "WORLD_36",               keyNum_t::K_WORLD_36     },
	{ "WORLD_37",               keyNum_t::K_WORLD_37     },
	{ "WORLD_38",               keyNum_t::K_WORLD_38     },
	{ "WORLD_39",               keyNum_t::K_WORLD_39     },
	{ "WORLD_40",               keyNum_t::K_WORLD_40     },
	{ "WORLD_41",               keyNum_t::K_WORLD_41     },
	{ "WORLD_42",               keyNum_t::K_WORLD_42     },
	{ "WORLD_43",               keyNum_t::K_WORLD_43     },
	{ "WORLD_44",               keyNum_t::K_WORLD_44     },
	{ "WORLD_45",               keyNum_t::K_WORLD_45     },
	{ "WORLD_46",               keyNum_t::K_WORLD_46     },
	{ "WORLD_47",               keyNum_t::K_WORLD_47     },
	{ "WORLD_48",               keyNum_t::K_WORLD_48     },
	{ "WORLD_49",               keyNum_t::K_WORLD_49     },
	{ "WORLD_50",               keyNum_t::K_WORLD_50     },
	{ "WORLD_51",               keyNum_t::K_WORLD_51     },
	{ "WORLD_52",               keyNum_t::K_WORLD_52     },
	{ "WORLD_53",               keyNum_t::K_WORLD_53     },
	{ "WORLD_54",               keyNum_t::K_WORLD_54     },
	{ "WORLD_55",               keyNum_t::K_WORLD_55     },
	{ "WORLD_56",               keyNum_t::K_WORLD_56     },
	{ "WORLD_57",               keyNum_t::K_WORLD_57     },
	{ "WORLD_58",               keyNum_t::K_WORLD_58     },
	{ "WORLD_59",               keyNum_t::K_WORLD_59     },
	{ "WORLD_60",               keyNum_t::K_WORLD_60     },
	{ "WORLD_61",               keyNum_t::K_WORLD_61     },
	{ "WORLD_62",               keyNum_t::K_WORLD_62     },
	{ "WORLD_63",               keyNum_t::K_WORLD_63     },
	{ "WORLD_64",               keyNum_t::K_WORLD_64     },
	{ "WORLD_65",               keyNum_t::K_WORLD_65     },
	{ "WORLD_66",               keyNum_t::K_WORLD_66     },
	{ "WORLD_67",               keyNum_t::K_WORLD_67     },
	{ "WORLD_68",               keyNum_t::K_WORLD_68     },
	{ "WORLD_69",               keyNum_t::K_WORLD_69     },
	{ "WORLD_70",               keyNum_t::K_WORLD_70     },
	{ "WORLD_71",               keyNum_t::K_WORLD_71     },
	{ "WORLD_72",               keyNum_t::K_WORLD_72     },
	{ "WORLD_73",               keyNum_t::K_WORLD_73     },
	{ "WORLD_74",               keyNum_t::K_WORLD_74     },
	{ "WORLD_75",               keyNum_t::K_WORLD_75     },
	{ "WORLD_76",               keyNum_t::K_WORLD_76     },
	{ "WORLD_77",               keyNum_t::K_WORLD_77     },
	{ "WORLD_78",               keyNum_t::K_WORLD_78     },
	{ "WORLD_79",               keyNum_t::K_WORLD_79     },
	{ "WORLD_80",               keyNum_t::K_WORLD_80     },
	{ "WORLD_81",               keyNum_t::K_WORLD_81     },
	{ "WORLD_82",               keyNum_t::K_WORLD_82     },
	{ "WORLD_83",               keyNum_t::K_WORLD_83     },
	{ "WORLD_84",               keyNum_t::K_WORLD_84     },
	{ "WORLD_85",               keyNum_t::K_WORLD_85     },
	{ "WORLD_86",               keyNum_t::K_WORLD_86     },
	{ "WORLD_87",               keyNum_t::K_WORLD_87     },
	{ "WORLD_88",               keyNum_t::K_WORLD_88     },
	{ "WORLD_89",               keyNum_t::K_WORLD_89     },
	{ "WORLD_90",               keyNum_t::K_WORLD_90     },
	{ "WORLD_91",               keyNum_t::K_WORLD_91     },
	{ "WORLD_92",               keyNum_t::K_WORLD_92     },
	{ "WORLD_93",               keyNum_t::K_WORLD_93     },
	{ "WORLD_94",               keyNum_t::K_WORLD_94     },
	{ "WORLD_95",               keyNum_t::K_WORLD_95     },

	{ "WINDOWS",                keyNum_t::K_SUPER        },
	{ "COMPOSE",                keyNum_t::K_COMPOSE      },
	{ "MODE",                   keyNum_t::K_MODE         },
	{ "HELP",                   keyNum_t::K_HELP         },
	{ "PRINT",                  keyNum_t::K_PRINT        },
	{ "SYSREQ",                 keyNum_t::K_SYSREQ       },
	{ "SCROLLOCK",              keyNum_t::K_SCROLLOCK    },
	{ "BREAK",                  keyNum_t::K_BREAK        },
	{ "MENU",                   keyNum_t::K_MENU         },
	{ "POWER",                  keyNum_t::K_POWER        },
	{ "EURO",                   keyNum_t::K_EURO         },
	{ "UNDO",                   keyNum_t::K_UNDO         },

	{ "XBOX360_A",              keyNum_t::K_XBOX360_A              },
	{ "XBOX360_B",              keyNum_t::K_XBOX360_B              },
	{ "XBOX360_X",              keyNum_t::K_XBOX360_X              },
	{ "XBOX360_Y",              keyNum_t::K_XBOX360_Y              },
	{ "XBOX360_LB",             keyNum_t::K_XBOX360_LB             },
	{ "XBOX360_RB",             keyNum_t::K_XBOX360_RB             },
	{ "XBOX360_START",          keyNum_t::K_XBOX360_START          },
	{ "XBOX360_GUIDE",          keyNum_t::K_XBOX360_GUIDE          },
	{ "XBOX360_LS",             keyNum_t::K_XBOX360_LS             },
	{ "XBOX360_RS",             keyNum_t::K_XBOX360_RS             },
	{ "XBOX360_BACK",           keyNum_t::K_XBOX360_BACK           },
	{ "XBOX360_LT",             keyNum_t::K_XBOX360_LT             },
	{ "XBOX360_RT",             keyNum_t::K_XBOX360_RT             },
	{ "XBOX360_DPAD_UP",        keyNum_t::K_XBOX360_DPAD_UP        },
	{ "XBOX360_DPAD_RIGHT",     keyNum_t::K_XBOX360_DPAD_RIGHT     },
	{ "XBOX360_DPAD_DOWN",      keyNum_t::K_XBOX360_DPAD_DOWN      },
	{ "XBOX360_DPAD_LEFT",      keyNum_t::K_XBOX360_DPAD_LEFT      },
	{ "XBOX360_DPAD_RIGHTUP",   keyNum_t::K_XBOX360_DPAD_RIGHTUP   },
	{ "XBOX360_DPAD_RIGHTDOWN", keyNum_t::K_XBOX360_DPAD_RIGHTDOWN },
	{ "XBOX360_DPAD_LEFTUP",    keyNum_t::K_XBOX360_DPAD_LEFTUP    },
	{ "XBOX360_DPAD_LEFTDOWN",  keyNum_t::K_XBOX360_DPAD_LEFTDOWN  },

	{ nullptr,                  Util::enum_cast<keyNum_t>(0)       }
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
void Field_Draw(const Util::LineEditData& edit, int x, int y, bool showCursor, bool noColorEscape, float alpha)
{
    //TODO support UTF-8 once LineEditData does
    //Extract the text we want to draw
    int len = edit.GetText().size();
    int lineStart = edit.GetViewStartPos();
    int cursorPos = edit.GetViewCursorPos();
    int drawWidth = std::min<size_t>(edit.GetWidth() - 1, len - lineStart);
    std::string text = Str::UTF32To8(std::u32string(edit.GetViewText(), drawWidth));

    // draw the text
	Color::Color color { 1.0f, 1.0f, 1.0f, alpha };
	SCR_DrawSmallStringExt(x, y, text.c_str(), color, false, noColorEscape);

    // draw the line scrollbar
    if (len > drawWidth) {
        static const Color::Color yellow = { 1.0f, 1.0f, 0.0f, 0.25f };
        float width = SCR_ConsoleFontStringWidth(text.c_str(), drawWidth);

        re.SetColor( yellow );
        re.DrawStretchPic(x + (width * lineStart) / len, y + 3, (width * drawWidth) / len, 2, 0, 0, 0, 0, cls.whiteShader);
    }

    // draw the cursor
    if (showCursor) {
        //Blink changes state approximately 4 times per second
        if (cls.realtime >> 8 & 1) {
            return;
        }

        Color::Color supportElementsColor = {1.0f, 1.0f, 1.0f, 0.66f * consoleState.currentAlphaFactor};
        re.SetColor( supportElementsColor );

        //Compute the position of the cursor
        float xpos, width, height;
		xpos = x + SCR_ConsoleFontStringWidth(text.c_str(), cursorPos);
		height = key_overstrikeMode ? SMALLCHAR_HEIGHT / (CONSOLE_FONT_VPADDING + 1) : 2;
		width = SMALLCHAR_WIDTH;

        re.DrawStretchPic(xpos, y + 2 - height, width, height, 0, 0, 0, 0, cls.whiteShader);
    }
}

/*
================
Field_Paste
================
*/
static void Field_Paste(Util::LineEditData& edit)
{
	int        pasteLen, width;
    char buffer[1024];

    CL_GetClipboardData(buffer, sizeof(buffer));

    const char* cbd = buffer;

	// send as if typed, so insert / overstrike works properly
	pasteLen = strlen( cbd );

	while ( pasteLen >= ( width = Q_UTF8_Width( cbd ) ) )
	{
		Field_CharEvent( edit, Q_UTF8_CodePoint( cbd ) );

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
void Field_KeyDownEvent(Util::LineEditData& edit, int key) {
    key = Str::ctolower(key);

    switch (key) {
        case K_DEL:
            edit.DeleteNext();
            break;

        case K_BACKSPACE:
            edit.DeletePrev();
            break;

        case K_RIGHTARROW:
            if (keys[ K_CTRL ].down) {
                //TODO: Skip a full word
                edit.CursorRight();
            } else {
                edit.CursorRight();
            }
            break;

        case K_LEFTARROW:
            if (keys[ K_CTRL ].down) {
                //TODO: Skip a full word
                edit.CursorLeft();
            } else {
                edit.CursorLeft();
            }
            break;

        case 'a':
            if (keys[ K_CTRL ].down) {
                edit.CursorStart();
            }
            break;

        case K_HOME:
            edit.CursorStart();
            break;

        case 'e':
            if (keys[ K_CTRL ].down) {
                edit.CursorEnd();
            }
            break;

        case K_END:
            edit.CursorEnd();
            break;

        case K_INS:
            if (keys[ K_SHIFT ].down) {
                Field_Paste(edit);
            } else {
                key_overstrikeMode = !key_overstrikeMode;
            }
            break;

        /*
        //kangz: I'm not sure we *need* this shortcut
        case 't':
            if ( keys[ K_CTRL ].down )
			if( edit->cursor)
			{
                char *p, tmp[4];

                if ( edit->cursor == len )
                {
                    --edit->cursor;
                    s = &edit->buffer[ Field_CursorToOffset( edit ) ];
                    width = Q_UTF8_Width( s );
                }

                --edit->cursor;
                p = &edit->buffer[ Field_CursorToOffset( edit ) ];
                memcpy( tmp, p, s - p );
                memmove( p, s, width );
                memcpy( p + width, tmp, s - p );
                edit->cursor += 2;
            }

            break;
        */
        case 'v':
            if (keys[ K_CTRL ].down) {
                Field_Paste( edit );
            }
            break;
        case 'd':
            if (keys[ K_CTRL ].down) {
                edit.DeleteNext();
            }
            break;
        case 'c':
        case 'u':
            if (keys[ K_CTRL ].down) {
                edit.Clear();
            }
            break;
        case 'k':
            if (keys[ K_CTRL ].down) {
				edit.DeleteEnd();
            }
            break;
    }
}

/*
==================
Field_CharEvent
==================
*/
void Field_CharEvent(Util::LineEditData& edit, int c )
{
    //
    // ignore any non printable chars
    //
    if ( c < 32 || c == 0x7f )
    {
        return;
    }

    // 'unprintable' on Mac - used for cursor keys, function keys etc.
    if ( (unsigned int)( c - 0xF700 ) < 0x200u )
    {
        return;
    }

    if (key_overstrikeMode) {
        edit.DeleteNext();
    }
    edit.AddChar(c);
}

/*
=============================================================================

CONSOLE LINE EDITING

==============================================================================
*/

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
	if (key == 'l' && keys[ K_CTRL ].down) {
		Cmd::BufferCommandText("clear");
		return;
	}

	// enter finishes the line
	if (key == K_ENTER or key == K_KP_ENTER) {

		//scroll lock state 1 or smaller will scroll down on own output
		if (con_scrollLock->integer <= 1) {
			consoleState.scrollLineIndex = consoleState.lines.size() - 1;
		}

		Log::Notice("]%s\n", Str::UTF32To8(g_consoleField.GetText()).c_str());

		// if not in the game always treat the input as a command
		if (cls.state != connstate_t::CA_ACTIVE) {
			g_consoleField.RunCommand();
		} else {
			g_consoleField.RunCommand(cl_consoleCommand->string);
		}

		if (cls.state == connstate_t::CA_DISCONNECTED) {
			SCR_UpdateScreen(); // force an update, because the command may take some time
		}
		return;
	}

	// command completion

	if ( key == K_TAB )
	{
		g_consoleField.AutoComplete();
		return;
	}

	// command history (ctrl-p ctrl-n for unix style)

	//----(SA)  added some mousewheel functionality to the console
	if ( ( key == K_MWHEELUP && keys[ K_SHIFT ].down ) || ( key == K_UPARROW ) || ( key == K_KP_UPARROW ) ||
			( ( Str::ctolower( key ) == 'p' ) && keys[ K_CTRL ].down ) )
	{
		g_consoleField.HistoryPrev();
		return;
	}

	//----(SA)  added some mousewheel functionality to the console
	if ( ( key == K_MWHEELDOWN && keys[ K_SHIFT ].down ) || ( key == K_DOWNARROW ) || ( key == K_KP_DOWNARROW ) ||
			( ( Str::ctolower( key ) == 'n' ) && keys[ K_CTRL ].down ) )
	{
		g_consoleField.HistoryNext();
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
			Con_ScrollUp( consoleState.visibleAmountOfLines );
		}

		return;
	}

	if ( key == K_MWHEELDOWN ) //----(SA) added some mousewheel functionality to the console
	{
		Con_PageDown();

		if ( keys[ K_CTRL ].down ) // hold <ctrl> to accelerate scrolling
		{
			Con_ScrollDown( consoleState.visibleAmountOfLines );
		}

		return;
	}

	// ctrl-home = top of console
	if ( ( key == K_HOME || key == K_KP_HOME ) && keys[ K_CTRL ].down )
	{
		Con_JumpUp();
		return;
	}

	// ctrl-end = bottom of console
	if ( ( key == K_END || key == K_KP_END ) && keys[ K_CTRL ].down )
	{
		Con_ScrollToBottom();
		return;
	}

	// pass to the next editline routine
	Field_KeyDownEvent(g_consoleField, key);
}

//============================================================================

/*
===================
Key_IsDown
===================
*/
bool Key_IsDown( int keynum )
{
	if ( keynum < 0 || keynum >= MAX_KEYS )
	{
		return false;
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

int Key_GetTeam( const char *arg, const char *cmd )
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
		if ( !Str::cisdigit( arg[ t ] ) )
		{
			break;
		}
	}

	if ( !arg[ t ] )
	{
		t = atoi( arg );

		if ( t != CLIP( t ) )
		{
			Log::Notice( "^3%s:^7 %d is not a valid team number\n", cmd, t );
			return -1;
		}

		return t;
	}

	l = strlen( arg );

	for ( unsigned t = 0; t < ARRAY_LEN( labels ); ++t )
	{
		// matching initial substring
		if ( !Q_strnicmp( arg, labels[ t ].label, l ) )
		{
			return labels[ t ].team;
		}
	}

fail:
	Log::Notice( "^3%s:^7 '%s^7' is not a valid team name\n", cmd, arg );
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
				keys[ keynum ].binding[ team ] = nullptr;
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
		keys[ keynum ].binding[ team ] = nullptr;
	}

	bindingsModified = true;
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
		return nullptr;
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
Key_Unbind_f
===================
*/
void Key_Unbind_f()
{
	int b = Cmd_Argc();
	int team = -1;

	if ( b < 2 || b > 3 )
	{
		Cmd_PrintUsage("[<team>] <key>", "remove commands from a key");
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
		Log::Notice( "\"%s\" isn't a valid key\n", Cmd_Argv( 1 ) );
		return;
	}

	Key_SetBinding( b, team, nullptr );
}

/*
===================
Key_Unbindall_f
===================
*/
void Key_Unbindall_f()
{
	int i;

	for ( i = 0; i < MAX_KEYS; i++ )
	{
		Key_SetBinding( i, -1, nullptr );
	}
}

/*
===================
Key_Bind_f
===================
*/
void Key_Bind_f()
{
	int        c, b;
	const char *key;
	const char *cmd = nullptr;
	int        team = -1;

	int teambind = !Q_stricmp( Cmd_Argv( 0 ), "teambind" );

	c = Cmd_Argc();

	if ( c < 2 + teambind )
	{
		Cmd_PrintUsage( teambind ? "<team> <key> [<command>]" : "<key> [<command>]", "attach a command to a key");
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
		Log::Notice( "\"%s\" isn't a valid key\n", key );
		return;
	}

	key = Key_KeynumToString( b );

	if ( !cmd )
	{
		if ( teambind )
		{
			if ( keys[ b ].binding[ team ] )
			{
				Log::Notice( "\"%s\"[%s] = %s\n", key, teamName[ team ], Cmd_QuoteString( keys[ b ].binding[ team ] ) );
			}
			else
			{
				Log::Notice( "\"%s\"[%s] is not bound\n", key, teamName[ team ] );
			}
		}
		else
		{
			bool bound = false;
			int      i;

			for ( i = 0; i < MAX_TEAMS; ++i )
			{
				if ( keys[ b ].binding[ i ] )
				{
					Log::Notice( "\"%s\"[%s] = %s\n", key, teamName[ i ], Cmd_QuoteString( keys[ b ].binding[ i ] ) );
					bound = true;
				}
			}

			if ( !bound )
			{
				Log::Notice( "\"%s\" is not bound\n", key );
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
void Key_EditBind_f()
{
	std::u32string buf;
	int            b;
	int            team = -1;

	b = Cmd_Argc();

	if ( b < 2 || b > 3 )
	{
		Cmd_PrintUsage("[<team>] <key>", nullptr);
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

	const char *key = Cmd_Argv( b - 1 );
	b = Key_StringToKeynum( key );

	if ( b == -1 )
	{
		Log::Notice( "\"%s\" isn't a valid key\n", key );
		return;
	}

	if ( team >= 0 )
	{
		buf = Str::UTF8To32("/teambind ");
		buf += Str::UTF8To32( teamName[ team ] );
		buf += Str::UTF8To32(" ");
	}
	else
	{
		buf = Str::UTF8To32("/bind ");
	}

	buf += Str::UTF8To32( Key_KeynumToString( b ) );
	buf += Str::UTF8To32(" ");

	const char *binding = Key_GetBinding( b, -team );
	if ( binding )
	{
		buf += Str::UTF8To32( Cmd::Escape( std::string( binding ) ) );
	}

	// FIXME: use text console if that's where the editbind command was entered
	Con_OpenConsole_f();
	g_consoleField.SetText( buf );
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
void Key_Bindlist_f()
{
	int i, team;

	for ( i = 0; i < MAX_KEYS; i++ )
	{
		bool teamSpecific = false;

		for ( team = 1; team < MAX_TEAMS; ++team )
		{
			if ( keys[ i ].binding[ team ] && keys[ i ].binding[ team ][ 0 ] )
			{
				teamSpecific = true;
				break;
			}
		}

		if ( !teamSpecific )
		{
			if ( keys[ i ].binding[ 0 ] && keys[ i ].binding[ 0 ][ 0 ] )
			{
				Log::Notice( "%s = %s\n", Key_KeynumToString( i ), keys[ i ].binding[ 0 ] );
			}
		}
		else
		{
			for ( team = 0; team < MAX_TEAMS; ++team )
			{
				if ( keys[ i ].binding[ team ] && keys[ i ].binding[ team ][ 0 ] )
				{
					Log::Notice( "%s[%s] = %s\n", Key_KeynumToString( i ), teamName[ team ], keys[ i ].binding[ team ] );
				}
			}
		}
	}
}

/*
============
Key_SetKeyData
============
*/
void Key_SetKeyData_f()
{
	if ( atoi( Cmd_Argv( 1 ) ) == plusCommand.check )
	{
		plusCommand.key  = atoi( Cmd_Argv( 2 ) ) - 1;
		plusCommand.time = atoi( Cmd_Argv( 3 ) );
		plusCommand.valid = true;
	}
	else
	{
		plusCommand.valid = false;
	}
}

int Key_GetKeyNumber()
{
	return plusCommand.valid ? plusCommand.key : -1;
}

unsigned int Key_GetKeyTime()
{
	return plusCommand.valid ? plusCommand.time : 0;
}

/*
===============
FindMatches

===============
*/
//TODO (kangz) rework the bind commands and their completion
static void FindMatches( const char *s )
{
    Cmd_OnCompleteMatch(s);
}

static void Field_TeamnameCompletion( void ( *callback )( const char *s ), int flags )
{
	if ( flags & FIELD_TEAM_SPECTATORS )
	{
		callback( "spectators" );
	}

	if ( flags & FIELD_TEAM_DEFAULT )
	{
		callback( "default" );
	}

	callback( "humans" );
	callback( "aliens" );
}

/*
===============
Field_CompleteKeyname
===============
*/
void Field_CompleteKeyname( int flags )
{
	if ( flags & FIELD_TEAM )
	{
		Field_TeamnameCompletion( FindMatches, flags );
	}

	Key_KeynameCompletion( FindMatches );
}

/*
===============
Field_CompleteTeamname
===============
*/
void Field_CompleteTeamname( int flags )
{
	Field_TeamnameCompletion( FindMatches, flags );
}

/*
============
Key_KeynameCompletion
============
*/
void Key_KeynameCompletion( void ( *callback )( const char *s ) )
{
	int i;

	for ( i = 0; keynames[ i ].name != nullptr; i++ )
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
		Cmd::Args arg(args);
		Cmd::CompletionResult res = Cmd::CompleteArgument(Cmd::Args(arg.EscapedArgs(nameArg)), argNum - nameArg - 1);
		for (const auto& candidate : res)
		{
			FindMatches(candidate.first.c_str());
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

static void Key_CompleteEditbind( char *, int argNum )
{
	if ( argNum < 4 )
	{
		Field_CompleteKeyname( argNum > 2 ? 0 : FIELD_TEAM | FIELD_TEAM_SPECTATORS | FIELD_TEAM_DEFAULT );
	}
}

/*
===============
Helper functions for Cmd_If_f & Cmd_ModCase_f
===============
*/
static const struct
{
	char name[ 8 ];
	unsigned short count;
	unsigned short bit;
	unsigned int index;
} modifierKeys[] =
{
	{ "shift", 5, 1, K_SHIFT },
	{ "ctrl", 4, 2, K_CTRL },
	{ "alt", 3, 4, K_ALT },
	{ "command", 7, 8, K_COMMAND },
	{ "cmd", 3, 8, K_COMMAND },
	{ "mode", 4, 16, K_MODE },
	{ "super", 5, 32, K_SUPER },
	{ "compose", 6, 64, K_COMPOSE },
	{ "menu", 7, 128, K_MENU },
	{ "", 0, 0, 0 }
};
// Following is no. of bits required for modifiers in the above list
// (it doesn't reflect the array length)
static const int NUM_RECOGNISED_MODIFIERS = 8;

struct modifierMask_t
{
	uint16_t down, up;
	int bits;
};

static modifierMask_t getModifierMask( const char *mods )
{
	int i;
	modifierMask_t mask;
	const char *ptr;
	static const modifierMask_t none = {0, 0, 0};

	mask = none;

	--mods;

	while ( *++mods == ' ' ) { /* skip leading spaces */; }

	ptr = mods;

	while ( *ptr )
	{
		int invert = ( *ptr == '!' );

		if ( invert )
		{
			++ptr;
		}

		for ( i = 0; modifierKeys[ i ].bit; ++i )
		{
			// is it this modifier?
			if ( !Q_strnicmp( ptr, modifierKeys[ i ].name, modifierKeys[ i ].count )
					&& ( ptr[ modifierKeys[ i ].count ] == ' ' ||
						ptr[ modifierKeys[ i ].count ] == ',' ||
						ptr[ modifierKeys[ i ].count ] == 0 ) )
			{
				if ( invert )
				{
					mask.up |= modifierKeys[ i ].bit;
				}
				else
				{
					mask.down |= modifierKeys[ i ].bit;
				}

				if ( ( mask.down & mask.up ) & modifierKeys[ i ].bit )
				{
					Log::Notice( "can't have %s both pressed and not pressed\n", modifierKeys[ i ].name );
					return none;
				}

				// right, parsed a word - skip it, maybe a comma, and any spaces
				ptr += modifierKeys[ i ].count - 1;

				while ( *++ptr == ' ' ) { /**/; }

				if ( *ptr == ',' )
				{
					while ( *++ptr == ' ' ) { /**/; }
				}

				// ready to parse the next one
				break;
			}
		}

		if ( !modifierKeys[ i ].bit )
		{
			Log::Notice( "unknown modifier key name in \"%s\"\n", mods );
			return none;
		}
	}

	for ( i = 0; i < NUM_RECOGNISED_MODIFIERS; ++i )
	{
		if ( mask.up & ( 1 << i ) )
		{
			++mask.bits;
		}

		if ( mask.down & ( 1 << i ) )
		{
			++mask.bits;
		}
	}

	return mask;
}

static int checkKeysDown( modifierMask_t mask )
{
	int i;

	for ( i = 0; modifierKeys[ i ].bit; ++i )
	{
		if ( ( mask.down & modifierKeys[ i ].bit ) && keys[ modifierKeys[ i ].index ].down == 0 )
		{
			return 0; // should be pressed, isn't pressed
		}

		if ( ( mask.up & modifierKeys[ i ].bit ) && keys[ modifierKeys[ i ].index ].down )
		{
			return 0; // should not be pressed, is pressed
		}
	}

	return 1; // all (not) pressed as requested
}

/*
===============
Key_ModCase_f

Takes a sequence of modifier/command pairs
Executes the command for the first matching modifier set

===============
*/
void Key_ModCase_f()
{
	int argc = Cmd_Argc();
	int index = 0;
	int max = 0;
	int count = ( argc - 1 ) / 2; // round down :-)
	const char *v;

	int mods[ 1 << NUM_RECOGNISED_MODIFIERS ];
	// want 'modifierMask_t mods[argc / 2 - 1];' (variable array, C99)
	// but MSVC apparently doesn't like that

	if ( argc < 3 )
	{
		Cmd_PrintUsage( "<modifiers> <command> [<modifiers> <command>] … [<command>]", nullptr );
		return;
	}

	while ( index < count )
	{
		modifierMask_t mask = getModifierMask( Cmd_Argv( 2 * index + 1 ) );

		if ( mask.bits == 0 )
		{
			return; // parse failure (reported) - abort
		}

		mods[ index ] = checkKeysDown( mask ) ? mask.bits : 0;

		if ( max < mods[ index ] )
		{
			max = mods[ index ];
		}

		++index;
	}

	// If we have a tail command, use it as default
	v = ( argc & 1 ) ? nullptr : Cmd_Argv( argc - 1 );

	// Search for a suitable command to execute.
	// Search is done as if the commands are sorted by modifier count
	// (descending) then parameter index no. (ascending).
	for ( ; max > 0; --max )
	{
		int i;

		for ( i = 0; i < index; ++i )
		{
			if ( mods[ i ] == max )
			{
				v = Cmd_Argv( 2 * i + 2 );
				goto found;
			}
		}
	}

found:

	if ( v && *v )
	{
		if ( *v == '/' || *v == '\\' )
		{
			Cmd::BufferCommandTextAfter(va("%s\n", v + 1), true);
		}
		else
		{
			Cmd::BufferCommandTextAfter(va("vstr %s\n", v), true);
		}
	}
}

/*
===================
CL_InitKeyCommands
===================
*/
void CL_InitKeyCommands()
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
	Cmd_AddCommand( "modcase", Key_ModCase_f );
	Cmd_SetCommandCompletionFunc( "editbind", Key_CompleteEditbind );
	Cmd_AddCommand( "setkeydata", Key_SetKeyData_f );
}

/*
===================
CL_KeyEvent

Called by the system for both key up and key down events
===================
*/
//static consoleCount = 0;
// fretn
bool consoleButtonWasPressed = false;

void CL_KeyEvent( int key, bool down, unsigned time )
{
	char     *kb;
	bool bypassMenu = false; // NERVE - SMF
	bool onlybinds = false;

	if ( key < 1 )
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
			if ( IN_IsNumLockDown() )
			{
				onlybinds = true;
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
			Cmd::BufferCommandText("toggle r_fullscreen; vid_restart");
			return;
		}
		else if ( key == 'q' )
		{
			Key_ClearStates();
			Cmd::BufferCommandText("quit");
			return;
		}
		else if ( key == K_TAB )
		{
			Key_ClearStates();
			Cmd::BufferCommandText("minimize");
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
		Cmd::BufferCommandText("minimize");
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
			if ( cls.state == connstate_t::CA_ACTIVE && !clc.demoplaying )
			{
				// Arnout: on request
				if ( cls.keyCatchers & KEYCATCH_CONSOLE ) // get rid of the console
				{
					Con_ToggleConsole_f();
				}
				else
				{
					Cmd::BufferCommandText( "toggleMenu" );
				}
			}
			else
			{
				CL_Disconnect_f();
				Audio::StopAllSounds();
			}

			return;
		}

		cgvm.CGameKeyEvent(key, down);
		return;
	}

	// Don't do anything if libRocket menus have focus
	// Everything is handled by libRocket. Also we don't want
	// to run any binds (since they won't be found).
	if ( cls.keyCatchers & KEYCATCH_UI && !( cls.keyCatchers & KEYCATCH_CONSOLE ) )
	{
		cgvm.CGameKeyEvent(key, down);
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
		// Handle any +commands which were invoked on the corresponding key-down
		Cmd::BufferCommandText(va("keyup %d %d %u", plusCommand.check, key, time));

		if ( cls.keyCatchers & KEYCATCH_CGAME && cgvm.IsActive() )
		{
			if ( !onlybinds )
			{
				cgvm.CGameKeyEvent(key, down);
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
	else if ( cls.keyCatchers & KEYCATCH_CGAME && !bypassMenu )
	{
		if ( cgvm.IsActive() )
		{
			if ( !onlybinds )
			{
				cgvm.CGameKeyEvent(key, down);
			}
		}
	}
	else if ( cls.state == connstate_t::CA_DISCONNECTED )
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

		if ( kb )
		{
			// down-only command
			Cmd::BufferCommandTextAfter(va("setkeydata %d %d %u\n%s", plusCommand.check, key + 1, time, kb), true);
			Cmd::BufferCommandTextAfter(va("setkeydata %d", plusCommand.check), true);
		}
	}
}

/*
===================
CL_CharEvent

Characters, already shifted/capslocked/etc.
===================
*/
static int CL_UTF8_unpack( int c )
{
	const char *str = Q_UTF8_Unstore( c );
	int chr = Q_UTF8_CodePoint( str );

	// filter out Apple control codes
	return (unsigned int)( chr - 0xF700 ) < 0x200u ? 0 : chr;
}

void CL_CharEvent( int c )
{
	// the console key should never be used as a char
	// ydnar: added uk equivalent of shift+`
	// the RIGHT way to do this would be to have certain keys disable the equivalent SE_CHAR event

	// fretn - this should be fixed in Com_EventLoop
	// but I can't be arsed to leave this as is

	// distribute the key down event to the appropriate handler
	if ( cls.keyCatchers & KEYCATCH_CONSOLE )
	{
		Field_CharEvent(g_consoleField, CL_UTF8_unpack(c));
	}
	else if ( cls.state == connstate_t::CA_DISCONNECTED )
	{
		Field_CharEvent(g_consoleField, CL_UTF8_unpack(c));
	}

	cgvm.CGameTextInputEvent(c);
}

/*
===================
Key_ClearStates
===================
*/
void Key_ClearStates()
{
	int i;

	anykeydown = 0;

	int oldKeyCatcher = Key_GetCatcher();
	Key_SetCatcher( 0 );

	for ( i = 0; i < MAX_KEYS; i++ )
	{
		if ( keys[ i ].down )
		{
			CL_KeyEvent( i, false, 0 );
		}

		keys[ i ].down = 0;
		keys[ i ].repeats = 0;
	}

	plusCommand.check = rand();

	Key_SetCatcher( oldKeyCatcher );
}

/*
===================
Key_SetTeam
===================
*/
void Key_SetTeam( int newTeam )
{
	if ( newTeam < 0 || newTeam >= MAX_TEAMS )
	{
		newTeam = DEFAULT_BINDING;
	}

	if ( bindTeam != newTeam )
	{
		Log::Debug( "%sSetting binding team index to %d",
			Color::ToString( Color::Green ),
			newTeam );
	}

	bindTeam = newTeam;
}
