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

#include "ui_local.h"
#include "../../engine/qcommon/q_unicode.h"

int ui_CursorToOffset( const char *buf, int cursor )
{
	int i = -1, j = 0;

	while ( ++i < cursor )
	{
		j += Q_UTF8_Width( buf + j );
	}

	return j;
}

int ui_OffsetToCursor( const char *buf, int offset )
{
	int i = 0, j = 0;

	while ( i < offset )
	{
		i += Q_UTF8_Width( buf + i );
		++j;
	}

	return j;
}
