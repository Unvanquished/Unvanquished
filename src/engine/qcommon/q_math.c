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

// q_math.c -- stateless support routines that are included in each code module
// Some of the vector functions are static inline in q_shared.h. q3asm
// doesn't understand static functions though, so we only want them in
// one file. That's what this is about.
#ifdef Q3_VM
#define FLT_EPSILON 1.19209290E-07F
#define atanf(x) ( (float) atan( (x) ) )
#define tanf(x) ( (float) tan( (x) ) )
#define sqrtf(x) ( (float) sqrt( (x) ) )
extern double asin( double );
#endif

#include "q_shared.h"

// *INDENT-OFF*
vec3_t   vec3_origin = { 0, 0, 0 };
vec3_t   axisDefault[ 3 ] = { { 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 } };

matrix_t matrixIdentity = {     1, 0, 0, 0,
                                0, 1, 0, 0,
                                0, 0, 1, 0,
                                0, 0, 0, 1
                          };

quat_t   quatIdentity = { 0, 0, 0, 1 };

vec4_t   colorBlack = { 0, 0, 0, 1 };
vec4_t   colorRed = { 1, 0, 0, 1 };
vec4_t   colorGreen = { 0, 1, 0, 1 };
vec4_t   colorBlue = { 0, 0, 1, 1 };
vec4_t   colorYellow = { 1, 1, 0, 1 };
vec4_t   colorOrange = { 1, 0.5, 0, 1 };
vec4_t   colorMagenta = { 1, 0, 1, 1 };
vec4_t   colorCyan = { 0, 1, 1, 1 };
vec4_t   colorWhite = { 1, 1, 1, 1 };
vec4_t   colorLtGrey = { 0.75, 0.75, 0.75, 1 };
vec4_t   colorMdGrey = { 0.5, 0.5, 0.5, 1 };
vec4_t   colorDkGrey = { 0.25, 0.25, 0.25, 1 };
vec4_t   colorMdRed = { 0.5, 0, 0, 1 };
vec4_t   colorMdGreen = { 0, 0.5, 0, 1 };
vec4_t   colorDkGreen = { 0, 0.20, 0, 1 };
vec4_t   colorMdCyan = { 0, 0.5, 0.5, 1 };
vec4_t   colorMdYellow = { 0.5, 0.5, 0, 1 };
vec4_t   colorMdOrange = { 0.5, 0.25, 0, 1 };
vec4_t   colorMdBlue = { 0, 0, 0.5, 1 };

vec4_t   clrBrown = { 0.68f,         0.68f,          0.56f,          1.f };
vec4_t   clrBrownDk = { 0.58f * 0.75f, 0.58f * 0.75f,  0.46f * 0.75f,  1.f };
vec4_t   clrBrownLine = { 0.0525f,       0.05f,          0.025f,         0.2f };
vec4_t   clrBrownLineFull = { 0.0525f,       0.05f,          0.025f,         1.f };

vec4_t   clrBrownTextLt2 = { 108 * 1.8 / 255.f,     88 * 1.8 / 255.f,   62 * 1.8 / 255.f,   1.f };
vec4_t   clrBrownTextLt = { 108 * 1.3 / 255.f,     88 * 1.3 / 255.f,   62 * 1.3 / 255.f,   1.f };
vec4_t   clrBrownText = { 108 / 255.f,         88 / 255.f,       62 / 255.f,       1.f };
vec4_t   clrBrownTextDk = { 20 / 255.f,          2 / 255.f,        0 / 255.f,        1.f };
vec4_t   clrBrownTextDk2 = { 108 * 0.75 / 255.f,    88 * 0.75 / 255.f,  62 * 0.75 / 255.f,  1.f };

vec4_t   g_color_table[ 32 ] =
{
	{ 0.2,  0.2,   0.2,     1.0    }, // 0 - black    0
	{ 1.0,  0.0,   0.0,     1.0    }, // 1 - red      1
	{ 0.0,  1.0,   0.0,     1.0    }, // 2 - green    2
	{ 1.0,  1.0,   0.0,     1.0    }, // 3 - yellow   3
	{ 0.0,  0.0,   1.0,     1.0    }, // 4 - blue     4
	{ 0.0,  1.0,   1.0,     1.0    }, // 5 - cyan     5
	{ 1.0,  0.0,   1.0,     1.0    }, // 6 - purple   6
	{ 1.0,  1.0,   1.0,     1.0    }, // 7 - white    7
	{ 1.0,  0.5,   0.0,     1.0    }, // 8 - orange   8
	{ 0.5,  0.5,   0.5,     1.0    }, // 9 - md.grey    9
	{ 0.75, 0.75,  0.75,    1.0    }, // : - lt.grey    10    // lt grey for names
	{ 0.75, 0.75,  0.75,    1.0    }, // ; - lt.grey    11
	{ 0.0,  0.5,   0.0,     1.0    }, // < - md.green   12
	{ 0.5,  0.5,   0.0,     1.0    }, // = - md.yellow  13
	{ 0.0,  0.0,   0.5,     1.0    }, // > - md.blue    14
	{ 0.5,  0.0,   0.0,     1.0    }, // ? - md.red   15
	{ 0.5,  0.25,  0.0,     1.0    }, // @ - md.orange  16
	{ 1.0,  0.6f,  0.1f,    1.0    }, // A - lt.orange  17
	{ 0.0,  0.5,   0.5,     1.0    }, // B - md.cyan    18
	{ 0.5,  0.0,   0.5,     1.0    }, // C - md.purple  19
	{ 0.0,  0.5,   1.0,     1.0    }, // D        20
	{ 0.5,  0.0,   1.0,     1.0    }, // E        21
	{ 0.2f, 0.6f,  0.8f,    1.0    }, // F        22
	{ 0.8f, 1.0,   0.8f,    1.0    }, // G        23
	{ 0.0,  0.4,   0.2f,    1.0    }, // H        24
	{ 1.0,  0.0,   0.2f,    1.0    }, // I        25
	{ 0.7f, 0.1f,  0.1f,    1.0    }, // J        26
	{ 0.6f, 0.2f,  0.0,     1.0    }, // K        27
	{ 0.8f, 0.6f,  0.2f,    1.0    }, // L        28
	{ 0.6f, 0.6f,  0.2f,    1.0    }, // M        29
	{ 1.0,  1.0,   0.75,    1.0    }, // N        30
	{ 1.0,  1.0,   0.5,     1.0    }, // O        31
};

vec3_t   bytedirs[ NUMVERTEXNORMALS ] =
{
	{ -0.525731, 0.000000,  0.850651  }, { -0.442863, 0.238856,  0.864188  },
	{ -0.295242, 0.000000,  0.955423  }, { -0.309017, 0.500000,  0.809017  },
	{ -0.162460, 0.262866,  0.951056  }, { 0.000000,  0.000000,  1.000000  },
	{ 0.000000,  0.850651,  0.525731  }, { -0.147621, 0.716567,  0.681718  },
	{ 0.147621,  0.716567,  0.681718  }, { 0.000000,  0.525731,  0.850651  },
	{ 0.309017,  0.500000,  0.809017  }, { 0.525731,  0.000000,  0.850651  },
	{ 0.295242,  0.000000,  0.955423  }, { 0.442863,  0.238856,  0.864188  },
	{ 0.162460,  0.262866,  0.951056  }, { -0.681718, 0.147621,  0.716567  },
	{ -0.809017, 0.309017,  0.500000  }, { -0.587785, 0.425325,  0.688191  },
	{ -0.850651, 0.525731,  0.000000  }, { -0.864188, 0.442863,  0.238856  },
	{ -0.716567, 0.681718,  0.147621  }, { -0.688191, 0.587785,  0.425325  },
	{ -0.500000, 0.809017,  0.309017  }, { -0.238856, 0.864188,  0.442863  },
	{ -0.425325, 0.688191,  0.587785  }, { -0.716567, 0.681718,  -0.147621 },
	{ -0.500000, 0.809017,  -0.309017 }, { -0.525731, 0.850651,  0.000000  },
	{ 0.000000,  0.850651,  -0.525731 }, { -0.238856, 0.864188,  -0.442863 },
	{ 0.000000,  0.955423,  -0.295242 }, { -0.262866, 0.951056,  -0.162460 },
	{ 0.000000,  1.000000,  0.000000  }, { 0.000000,  0.955423,  0.295242  },
	{ -0.262866, 0.951056,  0.162460  }, { 0.238856,  0.864188,  0.442863  },
	{ 0.262866,  0.951056,  0.162460  }, { 0.500000,  0.809017,  0.309017  },
	{ 0.238856,  0.864188,  -0.442863 }, { 0.262866,  0.951056,  -0.162460 },
	{ 0.500000,  0.809017,  -0.309017 }, { 0.850651,  0.525731,  0.000000  },
	{ 0.716567,  0.681718,  0.147621  }, { 0.716567,  0.681718,  -0.147621 },
	{ 0.525731,  0.850651,  0.000000  }, { 0.425325,  0.688191,  0.587785  },
	{ 0.864188,  0.442863,  0.238856  }, { 0.688191,  0.587785,  0.425325  },
	{ 0.809017,  0.309017,  0.500000  }, { 0.681718,  0.147621,  0.716567  },
	{ 0.587785,  0.425325,  0.688191  }, { 0.955423,  0.295242,  0.000000  },
	{ 1.000000,  0.000000,  0.000000  }, { 0.951056,  0.162460,  0.262866  },
	{ 0.850651,  -0.525731, 0.000000  }, { 0.955423,  -0.295242, 0.000000  },
	{ 0.864188,  -0.442863, 0.238856  }, { 0.951056,  -0.162460, 0.262866  },
	{ 0.809017,  -0.309017, 0.500000  }, { 0.681718,  -0.147621, 0.716567  },
	{ 0.850651,  0.000000,  0.525731  }, { 0.864188,  0.442863,  -0.238856 },
	{ 0.809017,  0.309017,  -0.500000 }, { 0.951056,  0.162460,  -0.262866 },
	{ 0.525731,  0.000000,  -0.850651 }, { 0.681718,  0.147621,  -0.716567 },
	{ 0.681718,  -0.147621, -0.716567 }, { 0.850651,  0.000000,  -0.525731 },
	{ 0.809017,  -0.309017, -0.500000 }, { 0.864188,  -0.442863, -0.238856 },
	{ 0.951056,  -0.162460, -0.262866 }, { 0.147621,  0.716567,  -0.681718 },
	{ 0.309017,  0.500000,  -0.809017 }, { 0.425325,  0.688191,  -0.587785 },
	{ 0.442863,  0.238856,  -0.864188 }, { 0.587785,  0.425325,  -0.688191 },
	{ 0.688191,  0.587785,  -0.425325 }, { -0.147621, 0.716567,  -0.681718 },
	{ -0.309017, 0.500000,  -0.809017 }, { 0.000000,  0.525731,  -0.850651 },
	{ -0.525731, 0.000000,  -0.850651 }, { -0.442863, 0.238856,  -0.864188 },
	{ -0.295242, 0.000000,  -0.955423 }, { -0.162460, 0.262866,  -0.951056 },
	{ 0.000000,  0.000000,  -1.000000 }, { 0.295242,  0.000000,  -0.955423 },
	{ 0.162460,  0.262866,  -0.951056 }, { -0.442863, -0.238856, -0.864188 },
	{ -0.309017, -0.500000, -0.809017 }, { -0.162460, -0.262866, -0.951056 },
	{ 0.000000,  -0.850651, -0.525731 }, { -0.147621, -0.716567, -0.681718 },
	{ 0.147621,  -0.716567, -0.681718 }, { 0.000000,  -0.525731, -0.850651 },
	{ 0.309017,  -0.500000, -0.809017 }, { 0.442863,  -0.238856, -0.864188 },
	{ 0.162460,  -0.262866, -0.951056 }, { 0.238856,  -0.864188, -0.442863 },
	{ 0.500000,  -0.809017, -0.309017 }, { 0.425325,  -0.688191, -0.587785 },
	{ 0.716567,  -0.681718, -0.147621 }, { 0.688191,  -0.587785, -0.425325 },
	{ 0.587785,  -0.425325, -0.688191 }, { 0.000000,  -0.955423, -0.295242 },
	{ 0.000000,  -1.000000, 0.000000  }, { 0.262866,  -0.951056, -0.162460 },
	{ 0.000000,  -0.850651, 0.525731  }, { 0.000000,  -0.955423, 0.295242  },
	{ 0.238856,  -0.864188, 0.442863  }, { 0.262866,  -0.951056, 0.162460  },
	{ 0.500000,  -0.809017, 0.309017  }, { 0.716567,  -0.681718, 0.147621  },
	{ 0.525731,  -0.850651, 0.000000  }, { -0.238856, -0.864188, -0.442863 },
	{ -0.500000, -0.809017, -0.309017 }, { -0.262866, -0.951056, -0.162460 },
	{ -0.850651, -0.525731, 0.000000  }, { -0.716567, -0.681718, -0.147621 },
	{ -0.716567, -0.681718, 0.147621  }, { -0.525731, -0.850651, 0.000000  },
	{ -0.500000, -0.809017, 0.309017  }, { -0.238856, -0.864188, 0.442863  },
	{ -0.262866, -0.951056, 0.162460  }, { -0.864188, -0.442863, 0.238856  },
	{ -0.809017, -0.309017, 0.500000  }, { -0.688191, -0.587785, 0.425325  },
	{ -0.681718, -0.147621, 0.716567  }, { -0.442863, -0.238856, 0.864188  },
	{ -0.587785, -0.425325, 0.688191  }, { -0.309017, -0.500000, 0.809017  },
	{ -0.147621, -0.716567, 0.681718  }, { -0.425325, -0.688191, 0.587785  },
	{ -0.162460, -0.262866, 0.951056  }, { 0.442863,  -0.238856, 0.864188  },
	{ 0.162460,  -0.262866, 0.951056  }, { 0.309017,  -0.500000, 0.809017  },
	{ 0.147621,  -0.716567, 0.681718  }, { 0.000000,  -0.525731, 0.850651  },
	{ 0.425325,  -0.688191, 0.587785  }, { 0.587785,  -0.425325, 0.688191  },
	{ 0.688191,  -0.587785, 0.425325  }, { -0.955423, 0.295242,  0.000000  },
	{ -0.951056, 0.162460,  0.262866  }, { -1.000000, 0.000000,  0.000000  },
	{ -0.850651, 0.000000,  0.525731  }, { -0.955423, -0.295242, 0.000000  },
	{ -0.951056, -0.162460, 0.262866  }, { -0.864188, 0.442863,  -0.238856 },
	{ -0.951056, 0.162460,  -0.262866 }, { -0.809017, 0.309017,  -0.500000 },
	{ -0.864188, -0.442863, -0.238856 }, { -0.951056, -0.162460, -0.262866 },
	{ -0.809017, -0.309017, -0.500000 }, { -0.681718, 0.147621,  -0.716567 },
	{ -0.681718, -0.147621, -0.716567 }, { -0.850651, 0.000000,  -0.525731 },
	{ -0.688191, 0.587785,  -0.425325 }, { -0.587785, 0.425325,  -0.688191 },
	{ -0.425325, 0.688191,  -0.587785 }, { -0.425325, -0.688191, -0.587785 },
	{ -0.587785, -0.425325, -0.688191 }, { -0.688191, -0.587785, -0.425325 }
};
// *INDENT-ON*

//==============================================================

int Q_rand( int *seed )
{
	*seed = ( 69069 * *seed + 1 );
	return *seed;
}

float Q_random( int *seed )
{
	return ( Q_rand( seed ) & 0xffff ) / ( float ) 0x10000;
}

float Q_crandom( int *seed )
{
	return 2.0 * ( Q_random( seed ) - 0.5 );
}

//=======================================================

byte ClampByte( int i )
{
	if ( i < 0 )
	{
		return 0;
	}

	if ( i > 255 )
	{
		return 255;
	}

	return i;
}

signed char ClampChar( int i )
{
	if ( i < -128 )
	{
		return -128;
	}

	if ( i > 127 )
	{
		return 127;
	}

	return i;
}

signed short ClampShort( int i )
{
	if ( i < -32768 )
	{
		return -32768;
	}

	if ( i > 0x7fff )
	{
		return 0x7fff;
	}

	return i;
}

// this isn't a real cheap function to call!
int DirToByte( vec3_t dir )
{
	int   i, best;
	float d, bestd;

	if ( !dir )
	{
		return 0;
	}

	bestd = 0;
	best = 0;

	for ( i = 0; i < NUMVERTEXNORMALS; i++ )
	{
		d = DotProduct( dir, bytedirs[ i ] );

		if ( d > bestd )
		{
			bestd = d;
			best = i;
		}
	}

	return best;
}

void ByteToDir( int b, vec3_t dir )
{
	if ( b < 0 || b >= NUMVERTEXNORMALS )
	{
		VectorCopy( vec3_origin, dir );
		return;
	}

	VectorCopy( bytedirs[ b ], dir );
}

unsigned ColorBytes3( float r, float g, float b )
{
	unsigned i;

	( ( byte * ) &i ) [ 0 ] = r * 255;
	( ( byte * ) &i ) [ 1 ] = g * 255;
	( ( byte * ) &i ) [ 2 ] = b * 255;

	return i;
}

unsigned ColorBytes4( float r, float g, float b, float a )
{
	unsigned i;

	( ( byte * ) &i ) [ 0 ] = r * 255;
	( ( byte * ) &i ) [ 1 ] = g * 255;
	( ( byte * ) &i ) [ 2 ] = b * 255;
	( ( byte * ) &i ) [ 3 ] = a * 255;

	return i;
}

float NormalizeColor( const vec3_t in, vec3_t out )
{
	float max;

	max = in[ 0 ];

	if ( in[ 1 ] > max )
	{
		max = in[ 1 ];
	}

	if ( in[ 2 ] > max )
	{
		max = in[ 2 ];
	}

	if ( !max )
	{
		VectorClear( out );
	}
	else
	{
		out[ 0 ] = in[ 0 ] / max;
		out[ 1 ] = in[ 1 ] / max;
		out[ 2 ] = in[ 2 ] / max;
	}

	return max;
}

void ClampColor( vec4_t color )
{
	int i;

	for ( i = 0; i < 4; i++ )
	{
		if ( color[ i ] < 0 )
		{
			color[ i ] = 0;
		}

		if ( color[ i ] > 1 )
		{
			color[ i ] = 1;
		}
	}
}

vec_t PlaneNormalize( vec4_t plane )
{
	vec_t length, ilength;

	length = sqrt( plane[ 0 ] * plane[ 0 ] + plane[ 1 ] * plane[ 1 ] + plane[ 2 ] * plane[ 2 ] );

	if ( length == 0 )
	{
		VectorClear( plane );
		return 0;
	}

	ilength = 1.0 / length;
	plane[ 0 ] = plane[ 0 ] * ilength;
	plane[ 1 ] = plane[ 1 ] * ilength;
	plane[ 2 ] = plane[ 2 ] * ilength;
	plane[ 3 ] = plane[ 3 ] * ilength;

	return length;
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenerate.
The normal will point out of the clock for clockwise ordered points
=====================
*/
qboolean PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c )
{
	vec3_t d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );
	CrossProduct( d2, d1, plane );

	if ( VectorNormalize( plane ) == 0 )
	{
		return qfalse;
	}

	plane[ 3 ] = DotProduct( a, plane );
	return qtrue;
}

/*
=====================
PlaneFromPoints

Returns false if the triangle is degenerate.
=====================
*/
qboolean PlaneFromPointsOrder( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c, qboolean cw )
{
	vec3_t d1, d2;

	VectorSubtract( b, a, d1 );
	VectorSubtract( c, a, d2 );

	if ( cw )
	{
		CrossProduct( d2, d1, plane );
	}
	else
	{
		CrossProduct( d1, d2, plane );
	}

	if ( VectorNormalize( plane ) == 0 )
	{
		return qfalse;
	}

	plane[ 3 ] = DotProduct( a, plane );
	return qtrue;
}

qboolean PlanesGetIntersectionPoint( const vec4_t plane1, const vec4_t plane2, const vec4_t plane3, vec3_t out )
{
	// http://www.cgafaq.info/wiki/Intersection_of_three_planes

	vec3_t n1, n2, n3;
	vec3_t n1n2, n2n3, n3n1;
	vec_t  denom;

	VectorNormalize2( plane1, n1 );
	VectorNormalize2( plane2, n2 );
	VectorNormalize2( plane3, n3 );

	CrossProduct( n1, n2, n1n2 );
	CrossProduct( n2, n3, n2n3 );
	CrossProduct( n3, n1, n3n1 );

	denom = DotProduct( n1, n2n3 );

	// check if the denominator is zero (which would mean that no intersection is to be found
	if ( denom == 0 )
	{
		// no intersection could be found, return <0,0,0>
		VectorClear( out );
		return qfalse;
	}

	VectorClear( out );

	VectorMA( out, plane1[ 3 ], n2n3, out );
	VectorMA( out, plane2[ 3 ], n3n1, out );
	VectorMA( out, plane3[ 3 ], n1n2, out );

	VectorScale( out, 1.0f / denom, out );

	return qtrue;
}

void PlaneIntersectRay( const vec3_t rayPos, const vec3_t rayDir, const vec4_t plane, vec3_t res )
{
	vec3_t dir;
	float  sect;

	VectorNormalize2( rayDir, dir );

	sect = - ( DotProduct( plane, rayPos ) - plane[ 3 ] ) / DotProduct( plane, rayDir );
	VectorScale( dir, sect, dir );
	VectorAdd( rayPos, dir, res );
}

/*
===============
RotatePointAroundVector
===============
*/
void RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees )
{
	float sind, cosd, expr;
	vec3_t dxp;

	degrees = DEG2RAD( degrees );
	sind = sin( degrees );
	cosd = cos( degrees );
	expr = ( 1 - cosd ) * DotProduct( dir, point );
	CrossProduct( dir, point, dxp );

	dst[ 0 ] = expr * dir[ 0 ] + cosd * point[ 0 ] + sind * dxp[ 0 ];
	dst[ 1 ] = expr * dir[ 1 ] + cosd * point[ 1 ] + sind * dxp[ 1 ];
	dst[ 2 ] = expr * dir[ 2 ] + cosd * point[ 2 ] + sind * dxp[ 2 ];
}

/*
===============
RotatePointAroundVertex

Rotate a point around a vertex
===============
*/
void RotatePointAroundVertex( vec3_t pnt, float rot_x, float rot_y, float rot_z, const vec3_t origin )
{
	float tmp[ 11 ];

	//float rad_x, rad_y, rad_z;

	/*rad_x = DEG2RAD( rot_x );
	   rad_y = DEG2RAD( rot_y );
	   rad_z = DEG2RAD( rot_z ); */

	// move pnt to rel{0,0,0}
	VectorSubtract( pnt, origin, pnt );

	// init temp values
	tmp[ 0 ] = sin( rot_x );
	tmp[ 1 ] = cos( rot_x );
	tmp[ 2 ] = sin( rot_y );
	tmp[ 3 ] = cos( rot_y );
	tmp[ 4 ] = sin( rot_z );
	tmp[ 5 ] = cos( rot_z );
	tmp[ 6 ] = pnt[ 1 ] * tmp[ 5 ];
	tmp[ 7 ] = pnt[ 0 ] * tmp[ 4 ];
	tmp[ 8 ] = pnt[ 0 ] * tmp[ 5 ];
	tmp[ 9 ] = pnt[ 1 ] * tmp[ 4 ];
	tmp[ 10 ] = pnt[ 2 ] * tmp[ 3 ];

	// rotate point
	pnt[ 0 ] = ( tmp[ 3 ] * ( tmp[ 8 ] - tmp[ 9 ] ) + pnt[ 3 ] * tmp[ 2 ] );
	pnt[ 1 ] = ( tmp[ 0 ] * ( tmp[ 2 ] * tmp[ 8 ] - tmp[ 2 ] * tmp[ 9 ] - tmp[ 10 ] ) + tmp[ 1 ] * ( tmp[ 7 ] + tmp[ 6 ] ) );
	pnt[ 2 ] = ( tmp[ 1 ] * ( -tmp[ 2 ] * tmp[ 8 ] + tmp[ 2 ] * tmp[ 9 ] + tmp[ 10 ] ) + tmp[ 0 ] * ( tmp[ 7 ] + tmp[ 6 ] ) );

	// move pnt back
	VectorAdd( pnt, origin, pnt );
}

/*
===============
RotateAroundDirection
===============
*/
void RotateAroundDirection( vec3_t axis[ 3 ], float yaw )
{
	// create an arbitrary axis[1]
	PerpendicularVector( axis[ 1 ], axis[ 0 ] );

	// rotate it around axis[0] by yaw
	if ( yaw )
	{
		vec3_t temp;

		VectorCopy( axis[ 1 ], temp );
		RotatePointAroundVector( axis[ 1 ], axis[ 0 ], temp, yaw );
	}

	// cross to get axis[2]
	CrossProduct( axis[ 0 ], axis[ 1 ], axis[ 2 ] );
}

/*
================
Q_isnan

Don't pass doubles to this
================
*/
int Q_isnan( float x )
{
	union
	{
		float        f;
		unsigned int i;
	} t;

	t.f = x;
	t.i &= 0x7FFFFFFF;
	t.i = 0x7F800000 - t.i;

	return ( int )( ( unsigned int ) t.i >> 31 );
}

void vectoangles( const vec3_t value1, vec3_t angles )
{
	float forward;
	float yaw, pitch;

	if ( value1[ 1 ] == 0 && value1[ 0 ] == 0 )
	{
		yaw = 0;

		if ( value1[ 2 ] > 0 )
		{
			pitch = 90;
		}
		else
		{
			pitch = 270;
		}
	}
	else
	{
		if ( value1[ 0 ] )
		{
			yaw = ( atan2( value1[ 1 ], value1[ 0 ] ) * 180 / M_PI );
		}
		else if ( value1[ 1 ] > 0 )
		{
			yaw = 90;
		}
		else
		{
			yaw = 270;
		}

		if ( yaw < 0 )
		{
			yaw += 360;
		}

		forward = sqrt( value1[ 0 ] * value1[ 0 ] + value1[ 1 ] * value1[ 1 ] );
		pitch = ( atan2( value1[ 2 ], forward ) * 180 / M_PI );

		if ( pitch < 0 )
		{
			pitch += 360;
		}
	}

	angles[ PITCH ] = -pitch;
	angles[ YAW ] = yaw;
	angles[ ROLL ] = 0;
}

/*
=================
AnglesToAxis
=================
*/
void AnglesToAxis( const vec3_t angles, vec3_t axis[ 3 ] )
{
	vec3_t right;

	// angle vectors returns "right" instead of "y axis"
	AngleVectors( angles, axis[ 0 ], right, axis[ 2 ] );
	VectorSubtract( vec3_origin, right, axis[ 1 ] );
}

void AxisClear( vec3_t axis[ 3 ] )
{
	axis[ 0 ][ 0 ] = 1;
	axis[ 0 ][ 1 ] = 0;
	axis[ 0 ][ 2 ] = 0;
	axis[ 1 ][ 0 ] = 0;
	axis[ 1 ][ 1 ] = 1;
	axis[ 1 ][ 2 ] = 0;
	axis[ 2 ][ 0 ] = 0;
	axis[ 2 ][ 1 ] = 0;
	axis[ 2 ][ 2 ] = 1;
}

void AxisCopy( vec3_t in[ 3 ], vec3_t out[ 3 ] )
{
	VectorCopy( in[ 0 ], out[ 0 ] );
	VectorCopy( in[ 1 ], out[ 1 ] );
	VectorCopy( in[ 2 ], out[ 2 ] );
}

void ProjectPointOnPlane( vec3_t dst, const vec3_t point, const vec3_t normal )
{
	float d = -DotProduct( point, normal );
	VectorMA( point, d, normal, dst );
}

/*
================
MakeNormalVectors

Given a normalized forward vector, create two
other perpendicular vectors
================
*/
void MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up )
{
	float d;

	// this rotate and negate guarantees a vector
	// not colinear with the original
	right[ 1 ] = -forward[ 0 ];
	right[ 2 ] = forward[ 1 ];
	right[ 0 ] = forward[ 2 ];

	d = DotProduct( right, forward );
	VectorMA( right, -d, forward, right );
	VectorNormalize( right );
	CrossProduct( right, forward, up );
}

void VectorRotate( vec3_t in, vec3_t matrix[ 3 ], vec3_t out )
{
	out[ 0 ] = DotProduct( in, matrix[ 0 ] );
	out[ 1 ] = DotProduct( in, matrix[ 1 ] );
	out[ 2 ] = DotProduct( in, matrix[ 2 ] );
}

//============================================================================

// *INDENT-OFF*
#if id386 && !( ( defined __linux__ || defined __FreeBSD__ || defined __GNUC__ ) && ( defined __i386__ ) ) // rb010123
long myftol( float f )
{
	static int tmp;
	__asm fld f
	__asm fistp tmp
	__asm mov  eax, tmp
}

#endif
// *INDENT-ON*

//============================================================

/*
===============
LerpAngle

===============
*/
float LerpAngle( float from, float to, float frac )
{
	if ( to - from > 180 )
	{
		to -= 360;
	}

	if ( to - from < -180 )
	{
		to += 360;
	}

	return ( from + frac * ( to - from ) );
}

/*
=================
LerpPosition

=================
*/

void LerpPosition( vec3_t start, vec3_t end, float frac, vec3_t out )
{
	vec3_t dist;

	VectorSubtract( end, start, dist );
	VectorMA( start, frac, dist, out );
}

/*
=================
AngleSubtract

Always returns a value from -180 to 180
=================
*/
float AngleSubtract( float a1, float a2 )
{
	float a = a1 - a2;

	while ( a > 180 )
	{
		a -= 360;
	}

	while ( a < -180 )
	{
		a += 360;
	}

	return a;
}

void AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 )
{
	v3[ 0 ] = AngleSubtract( v1[ 0 ], v2[ 0 ] );
	v3[ 1 ] = AngleSubtract( v1[ 1 ], v2[ 1 ] );
	v3[ 2 ] = AngleSubtract( v1[ 2 ], v2[ 2 ] );
}

float AngleMod( float a )
{
	return ( ( 360.0 / 65536 ) * ( ( int )( a * ( 65536 / 360.0 ) ) & 65535 ) );
}

/*
=================
AngleNormalize2Pi

returns angle normalized to the range [0 <= angle < 2*M_PI]
=================
*/
float AngleNormalize2Pi( float angle )
{
	return DEG2RAD( AngleNormalize360( RAD2DEG( angle ) ) );
}

/*
=================
AngleNormalize360

returns angle normalized to the range [0 <= angle < 360]
=================
*/
float AngleNormalize360( float angle )
{
	return ( 360.0 / 65536 ) * ( ( int )( angle * ( 65536 / 360.0 ) ) & 65535 );
}

/*
=================
AngleNormalize180

returns angle normalized to the range [-180 < angle <= 180]
=================
*/
float AngleNormalize180( float angle )
{
	angle = AngleNormalize360( angle );

	if ( angle > 180.0 )
	{
		angle -= 360.0;
	}

	return angle;
}

/*
=================
AngleDelta

returns the normalized delta from angle1 to angle2
=================
*/
float AngleDelta( float angle1, float angle2 )
{
	return AngleNormalize180( angle1 - angle2 );
}

/*
=================
AngleBetweenVectors

returns the angle between two vectors normalized to the range [0 <= angle <= 180]
=================
*/
float AngleBetweenVectors( const vec3_t a, const vec3_t b )
{
	vec_t alen, blen;

	alen = VectorLength( a );
	blen = VectorLength( b );

	if ( !alen || !blen )
	{
		return 0;
	}

	// complete dot product of two vectors a, b is |a| * |b| * cos(angle)
	// this results in:
	//
	// angle = acos( (a * b) / (|a| * |b|) )
	return RAD2DEG( acos( DotProduct( a, b ) / ( alen * blen ) ) );
}

//============================================================

/*
=================
SetPlaneSignbits
=================
*/
void SetPlaneSignbits( cplane_t *out )
{
	int bits, j;

	// for fast box on planeside test
	bits = 0;

	for ( j = 0; j < 3; j++ )
	{
		if ( out->normal[ j ] < 0 )
		{
			bits |= 1 << j;
		}
	}

	out->signbits = bits;
}

/*
==================
BoxOnPlaneSide

Returns 1, 2, or 1 + 2

// this is the slow, general version
int BoxOnPlaneSide2 (vec3_t emins, vec3_t emaxs, struct cplane_s *p)
{
        int   i;
        float dist1, dist2;
        int   sides;
        vec3_t  corners[2];

        for (i=0 ; i<3 ; i++)
        {
                if (p->normal[i] < 0)
                {
                        corners[0][i] = emins[i];
                        corners[1][i] = emaxs[i];
                }
                else
                {
                        corners[1][i] = emins[i];
                        corners[0][i] = emaxs[i];
                }
        }
        dist1 = DotProduct (p->normal, corners[0]) - p->dist;
        dist2 = DotProduct (p->normal, corners[1]) - p->dist;
        sides = 0;
        if (dist1 >= 0)
                sides = 1;
        if (dist2 < 0)
                sides |= 2;

        return sides;
}

==================
*/

// *INDENT-OFF*
#if 1 //!( defined __linux__ && defined __i386__ && !defined C_ONLY )

#if 1 //defined __LCC__ || defined C_ONLY || !id386 || __GNUC__

int BoxOnPlaneSide( vec3_t emins, vec3_t emaxs, struct cplane_s *p )
{
	float dist1, dist2;
	int   sides;

	// fast axial cases
	if ( p->type < 3 )
	{
		if ( p->dist <= emins[ p->type ] )
		{
			return 1;
		}

		if ( p->dist >= emaxs[ p->type ] )
		{
			return 2;
		}

		return 3;
	}

	// general case
	switch ( p->signbits )
	{
		case 0:
			dist1 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			dist2 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			break;

		case 1:
			dist1 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			dist2 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			break;

		case 2:
			dist1 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			dist2 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			break;

		case 3:
			dist1 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			dist2 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			break;

		case 4:
			dist1 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			dist2 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			break;

		case 5:
			dist1 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			dist2 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			break;

		case 6:
			dist1 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			dist2 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			break;

		case 7:
			dist1 = p->normal[ 0 ] * emins[ 0 ] + p->normal[ 1 ] * emins[ 1 ] + p->normal[ 2 ] * emins[ 2 ];
			dist2 = p->normal[ 0 ] * emaxs[ 0 ] + p->normal[ 1 ] * emaxs[ 1 ] + p->normal[ 2 ] * emaxs[ 2 ];
			break;

		default:
			dist1 = dist2 = 0; // shut up compiler
			break;
	}

	sides = 0;

	if ( dist1 >= p->dist )
	{
		sides = 1;
	}

	if ( dist2 < p->dist )
	{
		sides |= 2;
	}

	return sides;
}

#else
#pragma warning( disable: 4035 )

__inline __declspec( naked ) int BoxOnPlaneSide_fast( vec3_t emins, vec3_t emaxs, struct cplane_s *p )
{
	static int bops_initialized;
	static int Ljmptab[ 8 ];

	__asm
	{
		push ebx

		cmp bops_initialized, 1
		je initialized
		mov bops_initialized, 1

		mov Ljmptab[ 0 * 4 ], offset Lcase0
		mov Ljmptab[ 1 * 4 ], offset Lcase1
		mov Ljmptab[ 2 * 4 ], offset Lcase2
		mov Ljmptab[ 3 * 4 ], offset Lcase3
		mov Ljmptab[ 4 * 4 ], offset Lcase4
		mov Ljmptab[ 5 * 4 ], offset Lcase5
		mov Ljmptab[ 6 * 4 ], offset Lcase6
		mov Ljmptab[ 7 * 4 ], offset Lcase7

		initialized:

		mov edx, dword ptr[ 4 + 12 + esp ]
		mov ecx, dword ptr[ 4 + 4 + esp ]
		xor eax, eax
		mov ebx, dword ptr[ 4 + 8 + esp ]
		mov al, byte ptr[ 17 + edx ]
		cmp al, 8
		jge Lerror
		fld dword ptr[ 0 + edx ]
		fld st( 0 )
		jmp dword ptr[ Ljmptab + eax * 4 ]
		Lcase0 :
		fmul dword ptr[ ebx ]
		fld dword ptr[ 0 + 4 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ ecx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 4 + ebx ]
		fld dword ptr[ 0 + 8 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ 4 + ecx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 8 + ebx ]
		fxch  st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[ 8 + ecx ]
		fxch  st( 1 )
		faddp st( 3 ), st( 0 )
		fxch  st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase1 :
		fmul dword ptr[ ecx ]
		fld dword ptr[ 0 + 4 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ ebx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 4 + ebx ]
		fld dword ptr[ 0 + 8 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ 4 + ecx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 8 + ebx ]
		fxch  st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[ 8 + ecx ]
		fxch  st( 1 )
		faddp st( 3 ), st( 0 )
		fxch  st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase2 :
		fmul dword ptr[ ebx ]
		fld dword ptr[ 0 + 4 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ ecx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 4 + ecx ]
		fld dword ptr[ 0 + 8 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ 4 + ebx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 8 + ebx ]
		fxch  st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[ 8 + ecx ]
		fxch  st( 1 )
		faddp st( 3 ), st( 0 )
		fxch  st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase3 :
		fmul dword ptr[ ecx ]
		fld dword ptr[ 0 + 4 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ ebx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 4 + ecx ]
		fld dword ptr[ 0 + 8 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ 4 + ebx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 8 + ebx ]
		fxch  st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[ 8 + ecx ]
		fxch  st( 1 )
		faddp st( 3 ), st( 0 )
		fxch  st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase4 :
		fmul dword ptr[ ebx ]
		fld dword ptr[ 0 + 4 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ ecx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 4 + ebx ]
		fld dword ptr[ 0 + 8 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ 4 + ecx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 8 + ecx ]
		fxch  st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[ 8 + ebx ]
		fxch  st( 1 )
		faddp st( 3 ), st( 0 )
		fxch  st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase5 :
		fmul dword ptr[ ecx ]
		fld dword ptr[ 0 + 4 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ ebx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 4 + ebx ]
		fld dword ptr[ 0 + 8 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ 4 + ecx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 8 + ecx ]
		fxch  st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[ 8 + ebx ]
		fxch  st( 1 )
		faddp st( 3 ), st( 0 )
		fxch  st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase6 :
		fmul dword ptr[ ebx ]
		fld dword ptr[ 0 + 4 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ ecx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 4 + ecx ]
		fld dword ptr[ 0 + 8 + edx ]
		fxch  st( 2 )
		fmul dword ptr[ 4 + ebx ]
		fxch  st( 2 )
		fld   st( 0 )
		fmul dword ptr[ 8 + ecx ]
		fxch  st( 5 )
		faddp st( 3 ), st( 0 )
		fmul dword ptr[ 8 + ebx ]
		fxch  st( 1 )
		faddp st( 3 ), st( 0 )
		fxch  st( 3 )
		faddp st( 2 ), st( 0 )
		jmp LSetSides
		Lcase7 :
		fmul dword ptr[ ecx ]
		fld dword ptr[ 0 + 4 + edx ]
		fxch          st( 2 )
		fmul dword ptr[ ebx ]
		fxch          st( 2 )
		fld           st( 0 )
		fmul dword ptr[ 4 + ecx ]
		fld dword ptr[ 0 + 8 + edx ]
		fxch          st( 2 )
		fmul dword ptr[ 4 + ebx ]
		fxch          st( 2 )
		fld           st( 0 )
		fmul dword ptr[ 8 + ecx ]
		fxch          st( 5 )
		faddp         st( 3 ), st( 0 )
		fmul dword ptr[ 8 + ebx ]
		fxch          st( 1 )
		faddp         st( 3 ), st( 0 )
		fxch          st( 3 )
		faddp         st( 2 ), st( 0 )
		LSetSides :
		faddp st( 2 ), st( 0 )
		fcomp dword ptr[ 12 + edx ]
		xor ecx, ecx
		fnstsw ax
		fcomp dword ptr[ 12 + edx ]
		and ah, 1
		xor ah, 1
		add cl, ah
		fnstsw ax
		and ah, 1
		add ah, ah
		add cl, ah
		pop ebx
		mov eax, ecx
		ret
		Lerror :
		int 3
	}
}

int BoxOnPlaneSide( vec3_t emins, vec3_t emaxs, struct cplane_s *p )
{
	// fast axial cases

	if ( p->type < 3 )
	{
		if ( p->dist <= emins[ p->type ] )
		{
			return 1;
		}

		if ( p->dist >= emaxs[ p->type ] )
		{
			return 2;
		}

		return 3;
	}

	return BoxOnPlaneSide_fast( emins, emaxs, p );
}

#pragma warning( default: 4035 )

#endif
#endif

// *INDENT-ON*

/*
=================
RadiusFromBounds
=================
*/
float RadiusFromBounds( const vec3_t mins, const vec3_t maxs )
{
	int    i;
	vec3_t corner;
	float  a, b;

	for ( i = 0; i < 3; i++ )
	{
		a = Q_fabs( mins[ i ] );
		b = Q_fabs( maxs[ i ] );
		corner[ i ] = a > b ? a : b;
	}

	return VectorLength( corner );
}

void ZeroBounds( vec3_t mins, vec3_t maxs )
{
	mins[ 0 ] = mins[ 1 ] = mins[ 2 ] = 0;
	maxs[ 0 ] = maxs[ 1 ] = maxs[ 2 ] = 0;
}

void ClearBounds( vec3_t mins, vec3_t maxs )
{
	mins[ 0 ] = mins[ 1 ] = mins[ 2 ] = 99999;
	maxs[ 0 ] = maxs[ 1 ] = maxs[ 2 ] = -99999;
}

void AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs )
{
	if ( v[ 0 ] < mins[ 0 ] )
	{
		mins[ 0 ] = v[ 0 ];
	}

	if ( v[ 0 ] > maxs[ 0 ] )
	{
		maxs[ 0 ] = v[ 0 ];
	}

	if ( v[ 1 ] < mins[ 1 ] )
	{
		mins[ 1 ] = v[ 1 ];
	}

	if ( v[ 1 ] > maxs[ 1 ] )
	{
		maxs[ 1 ] = v[ 1 ];
	}

	if ( v[ 2 ] < mins[ 2 ] )
	{
		mins[ 2 ] = v[ 2 ];
	}

	if ( v[ 2 ] > maxs[ 2 ] )
	{
		maxs[ 2 ] = v[ 2 ];
	}
}

qboolean PointInBounds( const vec3_t v, const vec3_t mins, const vec3_t maxs )
{
	if ( v[ 0 ] < mins[ 0 ] )
	{
		return qfalse;
	}

	if ( v[ 0 ] > maxs[ 0 ] )
	{
		return qfalse;
	}

	if ( v[ 1 ] < mins[ 1 ] )
	{
		return qfalse;
	}

	if ( v[ 1 ] > maxs[ 1 ] )
	{
		return qfalse;
	}

	if ( v[ 2 ] < mins[ 2 ] )
	{
		return qfalse;
	}

	if ( v[ 2 ] > maxs[ 2 ] )
	{
		return qfalse;
	}

	return qtrue;
}

void BoundsAdd( vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 )
{
	if ( mins2[ 0 ] < mins[ 0 ] )
	{
		mins[ 0 ] = mins2[ 0 ];
	}

	if ( mins2[ 1 ] < mins[ 1 ] )
	{
		mins[ 1 ] = mins2[ 1 ];
	}

	if ( mins2[ 2 ] < mins[ 2 ] )
	{
		mins[ 2 ] = mins2[ 2 ];
	}

	if ( maxs2[ 0 ] > maxs[ 0 ] )
	{
		maxs[ 0 ] = maxs2[ 0 ];
	}

	if ( maxs2[ 1 ] > maxs[ 1 ] )
	{
		maxs[ 1 ] = maxs2[ 1 ];
	}

	if ( maxs2[ 2 ] > maxs[ 2 ] )
	{
		maxs[ 2 ] = maxs2[ 2 ];
	}
}

qboolean BoundsIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 )
{
	if ( maxs[ 0 ] < mins2[ 0 ] ||
	     maxs[ 1 ] < mins2[ 1 ] || maxs[ 2 ] < mins2[ 2 ] || mins[ 0 ] > maxs2[ 0 ] || mins[ 1 ] > maxs2[ 1 ] || mins[ 2 ] > maxs2[ 2 ] )
	{
		return qfalse;
	}

	return qtrue;
}

qboolean BoundsIntersectSphere( const vec3_t mins, const vec3_t maxs, const vec3_t origin, vec_t radius )
{
	if ( origin[ 0 ] - radius > maxs[ 0 ] ||
	     origin[ 0 ] + radius < mins[ 0 ] ||
	     origin[ 1 ] - radius > maxs[ 1 ] ||
	     origin[ 1 ] + radius < mins[ 1 ] || origin[ 2 ] - radius > maxs[ 2 ] || origin[ 2 ] + radius < mins[ 2 ] )
	{
		return qfalse;
	}

	return qtrue;
}

qboolean BoundsIntersectPoint( const vec3_t mins, const vec3_t maxs, const vec3_t origin )
{
	if ( origin[ 0 ] > maxs[ 0 ] ||
	     origin[ 0 ] < mins[ 0 ] || origin[ 1 ] > maxs[ 1 ] || origin[ 1 ] < mins[ 1 ] || origin[ 2 ] > maxs[ 2 ] || origin[ 2 ] < mins[ 2 ] )
	{
		return qfalse;
	}

	return qtrue;
}

int VectorCompare( const vec3_t v1, const vec3_t v2 )
{
	if ( v1[ 0 ] != v2[ 0 ] || v1[ 1 ] != v2[ 1 ] || v1[ 2 ] != v2[ 2 ] )
	{
		return 0;
	}

	return 1;
}

vec_t VectorNormalize( vec3_t v )
{
	float length, ilength;

	length = v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] + v[ 2 ] * v[ 2 ];

	if ( length )
	{
		/* writing it this way allows gcc to recognize that rsqrt can be used */
	 	ilength = 1/(float)sqrt (length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		v[ 0 ] *= ilength;
		v[ 1 ] *= ilength;
		v[ 2 ] *= ilength;
	}

	return length;
}

//
// fast vector normalize routine that does not check to make sure
// that length != 0, nor does it return length
//
void VectorNormalizeFast( vec3_t v )
{
	float ilength;

	ilength = Q_rsqrt( DotProduct( v, v ) );

	v[ 0 ] *= ilength;
	v[ 1 ] *= ilength;
	v[ 2 ] *= ilength;
}

vec_t VectorNormalize2( const vec3_t v, vec3_t out )
{
	float length, ilength;

	length = v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] + v[ 2 ] * v[ 2 ];

	if ( length )
	{
		/* writing it this way allows gcc to recognize that rsqrt can be used */
	 	ilength = 1/(float)sqrt (length);
		/* sqrt(length) = length * (1 / sqrt(length)) */
		length *= ilength;
		out[ 0 ] = v[ 0 ] * ilength;
		out[ 1 ] = v[ 1 ] * ilength;
		out[ 2 ] = v[ 2 ] * ilength;
	}
	else
	{
		VectorClear( out );
	}

	return length;
}

void _VectorMA( const vec3_t veca, float scale, const vec3_t vecb, vec3_t vecc )
{
	vecc[ 0 ] = veca[ 0 ] + scale * vecb[ 0 ];
	vecc[ 1 ] = veca[ 1 ] + scale * vecb[ 1 ];
	vecc[ 2 ] = veca[ 2 ] + scale * vecb[ 2 ];
}

vec_t _DotProduct( const vec3_t v1, const vec3_t v2 )
{
	return v1[ 0 ] * v2[ 0 ] + v1[ 1 ] * v2[ 1 ] + v1[ 2 ] * v2[ 2 ];
}

void _VectorSubtract( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
	out[ 0 ] = veca[ 0 ] - vecb[ 0 ];
	out[ 1 ] = veca[ 1 ] - vecb[ 1 ];
	out[ 2 ] = veca[ 2 ] - vecb[ 2 ];
}

void _VectorAdd( const vec3_t veca, const vec3_t vecb, vec3_t out )
{
	out[ 0 ] = veca[ 0 ] + vecb[ 0 ];
	out[ 1 ] = veca[ 1 ] + vecb[ 1 ];
	out[ 2 ] = veca[ 2 ] + vecb[ 2 ];
}

void _VectorCopy( const vec3_t in, vec3_t out )
{
	out[ 0 ] = in[ 0 ];
	out[ 1 ] = in[ 1 ];
	out[ 2 ] = in[ 2 ];
}

void _VectorScale( const vec3_t in, vec_t scale, vec3_t out )
{
	out[ 0 ] = in[ 0 ] * scale;
	out[ 1 ] = in[ 1 ] * scale;
	out[ 2 ] = in[ 2 ] * scale;
}

void CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross )
{
	cross[ 0 ] = v1[ 1 ] * v2[ 2 ] - v1[ 2 ] * v2[ 1 ];
	cross[ 1 ] = v1[ 2 ] * v2[ 0 ] - v1[ 0 ] * v2[ 2 ];
	cross[ 2 ] = v1[ 0 ] * v2[ 1 ] - v1[ 1 ] * v2[ 0 ];
}

vec_t VectorLength( const vec3_t v )
{
	return sqrt( v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] + v[ 2 ] * v[ 2 ] );
}

vec_t VectorLengthSquared( const vec3_t v )
{
	return ( v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] + v[ 2 ] * v[ 2 ] );
}

vec_t Distance( const vec3_t p1, const vec3_t p2 )
{
	vec3_t v;

	VectorSubtract( p2, p1, v );
	return VectorLength( v );
}

vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 )
{
	vec3_t v;

	VectorSubtract( p2, p1, v );
	return v[ 0 ] * v[ 0 ] + v[ 1 ] * v[ 1 ] + v[ 2 ] * v[ 2 ];
}

void VectorInverse( vec3_t v )
{
	v[ 0 ] = -v[ 0 ];
	v[ 1 ] = -v[ 1 ];
	v[ 2 ] = -v[ 2 ];
}

void Vector4Scale( const vec4_t in, vec_t scale, vec4_t out )
{
	out[ 0 ] = in[ 0 ] * scale;
	out[ 1 ] = in[ 1 ] * scale;
	out[ 2 ] = in[ 2 ] * scale;
	out[ 3 ] = in[ 3 ] * scale;
}

int NearestPowerOfTwo( int val )
{
	int answer;

	for ( answer = 1; answer < val; answer <<= 1 )
	{
		;
	}

	return answer;
}

int Q_log2( int val )
{
	int answer;

	answer = 0;

	while ( ( val >>= 1 ) != 0 )
	{
		answer++;
	}

	return answer;
}

/*
=================
PlaneTypeForNormal
=================
*/

/*
int PlaneTypeForNormal (vec3_t normal) {
        if ( normal[0] == 1.0 )
                return PLANE_X;
        if ( normal[1] == 1.0 )
                return PLANE_Y;
        if ( normal[2] == 1.0 )
                return PLANE_Z;

        return PLANE_NON_AXIAL;
}
*/

/*
================
AxisMultiply
================
*/
void AxisMultiply( float in1[ 3 ][ 3 ], float in2[ 3 ][ 3 ], float out[ 3 ][ 3 ] )
{
	out[ 0 ][ 0 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 0 ] + in1[ 0 ][ 2 ] * in2[ 2 ][ 0 ];
	out[ 0 ][ 1 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 1 ] + in1[ 0 ][ 2 ] * in2[ 2 ][ 1 ];
	out[ 0 ][ 2 ] = in1[ 0 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 0 ][ 1 ] * in2[ 1 ][ 2 ] + in1[ 0 ][ 2 ] * in2[ 2 ][ 2 ];
	out[ 1 ][ 0 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 0 ] + in1[ 1 ][ 2 ] * in2[ 2 ][ 0 ];
	out[ 1 ][ 1 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 1 ] + in1[ 1 ][ 2 ] * in2[ 2 ][ 1 ];
	out[ 1 ][ 2 ] = in1[ 1 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 1 ][ 1 ] * in2[ 1 ][ 2 ] + in1[ 1 ][ 2 ] * in2[ 2 ][ 2 ];
	out[ 2 ][ 0 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 0 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 0 ] + in1[ 2 ][ 2 ] * in2[ 2 ][ 0 ];
	out[ 2 ][ 1 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 1 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 1 ] + in1[ 2 ][ 2 ] * in2[ 2 ][ 1 ];
	out[ 2 ][ 2 ] = in1[ 2 ][ 0 ] * in2[ 0 ][ 2 ] + in1[ 2 ][ 1 ] * in2[ 1 ][ 2 ] + in1[ 2 ][ 2 ] * in2[ 2 ][ 2 ];
}

void AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up )
{
	float        angle;
	static float sr, sp, sy, cr, cp, cy;

	// static to help MS compiler fp bugs

	angle = angles[ YAW ] * ( M_PI * 2 / 360 );
	sy = sin( angle );
	cy = cos( angle );

	angle = angles[ PITCH ] * ( M_PI * 2 / 360 );
	sp = sin( angle );
	cp = cos( angle );

	angle = angles[ ROLL ] * ( M_PI * 2 / 360 );
	sr = sin( angle );
	cr = cos( angle );

	if ( forward )
	{
		forward[ 0 ] = cp * cy;
		forward[ 1 ] = cp * sy;
		forward[ 2 ] = -sp;
	}

	if ( right )
	{
		right[ 0 ] = ( -1 * sr * sp * cy + -1 * cr * -sy );
		right[ 1 ] = ( -1 * sr * sp * sy + -1 * cr * cy );
		right[ 2 ] = -1 * sr * cp;
	}

	if ( up )
	{
		up[ 0 ] = ( cr * sp * cy + -sr * -sy );
		up[ 1 ] = ( cr * sp * sy + -sr * cy );
		up[ 2 ] = cr * cp;
	}
}

/*
=================
PerpendicularVector

assumes "src" is normalized
=================
*/
void PerpendicularVector( vec3_t dst, const vec3_t src )
{
	int    pos;
	int    i;
	float  minelem = 1.0F;
	vec3_t tempvec;

	/*
	 ** find the smallest magnitude axially aligned vector
	 */
	for ( pos = 0, i = 0; i < 3; i++ )
	{
		if ( Q_fabs( src[ i ] ) < minelem )
		{
			pos = i;
			minelem = Q_fabs( src[ i ] );
		}
	}

	tempvec[ 0 ] = tempvec[ 1 ] = tempvec[ 2 ] = 0.0F;
	tempvec[ pos ] = 1.0F;

	/*
	 ** project the point onto the plane defined by src
	 */
	ProjectPointOnPlane( dst, tempvec, src );

	/*
	 ** normalize the result
	 */
	VectorNormalize( dst );
}

// Ridah

/*
=================
GetPerpendicularViewVector

  Used to find an "up" vector for drawing a sprite so that it always faces the view as best as possible
=================
*/
void GetPerpendicularViewVector( const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up )
{
	vec3_t v1, v2;

	VectorSubtract( point, p1, v1 );
	VectorNormalize( v1 );

	VectorSubtract( point, p2, v2 );
	VectorNormalize( v2 );

	CrossProduct( v1, v2, up );
	VectorNormalize( up );
}

/*
================
ProjectPointOntoVector
================
*/
void ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj )
{
	vec3_t pVec, vec;

	VectorSubtract( point, vStart, pVec );
	VectorSubtract( vEnd, vStart, vec );
	VectorNormalize( vec );
	// project onto the directional vector for this segment
	VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );
}

#define LINE_DISTANCE_EPSILON 1e-05f

/*
================
DistanceBetweenLineSegmentsSquared
Return the smallest distance between two line segments, squared
================
*/

vec_t DistanceBetweenLineSegmentsSquared( const vec3_t sP0, const vec3_t sP1,
    const vec3_t tP0, const vec3_t tP1, float *s, float *t )
{
	vec3_t sMag, tMag, diff;
	float  a, b, c, d, e;
	float  D;
	float  sN, sD;
	float  tN, tD;
	vec3_t separation;

	VectorSubtract( sP1, sP0, sMag );
	VectorSubtract( tP1, tP0, tMag );
	VectorSubtract( sP0, tP0, diff );
	a = DotProduct( sMag, sMag );
	b = DotProduct( sMag, tMag );
	c = DotProduct( tMag, tMag );
	d = DotProduct( sMag, diff );
	e = DotProduct( tMag, diff );
	sD = tD = D = a * c - b * b;

	if ( D < LINE_DISTANCE_EPSILON )
	{
		// the lines are almost parallel
		sN = 0.0; // force using point P0 on segment S1
		sD = 1.0; // to prevent possible division by 0.0 later
		tN = e;
		tD = c;
	}
	else
	{
		// get the closest points on the infinite  lines
		sN = ( b * e - c * d );
		tN = ( a * e - b * d );

		if ( sN < 0.0 )
		{
			// sN < 0 => the s=0 edge is visible
			sN = 0.0;
			tN = e;
			tD = c;
		}
		else if ( sN > sD )
		{
			// sN > sD => the s=1 edge is visible
			sN = sD;
			tN = e + b;
			tD = c;
		}
	}

	if ( tN < 0.0 )
	{
		// tN < 0 => the t=0 edge is visible
		tN = 0.0;

		// recompute sN for this edge
		if ( -d < 0.0 )
		{
			sN = 0.0;
		}
		else if ( -d > a )
		{
			sN = sD;
		}
		else
		{
			sN = -d;
			sD = a;
		}
	}
	else if ( tN > tD )
	{
		// tN > tD => the t=1 edge is visible
		tN = tD;

		// recompute sN for this edge
		if ( ( -d + b ) < 0.0 )
		{
			sN = 0;
		}
		else if ( ( -d + b ) > a )
		{
			sN = sD;
		}
		else
		{
			sN = ( -d + b );
			sD = a;
		}
	}

	// finally do the division to get *s and *t
	*s = ( fabs( sN ) < LINE_DISTANCE_EPSILON ? 0.0 : sN / sD );
	*t = ( fabs( tN ) < LINE_DISTANCE_EPSILON ? 0.0 : tN / tD );

	// get the difference of the two closest points
	VectorScale( sMag, *s, sMag );
	VectorScale( tMag, *t, tMag );
	VectorAdd( diff, sMag, separation );
	VectorSubtract( separation, tMag, separation );

	return VectorLengthSquared( separation );
}

/*
================
DistanceBetweenLineSegments

Return the smallest distance between two line segments
================
*/

vec_t DistanceBetweenLineSegments( const vec3_t sP0, const vec3_t sP1, const vec3_t tP0, const vec3_t tP1, float *s, float *t )
{
	return ( vec_t ) sqrt( DistanceBetweenLineSegmentsSquared( sP0, sP1, tP0, tP1, s, t ) );
}

/*
================
ProjectPointOntoVectorBounded
================
*/
void ProjectPointOntoVectorBounded( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj )
{
	vec3_t pVec, vec;
	int    j;

	VectorSubtract( point, vStart, pVec );
	VectorSubtract( vEnd, vStart, vec );
	VectorNormalize( vec );
	// project onto the directional vector for this segment
	VectorMA( vStart, DotProduct( pVec, vec ), vec, vProj );

	// check bounds
	for ( j = 0; j < 3; j++ )
	{
		if ( ( vProj[ j ] > vStart[ j ] && vProj[ j ] > vEnd[ j ] ) || ( vProj[ j ] < vStart[ j ] && vProj[ j ] < vEnd[ j ] ) )
		{
			break;
		}
	}

	if ( j < 3 )
	{
		if ( Q_fabs( vProj[ j ] - vStart[ j ] ) < Q_fabs( vProj[ j ] - vEnd[ j ] ) )
		{
			VectorCopy( vStart, vProj );
		}
		else
		{
			VectorCopy( vEnd, vProj );
		}
	}
}

/*
================
DistanceFromLineSquared
================
*/
float DistanceFromLineSquared( vec3_t p, vec3_t lp1, vec3_t lp2 )
{
	vec3_t proj, t;
	int    j;

	ProjectPointOntoVector( p, lp1, lp2, proj );

	for ( j = 0; j < 3; j++ )
	{
		if ( ( proj[ j ] > lp1[ j ] && proj[ j ] > lp2[ j ] ) || ( proj[ j ] < lp1[ j ] && proj[ j ] < lp2[ j ] ) )
		{
			break;
		}
	}

	if ( j < 3 )
	{
		if ( Q_fabs( proj[ j ] - lp1[ j ] ) < Q_fabs( proj[ j ] - lp2[ j ] ) )
		{
			VectorSubtract( p, lp1, t );
		}
		else
		{
			VectorSubtract( p, lp2, t );
		}

		return VectorLengthSquared( t );
	}

	VectorSubtract( p, proj, t );
	return VectorLengthSquared( t );
}

/*
================
DistanceFromVectorSquared
================
*/
float DistanceFromVectorSquared( vec3_t p, vec3_t lp1, vec3_t lp2 )
{
	vec3_t proj, t;

	ProjectPointOntoVector( p, lp1, lp2, proj );
	VectorSubtract( p, proj, t );
	return VectorLengthSquared( t );
}

float vectoyaw( const vec3_t vec )
{
	float yaw;

	if ( vec[ YAW ] == 0 && vec[ PITCH ] == 0 )
	{
		yaw = 0;
	}
	else
	{
		if ( vec[ PITCH ] )
		{
			yaw = ( atan2( vec[ YAW ], vec[ PITCH ] ) * 180 / M_PI );
		}
		else if ( vec[ YAW ] > 0 )
		{
			yaw = 90;
		}
		else
		{
			yaw = 270;
		}

		if ( yaw < 0 )
		{
			yaw += 360;
		}
	}

	return yaw;
}

/*
=================
AxisToAngles

  Used to convert the MD3 tag axis to MDC tag angles, which are much smaller

  This doesn't have to be fast, since it's only used for conversion in utils, try to avoid
  using this during gameplay
=================
*/
void AxisToAngles( /*const*/ vec3_t axis[ 3 ], vec3_t angles )
{
	float length1;
	float yaw, pitch, roll = 0.0f;

	if ( axis[ 0 ][ 1 ] == 0 && axis[ 0 ][ 0 ] == 0 )
	{
		yaw = 0;

		if ( axis[ 0 ][ 2 ] > 0 )
		{
			pitch = 90;
		}
		else
		{
			pitch = 270;
		}
	}
	else
	{
		if ( axis[ 0 ][ 0 ] )
		{
			yaw = ( atan2( axis[ 0 ][ 1 ], axis[ 0 ][ 0 ] ) * 180 / M_PI );
		}
		else if ( axis[ 0 ][ 1 ] > 0 )
		{
			yaw = 90;
		}
		else
		{
			yaw = 270;
		}

		if ( yaw < 0 )
		{
			yaw += 360;
		}

		length1 = sqrt( axis[ 0 ][ 0 ] * axis[ 0 ][ 0 ] + axis[ 0 ][ 1 ] * axis[ 0 ][ 1 ] );
		pitch = ( atan2( axis[ 0 ][ 2 ], length1 ) * 180 / M_PI );

		if ( pitch < 0 )
		{
			pitch += 360;
		}

		roll = ( atan2( axis[ 1 ][ 2 ], axis[ 2 ][ 2 ] ) * 180 / M_PI );

		if ( roll < 0 )
		{
			roll += 360;
		}
	}

	angles[ PITCH ] = -pitch;
	angles[ YAW ] = yaw;
	angles[ ROLL ] = roll;
}

float VectorDistance( vec3_t v1, vec3_t v2 )
{
	vec3_t dir;

	VectorSubtract( v2, v1, dir );
	return VectorLength( dir );
}

float VectorDistanceSquared( vec3_t v1, vec3_t v2 )
{
	vec3_t dir;

	VectorSubtract( v2, v1, dir );
	return VectorLengthSquared( dir );
}

// done.

/*
================
VectorMaxComponent

Return the biggest component of some vector
================
*/
float VectorMaxComponent( vec3_t v )
{
	float biggest = v[ 0 ];

	if ( v[ 1 ] > biggest )
	{
		biggest = v[ 1 ];
	}

	if ( v[ 2 ] > biggest )
	{
		biggest = v[ 2 ];
	}

	return biggest;
}

/*
================
VectorMinComponent

Return the smallest component of some vector
================
*/
float VectorMinComponent( vec3_t v )
{
	float smallest = v[ 0 ];

	if ( v[ 1 ] < smallest )
	{
		smallest = v[ 1 ];
	}

	if ( v[ 2 ] < smallest )
	{
		smallest = v[ 2 ];
	}

	return smallest;
}

//=============================================

// RB: XreaL matrix math functions

// *INDENT-OFF*
void MatrixIdentity( matrix_t m )
{
	m[ 0 ] = 1;
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = 1;
	m[ 9 ] = 0;
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = 1;
	m[ 14 ] = 0;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixClear( matrix_t m )
{
	m[ 0 ] = 0;
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = 0;
	m[ 9 ] = 0;
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = 0;
	m[ 14 ] = 0;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 0;
}

void MatrixCopy( const matrix_t in, matrix_t out )
{
#if id386_sse && defined __GNUC__ && 0
	asm volatile
	(
	  "movups         (%%edx),        %%xmm0\n"
	  "movups         0x10(%%edx),    %%xmm1\n"
	  "movups         0x20(%%edx),    %%xmm2\n"
	  "movups         0x30(%%edx),    %%xmm3\n"

	  "movups         %%xmm0,         (%%eax)\n"
	  "movups         %%xmm1,         0x10(%%eax)\n"
	  "movups         %%xmm2,         0x20(%%eax)\n"
	  "movups         %%xmm3,         0x30(%%eax)\n"
	  :
	  : "a"( out ), "d"( in )
	  : "memory"
	);
#elif id386_3dnow && defined __GNUC__
	asm volatile
	(
	  "femms\n"
	  "movq           (%%edx),        %%mm0\n"
	  "movq           8(%%edx),       %%mm1\n"
	  "movq           16(%%edx),      %%mm2\n"
	  "movq           24(%%edx),      %%mm3\n"
	  "movq           32(%%edx),      %%mm4\n"
	  "movq           40(%%edx),      %%mm5\n"
	  "movq           48(%%edx),      %%mm6\n"
	  "movq           56(%%edx),      %%mm7\n"

	  "movq           %%mm0,          (%%eax)\n"
	  "movq           %%mm1,          8(%%eax)\n"
	  "movq           %%mm2,          16(%%eax)\n"
	  "movq           %%mm3,          24(%%eax)\n"
	  "movq           %%mm4,          32(%%eax)\n"
	  "movq           %%mm5,          40(%%eax)\n"
	  "movq           %%mm6,          48(%%eax)\n"
	  "movq           %%mm7,          56(%%eax)\n"
	  "femms\n"
	  :
	  : "a"( out ), "d"( in )
	  : "memory"
	);
#else
	out[ 0 ] = in[ 0 ];
	out[ 4 ] = in[ 4 ];
	out[ 8 ] = in[ 8 ];
	out[ 12 ] = in[ 12 ];
	out[ 1 ] = in[ 1 ];
	out[ 5 ] = in[ 5 ];
	out[ 9 ] = in[ 9 ];
	out[ 13 ] = in[ 13 ];
	out[ 2 ] = in[ 2 ];
	out[ 6 ] = in[ 6 ];
	out[ 10 ] = in[ 10 ];
	out[ 14 ] = in[ 14 ];
	out[ 3 ] = in[ 3 ];
	out[ 7 ] = in[ 7 ];
	out[ 11 ] = in[ 11 ];
	out[ 15 ] = in[ 15 ];
#endif
}

qboolean MatrixCompare( const matrix_t a, const matrix_t b )
{
	return ( a[ 0 ] == b[ 0 ] && a[ 4 ] == b[ 4 ] && a[ 8 ] == b[ 8 ] && a[ 12 ] == b[ 12 ] &&
	         a[ 1 ] == b[ 1 ] && a[ 5 ] == b[ 5 ] && a[ 9 ] == b[ 9 ] && a[ 13 ] == b[ 13 ] &&
	         a[ 2 ] == b[ 2 ] && a[ 6 ] == b[ 6 ] && a[ 10 ] == b[ 10 ] && a[ 14 ] == b[ 14 ] &&
	         a[ 3 ] == b[ 3 ] && a[ 7 ] == b[ 7 ] && a[ 11 ] == b[ 11 ] && a[ 15 ] == b[ 15 ] );
}

void MatrixTransposeIntoXMM( const matrix_t m )
{
#if id386_sse && defined __GNUC__ && 0
	asm volatile
	( // reg[0]                       | reg[1]                | reg[2]                | reg[3]
	  // load transpose into XMM registers
	  "movlps         (%%eax),        %%xmm4\n" // m[0][0]                      | m[0][1]               | -                     | -
	  "movhps         16(%%eax),      %%xmm4\n" // m[0][0]                      | m[0][1]               | m[1][0]               | m[1][1]

	  "movlps         32(%%eax),      %%xmm3\n" // m[2][0]                      | m[2][1]               | -                     | -
	  "movhps         48(%%eax),      %%xmm3\n" // m[2][0]                      | m[2][1]               | m[3][0]               | m[3][1]

	  "movups         %%xmm4,         %%xmm5\n" // m[0][0]                      | m[0][1]               | m[1][0]               | m[1][1]

	  // 0x88 = 10 00 | 10 00 <-> 00 10 | 00 10          xmm4[00]                       xmm4[10]                xmm3[00]                xmm3[10]
	  "shufps         $0x88, %%xmm3,  %%xmm4\n" // m[0][0]                      | m[1][0]               | m[2][0]               | m[3][0]

	  // 0xDD = 11 01 | 11 01 <-> 01 11 | 01 11          xmm5[01]                       xmm5[11]                xmm3[01]                xmm3[11]
	  "shufps         $0xDD, %%xmm3,  %%xmm5\n" // m[0][1]                      | m[1][1]               | m[2][1]               | m[3][1]

	  "movlps         8(%%eax),       %%xmm6\n" // m[0][2]                      | m[0][3]               | -                     | -
	  "movhps         24(%%eax),      %%xmm6\n" // m[0][2]                      | m[0][3]               | m[1][2]               | m[1][3]

	  "movlps         40(%%eax),      %%xmm3\n" // m[2][2]                      | m[2][3]               | -                     | -
	  "movhps         56(%%eax),      %%xmm3\n" // m[2][2]                      | m[2][3]               | m[3][2]               | m[3][3]

	  "movups         %%xmm6,         %%xmm7\n" // m[0][2]                      | m[0][3]               | m[1][2]               | m[1][3]

	  // 0x88 = 10 00 | 10 00 <-> 00 10 | 00 10          xmm6[00]                       xmm6[10]                xmm3[00]                xmm3[10]
	  "shufps         $0x88, %%xmm3,  %%xmm6\n" // m[0][2]                      | m[1][2]               | m[2][2]               | m[3][2]

	  // 0xDD = 11 01 | 11 01 <-> 01 11 | 01 11          xmm7[01]                       xmm7[11]                xmm3[01]                xmm3[11]
	  "shufps         $0xDD, %%xmm3,  %%xmm7\n" // m[0][3]                      | m[1][3]               | m[2][3]               | m[3][3]
	  :
	  : "a"( m )
	  : "memory"
	);
#endif
}

void MatrixTranspose( const matrix_t in, matrix_t out )
{
#if id386_sse && defined __GNUC__ && 0
	// transpose the matrix into the xmm4-7
	MatrixTransposeIntoXMM( in );

	asm volatile
	(
	  "movups         %%xmm4,         (%%eax)\n"
	  "movups         %%xmm5,         0x10(%%eax)\n"
	  "movups         %%xmm6,         0x20(%%eax)\n"
	  "movups         %%xmm7,         0x30(%%eax)\n"
	  :
	  : "a"( out )
	  : "memory"
	);
#else
	out[ 0 ] = in[ 0 ];
	out[ 1 ] = in[ 4 ];
	out[ 2 ] = in[ 8 ];
	out[ 3 ] = in[ 12 ];
	out[ 4 ] = in[ 1 ];
	out[ 5 ] = in[ 5 ];
	out[ 6 ] = in[ 9 ];
	out[ 7 ] = in[ 13 ];
	out[ 8 ] = in[ 2 ];
	out[ 9 ] = in[ 6 ];
	out[ 10 ] = in[ 10 ];
	out[ 11 ] = in[ 14 ];
	out[ 12 ] = in[ 3 ];
	out[ 13 ] = in[ 7 ];
	out[ 14 ] = in[ 11 ];
	out[ 15 ] = in[ 15 ];
#endif
}

// helper functions for MatrixInverse from GtkRadiant C mathlib
static float m3_det( matrix3x3_t mat )
{
	float det;

	det = mat[ 0 ] * ( mat[ 4 ] * mat[ 8 ] - mat[ 7 ] * mat[ 5 ] )
	      - mat[ 1 ] * ( mat[ 3 ] * mat[ 8 ] - mat[ 6 ] * mat[ 5 ] )
	      + mat[ 2 ] * ( mat[ 3 ] * mat[ 7 ] - mat[ 6 ] * mat[ 4 ] );

	return ( det );
}

/*static int m3_inverse( matrix3x3_t mr, matrix3x3_t ma )
{
  float det = m3_det( ma );

  if (det == 0 )
  {
    return 1;
  }


  mr[0] =    ma[4]*ma[8] - ma[5]*ma[7]   / det;
  mr[1] = -( ma[1]*ma[8] - ma[7]*ma[2] ) / det;
  mr[2] =    ma[1]*ma[5] - ma[4]*ma[2]   / det;

  mr[3] = -( ma[3]*ma[8] - ma[5]*ma[6] ) / det;
  mr[4] =    ma[0]*ma[8] - ma[6]*ma[2]   / det;
  mr[5] = -( ma[0]*ma[5] - ma[3]*ma[2] ) / det;

  mr[6] =    ma[3]*ma[7] - ma[6]*ma[4]   / det;
  mr[7] = -( ma[0]*ma[7] - ma[6]*ma[1] ) / det;
  mr[8] =    ma[0]*ma[4] - ma[1]*ma[3]   / det;

  return 0;
}*/

static void m4_submat( matrix_t mr, matrix3x3_t mb, int i, int j )
{
	int ti, tj, idst = 0, jdst = 0;

	for ( ti = 0; ti < 4; ti++ )
	{
		if ( ti < i )
		{
			idst = ti;
		}
		else if ( ti > i )
		{
			idst = ti - 1;
		}

		for ( tj = 0; tj < 4; tj++ )
		{
			if ( tj < j )
			{
				jdst = tj;
			}
			else if ( tj > j )
			{
				jdst = tj - 1;
			}

			if ( ti != i && tj != j )
			{
				mb[ idst * 3 + jdst ] = mr[ ti * 4 + tj ];
			}
		}
	}
}

static float m4_det( matrix_t mr )
{
	float       det, result = 0, i = 1;
	matrix3x3_t msub3;
	int         n;

	for ( n = 0; n < 4; n++, i *= -1 )
	{
		m4_submat( mr, msub3, 0, n );

		det = m3_det( msub3 );
		result += mr[ n ] * det * i;
	}

	return result;
}

qboolean MatrixInverse( matrix_t matrix )
{
	float       mdet = m4_det( matrix );
	matrix3x3_t mtemp;
	int         i, j, sign;
	matrix_t    m4x4_temp;

#if 0

	if ( fabs( mdet ) < 0.0000000001 )
	{
		return qtrue;
	}

#endif

	MatrixCopy( matrix, m4x4_temp );

	for ( i = 0; i < 4; i++ )
	{
		for ( j = 0; j < 4; j++ )
		{
			sign = 1 - ( ( i + j ) % 2 ) * 2;

			m4_submat( m4x4_temp, mtemp, i, j );

			// FIXME: try using * inverse det and see if speed/accuracy are good enough
			matrix[ i + j * 4 ] = ( m3_det( mtemp ) * sign ) / mdet;
		}
	}

	return qfalse;
}

void MatrixSetupXRotation( matrix_t m, vec_t degrees )
{
	vec_t a = DEG2RAD( degrees );

	m[ 0 ] = 1;
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = cos( a );
	m[ 9 ] = -sin( a );
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = sin( a );
	m[ 10 ] = cos( a );
	m[ 14 ] = 0;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixSetupYRotation( matrix_t m, vec_t degrees )
{
	vec_t a = DEG2RAD( degrees );

	m[ 0 ] = cos( a );
	m[ 4 ] = 0;
	m[ 8 ] = sin( a );
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = 1;
	m[ 9 ] = 0;
	m[ 13 ] = 0;
	m[ 2 ] = -sin( a );
	m[ 6 ] = 0;
	m[ 10 ] = cos( a );
	m[ 14 ] = 0;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixSetupZRotation( matrix_t m, vec_t degrees )
{
	vec_t a = DEG2RAD( degrees );

	m[ 0 ] = cos( a );
	m[ 4 ] = -sin( a );
	m[ 8 ] = 0;
	m[ 12 ] = 0;
	m[ 1 ] = sin( a );
	m[ 5 ] = cos( a );
	m[ 9 ] = 0;
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = 1;
	m[ 14 ] = 0;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixSetupTranslation( matrix_t m, vec_t x, vec_t y, vec_t z )
{
	m[ 0 ] = 1;
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = x;
	m[ 1 ] = 0;
	m[ 5 ] = 1;
	m[ 9 ] = 0;
	m[ 13 ] = y;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = 1;
	m[ 14 ] = z;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixSetupScale( matrix_t m, vec_t x, vec_t y, vec_t z )
{
	m[ 0 ] = x;
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = y;
	m[ 9 ] = 0;
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = z;
	m[ 14 ] = 0;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixSetupShear( matrix_t m, vec_t x, vec_t y )
{
	m[ 0 ] = 1;
	m[ 4 ] = x;
	m[ 8 ] = 0;
	m[ 12 ] = 0;
	m[ 1 ] = y;
	m[ 5 ] = 1;
	m[ 9 ] = 0;
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = 1;
	m[ 14 ] = 0;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixMultiply( const matrix_t a, const matrix_t b, matrix_t out )
{
#if id386_sse
//#error MatrixMultiply
	int    i;
	__m128 _t0, _t1, _t2, _t3, _t4, _t5, _t6, _t7;

	_t4 = _mm_loadu_ps( &a[ 0 ] );
	_t5 = _mm_loadu_ps( &a[ 4 ] );
	_t6 = _mm_loadu_ps( &a[ 8 ] );
	_t7 = _mm_loadu_ps( &a[ 12 ] );

	for ( i = 0; i < 4; i++ )
	{
		_t0 = _mm_load1_ps( &b[ i * 4 + 0 ] );
		_t0 = _mm_mul_ps( _t4, _t0 );

		_t1 = _mm_load1_ps( &b[ i * 4 + 1 ] );
		_t1 = _mm_mul_ps( _t5, _t1 );

		_t2 = _mm_load1_ps( &b[ i * 4 + 2 ] );
		_t2 = _mm_mul_ps( _t6, _t2 );

		_t3 = _mm_load1_ps( &b[ i * 4 + 3 ] );
		_t3 = _mm_mul_ps( _t7, _t3 );

		_t1 = _mm_add_ps( _t0, _t1 );
		_t2 = _mm_add_ps( _t1, _t2 );
		_t3 = _mm_add_ps( _t2, _t3 );

		_mm_storeu_ps( &out[ i * 4 ], _t3 );
	}

#else
	out[ 0 ] = b[ 0 ] * a[ 0 ] + b[ 1 ] * a[ 4 ] + b[ 2 ] * a[ 8 ] + b[ 3 ] * a[ 12 ];
	out[ 1 ] = b[ 0 ] * a[ 1 ] + b[ 1 ] * a[ 5 ] + b[ 2 ] * a[ 9 ] + b[ 3 ] * a[ 13 ];
	out[ 2 ] = b[ 0 ] * a[ 2 ] + b[ 1 ] * a[ 6 ] + b[ 2 ] * a[ 10 ] + b[ 3 ] * a[ 14 ];
	out[ 3 ] = b[ 0 ] * a[ 3 ] + b[ 1 ] * a[ 7 ] + b[ 2 ] * a[ 11 ] + b[ 3 ] * a[ 15 ];

	out[ 4 ] = b[ 4 ] * a[ 0 ] + b[ 5 ] * a[ 4 ] + b[ 6 ] * a[ 8 ] + b[ 7 ] * a[ 12 ];
	out[ 5 ] = b[ 4 ] * a[ 1 ] + b[ 5 ] * a[ 5 ] + b[ 6 ] * a[ 9 ] + b[ 7 ] * a[ 13 ];
	out[ 6 ] = b[ 4 ] * a[ 2 ] + b[ 5 ] * a[ 6 ] + b[ 6 ] * a[ 10 ] + b[ 7 ] * a[ 14 ];
	out[ 7 ] = b[ 4 ] * a[ 3 ] + b[ 5 ] * a[ 7 ] + b[ 6 ] * a[ 11 ] + b[ 7 ] * a[ 15 ];

	out[ 8 ] = b[ 8 ] * a[ 0 ] + b[ 9 ] * a[ 4 ] + b[ 10 ] * a[ 8 ] + b[ 11 ] * a[ 12 ];
	out[ 9 ] = b[ 8 ] * a[ 1 ] + b[ 9 ] * a[ 5 ] + b[ 10 ] * a[ 9 ] + b[ 11 ] * a[ 13 ];
	out[ 10 ] = b[ 8 ] * a[ 2 ] + b[ 9 ] * a[ 6 ] + b[ 10 ] * a[ 10 ] + b[ 11 ] * a[ 14 ];
	out[ 11 ] = b[ 8 ] * a[ 3 ] + b[ 9 ] * a[ 7 ] + b[ 10 ] * a[ 11 ] + b[ 11 ] * a[ 15 ];

	out[ 12 ] = b[ 12 ] * a[ 0 ] + b[ 13 ] * a[ 4 ] + b[ 14 ] * a[ 8 ] + b[ 15 ] * a[ 12 ];
	out[ 13 ] = b[ 12 ] * a[ 1 ] + b[ 13 ] * a[ 5 ] + b[ 14 ] * a[ 9 ] + b[ 15 ] * a[ 13 ];
	out[ 14 ] = b[ 12 ] * a[ 2 ] + b[ 13 ] * a[ 6 ] + b[ 14 ] * a[ 10 ] + b[ 15 ] * a[ 14 ];
	out[ 15 ] = b[ 12 ] * a[ 3 ] + b[ 13 ] * a[ 7 ] + b[ 14 ] * a[ 11 ] + b[ 15 ] * a[ 15 ];
#endif
}

void MatrixMultiply2( matrix_t m, const matrix_t m2 )
{
	matrix_t tmp;

	MatrixCopy( m, tmp );
	MatrixMultiply( tmp, m2, m );
}

void MatrixMultiplyRotation( matrix_t m, vec_t pitch, vec_t yaw, vec_t roll )
{
	matrix_t tmp, rot;

	MatrixCopy( m, tmp );
	MatrixFromAngles( rot, pitch, yaw, roll );

	MatrixMultiply( tmp, rot, m );
}

void MatrixMultiplyZRotation( matrix_t m, vec_t degrees )
{
	matrix_t tmp;
	float angle = DEG2RAD( degrees );
	float s = sin( angle );
	float c = cos( angle );

	MatrixCopy( m, tmp );

	m[ 0 ] = tmp[ 0 ] * c + tmp[ 4 ] * s;
	m[ 1 ] = tmp[ 1 ] * c + tmp[ 5 ] * s;
	m[ 2 ] = tmp[ 2 ] * c + tmp[ 6 ] * s;
	m[ 3 ] = tmp[ 3 ] * c + tmp[ 7 ] * s;

	m[ 4 ] = tmp[ 0 ] * -s + tmp[ 4 ] * c;
	m[ 5 ] = tmp[ 1 ] * -s + tmp[ 5 ] * c;
	m[ 6 ] = tmp[ 2 ] * -s + tmp[ 6 ] * c;
	m[ 7 ] = tmp[ 3 ] * -s + tmp[ 7 ] * c;
}

void MatrixMultiplyTranslation( matrix_t m, vec_t x, vec_t y, vec_t z )
{
	m[ 12 ] += m[ 0 ] * x + m[ 4 ] * y + m[ 8 ] * z;
	m[ 13 ] += m[ 1 ] * x + m[ 5 ] * y + m[ 9 ] * z;
	m[ 14 ] += m[ 2 ] * x + m[ 6 ] * y + m[ 10 ] * z;
	m[ 15 ] += m[ 3 ] * x + m[ 7 ] * y + m[ 11 ] * z;
}

void MatrixMultiplyScale( matrix_t m, vec_t x, vec_t y, vec_t z )
{
	m[ 0 ] *= x;
	m[ 4 ] *= y;
	m[ 8 ] *= z;
	m[ 1 ] *= x;
	m[ 5 ] *= y;
	m[ 9 ] *= z;
	m[ 2 ] *= x;
	m[ 6 ] *= y;
	m[ 10 ] *= z;
	m[ 3 ] *= x;
	m[ 7 ] *= y;
	m[ 11 ] *= z;
}

void MatrixMultiplyShear( matrix_t m, vec_t x, vec_t y )
{
	matrix_t tmp;

	MatrixCopy( m, tmp );

	m[ 0 ] += m[ 4 ] * y;
	m[ 1 ] += m[ 5 ] * y;
	m[ 2 ] += m[ 6 ] * y;
	m[ 3 ] += m[ 7 ] * y;

	m[ 4 ] += tmp[ 0 ] * x;
	m[ 5 ] += tmp[ 1 ] * x;
	m[ 6 ] += tmp[ 2 ] * x;
	m[ 7 ] += tmp[ 3 ] * x;
}

void MatrixToAngles( const matrix_t m, vec3_t angles )
{
#if 1
	float theta;
	float cp;
	float sp;

	sp = m[ 2 ];

	// cap off our sin value so that we don't get any NANs
	if ( sp > 1.0 )
	{
		sp = 1.0;
	}
	else if ( sp < -1.0 )
	{
		sp = -1.0;
	}

	theta = -asin( sp );
	cp = cos( theta );

	if ( cp > 8192 * FLT_EPSILON )
	{
		angles[ PITCH ] = RAD2DEG( theta );
		angles[ YAW ] = RAD2DEG( atan2( m[ 1 ], m[ 0 ] ) );
		angles[ ROLL ] = RAD2DEG( atan2( m[ 6 ], m[ 10 ] ) );
	}
	else
	{
		angles[ PITCH ] = RAD2DEG( theta );
		angles[ YAW ] = RAD2DEG( -atan2( m[ 4 ], m[ 5 ] ) );
		angles[ ROLL ] = 0;
	}

#else
	float a;
	float ca;

	a = asin( -m[ 2 ] );
	ca = cos( a );

	if ( fabs( ca ) > 0.005 )  // Gimbal lock?
	{
		angles[ PITCH ] = RAD2DEG( atan2( m[ 6 ] / ca, m[ 10 ] / ca ) );
		angles[ YAW ] = RAD2DEG( a );
		angles[ ROLL ] = RAD2DEG( atan2( m[ 1 ] / ca, m[ 0 ] / ca ) );
	}
	else
	{
		// Gimbal lock has occurred
		angles[ PITCH ] = RAD2DEG( atan2( -m[ 9 ], m[ 5 ] ) );
		angles[ YAW ] = RAD2DEG( a );
		angles[ ROLL ] = 0;
	}

#endif
}

void MatrixFromAngles( matrix_t m, vec_t pitch, vec_t yaw, vec_t roll )
{
	static float sr, sp, sy, cr, cp, cy;

	// static to help MS compiler fp bugs
	sp = sin( DEG2RAD( pitch ) );
	cp = cos( DEG2RAD( pitch ) );

	sy = sin( DEG2RAD( yaw ) );
	cy = cos( DEG2RAD( yaw ) );

	sr = sin( DEG2RAD( roll ) );
	cr = cos( DEG2RAD( roll ) );

	m[ 0 ] = cp * cy;
	m[ 4 ] = ( sr * sp * cy + cr * -sy );
	m[ 8 ] = ( cr * sp * cy + -sr * -sy );
	m[ 12 ] = 0;
	m[ 1 ] = cp * sy;
	m[ 5 ] = ( sr * sp * sy + cr * cy );
	m[ 9 ] = ( cr * sp * sy + -sr * cy );
	m[ 13 ] = 0;
	m[ 2 ] = -sp;
	m[ 6 ] = sr * cp;
	m[ 10 ] = cr * cp;
	m[ 14 ] = 0;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixFromVectorsFLU( matrix_t m, const vec3_t forward, const vec3_t left, const vec3_t up )
{
	m[ 0 ] = forward[ 0 ];
	m[ 4 ] = left[ 0 ];
	m[ 8 ] = up[ 0 ];
	m[ 12 ] = 0;
	m[ 1 ] = forward[ 1 ];
	m[ 5 ] = left[ 1 ];
	m[ 9 ] = up[ 1 ];
	m[ 13 ] = 0;
	m[ 2 ] = forward[ 2 ];
	m[ 6 ] = left[ 2 ];
	m[ 10 ] = up[ 2 ];
	m[ 14 ] = 0;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixFromVectorsFRU( matrix_t m, const vec3_t forward, const vec3_t right, const vec3_t up )
{
	m[ 0 ] = forward[ 0 ];
	m[ 4 ] = -right[ 0 ];
	m[ 8 ] = up[ 0 ];
	m[ 12 ] = 0;
	m[ 1 ] = forward[ 1 ];
	m[ 5 ] = -right[ 1 ];
	m[ 9 ] = up[ 1 ];
	m[ 13 ] = 0;
	m[ 2 ] = forward[ 2 ];
	m[ 6 ] = -right[ 2 ];
	m[ 10 ] = up[ 2 ];
	m[ 14 ] = 0;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixFromQuat( matrix_t m, const quat_t q )
{
#if 1

	/*
	From Quaternion to Matrix and Back
	February 27th 2005
	J.M.P. van Waveren

	http://www.intel.com/cd/ids/developer/asmo-na/eng/293748.htm
	*/
	float x2, y2, z2 /*, w2*/;
	float yy2, xy2;
	float xz2, yz2, zz2;
	float wz2, wy2, wx2, xx2;

	x2 = q[ 0 ] + q[ 0 ];
	y2 = q[ 1 ] + q[ 1 ];
	z2 = q[ 2 ] + q[ 2 ];
	//w2 = q[3] + q[3]; //Is this used for some underlying optimization?

	yy2 = q[ 1 ] * y2;
	xy2 = q[ 0 ] * y2;

	xz2 = q[ 0 ] * z2;
	yz2 = q[ 1 ] * z2;
	zz2 = q[ 2 ] * z2;

	wz2 = q[ 3 ] * z2;
	wy2 = q[ 3 ] * y2;
	wx2 = q[ 3 ] * x2;
	xx2 = q[ 0 ] * x2;

	m[ 0 ] = -yy2 - zz2 + 1.0f;
	m[ 1 ] = xy2 + wz2;
	m[ 2 ] = xz2 - wy2;

	m[ 4 ] = xy2 - wz2;
	m[ 5 ] = -xx2 - zz2 + 1.0f;
	m[ 6 ] = yz2 + wx2;

	m[ 8 ] = xz2 + wy2;
	m[ 9 ] = yz2 - wx2;
	m[ 10 ] = -xx2 - yy2 + 1.0f;

	m[ 3 ] = m[ 7 ] = m[ 11 ] = m[ 12 ] = m[ 13 ] = m[ 14 ] = 0;
	m[ 15 ] = 1;

#else

	/*
	http://www.gamedev.net/reference/articles/article1691.asp#Q54
	Q54. How do I convert a quaternion to a rotation matrix?

	Assuming that a quaternion has been created in the form:

	Q = |X Y Z W|

	Then the quaternion can then be converted into a 4x4 rotation
	matrix using the following expression (Warning: you might have to
	transpose this matrix if you (do not) follow the OpenGL order!):

	 ?        2     2                                      ?
	 ? 1 - (2Y  + 2Z )   2XY - 2ZW         2XZ + 2YW       ?
	 ?                                                     ?
	 ?                          2     2                    ?
	M = ? 2XY + 2ZW         1 - (2X  + 2Z )   2YZ - 2XW       ?
	 ?                                                     ?
	 ?                                            2     2  ?
	 ? 2XZ - 2YW         2YZ + 2XW         1 - (2X  + 2Y ) ?
	 ?                                                     ?
	*/

	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/index.htm

	float xx, xy, xz, xw, yy, yz, yw, zz, zw;

	xx = q[ 0 ] * q[ 0 ];
	xy = q[ 0 ] * q[ 1 ];
	xz = q[ 0 ] * q[ 2 ];
	xw = q[ 0 ] * q[ 3 ];
	yy = q[ 1 ] * q[ 1 ];
	yz = q[ 1 ] * q[ 2 ];
	yw = q[ 1 ] * q[ 3 ];
	zz = q[ 2 ] * q[ 2 ];
	zw = q[ 2 ] * q[ 3 ];

	m[ 0 ] = 1 - 2 * ( yy + zz );
	m[ 1 ] = 2 * ( xy + zw );
	m[ 2 ] = 2 * ( xz - yw );
	m[ 4 ] = 2 * ( xy - zw );
	m[ 5 ] = 1 - 2 * ( xx + zz );
	m[ 6 ] = 2 * ( yz + xw );
	m[ 8 ] = 2 * ( xz + yw );
	m[ 9 ] = 2 * ( yz - xw );
	m[ 10 ] = 1 - 2 * ( xx + yy );

	m[ 3 ] = m[ 7 ] = m[ 11 ] = m[ 12 ] = m[ 13 ] = m[ 14 ] = 0;
	m[ 15 ] = 1;
#endif
}

void MatrixFromPlanes( matrix_t m, const vec4_t left, const vec4_t right, const vec4_t bottom, const vec4_t top, const vec4_t near, const vec4_t far )
{
	m[ 0 ] = ( right[ 0 ] - left[ 0 ] ) / 2;
	m[ 1 ] = ( top[ 0 ] - bottom[ 0 ] ) / 2;
	m[ 2 ] = ( far[ 0 ] - near[ 0 ] ) / 2;
	m[ 3 ] = right[ 0 ] - ( right[ 0 ] - left[ 0 ] ) / 2;

	m[ 4 ] = ( right[ 1 ] - left[ 1 ] ) / 2;
	m[ 5 ] = ( top[ 1 ] - bottom[ 1 ] ) / 2;
	m[ 6 ] = ( far[ 1 ] - near[ 1 ] ) / 2;
	m[ 7 ] = right[ 1 ] - ( right[ 1 ] - left[ 1 ] ) / 2;

	m[ 8 ] = ( right[ 2 ] - left[ 2 ] ) / 2;
	m[ 9 ] = ( top[ 2 ] - bottom[ 2 ] ) / 2;
	m[ 10 ] = ( far[ 2 ] - near[ 2 ] ) / 2;
	m[ 11 ] = right[ 2 ] - ( right[ 2 ] - left[ 2 ] ) / 2;

#if 0
	m[ 12 ] = ( right[ 3 ] - left[ 3 ] ) / 2;
	m[ 13 ] = ( top[ 3 ] - bottom[ 3 ] ) / 2;
	m[ 14 ] = ( far[ 3 ] - near[ 3 ] ) / 2;
	m[ 15 ] = right[ 3 ] - ( right[ 3 ] - left[ 3 ] ) / 2;
#else
	m[ 12 ] = ( -right[ 3 ] - -left[ 3 ] ) / 2;
	m[ 13 ] = ( -top[ 3 ] - -bottom[ 3 ] ) / 2;
	m[ 14 ] = ( -far[ 3 ] - -near[ 3 ] ) / 2;
	m[ 15 ] = -right[ 3 ] - ( -right[ 3 ] - -left[ 3 ] ) / 2;
#endif
}

void MatrixToVectorsFLU( const matrix_t m, vec3_t forward, vec3_t left, vec3_t up )
{
	if ( forward )
	{
		forward[ 0 ] = m[ 0 ]; // cp*cy;
		forward[ 1 ] = m[ 1 ]; // cp*sy;
		forward[ 2 ] = m[ 2 ]; //-sp;
	}

	if ( left )
	{
		left[ 0 ] = m[ 4 ]; // sr*sp*cy+cr*-sy;
		left[ 1 ] = m[ 5 ]; // sr*sp*sy+cr*cy;
		left[ 2 ] = m[ 6 ]; // sr*cp;
	}

	if ( up )
	{
		up[ 0 ] = m[ 8 ]; // cr*sp*cy+-sr*-sy;
		up[ 1 ] = m[ 9 ]; // cr*sp*sy+-sr*cy;
		up[ 2 ] = m[ 10 ]; // cr*cp;
	}
}

void MatrixToVectorsFRU( const matrix_t m, vec3_t forward, vec3_t right, vec3_t up )
{
	if ( forward )
	{
		forward[ 0 ] = m[ 0 ];
		forward[ 1 ] = m[ 1 ];
		forward[ 2 ] = m[ 2 ];
	}

	if ( right )
	{
		right[ 0 ] = -m[ 4 ];
		right[ 1 ] = -m[ 5 ];
		right[ 2 ] = -m[ 6 ];
	}

	if ( up )
	{
		up[ 0 ] = m[ 8 ];
		up[ 1 ] = m[ 9 ];
		up[ 2 ] = m[ 10 ];
	}
}

void MatrixSetupTransformFromVectorsFLU( matrix_t m, const vec3_t forward, const vec3_t left, const vec3_t up, const vec3_t origin )
{
	m[ 0 ] = forward[ 0 ];
	m[ 4 ] = left[ 0 ];
	m[ 8 ] = up[ 0 ];
	m[ 12 ] = origin[ 0 ];
	m[ 1 ] = forward[ 1 ];
	m[ 5 ] = left[ 1 ];
	m[ 9 ] = up[ 1 ];
	m[ 13 ] = origin[ 1 ];
	m[ 2 ] = forward[ 2 ];
	m[ 6 ] = left[ 2 ];
	m[ 10 ] = up[ 2 ];
	m[ 14 ] = origin[ 2 ];
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixSetupTransformFromVectorsFRU( matrix_t m, const vec3_t forward, const vec3_t right, const vec3_t up, const vec3_t origin )
{
	m[ 0 ] = forward[ 0 ];
	m[ 4 ] = -right[ 0 ];
	m[ 8 ] = up[ 0 ];
	m[ 12 ] = origin[ 0 ];
	m[ 1 ] = forward[ 1 ];
	m[ 5 ] = -right[ 1 ];
	m[ 9 ] = up[ 1 ];
	m[ 13 ] = origin[ 1 ];
	m[ 2 ] = forward[ 2 ];
	m[ 6 ] = -right[ 2 ];
	m[ 10 ] = up[ 2 ];
	m[ 14 ] = origin[ 2 ];
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixSetupTransformFromRotation( matrix_t m, const matrix_t rot, const vec3_t origin )
{
	m[ 0 ] = rot[ 0 ];
	m[ 4 ] = rot[ 4 ];
	m[ 8 ] = rot[ 8 ];
	m[ 12 ] = origin[ 0 ];
	m[ 1 ] = rot[ 1 ];
	m[ 5 ] = rot[ 5 ];
	m[ 9 ] = rot[ 9 ];
	m[ 13 ] = origin[ 1 ];
	m[ 2 ] = rot[ 2 ];
	m[ 6 ] = rot[ 6 ];
	m[ 10 ] = rot[ 10 ];
	m[ 14 ] = origin[ 2 ];
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixSetupTransformFromQuat( matrix_t m, const quat_t quat, const vec3_t origin )
{
	matrix_t rot;

	MatrixFromQuat( rot, quat );

	m[ 0 ] = rot[ 0 ];
	m[ 4 ] = rot[ 4 ];
	m[ 8 ] = rot[ 8 ];
	m[ 12 ] = origin[ 0 ];
	m[ 1 ] = rot[ 1 ];
	m[ 5 ] = rot[ 5 ];
	m[ 9 ] = rot[ 9 ];
	m[ 13 ] = origin[ 1 ];
	m[ 2 ] = rot[ 2 ];
	m[ 6 ] = rot[ 6 ];
	m[ 10 ] = rot[ 10 ];
	m[ 14 ] = origin[ 2 ];
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixAffineInverse( const matrix_t in, matrix_t out )
{
#if 0
	MatrixCopy( in, out );
	MatrixInverse( out );
#else
	// Tr3B - cleaned up
	out[ 0 ] = in[ 0 ];
	out[ 4 ] = in[ 1 ];
	out[ 8 ] = in[ 2 ];
	out[ 1 ] = in[ 4 ];
	out[ 5 ] = in[ 5 ];
	out[ 9 ] = in[ 6 ];
	out[ 2 ] = in[ 8 ];
	out[ 6 ] = in[ 9 ];
	out[ 10 ] = in[ 10 ];
	out[ 3 ] = 0;
	out[ 7 ] = 0;
	out[ 11 ] = 0;
	out[ 15 ] = 1;

	out[ 12 ] = - ( in[ 12 ] * out[ 0 ] + in[ 13 ] * out[ 4 ] + in[ 14 ] * out[ 8 ] );
	out[ 13 ] = - ( in[ 12 ] * out[ 1 ] + in[ 13 ] * out[ 5 ] + in[ 14 ] * out[ 9 ] );
	out[ 14 ] = - ( in[ 12 ] * out[ 2 ] + in[ 13 ] * out[ 6 ] + in[ 14 ] * out[ 10 ] );
#endif
}

void MatrixTransformNormal( const matrix_t m, const vec3_t in, vec3_t out )
{
	out[ 0 ] = m[ 0 ] * in[ 0 ] + m[ 4 ] * in[ 1 ] + m[ 8 ] * in[ 2 ];
	out[ 1 ] = m[ 1 ] * in[ 0 ] + m[ 5 ] * in[ 1 ] + m[ 9 ] * in[ 2 ];
	out[ 2 ] = m[ 2 ] * in[ 0 ] + m[ 6 ] * in[ 1 ] + m[ 10 ] * in[ 2 ];
}

void MatrixTransformNormal2( const matrix_t m, vec3_t inout )
{
	vec3_t tmp;

	tmp[ 0 ] = m[ 0 ] * inout[ 0 ] + m[ 4 ] * inout[ 1 ] + m[ 8 ] * inout[ 2 ];
	tmp[ 1 ] = m[ 1 ] * inout[ 0 ] + m[ 5 ] * inout[ 1 ] + m[ 9 ] * inout[ 2 ];
	tmp[ 2 ] = m[ 2 ] * inout[ 0 ] + m[ 6 ] * inout[ 1 ] + m[ 10 ] * inout[ 2 ];

	VectorCopy( tmp, inout );
}

void MatrixTransformPoint( const matrix_t m, const vec3_t in, vec3_t out )
{
	out[ 0 ] = m[ 0 ] * in[ 0 ] + m[ 4 ] * in[ 1 ] + m[ 8 ] * in[ 2 ] + m[ 12 ];
	out[ 1 ] = m[ 1 ] * in[ 0 ] + m[ 5 ] * in[ 1 ] + m[ 9 ] * in[ 2 ] + m[ 13 ];
	out[ 2 ] = m[ 2 ] * in[ 0 ] + m[ 6 ] * in[ 1 ] + m[ 10 ] * in[ 2 ] + m[ 14 ];
}

void MatrixTransformPoint2( const matrix_t m, vec3_t inout )
{
	vec3_t tmp;

	tmp[ 0 ] = m[ 0 ] * inout[ 0 ] + m[ 4 ] * inout[ 1 ] + m[ 8 ] * inout[ 2 ] + m[ 12 ];
	tmp[ 1 ] = m[ 1 ] * inout[ 0 ] + m[ 5 ] * inout[ 1 ] + m[ 9 ] * inout[ 2 ] + m[ 13 ];
	tmp[ 2 ] = m[ 2 ] * inout[ 0 ] + m[ 6 ] * inout[ 1 ] + m[ 10 ] * inout[ 2 ] + m[ 14 ];

	VectorCopy( tmp, inout );
}

void MatrixTransform4( const matrix_t m, const vec4_t in, vec4_t out )
{
#if id386_sse
//#error MatrixTransform4

	__m128 _t0, _t1, _t2, _x, _y, _z, _w, _m0, _m1, _m2, _m3;

	_m0 = _mm_loadu_ps( &m[ 0 ] );
	_m1 = _mm_loadu_ps( &m[ 4 ] );
	_m2 = _mm_loadu_ps( &m[ 8 ] );
	_m3 = _mm_loadu_ps( &m[ 12 ] );

	_t0 = _mm_loadu_ps( in );
	_x = _mm_shuffle_ps( _t0, _t0, _MM_SHUFFLE( 0, 0, 0, 0 ) );
	_y = _mm_shuffle_ps( _t0, _t0, _MM_SHUFFLE( 1, 1, 1, 1 ) );
	_z = _mm_shuffle_ps( _t0, _t0, _MM_SHUFFLE( 2, 2, 2, 2 ) );
	_w = _mm_shuffle_ps( _t0, _t0, _MM_SHUFFLE( 3, 3, 3, 3 ) );

	_t0 = _mm_mul_ps( _m3, _w );
	_t1 = _mm_mul_ps( _m2, _z );
	_t0 = _mm_add_ps( _t0, _t1 );

	_t1 = _mm_mul_ps( _m1, _y );
	_t2 = _mm_mul_ps( _m0, _x );
	_t1 = _mm_add_ps( _t1, _t2 );

	_t0 = _mm_add_ps( _t0, _t1 );

	_mm_storeu_ps( out, _t0 );
#else
	out[ 0 ] = m[ 0 ] * in[ 0 ] + m[ 4 ] * in[ 1 ] + m[ 8 ] * in[ 2 ] + m[ 12 ] * in[ 3 ];
	out[ 1 ] = m[ 1 ] * in[ 0 ] + m[ 5 ] * in[ 1 ] + m[ 9 ] * in[ 2 ] + m[ 13 ] * in[ 3 ];
	out[ 2 ] = m[ 2 ] * in[ 0 ] + m[ 6 ] * in[ 1 ] + m[ 10 ] * in[ 2 ] + m[ 14 ] * in[ 3 ];
	out[ 3 ] = m[ 3 ] * in[ 0 ] + m[ 7 ] * in[ 1 ] + m[ 11 ] * in[ 2 ] + m[ 15 ] * in[ 3 ];
#endif
}

void MatrixTransformPlane( const matrix_t m, const vec4_t in, vec4_t out )
{
	vec3_t translation;
	vec3_t planePos;

	// rotate the plane normal
	MatrixTransformNormal( m, in, out );

	// add new position to current plane position
	VectorSet( translation,  m[ 12 ], m[ 13 ], m[ 14 ] );
	VectorMA( translation, in[ 3 ], out, planePos );

	out[ 3 ] = DotProduct( out, planePos );
}

void MatrixTransformPlane2( const matrix_t m, vec4_t inout )
{
	vec4_t tmp;

	MatrixTransformPlane( m, inout, tmp );
	Vector4Copy( tmp, inout );
}

/*
replacement for glFrustum
see glspec30.pdf chapter 2.12 Coordinate Transformations
*/
void MatrixPerspectiveProjection( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far )
{
	m[ 0 ] = ( 2 * near ) / ( right - left );
	m[ 4 ] = 0;
	m[ 8 ] = ( right + left ) / ( right - left );
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = ( 2 * near ) / ( top - bottom );
	m[ 9 ] = ( top + bottom ) / ( top - bottom );
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = - ( far + near ) / ( far - near );
	m[ 14 ] = - ( 2 * far * near ) / ( far - near );
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = -1;
	m[ 15 ] = 0;
}

/*
same as D3DXMatrixPerspectiveOffCenterLH

http://msdn.microsoft.com/en-us/library/bb205353(VS.85).aspx
*/
void MatrixPerspectiveProjectionLH( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far )
{
	m[ 0 ] = ( 2 * near ) / ( right - left );
	m[ 4 ] = 0;
	m[ 8 ] = ( left + right ) / ( left - right );
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = ( 2 * near ) / ( top - bottom );
	m[ 9 ] = ( top + bottom ) / ( bottom - top );
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = far / ( far - near );
	m[ 14 ] = ( near * far ) / ( near - far );
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 1;
	m[ 15 ] = 0;
}

/*
same as D3DXMatrixPerspectiveOffCenterRH

http://msdn.microsoft.com/en-us/library/bb205354(VS.85).aspx
*/
void MatrixPerspectiveProjectionRH( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far )
{
	m[ 0 ] = ( 2 * near ) / ( right - left );
	m[ 4 ] = 0;
	m[ 8 ] = ( left + right ) / ( right - left );
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = ( 2 * near ) / ( top - bottom );
	m[ 9 ] = ( top + bottom ) / ( top - bottom );
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = far / ( near - far );
	m[ 14 ] = ( near * far ) / ( near - far );
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = -1;
	m[ 15 ] = 0;
}

/*
same as D3DXMatrixPerspectiveFovLH

http://msdn.microsoft.com/en-us/library/bb205350(VS.85).aspx
*/
void MatrixPerspectiveProjectionFovYAspectLH( matrix_t m, vec_t fov, vec_t aspect, vec_t near, vec_t far )
{
	vec_t width, height;

	width = tanf( DEG2RAD( fov * 0.5f ) );
	height = width / aspect;

	m[ 0 ] = 1 / width;
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = 1 / height;
	m[ 9 ] = 0;
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = far / ( far - near );
	m[ 14 ] = - ( near * far ) / ( far - near );
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 1;
	m[ 15 ] = 0;
}

void MatrixPerspectiveProjectionFovXYLH( matrix_t m, vec_t fovX, vec_t fovY, vec_t near, vec_t far )
{
	vec_t width, height;

	width = tanf( DEG2RAD( fovX * 0.5f ) );
	height = tanf( DEG2RAD( fovY * 0.5f ) );

	m[ 0 ] = 1 / width;
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = 1 / height;
	m[ 9 ] = 0;
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = far / ( far - near );
	m[ 14 ] = - ( near * far ) / ( far - near );
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 1;
	m[ 15 ] = 0;
}

void MatrixPerspectiveProjectionFovXYRH( matrix_t m, vec_t fovX, vec_t fovY, vec_t near, vec_t far )
{
	vec_t width, height;

	width = tanf( DEG2RAD( fovX * 0.5f ) );
	height = tanf( DEG2RAD( fovY * 0.5f ) );

	m[ 0 ] = 1 / width;
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = 1 / height;
	m[ 9 ] = 0;
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = far / ( near - far );
	m[ 14 ] = ( near * far ) / ( near - far );
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = -1;
	m[ 15 ] = 0;
}

// Tr3B: far plane at infinity, see RobustShadowVolumes.pdf by Nvidia
void MatrixPerspectiveProjectionFovXYInfiniteRH( matrix_t m, vec_t fovX, vec_t fovY, vec_t near )
{
	vec_t width, height;

	width = tanf( DEG2RAD( fovX * 0.5f ) );
	height = tanf( DEG2RAD( fovY * 0.5f ) );

	m[ 0 ] = 1 / width;
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = 0;
	m[ 1 ] = 0;
	m[ 5 ] = 1 / height;
	m[ 9 ] = 0;
	m[ 13 ] = 0;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = -1;
	m[ 14 ] = -2 * near;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = -1;
	m[ 15 ] = 0;
}

/*
replacement for glOrtho
see glspec30.pdf chapter 2.12 Coordinate Transformations
*/
void MatrixOrthogonalProjection( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far )
{
	m[ 0 ] = 2 / ( right - left );
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = - ( right + left ) / ( right - left );
	m[ 1 ] = 0;
	m[ 5 ] = 2 / ( top - bottom );
	m[ 9 ] = 0;
	m[ 13 ] = - ( top + bottom ) / ( top - bottom );
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = -2 / ( far - near );
	m[ 14 ] = - ( far + near ) / ( far - near );
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

/*
same as D3DXMatrixOrthoOffCenterLH

http://msdn.microsoft.com/en-us/library/bb205347(VS.85).aspx
*/
void MatrixOrthogonalProjectionLH( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far )
{
	m[ 0 ] = 2 / ( right - left );
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = ( left + right ) / ( left - right );
	m[ 1 ] = 0;
	m[ 5 ] = 2 / ( top - bottom );
	m[ 9 ] = 0;
	m[ 13 ] = ( top + bottom ) / ( bottom - top );
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = 1 / ( far - near );
	m[ 14 ] = near / ( near - far );
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

/*
same as D3DXMatrixOrthoOffCenterRH

http://msdn.microsoft.com/en-us/library/bb205348(VS.85).aspx
*/
void MatrixOrthogonalProjectionRH( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far )
{
	m[ 0 ] = 2 / ( right - left );
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = ( left + right ) / ( left - right );
	m[ 1 ] = 0;
	m[ 5 ] = 2 / ( top - bottom );
	m[ 9 ] = 0;
	m[ 13 ] = ( top + bottom ) / ( bottom - top );
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = 1 / ( near - far );
	m[ 14 ] = near / ( near - far );
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

/*
same as D3DXMatrixReflect

http://msdn.microsoft.com/en-us/library/bb205356%28v=VS.85%29.aspx
*/
void MatrixPlaneReflection( matrix_t m, const vec4_t plane )
{
	vec4_t P;
	Vector4Copy( plane, P );

	PlaneNormalize( P );

	/*
	-2 * P.a * P.a + 1  -2 * P.b * P.a      -2 * P.c * P.a        0
	-2 * P.a * P.b      -2 * P.b * P.b + 1  -2 * P.c * P.b        0
	-2 * P.a * P.c      -2 * P.b * P.c      -2 * P.c * P.c + 1    0
	-2 * P.a * P.d      -2 * P.b * P.d      -2 * P.c * P.d        1
	 */

	// Quake uses a different plane equation
	m[ 0 ] = -2 * P[ 0 ] * P[ 0 ] + 1;
	m[ 4 ] = -2 * P[ 0 ] * P[ 1 ];
	m[ 8 ] = -2 * P[ 0 ] * P[ 2 ];
	m[ 12 ] = 2 * P[ 0 ] * P[ 3 ];
	m[ 1 ] = -2 * P[ 1 ] * P[ 0 ];
	m[ 5 ] = -2 * P[ 1 ] * P[ 1 ] + 1;
	m[ 9 ] = -2 * P[ 1 ] * P[ 2 ];
	m[ 13 ] = 2 * P[ 1 ] * P[ 3 ];
	m[ 2 ] = -2 * P[ 2 ] * P[ 0 ];
	m[ 6 ] = -2 * P[ 2 ] * P[ 1 ];
	m[ 10 ] = -2 * P[ 2 ] * P[ 2 ] + 1;
	m[ 14 ] = 2 * P[ 2 ] * P[ 3 ];
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;

#if 0
	matrix_t m2;
	MatrixCopy( m, m2 );
	MatrixTranspose( m2, m );
#endif
}

void MatrixLookAtLH( matrix_t m, const vec3_t eye, const vec3_t dir, const vec3_t up )
{
	vec3_t dirN;
	vec3_t upN;
	vec3_t sideN;

#if 1
	CrossProduct( up, dir, sideN );
	VectorNormalize( sideN );

	CrossProduct( dir, sideN, upN );
	VectorNormalize( upN );
#else
	CrossProduct( dir, up, sideN );
	VectorNormalize( sideN );

	CrossProduct( sideN, dir, upN );
	VectorNormalize( upN );
#endif

	VectorNormalize2( dir, dirN );

	m[ 0 ] = sideN[ 0 ];
	m[ 4 ] = sideN[ 1 ];
	m[ 8 ] = sideN[ 2 ];
	m[ 12 ] = -DotProduct( sideN, eye );
	m[ 1 ] = upN[ 0 ];
	m[ 5 ] = upN[ 1 ];
	m[ 9 ] = upN[ 2 ];
	m[ 13 ] = -DotProduct( upN, eye );
	m[ 2 ] = dirN[ 0 ];
	m[ 6 ] = dirN[ 1 ];
	m[ 10 ] = dirN[ 2 ];
	m[ 14 ] = -DotProduct( dirN, eye );
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixLookAtRH( matrix_t m, const vec3_t eye, const vec3_t dir, const vec3_t up )
{
	vec3_t dirN;
	vec3_t upN;
	vec3_t sideN;

	CrossProduct( dir, up, sideN );
	VectorNormalize( sideN );

	CrossProduct( sideN, dir, upN );
	VectorNormalize( upN );

	VectorNormalize2( dir, dirN );

	m[ 0 ] = sideN[ 0 ];
	m[ 4 ] = sideN[ 1 ];
	m[ 8 ] = sideN[ 2 ];
	m[ 12 ] = -DotProduct( sideN, eye );
	m[ 1 ] = upN[ 0 ];
	m[ 5 ] = upN[ 1 ];
	m[ 9 ] = upN[ 2 ];
	m[ 13 ] = -DotProduct( upN, eye );
	m[ 2 ] = -dirN[ 0 ];
	m[ 6 ] = -dirN[ 1 ];
	m[ 10 ] = -dirN[ 2 ];
	m[ 14 ] = DotProduct( dirN, eye );
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixScaleTranslateToUnitCube( matrix_t m, const vec3_t mins, const vec3_t maxs )
{
	m[ 0 ] = 2 / ( maxs[ 0 ] - mins[ 0 ] );
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = - ( maxs[ 0 ] + mins[ 0 ] ) / ( maxs[ 0 ] - mins[ 0 ] );

	m[ 1 ] = 0;
	m[ 5 ] = 2 / ( maxs[ 1 ] - mins[ 1 ] );
	m[ 9 ] = 0;
	m[ 13 ] = - ( maxs[ 1 ] + mins[ 1 ] ) / ( maxs[ 1 ] - mins[ 1 ] );

	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = 2 / ( maxs[ 2 ] - mins[ 2 ] );
	m[ 14 ] = - ( maxs[ 2 ] + mins[ 2 ] ) / ( maxs[ 2 ] - mins[ 2 ] );

	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

void MatrixCrop( matrix_t m, const vec3_t mins, const vec3_t maxs )
{
	float scaleX, scaleY, scaleZ;
	float offsetX, offsetY, offsetZ;

	scaleX = 2.0f / ( maxs[ 0 ] - mins[ 0 ] );
	scaleY = 2.0f / ( maxs[ 1 ] - mins[ 1 ] );

	offsetX = -0.5f * ( maxs[ 0 ] + mins[ 0 ] ) * scaleX;
	offsetY = -0.5f * ( maxs[ 1 ] + mins[ 1 ] ) * scaleY;

	scaleZ = 1.0f / ( maxs[ 2 ] - mins[ 2 ] );
	offsetZ = -mins[ 2 ] * scaleZ;

	m[ 0 ] = scaleX;
	m[ 4 ] = 0;
	m[ 8 ] = 0;
	m[ 12 ] = offsetX;
	m[ 1 ] = 0;
	m[ 5 ] = scaleY;
	m[ 9 ] = 0;
	m[ 13 ] = offsetY;
	m[ 2 ] = 0;
	m[ 6 ] = 0;
	m[ 10 ] = scaleZ;
	m[ 14 ] = offsetZ;
	m[ 3 ] = 0;
	m[ 7 ] = 0;
	m[ 11 ] = 0;
	m[ 15 ] = 1;
}

//=============================================

// RB: XreaL quaternion math functions

// *INDENT-ON*

vec_t QuatNormalize( quat_t q )
{
	float length, ilength;

	length = q[ 0 ] * q[ 0 ] + q[ 1 ] * q[ 1 ] + q[ 2 ] * q[ 2 ] + q[ 3 ] * q[ 3 ];
	length = sqrt( length );

	if ( length )
	{
		ilength = 1 / length;
		q[ 0 ] *= ilength;
		q[ 1 ] *= ilength;
		q[ 2 ] *= ilength;
		q[ 3 ] *= ilength;
	}

	return length;
}

void QuatFromAngles( quat_t q, vec_t pitch, vec_t yaw, vec_t roll )
{
#if 1
	matrix_t tmp;

	MatrixFromAngles( tmp, pitch, yaw, roll );
	QuatFromMatrix( q, tmp );
#else
	static float sr, sp, sy, cr, cp, cy;

	// static to help MS compiler fp bugs
	sp = sin( DEG2RAD( pitch ) );
	cp = cos( DEG2RAD( pitch ) );

	sy = sin( DEG2RAD( yaw ) );
	cy = cos( DEG2RAD( yaw ) );

	sr = sin( DEG2RAD( roll ) );
	cr = cos( DEG2RAD( roll ) );

	q[ 0 ] = sr * cp * cy - cr * sp * sy; // x
	q[ 1 ] = cr * sp * cy + sr * cp * sy; // y
	q[ 2 ] = cr * cp * sy - sr * sp * cy; // z
	q[ 3 ] = cr * cp * cy + sr * sp * sy; // w
#endif
}

void QuatFromMatrix( quat_t q, const matrix_t m )
{
#if 1

	/*
	   From Quaternion to Matrix and Back
	   February 27th 2005
	   J.M.P. van Waveren

	   http://www.intel.com/cd/ids/developer/asmo-na/eng/293748.htm
	 */
	float t, s;

	if ( m[ 0 ] + m[ 5 ] + m[ 10 ] > 0.0f )
	{
		t = m[ 0 ] + m[ 5 ] + m[ 10 ] + 1.0f;
		s = ( 1.0f / sqrtf( t ) ) * 0.5f;

		q[ 3 ] = s * t;
		q[ 2 ] = ( m[ 1 ] - m[ 4 ] ) * s;
		q[ 1 ] = ( m[ 8 ] - m[ 2 ] ) * s;
		q[ 0 ] = ( m[ 6 ] - m[ 9 ] ) * s;
	}
	else if ( m[ 0 ] > m[ 5 ] && m[ 0 ] > m[ 10 ] )
	{
		t = m[ 0 ] - m[ 5 ] - m[ 10 ] + 1.0f;
		s = ( 1.0f / sqrtf( t ) ) * 0.5f;

		q[ 0 ] = s * t;
		q[ 1 ] = ( m[ 1 ] + m[ 4 ] ) * s;
		q[ 2 ] = ( m[ 8 ] + m[ 2 ] ) * s;
		q[ 3 ] = ( m[ 6 ] - m[ 9 ] ) * s;
	}
	else if ( m[ 5 ] > m[ 10 ] )
	{
		t = -m[ 0 ] + m[ 5 ] - m[ 10 ] + 1.0f;
		s = ( 1.0f / sqrtf( t ) ) * 0.5f;

		q[ 1 ] = s * t;
		q[ 0 ] = ( m[ 1 ] + m[ 4 ] ) * s;
		q[ 3 ] = ( m[ 8 ] - m[ 2 ] ) * s;
		q[ 2 ] = ( m[ 6 ] + m[ 9 ] ) * s;
	}
	else
	{
		t = -m[ 0 ] - m[ 5 ] + m[ 10 ] + 1.0f;
		s = ( 1.0f / sqrtf( t ) ) * 0.5f;

		q[ 2 ] = s * t;
		q[ 3 ] = ( m[ 1 ] - m[ 4 ] ) * s;
		q[ 0 ] = ( m[ 8 ] + m[ 2 ] ) * s;
		q[ 1 ] = ( m[ 6 ] + m[ 9 ] ) * s;
	}

#else
	float trace;

	// http://www.euclideanspace.com/maths/geometry/rotations/conversions/matrixToQuaternion/index.htm

	trace = 1.0f + m[ 0 ] + m[ 5 ] + m[ 10 ];

	if ( trace > 0.0f )
	{
		vec_t s = 0.5f / sqrt( trace );

		q[ 0 ] = ( m[ 6 ] - m[ 9 ] ) * s;
		q[ 1 ] = ( m[ 8 ] - m[ 2 ] ) * s;
		q[ 2 ] = ( m[ 1 ] - m[ 4 ] ) * s;
		q[ 3 ] = 0.25f / s;
	}
	else
	{
		if ( m[ 0 ] > m[ 5 ] && m[ 0 ] > m[ 10 ] )
		{
			// column 0
			float s = sqrt( 1.0f + m[ 0 ] - m[ 5 ] - m[ 10 ] ) * 2.0f;

			q[ 0 ] = 0.25f * s;
			q[ 1 ] = ( m[ 4 ] + m[ 1 ] ) / s;
			q[ 2 ] = ( m[ 8 ] + m[ 2 ] ) / s;
			q[ 3 ] = ( m[ 9 ] - m[ 6 ] ) / s;
		}
		else if ( m[ 5 ] > m[ 10 ] )
		{
			// column 1
			float s = sqrt( 1.0f + m[ 5 ] - m[ 0 ] - m[ 10 ] ) * 2.0f;

			q[ 0 ] = ( m[ 4 ] + m[ 1 ] ) / s;
			q[ 1 ] = 0.25f * s;
			q[ 2 ] = ( m[ 9 ] + m[ 6 ] ) / s;
			q[ 3 ] = ( m[ 8 ] - m[ 2 ] ) / s;
		}
		else
		{
			// column 2
			float s = sqrt( 1.0f + m[ 10 ] - m[ 0 ] - m[ 5 ] ) * 2.0f;

			q[ 0 ] = ( m[ 8 ] + m[ 2 ] ) / s;
			q[ 1 ] = ( m[ 9 ] + m[ 6 ] ) / s;
			q[ 2 ] = 0.25f * s;
			q[ 3 ] = ( m[ 4 ] - m[ 1 ] ) / s;
		}
	}

	QuatNormalize( q );
#endif
}

void QuatToVectorsFLU( const quat_t q, vec3_t forward, vec3_t left, vec3_t up )
{
	matrix_t tmp;

	MatrixFromQuat( tmp, q );
	MatrixToVectorsFRU( tmp, forward, left, up );
}

void QuatToVectorsFRU( const quat_t q, vec3_t forward, vec3_t right, vec3_t up )
{
	matrix_t tmp;

	MatrixFromQuat( tmp, q );
	MatrixToVectorsFRU( tmp, forward, right, up );
}

void QuatToAxis( const quat_t q, vec3_t axis[ 3 ] )
{
	matrix_t tmp;

	MatrixFromQuat( tmp, q );
	MatrixToVectorsFLU( tmp, axis[ 0 ], axis[ 1 ], axis[ 2 ] );
}

void QuatToAngles( const quat_t q, vec3_t angles )
{
	quat_t q2;

	q2[ 0 ] = q[ 0 ] * q[ 0 ];
	q2[ 1 ] = q[ 1 ] * q[ 1 ];
	q2[ 2 ] = q[ 2 ] * q[ 2 ];
	q2[ 3 ] = q[ 3 ] * q[ 3 ];

	angles[ PITCH ] = RAD2DEG( asin( -2 * ( q[ 2 ] * q[ 0 ] - q[ 3 ] * q[ 1 ] ) ) );
	angles[ YAW ] = RAD2DEG( atan2( 2 * ( q[ 2 ] * q[ 3 ] + q[ 0 ] * q[ 1 ] ), ( q2[ 2 ] - q2[ 3 ] - q2[ 0 ] + q2[ 1 ] ) ) );
	angles[ ROLL ] = RAD2DEG( atan2( 2 * ( q[ 3 ] * q[ 0 ] + q[ 2 ] * q[ 1 ] ), ( -q2[ 2 ] - q2[ 3 ] + q2[ 0 ] + q2[ 1 ] ) ) );
}

void QuatMultiply0( quat_t qa, const quat_t qb )
{
	quat_t tmp;

	QuatCopy( qa, tmp );
	QuatMultiply1( tmp, qb, qa );
}

void QuatMultiply1( const quat_t qa, const quat_t qb, quat_t qc )
{
	/*
	   from matrix and quaternion faq
	   x = w1x2 + x1w2 + y1z2 - z1y2
	   y = w1y2 + y1w2 + z1x2 - x1z2
	   z = w1z2 + z1w2 + x1y2 - y1x2

	   w = w1w2 - x1x2 - y1y2 - z1z2
	 */

	qc[ 0 ] = qa[ 3 ] * qb[ 0 ] + qa[ 0 ] * qb[ 3 ] + qa[ 1 ] * qb[ 2 ] - qa[ 2 ] * qb[ 1 ];
	qc[ 1 ] = qa[ 3 ] * qb[ 1 ] + qa[ 1 ] * qb[ 3 ] + qa[ 2 ] * qb[ 0 ] - qa[ 0 ] * qb[ 2 ];
	qc[ 2 ] = qa[ 3 ] * qb[ 2 ] + qa[ 2 ] * qb[ 3 ] + qa[ 0 ] * qb[ 1 ] - qa[ 1 ] * qb[ 0 ];
	qc[ 3 ] = qa[ 3 ] * qb[ 3 ] - qa[ 0 ] * qb[ 0 ] - qa[ 1 ] * qb[ 1 ] - qa[ 2 ] * qb[ 2 ];
}

void QuatMultiply2( const quat_t qa, const quat_t qb, quat_t qc )
{
	qc[ 0 ] = qa[ 3 ] * qb[ 0 ] + qa[ 0 ] * qb[ 3 ] + qa[ 1 ] * qb[ 2 ] + qa[ 2 ] * qb[ 1 ];
	qc[ 1 ] = qa[ 3 ] * qb[ 1 ] - qa[ 1 ] * qb[ 3 ] - qa[ 2 ] * qb[ 0 ] + qa[ 0 ] * qb[ 2 ];
	qc[ 2 ] = qa[ 3 ] * qb[ 2 ] - qa[ 2 ] * qb[ 3 ] - qa[ 0 ] * qb[ 1 ] + qa[ 1 ] * qb[ 0 ];
	qc[ 3 ] = qa[ 3 ] * qb[ 3 ] - qa[ 0 ] * qb[ 0 ] - qa[ 1 ] * qb[ 1 ] + qa[ 2 ] * qb[ 2 ];
}

void QuatMultiply3( const quat_t qa, const quat_t qb, quat_t qc )
{
	qc[ 0 ] = qa[ 3 ] * qb[ 0 ] + qa[ 0 ] * qb[ 3 ] + qa[ 1 ] * qb[ 2 ] + qa[ 2 ] * qb[ 1 ];
	qc[ 1 ] = -qa[ 3 ] * qb[ 1 ] + qa[ 1 ] * qb[ 3 ] - qa[ 2 ] * qb[ 0 ] + qa[ 0 ] * qb[ 2 ];
	qc[ 2 ] = -qa[ 3 ] * qb[ 2 ] + qa[ 2 ] * qb[ 3 ] - qa[ 0 ] * qb[ 1 ] + qa[ 1 ] * qb[ 0 ];
	qc[ 3 ] = -qa[ 3 ] * qb[ 3 ] + qa[ 0 ] * qb[ 0 ] - qa[ 1 ] * qb[ 1 ] + qa[ 2 ] * qb[ 2 ];
}

void QuatMultiply4( const quat_t qa, const quat_t qb, quat_t qc )
{
	qc[ 0 ] = qa[ 3 ] * qb[ 0 ] - qa[ 0 ] * qb[ 3 ] - qa[ 1 ] * qb[ 2 ] - qa[ 2 ] * qb[ 1 ];
	qc[ 1 ] = -qa[ 3 ] * qb[ 1 ] - qa[ 1 ] * qb[ 3 ] + qa[ 2 ] * qb[ 0 ] - qa[ 0 ] * qb[ 2 ];
	qc[ 2 ] = -qa[ 3 ] * qb[ 2 ] - qa[ 2 ] * qb[ 3 ] + qa[ 0 ] * qb[ 1 ] - qa[ 1 ] * qb[ 0 ];
	qc[ 3 ] = -qa[ 3 ] * qb[ 3 ] - qa[ 0 ] * qb[ 0 ] + qa[ 1 ] * qb[ 1 ] - qa[ 2 ] * qb[ 2 ];
}

void QuatSlerp( const quat_t from, const quat_t to, float frac, quat_t out )
{
#if 0
	quat_t to1;
	float omega, cosom, sinom, scale0, scale1;

	cosom = from[ 0 ] * to[ 0 ] + from[ 1 ] * to[ 1 ] + from[ 2 ] * to[ 2 ] + from[ 3 ] * to[ 3 ];

	if ( cosom < 0.0 )
	{
		cosom = -cosom;

		QuatCopy( to, to1 );
		QuatAntipodal( to1 );
	}
	else
	{
		QuatCopy( to, to1 );
	}

	if ( ( 1.0 - cosom ) > 0 )
	{
		omega = acos( cosom );
		sinom = sin( omega );
		scale0 = sin( ( 1.0 - frac ) * omega ) / sinom;
		scale1 = sin( frac * omega ) / sinom;
	}
	else
	{
		scale0 = 1.0 - frac;
		scale1 = frac;
	}

	out[ 0 ] = scale0 * from[ 0 ] + scale1 * to1[ 0 ];
	out[ 1 ] = scale0 * from[ 1 ] + scale1 * to1[ 1 ];
	out[ 2 ] = scale0 * from[ 2 ] + scale1 * to1[ 2 ];
	out[ 3 ] = scale0 * from[ 3 ] + scale1 * to1[ 3 ];
#else

	/*
	   Slerping Clock Cycles
	   February 27th 2005
	   J.M.P. van Waveren

	   http://www.intel.com/cd/ids/developer/asmo-na/eng/293747.htm
	 */
	float cosom, absCosom, sinom, sinSqr, omega, scale0, scale1;

	if ( frac <= 0.0f )
	{
		QuatCopy( from, out );
		return;
	}

	if ( frac >= 1.0f )
	{
		QuatCopy( to, out );
		return;
	}

	if ( QuatCompare( from, to ) )
	{
		QuatCopy( from, out );
		return;
	}

	cosom = from[ 0 ] * to[ 0 ] + from[ 1 ] * to[ 1 ] + from[ 2 ] * to[ 2 ] + from[ 3 ] * to[ 3 ];
	absCosom = fabs( cosom );

	if ( ( 1.0f - absCosom ) > 1e-6f )
	{
		sinSqr = 1.0f - absCosom * absCosom;
		sinom = 1.0f / sqrt( sinSqr );
		omega = atan2( sinSqr * sinom, absCosom );

		scale0 = sin( ( 1.0f - frac ) * omega ) * sinom;
		scale1 = sin( frac * omega ) * sinom;
	}
	else
	{
		scale0 = 1.0f - frac;
		scale1 = frac;
	}

	scale1 = ( cosom >= 0.0f ) ? scale1 : -scale1;

	out[ 0 ] = scale0 * from[ 0 ] + scale1 * to[ 0 ];
	out[ 1 ] = scale0 * from[ 1 ] + scale1 * to[ 1 ];
	out[ 2 ] = scale0 * from[ 2 ] + scale1 * to[ 2 ];
	out[ 3 ] = scale0 * from[ 3 ] + scale1 * to[ 3 ];
#endif
}

void QuatTransformVector( const quat_t q, const vec3_t in, vec3_t out )
{
	matrix_t m;

	MatrixFromQuat( m, q );
	MatrixTransformNormal( m, in, out );
}

#if defined(_WIN32) && !defined(__MINGW32__)
double rint( double x )
{
	return floor( x + 0.5 );
}
#endif
