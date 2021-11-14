/*
===========================================================================

Unvanquished GPL Source Code
Copyright (C) 2012 Unvanquished Developers

Unvanquished is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 2 of the License, or
(at your option) any later version.

Unvanquished is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.

===========================================================================
*/

struct rectDef_t
{
	float x; // horiz position
	float y; // vert position
	float w; // width
	float h; // height;
};

//
// Translation
//

#define _( text )              Trans_Gettext( text )
#define N_( text )             text
#define G_( text )             Trans_Pgettext( Trans_GenderContext( gender ), text )
#define P_( one, many, count ) Trans_GettextPlural( (one), (many), (count) )

const char* Trans_Gettext( const char *msgid ) PRINTF_TRANSLATE_ARG(1);
const char* Trans_Pgettext( const char *ctxt, const char *msgid ) PRINTF_TRANSLATE_ARG(2);
const char* Trans_GettextPlural( const char *msgid, const char *msgid_plural, int num ) PRINTF_TRANSLATE_ARG(1);

//
// Parsing
//

bool            PC_Float_Parse( int handle, float *f );
bool            PC_Color_Parse( int handle, Color::Color *c );
bool            PC_Int_Parse( int handle, int *i );
bool            PC_String_Parse( int handle, const char **out );

//
// Chat
//

Color::Color UI_GetChatColour( int which, int team );
