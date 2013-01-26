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

// q_shared.c -- stateless support routines that are included in each code dll
#include "q_shared.h"

#ifndef Q3_VM
/*
============================================================================

GROWLISTS

============================================================================
*/

// Com_Allocate / free all in one place for debugging
//extern          "C" void *Com_Allocate(int bytes);
//extern          "C" void Com_Dealloc(void *ptr);

void Com_InitGrowList( growList_t *list, int maxElements )
{
	list->maxElements = maxElements;
	list->currentElements = 0;
	list->elements = ( void ** ) Com_Allocate( list->maxElements * sizeof( void * ) );
}

void Com_DestroyGrowList( growList_t *list )
{
	Com_Dealloc( list->elements );
	memset( list, 0, sizeof( *list ) );
}

int Com_AddToGrowList( growList_t *list, void *data )
{
	void **old;

	if ( list->currentElements != list->maxElements )
	{
		list->elements[ list->currentElements ] = data;
		return list->currentElements++;
	}

	// grow, reallocate and move
	old = list->elements;

	if ( list->maxElements < 0 )
	{
		Com_Error( ERR_FATAL, "Com_AddToGrowList: maxElements = %i", list->maxElements );
	}

	if ( list->maxElements == 0 )
	{
		// initialize the list to hold 100 elements
		Com_InitGrowList( list, 100 );
		return Com_AddToGrowList( list, data );
	}

	list->maxElements *= 2;

//  Com_DPrintf("Resizing growlist to %i maxElements\n", list->maxElements);

	list->elements = ( void ** ) Com_Allocate( list->maxElements * sizeof( void * ) );

	if ( !list->elements )
	{
		Com_Error( ERR_DROP, "Growlist alloc failed" );
	}

	Com_Memcpy( list->elements, old, list->currentElements * sizeof( void * ) );

	Com_Dealloc( old );

	return Com_AddToGrowList( list, data );
}

void           *Com_GrowListElement( const growList_t *list, int index )
{
	if ( index < 0 || index >= list->currentElements )
	{
		Com_Error( ERR_DROP, "Com_GrowListElement: %i out of range of %i", index, list->currentElements );
	}

	return list->elements[ index ];
}

int Com_IndexForGrowListElement( const growList_t *list, const void *element )
{
	int i;

	for ( i = 0; i < list->currentElements; i++ )
	{
		if ( list->elements[ i ] == element )
		{
			return i;
		}
	}

	return -1;
}

//=============================================================================

memStream_t *AllocMemStream( byte *buffer, int bufSize )
{
	memStream_t *s;

	if ( buffer == NULL || bufSize <= 0 )
	{
		return NULL;
	}

	s = (memStream_t*)Com_Allocate( sizeof( memStream_t ) );

	if ( s == NULL )
	{
		return NULL;
	}

	Com_Memset( s, 0, sizeof( memStream_t ) );

	s->buffer = buffer;
	s->curPos = buffer;
	s->bufSize = bufSize;
	s->flags = 0;

	return s;
}

void FreeMemStream( memStream_t *s )
{
	Com_Dealloc( s );
}

int MemStreamRead( memStream_t *s, void *buffer, int len )
{
	int ret = 1;

	if ( s == NULL || buffer == NULL )
	{
		return 0;
	}

	if ( s->curPos + len > s->buffer + s->bufSize )
	{
		s->flags |= MEMSTREAM_FLAGS_EOF;
		len = s->buffer + s->bufSize - s->curPos;
		ret = 0;

		Com_Error( ERR_FATAL, "MemStreamRead: EOF reached" );
	}

	Com_Memcpy( buffer, s->curPos, len );
	s->curPos += len;

	return ret;
}

int MemStreamGetC( memStream_t *s )
{
	int c = 0;

	if ( s == NULL )
	{
		return -1;
	}

	if ( MemStreamRead( s, &c, 1 ) == 0 )
	{
		return -1;
	}

	return c;
}

int MemStreamGetLong( memStream_t *s )
{
	int c = 0;

	if ( s == NULL )
	{
		return -1;
	}

	if ( MemStreamRead( s, &c, 4 ) == 0 )
	{
		return -1;
	}

	return LittleLong( c );
}

int MemStreamGetShort( memStream_t *s )
{
	int c = 0;

	if ( s == NULL )
	{
		return -1;
	}

	if ( MemStreamRead( s, &c, 2 ) == 0 )
	{
		return -1;
	}

	return LittleShort( c );
}

float MemStreamGetFloat( memStream_t *s )
{
	floatint_t c;

	if ( s == NULL )
	{
		return -1;
	}

	if ( MemStreamRead( s, &c.i, 4 ) == 0 )
	{
		return -1;
	}

	return LittleFloat( c.f );
}
#endif
//=============================================================================

float Com_Clamp( float min, float max, float value )
{
	if ( value < min )
	{
		return min;
	}

	if ( value > max )
	{
		return max;
	}

	return value;
}

/*
COM_FixPath()
unixifies a pathname
*/

void COM_FixPath( char *pathname )
{
	while ( *pathname )
	{
		if ( *pathname == '\\' )
		{
			*pathname = '/';
		}

		pathname++;
	}
}

/*
============
COM_SkipPath
============
*/
char *COM_SkipPath( char *pathname )
{
	char *last;

	last = pathname;

	while ( *pathname )
	{
		if ( *pathname == '/' )
		{
			last = pathname + 1;
		}

		pathname++;
	}

	return last;
}

/*
==================
Com_CharIsOneOfCharset
==================
*/
static qboolean Com_CharIsOneOfCharset( char c, char *set )
{
	int i;

	for ( i = 0; i < strlen( set ); i++ )
	{
		if ( set[ i ] == c )
		{
			return qtrue;
		}
	}

	return qfalse;
}

/*
==================
Com_SkipCharset
==================
*/
char *Com_SkipCharset( char *s, char *sep )
{
	char *p = s;

	while ( p )
	{
		if ( Com_CharIsOneOfCharset( *p, sep ) )
		{
			p++;
		}
		else
		{
			break;
		}
	}

	return p;
}

/*
==================
Com_SkipTokens
==================
*/
char *Com_SkipTokens( char *s, int numTokens, char *sep )
{
	int  sepCount = 0;
	char *p = s;

	while ( sepCount < numTokens )
	{
		if ( Com_CharIsOneOfCharset( *p++, sep ) )
		{
			sepCount++;

			while ( Com_CharIsOneOfCharset( *p, sep ) )
			{
				p++;
			}
		}
		else if ( *p == '\0' )
		{
			break;
		}
	}

	if ( sepCount == numTokens )
	{
		return p;
	}
	else
	{
		return s;
	}
}

/*
============
COM_GetExtension
============
*/
const char     *COM_GetExtension( const char *name )
{
	int length, i;

	length = strlen( name ) - 1;
	i = length;

	if ( !i )
	{
		return "";
	}

	while ( name[ i ] != '.' )
	{
		i--;

		if ( name[ i ] == '/' || i == 0 )
		{
			return ""; // no extension
		}
	}

	return &name[ i + 1 ];
}

/*
============
COM_StripExtension
============
*/
void COM_StripExtension( const char *in, char *out )
{
	while ( *in && *in != '.' )
	{
		*out++ = *in++;
	}

	*out = 0;
}

/*
============
COM_StripExtension2
a safer version
============
*/
void COM_StripExtension2( const char *in, char *out, int destsize )
{
	int len = 0;

	while ( len < destsize - 1 && *in && *in != '.' )
	{
		*out++ = *in++;
		len++;
	}

	*out = 0;
}

void COM_StripFilename( char *in, char *out )
{
	char *end;
	Q_strncpyz( out, in, strlen( in ) + 1 );
	end = COM_SkipPath( out );
	*end = 0;
}

/*
============
COM_StripExtension3

RB: ioquake3 version
============
*/
void COM_StripExtension3( const char *src, char *dest, int destsize )
{
	int length;

	Q_strncpyz( dest, src, destsize );

	length = strlen( dest ) - 1;

	while ( length > 0 && dest[ length ] != '.' )
	{
		length--;

		if ( dest[ length ] == '/' )
		{
			return; // no extension
		}
	}

	if ( length )
	{
		dest[ length ] = 0;
	}
}

/*
==================
COM_DefaultExtension
==================
*/
void COM_DefaultExtension( char *path, int maxSize, const char *extension )
{
	char oldPath[ MAX_QPATH ];
	char *src;

//
// if path doesn't have a .EXT, append extension
// (extension should include the .)
//
	src = path + strlen( path ) - 1;

	while ( *src != '/' && src != path )
	{
		if ( *src == '.' )
		{
			return; // it has an extension
		}

		src--;
	}

	Q_strncpyz( oldPath, path, sizeof( oldPath ) );
	Com_sprintf( path, maxSize, "%s%s", oldPath, extension );
}

//============================================================================

/*
============
Com_HashKey
============
*/
int Com_HashKey( char *string, int maxlen )
{
	register int hash, i;

	hash = 0;

	for ( i = 0; i < maxlen && string[ i ] != '\0'; i++ )
	{
		hash += string[ i ] * ( 119 + i );
	}

	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) );
	return hash;
}

//============================================================================

/*
==================
COM_BitCheck

  Allows bit-wise checks on arrays with more than one item (> 32 bits)
==================
*/
qboolean COM_BitCheck( const int array[], int bitNum )
{
	int i;

	i = 0;

	while ( bitNum > 31 )
	{
		i++;
		bitNum -= 32;
	}

	return ( ( array[ i ] & ( 1 << bitNum ) ) != 0 ); // (SA) heh, whoops. :)
}

/*
==================
COM_BitSet

  Allows bit-wise SETS on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitSet( int array[], int bitNum )
{
	int i;

	i = 0;

	while ( bitNum > 31 )
	{
		i++;
		bitNum -= 32;
	}

	array[ i ] |= ( 1 << bitNum );
}

/*
==================
COM_BitClear

  Allows bit-wise CLEAR on arrays with more than one item (> 32 bits)
==================
*/
void COM_BitClear( int array[], int bitNum )
{
	int i;

	i = 0;

	while ( bitNum > 31 )
	{
		i++;
		bitNum -= 32;
	}

	array[ i ] &= ~( 1 << bitNum );
}

//============================================================================

/*
============================================================================

                                        BYTE ORDER FUNCTIONS

============================================================================
*/

short   ShortSwap( short l )
{
	byte b1, b2;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;

	return ( b1 << 8 ) + b2;
}

short   ShortNoSwap( short l )
{
	return l;
}

int    LongSwap( int l )
{
	byte b1, b2, b3, b4;

	b1 = l & 255;
	b2 = ( l >> 8 ) & 255;
	b3 = ( l >> 16 ) & 255;
	b4 = ( l >> 24 ) & 255;

	return ( ( int ) b1 << 24 ) + ( ( int ) b2 << 16 ) + ( ( int ) b3 << 8 ) + b4;
}

int LongNoSwap( int l )
{
	return l;
}

float FloatSwap( float f )
{
	union
	{
		float f;
		byte  b[ 4 ];
	} dat1, dat2;

	dat1.f = f;
	dat2.b[ 0 ] = dat1.b[ 3 ];
	dat2.b[ 1 ] = dat1.b[ 2 ];
	dat2.b[ 2 ] = dat1.b[ 1 ];
	dat2.b[ 3 ] = dat1.b[ 0 ];
	return dat2.f;
}

float FloatNoSwap( float f )
{
	return f;
}

/*
============================================================================

PARSING

============================================================================
*/

// multiple character punctuation tokens
static const char *punctuation[] =
{
	"+=", "-=", "*=", "/=", "&=", "|=", "++", "--",
	"&&", "||", "<=", ">=", "==", "!=",
	NULL
};

static char com_token[ MAX_TOKEN_CHARS ];
static char com_parsename[ MAX_TOKEN_CHARS ];
static int  com_lines;

static int  backup_lines;
static char *backup_text;

void COM_BeginParseSession( const char *name )
{
	com_lines = 0;
	Com_sprintf( com_parsename, sizeof( com_parsename ), "%s", name );
}

void COM_BackupParseSession( char **data_p )
{
	backup_lines = com_lines;
	backup_text = *data_p;
}

void COM_RestoreParseSession( char **data_p )
{
	com_lines = backup_lines;
	*data_p = backup_text;
}

void COM_SetCurrentParseLine( int line )
{
	com_lines = line;
}

int COM_GetCurrentParseLine( void )
{
	return com_lines;
}

char *COM_Parse( char **data_p )
{
	return COM_ParseExt( data_p, qtrue );
}

void PRINTF_LIKE(1) COM_ParseError( char *format, ... )
{
	va_list     argptr;
	static char string[ 4096 ];

	va_start( argptr, format );
	Q_vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );

	Com_Printf( S_COLOR_RED "ERROR: %s, line %d: %s\n", com_parsename, com_lines, string );
}

void PRINTF_LIKE(1) COM_ParseWarning( char *format, ... )
{
	va_list     argptr;
	static char string[ 4096 ];

	va_start( argptr, format );
	Q_vsnprintf( string, sizeof( string ), format, argptr );
	va_end( argptr );

	Com_Printf( "WARNING: %s, line %d: %s\n", com_parsename, com_lines, string );
}

/*
==============
COM_Parse

Parse a token out of a string
Will never return NULL, just empty strings

If "allowLineBreaks" is qtrue then an empty
string will be returned if the next token is
a newline.
==============
*/
static char *SkipWhitespace( char *data, qboolean *hasNewLines )
{
	int c;

	while ( ( c = *data ) <= ' ' )
	{
		if ( !c )
		{
			return NULL;
		}

		if ( c == '\n' )
		{
			com_lines++;
			*hasNewLines = qtrue;
		}

		data++;
	}

	return data;
}

int COM_Compress( char *data_p )
{
	char     *datai, *datao;
	int      c, size;
	qboolean ws = qfalse;

	size = 0;
	datai = datao = data_p;

	if ( datai )
	{
		while ( ( c = *datai ) != 0 )
		{
			if ( c == 13 || c == 10 )
			{
				*datao = c;
				datao++;
				ws = qfalse;
				datai++;
				size++;
				// skip double slash comments
			}
			else if ( c == '/' && datai[ 1 ] == '/' )
			{
				while ( *datai && *datai != '\n' )
				{
					datai++;
				}

				ws = qfalse;
				// skip /* */ comments
			}
			else if ( c == '/' && datai[ 1 ] == '*' )
			{
				datai += 2; // Arnout: skip over '/*'

				while ( *datai && ( *datai != '*' || datai[ 1 ] != '/' ) )
				{
					datai++;
				}

				if ( *datai )
				{
					datai += 2;
				}

				ws = qfalse;
			}
			else
			{
				if ( ws )
				{
					*datao = ' ';
					datao++;
				}

				*datao = c;
				datao++;
				datai++;
				ws = qfalse;
				size++;
			}
		}
	}

	*datao = 0;
	return size;
}

char *COM_ParseExt( char **data_p, qboolean allowLineBreaks )
{
	int      c = 0, len;
	qboolean hasNewLines = qfalse;
	char     *data;

	data = *data_p;
	len = 0;
	com_token[ 0 ] = 0;

	// make sure incoming data is valid
	if ( !data )
	{
		*data_p = NULL;
		return com_token;
	}

	// RF, backup the session data so we can unget easily
	COM_BackupParseSession( data_p );

	while ( 1 )
	{
		// skip whitespace
		data = SkipWhitespace( data, &hasNewLines );

		if ( !data )
		{
			*data_p = NULL;
			return com_token;
		}

		if ( hasNewLines && !allowLineBreaks )
		{
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[ 1 ] == '/' )
		{
			data += 2;

			while ( *data && *data != '\n' )
			{
				data++;
			}

			//com_lines++;
		}
		// skip /* */ comments
		else if ( c == '/' && data[ 1 ] == '*' )
		{
			data += 2;

			while ( *data && ( *data != '*' || data[ 1 ] != '/' ) )
			{
				data++;

				//if ( *data == '\n' )
				//{
				//	com_lines++;
				//}
			}

			if ( *data )
			{
				data += 2;
			}
		}
		else
		{
			break;
		}
	}

	// handle quoted strings
	if ( c == '\"' )
	{
		data++;

		while ( 1 )
		{
			c = *data++;

			if ( c == '\\' && *data == '\"' )
			{
				// Arnout: string-in-string
				if ( len < MAX_TOKEN_CHARS )
				{
					com_token[ len ] = '\"';
					len++;
				}

				data++;

				while ( 1 )
				{
					c = *data++;

					if ( !c )
					{
						com_token[ len ] = 0;
						*data_p = ( char * ) data;
						break;
					}

					if ( ( c == '\\' && *data == '\"' ) )
					{
						if ( len < MAX_TOKEN_CHARS )
						{
							com_token[ len ] = '\"';
							len++;
						}

						data++;
						c = *data++;
						break;
					}

					if ( len < MAX_TOKEN_CHARS )
					{
						com_token[ len ] = c;
						len++;
					}
				}
			}

			if ( c == '\"' || !c )
			{
				com_token[ len ] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}

			if ( len < MAX_TOKEN_CHARS )
			{
				com_token[ len ] = c;
				len++;
			}
		}
	}

	// parse a regular word
	do
	{
		if ( len < MAX_TOKEN_CHARS )
		{
			com_token[ len ] = c;
			len++;
		}

		data++;
		c = *data;

		if ( c == '\n' )
		{
			com_lines++;
		}
	}
	while ( c > 32 );

	if ( len == MAX_TOKEN_CHARS )
	{
//		Com_Printf ("Token exceeded %i chars, discarded.\n", MAX_TOKEN_CHARS);
		len = 0;
	}

	com_token[ len ] = 0;

	*data_p = ( char * ) data;
	return com_token;
}

char           *COM_Parse2( char **data_p )
{
	return COM_ParseExt2( data_p, qtrue );
}

// *INDENT-OFF*
char           *COM_ParseExt2( char **data_p, qboolean allowLineBreaks )
{
	int        c = 0, len;
	qboolean   hasNewLines = qfalse;
	char       *data;
	const char **punc;

	if ( !data_p )
	{
		Com_Error( ERR_FATAL, "COM_ParseExt: NULL data_p" );
	}

	data = *data_p;
	len = 0;
	com_token[ 0 ] = 0;

	// make sure incoming data is valid
	if ( !data )
	{
		*data_p = NULL;
		return com_token;
	}

	// RF, backup the session data so we can unget easily
	COM_BackupParseSession( data_p );

	// skip whitespace
	while ( 1 )
	{
		data = SkipWhitespace( data, &hasNewLines );

		if ( !data )
		{
			*data_p = NULL;
			return com_token;
		}

		if ( hasNewLines && !allowLineBreaks )
		{
			*data_p = data;
			return com_token;
		}

		c = *data;

		// skip double slash comments
		if ( c == '/' && data[ 1 ] == '/' )
		{
			data += 2;

			while ( *data && *data != '\n' )
			{
				data++;
			}
		}
		// skip /* */ comments
		else if ( c == '/' && data[ 1 ] == '*' )
		{
			data += 2;

			while ( *data && ( *data != '*' || data[ 1 ] != '/' ) )
			{
				data++;
			}

			if ( *data )
			{
				data += 2;
			}
		}
		else
		{
			// a real token to parse
			break;
		}
	}

	// handle quoted strings
	if ( c == '\"' )
	{
		data++;

		while ( 1 )
		{
			c = *data++;

			if ( ( c == '\\' ) && ( *data == '\"' ) )
			{
				// allow quoted strings to use \" to indicate the " character
				data++;
			}
			else if ( c == '\"' || !c )
			{
				com_token[ len ] = 0;
				*data_p = ( char * ) data;
				return com_token;
			}
			else if ( *data == '\n' )
			{
				com_lines++;
			}

			if ( len < MAX_TOKEN_CHARS - 1 )
			{
				com_token[ len ] = c;
				len++;
			}
		}
	}

	// check for a number
	// is this parsing of negative numbers going to cause expression problems
	if ( ( c >= '0' && c <= '9' ) ||
	     ( c == '-' && data[ 1 ] >= '0' && data[ 1 ] <= '9' ) ||
	     ( c == '.' && data[ 1 ] >= '0' && data[ 1 ] <= '9' ) ||
	     ( c == '-' && data[ 1 ] == '.' && data[ 2 ] >= '0' && data[ 2 ] <= '9' ) )
	{
		do
		{
			if ( len < MAX_TOKEN_CHARS - 1 )
			{
				com_token[ len ] = c;
				len++;
			}

			data++;

			c = *data;
		}
		while ( ( c >= '0' && c <= '9' ) || c == '.' );

		// parse the exponent
		if ( c == 'e' || c == 'E' )
		{
			if ( len < MAX_TOKEN_CHARS - 1 )
			{
				com_token[ len ] = c;
				len++;
			}

			data++;
			c = *data;

			if ( c == '-' || c == '+' )
			{
				if ( len < MAX_TOKEN_CHARS - 1 )
				{
					com_token[ len ] = c;
					len++;
				}

				data++;
				c = *data;
			}

			do
			{
				if ( len < MAX_TOKEN_CHARS - 1 )
				{
					com_token[ len ] = c;
					len++;
				}

				data++;

				c = *data;
			}
			while ( c >= '0' && c <= '9' );
		}

		if ( len == MAX_TOKEN_CHARS )
		{
			len = 0;
		}

		com_token[ len ] = 0;

		*data_p = ( char * ) data;
		return com_token;
	}

	// check for a regular word
	// we still allow forward and back slashes in name tokens for pathnames
	// and also colons for drive letters
	if ( ( c >= 'a' && c <= 'z' ) ||
	     ( c >= 'A' && c <= 'Z' ) ||
	     ( c == '_' ) ||
	     ( c == '/' ) ||
	     ( c == '\\' ) ||
	     ( c == '$' ) || ( c == '*' ) ) // Tr3B - for bad shader strings
	{
		do
		{
			if ( len < MAX_TOKEN_CHARS - 1 )
			{
				com_token[ len ] = c;
				len++;
			}

			data++;

			c = *data;
		}
		while
		( ( c >= 'a' && c <= 'z' ) ||
		    ( c >= 'A' && c <= 'Z' ) ||
		    ( c == '_' ) ||
		    ( c == '-' ) ||
		    ( c >= '0' && c <= '9' ) ||
		    ( c == '/' ) ||
		    ( c == '\\' ) ||
		    ( c == ':' ) ||
		    ( c == '.' ) ||
		    ( c == '$' ) ||
		    ( c == '*' ) ||
		    ( c == '@' ) );

		if ( len == MAX_TOKEN_CHARS )
		{
			len = 0;
		}

		com_token[ len ] = 0;

		*data_p = ( char * ) data;
		return com_token;
	}

	// check for multi-character punctuation token
	for ( punc = punctuation; *punc; punc++ )
	{
		int l;
		int j;

		l = strlen( *punc );

		for ( j = 0; j < l; j++ )
		{
			if ( data[ j ] != ( *punc ) [ j ] )
			{
				break;
			}
		}

		if ( j == l )
		{
			// a valid multi-character punctuation
			Com_Memcpy( com_token, *punc, l );
			com_token[ l ] = 0;
			data += l;
			*data_p = ( char * ) data;
			return com_token;
		}
	}

	// single character punctuation
	com_token[ 0 ] = *data;
	com_token[ 1 ] = 0;
	data++;
	*data_p = ( char * ) data;

	return com_token;
}

// *INDENT-ON*

/*
==================
COM_MatchToken
==================
*/
void COM_MatchToken( char **buf_p, char *match )
{
	char *token;

	token = COM_Parse( buf_p );

	if ( strcmp( token, match ) )
	{
		Com_Error( ERR_DROP, "MatchToken: %s != %s", token, match );
	}
}

/*
=================
SkipBracedSection_Depth

=================
*/
qboolean SkipBracedSection_Depth( char **program, int depth )
{
	char *token;

	do
	{
		token = COM_ParseExt( program, qtrue );

		if ( token[ 1 ] == 0 )
		{
			if ( token[ 0 ] == '{' )
			{
				depth++;
			}
			else if ( token[ 0 ] == '}' )
			{
				depth--;
			}
		}
	}
	while ( depth && *program );

	return depth == 0;
}

/*
=================
SkipBracedSection

The next token should be an open brace.
Skips until a matching close brace is found or the end of the input is reached.
Internal brace depths are properly skipped.
Returns whether the close brace was found.
=================
*/
qboolean SkipBracedSection( char **program )
{
	char *token;
	int  depth;

	depth = 0;

	do
	{
		token = COM_ParseExt( program, qtrue );

		if ( token[ 1 ] == 0 )
		{
			if ( token[ 0 ] == '{' )
			{
				depth++;
			}
			else if ( token[ 0 ] == '}' )
			{
				depth--;
			}
		}
	}
	while ( depth && *program );

	return depth == 0;
}

/*
=================
SkipRestOfLine
=================
*/
void SkipRestOfLine( char **data )
{
	char *p;
	int  c;

	p = *data;

	while ( ( c = *p++ ) != 0 )
	{
		if ( c == '\n' )
		{
			com_lines++;
			break;
		}
	}

	*data = p;
}

void Parse1DMatrix( char **buf_p, int x, float *m )
{
	char *token;
	int  i;

	COM_MatchToken( buf_p, "(" );

	for ( i = 0; i < x; i++ )
	{
		token = COM_Parse( buf_p );
		m[ i ] = atof( token );
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse2DMatrix( char **buf_p, int y, int x, float *m )
{
	int i;

	COM_MatchToken( buf_p, "(" );

	for ( i = 0; i < y; i++ )
	{
		Parse1DMatrix( buf_p, x, m + i * x );
	}

	COM_MatchToken( buf_p, ")" );
}

void Parse3DMatrix( char **buf_p, int z, int y, int x, float *m )
{
	int i;

	COM_MatchToken( buf_p, "(" );

	for ( i = 0; i < z; i++ )
	{
		Parse2DMatrix( buf_p, y, x, m + i * x * y );
	}

	COM_MatchToken( buf_p, ")" );
}

/*
===============
Com_ParseInfos
===============
*/
int Com_ParseInfos( char *buf, int max, char infos[][ MAX_INFO_STRING ] )
{
	const char *token;
	int        count;
	char       key[ MAX_TOKEN_CHARS ];

	count = 0;

	while ( 1 )
	{
		token = COM_Parse( &buf );

		if ( !token[ 0 ] )
		{
			break;
		}

		if ( strcmp( token, "{" ) )
		{
			Com_Printf( "Missing { in info file\n" );
			break;
		}

		if ( count == max )
		{
			Com_Printf( "Max infos exceeded\n" );
			break;
		}

		infos[ count ][ 0 ] = 0;

		while ( 1 )
		{
			token = COM_Parse( &buf );

			if ( !token[ 0 ] )
			{
				Com_Printf( "Unexpected end of info file\n" );
				break;
			}

			if ( !strcmp( token, "}" ) )
			{
				break;
			}

			Q_strncpyz( key, token, sizeof( key ) );

			token = COM_ParseExt( &buf, qfalse );

			if ( !token[ 0 ] )
			{
				token = "<NULL>";
			}

			Info_SetValueForKey( infos[ count ], key, token, qfalse );
		}

		count++;
	}

	return count;
}

void Com_Parse1DMatrix( char **buf_p, int x, float *m, qboolean checkBrackets )
{
	char *token;
	int  i;

	if ( checkBrackets )
	{
		COM_MatchToken( buf_p, "(" );
	}

	for ( i = 0; i < x; i++ )
	{
		token = COM_Parse2( buf_p );
		m[ i ] = atof( token );
	}

	if ( checkBrackets )
	{
		COM_MatchToken( buf_p, ")" );
	}
}

void Com_Parse2DMatrix( char **buf_p, int y, int x, float *m )
{
	int i;

	COM_MatchToken( buf_p, "(" );

	for ( i = 0; i < y; i++ )
	{
		Com_Parse1DMatrix( buf_p, x, m + i * x, qtrue );
	}

	COM_MatchToken( buf_p, ")" );
}

void Com_Parse3DMatrix( char **buf_p, int z, int y, int x, float *m )
{
	int i;

	COM_MatchToken( buf_p, "(" );

	for ( i = 0; i < z; i++ )
	{
		Com_Parse2DMatrix( buf_p, y, x, m + i * x * y );
	}

	COM_MatchToken( buf_p, ")" );
}

/*
===================
Com_HexStrToInt
===================
*/
int Com_HexStrToInt( const char *str )
{
	if ( !str || !str[ 0 ] )
	{
		return -1;
	}

	// check for hex code
	if ( str[ 0 ] == '0' && str[ 1 ] == 'x' )
	{
		int i, n = 0;

		for ( i = 2; i < strlen( str ); i++ )
		{
			char digit;

			n *= 16;

			digit = tolower( str[ i ] );

			if ( digit >= '0' && digit <= '9' )
			{
				digit -= '0';
			}
			else if ( digit >= 'a' && digit <= 'f' )
			{
				digit = digit - 'a' + 10;
			}
			else
			{
				return -1;
			}

			n += digit;
		}

		return n;
	}

	return -1;
}

#ifndef Q3_VM
/*
===================
Com_QuoteStr
===================
*/
const char *Com_QuoteStr( const char *str )
{
	static char   *buf = NULL;
	static size_t buflen = 0;

	size_t        length;
	char          *ptr;

	// quick exit if no quoting is needed
//	if (!strpbrk (str, "\";"))
//		return str;

	length = strlen( str );

	if ( buflen < 2 * length + 3 )
	{
		free( buf );
		buflen = 2 * length + 3;
		buf = (char*)Com_Allocate( buflen );
	}

	ptr = buf;
	*ptr++ = '"';
	--str;

	while ( *++str )
	{
		if ( *str == '"' )
		{
			*ptr++ = '\\';
		}

		*ptr++ = *str;
	}

	ptr[ 0 ] = '"';
	ptr[ 1 ] = 0;

	return buf;
}

/*
===================
Com_UnquoteStr
===================
*/
const char *Com_UnquoteStr( const char *str )
{
	static char *buf = NULL;

	size_t      length;
	char        *ptr;
	const char  *end;

	end = str + strlen( str );

	// Strip trailing spaces
	while ( --end >= str )
	{
		if ( *end != ' ' )
		{
			break;
		}
	}

	// end points at the last non-space character

	// If it doesn't begin with '"', return quickly
	if ( *str != '"' )
	{
		length = end + 1 - str;
		free( buf );
		buf = (char*)Com_Allocate( length + 1 );
		strncpy( buf, str, length );
		buf[ length ] = 0;
		return buf;
	}

	// It begins with '"'; if it ends with '"', lose that '"'
	if ( end > str && *end == '"' )
	{
		--end;
	}

	free( buf );
	buf = (char*)Com_Allocate( end + 1 - str );
	ptr = buf;

	// Copy, unquoting as we go
	// str[0] == '"', so that gets skipped
	while ( ++str <= end )
	{
		if ( str[ 0 ] == '\\' && str[ 1 ] == '"' && str < end ) // FIXME: \ semantics are broken
		{
			++str;
		}

		*ptr++ = *str;
	}

	*ptr = 0;

	return buf;
}


/*
============
Com_ClearForeignCharacters
some cvar values need to be safe from foreign characters
============
*/
const char *Com_ClearForeignCharacters( const char *str )
{
	static char *clean = NULL; // much longer than needed
	int          i, j, size;

	free( clean );
	size = strlen( str );
	clean = (char*)Com_Allocate ( size + 1 ); // guaranteed sufficient

	i = j = 0;

	while ( str[ i ] != '\0' )
	{
		int c = str[i] & 0xFF;
		if ( c < 0x80 )
		{
			if ( j == size )                 break; // out of buffer space
			clean[ j++ ] = str[ i++ ];
		}
		else if ( c >= 0xC2 && c <= 0xF4 )
		{
			int u, width = Q_UTF8Width( str + i );

			if ( j + width > size )          break; // out of buffer space

			if ( width == 1 )                continue; // should be multibyte

			u = Q_UTF8CodePoint( str + i );

			// Filtering out...
			if ( Q_UTF8WidthCP( u ) != width ) continue; // over-long form
			if ( u == 0xFEFF || u == 0xFFFE )  continue; // BOM
			if ( u >= 0x80 && u < 0xA0 )       continue; // undefined (from ISO8859-1)
			if ( u >= 0xD800 && u < 0xE000 )   continue; // UTF-16 surrogate halves
			if ( u >= 0x110000 )               continue; // out of range

			// width is in the range 1..4
			switch ( width )
			{
			case 4: clean[ j++ ] = str[ i++ ];
			case 3: clean[ j++ ] = str[ i++ ];
			case 2: clean[ j++ ] = str[ i++ ];
			case 1: clean[ j++ ] = str[ i++ ];
			}
		}
		// else invalid
	}

	clean[ j ] = '\0';

	return clean;
}
#endif
/*
============================================================================

                                        LIBRARY REPLACEMENT FUNCTIONS

============================================================================
*/

int Q_isprint( int c )
{
	if ( c >= 0x20 && c <= 0x7E )
	{
		return ( 1 );
	}

	return ( 0 );
}

int Q_islower( int c )
{
	if ( c >= 'a' && c <= 'z' )
	{
		return ( 1 );
	}

	return ( 0 );
}

int Q_isupper( int c )
{
	if ( c >= 'A' && c <= 'Z' )
	{
		return ( 1 );
	}

	return ( 0 );
}

int Q_isalpha( int c )
{
	if ( ( c >= 'a' && c <= 'z' ) || ( c >= 'A' && c <= 'Z' ) )
	{
		return ( 1 );
	}

	return ( 0 );
}

int Q_isnumeric( int c )
{
	if ( c >= '0' && c <= '9' )
	{
		return ( 1 );
	}

	return ( 0 );
}

int Q_isalphanumeric( int c )
{
	if ( Q_isalpha( c ) ||
	     Q_isnumeric( c ) )
	{
		return ( 1 );
	}

	return ( 0 );
}

int Q_isforfilename( int c )
{
	if ( ( Q_isalphanumeric( c ) || c == '_' ) && c != ' ' )  // space not allowed in filename
	{
		return ( 1 );
	}

	return ( 0 );
}

char *Q_strrchr( const char *string, int c )
{
	char cc = c;
	char *s;
	char *sp = ( char * ) 0;

	s = ( char * ) string;

	while ( *s )
	{
		if ( *s == cc )
		{
			sp = s;
		}

		s++;
	}

	if ( cc == 0 )
	{
		sp = s;
	}

	return sp;
}

/*
=============
Q_strtoi/l

Takes a null-terminated string (which represents either a float or integer
conforming to strtod) and an integer to assign to (if successful).

Returns true on success and vice versa.
Demonstration of behavior of strtod and conversions: http://codepad.org/YQKxV94R
-============
*/
qboolean Q_strtol( const char *s, long *outNum )
{
	char *p;

	if ( *s == '\0' )
	{
		return qfalse;
	}

	*outNum = strtod( s, &p );

	return *p == '\0';
}

qboolean Q_strtoi( const char *s, int *outNum )
{
	char *p;

	if ( *s == '\0' )
	{
		return qfalse;
	}

	*outNum = strtod( s, &p );

	return *p == '\0';
}

/*
=============
Q_strncpyz

Safe strncpy that ensures a trailing zero
=============
*/

#ifndef NDEBUG
void Q_strncpyzDebug( char *dest, const char *src, size_t destsize, const char *file, int line )
#else
void Q_strncpyz( char *dest, const char *src, int destsize )
#endif
{
#ifndef NDEBUG

	if ( !dest )
	{
		Com_Error( ERR_DROP, "Q_strncpyz: NULL dest (%s, %i)", file, line );
	}

	if ( !src )
	{
		Com_Error( ERR_DROP, "Q_strncpyz: NULL src (%s, %i)", file, line );
	}

	if ( destsize < 1 )
	{
		Com_Error( ERR_DROP, "Q_strncpyz: destsize < 1 (%s, %i)", file, line );
	}

#else

	if ( !dest )
	{
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL dest" );
	}

	if ( !src )
	{
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
	}

	if ( destsize < 1 )
	{
		Com_Error( ERR_FATAL, "Q_strncpyz: destsize < 1" );
	}

#endif

	strncpy( dest, src, destsize - 1 );
	dest[ destsize - 1 ] = 0;
}

int Q_stricmpn( const char *s1, const char *s2, int n )
{
	int c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- )
		{
			return 0; // strings are equal until end point
		}

		if ( c1 != c2 )
		{
			if ( c1 >= 'a' && c1 <= 'z' )
			{
				c1 -= ( 'a' - 'A' );
			}

			if ( c2 >= 'a' && c2 <= 'z' )
			{
				c2 -= ( 'a' - 'A' );
			}

			if ( c1 != c2 )
			{
				return c1 < c2 ? -1 : 1;
			}
		}
	}
	while ( c1 );

	return 0; // strings are equal
}

int Q_strncmp( const char *s1, const char *s2, int n )
{
	int c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- )
		{
			return 0; // strings are equal until end point
		}

		if ( c1 != c2 )
		{
			return c1 < c2 ? -1 : 1;
		}
	}
	while ( c1 );

	return 0; // strings are equal
}

int Q_stricmp( const char *s1, const char *s2 )
{
	return ( s1 && s2 ) ? Q_stricmpn( s1, s2, 99999 ) : -1;
}

char *Q_strlwr( char *s1 )
{
	char *s;

	for ( s = s1; *s; ++s )
	{
		if ( ( 'A' <= *s ) && ( *s <= 'Z' ) )
		{
			*s -= 'A' - 'a';
		}
	}

	return s1;
}

char *Q_strupr( char *s1 )
{
	char *cp;

	for ( cp = s1; *cp; ++cp )
	{
		if ( ( 'a' <= *cp ) && ( *cp <= 'z' ) )
		{
			*cp += 'A' - 'a';
		}
	}

	return s1;
}

// never goes past bounds or leaves without a terminating 0
void Q_strcat( char *dest, int size, const char *src )
{
	int l1;

	l1 = strlen( dest );

	if ( l1 >= size )
	{
		Com_Error( ERR_FATAL, "Q_strcat: already overflowed" );
	}

	Q_strncpyz( dest + l1, src, size - l1 );
}

int Q_strnicmp( const char *string1, const char *string2, int n )
{
	int c1, c2;

	if ( string1 == NULL )
	{
		if ( string2 == NULL )
		{
			return 0;
		}
		else
		{
			return -1;
		}
	}
	else if ( string2 == NULL )
	{
		return 1;
	}

	do
	{
		c1 = *string1++;
		c2 = *string2++;

		if ( !n-- )
		{
			return 0; // Strings are equal until end point
		}

		if ( c1 != c2 )
		{
			if ( c1 >= 'a' && c1 <= 'z' )
			{
				c1 -= ( 'a' - 'A' );
			}

			if ( c2 >= 'a' && c2 <= 'z' )
			{
				c2 -= ( 'a' - 'A' );
			}

			if ( c1 != c2 )
			{
				return c1 < c2 ? -1 : 1;
			}
		}
	}
	while ( c1 );

	return 0; // Strings are equal
}

void Q_strncpyz2( char *dst, const char *src, int dstSize )
{
	if ( !dst )
	{
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL dst" );
	}

	if ( !src )
	{
		Com_Error( ERR_FATAL, "Q_strncpyz: NULL src" );
	}

	if ( dstSize < 1 )
	{
		Com_Error( ERR_FATAL, "Q_strncpyz: dstSize < 1" );
	}

	strncpy( dst, src, dstSize - 1 );
	dst[ dstSize - 1 ] = 0;
}

int Q_strncasecmp( const char *s1, const char *s2, int n )
{
	int c1, c2;

	do
	{
		c1 = *s1++;
		c2 = *s2++;

		if ( !n-- )
		{
			return 0; // strings are equal until end point
		}

		if ( c1 != c2 )
		{
			if ( c1 >= 'a' && c1 <= 'z' )
			{
				c1 -= ( 'a' - 'A' );
			}

			if ( c2 >= 'a' && c2 <= 'z' )
			{
				c2 -= ( 'a' - 'A' );
			}

			if ( c1 != c2 )
			{
				return -1; // strings not equal
			}
		}
	}
	while ( c1 );

	return 0; // strings are equal
}

int Q_strcasecmp( const char *s1, const char *s2 )
{
	return Q_strncasecmp( s1, s2, 99999 );
}

/*
* Find the first occurrence of find in s.
*/
const char *Q_stristr( const char *s, const char *find )
{
	char   c, sc;
	size_t len;

	if ( ( c = *find++ ) != 0 )
	{
		if ( c >= 'a' && c <= 'z' )
		{
			c -= ( 'a' - 'A' );
		}

		len = strlen( find );

		do
		{
			do
			{
				if ( ( sc = *s++ ) == 0 )
				{
					return NULL;
				}

				if ( sc >= 'a' && sc <= 'z' )
				{
					sc -= ( 'a' - 'A' );
				}
			}
			while ( sc != c );
		}
		while ( Q_stricmpn( s, find, len ) != 0 );

		s--;
	}

	return s;
}

/*
=============
Q_strreplace

replaces content of find by replace in dest
=============
*/
qboolean Q_strreplace( char *dest, int destsize, const char *find, const char *replace )
{
	int  lstart, lfind, lreplace, lend;
	char *s;
	char backup[ 32000 ]; // big, but small enough to fit in PPC stack

	lend = strlen( dest );

	if ( lend >= destsize )
	{
		Com_Error( ERR_FATAL, "Q_strreplace: already overflowed" );
	}

	s = strstr( dest, find );

	if ( !s )
	{
		return qfalse;
	}
	else
	{
		Q_strncpyz( backup, dest, lend + 1 );
		lstart = s - dest;
		lfind = strlen( find );
		lreplace = strlen( replace );

		strncpy( s, replace, destsize - lstart - 1 );
		strncpy( s + lreplace, backup + lstart + lfind, destsize - lstart - lreplace - 1 );

		return qtrue;
	}
}

int Q_PrintStrlen( const char *string )
{
	int        len;
	const char *p;

	if ( !string )
	{
		return 0;
	}

	len = 0;
	p = string;

	while ( *p )
	{
		if ( Q_IsColorString( p ) )
		{
			p += 2;
			continue;
		}
		if ( *p == Q_COLOR_ESCAPE && p[1] == Q_COLOR_ESCAPE )
		{
			++p;
		}

		p++;
		len++;
	}

	return len;
}

char *Q_CleanStr( char *string )
{
	char *d;
	char *s;
	int c;

	s = string;
	d = string;

	while ( ( c = *s ) != 0 )
	{
		if ( Q_IsColorString( s ) )
		{
			s++;
		}
		else if ( c >= 0x20 && c != 0x7F )
		{
			*d++ = c;
		}

		s++;
	}

	*d = '\0';

	return string;
}

// strips whitespaces and bad characters
qboolean Q_isBadDirChar( char c )
{
	char badchars[] = { ';', '&', '(', ')', '|', '<', '>', '*', '?', '[', ']', '~', '+', '@', '!', '\\', '/', ' ', '\'', '\"', '\0' };
	int  i;

	for ( i = 0; badchars[ i ] != '\0'; i++ )
	{
		if ( c == badchars[ i ] )
		{
			return qtrue;
		}
	}

	return qfalse;
}

char *Q_CleanDirName( char *dirname )
{
	char *d;
	char *s;

	s = dirname;
	d = dirname;

	// clear trailing '.'s
	while ( *s == '.' )
	{
		s++;
	}

	while ( *s != '\0' )
	{
		if ( !Q_isBadDirChar( *s ) )
		{
			*d++ = *s;
		}

		s++;
	}

	*d = '\0';

	return dirname;
}

int Q_CountChar( const char *string, char tocount )
{
	int count;

	for ( count = 0; *string; string++ )
	{
		if ( *string == tocount )
		{
			count++;
		}
	}

	return count;
}

int QDECL PRINTF_LIKE(3) Com_sprintf( char *dest, int size, const char *fmt, ... )
{
	int     len;
	va_list argptr;

	va_start( argptr, fmt );
	len = Q_vsnprintf( dest, size, fmt, argptr );
	va_end( argptr );

	if ( len >= size )
	{
		Com_Printf( "Com_sprintf: Output length %d too short, %d bytes required.\n", size, len + 1 );
	}

	if ( len == -1 )
	{
		Com_Printf( "Com_sprintf: overflow of %i bytes buffer\n", size );
	}

	return len;
}

/*
============================================================================

 UTF-8

============================================================================
*/

// never returns more than 4
int Q_UTF8Width( const char *str )
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

int Q_UTF8WidthCP( int ch )
{
	if ( ch <=   0x007F ) { return 1; }
	if ( ch <=   0x07FF ) { return 2; }
	if ( ch <=   0xFFFF ) { return 3; }
	if ( ch <= 0x10FFFF ) { return 4; }
	return 0;
}

int Q_UTF8Strlen( const char *str )
{
  int l = 0;

  while( *str )
  {
    l++;

    str += Q_UTF8Width( str );
  }

  return l;
}

int Q_UTF8PrintStrlen( const char *str )
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

    str += Q_UTF8Width( str );
  }

  return l;
}

qboolean Q_UTF8ContByte( char c )
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

unsigned long Q_UTF8CodePoint( const char *str )
{
  int i, j;
  int n = 0;
  int size = Q_UTF8Width( str );
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
		Com_Error( ERR_DROP, "Q_UTF8CodePoint: overflow caught" );

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

char *Q_UTF8Encode( unsigned long codepoint )
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
int Q_UTF8Store( const char *s )
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

char *Q_UTF8Unstore( int e )
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

/*
============
va

does a varargs printf into a temp buffer, so I don't need to have
varargs versions of all text functions.
FIXME: make this buffer size safe someday

Ridah, modified this into a circular list, to further prevent stepping on
previous strings
============
*/
char     *QDECL PRINTF_LIKE(1) va( const char *format, ... )
{
	va_list     argptr;
#define MAX_VA_STRING 32000
	static char temp_buffer[ MAX_VA_STRING ];
	static char string[ MAX_VA_STRING ]; // in case va is called by nested functions
	static int  index = 0;
	char        *buf;
	int         len;

	va_start( argptr, format );
	Q_vsnprintf( temp_buffer, sizeof( temp_buffer ), format, argptr );
	va_end( argptr );

	if ( ( len = strlen( temp_buffer ) ) >= MAX_VA_STRING )
	{
		Com_Error( ERR_DROP, "Attempted to overrun string in call to va()" );
	}

	if ( len + index >= MAX_VA_STRING - 1 )
	{
		index = 0;
	}

	buf = &string[ index ];
	memcpy( buf, temp_buffer, len + 1 );

	index += len + 1;

	return buf;
}

/*
=====================================================================

  INFO STRINGS

=====================================================================
*/

/*
===============
Info_ValueForKey

Searches the string for the given
key and returns the associated value, or an empty string.
FIXME: overflow check?
===============
*/
char *Info_ValueForKey( const char *s, const char *key )
{
	char        pkey[ BIG_INFO_KEY ];
	static char value[ 2 ][ BIG_INFO_VALUE ]; // use two buffers so compares
	// work without stomping on each other
	static int  valueindex = 0;
	char        *o;

	if ( !s || !key )
	{
		return "";
	}

	if ( strlen( s ) >= BIG_INFO_STRING )
	{
		Com_Error( ERR_DROP, "Info_ValueForKey: oversize infostring [%s] [%s]", s, key );
	}

	valueindex ^= 1;

	if ( *s == '\\' )
	{
		s++;
	}

	while ( 1 )
	{
		o = pkey;

		while ( *s != '\\' )
		{
			if ( !*s )
			{
				return "";
			}

			*o++ = *s++;
		}

		*o = 0;
		s++;

		o = value[ valueindex ];

		while ( *s != '\\' && *s )
		{
			*o++ = *s++;
		}

		*o = 0;

		if ( !Q_stricmp( key, pkey ) )
		{
			return value[ valueindex ];
		}

		if ( !*s )
		{
			break;
		}

		s++;
	}

	return "";
}

/*
===================
Info_NextPair

Used to iterate through all the key/value pairs in an info string
===================
*/
void Info_NextPair( const char **head, char *key, char *value )
{
	char       *o;
	const char *s;

	s = *head;

	if ( *s == '\\' )
	{
		s++;
	}

	key[ 0 ] = 0;
	value[ 0 ] = 0;

	o = key;

	while ( *s != '\\' )
	{
		if ( !*s )
		{
			*o = 0;
			*head = s;
			return;
		}

		*o++ = *s++;
	}

	*o = 0;
	s++;

	o = value;

	while ( *s != '\\' && *s )
	{
		*o++ = *s++;
	}

	*o = 0;

	*head = s;
}

/*
===================
Info_RemoveKey
===================
*/
void Info_RemoveKey( char *s, const char *key, qboolean big )
{
	char *start;
	int maxlen = big ? BIG_INFO_STRING : MAX_INFO_STRING;
	int slen = strlen( s );
	static char pkey[ BIG_INFO_KEY ];
	static char value[ BIG_INFO_VALUE ];
	char *o;

	if ( slen >= maxlen )
	{
		Com_Error( ERR_DROP, "Info_RemoveKey: oversize infostring [%s] [%s]", s, key );
	}

	if ( strchr( key, '\\' ) )
	{
		return;
	}

	while ( 1 )
	{
		start = s;

		if ( *s == '\\' )
		{
			s++;
		}

		o = pkey;

		while ( *s != '\\' )
		{
			if ( !*s )
			{
				return;
			}

			*o++ = *s++;
		}

		*o = 0;
		s++;

		o = value;

		while ( *s != '\\' && *s )
		{
			if ( !*s )
			{
				return;
			}

			*o++ = *s++;
		}

		*o = 0;

		if ( !Q_stricmp( key, pkey ) )
		{
			memmove( start, s, strlen(s) + 1 );
			return;
		}

		if ( !*s )
		{
			return;
		}
	}
}

/*
==================
Info_Validate

Some characters are illegal in info strings because they
can mess up the server's parsing
==================
*/
qboolean Info_Validate( const char *s )
{
	if ( strchr( s, '\"' ) )
	{
		return qfalse;
	}

	if ( strchr( s, ';' ) )
	{
		return qfalse;
	}

	return qtrue;
}

/*
==================
Info_SetValueForKey

Changes or adds a key/value pair
==================
*/
void Info_SetValueForKey( char *s, const char *key, const char *value, qboolean big )
{
	int maxlen = big ? BIG_INFO_STRING : MAX_INFO_STRING;
	int slen = strlen( s );
	static char newi[ BIG_INFO_STRING ];

	if ( slen >= maxlen )
	{
		Com_Error( ERR_DROP, "Info_SetValueForKey: oversize infostring [%s] [%s] [%s]", s, key, value );
	}

	if ( strchr( key, '\\' ) || strchr( value, '\\' ) )
	{
		Com_Printf( "Can't use keys or values with a \\\n" );
		return;
	}

	if ( strchr( key, ';' ) || strchr( value, ';' ) )
	{
		Com_Printf( "Can't use keys or values with a semicolon\n" );
		return;
	}

	if ( strchr( key, '\"' ) || strchr( value, '\"' ) )
	{
		Com_Printf( "Can't use keys or values with a \"\n" );
		return;
	}

	Info_RemoveKey( s, key, big );

	if ( !value || !strlen( value ) )
	{
		return;
	}

	Com_sprintf( newi, maxlen, "\\%s\\%s", key, value );

	if ( strlen( newi ) + slen >= maxlen )
	{
		Com_Printf( "Info string length exceeded\n" );
		return;
	}

	strcat( s, newi );
}

/*
============
Com_ClientListContains
============
*/
qboolean Com_ClientListContains( const clientList_t *list, int clientNum )
{
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || !list )
	{
		return qfalse;
	}

	if ( clientNum < 32 )
	{
		return ( ( list->lo & ( 1 << clientNum ) ) != 0 );
	}
	else
	{
		return ( ( list->hi & ( 1 << ( clientNum - 32 ) ) ) != 0 );
	}
}

/*
============
Com_ClientListAdd
============
*/
void Com_ClientListAdd( clientList_t *list, int clientNum )
{
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || !list )
	{
		return;
	}

	if ( clientNum < 32 )
	{
		list->lo |= ( 1 << clientNum );
	}
	else
	{
		list->hi |= ( 1 << ( clientNum - 32 ) );
	}
}

/*
============
Com_ClientListRemove
============
*/
void Com_ClientListRemove( clientList_t *list, int clientNum )
{
	if ( clientNum < 0 || clientNum >= MAX_CLIENTS || !list )
	{
		return;
	}

	if ( clientNum < 32 )
	{
		list->lo &= ~( 1 << clientNum );
	}
	else
	{
		list->hi &= ~( 1 << ( clientNum - 32 ) );
	}
}

/*
============
Com_ClientListString
============
*/
char *Com_ClientListString( const clientList_t *list )
{
	static char s[ 17 ];

	s[ 0 ] = '\0';

	if ( !list )
	{
		return s;
	}

	Com_sprintf( s, sizeof( s ), "%08x%08x", list->hi, list->lo );
	return s;
}

/*
============
Com_ClientListParse
============
*/
void Com_ClientListParse( clientList_t *list, const char *s )
{
	if ( !list )
	{
		return;
	}

	list->lo = 0;
	list->hi = 0;

	if ( !s )
	{
		return;
	}

	if ( strlen( s ) != 16 )
	{
		return;
	}

	sscanf( s, "%8x%8x", &list->hi, &list->lo );
}

/*
================
VectorMatrixMultiply
================
*/
void VectorMatrixMultiply( const vec3_t p, vec3_t m[ 3 ], vec3_t out )
{
	out[ 0 ] = m[ 0 ][ 0 ] * p[ 0 ] + m[ 1 ][ 0 ] * p[ 1 ] + m[ 2 ][ 0 ] * p[ 2 ];
	out[ 1 ] = m[ 0 ][ 1 ] * p[ 0 ] + m[ 1 ][ 1 ] * p[ 1 ] + m[ 2 ][ 1 ] * p[ 2 ];
	out[ 2 ] = m[ 0 ][ 2 ] * p[ 0 ] + m[ 1 ][ 2 ] * p[ 1 ] + m[ 2 ][ 2 ] * p[ 2 ];
}

void Q_ParseNewlines( char *dest, const char *src, int destsize )
{
	for ( ; *src && destsize > 1; src++, destsize-- )
	{
		*dest++ = ( ( *src == '\\' && * ( ++src ) == 'n' ) ? '\n' : *src );
	}

	*dest++ = '\0';
}

//====================================================================
