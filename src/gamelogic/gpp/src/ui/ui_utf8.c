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
#include "ui_utf8.h"

int ui_CursorToOffset( const char *buf, int cursor )
{
	int i = -1, j = 0;

	while ( ++i < cursor )
	{
		j += ui_UTF8Width( buf + j );
	}

	return j;
}

int ui_OffsetToCursor( const char *buf, int offset )
{
	int i = 0, j = 0;

	while ( i < offset )
	{
		i += ui_UTF8Width( buf + i );
		++j;
	}

	return j;
}

// Copied from src/engine/common/q_shared.c

int ui_UTF8Width( const char *str )
{
  int                 ewidth;
  const unsigned char *s = (const unsigned char *)str;

  if( !str )
    return 0;

  if     ( 0x00 <= *s && *s <= 0x7F )
    ewidth = 0;
  else if( 0xC2 <= *s && *s <= 0xDF )
    ewidth = 1;
  else if( 0xE0 <= *s && *s <= 0xEF )
    ewidth = 2;
  else if( 0xF0 <= *s && *s <= 0xF4 )
    ewidth = 3;
  else
    ewidth = 0;

  for( ; *s && ewidth > 0; s++, ewidth-- );

  return s - (const unsigned char *)str + 1;
}

int ui_UTF8WidthCP( int ch )
{
	if ( ch <=   0x007F ) { return 1; }
	if ( ch <=   0x07FF ) { return 2; }
	if ( ch <=   0xFFFF ) { return 3; }
	if ( ch <= 0x10FFFF ) { return 4; }
	return 0;
}

int ui_UTF8Strlen( const char *str )
{
  int l = 0;

  while( *str )
  {
    l++;

    str += ui_UTF8Width( str );
  }

  return l;
}

qboolean ui_UTF8ContByte( char c )
{
  return (unsigned char )0x80 <= (unsigned char)c && (unsigned char)c <= (unsigned char )0xBF;
}

static qboolean ui_getbit(const unsigned char *p, int pos)
{
  p   += pos / 8;
  pos %= 8;

  return (*p & (1 << (7 - pos))) != 0;
}

static void ui_setbit(unsigned char *p, int pos, qboolean on)
{
  p   += pos / 8;
  pos %= 8;

  if( on )
    *p |= 1 << (7 - pos);
  else
    *p &= ~(1 << (7 - pos));
}

static void ui_shiftbitsright(unsigned char *p, unsigned long num, unsigned long by)
{
  int step, off;
  unsigned char *e;

  if( by >= num )
  {
    for( ; num > 8; p++, num -= 8 )
      *p = 0;

    *p &= (~0x00) >> num;

    return;
  }

  step = by / 8;
  off  = by % 8;

  for( e = p + (num + 7) / 8 - 1; e > p + step; e-- )
    *e = (*(e - step) >> off) | (*(e - step - 1) << (8 - off));

  *e = *(e - step) >> off;

  for( e = p; e < p + step; e++ )
    *e = 0;
}

unsigned long ui_UTF8CodePoint( const char *str )
{
  int i, j;
  int n = 0;
  int size = ui_UTF8Width( str );
  unsigned long codepoint = 0;
  unsigned char *p = (unsigned char *) &codepoint;

  if( size > sizeof( codepoint ) )
    size = sizeof( codepoint );
  else if( size < 1 )
    size = 1;

  for( i = (size > 1 ? size + 1 : 1); i < 8; i++ )
    ui_setbit(p, n++, ui_getbit((const unsigned char *)str, i));
  for( i = 1; i < size; i++ )
    for( j = 2; j < 8; j++ )
      ui_setbit(p, n++, ui_getbit(((const unsigned char *)str) + i, j));

  /*
  if( n > 8 * sizeof(codepoint) )
  {
		Com_Error( ERR_DROP, "ui_UTF8CodePoint: overflow caught" );

    return 0;
  }
  */

  ui_shiftbitsright(p, 8 * sizeof(codepoint), 8 * sizeof(codepoint) - n);

#ifndef Q3_BIG_ENDIAN
  for( i = 0; i < sizeof(codepoint) / 2; i++ )
  {
    p[i] ^= p[sizeof(codepoint) - 1 - i];
    p[sizeof(codepoint) - 1 - i] ^= p[i];
    p[i] ^= p[sizeof(codepoint) - 1 - i];
  }
#endif

  return codepoint;
}

char *ui_UTF8Encode( unsigned long codepoint )
{
  static char sbuf[2][5];
  static int index = 0;
  char *buf = sbuf[index++ & 1];

  if     (                        codepoint <= 0x007F )
  {
    buf[0] = codepoint;
    buf[1] = 0;
  }
  else if( 0x0080 <= codepoint && codepoint <= 0x07FF )
  {
    buf[0] = 0xC0 | ((codepoint & 0x0700) >> 6) | ((codepoint & 0x00C0) >> 6);
    buf[1] = 0x80 | (codepoint & 0x003F);
    buf[2] = 0;
  }
  else if( 0x0800 <= codepoint && codepoint <= 0xFFFF )
  {
    buf[0] = 0xE0 | ((codepoint & 0xF000) >> 12);
    buf[1] = 0x80 | ((codepoint & 0x0F00) >> 6) | ((codepoint & 0x00C0) >> 6);
    buf[2] = 0x80 | (codepoint & 0x003F);
    buf[3] = 0;
  }
  else if( 0x010000 <= codepoint && codepoint <= 0x10FFFF )
  {
    buf[0] = 0xF0 | ((codepoint & 0x1C0000 >> 18));
    buf[1] = 0x80 | ((codepoint & 0x030000 >> 16)) | ((codepoint & 0x00F000) >> 12);
    buf[2] = 0x80 | ((codepoint & 0x000F00) >> 6) | ((codepoint & 0x0000C0) >> 6);
    buf[3] = 0x80 | (codepoint & 0x00003F);
    buf[4] = 0;
  }
  else
  {
    buf[0] = 0;
  }

  return buf;
}

// s needs to have at least sizeof(int) allocated
int ui_UTF8Store( const char *s )
{
#ifdef Q3_BIG_ENDIAN
  int r = *(int *)s, i;
  unsigned char *p = (unsigned char *) &r;
  for( i = 0; i < sizeof(r) / 2; i++ )
  {
    p[i] ^= p[sizeof(r) - 1 - i];
    p[sizeof(r) - 1 - i] ^= p[i];
    p[i] ^= p[sizeof(r) - 1 - i];
  }
  return r;
#else
  return *(int *)s;
#endif
}

char *ui_UTF8Unstore( int e )
{
  static char sbuf[2][5];
  static int index = 0;
  char *buf = sbuf[index++ & 1];

#ifdef Q3_BIG_ENDIAN
  int i;
  unsigned char *p = (unsigned char *) buf;
  *(int *)buf = e;
  for( i = 0; i < sizeof(e) / 2; i++ )
  {
    p[i] ^= p[sizeof(e) - 1 - i];
    p[sizeof(e) - 1 - i] ^= p[i];
    p[i] ^= p[sizeof(e) - 1 - i];
  }
#else
  *(int *)buf = e;
#endif

  return buf;
}
