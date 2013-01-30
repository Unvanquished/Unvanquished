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

#include "keycodes.h"

#define MAX_TEAMS 4

typedef struct
{
	qboolean down;
	int      repeats; // if > 1, it is autorepeating
	char     *binding[ MAX_TEAMS ];
} qkey_t;

extern qboolean key_overstrikeMode;
extern qkey_t   keys[ MAX_KEYS ];
extern int      bindTeam;

// NOTE TTimo the declaration of field_t and Field_Clear is now in qcommon/qcommon.h

void            Field_KeyDownEvent( field_t *edit, int key );
void            Field_CharEvent( field_t *edit, const char *ch );
void            Field_Draw( field_t *edit, int x, int y, qboolean showCursor, qboolean noColorEscape, float alpha );
void            Field_BigDraw( field_t *edit, int x, int y, qboolean showCursor, qboolean noColorEscape );

extern field_t  g_consoleField;
extern field_t  chatField;
extern int      anykeydown;
extern qboolean chat_irc;

void            Key_WriteBindings( fileHandle_t f );
void            Key_SetBinding( int keynum, int team, const char *binding );
void            Key_GetBindingByString( const char *binding, int team, int *key1, int *key2 );
const char      *Key_GetBinding( int keynum, int team );
qboolean        Key_IsDown( int keynum );
qboolean        Key_GetOverstrikeMode( void );
void            Key_SetOverstrikeMode( qboolean state );
void            Key_ClearStates( void );
int             Key_GetKey( const char *binding, int team );

void            Key_SetTeam( int newTeam );

#ifndef DEDICATED
// from cl_input.c
void IN_ButtonDown( int );
void IN_ButtonUp( int );
#endif
