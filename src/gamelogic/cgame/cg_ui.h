/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

Daemon is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Daemon is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

typedef struct
{
	float x; // horiz position
	float y; // vert position
	float w; // width
	float h; // height;
}

rectDef_t;

typedef rectDef_t Rectangle;

//
// Translation
//

#define _( text )              Gettext( text )
#define N_( text )             text
#define C_( ctxt, text )       Pgettext( ctxt, text )
#define G_( text )             Pgettext( Trans_GenderContext( gender ), text )
#define P_( one, many, count ) GettextPlural( (one), (many), (count) )

const char *Gettext( const char *msgid ) PRINTF_TRANSLATE_ARG(1);
const char *Pgettext( const char *ctxt, const char *msgid ) PRINTF_TRANSLATE_ARG(2);
const char *GettextPlural( const char *msgid, const char *msgid2, int number ) PRINTF_TRANSLATE_ARG(1) PRINTF_TRANSLATE_ARG(1);

//
// Parsing
//

qboolean            String_Parse( char **p, const char **out );
qboolean            Script_Parse( char **p, const char **out );
qboolean            PC_Float_Parse( int handle, float *f );
qboolean            PC_Color_Parse( int handle, vec4_t *c );
qboolean            PC_Int_Parse( int handle, int *i );
qboolean            PC_Rect_Parse( int handle, rectDef_t *r );
qboolean            PC_String_Parse( int handle, const char **out );
qboolean            PC_Script_Parse( int handle, const char **out );

//
// Chat
//

int UI_GetChatColour( int which, int team );
