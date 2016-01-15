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

#ifndef Q_SHARED_H_
#define Q_SHARED_H_

// math.h/cmath uses _USE_MATH_DEFINES to decide if to define M_PI etc or not.
// So define _USE_MATH_DEFINES early before including math.h/cmath
// and before including any other header in case they bring in math.h/cmath indirectly.
#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif


// q_shared.h -- included first by ALL program modules.
// A user mod should never modify this file

#define PRODUCT_NAME            "Unvanquished"
#define PRODUCT_NAME_UPPER      "UNVANQUISHED" // Case, No spaces
#define PRODUCT_NAME_LOWER      "unvanquished" // No case, No spaces
#define PRODUCT_VERSION         "0.47"

#define ENGINE_NAME             "Daemon Engine"
#define ENGINE_VERSION          PRODUCT_VERSION

#ifdef REVISION
# define Q3_VERSION             PRODUCT_NAME " " PRODUCT_VERSION " " REVISION
#else
# define Q3_VERSION             PRODUCT_NAME " " PRODUCT_VERSION
#endif

#define Q3_ENGINE               ENGINE_NAME " " ENGINE_VERSION
#define Q3_ENGINE_DATE          __DATE__

#define CLIENT_WINDOW_TITLE     PRODUCT_NAME
#define CLIENT_WINDOW_MIN_TITLE PRODUCT_NAME_LOWER
#define GAMENAME_FOR_MASTER     PRODUCT_NAME_UPPER


#define AUTOEXEC_NAME           "autoexec.cfg"

#define CONFIG_NAME             "autogen.cfg"
#define KEYBINDINGS_NAME        "keybindings.cfg"
#define TEAMCONFIG_NAME         "teamconfig.cfg"
#define SERVERCONFIG_NAME       "autogen_server.cfg"

#define UNNAMED_PLAYER "UnnamedPlayer"

#define Q_UNUSED(x) (void)(x)

template<class T>
void ignore_result(T) {}

#define EXTERN_C extern "C"

// C standard library headers
#include <errno.h>
//#include <fenv.h>
#include <float.h>
#include <iso646.h>
#include <limits.h>
#include <locale.h>
#include <math.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>

// C++ standard library headers
#ifdef __cplusplus
#include <utility>
#include <functional>
#include <chrono>
#include <type_traits>
#include <initializer_list>
#include <tuple>
#include <new>
#include <memory>
#include <limits>
#include <exception>
#include <stdexcept>
#include <system_error>
#include <string>
#include <vector>
#include <array>
#include <list>
#include <forward_list>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>
#include <stack>
#include <queue>
#include <algorithm>
#include <iterator>
#include <random>
#include <numeric>
#include <thread>
#include <atomic>
#include <mutex>
#include <condition_variable>
#include <valarray>
#include <sstream>
#include <iostream>
#endif // __cplusplus

// vsnprintf is ISO/IEC 9899:1999
// abstracting this to make it portable
#ifdef _MSC_VER
//vsnprintf is non-conformant in MSVC--fails to null-terminate in case of overflow
#define Q_vsnprintf(dest, size, fmt, args) _vsnprintf_s( dest, size, _TRUNCATE, fmt, args )
#define Q_snprintf(dest, size, fmt, ...) _snprintf_s( dest, size, _TRUNCATE, fmt, __VA_ARGS__ )
#elif defined( _WIN32 )
#define Q_vsnprintf _vsnprintf
#define Q_snprintf  _snprintf
#else
#define Q_vsnprintf vsnprintf
#define Q_snprintf  snprintf
#endif

// msvc does not have roundf
#ifdef _MSC_VER
#define roundf( f ) ( floor( (f) + 0.5 ) )
#endif

//=============================================================

	using byte = uint8_t;
	using uint = unsigned int;
	enum qtrinary {qno, qyes, qmaybe};

	union floatint_t
	{
		float f;
		int i;
		uint ui;
	};

//=============================================================

#include "common/Platform.h"
#include "common/Compiler.h"
#include "common/Endian.h"

using qhandle_t = int;
using sfxHandle_t = int;
using fileHandle_t = int;
using clipHandle_t = int;

#define PAD(x,y)                ((( x ) + ( y ) - 1 ) & ~(( y ) - 1 ))
#define PADLEN(base, alignment) ( PAD(( base ), ( alignment )) - ( base ))
#define PADP(base, alignment)   ((void *) PAD((intptr_t) ( base ), ( alignment )))

#define STRING(s)  #s
// expand constants before stringifying them
#define XSTRING(s) STRING(s)

#define MAX_QINT 0x7fffffff
#define MIN_QINT ( -MAX_QINT - 1 )

#define HUGE_QFLT 3e38f // TODO: Replace HUGE_QFLT with MAX_QFLT

#ifndef BIT
#define BIT(x) ( 1 << ( x ) )
#endif

// the game guarantees that no string from the network will ever
// exceed MAX_STRING_CHARS
#define MAX_STRING_CHARS  1024 // max length of a string passed to Cmd_TokenizeString
#define MAX_STRING_TOKENS 256 // max tokens resulting from Cmd_TokenizeString
#define MAX_TOKEN_CHARS   1024 // max length of an individual token

#define MAX_ADDR_CHARS    sizeof("[1111:2222:3333:4444:5555:6666:7777:8888]:99999")

#define MAX_INFO_STRING   1024
#define MAX_INFO_KEY      1024
#define MAX_INFO_VALUE    1024

#define BIG_INFO_STRING   8192 // used for system info key only
#define BIG_INFO_KEY      8192
#define BIG_INFO_VALUE    8192

#define MAX_QPATH         64 // max length of a quake game pathname
#define MAX_OSPATH        256 // max length of a filesystem pathname
#define MAX_CMD           1024 // max length of a command line

// rain - increased to 36 to match MAX_NETNAME, fixes #13 - UI stuff breaks
// with very long names
#define MAX_NAME_LENGTH    128 // max length of a client name, in bytes

#define MAX_SAY_TEXT       400

#define MAX_BINARY_MESSAGE 32768 // max length of binary message

	enum messageStatus_t
	{
	  MESSAGE_EMPTY = 0,
	  MESSAGE_WAITING, // rate/packet limited
	  MESSAGE_WAITING_OVERFLOW, // packet too large with message
	};

//
// these aren't needed by any of the VMs.  put in another header?
//
#define MAX_MAP_AREA_BYTES 32 // bit vector of area visibility

	enum ha_pref
	{
	  h_high,
	  h_low,
	  h_dontcare
	};

	void *Hunk_Alloc( int size, ha_pref preference );

#define Com_Memset   memset
#define Com_Memcpy   memcpy

#define Com_Allocate malloc
#define Com_Dealloc  free

void *Com_Allocate_Aligned( size_t alignment, size_t size );
void  Com_Free_Aligned( void *ptr );

#define CIN_system   1
#define CIN_loop     2
#define CIN_hold     4
#define CIN_silent   8
#define CIN_shader   16

	/*
	==============================================================

	MATHLIB

	==============================================================
	*/

	using vec_t = float;
	using vec2_t = vec_t[2];

	using vec3_t = vec_t[3];
	using vec4_t = vec_t[4];

	using axis_t = vec3_t[3];
	using matrix3x3_t = vec_t[3 * 3];
	using matrix_t = vec_t[4 * 4];
	using quat_t = vec_t[4];

	// A transform_t represents a product of basic
	// transformations, which are a rotation about an arbitrary
	// axis, a uniform scale or a translation. Any a product can
	// alway be brought into the form rotate, then scale, then
	// translate. So the whole transform_t can be stored in 8
	// floats (quat: 4, scale: 1, translation: 3), which is very
	// convenient for SSE and GLSL, which operate on 4-dimensional
	// float vectors.
#if idx86_sse
    // Here we have a union of scalar struct and sse struct, transform_u and the
    // scalar struct must match transform_t so we have to use anonymous structs.
    // We disable compiler warnings when using -Wpedantic for this specific case.
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif
	ALIGNED(16, union transform_t {
		struct {
			quat_t rot;
			vec3_t trans;
			vec_t  scale;
		};
		struct {
			__m128 sseRot;
			__m128 sseTransScale;
		};
	});
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
#else
	ALIGNED(16, struct transform_t {
		quat_t rot;
		vec3_t trans;
		vec_t  scale;
	});
#endif

	using fixed4_t = int;
	using fixed8_t = int;
	using fixed16_t = int;

#ifndef M_PI
#define M_PI 3.14159265358979323846f // matches value in gcc v2 math.h
#endif

#ifndef M_SQRT2
#define M_SQRT2 1.414213562f
#endif

#ifndef M_ROOT3
#define M_ROOT3 1.732050808f
#endif

#ifndef LINE_DISTANCE_EPSILON
#define LINE_DISTANCE_EPSILON 1e-05f
#endif

#define ARRAY_LEN(x) ( sizeof( x ) / sizeof( *( x ) ) )

// angle indexes
#define PITCH 0 // up / down
#define YAW   1 // left / right
#define ROLL  2 // fall over

// plane sides
	enum planeSide_t
	{
	  SIDE_FRONT = 0,
	  SIDE_BACK = 1,
	  SIDE_ON = 2,
	  SIDE_CROSS = 3
	};

#define NUMVERTEXNORMALS 162
	extern vec3_t bytedirs[ NUMVERTEXNORMALS ];

// all drawing is done to a 640*480 virtual screen size
// and will be automatically scaled to the real resolution
#define SCREEN_WIDTH     640
#define SCREEN_HEIGHT    480

#define TINYCHAR_WIDTH   ( SMALLCHAR_WIDTH )
#define TINYCHAR_HEIGHT  ( SMALLCHAR_HEIGHT )

#define MINICHAR_WIDTH   8
#define MINICHAR_HEIGHT  12

#define SMALLCHAR_WIDTH  8
#define SMALLCHAR_HEIGHT 16

#define BIGCHAR_WIDTH    16
#define BIGCHAR_HEIGHT   16

#define GIANTCHAR_WIDTH  32
#define GIANTCHAR_HEIGHT 48

#define GAME_INIT_FRAMES 6
#define FRAMETIME        100 // msec

#include "logging.h"

#define DEG2RAD( a )                  ( ( ( a ) * M_PI ) / 180.0F )
#define RAD2DEG( a )                  ( ( ( a ) * 180.0f ) / M_PI )

#define Q_clamp( a, b, c )            Math::Clamp( (a), (b), (c) )
#define Q_lerp( from, to, frac )      ( ( from ) + ( frac ) * ( ( to ) - ( from ) ) )

struct cplane_t;

extern vec3_t   vec3_origin;
extern vec3_t   axisDefault[ 3 ];
extern matrix_t matrixIdentity;
extern quat_t   quatIdentity;

#define nanmask ( 255 << 23 )

#define IS_NAN( x ) ( ( ( *(int *)&( x ) ) & nanmask ) == nanmask )

#define Q_ftol(x) ((long)(x))

	inline unsigned int Q_floatBitsToUint( float number )
	{
		floatint_t t;

		t.f = number;
		return t.ui;
	}

	inline float Q_uintBitsToFloat( unsigned int number )
	{
		floatint_t t;

		t.ui = number;
		return t.f;
	}

	inline float Q_rsqrt( float number )
	{
		float x = 0.5f * number;
		float y;

		Q_UNUSED(x);

		// compute approximate inverse square root
#if defined( idx86_sse )
		_mm_store_ss( &y, _mm_rsqrt_ss( _mm_load_ss( &number ) ) );
#elif idppc

#ifdef __GNUC__
		asm( "frsqrte %0, %1" : "=f"( y ) : "f"( number ) );
#else
		y = __frsqrte( number );
#endif
#else
		y = Q_uintBitsToFloat( 0x5f3759df - (Q_floatBitsToUint( number ) >> 1) );
		y *= ( 1.5f - ( x * y * y ) ); // initial iteration
#endif
		y *= ( 1.5f - ( x * y * y ) ); // second iteration for higher precision
		return y;
	}

inline float Q_fabs( float x )
{
	return fabsf( x );
}

byte         ClampByte( int i );
signed char  ClampChar( int i );

// this isn't a real cheap function to call!
int          DirToByte( vec3_t dir );
void         ByteToDir( int b, vec3_t dir );

#define DotProduct( x,y )            ( ( x )[ 0 ] * ( y )[ 0 ] + ( x )[ 1 ] * ( y )[ 1 ] + ( x )[ 2 ] * ( y )[ 2 ] )
#define VectorSubtract( a,b,c )      ( ( c )[ 0 ] = ( a )[ 0 ] - ( b )[ 0 ],( c )[ 1 ] = ( a )[ 1 ] - ( b )[ 1 ],( c )[ 2 ] = ( a )[ 2 ] - ( b )[ 2 ] )
#define VectorAdd( a,b,c )           ( ( c )[ 0 ] = ( a )[ 0 ] + ( b )[ 0 ],( c )[ 1 ] = ( a )[ 1 ] + ( b )[ 1 ],( c )[ 2 ] = ( a )[ 2 ] + ( b )[ 2 ] )
#define VectorCopy( a,b )            ( ( b )[ 0 ] = ( a )[ 0 ],( b )[ 1 ] = ( a )[ 1 ],( b )[ 2 ] = ( a )[ 2 ] )
#define VectorScale( v, s, o )       ( ( o )[ 0 ] = ( v )[ 0 ] * ( s ),( o )[ 1 ] = ( v )[ 1 ] * ( s ),( o )[ 2 ] = ( v )[ 2 ] * ( s ) )
#define VectorMA( v, s, b, o )       ( ( o )[ 0 ] = ( v )[ 0 ] + ( b )[ 0 ] * ( s ),( o )[ 1 ] = ( v )[ 1 ] + ( b )[ 1 ] * ( s ),( o )[ 2 ] = ( v )[ 2 ] + ( b )[ 2 ] * ( s ) )
#define VectorLerpTrem( f, s, e, r ) (( r )[ 0 ] = ( s )[ 0 ] + ( f ) * (( e )[ 0 ] - ( s )[ 0 ] ), \
                                      ( r )[ 1 ] = ( s )[ 1 ] + ( f ) * (( e )[ 1 ] - ( s )[ 1 ] ), \
                                      ( r )[ 2 ] = ( s )[ 2 ] + ( f ) * (( e )[ 2 ] - ( s )[ 2 ] ))

#define VectorClear( a )             ( ( a )[ 0 ] = ( a )[ 1 ] = ( a )[ 2 ] = 0 )
#define VectorNegate( a,b )          ( ( b )[ 0 ] = -( a )[ 0 ],( b )[ 1 ] = -( a )[ 1 ],( b )[ 2 ] = -( a )[ 2 ] )
#define VectorSet( v, x, y, z )      ( ( v )[ 0 ] = ( x ), ( v )[ 1 ] = ( y ), ( v )[ 2 ] = ( z ) )

#define Vector2Set( v, x, y )        ( ( v )[ 0 ] = ( x ),( v )[ 1 ] = ( y ) )
#define Vector2Copy( a,b )           ( ( b )[ 0 ] = ( a )[ 0 ],( b )[ 1 ] = ( a )[ 1 ] )
#define Vector2Subtract( a,b,c )     ( ( c )[ 0 ] = ( a )[ 0 ] - ( b )[ 0 ],( c )[ 1 ] = ( a )[ 1 ] - ( b )[ 1 ] )

#define Vector4Set( v, x, y, z, n )  ( ( v )[ 0 ] = ( x ),( v )[ 1 ] = ( y ),( v )[ 2 ] = ( z ),( v )[ 3 ] = ( n ) )
#define Vector4Copy( a,b )           ( ( b )[ 0 ] = ( a )[ 0 ],( b )[ 1 ] = ( a )[ 1 ],( b )[ 2 ] = ( a )[ 2 ],( b )[ 3 ] = ( a )[ 3 ] )
#define Vector4MA( v, s, b, o )      ( ( o )[ 0 ] = ( v )[ 0 ] + ( b )[ 0 ] * ( s ),( o )[ 1 ] = ( v )[ 1 ] + ( b )[ 1 ] * ( s ),( o )[ 2 ] = ( v )[ 2 ] + ( b )[ 2 ] * ( s ),( o )[ 3 ] = ( v )[ 3 ] + ( b )[ 3 ] * ( s ) )
#define Vector4Average( v, b, s, o ) ( ( o )[ 0 ] = ( ( v )[ 0 ] * ( 1 - ( s ) ) ) + ( ( b )[ 0 ] * ( s ) ),( o )[ 1 ] = ( ( v )[ 1 ] * ( 1 - ( s ) ) ) + ( ( b )[ 1 ] * ( s ) ),( o )[ 2 ] = ( ( v )[ 2 ] * ( 1 - ( s ) ) ) + ( ( b )[ 2 ] * ( s ) ),( o )[ 3 ] = ( ( v )[ 3 ] * ( 1 - ( s ) ) ) + ( ( b )[ 3 ] * ( s ) ) )

#define DotProduct4(x, y)            (( x )[ 0 ] * ( y )[ 0 ] + ( x )[ 1 ] * ( y )[ 1 ] + ( x )[ 2 ] * ( y )[ 2 ] + ( x )[ 3 ] * ( y )[ 3 ] )

#define SnapVector( v )              do { ( v )[ 0 ] = ( floor( ( v )[ 0 ] + 0.5f ) ); ( v )[ 1 ] = ( floor( ( v )[ 1 ] + 0.5f ) ); ( v )[ 2 ] = ( floor( ( v )[ 2 ] + 0.5f ) ); } while ( 0 )

	float    RadiusFromBounds( const vec3_t mins, const vec3_t maxs );
	void     ZeroBounds( vec3_t mins, vec3_t maxs );
	void     ClearBounds( vec3_t mins, vec3_t maxs );
	void     AddPointToBounds( const vec3_t v, vec3_t mins, vec3_t maxs );

	void     BoundsAdd( vec3_t mins, vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 );
	bool BoundsIntersect( const vec3_t mins, const vec3_t maxs, const vec3_t mins2, const vec3_t maxs2 );
	bool BoundsIntersectSphere( const vec3_t mins, const vec3_t maxs, const vec3_t origin, vec_t radius );
	bool BoundsIntersectPoint( const vec3_t mins, const vec3_t maxs, const vec3_t origin );
	float BoundsMaxExtent( const vec3_t mins, const vec3_t maxs );

	inline void BoundsToCorners( const vec3_t mins, const vec3_t maxs, vec3_t corners[ 8 ] )
	{
		VectorSet( corners[ 0 ], mins[ 0 ], maxs[ 1 ], maxs[ 2 ] );
		VectorSet( corners[ 1 ], maxs[ 0 ], maxs[ 1 ], maxs[ 2 ] );
		VectorSet( corners[ 2 ], maxs[ 0 ], mins[ 1 ], maxs[ 2 ] );
		VectorSet( corners[ 3 ], mins[ 0 ], mins[ 1 ], maxs[ 2 ] );
		VectorSet( corners[ 4 ], mins[ 0 ], maxs[ 1 ], mins[ 2 ] );
		VectorSet( corners[ 5 ], maxs[ 0 ], maxs[ 1 ], mins[ 2 ] );
		VectorSet( corners[ 6 ], maxs[ 0 ], mins[ 1 ], mins[ 2 ] );
		VectorSet( corners[ 7 ], mins[ 0 ], mins[ 1 ], mins[ 2 ] );
	}

	int VectorCompare( const vec3_t v1, const vec3_t v2 );

	inline int Vector4Compare( const vec4_t v1, const vec4_t v2 )
	{
		if ( v1[ 0 ] != v2[ 0 ] || v1[ 1 ] != v2[ 1 ] || v1[ 2 ] != v2[ 2 ] || v1[ 3 ] != v2[ 3 ] )
		{
			return 0;
		}

		return 1;
	}

	inline void VectorLerp( const vec3_t from, const vec3_t to, float frac, vec3_t out )
	{
		out[ 0 ] = from[ 0 ] + ( ( to[ 0 ] - from[ 0 ] ) * frac );
		out[ 1 ] = from[ 1 ] + ( ( to[ 1 ] - from[ 1 ] ) * frac );
		out[ 2 ] = from[ 2 ] + ( ( to[ 2 ] - from[ 2 ] ) * frac );
	}

	inline int VectorCompareEpsilon( const vec3_t v1, const vec3_t v2, float epsilon )
	{
		vec3_t d;

		VectorSubtract( v1, v2, d );
		d[ 0 ] = fabs( d[ 0 ] );
		d[ 1 ] = fabs( d[ 1 ] );
		d[ 2 ] = fabs( d[ 2 ] );

		if ( d[ 0 ] > epsilon || d[ 1 ] > epsilon || d[ 2 ] > epsilon )
		{
			return 0;
		}

		return 1;
	}

	vec_t VectorLength( const vec3_t v );
	vec_t VectorLengthSquared( const vec3_t v );
	vec_t Distance( const vec3_t p1, const vec3_t p2 );
	vec_t DistanceSquared( const vec3_t p1, const vec3_t p2 );
	void  CrossProduct( const vec3_t v1, const vec3_t v2, vec3_t cross );
	vec_t VectorNormalize( vec3_t v );  // returns vector length
	void  VectorNormalizeFast( vec3_t v );  // does NOT return vector length, uses rsqrt approximation
	vec_t VectorNormalize2( const vec3_t v, vec3_t out );
	void  VectorInverse( vec3_t v );

	int   NearestPowerOfTwo( int val );

	int   Q_rand( int *seed );
	float Q_random( int *seed );
	float Q_crandom( int *seed );

#define random()  ( ( rand() & 0x7fff ) / ( (float)0x7fff ) )
#define crandom() ( 2.0 * ( random() - 0.5 ) )

	void vectoangles( const vec3_t value1, vec3_t angles );

	void  AnglesToAxis( const vec3_t angles, vec3_t axis[ 3 ] );
// TTimo: const vec_t ** would require explicit casts for ANSI C conformance
// see unix/const-arg.c
	void  AxisToAngles( /*const*/ vec3_t axis[ 3 ], vec3_t angles );
//void AxisToAngles ( const vec3_t axis[3], vec3_t angles );
	float VectorDistanceSquared( vec3_t v1, vec3_t v2 );

	void  AxisClear( vec3_t axis[ 3 ] );
	void  AxisCopy( vec3_t in[ 3 ], vec3_t out[ 3 ] );

	void  SetPlaneSignbits( struct cplane_t *out );
	int   BoxOnPlaneSide( const vec3_t emins, const vec3_t emaxs, const struct cplane_t *plane );

	float AngleMod( float a );
	float LerpAngle( float from, float to, float frac );
	float AngleSubtract( float a1, float a2 );
	void  AnglesSubtract( vec3_t v1, vec3_t v2, vec3_t v3 );

	float AngleNormalize360( float angle );
	float AngleNormalize180( float angle );
	float AngleDelta( float angle1, float angle2 );
	float AngleBetweenVectors( const vec3_t a, const vec3_t b );
	void  AngleVectors( const vec3_t angles, vec3_t forward, vec3_t right, vec3_t up );

	vec_t PlaneNormalize( vec4_t plane );  // returns normal length

	/* greebo: This calculates the intersection point of three planes.
	 * Returns <0,0,0> if no intersection point could be found, otherwise returns the coordinates of the intersection point
	 * (this may also be 0,0,0) */
	bool PlanesGetIntersectionPoint( const vec4_t plane1, const vec4_t plane2, const vec4_t plane3, vec3_t out );
	void     PlaneIntersectRay( const vec3_t rayPos, const vec3_t rayDir, const vec4_t plane, vec3_t res );

	bool PlaneFromPoints( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c );
	bool PlaneFromPointsOrder( vec4_t plane, const vec3_t a, const vec3_t b, const vec3_t c, bool cw );
	void     ProjectPointOnPlane( vec3_t dst, const vec3_t point, const vec3_t normal );
	void     RotatePointAroundVector( vec3_t dst, const vec3_t dir, const vec3_t point, float degrees );

	void     RotateAroundDirection( vec3_t axis[ 3 ], float yaw );
	void     MakeNormalVectors( const vec3_t forward, vec3_t right, vec3_t up );

	float    ProjectPointOntoRectangleOutwards( vec2_t out, const vec2_t point, const vec2_t dir, const vec2_t bounds[ 2 ] );
	void     ExponentialFade( float *value, float target, float lambda, float timedelta );
	#define  LinearRemap(x,an,ap,bn,bp) (((x)-(an))/((ap)-(an))*((bp)-(bn))+(bn))

// perpendicular vector could be replaced by this

//int       PlaneTypeForNormal( vec3_t normal );

	void VectorMatrixMultiply( const vec3_t p, vec3_t m[ 3 ], vec3_t out );

// RB: NOTE renamed MatrixMultiply to AxisMultiply because it conflicts with most new matrix functions
// It is important for mod developers to do this change as well or they risk a memory corruption by using
// the other MatrixMultiply function.
	void  AxisMultiply( float in1[ 3 ][ 3 ], float in2[ 3 ][ 3 ], float out[ 3 ][ 3 ] );
	void  PerpendicularVector( vec3_t dst, const vec3_t src );

// Ridah
	void  GetPerpendicularViewVector( const vec3_t point, const vec3_t p1, const vec3_t p2, vec3_t up );
	void  ProjectPointOntoVector( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj );
	void  ProjectPointOntoVectorBounded( vec3_t point, vec3_t vStart, vec3_t vEnd, vec3_t vProj );
	float DistanceFromLineSquared( vec3_t p, vec3_t lp1, vec3_t lp2 );

// done.

	vec_t DistanceBetweenLineSegmentsSquared( const vec3_t sP0, const vec3_t sP1,
	    const vec3_t tP0, const vec3_t tP1, float *s, float *t );

//=============================================

// RB: XreaL matrix math functions required by the renderer

	void     MatrixIdentity( matrix_t m );
	void     MatrixClear( matrix_t m );
	void     MatrixCopy( const matrix_t in, matrix_t out );
	bool MatrixCompare( const matrix_t a, const matrix_t b );
	void     MatrixTransposeIntoXMM( const matrix_t m );
	void     MatrixTranspose( const matrix_t in, matrix_t out );

// invert any m4x4 using Kramer's rule.. return true if matrix is singular, else return false
	bool MatrixInverse( matrix_t m );
	void     MatrixSetupXRotation( matrix_t m, vec_t degrees );
	void     MatrixSetupYRotation( matrix_t m, vec_t degrees );
	void     MatrixSetupZRotation( matrix_t m, vec_t degrees );
	void     MatrixSetupTranslation( matrix_t m, vec_t x, vec_t y, vec_t z );
	void     MatrixSetupScale( matrix_t m, vec_t x, vec_t y, vec_t z );
	void     MatrixSetupShear( matrix_t m, vec_t x, vec_t y );
	void     MatrixMultiply( const matrix_t a, const matrix_t b, matrix_t out );
	void     MatrixMultiply2( matrix_t m, const matrix_t m2 );
	void     MatrixMultiplyRotation( matrix_t m, vec_t pitch, vec_t yaw, vec_t roll );
	void     MatrixMultiplyZRotation( matrix_t m, vec_t degrees );
	void     MatrixMultiplyTranslation( matrix_t m, vec_t x, vec_t y, vec_t z );
	void     MatrixMultiplyScale( matrix_t m, vec_t x, vec_t y, vec_t z );
	void     MatrixMultiplyShear( matrix_t m, vec_t x, vec_t y );
	void     MatrixToAngles( const matrix_t m, vec3_t angles );
	void     MatrixFromAngles( matrix_t m, vec_t pitch, vec_t yaw, vec_t roll );
	void     MatrixFromVectorsFLU( matrix_t m, const vec3_t forward, const vec3_t left, const vec3_t up );
	void     MatrixFromVectorsFRU( matrix_t m, const vec3_t forward, const vec3_t right, const vec3_t up );
	void     MatrixFromQuat( matrix_t m, const quat_t q );
	void     MatrixFromPlanes( matrix_t m, const vec4_t left, const vec4_t right, const vec4_t bottom, const vec4_t top,
	                           const vec4_t near, const vec4_t far );
	void     MatrixToVectorsFLU( const matrix_t m, vec3_t forward, vec3_t left, vec3_t up );
	void     MatrixToVectorsFRU( const matrix_t m, vec3_t forward, vec3_t right, vec3_t up );
	void     MatrixSetupTransformFromVectorsFLU( matrix_t m, const vec3_t forward, const vec3_t left, const vec3_t up, const vec3_t origin );
	void     MatrixSetupTransformFromVectorsFRU( matrix_t m, const vec3_t forward, const vec3_t right, const vec3_t up, const vec3_t origin );
	void     MatrixSetupTransformFromRotation( matrix_t m, const matrix_t rot, const vec3_t origin );
	void     MatrixSetupTransformFromQuat( matrix_t m, const quat_t quat, const vec3_t origin );
	void     MatrixAffineInverse( const matrix_t in, matrix_t out );
	void     MatrixTransformNormal( const matrix_t m, const vec3_t in, vec3_t out );
	void     MatrixTransformNormal2( const matrix_t m, vec3_t inout );
	void     MatrixTransformPoint( const matrix_t m, const vec3_t in, vec3_t out );
	void     MatrixTransformPoint2( const matrix_t m, vec3_t inout );
	void     MatrixTransform4( const matrix_t m, const vec4_t in, vec4_t out );
	void     MatrixTransformPlane( const matrix_t m, const vec4_t in, vec4_t out );
	void     MatrixTransformPlane2( const matrix_t m, vec3_t inout );
	void     MatrixPerspectiveProjection( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far );
	void     MatrixPerspectiveProjectionLH( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far );
	void     MatrixPerspectiveProjectionRH( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far );
	void     MatrixPerspectiveProjectionFovYAspectLH( matrix_t m, vec_t fov, vec_t aspect, vec_t near, vec_t far );
	void     MatrixPerspectiveProjectionFovXYLH( matrix_t m, vec_t fovX, vec_t fovY, vec_t near, vec_t far );
	void     MatrixPerspectiveProjectionFovXYRH( matrix_t m, vec_t fovX, vec_t fovY, vec_t near, vec_t far );
	void     MatrixPerspectiveProjectionFovXYInfiniteRH( matrix_t m, vec_t fovX, vec_t fovY, vec_t near );
	void     MatrixOrthogonalProjection( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far );

	void     MatrixOrthogonalProjectionLH( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far );
	void     MatrixOrthogonalProjectionRH( matrix_t m, vec_t left, vec_t right, vec_t bottom, vec_t top, vec_t near, vec_t far );

	void     MatrixPlaneReflection( matrix_t m, const vec4_t plane );

	void     MatrixLookAtLH( matrix_t output, const vec3_t pos, const vec3_t dir, const vec3_t up );
	void     MatrixLookAtRH( matrix_t m, const vec3_t eye, const vec3_t dir, const vec3_t up );
	void     MatrixScaleTranslateToUnitCube( matrix_t m, const vec3_t mins, const vec3_t maxs );
	void     MatrixCrop( matrix_t m, const vec3_t mins, const vec3_t maxs );

	inline void AnglesToMatrix( const vec3_t angles, matrix_t m )
	{
		MatrixFromAngles( m, angles[ PITCH ], angles[ YAW ], angles[ ROLL ] );
	}

//=============================================

// RB: XreaL quaternion math functions required by the renderer

#define QuatSet(q,x,y,z,w) (( q )[ 0 ] = ( x ),( q )[ 1 ] = ( y ),( q )[ 2 ] = ( z ),( q )[ 3 ] = ( w ))
#define QuatCopy(a,b)      (( b )[ 0 ] = ( a )[ 0 ],( b )[ 1 ] = ( a )[ 1 ],( b )[ 2 ] = ( a )[ 2 ],( b )[ 3 ] = ( a )[ 3 ] )

#define QuatCompare(a,b)   (( a )[ 0 ] == ( b )[ 0 ] && ( a )[ 1 ] == ( b )[ 1 ] && ( a )[ 2 ] == ( b )[ 2 ] && ( a )[ 3 ] == ( b )[ 3 ] )

	inline void QuatClear( quat_t q )
	{
		q[ 0 ] = 0;
		q[ 1 ] = 0;
		q[ 2 ] = 0;
		q[ 3 ] = 1;
	}

	inline void QuatZero( quat_t o )
	{
		o[ 0 ] = 0.0f;
		o[ 1 ] = 0.0f;
		o[ 2 ] = 0.0f;
		o[ 3 ] = 0.0f;
	}

	inline void QuatAdd( const quat_t p, const quat_t q,
				    quat_t o )
	{
		o[ 0 ] = p[ 0 ] + q[ 0 ];
		o[ 1 ] = p[ 1 ] + q[ 1 ];
		o[ 2 ] = p[ 2 ] + q[ 2 ];
		o[ 3 ] = p[ 3 ] + q[ 3 ];
	}

	inline void QuatMA( const quat_t p, float f, const quat_t q,
				   quat_t o )
	{
		o[ 0 ] = p[ 0 ] + f * q[ 0 ];
		o[ 1 ] = p[ 1 ] + f * q[ 1 ];
		o[ 2 ] = p[ 2 ] + f * q[ 2 ];
		o[ 3 ] = p[ 3 ] + f * q[ 3 ];
	}

	inline void QuatCalcW( quat_t q )
	{
		vec_t term = 1.0f - ( q[ 0 ] * q[ 0 ] + q[ 1 ] * q[ 1 ] + q[ 2 ] * q[ 2 ] );

		if ( term < 0.0 )
		{
			q[ 3 ] = 0.0;
		}
		else
		{
			q[ 3 ] = -sqrt( term );
		}
	}

	inline void QuatInverse( quat_t q )
	{
		q[ 0 ] = -q[ 0 ];
		q[ 1 ] = -q[ 1 ];
		q[ 2 ] = -q[ 2 ];
	}

	inline void QuatAntipodal( quat_t q )
	{
		q[ 0 ] = -q[ 0 ];
		q[ 1 ] = -q[ 1 ];
		q[ 2 ] = -q[ 2 ];
		q[ 3 ] = -q[ 3 ];
	}

	inline vec_t QuatLength( const quat_t q )
	{
		return ( vec_t ) sqrt( q[ 0 ] * q[ 0 ] + q[ 1 ] * q[ 1 ] + q[ 2 ] * q[ 2 ] + q[ 3 ] * q[ 3 ] );
	}

	vec_t QuatNormalize( quat_t q );

	void  QuatFromAngles( quat_t q, vec_t pitch, vec_t yaw, vec_t roll );

	inline void AnglesToQuat( const vec3_t angles, quat_t q )
	{
		QuatFromAngles( q, angles[ PITCH ], angles[ YAW ], angles[ ROLL ] );
	}

	void QuatFromMatrix( quat_t q, const matrix_t m );
	void QuatToVectorsFLU( const quat_t quat, vec3_t forward, vec3_t left, vec3_t up );
	void QuatToVectorsFRU( const quat_t quat, vec3_t forward, vec3_t right, vec3_t up );

	void QuatToAxis( const quat_t q, vec3_t axis[ 3 ] );
	void QuatToAngles( const quat_t q, vec3_t angles );

// Quaternion multiplication, analogous to the matrix multiplication routines.

// qa = rotate by qa, then qb
	void QuatMultiply0( quat_t qa, const quat_t qb );

// qc = rotate by qa, then qb
	void QuatMultiply1( const quat_t qa, const quat_t qb, quat_t qc );

// qc = rotate by qa, then by inverse of qb
	void QuatMultiply2( const quat_t qa, const quat_t qb, quat_t qc );

// qc = rotate by inverse of qa, then by qb
	void QuatMultiply3( const quat_t qa, const quat_t qb, quat_t qc );

// qc = rotate by inverse of qa, then by inverse of qb
	void QuatMultiply4( const quat_t qa, const quat_t qb, quat_t qc );

	void QuatSlerp( const quat_t from, const quat_t to, float frac, quat_t out );
	void QuatTransformVector( const quat_t q, const vec3_t in, vec3_t out );
	void QuatTransformVectorInverse( const quat_t q, const vec3_t in, vec3_t out );

//=============================================
// combining Transformations

#if idx86_sse
/* swizzles for _mm_shuffle_ps instruction */
#define SWZ_XXXX 0x00
#define SWZ_YXXX 0x01
#define SWZ_ZXXX 0x02
#define SWZ_WXXX 0x03
#define SWZ_XYXX 0x04
#define SWZ_YYXX 0x05
#define SWZ_ZYXX 0x06
#define SWZ_WYXX 0x07
#define SWZ_XZXX 0x08
#define SWZ_YZXX 0x09
#define SWZ_ZZXX 0x0a
#define SWZ_WZXX 0x0b
#define SWZ_XWXX 0x0c
#define SWZ_YWXX 0x0d
#define SWZ_ZWXX 0x0e
#define SWZ_WWXX 0x0f
#define SWZ_XXYX 0x10
#define SWZ_YXYX 0x11
#define SWZ_ZXYX 0x12
#define SWZ_WXYX 0x13
#define SWZ_XYYX 0x14
#define SWZ_YYYX 0x15
#define SWZ_ZYYX 0x16
#define SWZ_WYYX 0x17
#define SWZ_XZYX 0x18
#define SWZ_YZYX 0x19
#define SWZ_ZZYX 0x1a
#define SWZ_WZYX 0x1b
#define SWZ_XWYX 0x1c
#define SWZ_YWYX 0x1d
#define SWZ_ZWYX 0x1e
#define SWZ_WWYX 0x1f
#define SWZ_XXZX 0x20
#define SWZ_YXZX 0x21
#define SWZ_ZXZX 0x22
#define SWZ_WXZX 0x23
#define SWZ_XYZX 0x24
#define SWZ_YYZX 0x25
#define SWZ_ZYZX 0x26
#define SWZ_WYZX 0x27
#define SWZ_XZZX 0x28
#define SWZ_YZZX 0x29
#define SWZ_ZZZX 0x2a
#define SWZ_WZZX 0x2b
#define SWZ_XWZX 0x2c
#define SWZ_YWZX 0x2d
#define SWZ_ZWZX 0x2e
#define SWZ_WWZX 0x2f
#define SWZ_XXWX 0x30
#define SWZ_YXWX 0x31
#define SWZ_ZXWX 0x32
#define SWZ_WXWX 0x33
#define SWZ_XYWX 0x34
#define SWZ_YYWX 0x35
#define SWZ_ZYWX 0x36
#define SWZ_WYWX 0x37
#define SWZ_XZWX 0x38
#define SWZ_YZWX 0x39
#define SWZ_ZZWX 0x3a
#define SWZ_WZWX 0x3b
#define SWZ_XWWX 0x3c
#define SWZ_YWWX 0x3d
#define SWZ_ZWWX 0x3e
#define SWZ_WWWX 0x3f
#define SWZ_XXXY 0x40
#define SWZ_YXXY 0x41
#define SWZ_ZXXY 0x42
#define SWZ_WXXY 0x43
#define SWZ_XYXY 0x44
#define SWZ_YYXY 0x45
#define SWZ_ZYXY 0x46
#define SWZ_WYXY 0x47
#define SWZ_XZXY 0x48
#define SWZ_YZXY 0x49
#define SWZ_ZZXY 0x4a
#define SWZ_WZXY 0x4b
#define SWZ_XWXY 0x4c
#define SWZ_YWXY 0x4d
#define SWZ_ZWXY 0x4e
#define SWZ_WWXY 0x4f
#define SWZ_XXYY 0x50
#define SWZ_YXYY 0x51
#define SWZ_ZXYY 0x52
#define SWZ_WXYY 0x53
#define SWZ_XYYY 0x54
#define SWZ_YYYY 0x55
#define SWZ_ZYYY 0x56
#define SWZ_WYYY 0x57
#define SWZ_XZYY 0x58
#define SWZ_YZYY 0x59
#define SWZ_ZZYY 0x5a
#define SWZ_WZYY 0x5b
#define SWZ_XWYY 0x5c
#define SWZ_YWYY 0x5d
#define SWZ_ZWYY 0x5e
#define SWZ_WWYY 0x5f
#define SWZ_XXZY 0x60
#define SWZ_YXZY 0x61
#define SWZ_ZXZY 0x62
#define SWZ_WXZY 0x63
#define SWZ_XYZY 0x64
#define SWZ_YYZY 0x65
#define SWZ_ZYZY 0x66
#define SWZ_WYZY 0x67
#define SWZ_XZZY 0x68
#define SWZ_YZZY 0x69
#define SWZ_ZZZY 0x6a
#define SWZ_WZZY 0x6b
#define SWZ_XWZY 0x6c
#define SWZ_YWZY 0x6d
#define SWZ_ZWZY 0x6e
#define SWZ_WWZY 0x6f
#define SWZ_XXWY 0x70
#define SWZ_YXWY 0x71
#define SWZ_ZXWY 0x72
#define SWZ_WXWY 0x73
#define SWZ_XYWY 0x74
#define SWZ_YYWY 0x75
#define SWZ_ZYWY 0x76
#define SWZ_WYWY 0x77
#define SWZ_XZWY 0x78
#define SWZ_YZWY 0x79
#define SWZ_ZZWY 0x7a
#define SWZ_WZWY 0x7b
#define SWZ_XWWY 0x7c
#define SWZ_YWWY 0x7d
#define SWZ_ZWWY 0x7e
#define SWZ_WWWY 0x7f
#define SWZ_XXXZ 0x80
#define SWZ_YXXZ 0x81
#define SWZ_ZXXZ 0x82
#define SWZ_WXXZ 0x83
#define SWZ_XYXZ 0x84
#define SWZ_YYXZ 0x85
#define SWZ_ZYXZ 0x86
#define SWZ_WYXZ 0x87
#define SWZ_XZXZ 0x88
#define SWZ_YZXZ 0x89
#define SWZ_ZZXZ 0x8a
#define SWZ_WZXZ 0x8b
#define SWZ_XWXZ 0x8c
#define SWZ_YWXZ 0x8d
#define SWZ_ZWXZ 0x8e
#define SWZ_WWXZ 0x8f
#define SWZ_XXYZ 0x90
#define SWZ_YXYZ 0x91
#define SWZ_ZXYZ 0x92
#define SWZ_WXYZ 0x93
#define SWZ_XYYZ 0x94
#define SWZ_YYYZ 0x95
#define SWZ_ZYYZ 0x96
#define SWZ_WYYZ 0x97
#define SWZ_XZYZ 0x98
#define SWZ_YZYZ 0x99
#define SWZ_ZZYZ 0x9a
#define SWZ_WZYZ 0x9b
#define SWZ_XWYZ 0x9c
#define SWZ_YWYZ 0x9d
#define SWZ_ZWYZ 0x9e
#define SWZ_WWYZ 0x9f
#define SWZ_XXZZ 0xa0
#define SWZ_YXZZ 0xa1
#define SWZ_ZXZZ 0xa2
#define SWZ_WXZZ 0xa3
#define SWZ_XYZZ 0xa4
#define SWZ_YYZZ 0xa5
#define SWZ_ZYZZ 0xa6
#define SWZ_WYZZ 0xa7
#define SWZ_XZZZ 0xa8
#define SWZ_YZZZ 0xa9
#define SWZ_ZZZZ 0xaa
#define SWZ_WZZZ 0xab
#define SWZ_XWZZ 0xac
#define SWZ_YWZZ 0xad
#define SWZ_ZWZZ 0xae
#define SWZ_WWZZ 0xaf
#define SWZ_XXWZ 0xb0
#define SWZ_YXWZ 0xb1
#define SWZ_ZXWZ 0xb2
#define SWZ_WXWZ 0xb3
#define SWZ_XYWZ 0xb4
#define SWZ_YYWZ 0xb5
#define SWZ_ZYWZ 0xb6
#define SWZ_WYWZ 0xb7
#define SWZ_XZWZ 0xb8
#define SWZ_YZWZ 0xb9
#define SWZ_ZZWZ 0xba
#define SWZ_WZWZ 0xbb
#define SWZ_XWWZ 0xbc
#define SWZ_YWWZ 0xbd
#define SWZ_ZWWZ 0xbe
#define SWZ_WWWZ 0xbf
#define SWZ_XXXW 0xc0
#define SWZ_YXXW 0xc1
#define SWZ_ZXXW 0xc2
#define SWZ_WXXW 0xc3
#define SWZ_XYXW 0xc4
#define SWZ_YYXW 0xc5
#define SWZ_ZYXW 0xc6
#define SWZ_WYXW 0xc7
#define SWZ_XZXW 0xc8
#define SWZ_YZXW 0xc9
#define SWZ_ZZXW 0xca
#define SWZ_WZXW 0xcb
#define SWZ_XWXW 0xcc
#define SWZ_YWXW 0xcd
#define SWZ_ZWXW 0xce
#define SWZ_WWXW 0xcf
#define SWZ_XXYW 0xd0
#define SWZ_YXYW 0xd1
#define SWZ_ZXYW 0xd2
#define SWZ_WXYW 0xd3
#define SWZ_XYYW 0xd4
#define SWZ_YYYW 0xd5
#define SWZ_ZYYW 0xd6
#define SWZ_WYYW 0xd7
#define SWZ_XZYW 0xd8
#define SWZ_YZYW 0xd9
#define SWZ_ZZYW 0xda
#define SWZ_WZYW 0xdb
#define SWZ_XWYW 0xdc
#define SWZ_YWYW 0xdd
#define SWZ_ZWYW 0xde
#define SWZ_WWYW 0xdf
#define SWZ_XXZW 0xe0
#define SWZ_YXZW 0xe1
#define SWZ_ZXZW 0xe2
#define SWZ_WXZW 0xe3
#define SWZ_XYZW 0xe4
#define SWZ_YYZW 0xe5
#define SWZ_ZYZW 0xe6
#define SWZ_WYZW 0xe7
#define SWZ_XZZW 0xe8
#define SWZ_YZZW 0xe9
#define SWZ_ZZZW 0xea
#define SWZ_WZZW 0xeb
#define SWZ_XWZW 0xec
#define SWZ_YWZW 0xed
#define SWZ_ZWZW 0xee
#define SWZ_WWZW 0xef
#define SWZ_XXWW 0xf0
#define SWZ_YXWW 0xf1
#define SWZ_ZXWW 0xf2
#define SWZ_WXWW 0xf3
#define SWZ_XYWW 0xf4
#define SWZ_YYWW 0xf5
#define SWZ_ZYWW 0xf6
#define SWZ_WYWW 0xf7
#define SWZ_XZWW 0xf8
#define SWZ_YZWW 0xf9
#define SWZ_ZZWW 0xfa
#define SWZ_WZWW 0xfb
#define SWZ_XWWW 0xfc
#define SWZ_YWWW 0xfd
#define SWZ_ZWWW 0xfe
#define SWZ_WWWW 0xff
#define sseSwizzle( a, mask ) _mm_shuffle_ps( (a), (a), SWZ_##mask )

	inline __m128 unitQuat() {
		return _mm_set_ps( 1.0f, 0.0f, 0.0f, 0.0f ); // order is reversed
	}
	inline __m128 sseLoadInts( const int vec[4] ) {
		return *(__m128 *)vec;
	}
	inline __m128 mask_0000() {
		static const ALIGNED(16, int vec[4]) = {  0,  0,  0,  0 };
		return sseLoadInts( vec );
	}
	inline __m128 mask_000W() {
		static const ALIGNED(16, int vec[4]) = {  0,  0,  0, -1 };
		return sseLoadInts( vec );
	}
	inline __m128 mask_XYZ0() {
		static const ALIGNED(16, int vec[4]) = { -1, -1, -1,  0 };
		return sseLoadInts( vec );
	}

	inline __m128 sign_000W() {
		static const ALIGNED(16, int vec[4]) = { 0, 0, 0, 1<<31 };
		return sseLoadInts( vec );
	}
	inline __m128 sign_XYZ0() {
		static const ALIGNED(16, int vec[4]) = { 1<<31, 1<<31, 1<<31,  0 };
		return sseLoadInts( vec );
	}
	inline __m128 sign_XYZW() {
		static const ALIGNED(16, int vec[4]) = { 1<<31, 1<<31, 1<<31, 1<<31 };
		return sseLoadInts( vec );
	}

	inline __m128 sseDot4( __m128 a, __m128 b ) {
		__m128 prod = _mm_mul_ps( a, b );
		__m128 sum1 = _mm_add_ps( prod, sseSwizzle( prod, YXWZ ) );
		__m128 sum2 = _mm_add_ps( sum1, sseSwizzle( sum1, ZWXY ) );
		return sum2;
	}
	inline __m128 sseCrossProduct( __m128 a, __m128 b ) {
		__m128 a_yzx = sseSwizzle( a, YZXW );
		__m128 b_yzx = sseSwizzle( b, YZXW );
		__m128 c_zxy = _mm_sub_ps( _mm_mul_ps( a, b_yzx ),
					   _mm_mul_ps( a_yzx, b ) );
		return sseSwizzle( c_zxy, YZXW );
	}
	inline __m128 sseQuatMul( __m128 a, __m128 b ) {
		__m128 a1 = sseSwizzle( a, WWWW );
		__m128 c1 = _mm_mul_ps( a1, b );
		__m128 a2 = sseSwizzle( a, XYZX );
		__m128 b2 = sseSwizzle( b, WWWX );
		__m128 c2 = _mm_xor_ps( _mm_mul_ps(a2, b2), sign_000W() );
		__m128 a3 = sseSwizzle( a, YZXY );
		__m128 b3 = sseSwizzle( b, ZXYY );
		__m128 c3 = _mm_xor_ps( _mm_mul_ps(a3, b3), sign_000W() );
		__m128 a4 = sseSwizzle( a, ZXYZ);
		__m128 b4 = sseSwizzle( b, YZXZ);
		__m128 c4 = _mm_mul_ps( a4, b4 );
		return _mm_add_ps( _mm_add_ps(c1, c2), _mm_sub_ps(c3, c4) );
	}
	inline __m128 sseQuatNormalize( __m128 q ) {
		__m128 p = _mm_mul_ps( q, q );
		__m128 t, h;
		p = _mm_add_ps( sseSwizzle( p, XXZZ ),
				sseSwizzle( p, YYWW ) );
		p = _mm_add_ps( sseSwizzle( p, XXXX ),
				sseSwizzle( p, ZZZZ ) );
		t = _mm_rsqrt_ps( p );
		h = _mm_mul_ps( _mm_set1_ps( 0.5f ), t );
		t = _mm_mul_ps( _mm_mul_ps( t, t ), p );
		t = _mm_sub_ps( _mm_set1_ps( 3.0f ), t );
		t = _mm_mul_ps( h, t );
		return _mm_mul_ps( q, t );
	}
	inline __m128 sseQuatTransform( __m128 q, __m128 vec ) {
		__m128 t, t2;
		t = sseCrossProduct( q, vec );
		t = _mm_add_ps( t, t );
		t2 = sseCrossProduct( q, t );
		t = _mm_mul_ps( sseSwizzle( q, WWWW ), t );
		return _mm_add_ps( _mm_add_ps( vec, t2 ), t );
	}
	inline __m128 sseQuatTransformInverse( __m128 q, __m128 vec ) {
		__m128 t, t2;
		t = sseCrossProduct( vec, q );
		t = _mm_add_ps( t, t );
		t2 = sseCrossProduct( t, q );
		t = _mm_mul_ps( sseSwizzle( q, WWWW ), t );
		return _mm_add_ps( _mm_add_ps( vec, t2 ), t );
	}
	inline __m128 sseLoadVec3( const vec3_t vec ) {
		__m128 v = _mm_load_ss( &vec[ 2 ] );
		v = sseSwizzle( v, YYXY );
		v = _mm_loadl_pi( v, (__m64 *)vec );
		return v;
	}
	inline void sseStoreVec3( __m128 in, vec3_t out ) {
		_mm_storel_pi( (__m64 *)out, in );
		__m128 v = sseSwizzle( in, ZZZZ );
		_mm_store_ss( &out[ 2 ], v );
	}
	inline void TransInit( transform_t *t ) {
		__m128 u = unitQuat();
		t->sseRot = u;
		t->sseTransScale = u;
	}
	inline void TransCopy( const transform_t *in, transform_t *out ) {
		out->sseRot = in->sseRot;
		out->sseTransScale = in->sseTransScale;
	}
	inline void TransformPoint( const transform_t *t,
					   const vec3_t in, vec3_t out ) {
		__m128 ts = t->sseTransScale;
		__m128 tmp = sseQuatTransform( t->sseRot, _mm_loadu_ps( in ) );
		tmp = _mm_mul_ps( tmp, sseSwizzle( ts, WWWW ) );
		tmp = _mm_add_ps( tmp, ts );
		sseStoreVec3( tmp, out );
	}
	inline void TransformPointInverse( const transform_t *t,
						  const vec3_t in, vec3_t out ) {
		__m128 ts = t->sseTransScale;
		__m128 v = _mm_sub_ps( _mm_loadu_ps( in ), ts );
		v = _mm_mul_ps( v, _mm_rcp_ps( sseSwizzle( ts, WWWW ) ) );
		v = sseQuatTransformInverse( t->sseRot, v );
		sseStoreVec3( v, out );
	}
	inline void TransformNormalVector( const transform_t *t,
						  const vec3_t in, vec3_t out ) {
		__m128 v = _mm_loadu_ps( in );
		v = sseQuatTransform( t->sseRot, v );
		sseStoreVec3( v, out );
	}
	inline void TransformNormalVectorInverse( const transform_t *t,
							 const vec3_t in, vec3_t out ) {
		__m128 v = _mm_loadu_ps( in );
		v = sseQuatTransformInverse( t->sseRot, v );
		sseStoreVec3( v, out );
	}
	inline __m128 sseAxisAngleToQuat( const vec3_t axis, float angle ) {
		__m128 sa = _mm_set1_ps( sin( 0.5f * angle ) );
		__m128 ca = _mm_set1_ps( cos( 0.5f * angle ) );
		__m128 a = _mm_loadu_ps( axis );
		a = _mm_and_ps( a, mask_XYZ0() );
		a = _mm_mul_ps( a, sa );
		return _mm_or_ps( a, _mm_and_ps( ca, mask_000W() ) );
	}
	inline void TransInitRotationQuat( const quat_t quat,
						  transform_t *t ) {
		t->sseRot = _mm_loadu_ps( quat );
		t->sseTransScale = unitQuat();
	}
	inline void TransInitRotation( const vec3_t axis, float angle,
					      transform_t *t ) {
		t->sseRot = sseAxisAngleToQuat( axis, angle );
		t->sseTransScale = unitQuat();
	}
	inline void TransInitTranslation( const vec3_t vec, transform_t *t ) {
		__m128 v = _mm_loadu_ps( vec );
		v = _mm_and_ps( v, mask_XYZ0() );
		t->sseRot = unitQuat();
		t->sseTransScale = _mm_or_ps( v, unitQuat() );
	}
	inline void TransInitScale( float factor, transform_t *t ) {
		__m128 f = _mm_set1_ps( factor );
		f = _mm_and_ps( f, mask_000W() );
		t->sseRot = unitQuat();
		t->sseTransScale = f;
	}
	inline void TransInsRotationQuat( const quat_t quat, transform_t *t ) {
		__m128 q = _mm_loadu_ps( quat );
		t->sseRot = sseQuatMul( t->sseRot, q );
	}
	inline void TransInsRotation( const vec3_t axis, float angle,
					     transform_t *t ) {
		__m128 q = sseAxisAngleToQuat( axis, angle );
		t->sseRot = sseQuatMul( q, t->sseRot );
	}
	inline void TransAddRotationQuat( const quat_t quat, transform_t *t ) {
		__m128 q = _mm_loadu_ps( quat );
		__m128 transformed = sseQuatTransform( q, t->sseTransScale );
		t->sseRot = sseQuatMul( q, t->sseRot );
		t->sseTransScale = _mm_or_ps( _mm_and_ps( transformed, mask_XYZ0() ),
					      _mm_and_ps( t->sseTransScale, mask_000W() ) );
	}
	inline void TransAddRotation( const vec3_t axis, float angle,
					     transform_t *t ) {
		__m128 q = sseAxisAngleToQuat( axis, angle );
		__m128 transformed = sseQuatTransform( q, t->sseTransScale );
		t->sseRot = sseQuatMul( t->sseRot, q );
		t->sseTransScale = _mm_or_ps( _mm_and_ps( transformed, mask_XYZ0() ),
					      _mm_and_ps( t->sseTransScale, mask_000W() ) );
	}
	inline void TransInsScale( float factor, transform_t *t ) {
		t->scale *= factor;
	}
	inline void TransAddScale( float factor, transform_t *t ) {
		__m128 f = _mm_set1_ps( factor );
		t->sseTransScale = _mm_mul_ps( f, t->sseTransScale );
	}
	inline void TransInsTranslation( const vec3_t vec,
						transform_t *t ) {
		__m128 v = _mm_loadu_ps( vec );
		__m128 ts = t->sseTransScale;
		v = sseQuatTransform( t->sseRot, v );
		v = _mm_mul_ps( v, sseSwizzle( ts, WWWW ) );
		v = _mm_and_ps( v, mask_XYZ0() );
		t->sseTransScale = _mm_add_ps( ts, v );
	}
	inline void TransAddTranslation( const vec3_t vec,
						transform_t *t ) {
		__m128 v = _mm_loadu_ps( vec );
		v = _mm_and_ps( v, mask_XYZ0() );
		t->sseTransScale = _mm_add_ps( t->sseTransScale, v );
	}
	inline void TransCombine( const transform_t *a,
					 const transform_t *b,
					 transform_t *out ) {
		__m128 aRot = a->sseRot;
		__m128 aTS = a->sseTransScale;
		__m128 bRot = b->sseRot;
		__m128 bTS = b->sseTransScale;
		__m128 tmp = sseQuatTransform( bRot, aTS );
		tmp = _mm_or_ps( _mm_and_ps( tmp, mask_XYZ0() ),
				 _mm_and_ps( aTS, mask_000W() ) );
		tmp = _mm_mul_ps( tmp, sseSwizzle( bTS, WWWW ) );
		out->sseTransScale = _mm_add_ps( tmp, _mm_and_ps( bTS, mask_XYZ0() ) );
		out->sseRot = sseQuatMul( bRot, aRot );
	}
	inline void TransInverse( const transform_t *in,
					 transform_t *out ) {
		__m128 rot = in->sseRot;
		__m128 ts = in->sseTransScale;
		__m128 invS = _mm_rcp_ps( sseSwizzle( ts, WWWW ) );
		__m128 invRot = _mm_xor_ps( rot, sign_XYZ0() );
		__m128 invT = _mm_xor_ps( ts, sign_XYZ0() );
		__m128 tmp = sseQuatTransform( invRot, invT );
		tmp = _mm_mul_ps( tmp, invS );
		out->sseRot = invRot;
		out->sseTransScale = _mm_or_ps( _mm_and_ps( tmp, mask_XYZ0() ),
						_mm_and_ps( invS, mask_000W() ) );
	}
	inline void TransStartLerp( transform_t *t ) {
		t->sseRot = mask_0000();
		t->sseTransScale = mask_0000();
	}
	inline void TransAddWeight( float weight, const transform_t *a,
					   transform_t *out ) {
		__m128 w = _mm_set1_ps( weight );
		__m128 d = sseDot4( a->sseRot, out->sseRot );
		out->sseTransScale = _mm_add_ps( out->sseTransScale,
						 _mm_mul_ps( w, a->sseTransScale ) );
		w = _mm_xor_ps( w, _mm_and_ps( d, sign_XYZW() ) );
		out->sseRot = _mm_add_ps( out->sseRot,
					  _mm_mul_ps( w, a->sseRot ) );
}
	inline void TransEndLerp( transform_t *t ) {
		t->sseRot = sseQuatNormalize( t->sseRot );
	}
#else
	void TransInit( transform_t *t );
	void TransCopy( const transform_t *in, transform_t *out );

	void TransformPoint( const transform_t *t, const vec3_t in, vec3_t out );
	void TransformPointInverse( const transform_t *t, const vec3_t in, vec3_t out );
	void TransformNormalVector( const transform_t *t, const vec3_t in, vec3_t out );
	void TransformNormalVectorInverse( const transform_t *t, const vec3_t in, vec3_t out );

	void TransInitRotationQuat( const quat_t quat, transform_t *t );
	void TransInitRotation( const vec3_t axis, float angle,
				transform_t *t );
	void TransInitTranslation( const vec3_t vec, transform_t *t );
	void TransInitScale( float factor, transform_t *t );

	void TransInsRotationQuat( const quat_t quat, transform_t *t );
	void TransInsRotation( const vec3_t axis, float angle, transform_t *t );
	void TransAddRotationQuat( const quat_t quat, transform_t *t );
	void TransAddRotation( const vec3_t axis, float angle, transform_t *t );
	void TransInsScale( float factor, transform_t *t );
	void TransAddScale( float factor, transform_t *t );
	void TransInsTranslation( const vec3_t vec, transform_t *t );
	void TransAddTranslation( const vec3_t vec, transform_t *t );

	void TransCombine( const transform_t *a, const transform_t *b,
			   transform_t *c );
	void TransInverse( const transform_t *in, transform_t *out );

	void TransStartLerp( transform_t *t );
	void TransAddWeight( float weight, const transform_t *a, transform_t *t );
	void TransEndLerp( transform_t *t );
#endif

//=============================================

	struct growList_t
	{
		bool frameMemory;
		int      currentElements;
		int      maxElements; // will reallocate and move when exceeded
		void     **elements;
	};

// you don't need to init the growlist if you don't mind it growing and moving
// the list as it expands
	void Com_InitGrowList( growList_t *list, int maxElements );
	void Com_DestroyGrowList( growList_t *list );
	int  Com_AddToGrowList( growList_t *list, void *data );
	void *Com_GrowListElement( const growList_t *list, int index );

//=============================================================================

	enum
	{
	  MEMSTREAM_SEEK_SET,
	  MEMSTREAM_SEEK_CUR,
	  MEMSTREAM_SEEK_END
	};

	enum
	{
	  MEMSTREAM_FLAGS_EOF = BIT( 0 ),
	  MEMSTREAM_FLAGS_ERR = BIT( 1 ),
	};

//=============================================

	float      Com_Clamp( float min, float max, float value );

	char       *COM_SkipPath( char *pathname );
	char       *Com_SkipTokens( char *s, int numTokens, const char *sep );
	void       COM_FixPath( char *pathname );
	const char *COM_GetExtension( const char *name );
	void       COM_StripExtension( const char *in, char *out );
	void       COM_StripExtension2( const char *in, char *out, int destsize );
	void       COM_StripExtension3( const char *src, char *dest, int destsize );
	void       COM_DefaultExtension( char *path, int maxSize, const char *extension );

	void       COM_BeginParseSession( const char *name );
	char       *COM_Parse( const char **data_p );

// RB: added COM_Parse2 for having a Doom 3 style tokenizer.
	char       *COM_Parse2( const char **data_p );
	char       *COM_ParseExt2( const char **data_p, bool allowLineBreak );

	char       *COM_ParseExt( const char **data_p, bool allowLineBreak );
	int        COM_Compress( char *data_p );
	void       COM_ParseError( const char *format, ... ) PRINTF_LIKE(1);
	void       COM_ParseWarning( const char *format, ... ) PRINTF_LIKE(1);

	int        Com_ParseInfos( const char *buf, int max, char infos[][ MAX_INFO_STRING ] );

	int        Com_HashKey( char *string, int maxlen );

#define MAX_TOKENLENGTH 1024

#ifndef TT_STRING
//token types
#define TT_STRING      1 // string
#define TT_LITERAL     2 // literal
#define TT_NUMBER      3 // number
#define TT_NAME        4 // name
#define TT_PUNCTUATION 5 // punctuation
#endif

	struct pc_token_t
	{
		int   type;
		int   subtype;
		int   intvalue;
		float floatvalue;
		char  string[ MAX_TOKENLENGTH ];
		int   line;
		int   linescrossed;
	};

// data is an in/out parm, returns a parsed out token

	void      COM_MatchToken( char **buf_p, char *match );

	bool  SkipBracedSection( const char **program );
	bool  SkipBracedSection_Depth( const char **program, int depth );  // start at given depth if already
	void      SkipRestOfLine( const char **data );

	void      Parse1DMatrix( const char **buf_p, int x, float *m );
	void      Parse2DMatrix( const char **buf_p, int y, int x, float *m );
	void      Parse3DMatrix( const char **buf_p, int z, int y, int x, float *m );

	int QDECL Com_sprintf( char *dest, int size, const char *fmt, ... ) PRINTF_LIKE(3);

// mode parm for FS_FOpenFile
	enum fsMode_t
	{
	  FS_READ,
	  FS_WRITE,
	  FS_APPEND,
	  FS_APPEND_SYNC,
	  FS_READ_DIRECT,
	  FS_UPDATE,
	  FS_WRITE_VIA_TEMPORARY,
	};

	enum fsOrigin_t
	{
	  FS_SEEK_CUR,
	  FS_SEEK_END,
	  FS_SEEK_SET
	};

	int        Com_HexStrToInt( const char *str );

	const char *Com_ClearForeignCharacters( const char *str );

//=============================================

    bool Q_strtoi( const char *s, int *outNum );

// portable case insensitive compare
	int        Q_stricmp( const char *s1, const char *s2 );
	int        Q_strncmp( const char *s1, const char *s2, int n );
	int        Q_strnicmp( const char *s1, const char *s2, int n );
	char       *Q_strlwr( char *s1 );
	char       *Q_strupr( char *s1 );
	const char *Q_stristr( const char *s, const char *find );

// buffer size safe library replacements
	void Q_strncpyz( char *dest, const char *src, int destsize );

	void     Q_strcat( char *dest, int destsize, const char *src );

	int      Com_Filter( const char *filter, const char *name, int casesensitive );

// parse "\n" into '\n'
	void     Q_ParseNewlines( char *dest, const char *src, int destsize );

// Count the number of char tocount encountered in string
	int      Q_CountChar( const char *string, char tocount );

//=============================================

	char     *QDECL va( const char *format, ... ) PRINTF_LIKE(1);

//=============================================

//
// key / value info strings
//
	const char *Info_ValueForKey( const char *s, const char *key );
	void       Info_RemoveKey( char *s, const char *key , bool big );
	void       Info_RemoveKey_big( char *s, const char *key );
	void       Info_SetValueForKey( char *s, const char *key, const char *value , bool big );
	void       Info_SetValueForKeyRocket( char *s, const char *key, const char *value, bool big );
	bool   Info_Validate( const char *s );
	void       Info_NextPair( const char **s, char *key, char *value );

//=============================================
/*
 * CVARS (console variables)
 *
 * Many variables can be used for cheating purposes, so when
 * cheats is zero, force all unspecified variables to their
 * default values.
 */

/**
 * set to cause it to be saved to autogen
 * used for system variables, not for player
 * specific configurations
 */
#define CVAR_ARCHIVE             BIT(0)
#define CVAR_USERINFO            BIT(1)    /*< sent to server on connect or change */
#define CVAR_SERVERINFO          BIT(2)    /*< sent in response to front end requests */
#define CVAR_SYSTEMINFO          BIT(3)    /*< these cvars will be duplicated on all clients */

/**
 * don't allow change from console at all,
 * but can be set from the command line
 */
#define CVAR_INIT                BIT(4)

/**
 * will only change when C code next does a Cvar_Get(),
 * so it can't be changed without proper initialization.
 * modified will be set, even though the value hasn't changed yet
 */
#define CVAR_LATCH               BIT(5)
#define CVAR_ROM                 BIT(6)   /*< display only, cannot be set by user at all */
#define CVAR_USER_CREATED        BIT(7)   /*< created by a set command */
#define CVAR_TEMP                BIT(8)   /*< can be set even when cheats are disabled, but is not archived */
#define CVAR_CHEAT               BIT(9)   /*< can not be changed if cheats are disabled */
#define CVAR_NORESTART           BIT(10)  /*< do not clear when a cvar_restart is issued */
#define CVAR_SHADER              BIT(11)  /*< tell renderer to recompile shaders. */

/**
 * unsafe system cvars (renderer, sound settings,
 * anything that might cause a crash)
 */
#define CVAR_UNSAFE              BIT(12)

/**
 * won't automatically be send to clients,
 * but server browsers will see it
 */
#define CVAR_SERVERINFO_NOUPDATE BIT(13)
#define CVAR_USER_ARCHIVE        BIT(14)
#define CVAR_NONEXISTENT         0xFFFFFFFF /*< Cvar doesn't exist. */

#define CVAR_ARCHIVE_BITS        (CVAR_ARCHIVE | CVAR_USER_ARCHIVE)

#define MAX_CVAR_VALUE_STRING 256

	using cvarHandle_t = int;

/**
 * the modules that run in the virtual machine can't access the cvar_t directly,
 * so they must ask for structured updates
 */
	struct vmCvar_t
	{
		cvarHandle_t handle;
		int          modificationCount;
		float        value;
		int          integer;
		char         string[ MAX_CVAR_VALUE_STRING ];
	};

	/*
	==============================================================

	COLLISION DETECTION

	==============================================================
	*/

#include "surfaceflags.h" // shared with the q3map utility

// plane types are used to speed some tests
// 0-2 are axial planes
#define PLANE_X          0
#define PLANE_Y          1
#define PLANE_Z          2
#define PLANE_NON_AXIAL  3
#define PLANE_NON_PLANAR 4

	/*
	=================
	PlaneTypeForNormal
	=================
	*/

#define PlaneTypeForNormal( x ) ( x[ 0 ] == 1.0 ? PLANE_X : ( x[ 1 ] == 1.0 ? PLANE_Y : ( x[ 2 ] == 1.0 ? PLANE_Z : ( x[ 0 ] == 0.f && x[ 1 ] == 0.f && x[ 2 ] == 0.f ? PLANE_NON_PLANAR : PLANE_NON_AXIAL ) ) ) )

// plane_t structure
	struct cplane_t
	{
		vec3_t normal;
		float  dist;
		byte   type; // for fast side tests: 0,1,2 = axial, 3 = nonaxial
		byte   signbits; // signx + (signy<<1) + (signz<<2), used as lookup during collision
		byte   pad[ 2 ];
	};

	enum traceType_t
	{
	  TT_NONE,

	  TT_AABB,
	  TT_CAPSULE,
	  TT_BISPHERE,

	  TT_NUM_TRACE_TYPES
	};

// a trace is returned when a box is swept through the world
	struct trace_t
	{
		bool allsolid; // if true, plane is not valid
		bool startsolid; // if true, the initial point was in a solid area
		float    fraction; // time completed, 1.0 = didn't hit anything
		vec3_t   endpos; // final position
		cplane_t plane; // surface normal at impact, transformed to world space
		int      surfaceFlags; // surface hit
		int      contents; // contents on other side of surface hit
		int      entityNum; // entity the contacted sirface is a part of
		float    lateralFraction; // fraction of collision tangetially to the trace direction
	};

// trace->entityNum can also be 0 to (MAX_GENTITIES-1)
// or ENTITYNUM_NONE, ENTITYNUM_WORLD

// markfragments are returned by CM_MarkFragments()
	struct markFragment_t
	{
		int firstPoint;
		int numPoints;
	};

	struct orientation_t
	{
		vec3_t origin;
		vec3_t axis[ 3 ];
	};

//=====================================================================

// in order from highest priority to lowest
// if none of the catchers are active, bound key strings will be executed
#define KEYCATCH_CONSOLE 0x0001
#define KEYCATCH_UI      0x0002
#define KEYCATCH_MESSAGE 0x0004
#define KEYCATCH_CGAME   0x0008

#define KEYEVSTATE_DOWN 1
#define KEYEVSTATE_CHAR 2
#define KEYEVSTATE_SUP  8

// sound channels
// channel 0 never willingly overrides
// other channels will always override a playing sound on that channel
	enum soundChannel_t
	{
	  CHAN_AUTO,
	  CHAN_LOCAL, // menu sounds, etc
	  CHAN_WEAPON,
	  CHAN_VOICE,
	  CHAN_ITEM,
	  CHAN_BODY,
	  CHAN_LOCAL_SOUND, // chat messages, etc
	  CHAN_ANNOUNCER, // announcer voices, etc
	  CHAN_VOICE_BG, // xkan - background sound for voice (radio static, etc.)
	};

	/*
	========================================================================

	  ELEMENTS COMMUNICATED ACROSS THE NET

	========================================================================
	*/
#define ANIM_BITS 10

#define ANGLE2SHORT( x ) ( (int)( ( x ) * 65536 / 360 ) & 65535 )
#define SHORT2ANGLE( x ) ( ( x ) * ( 360.0 / 65536 ) )

#define SNAPFLAG_RATE_DELAYED 1
#define SNAPFLAG_NOT_ACTIVE   2 // snapshot used during connection and for zombies
#define SNAPFLAG_SERVERCOUNT  4 // toggled every map_restart so transitions can be detected

//
// per-level limits
//
#define MAX_CLIENTS         64 // JPW NERVE back to q3ta default was 128    // absolute limit

#define GENTITYNUM_BITS     10 // JPW NERVE put q3ta default back for testing // don't need to send any more

#define MAX_GENTITIES       ( 1 << GENTITYNUM_BITS )

// entitynums are communicated with GENTITY_BITS, so any reserved
// values thatare going to be communcated over the net need to
// also be in this range
#define ENTITYNUM_NONE           ( MAX_GENTITIES - 1 )
#define ENTITYNUM_WORLD          ( MAX_GENTITIES - 2 )
#define ENTITYNUM_MAX_NORMAL     ( MAX_GENTITIES - 2 )

#define MODELINDEX_BITS          9 // minimum requirement for MAX_SUBMODELS and MAX_MODELS

#define MAX_SUBMODELS            512 // 9 bits sent (see qcommon/msg.c); q3map2 limits to 1024 via MAX_MAP_MODELS; not set to 512 to avoid overlap with fake handles
#define MAX_MODELS               256 // 9 bits sent (see qcommon/msg.c), but limited by game VM
#define MAX_SOUNDS               256 // 8 bits sent (via eventParm; see qcommon/msg.c)
#define MAX_CS_SKINS             64
#define MAX_CSSTRINGS            32

#define MAX_CS_SHADERS           32
#define MAX_SERVER_TAGS          256
#define MAX_TAG_FILES            64

#define MAX_CONFIGSTRINGS        1024

// these are the only configstrings that the system reserves, all the
// other ones are strictly for servergame to clientgame communication
#define CS_SERVERINFO          0 // an info string with all the serverinfo cvars
#define CS_SYSTEMINFO          1 // an info string for server system to client system configuration (timescale, etc)

#define RESERVED_CONFIGSTRINGS 2 // game can't modify below this, only the system can

#define MAX_GAMESTATE_CHARS    16000

using GameStateCSs = std::array<std::string, MAX_CONFIGSTRINGS>;

#define REF_FORCE_DLIGHT       ( 1 << 31 ) // RF, passed in through overdraw parameter, force this dlight under all conditions
#define REF_JUNIOR_DLIGHT      ( 1 << 30 ) // (SA) this dlight does not light surfaces.  it only affects dynamic light grid
#define REF_DIRECTED_DLIGHT    ( 1 << 29 ) // ydnar: global directional light, origin should be interpreted as a normal vector
#define REF_RESTRICT_DLIGHT    ( 1 << 1 ) // dlight is restricted to following entities
#define REF_INVERSE_DLIGHT     ( 1 << 0 ) // inverse dlight for dynamic shadows

// bit field limits
#define MAX_STATS              16
#define MAX_PERSISTANT         16
#define MAX_MISC               16

#define MAX_EVENTS             4 // max events per frame before we drop events

#define PS_PMOVEFRAMECOUNTBITS 6

// playerState_t is the information needed by both the client and server
// to predict player motion and actions
// nothing outside of pmove should modify these, or some degree of prediction error
// will occur

// you can't add anything to this without modifying the code in msg.c
// (Gordon: unless it doesn't need transmission over the network, in which case it should probably go into the new pmext struct anyway)

// playerState_t is a full superset of entityState_t as it is used by players,
// so if a playerState_t is transmitted, the entityState_t can be fully derived
// from it.
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)
	struct playerState_t
	{
		int    commandTime; // cmd->serverTime of last executed command
		int    pm_type;
		int    bobCycle; // for view bobbing and footstep generation
		int    pm_flags; // ducked, jump_held, etc
		int    pm_time;

		vec3_t origin;
		vec3_t velocity;
		int    weaponTime;
		int    gravity;

		int   speed;
		int   delta_angles[ 3 ]; // add to command angles to get view direction
		// changed by spawns, rotating objects, and teleporters

		int groundEntityNum; // ENTITYNUM_NONE = in air

		int legsTimer; // don't change low priority animations until this runs out
		int legsAnim; // mask off ANIM_TOGGLEBIT

		int torsoTimer; // don't change low priority animations until this runs out
		int torsoAnim; // mask off ANIM_TOGGLEBIT

		int movementDir; // a number 0 to 7 that represents the relative angle
		// of movement to the view angle (axial and diagonals)
		// when at rest, the value will remain unchanged
		// used to twist the legs during strafing

		int eFlags; // copied to entityState_t->eFlags

		int eventSequence; // pmove generated events
		int events[ MAX_EVENTS ];
		int eventParms[ MAX_EVENTS ];
		int oldEventSequence; // so we can see which events have been added since we last converted to entityState_t

		int externalEvent; // events set on player from another source
		int externalEventParm;
		int externalEventTime;

		int clientNum; // ranges from 0 to MAX_CLIENTS-1

		// weapon info
		int weapon; // copied to entityState_t->weapon
		int weaponstate;

		vec3_t viewangles; // for fixed views
		int    viewheight;

		// damage feedback
		int damageEvent; // when it changes, latch the other parms
		int damageYaw;
		int damagePitch;
		int damageCount;

		int stats[ MAX_STATS ];
		int persistant[ MAX_PERSISTANT ]; // stats that aren't cleared on death

		// ----------------------------------------------------------------------
		// So to use persistent variables here, which don't need to come from the server,
		// we could use a marker variable, and use that to store everything after it
		// before we read in the new values for the predictedPlayerState, then restore them
		// after copying the structure received from the server.

		// Arnout: use the pmoveExt_t structure in bg_public.h to store this kind of data now (presistant on client, not network transmitted)

		int ping; // server to game info for scoreboard
		int pmove_framecount;
		int entityEventSequence;

		int           generic1;
		int           loopSound;
		int           otherEntityNum;
		vec3_t        grapplePoint; // location of grapple to pull towards if PMF_GRAPPLE_PULL
		int           weaponAnim; // mask off ANIM_TOGGLEBIT
		int           ammo;
		int           clips; // clips held
		int           tauntTimer; // don't allow another taunt until this runs out
		int           misc[ MAX_MISC ]; // misc data
	};

//====================================================================

//
// usercmd_t->button bits, many of which are generated by the client system,
// so they aren't game/cgame only definitions
//

#define USERCMD_BUTTONS     16 // bits allocated for buttons (multiple of 8)

#define BUTTON_ATTACK       0
#define BUTTON_TALK         1  // disables actions
#define BUTTON_USE_HOLDABLE 2
#define BUTTON_GESTURE      3
#define BUTTON_WALKING      4  // walking can't just be inferred from MOVE_RUN
                               // because a key pressed late in the frame will
                               // only generate a small move value for that
                               // frame; walking will use different animations
                               // and won't generate footsteps
#define BUTTON_SPRINT       5
#define BUTTON_ACTIVATE     6
#define BUTTON_ANY          7  // if any key is pressed
#define BUTTON_ATTACK2      8
//                          9
//                          10
//                          11
//                          12
//                          13
#define BUTTON_RALLY        14
#define BUTTON_DODGE        15

#define MOVE_RUN          120 // if forwardmove or rightmove are >= MOVE_RUN,
// then BUTTON_WALKING should be set

// Arnout: doubleTap buttons - DT_NUM can be max 8
	enum dtType_t
	{
	  DT_NONE,
	  DT_MOVELEFT,
	  DT_MOVERIGHT,
	  DT_FORWARD,
	  DT_BACK,
	  DT_UP,
	  DT_NUM
	};

// usercmd_t is sent to the server each client frame
	struct usercmd_t
	{
		int         serverTime;
		int         angles[ 3 ];

		signed char forwardmove, rightmove, upmove;
		byte        doubleTap; // Arnout: only 3 bits used

		byte        weapon;
		byte        flags;

		byte        buttons[ USERCMD_BUTTONS / 8 ];
	};

// Some functions for buttons manipulation & testing
	inline void usercmdPressButton( byte *buttons, int bit )
	{
		buttons[bit / 8] |= 1 << ( bit & 7 );
	}

	inline void usercmdReleaseButton( byte *buttons, int bit )
	{
		buttons[bit / 8] &= ~( 1 << ( bit & 7 ) );
	}

	inline void usercmdClearButtons( byte *buttons )
	{
		memset( buttons, 0, USERCMD_BUTTONS / 8 );
	}

	inline void usercmdCopyButtons( byte *dest, const byte *source )
	{
		memcpy( dest, source, USERCMD_BUTTONS / 8 );
	}

	inline void usercmdLatchButtons( byte *dest, const byte *srcNew, const byte *srcOld )
	{
		int i;
		for ( i = 0; i < USERCMD_BUTTONS / 8; ++i )
		{
			 dest[i] |= srcNew[i] & ~srcOld[i];
		}
	}

	inline bool usercmdButtonPressed( const byte *buttons, int bit )
	{
		return ( buttons[bit / 8] & ( 1 << ( bit & 7 ) ) ) ? true : false;
	}

	inline bool usercmdButtonsDiffer( const byte *a, const byte *b )
	{
		return memcmp( a, b, USERCMD_BUTTONS / 8 ) ? true : false;
	}

//===================================================================

// if entityState->solid == SOLID_BMODEL, modelindex is an inline model number
#define SOLID_BMODEL 0xffffff

	enum trType_t
	{
	  TR_STATIONARY,
	  TR_INTERPOLATE, // non-parametric, but interpolate between snapshots
	  TR_LINEAR,
	  TR_LINEAR_STOP,
	  TR_SINE, // value = base + sin( time / duration ) * delta
	  TR_GRAVITY,
	  TR_BUOYANCY,
	};

	struct trajectory_t
	{
		trType_t trType;
		int      trTime;
		int      trDuration; // if non 0, trTime + trDuration = stop time
//----(SA)  removed
		vec3_t   trBase;
		vec3_t   trDelta; // velocity, etc
//----(SA)  removed
	};

// entityState_t is the information conveyed from the server
// in an update message about entities that the client will
// need to render in some way
// Different eTypes may use the information in different ways
// The messages are delta compressed, so it doesn't really matter if
// the structure size is fairly large
//
// NOTE: all fields in here must be 32 bits (or those within sub-structures)
//
// You can use Com_EntityTypeName to get a String representation of this enum
	enum entityType_t
	{
		ET_GENERAL,
		ET_PLAYER,
		ET_ITEM,

		ET_BUILDABLE,       // buildable type

		ET_LOCATION,

		ET_MISSILE,
		ET_MOVER,
		ET_UNUSED,
		ET_PORTAL,
		ET_SPEAKER,
		ET_PUSHER,
		ET_TELEPORTER,
		ET_INVISIBLE,
		ET_FIRE,

		ET_CORPSE,
		ET_PARTICLE_SYSTEM,
		ET_ANIMMAPOBJ,
		ET_MODELDOOR,
		ET_LIGHTFLARE,
		ET_LEV2_ZAP_CHAIN,

		ET_BEACON,

		ET_EVENTS       // any of the EV_* events can be added freestanding
		// by setting eType to ET_EVENTS + eventNum
		// this avoids having to set eFlags and eventNum
	};

	const char *Com_EntityTypeName(entityType_t entityType);

	struct entityState_t
	{
		int          number; // entity index
		entityType_t eType; // entityType_t
		int          eFlags;

		trajectory_t pos; // for calculating position
		trajectory_t apos; // for calculating angles

		int          time;
		int          time2;

		vec3_t       origin;
		vec3_t       origin2;

		vec3_t       angles;
		vec3_t       angles2;

		int          otherEntityNum; // shotgun sources, etc
		int          otherEntityNum2;

// FIXME: separate field, but doing this for compat reasons
#define otherEntityNum3 groundEntityNum

		int          groundEntityNum; // ENTITYNUM_NONE = in air

		int          constantLight; // r + (g<<8) + (b<<16) + (intensity<<24)
		int          loopSound; // constantly loop this sound

		int          modelindex;
		int          modelindex2;
		int          clientNum; // 0 to (MAX_CLIENTS - 1), for players and corpses
		int          frame;

		int          solid; // for client side prediction, trap_linkentity sets this properly

		// old style events, in for compatibility only
		int event; // impulse events -- muzzle flashes, footsteps, etc
		int eventParm;

		int eventSequence; // pmove generated events
		int events[ MAX_EVENTS ];
		int eventParms[ MAX_EVENTS ];

		// for players
		int weapon; // determines weapon and flash model, etc
		int legsAnim; // mask off ANIM_TOGGLEBIT
		int torsoAnim; // mask off ANIM_TOGGLEBIT

		int           misc; // bit flags
		int           generic1;
		int           weaponAnim; // mask off ANIM_TOGGLEBIT
	};

	enum connstate_t
	{
	  CA_UNINITIALIZED,
	  CA_DISCONNECTED, // not talking to a server
	  CA_CONNECTING, // sending request packets to the server
	  CA_CHALLENGING, // sending challenge packets to the server
	  CA_CONNECTED, // netchan_t established, getting gamestate
	  CA_DOWNLOADING, // downloading a file
	  CA_LOADING, // only during cgame initialization, never during main loop
	  CA_PRIMED, // got gamestate, waiting for first frame
	  CA_ACTIVE, // game views should be displayed
	  CA_CINEMATIC // playing a cinematic or a static pic, not connected to a server
	};

// font support

#define GLYPH_START     0
#define GLYPH_END       255
#define GLYPH_CHARSTART 32
#define GLYPH_CHAREND   127
#define GLYPHS_PER_FONT ( GLYPH_END - GLYPH_START + 1 )
struct glyphInfo_t
{
	int       height; // number of scan lines
	int       top; // top of glyph in buffer
	int       bottom; // bottom of glyph in buffer
	int       pitch; // width for copying
	int       xSkip; // x adjustment
	int       imageWidth; // width of actual image
	int       imageHeight; // height of actual image
	float     s; // x offset in image where glyph starts
	float     t; // y offset in image where glyph starts
	float     s2;
	float     t2;
	qhandle_t glyph; // handle to the shader with the glyph
	char      shaderName[ 32 ];
};

using fontHandle_t = int;

using glyphBlock_t = glyphInfo_t[256];

struct fontInfo_t
{
	void         *face, *faceData, *fallback, *fallbackData;
	glyphInfo_t  *glyphBlock[0x110000 / 256]; // glyphBlock_t
	int           pointSize;
	int           height;
	float         glyphScale;
	char          name[ MAX_QPATH ];
};

struct fontMetrics_t
{
	fontHandle_t  handle;
	bool      isBitmap;
	int           pointSize;
	int           height;
	float         glyphScale;
};

#define Square( x ) ( ( x ) * ( x ) )

// real time
//=============================================

struct qtime_t
{
    int tm_sec; /* seconds after the minute - [0,59] */
    int tm_min; /* minutes after the hour - [0,59] */
    int tm_hour; /* hours since midnight - [0,23] */
    int tm_mday; /* day of the month - [1,31] */
    int tm_mon; /* months since January - [0,11] */
    int tm_year; /* years since 1900 */
    int tm_wday; /* days since Sunday - [0,6] */
    int tm_yday; /* days since January 1 - [0,365] */
    int tm_isdst; /* daylight savings time flag */
};

int        Com_RealTime( qtime_t *qtime );
int        Com_GMTime( qtime_t *qtime );
// Com_Time: client gets local time, server gets GMT
#ifdef BUILD_SERVER
#define Com_Time(t) Com_GMTime(t)
#else
#define Com_Time(t) Com_RealTime(t)
#endif

// server browser sources
#define AS_LOCAL     0
#define AS_GLOBAL    1 // NERVE - SMF - modified
#define AS_FAVORITES 2

// cinematic states
	enum e_status
	{
	  FMV_IDLE,
	  FMV_PLAY, // play
	  FMV_EOF, // all other conditions, i.e. stop/EOF/abort
	  FMV_ID_BLT,
	  FMV_ID_IDLE,
	  FMV_LOOPED,
	  FMV_ID_WAIT
	};

	enum demoState_t
	{
	  DS_NONE,
	  DS_PLAYBACK,
	  DS_RECORDING,
	  DS_NUM_DEMO_STATES
	};

#define MAX_GLOBAL_SERVERS       4096
#define MAX_OTHER_SERVERS        128
#define MAX_PINGREQUESTS         16
#define MAX_SERVERSTATUSREQUESTS 16

#define GENTITYNUM_MASK           ( MAX_GENTITIES - 1 )

#define MAX_EMOTICON_NAME_LEN     16
#define MAX_EMOTICONS             64

#define MAX_OBJECTIVES            64
#define MAX_LOCATIONS             64
#define MAX_MODELS                256 // these are sent over the net as 8 bits
#define MAX_SOUNDS                256 // so they cannot be blindly increased
#define MAX_GAME_SHADERS          64
#define MAX_GRADING_TEXTURES      64
#define MAX_REVERB_EFFECTS        64
#define MAX_GAME_PARTICLE_SYSTEMS 64
#define MAX_HOSTNAME_LENGTH       80
#define MAX_NEWS_STRING           10000

	struct emoticon_t
	{
		char      name[ MAX_EMOTICON_NAME_LEN ];
#ifndef GAME
		int       width;
		qhandle_t shader;
#endif
	};

	struct clientList_t
	{
		uint hi;
		uint lo;
	};

	bool Com_ClientListContains( const clientList_t *list, int clientNum );
	void     Com_ClientListAdd( clientList_t *list, int clientNum );
	void     Com_ClientListRemove( clientList_t *list, int clientNum );
	char     *Com_ClientListString( const clientList_t *list );
	void     Com_ClientListParse( clientList_t *list, const char *s );

	/* This should not be changed because this value is
	* expected to be the same on the client and on the server */
#define RSA_PUBLIC_EXPONENT 65537
#define RSA_KEY_LENGTH      2048
#define RSA_STRING_LENGTH   ( RSA_KEY_LENGTH / 4 + 1 )

#include "common/Common.h"

#endif /* Q_SHARED_H_ */
