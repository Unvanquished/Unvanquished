/*
===========================================================================
Copyright (C) 2012-2013 Unvanquished Developers

This file is part of Daemon source code.

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

// Unicode & UTF-8 handling

#include "q_shared.h"
#include "q_unicode.h"

// never returns more than 4
int Q_UTF8_Width( const char *str )
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

int Q_UTF8_WidthCP( int ch )
{
	if ( ch <=   0x007F ) { return 1; }
	if ( ch <=   0x07FF ) { return 2; }
	if ( ch <=   0xFFFF ) { return 3; }
	if ( ch <= 0x10FFFF ) { return 4; }
	return 0;
}

int Q_UTF8_Strlen( const char *str )
{
  int l = 0;

  while( *str )
  {
    l++;

    str += Q_UTF8_Width( str );
  }

  return l;
}

int Q_UTF8_PrintStrlen( const char *str )
{
  int l = 0;

  while( *str )
  {
    if( Q_IsColorString( str ) )
    {
      str += 2;
      continue;
    }
    if( *str == Q_COLOR_ESCAPE && str[1] == Q_COLOR_ESCAPE )
    {
      ++str;
    }

    l++;

    str += Q_UTF8_Width( str );
  }

  return l;
}

qboolean Q_UTF8_ContByte( char c )
{
  return (unsigned char )0x80 <= (unsigned char)c && (unsigned char)c <= (unsigned char )0xBF;
}

static qboolean getbit(const unsigned char *p, int pos)
{
  p   += pos / 8;
  pos %= 8;

  return (*p & (1 << (7 - pos))) != 0;
}

static void setbit(unsigned char *p, int pos, qboolean on)
{
  p   += pos / 8;
  pos %= 8;

  if( on )
    *p |= 1 << (7 - pos);
  else
    *p &= ~(1 << (7 - pos));
}

static void shiftbitsright(unsigned char *p, unsigned long num, unsigned long by)
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

unsigned long Q_UTF8_CodePoint( const char *str )
{
  int i, j;
  int n = 0;
  int size = Q_UTF8_Width( str );
  unsigned long codepoint = 0;
  unsigned char *p = (unsigned char *) &codepoint;

  if( size > sizeof( codepoint ) )
    size = sizeof( codepoint );
  else if( size < 1 )
    size = 1;

  for( i = (size > 1 ? size + 1 : 1); i < 8; i++ )
    setbit(p, n++, getbit((const unsigned char *)str, i));
  for( i = 1; i < size; i++ )
    for( j = 2; j < 8; j++ )
      setbit(p, n++, getbit(((const unsigned char *)str) + i, j));

  /*
  if( n > 8 * sizeof(codepoint) )
  {
		Com_Error( ERR_DROP, "Q_UTF8_CodePoint: overflow caught" );

    return 0;
  }
  */

  shiftbitsright(p, 8 * sizeof(codepoint), 8 * sizeof(codepoint) - n);

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

char *Q_UTF8_Encode( unsigned long codepoint )
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
int Q_UTF8_Store( const char *s )
{
#ifdef Q3_VM
	int i = 0;
	int r = 0;
	while ( s[ i ] )
	{
		r |= ( s[ i ] & 0xFF ) << ( i * 3 );
		++i;
	}
#elif defined Q3_BIG_ENDIAN
  int r = *(int *)s, i;
  unsigned char *p = (unsigned char *) &r;
  for( i = 0; i < sizeof(r) / 2; i++ )
  {
    p[i] ^= p[sizeof(r) - 1 - i];
    p[sizeof(r) - 1 - i] ^= p[i];
    p[i] ^= p[sizeof(r) - 1 - i];
  }
#else
  int r = *(int *)s;
  // don't assume that s is NUL-padded to four bytes
  if ( ( r & 0x000000FF ) == 0 ) { return 0; }
  if ( ( r & 0x0000FF00 ) == 0 ) { return r & 0x000000FF; }
  if ( ( r & 0x00FF0000 ) == 0 ) { return r & 0x0000FFFF; }
  if ( ( r & 0xFF000000 ) == 0 ) { return r & 0x00FFFFFF; }
#endif
  return r;
}

char *Q_UTF8_Unstore( int e )
{
  static char sbuf[2][5];
  static int index = 0;
  char *buf = sbuf[index++ & 1];

#ifdef Q3_VM
	int i = 0;
	while ( e )
	{
		buf[ i++ ] = (char) e;
		e >>= 8;
	}
	buf[ i ] = 0;
#elif defined Q3_BIG_ENDIAN
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
