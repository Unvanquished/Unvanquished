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

#include "qcommon/q_shared.h"
#include "qcommon.h"

static huffman_t msgHuff;
static bool  msgInit = false;

/*
==============================================================================

                        MESSAGE IO FUNCTIONS

Handles byte ordering and avoids alignment errors
==============================================================================
*/

void MSG_initHuffman();

void MSG_Init( msg_t *buf, byte *data, int length )
{
	if ( !msgInit )
	{
		MSG_initHuffman();
	}

	Com_Memset( buf, 0, sizeof( *buf ) );
	buf->data = data;
	buf->maxsize = length;
}

void MSG_InitOOB( msg_t *buf, byte *data, int length )
{
	if ( !msgInit )
	{
		MSG_initHuffman();
	}

	Com_Memset( buf, 0, sizeof( *buf ) );
	buf->data = data;
	buf->maxsize = length;
	buf->oob = true;
}

void MSG_Clear( msg_t *buf )
{
	buf->cursize = 0;
	buf->overflowed = false;
	buf->bit = 0; //<- in bits
}

void MSG_Bitstream( msg_t *buf )
{
	buf->oob = false;
}

void MSG_Uncompressed( msg_t *buf )
{
	// align to byte-boundary
	buf->bit = ( buf->bit + 7 ) & ~7;
	buf->oob = true;
}

void MSG_BeginReading( msg_t *msg )
{
	msg->readcount = 0;
	msg->bit = 0;
	msg->oob = false;
}

void MSG_BeginReadingOOB( msg_t *msg )
{
	msg->readcount = 0;
	msg->bit = 0;
	msg->oob = true;
}

void MSG_BeginReadingUncompressed( msg_t *buf )
{
	// align to byte-boundary
	buf->bit = ( buf->bit + 7 ) & ~7;
	buf->oob = true;
}

void MSG_Copy( msg_t *buf, byte *data, int length, msg_t *src )
{
	if ( length < src->cursize )
	{
		Com_Error( ERR_DROP, "MSG_Copy: can't copy %d into a smaller %d msg_t buffer", src->cursize, length );
	}

	Com_Memcpy( buf, src, sizeof( msg_t ) );
	buf->data = data;
	Com_Memcpy( buf->data, src->data, src->cursize );
}

/*
=============================================================================

bit functions

=============================================================================
*/

// negative bit values include signs
void MSG_WriteBits( msg_t *msg, int value, int bits )
{
	int i;

	msg->uncompsize += bits; // NERVE - SMF - net debugging

	// this isn't an exact overflow check, but close enough
	if ( msg->maxsize - msg->cursize < 32 )
	{
		msg->overflowed = true;
		return;
	}

	if ( bits == 0 || bits < -31 || bits > 32 )
	{
		Com_Error( ERR_DROP, "MSG_WriteBits: bad bits %i", bits );
	}

	if ( bits < 0 )
	{
		bits = -bits;
	}

	if ( msg->oob )
	{
		if ( bits == 8 )
		{
			msg->data[ msg->cursize ] = value;
			msg->cursize += 1;
			msg->bit += 8;
		}
		else if ( bits == 16 )
		{
			unsigned short *sp = ( unsigned short * ) &msg->data[ msg->cursize ];

			*sp = LittleShort( value );
			msg->cursize += 2;
			msg->bit += 16;
		}
		else if ( bits == 32 )
		{
			unsigned int *ip = ( unsigned int * ) &msg->data[ msg->cursize ];

			*ip = LittleLong( value );
			msg->cursize += 4;
			msg->bit += 8;
		}
		else
		{
			Com_Error( ERR_DROP, "can't read %d bits", bits );
		}
	}
	else
	{
		value &= ( 0xffffffff >> ( 32 - bits ) );

		if ( bits & 7 )
		{
			int nbits;

			nbits = bits & 7;

			for ( i = 0; i < nbits; i++ )
			{
				Huff_putBit( ( value & 1 ), msg->data, &msg->bit );
				value = ( value >> 1 );
			}

			bits = bits - nbits;
		}

		if ( bits )
		{
			for ( i = 0; i < bits; i += 8 )
			{
				Huff_offsetTransmit( &msgHuff.compressor, ( value & 0xff ), msg->data, &msg->bit );
				value = ( value >> 8 );
			}
		}

		msg->cursize = ( msg->bit >> 3 ) + 1;
	}
}

int MSG_ReadBits( msg_t *msg, int bits )
{
	int      value;
	int      get;
	bool sgn;
	int      i, nbits;

	value = 0;

	if ( bits < 0 )
	{
		bits = -bits;
		sgn = true;
	}
	else
	{
		sgn = false;
	}

	if ( msg->oob )
	{
		if ( bits == 8 )
		{
			value = msg->data[ msg->readcount ];
			msg->readcount += 1;
			msg->bit += 8;
		}
		else if ( bits == 16 )
		{
			unsigned short *sp = ( unsigned short * ) &msg->data[ msg->readcount ];

			value = LittleShort( *sp );
			msg->readcount += 2;
			msg->bit += 16;
		}
		else if ( bits == 32 )
		{
			unsigned int *ip = ( unsigned int * ) &msg->data[ msg->readcount ];

			value = LittleLong( *ip );
			msg->readcount += 4;
			msg->bit += 32;
		}
		else
		{
			Com_Error( ERR_DROP, "can't read %d bits", bits );
		}
	}
	else
	{
		nbits = 0;

		if ( bits & 7 )
		{
			nbits = bits & 7;

			for ( i = 0; i < nbits; i++ )
			{
				value |= ( Huff_getBit( msg->data, &msg->bit ) << i );
			}

			bits = bits - nbits;
		}

		if ( bits )
		{
			for ( i = 0; i < bits; i += 8 )
			{
				Huff_offsetReceive( msgHuff.decompressor.tree, &get, msg->data, &msg->bit );
				value |= ( get << ( i + nbits ) );
			}
		}

		msg->readcount = ( msg->bit >> 3 ) + 1;
	}

	if ( sgn )
	{
		if ( value & ( 1 << ( bits - 1 ) ) )
		{
			value |= -1 ^ ( ( 1 << bits ) - 1 );
		}
	}

	return value;
}

//================================================================================

//
// writing functions
//

void MSG_WriteChar( msg_t *sb, int c )
{
#ifdef PARANOID

	if ( c < -128 || c > 127 )
	{
		Com_Error( ERR_FATAL, "MSG_WriteChar: range error" );
	}

#endif

	MSG_WriteBits( sb, c, 8 );
}

void MSG_WriteByte( msg_t *sb, int c )
{
#ifdef PARANOID

	if ( c < 0 || c > 255 )
	{
		Com_Error( ERR_FATAL, "MSG_WriteByte: range error" );
	}

#endif

	MSG_WriteBits( sb, c, 8 );
}

void MSG_WriteData( msg_t *buf, const void *data, int length )
{
	int i;

	for ( i = 0; i < length; i++ )
	{
		MSG_WriteByte( buf, ( ( byte * ) data ) [ i ] );
	}
}

void MSG_WriteShort( msg_t *sb, int c )
{
#ifdef PARANOID

	if ( c < ( ( short ) 0x8000 ) || c > ( short ) 0x7fff )
	{
		Com_Error( ERR_FATAL, "MSG_WriteShort: range error" );
	}

#endif

	MSG_WriteBits( sb, c, 16 );
}

void MSG_WriteLong( msg_t *sb, int c )
{
	MSG_WriteBits( sb, c, 32 );
}

void MSG_WriteFloat( msg_t *sb, float f )
{
	union
	{
		float f;
		int   l;
	} dat;

	dat.f = f;
	MSG_WriteBits( sb, dat.l, 32 );
}

void MSG_WriteString( msg_t *sb, const char *s )
{
	if ( !s )
	{
		MSG_WriteData( sb, "", 1 );
	}
	else
	{
		int  l;
		char string[ MAX_STRING_CHARS ];

		l = strlen( s );

		if ( l >= MAX_STRING_CHARS )
		{
			Com_Printf( "MSG_WriteString: MAX_STRING_CHARS exceeded\n" );
			MSG_WriteData( sb, "", 1 );
			return;
		}

		Q_strncpyz( string, s, sizeof( string ) );

		MSG_WriteData( sb, string, l + 1 );
	}
}

void MSG_WriteBigString( msg_t *sb, const char *s )
{
	if ( !s )
	{
		MSG_WriteData( sb, "", 1 );
	}
	else
	{
		int  l;
		char string[ BIG_INFO_STRING ];

		l = strlen( s );

		if ( l >= BIG_INFO_STRING )
		{
			Com_Printf( "MSG_WriteBigString: BIG_INFO_STRING exceeded\n" );
			MSG_WriteData( sb, "", 1 );
			return;
		}

		Q_strncpyz( string, s, sizeof( string ) );

		MSG_WriteData( sb, string, l + 1 );
	}
}

void MSG_WriteAngle( msg_t *sb, float f )
{
	MSG_WriteByte( sb, ( int )( f * 256 / 360 ) & 255 );
}

void MSG_WriteAngle16( msg_t *sb, float f )
{
	MSG_WriteShort( sb, ANGLE2SHORT( f ) );
}

//============================================================

//
// reading functions
//

// returns -1 if no more characters are available
int MSG_ReadChar( msg_t *msg )
{
	int c;

	c = ( signed char ) MSG_ReadBits( msg, 8 );

	if ( msg->readcount > msg->cursize )
	{
		c = -1;
	}

	return c;
}

int MSG_ReadByte( msg_t *msg )
{
	int c;

	c = ( unsigned char ) MSG_ReadBits( msg, 8 );

	if ( msg->readcount > msg->cursize )
	{
		c = -1;
	}

	return c;
}

int MSG_ReadShort( msg_t *msg )
{
	int c;

	c = ( short ) MSG_ReadBits( msg, 16 );

	if ( msg->readcount > msg->cursize )
	{
		c = -1;
	}

	return c;
}

int MSG_ReadLong( msg_t *msg )
{
	int c;

	c = MSG_ReadBits( msg, 32 );

	if ( msg->readcount > msg->cursize )
	{
		c = -1;
	}

	return c;
}

float MSG_ReadFloat( msg_t *msg )
{
	union
	{
		byte  b[ 4 ];
		float f;
		int   l;
	} dat;

	dat.l = MSG_ReadBits( msg, 32 );

	if ( msg->readcount > msg->cursize )
	{
		dat.f = -1;
	}

	return dat.f;
}

char           *MSG_ReadString( msg_t *msg )
{
	static char string[ MAX_STRING_CHARS ];
	unsigned l;
    int c;

	l = 0;

	do
	{
		c = MSG_ReadByte( msg );  // use ReadByte so -1 is out of bounds

		if ( c == -1 || c == 0 )
		{
			break;
		}

		string[ l ] = c;
		l++;
	}
	while ( l < sizeof( string ) - 1 );

	string[ l ] = 0;

	return string;
}

char           *MSG_ReadBigString( msg_t *msg )
{
	static char string[ BIG_INFO_STRING ];
	unsigned l;
    int c;

	l = 0;

	do
	{
		c = MSG_ReadByte( msg );  // use ReadByte so -1 is out of bounds

		if ( c == -1 || c == 0 )
		{
			break;
		}

		string[ l ] = c;
		l++;
	}
	while ( l < sizeof( string ) - 1 );

	string[ l ] = 0;

	return string;
}

char           *MSG_ReadStringLine( msg_t *msg )
{
	static char string[ MAX_STRING_CHARS ];
	unsigned l;
    int c;

	l = 0;

	do
	{
		c = MSG_ReadByte( msg );  // use ReadByte so -1 is out of bounds

		if ( c == -1 || c == 0 || c == '\n' )
		{
			break;
		}

		string[ l ] = c;
		l++;
	}
	while ( l < sizeof( string ) - 1 );

	string[ l ] = 0;

	return string;
}

float MSG_ReadAngle16( msg_t *msg )
{
	return SHORT2ANGLE( MSG_ReadShort( msg ) );
}

void MSG_ReadData( msg_t *msg, void *data, int len )
{
	int i;

	for ( i = 0; i < len; i++ )
	{
		( ( byte * ) data ) [ i ] = MSG_ReadByte( msg );
	}
}

// a string hasher which gives the same hash value even if the
// string is later modified via the legacy MSG read/write code
int MSG_HashKey( const char *string, int maxlen )
{
	int hash, i;

	hash = 0;

	for ( i = 0; i < maxlen && string[ i ] != '\0'; i++ )
	{
		if ( string[ i ] & 0x80 )
		{
			hash += '.' * ( 119 + i );
		}
		else
		{
			hash += string[ i ] * ( 119 + i );
		}
	}

	hash = ( hash ^ ( hash >> 10 ) ^ ( hash >> 20 ) );
	return hash;
}

/*
=============================================================================

delta functions

=============================================================================
*/

extern cvar_t *cl_shownet;

#define LOG( x ) if ( cl_shownet && cl_shownet->integer == 4 ) { Com_Printf( "%s ", x ); };

void MSG_WriteDelta( msg_t *msg, int oldV, int newV, int bits )
{
	if ( oldV == newV )
	{
		MSG_WriteBits( msg, 0, 1 );
		return;
	}

	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteBits( msg, newV, bits );
}

int MSG_ReadDelta( msg_t *msg, int oldV, int bits )
{
	if ( MSG_ReadBits( msg, 1 ) )
	{
		return MSG_ReadBits( msg, bits );
	}

	return oldV;
}

void MSG_WriteDeltaFloat( msg_t *msg, float oldV, float newV )
{
	if ( oldV == newV )
	{
		MSG_WriteBits( msg, 0, 1 );
		return;
	}

	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteBits( msg, * ( int * ) &newV, 32 );
}

float MSG_ReadDeltaFloat( msg_t *msg, float oldV )
{
	if ( MSG_ReadBits( msg, 1 ) )
	{
		float newV;

		* ( int * ) &newV = MSG_ReadBits( msg, 32 );
		return newV;
	}

	return oldV;
}

/*
=============================================================================

delta functions with keys

=============================================================================
*/

static const unsigned int kbitmask[ 32 ] =
{
	0x00000001, 0x00000003, 0x00000007, 0x0000000F,
	0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
	0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
	0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
	0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
	0x001FFFFf, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
	0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
	0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF,
};

void MSG_WriteDeltaKey( msg_t *msg, int key, int oldV, int newV, int bits )
{
	if ( oldV == newV )
	{
		MSG_WriteBits( msg, 0, 1 );
		return;
	}

	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteBits( msg, newV ^ key, bits );
}

int MSG_ReadDeltaKey( msg_t *msg, int key, int oldV, int bits )
{
	if ( MSG_ReadBits( msg, 1 ) )
	{
		return MSG_ReadBits( msg, bits ) ^ ( key & kbitmask[ bits - 1 ] );
	}

	return oldV;
}

void MSG_WriteDeltaKeyFloat( msg_t *msg, int key, float oldV, float newV )
{
	if ( oldV == newV )
	{
		MSG_WriteBits( msg, 0, 1 );
		return;
	}

	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteBits( msg, ( * ( int * ) &newV ) ^ key, 32 );
}

float MSG_ReadDeltaKeyFloat( msg_t *msg, int key, float oldV )
{
	if ( MSG_ReadBits( msg, 1 ) )
	{
		float newV;

		* ( int * ) &newV = MSG_ReadBits( msg, 32 ) ^ key;
		return newV;
	}

	return oldV;
}

/*
============================================================================

usercmd_t communication

============================================================================
*/

/*
=====================
MSG_WriteDeltaUsercmd
=====================
*/
void MSG_WriteDeltaUsercmd( msg_t *msg, usercmd_t *from, usercmd_t *to )
{
	int i;

	if ( to->serverTime - from->serverTime < 256 )
	{
		MSG_WriteBits( msg, 1, 1 );
		MSG_WriteBits( msg, to->serverTime - from->serverTime, 8 );
	}
	else
	{
		MSG_WriteBits( msg, 0, 1 );
		MSG_WriteBits( msg, to->serverTime, 32 );
	}

	if ( from->angles[ 0 ] == to->angles[ 0 ] &&
	     from->angles[ 1 ] == to->angles[ 1 ] &&
	     from->angles[ 2 ] == to->angles[ 2 ] &&
	     from->forwardmove == to->forwardmove &&
	     from->rightmove == to->rightmove &&
	     from->upmove == to->upmove &&
	     !memcmp( from->buttons, to->buttons, sizeof( from->buttons ) ) &&
	     from->weapon == to->weapon &&
	     from->flags == to->flags && from->doubleTap == to->doubleTap)
	{
		// NERVE - SMF
		MSG_WriteBits( msg, 0, 1 );  // no change
		return;
	}

	MSG_WriteBits( msg, 1, 1 );
	MSG_WriteDelta( msg, from->angles[ 0 ], to->angles[ 0 ], 16 );
	MSG_WriteDelta( msg, from->angles[ 1 ], to->angles[ 1 ], 16 );
	MSG_WriteDelta( msg, from->angles[ 2 ], to->angles[ 2 ], 16 );
	MSG_WriteDelta( msg, from->forwardmove, to->forwardmove, 8 );
	MSG_WriteDelta( msg, from->rightmove, to->rightmove, 8 );
	MSG_WriteDelta( msg, from->upmove, to->upmove, 8 );
	for ( i = 0; i < USERCMD_BUTTONS / 8; ++i )
	{
		MSG_WriteDelta( msg, from->buttons[i], to->buttons[i], 8 );
	}
	MSG_WriteDelta( msg, from->weapon, to->weapon, 8 );
	MSG_WriteDelta( msg, from->flags, to->flags, 8 );
	MSG_WriteDelta( msg, from->doubleTap, to->doubleTap, 3 );
}

/*
=====================
MSG_ReadDeltaUsercmd
=====================
*/
void MSG_ReadDeltaUsercmd( msg_t *msg, usercmd_t *from, usercmd_t *to )
{
	int i;

	if ( MSG_ReadBits( msg, 1 ) )
	{
		to->serverTime = from->serverTime + MSG_ReadBits( msg, 8 );
	}
	else
	{
		to->serverTime = MSG_ReadBits( msg, 32 );
	}

	if ( MSG_ReadBits( msg, 1 ) )
	{
		to->angles[ 0 ] = MSG_ReadDelta( msg, from->angles[ 0 ], 16 );
		to->angles[ 1 ] = MSG_ReadDelta( msg, from->angles[ 1 ], 16 );
		to->angles[ 2 ] = MSG_ReadDelta( msg, from->angles[ 2 ], 16 );
		to->forwardmove = MSG_ReadDelta( msg, from->forwardmove, 8 );
		to->rightmove = MSG_ReadDelta( msg, from->rightmove, 8 );
		to->upmove = MSG_ReadDelta( msg, from->upmove, 8 );
		if ( to->forwardmove == -128 )
			to->forwardmove = -127;
		if ( to->rightmove == -128 )
			to->rightmove = -127;
		if ( to->upmove == -128 )
			to->upmove = -127;
		for ( i = 0; i < USERCMD_BUTTONS / 8; ++i )
		{
			to->buttons[i] = MSG_ReadDelta( msg, from->buttons[i], 8 );
		}
		to->weapon = MSG_ReadDelta( msg, from->weapon, 8 );
		to->flags = MSG_ReadDelta( msg, from->flags, 8 );
		to->doubleTap = MSG_ReadDelta( msg, from->doubleTap, 3 ) & 0x7;
	}
	else
	{
		to->angles[ 0 ] = from->angles[ 0 ];
		to->angles[ 1 ] = from->angles[ 1 ];
		to->angles[ 2 ] = from->angles[ 2 ];
		to->forwardmove = from->forwardmove;
		to->rightmove = from->rightmove;
		to->upmove = from->upmove;
		usercmdCopyButtons( to->buttons, from->buttons );
		to->weapon = from->weapon;
		to->flags = from->flags;
		to->doubleTap = from->doubleTap;
	}
}

/*
=============================================================================

entityState_t communication

=============================================================================
*/

struct netField_t
{
	const char *name;
	int  offset;
	int  bits;
	int  used;
};

#define NETF( x ) # x,int((size_t)&( (entityState_t*)0 )->x)

static netField_t entityStateFields[] =
{
	{ NETF( eType ),             8              , 0 },
	{ NETF( eFlags ),            24             , 0 },
	{ NETF( pos.trType ),        8              , 0 },
	{ NETF( pos.trTime ),        32             , 0 },
	{ NETF( pos.trDuration ),    32             , 0 },
	{ NETF( pos.trBase[ 0 ] ),   0              , 0 },
	{ NETF( pos.trBase[ 1 ] ),   0              , 0 },
	{ NETF( pos.trBase[ 2 ] ),   0              , 0 },
	{ NETF( pos.trDelta[ 0 ] ),  0              , 0 },
	{ NETF( pos.trDelta[ 1 ] ),  0              , 0 },
	{ NETF( pos.trDelta[ 2 ] ),  0              , 0 },
	{ NETF( apos.trType ),       8              , 0 },
	{ NETF( apos.trTime ),       32             , 0 },
	{ NETF( apos.trDuration ),   32             , 0 },
	{ NETF( apos.trBase[ 0 ] ),  0              , 0 },
	{ NETF( apos.trBase[ 1 ] ),  0              , 0 },
	{ NETF( apos.trBase[ 2 ] ),  0              , 0 },
	{ NETF( apos.trDelta[ 0 ] ), 0              , 0 },
	{ NETF( apos.trDelta[ 1 ] ), 0              , 0 },
	{ NETF( apos.trDelta[ 2 ] ), 0              , 0 },
	{ NETF( time ),              32             , 0 },
	{ NETF( time2 ),             32             , 0 },
	{ NETF( origin[ 0 ] ),       0              , 0 },
	{ NETF( origin[ 1 ] ),       0              , 0 },
	{ NETF( origin[ 2 ] ),       0              , 0 },
	{ NETF( origin2[ 0 ] ),      0              , 0 },
	{ NETF( origin2[ 1 ] ),      0              , 0 },
	{ NETF( origin2[ 2 ] ),      0              , 0 },
	{ NETF( angles[ 0 ] ),       0              , 0 },
	{ NETF( angles[ 1 ] ),       0              , 0 },
	{ NETF( angles[ 2 ] ),       0              , 0 },
	{ NETF( angles2[ 0 ] ),      0              , 0 },
	{ NETF( angles2[ 1 ] ),      0              , 0 },
	{ NETF( angles2[ 2 ] ),      0              , 0 },
	{ NETF( otherEntityNum ),    GENTITYNUM_BITS, 0 },
	{ NETF( otherEntityNum2 ),   GENTITYNUM_BITS, 0 },
	{ NETF( groundEntityNum ),   GENTITYNUM_BITS, 0 },
	{ NETF( loopSound ),         8              , 0 },
	{ NETF( constantLight ),     32             , 0 },
	{ NETF( modelindex ),        MODELINDEX_BITS, 0 },
	{ NETF( modelindex2 ),       MODELINDEX_BITS, 0 },
	{ NETF( frame ),             16             , 0 },
	{ NETF( clientNum ),         8              , 0 },
	{ NETF( solid ),             24             , 0 },
	{ NETF( event ),             10             , 0 },
	{ NETF( eventParm ),         8              , 0 },
	{ NETF( eventSequence ),     8              , 0 },  // warning: need to modify cg_event.c at "// check the sequencial list" if you change this
	{ NETF( events[ 0 ] ),       8              , 0 },
	{ NETF( events[ 1 ] ),       8              , 0 },
	{ NETF( events[ 2 ] ),       8              , 0 },
	{ NETF( events[ 3 ] ),       8              , 0 },
	{ NETF( eventParms[ 0 ] ),   8              , 0 },
	{ NETF( eventParms[ 1 ] ),   8              , 0 },
	{ NETF( eventParms[ 2 ] ),   8              , 0 },
	{ NETF( eventParms[ 3 ] ),   8              , 0 },
	{ NETF( weapon ),            8              , 0 },
	{ NETF( legsAnim ),          ANIM_BITS      , 0 },
	{ NETF( torsoAnim ),         ANIM_BITS      , 0 },
	{ NETF( generic1 ),          10             , 0 },
	{ NETF( misc ),              MAX_MISC       , 0 },
	{ NETF( weaponAnim ),        ANIM_BITS      , 0 },
};

static int QDECL qsort_entitystatefields( const void *a, const void *b )
{
	int aa, bb;

	aa = * ( ( int * ) a );
	bb = * ( ( int * ) b );

	if ( entityStateFields[ aa ].used > entityStateFields[ bb ].used )
	{
		return -1;
	}

	if ( entityStateFields[ bb ].used > entityStateFields[ aa ].used )
	{
		return 1;
	}

	return 0;
}

void MSG_PrioritiseEntitystateFields()
{
	int fieldorders[ ARRAY_LEN( entityStateFields ) ];
	int numfields = ARRAY_LEN( entityStateFields );
	int i;

	for ( i = 0; i < numfields; i++ )
	{
		fieldorders[ i ] = i;
	}

	qsort( fieldorders, numfields, sizeof( int ), qsort_entitystatefields );

	Com_Printf( "Entitystate fields in order of priority\n" );
	Com_Printf( "netField_t entityStateFields[] = {\n" );

	for ( i = 0; i < numfields; i++ )
	{
		Com_Printf( "{ NETF (%s), %i },\n", entityStateFields[ fieldorders[ i ] ].name, entityStateFields[ fieldorders[ i ] ].bits );
	}

	Com_Printf( "};\n" );
}

// if (int)f == f and (int)f + ( 1<<(FLOAT_INT_BITS-1) ) < ( 1 << FLOAT_INT_BITS )
// the float will be sent with FLOAT_INT_BITS, otherwise all 32 bits will be sent
#define FLOAT_INT_BITS 13
#define FLOAT_INT_BIAS ( 1 << ( FLOAT_INT_BITS - 1 ) )

/*
==================
MSG_WriteDeltaEntity

Writes part of a packetentities message, including the entity number.
Can delta from either a baseline or a previous packet_entity
If to is nullptr, a remove entity update will be sent
If force is not set, then nothing at all will be generated if the entity is
identical, under the assumption that the in-order delta code will catch it.
==================
*/
void MSG_WriteDeltaEntity( msg_t *msg, entityState_t *from, entityState_t *to, bool force )
{
	int        i, lc;
	netField_t *field;
	int        trunc;
	float      fullFloat;
	int        *fromF, *toF;

	const int numFields = ARRAY_LEN(entityStateFields);

	// all fields should be 32 bits to avoid any compiler packing issues
	// the "number" field is not part of the field list
	// if this assert fails, someone added a field to the entityState_t
	// struct without updating the message fields
	static_assert(numFields + 1 == sizeof(*from) / 4, "entityState_t out of sync with entityStateFields");

	// a nullptr to is a delta remove message
	if ( to == nullptr )
	{
		if ( from == nullptr )
		{
			return;
		}

		if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) )
		{
			Com_Printf( "W|%3i: #%-3i remove\n", msg->cursize, from->number );
		}

		MSG_WriteBits( msg, from->number, GENTITYNUM_BITS );
		MSG_WriteBits( msg, 1, 1 );
		return;
	}

	if ( to->number < 0 || to->number >= MAX_GENTITIES )
	{
		Com_Error( ERR_FATAL, "MSG_WriteDeltaEntity: Bad entity number: %i", to->number );
	}

	lc = 0;

	// build the change vector as bytes so it is endien independent
	for ( i = 0, field = entityStateFields; i < numFields; i++, field++ )
	{
		fromF = ( int * )( ( byte * ) from + field->offset );
		toF = ( int * )( ( byte * ) to + field->offset );

		if ( *fromF != *toF )
		{
			lc = i + 1;

			field->used++;
		}
	}

	if ( lc == 0 )
	{
		// nothing at all changed
		if ( !force )
		{
			return; // nothing at all
		}

		// write two bits for no change
		MSG_WriteBits( msg, to->number, GENTITYNUM_BITS );
		MSG_WriteBits( msg, 0, 1 );  // not removed
		MSG_WriteBits( msg, 0, 1 );  // no delta
		return;
	}

	MSG_WriteBits( msg, to->number, GENTITYNUM_BITS );
	MSG_WriteBits( msg, 0, 1 );  // not removed
	MSG_WriteBits( msg, 1, 1 );  // we have a delta

	MSG_WriteByte( msg, lc );  // # of changes

//  Com_Printf( "Delta for ent %i: ", to->number );

	for ( i = 0, field = entityStateFields; i < lc; i++, field++ )
	{
		fromF = ( int * )( ( byte * ) from + field->offset );
		toF = ( int * )( ( byte * ) to + field->offset );

		if ( *fromF == *toF )
		{
			MSG_WriteBits( msg, 0, 1 );  // no change
			continue;
		}

		MSG_WriteBits( msg, 1, 1 );  // changed

		if ( field->bits == 0 )
		{
			// float
			fullFloat = * ( float * ) toF;
			trunc = ( int ) fullFloat;

			if ( fullFloat == 0.0f )
			{
				MSG_WriteBits( msg, 0, 1 );
			}
			else
			{
				MSG_WriteBits( msg, 1, 1 );

				if ( trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 && trunc + FLOAT_INT_BIAS < ( 1 << FLOAT_INT_BITS ) )
				{
					// send as small integer
					MSG_WriteBits( msg, 0, 1 );
					MSG_WriteBits( msg, trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS );
//                  if ( print ) {
//                      Com_Printf( "%s:%i ", field->name, trunc );
//                  }
				}
				else
				{
					// send as full floating point value
					MSG_WriteBits( msg, 1, 1 );
					MSG_WriteBits( msg, *toF, 32 );
//                  if ( print ) {
//                      Com_Printf( "%s:%f ", field->name, *(float *)toF );
//                  }
				}
			}
		}
		else
		{
			if ( *toF == 0 )
			{
				MSG_WriteBits( msg, 0, 1 );
			}
			else
			{
				MSG_WriteBits( msg, 1, 1 );
				// integer
				MSG_WriteBits( msg, *toF, field->bits );
//              if ( print ) {
//                  Com_Printf( "%s:%i ", field->name, *toF );
//              }
			}
		}
	}

//  Com_Printf( "\n" );

	/*
	        c = msg->cursize - c;

	        if ( print ) {
	                if ( msg->bit == 0 ) {
	                        endBit = msg->cursize * 8 - GENTITYNUM_BITS;
	                } else {
	                        endBit = ( msg->cursize - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
	                }
	                Com_Printf( " (%i bits)\n", endBit - startBit  );
	        }
	*/
}

/*
==================
MSG_ReadDeltaEntity

The entity number has already been read from the message, which
is how the from state is identified.

If the delta removes the entity, entityState_t->number will be set to MAX_GENTITIES-1

Can go from either a baseline or a previous packet_entity
==================
*/
extern cvar_t *cl_shownet;

void MSG_ReadDeltaEntity( msg_t *msg, const entityState_t *from, entityState_t *to, int number )
{
	int        i, lc;
	int        numFields;
	netField_t *field;
	int        *fromF, *toF;
	int        print;
	int        trunc;
	int        startBit, endBit;

	if ( number < 0 || number >= MAX_GENTITIES )
	{
		Com_Error( ERR_DROP, "Bad delta entity number: %i", number );
	}

	if ( msg->bit == 0 )
	{
		startBit = msg->readcount * 8 - GENTITYNUM_BITS;
	}
	else
	{
		startBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
	}

	// check for a remove
	if ( MSG_ReadBits( msg, 1 ) == 1 )
	{
		memset( to, 0, sizeof( *to ) );
		to->number = MAX_GENTITIES - 1;

		if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) )
		{
			Com_Printf( "%3i: #%-3i remove\n", msg->readcount, number );
		}

		return;
	}

	// check for no delta
	if ( MSG_ReadBits( msg, 1 ) == 0 )
	{
		*to = *from;
		to->number = number;
		return;
	}

	numFields = ARRAY_LEN( entityStateFields );
	lc = MSG_ReadByte( msg );

	if ( lc > numFields || lc < 0 )
	{
		Com_Error( ERR_DROP, "invalid entityState field count" );
	}

	// shownet 2/3 will interleave with other printed info, -1 will
	// just print the delta records`
	if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -1 ) )
	{
		print = 1;
		Com_Printf( "%3i: #%-3i ", msg->readcount, to->number );
	}
	else
	{
		print = 0;
	}

	to->number = number;

	for ( i = 0, field = entityStateFields; i < lc; i++, field++ )
	{
		fromF = ( int * )( ( byte * ) from + field->offset );
		toF = ( int * )( ( byte * ) to + field->offset );

		if ( !MSG_ReadBits( msg, 1 ) )
		{
			// no change
			*toF = *fromF;
		}
		else
		{
			if ( field->bits == 0 )
			{
				// float
				if ( MSG_ReadBits( msg, 1 ) == 0 )
				{
					* ( float * ) toF = 0.0f;
				}
				else
				{
					if ( MSG_ReadBits( msg, 1 ) == 0 )
					{
						// integral float
						trunc = MSG_ReadBits( msg, FLOAT_INT_BITS );
						// bias to allow equal parts positive and negative
						trunc -= FLOAT_INT_BIAS;
						* ( float * ) toF = trunc;

						if ( print )
						{
							Com_Printf( "%s:%i ", field->name, trunc );
						}
					}
					else
					{
						// full floating point value
						*toF = MSG_ReadBits( msg, 32 );

						if ( print )
						{
							Com_Printf( "%s:%f ", field->name, * ( float * ) toF );
						}
					}
				}
			}
			else
			{
				if ( MSG_ReadBits( msg, 1 ) == 0 )
				{
					*toF = 0;
				}
				else
				{
					// integer
					*toF = MSG_ReadBits( msg, field->bits );

					if ( print )
					{
						Com_Printf( "%s:%i ", field->name, *toF );
					}
				}
			}
		}
	}

	for ( i = lc, field = &entityStateFields[ lc ]; i < numFields; i++, field++ )
	{
		fromF = ( int * )( ( byte * ) from + field->offset );
		toF = ( int * )( ( byte * ) to + field->offset );
		// no change
		*toF = *fromF;
	}

	if ( print )
	{
		if ( msg->bit == 0 )
		{
			endBit = msg->readcount * 8 - GENTITYNUM_BITS;
		}
		else
		{
			endBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
		}

		Com_Printf( " (%i bits)\n", endBit - startBit );
	}
}

/*
============================================================================

player_state_t communication

============================================================================
*/

// using the stringizing operator to save typing...
#define PSF( x ) # x,int((size_t)&( (playerState_t*)0 )->x)

static netField_t playerStateFields[] =
{
	{ PSF( commandTime ),          32             , 0 }
	,
	{ PSF( pm_type ),              8              , 0 }
	,
	{ PSF( bobCycle ),             8              , 0 }
	,
	{ PSF( pm_flags ),             16             , 0 }
	,
	{ PSF( pm_time ),              -16            , 0 }
	,
	{ PSF( origin[ 0 ] ),          0              , 0 }
	,
	{ PSF( origin[ 1 ] ),          0              , 0 }
	,
	{ PSF( origin[ 2 ] ),          0              , 0 }
	,
	{ PSF( velocity[ 0 ] ),        0              , 0 }
	,
	{ PSF( velocity[ 1 ] ),        0              , 0 }
	,
	{ PSF( velocity[ 2 ] ),        0              , 0 }
	,
	{ PSF( weaponTime ),           -16            , 0 }
	,
	{ PSF( gravity ),              16             , 0 }
	,
	{ PSF( speed ),                16             , 0 }
	,
	{ PSF( delta_angles[ 0 ] ),    16             , 0 }
	,
	{ PSF( delta_angles[ 1 ] ),    16             , 0 }
	,
	{ PSF( delta_angles[ 2 ] ),    16             , 0 }
	,
	{ PSF( groundEntityNum ),      GENTITYNUM_BITS, 0 }
	,
	{ PSF( legsTimer ),            16             , 0 }
	,
	{ PSF( torsoTimer ),           16             , 0 }
	,
	{ PSF( legsAnim ),             ANIM_BITS      , 0 }
	,
	{ PSF( torsoAnim ),            ANIM_BITS      , 0 }
	,
	{ PSF( movementDir ),          8              , 0 }
	,
	{ PSF( eFlags ),               24             , 0 }
	,
	{ PSF( eventSequence ),        8              , 0 }
	,
	{ PSF( events[ 0 ] ),          8              , 0 }
	,
	{ PSF( events[ 1 ] ),          8              , 0 }
	,
	{ PSF( events[ 2 ] ),          8              , 0 }
	,
	{ PSF( events[ 3 ] ),          8              , 0 }
	,
	{ PSF( eventParms[ 0 ] ),      8              , 0 }
	,
	{ PSF( eventParms[ 1 ] ),      8              , 0 }
	,
	{ PSF( eventParms[ 2 ] ),      8              , 0 }
	,
	{ PSF( eventParms[ 3 ] ),      8              , 0 }
	,
	{ PSF( clientNum ),            8              , 0 }
	,
	{ PSF( weapon ),               7              , 0 }
	,
	{ PSF( weaponstate ),          4              , 0 }
	,
	{ PSF( viewangles[ 0 ] ),      0              , 0 }
	,
	{ PSF( viewangles[ 1 ] ),      0              , 0 }
	,
	{ PSF( viewangles[ 2 ] ),      0              , 0 }
	,
	{ PSF( viewheight ),           -8             , 0 }
	,
	{ PSF( damageEvent ),          8              , 0 }
	,
	{ PSF( damageYaw ),            8              , 0 }
	,
	{ PSF( damagePitch ),          8              , 0 }
	,
	{ PSF( damageCount ),          8              , 0 }
	,
	{ PSF( generic1 ),             10             , 0 }
	,
	{ PSF( loopSound ),            16             , 0 }
	,
	{ PSF( grapplePoint[ 0 ] ),    0              , 0 }
	,
	{ PSF( grapplePoint[ 1 ] ),    0              , 0 }
	,
	{ PSF( grapplePoint[ 2 ] ),    0              , 0 }
	,
	{ PSF( ammo ),                 12             , 0 }
	,
	{ PSF( clips ),                4              , 0 }
	,
	{ PSF( tauntTimer ),           12             , 0 }
	,
	{ PSF( otherEntityNum ),       10             , 0 }
	,
	{ PSF( weaponAnim ),           ANIM_BITS      , 0 }
};

static int QDECL qsort_playerstatefields( const void *a, const void *b )
{
	int aa, bb;

	aa = * ( ( int * ) a );
	bb = * ( ( int * ) b );

	if ( playerStateFields[ aa ].used > playerStateFields[ bb ].used )
	{
		return -1;
	}

	if ( playerStateFields[ bb ].used > playerStateFields[ aa ].used )
	{
		return 1;
	}

	return 0;
}

void MSG_PrioritisePlayerStateFields()
{
	int fieldorders[ ARRAY_LEN( playerStateFields ) ];
	int numfields = ARRAY_LEN( playerStateFields );
	int i;

	for ( i = 0; i < numfields; i++ )
	{
		fieldorders[ i ] = i;
	}

	qsort( fieldorders, numfields, sizeof( int ), qsort_playerstatefields );

	Com_Printf( "Playerstate fields in order of priority\n" );
	Com_Printf( "netField_t playerStateFields[] = {\n" );

	for ( i = 0; i < numfields; i++ )
	{
		Com_Printf( "{ PSF(%s), %i },\n", playerStateFields[ fieldorders[ i ] ].name, playerStateFields[ fieldorders[ i ] ].bits );
	}

	Com_Printf( "};\n" );
}

/*
=============
MSG_WriteDeltaPlayerstate

=============
*/
void MSG_WriteDeltaPlayerstate( msg_t *msg, playerState_t *from, playerState_t *to )
{
	int           i, lc;
	playerState_t dummy;
	int           statsbits;
	int           persistantbits;
	int           numFields;
	netField_t *field;
	int        *fromF, *toF;
	float      fullFloat;
	int        trunc;
	int        startBit, endBit;
	int        print;
	int        miscbits;

	if ( !from )
	{
		from = &dummy;
		memset( &dummy, 0, sizeof( dummy ) );
	}

	if ( msg->bit == 0 )
	{
		startBit = msg->cursize * 8 - GENTITYNUM_BITS;
	}
	else
	{
		startBit = ( msg->cursize - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
	}

	// shownet 2/3 will interleave with other printed info, -2 will
	// just print the delta records
	if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -2 ) )
	{
		print = 1;
		Com_Printf( "W|%3i: playerstate ", msg->cursize );
	}
	else
	{
		print = 0;
	}

	numFields = ARRAY_LEN( playerStateFields );

	lc = 0;

	for ( i = 0, field = playerStateFields; i < numFields; i++, field++ )
	{
		fromF = ( int * )( ( byte * ) from + field->offset );
		toF = ( int * )( ( byte * ) to + field->offset );

		if ( *fromF != *toF )
		{
			lc = i + 1;

			field->used++;
		}
	}

	MSG_WriteByte( msg, lc );  // # of changes

	for ( i = 0, field = playerStateFields; i < lc; i++, field++ )
	{
		fromF = ( int * )( ( byte * ) from + field->offset );
		toF = ( int * )( ( byte * ) to + field->offset );

		if ( *fromF == *toF )
		{
			MSG_WriteBits( msg, 0, 1 );  // no change
			continue;
		}

		MSG_WriteBits( msg, 1, 1 );  // changed

		if ( field->bits == 0 )
		{
			// float
			fullFloat = * ( float * ) toF;
			trunc = ( int ) fullFloat;

			if ( trunc == fullFloat && trunc + FLOAT_INT_BIAS >= 0 && trunc + FLOAT_INT_BIAS < ( 1 << FLOAT_INT_BITS ) )
			{
				// send as small integer
				MSG_WriteBits( msg, 0, 1 );
				MSG_WriteBits( msg, trunc + FLOAT_INT_BIAS, FLOAT_INT_BITS );
//              if ( print ) {
//                  Com_Printf( "%s:%i ", field->name, trunc );
//              }
			}
			else
			{
				// send as full floating point value
				MSG_WriteBits( msg, 1, 1 );
				MSG_WriteBits( msg, *toF, 32 );
//              if ( print ) {
//                  Com_Printf( "%s:%f ", field->name, *(float *)toF );
//              }
			}
		}
		else
		{
			// integer
			MSG_WriteBits( msg, *toF, field->bits );
//          if ( print ) {
//              Com_Printf( "%s:%i ", field->name, *toF );
//          }
		}
	}

	//
	// send the arrays
	//
	statsbits = 0;

	for ( i = 0; i < MAX_STATS; i++ )
	{
		if ( to->stats[ i ] != from->stats[ i ] )
		{
			statsbits |= 1 << i;
		}
	}

	persistantbits = 0;

	for ( i = 0; i < MAX_PERSISTANT; i++ )
	{
		if ( to->persistant[ i ] != from->persistant[ i ] )
		{
			persistantbits |= 1 << i;
		}
	}

	miscbits = 0;

	for ( i = 0; i < MAX_MISC; i++ )
	{
		if ( to->misc[ i ] != from->misc[ i ] )
		{
			miscbits |= 1 << i;
		}
	}

	if ( statsbits || persistantbits || miscbits )
	{
		MSG_WriteBits( msg, 1, 1 );  // something changed

		if ( statsbits )
		{
			MSG_WriteBits( msg, 1, 1 );  // changed
			MSG_WriteShort( msg, statsbits );

			for ( i = 0; i < MAX_STATS; i++ )
			{
				if ( statsbits & ( 1 << i ) )
				{
					// RF, changed to long to allow more flexibility
//                  MSG_WriteLong (msg, to->stats[i]);
					MSG_WriteShort( msg, to->stats[ i ] );  //----(SA)    back to short since weapon bits are handled elsewhere now
				}
			}
		}
		else
		{
			MSG_WriteBits( msg, 0, 1 );  // no change to stats
		}

		if ( persistantbits )
		{
			MSG_WriteBits( msg, 1, 1 );  // changed
			MSG_WriteShort( msg, persistantbits );

			for ( i = 0; i < MAX_PERSISTANT; i++ )
			{
				if ( persistantbits & ( 1 << i ) )
				{
					MSG_WriteShort( msg, to->persistant[ i ] );
				}
			}
		}
		else
		{
			MSG_WriteBits( msg, 0, 1 );  // no change to persistent
		}

		if ( miscbits )
		{
			MSG_WriteBits( msg, 1, 1 );  // changed
			MSG_WriteBits( msg, miscbits, MAX_MISC );

			for ( i = 0; i < MAX_MISC; i++ )
			{
				if ( miscbits & ( 1 << i ) )
				{
					MSG_WriteLong( msg, to->misc[ i ] );
				}
			}
		}
		else
		{
			MSG_WriteBits( msg, 0, 1 );  // no change
		}
	}
	else
	{
		MSG_WriteBits( msg, 0, 1 );  // no change to any
	}

	if ( print )
	{
		if ( msg->bit == 0 )
		{
			endBit = msg->cursize * 8 - GENTITYNUM_BITS;
		}
		else
		{
			endBit = ( msg->cursize - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
		}

		Com_Printf( " (%i bits)\n", endBit - startBit );
	}
}

/*
===================
MSG_ReadDeltaPlayerstate
===================
*/
void MSG_ReadDeltaPlayerstate( msg_t *msg, playerState_t *from, playerState_t *to )
{
	int           i, lc;
	int           bits;
	netField_t    *field;
	int           numFields;
	int           startBit, endBit;
	int           print;
	int           *fromF, *toF;
	int           trunc;
	playerState_t dummy;

	if ( !from )
	{
		from = &dummy;
		memset( &dummy, 0, sizeof( dummy ) );
	}

	*to = *from;

	if ( msg->bit == 0 )
	{
		startBit = msg->readcount * 8 - GENTITYNUM_BITS;
	}
	else
	{
		startBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
	}

	// shownet 2/3 will interleave with other printed info, -2 will
	// just print the delta records
	if ( cl_shownet && ( cl_shownet->integer >= 2 || cl_shownet->integer == -2 ) )
	{
		print = 1;
		Com_Printf( "%3i: playerstate ", msg->readcount );
	}
	else
	{
		print = 0;
	}

	numFields = ARRAY_LEN( playerStateFields );
	lc = MSG_ReadByte( msg );

	if ( lc > numFields || lc < 0 )
	{
		Com_Error( ERR_DROP, "invalid playerState field count" );
	}

	for ( i = 0, field = playerStateFields; i < lc; i++, field++ )
	{
		fromF = ( int * )( ( byte * ) from + field->offset );
		toF = ( int * )( ( byte * ) to + field->offset );

		if ( !MSG_ReadBits( msg, 1 ) )
		{
			// no change
			*toF = *fromF;
		}
		else
		{
			if ( field->bits == 0 )
			{
				// float
				if ( MSG_ReadBits( msg, 1 ) == 0 )
				{
					// integral float
					trunc = MSG_ReadBits( msg, FLOAT_INT_BITS );
					// bias to allow equal parts positive and negative
					trunc -= FLOAT_INT_BIAS;
					* ( float * ) toF = trunc;

					if ( print )
					{
						Com_Printf( "%s:%i ", field->name, trunc );
					}
				}
				else
				{
					// full floating point value
					*toF = MSG_ReadBits( msg, 32 );

					if ( print )
					{
						Com_Printf( "%s:%f ", field->name, * ( float * ) toF );
					}
				}
			}
			else
			{
				// integer
				*toF = MSG_ReadBits( msg, field->bits );

				if ( print )
				{
					Com_Printf( "%s:%i ", field->name, *toF );
				}
			}
		}
	}

	for ( i = lc, field = &playerStateFields[ lc ]; i < numFields; i++, field++ )
	{
		fromF = ( int * )( ( byte * ) from + field->offset );
		toF = ( int * )( ( byte * ) to + field->offset );
		// no change
		*toF = *fromF;
	}

	// read the arrays
	if ( MSG_ReadBits( msg, 1 ) )
	{
		// one general bit tells if any of this infrequently changing stuff has changed
		// parse stats
		if ( MSG_ReadBits( msg, 1 ) )
		{
			LOG( "PS_STATS" );
			bits = MSG_ReadShort( msg );

			for ( i = 0; i < MAX_STATS; i++ )
			{
				if ( bits & ( 1 << i ) )
				{
					// RF, changed to long to allow more flexibility
//                  to->stats[i] = MSG_ReadLong(msg);
					to->stats[ i ] = MSG_ReadShort( msg );  //----(SA)    back to short since weapon bits are handled elsewhere now
				}
			}
		}

		// parse persistent stats
		if ( MSG_ReadBits( msg, 1 ) )
		{
			LOG( "PS_PERSISTANT" );
			bits = MSG_ReadShort( msg );

			for ( i = 0; i < MAX_PERSISTANT; i++ )
			{
				if ( bits & ( 1 << i ) )
				{
					to->persistant[ i ] = MSG_ReadShort( msg );
				}
			}
		}

		// parse misc data
		if ( MSG_ReadBits( msg, 1 ) )
		{
			LOG( "PS_MISC" );
			bits = MSG_ReadBits( msg, MAX_MISC );

			for ( i = 0; i < MAX_MISC; i++ )
			{
				if ( bits & ( 1 << i ) )
				{
					to->misc[ i ] = MSG_ReadLong( msg );
				}
			}
		}
	}

	if ( print )
	{
		if ( msg->bit == 0 )
		{
			endBit = msg->readcount * 8 - GENTITYNUM_BITS;
		}
		else
		{
			endBit = ( msg->readcount - 1 ) * 8 + msg->bit - GENTITYNUM_BITS;
		}

		Com_Printf( " (%i bits)\n", endBit - startBit );
	}
}

static const int msg_hData[ 256 ] =
{
	250315, // 0
	41193, // 1
	6292, // 2
	7106, // 3
	3730, // 4
	3750, // 5
	6110, // 6
	23283, // 7
	33317, // 8
	6950, // 9
	7838, // 10
	9714, // 11
	9257, // 12
	17259, // 13
	3949, // 14
	1778, // 15
	8288, // 16
	1604, // 17
	1590, // 18
	1663, // 19
	1100, // 20
	1213, // 21
	1238, // 22
	1134, // 23
	1749, // 24
	1059, // 25
	1246, // 26
	1149, // 27
	1273, // 28
	4486, // 29
	2805, // 30
	3472, // 31
	21819, // 32
	1159, // 33
	1670, // 34
	1066, // 35
	1043, // 36
	1012, // 37
	1053, // 38
	1070, // 39
	1726, // 40
	888, // 41
	1180, // 42
	850, // 43
	960, // 44
	780, // 45
	1752, // 46
	3296, // 47
	10630, // 48
	4514, // 49
	5881, // 50
	2685, // 51
	4650, // 52
	3837, // 53
	2093, // 54
	1867, // 55
	2584, // 56
	1949, // 57
	1972, // 58
	940, // 59
	1134, // 60
	1788, // 61
	1670, // 62
	1206, // 63
	5719, // 64
	6128, // 65
	7222, // 66
	6654, // 67
	3710, // 68
	3795, // 69
	1492, // 70
	1524, // 71
	2215, // 72
	1140, // 73
	1355, // 74
	971, // 75
	2180, // 76
	1248, // 77
	1328, // 78
	1195, // 79
	1770, // 80
	1078, // 81
	1264, // 82
	1266, // 83
	1168, // 84
	965, // 85
	1155, // 86
	1186, // 87
	1347, // 88
	1228, // 89
	1529, // 90
	1600, // 91
	2617, // 92
	2048, // 93
	2546, // 94
	3275, // 95
	2410, // 96
	3585, // 97
	2504, // 98
	2800, // 99
	2675, // 100
	6146, // 101
	3663, // 102
	2840, // 103
	14253, // 104
	3164, // 105
	2221, // 106
	1687, // 107
	3208, // 108
	2739, // 109
	3512, // 110
	4796, // 111
	4091, // 112
	3515, // 113
	5288, // 114
	4016, // 115
	7937, // 116
	6031, // 117
	5360, // 118
	3924, // 119
	4892, // 120
	3743, // 121
	4566, // 122
	4807, // 123
	5852, // 124
	6400, // 125
	6225, // 126
	8291, // 127
	23243, // 128
	7838, // 129
	7073, // 130
	8935, // 131
	5437, // 132
	4483, // 133
	3641, // 134
	5256, // 135
	5312, // 136
	5328, // 137
	5370, // 138
	3492, // 139
	2458, // 140
	1694, // 141
	1821, // 142
	2121, // 143
	1916, // 144
	1149, // 145
	1516, // 146
	1367, // 147
	1236, // 148
	1029, // 149
	1258, // 150
	1104, // 151
	1245, // 152
	1006, // 153
	1149, // 154
	1025, // 155
	1241, // 156
	952, // 157
	1287, // 158
	997, // 159
	1713, // 160
	1009, // 161
	1187, // 162
	879, // 163
	1099, // 164
	929, // 165
	1078, // 166
	951, // 167
	1656, // 168
	930, // 169
	1153, // 170
	1030, // 171
	1262, // 172
	1062, // 173
	1214, // 174
	1060, // 175
	1621, // 176
	930, // 177
	1106, // 178
	912, // 179
	1034, // 180
	892, // 181
	1158, // 182
	990, // 183
	1175, // 184
	850, // 185
	1121, // 186
	903, // 187
	1087, // 188
	920, // 189
	1144, // 190
	1056, // 191
	3462, // 192
	2240, // 193
	4397, // 194
	12136, // 195
	7758, // 196
	1345, // 197
	1307, // 198
	3278, // 199
	1950, // 200
	886, // 201
	1023, // 202
	1112, // 203
	1077, // 204
	1042, // 205
	1061, // 206
	1071, // 207
	1484, // 208
	1001, // 209
	1096, // 210
	915, // 211
	1052, // 212
	995, // 213
	1070, // 214
	876, // 215
	1111, // 216
	851, // 217
	1059, // 218
	805, // 219
	1112, // 220
	923, // 221
	1103, // 222
	817, // 223
	1899, // 224
	1872, // 225
	976, // 226
	841, // 227
	1127, // 228
	956, // 229
	1159, // 230
	950, // 231
	7791, // 232
	954, // 233
	1289, // 234
	933, // 235
	1127, // 236
	3207, // 237
	1020, // 238
	927, // 239
	1355, // 240
	768, // 241
	1040, // 242
	745, // 243
	952, // 244
	805, // 245
	1073, // 246
	740, // 247
	1013, // 248
	805, // 249
	1008, // 250
	796, // 251
	996, // 252
	1057, // 253
	11457, // 254
	13504, // 255
};

void MSG_initHuffman()
{
	int i, j;

	msgInit = true;
	Huff_Init( &msgHuff );

	for ( i = 0; i < 256; i++ )
	{
		for ( j = 0; j < msg_hData[ i ]; j++ )
		{
			Huff_addRef( &msgHuff.compressor, ( byte ) i );  /* Do update */
			Huff_addRef( &msgHuff.decompressor, ( byte ) i );  /* Do update */
		}
	}
}

//===========================================================================
