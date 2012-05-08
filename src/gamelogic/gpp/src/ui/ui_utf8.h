/*
===========================================================================

Daemon GPL Source Code
Copyright (C) 2012 Unvanquished Developers

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

===========================================================================
*/

#ifndef __UI_UTF8_H
#define __UI_UTF8_H

int ui_CursorToOffset( const char *buf, int cursor );
int ui_OffsetToCursor( const char *buf, int offset );

// Copied from src/engine/common/q_shared.c
int ui_UTF8Width( const char *str );
int ui_UTF8WidthCP( int ch );
int ui_UTF8Strlen( const char *str );
qboolean ui_UTF8ContByte( char c );
unsigned long ui_UTF8CodePoint( const char *str );
char *ui_UTF8Encode( unsigned long codepoint );
int ui_UTF8Store( const char *s );
char *ui_UTF8Unstore( int e );

#endif
